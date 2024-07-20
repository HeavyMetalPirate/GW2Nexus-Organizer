#ifndef DATETIME_UTILS_H
#define DATETIME_UTILS_H

#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>

// Helpers for date parsing
inline std::chrono::system_clock::time_point parse_date(const std::string& date_str) {
    std::tm tm = {};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    std::time_t time_utc = _mkgmtime(&tm);
    return std::chrono::system_clock::from_time_t(time_utc);
}
inline std::string format_date(const std::chrono::system_clock::time_point& time_point) {
    std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm;
    localtime_s(&tm, &time_t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S%z");
    return ss.str();
}
inline std::string format_date_output(const std::chrono::system_clock::time_point& time_point) {
    std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm;
    localtime_s(&tm, &time_t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d\n%H:%M:%S");
    return ss.str();
}
inline std::chrono::system_clock::time_point getUTCinLocalTimePoint(const std::chrono::system_clock::time_point utc) {
    std::time_t time_t_utc = std::chrono::system_clock::to_time_t(utc);
    std::tm local_tm;
    localtime_s(&local_tm, &time_t_utc);

    std::time_t time_t_local = std::mktime(&local_tm);
    return std::chrono::system_clock::from_time_t(time_t_local);
}
inline std::string getUTCinLocalDate(const std::string& date_str) {
    if (date_str.empty()) return date_str;
    std::chrono::system_clock::time_point utc = parse_date(date_str);
    std::chrono::system_clock::time_point local = getUTCinLocalTimePoint(utc);
    return format_date_output(local);
}
inline std::string format_date_output(const std::string& date_str) {
    if (date_str.size() < 19) return date_str;
    //comes in format "2024-07-05T15:50:19+0200"
    std::string datePart = date_str.substr(0, 10);
    std::string timePart = date_str.substr(11, 8);
    return datePart + "\n" + timePart;
}

#endif