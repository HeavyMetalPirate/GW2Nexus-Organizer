#ifndef GLOBALS_H
#define GLOBALS_H

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#ifndef STRICT
#define STRICT
#endif // !STRICT

#include <string>
#include <ranges>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <ranges>
#include <numeric>
#include <filesystem>
#include <fstream>
#include <thread>
#include <iostream>

#include "OmegaGlobals.h"
#include "resource.h"
#include "utility/LoggerWrapper.h"
#include "utility/DateTime.h"
#include "utility/StringUtils.h"

#include "entity/OrganizerItems.h"
#include "entity/GW2API.h"

namespace fs = std::filesystem;

extern bool organizerRendered;
extern bool todoListRendered;

extern std::string accountName;
extern std::string characterName;

extern bool unloading;
extern bool autotrackActive;

// Structs
struct EvAgentUpdate				// when ev is null
{
	char account[64];		// dst->name	= account name
	char character[64];		// src->name	= character name
	uintptr_t id;			// src->id		= agent id
	uintptr_t instanceId;	// dst->id		= instance id (per map)
	uint32_t added;			// src->prof	= is new agent
	uint32_t target;		// src->elite	= is new targeted agent
	uint32_t Self;			// dst->Self	= is Self
	uint32_t prof;			// dst->prof	= profession / core spec
	uint32_t elite;			// dst->elite	= elite spec
	uint16_t team;			// src->team	= team
	uint16_t subgroup;		// dst->team	= subgroup
};

// Utility

inline std::string getAddonFolder() {
	std::string pathFolder = APIDefs->Paths.GetAddonDirectory(ADDON_NAME);
	// Create folder if not exist
	if (!fs::exists(pathFolder)) {
		try {
			fs::create_directory(pathFolder);
		}
		catch (const std::exception& e) {
			std::string message = "Could not create addon directory: ";
			message.append(pathFolder);
			APIDefs->Log(ELogLevel::ELogLevel_CRITICAL, ADDON_NAME, message.c_str());

			// Suppress the warning for the unused variable 'e'
#pragma warning(suppress: 4101)
			e;
		}
	}
	return pathFolder;
}

inline void LoadSettings() {
	std::string pathData = getAddonFolder() + "/settings.json";
	if (fs::exists(pathData)) {
		std::ifstream dataFile(pathData);

		if (dataFile.is_open()) {
			json jsonData;
			dataFile >> jsonData;
			dataFile.close();
			// parse settings, yay
			settings = jsonData;
		}
	}
}

inline void StoreSettings() {
	json j = settings;

	std::string pathData = getAddonFolder() + "/settings.json";
	std::ofstream outputFile(pathData);
	if (outputFile.is_open()) {
		outputFile << j.dump(4) << std::endl;
		outputFile.close();
	}
	else {
		APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not store settings.json - configuration might get lost between loads.");
	}
}

static const std::string fallbackIdConversion(const std::string& str) {
	std::vector<std::string> junctionWords = { "of", "and", "the", "in", "on", "at", "for", "with", "by", "to", "as" };
	std::stringstream ss(str);
	std::string word, result;

	while (std::getline(ss, word, '_')) {
		if (!word.empty()) {
			std::transform(word.begin(), word.end(), word.begin(), ::tolower);
			if (std::find(junctionWords.begin(), junctionWords.end(), word) == junctionWords.end()) {
				word[0] = toupper(word[0]);
			}
			result += word + " ";
		}
	}
	if (!result.empty()) {
		result.pop_back();
	}

	return result;
}
static const std::string getCraftableName(std::string id) {
	try {
		return dailyCraftablesTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for craftable '" + id + "'.");
		return fallbackIdConversion(id);
	}
}
static const std::string getMetaName(std::string id) {
	try {
		return mapChestsTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for meta '" + id + "'.");
		return fallbackIdConversion(id);
	}
}

static const std::string getWorldBossName(std::string id) {
	try {
		return worldbossesTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for world boss '" + id + "'.");
		return fallbackIdConversion(id);
	}
}
static const std::string getDungeonName(std::string id) {
	try {
		return dungeonTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for dungeon '" + id + "'.");
		return fallbackIdConversion(id);
	}
}
static const std::string getDungeonPathName(std::string id) {
	try {
		return dungeonPathsTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for dungeon path '" + id + "'.");
		return fallbackIdConversion(id);
	}
}
static const std::string getRaidName(std::string id) {
	try {
		return raidTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for raid '" + id + "'.");
		return fallbackIdConversion(id);
	}
}
static const std::string getRaidBossName(std::string id) {
	try {
		return raidBossesTranslator.at(id);
	}
	catch (...) {
		Log(ELogLevel_WARNING, ADDON_NAME, "Could not obtain title for raid boss '" + id + "'.");
		return fallbackIdConversion(id);
	}
}

#endif // GLOBALS_H