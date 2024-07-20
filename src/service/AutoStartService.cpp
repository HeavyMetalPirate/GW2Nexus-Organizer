#include "AutoStartService.h"

std::tm getUTCTime(std::time_t time) {
    std::tm utc_tm;
    gmtime_s(&utc_tm, &time);
    return utc_tm;
}

void AutoStartService::initialize() {
    initRunning = true;
    initializer = std::thread([&] {
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
    initializer.detach();
}
void AutoStartService::startWorker() {
	running = true;
	worker = std::thread(&AutoStartService::CheckResetTimes, this);
    worker.detach();
}
void AutoStartService::endWorker() {
	running = false;
    initRunning = false;
	if (worker.joinable()) worker.join();
    if (initializer.joinable()) initializer.join();
}

void AutoStartService::CheckResetTimes() {
    while (running) {
        if (!organizerRepo->firstInitializeDone()) continue;
        if (CheckDailyReset()) {
            PerformDailyReset();
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Avoid triggering multiple times within the same minute
        }
        if (CheckWeeklyReset()) {
            PerformWeeklyReset();
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Avoid triggering multiple times within the same minute
        }
        if (CheckWvWReset(0)) { // TODO get region param from somewhere like, idk, account?
            PerformWvWReset();
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Avoid triggering multiple times within the same minute
        }
        for (int i = 0; i < 1000; i++) {
            Sleep(1);
            if (!running) return;
        }
    }
}


// =================================================================================
// Performers
void AutoStartService::PerformDailyReset() {
    APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Daily Reset triggered.");
    CreateTasksForRepeatMode(RepeatMode::DAILY);
    
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
        if (item->repeatMode != mode) continue; // not daily

        bool startTask = true;
        // Check all existing instances on whether we may start the task
        for (auto instance : organizerRepo->getTaskInstances()) {
            if (instance->itemId != item->id) continue; // not of this type
            if (instance->deleted) continue; // ignore deleted instances anyways
            if (!strContains(instance->owner, account)) continue; // not the owner of the instance

            // At this point we know the instance is of the right type
            // for the right owner and isn't a deleted instance, so
            // check if we have an open instance of that type
            if (!instance->completed) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Task '" + item->title + "' not completed on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break; // not finished yet on this account
            }

            // If the instance is not open but completed, check if the completion date was "since last reset"
            std::chrono::time_point lastReset = mode == RepeatMode::DAILY ? getLastDailyReset() : getLastWeeklyReset();
            if (instance->completed && parse_date(instance->startDate) >= lastReset) {
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Task '" + item->title + "' already completed since relevant reset on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break;
            }
        }
        if (startTask) {
            OrganizerItemInstance newInstance = {};
            newInstance.startDate = format_date(std::chrono::system_clock::now());
            newInstance.endDate = format_date(mode == RepeatMode::DAILY ? getNextDailyReset() : getNextWeeklyReset());
            newInstance.completed = false;
            newInstance.itemId = item->id;
            newInstance.owner = account;
            organizerRepo->addTaskInstance(new OrganizerItemInstance(newInstance));
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Created task '" + item->title + "' for account " + account).c_str());
        }
    }

    // the same for API Tasks
    for (auto apiTask : organizerRepo->getApiTaskConfigurables()) {
        if (!apiTask->active) continue;
        if (apiTask->item.repeatMode != mode) continue;

        bool startTask = true;
        // Check all existing instances on whether we may start the task
        for (auto instance : organizerRepo->getTaskInstances()) {
            if (instance->itemId != apiTask->item.id) continue; // not of this type
            if (instance->deleted) continue; // ignore deleted instances anyways
            if (!strContains(instance->owner, account)) continue; // not the owner of the instance

            // At this point we know the instance is of the right type
            // for the right owner and isn't a deleted instance, so
            // check if we have an open instance of that type
            if (!instance->completed) {
                APIDefs->Log(ELogLevel_WARNING, ADDON_NAME, ("Task '" + apiTask->item.title + "' not completed on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break; // not finished yet on this account
            }

            // If the instance is not open but completed, check if the completion date was "since last reset"
            std::chrono::time_point lastReset = mode == RepeatMode::DAILY ? getLastDailyReset() : getLastWeeklyReset();
            if (instance->completed && parse_date(instance->startDate) >= lastReset) {
                APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Task '" + apiTask->item.title + "' already completed since relevant reset on account " + account + " - skipping creation.").c_str());
                startTask = false;
                break;
            }
        }
        if (startTask) {
            OrganizerItemInstance newInstance = {};
            newInstance.startDate = format_date(std::chrono::system_clock::now());
            newInstance.endDate = format_date(mode == RepeatMode::DAILY ? getNextDailyReset() : getNextWeeklyReset());
            newInstance.completed = false;
            newInstance.itemId = apiTask->item.id;
            newInstance.owner = account;
            organizerRepo->addTaskInstance(new OrganizerItemInstance(newInstance));
            APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Created task '" + apiTask->item.title + "' for account " + account).c_str());
        }
    }
}

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