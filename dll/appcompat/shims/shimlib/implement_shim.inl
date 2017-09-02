/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/shimlib/implement_shim.inl
 * PURPOSE:         Shimlib helper file, used to register shims setup with macro's from setup_shim.inl
 * PROGRAMMER:      Mark Jansen
 */

#ifndef SHIM_NS
#error "A namespace should be provided in SHIM_NS before including this file!"
#endif

#ifndef SHIM_NUM_HOOKS
#error "The number of hooks should be provided in SHIM_NUM_HOOKS before including this file!"
#endif

#ifndef SHIM_OBJ_NAME
#error "setup_shim.inl should be included before this file!"
#endif

#if SHIM_NUM_HOOKS > 0
#ifndef SHIM_SETUP_HOOKS
#error "Please define a hook: #define SHIM_SETUP_HOOKS SHIM_HOOK(num, dll_name, function_name, your_function)"
#endif
#else
#ifdef SHIM_SETUP_HOOKS
#error "Hooks are defined, yet SHIM_NUM_HOOKS is <= 0 !"
#endif
#endif

PHOOKAPI WINAPI SHIM_OBJ_NAME(GetHookAPIs)(DWORD fdwReason, PCSTR pszCmdLine, PDWORD pdwHookCount)
{
    if (pszCmdLine)
    {
        SHIM_OBJ_NAME(g_szCommandLine) = ShimLib_StringDuplicateA(pszCmdLine);
    }
    else
    {
        SHIM_OBJ_NAME(g_szCommandLine) = "";
    }
    SHIM_OBJ_NAME(g_pAPIHooks) = ShimLib_ShimMalloc(sizeof(HOOKAPI) * SHIM_NUM_HOOKS);
    if (SHIM_NUM_HOOKS)
        ZeroMemory(SHIM_OBJ_NAME(g_pAPIHooks), sizeof(HOOKAPI) * SHIM_NUM_HOOKS);
    *pdwHookCount = SHIM_NUM_HOOKS;

#ifdef SHIM_NOTIFY_FN
    if (!SHIM_NOTIFY_FN(fdwReason, NULL))
        return NULL;
#endif

#if SHIM_NUM_HOOKS > 0
    SHIM_SETUP_HOOKS
#endif
    return SHIM_OBJ_NAME(g_pAPIHooks);
}


#if defined(_MSC_VER)
#pragma section(".shm$BBB",long,read)
#endif

_SHMALLOC(".shm$BBB") SHIMREG SHIM_OBJ_NAME(_shim_fn) =
{
    SHIM_OBJ_NAME(GetHookAPIs),
#ifdef SHIM_NOTIFY_FN
    SHIM_NOTIFY_FN,
#else
    NULL,
#endif
    SHIM_STRINGIFY(SHIM_NS)
};

#undef SHIM_SETUP_HOOKS
#undef SHIM_NOTIFY_FN
#undef SHIM_NUM_HOOKS
#undef SHIM_NS
