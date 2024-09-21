#ifndef DATETIME_H
#define DATETIME_H

#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <mutex>

static std::mutex mutex_;

class DateTime {
public:
    // Constructors
    DateTime() {
        timePoint_ = std::time(nullptr);
        timeZoneOffset_ = detectSystemTimeZoneOffset();
    }

    DateTime(const std::string& timeStr) {
        parseFromString(timeStr);
    }

    DateTime(std::time_t tp, int tzOffset = 0) : timePoint_(tp), timeZoneOffset_(tzOffset) {}

    // Parse from string in the format "2024-08-03T02:00:00+0200"
    void parseFromString(const std::string& timeStr) {
        std::tm tm = {};
        std::string timePart = timeStr.substr(0, 19);  // "2024-08-03T02:00:00"
        std::string tzOffsetStr = timeStr.substr(19);  // "+0200"

        std::istringstream ss(timePart);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            throw std::invalid_argument("Failed to parse time string");
        }

        // Convert to time_t assuming UTC
        tm.tm_isdst = -1;  // This tells mktime to determine DST
        timePoint_ = std::mktime(&tm);

        // Parse the timezone offset and adjust the timePoint_ correctly
        parseTimezoneOffset(tzOffsetStr);
        timePoint_ += timeZoneOffset_;  // Adjust to the correct UTC time based on the given time zone
    }

    // Convert to string with the correct timezone information
    std::string toString() const {
        return formatTime("%Y-%m-%dT%H:%M:%S") + formatTimezoneOffset();
    }

    // Get current time in local time zone
    static DateTime nowLocal() {
        DateTime dt;
        dt.timeZoneOffset_ = detectSystemTimeZoneOffset();
        return dt;
    }

    // Create a DateTime representing today at a specific hour and minute (local time)
    static DateTime fromTodayAt(int hour, int minute) {
        std::tm tm = {};
        DateTime result;

        // Get the current local time
        std::time_t now = std::time(nullptr);
        localtimeSafe(&tm, &now);

        // Set the hour and minute while keeping the current day
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = 0;  // Seconds are set to 0

        // Convert back to time_t (local time)
        result.timePoint_ = std::mktime(&tm);

        // Set the timezone offset (detected from the system)
        result.timeZoneOffset_ = detectSystemTimeZoneOffset();

        return result;
    }

    // Thread-safe comparison with the current system time
    bool isBeforeNow() const {
        return timePoint_ < std::time(nullptr);
    }

    bool isAfterNow() const {
        return timePoint_ > std::time(nullptr);
    }

    bool isEqualNow() const {
        return timePoint_ == std::time(nullptr);
    }

    // Thread-safe comparison between two DateTime objects
    bool operator<(const DateTime& other) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timePoint_ < other.timePoint_;
    }

    bool operator>(const DateTime& other) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timePoint_ > other.timePoint_;
    }

    bool operator==(const DateTime& other) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timePoint_ == other.timePoint_;
    }

    // Get time zone information
    std::string getTimeZone() const {
        std::tm local_tm = {};
        localtimeSafe(&local_tm, &timePoint_);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Z", &local_tm);
        return std::string(buf);
    }

    // Special format for UI: "YYYY-MM-DD HH:mm:ss"
    std::string toStringNice() const {
        return formatTime("%Y-%m-%d %H:%M:%S");
    }

    // Special format for UI with newline: "YYYY-MM-DD\nHH:mm:ss"
    std::string toStringNiceNewline() const {
        std::string date = formatTime("%Y-%m-%d");
        std::string time = formatTime("%H:%M:%S");
        return date + "\n" + time;
    }

    // Add or subtract days
    DateTime& addDays(int days) {
        std::lock_guard<std::mutex> lock(mutex_);
        timePoint_ += days * 86400;  // 86400 seconds in a day
        return *this;
    }

    DateTime& subtractDays(int days) {
        return addDays(-days);
    }

    // Add or subtract hours
    DateTime& addHours(int hours) {
        std::lock_guard<std::mutex> lock(mutex_);
        timePoint_ += hours * 3600;  // 3600 seconds in an hour
        return *this;
    }

    DateTime& subtractHours(int hours) {
        return addHours(-hours);
    }

    // Add or subtract minutes
    DateTime& addMinutes(int minutes) {
        std::lock_guard<std::mutex> lock(mutex_);
        timePoint_ += minutes * 60;  // 60 seconds in a minute
        return *this;
    }

    DateTime& subtractMinutes(int minutes) {
        return addMinutes(-minutes);
    }

    // Get the previous UTC 00:00 (midnight)
    DateTime getLastDaily() const {
        DateTime result = *this;
        result.toUTC();

        std::tm tm = {};
        localtimeSafe(&tm, &result.timePoint_);

        // Set the time to 00:00:00
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        std::time_t lastMidnight = std::mktime(&tm);

        // If we're already at or after 00:00, subtract one day
        if (result.timePoint_ < lastMidnight) {
            lastMidnight -= 86400;  // Go back one day
        }

        result.timePoint_ = lastMidnight;
        result.toLocalTime();  // Convert back to local time
        return result;
    }

    // Get the next UTC 00:00 (midnight)
    DateTime getNextDaily() const {
        DateTime result = *this;
        result.toUTC();

        std::tm tm = {};
        localtimeSafe(&tm, &result.timePoint_);

        // Set the time to 00:00:00
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;

        std::time_t nextMidnight = std::mktime(&tm);

        // If the timePoint_ is already at or before 00:00, add one day
        if (result.timePoint_ >= nextMidnight) {
            nextMidnight += 86400;  // Move forward one day
        }

        result.timePoint_ = nextMidnight;
        result.toLocalTime();  // Convert back to local time
        return result;
    }

    // Get the previous Monday at 07:30 UTC
    DateTime getLastWeekly() const {
        DateTime result = *this;
        result.toUTC();

        std::tm tm = {};
        localtimeSafe(&tm, &result.timePoint_);

        // Set the time to 07:30:00
        tm.tm_hour = 7;
        tm.tm_min = 30;
        tm.tm_sec = 0;

        // Calculate the previous Monday
        int daysToMonday = (tm.tm_wday == 0) ? 6 : tm.tm_wday - 1;  // Sunday is 0, Monday is 1
        tm.tm_mday -= daysToMonday;

        std::time_t lastMonday = std::mktime(&tm);

        // If the timePoint_ is already at or after Monday 07:30, subtract one week
        if (result.timePoint_ < lastMonday) {
            lastMonday -= 7 * 86400;  // Go back one week
        }

        result.timePoint_ = lastMonday;
        result.toLocalTime();  // Convert back to local time
        return result;
    }

    // Get the next Monday at 07:30 UTC
    DateTime getNextWeekly() const {
        DateTime result = *this;
        result.toUTC();

        std::tm tm = {};
        localtimeSafe(&tm, &result.timePoint_);

        // Set the time to 07:30:00
        tm.tm_hour = 7;
        tm.tm_min = 30;
        tm.tm_sec = 0;

        // Calculate the next Monday
        int daysToMonday = (tm.tm_wday == 0) ? 1 : 8 - tm.tm_wday;
        tm.tm_mday += daysToMonday;

        std::time_t nextMonday = std::mktime(&tm);

        // If the timePoint_ is already at or before Monday 07:30, add one week
        if (result.timePoint_ >= nextMonday) {
            nextMonday += 7 * 86400;  // Move forward one week
        }

        result.timePoint_ = nextMonday;
        result.toLocalTime();  // Convert back to local time
        return result;
    }

    // Get today as day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
    int GetTodayAsDayOfWeek() const {
        std::tm tm = {};
        localtimeSafe(&tm, &timePoint_);
        return tm.tm_wday;
    }

    // Get today as day of the month (e.g., 1, 15, 30)
    int GetTodayAsDayOfMonth() const {
        std::tm tm = {};
        localtimeSafe(&tm, &timePoint_);
        return tm.tm_mday;
    }

    // Check if today is the last day of the month
    bool IsTodayUltimoOfMonth() const {
        std::tm tm = {};
        localtimeSafe(&tm, &timePoint_);

        // Save the current day of the month
        int currentDay = tm.tm_mday;

        // Move to the first day of the next month
        tm.tm_mday = 1;
        tm.tm_mon += 1;

        // Convert back to time_t to adjust the date
        std::time_t nextMonth = std::mktime(&tm);

        // Go back one day to get the last day of the current month
        nextMonth -= 86400;  // 86400 seconds = 1 day
        localtimeSafe(&tm, &nextMonth);

        return currentDay == tm.tm_mday;
    }

private:
    std::time_t timePoint_;
    int timeZoneOffset_;  // In seconds

    // Detect system time zone offset
    static int detectSystemTimeZoneOffset() {
        std::time_t now = std::time(nullptr);
        std::tm local_tm = {};
        localtimeSafe(&local_tm, &now);
        std::tm gm_tm = {};
        gmtimeSafe(&gm_tm, &now);

        int localSeconds = local_tm.tm_hour * 3600 + local_tm.tm_min * 60 + local_tm.tm_sec;
        int gmSeconds = gm_tm.tm_hour * 3600 + gm_tm.tm_min * 60 + gm_tm.tm_sec;

        return localSeconds - gmSeconds;
    }

    // Parse timezone offset string like "+0200"
    void parseTimezoneOffset(const std::string& tzOffsetStr) {
        if (tzOffsetStr.size() != 5 || (tzOffsetStr[0] != '+' && tzOffsetStr[0] != '-')) {
            throw std::invalid_argument("Invalid timezone offset format");
        }

        int tzHours = std::stoi(tzOffsetStr.substr(1, 2));
        int tzMinutes = std::stoi(tzOffsetStr.substr(3, 2));

        timeZoneOffset_ = (tzHours * 3600 + tzMinutes * 60);

        if (tzOffsetStr[0] == '-') {
            timeZoneOffset_ = -timeZoneOffset_;
        }

        // Adjust timePoint_ to UTC
        timePoint_ -= timeZoneOffset_;
    }

    // Format the timezone offset back to a string
    std::string formatTimezoneOffset() const {
        int offset = timeZoneOffset_;
        char sign = '+';
        if (offset < 0) {
            sign = '-';
            offset = -offset;
        }
        int hours = offset / 3600;
        int minutes = (offset % 3600) / 60;

        std::ostringstream ss;
        ss << sign << std::setw(2) << std::setfill('0') << hours
            << std::setw(2) << std::setfill('0') << minutes;
        return ss.str();
    }

    // Convert timePoint_ to UTC
    void toUTC() {
        timePoint_ -= timeZoneOffset_;
    }

    // Convert timePoint_ to local time
    void toLocalTime() {
        timePoint_ += timeZoneOffset_;
    }

    // Thread-safe localtime
    static void localtimeSafe(std::tm* result, const std::time_t* timePoint) {
#ifdef _WIN32
        localtime_s(result, timePoint);
#else
        localtime_r(timePoint, result);
#endif
    }

    // Thread-safe gmtime
    static void gmtimeSafe(std::tm* result, const std::time_t* timePoint) {
#ifdef _WIN32
        gmtime_s(result, timePoint);
#else
        gmtime_r(timePoint, result);
#endif
    }

    // Helper function to format time with the given format
    std::string formatTime(const std::string& format) const {
        std::tm tm = {};
        localtimeSafe(&tm, &timePoint_);

        std::ostringstream ss;
        ss << std::put_time(&tm, format.c_str());
        return ss.str();
    }
};

#endif // DATETIME_H
