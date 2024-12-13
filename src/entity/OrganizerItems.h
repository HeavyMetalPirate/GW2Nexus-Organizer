#ifndef ORGANIZER_ITEMS_H
#define ORGANIZER_ITEMS_H

#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class ItemType : uint8_t {
	DEFAULT = 0,
	DAILY_CRAFTING = 1,
	DAILY_DUNGEON_PATH = 2,
	DAILY_DUNGEON_FULL = 3,
	DAILY_MAP_CHEST = 4,
	DAILY_WIZARD_VAULT = 5,
	DAILY_WORLD_BOSS = 6,
	DAILY_ACHIEVEMENT = 7,
	WEEKLY_WIZARDS_VAULT = 8,
	WEEKLY_RAID_ENCOUNTER = 9,
	WEEKLY_RAID_FULL = 10
};
static const char* ItemTypeCombo[] = {
	"Custom", "Daily Crafting", "Daily Dungeon Path", "Daily Complete Dungeon", "Map Meta", "Daily Vault",
	"World Boss", "Achievement", "Weekly Vault", "Raid Encounter", "Raid Wing"
};
inline ItemType convertToType(uint8_t id) {
	return static_cast<ItemType>(id);
}
inline const char* ItemTypeValue(ItemType type) {
	switch (type) {
	case ItemType::DEFAULT: return "Custom";
	case ItemType::DAILY_CRAFTING: return "Daily Crafting";
	case ItemType::DAILY_DUNGEON_PATH: return "Daily Dungeon Path";
	case ItemType::DAILY_DUNGEON_FULL: return "Daily Complete Dungeon";
	case ItemType::DAILY_MAP_CHEST: return "Map Meta";
	case ItemType::DAILY_WIZARD_VAULT: return "Daily Vault";
	case ItemType::DAILY_WORLD_BOSS: return "World Boss";
	case ItemType::DAILY_ACHIEVEMENT: return "Achievement";
	case ItemType::WEEKLY_WIZARDS_VAULT: return "Weekly Vault";
	case ItemType::WEEKLY_RAID_ENCOUNTER: return "Raid Encounter";
	case ItemType::WEEKLY_RAID_FULL: return "Raid Wing";

	}
	return "Unknown";
}

NLOHMANN_JSON_SERIALIZE_ENUM(ItemType, {
	{ItemType::DEFAULT, "default"},
	{ItemType::DAILY_CRAFTING, "daily_crafting"},
	{ItemType::DAILY_DUNGEON_PATH, "daily_dungeon"},
	{ItemType::DAILY_DUNGEON_FULL, "daily_dungeon_full"},
	{ItemType::DAILY_MAP_CHEST, "daily_mapchest"},
	{ItemType::DAILY_WIZARD_VAULT, "daily_wizardvault"},
	{ItemType::DAILY_WORLD_BOSS, "daily_worldboss"},
	{ItemType::DAILY_ACHIEVEMENT, "daily_achievement"},
	{ItemType::WEEKLY_WIZARDS_VAULT, "weekly_wizardvault"},
	{ItemType::WEEKLY_RAID_ENCOUNTER, "weekly_raid_encounter"},
	{ItemType::WEEKLY_RAID_FULL, "weekly_raid_wing"}

})

enum class RepeatMode : uint8_t {
	ONCE = 0,
	DAILY = 1,
	WEEKLY = 2,
	CUSTOM = 3,
	COUNT = 4
};
static const char* RepeatModeCombo[] = {
	"once", "daily", "weekly", "custom"
};
inline const char* RepeatModeValue(RepeatMode mode) {
	switch (mode) {
	case RepeatMode::ONCE: return "no repeat";
	case RepeatMode::DAILY: return "daily reset";
	case RepeatMode::WEEKLY: return "weekly reset";
	case RepeatMode::CUSTOM: return "custom";
	}
	return "unknown";
}
NLOHMANN_JSON_SERIALIZE_ENUM(RepeatMode, {
	{RepeatMode::ONCE, "once"},
	{RepeatMode::DAILY, "daily"},
	{RepeatMode::WEEKLY, "weekly"},
	{RepeatMode::CUSTOM, "custom"}
})

struct OrganizerItem {
	int id;
	std::string apiId;

	std::string title;
	std::string description;

	ItemType type;
	RepeatMode repeatMode;

	//Custom Repeat Mode
	int intervalMode;
	std::vector<int> daysOfWeek;
	std::vector<int> daysOfMonth;
	int dueHours;
	int dueMinutes;

	std::map<std::string, bool> accountConfiguration;
	std::map<std::string, std::string> accountConfigurationUntil;

	bool deleted;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OrganizerItem, id, apiId, title, description, type, repeatMode, intervalMode, daysOfWeek, daysOfMonth, dueHours, dueMinutes, accountConfiguration, accountConfigurationUntil, deleted);

struct OrganizerItemInstance {
	int id;
	std::string owner; // account
	int itemId;

	std::string startDate;
	std::string endDate;

	bool completed;
	std::string completionDate;
	bool notified;

	bool deleted;

	std::vector<OrganizerItemInstance> childItems;
	int parentId;

	//temp
	bool hasEndDate;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OrganizerItemInstance, id, owner, itemId, startDate, endDate, completed, completionDate, notified, deleted, childItems, parentId);

struct ApiTaskConfigurable {
	OrganizerItem item;
	std::string originalId;
	std::map<std::string, bool> accountConfiguration;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ApiTaskConfigurable, item, originalId, accountConfiguration);

#endif