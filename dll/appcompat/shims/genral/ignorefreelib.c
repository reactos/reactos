/*
 * PROJECT:     ReactOS 'General' Shim library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Ignore FreeLibrary calls
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>


#define SHIM_NS         IgnoreFreeLibrary
#include <setup_shim.inl>

typedef BOOL(WINAPI* FREELIBRARYPROC)(HMODULE);

const char** g_Names;
static int g_NameCount;

BOOL WINAPI SHIM_OBJ_NAME(FreeLibrary)(HMODULE hModule)
{
    char Buffer[MAX_PATH], *Ptr = Buffer;
    DWORD len, wanted = ARRAYSIZE(Buffer);
    for (;;)
    {
        len = GetModuleFileNameA(hModule, Ptr, wanted);
        if (len < wanted)
            break;

        wanted *= 2;
        if (Ptr != Buffer)
            ShimLib_ShimFree(Ptr);

        Ptr = ShimLib_ShimMalloc(wanted);
        if (!Ptr)
            break;
    }

    if (Ptr && len)
    {
        char* ModuleName = NULL;
        int n;
        for (; len; len--)
        {
            ModuleName = Ptr + len;
            if (ModuleName[-1] == '/' || ModuleName[-1] == '\\')
                break;
        }
        for (n = 0; n < g_NameCount; ++n)
        {
            if (!_stricmp(g_Names[n], ModuleName))
            {
                SHIM_INFO("Prevented unload of %s\n", ModuleName);
                if (Ptr && Ptr != Buffer)
                    ShimLib_ShimFree(Ptr);
                return TRUE;
            }
        }
    }

    if (Ptr && Ptr != Buffer)
        ShimLib_ShimFree(Ptr);

    return CALL_SHIM(0, FREELIBRARYPROC)(hModule);
}

static VOID InitIgnoreFreeLibrary(PCSTR CommandLine)
{
    PCSTR prev, cur;
    int count = 1, n = 0;
    const char** names;

    if (!CommandLine || !*CommandLine)
        return;

    prev = CommandLine;
    while ((cur = strchr(prev, ';')))
    {
        count++;
        prev = cur + 1;
    }

    names = ShimLib_ShimMalloc(sizeof(char*) * count);
    if (!names)
    {
        SHIM_WARN("Unable to allocate %u bytes\n", sizeof(char*) * count);
        return;
    }

    prev = CommandLine;
    while ((cur = strchr(prev, ';')))
    {
        names[n] = ShimLib_StringNDuplicateA(prev, cur - prev + 1);
        if (!names[n])
        {
            SHIM_WARN("Unable to allocate %u bytes\n", cur - prev + 2);
            goto fail;
        }
        n++;
        prev = cur + 1;
    }
    names[n] = ShimLib_StringDuplicateA(prev);
    if (!names[n])
    {
        SHIM_WARN("Unable to allocate last string\n");
        goto fail;
    }

    g_Names = names;
    g_NameCount = count;
    return;

fail:
    --n;
    while (n >= 0)
    {
        if (names[n])
            ShimLib_ShimFree((PVOID)names[n]);

        --n;
    }
    ShimLib_ShimFree((PVOID)names);
}

BOOL WINAPI SHIM_OBJ_NAME(Notify)(DWORD fdwReason, PVOID ptr)
{
    if (fdwReason == SHIM_NOTIFY_ATTACH)
    {
        SHIM_MSG("IgnoreFreeLibrary(%s)\n", SHIM_OBJ_NAME(g_szCommandLine));
        InitIgnoreFreeLibrary(SHIM_OBJ_NAME(g_szCommandLine));
    }
    return TRUE;
}


#define SHIM_NOTIFY_FN SHIM_OBJ_NAME(Notify)
#define SHIM_NUM_HOOKS  1
#define SHIM_SETUP_HOOKS \
    SHIM_HOOK(0, "KERNEL32.DLL", "FreeLibrary", SHIM_OBJ_NAME(FreeLibrary))

#include <implement_shim.inl>
