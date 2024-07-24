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
void ProcessKeybind(const char* aIdentifier);
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
	AddonDef.Version.Minor = 1;
	AddonDef.Version.Build = 1;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "Heavy Metal Pirate.2695";
	AddonDef.Description = "Tools to help you stay organized throughout Tyria.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	/* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
	//AddonDef.Provider = EUpdateProvider_GitHub;
	//AddonDef.UpdateLink = "https://github.com/RaidcoreGG/GW2Nexus-AddonTemplate";

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

	NexusLink = (NexusLinkData*)APIDefs->GetResource("DL_NEXUS_LINK");
	MumbleLink = (Mumble::Data*)APIDefs->GetResource("DL_MUMBLE_LINK");

	LoadSettings();

	organizerRepo = new OrganizerRepository();
	organizerRepo->initialize();


	renderer = Renderer();
	apiTokenService = ApiTokenService();
	apiTokenService.startService();

	autoStartService = AutoStartService();
	autoStartService.initialize();
	autoStartService.startWorker();

	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_CLOSE", IDB_ICON_CLOSE, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_OPTIONS", IDB_ICON_OPTIONS, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_TRASH", IDB_ICON_TRASH, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_CHECK", IDB_ICON_CHECK, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_ADD", IDB_ICON_ADD, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_REPEAT", IDB_ICON_CLOCK, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_REACTIVATE", IDB_ICON_REACTIVATE, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_START", IDB_ICON_START, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_EDIT", IDB_ICON_EDIT, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_SHORTCUT", IDB_ICON_SHORTCUT, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_SHORTCUT_HOVER", IDB_ICON_SHORTCUT_HOVER, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_NO_REPEAT", IDB_ICON_NO_REPEAT, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_SAVE", IDB_ICON_SAVE, hSelf, nullptr);
	APIDefs->LoadTextureFromResource("ICON_ORGANIZER_CANCEL", IDB_ICON_CANCEL, hSelf, nullptr);

	APIDefs->RegisterKeybindWithString("ORG_KEYBIND", ProcessKeybind, "ALT+K");
	APIDefs->AddShortcut("ORG_SHORTCUT", "ICON_ORGANIZER_SHORTCUT", "ICON_ORGANIZER_SHORTCUT_HOVER", "ORG_KEYBIND", "Organizer");

	// Events
	APIDefs->SubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);
	APIDefs->SubscribeEvent("EV_ACCOUNT_NAME", HandleAccountName);
	APIDefs->SubscribeEvent("EV_ARCDPS_SELF_JOIN", HandleSelfJoin);

	APIDefs->SubscribeEvent(EV_NAME_DAILY_RESET, HandleTriggerDailyReset);
	APIDefs->SubscribeEvent(EV_NAME_WEEKLY_RESET, HandleTriggerWeeklyReset);

	// Add an options window and a regular render callback
	APIDefs->RegisterRender(ERenderType_PreRender, AddonPreRender);
	APIDefs->RegisterRender(ERenderType_Render, AddonRender);
	APIDefs->RegisterRender(ERenderType_PostRender, AddonPostRender);
	//APIDefs->RegisterRender(ERenderType_OptionsRender, AddonOptions);

	APIDefs->RaiseEventNotification("EV_REQUEST_ACCOUNT_NAME"); // Request account name at load
	APIDefs->RaiseEventNotification("EV_REPLAY_ARCDPS_SQUAD_JOIN"); // Request all squad joins in case player is in a squad at load time

	APIDefs->Log(ELogLevel_DEBUG, ADDON_NAME, "<c=#00ff00>Organizer</c> was loaded.");
}

///----------------------------------------------------------------------------------------------------
/// AddonUnload:
/// 	Everything you registered in AddonLoad, you should "undo" here.
///----------------------------------------------------------------------------------------------------
void AddonUnload()
{
	/* Stop components */
	unloading = true;
	apiTokenService.stopService();
	autoStartService.endWorker();
	organizerRepo->unload();

	/* let's clean up after ourselves */
	StoreSettings();
	organizerRepo->save();

	APIDefs->RemoveShortcut("ORG_SHORTCUT");
	APIDefs->DeregisterKeybind("ORG_KEYBIND");

	APIDefs->UnsubscribeEvent("EV_MUMBLE_IDENTITY_UPDATED", HandleIdentityChanged);
	APIDefs->UnsubscribeEvent("EV_ACCOUNT_NAME", HandleAccountName);
	APIDefs->UnsubscribeEvent("EV_ARCDPS_SELF_JOIN", HandleSelfJoin);

	APIDefs->UnsubscribeEvent(EV_NAME_DAILY_RESET, HandleTriggerDailyReset);
	APIDefs->UnsubscribeEvent(EV_NAME_WEEKLY_RESET, HandleTriggerWeeklyReset);

	APIDefs->DeregisterRender(AddonPreRender);
	APIDefs->DeregisterRender(AddonRender);
	APIDefs->DeregisterRender(AddonPostRender);
	//APIDefs->DeregisterRender(AddonOptions);

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
}

///----------------------------------------------------------------------------------------------------
/// AddonOptions:
/// 	Basically an ImGui callback that doesn't need its own Begin/End calls.
///----------------------------------------------------------------------------------------------------
void AddonOptions()
{
	// TODO impl
}

void AddonSimpleShortcut() {
	// TODO Impl
}

void AddonPreRender() {
	renderer.preRender();
}
void AddonPostRender() {
	renderer.postRender();
}

void ProcessKeybind(const char* aIdentifier)
{
	if (strcmp(aIdentifier, "ORG_KEYBIND") == 0)
	{
		todoListRendered = !todoListRendered;
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