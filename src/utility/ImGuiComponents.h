#ifndef CUSTOM_IMGUI_COMPONENTS_H
#define CUSTOM_IMGUI_COMPONENTS_H

#include <string>
#include <map>
#include "../imgui/imgui.h"

enum ChartType {
    BAR_CHART,
    LINE_CHART
};

bool CardTab(const char* label, bool selected);
bool DateTimePicker(const char* label, std::string& dateStr, bool showTime = true);
void BarChart(const char* label, std::map<std::string, int> values, float nexusScaling, ChartType chartType);
#endif