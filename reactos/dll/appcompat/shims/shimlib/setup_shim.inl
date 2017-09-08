/*
 * PROJECT:     ReactOS Shim helper library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shimlib helper file, used for setting up the macro's used when registering a shim.
 * COPYRIGHT:   Copyright 2016,2017 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef SHIM_NS
#error "A namespace should be provided in SHIM_NS before including this file!"
#endif

#ifndef SHIM_OBJ_NAME
#define SHIM_OBJ_NAME3(ns, name) ns ## _ ## name
#define SHIM_OBJ_NAME2(ns, name) SHIM_OBJ_NAME3(ns, name)
#define SHIM_OBJ_NAME(name) SHIM_OBJ_NAME2(SHIM_NS, name)

#define SHIM_STRINGIFY2(X_) # X_
#define SHIM_STRINGIFY(X_) SHIM_STRINGIFY2(X_)

/* TODO: static_assert on (num < SHIM_NUM_HOOKS) */

#define SHIM_HOOK(num, dll, function, target) \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].LibraryName = dll; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].FunctionName = function; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].ReplacementFunction = target; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].OriginalFunction = NULL; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].Reserved[0] = NULL; \
    SHIM_OBJ_NAME(g_pAPIHooks)[num].Reserved[1] = NULL;

#define CALL_SHIM(SHIM_NUM, SHIM_CALLCONV) \
    ((SHIM_CALLCONV)(SHIM_OBJ_NAME(g_pAPIHooks)[SHIM_NUM].OriginalFunction))

#endif


PCSTR SHIM_OBJ_NAME(g_szModuleName) = SHIM_STRINGIFY(SHIM_NS);
PCSTR SHIM_OBJ_NAME(g_szCommandLine);
PHOOKAPI SHIM_OBJ_NAME(g_pAPIHooks);

