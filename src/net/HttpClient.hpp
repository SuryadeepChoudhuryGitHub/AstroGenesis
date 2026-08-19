#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <future>
#include <memory>

namespace AstroGenesis {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string errorMessage;
    bool success = false;
    double elapsedSeconds = 0.0;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Synchronous GET request (recommended for background worker threads)
    HttpResponse get(const std::string& url, 
                     const std::map<std::string, std::string>& headers = {},
                     int timeoutSeconds = 15);

    // Synchronous POST request
    HttpResponse post(const std::string& url, 
                      const std::string& body,
                      const std::map<std::string, std::string>& headers = {},
                      int timeoutSeconds = 15);

    // Asynchronous non-blocking request
    std::future<HttpResponse> getAsync(const std::string& url,
                                       const std::map<std::string, std::string>& headers = {},
                                       int timeoutSeconds = 15);

    // Utility: URL encode string (spaces to %20 or +, special characters)
    static std::string urlEncode(const std::string& value);

    // Utility: Parse URL into host, path, port, isHttps
    static bool parseUrl(const std::string& url, std::wstring& outHost, std::wstring& outPath, unsigned short& outPort, bool& outIsHttps);

private:
    HttpResponse executeRequest(const std::string& verb,
                                const std::string& url,
                                const std::string& postData,
                                const std::map<std::string, std::string>& headers,
                                int timeoutSeconds);
};

} // namespace AstroGenesis
