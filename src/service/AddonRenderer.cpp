#include "AddonRenderer.h"
#include "OrganizerRepository.h"

/* render proto */
void renderOrganizer();
void renderTodoList();
/* Component proto */
void renderCurrentTasks();
void renderDoneTasks();
void renderTaskConfiguration();
void renderAPITasks();
void renderNewTaskDialog();

/* control flags */        
static int selected_tab = 0;

bool renderAddNew = false;
bool renderAddNewDialog = false;
bool renderChangeInterval = false;

OrganizerItem newItem = {};
OrganizerItem* changeIntervalItem = nullptr;

OrganizerItemInstance newInstance = {};

OrganizerItem* editItem = nullptr;

bool displayOwnOnly = true;

ImVec2 changeIntervalPos = {};

/* filter flags */
std::string tableFilter;

/* Render Constants */
const ImVec4 colorRed = ImVec4(1, 0, 0, 1);

/* Icon resources */
Texture* iconClose = nullptr;
Texture* iconOptions = nullptr;
Texture* iconTrash = nullptr;
Texture* iconCheck = nullptr;
Texture* iconAdd = nullptr;
Texture* iconRepeat = nullptr;

/* Pagination */
int itemsPerPage = 25;
int currentPage = 1;

void Renderer::preRender() {
    if (iconClose == nullptr)
        iconClose = APIDefs->GetTexture("ICON_ORGANIZER_CLOSE");
    if (iconOptions == nullptr)
        iconOptions = APIDefs->GetTexture("ICON_ORGANIZER_OPTIONS");
    if (iconCheck == nullptr)
        iconCheck = APIDefs->GetTexture("ICON_ORGANIZER_CHECK");
    if (iconTrash == nullptr)
        iconTrash = APIDefs->GetTexture("ICON_ORGANIZER_TRASH");
    if (iconAdd == nullptr)
        iconAdd = APIDefs->GetTexture("ICON_ORGANIZER_ADD");
    if (iconRepeat == nullptr)
        iconRepeat = APIDefs->GetTexture("ICON_ORGANIZER_REPEAT");


    if (accountName.empty()) displayOwnOnly = false;
}
void Renderer::render() {
    renderNewTaskDialog();
    renderTodoList();
	renderOrganizer();
}
void Renderer::postRender() {
	// TODO impl
}

void Renderer::unload() {
    organizerRepo->save();
}

// ==== render functions ====
void renderNewTaskDialog() {
    if (!renderAddNewDialog) return;

    if (ImGui::Begin("New Task", &renderAddNewDialog, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImVec2 windowSize = ImVec2();
        windowSize.x = 0;
        windowSize.y = newInstance.hasEndDate ? 550.0f : 270.0f;
        
        std::vector<OrganizerItem*> items = organizerRepo->getConfigurableItems();
        if (ImGui::BeginCombo("Category", newInstance.itemId == 0
            ? "New..."
            : organizerRepo->getConfigurableItemById(newInstance.itemId)->title.c_str())) {
            if (ImGui::Selectable("New...", newInstance.itemId == 0)) {
                newInstance.itemId = 0;
                newItem = {};
                newItem.type = ItemType::DEFAULT;
                newItem.repeatMode = RepeatMode::ONCE;
            }
            int i = 0;
            for (auto item : items) {
                if (item->deleted) continue;
                ++i;
                if (ImGui::Selectable(item->title.c_str(), newInstance.itemId == i)) {
                    newItem = OrganizerItem(*item);
                    newInstance.itemId = newItem.id;
                }
            }

            ImGui::EndCombo();
        }

        char bufferTitle[256];
        strncpy_s(bufferTitle, newItem.title.c_str(), sizeof(bufferTitle));
        if (ImGui::InputText("Title", bufferTitle, sizeof(bufferTitle))) {
            newItem.title = bufferTitle;
        }
        char bufferDesc[2048];
        strncpy_s(bufferDesc, newItem.description.c_str(), sizeof(bufferDesc));
        if (ImGui::InputTextMultiline("Description", bufferDesc, sizeof(bufferDesc))) {
            newItem.description = bufferDesc;
        }

        if (ImGui::Checkbox("Has due date", &newInstance.hasEndDate)) {
            if (newInstance.hasEndDate) {
                newInstance.endDate = format_date(std::chrono::system_clock::now());
            }
            else {
                newInstance.endDate = "";
            }
        }

        if (newInstance.hasEndDate) {
            if (DateTimePicker("Due", newInstance.endDate)) {

            }
        }

        if (ImGui::Button("Save")) {
            if (!newItem.title.empty()) {
                OrganizerItem* newCategory = new OrganizerItem(newItem);
                organizerRepo->addConfigurableItem(newCategory);
                newInstance.itemId = newCategory->id;

                // Set owner of the task
                if (accountName.empty() && characterName.empty()) {
                    newInstance.owner = "-";
                }
                else if (characterName.empty()) {
                    newInstance.owner = accountName;
                }
                else {
                    newInstance.owner = characterName + "\n" + accountName;
                }

                organizerRepo->addTaskInstance(new OrganizerItemInstance(newInstance));
                newItem = {};
                newInstance = {};
                renderAddNewDialog = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            newItem = {};
            newInstance = {};
            renderAddNewDialog = false;
        }

        ImGui::End();
    }
}

void renderTodoList() {
    if (!todoListRendered) return;

    if (ImGui::Begin("TODOs", &todoListRendered, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
                                                | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        // Render custom title bar first... sigh
        ImGui::PushFont((ImFont*)NexusLink->FontBig);
        ImGui::Text("TODOs");
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetWindowWidth() - (3*30 + 5)); // 3* buttons plus 5 generic to the edge
        if (iconAdd != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconAdd->Resource, { 20,20 })) {
                renderAddNewDialog = true;
                newItem = {};
                newItem.type = ItemType::DEFAULT;
                newItem.repeatMode = RepeatMode::ONCE;

                newInstance = {};
                newInstance.startDate = format_date(std::chrono::system_clock::now());
                newInstance.endDate = "";
                newInstance.completed = false;

            }
        }
        else {
            if (ImGui::Button("+", { 20,20 })) {
                renderAddNewDialog = true;
                newItem = {};
                newItem.type = ItemType::DEFAULT;
                newItem.repeatMode = RepeatMode::ONCE;

                newInstance = {};
                newInstance.startDate = format_date(std::chrono::system_clock::now());
                newInstance.endDate = "";
                newInstance.completed = false;
            }
        }
        ImGui::SameLine();
        if (iconOptions != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconOptions->Resource, { 20,20 })) {
                organizerRendered = !organizerRendered;
            }
        }
        else {
            if (ImGui::Button("O", { 20,20 })) {
                organizerRendered = !organizerRendered;
            }
        }
        ImGui::SameLine();
        if (iconClose != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconClose->Resource, { 20,20 })) {
                todoListRendered = false;
            }
        }
        else {
            if (ImGui::Button("X", { 20,20 })) {
                todoListRendered = false;
            }
        }
        ImGui::Separator();
        
        // Finally, content.
        char bufferTaskFilter[256];
        strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
        if (ImGui::InputText("Filter", bufferTaskFilter, sizeof(bufferTaskFilter))) {
            tableFilter = bufferTaskFilter;
        }
        if (!accountName.empty()) {
            ImGui::Checkbox("Show only tasks of this account", &displayOwnOnly);
        }
        ImGui::Separator();

        if (ImGui::BeginChild("TodoContent")) {
            ImVec2 window_size = ImGui::GetWindowSize();
            float table_width = window_size.x;
            float rightColumnWidth = 25.0f;
            float leftColumnWidth = table_width - (2 * rightColumnWidth) - 25.0;
            if (ImGui::GetCurrentWindow()->ScrollbarY) {
                leftColumnWidth -= 10.0f;
            }

            if (ImGui::BeginTable("TODOTable", 4, ImGuiTableFlags_BordersH)) {
                ImGui::TableSetupColumn("##ChangeInterval", ImGuiTableColumnFlags_WidthFixed, 25.0f);
                ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthFixed, leftColumnWidth - 25.0f);
                ImGui::TableSetupColumn("##Complete", ImGuiTableColumnFlags_WidthFixed, rightColumnWidth);
                ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, rightColumnWidth);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (auto task : organizerRepo->getTaskInstances()) {
                    if (displayOwnOnly && !strContains(task->owner, accountName)) continue;
                    if (task->completed || task->deleted) continue;
                    OrganizerItem* item = organizerRepo->getConfigurableItemById(task->itemId);
                    if (item == nullptr) continue;
                    if (item->deleted) continue;

                    if (!tableFilter.empty()) {
                        bool found = false;
                        if (strContains(item->title, tableFilter)) found = true;
                        if (strContains(item->description, tableFilter)) found = true;
                        if (strContains(task->owner, tableFilter)) found = true;
                        if (strContains(std::string(ItemTypeValue(item->type)), tableFilter)) found = true;
                        if (strContains(task->startDate, tableFilter)) found = true;
                        if (strContains(task->endDate, tableFilter)) found = true;

                        if (!found) continue;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); // Interval buttons
                    if (item->apiId.empty()) {
                        // not a GW2 API Item => eligible for interval changes on the spot
                        if (iconRepeat != nullptr) {
                            ImGui::PushID(hashString("changeinterval_" + std::to_string(task->id)));
                            if (ImGui::ImageButton((ImTextureID)iconRepeat->Resource, { 20,20 }, { 0,0 }, { 1,1 })) {
                                renderChangeInterval = true;
                                changeIntervalPos = ImGui::GetMousePos();
                                changeIntervalItem = item;
                            }
                            ImGui::PopID();
                        }
                        else {
                            if (ImGui::Button("R", { 20,20 })) {
                                renderChangeInterval = true;
                                changeIntervalPos = ImGui::GetMousePos();
                                changeIntervalItem = item;
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Change repetition of this task");
                            ImGui::EndTooltip();
                        }
                    }
                    else {
                        // GW2 API item, cannot change intervals on these, duh
                        ImGui::Text("------");
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Can't edit intervals for tasks from the GW2 API!");
                            ImGui::EndTooltip(); 
                        }
                    }

                    ImGui::TableNextColumn(); // Title text
                    if (task->endDate.empty()) {
                        ImGui::Text(item->title.c_str());
                    }
                    else {
                        auto dueDate = parse_date(task->endDate);
                        auto now = std::chrono::system_clock::now();
                        auto zonedNow = std::chrono::zoned_time(std::chrono::current_zone(), now);
                        if (dueDate.time_since_epoch() < zonedNow.get_local_time().time_since_epoch()) {
                            ImGui::TextColored(colorRed, item->title.c_str());
                        }
                        else {
                            ImGui::Text(item->title.c_str());                            
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::TextWrapped(item->description.c_str());

                        ImGui::Text("");
                        ImGui::Text(ItemTypeValue(item->type));
                        ImGui::SameLine();
                        ImGui::Text(" - ");
                        ImGui::SameLine();
                        ImGui::Text(RepeatModeValue(item->repeatMode));
                        ImGui::Text(("Owner: " + replaceNewLines(task->owner)).c_str());
                        if (!task->endDate.empty()) {
                            std::string end = replaceNewLines(format_date_output(task->endDate));
                            ImGui::Text(("Due: " + end).c_str());
                        }
                        ImGui::EndTooltip();
                    }

                    ImGui::TableNextColumn(); // Finish button
                    if (iconCheck != nullptr) {
                        ImGui::PushID(hashString("finish_" + std::to_string(task->id)));
                        if (ImGui::ImageButton((ImTextureID)iconCheck->Resource, { 20,20 }, { 0,0 }, { 1,1 })) { 
                            task->completionDate = format_date(std::chrono::system_clock::now());
                            task->completed = true;
                            organizerRepo->save();
                        }
                        ImGui::PopID();
                    }
                    ImGui::TableNextColumn(); // Delete button
                    if (iconTrash != nullptr) {
                        ImGui::PushID(hashString("remove_" + std::to_string(task->id)));
                        if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, { 20,20 }, { 0,0 }, { 1,1 })) { 
                            task->deleted = true;
                            organizerRepo->save();
                        }
                        ImGui::PopID();
                    }
                }

                ImGui::EndTable();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }

    if (renderChangeInterval && changeIntervalItem != nullptr) {
        if (ImGui::Begin("##interval", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse 
            | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            for (int i = 0; i < static_cast<int>(RepeatMode::COUNT); ++i) {
                RepeatMode currentEnum = static_cast<RepeatMode>(i);
                if (ImGui::Button(RepeatModeValue(currentEnum), { 150,20 })) {
                    changeIntervalItem->repeatMode = currentEnum;
                    organizerRepo->save();
                    changeIntervalItem = nullptr;
                    renderChangeInterval = false;
                }
            }
            if (ImGui::Button("Cancel", { 150,20 })) {
                changeIntervalItem = nullptr;
                renderChangeInterval = false;
            }
        }
    }
}

void renderOrganizer() {
    if (!organizerRendered) return;

    // Create the main window
    if (ImGui::Begin("Organizer", &organizerRendered, ImGuiWindowFlags_NoCollapse)) {

        // Create a child window for the tab bar on the left
        ImGui::BeginChild("Left Pane", ImVec2(150, 0), true);

        // Render vertical card-style tabs

        if (CardTab("Open Tasks", selected_tab == 0)) selected_tab = 0;
        if (CardTab("Ingame activities", selected_tab == 1)) selected_tab = 1;
        if (CardTab("Completed Tasks", selected_tab == 2)) selected_tab = 2;
        if (CardTab("Task configuration", selected_tab == 3)) selected_tab = 3;

        ImGui::EndChild();

        // Create a child window for the content on the right
        ImGui::SameLine();
        ImGui::BeginChild("Right Pane", ImVec2(0, 0), true);

        // Render content based on the selected tab
        if (selected_tab == 0)
        {
            renderCurrentTasks();
        }
        else if (selected_tab == 1)
        {
            renderAPITasks();
        }
        else if (selected_tab == 2)
        {
            renderDoneTasks();
        }
        else if (selected_tab == 3)
        {
            renderTaskConfiguration();
        }

        ImGui::EndChild();

        ImGui::End();
    }
}

// ==== component functions ====
void renderCurrentTasks() {
    if (!renderAddNew) {
        if (ImGui::Button("Add new task")) {
            renderAddNew = true;
            newItem = {};
            newItem.type = ItemType::DEFAULT;
            newItem.repeatMode = RepeatMode::ONCE;

            newInstance = {};
            newInstance.startDate = format_date(std::chrono::system_clock::now());
            newInstance.endDate = "";
            newInstance.completed = false;
        }
    }
    else {
        ImVec2 windowSize = ImVec2();
        windowSize.x = 0;
        windowSize.y = newInstance.hasEndDate ? 550.0f : 270.0f;
        ImGui::BeginChild("New Task Item", windowSize, true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);

        std::vector<OrganizerItem*> items = organizerRepo->getConfigurableItems();
        if (ImGui::BeginCombo("Category", newInstance.itemId == 0 
                                                                ? "New..." 
                                                                : organizerRepo->getConfigurableItemById(newInstance.itemId)->title.c_str())) {
            if (ImGui::Selectable("New...", newInstance.itemId == 0)) {
                newInstance.itemId = 0;
                newItem = {};
                newItem.type = ItemType::DEFAULT;
                newItem.repeatMode = RepeatMode::ONCE;
            }
            int i = 0;
            for (auto item : items) {
                if (item->deleted) continue;
                ++i;
                if (ImGui::Selectable(item->title.c_str(), newInstance.itemId == i)) {
                    newItem = OrganizerItem(*item);
                    newInstance.itemId = newItem.id;
                }
            }

            ImGui::EndCombo();
        }

        char bufferTitle[256];
        strncpy_s(bufferTitle, newItem.title.c_str(), sizeof(bufferTitle));
        if (ImGui::InputText("Title", bufferTitle, sizeof(bufferTitle))) {
            newItem.title = bufferTitle;
        }
        char bufferDesc[2048];
        strncpy_s(bufferDesc, newItem.description.c_str(), sizeof(bufferDesc));
        if (ImGui::InputTextMultiline("Description", bufferDesc, sizeof(bufferDesc))) {
            newItem.description = bufferDesc;
        }

        if (ImGui::Checkbox("Has due date", &newInstance.hasEndDate)) {
            if (newInstance.hasEndDate) {
                newInstance.endDate = format_date(std::chrono::system_clock::now());
            }
            else {
                newInstance.endDate = "";
            }
        }

        if (newInstance.hasEndDate) {
            if (DateTimePicker("Due", newInstance.endDate)) {

            }
        }

        if (ImGui::Button("Save")) {
            if (!newItem.title.empty()) {
                OrganizerItem* newCategory = new OrganizerItem(newItem);
                organizerRepo->addConfigurableItem(newCategory);
                newInstance.itemId = newCategory->id;

                // Set owner of the task
                if (accountName.empty() && characterName.empty()) {
                    newInstance.owner = "-";
                }
                else if (characterName.empty()) {
                    newInstance.owner = accountName;
                }
                else {
                    newInstance.owner = characterName + "\n" + accountName;
                }

                organizerRepo->addTaskInstance(new OrganizerItemInstance(newInstance));
                newItem = {};
                newInstance = {};
                renderAddNew = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            newItem = {};
            newInstance = {};
            renderAddNew = false;
        }
        ImGui::EndChild();
    }

    char bufferTaskFilter[256];
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    if (ImGui::BeginTable("CurrentTasksTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Title", 0, 0.3f);
        ImGui::TableSetupColumn("Description", 0, 0.0f);
        ImGui::TableSetupColumn("Type", 0, 0.2f);
        ImGui::TableSetupColumn("Owner", 0, 0.4f);
        ImGui::TableSetupColumn("Started", 0, 0.2f);
        ImGui::TableSetupColumn("Due", 0, 0.2f);
        ImGui::TableSetupColumn("##Complete", 0, 0.2f);
        ImGui::TableSetupColumn("##Delete", 0, 0.2f);

        ImGui::TableHeadersRow();

        for (auto task : organizerRepo->getTaskInstances()) {
            if (task->completed || task->deleted) continue;
            OrganizerItem* item = organizerRepo->getConfigurableItemById(task->itemId);

            if (!tableFilter.empty()) {
                if (item == nullptr) continue;
                bool found = false;
                if (strContains(item->title, tableFilter)) found = true;
                if (strContains(item->description, tableFilter)) found = true;
                if (strContains(task->owner, tableFilter)) found = true;
                if (strContains(std::string(ItemTypeValue(item->type)),tableFilter)) found = true;
                if (strContains(task->startDate,tableFilter)) found = true;
                if (strContains(task->endDate,tableFilter)) found = true;

                if (!found) continue;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped(item == nullptr? "Unknown!" : item->title.c_str());
            ImGui::TableNextColumn(); 
            ImGui::TextWrapped(item == nullptr ? "Unknown!" : item->description.c_str());
            ImGui::TableNextColumn();
            ImGui::Text(item == nullptr ? "Unknown!" : ItemTypeValue(item->type));
            ImGui::TableNextColumn(); 
            ImGui::Text(task->owner.c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(format_date_output(task->startDate).c_str());
            ImGui::TableNextColumn(); 
            if (task->endDate.empty()) {
                ImGui::Text("-");
            }
            else {
                auto dueDate = parse_date(task->endDate);
                auto now = std::chrono::system_clock::now();
                auto zonedNow = std::chrono::zoned_time(std::chrono::current_zone(), now);
                
                if (dueDate.time_since_epoch() < zonedNow.get_local_time().time_since_epoch()) {
                    ImGui::TextColored(colorRed, format_date_output(task->endDate).c_str());
                }
                else {
                    ImGui::Text(format_date_output(task->endDate).c_str());
                }
            }
            ImGui::TableNextColumn(); 
            if (ImGui::Button(("Complete##" + std::to_string(task->id)).c_str())) {
                task->completed = true;
                task->completionDate = format_date(std::chrono::system_clock::now());
                organizerRepo->save();
            }
            ImGui::TableNextColumn(); 
            if (ImGui::Button(("Remove##" + std::to_string(task->id)).c_str())) {
                task->deleted = true;
                organizerRepo->save();
            }
        }
        ImGui::EndTable();
    }
}
void renderAPITasks() {

    if (ImGui::CollapsingHeader("Wizards Vault")) {
        for (auto progress : organizerRepo->wizardsVaultDaily) {
            ImGui::SetCursorPosX(30);
            if (ImGui::CollapsingHeader(("Progress for account: " + progress.first).c_str())) {
                ImGui::SetCursorPosX(30);
                if (ImGui::BeginTable(("WizardVaultTable" + progress.first).c_str(), 5, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableSetupColumn("Title", 0, 0.6f);
                    ImGui::TableSetupColumn("Track", 0, 0.05f);
                    ImGui::TableSetupColumn("Progress", 0, 0.15f);
                    ImGui::TableSetupColumn("Status", 0, 0.15f);
                    ImGui::TableSetupColumn("##AutoTrack", 0, 0.15f);
                    ImGui::TableHeadersRow();

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextWrapped("Daily Overall");
                    ImGui::TableNextColumn();
                    ImGui::Text("");
                    ImGui::TableNextColumn();
                    ImGui::Text((std::to_string(progress.second.meta_progress_current) + "/" + std::to_string(progress.second.meta_progress_complete)).c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text(progress.second.meta_reward_claimed ? "Claimed" : "Unclaimed");
                    ImGui::TableNextColumn();
                    ImGui::Text(""); // TODO auto track button feasible?
                    
                    for (auto objective : progress.second.objectives) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped(objective.title.c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text(objective.track.c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text((std::to_string(objective.progress_current) + "/" + std::to_string(objective.progress_complete)).c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text(objective.claimed ? "Claimed" : "Unclaimed");
                        ImGui::TableNextColumn();
                        ImGui::Text(""); // TODO auto track button feasible?
                    }

                    if (organizerRepo->wizardsVaultWeekly.count(progress.first)) {
                        ImGui::TableHeadersRow();

                        gw2::wizardsvault::MetaProgress weeklyProgress = organizerRepo->wizardsVaultWeekly[progress.first];

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped("Weekly Overall");
                        ImGui::TableNextColumn();
                        ImGui::Text("");
                        ImGui::TableNextColumn();
                        ImGui::Text((std::to_string(weeklyProgress.meta_progress_current) + "/" + std::to_string(weeklyProgress.meta_progress_complete)).c_str());
                        ImGui::TableNextColumn();
                        ImGui::Text(weeklyProgress.meta_reward_claimed ? "Claimed" : "Unclaimed");
                        ImGui::TableNextColumn();
                        bool temp = false;
                        if (ImGui::Checkbox(("Auto track##trackwvweekly_" + progress.first).c_str(), &temp)) { // TODO actual settings which recipes are tracked
                            // TODO if checked, create configurable item (if not exists) + task instance (if no instance exists for the day for that account, no matter if done or open)
                        }

                        for (auto objective : weeklyProgress.objectives) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextWrapped(objective.title.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text(objective.track.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text((std::to_string(objective.progress_current) + "/" + std::to_string(objective.progress_complete)).c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text(objective.claimed ? "Claimed" : "Unclaimed");
                            ImGui::TableNextColumn();
                            bool temp = false;
                            if (ImGui::Checkbox(("Auto track##trackwvweekly_" + progress.first + "-" + std::to_string(objective.id)).c_str(), &temp)) { // TODO actual settings which recipes are tracked
                                // TODO if checked, create configurable item (if not exists) + task instance (if no instance exists for the day for that account, no matter if done or open)
                            }
                        }
                    }

                    ImGui::EndTable();
                }
            }
        }
    }
    
    if (ImGui::CollapsingHeader("Achievements")) {
        for (auto category : organizerRepo->achievements) {
            ImGui::SetCursorPosX(30);
            if (ImGui::CollapsingHeader(category.first.name.c_str())) {
                
                ImGui::SetCursorPosX(30);
                ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId("achievement_group_" + std::to_string(category.first.id));
                if (configurable == nullptr) {
                    // NO OP
                }
                else {
                    if (ImGui::Checkbox(("Track all achievements for category " + category.first.name).c_str(), &configurable->active)) {
                        organizerRepo->save();
                    }
                }
                ImGui::SetCursorPosX(30);
                if (ImGui::BeginTable(("AchievementsTable" + category.first.name).c_str(), 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableSetupColumn("Name", 0, 0.5f);
                    ImGui::TableSetupColumn("Description", 0, 0.5f);
                    ImGui::TableSetupColumn("##AutoTrack", 0, 0.2f);
                    ImGui::TableHeadersRow();

                    for (auto achievement : category.second) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped(achievement.name.c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped(achievement.requirement.c_str());
                        ImGui::TableNextColumn();

                        ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId("achievement_single_" + std::to_string(achievement.id));
                        if (configurable == nullptr) {
                            ImGui::Text("n/a");
                        }
                        else {
                            if (ImGui::Checkbox(("Auto track##track_" + std::to_string(achievement.id)).c_str(), &configurable->active)) {
                                organizerRepo->save();
                            }
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Daily Crafting")) {
        ImGui::SetCursorPosX(30);
        if (ImGui::BeginTable("DailyCraftsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableSetupColumn("Item", 0, 0.3f);
            ImGui::TableSetupColumn("Status", 0, 0.15f);
            ImGui::TableSetupColumn("##AutoTrack", 0, 0.15f);
            ImGui::TableHeadersRow();

            for (auto craftable : organizerRepo->dailycraft.recipes) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped(dailyCraftablesTranslator.at(craftable).c_str()); // TODO translate to human readable value instead of API value!
                ImGui::TableNextColumn();
                std::vector<std::string> progression = organizerRepo->getAccountProgression(craftable);
                if (progression.empty()) {
                    ImGui::TextWrapped("n/a");
                }
                else {
                    std::ostringstream oss;
                    for (size_t i = 0; i < progression.size(); ++i) {
                        oss << progression[i];
                        if (i != progression.size() - 1) {
                            oss << '\n';
                        }
                    }
                    ImGui::TextWrapped(oss.str().c_str());
                }
                ImGui::TableNextColumn();
                ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(craftable);
                if (configurable == nullptr) {
                    ImGui::Text("n/a");
                }
                else {
                    if (ImGui::Checkbox(("Auto track##track_" + craftable).c_str(), &configurable->active)) {
                        organizerRepo->save();
                    }
                }
            }

            ImGui::EndTable();
        }
    }
    
    if (ImGui::CollapsingHeader("Map Metas")) {
        ImGui::SetCursorPosX(30);
        if (ImGui::BeginTable("MetaTrackTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableSetupColumn("Meta", 0, 0.3f);
            ImGui::TableSetupColumn("Status", 0, 0.15f);
            ImGui::TableSetupColumn("##AutoTrack", 0, 0.15f);
            ImGui::TableHeadersRow();

            for (auto meta : organizerRepo->mapchests.chests) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped(mapChestsTranslator.at(meta).c_str()); // TODO translate to human readable value instead of API value!
                ImGui::TableNextColumn();
                std::vector<std::string> progression = organizerRepo->getAccountProgression(meta);
                if (progression.empty()) {
                    ImGui::TextWrapped("n/a");
                }
                else {
                    std::ostringstream oss;
                    for (size_t i = 0; i < progression.size(); ++i) {
                        oss << progression[i];
                        if (i != progression.size() - 1) {
                            oss << '\n';
                        }
                    }
                    ImGui::TextWrapped(oss.str().c_str());
                }
                ImGui::TableNextColumn();

                ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(meta);
                if (configurable == nullptr) {
                    ImGui::Text("n/a");
                }
                else {
                    if (ImGui::Checkbox(("Auto track##track_" + meta).c_str(), &configurable->active)) {
                        organizerRepo->save();
                    }
                }
            }

            ImGui::EndTable();
        }
    }
    
    if (ImGui::CollapsingHeader("World Bosses")) {
        ImGui::SetCursorPosX(30);
        if (ImGui::BeginTable("WorldBossesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableSetupColumn("Boss", 0, 0.3f);
            ImGui::TableSetupColumn("Status", 0, 0.15f);
            ImGui::TableSetupColumn("##AutoTrack", 0, 0.15f);
            ImGui::TableHeadersRow();

            for (auto boss : organizerRepo->worldbosses.worldbosses) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped(worldbossesTranslator.at(boss).c_str()); // TODO translate to human readable value instead of API value!
                ImGui::TableNextColumn();
                std::vector<std::string> progression = organizerRepo->getAccountProgression(boss);
                if (progression.empty()) {
                    ImGui::TextWrapped("n/a");
                }
                else {
                    std::ostringstream oss;
                    for (size_t i = 0; i < progression.size(); ++i) {
                        oss << progression[i];
                        if (i != progression.size() - 1) {
                            oss << '\n';
                        }
                    }
                    ImGui::TextWrapped(oss.str().c_str());
                }
                ImGui::TableNextColumn();
                ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(boss);
                if (configurable == nullptr) {
                    ImGui::Text("n/a");
                }
                else {
                    if (ImGui::Checkbox(("Auto track##track_" + boss).c_str(), &configurable->active)) {
                        organizerRepo->save();
                    }
                }
            }

            ImGui::EndTable();
        }
    }
    
    if (ImGui::CollapsingHeader("Dungeons")) {
        for (auto dungeon : organizerRepo->dungeons) {
            ImGui::SetCursorPosX(30);
            if (ImGui::CollapsingHeader(dungeonTranslator.at(dungeon.id).c_str())) {
                ImGui::SetCursorPosX(30);
                ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(dungeon.id);
                if (configurable == nullptr) {
                    // NO OP
                }
                else {
                    if (ImGui::Checkbox(("Track all paths of " + dungeonTranslator.at(dungeon.id)).c_str(), &configurable->active)) {
                        organizerRepo->save();
                    }
                }
                ImGui::SetCursorPosX(30);
                if (ImGui::BeginTable("DungeonPathsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableSetupColumn("Path", 0, 0.3f);
                    ImGui::TableSetupColumn("Type", 0, 0.3f);
                    ImGui::TableSetupColumn("Status", 0, 0.15f);
                    ImGui::TableSetupColumn("##AutoTrack", 0, 0.15f);
                    ImGui::TableHeadersRow();
                    for (auto path : dungeon.paths) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped(dungeonPathsTranslator.at(path.id).c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextWrapped(path.type.c_str());
                        ImGui::TableNextColumn();
                        std::vector<std::string> progression = organizerRepo->getAccountProgression(path.id);
                        if (progression.empty()) {
                            ImGui::TextWrapped("n/a");
                        }
                        else {
                            std::ostringstream oss;
                            for (size_t i = 0; i < progression.size(); ++i) {
                                oss << progression[i];
                                if (i != progression.size() - 1) {
                                    oss << '\n';
                                }
                            }
                            ImGui::TextWrapped(oss.str().c_str());
                        }
                        ImGui::TableNextColumn();
                        ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(path.id);
                        if (configurable == nullptr) {
                            // NO OP
                        }
                        else {
                            if (ImGui::Checkbox(("Auto track##track_" + path.id).c_str(), &configurable->active)) {
                                organizerRepo->save();
                            }
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
    }
    
    if (ImGui::CollapsingHeader("Raids")) {
        for (auto raid : organizerRepo->raids) {
            for (auto wing : raid.wings) {
                ImGui::SetCursorPosX(30);
                if (ImGui::CollapsingHeader(raidTranslator.at(wing.id).c_str())) {
                    ImGui::SetCursorPosX(30);
                    ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(wing.id);
                    if (configurable == nullptr) {
                        // NO OP
                    }
                    else {
                        if (ImGui::Checkbox(("Track all bosses of " + raidTranslator.at(wing.id)).c_str(), &configurable->active)) {
                            organizerRepo->save();
                        }
                    }
                    ImGui::SetCursorPosX(30);
                    if (ImGui::BeginTable("RaidTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
                        ImGui::TableSetupColumn("Boss", 0, 0.3f);
                        ImGui::TableSetupColumn("Status", 0, 0.15f);
                        ImGui::TableSetupColumn("##AutoTrack", 0, 0.15f);
                        ImGui::TableHeadersRow();

                        for (auto boss : wing.events) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextWrapped(raidBossesTranslator.at(boss.id).c_str());
                            ImGui::TableNextColumn();
                            std::vector<std::string> progression = organizerRepo->getAccountProgression(boss.id);
                            if (progression.empty()) {
                                ImGui::TextWrapped("n/a");
                            }
                            else {
                                std::ostringstream oss;
                                for (size_t i = 0; i < progression.size(); ++i) {
                                    oss << progression[i];
                                    if (i != progression.size() - 1) {
                                        oss << '\n';
                                    }
                                }
                                ImGui::TextWrapped(oss.str().c_str());
                            }
                            ImGui::TableNextColumn();
                            ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(boss.id);
                            if (configurable == nullptr) {
                                // NO OP
                            }
                            else {
                                if (ImGui::Checkbox(("Auto track##track_" + boss.id).c_str(), &configurable->active)) {
                                    organizerRepo->save();
                                }
                            }
                        }

                        ImGui::EndTable();
                    }
                }
            }
        }
    }
}
void renderDoneTasks() {
    char bufferTaskFilter[256];
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    int totalAvailableTasks = 0;
    int startAtTaskCount = (currentPage - 1) * itemsPerPage;
    int endAtTaskCount = currentPage * itemsPerPage;

    if (ImGui::BeginTable("DoneTasksTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Title", 0, 0.3f);
        ImGui::TableSetupColumn("Description", 0, 0.0f);
        ImGui::TableSetupColumn("Type", 0, 0.2f);
        ImGui::TableSetupColumn("Owner", 0, 0.4f);
        ImGui::TableSetupColumn("Started", 0, 0.2f);
        ImGui::TableSetupColumn("Completed", 0, 0.2f);
        ImGui::TableSetupColumn("##Complete", 0, 0.2f);
        ImGui::TableSetupColumn("##Delete", 0, 0.2f);

        ImGui::TableHeadersRow();

        for (auto task : organizerRepo->getTaskInstances()) {
            if (!task->completed || task->deleted) continue;
            OrganizerItem* item = organizerRepo->getConfigurableItemById(task->itemId);
            if (item->deleted) continue;

            if (!tableFilter.empty()) {
                if (item == nullptr) continue;
                bool found = false;
                if (strContains(item->title, tableFilter)) found = true;
                if (strContains(item->description, tableFilter)) found = true;
                if (strContains(task->owner, tableFilter)) found = true;
                if (strContains(std::string(ItemTypeValue(item->type)), tableFilter)) found = true;
                if (strContains(task->startDate, tableFilter)) found = true;
                if (strContains(task->endDate, tableFilter)) found = true;

                if (!found) continue;
            }

            totalAvailableTasks++;
            if (totalAvailableTasks <= startAtTaskCount) continue; // still data of previous page
            if (totalAvailableTasks > endAtTaskCount) continue; // data of next page

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped(item == nullptr ? "Unknown!" : item->title.c_str());
            ImGui::TableNextColumn(); 
            ImGui::TextWrapped(item == nullptr ? "Unknown!" : item->description.c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(item == nullptr ? "Unknown!" : ItemTypeValue(item->type));
            ImGui::TableNextColumn(); 
            ImGui::TextWrapped(task->owner.c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(format_date_output(task->startDate).c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(format_date_output(task->completionDate).c_str());
            ImGui::TableNextColumn(); 
            if (ImGui::Button(("Reactivate##" + std::to_string(task->id)).c_str())) {
                task->completed = false;
                task->completionDate = "";
                organizerRepo->save();
            }
            ImGui::TableNextColumn(); 
            if (ImGui::Button(("Remove##" + std::to_string(task->id)).c_str())) {
                task->deleted = true;
                organizerRepo->save();
            }
        }
        ImGui::EndTable();
    }

    // I expect this table to get quite populated quite fast so TODO:
    // - add a drop down "Show last 25, 50, 100"
    // - add pagination
    int currentIndex = itemsPerPage == 25 ? 0 : itemsPerPage == 50 ? 1 : 2;
    static const char* itemsPerPageComboItems[] = { "25", "50", "100" };
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("Items per page", &currentIndex, itemsPerPageComboItems, IM_ARRAYSIZE(itemsPerPageComboItems))) {
        switch (currentIndex) {
        case 0: itemsPerPage = 25; break;
        case 1: itemsPerPage = 50; break;
        case 2: itemsPerPage = 100; break;
        default: itemsPerPage = 25;
        }
    }
    ImGui::SameLine();

    // Text "Page X of Y"
    int totalPagesAvailable = (totalAvailableTasks + itemsPerPage - 1) / itemsPerPage;
    std::string pagesText = "Page " + std::to_string(currentPage) + " of " + std::to_string(totalPagesAvailable);

    float textWidth = ImGui::CalcTextSize(pagesText.c_str()).x;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2);
    ImGui::Text(pagesText.c_str());
    // page buttons
    ImGui::SameLine();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2 - 25 - 15);
    if (ImGui::Button("<", {25,25})) {
        currentPage--;
        if (currentPage == 0) currentPage = 1;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2 + (textWidth) + 15);
    if (ImGui::Button(">", {25,25})) {
        currentPage++;
        if (currentPage > totalPagesAvailable) currentPage = totalPagesAvailable;
    }

    /*
    * BarChart Sample for later usage
    std::map<std::string, int> values = { 
        {"2023-07-01", 5}, 
        {"2023-07-02", 3}, 
        {"2023-07-03", 0}, 
        {"2023-07-04", 4}
    };
    BarChart("TasksBarChart", values);
    */
}
void renderTaskConfiguration() {
    // TODO inputs for new category configuration

    char bufferTaskFilter[256];
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    if (ImGui::BeginTable("ConfigurableItemsTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Title", 0, 0.3f);
        ImGui::TableSetupColumn("Description", 0, 0.0f);
        if (editItem == nullptr) {
            ImGui::TableSetupColumn("Type", 0, 0.15f);
            ImGui::TableSetupColumn("Repeat Mode", 0, 0.15f);
            ImGui::TableSetupColumn("Interval", 0, 0.15f);
        }
        else {
            ImGui::TableSetupColumn("Type", 0, 0.3f);
            ImGui::TableSetupColumn("Repeat Mode", 0, 0.3f);
            ImGui::TableSetupColumn("Interval", 0, 0.3f);
        }
        ImGui::TableSetupColumn("##StartTask", 0, 0.15f);
        ImGui::TableSetupColumn("##EditTask", 0, 0.15f);
        ImGui::TableSetupColumn("##Delete", 0, 0.15f);

        ImGui::TableHeadersRow();

        for (auto item : organizerRepo->getConfigurableItems()) {
            if (item->deleted) continue;

            if (!tableFilter.empty()) {
                bool found = false;
                if (strContains(item->title,tableFilter)) found = true;
                if (strContains(item->description,tableFilter)) found = true;
                if (strContains(std::string(ItemTypeValue(item->type)),tableFilter)) found = true;
                if (strContains(std::string(RepeatModeValue(item->repeatMode)),tableFilter)) found = true;
                if (!found) continue;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::TextWrapped(item->title.c_str());
            }
            else {
                char bufferTitle[256];
                strncpy_s(bufferTitle, editItem->title.c_str(), sizeof(bufferTitle));
                if (ImGui::InputText("##Title", bufferTitle, sizeof(bufferTitle))) {
                    editItem->title = bufferTitle;
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::TextWrapped(item->description.c_str());
            }
            else {
                char bufferDesc[256];
                strncpy_s(bufferDesc, editItem->description.c_str(), sizeof(bufferDesc));
                if (ImGui::InputTextMultiline("##Desc", bufferDesc, sizeof(bufferDesc))) {
                    editItem->description = bufferDesc;
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::Text(ItemTypeValue(item->type));
            }
            else {
                int typeValue = static_cast<int>(editItem->type);
                if (ImGui::Combo("##Type", &typeValue, ItemTypeCombo, IM_ARRAYSIZE(ItemTypeCombo))) {
                    editItem->type = convertToType(typeValue);
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::Text(RepeatModeValue(item->repeatMode));
            }
            else {
                int repeatValue = static_cast<int>(editItem->repeatMode);
                if (ImGui::Combo("##RepeatMode", &repeatValue, RepeatModeCombo, IM_ARRAYSIZE(RepeatModeCombo))) {
                    editItem->repeatMode = static_cast<RepeatMode>(repeatValue);
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::Text(std::to_string(item->customRepeatInterval).c_str());
            }
            else {
                if (ImGui::InputInt("##Interval", &editItem->customRepeatInterval)) {
                    if (editItem->customRepeatInterval < 0) editItem->customRepeatInterval = 0;
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                if (ImGui::Button(("Start##" + std::to_string(item->id)).c_str())) {
                    renderAddNew = true;
                    newItem = OrganizerItem(*item);

                    newInstance = {};
                    newInstance.startDate = format_date(std::chrono::system_clock::now());
                    newInstance.endDate = "";
                    newInstance.completed = false;
                    newInstance.itemId = item->id;
                    selected_tab = 0;
                }
            }
            else {
                if (ImGui::Button(("Save##" + std::to_string(item->id)).c_str())) {
                    item->title = editItem->title;
                    item->description = editItem->description;
                    item->type = editItem->type;
                    item->repeatMode = editItem->repeatMode;
                    item->customRepeatInterval = editItem->customRepeatInterval;
                    editItem = nullptr;
                    organizerRepo->save();
                }
            }
            ImGui::TableNextColumn();
            if (editItem == nullptr || editItem->id != item->id) {
                if (ImGui::Button(("Edit##" + std::to_string(item->id)).c_str())) {
                    editItem = new OrganizerItem(*item);
                }
            }
            else {
                if (ImGui::Button(("Cancel##" + std::to_string(item->id)).c_str())) {
                    editItem = nullptr;
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                if (ImGui::Button(("Remove##" + std::to_string(item->id)).c_str())) {
                    item->deleted = true;
                    organizerRepo->save();
                }
            }
            else {

            }
        }
        ImGui::EndTable();
    }

    ImGui::Text("Tasks configured by Guild Wars 2 API");
    ImGui::Text("(Only tracked ones will show up here. Configure tracked tasks in the 'Ingame Activities' tab.");

    if (ImGui::BeginTable("APIItemsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Title", 0, 0.3f);
        ImGui::TableSetupColumn("Description", 0, 0.0f);
        ImGui::TableSetupColumn("Type", 0, 0.4f);
        ImGui::TableSetupColumn("Repeat Mode", 0, 0.25f);
        
        ImGui::TableHeadersRow();
        
        for (auto configurable : organizerRepo->getApiTaskConfigurables()) {
            if (!configurable->active) continue; // we are only interested in what we configured
            if (!tableFilter.empty()) {
                bool found = false;
                if (strContains(configurable->item.title, tableFilter)) found = true;
                if (strContains(configurable->item.description, tableFilter)) found = true;
                if (strContains(std::string(ItemTypeValue(configurable->item.type)), tableFilter)) found = true;
                if (strContains(std::string(RepeatModeValue(configurable->item.repeatMode)), tableFilter)) found = true;
                if (!found) continue;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped(configurable->item.title.c_str());

            ImGui::TableNextColumn();
            ImGui::TextWrapped(configurable->item.description.c_str());

            ImGui::TableNextColumn();
            ImGui::Text(ItemTypeValue(configurable->item.type));

            ImGui::TableNextColumn();
            ImGui::Text(RepeatModeValue(configurable->item.repeatMode));
        }
        
        ImGui::EndTable();
    }
}