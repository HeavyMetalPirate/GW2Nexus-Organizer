#ifndef OMEGA_GLOBALS_H
#define OMEGA_GLOBALS_H

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

#include "Constants.h"
#include "Settings.h"

extern AddonDefinition AddonDef;
extern HMODULE hSelf;
extern AddonAPI* APIDefs;
extern Mumble::Data* MumbleLink;
extern NexusLinkData* NexusLink;
extern addon::Settings settings;

#endif