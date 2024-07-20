#ifndef ORGANIZER_REPO_H
#define ORGANIZER_REPO_H

#include "../Globals.h"
#include "DataLoadingFunctions.h"

class OrganizerRepository {
public:
	/// <summary>
	/// Function to initialize the repository from storage
	/// </summary>
	void initialize();

	/// <summary>
	/// Function to save data to storage
	/// </summary>
	void save();

	std::vector<OrganizerItem*> getConfigurableItems() {
		return this->configurableItems;
	}
	std::vector<OrganizerItemInstance*> getTaskInstances() {
		return this->taskInstances;
	}
	std::vector<ApiTaskConfigurable*> getApiTaskConfigurables() {
		return this->apiTaskConfigurables;
	}

	OrganizerItem* getConfigurableItemById(int id);
	ApiTaskConfigurable* getApiTaskConfigurableByOriginalId(std::string originalId);

	void addConfigurableItem(OrganizerItem* item);
	void addApiTaskConfigurable(ApiTaskConfigurable* configurable);
	void addTaskInstance(OrganizerItemInstance* instance);

	void addAccountProgression(std::string progressionId, std::string accountName);
	std::vector<std::string> getAccountProgression(std::string progressionId);

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
	int nextApiTaskConfigurableId = 0;
	int nextConfigurableItemId = 10000; // start high so we have some reserved ones
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
};

#endif