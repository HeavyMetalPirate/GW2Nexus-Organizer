#include "ImGuiComponents.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <format>

#include "../Constants.h"

void BarChart(const char* label, std::map<std::string, int> values)
{
    // Sample call:
    //  const char* dates[] = { "2023-07-01", "2023-07-02", "2023-07-03", "2023-07-04" };
    //  int tasks[] = { 5, 3, 8, 2 };
    //  int num_dates = sizeof(tasks) / sizeof(tasks[0]);
    //  BarChart(dates, tasks, num_dates);

    ImGui::Begin(label);

    // Get the maximum value for the Y-axis
    int max_tasks = 0;
    for(auto value: values) {
        if (value.second > max_tasks) {
            max_tasks = value.second;
        }
    }

    // Draw the bars
    int i = 0;
    for (auto label: values) {
        float bar_width = ImGui::CalcTextSize(label.first.c_str()).x;

        if (label.second > 0) {
            float bar_height = (float)label.second / max_tasks;

            ImGui::SetCursorPosX(i * bar_width + 5 * i);
            ImGui::SetCursorPosY(150.0f - bar_height * 100);
            // TODO maybe a more cool routine for bar drawing intead of a button lmao
            ImGui::Button("", ImVec2(bar_width, bar_height * 100));
        }

        // Label the bar with the date
        ImGui::SetCursorPosX(i * bar_width + 5 * i);
        ImGui::SetCursorPosY(150.0f);
        ImGui::Text(label.first.c_str());
        ++i;
    }

    ImGui::End();
}


bool CardTab(const char* label, bool selected)
{
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
    bool pressed = ImGui::Button(label, ImVec2(120, 40));
    ImGui::PopStyleColor(2);
    return pressed;
}

bool DateTimePicker(const char* label, std::string& dateTimeStr) {
    bool valueChanged = false;

    int year, month, day, hour, minute, second, timezoneOffsetHour, timezoneOffsetMinute;
    char timezoneSign;

    // Parse the date/time string in ISO 8601 format
    std::regex dateTimeRegex(R"(^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})([+-])(\d{2}):(\d{2})$)");
    std::smatch matches;
    if (std::regex_match(dateTimeStr, matches, dateTimeRegex)) {
        year = std::stoi(matches[1].str());
        month = std::stoi(matches[2].str());
        day = std::stoi(matches[3].str());
        hour = std::stoi(matches[4].str());
        minute = std::stoi(matches[5].str());
        second = 0;
        timezoneSign = matches[7].str()[0];
        timezoneOffsetHour = std::stoi(matches[8].str());
        timezoneOffsetMinute = std::stoi(matches[9].str());
    }
    else {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm{};
        std::tm utc_tm{};

        localtime_s(&now_tm, &now_time_t);
        gmtime_s(&utc_tm, &now_time_t);

        // Extract time components
        year = now_tm.tm_year + 1900;
        month = now_tm.tm_mon + 1;
        day = now_tm.tm_mday;
        hour = now_tm.tm_hour;
        minute = now_tm.tm_min;
        second = 0;

        // Calculate timezone offset
        std::time_t local_time = std::mktime(&now_tm);
        std::time_t gm_time = std::mktime(&utc_tm);
        int timezone_offset_seconds = static_cast<int>(std::difftime(local_time, gm_time));
        timezoneOffsetHour = timezone_offset_seconds / 3600;
        timezoneOffsetMinute = (timezone_offset_seconds % 3600) / 60;

        timezoneSign = (timezone_offset_seconds >= 0) ? '+' : '-';
        if (timezone_offset_seconds < 0) {
            timezoneOffsetHour = -timezoneOffsetHour;
            timezoneOffsetMinute = -timezoneOffsetMinute;
        }
    }

    int daysInMonth;
    if (month == 2) {
        // February, leap year calc
        daysInMonth = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 29 : 28;
    }
    else {
        daysInMonth = calender.at(month);
    }

    int startDay;
    std::tm time_in = { 0, 0, 0, 1, month - 1, year - 1900 }; // month is 0-11 in struct tm
    std::mktime(&time_in);
    int wday = time_in.tm_wday; // 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    startDay = (wday == 0) ? 6 : wday - 1;

    const int daysPerWeek = 7;
    int totalCells = (daysInMonth + startDay);
    int numRows = (totalCells + daysPerWeek - 1) / daysPerWeek;

    ImGui::BeginChild("DatePickerFrame", { static_cast<float>(8 * 35), static_cast<float>((numRows) * 35 + 100) }, true, ImGuiWindowFlags_AlwaysAutoResize);

    // Month selection
    int monthComboValue = month - 1;
    if (monthComboValue < 0) monthComboValue = 0;
    if (monthComboValue > 11) monthComboValue = 11;
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("##Month", &monthComboValue, monthsComboBoxItems, IM_ARRAYSIZE(monthsComboBoxItems))) {
        month = monthComboValue + 1;
        valueChanged = true;
    }
    // Year Input
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("##Year", &year)) {
        valueChanged = true;
    }

    ImGui::BeginChild("CalendarChild", { static_cast<float>(8 * 35), static_cast<float>((numRows) * 35 + 15) }, true, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginTable("CalendarTable", daysPerWeek, ImGuiTableFlags_Borders)) {
        for (auto dayOfWeekStr : daysOfWeek) {
            ImGui::TableSetupColumn(dayOfWeekStr, ImGuiTableColumnFlags_WidthFixed, 31.0f);
        }
        ImGui::TableHeadersRow();

        int buttonDay = 1;

        for (int row = 0; row < numRows; ++row) {
            ImGui::TableNextRow();
            for (int col = 0; col < daysPerWeek; ++col) {
                ImGui::TableSetColumnIndex(col);
                if (row == 0 && col < startDay) {
                    ImGui::Text(" ");
                }
                else if (buttonDay <= daysInMonth) {
                    std::string buttonLabel = std::to_string(buttonDay);

                    bool stylePushed = false;
                    if (buttonDay == day && !valueChanged) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                        stylePushed = true;
                    }

                    if (ImGui::Button(buttonLabel.c_str(), ImVec2(30, 30))) {
                        day = buttonDay;
                        valueChanged = true;
                    }

                    if (stylePushed) {
                        ImGui::PopStyleColor();
                    }

                    ++buttonDay;
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::Text("Time (24H):");
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("##DatePickerHour", &hour)) {
        if (hour < 0) hour = 0;
        if (hour > 23) hour = 23;
        valueChanged = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("##DatePickerMinute", &minute)) {
        if (minute < 0) {
            minute = 59;
            --hour;
            if (hour < 0) hour = 0;
        }
        if (minute > 59) {
            minute = 0;
            ++hour;
            if (hour > 23) {
                hour = 0;
            }
        }
        valueChanged = true;
    }

    ImGui::EndChild();

    if (valueChanged) {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << year << "-"
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day << "T"
            << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second
            << timezoneSign << std::setw(2) << std::setfill('0') << timezoneOffsetHour << ":"
            << std::setw(2) << std::setfill('0') << timezoneOffsetMinute;
        dateTimeStr = oss.str();
    }

    return valueChanged;
}