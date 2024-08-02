#ifndef AUTO_START_SERVICE_H
#define AUTO_START_SERVICE_H

#include "../Globals.h"
#include "SharedServices.h"

class AutoStartService {
public:
	void initialize();
	void startWorker();
	void endWorker();

	void PerformDailyReset();
	void PerformWeeklyReset();

private:
	bool running = false;
	bool initRunning = false;
	std::thread worker;
	std::thread initializer;

	/* Thread functions proto */
	void CheckResetTimes();
	bool CheckDailyReset();
	bool CheckWeeklyReset();
	bool CheckWvWReset(int region); // this one is a big TODO eventually

	void PerformWvWReset();

	void CreateTasksForRepeatMode(RepeatMode mode);
	void CreateTasksForAccount(RepeatMode mode, std::string account);

	void CheckNotifications();

	std::chrono::system_clock::time_point getLastDailyReset() const;
	std::chrono::system_clock::time_point getLastWeeklyReset() const;
	std::chrono::system_clock::time_point getNextDailyReset() const;
	std::chrono::system_clock::time_point getNextWeeklyReset() const;
};

#endif
