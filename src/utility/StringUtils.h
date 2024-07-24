#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <algorithm>

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