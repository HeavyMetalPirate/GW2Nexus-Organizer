#include "OrganizerRepository.h"

/* Thread Protos */
void InitializeDailyCrafting(OrganizerRepository* repo);
void InitializeWorldbosses(OrganizerRepository* repo);
void InitializeMapchests(OrganizerRepository* repo);
void InitializeDungeons(OrganizerRepository* repo);
void InitializeRaids(OrganizerRepository* repo);
void InitializeAchievements(OrganizerRepository* repo);
void LoadAccountProgress(OrganizerRepository* repo);
void CheckAccountProgressTasks(OrganizerRepository* repo, std::string accountName);

std::map<std::string, std::vector<std::string>> raidEncounters = {};
std::map<std::string, std::vector<std::string>> dungeonPaths = {};

bool autotrackActive = false;

bool OrganizerRepository::firstInitializeDone() {
	if (!accountProgressInitialized) return false;
	if (dungeons.empty()) return false;
	if (raids.empty()) return false;
	if (mapchests.chests.empty()) return false;
	if (worldbosses.worldbosses.empty()) return false;
	if (dailycraft.recipes.empty()) return false;
	if (achievements.empty()) return false;

	return true;
}

void OrganizerRepository::reset() {
	this->accountProgressInitialized = false;
	this->dungeons.clear();
	this->raids.clear();
	this->mapchests.chests.clear();
	this->worldbosses.worldbosses.clear();
	this->dailycraft.recipes.clear();
	this->achievements.clear();

	this->accounts.clear();
	this->progressionPerAccount.clear();

	this->initialize();
}

void OrganizerRepository::initialize() {
	try {
		// Get addon directory
		std::string pathData = getAddonFolder() + "/config_items.json";
		if (fs::exists(pathData)) {
			std::ifstream dataFile(pathData);

			if (dataFile.is_open()) {
				json jsonData;
				dataFile >> jsonData;
				dataFile.close();
				
				std::vector<OrganizerItem> config = jsonData;
				if (!this->configurableItems.empty()) {
					// pre-clear
					this->configurableItems.clear();
				}
				for (auto item : config) {
					auto it = std::find_if(configurableItems.begin(), configurableItems.end(),
						[&item](OrganizerItem* existing) {
							return existing->id == item.id;
						});
					if (it != configurableItems.end()) continue; // item already exists, continue to avoid id collision
					configurableItems.push_back(new OrganizerItem(item));
				}
			}
		}

		pathData = getAddonFolder() + "/organizer.json";
		if (fs::exists(pathData)) {
			std::ifstream dataFile(pathData);

			if (dataFile.is_open()) {
				json jsonData;
				dataFile >> jsonData;
				dataFile.close();

				std::vector<OrganizerItemInstance> config = jsonData;
				if (!this->taskInstances.empty()) {
					// pre-clear
					this->taskInstances.clear();
				}
				for (auto item : config) {
					auto it = std::find_if(taskInstances.begin(), taskInstances.end(),
						[&item](OrganizerItemInstance* existing) {
							return existing->id == item.id;
						});
					if (it != taskInstances.end()) continue; // item already exists, continue to avoid id collision
					taskInstances.push_back(new OrganizerItemInstance(item));
				}
			}
		}

		pathData = getAddonFolder() + "/auto_tracked.json";
		if (fs::exists(pathData)) {
			std::ifstream dataFile(pathData);

			if (dataFile.is_open()) {
				json jsonData;
				dataFile >> jsonData;
				dataFile.close();

				std::vector<ApiTaskConfigurable> config = jsonData;
				if (!this->apiTaskConfigurables.empty()) {
					// pre-clear
					this->apiTaskConfigurables.clear();
				}
				for (auto item : config) {
					auto it = std::find_if(apiTaskConfigurables.begin(), apiTaskConfigurables.end(),
						[&item](ApiTaskConfigurable* existing) {
							return existing->item.id == item.item.id;
						});
					if (it != apiTaskConfigurables.end()) continue; // item already exists, continue to avoid id collision
					apiTaskConfigurables.push_back(new ApiTaskConfigurable(item));
				}
			}
		}
		// InitializerThreads
		std::thread* initDailyCrafting = new std::thread(InitializeDailyCrafting, this);
		threadPool.push_back(initDailyCrafting);
		
		std::thread* initWorldBosses = new std::thread(InitializeWorldbosses, this);
		threadPool.push_back(initWorldBosses);
		
		std::thread* initMapchests = new std::thread(InitializeMapchests, this);
		threadPool.push_back(initMapchests);
		
		std::thread* initRaids = new std::thread(InitializeRaids, this);
		threadPool.push_back(initRaids);

		std::thread* initDungeons = new std::thread(InitializeDungeons, this);
		threadPool.push_back(initDungeons);
		
		std::thread* initAchievements = new std::thread(InitializeAchievements, this);
		threadPool.push_back(initAchievements);
		
		std::thread* loadAccountProg = new std::thread(LoadAccountProgress, this);
		threadPool.push_back(loadAccountProg);

		// Create Task instances for stuff like wizards vault that is more complex than i.e. world bosses
		ApiTaskConfigurable* dailyWV = new ApiTaskConfigurable();
		dailyWV->originalId = "wizardsvault_daily";
		OrganizerItem dailyWVitem = OrganizerItem();
		dailyWVitem.apiId = "wizardsvault_daily";
		dailyWVitem.title = "Daily Wizards Vault";
		dailyWVitem.description = "Complete the daily Wizards Vault meta achievement.";
		dailyWVitem.type = ItemType::DAILY_WIZARD_VAULT;
		dailyWVitem.repeatMode = RepeatMode::DAILY;
		dailyWV->item = dailyWVitem;
		addApiTaskConfigurable(dailyWV);

		ApiTaskConfigurable* weeklyWV = new ApiTaskConfigurable();
		weeklyWV->originalId = "wizardsvault_weekly";
		OrganizerItem weeklyWVitem = OrganizerItem();
		weeklyWVitem.apiId = "wizardsvault_weekly";
		weeklyWVitem.title = "Weekly Wizards Vault";
		weeklyWVitem.description = "Complete the weekly Wizards Vault meta achievement.";
		weeklyWVitem.type = ItemType::WEEKLY_WIZARDS_VAULT;
		weeklyWVitem.repeatMode = RepeatMode::WEEKLY;
		weeklyWV->item = weeklyWVitem;
		addApiTaskConfigurable(weeklyWV);

	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::init(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling OrganizerRepository::initialize()!");
	}
}

void OrganizerRepository::unload() {
	for (auto thread : threadPool) {
		if (thread->joinable()) thread->join();
	}
}

void OrganizerRepository::save() {
	std::lock_guard<std::mutex> guard(saveMutex);
	try {
		std::vector<OrganizerItem> config = std::vector<OrganizerItem>();
		for (auto item : configurableItems) config.push_back(*item);
		json j = config;

		std::string pathData = getAddonFolder() + "/config_items.json";
		std::ofstream outputFile(pathData);
		if (outputFile.is_open()) {
			outputFile << j.dump(4) << std::endl;
			outputFile.close();
		}
		else {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not store config_items.json - configuration might get lost between loads.");
		}

		std::vector<OrganizerItemInstance> tasks = std::vector<OrganizerItemInstance>();
		for (auto item : taskInstances) tasks.push_back(*item);
		j = tasks;

		std::string pathDataOrganizer = getAddonFolder() + "/organizer.json";
		std::ofstream outputFileOrganizer(pathDataOrganizer);
		if (outputFileOrganizer.is_open()) {
			outputFileOrganizer << j.dump(4) << std::endl;
			outputFileOrganizer.close();
		}
		else {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not store organizer.json - configuration might get lost between loads.");
		}

		std::vector<ApiTaskConfigurable> apiConfigurables = std::vector<ApiTaskConfigurable>();
		for (auto item : apiTaskConfigurables) {
			ApiTaskConfigurable configurable = ApiTaskConfigurable(*item);
			apiConfigurables.push_back(configurable);
		}
		j = apiConfigurables;

		std::string pathDataApiTasks = getAddonFolder() + "/auto_tracked.json";
		std::ofstream outputFileApiTasks(pathDataApiTasks);
		if (outputFileApiTasks.is_open()) {
			outputFileApiTasks << j.dump(4) << std::endl;
			outputFileApiTasks.close();
		}
		else {
			APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, "Could not store auto_tracked.json - configuration might get lost between loads.");
		}

		// Possible TODO here: sort task instances by completion date descending, then start date ascending (if completion date is null or equal)
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, e.what());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling OrganizerRepository::save()!");
	}
}

void OrganizerRepository::performCleanup() {
	// plan: in a thread (so we can be as shitty performing as we want to really because vectors are stupid), do:
	// - check each task instance
	//  - if done: check completion date
	//  - if deleted: check creation date
	// - if date difference > settings.retetionthingy, remove from vector
	// - after all instances are done, check all configurations
	//   - check if it still has instances left
	//   - if no instances, delete the category if repeatmode = none
	//   - extra with no solution for now: repeatables without subscribers? users may wanna pause these for a while and keep the configuration around still/
	// - save at the end
	

	std::thread([&] {
		if (settings.retentionPeriod <= 0) return; // 0 = no cleanup configured, and negatives for sanity as well
		auto retentionDate = DateTime::nowLocal().subtractDays(settings.retentionPeriod);

		this->taskInstances.erase(
			std::remove_if(this->taskInstances.begin(), this->taskInstances.end(), [&](OrganizerItemInstance* instance) {
				if (!instance->completed && !instance->deleted) return false; // still active
				try {
					auto referenceDate = instance->completed ? DateTime(instance->completionDate) : DateTime(instance->startDate);

#ifndef NDEBUG
					APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Checking for instance " + std::to_string(instance->id) + " of type " + std::to_string(instance->itemId)).c_str());
					APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Retention Date: " + retentionDate.toStringNice()).c_str());
					APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Reference Date: " + referenceDate.toStringNice()).c_str());
#endif // !_NDEBUG


					if (retentionDate > referenceDate) {
#ifndef NDEBUG
						APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Instance should be deleted!");
#endif
						return true; // delete please
					}
				}
				catch (const std::exception& e) {
					APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, e.what());
				}
				catch (...) {
					APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling OrganizerRepository::performCleanup()!");
				}
				return false;
			}), this->taskInstances.end()
		);

		if (settings.deleteEmptyConfigs) {

			// delete empty configurations
			this->configurableItems.erase(
				std::remove_if(this->configurableItems.begin(), this->configurableItems.end(), [&](OrganizerItem* item) {

#ifndef NDEBUG
					APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Checking ConfigItem with id " + std::to_string(item->id)).c_str());
#endif // !NDEBUG
					if (item->id < organizerItemStartId) return false; // keep the pre defined item ids

					// if still existing instances, no delete please
					if (std::any_of(this->taskInstances.begin(), this->taskInstances.end(), [&](const OrganizerItemInstance* instance) {return item->id == instance->itemId;})) {
#ifndef NDEBUG
						APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Not deleting because instances still available!");
#endif // !NDEBUG
						return false;
					}
			
					// subscription based, delete if no existing instances and no subscribers left
					if (item->repeatMode != RepeatMode::ONCE && settings.deleteUnsubscribedConfigs) {
#ifndef NDEBUG
						APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Subscribers left? " + item->accountConfiguration.empty() ? "No" : "Yes");
#endif // !NDEBUG
						return item->accountConfiguration.empty();
					}
					// single execution based, delete if no existing instances
					else if(item->repeatMode == RepeatMode::ONCE) {
#ifndef NDEBUG
						APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Deleting because RepeatMode = ONCE and no tasks left!");
#endif // !NDEBUG
						return true;
					}
#ifndef NDEBUG
					APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Not deleting because other conditions not met!");
#endif // !NDEBUG
					return false;

				}), this->configurableItems.end()
			);

		
		}

		// store changes
		save();
	}).detach();
}

OrganizerItem* OrganizerRepository::getConfigurableItemById(int id) {
	for (auto item : configurableItems) {
		if (item->id == id) return item;
	}
	// if we get here, scan the auto configurable things, maybe it's there!
	for (auto api : apiTaskConfigurables) {
		if (api->item.id == id) return new OrganizerItem(api->item);
	}
	return nullptr; // YOLO
}

ApiTaskConfigurable* OrganizerRepository::getApiTaskConfigurableByOriginalId(std::string originalId) {
	for (auto configurable : apiTaskConfigurables) {
		if (configurable->originalId == originalId) return configurable;
	}
	return nullptr; // YOLO
}

void OrganizerRepository::addConfigurableItem(OrganizerItem* item) {
	if (item->title.empty()) return; // empty title is not allowed
	if (item->id > 0) return; // already known item, skip adding
	for (auto i : configurableItems) {
		if (i->title == item->title) { // known item by title
			item->id = i->id;
			return;
		}
	}
	item->id = getNextConfiguratbleItemId();
	configurableItems.push_back(item);
	save();
}

void OrganizerRepository::addTaskInstance(OrganizerItemInstance* instance) {
	if (instance->id > 0 || instance->itemId == 0) return; // sanity checks before adding a known or defect item
	instance->id = getNextTaskInstanceId();

	if (!instance->childItems.empty()) {
		for(int i = 0; i < instance->childItems.size(); i++) {
			instance->childItems[i].parentId = instance->id;
			instance->childItems[i].id = i;
		}
	}

	taskInstances.push_back(instance);
	save();
}

void OrganizerRepository::addApiTaskConfigurable(ApiTaskConfigurable* configurable) {
	if (configurable->item.id > 0) return; // already known
	
	// check if we already know this item by its originalId
	// in case we already have that id stored, update the base item
	for (auto c : apiTaskConfigurables) {
		if (c->originalId == configurable->originalId) {
			// Update the base item, then leave it; just update the interesting flags and leave the user
			// configured stuff like item ids, subscriptions etc!
			c->item.title = configurable->item.title;
			c->item.description = configurable->item.description;
			c->item.type = configurable->item.type;
			c->item.repeatMode = configurable->item.repeatMode;
			save();
			return;
		}
	}

	int nextId = getNextApiTaskConfigurableId();
	if (nextId == 0) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Cannot add more API tasks, went over the threshold.");
		return;
	}
	configurable->item.id = nextId;
	apiTaskConfigurables.push_back(configurable);
	save();
}

int OrganizerRepository::getNextConfiguratbleItemId() {
	for (auto item : configurableItems) {
		if (item->id > nextConfigurableItemId) nextConfigurableItemId = item->id;
	}
	return ++nextConfigurableItemId;
}

int OrganizerRepository::getNextTaskInstanceId() {
	for (auto instance : taskInstances) {
		if (instance->id > nextTaskInstanceId) nextTaskInstanceId = instance->id;
	}
	return ++nextTaskInstanceId;
}

int OrganizerRepository::getNextApiTaskConfigurableId() {
	for (auto configurable : apiTaskConfigurables) {
		if (configurable->item.id > nextApiTaskConfigurableId) nextApiTaskConfigurableId = configurable->item.id;
	}

	if (nextApiTaskConfigurableId >= 10000) {
		return 0; // no more allowed!
	}

	return ++nextApiTaskConfigurableId;
}

void OrganizerRepository::addAccountProgression(std::string progressionId, std::string accountName) {
	if (progressionPerAccount.count(progressionId) == 0) {
		progressionPerAccount.emplace(progressionId, std::vector<std::string>());
	}
	auto v = progressionPerAccount.at(progressionId);
	if (std::find(v.begin(), v.end(), accountName) != v.end()) {
		// already contains, so leave
		return;
	}
	progressionPerAccount.at(progressionId).push_back(accountName);
}

std::vector<std::string> OrganizerRepository::getAccountProgression(std::string progressionId) {
	if (progressionPerAccount.count(progressionId) == 0) {
		progressionPerAccount.emplace(progressionId, std::vector<std::string>());
	}
	return progressionPerAccount.at(progressionId);
}

void OrganizerRepository::CompleteTask(OrganizerItemInstance* task) {
	if (task->parentId == 0) {
		// we are dealing with a (possible parent)
		task->completionDate = DateTime::nowLocal().toString();
		task->completed = true;
		// complete children check
		if (task->childItems.size() > 0) {
			for (int i = 0; i < task->childItems.size(); i++) {
				task->childItems[i].completionDate = task->completionDate;
				task->childItems[i].completed = true;
			}
		}
	}
	else {
		// we are dealing with a definite child
		int parentIndex = 0;
		// find parent
		for (auto instance : this->taskInstances) {
			if (instance->id == task->parentId) {
				break;
			}
			parentIndex++;
		}
		// if parent found, we know the child task index == id
		if (parentIndex < this->taskInstances.size()) {
			bool allComplete = true; // do wrong positive search thingy
			this->taskInstances[parentIndex]->childItems[task->id].completed = true;
			this->taskInstances[parentIndex]->childItems[task->id].completionDate = DateTime::nowLocal().toString();

			// check full completion
			for (auto child : this->taskInstances[parentIndex]->childItems) {
				// check for deleted tasks as well
				if (!child.completed && !child.deleted) {
					allComplete = false;
					break;
				}
			}
			if (allComplete) {
				taskInstances[parentIndex]->completionDate = DateTime::nowLocal().toString();
				taskInstances[parentIndex]->completed = true;
			}
		}
	}
 	this->save();
}
void OrganizerRepository::DeleteTask(OrganizerItemInstance* task) {
	if (task->parentId == 0) {
		// we are dealing with a (possible parent)
		task->deleted = true;
		// complete children check
		if (task->childItems.size() > 0) {
			for (int i = 0; i < task->childItems.size(); i++) {
				task->childItems[i].deleted = true;
			}
		}
	}
	else {
		// we are dealing with a definite child
		int parentIndex = 0;
		// find parent
		for (auto instance : this->taskInstances) {
			if (instance->id == task->parentId) {
				break;
			}
			parentIndex++;
		}
		// if parent found, we know the child task index == id
		if (parentIndex < this->taskInstances.size()) {
			bool allDeleted = true; // do wrong positive search thingy
			this->taskInstances[parentIndex]->childItems[task->id].deleted = true;
			
			// check full completion
			for (auto child : this->taskInstances[parentIndex]->childItems) {
				// check for deleted tasks as well
				if (!child.completed && !child.deleted) {
					allDeleted = false;
					break;
				}
			}
			if (allDeleted) {
				taskInstances[parentIndex]->deleted = true;
			}
		}
	}
	this->save();
}
void OrganizerRepository::RestoreTask(OrganizerItemInstance* task) {
	if (task->parentId == 0) {
		// we are dealing with a (possible parent)
		task->deleted = false;
		task->completed = false;
		task->completionDate = "";

		// complete children check
		if (task->childItems.size() > 0) {
			for (int i = 0; i < task->childItems.size(); i++) {
				task->childItems[i].deleted = false;
				task->childItems[i].completed = false;
				task->childItems[i].completionDate = "";
			}
		}
	}
	else {
		// we are dealing with a definite child
		int parentIndex = 0;
		// find parent
		for (auto instance : this->taskInstances) {
			if (instance->id == task->parentId) {
				break;
			}
			parentIndex++;
		}
		// if parent found, we know the child task index == id
		if (parentIndex < this->taskInstances.size()) {
			this->taskInstances[parentIndex]->childItems[task->id].deleted = false;
			this->taskInstances[parentIndex]->childItems[task->id].completed = false;
			this->taskInstances[parentIndex]->childItems[task->id].completionDate = "";
		}
		// Reset also the parent instance in case it was already deleted/completed
		this->taskInstances[parentIndex]->completed = false;
		this->taskInstances[parentIndex]->completionDate = "";
		this->taskInstances[parentIndex]->deleted = false;

	}
	this->save();
}

// Threading helpers
void InitializeDailyCrafting(OrganizerRepository* repo) {
	try {
		std::string url = baseUrl + "/v2/dailycrafting";
		std::string response = HTTPClient::GetRequest(url);
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Daily Crafting API. Certain functionality might not be fully available.");
			return;
		}
		json j = json::parse(response);
		repo->dailycraft.recipes = j.get<std::vector<std::string>>();
		// Add auto configurables
		for (auto recipe : repo->dailycraft.recipes) {
			ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
			configurable->originalId = recipe;
			OrganizerItem item = OrganizerItem();
			item.apiId = recipe;
			item.title = getCraftableName(recipe);
			item.description = "Daily craftable crafted: " + item.title;
			item.type = ItemType::DAILY_CRAFTING;
			item.repeatMode = RepeatMode::DAILY;
			configurable->item = item;
			repo->addApiTaskConfigurable(configurable);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::InitializeDailyCrafting(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Daily Crafting data. Certain functionality might not be fully available.");
	}
}
void InitializeWorldbosses(OrganizerRepository* repo) {
	try {
		std::string url = baseUrl + "/v2/worldbosses";
		std::string response = HTTPClient::GetRequest(url);
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from WorldBosses API. Certain functionality might not be fully available.");
			return;
		}
		json j = json::parse(response);
		repo->worldbosses.worldbosses = j.get<std::vector<std::string>>();

		// Add auto configurables
		for (auto worldboss : repo->worldbosses.worldbosses) {
			ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
			configurable->originalId = worldboss;
			OrganizerItem item = OrganizerItem();
			item.apiId = worldboss;
			item.title = getWorldBossName(worldboss);
			item.description = "World Boss slain: " + item.title;
			item.type = ItemType::DAILY_WORLD_BOSS;
			item.repeatMode = RepeatMode::DAILY;
			configurable->item = item;
			repo->addApiTaskConfigurable(configurable);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::InitializeWorldbosses(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load WorldBosses data. Certain functionality might not be fully available.");
	}
}
void InitializeMapchests(OrganizerRepository* repo) {
	try {
		std::string url = baseUrl + "/v2/mapchests";
		std::string response = HTTPClient::GetRequest(url);
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from MapChests API. Certain functionality might not be fully available.");
			return;
		}
		json j = json::parse(response);
		repo->mapchests.chests = j.get<std::vector<std::string>>();

		// Add auto configurables
		for (auto meta : repo->mapchests.chests) {
			ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
			configurable->originalId = meta;
			OrganizerItem item = OrganizerItem();
			item.apiId = meta;
			item.title = getMetaName(meta);
			item.description = "Map Meta completed: " + item.title;
			item.type = ItemType::DAILY_MAP_CHEST;
			item.repeatMode = RepeatMode::DAILY;
			configurable->item = item;
			repo->addApiTaskConfigurable(configurable);
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::InitializeMapchests(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load MapChests data. Certain functionality might not be fully available.");
	}
}
void InitializeDungeons(OrganizerRepository* repo) {
	try {
		std::string url = baseUrl + "/v2/dungeons?ids=all";
		std::string response = HTTPClient::GetRequest(url);
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Dungeons API. Certain functionality might not be fully available.");
			return;
		}
		json j = json::parse(response);
		repo->dungeons = j.get<std::vector<gw2::dungeon::Dungeon>>();

		// Add auto configurables
		for (auto dungeon : repo->dungeons) {
			ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
			configurable->originalId = dungeon.id;
			OrganizerItem item = OrganizerItem();
			item.apiId = dungeon.id;
			item.title = getDungeonName(dungeon.id);
			item.description = "Full dungeon completed: " + item.title;
			item.type = ItemType::DAILY_DUNGEON_FULL;
			item.repeatMode = RepeatMode::DAILY;
			configurable->item = item;
			repo->addApiTaskConfigurable(configurable);
			dungeonPaths[dungeon.id] = {};

			for (auto path : dungeon.paths) {
				ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
				configurable->originalId = path.id;
				OrganizerItem item = OrganizerItem();
				item.apiId = path.id;
				item.title = getDungeonPathName(path.id);
				item.description = "Dungeon Path completed: " + item.title;
				item.type = ItemType::DAILY_DUNGEON_PATH;
				item.repeatMode = RepeatMode::DAILY;
				configurable->item = item;
				repo->addApiTaskConfigurable(configurable);

				if (path.type == "Explorable") { // ignore story paths because lol
					dungeonPaths[dungeon.id].push_back(path.id);
				}
			}
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::InitializeDungeons(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Dungeons data. Certain functionality might not be fully available.");
	}
}
void InitializeRaids(OrganizerRepository* repo) {
	try {
		std::string url = baseUrl + "/v2/raids?ids=all";
		std::string response = HTTPClient::GetRequest(url);
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Raids API. Certain functionality might not be fully available.");
			return;
		}
		json j = json::parse(response);
		repo->raids = j.get<std::vector<gw2::raid::Raid>>();

		// Add auto configurables
		for (auto raid : repo->raids) {
			for (auto wing : raid.wings) {
				ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
				configurable->originalId = wing.id;
				OrganizerItem item = OrganizerItem();
				item.apiId = wing.id;
				item.title = getRaidName(wing.id);
				item.description = "Raid Wing completed: " + item.title;
				item.type = ItemType::WEEKLY_RAID_FULL;
				item.repeatMode = RepeatMode::WEEKLY;
				configurable->item = item;
				repo->addApiTaskConfigurable(configurable);

				raidEncounters[wing.id] = {};

				for (auto encounter : wing.events) {
					ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
					configurable->originalId = encounter.id;
					OrganizerItem item = OrganizerItem();
					item.apiId = encounter.id;
					item.title = getRaidBossName(encounter.id);
					item.description = "Raid Encounter completed: " + item.title;
					item.type = ItemType::WEEKLY_RAID_ENCOUNTER;
					item.repeatMode = RepeatMode::WEEKLY;
					configurable->item = item;
					repo->addApiTaskConfigurable(configurable);
					raidEncounters[wing.id].push_back(encounter.id);
				}
			}
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::InitializeRaids(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Raids data. Certain functionality might not be fully available.");
	}
}
void InitializeAchievements(OrganizerRepository* repo) {
	try {
		std::string url = baseUrl + "/v2/achievements/categories?ids=88,238,243,250,321,330,261,346,365"; // Potential TODO: more categories, maybe even user defined categories?
		std::string response = HTTPClient::GetRequest(url);
		if (response == "") {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Achievement Category API. Certain functionality might not be fully available.");
			return;
		}
		json j = json::parse(response);
		std::vector<gw2::achievements::AchievementCategory> categories = j.get<std::vector<gw2::achievements::AchievementCategory>>();
		for (auto category : categories) {

			repo->achievements.emplace(category, std::vector<gw2::achievements::Achievement>());

			std::ostringstream oss;
			for (size_t i = 0; i < category.achievements.size(); ++i) {
				oss << category.achievements[i];
				if (i != category.achievements.size() - 1) {
					oss << ",";
				}
			}

			std::string ids = oss.str();
			url = baseUrl + "/v2/achievements?ids=" + ids;
			response = HTTPClient::GetRequest(url);
			if (response == "") {
				APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Empty Response from Achievement Category API. Certain functionality might not be fully available.");
				return;
			}
			json j = json::parse(response);
			std::vector<gw2::achievements::Achievement> achievements = j.get<std::vector<gw2::achievements::Achievement>>();
			for (auto achievement : achievements) {
				repo->achievements[category].push_back(achievement);
			}
			std::sort(repo->achievements[category].begin(), repo->achievements[category].end(), [](const gw2::achievements::Achievement& a, const gw2::achievements::Achievement& b) {
				return a.name < b.name;
			});
		}

		for (auto achievementEntry : repo->achievements) {
			ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
			configurable->originalId = "achievement_group_" + std::to_string(achievementEntry.first.id);
			OrganizerItem item = OrganizerItem();
			item.apiId = std::to_string(achievementEntry.first.id);
			item.title = achievementEntry.first.name;
			item.description = achievementEntry.first.description;
			item.type = ItemType::DAILY_ACHIEVEMENT;
			switch (achievementEntry.first.id) {
			case 261:
			case 346:
			case 365:
				item.repeatMode = RepeatMode::WEEKLY; break;
			default:
				item.repeatMode = RepeatMode::DAILY;
			}
			configurable->item = item;
			repo->addApiTaskConfigurable(configurable);

			for (auto achievement : achievementEntry.second) {
				ApiTaskConfigurable* configurable = new ApiTaskConfigurable();
				configurable->originalId = "achievement_single_" + std::to_string(achievement.id);
				OrganizerItem item = OrganizerItem();
				item.apiId = std::to_string(achievement.id);
				item.title = achievement.name;
				item.description = achievement.requirement;
				item.type = ItemType::DAILY_ACHIEVEMENT;
				switch (achievementEntry.first.id) {
					case ACHIEVEMENT_GROUP_WEEKLY_FRACTALS:
					case ACHIEVEMENT_GROUP_WVW:
					case ACHIEVEMENT_GROUP_RIFTS:
						item.repeatMode = RepeatMode::WEEKLY; break;
					default:
						item.repeatMode = RepeatMode::DAILY;
				}
				configurable->item = item;
				repo->addApiTaskConfigurable(configurable);
			}
		}
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::InitializeAchievements(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Achievements data. Certain functionality might not be fully available.");
	}
}
void LoadAccountProgress(OrganizerRepository* repo) {
	repo->accountProgressInitialized = false;
	// Preload all configured keys to activate account repository
	for (auto key : settings.apiKeys) {
		if (unloading) break;
		try {
			gw2::account::Account account = LoadAccountData(key.apiKey);
			if (account.name.empty()) continue;
			repo->accounts.emplace(account.name, account);
		}
		catch (const std::exception& e) {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgress() during Preload: " + std::string(e.what())).c_str());
		}
		catch (...) {
			APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account progression data during preload. Certain functionality might not be fully available.");
		}
	}

	// At this point, all configured keys have run through first time so set flag
	repo->accountProgressInitialized = true;

	/* Pre-Start Delay to let GW2 API update */
	/* Apparently it is necessary to let the GW2 API reset after game login.
	   Tracking my ominous caching issues I found out that the /account/** endpoints do not reset at server reset but rather will be reset with
	   the next client login. Until this reset is reflected in the API however it takes a while. Bit of testing showed 5+ minutes quite possible,
	   and I wouldn't bet that it won't go beyond that even.

	   So the backup plan really is: let the thread sleep for 10ish minutes on start up, then kick it off with a huzzaaaah!
	 */
	autotrackActive = false;
	APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Delaying API calls to Account data...");
	for (auto i = 0; i < (300000); i++) {
		Sleep(1);
		if (unloading) break;
	}
	autotrackActive = true;
	APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Auto Tracking initialized.");

	while (!unloading) {
		for (auto key : settings.apiKeys) {
			if (unloading) break;
			try {
				gw2::account::Account account = LoadAccountData(key.apiKey);
				if (account.name.empty()) continue;
				repo->accounts.emplace(account.name, account);
				if (unloading) break;

				// Fetch progression for Daily Crafting
				for (auto recipe : LoadDailyCrafting(key.apiKey)) {
					repo->addAccountProgression(recipe, account.name);
				}
				if (unloading) break;

				// Fetch progression for Daily Dungeons
				for (auto dungeon : LoadDailyDungeons(key.apiKey)) {
					repo->addAccountProgression(dungeon, account.name);
				}
				if (unloading) break;

				// Fetch progression for Daily Map Chests
				for (auto chest : LoadDailyMetas(key.apiKey)) {
					repo->addAccountProgression(chest, account.name);
				}
				if (unloading) break;

				// Fetch progression for Daily World Bosses
				for (auto worldboss : LoadWorldBosses(key.apiKey)) {
					repo->addAccountProgression(worldboss, account.name);
				}
				if (unloading) break;

				// Fetch progression for Daily Wizards Vault Daily
				repo->wizardsVaultDaily[account.name] = LoadDailyWizardsVault(key.apiKey);
				if (unloading) break;
				// Fetch progression for Daily Wizards Vault Weekly
				repo->wizardsVaultWeekly[account.name] = LoadWeeklyWizardsVault(key.apiKey);
				if (unloading) break;

				// Fetch progression for Daily Raids
				for (auto raid : LoadRaids(key.apiKey)) {
					repo->addAccountProgression(raid, account.name);
					if (unloading) break;
				}

				// TODO check off in organizer repo if open task instance for the type is available for that account owner
				CheckAccountProgressTasks(repo, account.name);
			}
			catch (const std::exception& e) {
				APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::LoadAccountProgress(): " + std::string(e.what())).c_str());
			}
			catch (...) {
				APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not load Account progression data. Certain functionality might not be fully available.");
			}

		}

		if (unloading) break;
		for (auto i = 0; i < 60000; i++) {
			Sleep(1);
			if (unloading) break;
		}
	}
}

void CheckAccountProgressTasks(OrganizerRepository* repo, std::string accountName) {
	try {
		for (auto task : repo->getTaskInstances()) {
			if (task->completed || task->deleted) continue; // filter out already completed ones
			if (!strContains(task->owner, accountName)) continue; // filter out tasks not belonging to account
			OrganizerItem* item = repo->getConfigurableItemById(task->itemId);
			if (item->deleted) continue; // filter out deleted categories
			if (item->type == ItemType::DEFAULT) continue; // we can't handle default/custom types automatically
			if (item->apiId.empty()) continue; // we need the api id to check against 
			
			if (item->type == ItemType::DAILY_WIZARD_VAULT) {
				// check daily wizards vault data for account
				if (repo->wizardsVaultDaily[accountName].meta_reward_claimed) {
					APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Detected completion of task '" + item->title + "' for account '" + accountName + "'.").c_str());
					task->completed = true;
					task->completionDate = DateTime::nowLocal().toString();
				}
			} 
			else if (item->type == ItemType::WEEKLY_WIZARDS_VAULT) {
				// check weekly wizards vault data for account
				if (repo->wizardsVaultWeekly[accountName].meta_reward_claimed) {
					APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Detected completion of task '" + item->title + "' for account '" + accountName + "'.").c_str());
					task->completed = true;
					task->completionDate = DateTime::nowLocal().toString();
				}
			}
			/*
				TODO: Special checks:
				Suppose we have a "all daily crafting" or "all world bosses" task, then we wanna fetch that from the account progression 
				using known values for that category and check that task off if *all* of them are fullfilled for that account;
				See example below for raid and dungeons

				Another future consideration might be when we allow achievement selections as task configurations (i.e. user wants to track "Achievement X" as TODO by 
				selecting it from a dropdown or input of the ID or whatever works and the API might actually return status of the achievement so we can automatically tick it off)
			*/
			else if (raidEncounters.contains(item->apiId)) {
				bool raidCompleted = true; // we expect our players to be pro raiders!
				for (auto encounter : raidEncounters[item->apiId]) {
					auto accounts = repo->getAccountProgression(encounter);
					auto result = std::ranges::find(accounts, accountName);
					// check if account name is missing
					if (result == accounts.end()) {
						raidCompleted = false;
						break; // oh oh, missing so not complete, can break here
					}
				}
				if (raidCompleted) {
					APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Detected completion of task '" + item->title + "' for account '" + accountName + "'.").c_str());
					task->completed = true;
					task->completionDate = DateTime::nowLocal().toString();
				}
			}
			else if (dungeonPaths.contains(item->apiId)) {
				bool dungeonCompleted = true; // we expect our players to be pro raiders!
				for (auto path : dungeonPaths[item->apiId]) {
					auto accounts = repo->getAccountProgression(path);
					auto result = std::ranges::find(accounts, accountName);
					// check if account name is missing
					if (result == accounts.end()) {
						dungeonCompleted = false;
						break; // oh oh, missing so not complete, can break here
					}
				}
				if (dungeonCompleted) {
					APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Detected completion of task '" + item->title + "' for account '" + accountName + "'.").c_str());
					task->completed = true;
					task->completionDate = DateTime::nowLocal().toString();
				}
			}
			else {
				// check progress by account progression map
				auto accounts = repo->getAccountProgression(item->apiId);
				for (auto account : accounts) {
					if (account == accountName) {
						APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Detected completion of task '" + item->title + "' for account '" + account + "'.").c_str());
						task->completed = true;
						task->completionDate = DateTime::nowLocal().toString();
					}
				}
			}
		}

		repo->save();
	}
	catch (const std::exception& e) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("OrganizerRepository::CheckAccountProgressTasks(): " + std::string(e.what())).c_str());
	}
	catch (...) {
		APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, "Could not check Account progression against task data. Certain functionality might not be fully available.");
	}
}
