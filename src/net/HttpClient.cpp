#include "net/HttpClient.hpp"
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

namespace AstroGenesis {

static std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], sizeNeeded);
    return wstrTo;
}

static std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

HttpClient::HttpClient() {}
HttpClient::~HttpClient() {}

std::string HttpClient::urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << "%20";
        } else {
            escaped << '%' << std::setw(2) << ((int)(unsigned char)c);
        }
    }

    return escaped.str();
}

bool HttpClient::parseUrl(const std::string& url, std::wstring& outHost, std::wstring& outPath, unsigned short& outPort, bool& outIsHttps) {
    std::wstring wUrl = stringToWString(url);
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[512] = {0};
    wchar_t urlPath[4096] = {0};
    wchar_t extraInfo[4096] = {0};

    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(hostName[0]);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(urlPath[0]);
    urlComp.lpszExtraInfo = extraInfo;
    urlComp.dwExtraInfoLength = sizeof(extraInfo) / sizeof(extraInfo[0]);

    if (!WinHttpCrackUrl(wUrl.c_str(), (DWORD)wUrl.length(), 0, &urlComp)) {
        return false;
    }

    outHost = hostName;
    std::wstring fullPath = urlPath;
    if (extraInfo[0] != L'\0') {
        fullPath += extraInfo;
    }
    if (fullPath.empty()) fullPath = L"/";
    outPath = fullPath;
    outPort = urlComp.nPort;
    outIsHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    return true;
}

HttpResponse HttpClient::executeRequest(const std::string& verb,
                                       const std::string& url,
                                       const std::string& postData,
                                       const std::map<std::string, std::string>& headers,
                                       int timeoutSeconds) {
    HttpResponse response;
    auto startTime = std::chrono::high_resolution_clock::now();

    std::wstring host, path;
    unsigned short port = 0;
    bool isHttps = true;

    if (!parseUrl(url, host, path, port, isHttps)) {
        response.success = false;
        response.errorMessage = "Failed to parse URL: " + url;
        return response;
    }

    HINTERNET hSession = WinHttpOpen(L"AstroGenesis/1.0 (Windows NT)",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        response.success = false;
        response.errorMessage = "Failed to open WinHTTP session. Error: " + std::to_string(GetLastError());
        return response;
    }

    // Set timeouts (resolve, connect, send, receive)
    DWORD timeoutMs = (DWORD)(timeoutSeconds * 1000);
    WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) {
        response.success = false;
        response.errorMessage = "Failed to connect to host: " + wstringToString(host);
        WinHttpCloseHandle(hSession);
        return response;
    }

    std::wstring wVerb = stringToWString(verb);
    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wVerb.c_str(), path.c_str(),
                                           NULL, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        response.success = false;
        response.errorMessage = "Failed to open WinHTTP request.";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    // Add custom headers
    std::wstring allHeaders;
    for (const auto& kv : headers) {
        allHeaders += stringToWString(kv.first + ": " + kv.second + "\r\n");
    }
    if (!allHeaders.empty()) {
        WinHttpAddRequestHeaders(hRequest, allHeaders.c_str(), (DWORD)allHeaders.length(), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    // Send Request
    LPVOID pOptional = (postData.empty()) ? WINHTTP_NO_REQUEST_DATA : (LPVOID)postData.c_str();
    DWORD optLength = (DWORD)postData.length();

    BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                      pOptional, optLength, optLength, 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (bResults) {
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
        response.statusCode = (int)statusCode;

        // Read response body in chunks
        DWORD bytesAvailable = 0;
        std::vector<char> buffer;

        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
            std::vector<char> chunk(bytesAvailable);
            DWORD bytesRead = 0;
            if (WinHttpReadData(hRequest, chunk.data(), bytesAvailable, &bytesRead) && bytesRead > 0) {
                buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytesRead);
            } else {
                break;
            }
        }

        response.body.assign(buffer.begin(), buffer.end());
        response.success = (response.statusCode >= 200 && response.statusCode < 300);
        if (!response.success && response.errorMessage.empty()) {
            response.errorMessage = "HTTP Error " + std::to_string(response.statusCode);
        }
    } else {
        DWORD err = GetLastError();
        response.success = false;
        response.errorMessage = "WinHTTP Request failed. Error code: " + std::to_string(err);
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    auto endTime = std::chrono::high_resolution_clock::now();
    response.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    return response;
}

HttpResponse HttpClient::get(const std::string& url, 
                             const std::map<std::string, std::string>& headers,
                             int timeoutSeconds) {
    return executeRequest("GET", url, "", headers, timeoutSeconds);
}

HttpResponse HttpClient::post(const std::string& url, 
                              const std::string& body,
                              const std::map<std::string, std::string>& headers,
                              int timeoutSeconds) {
    return executeRequest("POST", url, body, headers, timeoutSeconds);
}

std::future<HttpResponse> HttpClient::getAsync(const std::string& url,
                                               const std::map<std::string, std::string>& headers,
                                               int timeoutSeconds) {
    return std::async(std::launch::async, [this, url, headers, timeoutSeconds]() {
        return this->get(url, headers, timeoutSeconds);
    });
}

} // namespace AstroGenesis
