#ifndef ORGANIZER_REPO_H
#define ORGANIZER_REPO_H

#include "../Globals.h"
#include "DataLoadingFunctions.h"
#include <mutex>

enum class SortProperty : int {
	NAME = 0,
	DESCRIPTION = 1,
	TYPE = 2,
	REPEAT_MODE = 3,
	START_DATE = 4,
	END_DATE = 5,
	COMPLETE_DATE = 6,
	OWNER = 7
};

class OrganizerRepository {
public:
	/// <summary>
	/// Function to initialize the repository from storage
	/// </summary>
	void initialize();
	void unload();
	void reset();

	/// <summary>
	/// Function to save data to storage
	/// </summary>
	void save();

	void performCleanup();

	std::vector<OrganizerItem*> getConfigurableItems() {
		return this->configurableItems;
	}
	std::vector<OrganizerItemInstance*> getTaskInstances() {
		return this->taskInstances;
	}
	std::vector<ApiTaskConfigurable*> getApiTaskConfigurables() {
		return this->apiTaskConfigurables;
	}

	void UpdateSortSpecs(SortProperty property, bool ascending) {
		currentSort = property;
		ascendingSort = ascending;
		auto comparatorInstances = [property, ascending, this](const OrganizerItemInstance* a, const OrganizerItemInstance* b) {
			OrganizerItem* itemA = getConfigurableItemById(a->itemId);
			OrganizerItem* itemB = getConfigurableItemById(b->itemId);

			if (itemA == nullptr) return false;
			if (itemB == nullptr) return false;

			std::string valueA, valueB;
			switch (property) {
			case SortProperty::NAME:
				valueA = toLower(itemA->title);
				valueB = toLower(itemB->title);
				break;
			case SortProperty::DESCRIPTION:
				valueA = toLower(itemA->description);
				valueB = toLower(itemB->description);
				break;
			case SortProperty::TYPE:
				valueA = ItemTypeValue(itemA->type);
				valueB = ItemTypeValue(itemB->type);
				break;
			case SortProperty::REPEAT_MODE:
				valueA = RepeatModeValue(itemA->repeatMode);
				valueB = RepeatModeValue(itemB->repeatMode);
				break;
			case SortProperty::OWNER:
				valueA = toLower(a->owner);
				valueB = toLower(b->owner);
				break;
			case SortProperty::START_DATE:
				valueA = a->startDate;
				valueB = b->startDate;
				break;
			case SortProperty::END_DATE:
				valueA = a->endDate;
				valueB = b->endDate;
				break;
			case SortProperty::COMPLETE_DATE:
				valueA = a->completionDate;
				valueB = b->completionDate;
				break;
			default:
				valueA = toLower(itemA->title);
				valueB = toLower(itemB->title);
			}

			return ascending ? valueA < valueB : valueA > valueB;
			};
		std::sort(this->taskInstances.begin(), this->taskInstances.end(), comparatorInstances);

		auto comparatorItems = [property, ascending, this](const OrganizerItem* itemA, const OrganizerItem* itemB) {
			std::string valueA, valueB;
			switch (property) {
			case SortProperty::NAME:
				valueA = toLower(itemA->title);
				valueB = toLower(itemB->title);
				break;
			case SortProperty::DESCRIPTION:
				valueA = toLower(itemA->description);
				valueB = toLower(itemB->description);
				break;
			case SortProperty::TYPE:
				valueA = ItemTypeValue(itemA->type);
				valueB = ItemTypeValue(itemB->type);
				break;
			case SortProperty::REPEAT_MODE:
				valueA = RepeatModeValue(itemA->repeatMode);
				valueB = RepeatModeValue(itemB->repeatMode);
				break;
			case SortProperty::OWNER:
				//NOOP
				break;
			case SortProperty::START_DATE:
				//NOOP
				break;
			case SortProperty::END_DATE:
				//NOOP
				break;
			default:
				valueA = toLower(itemA->title);
				valueB = toLower(itemB->title);
			}
			return ascending ? valueA < valueB : valueA > valueB;
		};
		std::sort(this->configurableItems.begin(), this->configurableItems.end(), comparatorItems);
	}

	OrganizerItem* getConfigurableItemById(int id);
	ApiTaskConfigurable* getApiTaskConfigurableByOriginalId(std::string originalId);

	void addConfigurableItem(OrganizerItem* item);
	void addApiTaskConfigurable(ApiTaskConfigurable* configurable);
	void addTaskInstance(OrganizerItemInstance* instance);

	void addAccountProgression(std::string progressionId, std::string accountName);
	std::vector<std::string> getAccountProgression(std::string progressionId);

	void CompleteTask(OrganizerItemInstance* task);
	void DeleteTask(OrganizerItemInstance* task);
	void RestoreTask(OrganizerItemInstance* task);

	// Wizards Vault progress
	std::map<std::string, gw2::wizardsvault::MetaProgress> wizardsVaultDaily = std::map<std::string, gw2::wizardsvault::MetaProgress>();
	std::map<std::string, gw2::wizardsvault::MetaProgress> wizardsVaultWeekly = std::map<std::string, gw2::wizardsvault::MetaProgress>();

	// Achievements stuff
	std::map<gw2::achievements::AchievementCategory, std::vector<gw2::achievements::Achievement>> achievements =
		std::map<gw2::achievements::AchievementCategory, std::vector<gw2::achievements::Achievement>>();

	// GW2 API data to check against
	std::vector<gw2::dungeon::Dungeon> dungeons;
	std::vector<gw2::raid::Raid> raids;
	gw2::world::MapChests mapchests;
	gw2::world::WorldBoss worldbosses;
	gw2::crafting::DailyCrafting dailycraft;

	// Accounts found
	std::map<std::string, gw2::account::Account> accounts = std::map<std::string, gw2::account::Account>();

	// Track state
	bool firstInitializeDone();
	bool accountProgressInitialized = false;

private:
	std::mutex saveMutex;

	SortProperty currentSort;
	bool ascendingSort;

	int nextApiTaskConfigurableId = 0;
	int nextConfigurableItemId = organizerItemStartId; // start high so we have some reserved ones
	int nextTaskInstanceId = -1;
	int getNextConfiguratbleItemId();
	int getNextTaskInstanceId();
	int getNextApiTaskConfigurableId();

	// Tasks et al
	std::vector<OrganizerItem*> configurableItems = std::vector<OrganizerItem*>();
	std::vector<ApiTaskConfigurable*> apiTaskConfigurables = std::vector<ApiTaskConfigurable*>();
	std::vector<OrganizerItemInstance*> taskInstances = std::vector<OrganizerItemInstance*>();
		
	std::map<std::string, std::vector<std::string>> progressionPerAccount = std::map<std::string, std::vector<std::string>>();
	
	std::thread initializerThread;

	std::vector<std::thread*> threadPool = std::vector<std::thread*>();
};

#endif