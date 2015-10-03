/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/ntvdm.c
 * PURPOSE:         Virtual DOS Machine
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "resource.h"

/* VARIABLES ******************************************************************/

static HANDLE CurrentConsoleOutput = INVALID_HANDLE_VALUE;

static HANDLE ConsoleInput  = INVALID_HANDLE_VALUE;
static HANDLE ConsoleOutput = INVALID_HANDLE_VALUE;
static DWORD  OrgConsoleInputMode, OrgConsoleOutputMode;

NTVDM_SETTINGS GlobalSettings;

// Command line of NTVDM
INT     NtVdmArgc;
WCHAR** NtVdmArgv;

HWND hConsoleWnd = NULL;
static HMENU hConsoleMenu  = NULL;
static INT   VdmMenuPos    = -1;
static BOOLEAN ShowPointer = TRUE;

/*
 * Those menu helpers were taken from the GUI frontend in winsrv.dll
 */
typedef struct _VDM_MENUITEM
{
    UINT uID;
    const struct _VDM_MENUITEM *SubMenu;
    UINT_PTR uCmdID;
} VDM_MENUITEM, *PVDM_MENUITEM;

static const VDM_MENUITEM VdmMenuItems[] =
{
    { IDS_VDM_DUMPMEM_TXT, NULL, ID_VDM_DUMPMEM_TXT },
    { IDS_VDM_DUMPMEM_BIN, NULL, ID_VDM_DUMPMEM_BIN },
    { -1, NULL, 0 },    /* Separator */
    // { IDS_VDM_MOUNT_FLOPPY, NULL, ID_VDM_DRIVES },
    // { IDS_VDM_EJECT_FLOPPY, NULL, ID_VDM_DRIVES },
    { -1, NULL, 0 },    /* Separator */
    { IDS_VDM_QUIT       , NULL, ID_VDM_QUIT        },

    { 0, NULL, 0 }      /* End of list */
};

static const VDM_MENUITEM VdmMainMenuItems[] =
{
    { -1, NULL, 0 },    /* Separator */
    { IDS_HIDE_MOUSE,   NULL, ID_SHOWHIDE_MOUSE },  /* "Hide mouse"; can be renamed to "Show mouse" */
    { IDS_VDM_MENU  ,   VdmMenuItems,         0 },  /* ReactOS VDM Menu */

    { 0, NULL, 0 }      /* End of list */
};

static VOID
AppendMenuItems(HMENU hMenu,
                const VDM_MENUITEM *Items)
{
    UINT i = 0;
    WCHAR szMenuString[256];
    HMENU hSubMenu;

    do
    {
        if (Items[i].uID != (UINT)-1)
        {
            if (LoadStringW(GetModuleHandle(NULL),
                            Items[i].uID,
                            szMenuString,
                            ARRAYSIZE(szMenuString)) > 0)
            {
                if (Items[i].SubMenu != NULL)
                {
                    hSubMenu = CreatePopupMenu();
                    if (hSubMenu != NULL)
                    {
                        AppendMenuItems(hSubMenu, Items[i].SubMenu);

                        if (!AppendMenuW(hMenu,
                                         MF_STRING | MF_POPUP,
                                         (UINT_PTR)hSubMenu,
                                         szMenuString))
                        {
                            DestroyMenu(hSubMenu);
                        }
                    }
                }
                else
                {
                    AppendMenuW(hMenu,
                                MF_STRING,
                                Items[i].uCmdID,
                                szMenuString);
                }
            }
        }
        else
        {
            AppendMenuW(hMenu,
                        MF_SEPARATOR,
                        0,
                        NULL);
        }
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].uCmdID == 0));
}

BOOL
VdmMenuExists(HMENU hConsoleMenu)
{
    INT MenuPos, i;
    MenuPos = GetMenuItemCount(hConsoleMenu);

    /* Check for the presence of one of the VDM menu items */
    for (i = 0; i <= MenuPos; i++)
    {
        if (GetMenuItemID(hConsoleMenu, i) == ID_SHOWHIDE_MOUSE)
        {
            /* Set VdmMenuPos to the position of the existing menu */
            VdmMenuPos = i - 1;
            return TRUE;
        }
    }
    return FALSE;
}

static VOID
UpdateVdmMenuMouse(VOID)
{
    WCHAR szMenuString[256];

    /* Update "Hide/Show mouse" menu item */
    if (LoadStringW(GetModuleHandle(NULL),
                    (!ShowPointer ? IDS_SHOW_MOUSE : IDS_HIDE_MOUSE),
                    szMenuString,
                    ARRAYSIZE(szMenuString)) > 0)
    {
        ModifyMenuW(hConsoleMenu, ID_SHOWHIDE_MOUSE,
                    MF_BYCOMMAND, ID_SHOWHIDE_MOUSE, szMenuString);
    }
}

/*static*/ VOID
UpdateVdmMenuDisks(VOID)
{
    UINT_PTR ItemID;
    UNICODE_STRING ValueString;
    USHORT i;

    WCHAR szNoMedia[100];
    WCHAR szMenuString1[256], szMenuString2[256];

    /* Update the disks menu items */

    LoadStringW(GetModuleHandle(NULL),
                IDS_NO_MEDIA,
                szNoMedia,
                ARRAYSIZE(szNoMedia));

    LoadStringW(GetModuleHandle(NULL),
                IDS_VDM_MOUNT_FLOPPY,
                szMenuString1,
                ARRAYSIZE(szMenuString1));

    for (i = 0; i < ARRAYSIZE(GlobalSettings.FloppyDisks); ++i)
    {
        ItemID = ID_VDM_DRIVES + (2 * i);

        if (GlobalSettings.FloppyDisks[i].Length != 0 &&
            GlobalSettings.FloppyDisks[i].Buffer      &&
            GlobalSettings.FloppyDisks[i].Buffer != '\0')
        {
            /* Convert the ANSI string to UNICODE */
            RtlAnsiStringToUnicodeString(&ValueString, &GlobalSettings.FloppyDisks[i], TRUE);

            /* Update item text */
            _snwprintf(szMenuString2, ARRAYSIZE(szMenuString2), szMenuString1, i, ValueString.Buffer);
            szMenuString2[ARRAYSIZE(szMenuString2) - 1] = UNICODE_NULL;
            ModifyMenuW(hConsoleMenu, ItemID, MF_BYCOMMAND | MF_STRING, ItemID, szMenuString2);

            RtlFreeUnicodeString(&ValueString);

            /* Enable the eject item */
            EnableMenuItem(hConsoleMenu, ItemID + 1, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            /* Update item text */
            _snwprintf(szMenuString2, ARRAYSIZE(szMenuString2), szMenuString1, i, szNoMedia);
            szMenuString2[ARRAYSIZE(szMenuString2) - 1] = UNICODE_NULL;
            ModifyMenuW(hConsoleMenu, ItemID, MF_BYCOMMAND | MF_STRING, ItemID, szMenuString2);

            /* Disable the eject item */
            EnableMenuItem(hConsoleMenu, ItemID + 1, MF_BYCOMMAND | MF_GRAYED);
        }
    }
}

static VOID
UpdateVdmMenu(VOID)
{
    UpdateVdmMenuMouse();
    UpdateVdmMenuDisks();
}

static VOID
CreateVdmMenu(HANDLE ConOutHandle)
{
    HMENU hVdmSubMenu;
    UINT_PTR ItemID;
    UINT Pos;
    USHORT i;
    WCHAR szNoMedia[100];
    WCHAR szMenuString1[256], szMenuString2[256];

    hConsoleMenu = ConsoleMenuControl(ConOutHandle,
                                      ID_SHOWHIDE_MOUSE,
                                      ID_VDM_DRIVES + (2 * ARRAYSIZE(GlobalSettings.FloppyDisks)));
    if (hConsoleMenu == NULL) return;

    /* Get the position where we are going to insert our menu items */
    VdmMenuPos = GetMenuItemCount(hConsoleMenu);

    /* Really add the menu if it doesn't already exist (in case eg. NTVDM crashed) */
    if (!VdmMenuExists(hConsoleMenu))
    {
        /* Add all the menu entries */
        AppendMenuItems(hConsoleMenu, VdmMainMenuItems);

        /* Add the removable drives menu entries */
        hVdmSubMenu = GetSubMenu(hConsoleMenu, VdmMenuPos + 2); // VdmMenuItems
        Pos = 3; // After the 2 items and the separator in VdmMenuItems

        LoadStringW(GetModuleHandle(NULL),
                    IDS_NO_MEDIA,
                    szNoMedia,
                    ARRAYSIZE(szNoMedia));

        LoadStringW(GetModuleHandle(NULL),
                    IDS_VDM_MOUNT_FLOPPY,
                    szMenuString1,
                    ARRAYSIZE(szMenuString1));

        /* Drive 'x' -- Mount */
        for (i = 0; i < ARRAYSIZE(GlobalSettings.FloppyDisks); ++i)
        {
            ItemID = ID_VDM_DRIVES + (2 * i);

            /* Add the item */
            _snwprintf(szMenuString2, ARRAYSIZE(szMenuString2), szMenuString1, i, szNoMedia);
            szMenuString2[ARRAYSIZE(szMenuString2) - 1] = UNICODE_NULL;
            InsertMenuW(hVdmSubMenu, Pos++, MF_STRING | MF_BYPOSITION, ItemID, szMenuString2);
        }

        LoadStringW(GetModuleHandle(NULL),
                    IDS_VDM_EJECT_FLOPPY,
                    szMenuString1,
                    ARRAYSIZE(szMenuString1));

        /* Drive 'x' -- Eject */
        for (i = 0; i < ARRAYSIZE(GlobalSettings.FloppyDisks); ++i)
        {
            ItemID = ID_VDM_DRIVES + (2 * i);

            /* Add the item */
            _snwprintf(szMenuString2, ARRAYSIZE(szMenuString2), szMenuString1, i);
            szMenuString2[ARRAYSIZE(szMenuString2) - 1] = UNICODE_NULL;
            InsertMenuW(hVdmSubMenu, Pos++, MF_STRING | MF_BYPOSITION, ItemID + 1, szMenuString2);
        }

        /* Refresh the menu state */
        UpdateVdmMenu();
        DrawMenuBar(hConsoleWnd);
    }
}

static VOID
DestroyVdmMenu(VOID)
{
    UINT i = 0;
    const VDM_MENUITEM *Items = VdmMainMenuItems;

    do
    {
        DeleteMenu(hConsoleMenu, VdmMenuPos, MF_BYPOSITION);
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].uCmdID == 0));

    DrawMenuBar(hConsoleWnd);
}

static VOID ShowHideMousePointer(HANDLE ConOutHandle, BOOLEAN ShowPtr)
{
    if (ShowPtr)
    {
        /* Be sure the cursor will be shown */
        while (ShowConsoleCursor(ConOutHandle, TRUE) < 0) ;
    }
    else
    {
        /* Be sure the cursor will be hidden */
        while (ShowConsoleCursor(ConOutHandle, FALSE) >= 0) ;
    }
}

static VOID EnableExtraHardware(HANDLE ConsoleInput)
{
    DWORD ConInMode;

    if (GetConsoleMode(ConsoleInput, &ConInMode))
    {
#if 0
        // GetNumberOfConsoleMouseButtons();
        // GetSystemMetrics(SM_CMOUSEBUTTONS);
        // GetSystemMetrics(SM_MOUSEPRESENT);
        if (MousePresent)
        {
#endif
            /* Support mouse input events if there is a mouse on the system */
            ConInMode |= ENABLE_MOUSE_INPUT;
#if 0
        }
        else
        {
            /* Do not support mouse input events if there is no mouse on the system */
            ConInMode &= ~ENABLE_MOUSE_INPUT;
        }
#endif

        SetConsoleMode(ConsoleInput, ConInMode);
    }
}


static NTSTATUS
NTAPI
NtVdmConfigureBios(IN PWSTR ValueName,
                   IN ULONG ValueType,
                   IN PVOID ValueData,
                   IN ULONG ValueLength,
                   IN PVOID Context,
                   IN PVOID EntryContext)
{
    PNTVDM_SETTINGS Settings = (PNTVDM_SETTINGS)Context;
    UNICODE_STRING ValueString;

    /* Check for the type of the value */
    if (ValueType != REG_SZ)
    {
        RtlInitEmptyAnsiString(&Settings->BiosFileName, NULL, 0);
        return STATUS_SUCCESS;
    }

    /* Convert the UNICODE string to ANSI and store it */
    RtlInitEmptyUnicodeString(&ValueString, (PWCHAR)ValueData, ValueLength);
    ValueString.Length = ValueString.MaximumLength;
    RtlUnicodeStringToAnsiString(&Settings->BiosFileName, &ValueString, TRUE);

    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
NtVdmConfigureRom(IN PWSTR ValueName,
                  IN ULONG ValueType,
                  IN PVOID ValueData,
                  IN ULONG ValueLength,
                  IN PVOID Context,
                  IN PVOID EntryContext)
{
    PNTVDM_SETTINGS Settings = (PNTVDM_SETTINGS)Context;
    UNICODE_STRING ValueString;

    /* Check for the type of the value */
    if (ValueType != REG_MULTI_SZ)
    {
        RtlInitEmptyAnsiString(&Settings->RomFiles, NULL, 0);
        return STATUS_SUCCESS;
    }

    /* Convert the UNICODE string to ANSI and store it */
    RtlInitEmptyUnicodeString(&ValueString, (PWCHAR)ValueData, ValueLength);
    ValueString.Length = ValueString.MaximumLength;
    RtlUnicodeStringToAnsiString(&Settings->RomFiles, &ValueString, TRUE);

    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
NtVdmConfigureFloppy(IN PWSTR ValueName,
                     IN ULONG ValueType,
                     IN PVOID ValueData,
                     IN ULONG ValueLength,
                     IN PVOID Context,
                     IN PVOID EntryContext)
{
    PNTVDM_SETTINGS Settings = (PNTVDM_SETTINGS)Context;
    UNICODE_STRING ValueString;
    ULONG DiskNumber = (ULONG)EntryContext;

    ASSERT(DiskNumber < ARRAYSIZE(Settings->FloppyDisks));

    /* Check whether the Hard Disk entry was not already configured */
    if (Settings->FloppyDisks[DiskNumber].Buffer != NULL)
    {
        DPRINT1("Floppy Disk %d -- '%Z' already configured\n", DiskNumber, &Settings->FloppyDisks[DiskNumber]);
        return STATUS_SUCCESS;
    }

    /* Check for the type of the value */
    if (ValueType != REG_SZ)
    {
        RtlInitEmptyAnsiString(&Settings->FloppyDisks[DiskNumber], NULL, 0);
        return STATUS_SUCCESS;
    }

    /* Convert the UNICODE string to ANSI and store it */
    RtlInitEmptyUnicodeString(&ValueString, (PWCHAR)ValueData, ValueLength);
    ValueString.Length = ValueString.MaximumLength;
    RtlUnicodeStringToAnsiString(&Settings->FloppyDisks[DiskNumber], &ValueString, TRUE);

    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
NtVdmConfigureHDD(IN PWSTR ValueName,
                  IN ULONG ValueType,
                  IN PVOID ValueData,
                  IN ULONG ValueLength,
                  IN PVOID Context,
                  IN PVOID EntryContext)
{
    PNTVDM_SETTINGS Settings = (PNTVDM_SETTINGS)Context;
    UNICODE_STRING ValueString;
    ULONG DiskNumber = (ULONG)EntryContext;

    ASSERT(DiskNumber < ARRAYSIZE(Settings->HardDisks));

    /* Check whether the Hard Disk entry was not already configured */
    if (Settings->HardDisks[DiskNumber].Buffer != NULL)
    {
        DPRINT1("Hard Disk %d -- '%Z' already configured\n", DiskNumber, &Settings->HardDisks[DiskNumber]);
        return STATUS_SUCCESS;
    }

    /* Check for the type of the value */
    if (ValueType != REG_SZ)
    {
        RtlInitEmptyAnsiString(&Settings->HardDisks[DiskNumber], NULL, 0);
        return STATUS_SUCCESS;
    }

    /* Convert the UNICODE string to ANSI and store it */
    RtlInitEmptyUnicodeString(&ValueString, (PWCHAR)ValueData, ValueLength);
    ValueString.Length = ValueString.MaximumLength;
    RtlUnicodeStringToAnsiString(&Settings->HardDisks[DiskNumber], &ValueString, TRUE);

    return STATUS_SUCCESS;
}

static RTL_QUERY_REGISTRY_TABLE
NtVdmConfigurationTable[] =
{
    {
        NtVdmConfigureBios,
        0,
        L"BiosFile",
        NULL,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureRom,
        RTL_QUERY_REGISTRY_NOEXPAND,
        L"RomFiles",
        NULL,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureFloppy,
        0,
        L"FloppyDisk0",
        (PVOID)0,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureFloppy,
        0,
        L"FloppyDisk1",
        (PVOID)1,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureHDD,
        0,
        L"HardDisk0",
        (PVOID)0,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureHDD,
        0,
        L"HardDisk1",
        (PVOID)1,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureHDD,
        0,
        L"HardDisk2",
        (PVOID)2,
        REG_NONE,
        NULL,
        0
    },

    {
        NtVdmConfigureHDD,
        0,
        L"HardDisk3",
        (PVOID)3,
        REG_NONE,
        NULL,
        0
    },

    /* End of table */
    {0}
};

static BOOL
LoadGlobalSettings(IN PNTVDM_SETTINGS Settings)
{
    NTSTATUS Status;

    ASSERT(Settings);

    /*
     * Now we can do:
     * - CPU core choice
     * - Video choice
     * - Sound choice
     * - Mem?
     * - ...
     * - Standalone mode?
     * - Debug settings
     */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"NTVDM",
                                    NtVdmConfigurationTable,
                                    Settings,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NTVDM registry settings cannot be fully initialized, using default ones. Status = 0x%08lx\n", Status);
    }

    return NT_SUCCESS(Status);
}

static VOID
FreeGlobalSettings(IN PNTVDM_SETTINGS Settings)
{
    USHORT i;

    ASSERT(Settings);

    if (Settings->BiosFileName.Buffer)
        RtlFreeAnsiString(&Settings->BiosFileName);

    if (Settings->RomFiles.Buffer)
        RtlFreeAnsiString(&Settings->RomFiles);

    for (i = 0; i < ARRAYSIZE(Settings->FloppyDisks); ++i)
    {
        if (Settings->FloppyDisks[i].Buffer)
            RtlFreeAnsiString(&Settings->FloppyDisks[i]);
    }

    for (i = 0; i < ARRAYSIZE(Settings->HardDisks); ++i)
    {
        if (Settings->HardDisks[i].Buffer)
            RtlFreeAnsiString(&Settings->HardDisks[i]);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
DisplayMessage(IN LPCWSTR Format, ...)
{
#ifndef WIN2K_COMPLIANT
    WCHAR  StaticBuffer[256];
    LPWSTR Buffer = StaticBuffer; // Use the static buffer by default.
#else
    WCHAR  Buffer[2048]; // Large enough. If not, increase it by hand.
#endif
    size_t MsgLen;
    va_list args;

    va_start(args, Format);

#ifndef WIN2K_COMPLIANT
    /*
     * Retrieve the message length and if it is too long, allocate
     * an auxiliary buffer; otherwise use the static buffer.
     * The string is built to be NULL-terminated.
     */
    MsgLen = _vscwprintf(Format, args);
    if (MsgLen >= ARRAYSIZE(StaticBuffer))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, (MsgLen + 1) * sizeof(WCHAR));
        if (Buffer == NULL)
        {
            /* Allocation failed, use the static buffer and display a suitable error message */
            Buffer = StaticBuffer;
            Format = L"DisplayMessage()\nOriginal message is too long and allocating an auxiliary buffer failed.";
            MsgLen = wcslen(Format);
        }
    }
#else
    MsgLen = ARRAYSIZE(Buffer) - 1;
#endif

    RtlZeroMemory(Buffer, (MsgLen + 1) * sizeof(WCHAR));
    _vsnwprintf(Buffer, MsgLen, Format, args);

    va_end(args);

    /* Display the message */
    DPRINT1("\n\nNTVDM Subsystem\n%S\n\n", Buffer);
    MessageBoxW(hConsoleWnd, Buffer, L"NTVDM Subsystem", MB_OK);

#ifndef WIN2K_COMPLIANT
    /* Free the buffer if needed */
    if (Buffer != StaticBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
#endif
}

/*
 * This function, derived from DisplayMessage, is used by the BIOS and
 * the DOS to display messages to an output device. A printer function
 * is given for printing the characters.
 */
VOID
PrintMessageAnsi(IN CHAR_PRINT CharPrint,
                 IN LPCSTR Format, ...)
{
    static CHAR CurChar = 0;
    LPSTR str;

#ifndef WIN2K_COMPLIANT
    CHAR  StaticBuffer[256];
    LPSTR Buffer = StaticBuffer; // Use the static buffer by default.
#else
    CHAR  Buffer[2048]; // Large enough. If not, increase it by hand.
#endif
    size_t MsgLen;
    va_list args;

    va_start(args, Format);

#ifndef WIN2K_COMPLIANT
    /*
     * Retrieve the message length and if it is too long, allocate
     * an auxiliary buffer; otherwise use the static buffer.
     * The string is built to be NULL-terminated.
     */
    MsgLen = _vscprintf(Format, args);
    if (MsgLen >= ARRAYSIZE(StaticBuffer))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, (MsgLen + 1) * sizeof(CHAR));
        if (Buffer == NULL)
        {
            /* Allocation failed, use the static buffer and display a suitable error message */
            Buffer = StaticBuffer;
            Format = "DisplayMessageAnsi()\nOriginal message is too long and allocating an auxiliary buffer failed.";
            MsgLen = strlen(Format);
        }
    }
#else
    MsgLen = ARRAYSIZE(Buffer) - 1;
#endif

    RtlZeroMemory(Buffer, (MsgLen + 1) * sizeof(CHAR));
    _vsnprintf(Buffer, MsgLen, Format, args);

    va_end(args);

    /* Display the message */
    // DPRINT1("\n\nNTVDM DOS32\n%s\n\n", Buffer);

    MsgLen = strlen(Buffer);
    str = Buffer;
    while (MsgLen--)
    {
        if (*str == '\n' && CurChar != '\r')
            CharPrint('\r');

        CurChar = *str++;
        CharPrint(CurChar);
    }

#ifndef WIN2K_COMPLIANT
    /* Free the buffer if needed */
    if (Buffer != StaticBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
#endif
}

static VOID
ConsoleCleanup(VOID);

static VOID
VdmShutdown(BOOLEAN Immediate)
{
    /*
     * Immediate = TRUE:  Immediate shutdown;
     *             FALSE: Delayed shutdown.
     */
    static BOOLEAN MustShutdown = FALSE;

    /* If a shutdown is ongoing, just return */
    if (MustShutdown)
    {
        DPRINT1("Shutdown is ongoing...\n");
        Sleep(INFINITE);
        return;
    }

    /* First notify DOS to see whether we can shut down now */
    MustShutdown = DosShutdown(Immediate);
    /*
     * In case we perform an immediate shutdown, or the DOS says
     * we can shut down, do it now.
     */
    MustShutdown = MustShutdown || Immediate;

    if (MustShutdown)
    {
        EmulatorTerminate();

        BiosCleanup();
        EmulatorCleanup();
        ConsoleCleanup();

        FreeGlobalSettings(&GlobalSettings);

        DPRINT1("\n\n\nNTVDM - Exiting...\n\n\n");
        /* Some VDDs rely on the fact that NTVDM calls ExitProcess on Windows */
        ExitProcess(0);
    }
}

static BOOL
WINAPI
ConsoleCtrlHandler(DWORD ControlType)
{
    switch (ControlType)
    {
        case CTRL_LAST_CLOSE_EVENT:
        {
            /* Delayed shutdown */
            DPRINT1("NTVDM delayed killing in the CTRL_LAST_CLOSE_EVENT CtrlHandler!\n");
            VdmShutdown(FALSE);
            break;
        }

        default:
        {
            /* Stop the VDM if the user logs out or closes the console */
            DPRINT1("Killing NTVDM in the 'default' CtrlHandler!\n");
            VdmShutdown(TRUE);
        }
    }
    return TRUE;
}

static VOID
ConsoleInitUI(VOID)
{
    hConsoleWnd = GetConsoleWindow();
    CreateVdmMenu(ConsoleOutput);
}

static VOID
ConsoleCleanupUI(VOID)
{
    /* Display again properly the mouse pointer */
    if (ShowPointer) ShowHideMousePointer(ConsoleOutput, ShowPointer);

    DestroyVdmMenu();
}

BOOL
ConsoleAttach(VOID)
{
    /* Save the original input and output console modes */
    if (!GetConsoleMode(ConsoleInput , &OrgConsoleInputMode ) ||
        !GetConsoleMode(ConsoleOutput, &OrgConsoleOutputMode))
    {
        CloseHandle(ConsoleOutput);
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot save console in/out modes\n");
        return FALSE;
    }

    /* Set the console input mode */
    // FIXME: Activate ENABLE_WINDOW_INPUT when we will want to perform actions
    // upon console window events (screen buffer resize, ...).
    SetConsoleMode(ConsoleInput, 0 /* | ENABLE_WINDOW_INPUT */);
    EnableExtraHardware(ConsoleInput);

    /* Set the console output mode */
    // SetConsoleMode(ConsoleOutput, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);

    /* Initialize the UI */
    ConsoleInitUI();

    return TRUE;
}

VOID
ConsoleDetach(VOID)
{
    /* Cleanup the UI */
    ConsoleCleanupUI();

    /* Restore the original input and output console modes */
    SetConsoleMode(ConsoleOutput, OrgConsoleOutputMode);
    SetConsoleMode(ConsoleInput , OrgConsoleInputMode );
}

VOID
ConsoleReattach(HANDLE ConOutHandle)
{
    DestroyVdmMenu();
    CurrentConsoleOutput = ConOutHandle;
    CreateVdmMenu(ConOutHandle);

    /* Synchronize mouse cursor display with console screenbuffer switches */
    ShowHideMousePointer(CurrentConsoleOutput, ShowPointer);
}

static BOOL
ConsoleInit(VOID)
{
    /* Set the handler routine */
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    /* Enable the CTRL_LAST_CLOSE_EVENT */
    SetLastConsoleEventActive();

    /*
     * NOTE: The CONIN$ and CONOUT$ "virtual" files
     * always point to non-redirected console handles.
     */

    /* Get the input handle to the real console, and check for success */
    ConsoleInput = CreateFileW(L"CONIN$",
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
    if (ConsoleInput == INVALID_HANDLE_VALUE)
    {
        wprintf(L"FATAL: Cannot retrieve a handle to the console input\n");
        return FALSE;
    }

    /* Get the output handle to the real console, and check for success */
    ConsoleOutput = CreateFileW(L"CONOUT$",
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
    if (ConsoleOutput == INVALID_HANDLE_VALUE)
    {
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot retrieve a handle to the console output\n");
        return FALSE;
    }

    /* Effectively attach to the console */
    return ConsoleAttach();
}

static VOID
ConsoleCleanup(VOID)
{
    /* Detach from the console */
    ConsoleDetach();

    /* Close the console handles */
    if (ConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(ConsoleOutput);
    if (ConsoleInput  != INVALID_HANDLE_VALUE) CloseHandle(ConsoleInput);
}

VOID MenuEventHandler(PMENU_EVENT_RECORD MenuEvent)
{
    switch (MenuEvent->dwCommandId)
    {
        case ID_SHOWHIDE_MOUSE:
            ShowPointer = !ShowPointer;
            ShowHideMousePointer(CurrentConsoleOutput, ShowPointer);
            UpdateVdmMenuMouse();
            break;

        case ID_VDM_DUMPMEM_TXT:
            DumpMemory(TRUE);
            break;

        case ID_VDM_DUMPMEM_BIN:
            DumpMemory(FALSE);
            break;

        /* Drive 0 -- Mount */
        /* Drive 1 -- Mount */
        case ID_VDM_DRIVES + 0:
        case ID_VDM_DRIVES + 2:
        {
            ULONG DiskNumber = (MenuEvent->dwCommandId - ID_VDM_DRIVES) / 2;
            MountFloppy(DiskNumber);
            break;
        }

        /* Drive 0 -- Eject */
        /* Drive 1 -- Eject */
        case ID_VDM_DRIVES + 1:
        case ID_VDM_DRIVES + 3:
        {
            ULONG DiskNumber = (MenuEvent->dwCommandId - ID_VDM_DRIVES - 1) / 2;
            EjectFloppy(DiskNumber);
            break;
        }

        case ID_VDM_QUIT:
            /* Stop the VDM */
            // EmulatorTerminate();

            /* Nothing runs, so exit immediately */
            DPRINT1("Killing NTVDM via console menu!\n");
            VdmShutdown(TRUE);
            break;

        default:
            break;
    }
}

VOID FocusEventHandler(PFOCUS_EVENT_RECORD FocusEvent)
{
    DPRINT1("Focus events not handled\n");
}


INT
wmain(INT argc, WCHAR *argv[])
{
    NtVdmArgc = argc;
    NtVdmArgv = argv;

#ifdef STANDALONE

    if (argc < 2)
    {
        wprintf(L"\nReactOS Virtual DOS Machine\n\n"
                L"Usage: NTVDM <executable> [<parameters>]\n");
        return 0;
    }

#endif

#ifdef ADVANCED_DEBUGGING
    {
    INT i = 20;

    printf("Waiting for debugger (10 secs)..");
    while (i--)
    {
        printf(".");
        if (IsDebuggerPresent())
        {
            DbgBreakPoint();
            break;
        }
        Sleep(500);
    }
    printf("Continue\n");
    }
#endif

    /* Load the global VDM settings */
    LoadGlobalSettings(&GlobalSettings);

    DPRINT1("\n\n\nNTVDM - Starting...\n\n\n");

    /* Initialize the console */
    if (!ConsoleInit())
    {
        wprintf(L"FATAL: A problem occurred when trying to initialize the console\n");
        goto Cleanup;
    }

    /* Initialize the emulator */
    if (!EmulatorInitialize(ConsoleInput, ConsoleOutput))
    {
        wprintf(L"FATAL: Failed to initialize the emulator\n");
        goto Cleanup;
    }

    /* Initialize the system BIOS and option ROMs */
    if (!BiosInitialize(GlobalSettings.BiosFileName.Buffer,
                        GlobalSettings.RomFiles.Buffer))
    {
        wprintf(L"FATAL: Failed to initialize the VDM BIOS.\n");
        goto Cleanup;
    }

    /* Let's go! Start simulation */
    CpuSimulate();

    /* Quit the VDM */
Cleanup:
    VdmShutdown(TRUE);
    return 0;
}

/* EOF */
