#ifndef EXPLORER_PLUG_H
#define EXPLORER_PLUG_H

#include "env.h"

// void plug_init(void);
// void* plug_pre_reload(void);
// void plug_post_reload(void* state);
// void plug_update(Env env);
// void plug_reset(void);

#define LIST_OF_PLUGS \
    PLUG(plug_init, void, ImGuiContext*) \
    PLUG(plug_pre_reload, void*, void) \
    PLUG(plug_post_reload, void, void*) \
    PLUG(plug_update, void, Env) \
    PLUG(plug_reset, void, void)

#endif // !EXPLORER_PLUG_H
