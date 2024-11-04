///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - All rights reserved.
///
/// This code is licensed under the MIT license.
/// You should have received a copy of the license along with this source file.
/// You may obtain a copy of the license at: https://opensource.org/license/MIT
/// 
/// Name         :  entry.cpp
/// Description  :  Simple example of a Nexus addon implementation.
///----------------------------------------------------------------------------------------------------
/// 
#include "service/SharedServices.h"
#include "service/AddonRenderer.h"
#include "service/ApiTokenService.h"
#include "service/AutoStartService.h"

#include "Globals.h"

/* proto */
void AddonLoad(AddonAPI* aApi);
void AddonUnload();
void AddonPreRender();
void AddonRender();
void AddonPostRender();
void AddonOptions();
void AddonSimpleShortcut();
void ProcessKeybind(const char* aIdentifier, bool isRelease);
void HandleIdentityChanged(void* eventArgs);
void HandleAccountName(void* eventArgs);
void HandleSelfJoin(void* eventArgs);
void HandleTriggerDailyReset(void* eventArgs);
void HandleTriggerWeeklyReset(void* eventArgs);

/* globals */
AddonDefinition AddonDef	= {};
HMODULE hSelf				= nullptr;
AddonAPI* APIDefs			= nullptr;
NexusLinkData* NexusLink	= nullptr;
Mumble::Data * MumbleLink	= nullptr;
OrganizerRepository* organizerRepo = nullptr;
addon::Settings settings;

ApiTokenService apiTokenService;
NotificationService notificationService;
AutoStartService autoStartService;

bool unloading = false;

std::string accountName = "";
std::string characterName = "";

bool organizerRendered = false;
bool todoListRendered = false;

/* services */
Renderer renderer;

///----------------------------------------------------------------------------------------------------
/// DllMain:
/// 	Main entry point for DLL.
/// 	We are not interested in this, all we get is our own HMODULE in case we need it.
///----------------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH: hSelf = hModule; break;
		case DLL_PROCESS_DETACH: break;
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
	}
	return TRUE;
}

///----------------------------------------------------------------------------------------------------
/// GetAddonDef:
/// 	Export needed to give Nexus information about the addon.
///----------------------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
	AddonDef.Signature = -216687354; // set to random unused negative integer
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Organizer";
	AddonDef.Version.Major = 0;
	AddonDef.Version.Minor = 4;
	AddonDef.Version.Build = 0;
	AddonDef.Version.Revision = 4;
	AddonDef.Author = "Heavy Metal Pirate.2695";
	AddonDef.Description = "Tools to help you stay organized throughout Tyria.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	/* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/HeavyMetalPirate/GW2Nexus-Organizer";

	return &AddonDef;
}

///----------------------------------------------------------------------------------------------------
/// AddonLoad:
/// 	Load function for the addon, will receive a pointer to the API.
/// 	(You probably want to store it.)
///----------------------------------------------------------------------------------------------------
void AddonLoad(AddonAPI* aApi)
{
	unloading = false;

	APIDefs = aApi; // store the api somewhere easily accessible

	ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext); // cast to ImGuiContext*
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree); // on imgui 1.80+

	NexusLink = (NexusLinkData*)APIDefs->DataLink.Get("DL_NEXUS_LINK");
	MumbleLink = (Mumble::Data*)APIDefs->DataLink.Get("DL_MUMBLE_LINK");

	LoadSettings();

	organizerRepo = new OrganizerRepository();
	organizerRepo->initialize();
	organizerRepo->performCleanup();

	renderer = Renderer();
	apiTokenService = ApiTokenService();
	apiTokenService.startService();

	autoStartService = AutoStartService();
	autoStartService.initialize();
	autoStartService.startWorker();

	notificationService = NotificationService();
	
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_CLOSE", IDB_ICON_CLOSE, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_OPTIONS", IDB_ICON_OPTIONS, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_TRASH", IDB_ICON_TRASH, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_CHECK", IDB_ICON_CHECK, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_ADD", IDB_ICON_ADD, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_REPEAT", IDB_ICON_CLOCK, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_REACTIVATE", IDB_ICON_REACTIVATE, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_START", IDB_ICON_START, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_EDIT", IDB_ICON_EDIT, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_SHORTCUT", IDB_ICON_SHORTCUT, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_SHORTCUT_HOVER", IDB_ICON_SHORTCUT_HOVER, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_NO_REPEAT", IDB_ICON_NO_REPEAT, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_SAVE", IDB_ICON_SAVE, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_CANCEL", IDB_ICON_CANCEL, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_SUBSCRIBE", IDB_ICON_SUBSCRIBE, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_PIN", IDB_ICON_PIN, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_LIST", IDB_ICON_LIST, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_EXPAND", IDB_ICON_EXPAND, hSelf, nullptr);
	APIDefs->Textures.LoadFromResource("ICON_ORGANIZER_COLLAPSE", IDB_ICON_COLLAPSE, hSelf, nullptr);


	APIDefs->InputBinds.RegisterWithString("ORG_KEYBIND", ProcessKeybind, "ALT+K");
	if (!settings.hideShortcut) {
		APIDefs->QuickAccess.Add("ORG_SHORTCUT", "ICON_ORGANIZER_SHORTCUT", "ICON_ORGANIZER_SHORTCUT_HOVER", "ORG_KEYBIND", "Organizer");
		APIDefs->QuickAccess.AddContextMenu("ORG_CONTEXT_MENU", "ORG_SHORTCUT", AddonSimpleShortcut);
	}

	// Events
	APIDefs->Events.Subscribe("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);
	APIDefs->Events.Subscribe("EV_ACCOUNT_NAME", HandleAccountName);
	APIDefs->Events.Subscribe("EV_ARCDPS_SELF_JOIN", HandleSelfJoin);

	APIDefs->Events.Subscribe(EV_NAME_DAILY_RESET, HandleTriggerDailyReset);
	APIDefs->Events.Subscribe(EV_NAME_WEEKLY_RESET, HandleTriggerWeeklyReset);

	// Add an options window and a regular render callback
	APIDefs->Renderer.Register(ERenderType_PreRender, AddonPreRender);
	APIDefs->Renderer.Register(ERenderType_Render, AddonRender);
	APIDefs->Renderer.Register(ERenderType_PostRender, AddonPostRender);
	APIDefs->Renderer.Register(ERenderType_OptionsRender, AddonOptions);

	APIDefs->Events.RaiseNotification("EV_REQUEST_ACCOUNT_NAME"); // Request account name at load
	APIDefs->Events.RaiseNotification("EV_REPLAY_ARCDPS_SQUAD_JOIN"); // Request all squad joins in case player is in a squad at load time

	APIDefs->UI.RegisterCloseOnEscape("Organizer", &organizerRendered);
	APIDefs->UI.RegisterCloseOnEscape("TODOs", &todoListRendered);

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#00ff00>Organizer</c> was loaded.");
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Current Date/Time: " + DateTime::nowLocal().toString()).c_str());
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Last Daily Reset: " + DateTime::nowLocal().getLastDaily().toString()).c_str());
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Next Daily Reset: " + DateTime::nowLocal().getNextDaily().toString()).c_str());
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Last Weekly Reset: " + DateTime::nowLocal().getLastWeekly().toString()).c_str());
	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, ("Next Weekly Reset: " + DateTime::nowLocal().getNextWeekly().toString()).c_str());

}

///----------------------------------------------------------------------------------------------------
/// AddonUnload:
/// 	Everything you registered in AddonLoad, you should "undo" here.
///----------------------------------------------------------------------------------------------------
void AddonUnload()
{
	/* Stop components */
	unloading = true;
	notificationService.unload();
	apiTokenService.stopService();
	autoStartService.endWorker();
	organizerRepo->unload();

	/* let's clean up after ourselves */
	StoreSettings();
	organizerRepo->save();

	if (!settings.hideShortcut) {
		APIDefs->QuickAccess.Remove("ORG_SHORTCUT");
		APIDefs->QuickAccess.RemoveContextMenu("ORG_CONTEXT_MENU");
	}
	APIDefs->InputBinds.Deregister("ORG_KEYBIND");

	APIDefs->Events.Unsubscribe("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);
	APIDefs->Events.Unsubscribe("EV_ACCOUNT_NAME", HandleAccountName);
	APIDefs->Events.Unsubscribe("EV_ARCDPS_SELF_JOIN", HandleSelfJoin);

	APIDefs->Events.Unsubscribe(EV_NAME_DAILY_RESET, HandleTriggerDailyReset);
	APIDefs->Events.Unsubscribe(EV_NAME_WEEKLY_RESET, HandleTriggerWeeklyReset);

	APIDefs->Renderer.Deregister(AddonPreRender);
	APIDefs->Renderer.Deregister(AddonRender);
	APIDefs->Renderer.Deregister(AddonPostRender);
	APIDefs->Renderer.Deregister(AddonOptions);

	APIDefs->UI.DeregisterCloseOnEscape("Organizer");
	APIDefs->UI.DeregisterCloseOnEscape("TODOs");

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#ff0000>Signing off</c>, it was an honor commander.");
}

///----------------------------------------------------------------------------------------------------
/// AddonRender:
/// 	Called every frame. Safe to render any ImGui.
/// 	You can control visibility on loading screens with NexusLink->IsGameplay.
///----------------------------------------------------------------------------------------------------
void AddonRender()
{
	renderer.render();
	notificationService.render();
}

///----------------------------------------------------------------------------------------------------
/// AddonOptions:
/// 	Basically an ImGui callback that doesn't need its own Begin/End calls.
///----------------------------------------------------------------------------------------------------
void AddonOptions()
{
	if (ImGui::Checkbox("Hide Shortcut Icon", &settings.hideShortcut)) {
		if (settings.hideShortcut) {
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Hiding QuickAccess button.");
			APIDefs->QuickAccess.Remove("ORG_SHORTCUT");
			APIDefs->QuickAccess.RemoveContextMenu("ORG_CONTEXT_MENU");
		}
		else {
			APIDefs->Log(ELogLevel_INFO, ADDON_NAME, "Showing QuickAccess button.");
			APIDefs->QuickAccess.Add("ORG_SHORTCUT", "ICON_ORGANIZER_SHORTCUT", "ICON_ORGANIZER_SHORTCUT_HOVER", "ORG_KEYBIND", "Organizer");
			APIDefs->QuickAccess.AddContextMenu("ORG_CONTEXT_MENU", "ORG_SHORTCUT", AddonSimpleShortcut);
		}
		StoreSettings();
	}
}

void AddonSimpleShortcut() {
	ImGui::Checkbox("TODOs", &todoListRendered);
	ImGui::Checkbox("Configuration", &organizerRendered);
}

void AddonPreRender() {
	renderer.preRender();
	notificationService.preRender();
}
void AddonPostRender() {
	renderer.postRender();
	notificationService.postRender();
}

void ProcessKeybind(const char* aIdentifier, bool isRelease)
{
	if (strcmp(aIdentifier, "ORG_KEYBIND") == 0)
	{
		if(!isRelease) todoListRendered = !todoListRendered; // only flip on press
		return;
	}
}

void HandleIdentityChanged(void* anEventArgs) {
	Mumble::Identity* identity = (Mumble::Identity*)anEventArgs;
	if (identity == nullptr) return;
	characterName = identity->Name;
}

void HandleAccountName(void* eventArgs) {
	if (!accountName.empty()) return; // we already got the name, ignore this

	const char* name = (const char*)eventArgs;
	APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Received Account Name: " + std::string(name)).c_str());

	accountName = std::string(name);
	if (!accountName.empty())
		accountName = accountName.substr(1);
}

void HandleSelfJoin(void* eventArgs) {
	if (!accountName.empty()) return; // we already got the name, ignore this

	EvAgentUpdate* ev = (EvAgentUpdate*)eventArgs;
	const char* name = ev->account;
	APIDefs->Log(ELogLevel_INFO, ADDON_NAME, ("Received Account Name: " + std::string(name)).c_str());

	accountName = std::string(name);
	if (!accountName.empty())
		accountName = accountName.substr(1);
}

void HandleTriggerDailyReset(void* eventArgs) {
	autoStartService.PerformDailyReset();
}
void HandleTriggerWeeklyReset(void* eventArgs) {
	autoStartService.PerformWeeklyReset();
}
