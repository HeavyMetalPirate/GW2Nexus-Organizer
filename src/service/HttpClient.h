#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#pragma comment(lib, "urlmon.lib")

#include "../OmegaGlobals.h"

#include <urlmon.h>
#include <string>
#include <sstream>
#include <format>
#include <regex>

namespace HTTPClient {
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
}
#endif