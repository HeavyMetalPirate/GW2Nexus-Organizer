#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace addon {
	struct Notifications {
		bool enabled;
		int x;
		int y;
		int width;
		int height;
		int duration;
		int direction;
		int minutesUntilDue;
	};
	struct ApiKey {
		std::string apiKey;
		std::string identifier;
	};
	struct Settings {
		std::vector<ApiKey> apiKeys;
		Notifications notifications;
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Notifications, enabled, x, y, width, height, duration, direction, minutesUntilDue);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ApiKey, apiKey, identifier);
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, apiKeys, notifications);

}

#endif