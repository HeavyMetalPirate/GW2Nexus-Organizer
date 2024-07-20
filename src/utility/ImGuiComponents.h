#ifndef CUSTOM_IMGUI_COMPONENTS_H
#define CUSTOM_IMGUI_COMPONENTS_H

#include <string>
#include <map>
#include "../imgui/imgui.h"

bool CardTab(const char* label, bool selected);
bool DateTimePicker(const char* label, std::string& dateStr);
void BarChart(const char* label, std::map<std::string, int> values);
#endif