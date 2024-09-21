#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

//#pragma comment(lib, "urlmon.lib")

#pragma comment(lib, "winhttp.lib")

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>


#include "../OmegaGlobals.h"

//#include <urlmon.h>
#include <string>
#include <sstream>
#include <format>
#include <regex>

namespace HTTPClient {
    class HttpClient {
    public:
        std::string HttpRequest(const std::wstring& url, const std::wstring& method, const std::wstring& data = L"") {

            std::wstring server, path;
            INTERNET_PORT port;
            bool isHttps;
            ParseUrl(url, server, path, port, isHttps);

            HINTERNET hSession = WinHttpOpen(L"A Simple HTTP Client/1.0",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) {
                return "Error opening session";
            }

            HINTERNET hConnect = WinHttpConnect(hSession, server.c_str(), port, 0);
            if (!hConnect) {
                WinHttpCloseHandle(hSession);
                return "Error connecting to server";
            }

            HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
                NULL, WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                isHttps ? WINHTTP_FLAG_SECURE : 0);
            if (!hRequest) {
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return "Error opening request";
            }

            // Set cache control headers
            LPCWSTR headers = L"Cache-Control: no-cache\r\nPragma: no-cache\r\n";
            if (!WinHttpAddRequestHeaders(hRequest, headers, -1, WINHTTP_ADDREQ_FLAG_ADD)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return "Error adding headers";
            }

            // If POST request, send the data
            if (method == L"POST" && !data.empty()) {
                if (!WinHttpSendRequest(hRequest,
                    WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                    (LPVOID)data.c_str(), static_cast<DWORD>(data.length() * sizeof(wchar_t)),
                    static_cast<DWORD>(data.length() * sizeof(wchar_t)), 0)) {
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    return "Error sending POST data";
                }
            }
            else {
                if (!WinHttpSendRequest(hRequest,
                    WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    return "Error sending GET request";
                }
            }

            if (!WinHttpReceiveResponse(hRequest, NULL)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return "Error receiving response";
            }

            // Read the response
            std::string response = ReadResponse(hRequest);

            // Clean up handles
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            return response;
        }

    private:
        void ParseUrl(const std::wstring& url, std::wstring& server, std::wstring& path, INTERNET_PORT& port, bool& isHttps) {
            URL_COMPONENTS urlComp;
            memset(&urlComp, 0, sizeof(urlComp));
            urlComp.dwStructSize = sizeof(urlComp);

            wchar_t serverName[256];
            wchar_t urlPath[1024];

            urlComp.lpszHostName = serverName;
            urlComp.dwHostNameLength = ARRAYSIZE(serverName);
            urlComp.lpszUrlPath = urlPath;
            urlComp.dwUrlPathLength = ARRAYSIZE(urlPath);

            urlComp.dwSchemeLength = (DWORD)-1;

            WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp);

            server = serverName;
            path = urlPath;
            port = urlComp.nPort;
            isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);
        }

        std::string ReadResponse(HINTERNET hRequest) {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;
            std::string response;

            do {
                // Check the size of available data
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                    break;
                }

                if (dwSize == 0) {
                    break;
                }

                // Allocate buffer for the available data
                char* buffer = new char[dwSize + 1];
                ZeroMemory(buffer, dwSize + 1);

                // Read the data
                if (!WinHttpReadData(hRequest, (LPVOID)buffer, dwSize, &dwDownloaded)) {
                    delete[] buffer;
                    break;
                }

                // Append the data to the response string
                response.append(buffer, dwDownloaded);

                delete[] buffer;
            } while (dwSize > 0);

            return response;
        }
    };

    // Wrapper function to perform a GET request
    static std::string GetRequest(const std::string url) {
#ifndef NDEBUG
        std::regex token_regex(R"((access_token=)[0-9A-Fa-f-]+(-[0-9A-Fa-f]{5}))");
        std::string obfuscated_url = std::regex_replace(url, token_regex, "$1[REDACTED]");
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Requesting URL (GET): " + obfuscated_url).c_str());
#endif

        HttpClient client;
        std::wstring wUrl(url.begin(), url.end()); // Convert std::string to std::wstring
        return client.HttpRequest(wUrl, L"GET");
    }

    // Wrapper function to perform a POST request
    static std::string PostRequest(const std::string url, const std::string postData) {
#ifndef NDEBUG
        std::regex token_regex(R"((access_token=)[0-9A-Fa-f-]+(-[0-9A-Fa-f]{5}))");
        std::string obfuscated_url = std::regex_replace(url, token_regex, "$1[REDACTED]");
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Requesting URL: (POST)" + obfuscated_url).c_str());
#endif

        HttpClient client;
        std::wstring wUrl(url.begin(), url.end()); // Convert std::string to std::wstring
        std::wstring wPostData(postData.begin(), postData.end()); // Convert std::string to std::wstring
        return client.HttpRequest(wUrl, L"POST", wPostData);
    }

    /*
    static std::string GetRequest(std::string url) {
#ifndef NDEBUG
        std::regex token_regex(R"((access_token=)[0-9A-Fa-f-]+(-[0-9A-Fa-f]{5}))");
        std::string obfuscated_url = std::regex_replace(url, token_regex, "$1[REDACTED]");
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Requesting URL: " + obfuscated_url).c_str());
#endif

        const char* cUrl = url.c_str();
        std::wstring wUrl(cUrl, cUrl + strlen(cUrl));
        IStream* stream;

        DWORD flags = BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE; // Flag to disable caching
        HRESULT result = URLOpenBlockingStream(0, wUrl.c_str(), &stream, flags, 0);
        if (result != 0) {
#ifndef NDEBUG
            APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Error Result: " + std::to_string(result)).c_str());
#endif
            return "";
        }

        const unsigned long chunkSize = 128;
        char buffer[chunkSize];
        unsigned long bytesRead;
        std::stringstream strStream;

        stream->Read(buffer, chunkSize, &bytesRead);
        while (bytesRead > 0) {
            strStream.write(buffer, (long long)bytesRead);
            stream->Read(buffer, chunkSize, &bytesRead);
        }
        stream->Release();
        std::string response = strStream.str();
#ifndef NDEBUG
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Got Response: " + response).c_str());
#endif
        return response;
    }
    */
}
#endif