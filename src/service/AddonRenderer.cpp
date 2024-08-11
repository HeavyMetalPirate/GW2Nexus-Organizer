#include "AddonRenderer.h"
#include "OrganizerRepository.h"

/* render proto */
void renderOrganizer();
void renderTodoList();
/* Component tabs proto */
void renderCurrentTasks();
void renderDoneTasks();
void renderTaskConfiguration();
void renderAPITasks();
void renderSettings();
/* Misc rendering proto */
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

std::string newTaskName;

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
Texture* iconNoRepeat = nullptr;
Texture* iconReactivate = nullptr;
Texture* iconStart = nullptr;
Texture* iconEdit = nullptr;
Texture* iconSave = nullptr;
Texture* iconCancel = nullptr;

/* Pagination */
static const char* itemsPerPageComboItems[] = { "10", "25", "50" };
int doneItemsPerPage = 10;
int doneCurrentPage = 1;
int configItemsPerPage = 10;
int configCurrentPage = 1;

void Renderer::preRender() {
    if (iconClose == nullptr)
        iconClose = APIDefs->Textures.Get("ICON_ORGANIZER_CLOSE");
    if (iconOptions == nullptr)
        iconOptions = APIDefs->Textures.Get("ICON_ORGANIZER_OPTIONS");
    if (iconCheck == nullptr)
        iconCheck = APIDefs->Textures.Get("ICON_ORGANIZER_CHECK");
    if (iconTrash == nullptr)
        iconTrash = APIDefs->Textures.Get("ICON_ORGANIZER_TRASH");
    if (iconAdd == nullptr)
        iconAdd = APIDefs->Textures.Get("ICON_ORGANIZER_ADD");
    if (iconRepeat == nullptr)
        iconRepeat = APIDefs->Textures.Get("ICON_ORGANIZER_REPEAT");
    if (iconReactivate == nullptr)
        iconReactivate = APIDefs->Textures.Get("ICON_ORGANIZER_REACTIVATE");
    if (iconStart == nullptr)
        iconStart = APIDefs->Textures.Get("ICON_ORGANIZER_START");
    if (iconEdit == nullptr)
        iconEdit = APIDefs->Textures.Get("ICON_ORGANIZER_EDIT");
    if (iconNoRepeat == nullptr)
        iconNoRepeat = APIDefs->Textures.Get("ICON_ORGANIZER_NO_REPEAT");
    if (iconSave == nullptr)
        iconSave = APIDefs->Textures.Get("ICON_ORGANIZER_SAVE");
    if (iconCancel == nullptr)
        iconCancel = APIDefs->Textures.Get("ICON_ORGANIZER_CANCEL");

    if (accountName.empty()) displayOwnOnly = false;
}
void Renderer::render() {
    try {
        renderNewTaskDialog();
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("AddonRenderer::Render():RenderNewTaskDialog(): " + std::string(e.what())).c_str());
    }
    catch (...) {
        APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("Unknown exception while calling AddonRenderer::Render():RenderNewTaskDialog()"));
    }
    try {
        renderTodoList();
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("AddonRenderer::Render():RenderTodoList(): " + std::string(e.what())).c_str());
    }
    catch (...) {
        APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("Unknown exception while calling AddonRenderer::Render():RenderTodoList()"));
    }
    try {
        renderOrganizer();
    }
    catch (const std::exception& e) {
        APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("AddonRenderer::Render():RenderOrganizer(): " + std::string(e.what())).c_str());
    }
    catch (...) {
        APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("Unknown exception while calling AddonRenderer::Render():RenderOrganizer()"));
    }
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
                DateTime now = DateTime::nowLocal();
                newInstance.endDate = now.toString();
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

    ImGui::SetNextWindowSize(ImVec2(370.0f, 350.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("TODOs", &todoListRendered, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
                                                | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        // Render custom title bar first... sigh
        ImGui::PushFont((ImFont*)NexusLink->FontBig);
        ImGui::Text("TODOs");
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetWindowWidth() - (3 * (35 * NexusLink->Scaling) + 5)); // 3* buttons plus 5 generic to the edge
        if (iconAdd != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconAdd->Resource, { 20 * NexusLink->Scaling, 20 * NexusLink->Scaling })) {
                renderAddNewDialog = true;
                newItem = {};
                newItem.type = ItemType::DEFAULT;
                newItem.repeatMode = RepeatMode::ONCE;

                newInstance = {};
                DateTime now = DateTime::nowLocal();
                newInstance.startDate = now.toString();
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
                DateTime now = DateTime::nowLocal();
                newInstance.startDate = now.toString();
                newInstance.endDate = "";
                newInstance.completed = false;
            }
        }
        ImGui::SameLine();
        if (iconOptions != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconOptions->Resource, { 20 * NexusLink->Scaling, 20 * NexusLink->Scaling })) {
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
            if (ImGui::ImageButton((ImTextureID)iconClose->Resource, { 20 * NexusLink->Scaling, 20 * NexusLink->Scaling })) {
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
        char bufferNewTask[256];
        strncpy_s(bufferNewTask, newTaskName.c_str(), sizeof(bufferNewTask));
        if (ImGui::InputText("##TaskName", bufferNewTask, sizeof(bufferNewTask))) {
            newTaskName = bufferNewTask;
        }
        ImGui::SameLine();
        if (ImGui::Button("Create Task")) {
            if (!newTaskName.empty()) {
                newItem = {};
                newItem.title = newTaskName;
                newItem.description = "";
                newItem.type = ItemType::DEFAULT;
                newItem.repeatMode = RepeatMode::ONCE;

                newInstance = {};
                DateTime now = DateTime::nowLocal();
                newInstance.startDate = now.toString();
                newInstance.endDate = "";
                newInstance.completed = false;

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
                newTaskName = "";
            }
        }

        if (ImGui::CollapsingHeader("Filter")) {
            char bufferTaskFilter[256];
            strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
            if (ImGui::InputText("Filter", bufferTaskFilter, sizeof(bufferTaskFilter))) {
                tableFilter = bufferTaskFilter;
            }
            if (!accountName.empty()) {
                ImGui::Checkbox("Show only tasks of this account", &displayOwnOnly);
            }
        }
        ImGui::Separator();

        if (ImGui::BeginChild("TodoContent")) {
            ImVec2 window_size = ImGui::GetWindowSize();
            float table_width = window_size.x;
            float rightColumnWidth = 30.0f * NexusLink->Scaling;
            float leftColumnWidth = table_width - (2 * rightColumnWidth) - (30.0f * NexusLink->Scaling);
            if (ImGui::GetCurrentWindow()->ScrollbarY) {
                leftColumnWidth -= (10.0f * NexusLink->Scaling);
            }

            if (ImGui::BeginTable("TODOTable", 4, ImGuiTableFlags_BordersH)) {
                ImGui::TableSetupColumn("##ChangeInterval", ImGuiTableColumnFlags_WidthFixed, 30.0f * NexusLink->Scaling);
                ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthFixed, leftColumnWidth - (30.0f * NexusLink->Scaling));
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
                            ImTextureID buttonTex = item->repeatMode == RepeatMode::ONCE ? iconNoRepeat->Resource : iconRepeat->Resource;
                            if (ImGui::ImageButton(buttonTex, { 20 * NexusLink->Scaling, 20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
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
                        DateTime dueDate = DateTime(task->endDate);
                        if (dueDate.isBeforeNow()) {
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
                            DateTime endDate = DateTime(task->endDate);
                            ImGui::Text(("Due: " + endDate.toStringNice()).c_str());
                        }
                        ImGui::EndTooltip();
                    }
                    /* TODO might use this, might not
                    switch (item->repeatMode) {
                        case RepeatMode::DAILY: ImGui::Text("(daily)"); break;
                        case RepeatMode::WEEKLY: ImGui::Text("(weekly)"); break;
                        default: ImGui::Text("");
                    } 
                    */

                    ImGui::TableNextColumn(); // Finish button
                    if (iconCheck != nullptr) {
                        ImGui::PushID(hashString("finish_" + std::to_string(task->id)));
                        if (ImGui::ImageButton((ImTextureID)iconCheck->Resource, { 20 * NexusLink->Scaling, 20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                            task->completionDate = DateTime::nowLocal().toString();
                            task->completed = true;
                            organizerRepo->save();
                        }
                        ImGui::PopID();
                    }
                    else {
                        if (ImGui::Button(("C##" + std::to_string(task->id)).c_str(), {20,20})) {
                            task->completionDate = DateTime::nowLocal().toString();
                            task->completed = true;
                            organizerRepo->save();
                        }
                    }
                    ImGui::TableNextColumn(); // Delete button
                    if (iconTrash != nullptr) {
                        ImGui::PushID(hashString("remove_" + std::to_string(task->id)));
                        if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, { 20 * NexusLink->Scaling, 20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                            task->deleted = true;
                            organizerRepo->save();
                        }
                        ImGui::PopID();
                    }
                    else {
                        if (ImGui::Button(("D##" + std::to_string(task->id)).c_str(), { 20,20 })) {
                            task->deleted = true;
                            organizerRepo->save();
                        }
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
                if (ImGui::Button(RepeatModeValue(currentEnum), { 150 * NexusLink->Scaling, 20 * NexusLink->Scaling })) {
                    if (!changeIntervalItem->accountConfiguration.contains(accountName) && !accountName.empty()) {
                        changeIntervalItem->accountConfiguration.emplace(accountName, currentEnum == RepeatMode::ONCE ? false : true);
                    }
                    else if(!accountName.empty()) {
                        changeIntervalItem->accountConfiguration[accountName] = currentEnum == RepeatMode::ONCE ? false : true;
                    }
                    changeIntervalItem->repeatMode = currentEnum;
                    organizerRepo->save();
                    changeIntervalItem = nullptr;
                    renderChangeInterval = false;
                }
            }
            if (ImGui::Button("Cancel", { 150 * NexusLink->Scaling,20 * NexusLink->Scaling })) {
                changeIntervalItem = nullptr;
                renderChangeInterval = false;
            }
            ImGui::End();
        }
    }
}

void renderOrganizer() {
    if (!organizerRendered) return;

    // Create the main window
    ImGui::SetNextWindowSize(ImVec2(960.0f * NexusLink->Scaling, 770.0f * NexusLink->Scaling), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Organizer", &organizerRendered, ImGuiWindowFlags_NoCollapse)) {

        // Create a child window for the tab bar on the left
        if (ImGui::BeginChild("Left Pane", ImVec2(140 * NexusLink->Scaling, 0), true)) {

            // Render vertical card-style tabs

            if (CardTab("Open Tasks", selected_tab == 0)) selected_tab = 0;
            if (CardTab("Ingame activities", selected_tab == 1)) selected_tab = 1;
            if (CardTab("Completed Tasks", selected_tab == 2)) selected_tab = 2;
            if (CardTab("Task configuration", selected_tab == 3)) selected_tab = 3;
            if (CardTab("Settings", selected_tab == 4)) selected_tab = 4;

            ImGui::EndChild();
        }

        // Create a child window for the content on the right
        ImGui::SameLine();
        if (ImGui::BeginChild("Right Pane", ImVec2(0, 0), true)) {
            try {
                switch (selected_tab) {
                case 0: renderCurrentTasks(); break;
                case 1: renderAPITasks(); break;
                case 2: renderDoneTasks(); break;
                case 3: renderTaskConfiguration(); break;
                case 4: renderSettings(); break;
                default: ImGui::Text(("Unknown Tab: " + std::to_string(selected_tab)).c_str());
                }
            }
            catch (const std::exception& e) {
                APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("AddonRenderer::RenderOrganizer() with selected_tab " + std::to_string(selected_tab) + ": " + std::string(e.what())).c_str());
            }
            catch (...) {
                APIDefs->Log(ELogLevel_CRITICAL, ADDON_NAME, ("Unknown exception while calling AddonRenderer::RenderOrganizer() with selected_tab " + std::to_string(selected_tab)).c_str());
            }

            ImGui::EndChild();
        }

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
            newInstance.startDate = DateTime::nowLocal().toString();
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
                newInstance.endDate = DateTime::nowLocal().toString();
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

    if (ImGui::BeginTable("CurrentTasksTable", 8, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Started", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Due", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("##Complete", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 25.0f);

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
            ImGui::TextWrapped(item == nullptr ? "Unknown!" : ItemTypeValue(item->type));
            ImGui::TableNextColumn(); 
            ImGui::Text(task->owner.c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(DateTime(task->startDate).toStringNiceNewline().c_str());
            ImGui::TableNextColumn(); 
            if (task->endDate.empty()) {
                ImGui::Text("-");
            }
            else {
                auto dueDate = DateTime(task->endDate);               
                if (dueDate.isBeforeNow()) {
                    ImGui::TextColored(colorRed, dueDate.toStringNiceNewline().c_str());
                }
                else {
                    ImGui::Text(dueDate.toStringNiceNewline().c_str());
                }
            }
            ImGui::TableNextColumn(); // Finish button
            if (iconCheck != nullptr) {
                ImGui::PushID(hashString("finish_" + std::to_string(task->id)));
                if (ImGui::ImageButton((ImTextureID)iconCheck->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                    task->completionDate = DateTime::nowLocal().toString();
                    task->completed = true;
                    organizerRepo->save();
                }
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Complete Task");
                    ImGui::EndTooltip();
                }
            }
            else {
                if (ImGui::Button(("Fin##" + std::to_string(task->id)).c_str(), { 20 * NexusLink->Scaling,20 * NexusLink->Scaling })) {
                    task->completionDate = DateTime::nowLocal().toString();
                    task->completed = true;
                    organizerRepo->save();
                }
            }
            ImGui::TableNextColumn(); // Delete button
            if (iconTrash != nullptr) {
                ImGui::PushID(hashString("remove_" + std::to_string(task->id)));
                if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                    task->deleted = true;
                    organizerRepo->save();
                }
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Delete Task");
                    ImGui::EndTooltip();
                }
            }
            else {
                if (ImGui::Button(("Del##" + std::to_string(task->id)).c_str(), {20 * NexusLink->Scaling,20 * NexusLink->Scaling })) {
                    task->deleted = true;
                    organizerRepo->save();
                }
            }
        }
        ImGui::EndTable();
    }
}
void renderAPITasks() {

    if (ImGui::CollapsingHeader("Wizards Vault")) {
        ImGui::SetCursorPosX(30);

        if (organizerRepo->wizardsVaultDaily.empty()) {
            ImGui::TextWrapped("No API key configured or data loading for account not successful.");
            ImGui::TextWrapped("Please go to the Settings tab and configure a valid API key with at least 'account' and 'progress' permissions!");
            ImGui::TextWrapped("(Note: data loading from API can be slow at times, this warning will disappear once Wizards Vault data has been successfully loaded)");
        }
        else {
            ApiTaskConfigurable* dailyWv = organizerRepo->getApiTaskConfigurableByOriginalId("wizardsvault_daily");
            if (dailyWv == nullptr || accountName.empty()) {
                ImGui::Text("Daily Meta achievement tracking not available at this time!");
            }
            else {
                if (dailyWv->accountConfiguration.count(accountName) == 0) {
                    dailyWv->accountConfiguration.emplace(accountName, false);
                }
                if (ImGui::Checkbox("Auto track daily meta achievement", &dailyWv->accountConfiguration[accountName])) {
                    organizerRepo->save();
                    APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
                }
            }
            ImGui::SetCursorPosX(30);
            ApiTaskConfigurable* weeklyWv = organizerRepo->getApiTaskConfigurableByOriginalId("wizardsvault_weekly");
            if (weeklyWv == nullptr || accountName.empty()) {
                ImGui::Text("Weekly Meta achievement tracking not available at this time!");
            }
            else {
                if (weeklyWv->accountConfiguration.count(accountName) == 0) {
                    weeklyWv->accountConfiguration.emplace(accountName, false);
                }
                if (ImGui::Checkbox("Auto track weekly meta achievement", &weeklyWv->accountConfiguration[accountName])) {
                    organizerRepo->save();
                    APIDefs->Events.RaiseNotification(EV_NAME_WEEKLY_RESET);
                }
            }

            for (auto progress : organizerRepo->wizardsVaultDaily) {
                ImGui::SetCursorPosX(30);
                if (ImGui::CollapsingHeader(("Progress for account: " + progress.first).c_str())) {
                    ImGui::SetCursorPosX(30);
                    if (ImGui::BeginTable(("WizardVaultTable" + progress.first).c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
                        ImGui::TableSetupColumn("Title", 0, 0.6f);
                        ImGui::TableSetupColumn("Track", 0, 0.05f);
                        ImGui::TableSetupColumn("Progress", 0, 0.15f);
                        ImGui::TableSetupColumn("Status", 0, 0.15f);
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
                            }
                        }

                        ImGui::EndTable();
                    }
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
                if (configurable == nullptr || accountName.empty()) {
                    // NO OP
                }
                else {
                    if (configurable->accountConfiguration.count(accountName) == 0) {
                        configurable->accountConfiguration.emplace(accountName, false);
                    }
                    if (ImGui::Checkbox(("Track category " + category.first.name).c_str(), &configurable->accountConfiguration[accountName])) {
                        organizerRepo->save();
                        APIDefs->Events.RaiseNotification(configurable->item.repeatMode == RepeatMode::WEEKLY ? EV_NAME_WEEKLY_RESET : EV_NAME_DAILY_RESET);
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
                        ImGui::Text("");
                        /*
                        * This segment is deactivated for now - single API Achievement tracking leads to a lot of issues because of how
                        * the achievement API works; it changes the achievement id each cycle for a lot of achievements. However, the "track" button
                        * would just subscribe to *that one specific* achievement (i.e. "Daily T4 Thaumanova") instead of the placeholder value (i.e. "Daily T4 Fractal 1/2/3")
                        * => this leads to the issue that the user would always get the same specific task (i.e. "Daily T4 Thaumanova") instead of the right one for the cycle
                        * This happens with Fractals, Strikes, Daily LWS/IBS/EoD, Weekly Rifts, leaving only Weekly WvW and Weekly Fractals left and for those... well, they
                        * have issues on their own already so leave it as is.
                        * 
                        * Future consideration might be a whitelist "achievement category that has static ids" or a "group by achievement category" kinda sanitazion and the 
                        * Task AutoStartService might read the current rotation to display the correct task; this could potentially lead to confusions however when a user
                        * does not complete the task and it sticks into the next cycle, remaining uncompletable at that point
                        * 
                        ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId("achievement_single_" + std::to_string(achievement.id));
                        if (configurable == nullptr || accountName.empty()) {
                            ImGui::Text("n/a");
                        }
                        else {
                            if (configurable->accountConfiguration.count(accountName) == 0) {
                                configurable->accountConfiguration.emplace(accountName, false);
                            }
                            if (ImGui::Checkbox(("Auto track##track_" + std::to_string(achievement.id)).c_str(), &configurable->accountConfiguration[accountName])) {
                                organizerRepo->save();
                            }
                        }
                        */
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
                if (configurable == nullptr || accountName.empty()) {
                    ImGui::Text("n/a");
                }
                else {
                    if (configurable->accountConfiguration.count(accountName) == 0) {
                        configurable->accountConfiguration.emplace(accountName, false);
                    }
                    if (ImGui::Checkbox(("Auto track##track_" + craftable).c_str(), &configurable->accountConfiguration[accountName])) {
                        organizerRepo->save();
                        APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
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
                if (configurable == nullptr || accountName.empty()) {
                    ImGui::Text("n/a");
                }
                else {
                    if (configurable->accountConfiguration.count(accountName) == 0) {
                        configurable->accountConfiguration.emplace(accountName, false);
                    }
                    if (ImGui::Checkbox(("Auto track##track_" + meta).c_str(), &configurable->accountConfiguration[accountName])) {
                        organizerRepo->save();
                        APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
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
                if (configurable == nullptr || accountName.empty()) {
                    ImGui::Text("n/a");
                }
                else {
                    if (configurable->accountConfiguration.count(accountName) == 0) {
                        configurable->accountConfiguration.emplace(accountName, false);
                    }
                    if (ImGui::Checkbox(("Auto track##track_" + boss).c_str(), &configurable->accountConfiguration[accountName])) {
                        organizerRepo->save();
                        APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
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
                if (configurable == nullptr || accountName.empty()) {
                    // NO OP
                }
                else {
                    if (configurable->accountConfiguration.count(accountName) == 0) {
                        configurable->accountConfiguration.emplace(accountName, false);
                    }
                    if (ImGui::Checkbox(("Track all paths of " + dungeonTranslator.at(dungeon.id)).c_str(), &configurable->accountConfiguration[accountName])) {
                        organizerRepo->save();
                        APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
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
                        if (configurable == nullptr || accountName.empty()) {
                            // NO OP
                        }
                        else {
                            if (configurable->accountConfiguration.count(accountName) == 0) {
                                configurable->accountConfiguration.emplace(accountName, false);
                            }
                            if (ImGui::Checkbox(("Auto track##track_" + path.id).c_str(), &configurable->accountConfiguration[accountName])) {
                                organizerRepo->save();
                                APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
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
                    if (configurable == nullptr || accountName.empty()) {
                        // NO OP
                    }
                    else {
                        if (configurable->accountConfiguration.count(accountName) == 0) {
                            configurable->accountConfiguration.emplace(accountName, false);
                        }
                        if (ImGui::Checkbox(("Track all bosses of " + raidTranslator.at(wing.id)).c_str(), &configurable->accountConfiguration[accountName])) {
                            organizerRepo->save();
                            APIDefs->Events.RaiseNotification(EV_NAME_WEEKLY_RESET);
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
                            if (configurable == nullptr || accountName.empty()) {
                                // NO OP
                            }
                            else {
                                if (configurable->accountConfiguration.count(accountName) == 0) {
                                    configurable->accountConfiguration.emplace(accountName, false);
                                }
                                if (ImGui::Checkbox(("Auto track##track_" + boss.id).c_str(), &configurable->accountConfiguration[accountName])) {
                                    organizerRepo->save(); 
                                    APIDefs->Events.RaiseNotification(EV_NAME_WEEKLY_RESET);
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
    int startAtTaskCount = (doneCurrentPage - 1) * doneItemsPerPage;
    int endAtTaskCount = doneCurrentPage * doneItemsPerPage;

    if (ImGui::BeginTable("DoneTasksTable", 8, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Started", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Completed", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("##Reactivate", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 25.0f);

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
            ImGui::TextWrapped(item == nullptr ? "Unknown!" : ItemTypeValue(item->type));
            ImGui::TableNextColumn(); 
            ImGui::TextWrapped(task->owner.c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(DateTime(task->startDate).toStringNiceNewline().c_str());
            ImGui::TableNextColumn(); 
            ImGui::Text(DateTime(task->completionDate).toStringNiceNewline().c_str());
            ImGui::TableNextColumn(); 
            if (iconReactivate != nullptr) {
                ImGui::PushID(hashString("react_" + std::to_string(task->id)));
                if (ImGui::ImageButton((ImTextureID)iconReactivate->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                    task->completed = false;
                    task->completionDate = "";
                    organizerRepo->save();
                }
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Reactivate Task");
                    ImGui::EndTooltip();
                }
            }
            else {
                if (ImGui::Button(("Reactivate##" + std::to_string(task->id)).c_str())) {
                    task->completed = false;
                    task->completionDate = "";
                    organizerRepo->save();
                }
            }
            ImGui::TableNextColumn(); 
            if (iconTrash != nullptr) {
                ImGui::PushID(hashString("remove_" + std::to_string(task->id)));
                if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                    task->deleted = true;
                    organizerRepo->save();
                }
                ImGui::PopID();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Delete Task");
                    ImGui::EndTooltip();
                }
            }
            else {
                if (ImGui::Button(("Del##" + std::to_string(task->id)).c_str(), { 20 * NexusLink->Scaling,20 * NexusLink->Scaling })) {
                    task->deleted = true;
                    organizerRepo->save();
                }
            }
        }
        ImGui::EndTable();
    }

    // I expect this table to get quite populated quite fast so TODO:
    // - add a drop down "Show last 10, 25, 50"
    // - add pagination
    int currentIndex = doneItemsPerPage == 10 ? 0 : doneItemsPerPage == 25 ? 1 : 2;
    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::Combo("Items per page", &currentIndex, itemsPerPageComboItems, IM_ARRAYSIZE(itemsPerPageComboItems))) {
        switch (currentIndex) {
        case 0: doneItemsPerPage = 10; break;
        case 1: doneItemsPerPage = 25; break;
        case 2: doneItemsPerPage = 50; break;
        default: doneItemsPerPage = 10;
        }
    }
    ImGui::SameLine();

    // Text "Page X of Y"
    int totalPagesAvailable = (totalAvailableTasks + doneItemsPerPage - 1) / doneItemsPerPage;
    std::string pagesText = "Page " + std::to_string(doneCurrentPage) + " of " + std::to_string(totalPagesAvailable);

    float textWidth = ImGui::CalcTextSize(pagesText.c_str()).x;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2);
    ImGui::Text(pagesText.c_str());
    // page buttons
    ImGui::SameLine();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2 - 25 * NexusLink->Scaling - 15 * NexusLink->Scaling);
    if (ImGui::Button("<", {25 * NexusLink->Scaling,25 * NexusLink->Scaling })) {
        doneCurrentPage--;
        if (doneCurrentPage == 0) doneCurrentPage = 1;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2 + (textWidth) + 15 * NexusLink->Scaling);
    if (ImGui::Button(">", {25 * NexusLink->Scaling,25 * NexusLink->Scaling })) {
        doneCurrentPage++;
        if (doneCurrentPage > totalPagesAvailable) doneCurrentPage = totalPagesAvailable;
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
    char bufferTaskFilter[256];
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    int totalAvailableTasks = 0;
    int startAtTaskCount = (configCurrentPage - 1) * configItemsPerPage;
    int endAtTaskCount = configCurrentPage * configItemsPerPage;

    if (ImGui::BeginTable("ConfigurableItemsTable", 8, ImGuiTableFlags_Borders)) {

        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Repeat Mode", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Interval", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("##StartTask", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##EditTask", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 25.0f);

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
#
            totalAvailableTasks++;
            if (totalAvailableTasks <= startAtTaskCount) continue; // still data of previous page
            if (totalAvailableTasks > endAtTaskCount) continue; // data of next page

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::TextWrapped(item->title.c_str());
            }
            else {
                char bufferTitle[256];
                strncpy_s(bufferTitle, editItem->title.c_str(), sizeof(bufferTitle));
                ImGui::PushItemWidth(-FLT_MIN); // Use remaining space for the item
                if (ImGui::InputText("##Title", bufferTitle, sizeof(bufferTitle))) {
                    editItem->title = bufferTitle;
                }
                ImGui::PopItemWidth();
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::TextWrapped(item->description.c_str());
            }
            else {
                char bufferDesc[256];
                strncpy_s(bufferDesc, editItem->description.c_str(), sizeof(bufferDesc));
                ImGui::PushItemWidth(-FLT_MIN); // Use remaining space for the item
                if (ImGui::InputTextMultiline("##Desc", bufferDesc, sizeof(bufferDesc))) {
                    editItem->description = bufferDesc;
                }
                ImGui::PopItemWidth();
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::Text(ItemTypeValue(item->type));
            }
            else {
                int typeValue = static_cast<int>(editItem->type);
                ImGui::PushItemWidth(-FLT_MIN); // Use remaining space for the item
                if (ImGui::Combo("##Type", &typeValue, ItemTypeCombo, IM_ARRAYSIZE(ItemTypeCombo))) {
                    editItem->type = convertToType(typeValue);
                }
                ImGui::PopItemWidth();
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::Text(RepeatModeValue(item->repeatMode));
            }
            else {
                int repeatValue = static_cast<int>(editItem->repeatMode);
                ImGui::PushItemWidth(-FLT_MIN); // Use remaining space for the item
                if (ImGui::Combo("##RepeatMode", &repeatValue, RepeatModeCombo, IM_ARRAYSIZE(RepeatModeCombo))) {
                    editItem->repeatMode = static_cast<RepeatMode>(repeatValue);
                }
                ImGui::PopItemWidth();
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::Text(std::to_string(item->customRepeatInterval).c_str());
            }
            else {
                ImGui::PushItemWidth(-FLT_MIN); // Use remaining space for the item
                if (ImGui::InputInt("##Interval", &editItem->customRepeatInterval)) {
                    if (editItem->customRepeatInterval < 0) editItem->customRepeatInterval = 0;
                }
                ImGui::PopItemWidth();
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                if (item->repeatMode == RepeatMode::ONCE) {
                    // Single time task, begin "start task" routine
                    if (iconStart != nullptr) {
                        ImGui::PushID(hashString("start_" + std::to_string(item->id)));
                        if (ImGui::ImageButton((ImTextureID)iconStart->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {

                            renderAddNew = true;
                            newItem = OrganizerItem(*item);

                            newInstance = {};
                            newInstance.startDate = DateTime::nowLocal().toString();
                            newInstance.endDate = "";
                            newInstance.completed = false;
                            newInstance.itemId = item->id;
                            selected_tab = 0;

                        }
                        ImGui::PopID();
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Start Task");
                            ImGui::EndTooltip();
                        }
                    }
                    else {
                        if (ImGui::Button(("Start##" + std::to_string(item->id)).c_str())) {
                            renderAddNew = true;
                            newItem = OrganizerItem(*item);

                            newInstance = {};
                            newInstance.startDate = DateTime::nowLocal().toString();
                            newInstance.endDate = "";
                            newInstance.completed = false;
                            newInstance.itemId = item->id;
                            selected_tab = 0;
                        }
                    }
                }
                else if(!accountName.empty() && item->repeatMode != RepeatMode::ONCE) {
                    // repeatable task, make the button subscribe / unsubscribe depending on subscription status!
                    if (!item->accountConfiguration.contains(accountName)) {
                        item->accountConfiguration.emplace(accountName, false);
                    }
                    Texture* btnTex = nullptr; // TODO
                    bool subscribe;
                    if(item->accountConfiguration[accountName]) {
                        // is subscribed, btnTex should be "unsubscribe"
                        btnTex = iconCancel;
                        subscribe = false;
                    }
                    else {
                        // is unsubscribed, btnTex should be "subscribe"
                        btnTex = iconStart;
                        subscribe = true;
                    }

                    if (btnTex != nullptr) {
                        ImGui::PushID(hashString("subscribe_" + std::to_string(item->id)));
                        if (ImGui::ImageButton((ImTextureID)btnTex->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                            item->accountConfiguration[accountName] = subscribe;
                            organizerRepo->save();
                            APIDefs->Events.RaiseNotification(item->repeatMode == RepeatMode::WEEKLY ? EV_NAME_WEEKLY_RESET : EV_NAME_DAILY_RESET);
                        }
                        ImGui::PopID();
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text(subscribe ? "Subscribe" : "Unsubscribe");
                            ImGui::EndTooltip();
                        }
                    }
                    else {
                        if (ImGui::Button(((subscribe ? "Subscribe" : "Unsubscribe") + std::string("##") + std::to_string(item->id)).c_str())) {
                            item->accountConfiguration[accountName] = subscribe;
                            organizerRepo->save();
                            APIDefs->Events.RaiseNotification(item->repeatMode == RepeatMode::WEEKLY ? EV_NAME_WEEKLY_RESET : EV_NAME_DAILY_RESET);
                        }
                    }
                }
            }
            else {
                if (iconSave != nullptr) {
                    ImGui::PushID(hashString("save_" + std::to_string(item->id)));
                    if (ImGui::ImageButton((ImTextureID)iconSave->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                        item->title = editItem->title;
                        item->description = editItem->description;
                        item->type = editItem->type;
                        item->repeatMode = editItem->repeatMode;
                        item->customRepeatInterval = editItem->customRepeatInterval;
                        editItem = nullptr;
                        organizerRepo->save();
                    }
                    ImGui::PopID();
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Save");
                        ImGui::EndTooltip();
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
            }
            ImGui::TableNextColumn();
            if (editItem == nullptr || editItem->id != item->id) {
                if (iconEdit != nullptr) {
                    ImGui::PushID(hashString("edit_" + std::to_string(item->id)));
                    if (ImGui::ImageButton((ImTextureID)iconEdit->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                        editItem = new OrganizerItem(*item);
                    }
                    ImGui::PopID();
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Edit configuration");
                        ImGui::EndTooltip();
                    }
                }
                else {
                    if (ImGui::Button(("Edit##" + std::to_string(item->id)).c_str())) {
                        editItem = new OrganizerItem(*item);
                    }
                }
            }
            else {
                if (iconCancel != nullptr) {
                    ImGui::PushID(hashString("cancel_" + std::to_string(item->id)));
                    if (ImGui::ImageButton((ImTextureID)iconCancel->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                        editItem = nullptr;
                    }
                    ImGui::PopID();
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Cancel");
                        ImGui::EndTooltip();
                    }
                }
                else {
                    if (ImGui::Button(("Cancel##" + std::to_string(item->id)).c_str())) {
                        editItem = nullptr;
                    }
                }
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                if (iconTrash != nullptr) {
                    ImGui::PushID(hashString("remove_" + std::to_string(item->id)));
                    if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, { 20 * NexusLink->Scaling,20 * NexusLink->Scaling }, { 0,0 }, { 1,1 })) {
                        item->deleted = true;
                        organizerRepo->save();
                    }
                    ImGui::PopID();
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Delete configuration");
                        ImGui::EndTooltip();
                    }
                }
                else {
                    if (ImGui::Button(("Remove##" + std::to_string(item->id)).c_str())) {
                        item->deleted = true;
                        organizerRepo->save();
                    }
                }
            }
            else {

            }
        }
        ImGui::EndTable();
    }

    int currentIndex = configItemsPerPage == 10 ? 0 : configItemsPerPage == 25 ? 1 : 2;
    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::Combo("Items per page", &currentIndex, itemsPerPageComboItems, IM_ARRAYSIZE(itemsPerPageComboItems))) {
        switch (currentIndex) {
        case 0: configItemsPerPage = 10; break;
        case 1: configItemsPerPage = 25; break;
        case 2: configItemsPerPage = 50; break;
        default: configItemsPerPage = 10;
        }
    }
    ImGui::SameLine();

    // Text "Page X of Y"
    int totalPagesAvailable = (totalAvailableTasks + configItemsPerPage - 1) / configItemsPerPage;
    std::string pagesText = "Page " + std::to_string(configCurrentPage) + " of " + std::to_string(totalPagesAvailable);

    float textWidth = ImGui::CalcTextSize(pagesText.c_str()).x;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2);
    ImGui::Text(pagesText.c_str());
    // page buttons
    ImGui::SameLine();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2 - 25 * NexusLink->Scaling - 15 * NexusLink->Scaling);
    if (ImGui::Button("<", { 25 * NexusLink->Scaling,25 * NexusLink->Scaling })) {
        configCurrentPage--;
        if (configCurrentPage == 0) configCurrentPage = 1;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX((windowWidth - textWidth) / 2 + (textWidth)+15 * NexusLink->Scaling);
    if (ImGui::Button(">", { 25 * NexusLink->Scaling,25 * NexusLink->Scaling })) {
        configCurrentPage++;
        if (configCurrentPage > totalPagesAvailable) configCurrentPage = totalPagesAvailable;
    }

    ImGui::Separator();

    ImGui::Text("Tasks configured by Guild Wars 2 API");
    ImGui::Text("(Only tracked ones will show up here. Configure tracked tasks in the 'Ingame Activities' tab.");

    if (ImGui::BeginTable("APIItemsTable", 4, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Repeat Mode", ImGuiTableColumnFlags_WidthFixed, 100.0f);

        ImGui::TableHeadersRow();

        for (auto configurable : organizerRepo->getApiTaskConfigurables()) {
            // Check if task is configured for this account
            bool configured = false;
            if (!accountName.empty() && configurable->accountConfiguration.count(accountName)) {
                configured = configurable->accountConfiguration[accountName];
            }
            if (!configured) continue;

            // Check if task meets filter criteria
            if (!tableFilter.empty()) {
                bool found = false;
                if (strContains(configurable->item.title, tableFilter)) found = true;
                if (strContains(configurable->item.description, tableFilter)) found = true;
                if (strContains(std::string(ItemTypeValue(configurable->item.type)), tableFilter)) found = true;
                if (strContains(std::string(RepeatModeValue(configurable->item.repeatMode)), tableFilter)) found = true;
                if (!found) continue;
            }

            // All checks passed, print data
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
void renderSettings() {
    ImGui::Separator();
    static char newKeyBuffer[128] = "";
    static char newDescriptionBuffer[256] = "";

    ImGui::InputText("New API Key", newKeyBuffer, IM_ARRAYSIZE(newKeyBuffer));
    ImGui::InputText("Identifier", newDescriptionBuffer, IM_ARRAYSIZE(newDescriptionBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Add Key")) {
        std::string newKey(newKeyBuffer);
        std::string newDescription(newDescriptionBuffer);
        if (!newKey.empty() && !newDescription.empty()) {
            addon::ApiKey apiKey = { newKey, newDescription };
            settings.apiKeys.push_back(apiKey);

            newKeyBuffer[0] = '\0';  // Clear the input buffer
            newDescriptionBuffer[0] = '\0';  // Clear the input buffer

            StoreSettings();
        }
    }

    // Display existing keys with descriptions and remove button in a table
    ImGui::Separator();
    if (ImGui::BeginTable("APIKeysTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Identifier", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn("API Key", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##Permissions", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (auto it = settings.apiKeys.begin(); it != settings.apiKeys.end(); ) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text(it->identifier.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped(maskApiKey(it->apiKey).c_str());
            ImGui::TableSetColumnIndex(2);
            gw2::token::ApiToken* token = apiTokenService.getToken(it->identifier);
            if (token != nullptr) {
                if (ImGui::CollapsingHeader(("Permissions...##" + it->identifier).c_str())) {
                    std::stringstream stream;
                    for (auto perm : token->permissions) {
                        stream << perm << std::endl;
                    }
                    ImGui::TextWrapped("%s", stream.str().c_str());
                }
            }
            else {
                ImGui::TextWrapped("%s", "No token info available at this time.");
            }
            
            ImGui::TableSetColumnIndex(3);
            if (ImGui::Button(("Remove##" + it->identifier).c_str())) {
                it = settings.apiKeys.erase(it);  // Remove the key and get the next iterator
                StoreSettings();
            }
            else {
                ++it;  // Move to the next iterator
            }
        }

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::Checkbox("Enable Notifications", &settings.notifications.enabled)) {
        StoreSettings();
    }
    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::InputInt("Notification X minutes before", &settings.notifications.minutesUntilDue)) {
        StoreSettings();
    }

    ImGui::Text("Notifications position (X:Y)");

    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::InputInt("##NotificationPosX", &settings.notifications.x)) {
        StoreSettings();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::InputInt("##NotificationPosY", &settings.notifications.y)) {
        StoreSettings();
    }

    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::InputInt("Width", &settings.notifications.width)) {
        StoreSettings();
    }
    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::InputInt("Height", &settings.notifications.height)) {
        StoreSettings();
    }

    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    int durationSec = settings.notifications.duration / 1000;
    if (ImGui::InputInt("Duration (sec.)", &durationSec)) {
        settings.notifications.duration = durationSec * 1000;
        StoreSettings();
    }
    // TODO growth direction, Border Color

    if (ImGui::Button("Test Notifications")) {
        toast::ToastData* data = new toast::ToastData();
        data->toastId = "Sample_" + std::to_string(MumbleLink->UITick);
        data->title = "Test Notification";
        data->text = "This is a sample notification.";
        notificationService.queueToast(data);
    }

    ImGui::Separator();
    
    if (ImGui::Button("Trigger Daily Reset")) {
        APIDefs->Events.RaiseNotification(EV_NAME_DAILY_RESET);
    }
    ImGui::SameLine();
    if (ImGui::Button("Trigger Weekly Reset")) {
        APIDefs->Events.RaiseNotification(EV_NAME_WEEKLY_RESET);
    }
    
}