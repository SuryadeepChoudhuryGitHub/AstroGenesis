#include "net/HttpClient.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <cstdio>
#include <cstdlib>
#include <array>
#include <memory>
#endif

namespace AstroGenesis {

#ifdef _WIN32
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
#endif

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

bool HttpClient::parseUrl(const std::string& url, std::string& outHost, std::string& outPath, unsigned short& outPort, bool& outIsHttps) {
    std::string temp = url;
    outIsHttps = true;
    outPort = 443;

    if (temp.rfind("http://", 0) == 0) {
        outIsHttps = false;
        outPort = 80;
        temp = temp.substr(7);
    } else if (temp.rfind("https://", 0) == 0) {
        outIsHttps = true;
        outPort = 443;
        temp = temp.substr(8);
    }

    size_t slashPos = temp.find('/');
    if (slashPos != std::string::npos) {
        outHost = temp.substr(0, slashPos);
        outPath = temp.substr(slashPos);
    } else {
        outHost = temp;
        outPath = "/";
    }

    size_t colonPos = outHost.find(':');
    if (colonPos != std::string::npos) {
        try {
            outPort = (unsigned short)std::stoi(outHost.substr(colonPos + 1));
        } catch (...) {}
        outHost = outHost.substr(0, colonPos);
    }

    return !outHost.empty();
}

#ifdef _WIN32
HttpResponse HttpClient::executeRequest(const std::string& verb,
                                       const std::string& url,
                                       const std::string& postData,
                                       const std::map<std::string, std::string>& headers,
                                       int timeoutSeconds) {
    HttpResponse response;
    auto startTime = std::chrono::high_resolution_clock::now();

    std::string host, path;
    unsigned short port = 0;
    bool isHttps = true;

    if (!parseUrl(url, host, path, port, isHttps)) {
        response.success = false;
        response.errorMessage = "Failed to parse URL: " + url;
        return response;
    }

    std::wstring wHost = stringToWString(host);
    std::wstring wPath = stringToWString(path);

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

    // Explicitly configure modern TLS protocols (TLS 1.2 & TLS 1.3)
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
    protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), port, 0);
    if (!hConnect) {
        response.success = false;
        response.errorMessage = "Failed to connect to host: " + host;
        WinHttpCloseHandle(hSession);
        return response;
    }

    std::wstring wVerb = stringToWString(verb);
    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wVerb.c_str(), wPath.c_str(),
                                           NULL, WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        response.success = false;
        response.errorMessage = "Failed to open WinHTTP request.";
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }

    if (isHttps) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

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
#else
HttpResponse HttpClient::executeRequest(const std::string& verb,
                                       const std::string& url,
                                       const std::string& postData,
                                       const std::map<std::string, std::string>& headers,
                                       int timeoutSeconds) {
    HttpResponse response;
    auto startTime = std::chrono::high_resolution_clock::now();

    // macOS & Linux built-in curl execution
    std::string cmd = "curl -s -w \"\\n__HTTP_STATUS__%{http_code}\" --max-time " + std::to_string(timeoutSeconds);
    if (verb == "POST") {
        cmd += " -X POST";
        if (!postData.empty()) {
            cmd += " -d " + std::string("\"") + postData + std::string("\"");
        }
    }
    for (const auto& kv : headers) {
        cmd += " -H \"" + kv.first + ": " + kv.second + "\"";
    }
    cmd += " \"" + url + "\"";

    std::array<char, 4096> buffer;
    std::string output;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        response.success = false;
        response.errorMessage = "Failed to execute curl process.";
        return response;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    pclose(pipe);

    // Extract HTTP status code tag
    const std::string tag = "\n__HTTP_STATUS__";
    size_t tagPos = output.rfind(tag);
    if (tagPos != std::string::npos) {
        std::string codeStr = output.substr(tagPos + tag.length());
        try {
            response.statusCode = std::stoi(codeStr);
            response.body = output.substr(0, tagPos);
        } catch (...) {
            response.statusCode = 0;
            response.body = output;
        }
    } else {
        response.body = output;
    }

    response.success = (response.statusCode >= 200 && response.statusCode < 300);
    if (!response.success && response.errorMessage.empty()) {
        response.errorMessage = (response.statusCode > 0) ? ("HTTP Error " + std::to_string(response.statusCode)) : "Network request failed";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    response.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();
    return response;
}
#endif

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
