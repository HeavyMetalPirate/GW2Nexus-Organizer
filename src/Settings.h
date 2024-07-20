#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace addon {
	struct ApiKey {
		std::string apiKey;
		std::string identifier;
	};
	struct Settings {
		std::vector<ApiKey> apiKeys;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ApiKey, apiKey, identifier);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings, apiKeys);
}

#endif