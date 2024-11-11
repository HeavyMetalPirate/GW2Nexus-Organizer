#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <ranges>
#include <sstream>
#include <Windows.h>

inline bool string_replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

inline std::wstring utf8ToWide(const std::string& utf8Str) {
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    std::wstring wideStr(wideSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideSize);
    return wideStr;
}

inline std::string wideToUtf8(const std::wstring& wideStr) {
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8Str(utf8Size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Size, nullptr, nullptr);
    return utf8Str;
}
inline std::string sanitizeDashes(const std::string& input) {
    // Convert from UTF-8 string to wide string
    std::wstring wideInput = utf8ToWide(input);

    // Replace the wide character em dash with the standard dash
    std::wstring emDash = L"—";
    std::wstring standardDash = L"-";
    size_t pos = 0;
    while ((pos = wideInput.find(emDash, pos)) != std::wstring::npos) {
        wideInput.replace(pos, emDash.length(), standardDash);
        pos += standardDash.length();
    }

    // Convert back to UTF-8 string
    return wideToUtf8(wideInput);
}

inline std::string sanitize_string(std::string& str) {
    return sanitizeDashes(str);
}

inline std::string toLower(const std::string& str) {
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
	return lowerStr;
}

inline bool strContains(const std::string& str, const std::string& filter) {
	return toLower(str).find(toLower(filter)) != std::string::npos;
}

inline std::string replaceNewLines(const std::string& str) {
    std::string modified_str;
    for (char ch : str) {
        if (ch == '\n') {
            modified_str += " - ";
        }
        else {
            modified_str += ch;
        }
    }
    return modified_str;
}


inline unsigned int hashString(const std::string& str)
{
    const unsigned int fnv_prime = 16777619u;
    const unsigned int offset_basis = 2166136261u;
    unsigned int hash = offset_basis;

    for (char c : str)
    {
        hash ^= c;
        hash *= fnv_prime;
    }

    return hash;
}

inline std::string maskApiKey(const std::string& apiKey) {
    std::stringstream ss(apiKey);
    std::string segment;
    std::vector<std::string> segments;

    // Split the apiKey by '-'
    while (std::getline(ss, segment, '-')) {
        segments.push_back(segment);
    }

    // Ensure there are at least two segments
    if (segments.size() < 2) {
        return apiKey; // or handle error
    }

    // Construct the masked string
    std::string maskedKey = segments.front() + "-...-" + segments.back();
    return maskedKey;
}

#endif