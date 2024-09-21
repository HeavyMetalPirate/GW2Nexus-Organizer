#ifndef LOGGER_UTILS_H
#define LOGGER_UTILS_H

#include "../OmegaGlobals.h"

static std::map<std::string, int> loggedMessages = std::map<std::string, int>();

static const void Log(ELogLevel logLevel, const char* addonName, std::string message) {
	if (loggedMessages.contains(message)) {
		loggedMessages[message] = loggedMessages[message] + 1;
		return;
	}
	else {
		loggedMessages.emplace(message, 1);
	}
	APIDefs->Log(logLevel, addonName, message.c_str());
}

#endif
