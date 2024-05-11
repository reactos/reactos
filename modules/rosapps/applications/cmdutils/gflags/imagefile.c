/*
 * PROJECT:     Global Flags utility
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Global Flags utility image file options
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "gflags.h"

static PWSTR ImageFile = NULL;
static DWORD OptionsAdd = 0;
static DWORD OptionsRemove = 0;
static BOOL OptionsSet = FALSE;

typedef struct FlagInfo
{
    DWORD dwFlag;
    const wchar_t* szAbbr;
    WORD wDest;
    const wchar_t* szDesc;
} FlagInfo;

#define FLG_DISABLE_DBGPRINT                0x8000000
#define FLG_CRITSEC_EVENT_CREATION          0x10000000
#define FLG_STOP_ON_UNHANDLED_EXCEPTION     0x20000000
#define FLG_ENABLE_HANDLE_EXCEPTIONS        0x40000000
#define FLG_DISABLE_PROTDLLS                0x80000000


static const FlagInfo g_Flags[] =
{
    {FLG_STOP_ON_EXCEPTION, L"soe", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Stop on exception"},
    {FLG_SHOW_LDR_SNAPS, L"sls", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Show loader snaps"},
    {FLG_DEBUG_INITIAL_COMMAND, L"dic", (DEST_REGISTRY), L"Debug initial command"},
    {FLG_STOP_ON_HUNG_GUI, L"shg", (DEST_KERNEL), L"Stop on hung GUI"},
    {FLG_HEAP_ENABLE_TAIL_CHECK, L"htc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap tail checking"},
    {FLG_HEAP_ENABLE_FREE_CHECK, L"hfc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap free checking"},
    {FLG_HEAP_VALIDATE_PARAMETERS, L"hpc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap parameter checking"},
    {FLG_HEAP_VALIDATE_ALL, L"hvc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap validation on call"},
    {FLG_APPLICATION_VERIFIER, L"vrf", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable application verifier"},
    // FLG_MONITOR_SILENT_PROCESS_EXIT
    {FLG_POOL_ENABLE_TAGGING, L"ptg", (DEST_REGISTRY), L"Enable pool tagging"},
    {FLG_HEAP_ENABLE_TAGGING, L"htg", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap tagging"},
    {FLG_USER_STACK_TRACE_DB, L"ust", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Create user mode stack trace database"},
    {FLG_KERNEL_STACK_TRACE_DB, L"kst", (DEST_REGISTRY), L"Create kernel mode stack trace database"},
    {FLG_MAINTAIN_OBJECT_TYPELIST, L"otl", (DEST_REGISTRY), L"Maintain a list of objects for each type"},
    {FLG_HEAP_ENABLE_TAG_BY_DLL, L"htd", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable heap tagging by DLL"},
    {FLG_DISABLE_STACK_EXTENSION, L"dse", (DEST_IMAGE), L"Disable stack extension"},

    {FLG_ENABLE_CSRDEBUG, L"d32", (DEST_REGISTRY), L"Enable debugging of Win32 subsystem"},
    {FLG_ENABLE_KDEBUG_SYMBOL_LOAD, L"ksl", (DEST_REGISTRY | DEST_KERNEL), L"Enable loading of kernel debugger symbols"},
    {FLG_DISABLE_PAGE_KERNEL_STACKS, L"dps", (DEST_REGISTRY), L"Disable paging of kernel stacks"},
    {FLG_ENABLE_SYSTEM_CRIT_BREAKS, L"scb", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable system critical breaks"},
    {FLG_HEAP_DISABLE_COALESCING, L"dhc", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Disable heap coalesce on free"},
    {FLG_ENABLE_CLOSE_EXCEPTIONS, L"ece", (DEST_REGISTRY | DEST_KERNEL), L"Enable close exception"},
    {FLG_ENABLE_EXCEPTION_LOGGING, L"eel", (DEST_REGISTRY | DEST_KERNEL), L"Enable exception logging"},
    {FLG_ENABLE_HANDLE_TYPE_TAGGING, L"eot", (DEST_REGISTRY | DEST_KERNEL), L"Enable object handle type tagging"},
    {FLG_HEAP_PAGE_ALLOCS, L"hpa", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Enable page heap"},
    {FLG_DEBUG_INITIAL_COMMAND_EX, L"dwl", (DEST_REGISTRY), L"Debug WinLogon"},
    {FLG_DISABLE_DBGPRINT, L"ddp", (DEST_REGISTRY | DEST_KERNEL), L"Buffer DbgPrint Output"},
    {FLG_CRITSEC_EVENT_CREATION, L"cse", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Early critical section event creation"},
    {FLG_STOP_ON_UNHANDLED_EXCEPTION, L"sue", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Stop on unhandled user-mode exception"},
    {FLG_ENABLE_HANDLE_EXCEPTIONS, L"bhd", (DEST_REGISTRY | DEST_KERNEL), L"Enable bad handles detection"},
    {FLG_DISABLE_PROTDLLS, L"dpd", (DEST_REGISTRY | DEST_KERNEL | DEST_IMAGE), L"Disable protected DLL verification"},
};

void PrintFlags(IN DWORD GlobalFlags, IN OPTIONAL WORD Dest)
{
    DWORD n;

    for (n = 0; n < ARRAYSIZE(g_Flags); ++n)
    {
        if (!Dest || (g_Flags[n].wDest & Dest))
        {
            if (g_Flags[n].dwFlag & GlobalFlags)
            {
                wprintf(L"    %s - %s\n", g_Flags[n].szAbbr, g_Flags[n].szDesc);
            }
        }
    }
}

static void ShowStatus(DWORD GlobalFlags, DWORD Ignored)
{
    if (GlobalFlags)
    {
        wprintf(L"Current Registry Settings for %s executable are: %08x\n", ImageFile, GlobalFlags);
        PrintFlags(GlobalFlags, 0);
    }
    else
    {
        wprintf(L"No Registry Settings for %s executable\n", ImageFile);
    }
    if (Ignored)
    {
        wprintf(L"The following settings were ignored: %08x\n", Ignored);
        PrintFlags(Ignored, 0);
    }
}

static DWORD ValidateFlags(DWORD GlobalFlags, WORD Dest)
{
    DWORD n;
    DWORD Valid = 0;

    for (n = 0; n < ARRAYSIZE(g_Flags); ++n)
    {
        if (g_Flags[n].wDest & Dest)
        {
            Valid |= g_Flags[n].dwFlag;
        }
    }

    return GlobalFlags & Valid;
}

static DWORD FindFlag(PCWSTR Name, WORD Dest)
{
    DWORD n;

    for (n = 0; n < ARRAYSIZE(g_Flags); ++n)
    {
        if (g_Flags[n].wDest & Dest)
        {
            if (!_wcsicmp(Name, g_Flags[n].szAbbr))
            {
                return g_Flags[n].dwFlag;
            }
        }
    }

    return 0;
}

static VOID ModifyStatus(VOID)
{
    LONG Ret;
    DWORD GlobalFlags, Requested, Ignored;
    HKEY IFEOKey;
    WCHAR Buffer[11];

    if (!OpenImageFileExecOptions(KEY_WRITE | KEY_READ, ImageFile, &IFEOKey))
    {
        return;
    }

    if (OptionsSet)
    {
        Requested = OptionsAdd;
    }
    else
    {
        Requested = ReadSZFlagsFromRegistry(IFEOKey, L"GlobalFlag");
        Requested &= ~OptionsRemove;
        Requested |= OptionsAdd;
    }

    GlobalFlags = ValidateFlags(Requested, DEST_IMAGE);
    Ignored = GlobalFlags ^ Requested;

    if (GlobalFlags)
    {
        wsprintf(Buffer, L"0x%08x", GlobalFlags);
        Ret = RegSetValueExW(IFEOKey, L"GlobalFlag", 0, REG_SZ, (BYTE*)Buffer, (wcslen(Buffer) + 1) * sizeof(WCHAR));
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegSetValueEx failed (%d)\n", Ret);
        }
        else
        {
            ShowStatus(GlobalFlags, Ignored);
        }
    }
    else
    {
        Ret = RegDeleteValueW(IFEOKey, L"GlobalFlag");
        if (Ret != ERROR_SUCCESS)
        {
            wprintf(L"MS: RegDeleteValue failed (%d)\n", Ret);
        }
        else
        {
            ShowStatus(GlobalFlags, Ignored);
        }
    }
    CloseHandle(IFEOKey);
}


static VOID DisplayStatus(VOID)
{
    HKEY IFEOKey;
    DWORD GlobalFlags;

    if (!OpenImageFileExecOptions(KEY_READ, ImageFile, &IFEOKey))
    {
        return;
    }

    GlobalFlags = ReadSZFlagsFromRegistry(IFEOKey, L"GlobalFlag");
    ShowStatus(GlobalFlags, 0);

    CloseHandle(IFEOKey);
}


BOOL ImageFile_ParseCmdline(INT i, int argc, LPWSTR argv[])
{
    for (; i < argc; i++)
    {
        if (ImageFile == NULL)
        {
            ImageFile = argv[i];
        }
        else if (argv[i][0] == '+')
        {
            if (OptionsSet)
            {
                wprintf(L"Unexpected argument - '%s'\n", argv[i]);
                return FALSE;
            }
            OptionsAdd |= FindFlag(argv[i] + 1, DEST_IMAGE);
        }
        else if (argv[i][0] == '-')
        {
            if (OptionsSet)
            {
                wprintf(L"Unexpected argument - '%s'\n", argv[i]);
                return FALSE;
            }
            OptionsRemove |= FindFlag(argv[i] + 1, DEST_IMAGE);
        }
        else
        {
            OptionsSet = TRUE;
            OptionsAdd = wcstoul(argv[i], NULL, 16);
            if (OptionsAdd == ~0)
                OptionsAdd = 0;
        }
    }

    if (ImageFile == NULL)
    {
        wprintf(L"No Image specified\n");
        return FALSE;
    }

    return TRUE;
}

INT ImageFile_Execute()
{
    if (!OptionsAdd && !OptionsRemove && !OptionsSet)
    {
        DisplayStatus();
    }
    else
    {
        ModifyStatus();
    }

    return 0;
}
