#include "AutoStartService.h"

std::tm getUTCTime(std::time_t time) {
    std::tm utc_tm;
    gmtime_s(&utc_tm, &time);
    return utc_tm;
}

void AutoStartService::initialize() {
    initRunning = true;
    initializer = std::thread([&] {
        // Wait a couple for arcdps to give us the account name if possible
        for (int i = 0; i < 15000; i++) {
            Sleep(1);
            if (unloading || !running) return;
        }

        // Wait for organizerRepo to complete init so we have all configured accounts
#ifndef NDEBUG
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "Waiting for OrganizerRepository to complete first init loop...");
#endif
        while (!organizerRepo->firstInitializeDone()) {
            if (!initRunning) return;
            Sleep(10);
        }
#ifndef NDEBUG
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, "OrganizerRepository completed first init loop, starting AutoStartService initializer thread.");
#endif
        PerformDailyReset();
        PerformWeeklyReset();
    });
    //initializer.detach();
}
void AutoStartService::startWorker() {
	running = true;
	worker = std::thread(&AutoStartService::CheckResetTimes, this);
    //worker.detach();
}
void AutoStartService::endWorker() {
	running = false;
    initRunning = false;
	if (worker.joinable()) worker.join();
    if (initializer.joinable()) initializer.join();
}

void AutoStartService::ResetRolloverPause(RepeatMode mode) {
    // Sleep for a while to let the GW2 API roll over
    for (int i = 0; i < 300000; i++) {
        Sleep(1);
        if (unloading || !running) return;
    }
    // save current state just in case
    organizerRepo->save();
    // Reset organizerRepo to load new data in hopefully
    organizerRepo->reset();
    // Sleep for a while so organizerRepo can initialize
    while (!organizerRepo->firstInitializeDone()) {
        Sleep(1);
        if (unloading || !running) return;
    }
}

void AutoStartService::CheckResetTimes() {
    while (running) {
        if (!organizerRepo->firstInitializeDone()) continue;
        
        if (CheckDailyReset()) {
            ResetRolloverPause(RepeatMode::DAILY);
            PerformDailyReset();
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Avoid triggering multiple times within the same minute
        }
        if (CheckWeeklyReset()) {
            ResetRolloverPause(RepeatMode::WEEKLY);
            PerformWeeklyReset();
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Avoid triggering multiple times within the same minute
        }
        if (CheckWvWReset(0)) { // TODO get region param from somewhere like, idk, account?
            PerformWvWReset();
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Avoid triggering multiple times within the same minute
        }
        CheckNotifications();
        for (int i = 0; i < 1000; i++) {
            Sleep(1);
            if (!running) return;
        }
    }
}

void AutoStartService::CheckNotifications() {
    std::string owner = "";

    if (accountName.empty() && characterName.empty()) {
        owner = "Everyone";
    }
    else if (!accountName.empty()) {
        owner = accountName;
    }
    else if (!characterName.empty()) {
        owner = characterName;
    }

    for (auto instance : organizerRepo->getTaskInstances()) {
        if (instance->completed || instance->deleted) continue; // filter complete/deleted instances
        if (instance->endDate.empty()) continue; // filter instances with no due date
        if (instance->notified) continue; // filter already notified tasks
        if (!strContains(instance->owner, owner)) continue; // not owner of task

        try {

            auto dueDate = DateTime(instance->endDate);
            auto now = DateTime::nowLocal();
            int minutes = settings.notifications.minutesUntilDue > 0 ? settings.notifications.minutesUntilDue : 15;
            now.addMinutes(minutes);

            if (dueDate < now) {
                // fire notification pewpew
                toast::ToastData* data = new toast::ToastData();
                data->toastId = "taskNotification_" + std::to_string(instance->id);

                OrganizerItem* item = organizerRepo->getConfigurableItemById(instance->itemId);
                data->title = item->title;
                data->text = "Task due: " + dueDate.toStringNice();
                data->chatLink = "";
                data->texture = nullptr; // possible TODO if I have a cool icon

                notificationService.queueToast(data);
                instance->notified = true;
                organizerRepo->save();
            }
        }
        catch (...) {
            continue;
        }
    }
}

// =================================================================================
// Performers
void AutoStartService::PerformDailyReset() {
    APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Daily Reset triggered.");
    CreateTasksForRepeatMode(RepeatMode::DAILY);
    CreateTasksForRepeatMode(RepeatMode::CUSTOM);
    
}
void AutoStartService::PerformWeeklyReset() {
    APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Weekly Reset triggered.");
    CreateTasksForRepeatMode(RepeatMode::WEEKLY);
}
void AutoStartService::PerformWvWReset() {
    APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "WvW Reset triggered.");
    // Contents TODO, possibly not used at all
}

void AutoStartService::CreateTasksForRepeatMode(RepeatMode mode) {

#ifndef NDEBUG
    APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Available accounts: " + std::to_string(organizerRepo->accounts.size())).c_str());
    APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Available Task configs: " + std::to_string(organizerRepo->getConfigurableItems().size())).c_str());
    APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Available APITask configs: " + std::to_string(organizerRepo->getApiTaskConfigurables().size())).c_str());
#endif

    // fallback if accounts is empty i.e. if no api key is defined
    if (organizerRepo->accounts.empty()) {
        CreateTasksForAccount(mode, accountName.empty() ? "Everyone" : accountName);
    }
    else {
        for (auto account : organizerRepo->accounts) {
            CreateTasksForAccount(mode, account.first);
        }
    }
}

void AutoStartService::CreateTasksForAccount(RepeatMode mode, std::string account) {
    for (auto item : organizerRepo->getConfigurableItems()) {

#ifndef NDEBUG
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Item: " + item->title + " with id " + std::to_string(item->id)).c_str());
        APIDefs->Log(ELogLevel_TRACE, ADDON_NAME, ("Flags: deleted = " + std::to_string(item->deleted) + ", repeatMode = " + std::string(RepeatModeValue(mode))).c_str());
#endif
        if (item->deleted) continue; // task does not technically exist anymore
        if (item->repeatMode != mode) continue; // not the correct mode
        if (account != "Everyone" && !item->accountConfiguration.contains(account)) continue; // no subscription data
        if (account != "Everyone" && !item->accountConfiguration[account]) continue; // not subscribed on that account

        // check subscription until, and if met, set accountConfiguration to false and subUntil empty
        if (item->accountConfigurationUntil.contains(account) && !item->accountConfigurationUntil[account].empty()) {
            auto subUntil = DateTime(item->accountConfigurationUntil[account]);
            if (subUntil.isBeforeNow()) {
                item->accountConfiguration[account] = false;
                item->accountConfigurationUntil[account] = "";
                organizerRepo->save();
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Subscription to Task '" + item->title + "' completed for account " + account + " - skipping creation.").c_str());
                continue;
            }
        }

        if (mode == RepeatMode::CUSTOM) {
            // check if today is *the* day
            if (!item->daysOfWeek.empty()) {
                // check if week days match
                int day = DateTime().GetTodayAsDayOfWeek();
                if (std::count(item->daysOfWeek.begin(), item->daysOfWeek.end(), day) == 0) continue; // Today is *not* the day

            }
            else if(!item->daysOfMonth.empty()) {
                // check if day of month matches
                // TODO special case: ultimo of month
                int day = DateTime().GetTodayAsDayOfMonth();
                if (std::count(item->daysOfMonth.begin(), item->daysOfMonth.end(), day) == 0) continue; // Today is *not* the day
            }
        }

        bool startTask = true;
        // Check all existing instances on whether we may start the task
        for (auto instance : organizerRepo->getTaskInstances()) {
            if (instance->itemId != item->id) continue; // not of this type
            if (!strContains(instance->owner, account)) continue; // not the owner of the instance

            // At this point we know the instance is of the right type
            // for the right owner and isn't a deleted instance, so
            // check if we have an open instance of that type
            if (!instance->completed && !instance->deleted) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Task '" + item->title + "' not completed on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break; // not finished yet on this account
            }

            // If the instance is not open but completed or deleted, check if the begin date was "since last reset"
            auto lastReset = mode == RepeatMode::DAILY || mode == RepeatMode::CUSTOM ? // daily and custom are triggered daily for that day
                                        DateTime::nowLocal().getLastDaily() : 
                                        DateTime::nowLocal().getLastWeekly();
            auto startDate = DateTime(instance->startDate);  
            if ((instance->completed || instance->deleted) && startDate > lastReset) {
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Task '" + item->title + "' already started and completed/deleted since relevant reset on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break;
            }

            if ((instance->completed || instance->deleted) && (instance->endDate.length() > 5 && DateTime(instance->endDate) > lastReset)) {
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Task '" + item->title + "' already completed since relevant reset on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break;
            }
        }
        if (startTask) {
            OrganizerItemInstance newInstance = {};
            newInstance.startDate = DateTime::nowLocal().toString();

            // LOOK MA, NO HANDS DOING SWITCH EXPRESSIONS!
            auto nextReset = [&]() {
                switch (mode) {
                    case RepeatMode::DAILY: return DateTime::nowLocal().getNextDaily();
                    case RepeatMode::WEEKLY: return DateTime::nowLocal().getNextWeekly();
                    case RepeatMode::CUSTOM: return DateTime::fromTodayAt(item->dueHours, item->dueMinutes);
                default: return DateTime::nowLocal();
                }
            }();
            newInstance.endDate = nextReset.toString();
            newInstance.completed = false;
            newInstance.itemId = item->id;
            newInstance.owner = account;

            // Speciality - Task Instance Groups! yaaaay!
            // TODO at a later point, phew.

            organizerRepo->addTaskInstance(new OrganizerItemInstance(newInstance));
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Created task '" + item->title + "' for account " + account).c_str());
        }
    }

    // the same for API Tasks
    for (auto apiTask : organizerRepo->getApiTaskConfigurables()) {
        if (!apiTask->accountConfiguration.contains(account)) continue; // no subscription data
        if (!apiTask->accountConfiguration[account]) continue; // not subscribed
        if (apiTask->item.repeatMode != mode) continue; // wrong repeat mode

        bool startTask = true;
        // Check all existing instances on whether we may start the task
        for (auto instance : organizerRepo->getTaskInstances()) {
            if (instance->itemId != apiTask->item.id) continue; // not of this type
            if (!strContains(instance->owner, account)) continue; // not the owner of the instance

            // At this point we know the instance is of the right type
            // for the right owner and isn't a deleted instance, so
            // check if we have an open instance of that type
            if (!instance->completed && !instance->deleted) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Task '" + apiTask->item.title + "' not completed on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break; // not finished yet on this account
            }

            // If the instance is not open but completed, check if the start date was "since last reset"
            auto lastReset = mode == RepeatMode::DAILY ? DateTime::nowLocal().getLastDaily() : DateTime::nowLocal().getLastWeekly();
            auto startDate = DateTime(instance->startDate);
            
            if ((instance->completed || instance->deleted) && startDate > lastReset) {
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Task '" + apiTask->item.title + "' already started and completed/deleted since relevant reset on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break;
            }

            // If the instance is not open but completed, check if the completion date was "since last reset"
            if ((instance->completed || instance->deleted) && (instance->endDate.length() > 5 && DateTime(instance->endDate) > lastReset)) {
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Task '" + apiTask->item.title + "' already completed since relevant reset on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break;
            }
        }
        if (startTask) {
            OrganizerItemInstance newInstance = {};
            newInstance.startDate = DateTime::nowLocal().toString();
            auto nextReset = mode == RepeatMode::DAILY ? DateTime::nowLocal().getNextDaily() : DateTime::nowLocal().getNextWeekly();
            newInstance.endDate = nextReset.toString(); newInstance.completed = false;
            newInstance.itemId = apiTask->item.id;
            newInstance.owner = account;

            // New Speciality: Task Instance Groups for API Tasks! YAY!
            // We test for:
            // - tasks that start with originalId = achievement_group_ => Achievement Group, start all tasks of that group (TODO filters for fraccies)
            // - dungeon & raids: check for if originalId is contained in their constants translator map; if yes, it's a task group, start all instances!
            if (apiTask->originalId.starts_with("achievement_group_")) {
                // TODO some filtering for some of the categories probably
                // Like, we don't need Tier 1,2,3,4 Fractal for the same fractal dungeon i.e. Thaumanova - "Daily Thaumanova" would be enough
                for (auto achievementGroup : organizerRepo->achievements) {
                    if (std::to_string(achievementGroup.first.id) != apiTask->item.apiId) continue; // take item.apiId because in "originalId" it has a prefix
                    for (auto achievement : achievementGroup.second) {
                        // find the fitting task configuration
                        ApiTaskConfigurable* taskconfig = organizerRepo->getApiTaskConfigurableByOriginalId(std::format("achievement_single_{}",std::to_string(achievement.id)));
                        if (taskconfig) {

                            // omega filtering for tasks like fractals
                            if (achievementGroup.first.id == ACHIEVEMENT_GROUP_DAILY_FRACTALS) {
                                std::string title = taskconfig->item.title;
                                if (!title.starts_with("Daily Tier 1") && !title.starts_with("Daily Recommended Fractal"))
                                    continue;
                            }

                            OrganizerItemInstance subInstance = {};
                            subInstance.startDate = DateTime::nowLocal().toString();
                            subInstance.endDate = nextReset.toString(); newInstance.completed = false;
                            subInstance.itemId = taskconfig->item.id;
                            subInstance.owner = account;
                            newInstance.childItems.push_back(subInstance);
                        }
                    }
                }
            }
            else if (dungeonTranslator.contains(apiTask->originalId)) {
                for (auto dungeon : organizerRepo->dungeons) {
                    if (dungeon.id != apiTask->originalId) continue;
                    for (auto path : dungeon.paths) {
                        ApiTaskConfigurable* taskconfig = organizerRepo->getApiTaskConfigurableByOriginalId(path.id);
                        if (taskconfig) {
                            OrganizerItemInstance subInstance = {};
                            subInstance.startDate = DateTime::nowLocal().toString();
                            subInstance.endDate = nextReset.toString(); newInstance.completed = false;
                            subInstance.itemId = taskconfig->item.id;
                            subInstance.owner = account;
                            newInstance.childItems.push_back(subInstance);
                        }
                    }
                }
            }
            else if (raidTranslator.contains(apiTask->originalId)) {
                for (auto raid : organizerRepo->raids) {
                    for (auto wing : raid.wings) {
                        if (wing.id != apiTask->originalId) continue;
                        for (auto event : wing.events) {
                            ApiTaskConfigurable* taskconfig = organizerRepo->getApiTaskConfigurableByOriginalId(event.id);
                            if (taskconfig) {
                                OrganizerItemInstance subInstance = {};
                                subInstance.startDate = DateTime::nowLocal().toString();
                                subInstance.endDate = nextReset.toString(); newInstance.completed = false;
                                subInstance.itemId = taskconfig->item.id;
                                subInstance.owner = account;
                                newInstance.childItems.push_back(subInstance);
                            }
                        }
                    }
                    
                }
            }

            organizerRepo->addTaskInstance(new OrganizerItemInstance(newInstance));
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Created task '" + apiTask->item.title + "' for account " + account).c_str());
        }
    }
}
/*
std::chrono::system_clock::time_point AutoStartService::getLastDailyReset() const {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto utc_time = floor<days>(now); // Start of the current day (00:00 UTC)

    if (now < utc_time + hours{ 24 }) {
        return utc_time; // Today 00:00 UTC
    }
    else {
        return utc_time - days{ 1 }; // Yesterday 00:00 UTC
    }
}

std::chrono::system_clock::time_point AutoStartService::getLastWeeklyReset() const {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto utc_time = floor<days>(now);
    auto time_t_now = system_clock::to_time_t(utc_time);
    std::tm utc_tm = getUTCTime(time_t_now);

    int days_since_last_monday = (utc_tm.tm_wday + 6) % 7; // Calculate days since last Monday
    auto last_monday = utc_time - days{ days_since_last_monday };

    // 07:30 UTC on last Monday
    auto last_weekly_reset = last_monday + hours{ 7 } + minutes{ 30 };

    if (now < last_weekly_reset + days{ 7 }) {
        return last_weekly_reset; // Last Monday 07:30 UTC
    }
    else {
        return last_weekly_reset - days{ 7 }; // Monday before last 07:30 UTC
    }
}

std::chrono::system_clock::time_point AutoStartService::getNextDailyReset() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto utc_time = floor<days>(now) + days{ 1 }; // Round down to days and add 1 day
    return utc_time;
}

std::chrono::system_clock::time_point AutoStartService::getNextWeeklyReset() const {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto utc_time = floor<days>(now);
    auto time_t_now = system_clock::to_time_t(utc_time);
    std::tm utc_tm = getUTCTime(time_t_now);

    int days_to_next_monday = (8 - utc_tm.tm_wday) % 7; // Calculate days until next Monday
    auto next_monday = utc_time + days{ days_to_next_monday };

    // Add 7 hours and 30 minutes to get 07:30 UTC
    auto next_weekly_reset = next_monday + hours{ 7 } + minutes{ 30 };
    
    // If for some reason calculations gave same day, add a week to fix it
    if (next_weekly_reset < now) {
        next_weekly_reset = next_weekly_reset + days{ 7 };
    }

    return next_weekly_reset;
}
*/

// =================================================================================
// Checkers
bool AutoStartService::CheckDailyReset() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm = getUTCTime(now_c);

    return (utc_tm.tm_hour == 0 && utc_tm.tm_min == 0);
}

bool AutoStartService::CheckWeeklyReset() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm = getUTCTime(now_c);

    return (utc_tm.tm_wday == 1 && utc_tm.tm_hour == 7 && utc_tm.tm_min == 30);
}

bool AutoStartService::CheckWvWReset(int region) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm = getUTCTime(now_c);

    if (region == 1) { // US
        return (utc_tm.tm_wday == 6 && utc_tm.tm_hour == 2 && utc_tm.tm_min == 0); // Saturday 02:00 UTC
    }
    else if (region == 2) { // EU
        return (utc_tm.tm_wday == 5 && utc_tm.tm_hour == 18 && utc_tm.tm_min == 0); // Friday 18:00 UTC
    }
    return false;
}