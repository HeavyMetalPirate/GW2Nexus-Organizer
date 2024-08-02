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

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

#include "Constants.h"
#include "Settings.h"
#include "resource.h"
#include "utility/DateTimeUtils.h"
#include "utility/StringUtils.h"

#include "entity/OrganizerItems.h"
#include "entity/GW2API.h"

namespace fs = std::filesystem;

extern AddonDefinition AddonDef;
extern HMODULE hSelf;
extern AddonAPI* APIDefs;
extern Mumble::Data* MumbleLink;
extern NexusLinkData* NexusLink;
extern addon::Settings settings;

extern bool organizerRendered;
extern bool todoListRendered;

extern std::string accountName;
extern std::string characterName;

extern bool unloading;

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

#endif // GLOBALS_H