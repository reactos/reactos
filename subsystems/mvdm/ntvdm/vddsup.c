/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/vddsup.c
 * PURPOSE:         Virtual Device Drivers (VDD) Support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "vddsup.h"

#include "cpu/bop.h"
#include <isvbop.h>

typedef VOID (WINAPI *VDD_PROC)(VOID);

typedef struct _VDD_MODULE
{
    HMODULE  hDll;
    VDD_PROC DispatchRoutine;
} VDD_MODULE, *PVDD_MODULE;

// WARNING: A structure with the same name exists in nt_vdd.h,
// however it is not declared because its inclusion was prevented
// with #define NO_NTVDD_COMPAT, see ntvdm.h
typedef struct _VDD_USER_HANDLERS
{
    LIST_ENTRY Entry;

    HANDLE            hVdd;
    PFNVDD_UCREATE    Ucr_Handler;
    PFNVDD_UTERMINATE Uterm_Handler;
    PFNVDD_UBLOCK     Ublock_Handler;
    PFNVDD_URESUME    Uresume_Handler;
} VDD_USER_HANDLERS, *PVDD_USER_HANDLERS;

/* PRIVATE VARIABLES **********************************************************/

// TODO: Maybe use a linked list.
// But the number of elements must be <= MAXUSHORT (MAXWORD)
#define MAX_VDD_MODULES 0xFF + 1
static VDD_MODULE VDDList[MAX_VDD_MODULES] = {{NULL}};

// Valid handles of VDD DLLs start at 1 and finish at MAX_VDD_MODULES
#define ENTRY_TO_HANDLE(Entry)  ((Entry)  + 1)
#define HANDLE_TO_ENTRY(Handle) ((Handle) - 1)
#define IS_VALID_HANDLE(Handle) ((Handle) > 0 && (Handle) <= MAX_VDD_MODULES)

static LIST_ENTRY VddUserHooksList = {&VddUserHooksList, &VddUserHooksList};

/* PRIVATE FUNCTIONS **********************************************************/

static USHORT GetNextFreeVDDEntry(VOID)
{
    USHORT Entry;
    for (Entry = 0; Entry < ARRAYSIZE(VDDList); ++Entry)
    {
        if (VDDList[Entry].hDll == NULL) break;
    }
    return Entry;
}

static VOID WINAPI ThirdPartyVDDBop(LPWORD Stack)
{
    /* Get the Function Number and skip it */
    BYTE FuncNum = *(PBYTE)SEG_OFF_TO_PTR(getCS(), getIP());
    setIP(getIP() + 1);

    switch (FuncNum)
    {
        /* RegisterModule */
        case 0:
        {
            BOOL Success = TRUE;
            WORD RetVal  = 0;
            WORD Entry   = 0;
            LPCSTR DllName = NULL,
                   InitRoutineName     = NULL,
                   DispatchRoutineName = NULL;
            HMODULE hDll = NULL;
            VDD_PROC InitRoutine     = NULL,
                     DispatchRoutine = NULL;

            DPRINT("RegisterModule() called\n");

            /* Clear the Carry Flag (no error happened so far) */
            setCF(0);

            /* Retrieve the next free entry in the table (used later on) */
            Entry = GetNextFreeVDDEntry();
            if (Entry >= MAX_VDD_MODULES)
            {
                DPRINT1("Failed to create a new VDD module entry\n");
                Success = FALSE;
                RetVal = 4;
                goto Quit;
            }

            /* Retrieve the VDD name in DS:SI */
            DllName = (LPCSTR)SEG_OFF_TO_PTR(getDS(), getSI());

            /* Retrieve the initialization routine API name in ES:DI (optional --> ES=DI=0) */
            if (TO_LINEAR(getES(), getDI()) != 0)
                InitRoutineName = (LPCSTR)SEG_OFF_TO_PTR(getES(), getDI());

            /* Retrieve the dispatch routine API name in DS:BX */
            DispatchRoutineName = (LPCSTR)SEG_OFF_TO_PTR(getDS(), getBX());

            DPRINT1("DllName = '%s' - InitRoutineName = '%s' - DispatchRoutineName = '%s'\n",
                    (DllName ? DllName : "n/a"),
                    (InitRoutineName ? InitRoutineName : "n/a"),
                    (DispatchRoutineName ? DispatchRoutineName : "n/a"));

            /* Load the VDD DLL */
            hDll = LoadLibraryA(DllName);
            if (hDll == NULL)
            {
                DWORD LastError = GetLastError();
                Success = FALSE;

                if (LastError == ERROR_NOT_ENOUGH_MEMORY)
                {
                    DPRINT1("Not enough memory to load DLL '%s'\n", DllName);
                    RetVal = 4;
                    goto Quit;
                }
                else
                {
                    DPRINT1("Failed to load DLL '%s'; last error = %d\n", DllName, LastError);
                    RetVal = 1;
                    goto Quit;
                }
            }

            /* Load the initialization routine if needed */
            if (InitRoutineName)
            {
                InitRoutine = (VDD_PROC)GetProcAddress(hDll, InitRoutineName);
                if (InitRoutine == NULL)
                {
                    DPRINT1("Failed to load the initialization routine '%s'\n", InitRoutineName);
                    Success = FALSE;
                    RetVal = 3;
                    goto Quit;
                }
            }

            /* Load the dispatch routine */
            DispatchRoutine = (VDD_PROC)GetProcAddress(hDll, DispatchRoutineName);
            if (DispatchRoutine == NULL)
            {
                DPRINT1("Failed to load the dispatch routine '%s'\n", DispatchRoutineName);
                Success = FALSE;
                RetVal = 2;
                goto Quit;
            }

            /* If we reached this point, that means everything is OK */

            /* Register the VDD DLL */
            VDDList[Entry].hDll = hDll;
            VDDList[Entry].DispatchRoutine = DispatchRoutine;

            /* Call the initialization routine if needed */
            if (InitRoutine) InitRoutine();

            /* We succeeded. RetVal will contain a valid VDD DLL handle */
            Success = TRUE;
            RetVal  = ENTRY_TO_HANDLE(Entry); // Convert the entry to a valid handle

Quit:
            if (!Success)
            {
                /* Unload the VDD DLL */
                if (hDll) FreeLibrary(hDll);

                /* Set the Carry Flag to indicate that an error happened */
                setCF(1);
            }
            // else
            // {
                // /* Clear the Carry Flag (success) */
                // setCF(0);
            // }
            setAX(RetVal);
            break;
        }

        /* UnRegisterModule */
        case 1:
        {
            WORD Handle = getAX();
            WORD Entry  = HANDLE_TO_ENTRY(Handle); // Convert the handle to a valid entry

            DPRINT("UnRegisterModule() called\n");

            /* Sanity checks */
            if (!IS_VALID_HANDLE(Handle) || VDDList[Entry].hDll == NULL)
            {
                DPRINT1("Invalid VDD DLL Handle: %d\n", Entry);
                /* Stop the VDM */
                EmulatorTerminate();
                return;
            }

            /* Unregister the VDD DLL */
            FreeLibrary(VDDList[Entry].hDll);
            VDDList[Entry].hDll = NULL;
            VDDList[Entry].DispatchRoutine = NULL;
            break;
        }

        /* DispatchCall */
        case 2:
        {
            WORD Handle = getAX();
            WORD Entry  = HANDLE_TO_ENTRY(Handle); // Convert the handle to a valid entry

            DPRINT("DispatchCall() called\n");

            /* Sanity checks */
            if (!IS_VALID_HANDLE(Handle)    ||
                VDDList[Entry].hDll == NULL ||
                VDDList[Entry].DispatchRoutine == NULL)
            {
                DPRINT1("Invalid VDD DLL Handle: %d\n", Entry);
                /* Stop the VDM */
                EmulatorTerminate();
                return;
            }

            /* Call the dispatch routine */
            VDDList[Entry].DispatchRoutine();
            break;
        }

        default:
        {
            DPRINT1("Unknown 3rd-party VDD BOP Function: 0x%02X\n", FuncNum);
            setCF(1);
            break;
        }
    }
}

static BOOL LoadInstallableVDD(VOID)
{
// FIXME: These strings should be localized.
#define ERROR_MEMORYVDD L"Insufficient memory to load installable Virtual Device Drivers."
#define ERROR_REGVDD    L"Virtual Device Driver format in the registry is invalid."
#define ERROR_LOADVDD   L"An installable Virtual Device Driver failed Dll initialization."

    BOOL  Success = TRUE;
    LONG  Error   = 0;
    DWORD Type    = 0;
    DWORD BufSize = 0;

    HKEY    hVDDKey;
    LPCWSTR VDDKeyName   = L"SYSTEM\\CurrentControlSet\\Control\\VirtualDeviceDrivers";
    LPWSTR  VDDValueName = L"VDD";
    LPWSTR  VDDList      = NULL;

    HANDLE hVDD;

    /* Try to open the VDD registry key */
    Error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          VDDKeyName,
                          0,
                          KEY_QUERY_VALUE,
                          &hVDDKey);
    if (Error == ERROR_FILE_NOT_FOUND)
    {
        /* If the key just doesn't exist, don't do anything else */
        return TRUE;
    }
    else if (Error != ERROR_SUCCESS)
    {
        /* The key exists but there was an access error: display an error and quit */
        DisplayMessage(ERROR_REGVDD);
        return FALSE;
    }

    /*
     * Retrieve the size of the VDD registry value
     * and check that it's of REG_MULTI_SZ type.
     */
    Error = RegQueryValueExW(hVDDKey,
                             VDDValueName,
                             NULL,
                             &Type,
                             NULL,
                             &BufSize);
    if (Error == ERROR_FILE_NOT_FOUND)
    {
        /* If the value just doesn't exist, don't do anything else */
        Success = TRUE;
        goto Quit;
    }
    else if (Error != ERROR_SUCCESS || Type != REG_MULTI_SZ)
    {
        /*
         * The value exists but there was an access error or
         * is of the wrong type: display an error and quit.
         */
        DisplayMessage(ERROR_REGVDD);
        Success = FALSE;
        goto Quit;
    }

    /* Allocate the buffer */
    BufSize = (BufSize < 2*sizeof(WCHAR) ? 2*sizeof(WCHAR) : BufSize);
    VDDList = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, BufSize);
    if (VDDList == NULL)
    {
        DisplayMessage(ERROR_MEMORYVDD);
        Success = FALSE;
        goto Quit;
    }

    /* Retrieve the list of VDDs to load */
    if (RegQueryValueExW(hVDDKey,
                         VDDValueName,
                         NULL,
                         NULL,
                         (LPBYTE)VDDList,
                         &BufSize) != ERROR_SUCCESS)
    {
        DisplayMessage(ERROR_REGVDD);
        Success = FALSE;
        goto Quit;
    }

    /* Load the VDDs */
    VDDValueName = VDDList;
    while (*VDDList)
    {
        DPRINT1("Loading VDD '%S'... ", VDDList);
        hVDD = LoadLibraryW(VDDList);
        if (hVDD == NULL)
        {
            DbgPrint("Failed\n");
            DisplayMessage(ERROR_LOADVDD);
        }
        else
        {
            DbgPrint("Succeeded\n");
        }
        /* Go to next string */
        VDDList += wcslen(VDDList) + 1;
    }
    VDDList = VDDValueName;

Quit:
    if (VDDList) RtlFreeHeap(RtlGetProcessHeap(), 0, VDDList);
    RegCloseKey(hVDDKey);
    return Success;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * NOTE: This function can be called multiple times by the same VDD, if
 * it wants to install different hooks for a same action. The most recent
 * registered hooks are called first.
 */
BOOL
WINAPI
VDDInstallUserHook(IN HANDLE hVdd,
                   IN PFNVDD_UCREATE Ucr_Handler,
                   IN PFNVDD_UTERMINATE Uterm_Handler,
                   IN PFNVDD_UBLOCK Ublock_Handler,
                   IN PFNVDD_URESUME Uresume_Handler)
{
    PVDD_USER_HANDLERS UserHook;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // NOTE: If we want that a VDD can install hooks only once, it's here
    // that we need to check whether a hook entry is already registered.

    /* Create and initialize a new hook entry... */
    UserHook = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*UserHook));
    if (UserHook == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    UserHook->hVdd            = hVdd;
    UserHook->Ucr_Handler     = Ucr_Handler;
    UserHook->Uterm_Handler   = Uterm_Handler;
    UserHook->Ublock_Handler  = Ublock_Handler;
    UserHook->Uresume_Handler = Uresume_Handler;

    /* ... and add it at the top of the list of hooks */
    InsertHeadList(&VddUserHooksList, &UserHook->Entry);

    return TRUE;
}

/*
 * NOTE: This function uninstalls the latest installed hooks for a given VDD.
 * It can be called multiple times by the same VDD to uninstall many hooks
 * installed by multiple invocations of VDDInstallUserHook.
 */
BOOL
WINAPI
VDDDeInstallUserHook(IN HANDLE hVdd)
{
    PLIST_ENTRY Pointer;
    PVDD_USER_HANDLERS UserHook;

    /* Check validity of the VDD handle */
    if (hVdd == NULL || hVdd == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Uninstall the latest installed hooks */
    for (Pointer = VddUserHooksList.Flink; Pointer != &VddUserHooksList; Pointer = Pointer->Flink)
    {
        UserHook = CONTAINING_RECORD(Pointer, VDD_USER_HANDLERS, Entry);
        if (UserHook->hVdd == hVdd)
        {
            RemoveEntryList(&UserHook->Entry);
            RtlFreeHeap(RtlGetProcessHeap(), 0, UserHook);
            return TRUE;
        }
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/*
 * Internal functions for calling the VDD user hooks.
 * Their names come directly from the Windows 2kX DDK.
 */

VOID VDDCreateUserHook(USHORT DosPDB)
{
    PLIST_ENTRY Pointer;
    PVDD_USER_HANDLERS UserHook;

    /* Call the hooks starting from the most recent ones */
    for (Pointer = VddUserHooksList.Flink; Pointer != &VddUserHooksList; Pointer = Pointer->Flink)
    {
        UserHook = CONTAINING_RECORD(Pointer, VDD_USER_HANDLERS, Entry);
        if (UserHook->Ucr_Handler) UserHook->Ucr_Handler(DosPDB);
    }
}

VOID VDDTerminateUserHook(USHORT DosPDB)
{
    PLIST_ENTRY Pointer;
    PVDD_USER_HANDLERS UserHook;

    /* Call the hooks starting from the most recent ones */
    for (Pointer = VddUserHooksList.Flink; Pointer != &VddUserHooksList; Pointer = Pointer->Flink)
    {
        UserHook = CONTAINING_RECORD(Pointer, VDD_USER_HANDLERS, Entry);
        if (UserHook->Uterm_Handler) UserHook->Uterm_Handler(DosPDB);
    }
}

VOID VDDBlockUserHook(VOID)
{
    PLIST_ENTRY Pointer;
    PVDD_USER_HANDLERS UserHook;

    /* Call the hooks starting from the most recent ones */
    for (Pointer = VddUserHooksList.Flink; Pointer != &VddUserHooksList; Pointer = Pointer->Flink)
    {
        UserHook = CONTAINING_RECORD(Pointer, VDD_USER_HANDLERS, Entry);
        if (UserHook->Ublock_Handler) UserHook->Ublock_Handler();
    }
}

VOID VDDResumeUserHook(VOID)
{
    PLIST_ENTRY Pointer;
    PVDD_USER_HANDLERS UserHook;

    /* Call the hooks starting from the most recent ones */
    for (Pointer = VddUserHooksList.Flink; Pointer != &VddUserHooksList; Pointer = Pointer->Flink)
    {
        UserHook = CONTAINING_RECORD(Pointer, VDD_USER_HANDLERS, Entry);
        if (UserHook->Uresume_Handler) UserHook->Uresume_Handler();
    }
}


VOID VDDSupInitialize(VOID)
{
    /* Register the 3rd-party VDD BOP Handler */
    RegisterBop(BOP_3RDPARTY, ThirdPartyVDDBop);

    /* Load the installable VDDs from the registry */
    LoadInstallableVDD();
}

/* EOF */
