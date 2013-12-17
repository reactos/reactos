/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            vddsup.c
 * PURPOSE:         Virtual Device Drivers (VDD) Support
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "vddsup.h"

#include "bop.h"
#include "registers.h"

typedef VOID (WINAPI *VDD_PROC)(VOID);

typedef struct _VDD_MODULE
{
    HMODULE  hDll;
    VDD_PROC DispatchRoutine;
} VDD_MODULE, *PVDD_MODULE;

/* BOP Identifiers */
#define BOP_3RDPARTY    0x58    // 3rd-party VDD BOP

/* PRIVATE VARIABLES **********************************************************/

// TODO: Maybe use a linked list.
// But the number of elements must be <= MAXUSHORT
#define MAX_VDD_MODULES 0xFF + 1
VDD_MODULE VDDList[MAX_VDD_MODULES] = {{NULL}};

// Valid handles of VDD DLLs start at 1 and finish at MAX_VDD_MODULES
#define ENTRY_TO_HANDLE(Entry)  ((Entry)  + 1)
#define HANDLE_TO_ENTRY(Handle) ((Handle) - 1)
#define IS_VALID_HANDLE(Handle) ((Handle) > 0 && (Handle) <= MAX_VDD_MODULES)

/* PRIVATE FUNCTIONS **********************************************************/

USHORT GetNextFreeVDDEntry(VOID)
{
    USHORT Entry = MAX_VDD_MODULES;
    for (Entry = 0; Entry < sizeof(VDDList)/sizeof(VDDList[0]); ++Entry)
    {
        if (VDDList[Entry].hDll == NULL) break;
    }
    return Entry;
}

VOID WINAPI ThirdPartyVDDBop(LPWORD Stack)
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

            DPRINT1("RegisterModule() called\n");

            /* Clear the Carry Flag (no error happened so far) */
            setCF(0);

            /* Retrieve the VDD name in DS:SI */
            DllName = (LPCSTR)SEG_OFF_TO_PTR(getDS(), getSI());

            /* Retrieve the initialization routine API name in ES:DI (optional --> ES=DI=0) */
            if (getES() != 0 || getDI() != 0)
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

            /* If we arrived there, that means everything is OK */

            /* Register the VDD DLL */
            Entry = GetNextFreeVDDEntry();
            if (Entry == MAX_VDD_MODULES)
            {
                DPRINT1("Failed to create a new VDD module entry\n");
                Success = FALSE;
                RetVal = 4;
                goto Quit;
            }
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

            DPRINT1("UnRegisterModule() called\n");

            /* Sanity checks */
            if (!IS_VALID_HANDLE(Handle) || VDDList[Entry].hDll == NULL)
            {
                DPRINT1("Invalid VDD DLL Handle: %d\n", Entry);
                /* Stop the VDM */
                VdmRunning = FALSE;
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

            DPRINT1("DispatchCall() called\n");

            /* Sanity checks */
            if (!IS_VALID_HANDLE(Handle)    ||
                VDDList[Entry].hDll == NULL ||
                VDDList[Entry].DispatchRoutine == NULL)
            {
                DPRINT1("Invalid VDD DLL Handle: %d\n", Entry);
                /* Stop the VDM */
                VdmRunning = FALSE;
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

/* PUBLIC FUNCTIONS ***********************************************************/

VOID VDDSupInitialize(VOID)
{
    /* Register the 3rd-party VDD BOP Handler */
    RegisterBop(BOP_3RDPARTY, ThirdPartyVDDBop);
}

/* EOF */
