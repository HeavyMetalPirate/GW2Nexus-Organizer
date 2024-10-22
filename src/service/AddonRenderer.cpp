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
void renderStatistics();
void renderSettings();
/* Misc rendering proto */
void renderNewTaskDialog();

/* control flags */        
bool todoListPinned = false;

static int selected_tab = 0;

bool renderAddNew = false;
bool renderAddNewDialog = false;
bool renderChangeInterval = false;

bool showDeletedItems = false;
bool showDeletedInstances = false;

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
const ImVec4 colorRed = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
const ImVec4 colorGreen = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
const ImVec4 colorGrey = ImVec4(0.37f, 0.37f, 0.37f, 0.7f);

const static int tab_active = 0;
const static int tab_ingame_activities = 1;
const static int tab_done = 2;
const static int tab_task_settings = 3;
const static int tab_statistics = 4;
const static int tab_organizer_settings = 5;

ImVec2 imageButtonSize;

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
Texture* iconSubscribe = nullptr;
Texture* iconPin = nullptr;

/* Pagination */
static const char* itemsPerPageComboItems[] = { "10", "25", "50" };
int doneItemsPerPage = 10;
int doneCurrentPage = 1;
int configItemsPerPage = 10;
int configCurrentPage = 1;

/* Statistics control flags */
auto statisticsStart = DateTime::fromTodayAt(0, 0).subtractDays(7);
bool renderStatisticsStartPicker = false;
auto statisticsEnd = DateTime::fromTodayAt(0, 0);
bool renderStatisticsEndPicker = false;
ChartType statisticsChartType = ChartType::BAR_CHART;
std::map<int, bool> statisticsFilter = std::map<int, bool>();

void Renderer::preRender() {
    // Set button size for image buttons
    imageButtonSize = ImVec2(28 * NexusLink->Scaling, 28 * NexusLink->Scaling);

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
    if (iconSubscribe == nullptr)
        iconSubscribe = APIDefs->Textures.Get("ICON_ORGANIZER_SUBSCRIBE");
    if (iconPin == nullptr)
        iconPin = APIDefs->Textures.Get("ICON_ORGANIZER_PIN");


    if (accountName.empty()) displayOwnOnly = false;
}
void Renderer::render() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImVec2 oldPadding = style.FramePadding;
    //ImVec2 oldSpacing = style.ItemSpacing;
    //ImVec2 oldInnerSpacing = style.ItemInnerSpacing;
    float oldBorderSize = style.FrameBorderSize;

    style.FramePadding = ImVec2(0, 0);  
    //style.ItemSpacing = ImVec2(0, 0);   
    //style.ItemInnerSpacing = ImVec2(0, 0);
    style.FrameBorderSize = 0.0f;

    try {
        renderNewTaskDialog();
    }
    catch (const std::exception& e) {
        Log(ELogLevel_CRITICAL, ADDON_NAME, "AddonRenderer::Render():RenderNewTaskDialog(): " + std::string(e.what()));
    }
    catch (...) {
        Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling AddonRenderer::Render():RenderNewTaskDialog()");
    }
    try {
        renderTodoList();
    }
    catch (const std::exception& e) {
        Log(ELogLevel_CRITICAL, ADDON_NAME, "AddonRenderer::Render():RenderTodoList(): " + std::string(e.what()));
    }
    catch (...) {
        Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling AddonRenderer::Render():RenderTodoList()");
    }
    try {
        renderOrganizer();
    }
    catch (const std::exception& e) {
        Log(ELogLevel_CRITICAL, ADDON_NAME, "AddonRenderer::Render():RenderOrganizer(): " + std::string(e.what()));
    }
    catch (...) {
        Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling AddonRenderer::Render():RenderOrganizer()");
    }

    style.FramePadding = oldPadding;
    //style.ItemSpacing = oldSpacing;
    //style.ItemInnerSpacing = oldInnerSpacing;
    style.FrameBorderSize = oldBorderSize;
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

        static char bufferTitle[256] = "";
        strncpy_s(bufferTitle, newItem.title.c_str(), sizeof(bufferTitle));
        if (ImGui::InputText("Title", bufferTitle, sizeof(bufferTitle))) {
            newItem.title = bufferTitle;
        }
        static char bufferDesc[2048] = "";
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
    if (!todoListRendered && !todoListPinned) return;

    ImGui::SetNextWindowSize(ImVec2(370.0f, 350.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("TODOs", &todoListRendered, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
                                                | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        // Render custom title bar first... sigh
        ImGui::PushFont((ImFont*)NexusLink->FontBig);
        ImGui::Text("TODOs");
        ImGui::PopFont();
        ImGui::SameLine(ImGui::GetWindowWidth() - (4 * (32 * NexusLink->Scaling) + 5)); // 3* buttons plus 5 generic to the edge
        if (iconAdd != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconAdd->Resource, imageButtonSize)) {
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
            if (ImGui::ImageButton((ImTextureID)iconOptions->Resource, imageButtonSize)) {
                organizerRendered = !organizerRendered;
            }
        }
        else {
            if (ImGui::Button("O", { 20,20 })) {
                organizerRendered = !organizerRendered;
            }
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, todoListPinned ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
        if (iconPin != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconPin->Resource, imageButtonSize)) {
                todoListPinned = !todoListPinned;
                // (De)Register Close On Escape
                // if we don't deregister, one tap on ESC will be "wasted" because Nexus still toggles the flag but changes nothing
                // This oughta help improve the user experience
                if (todoListPinned) {
                    APIDefs->UI.DeregisterCloseOnEscape("TODOs");
                }
                else {
                    APIDefs->UI.RegisterCloseOnEscape("TODOs", &todoListRendered);
                }
            }
        }
        else {
            if (ImGui::Button("Pin", { 20,20 })) {
                todoListPinned = !todoListPinned;
                if (todoListPinned) {
                    APIDefs->UI.DeregisterCloseOnEscape("TODOs");
                }
                else {
                    APIDefs->UI.RegisterCloseOnEscape("TODOs", &todoListRendered);
                }
            }
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (iconClose != nullptr) {
            if (ImGui::ImageButton((ImTextureID)iconClose->Resource, imageButtonSize)) {
                todoListRendered = false;
                todoListPinned = false;
            }
        }
        else {
            if (ImGui::Button("X", { 20,20 })) {
                todoListRendered = false;
                todoListPinned = false;
            }
        }
        ImGui::Separator();
        
        // Finally, content.
        static char bufferNewTask[256] = "";
        ImGui::InputText("##TaskName", bufferNewTask, IM_ARRAYSIZE(bufferNewTask));
        ImGui::SameLine();
        if (ImGui::Button("Create Task")) {
            newTaskName = bufferNewTask;
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
                strncpy_s(bufferNewTask, newTaskName.c_str(), sizeof(bufferNewTask));
            }
        }

        if (ImGui::CollapsingHeader("Filter")) {
            static char bufferTaskFilter[256] = "";
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
            if (ImGui::InputText("##Filter", bufferTaskFilter, IM_ARRAYSIZE(bufferTaskFilter))) {
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

            if (ImGui::BeginTable("TODOTable", 4, ImGuiTableFlags_Sortable | ImGuiTableFlags_BordersH )) {
                ImGui::TableSetupColumn("##ChangeInterval", ImGuiTableColumnFlags_WidthFixed, 30.0f * NexusLink->Scaling);
                ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthFixed, leftColumnWidth - (30.0f * NexusLink->Scaling));
                ImGui::TableSetupColumn("##Complete", ImGuiTableColumnFlags_WidthFixed, rightColumnWidth);
                ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, rightColumnWidth);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
                    if (sortSpecs->SpecsDirty) {
                        const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
                        if (spec.ColumnIndex == 1) {
                            organizerRepo->UpdateSortSpecs(SortProperty::NAME, spec.SortDirection == ImGuiSortDirection_Ascending);
                            sortSpecs->SpecsDirty = false;
                        }
                    }
                }

                auto tasklist = organizerRepo->getTaskInstances();
                for (auto task : tasklist) {
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
                            if (ImGui::ImageButton(buttonTex, imageButtonSize, { 0,0 }, { 1,1 })) {
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

                    ImGui::TableNextColumn(); // Finish button
                    if (iconCheck != nullptr) {
                        ImGui::PushID(hashString("finish_" + std::to_string(task->id)));
                        if (ImGui::ImageButton((ImTextureID)iconCheck->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
                        if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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

                    if (currentEnum == RepeatMode::CUSTOM) {
                        // we do something entirely different here because the settings are super complex: 
                        // open the configuration at configs tab with the item in edit
                        // for this we need to do the following:
                        // - set editItem to changeIntervalItem
                        editItem = new OrganizerItem(*changeIntervalItem);
                        editItem->repeatMode = RepeatMode::CUSTOM;
                        // - figure out on which page of the pagination it actually is and set that flag accordingly
                        int idx = 0;
                        for (auto item : organizerRepo->getConfigurableItems()) {
                            if (item->id == editItem->id) break;
                            idx++;
                        }
                        configCurrentPage = (idx / configItemsPerPage) + 1;
                        // - set the selected_tab flag
                        selected_tab = tab_task_settings;
                        // - set the render config flag
                        organizerRendered = true;
                    }
                    else {
                        if (!changeIntervalItem->accountConfiguration.contains(accountName) && !accountName.empty()) {
                            changeIntervalItem->accountConfiguration.emplace(accountName, currentEnum == RepeatMode::ONCE ? false : true);
                        }
                        else if (!accountName.empty()) {
                            changeIntervalItem->accountConfiguration[accountName] = currentEnum == RepeatMode::ONCE ? false : true;
                        }
                        changeIntervalItem->repeatMode = currentEnum;
                        organizerRepo->save();
                    }
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

            if (CardTab("Open Tasks", selected_tab == tab_active)) selected_tab = tab_active;
            if (CardTab("Ingame activities", selected_tab == tab_ingame_activities)) selected_tab = tab_ingame_activities;
            if (CardTab("Completed Tasks", selected_tab == tab_done)) selected_tab = tab_done;
            if (CardTab("Task configuration", selected_tab == tab_task_settings)) selected_tab = tab_task_settings;
            if (CardTab("Statistics", selected_tab == tab_statistics)) selected_tab = tab_statistics;
            if (CardTab("Settings", selected_tab == tab_organizer_settings)) selected_tab = tab_organizer_settings;

            ImGui::EndChild();
        }

        // Create a child window for the content on the right
        ImGui::SameLine();
        if (ImGui::BeginChild("Right Pane", ImVec2(0, 0), true)) {
            try {
                switch (selected_tab) {
                case tab_active: renderCurrentTasks(); break;
                case tab_ingame_activities: renderAPITasks(); break;
                case tab_done: renderDoneTasks(); break;
                case tab_task_settings: renderTaskConfiguration(); break;
                case tab_organizer_settings: renderSettings(); break;
                case tab_statistics: renderStatistics(); break;
                default: ImGui::Text(("Unknown Tab: " + std::to_string(selected_tab)).c_str());
                }
            }
            catch (const std::exception& e) {
                Log(ELogLevel_CRITICAL, ADDON_NAME, "AddonRenderer::RenderOrganizer() with selected_tab " + std::to_string(selected_tab) + ": " + std::string(e.what()));
            }
            catch (...) {
                Log(ELogLevel_CRITICAL, ADDON_NAME, "Unknown exception while calling AddonRenderer::RenderOrganizer() with selected_tab " + std::to_string(selected_tab));
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

        static char bufferTitle[256] = "";
        strncpy_s(bufferTitle, newItem.title.c_str(), sizeof(bufferTitle));
        if (ImGui::InputText("Title", bufferTitle, sizeof(bufferTitle))) {
            newItem.title = bufferTitle;
        }
        static char bufferDesc[2048] = "";
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

    static char bufferTaskFilter[256] = "";
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    ImGui::Checkbox("Show deleted tasks", &showDeletedInstances);

    if (ImGui::BeginTable("CurrentTasksTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Started", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Due", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("##Complete", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 25.0f);

        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty) {
                const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
                SortProperty property = SortProperty::NAME;
                switch (spec.ColumnIndex) {
                case 0: property = SortProperty::NAME; break;
                case 1: property = SortProperty::DESCRIPTION; break;
                case 2: property = SortProperty::TYPE; break;
                case 3: property = SortProperty::OWNER; break;
                case 4: property = SortProperty::START_DATE; break;
                case 5: property = SortProperty::END_DATE; break;
                default: property = SortProperty::NAME;
                }
                organizerRepo->UpdateSortSpecs(property, spec.SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;
            }
        }

        for (auto task : organizerRepo->getTaskInstances()) {
            if (task->completed || (task->deleted && !showDeletedInstances)) continue;
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
            if (task->deleted) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(colorGrey));
            }

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
            ImGui::TableNextColumn(); 
            if (task->deleted) {
                ImGui::Text("");
            }
            else {
                // Finish button
                if (iconCheck != nullptr) {
                    ImGui::PushID(hashString("finish_" + std::to_string(task->id)));
                    if (ImGui::ImageButton((ImTextureID)iconCheck->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
                    if (ImGui::Button(("Fin##" + std::to_string(task->id)).c_str(), imageButtonSize)) {
                        task->completionDate = DateTime::nowLocal().toString();
                        task->completed = true;
                        organizerRepo->save();
                    }
                }
            }
            ImGui::TableNextColumn(); // Delete button
            if (task->deleted) {
                if (iconReactivate != nullptr) {
                    ImGui::PushID(hashString("react_" + std::to_string(task->id)));
                    if (ImGui::ImageButton((ImTextureID)iconReactivate->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
                        task->deleted = false;
                        organizerRepo->save();
                    }
                    ImGui::PopID();
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Restore Task");
                        ImGui::EndTooltip();
                    }
                }
                else {
                    if (ImGui::Button(("Restore##" + std::to_string(task->id)).c_str())) {
                        task->deleted = false;
                        organizerRepo->save();
                    }
                }
            }
            else {
                if (iconTrash != nullptr) {
                    ImGui::PushID(hashString("remove_" + std::to_string(task->id)));
                    if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
                    if (ImGui::Button(("Del##" + std::to_string(task->id)).c_str(), imageButtonSize)) {
                        task->deleted = true;
                        organizerRepo->save();
                    }
                }
            }
        }
        ImGui::EndTable();
    }
}
void renderAPITasks() {
    ImGui::TextWrapped("Auto tracking completion via GW2 API:");
    ImGui::SameLine();
    if (autotrackActive) {
        ImGui::TextColored(colorGreen, "Active");
    }
    else {
        ImGui::TextColored(colorRed, "Stopped");
    }

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
                ImGui::TextWrapped(getCraftableName(craftable).c_str()); // TODO translate to human readable value instead of API value!
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
                ImGui::TextWrapped(getMetaName(meta).c_str());
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
                ImGui::TextWrapped(getWorldBossName(boss).c_str());
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
            if (ImGui::CollapsingHeader(getDungeonName(dungeon.id).c_str())) {
                ImGui::SetCursorPosX(30);
                ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(dungeon.id);
                if (configurable == nullptr || accountName.empty()) {
                    // NO OP
                }
                else {
                    if (configurable->accountConfiguration.count(accountName) == 0) {
                        configurable->accountConfiguration.emplace(accountName, false);
                    }
                    if (ImGui::Checkbox(("Track all paths of " + getDungeonName(dungeon.id)).c_str(), &configurable->accountConfiguration[accountName])) {
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
                        ImGui::TextWrapped(getDungeonPathName(path.id).c_str());
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
                if (ImGui::CollapsingHeader(getRaidName(wing.id).c_str())) {
                    ImGui::SetCursorPosX(30);
                    ApiTaskConfigurable* configurable = organizerRepo->getApiTaskConfigurableByOriginalId(wing.id);
                    if (configurable == nullptr || accountName.empty()) {
                        // NO OP
                    }
                    else {
                        if (configurable->accountConfiguration.count(accountName) == 0) {
                            configurable->accountConfiguration.emplace(accountName, false);
                        }
                        if (ImGui::Checkbox(("Track all bosses of " + getRaidName(wing.id)).c_str(), &configurable->accountConfiguration[accountName])) {
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
                            ImGui::TextWrapped(getRaidBossName(boss.id).c_str());
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
    static char bufferTaskFilter[256] = "";
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    int totalAvailableTasks = 0;
    int startAtTaskCount = (doneCurrentPage - 1) * doneItemsPerPage;
    int endAtTaskCount = doneCurrentPage * doneItemsPerPage;

    if (ImGui::BeginTable("DoneTasksTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Started", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Completed", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("##Reactivate", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 25.0f);

        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty) {
                const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
                SortProperty property = SortProperty::NAME;
                switch (spec.ColumnIndex) {
                case 0: property = SortProperty::NAME; break;
                case 1: property = SortProperty::DESCRIPTION; break;
                case 2: property = SortProperty::TYPE; break;
                case 3: property = SortProperty::OWNER; break;
                case 4: property = SortProperty::START_DATE; break;
                case 5: property = SortProperty::END_DATE; break;
                default: property = SortProperty::NAME;
                }
                organizerRepo->UpdateSortSpecs(property, spec.SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;
            }
        }

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
                if (ImGui::ImageButton((ImTextureID)iconReactivate->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
                if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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

    // I expect this table to get quite populated quite fast so:
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
}
void renderTaskConfiguration() {
    static char bufferTaskFilter[256] = "";
    strncpy_s(bufferTaskFilter, tableFilter.c_str(), sizeof(bufferTaskFilter));
    if (ImGui::InputText("Filter Table", bufferTaskFilter, sizeof(bufferTaskFilter))) {
        tableFilter = bufferTaskFilter;
    }

    ImGui::Checkbox("Show deleted configurations", &showDeletedItems);

    int totalAvailableTasks = 0;
    int startAtTaskCount = (configCurrentPage - 1) * configItemsPerPage;
    int endAtTaskCount = configCurrentPage * configItemsPerPage;

    if (ImGui::BeginTable("ConfigurableItemsTable", 8, ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Repeat Mode", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Interval", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("##StartTask", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##EditTask", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 25.0f);

        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty) {
                const ImGuiTableColumnSortSpecs& spec = sortSpecs->Specs[0];
                SortProperty property = SortProperty::NAME;
                switch (spec.ColumnIndex) {
                case 0: property = SortProperty::NAME; break;
                case 1: property = SortProperty::DESCRIPTION; break;
                case 2: property = SortProperty::TYPE; break;
                case 3: property = SortProperty::REPEAT_MODE; break;
                default: property = SortProperty::NAME;
                }
                organizerRepo->UpdateSortSpecs(property, spec.SortDirection == ImGuiSortDirection_Ascending);
                sortSpecs->SpecsDirty = false;
            }
        }

        for (auto item : organizerRepo->getConfigurableItems()) {
            if (item->deleted && !showDeletedItems) continue;

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
            if (item->deleted) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(colorGrey));
            }

            ImGui::TableNextColumn();
            if (editItem == nullptr || editItem->id != item->id) {
                ImGui::TextWrapped(item->title.c_str());
            }
            else {
                static char bufferTitle[256] = "";
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
                static char bufferDesc[256] = "";
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
                if (item->repeatMode == RepeatMode::CUSTOM) {
                    ImGui::Text(item->intervalMode == 0 ? "Day of Week:" : "Day of Month:");
                    if (item->intervalMode == 0) {
                        std::vector<int> sortedDays = item->daysOfWeek;
                        std::sort(sortedDays.begin(), sortedDays.end(), [](int a, int b) {
                            return a < b;
                        });
                        std::stringstream dayListStream;
                        for (size_t i = 0; i < sortedDays.size(); ++i) {
                            if (i > 0) {
                                dayListStream << ", ";
                            }
                            dayListStream << daysOfWeekItems[sortedDays[i]];
                        }
                        std::string dayList = dayListStream.str();
                        ImGui::TextWrapped("Selected Days of Week: %s", dayList.c_str());

                    } // could be just "else" right now but maybe we support more modes later down the road
                    else if (item->intervalMode == 1) {
                        std::vector<int> sortedDays = item->daysOfMonth;
                        std::sort(sortedDays.begin(), sortedDays.end(), [](int a, int b) {
                            return a < b;
                            });
                        std::stringstream dayListStream;
                        for (size_t i = 0; i < sortedDays.size(); ++i) {
                            if (i > 0) {
                                dayListStream << ", ";                              }
                            if (sortedDays[i] == 99) {
                                dayListStream << "Ultimo"; 
                            }
                            else {
                                dayListStream << sortedDays[i];
                            }
                        }
                        std::string dayList = dayListStream.str();
                        ImGui::TextWrapped("%s", dayList.c_str());
                    }
                    ImGui::Text("");
                    ImGui::Text("Due Time: ");
                    ImGui::SameLine();
                    std::stringstream timeStream;
                    timeStream << std::setfill('0') << std::setw(2) << item->dueHours << ":"
                        << std::setfill('0') << std::setw(2) << item->dueMinutes;
                    std::string timeString = timeStream.str();
                    ImGui::Text("%s", timeString.c_str());

                }
                else {
                    ImGui::Text("-");
                }
            }
            else {
                if (editItem->repeatMode == RepeatMode::CUSTOM) {
                    ImGui::PushItemWidth(-FLT_MIN); // Use remaining space for the item
                    if (ImGui::Combo("Interval Mode", &editItem->intervalMode, intervalModeItems, IM_ARRAYSIZE(intervalModeItems))) {
                        // Change the mode
                        // Potentially clear the vectors when switching modes
                        editItem->daysOfWeek.clear();
                        editItem->daysOfMonth.clear();
                    }

                    // Days of Week Input List
                    if (editItem->intervalMode == 0) {
                        std::vector<int> sortedDays = editItem->daysOfWeek;
                        std::sort(sortedDays.begin(), sortedDays.end(), [](int a, int b) {
                            return a < b;
                        });
                        std::stringstream dayListStream;
                        for (size_t i = 0; i < sortedDays.size(); ++i) {
                            if (i > 0) {
                                dayListStream << ", ";
                            }
                            dayListStream << daysOfWeekItems[sortedDays[i]];
                        }
                        std::string dayList = dayListStream.str();
                        ImGui::TextWrapped("Selected Days of Week: %s", dayList.c_str());

                        // Popup to select days of the week
                        if (ImGui::Button("Select Days")) {
                            ImGui::OpenPopup("SelectDaysOfWeekPopup");
                        }
                        if (ImGui::BeginPopup("SelectDaysOfWeekPopup")) {
                            for (int i = 0; i < 7; ++i) {
                                bool isSelected = std::find(editItem->daysOfWeek.begin(), editItem->daysOfWeek.end(), i) != editItem->daysOfWeek.end();

                                // Highlight selected days
                                if (isSelected) {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
                                }
                                if (ImGui::Button(daysOfWeekItems[i])) {
                                    if (!isSelected) {
                                        editItem->daysOfWeek.push_back(i);
                                    }
                                    else {
                                        editItem->daysOfWeek.erase(std::remove(editItem->daysOfWeek.begin(), editItem->daysOfWeek.end(), i), editItem->daysOfWeek.end());
                                    }
                                }

                                if (isSelected) {
                                    ImGui::PopStyleColor();
                                }
                            }
                            ImGui::EndPopup();
                        }
                    }

                    // Days of Month Input List
                    if (editItem->intervalMode == 1) {
                        std::vector<int> sortedDays = editItem->daysOfMonth;
                        std::sort(sortedDays.begin(), sortedDays.end(), [](int a, int b) {
                            return a < b;
                        });

                        std::stringstream dayListStream;
                        for (size_t i = 0; i < sortedDays.size(); ++i) {
                            if (i > 0) {
                                dayListStream << ", ";                            
                            }
                            if (sortedDays[i] == 99) {
                                dayListStream << "Ultimo";
                            }
                            else {
                                dayListStream << sortedDays[i];
                            }
                        }
                        std::string dayList = dayListStream.str();
                        ImGui::TextWrapped("Selected Days of Month: %s", dayList.c_str());

                        // Popup to select days of the month
                        if (ImGui::Button("Select Days")) {
                            ImGui::OpenPopup("SelectDaysPopup");
                        }
                        if (ImGui::BeginPopup("SelectDaysPopup")) {
                            // Create a grid of buttons for days 1-31
                            const int buttonsPerRow = 8; 
                            for (int day = 1; day <= daysOfMonthCount; ++day) {
                                // Highlight the button if the day is already selected
                                bool isSelected = std::find(editItem->daysOfMonth.begin(), editItem->daysOfMonth.end(), day) != editItem->daysOfMonth.end();

                                if (isSelected) {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));  // Highlight color
                                }
                                if (ImGui::Button(std::to_string(day).c_str(), ImVec2(40 * NexusLink->Scaling, 40 * NexusLink->Scaling))) {
                                    // Toggle selection: add if not in vector, remove if already in vector
                                    if (!isSelected) {
                                        editItem->daysOfMonth.push_back(day);
                                    }
                                    else {
                                        editItem->daysOfMonth.erase(std::remove(editItem->daysOfMonth.begin(), editItem->daysOfMonth.end(), day), editItem->daysOfMonth.end());
                                    }
                                }
                                if (isSelected) {
                                    ImGui::PopStyleColor();
                                }
                                if (day % buttonsPerRow != 0) ImGui::SameLine(); // Grid logic
                            }
                            ImGui::Text(""); // Empty text to get to next line

                            // "Ultimo" Button for the last day of the month (represented by 99)
                            bool isUltimoSelected = std::find(editItem->daysOfMonth.begin(), editItem->daysOfMonth.end(), 99) != editItem->daysOfMonth.end();
                            if (isUltimoSelected) {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));  // Highlight color for "Ultimo"
                            }
                            if (ImGui::Button("Ultimo", ImVec2((8*40 + 7 * 5)*NexusLink->Scaling, 40 * NexusLink->Scaling))) {
                                // Toggle selection for "Ultimo"
                                if (!isUltimoSelected) {
                                    editItem->daysOfMonth.push_back(99);
                                }
                                else {
                                    editItem->daysOfMonth.erase(std::remove(editItem->daysOfMonth.begin(), editItem->daysOfMonth.end(), 99), editItem->daysOfMonth.end());
                                }
                            }
                            if (isUltimoSelected) {
                                ImGui::PopStyleColor();
                            }

                            ImGui::EndPopup();
                        }
                    }
                    
                    ImGui::PopItemWidth();

                    // Due Hours Input
                    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
                    ImGui::InputInt(("##hours" + std::to_string(editItem->id)).c_str(), &editItem->dueHours);
                    if (editItem->dueHours < 0) editItem->dueHours = 0;
                    if (editItem->dueHours > 23) editItem->dueHours = 23;
                    ImGui::SameLine();
                    ImGui::Text(":");
                    ImGui::SameLine();
                    // Due Minutes Input
                    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
                    ImGui::InputInt(("##minutes" + std::to_string(editItem->id)).c_str(), &editItem->dueMinutes);
                    if (editItem->dueMinutes < 0) editItem->dueMinutes = 0;
                    if (editItem->dueMinutes > 59) editItem->dueMinutes = 59;
                }
                else {
                    ImGui::Text("-");
                }
            }
            ImGui::TableNextColumn(); 
            if (item->deleted) {
                ImGui::Text("");
            }
            else {
                if (editItem == nullptr || editItem->id != item->id) {
                    if (item->repeatMode == RepeatMode::ONCE) {
                        // Single time task, begin "start task" routine
                        if (iconStart != nullptr) {
                            ImGui::PushID(hashString("start_" + std::to_string(item->id)));
                            if (ImGui::ImageButton((ImTextureID)iconStart->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {

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
                    else if (!accountName.empty() && item->repeatMode != RepeatMode::ONCE) {
                        // repeatable task, make the button subscribe / unsubscribe depending on subscription status!
                        if (!item->accountConfiguration.contains(accountName)) {
                            item->accountConfiguration.emplace(accountName, false);
                        }
                        Texture* btnTex = nullptr; // TODO
                        bool subscribe;
                        if (item->accountConfiguration[accountName]) {
                            // is subscribed, btnTex should be "unsubscribe"
                            btnTex = iconCancel;
                            subscribe = false;
                        }
                        else {
                            // is unsubscribed, btnTex should be "subscribe"
                            btnTex = iconSubscribe;
                            subscribe = true;
                        }

                        if (btnTex != nullptr) {
                            ImGui::PushID(hashString("subscribe_" + std::to_string(item->id)));
                            if (ImGui::ImageButton((ImTextureID)btnTex->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
                        if (ImGui::ImageButton((ImTextureID)iconSave->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
                            item->title = editItem->title;
                            item->description = editItem->description;
                            item->type = editItem->type;
                            item->repeatMode = editItem->repeatMode;
                            item->intervalMode = editItem->intervalMode;
                            item->daysOfMonth = editItem->daysOfMonth;
                            item->daysOfWeek = editItem->daysOfWeek;
                            item->dueHours = editItem->dueHours;
                            item->dueMinutes = editItem->dueMinutes;
                            
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
                            item->intervalMode = editItem->intervalMode;
                            item->daysOfMonth = editItem->daysOfMonth;
                            item->daysOfWeek = editItem->daysOfWeek;
                            item->dueHours = editItem->dueHours;
                            item->dueMinutes = editItem->dueMinutes;

                            editItem = nullptr;
                            organizerRepo->save();
                        }
                    }
                }
            }
            ImGui::TableNextColumn(); 
            if (item->deleted) {
                ImGui::Text("");
            }
            else {
                if (editItem == nullptr || editItem->id != item->id) {
                    if (iconEdit != nullptr) {
                        ImGui::PushID(hashString("edit_" + std::to_string(item->id)));
                        if (ImGui::ImageButton((ImTextureID)iconEdit->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
                        if (ImGui::ImageButton((ImTextureID)iconCancel->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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
            }
            ImGui::TableNextColumn(); 
            if (editItem == nullptr || editItem->id != item->id) {
                if (item->deleted) {
                    if (iconReactivate != nullptr) {
                        ImGui::PushID(hashString("react_" + std::to_string(item->id)));
                        if (ImGui::ImageButton((ImTextureID)iconReactivate->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
                            item->deleted = false;
                            organizerRepo->save();
                        }
                        ImGui::PopID();
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Restore Configuration");
                            ImGui::EndTooltip();
                        }
                    }
                    else {
                        if (ImGui::Button(("Restore##" + std::to_string(item->id)).c_str())) {
                            item->deleted = false;
                            organizerRepo->save();
                        }
                    }
                }
                else {
                    if (iconTrash != nullptr) {
                        ImGui::PushID(hashString("remove_" + std::to_string(item->id)));
                        if (ImGui::ImageButton((ImTextureID)iconTrash->Resource, imageButtonSize, { 0,0 }, { 1,1 })) {
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

    ImGui::Separator();
    ImGui::SetNextItemWidth(100.0f * NexusLink->Scaling);
    if (ImGui::InputInt("Delete done/deleted tasks after days (0 = never)", &settings.retentionPeriod)) {
        StoreSettings();
    }
    if (ImGui::Checkbox("Delete configurations with no tasks attached", &settings.deleteEmptyConfigs)) {
        StoreSettings();
    }
    if (settings.deleteEmptyConfigs) {
        if (ImGui::Checkbox("Delete configurations with no subscribers", &settings.deleteUnsubscribedConfigs)) {
            StoreSettings();
        }
    }
    ImGui::Text("");
    ImGui::TextWrapped("Cleanup will run with each addon load. During cleanup, all completed and deleted tasks that are older than the retention period configured will be physically removed from the data set.");
    if (settings.deleteEmptyConfigs) {
        ImGui::PushStyleColor(ImGuiCol_Text, colorRed);
        ImGui::TextWrapped("If a configuration has no more tasks attached to it, it will also be removed permanently.");
        if (settings.deleteUnsubscribedConfigs) {
            ImGui::TextWrapped("If a configuration has no subscribers attached to it, it will also be removed permanently.");
        }
        ImGui::PopStyleColor();
    }
    if (ImGui::Button("Perform cleanup now")) {
        organizerRepo->performCleanup();
    }   
}
void renderStatistics() {
    // Create sort of a split pane like for the main window;
    // Left side becomes a lean list of task configurations that are selectable for the statistics
    // Right side becomes the charts
    // Controls for charts probably go either to the left Pane above all, or bottom below all
    // if below all, Left pane max height should be twice the chart height; so if more configs are available,
    // we want this pane to scroll. else we do not really care, scrolling should only become available if list exceeds main window height
    
    if (ImGui::BeginChild("StatisticsFilterPane", ImVec2(180 * NexusLink->Scaling, 0), true)) {
        ImGui::Text("From:");
        ImGui::SameLine();
        ImGui::SetCursorPosX(60.0f);
        if (ImGui::Button(statisticsStart.toStringNice().substr(0, 10).c_str())) {
            ImGui::OpenPopup("StatisticsBeginPopup");
        }
        if (ImGui::BeginPopupModal("StatisticsBeginPopup")) {
            auto date = statisticsStart.toString();
            if (DateTimePicker("Statistics Begin", date, false)) {
                statisticsStart = DateTime(date);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Text("To:");
        ImGui::SameLine();
        ImGui::SetCursorPosX(60.0f);
        if (ImGui::Button(statisticsEnd.toStringNice().substr(0, 10).c_str())) {
            ImGui::OpenPopup("StatisticsEndPopup");
        }
        if (ImGui::BeginPopupModal("StatisticsEndPopup")) {

            auto date = statisticsEnd.toString();
            if (DateTimePicker("Statistics End", date, false)) {
                statisticsEnd = DateTime(date);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::RadioButton("Bar Chart", statisticsChartType == ChartType::BAR_CHART)) {
            statisticsChartType = ChartType::BAR_CHART;
        }
        if (ImGui::RadioButton("Line Chart", statisticsChartType == ChartType::LINE_CHART)) {
            statisticsChartType = ChartType::LINE_CHART;
        }

        ImGui::Separator();

        for (const auto& item : organizerRepo->getConfigurableItems()) {
            if (statisticsFilter.find(item->id) == statisticsFilter.end()) {
                statisticsFilter[item->id] = true; // Initialize as not selected
            }
        }
        for (const auto& item : organizerRepo->getApiTaskConfigurables()) {
            if (item->accountConfiguration.empty()) continue; // no API tasks that have 0 subscribers
            bool subscribed = false;
            for (auto sub : item->accountConfiguration) {
                if (sub.second) {
                    subscribed = true;
                    break;
                }
            }
            if (!subscribed) continue; // not subscribed currently

            if (statisticsFilter.find(item->item.id) == statisticsFilter.end()) {
                statisticsFilter[item->item.id] = true; // Initialize as not selected
            }
        }

        bool allSelected = true;
        for (const auto& item : organizerRepo->getConfigurableItems()) {
            if (!statisticsFilter[item->id]) {
                allSelected = false;
                break;
            }
        }
        if (allSelected) { // only continue if not already broken out earlier
            for (const auto& item : organizerRepo->getApiTaskConfigurables()) {
                if (item->accountConfiguration.empty()) continue; // no API tasks that have 0 subscribers
                bool subscribed = false;
                for (auto sub : item->accountConfiguration) {
                    if (sub.second) {
                        subscribed = true;
                        break;
                    }
                }
                if (!subscribed) continue; // not subscribed currently

                if (!statisticsFilter[item->item.id]) {
                    allSelected = false;
                    break;
                }
            }
        }

        bool selectAll = allSelected;
        if (ImGui::Checkbox("Select All", &selectAll)) {
            for (auto entry : statisticsFilter) {
                statisticsFilter[entry.first] = selectAll;
            }
        }
        ImGui::Separator();

        for (auto item : organizerRepo->getConfigurableItems()) {
            bool selected = statisticsFilter[item->id];

            // Start wrapping text
            if (ImGui::Checkbox(("##checkbox" + std::to_string(item->id)).c_str(), &selected)) {
                statisticsFilter[item->id] = selected;
            }
            ImGui::SameLine();
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 140 * NexusLink->Scaling);
            ImGui::TextWrapped("%s", item->title.c_str()); // Display wrapped text
            ImGui::PopTextWrapPos();
        }
        ImGui::Separator();
        for (auto item : organizerRepo->getApiTaskConfigurables()) {
            if (item->accountConfiguration.empty()) continue; // no API tasks that have 0 subscribers
            bool subscribed = false;
            for (auto sub : item->accountConfiguration) {
                if (sub.second) {
                    subscribed = true;
                    break;
                }
            }
            if (!subscribed) continue; // not subscribed currently

            bool selected = statisticsFilter[item->item.id];

            // Start wrapping text
            if (ImGui::Checkbox(("##checkbox" + std::to_string(item->item.id)).c_str(), &selected)) {
                statisticsFilter[item->item.id] = selected;
            }
            ImGui::SameLine();
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 140 * NexusLink->Scaling);
            ImGui::TextWrapped("%s", item->item.title.c_str()); // Display wrapped text
            ImGui::PopTextWrapPos();
        }
        ImGui::EndChild();
    }

    ImGui::SameLine();
    if (ImGui::BeginChild("StatisticsGraphsPane", ImVec2(0, 0), true)) {

        std::map<std::string, int> startedTasks = {};
        std::map<std::string, int> completedTasks = {};

        auto current = statisticsStart;
        // "fix" end date;
        // the user provides the end date at 00:00:00 typically;
        // because I am a lazy bum and did never implement <= for DateTime, I just add a day here instead and logically have the same result
        auto fixedEnd = DateTime(statisticsEnd).addDays(1);
        while (current < fixedEnd) {
            std::string label = current.toStringNice().substr(0, 10);
            startedTasks[label] = 0;
            completedTasks[label] = 0;
            current = current.addDays(1);
        }

        for (auto task : organizerRepo->getTaskInstances()) {
            OrganizerItem* item = organizerRepo->getConfigurableItemById(task->itemId);
            if (task->deleted) continue; // TODO possible override flag?
            if (!statisticsFilter[item->id]) continue; // item filtered

            // completion checks first
            if (task->completed) {
                auto completionDate = DateTime(task->completionDate);
                if (completionDate > statisticsStart && completionDate < fixedEnd) {
                    std::string endLabel = completionDate.toStringNice().substr(0, 10);
                    completedTasks[endLabel]++;
                }
            }

            auto startDate = DateTime(task->startDate);
            if (startDate < statisticsStart) continue;
            if (startDate > fixedEnd) continue;
            std::string startLabel = startDate.toStringNice().substr(0, 10);
            startedTasks[startLabel]++;
        }

        BarChart("Started Tasks", startedTasks, NexusLink->Scaling, statisticsChartType);
        ImGui::Separator();
        BarChart("Completed Tasks", completedTasks, NexusLink->Scaling, statisticsChartType);
        ImGui::Separator();

        ImGui::EndChild();
    }
}