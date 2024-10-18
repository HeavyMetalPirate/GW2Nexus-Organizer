#include "ImGuiComponents.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <format>

#include "../Constants.h"

// Function to dynamically generate Y-axis labels
std::vector<int> generateYAxisLabels(int max_count) {
    if (max_count <= 0) {
        return { 0 };  // Avoid invalid cases, return a label of 0
    }

    // Calculate the magnitude order of the max count
    int magnitude = std::pow(10, std::floor(std::log10(max_count)));

    // Find an appropriate step size (1, 2, or 5 times the magnitude)
    int step;
    if (max_count / magnitude <= 2) {
        step = magnitude / 2;
    }
    else if (max_count / magnitude <= 5) {
        step = magnitude;
    }
    else {
        step = magnitude * 2;
    }

    // Ensure step is always at least 1 to avoid division issues
    if (step == 0) {
        step = 1;
    }

    // Generate Y-axis labels
    std::vector<int> labels;
    for (int i = 0; i <= max_count + step; i += step) {
        labels.push_back(i);
    }

    return labels;
}

// Function to calculate the height of bars based on max value
float calculateBarHeight(int count, int max_count, int graph_height) {
    // Generate Y-axis labels and find the maximum label (not the raw max_count)
    std::vector<int> labels = generateYAxisLabels(max_count);
    int max_label = labels.back();  // Get the highest Y-axis label

    if (max_label == 0) {
        return 0.0;  // Avoid division by zero
    }

    // Calculate the bar height based on the highest Y-axis label
    return (static_cast<float>(count) / max_label) * graph_height;
}

void BarChart(const char* label, std::map<std::string, int> values, float nexusScaling, ChartType chartType)
{
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    ImGui::BeginChild(label, {ImGui::GetWindowSize().x, ImGui::GetWindowSize().y / 3}, false, ImGuiWindowFlags_NoScrollbar);
    
    ImVec2 windowSize = ImGui::GetWindowSize();
    const float margin = 30 * nexusScaling;
    const float graphHeight = cursorPos.y + (windowSize.y - margin);
    const float graphWidth = cursorPos.x + (windowSize.x - margin);

    ImGui::Text(label);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // Draw Outlines for Chart
    draw_list->AddLine(
        ImVec2(cursorPos.x + margin, graphHeight),
        ImVec2(graphWidth, graphHeight),
        IM_COL32(255, 0, 255, 255),
        1.5f * nexusScaling
    );
    draw_list->AddLine(
        ImVec2(cursorPos.x + margin, cursorPos.y + margin),
        ImVec2(cursorPos.x + margin, graphHeight),
        IM_COL32(255, 0, 255, 255),
        1.5f * nexusScaling
    );

    // Bar stuff
    int maxValue = 0;
    for (auto entry : values) {
        if (entry.second > maxValue) maxValue = entry.second;
    }
    std::vector<int> yLabels = generateYAxisLabels(maxValue);

    int yAxisLength = (graphHeight)-(cursorPos.y + margin);
    float yAxisSpacing = yLabels.size() > 1 ? static_cast<float>(yAxisLength) / (yLabels.size() - 1) : 0;
    const ImVec2 yAxisLabelSize = ImGui::CalcTextSize("100");
    const float labelX = cursorPos.x + margin - (yAxisLabelSize.x * nexusScaling);

    for (int i = yLabels.size() - 1; i >= 0; i--) {
        float labelY = cursorPos.y + margin + (yAxisLength - (i * yAxisSpacing)) - (yAxisLabelSize.y * nexusScaling) / 2;

        draw_list->AddText(
            ImVec2(labelX, labelY),
            IM_COL32(255, 255, 255, 255),
            std::to_string(yLabels[i]).c_str()
        );
    }

    int xAxisLength = graphWidth - (cursorPos.x + margin);
    float xAxisSpacing = values.empty() ? 0 : xAxisLength / values.size();
    float textWidth = ImGui::CalcTextSize("2023-07-01").x * nexusScaling;

    bool shorten = false;
    bool shortenExtreme = false;
    if (textWidth > xAxisSpacing) {
        shorten = true;
        textWidth = ImGui::CalcTextSize("23-07-01").x * nexusScaling;
        if (textWidth > xAxisSpacing) {
            shortenExtreme = true;
            textWidth = ImGui::CalcTextSize("07-01").x * nexusScaling;
        }
    }


    int i = 0;
    float previousHeight = 0;
    float previousX = cursorPos.x + margin;

    for (auto entry : values) {
        // x axis drawing

        std::string text;
        if (shortenExtreme) {
            text = entry.first.substr(5);
        }
        else if (shorten) {
            text = entry.first.substr(2);
        }
        else {
            text = entry.first;
        }

        draw_list->AddText(
            ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i)), graphHeight + (5 * nexusScaling)),
            IM_COL32(255, 255, 255, 255),
            text.c_str()
        );

        float height = calculateBarHeight(entry.second, maxValue, yAxisLength);
        if (chartType == ChartType::LINE_CHART) {

            const float dotSize = 4.0f;
            ImVec2 topLeft = ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2) - (dotSize / 2), graphHeight - height - dotSize);
            ImVec2 bottomRight = ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2) + (dotSize / 2), graphHeight - height + dotSize);

            if (ImGui::IsMouseHoveringRect(topLeft, bottomRight)) {
                ImGui::BeginTooltip();
                ImGui::Text((entry.first.substr(0, 10) + ": ").c_str());
                ImGui::SameLine();
                ImGui::Text(std::to_string(entry.second).c_str());
                ImGui::EndTooltip();
            }

            // Draw the dot
            draw_list->AddCircleFilled(
                ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2), graphHeight - height),
                dotSize,
                IM_COL32(0, 0, 255, 255)
            );
            // Draw the line between
            draw_list->AddLine(
                ImVec2(previousX, graphHeight - previousHeight),
                ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2), graphHeight - height),
                IM_COL32(255, 255, 255, 255),
                1.5f * nexusScaling
            );
        }
        else if (chartType == ChartType::BAR_CHART) {
            const float bar_width = textWidth * 0.7;

            ImVec2 topLeft = ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2) - (bar_width / 2), graphHeight - height);
            ImVec2 bottomRight = ImVec2((cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2) + (bar_width / 2), graphHeight);

            if (ImGui::IsMouseHoveringRect(topLeft, bottomRight)) {
                ImGui::BeginTooltip();
                ImGui::Text((entry.first.substr(0, 10) + ": ").c_str());
                ImGui::SameLine();
                ImGui::Text(std::to_string(entry.second).c_str());
                ImGui::EndTooltip();
            }

            draw_list->AddRectFilled(
                topLeft,
                bottomRight,
                IM_COL32(255, 255, 255, 255)
            );
        }

        previousHeight = height;
        previousX = (cursorPos.x + (margin * 1.5) + (xAxisSpacing * i) + textWidth / 2);
        ++i;
    }

    ImGui::SetCursorPosY(graphHeight);

    ImGui::EndChild();
}

bool CardTab(const char* label, bool selected)
{
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, selected ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
    bool pressed = ImGui::Button(label, ImVec2(120, 40));
    ImGui::PopStyleColor(2);
    return pressed;
}

bool DateTimePicker(const char* label, std::string& dateTimeStr, bool showTime) {
    bool valueChanged = false;

    int year, month, day, hour, minute, second, timezoneOffsetHour, timezoneOffsetMinute;
    char timezoneSign;

    // Parse the date/time string in ISO 8601 format
    std::regex dateTimeRegex(R"(^(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})([+-])(\d{2})(\d{2})$)");
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

    if (showTime) {
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
            << timezoneSign << std::setw(2) << std::setfill('0') << timezoneOffsetHour
            << std::setw(2) << std::setfill('0') << timezoneOffsetMinute;
        dateTimeStr = oss.str();
    }

    return valueChanged;
}