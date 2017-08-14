/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/usetup.c
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include <usetup.h>

#include "bootsup.h"
#include "chkdsk.h"
#include "cmdcons.h"
#include "format.h"
#include "settings.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS & LOCALS *********************************************************/

HANDLE ProcessHeap;

BOOLEAN IsUnattendedSetup = FALSE;
static USETUP_DATA USetupData;

/*
 * NOTE: Technically only used for the COPYCONTEXT InstallPath member
 * for the filequeue functionality.
 */
static UNICODE_STRING InstallPath;

// FIXME: Is it really useful?? Just used for SetDefaultPagefile...
static WCHAR DestinationDriveLetter;


/* OTHER Stuff *****/

PWCHAR SelectedLanguageId;
static WCHAR DefaultLanguage[20];   // Copy of string inside LanguageList
static WCHAR DefaultKBLayout[20];   // Copy of string inside KeyboardList

static BOOLEAN RepairUpdateFlag = FALSE;

static HANDLE hPnpThread = NULL;

static PPARTLIST PartitionList = NULL;
static PPARTENTRY TempPartition = NULL;
static PFILE_SYSTEM_LIST FileSystemList = NULL;
static FORMATMACHINESTATE FormatState = Start;

/*****************************************************/

static HINF SetupInf;

static HSPFILEQ SetupFileQueue = NULL;

static PNTOS_INSTALLATION CurrentInstallation = NULL;
static PGENERIC_LIST NtOsInstallsList = NULL;

static PGENERIC_LIST ComputerList = NULL;
static PGENERIC_LIST DisplayList  = NULL;
static PGENERIC_LIST KeyboardList = NULL;
static PGENERIC_LIST LayoutList   = NULL;
static PGENERIC_LIST LanguageList = NULL;


/* FUNCTIONS ****************************************************************/

static VOID
PrintString(char* fmt,...)
{
    char buffer[512];
    va_list ap;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    RtlInitAnsiString(&AnsiString, buffer);
    RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
    NtDisplayString(&UnicodeString);
    RtlFreeUnicodeString(&UnicodeString);
}


static VOID
DrawBox(IN SHORT xLeft,
        IN SHORT yTop,
        IN SHORT Width,
        IN SHORT Height)
{
    COORD coPos;
    DWORD Written;

    /* Draw upper left corner */
    coPos.X = xLeft;
    coPos.Y = yTop;
    FillConsoleOutputCharacterA(StdOutput,
                                0xDA, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw upper edge */
    coPos.X = xLeft + 1;
    coPos.Y = yTop;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC4, // '-',
                                Width - 2,
                                coPos,
                                &Written);

    /* Draw upper right corner */
    coPos.X = xLeft + Width - 1;
    coPos.Y = yTop;
    FillConsoleOutputCharacterA(StdOutput,
                                0xBF, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw right edge, inner space and left edge */
    for (coPos.Y = yTop + 1; coPos.Y < yTop + Height - 1; coPos.Y++)
    {
        coPos.X = xLeft;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB3, // '|',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = xLeft + 1;
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    Width - 2,
                                    coPos,
                                    &Written);

        coPos.X = xLeft + Width - 1;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB3, // '|',
                                    1,
                                    coPos,
                                    &Written);
    }

    /* Draw lower left corner */
    coPos.X = xLeft;
    coPos.Y = yTop + Height - 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC0, // '+',
                                1,
                                coPos,
                                &Written);

    /* Draw lower edge */
    coPos.X = xLeft + 1;
    coPos.Y = yTop + Height - 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xC4, // '-',
                                Width - 2,
                                coPos,
                                &Written);

    /* Draw lower right corner */
    coPos.X = xLeft + Width - 1;
    coPos.Y = yTop + Height - 1;
    FillConsoleOutputCharacterA(StdOutput,
                                0xD9, // '+',
                                1,
                                coPos,
                                &Written);
}


VOID
PopupError(PCCH Text,
           PCCH Status,
           PINPUT_RECORD Ir,
           ULONG WaitEvent)
{
    SHORT yTop;
    SHORT xLeft;
    COORD coPos;
    DWORD Written;
    ULONG Length;
    ULONG MaxLength;
    ULONG Lines;
    PCHAR p;
    PCCH pnext;
    BOOLEAN LastLine;
    SHORT Width;
    SHORT Height;

    /* Count text lines and longest line */
    MaxLength = 0;
    Lines = 0;
    pnext = Text;

    while (TRUE)
    {
        p = strchr(pnext, '\n');

        if (p == NULL)
        {
            Length = strlen(pnext);
            LastLine = TRUE;
        }
        else
        {
            Length = (ULONG)(p - pnext);
            LastLine = FALSE;
        }

        Lines++;

        if (Length > MaxLength)
            MaxLength = Length;

        if (LastLine == TRUE)
            break;

        pnext = p + 1;
    }

    /* Check length of status line */
    if (Status != NULL)
    {
        Length = strlen(Status);

        if (Length > MaxLength)
            MaxLength = Length;
    }

    Width = MaxLength + 4;
    Height = Lines + 2;

    if (Status != NULL)
        Height += 2;

    yTop = (yScreen - Height) / 2;
    xLeft = (xScreen - Width) / 2;


    /* Set screen attributes */
    coPos.X = xLeft;
    for (coPos.Y = yTop; coPos.Y < yTop + Height; coPos.Y++)
    {
        FillConsoleOutputAttribute(StdOutput,
                                   FOREGROUND_RED | BACKGROUND_WHITE,
                                   Width,
                                   coPos,
                                   &Written);
    }

    DrawBox(xLeft, yTop, Width, Height);

    /* Print message text */
    coPos.Y = yTop + 1;
    pnext = Text;
    while (TRUE)
    {
        p = strchr(pnext, '\n');

        if (p == NULL)
        {
            Length = strlen(pnext);
            LastLine = TRUE;
        }
        else
        {
            Length = (ULONG)(p - pnext);
            LastLine = FALSE;
        }

        if (Length != 0)
        {
            coPos.X = xLeft + 2;
            WriteConsoleOutputCharacterA(StdOutput,
                                         pnext,
                                         Length,
                                         coPos,
                                         &Written);
        }

        if (LastLine == TRUE)
            break;

        coPos.Y++;
        pnext = p + 1;
    }

    /* Print separator line and status text */
    if (Status != NULL)
    {
        coPos.Y = yTop + Height - 3;
        coPos.X = xLeft;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC3, // '+',
                                    1,
                                    coPos,
                                    &Written);

        coPos.X = xLeft + 1;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xC4, // '-',
                                    Width - 2,
                                    coPos,
                                    &Written);

        coPos.X = xLeft + Width - 1;
        FillConsoleOutputCharacterA(StdOutput,
                                    0xB4, // '+',
                                    1,
                                    coPos,
                                    &Written);

        coPos.Y++;
        coPos.X = xLeft + 2;
        WriteConsoleOutputCharacterA(StdOutput,
                                     Status,
                                     min(strlen(Status), (SIZE_T)Width - 4),
                                     coPos,
                                     &Written);
    }

    if (WaitEvent == POPUP_WAIT_NONE)
        return;

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if (WaitEvent == POPUP_WAIT_ANY_KEY ||
            Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)
        {
            return;
        }
    }
}


/*
 * Confirm quit setup
 * RETURNS
 *   TRUE: Quit setup.
 *   FALSE: Don't quit setup.
 */
static BOOL
ConfirmQuit(PINPUT_RECORD Ir)
{
    BOOL Result = FALSE;
    MUIDisplayError(ERROR_NOT_INSTALLED, NULL, POPUP_WAIT_NONE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            Result = TRUE;
            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)  /* ENTER */
        {
            Result = FALSE;
            break;
        }
    }

    return Result;
}


static VOID
UpdateKBLayout(VOID)
{
    PGENERIC_LIST_ENTRY ListEntry;
    LPCWSTR pszNewLayout;

    pszNewLayout = MUIDefaultKeyboardLayout();

    if (LayoutList == NULL)
    {
        LayoutList = CreateKeyboardLayoutList(SetupInf, DefaultKBLayout);
        if (LayoutList == NULL)
        {
            /* FIXME: Handle error! */
            return;
        }
    }

    ListEntry = GetFirstListEntry(LayoutList);

    /* Search for default layout (if provided) */
    if (pszNewLayout != NULL)
    {
        while (ListEntry != NULL)
        {
            if (!wcscmp(pszNewLayout, GetListEntryUserData(ListEntry)))
            {
                SetCurrentListEntry(LayoutList, ListEntry);
                break;
            }

            ListEntry = GetNextListEntry(ListEntry);
        }
    }
}


/*
 * Displays the LanguagePage.
 *
 * Next pages: WelcomePage, QuitPage
 *
 * SIDEEFFECTS
 *  Init SelectedLanguageId
 *  Init USetupData.LanguageId
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
LanguagePage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;
    PWCHAR NewLanguageId;
    BOOL RefreshPage = FALSE;

    /* Initialize the computer settings list */
    if (LanguageList == NULL)
    {
        LanguageList = CreateLanguageList(SetupInf, DefaultLanguage);
        if (LanguageList == NULL)
        {
           PopupError("Setup failed to initialize available translations", NULL, NULL, POPUP_WAIT_NONE);
           return WELCOME_PAGE;
        }
    }

    /* Load the font */
    USetupData.LanguageId = 0;
    SelectedLanguageId = DefaultLanguage;
    SetConsoleCodePage();
    UpdateKBLayout();

    /* If there's just a single language in the list skip
     * the language selection process altogether! */
    if (GenericListHasSingleEntry(LanguageList))
    {
        USetupData.LanguageId = (LANGID)(wcstol(SelectedLanguageId, NULL, 16) & 0xFFFF);
        return WELCOME_PAGE;
    }

    InitGenericListUi(&ListUi, LanguageList);
    DrawGenericList(&ListUi,
                    2,
                    18,
                    xScreen - 3,
                    yScreen - 3);

    ScrollToPositionGenericList(&ListUi, GetDefaultLanguageIndex());

    MUIDisplayPage(LANGUAGE_PAGE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            ScrollDownGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            ScrollUpGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_NEXT))  /* PAGE DOWN */
        {
            ScrollPageDownGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR))  /* PAGE UP */
        {
            ScrollPageUpGenericList(&ListUi);
            RefreshPage = TRUE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;
            else
                RedrawGenericList(&ListUi);
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)  /* ENTER */
        {
            SelectedLanguageId = (PWCHAR)GetListEntryUserData(GetCurrentListEntry(LanguageList));

            USetupData.LanguageId = (LANGID)(wcstol(SelectedLanguageId, NULL, 16) & 0xFFFF);

            if (wcscmp(SelectedLanguageId, DefaultLanguage))
            {
                UpdateKBLayout();
            }

            /* Load the font */
            SetConsoleCodePage();

            return WELCOME_PAGE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar > 0x60) && (Ir->Event.KeyEvent.uChar.AsciiChar < 0x7b))
        {
            /* a-z */
            GenericListKeyPress(&ListUi, Ir->Event.KeyEvent.uChar.AsciiChar);
            RefreshPage = TRUE;
        }

        if (RefreshPage)
        {
            NewLanguageId = (PWCHAR)GetListEntryUserData(GetCurrentListEntry(LanguageList));

            if (SelectedLanguageId != NewLanguageId)
            {
                /* Clear the language page */
                MUIClearPage(LANGUAGE_PAGE);

                SelectedLanguageId = NewLanguageId;

                /* Load the font */
                SetConsoleCodePage();

                /* Redraw language selection page in native language */
                MUIDisplayPage(LANGUAGE_PAGE);
            }

            RefreshPage = FALSE;
        }
    }

    return WELCOME_PAGE;
}


/*
 * Start page
 *
 * Next pages:
 *  LanguagePage (at once, default)
 *  InstallIntroPage (at once, if unattended)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Init Sdi
 *  Init USetupData.SourcePath
 *  Init USetupData.SourceRootPath
 *  Init USetupData.SourceRootDir
 *  Init SetupInf
 *  Init USetupData.RequiredPartitionDiskSpace
 *  Init IsUnattendedSetup
 *  If unattended, init *List and sets the Codepage
 *  If unattended, init SelectedLanguageId
 *  If unattended, init USetupData.LanguageId
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
SetupStartPage(PINPUT_RECORD Ir)
{
    NTSTATUS Status;
    ULONG Error;
    PGENERIC_LIST_ENTRY ListEntry;

    CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    /* Get the source path and source root path */
    Status = GetSourcePaths(&USetupData.SourcePath,
                            &USetupData.SourceRootPath,
                            &USetupData.SourceRootDir);
    if (!NT_SUCCESS(Status))
    {
        CONSOLE_PrintTextXY(6, 15, "GetSourcePaths() failed (Status 0x%08lx)", Status);
        MUIDisplayError(ERROR_NO_SOURCE_DRIVE, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }
    DPRINT1("SourcePath: '%wZ'\n", &USetupData.SourcePath);
    DPRINT1("SourceRootPath: '%wZ'\n", &USetupData.SourceRootPath);
    DPRINT1("SourceRootDir: '%wZ'\n", &USetupData.SourceRootDir);

    /* Load 'txtsetup.sif' from the installation media */
    Error = LoadSetupInf(&SetupInf, &USetupData);
    if (Error != ERROR_SUCCESS)
    {
        MUIDisplayError(Error, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    /* Start the PnP thread */
    if (hPnpThread != NULL)
    {
        NtResumeThread(hPnpThread, NULL);
        hPnpThread = NULL;
    }

    CheckUnattendedSetup(&USetupData);

    if (IsUnattendedSetup)
    {
        // TODO: Read options from inf
        ComputerList = CreateComputerTypeList(SetupInf);
        DisplayList = CreateDisplayDriverList(SetupInf);
        KeyboardList = CreateKeyboardDriverList(SetupInf);
        LayoutList = CreateKeyboardLayoutList(SetupInf, DefaultKBLayout);
        LanguageList = CreateLanguageList(SetupInf, DefaultLanguage);

        /* new part */
        wcscpy(SelectedLanguageId, USetupData.LocaleID);
        USetupData.LanguageId = (LANGID)(wcstol(SelectedLanguageId, NULL, 16) & 0xFFFF);

        /* first we hack LanguageList */
        ListEntry = GetFirstListEntry(LanguageList);
        while (ListEntry != NULL)
        {
            if (!wcsicmp(USetupData.LocaleID, GetListEntryUserData(ListEntry)))
            {
                DPRINT("found %S in LanguageList\n",GetListEntryUserData(ListEntry));
                SetCurrentListEntry(LanguageList, ListEntry);
                break;
            }

            ListEntry = GetNextListEntry(ListEntry);
        }

        /* now LayoutList */
        ListEntry = GetFirstListEntry(LayoutList);
        while (ListEntry != NULL)
        {
            if (!wcsicmp(USetupData.LocaleID, GetListEntryUserData(ListEntry)))
            {
                DPRINT("found %S in LayoutList\n",GetListEntryUserData(ListEntry));
                SetCurrentListEntry(LayoutList, ListEntry);
                break;
            }

            ListEntry = GetNextListEntry(ListEntry);
        }

        SetConsoleCodePage();

        return INSTALL_INTRO_PAGE;
    }

    return LANGUAGE_PAGE;
}


/*
 * Displays the WelcomePage.
 *
 * Next pages:
 *  InstallIntroPage (default)
 *  RecoveryPage
 *  LicensePage
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
WelcomePage(PINPUT_RECORD Ir)
{
    MUIDisplayPage(WELCOME_PAGE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return INSTALL_INTRO_PAGE;
        }
        else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'R') /* R */
        {
            return RECOVERY_PAGE;
        }
        else if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'L') /* L */
        {
            return LICENSE_PAGE;
        }
    }

    return WELCOME_PAGE;
}


/*
 * Displays the License page.
 *
 * Next page:
 *  WelcomePage (default)
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
LicensePage(PINPUT_RECORD Ir)
{
    MUIDisplayPage(LICENSE_PAGE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)  /* ENTER */
        {
            return WELCOME_PAGE;
        }
    }

    return LICENSE_PAGE;
}


/*
 * Displays the UpgradeRepairPage.
 *
 * Next pages:
 *  RebootPage (default)
 *  InstallIntroPage
 *  RecoveryPage
 *  WelcomePage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
UpgradeRepairPage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;

/*** HACK!! ***/
    if (PartitionList == NULL)
    {
        PartitionList = CreatePartitionList();
        if (PartitionList == NULL)
        {
            /* FIXME: show an error dialog */
            MUIDisplayError(ERROR_DRIVE_INFORMATION, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }
        else if (IsListEmpty(&PartitionList->DiskListHead))
        {
            MUIDisplayError(ERROR_NO_HDD, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }

        TempPartition = NULL;
        FormatState = Start;
    }
/**************/

    NtOsInstallsList = CreateNTOSInstallationsList(PartitionList);
    if (!NtOsInstallsList)
        DPRINT1("Failed to get a list of NTOS installations; continue installation...\n");
    if (!NtOsInstallsList || GetNumberOfListEntries(NtOsInstallsList) == 0)
    {
        RepairUpdateFlag = FALSE;

        // return INSTALL_INTRO_PAGE;
        return DEVICE_SETTINGS_PAGE;
        // return SCSI_CONTROLLER_PAGE;
    }

    MUIDisplayPage(UPGRADE_REPAIR_PAGE);

    InitGenericListUi(&ListUi, NtOsInstallsList);
    DrawGenericList(&ListUi,
                    2, 23,
                    xScreen - 3,
                    yScreen - 3);

    SaveGenericListState(NtOsInstallsList);

    // return HandleGenericList(&ListUi, DEVICE_SETTINGS_PAGE, Ir);
    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x00)
        {
            switch (Ir->Event.KeyEvent.wVirtualKeyCode)
            {
            case VK_DOWN:   /* DOWN */
                ScrollDownGenericList(&ListUi);
                break;
            case VK_UP:     /* UP */
                ScrollUpGenericList(&ListUi);
                break;
            case VK_NEXT:   /* PAGE DOWN */
                ScrollPageDownGenericList(&ListUi);
                break;
            case VK_PRIOR:  /* PAGE UP */
                ScrollPageUpGenericList(&ListUi);
                break;
            case VK_F3:     /* F3 */
            {
                if (ConfirmQuit(Ir) == TRUE)
                    return QUIT_PAGE;
                else
                    RedrawGenericList(&ListUi);
                break;
            }
            case VK_ESCAPE: /* ESC */
            {
                RestoreGenericListState(NtOsInstallsList);
                // return nextPage;    // prevPage;

                // return INSTALL_INTRO_PAGE;
                return DEVICE_SETTINGS_PAGE;
                // return SCSI_CONTROLLER_PAGE;
            }
            }
        }
        else
        {
            // switch (toupper(Ir->Event.KeyEvent.uChar.AsciiChar))
            // if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
            if (toupper(Ir->Event.KeyEvent.uChar.AsciiChar) == 'U')  /* U */
            {
                /* Retrieve the current installation */
                CurrentInstallation = (PNTOS_INSTALLATION)GetListEntryUserData(GetCurrentListEntry(NtOsInstallsList));
                DPRINT1("Selected installation for repair: \"%S\" ; DiskNumber = %d , PartitionNumber = %d\n",
                        CurrentInstallation->InstallationName, CurrentInstallation->DiskNumber, CurrentInstallation->PartitionNumber);

                RepairUpdateFlag = TRUE;

                // return nextPage;
                /***/return INSTALL_INTRO_PAGE;/***/
            }
            else if ((Ir->Event.KeyEvent.uChar.AsciiChar > 0x60) &&
                     (Ir->Event.KeyEvent.uChar.AsciiChar < 0x7b))   /* a-z */
            {
                GenericListKeyPress(&ListUi, Ir->Event.KeyEvent.uChar.AsciiChar);
            }
        }
    }

    return UPGRADE_REPAIR_PAGE;
}


/*
 * Displays the InstallIntroPage.
 *
 * Next pages:
 *  DeviceSettingsPage  (At once if repair or update is selected)
 *  SelectPartitionPage (At once if unattended setup)
 *  DeviceSettingsPage  (default)
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
InstallIntroPage(PINPUT_RECORD Ir)
{
    if (RepairUpdateFlag)
    {
#if 1 /* Old code that looks good */

        // return SELECT_PARTITION_PAGE;
        return DEVICE_SETTINGS_PAGE;

#else /* Possible new code? */

        return DEVICE_SETTINGS_PAGE;
        // return SCSI_CONTROLLER_PAGE;

#endif
    }

    if (IsUnattendedSetup)
        return SELECT_PARTITION_PAGE;

    MUIDisplayPage(INSTALL_INTRO_PAGE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return UPGRADE_REPAIR_PAGE;
        }
    }

    return INSTALL_INTRO_PAGE;
}


#if 0
static PAGE_NUMBER
ScsiControllerPage(PINPUT_RECORD Ir)
{
    // MUIDisplayPage(SCSI_CONTROLLER_PAGE);

    CONSOLE_SetTextXY(6, 8, "Setup detected the following mass storage devices:");

    /* FIXME: print loaded mass storage driver descriptions */
#if 0
    CONSOLE_SetTextXY(8, 10, "TEST device");
#endif

    CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return DEVICE_SETTINGS_PAGE;
        }
    }

    return SCSI_CONTROLLER_PAGE;
}

static PAGE_NUMBER
OemDriverPage(PINPUT_RECORD Ir)
{
    // MUIDisplayPage(OEM_DRIVER_PAGE);

    CONSOLE_SetTextXY(6, 8, "This is the OEM driver page!");

    /* FIXME: Implement!! */

    CONSOLE_SetStatusText("   ENTER = Continue   F3 = Quit");

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)) /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return DEVICE_SETTINGS_PAGE;
        }
    }

    return OEM_DRIVER_PAGE;
}
#endif


/*
 * Displays the DeviceSettingsPage.
 *
 * Next pages:
 *  SelectPartitionPage (At once if repair or update is selected)
 *  ComputerSettingsPage
 *  DisplaySettingsPage
 *  KeyboardSettingsPage
 *  LayoutsettingsPage
 *  SelectPartitionPage
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Init ComputerList
 *  Init DisplayList
 *  Init KeyboardList
 *  Init LayoutList
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
DeviceSettingsPage(PINPUT_RECORD Ir)
{
    static ULONG Line = 16;

    /* Initialize the computer settings list */
    if (ComputerList == NULL)
    {
        ComputerList = CreateComputerTypeList(SetupInf);
        if (ComputerList == NULL)
        {
            MUIDisplayError(ERROR_LOAD_COMPUTER, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }
    }

    /* Initialize the display settings list */
    if (DisplayList == NULL)
    {
        DisplayList = CreateDisplayDriverList(SetupInf);
        if (DisplayList == NULL)
        {
            MUIDisplayError(ERROR_LOAD_DISPLAY, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }
    }

    /* Initialize the keyboard settings list */
    if (KeyboardList == NULL)
    {
        KeyboardList = CreateKeyboardDriverList(SetupInf);
        if (KeyboardList == NULL)
        {
            MUIDisplayError(ERROR_LOAD_KEYBOARD, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }
    }

    /* Initialize the keyboard layout list */
    if (LayoutList == NULL)
    {
        LayoutList = CreateKeyboardLayoutList(SetupInf, DefaultKBLayout);
        if (LayoutList == NULL)
        {
            /* FIXME: report error */
            MUIDisplayError(ERROR_LOAD_KBLAYOUT, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }
    }

    if (RepairUpdateFlag)
        return SELECT_PARTITION_PAGE;

    // if (IsUnattendedSetup)
        // return SELECT_PARTITION_PAGE;

    MUIDisplayPage(DEVICE_SETTINGS_PAGE);

    CONSOLE_SetTextXY(25, 11, GetListEntryText(GetCurrentListEntry(ComputerList)));
    CONSOLE_SetTextXY(25, 12, GetListEntryText(GetCurrentListEntry(DisplayList)));
    CONSOLE_SetTextXY(25, 13, GetListEntryText(GetCurrentListEntry(KeyboardList)));
    CONSOLE_SetTextXY(25, 14, GetListEntryText(GetCurrentListEntry(LayoutList)));

    CONSOLE_InvertTextXY(24, Line, 48, 1);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            CONSOLE_NormalTextXY(24, Line, 48, 1);

            if (Line == 14)
                Line = 16;
            else if (Line == 16)
                Line = 11;
            else
                Line++;

            CONSOLE_InvertTextXY(24, Line, 48, 1);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            CONSOLE_NormalTextXY(24, Line, 48, 1);

            if (Line == 11)
                Line = 16;
            else if (Line == 16)
                Line = 14;
            else
                Line--;

            CONSOLE_InvertTextXY(24, Line, 48, 1);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            if (Line == 11)
                return COMPUTER_SETTINGS_PAGE;
            else if (Line == 12)
                return DISPLAY_SETTINGS_PAGE;
            else if (Line == 13)
                return KEYBOARD_SETTINGS_PAGE;
            else if (Line == 14)
                return LAYOUT_SETTINGS_PAGE;
            else if (Line == 16)
                return SELECT_PARTITION_PAGE;
        }
    }

    return DEVICE_SETTINGS_PAGE;
}


/*
 * Handles generic selection lists.
 *
 * PARAMS
 * GenericList: The list to handle.
 * nextPage: The page it needs to jump to after this page.
 * Ir: The PINPUT_RECORD
 */
static PAGE_NUMBER
HandleGenericList(PGENERIC_LIST_UI ListUi,
                  PAGE_NUMBER nextPage,
                  PINPUT_RECORD Ir)
{
    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            ScrollDownGenericList(ListUi);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            ScrollUpGenericList(ListUi);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_NEXT))  /* PAGE DOWN */
        {
            ScrollPageDownGenericList(ListUi);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR))  /* PAGE UP */
        {
            ScrollPageUpGenericList(ListUi);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;
            else
                RedrawGenericList(ListUi);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))  /* ESC */
        {
            RestoreGenericListState(ListUi->List);
            return nextPage;    // Use some "prevPage;" instead?
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return nextPage;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar > 0x60) && (Ir->Event.KeyEvent.uChar.AsciiChar < 0x7b))
        {
            /* a-z */
            GenericListKeyPress(ListUi, Ir->Event.KeyEvent.uChar.AsciiChar);
        }
    }
}


/*
 * Displays the ComputerSettingsPage.
 *
 * Next pages:
 *  DeviceSettingsPage
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
ComputerSettingsPage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;
    MUIDisplayPage(COMPUTER_SETTINGS_PAGE);

    InitGenericListUi(&ListUi, ComputerList);
    DrawGenericList(&ListUi,
                    2,
                    18,
                    xScreen - 3,
                    yScreen - 3);

    SaveGenericListState(ComputerList);

    return HandleGenericList(&ListUi, DEVICE_SETTINGS_PAGE, Ir);
}


/*
 * Displays the DisplaySettingsPage.
 *
 * Next pages:
 *  DeviceSettingsPage
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
DisplaySettingsPage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;
    MUIDisplayPage(DISPLAY_SETTINGS_PAGE);

    InitGenericListUi(&ListUi, DisplayList);
    DrawGenericList(&ListUi,
                    2,
                    18,
                    xScreen - 3,
                    yScreen - 3);

    SaveGenericListState(DisplayList);

    return HandleGenericList(&ListUi, DEVICE_SETTINGS_PAGE, Ir);
}


/*
 * Displays the KeyboardSettingsPage.
 *
 * Next pages:
 *  DeviceSettingsPage
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
KeyboardSettingsPage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;
    MUIDisplayPage(KEYBOARD_SETTINGS_PAGE);

    InitGenericListUi(&ListUi, KeyboardList);
    DrawGenericList(&ListUi,
                    2,
                    18,
                    xScreen - 3,
                    yScreen - 3);

    SaveGenericListState(KeyboardList);

    return HandleGenericList(&ListUi, DEVICE_SETTINGS_PAGE, Ir);
}


/*
 * Displays the LayoutSettingsPage.
 *
 * Next pages:
 *  DeviceSettingsPage
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
LayoutSettingsPage(PINPUT_RECORD Ir)
{
    GENERIC_LIST_UI ListUi;
    MUIDisplayPage(LAYOUT_SETTINGS_PAGE);

    InitGenericListUi(&ListUi, LayoutList);
    DrawGenericList(&ListUi,
                    2,
                    18,
                    xScreen - 3,
                    yScreen - 3);

    SaveGenericListState(LayoutList);

    return HandleGenericList(&ListUi, DEVICE_SETTINGS_PAGE, Ir);
}


static BOOL
IsDiskSizeValid(PPARTENTRY PartEntry)
{
    ULONGLONG size;

    size = PartEntry->SectorCount.QuadPart * PartEntry->DiskEntry->BytesPerSector;
    size = (size + (512 * KB)) / MB;  /* in MBytes */

    if (size < USetupData.RequiredPartitionDiskSpace)
    {
        /* Partition is too small so ask for another one */
        DPRINT1("Partition is too small (size: %I64u MB), required disk space is %lu MB\n", size, USetupData.RequiredPartitionDiskSpace);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


/*
 * Displays the SelectPartitionPage.
 *
 * Next pages:
 *  SelectFileSystemPage (At once if unattended)
 *  SelectFileSystemPage (Default if free space is selected)
 *  CreatePrimaryPartitionPage
 *  CreateExtendedPartitionPage
 *  CreateLogicalPartitionPage
 *  ConfirmDeleteSystemPartitionPage (if the selected partition is the system partition, aka with the boot flag set)
 *  DeletePartitionPage
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Set InstallShortcut (only if not unattended + free space is selected)
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
SelectPartitionPage(PINPUT_RECORD Ir)
{
    PARTLIST_UI ListUi;
    ULONG Error;

    if (PartitionList == NULL)
    {
        PartitionList = CreatePartitionList();
        if (PartitionList == NULL)
        {
            /* FIXME: show an error dialog */
            MUIDisplayError(ERROR_DRIVE_INFORMATION, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }
        else if (IsListEmpty(&PartitionList->DiskListHead))
        {
            MUIDisplayError(ERROR_NO_HDD, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }

        TempPartition = NULL;
        FormatState = Start;
    }

    if (RepairUpdateFlag)
    {
        /* Determine the selected installation disk & partition */
        if (!SelectPartition(PartitionList,
                             CurrentInstallation->DiskNumber,
                             CurrentInstallation->PartitionNumber))
        {
            DPRINT1("Whaaaaaat?!?! ASSERT!\n");
            ASSERT(FALSE);
        }

        return SELECT_FILE_SYSTEM_PAGE;
    }

    MUIDisplayPage(SELECT_PARTITION_PAGE);

    InitPartitionListUi(&ListUi, PartitionList,
                        2,
                        23,
                        xScreen - 3,
                        yScreen - 3);
    DrawPartitionList(&ListUi);

    if (IsUnattendedSetup)
    {
        if (!SelectPartition(PartitionList,
                             USetupData.DestinationDiskNumber,
                             USetupData.DestinationPartitionNumber))
        {
            if (USetupData.AutoPartition)
            {
                if (PartitionList->CurrentPartition->LogicalPartition)
                {
                    CreateLogicalPartition(PartitionList,
                                           PartitionList->CurrentPartition->SectorCount.QuadPart,
                                           TRUE);
                }
                else
                {
                    CreatePrimaryPartition(PartitionList,
                                           PartitionList->CurrentPartition->SectorCount.QuadPart,
                                           TRUE);
                }

// FIXME?? Aren't we going to enter an infinite loop, if this test fails??
                if (!IsDiskSizeValid(PartitionList->CurrentPartition))
                {
                    MUIDisplayError(ERROR_INSUFFICIENT_PARTITION_SIZE, Ir, POPUP_WAIT_ANY_KEY,
                                    USetupData.RequiredPartitionDiskSpace);
                    return SELECT_PARTITION_PAGE; /* let the user select another partition */
                }

                return SELECT_FILE_SYSTEM_PAGE;
            }
        }
        else
        {
            DrawPartitionList(&ListUi);

// FIXME?? Aren't we going to enter an infinite loop, if this test fails??
            if (!IsDiskSizeValid(PartitionList->CurrentPartition))
            {
                MUIDisplayError(ERROR_INSUFFICIENT_PARTITION_SIZE, Ir, POPUP_WAIT_ANY_KEY,
                                USetupData.RequiredPartitionDiskSpace);
                return SELECT_PARTITION_PAGE; /* let the user select another partition */
            }

            return SELECT_FILE_SYSTEM_PAGE;
        }
    }

    while (TRUE)
    {
        /* Update status text */
        if (PartitionList->CurrentPartition == NULL)
        {
            CONSOLE_SetStatusText(MUIGetString(STRING_INSTALLCREATEPARTITION));
        }
        else if (PartitionList->CurrentPartition->LogicalPartition)
        {
            if (PartitionList->CurrentPartition->IsPartitioned)
            {
                CONSOLE_SetStatusText(MUIGetString(STRING_DELETEPARTITION));
            }
            else
            {
                CONSOLE_SetStatusText(MUIGetString(STRING_INSTALLCREATELOGICAL));
            }
        }
        else
        {
            if (PartitionList->CurrentPartition->IsPartitioned)
            {
                if (IsContainerPartition(PartitionList->CurrentPartition->PartitionType))
                {
                    CONSOLE_SetStatusText(MUIGetString(STRING_DELETEPARTITION));
                }
                else
                {
                    CONSOLE_SetStatusText(MUIGetString(STRING_INSTALLDELETEPARTITION));
                }
            }
            else
            {
                CONSOLE_SetStatusText(MUIGetString(STRING_INSTALLCREATEPARTITION));
            }
        }

        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
            {
                DestroyPartitionList(PartitionList);
                PartitionList = NULL;
                return QUIT_PAGE;
            }

            break;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            ScrollDownPartitionList(&ListUi);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            ScrollUpPartitionList(&ListUi);
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN)  /* ENTER */
        {
            if (IsContainerPartition(PartitionList->CurrentPartition->PartitionType))
                continue; // return SELECT_PARTITION_PAGE;

            if (PartitionList->CurrentPartition == NULL ||
                PartitionList->CurrentPartition->IsPartitioned == FALSE)
            {
                if (PartitionList->CurrentPartition->LogicalPartition)
                {
                    CreateLogicalPartition(PartitionList,
                                           0ULL,
                                           TRUE);
                }
                else
                {
                    CreatePrimaryPartition(PartitionList,
                                           0ULL,
                                           TRUE);
                }
            }

            if (!IsDiskSizeValid(PartitionList->CurrentPartition))
            {
                MUIDisplayError(ERROR_INSUFFICIENT_PARTITION_SIZE, Ir, POPUP_WAIT_ANY_KEY,
                                USetupData.RequiredPartitionDiskSpace);
                return SELECT_PARTITION_PAGE; /* let the user select another partition */
            }

            return SELECT_FILE_SYSTEM_PAGE;
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'P')  /* P */
        {
            if (PartitionList->CurrentPartition->LogicalPartition == FALSE)
            {
                Error = PrimaryPartitionCreationChecks(PartitionList);
                if (Error != NOT_AN_ERROR)
                {
                    MUIDisplayError(Error, Ir, POPUP_WAIT_ANY_KEY);
                    return SELECT_PARTITION_PAGE;
                }

                return CREATE_PRIMARY_PARTITION_PAGE;
            }
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'E')  /* E */
        {
            if (PartitionList->CurrentPartition->LogicalPartition == FALSE)
            {
                Error = ExtendedPartitionCreationChecks(PartitionList);
                if (Error != NOT_AN_ERROR)
                {
                    MUIDisplayError(Error, Ir, POPUP_WAIT_ANY_KEY);
                    return SELECT_PARTITION_PAGE;
                }

                return CREATE_EXTENDED_PARTITION_PAGE;
            }
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'L')  /* L */
        {
            if (PartitionList->CurrentPartition->LogicalPartition == TRUE)
            {
                Error = LogicalPartitionCreationChecks(PartitionList);
                if (Error != NOT_AN_ERROR)
                {
                    MUIDisplayError(Error, Ir, POPUP_WAIT_ANY_KEY);
                    return SELECT_PARTITION_PAGE;
                }

                return CREATE_LOGICAL_PARTITION_PAGE;
            }
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'D')  /* D */
        {
            if (PartitionList->CurrentPartition->IsPartitioned == FALSE)
            {
                MUIDisplayError(ERROR_DELETE_SPACE, Ir, POPUP_WAIT_ANY_KEY);
                return SELECT_PARTITION_PAGE;
            }

            if (PartitionList->CurrentPartition->BootIndicator ||
                PartitionList->CurrentPartition == PartitionList->SystemPartition)
            {
                return CONFIRM_DELETE_SYSTEM_PARTITION_PAGE;
            }

            return DELETE_PARTITION_PAGE;
        }
    }

    return SELECT_PARTITION_PAGE;
}


#define PARTITION_SIZE_INPUT_FIELD_LENGTH 6
/* Restriction for MaxSize: pow(10, PARTITION_SIZE_INPUT_FIELD_LENGTH)-1 */
#define PARTITION_MAXSIZE 999999

static VOID
ShowPartitionSizeInputBox(SHORT Left,
                          SHORT Top,
                          SHORT Right,
                          SHORT Bottom,
                          ULONG MaxSize,
                          PCHAR InputBuffer,
                          PBOOLEAN Quit,
                          PBOOLEAN Cancel)
{
    INPUT_RECORD Ir;
    COORD coPos;
    DWORD Written;
    CHAR Buffer[128];
    WCHAR PartitionSizeBuffer[100];
    ULONG Index;
    WCHAR ch;
    SHORT iLeft;
    SHORT iTop;

    if (Quit != NULL)
        *Quit = FALSE;

    if (Cancel != NULL)
        *Cancel = FALSE;

    DrawBox(Left, Top, Right - Left + 1, Bottom - Top + 1);

    /* Print message */
    coPos.X = Left + 2;
    coPos.Y = Top + 2;
    strcpy(Buffer, MUIGetString(STRING_PARTITIONSIZE));
    iLeft = coPos.X + strlen(Buffer) + 1;
    iTop = coPos.Y;

    WriteConsoleOutputCharacterA(StdOutput,
                                 Buffer,
                                 strlen(Buffer),
                                 coPos,
                                 &Written);

    sprintf(Buffer, MUIGetString(STRING_MAXSIZE), MaxSize);
    coPos.X = iLeft + PARTITION_SIZE_INPUT_FIELD_LENGTH + 1;
    coPos.Y = iTop;
    WriteConsoleOutputCharacterA(StdOutput,
                                 Buffer,
                                 strlen(Buffer),
                                 coPos,
                                 &Written);

    swprintf(PartitionSizeBuffer, L"%lu", MaxSize);
    Index = wcslen(PartitionSizeBuffer);
    CONSOLE_SetInputTextXY(iLeft,
                           iTop,
                           PARTITION_SIZE_INPUT_FIELD_LENGTH,
                           PartitionSizeBuffer);

    while (TRUE)
    {
        CONSOLE_ConInKey(&Ir);

        if ((Ir.Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir.Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (Quit != NULL)
                *Quit = TRUE;

            PartitionSizeBuffer[0] = UNICODE_NULL;
            break;
        }
        else if (Ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)    /* ENTER */
        {
            break;
        }
        else if (Ir.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)    /* ESCAPE */
        {
            if (Cancel != NULL)
                *Cancel = TRUE;

            PartitionSizeBuffer[0] = UNICODE_NULL;
            break;
        }
        else if ((Ir.Event.KeyEvent.wVirtualKeyCode == VK_BACK) &&  /* BACKSPACE */
                 (Index > 0))
        {
            Index--;
            PartitionSizeBuffer[Index] = UNICODE_NULL;

            CONSOLE_SetInputTextXY(iLeft,
                                   iTop,
                                   PARTITION_SIZE_INPUT_FIELD_LENGTH,
                                   PartitionSizeBuffer);
        }
        else if ((Ir.Event.KeyEvent.uChar.AsciiChar != 0x00) &&
                 (Index < PARTITION_SIZE_INPUT_FIELD_LENGTH))
        {
            ch = (WCHAR)Ir.Event.KeyEvent.uChar.AsciiChar;

            if ((ch >= L'0') && (ch <= L'9'))
            {
                PartitionSizeBuffer[Index] = ch;
                Index++;
                PartitionSizeBuffer[Index] = UNICODE_NULL;

                CONSOLE_SetInputTextXY(iLeft,
                                       iTop,
                                       PARTITION_SIZE_INPUT_FIELD_LENGTH,
                                       PartitionSizeBuffer);
            }
        }
    }

    /* Convert UNICODE --> ANSI the poor man's way */
    sprintf(InputBuffer, "%S", PartitionSizeBuffer);
}


/*
 * Displays the CreatePrimaryPartitionPage.
 *
 * Next pages:
 *  SelectPartitionPage
 *  SelectFileSystemPage (default)
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
CreatePrimaryPartitionPage(PINPUT_RECORD Ir)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    BOOLEAN Quit;
    BOOLEAN Cancel;
    CHAR InputBuffer[50];
    ULONG MaxSize;
    ULONGLONG PartSize;
    ULONGLONG DiskSize;
    ULONGLONG SectorCount;
    PCHAR Unit;

    if (PartitionList == NULL ||
        PartitionList->CurrentDisk == NULL ||
        PartitionList->CurrentPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    DiskEntry = PartitionList->CurrentDisk;
    PartEntry = PartitionList->CurrentPartition;

    CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_CHOOSENEWPARTITION));

    DiskSize = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = DiskSize / GB;
        Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    {
        DiskSize = DiskSize / MB;
        if (DiskSize == 0)
            DiskSize = 1;

        Unit = MUIGetString(STRING_MB);
    }

    if (DiskEntry->DriverName.Length > 0)
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDINFOPARTCREATE_1),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            &DiskEntry->DriverName,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }
    else
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDINFOPARTCREATE_2),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }

    CONSOLE_SetTextXY(6, 12, MUIGetString(STRING_HDDSIZE));

#if 0
    CONSOLE_PrintTextXY(8, 10, "Maximum size of the new partition is %I64u MB",
                        PartitionList->CurrentPartition->SectorCount * DiskEntry->BytesPerSector / MB);
#endif

    CONSOLE_SetStatusText(MUIGetString(STRING_CREATEPARTITION));

    PartEntry = PartitionList->CurrentPartition;
    while (TRUE)
    {
        MaxSize = (PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector) / MB;  /* in MBytes (rounded) */

        if (MaxSize > PARTITION_MAXSIZE)
            MaxSize = PARTITION_MAXSIZE;

        ShowPartitionSizeInputBox(12, 14, xScreen - 12, 17, /* left, top, right, bottom */
                                  MaxSize, InputBuffer, &Quit, &Cancel);

        if (Quit == TRUE)
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Cancel == TRUE)
        {
            return SELECT_PARTITION_PAGE;
        }
        else
        {
            PartSize = atoi(InputBuffer);

            if (PartSize < 1)
            {
                /* Too small */
                continue;
            }

            if (PartSize > MaxSize)
            {
                /* Too large */
                continue;
            }

            /* Convert to bytes */
            if (PartSize == MaxSize)
            {
                /* Use all of the unpartitioned disk space */
                SectorCount = PartEntry->SectorCount.QuadPart;
            }
            else
            {
                /* Calculate the sector count from the size in MB */
                SectorCount = PartSize * MB / DiskEntry->BytesPerSector;

                /* But never get larger than the unpartitioned disk space */
                if (SectorCount > PartEntry->SectorCount.QuadPart)
                    SectorCount = PartEntry->SectorCount.QuadPart;
            }

            DPRINT ("Partition size: %I64u bytes\n", PartSize);

            CreatePrimaryPartition(PartitionList,
                                   SectorCount,
                                   FALSE);

            return SELECT_PARTITION_PAGE;
        }
    }

    return CREATE_PRIMARY_PARTITION_PAGE;
}


/*
 * Displays the CreateExtendedPartitionPage.
 *
 * Next pages:
 *  SelectPartitionPage (default)
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
CreateExtendedPartitionPage(PINPUT_RECORD Ir)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    BOOLEAN Quit;
    BOOLEAN Cancel;
    CHAR InputBuffer[50];
    ULONG MaxSize;
    ULONGLONG PartSize;
    ULONGLONG DiskSize;
    ULONGLONG SectorCount;
    PCHAR Unit;

    if (PartitionList == NULL ||
        PartitionList->CurrentDisk == NULL ||
        PartitionList->CurrentPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    DiskEntry = PartitionList->CurrentDisk;
    PartEntry = PartitionList->CurrentPartition;

    CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_CHOOSE_NEW_EXTENDED_PARTITION));

    DiskSize = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = DiskSize / GB;
        Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    {
        DiskSize = DiskSize / MB;
        if (DiskSize == 0)
            DiskSize = 1;

        Unit = MUIGetString(STRING_MB);
    }

    if (DiskEntry->DriverName.Length > 0)
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDINFOPARTCREATE_1),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            &DiskEntry->DriverName,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }
    else
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDINFOPARTCREATE_2),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }

    CONSOLE_SetTextXY(6, 12, MUIGetString(STRING_HDDSIZE));

#if 0
    CONSOLE_PrintTextXY(8, 10, "Maximum size of the new partition is %I64u MB",
                        PartitionList->CurrentPartition->SectorCount * DiskEntry->BytesPerSector / MB);
#endif

    CONSOLE_SetStatusText(MUIGetString(STRING_CREATEPARTITION));

    PartEntry = PartitionList->CurrentPartition;
    while (TRUE)
    {
        MaxSize = (PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector) / MB;  /* in MBytes (rounded) */

        if (MaxSize > PARTITION_MAXSIZE)
            MaxSize = PARTITION_MAXSIZE;

        ShowPartitionSizeInputBox(12, 14, xScreen - 12, 17, /* left, top, right, bottom */
                                  MaxSize, InputBuffer, &Quit, &Cancel);

        if (Quit == TRUE)
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Cancel == TRUE)
        {
            return SELECT_PARTITION_PAGE;
        }
        else
        {
            PartSize = atoi(InputBuffer);

            if (PartSize < 1)
            {
                /* Too small */
                continue;
            }

            if (PartSize > MaxSize)
            {
                /* Too large */
                continue;
            }

            /* Convert to bytes */
            if (PartSize == MaxSize)
            {
                /* Use all of the unpartitioned disk space */
                SectorCount = PartEntry->SectorCount.QuadPart;
            }
            else
            {
                /* Calculate the sector count from the size in MB */
                SectorCount = PartSize * MB / DiskEntry->BytesPerSector;

                /* But never get larger than the unpartitioned disk space */
                if (SectorCount > PartEntry->SectorCount.QuadPart)
                    SectorCount = PartEntry->SectorCount.QuadPart;
            }

            DPRINT ("Partition size: %I64u bytes\n", PartSize);

            CreateExtendedPartition(PartitionList,
                                    SectorCount);

            return SELECT_PARTITION_PAGE;
        }
    }

    return CREATE_EXTENDED_PARTITION_PAGE;
}


/*
 * Displays the CreateLogicalPartitionPage.
 *
 * Next pages:
 *  SelectFileSystemPage (default)
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
CreateLogicalPartitionPage(PINPUT_RECORD Ir)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    BOOLEAN Quit;
    BOOLEAN Cancel;
    CHAR InputBuffer[50];
    ULONG MaxSize;
    ULONGLONG PartSize;
    ULONGLONG DiskSize;
    ULONGLONG SectorCount;
    PCHAR Unit;

    if (PartitionList == NULL ||
        PartitionList->CurrentDisk == NULL ||
        PartitionList->CurrentPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    DiskEntry = PartitionList->CurrentDisk;
    PartEntry = PartitionList->CurrentPartition;

    CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_CHOOSE_NEW_LOGICAL_PARTITION));

    DiskSize = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = DiskSize / GB;
        Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    {
        DiskSize = DiskSize / MB;
        if (DiskSize == 0)
            DiskSize = 1;

        Unit = MUIGetString(STRING_MB);
    }

    if (DiskEntry->DriverName.Length > 0)
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDINFOPARTCREATE_1),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            &DiskEntry->DriverName,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }
    else
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDINFOPARTCREATE_2),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }

    CONSOLE_SetTextXY(6, 12, MUIGetString(STRING_HDDSIZE));

#if 0
    CONSOLE_PrintTextXY(8, 10, "Maximum size of the new partition is %I64u MB",
                        PartitionList->CurrentPartition->SectorCount * DiskEntry->BytesPerSector / MB);
#endif

    CONSOLE_SetStatusText(MUIGetString(STRING_CREATEPARTITION));

    PartEntry = PartitionList->CurrentPartition;
    while (TRUE)
    {
        MaxSize = (PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector) / MB;  /* in MBytes (rounded) */

        if (MaxSize > PARTITION_MAXSIZE)
            MaxSize = PARTITION_MAXSIZE;

        ShowPartitionSizeInputBox(12, 14, xScreen - 12, 17, /* left, top, right, bottom */
                                  MaxSize, InputBuffer, &Quit, &Cancel);

        if (Quit == TRUE)
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Cancel == TRUE)
        {
            return SELECT_PARTITION_PAGE;
        }
        else
        {
            PartSize = atoi(InputBuffer);

            if (PartSize < 1)
            {
                /* Too small */
                continue;
            }

            if (PartSize > MaxSize)
            {
                /* Too large */
                continue;
            }

            /* Convert to bytes */
            if (PartSize == MaxSize)
            {
                /* Use all of the unpartitioned disk space */
                SectorCount = PartEntry->SectorCount.QuadPart;
            }
            else
            {
                /* Calculate the sector count from the size in MB */
                SectorCount = PartSize * MB / DiskEntry->BytesPerSector;

                /* But never get larger than the unpartitioned disk space */
                if (SectorCount > PartEntry->SectorCount.QuadPart)
                    SectorCount = PartEntry->SectorCount.QuadPart;
            }

            DPRINT("Partition size: %I64u bytes\n", PartSize);

            CreateLogicalPartition(PartitionList,
                                   SectorCount,
                                   FALSE);

            return SELECT_PARTITION_PAGE;
        }
    }

    return CREATE_LOGICAL_PARTITION_PAGE;
}


/*
 * Displays the ConfirmDeleteSystemPartitionPage.
 *
 * Next pages:
 *  DeletePartitionPage (default)
 *  SelectPartitionPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
ConfirmDeleteSystemPartitionPage(PINPUT_RECORD Ir)
{
    MUIDisplayPage(CONFIRM_DELETE_SYSTEM_PARTITION_PAGE);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) /* ENTER */
        {
            return DELETE_PARTITION_PAGE;
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)  /* ESC */
        {
            return SELECT_PARTITION_PAGE;
        }
    }

    return CONFIRM_DELETE_SYSTEM_PARTITION_PAGE;
}


/*
 * Displays the DeletePartitionPage.
 *
 * Next pages:
 *  SelectPartitionPage (default)
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
DeletePartitionPage(PINPUT_RECORD Ir)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    ULONGLONG DiskSize;
    ULONGLONG PartSize;
    PCHAR Unit;
    CHAR PartTypeString[32];

    if (PartitionList == NULL ||
        PartitionList->CurrentDisk == NULL ||
        PartitionList->CurrentPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    DiskEntry = PartitionList->CurrentDisk;
    PartEntry = PartitionList->CurrentPartition;

    MUIDisplayPage(DELETE_PARTITION_PAGE);

    GetPartTypeStringFromPartitionType(PartEntry->PartitionType,
                                       PartTypeString,
                                       ARRAYSIZE(PartTypeString));

    PartSize = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
    if (PartSize >= 10 * GB) /* 10 GB */
    {
        PartSize = PartSize / GB;
        Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    if (PartSize >= 10 * MB) /* 10 MB */
    {
        PartSize = PartSize / MB;
        Unit = MUIGetString(STRING_MB);
    }
    else
    {
        PartSize = PartSize / KB;
        Unit = MUIGetString(STRING_KB);
    }

    if (*PartTypeString == '\0') // STRING_FORMATUNKNOWN ??
    {
        CONSOLE_PrintTextXY(6, 10,
                            MUIGetString(STRING_HDDINFOUNK2),
                            (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
                            (PartEntry->DriveLetter == 0) ? '-' : ':',
                            PartEntry->PartitionType,
                            PartSize,
                            Unit);
    }
    else
    {
        CONSOLE_PrintTextXY(6, 10,
                            "   %c%c  %s    %I64u %s",
                            (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
                            (PartEntry->DriveLetter == 0) ? '-' : ':',
                            PartTypeString,
                            PartSize,
                            Unit);
    }

    DiskSize = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
#if 0
    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = DiskSize / GB;
        Unit = MUIGetString(STRING_GB);
    }
    else
#endif
    {
        DiskSize = DiskSize / MB;
        if (DiskSize == 0)
            DiskSize = 1;

        Unit = MUIGetString(STRING_MB);
    }

    if (DiskEntry->DriverName.Length > 0)
    {
        CONSOLE_PrintTextXY(6, 12,
                            MUIGetString(STRING_HDINFOPARTDELETE_1),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            &DiskEntry->DriverName,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }
    else
    {
        CONSOLE_PrintTextXY(6, 12,
                            MUIGetString(STRING_HDINFOPARTDELETE_2),
                            DiskSize,
                            Unit,
                            DiskEntry->DiskNumber,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)  /* ESC */
        {
            return SELECT_PARTITION_PAGE;
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == 'D') /* D */
        {
            DeleteCurrentPartition(PartitionList);

            return SELECT_PARTITION_PAGE;
        }
    }

    return DELETE_PARTITION_PAGE;
}


/*
 * Displays the SelectFileSystemPage.
 *
 * Next pages:
 *  CheckFileSystemPage (At once if RepairUpdate is selected)
 *  CheckFileSystemPage (At once if Unattended and not USetupData.FormatPartition)
 *  FormatPartitionPage (At once if Unattended and USetupData.FormatPartition)
 *  SelectPartitionPage (If the user aborts)
 *  FormatPartitionPage (Default)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Sets PartEntry->DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].PartitionType (via UpdatePartitionType)
 *  Calls CheckActiveSystemPartition()
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
SelectFileSystemPage(PINPUT_RECORD Ir)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    ULONGLONG DiskSize;
    ULONGLONG PartSize;
    PCHAR DiskUnit;
    PCHAR PartUnit;
    CHAR PartTypeString[32];
    FORMATMACHINESTATE PreviousFormatState;

    DPRINT("SelectFileSystemPage()\n");

    if (PartitionList == NULL ||
        PartitionList->CurrentDisk == NULL ||
        PartitionList->CurrentPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    /* Find or set the active system partition */
    CheckActiveSystemPartition(PartitionList);
    if (PartitionList->SystemPartition == NULL)
    {
        /* FIXME: show an error dialog */
        //
        // Error dialog should say that we cannot find a suitable
        // system partition and create one on the system. At this point,
        // it may be nice to ask the user whether he wants to continue,
        // or use an external drive as the system drive/partition
        // (e.g. floppy, USB drive, etc...)
        //
        return QUIT_PAGE;
    }

    PreviousFormatState = FormatState;
    switch (FormatState)
    {
        case Start:
        {
            if (PartitionList->CurrentPartition != PartitionList->SystemPartition)
            {
                TempPartition = PartitionList->SystemPartition;
                TempPartition->NeedsCheck = TRUE;

                FormatState = FormatSystemPartition;
                DPRINT1("FormatState: Start --> FormatSystemPartition\n");
            }
            else
            {
                TempPartition = PartitionList->CurrentPartition;
                TempPartition->NeedsCheck = TRUE;

                FormatState = FormatInstallPartition;
                DPRINT1("FormatState: Start --> FormatInstallPartition\n");
            }
            break;
        }

        case FormatSystemPartition:
        {
            TempPartition = PartitionList->CurrentPartition;
            TempPartition->NeedsCheck = TRUE;

            FormatState = FormatInstallPartition;
            DPRINT1("FormatState: FormatSystemPartition --> FormatInstallPartition\n");
            break;
        }

        case FormatInstallPartition:
        {
            if (GetNextUnformattedPartition(PartitionList,
                                            NULL,
                                            &TempPartition))
            {
                FormatState = FormatOtherPartition;
                TempPartition->NeedsCheck = TRUE;
                DPRINT1("FormatState: FormatInstallPartition --> FormatOtherPartition\n");
            }
            else
            {
                FormatState = FormatDone;
                DPRINT1("FormatState: FormatInstallPartition --> FormatDone\n");
                return CHECK_FILE_SYSTEM_PAGE;
            }
            break;
        }

        case FormatOtherPartition:
        {
            if (GetNextUnformattedPartition(PartitionList,
                                            NULL,
                                            &TempPartition))
            {
                FormatState = FormatOtherPartition;
                TempPartition->NeedsCheck = TRUE;
                DPRINT1("FormatState: FormatOtherPartition --> FormatOtherPartition\n");
            }
            else
            {
                FormatState = FormatDone;
                DPRINT1("FormatState: FormatOtherPartition --> FormatDone\n");
                return CHECK_FILE_SYSTEM_PAGE;
            }
            break;
        }

        default:
        {
            DPRINT1("FormatState: Invalid value %ld\n", FormatState);
            /* FIXME: show an error dialog */
            return QUIT_PAGE;
        }
    }

    PartEntry = TempPartition;
    DiskEntry = PartEntry->DiskEntry;

    /* Adjust disk size */
    DiskSize = DiskEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
    if (DiskSize >= 10 * GB) /* 10 GB */
    {
        DiskSize = DiskSize / GB;
        DiskUnit = MUIGetString(STRING_GB);
    }
    else
    {
        DiskSize = DiskSize / MB;
        DiskUnit = MUIGetString(STRING_MB);
    }

    /* Adjust partition size */
    PartSize = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
    if (PartSize >= 10 * GB) /* 10 GB */
    {
        PartSize = PartSize / GB;
        PartUnit = MUIGetString(STRING_GB);
    }
    else
    {
        PartSize = PartSize / MB;
        PartUnit = MUIGetString(STRING_MB);
    }

    /* Adjust partition type */
    GetPartTypeStringFromPartitionType(PartEntry->PartitionType,
                                       PartTypeString,
                                       ARRAYSIZE(PartTypeString));

    if (PartEntry->AutoCreate == TRUE)
    {
        CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_NEWPARTITION));

#if 0
        CONSOLE_PrintTextXY(8, 10, "Partition %lu (%I64u %s) %s of",
                            PartEntry->PartitionNumber,
                            PartSize,
                            PartUnit,
                            PartTypeString);
#endif

        CONSOLE_PrintTextXY(8, 10, MUIGetString(STRING_HDINFOPARTZEROED_1),
                            DiskEntry->DiskNumber,
                            DiskSize,
                            DiskUnit,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            &DiskEntry->DriverName,
                            DiskEntry->NoMbr ? "GPT" : "MBR");

        CONSOLE_SetTextXY(6, 12, MUIGetString(STRING_PARTFORMAT));

        PartEntry->AutoCreate = FALSE;
    }
    else if (PartEntry->New == TRUE)
    {
        switch (FormatState)
        {
            case FormatSystemPartition:
                CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_NONFORMATTEDSYSTEMPART));
                break;

            case FormatInstallPartition:
                CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_NONFORMATTEDPART));
                break;

            case FormatOtherPartition:
                CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_NONFORMATTEDOTHERPART));
                break;

            default:
                break;
        }

        CONSOLE_SetTextXY(6, 10, MUIGetString(STRING_PARTFORMAT));
    }
    else
    {
        CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_INSTALLONPART));

        if (*PartTypeString == '\0') // STRING_FORMATUNKNOWN ??
        {
            CONSOLE_PrintTextXY(8, 10,
                                MUIGetString(STRING_HDDINFOUNK4),
                                (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
                                (PartEntry->DriveLetter == 0) ? '-' : ':',
                                PartEntry->PartitionType,
                                PartSize,
                                PartUnit);
        }
        else
        {
            CONSOLE_PrintTextXY(8, 10,
                                "%c%c  %s    %I64u %s",
                                (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter,
                                (PartEntry->DriveLetter == 0) ? '-' : ':',
                                PartTypeString,
                                PartSize,
                                PartUnit);
        }

        CONSOLE_PrintTextXY(6, 12, MUIGetString(STRING_HDINFOPARTEXISTS_1),
                            DiskEntry->DiskNumber,
                            DiskSize,
                            DiskUnit,
                            DiskEntry->Port,
                            DiskEntry->Bus,
                            DiskEntry->Id,
                            &DiskEntry->DriverName,
                            DiskEntry->NoMbr ? "GPT" : "MBR");
    }

    MUIDisplayPage(SELECT_FILE_SYSTEM_PAGE);

    if (FileSystemList == NULL)
    {
        /* Create the file system list, and by default select the "FAT" file system */
        FileSystemList = CreateFileSystemList(6, 26, PartEntry->New, L"FAT");
        if (FileSystemList == NULL)
        {
            /* FIXME: show an error dialog */
            return QUIT_PAGE;
        }
    }

    if (RepairUpdateFlag)
    {
        return CHECK_FILE_SYSTEM_PAGE;
        //return SELECT_PARTITION_PAGE;
    }

    if (IsUnattendedSetup)
    {
        if (USetupData.FormatPartition)
        {
            /*
             * We use whatever currently selected file system we have
             * (by default, this is "FAT", as per the initialization
             * performed above). Note that it may be interesting to specify
             * which file system to use in unattended installations, in the
             * txtsetup.sif file.
             */
            return FORMAT_PARTITION_PAGE;
        }

        return CHECK_FILE_SYSTEM_PAGE;
    }

    DrawFileSystemList(FileSystemList);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))  /* ESC */
        {
            FormatState = Start;
            return SELECT_PARTITION_PAGE;
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            ScrollDownFileSystemList(FileSystemList);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            ScrollUpFileSystemList(FileSystemList);
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) /* ENTER */
        {
            if (!FileSystemList->Selected->FileSystem)
                return SELECT_FILE_SYSTEM_PAGE;
            else
                return FORMAT_PARTITION_PAGE;
        }
    }

    FormatState = PreviousFormatState;

    return SELECT_FILE_SYSTEM_PAGE;
}


/*
 * Displays the FormatPartitionPage.
 *
 * Next pages:
 *  InstallDirectoryPage (At once if IsUnattendedSetup or InstallShortcut)
 *  SelectPartitionPage  (At once)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Sets PartitionList->CurrentPartition->FormatState
 *  Sets USetupData.DestinationRootPath
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
FormatPartitionPage(PINPUT_RECORD Ir)
{
    UNICODE_STRING PartitionRootPath;
    WCHAR PathBuffer[MAX_PATH];
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PFILE_SYSTEM_ITEM SelectedFileSystem;
    NTSTATUS Status;

#ifndef NDEBUG
    ULONG Line;
    ULONG i;
    PPARTITION_INFORMATION PartitionInfo;
#endif

    DPRINT("FormatPartitionPage()\n");

    MUIDisplayPage(FORMAT_PARTITION_PAGE);

    if (PartitionList == NULL || TempPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    PartEntry = TempPartition;
    DiskEntry = PartEntry->DiskEntry;

    SelectedFileSystem = FileSystemList->Selected;

    while (TRUE)
    {
        if (!IsUnattendedSetup)
        {
            CONSOLE_ConInKey(Ir);
        }

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.wVirtualKeyCode == VK_RETURN || IsUnattendedSetup) /* ENTER */
        {
            CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

            if (!PreparePartitionForFormatting(PartEntry, SelectedFileSystem->FileSystem))
            {
                /* FIXME: show an error dialog */
                return QUIT_PAGE;
            }

#ifndef NDEBUG
            CONSOLE_PrintTextXY(6, 12,
                                "Cylinders: %I64u  Tracks/Cyl: %lu  Sectors/Trk: %lu  Bytes/Sec: %lu  %c",
                                DiskEntry->Cylinders,
                                DiskEntry->TracksPerCylinder,
                                DiskEntry->SectorsPerTrack,
                                DiskEntry->BytesPerSector,
                                DiskEntry->Dirty ? '*' : ' ');

            Line = 13;

            for (i = 0; i < DiskEntry->LayoutBuffer->PartitionCount; i++)
            {
                PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[i];

                CONSOLE_PrintTextXY(6, Line,
                                    "%2u:  %2lu  %c  %12I64u  %12I64u  %02x",
                                    i,
                                    PartitionInfo->PartitionNumber,
                                    PartitionInfo->BootIndicator ? 'A' : '-',
                                    PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector,
                                    PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector,
                                    PartitionInfo->PartitionType);
                Line++;
            }
#endif

            /* Commit the partition changes to the disk */
            if (!WritePartitionsToDisk(PartitionList))
            {
                DPRINT("WritePartitionsToDisk() failed\n");
                MUIDisplayError(ERROR_WRITE_PTABLE, Ir, POPUP_WAIT_ENTER);
                return QUIT_PAGE;
            }

            /* Set PartitionRootPath */
            StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                    L"\\Device\\Harddisk%lu\\Partition%lu",
                    DiskEntry->DiskNumber,
                    PartEntry->PartitionNumber);
            RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
            DPRINT("PartitionRootPath: %wZ\n", &PartitionRootPath);

            /* Format the partition */
            if (SelectedFileSystem->FileSystem)
            {
                Status = FormatPartition(&PartitionRootPath,
                                         SelectedFileSystem);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("FormatPartition() failed with status 0x%08lx\n", Status);
                    MUIDisplayError(ERROR_FORMATTING_PARTITION, Ir, POPUP_WAIT_ANY_KEY, PathBuffer);
                    return QUIT_PAGE;
                }

                PartEntry->FormatState = Formatted;
                // PartEntry->FileSystem  = FileSystem;
                PartEntry->New = FALSE;
            }

#ifndef NDEBUG
            CONSOLE_SetStatusText("   Done.  Press any key ...");
            CONSOLE_ConInKey(Ir);
#endif

            return SELECT_FILE_SYSTEM_PAGE;
        }
    }

    return FORMAT_PARTITION_PAGE;
}


/*
 * Displays the CheckFileSystemPage.
 *
 * Next pages:
 *  InstallDirectoryPage (At once)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Inits or reloads FileSystemList
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
CheckFileSystemPage(PINPUT_RECORD Ir)
{
    PFILE_SYSTEM CurrentFileSystem;
    UNICODE_STRING PartitionRootPath;
    WCHAR PathBuffer[MAX_PATH];
    CHAR Buffer[MAX_PATH];
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    NTSTATUS Status;

    if (PartitionList == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    if (!GetNextUncheckedPartition(PartitionList, &DiskEntry, &PartEntry))
    {
        return INSTALL_DIRECTORY_PAGE;
    }

    /* Set PartitionRootPath */
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
            L"\\Device\\Harddisk%lu\\Partition%lu",
            DiskEntry->DiskNumber,
            PartEntry->PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT("PartitionRootPath: %wZ\n", &PartitionRootPath);

    CONSOLE_SetTextXY(6, 8, MUIGetString(STRING_CHECKINGPART));

    CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    CurrentFileSystem = PartEntry->FileSystem;
    DPRINT1("CheckFileSystemPage -- PartitionType: 0x%02X ; FileSystemName: %S\n",
            PartEntry->PartitionType, (CurrentFileSystem ? CurrentFileSystem->FileSystemName : L"n/a"));

    /* HACK: Do not try to check a partition with an unknown filesystem */
    if (CurrentFileSystem == NULL)
    {
        PartEntry->NeedsCheck = FALSE;
        return CHECK_FILE_SYSTEM_PAGE;
    }

    if (CurrentFileSystem->ChkdskFunc == NULL)
    {
        sprintf(Buffer,
                "Setup is currently unable to check a partition formatted in %S.\n"
                "\n"
                "  \x07  Press ENTER to continue Setup.\n"
                "  \x07  Press F3 to quit Setup.",
                CurrentFileSystem->FileSystemName);

        PopupError(Buffer,
                   MUIGetString(STRING_QUITCONTINUE),
                   NULL, POPUP_WAIT_NONE);

        while (TRUE)
        {
            CONSOLE_ConInKey(Ir);

            if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x00 &&
                Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3)  /* F3 */
            {
                if (ConfirmQuit(Ir))
                    return QUIT_PAGE;
                else
                    return CHECK_FILE_SYSTEM_PAGE;
            }
            else if (Ir->Event.KeyEvent.uChar.AsciiChar == VK_RETURN) /* ENTER */
            {
                PartEntry->NeedsCheck = FALSE;
                return CHECK_FILE_SYSTEM_PAGE;
            }
        }
    }
    else
    {
        Status = ChkdskPartition(&PartitionRootPath, CurrentFileSystem);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ChkdskPartition() failed with status 0x%08lx\n", Status);
            // sprintf(Buffer, "Setup failed to verify the selected partition.\n"
            sprintf(Buffer, "ChkDsk detected some disk errors.\n"
                    "(Status 0x%08lx).\n", Status);
            PopupError(Buffer,
                       // MUIGetString(STRING_REBOOTCOMPUTER),
                       MUIGetString(STRING_CONTINUE),
                       Ir, POPUP_WAIT_ENTER);

            // return QUIT_PAGE;
        }

        PartEntry->NeedsCheck = FALSE;
        return CHECK_FILE_SYSTEM_PAGE;
    }
}


/*
 * Displays the InstallDirectoryPage1.
 *
 * Next pages:
 *  PrepareCopyPage (At once)
 *
 * SIDEEFFECTS
 *  Inits USetupData.DestinationRootPath
 *  Inits USetupData.DestinationPath
 *  Inits USetupData.DestinationArcPath
 *  Inits DestinationDriveLetter
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
InstallDirectoryPage1(PWSTR InstallDir,
                      PDISKENTRY DiskEntry,
                      PPARTENTRY PartEntry)
{
    WCHAR PathBuffer[MAX_PATH];

/** Equivalent of 'NTOS_INSTALLATION::PathComponent' **/
    /* Create 'InstallPath' string */
    RtlFreeUnicodeString(&InstallPath);
    RtlCreateUnicodeString(&InstallPath, InstallDir);

    /* Create 'USetupData.DestinationRootPath' string */
    RtlFreeUnicodeString(&USetupData.DestinationRootPath);
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
            L"\\Device\\Harddisk%lu\\Partition%lu\\",
            DiskEntry->DiskNumber,
            PartEntry->PartitionNumber);
    RtlCreateUnicodeString(&USetupData.DestinationRootPath, PathBuffer);
    DPRINT("DestinationRootPath: %wZ\n", &USetupData.DestinationRootPath);

/** Equivalent of 'NTOS_INSTALLATION::SystemNtPath' **/
    /* Create 'USetupData.DestinationPath' string */
    RtlFreeUnicodeString(&USetupData.DestinationPath);
    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                 USetupData.DestinationRootPath.Buffer, InstallDir);
    RtlCreateUnicodeString(&USetupData.DestinationPath, PathBuffer);

/** Equivalent of 'NTOS_INSTALLATION::SystemArcPath' **/
    /* Create 'USetupData.DestinationArcPath' */
    RtlFreeUnicodeString(&USetupData.DestinationArcPath);
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
            L"multi(0)disk(0)rdisk(%lu)partition(%lu)\\",
            DiskEntry->BiosDiskNumber,
            PartEntry->PartitionNumber);
    ConcatPaths(PathBuffer, ARRAYSIZE(PathBuffer), 1, InstallDir);
    RtlCreateUnicodeString(&USetupData.DestinationArcPath, PathBuffer);

    /* Initialize DestinationDriveLetter */
    DestinationDriveLetter = (WCHAR)PartEntry->DriveLetter;

    return PREPARE_COPY_PAGE;
}


/*
 * Displays the InstallDirectoryPage.
 *
 * Next pages:
 *  PrepareCopyPage (As the direct result of InstallDirectoryPage1)
 *  QuitPage
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
InstallDirectoryPage(PINPUT_RECORD Ir)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    WCHAR InstallDir[MAX_PATH];
    WCHAR c;
    ULONG Length;

    /* We do not need the filesystem list anymore */
    if (FileSystemList != NULL)
    {
        DestroyFileSystemList(FileSystemList);
        FileSystemList = NULL;
    }

    if (PartitionList == NULL ||
        PartitionList->CurrentDisk == NULL ||
        PartitionList->CurrentPartition == NULL)
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    DiskEntry = PartitionList->CurrentDisk;
    PartEntry = PartitionList->CurrentPartition;

    // if (IsUnattendedSetup)
    if (RepairUpdateFlag)
        wcscpy(InstallDir, CurrentInstallation->PathComponent); // SystemNtPath
    else if (USetupData.InstallationDirectory[0])
        wcscpy(InstallDir, USetupData.InstallationDirectory);
    else
        wcscpy(InstallDir, L"\\ReactOS");

    Length = wcslen(InstallDir);

    /*
     * Check the validity of the predefined 'InstallDir'. If we are either
     * in unattended setup or in update/repair mode, and the installation path
     * is valid, just perform the installation. Otherwise (either in the case
     * of an invalid path, or we are in regular setup), display the UI and allow
     * the user to specify a new installation path.
     */
    if ((RepairUpdateFlag || IsUnattendedSetup) &&
        IsValidPath(InstallDir, Length))
    {
        return InstallDirectoryPage1(InstallDir,
                                     DiskEntry,
                                     PartEntry);
    }

    MUIDisplayPage(INSTALL_DIRECTORY_PAGE);
    CONSOLE_SetInputTextXY(8, 11, 51, InstallDir);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            /*
             * Check for the validity of the installation directory and pop up
             * an error if it is not the case. Then the user can fix its input.
             */
            if (!IsValidPath(InstallDir, Length))
            {
                MUIDisplayError(ERROR_DIRECTORY_NAME, Ir, POPUP_WAIT_ENTER);
                return INSTALL_DIRECTORY_PAGE;
            }
            return InstallDirectoryPage1(InstallDir,
                                         DiskEntry,
                                         PartEntry);
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x08) /* BACKSPACE */
        {
            if (Length > 0)
            {
                Length--;
                InstallDir[Length] = 0;
                CONSOLE_SetInputTextXY(8, 11, 51, InstallDir);
            }
        }
        else if (isprint(Ir->Event.KeyEvent.uChar.AsciiChar))
        {
            if (Length < 50)
            {
                c = (WCHAR)Ir->Event.KeyEvent.uChar.AsciiChar;
                if (iswalpha(c) || iswdigit(c) || c == '.' || c == '\\' || c == '-' || c == '_')
                {
                    InstallDir[Length] = c;
                    Length++;
                    InstallDir[Length] = 0;
                    CONSOLE_SetInputTextXY(8, 11, 51, InstallDir);
                }
            }
        }
    }

    return INSTALL_DIRECTORY_PAGE;
}


static BOOLEAN
AddSectionToCopyQueueCab(HINF InfFile,
                         PWCHAR SectionName,
                         PWCHAR SourceCabinet,
                         PCUNICODE_STRING DestinationPath,
                         PINPUT_RECORD Ir)
{
    INFCONTEXT FilesContext;
    INFCONTEXT DirContext;
    PWCHAR FileKeyName;
    PWCHAR FileKeyValue;
    PWCHAR DirKeyValue;
    PWCHAR TargetFileName;

    /*
     * This code enumerates the list of files in reactos.dff / reactos.inf
     * that need to be extracted from reactos.cab and be installed in their
     * respective directories.
     */

    /* Search for the SectionName section */
    if (!SetupFindFirstLineW(InfFile, SectionName, NULL, &FilesContext))
    {
        CHAR Buffer[128];
        sprintf(Buffer, MUIGetString(STRING_TXTSETUPFAILED), SectionName);
        PopupError(Buffer, MUIGetString(STRING_REBOOTCOMPUTER), Ir, POPUP_WAIT_ENTER);
        return FALSE;
    }

    /*
     * Enumerate the files in the section and add them to the file queue.
     */
    do
    {
        /* Get source file name and target directory id */
        if (!INF_GetData(&FilesContext, &FileKeyName, &FileKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            break;
        }

        /* Get optional target file name */
        if (!INF_GetDataField(&FilesContext, 2, &TargetFileName))
            TargetFileName = NULL;

        DPRINT("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

        /* Lookup target directory */
        if (!SetupFindFirstLineW(InfFile, L"Directories", FileKeyValue, &DirContext))
        {
            /* FIXME: Handle error! */
            DPRINT1("SetupFindFirstLine() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(FileKeyValue);
            INF_FreeData(TargetFileName);
            break;
        }

        INF_FreeData(FileKeyValue);

        if (!INF_GetData(&DirContext, NULL, &DirKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(TargetFileName);
            break;
        }

        if (!SetupQueueCopy(SetupFileQueue,
                            SourceCabinet,
                            USetupData.SourceRootPath.Buffer,
                            USetupData.SourceRootDir.Buffer,
                            FileKeyName,
                            DirKeyValue,
                            TargetFileName))
        {
            /* FIXME: Handle error! */
            DPRINT1("SetupQueueCopy() failed\n");
        }

        INF_FreeData(FileKeyName);
        INF_FreeData(TargetFileName);
        INF_FreeData(DirKeyValue);
    } while (SetupFindNextLine(&FilesContext, &FilesContext));

    return TRUE;
}


static BOOLEAN
AddSectionToCopyQueue(HINF InfFile,
                      PWCHAR SectionName,
                      PWCHAR SourceCabinet,
                      PCUNICODE_STRING DestinationPath,
                      PINPUT_RECORD Ir)
{
    INFCONTEXT FilesContext;
    INFCONTEXT DirContext;
    PWCHAR FileKeyName;
    PWCHAR FileKeyValue;
    PWCHAR DirKeyValue;
    PWCHAR TargetFileName;
    WCHAR CompleteOrigDirName[512]; // FIXME: MAX_PATH is not enough?

    if (SourceCabinet)
        return AddSectionToCopyQueueCab(InfFile, L"SourceFiles", SourceCabinet, DestinationPath, Ir);

    /*
     * This code enumerates the list of files in txtsetup.sif
     * that need to be installed in their respective directories.
     */

    /* Search for the SectionName section */
    if (!SetupFindFirstLineW(InfFile, SectionName, NULL, &FilesContext))
    {
        CHAR Buffer[128];
        sprintf(Buffer, MUIGetString(STRING_TXTSETUPFAILED), SectionName);
        PopupError(Buffer, MUIGetString(STRING_REBOOTCOMPUTER), Ir, POPUP_WAIT_ENTER);
        return FALSE;
    }

    /*
     * Enumerate the files in the section and add them to the file queue.
     */
    do
    {
        /* Get source file name */
        if (!INF_GetDataField(&FilesContext, 0, &FileKeyName))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            break;
        }

        /* Get target directory id */
        if (!INF_GetDataField(&FilesContext, 13, &FileKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            INF_FreeData(FileKeyName);
            break;
        }

        /* Get optional target file name */
        if (!INF_GetDataField(&FilesContext, 11, &TargetFileName))
            TargetFileName = NULL;
        else if (!*TargetFileName)
            TargetFileName = NULL;

        DPRINT("FileKeyName: '%S'  FileKeyValue: '%S'\n", FileKeyName, FileKeyValue);

        /* Lookup target directory */
        if (!SetupFindFirstLineW(InfFile, L"Directories", FileKeyValue, &DirContext))
        {
            /* FIXME: Handle error! */
            DPRINT1("SetupFindFirstLine() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(FileKeyValue);
            INF_FreeData(TargetFileName);
            break;
        }

        INF_FreeData(FileKeyValue);

        if (!INF_GetData(&DirContext, NULL, &DirKeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT1("INF_GetData() failed\n");
            INF_FreeData(FileKeyName);
            INF_FreeData(TargetFileName);
            break;
        }

        if ((DirKeyValue[0] == UNICODE_NULL) || (DirKeyValue[0] == L'\\' && DirKeyValue[1] == UNICODE_NULL))
        {
            /* Installation path */
            DPRINT("InstallationPath: '%S'\n", DirKeyValue);

            StringCchCopyW(CompleteOrigDirName, ARRAYSIZE(CompleteOrigDirName),
                           USetupData.SourceRootDir.Buffer);

            DPRINT("InstallationPath(2): '%S'\n", CompleteOrigDirName);
        }
        else if (DirKeyValue[0] == L'\\')
        {
            /* Absolute path */
            DPRINT("AbsolutePath: '%S'\n", DirKeyValue);

            StringCchCopyW(CompleteOrigDirName, ARRAYSIZE(CompleteOrigDirName),
                           DirKeyValue);

            DPRINT("AbsolutePath(2): '%S'\n", CompleteOrigDirName);
        }
        else // if (DirKeyValue[0] != L'\\')
        {
            /* Path relative to the installation path */
            DPRINT("RelativePath: '%S'\n", DirKeyValue);

            CombinePaths(CompleteOrigDirName, ARRAYSIZE(CompleteOrigDirName), 2,
                         USetupData.SourceRootDir.Buffer, DirKeyValue);

            DPRINT("RelativePath(2): '%S'\n", CompleteOrigDirName);
        }

        if (!SetupQueueCopy(SetupFileQueue,
                            SourceCabinet,
                            USetupData.SourceRootPath.Buffer,
                            CompleteOrigDirName,
                            FileKeyName,
                            DirKeyValue,
                            TargetFileName))
        {
            /* FIXME: Handle error! */
            DPRINT1("SetupQueueCopy() failed\n");
        }

        INF_FreeData(FileKeyName);
        INF_FreeData(TargetFileName);
        INF_FreeData(DirKeyValue);
    } while (SetupFindNextLine(&FilesContext, &FilesContext));

    return TRUE;
}


static BOOLEAN
PrepareCopyPageInfFile(HINF InfFile,
                       PWCHAR SourceCabinet,
                       PINPUT_RECORD Ir)
{
    NTSTATUS Status;
    INFCONTEXT DirContext;
    PWCHAR AdditionalSectionName = NULL;
    PWCHAR DirKeyValue;
    WCHAR PathBuffer[MAX_PATH];

    /* Add common files */
    if (!AddSectionToCopyQueue(InfFile, L"SourceDisksFiles", SourceCabinet, &USetupData.DestinationPath, Ir))
        return FALSE;

    /* Add specific files depending of computer type */
    if (SourceCabinet == NULL)
    {
        if (!ProcessComputerFiles(InfFile, ComputerList, &AdditionalSectionName))
            return FALSE;

        if (AdditionalSectionName)
        {
            if (!AddSectionToCopyQueue(InfFile, AdditionalSectionName, SourceCabinet, &USetupData.DestinationPath, Ir))
                return FALSE;
        }
    }

    /* Create directories */

    /*
     * FIXME:
     * Copying files to USetupData.DestinationRootPath should be done from within
     * the SystemPartitionFiles section.
     * At the moment we check whether we specify paths like '\foo' or '\\' for that.
     * For installing to USetupData.DestinationPath specify just '\' .
     */

    /* Get destination path */
    StringCchCopyW(PathBuffer, ARRAYSIZE(PathBuffer), USetupData.DestinationPath.Buffer);

    DPRINT("FullPath(1): '%S'\n", PathBuffer);

    /* Create the install directory */
    Status = SetupCreateDirectory(PathBuffer);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
    {
        DPRINT1("Creating directory '%S' failed: Status = 0x%08lx\n", PathBuffer, Status);
        MUIDisplayError(ERROR_CREATE_INSTALL_DIR, Ir, POPUP_WAIT_ENTER);
        return FALSE;
    }

    /* Search for the 'Directories' section */
    if (!SetupFindFirstLineW(InfFile, L"Directories", NULL, &DirContext))
    {
        if (SourceCabinet)
        {
            MUIDisplayError(ERROR_CABINET_SECTION, Ir, POPUP_WAIT_ENTER);
        }
        else
        {
            MUIDisplayError(ERROR_TXTSETUP_SECTION, Ir, POPUP_WAIT_ENTER);
        }

        return FALSE;
    }

    /* Enumerate the directory values and create the subdirectories */
    do
    {
        if (!INF_GetData(&DirContext, NULL, &DirKeyValue))
        {
            DPRINT1("break\n");
            break;
        }

        if ((DirKeyValue[0] == UNICODE_NULL) || (DirKeyValue[0] == L'\\' && DirKeyValue[1] == UNICODE_NULL))
        {
            /* Installation path */
            DPRINT("InstallationPath: '%S'\n", DirKeyValue);

            StringCchCopyW(PathBuffer, ARRAYSIZE(PathBuffer),
                           USetupData.DestinationPath.Buffer);

            DPRINT("InstallationPath(2): '%S'\n", PathBuffer);
        }
        else if (DirKeyValue[0] == L'\\')
        {
            /* Absolute path */
            DPRINT("AbsolutePath: '%S'\n", DirKeyValue);

            CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                         USetupData.DestinationRootPath.Buffer, DirKeyValue);

            DPRINT("AbsolutePath(2): '%S'\n", PathBuffer);

            Status = SetupCreateDirectory(PathBuffer);
            if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
            {
                INF_FreeData(DirKeyValue);
                DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
                MUIDisplayError(ERROR_CREATE_DIR, Ir, POPUP_WAIT_ENTER);
                return FALSE;
            }
        }
        else // if (DirKeyValue[0] != L'\\')
        {
            /* Path relative to the installation path */
            DPRINT("RelativePath: '%S'\n", DirKeyValue);

            CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                         USetupData.DestinationPath.Buffer, DirKeyValue);

            DPRINT("RelativePath(2): '%S'\n", PathBuffer);

            Status = SetupCreateDirectory(PathBuffer);
            if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_COLLISION)
            {
                INF_FreeData(DirKeyValue);
                DPRINT("Creating directory '%S' failed: Status = 0x%08lx", PathBuffer, Status);
                MUIDisplayError(ERROR_CREATE_DIR, Ir, POPUP_WAIT_ENTER);
                return FALSE;
            }
        }

        INF_FreeData(DirKeyValue);
    } while (SetupFindNextLine(&DirContext, &DirContext));

    return TRUE;
}


/*
 * Displays the PrepareCopyPage.
 *
 * Next pages:
 *  FileCopyPage(At once)
 *  QuitPage
 *
 * SIDEEFFECTS
 * Inits SetupFileQueue
 * Calls PrepareCopyPageInfFile
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
PrepareCopyPage(PINPUT_RECORD Ir)
{
    HINF InfHandle;
    WCHAR PathBuffer[MAX_PATH];
    INFCONTEXT CabinetsContext;
    ULONG InfFileSize;
    PWCHAR KeyValue;
    UINT ErrorLine;
    PVOID InfFileData;

    MUIDisplayPage(PREPARE_COPY_PAGE);

    /* Create the file queue */
    SetupFileQueue = SetupOpenFileQueue();
    if (SetupFileQueue == NULL)
    {
        MUIDisplayError(ERROR_COPY_QUEUE, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    if (!PrepareCopyPageInfFile(SetupInf, NULL, Ir))
    {
        /* FIXME: show an error dialog */
        return QUIT_PAGE;
    }

    /* Search for the 'Cabinets' section */
    if (!SetupFindFirstLineW(SetupInf, L"Cabinets", NULL, &CabinetsContext))
    {
        return FILE_COPY_PAGE;
    }

    /*
     * Enumerate the directory values in the 'Cabinets'
     * section and parse their inf files.
     */
    do
    {
        if (!INF_GetData(&CabinetsContext, NULL, &KeyValue))
            break;

        CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                     USetupData.SourcePath.Buffer, KeyValue);

#ifdef __REACTOS__
        CabinetInitialize();
        CabinetSetEventHandlers(NULL, NULL, NULL);
        CabinetSetCabinetName(PathBuffer);

        if (CabinetOpen() == CAB_STATUS_SUCCESS)
        {
            DPRINT("Cabinet %S\n", CabinetGetCabinetName());

            InfFileData = CabinetGetCabinetReservedArea(&InfFileSize);
            if (InfFileData == NULL)
            {
                MUIDisplayError(ERROR_CABINET_SCRIPT, Ir, POPUP_WAIT_ENTER);
                return QUIT_PAGE;
            }
        }
        else
        {
            DPRINT("Cannot open cabinet: %S.\n", CabinetGetCabinetName());
            MUIDisplayError(ERROR_CABINET_MISSING, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }

        InfHandle = INF_OpenBufferedFileA((PSTR)InfFileData,
                                          InfFileSize,
                                          NULL,
                                          INF_STYLE_WIN4,
                                          USetupData.LanguageId,
                                          &ErrorLine);

        if (InfHandle == INVALID_HANDLE_VALUE)
        {
            MUIDisplayError(ERROR_INVALID_CABINET_INF, Ir, POPUP_WAIT_ENTER);
            return QUIT_PAGE;
        }

        CabinetCleanup();

        if (!PrepareCopyPageInfFile(InfHandle, KeyValue, Ir))
        {
            /* FIXME: show an error dialog */
            return QUIT_PAGE;
        }
#endif
    } while (SetupFindNextLine(&CabinetsContext, &CabinetsContext));

    return FILE_COPY_PAGE;
}


VOID
NTAPI
SetupUpdateMemoryInfo(IN PCOPYCONTEXT CopyContext,
                      IN BOOLEAN First)
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;

    /* Get the memory information from the system */
    NtQuerySystemInformation(SystemPerformanceInformation,
                             &PerfInfo,
                             sizeof(PerfInfo),
                             NULL);

    /* Check if this is initial setup */
    if (First)
    {
        /* Set maximum limits to be total RAM pages */
        ProgressSetStepCount(CopyContext->MemoryBars[0], PerfInfo.CommitLimit);
        ProgressSetStepCount(CopyContext->MemoryBars[1], PerfInfo.CommitLimit);
        ProgressSetStepCount(CopyContext->MemoryBars[2], PerfInfo.CommitLimit);
    }

    /* Set current values */
    ProgressSetStep(CopyContext->MemoryBars[0], PerfInfo.PagedPoolPages + PerfInfo.NonPagedPoolPages);
    ProgressSetStep(CopyContext->MemoryBars[1], PerfInfo.ResidentSystemCachePage);
    ProgressSetStep(CopyContext->MemoryBars[2], PerfInfo.AvailablePages);
}


static UINT
CALLBACK
FileCopyCallback(PVOID Context,
                 UINT Notification,
                 UINT_PTR Param1,
                 UINT_PTR Param2)
{
    PCOPYCONTEXT CopyContext;

    CopyContext = (PCOPYCONTEXT)Context;

    switch (Notification)
    {
        case SPFILENOTIFY_STARTSUBQUEUE:
            CopyContext->TotalOperations = (ULONG)Param2;
            ProgressSetStepCount(CopyContext->ProgressBar,
                                 CopyContext->TotalOperations);
            SetupUpdateMemoryInfo(CopyContext, TRUE);
            break;

        case SPFILENOTIFY_STARTCOPY:
            /* Display copy message */
            CONSOLE_SetStatusText(MUIGetString(STRING_COPYING), (PWSTR)Param1);
            SetupUpdateMemoryInfo(CopyContext, FALSE);
            break;

        case SPFILENOTIFY_ENDCOPY:
            CopyContext->CompletedOperations++;

            /* SYSREG checkpoint */
            if (CopyContext->TotalOperations >> 1 == CopyContext->CompletedOperations)
                DPRINT1("CHECKPOINT:HALF_COPIED\n");

            ProgressNextStep(CopyContext->ProgressBar);
            SetupUpdateMemoryInfo(CopyContext, FALSE);
            break;
    }

    return 0;
}


/*
 * Displays the FileCopyPage.
 *
 * Next pages:
 *  RegistryPage(At once)
 *
 * SIDEEFFECTS
 *  Calls SetupCommitFileQueueW
 *  Calls SetupCloseFileQueue
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
FileCopyPage(PINPUT_RECORD Ir)
{
    COPYCONTEXT CopyContext;
    unsigned int mem_bar_width;

    MUIDisplayPage(FILE_COPY_PAGE);

    /* Create context for the copy process */
    CopyContext.DestinationRootPath = USetupData.DestinationRootPath.Buffer;
    CopyContext.InstallPath = InstallPath.Buffer;
    CopyContext.TotalOperations = 0;
    CopyContext.CompletedOperations = 0;

    /* Create the progress bar as well */
    CopyContext.ProgressBar = CreateProgressBar(13,
                                                26,
                                                xScreen - 13,
                                                yScreen - 20,
                                                10,
                                                24,
                                                TRUE,
                                                MUIGetString(STRING_SETUPCOPYINGFILES));

    // fit memory bars to screen width, distribute them uniform
    mem_bar_width = (xScreen - 26) / 5;
    mem_bar_width -= mem_bar_width % 2;  // make even
    /* ATTENTION: The following progress bars are debug stuff, which should not be translated!! */
    /* Create the paged pool progress bar */
    CopyContext.MemoryBars[0] = CreateProgressBar(13,
                                                  40,
                                                  13 + mem_bar_width,
                                                  43,
                                                  13,
                                                  44,
                                                  FALSE,
                                                  "Kernel Pool");

    /* Create the non paged pool progress bar */
    CopyContext.MemoryBars[1] = CreateProgressBar((xScreen / 2)- (mem_bar_width / 2),
                                                  40,
                                                  (xScreen / 2) + (mem_bar_width / 2),
                                                  43,
                                                  (xScreen / 2)- (mem_bar_width / 2),
                                                  44,
                                                  FALSE,
                                                  "Kernel Cache");

    /* Create the global memory progress bar */
    CopyContext.MemoryBars[2] = CreateProgressBar(xScreen - 13 - mem_bar_width,
                                                  40,
                                                  xScreen - 13,
                                                  43,
                                                  xScreen - 13 - mem_bar_width,
                                                  44,
                                                  FALSE,
                                                  "Free Memory");

    /* Do the file copying */
    SetupCommitFileQueueW(NULL,
                          SetupFileQueue,
                          FileCopyCallback,
                          &CopyContext);

    /* If we get here, we're done, so cleanup the queue and progress bar */
    SetupCloseFileQueue(SetupFileQueue);
    DestroyProgressBar(CopyContext.ProgressBar);
    DestroyProgressBar(CopyContext.MemoryBars[0]);
    DestroyProgressBar(CopyContext.MemoryBars[1]);
    DestroyProgressBar(CopyContext.MemoryBars[2]);

    /* Create the $winnt$.inf file */
    InstallSetupInfFile(&USetupData);

    /* Go display the next page */
    return REGISTRY_PAGE;
}


/*
 * Displays the RegistryPage.
 *
 * Next pages:
 *  SuccessPage (if RepairUpdate)
 *  BootLoaderPage (default)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Calls RegInitializeRegistry
 *  Calls ImportRegistryFile
 *  Calls SetDefaultPagefile
 *  Calls SetMountedDeviceValues
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
RegistryPage(PINPUT_RECORD Ir)
{
    NTSTATUS Status;
    INFCONTEXT InfContext;
    PWSTR Action;
    PWSTR File;
    PWSTR Section;
    BOOLEAN Success;
    BOOLEAN ShouldRepairRegistry = FALSE;
    BOOLEAN Delete;

    MUIDisplayPage(REGISTRY_PAGE);

    if (RepairUpdateFlag)
    {
        DPRINT1("TODO: Updating / repairing the registry is not completely implemented yet!\n");

        /* Verify the registry hives and check whether we need to update or repair any of them */
        Status = VerifyRegistryHives(&USetupData.DestinationPath, &ShouldRepairRegistry);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("VerifyRegistryHives failed, Status 0x%08lx\n", Status);
            ShouldRepairRegistry = FALSE;
        }
        if (!ShouldRepairRegistry)
            DPRINT1("No need to repair the registry\n");
    }

DoUpdate:
    /* Update the registry */
    CONSOLE_SetStatusText(MUIGetString(STRING_REGHIVEUPDATE));

    /* Initialize the registry and setup the registry hives */
    Status = RegInitializeRegistry(&USetupData.DestinationPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RegInitializeRegistry() failed\n");
        /********** HACK!!!!!!!!!!! **********/
        if (Status == STATUS_NOT_IMPLEMENTED)
        {
            /* The hack was called, display its corresponding error */
            MUIDisplayError(ERROR_INITIALIZE_REGISTRY, Ir, POPUP_WAIT_ENTER);
        }
        else
        /*************************************/
        {
            /* Something else failed */
            MUIDisplayError(ERROR_CREATE_HIVE, Ir, POPUP_WAIT_ENTER);
        }
        return QUIT_PAGE;
    }

    if (!RepairUpdateFlag || ShouldRepairRegistry)
    {
        /*
         * We fully setup the hives, in case we are doing a fresh installation
         * (RepairUpdateFlag == FALSE), or in case we are doing an update
         * (RepairUpdateFlag == TRUE) BUT we have some registry hives to
         * "repair" (aka. recreate: ShouldRepairRegistry == TRUE).
         */

        Success = SetupFindFirstLineW(SetupInf, L"HiveInfs.Fresh", NULL, &InfContext);       // Windows-compatible
        if (!Success)
            Success = SetupFindFirstLineW(SetupInf, L"HiveInfs.Install", NULL, &InfContext); // ReactOS-specific

        if (!Success)
        {
            DPRINT1("SetupFindFirstLine() failed\n");
            MUIDisplayError(ERROR_FIND_REGISTRY, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }
    }
    else // if (RepairUpdateFlag && !ShouldRepairRegistry)
    {
        /*
         * In case we are doing an update (RepairUpdateFlag == TRUE) and
         * NO registry hives need a repair (ShouldRepairRegistry == FALSE),
         * we only update the hives.
         */

        Success = SetupFindFirstLineW(SetupInf, L"HiveInfs.Upgrade", NULL, &InfContext);
        if (!Success)
        {
            /* Nothing to do for update! */
            DPRINT1("No update needed for the registry!\n");
            goto Cleanup;
        }
    }

    do
    {
        INF_GetDataField(&InfContext, 0, &Action);
        INF_GetDataField(&InfContext, 1, &File);
        INF_GetDataField(&InfContext, 2, &Section);

        DPRINT("Action: %S  File: %S  Section %S\n", Action, File, Section);

        if (Action == NULL)
        {
            INF_FreeData(Action);
            INF_FreeData(File);
            INF_FreeData(Section);
            break; // Hackfix
        }

        if (!_wcsicmp(Action, L"AddReg"))
            Delete = FALSE;
        else if (!_wcsicmp(Action, L"DelReg"))
            Delete = TRUE;
        else
        {
            DPRINT1("Unrecognized registry INF action '%S'\n", Action);
            INF_FreeData(Action);
            INF_FreeData(File);
            INF_FreeData(Section);
            continue;
        }

        INF_FreeData(Action);

        CONSOLE_SetStatusText(MUIGetString(STRING_IMPORTFILE), File);

        if (!ImportRegistryFile(USetupData.SourcePath.Buffer, File, Section, USetupData.LanguageId, Delete))
        {
            DPRINT1("Importing %S failed\n", File);
            INF_FreeData(File);
            INF_FreeData(Section);
            MUIDisplayError(ERROR_IMPORT_HIVE, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }
    } while (SetupFindNextLine(&InfContext, &InfContext));

    if (!RepairUpdateFlag || ShouldRepairRegistry)
    {
        /* See the explanation for this test above */

        /* Update display registry settings */
        CONSOLE_SetStatusText(MUIGetString(STRING_DISPLAYETTINGSUPDATE));
        if (!ProcessDisplayRegistry(SetupInf, DisplayList))
        {
            MUIDisplayError(ERROR_UPDATE_DISPLAY_SETTINGS, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }

        /* Set the locale */
        CONSOLE_SetStatusText(MUIGetString(STRING_LOCALESETTINGSUPDATE));
        if (!ProcessLocaleRegistry(LanguageList))
        {
            MUIDisplayError(ERROR_UPDATE_LOCALESETTINGS, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }

        /* Add keyboard layouts */
        CONSOLE_SetStatusText(MUIGetString(STRING_ADDKBLAYOUTS));
        if (!AddKeyboardLayouts())
        {
            MUIDisplayError(ERROR_ADDING_KBLAYOUTS, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }

        /* Set GeoID */
        if (!SetGeoID(MUIGetGeoID()))
        {
            MUIDisplayError(ERROR_UPDATE_GEOID, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }

        if (!IsUnattendedSetup)
        {
            /* Update keyboard layout settings */
            CONSOLE_SetStatusText(MUIGetString(STRING_KEYBOARDSETTINGSUPDATE));
            if (!ProcessKeyboardLayoutRegistry(LayoutList))
            {
                MUIDisplayError(ERROR_UPDATE_KBSETTINGS, Ir, POPUP_WAIT_ENTER);
                goto Cleanup;
            }
        }

        /* Add codepage information to registry */
        CONSOLE_SetStatusText(MUIGetString(STRING_CODEPAGEINFOUPDATE));
        if (!AddCodePage())
        {
            MUIDisplayError(ERROR_ADDING_CODEPAGE, Ir, POPUP_WAIT_ENTER);
            goto Cleanup;
        }

        /* Set the default pagefile entry */
        SetDefaultPagefile(DestinationDriveLetter);

        /* Update the mounted devices list */
        // FIXME: This should technically be done by mountmgr (if AutoMount is enabled)!
        SetMountedDeviceValues(PartitionList);
    }

Cleanup:
    //
    // TODO: Unload all the registry stuff, perform cleanup,
    // and copy the created hive files into .sav files.
    //
    RegCleanupRegistry(&USetupData.DestinationPath);

    /*
     * Check whether we were in update/repair mode but we were actually
     * repairing the registry hives. If so, we have finished repairing them,
     * and we now reset the flag and run the proper registry update.
     * Otherwise we have finished the registry update!
     */
    if (RepairUpdateFlag && ShouldRepairRegistry)
    {
        ShouldRepairRegistry = FALSE;
        goto DoUpdate;
    }

    if (NT_SUCCESS(Status))
    {
        CONSOLE_SetStatusText(MUIGetString(STRING_DONE));
        return BOOT_LOADER_PAGE;
    }
    else
    {
        return QUIT_PAGE;
    }
}


/*
 * Displays the BootLoaderPage.
 *
 * Next pages:
 *  SuccessPage (if RepairUpdate)
 *  BootLoaderHarddiskMbrPage
 *  BootLoaderHarddiskVbrPage
 *  BootLoaderFloppyPage
 *  SuccessPage
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Calls RegInitializeRegistry
 *  Calls ImportRegistryFile
 *  Calls SetDefaultPagefile
 *  Calls SetMountedDeviceValues
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
BootLoaderPage(PINPUT_RECORD Ir)
{
    UCHAR PartitionType;
    BOOLEAN InstallOnFloppy;
    USHORT Line = 12;
    WCHAR PathBuffer[MAX_PATH];

    CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    RtlFreeUnicodeString(&USetupData.SystemRootPath);
    StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
            L"\\Device\\Harddisk%lu\\Partition%lu\\",
            PartitionList->SystemPartition->DiskEntry->DiskNumber,
            PartitionList->SystemPartition->PartitionNumber);
    RtlCreateUnicodeString(&USetupData.SystemRootPath, PathBuffer);
    DPRINT1("SystemRootPath: %wZ\n", &USetupData.SystemRootPath);

    PartitionType = PartitionList->SystemPartition->PartitionType;

    /* For unattended setup, skip MBR installation or install on floppy if needed */
    if (IsUnattendedSetup)
    {
        if ((USetupData.MBRInstallType == 0) ||
            (USetupData.MBRInstallType == 1))
        {
            goto Quit;
        }
    }

    /*
     * We may install an MBR or VBR, but before that, check whether
     * we need to actually install the VBR on floppy.
     */
    if (PartitionType == PARTITION_ENTRY_UNUSED)
    {
        DPRINT("Error: system partition invalid (unused)\n");
        InstallOnFloppy = TRUE;
    }
    else if (PartitionType == PARTITION_OS2BOOTMGR)
    {
        /* OS/2 boot manager partition */
        DPRINT("Found OS/2 boot manager partition\n");
        InstallOnFloppy = TRUE;
    }
    else if (PartitionType == PARTITION_EXT2)
    {
        /* Linux EXT2 partition */
        DPRINT("Found Linux EXT2 partition\n");
        InstallOnFloppy = FALSE;
    }
    else if (PartitionType == PARTITION_IFS)
    {
        /* NTFS partition */
        DPRINT("Found NTFS partition\n");

        // FIXME: Make it FALSE when we'll support NTFS installation!
        InstallOnFloppy = TRUE;
    }
    else if ((PartitionType == PARTITION_FAT_12) ||
             (PartitionType == PARTITION_FAT_16) ||
             (PartitionType == PARTITION_HUGE)   ||
             (PartitionType == PARTITION_XINT13) ||
             (PartitionType == PARTITION_FAT32)  ||
             (PartitionType == PARTITION_FAT32_XINT13))
    {
        DPRINT("Found FAT partition\n");
        InstallOnFloppy = FALSE;
    }
    else
    {
        /* Unknown partition */
        DPRINT("Unknown partition found\n");
        InstallOnFloppy = TRUE;
    }

    /* We should install on floppy */
    if (InstallOnFloppy)
    {
        USetupData.MBRInstallType = 1;
        goto Quit;
    }

    /* Is it an unattended install on hdd? */
    if (IsUnattendedSetup)
    {
        if ((USetupData.MBRInstallType == 2) ||
            (USetupData.MBRInstallType == 3))
        {
            goto Quit;
        }
    }

    MUIDisplayPage(BOOT_LOADER_PAGE);
    CONSOLE_InvertTextXY(8, Line, 60, 1);

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_DOWN))  /* DOWN */
        {
            CONSOLE_NormalTextXY(8, Line, 60, 1);

            Line++;
            if (Line < 12)
                Line = 15;

            if (Line > 15)
                Line = 12;

            CONSOLE_InvertTextXY(8, Line, 60, 1);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_UP))  /* UP */
        {
            CONSOLE_NormalTextXY(8, Line, 60, 1);

            Line--;
            if (Line < 12)
                Line = 15;

            if (Line > 15)
                Line = 12;

            CONSOLE_InvertTextXY(8, Line, 60, 1);
        }
        else if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
                 (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)    /* ENTER */
        {
            if (Line == 12)
            {
                /* Install on both MBR and VBR */
                USetupData.MBRInstallType = 2;
                break;
            }
            else if (Line == 13)
            {
                /* Install on VBR only */
                USetupData.MBRInstallType = 3;
                break;
            }
            else if (Line == 14)
            {
                /* Install on floppy */
                USetupData.MBRInstallType = 1;
                break;
            }
            else if (Line == 15)
            {
                /* Skip MBR installation */
                USetupData.MBRInstallType = 0;
                break;
            }

            return BOOT_LOADER_PAGE;
        }
    }

Quit:
    switch (USetupData.MBRInstallType)
    {
        /* Skip MBR installation */
        case 0:
            return SUCCESS_PAGE;

        /* Install on floppy */
        case 1:
            return BOOT_LOADER_FLOPPY_PAGE;

        /* Install on both MBR and VBR */
        case 2:
            return BOOT_LOADER_HARDDISK_MBR_PAGE;

        /* Install on VBR only */
        case 3:
            return BOOT_LOADER_HARDDISK_VBR_PAGE;
    }

    return BOOT_LOADER_PAGE;
}


/*
 * Displays the BootLoaderFloppyPage.
 *
 * Next pages:
 *  SuccessPage (At once)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Calls InstallFatBootcodeToFloppy()
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
BootLoaderFloppyPage(PINPUT_RECORD Ir)
{
    NTSTATUS Status;

    MUIDisplayPage(BOOT_LOADER_FLOPPY_PAGE);

//  CONSOLE_SetStatusText(MUIGetString(STRING_PLEASEWAIT));

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if ((Ir->Event.KeyEvent.uChar.AsciiChar == 0x00) &&
            (Ir->Event.KeyEvent.wVirtualKeyCode == VK_F3))  /* F3 */
        {
            if (ConfirmQuit(Ir) == TRUE)
                return QUIT_PAGE;

            break;
        }
        else if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D)    /* ENTER */
        {
            if (DoesPathExist(NULL, L"\\Device\\Floppy0\\") == FALSE)
            {
                MUIDisplayError(ERROR_NO_FLOPPY, Ir, POPUP_WAIT_ENTER);
                return BOOT_LOADER_FLOPPY_PAGE;
            }

            Status = InstallFatBootcodeToFloppy(&USetupData.SourceRootPath, &USetupData.DestinationArcPath);
            if (!NT_SUCCESS(Status))
            {
                /* Print error message */
                return BOOT_LOADER_FLOPPY_PAGE;
            }

            return SUCCESS_PAGE;
        }
    }

    return BOOT_LOADER_FLOPPY_PAGE;
}


/*
 * Displays the BootLoaderHarddiskVbrPage.
 *
 * Next pages:
 *  SuccessPage (At once)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Calls InstallVBRToPartition()
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
BootLoaderHarddiskVbrPage(PINPUT_RECORD Ir)
{
    NTSTATUS Status;

    Status = InstallVBRToPartition(&USetupData.SystemRootPath,
                                   &USetupData.SourceRootPath,
                                   &USetupData.DestinationArcPath,
                                   PartitionList->SystemPartition->PartitionType);
    if (!NT_SUCCESS(Status))
    {
        MUIDisplayError(ERROR_WRITE_BOOT, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    return SUCCESS_PAGE;
}


/*
 * Displays the BootLoaderHarddiskMbrPage.
 *
 * Next pages:
 *  SuccessPage (At once)
 *  QuitPage
 *
 * SIDEEFFECTS
 *  Calls InstallVBRToPartition()
 *  Calls InstallMbrBootCodeToDisk()
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
BootLoaderHarddiskMbrPage(PINPUT_RECORD Ir)
{
    NTSTATUS Status;
    WCHAR DestinationDevicePathBuffer[MAX_PATH];
    WCHAR SourceMbrPathBuffer[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Step 1: Write the VBR */
    Status = InstallVBRToPartition(&USetupData.SystemRootPath,
                                   &USetupData.SourceRootPath,
                                   &USetupData.DestinationArcPath,
                                   PartitionList->SystemPartition->PartitionType);
    if (!NT_SUCCESS(Status))
    {
        MUIDisplayError(ERROR_WRITE_BOOT, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    /* Step 2: Write the MBR */
    StringCchPrintfW(DestinationDevicePathBuffer, ARRAYSIZE(DestinationDevicePathBuffer),
            L"\\Device\\Harddisk%d\\Partition0",
            PartitionList->SystemPartition->DiskEntry->DiskNumber);

    CombinePaths(SourceMbrPathBuffer, ARRAYSIZE(SourceMbrPathBuffer), 2, USetupData.SourceRootPath.Buffer, L"\\loader\\dosmbr.bin");

    if (IsThereAValidBootSector(DestinationDevicePathBuffer))
    {
        /* Save current MBR */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, USetupData.SystemRootPath.Buffer, L"mbr.old");

        DPRINT1("Save MBR: %S ==> %S\n", DestinationDevicePathBuffer, DstPath);
        Status = SaveBootSector(DestinationDevicePathBuffer, DstPath, sizeof(PARTITION_SECTOR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SaveBootSector() failed (Status %lx)\n", Status);
            // Don't care if we succeeded or not saving the old MBR, just go ahead.
        }
    }

    DPRINT1("Install MBR bootcode: %S ==> %S\n",
            SourceMbrPathBuffer, DestinationDevicePathBuffer);
    Status = InstallMbrBootCodeToDisk(SourceMbrPathBuffer,
                                      DestinationDevicePathBuffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallMbrBootCodeToDisk() failed (Status %lx)\n",
                Status);
        MUIDisplayError(ERROR_INSTALL_BOOTCODE, Ir, POPUP_WAIT_ENTER);
        return QUIT_PAGE;
    }

    return SUCCESS_PAGE;
}


/*
 * Displays the QuitPage.
 *
 * Next pages:
 *  FlushPage (At once)
 *
 * SIDEEFFECTS
 *  Destroy the Lists
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
QuitPage(PINPUT_RECORD Ir)
{
    MUIDisplayPage(QUIT_PAGE);

    /* Destroy the NTOS installations list */
    if (NtOsInstallsList != NULL)
    {
        DestroyGenericList(NtOsInstallsList, TRUE);
        NtOsInstallsList = NULL;
    }

    /* Destroy the partition list */
    if (PartitionList != NULL)
    {
        DestroyPartitionList(PartitionList);
        PartitionList = NULL;
    }
    TempPartition = NULL;
    FormatState = Start;

    /* Destroy the filesystem list */
    if (FileSystemList != NULL)
    {
        DestroyFileSystemList(FileSystemList);
        FileSystemList = NULL;
    }

    /* Destroy the computer settings list */
    if (ComputerList != NULL)
    {
        DestroyGenericList(ComputerList, TRUE);
        ComputerList = NULL;
    }

    /* Destroy the display settings list */
    if (DisplayList != NULL)
    {
        DestroyGenericList(DisplayList, TRUE);
        DisplayList = NULL;
    }

    /* Destroy the keyboard settings list */
    if (KeyboardList != NULL)
    {
        DestroyGenericList(KeyboardList, TRUE);
        KeyboardList = NULL;
    }

    /* Destroy the keyboard layout list */
    if (LayoutList != NULL)
    {
        DestroyGenericList(LayoutList, TRUE);
        LayoutList = NULL;
    }

    /* Destroy the languages list */
    if (LanguageList != NULL)
    {
        DestroyGenericList(LanguageList, FALSE);
        LanguageList = NULL;
    }

    CONSOLE_SetStatusText(MUIGetString(STRING_REBOOTCOMPUTER2));

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return FLUSH_PAGE;
        }
    }
}


/*
 * Displays the SuccessPage.
 *
 * Next pages:
 *  FlushPage (At once)
 *
 * SIDEEFFECTS
 *  Destroy the Lists
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
SuccessPage(PINPUT_RECORD Ir)
{
    MUIDisplayPage(SUCCESS_PAGE);

    if (IsUnattendedSetup)
        return FLUSH_PAGE;

    while (TRUE)
    {
        CONSOLE_ConInKey(Ir);

        if (Ir->Event.KeyEvent.uChar.AsciiChar == 0x0D) /* ENTER */
        {
            return FLUSH_PAGE;
        }
    }
}


/*
 * Displays the FlushPage.
 *
 * Next pages:
 *  RebootPage (At once)
 *
 * RETURNS
 *   Number of the next page.
 */
static PAGE_NUMBER
FlushPage(PINPUT_RECORD Ir)
{
    MUIDisplayPage(FLUSH_PAGE);
    return REBOOT_PAGE;
}


DWORD WINAPI
PnpEventThread(IN LPVOID lpParameter);


/*
 * The start routine and page management
 */
NTSTATUS
RunUSetup(VOID)
{
    NTSTATUS Status;
    INPUT_RECORD Ir;
    PAGE_NUMBER Page;
    BOOLEAN Old;

    InfSetHeap(ProcessHeap);

    /* Tell the Cm this is a setup boot, and it has to behave accordingly */
    Status = NtInitializeRegistry(CM_BOOT_FLAG_SETUP);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtInitializeRegistry() failed (Status 0x%08lx)\n", Status);

    /* Create the PnP thread in suspended state */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 TRUE,
                                 0,
                                 0,
                                 0,
                                 PnpEventThread,
                                 &SetupInf,
                                 &hPnpThread,
                                 NULL);
    if (!NT_SUCCESS(Status))
        hPnpThread = NULL;

    if (!CONSOLE_Init())
    {
        PrintString(MUIGetString(STRING_CONSOLEFAIL1));
        PrintString(MUIGetString(STRING_CONSOLEFAIL2));
        PrintString(MUIGetString(STRING_CONSOLEFAIL3));

        /* We failed to initialize the video, just quit the installer */
        return STATUS_APP_INIT_FAILURE;
    }

    /* Initialize global unicode strings */
    RtlInitUnicodeString(&USetupData.SourcePath, NULL);
    RtlInitUnicodeString(&USetupData.SourceRootPath, NULL);
    RtlInitUnicodeString(&USetupData.SourceRootDir, NULL);
    RtlInitUnicodeString(&InstallPath, NULL);
    RtlInitUnicodeString(&USetupData.DestinationPath, NULL);
    RtlInitUnicodeString(&USetupData.DestinationArcPath, NULL);
    RtlInitUnicodeString(&USetupData.DestinationRootPath, NULL);
    RtlInitUnicodeString(&USetupData.SystemRootPath, NULL);

    /* Hide the cursor */
    CONSOLE_SetCursorType(TRUE, FALSE);

    /* Global Initialization page */
    CONSOLE_ClearScreen();
    CONSOLE_Flush();
    Page = SetupStartPage(&Ir);

    while (Page != REBOOT_PAGE && Page != RECOVERY_PAGE)
    {
        CONSOLE_ClearScreen();
        CONSOLE_Flush();

        // CONSOLE_SetUnderlinedTextXY(4, 3, " ReactOS " KERNEL_VERSION_STR " Setup ");
        // CONSOLE_Flush();

        switch (Page)
        {
            /* Language page */
            case LANGUAGE_PAGE:
                Page = LanguagePage(&Ir);
                break;

            /* Welcome page */
            case WELCOME_PAGE:
                Page = WelcomePage(&Ir);
                break;

            /* License page */
            case LICENSE_PAGE:
                Page = LicensePage(&Ir);
                break;

            /* Install pages */
            case INSTALL_INTRO_PAGE:
                Page = InstallIntroPage(&Ir);
                break;

#if 0
            case SCSI_CONTROLLER_PAGE:
                Page = ScsiControllerPage(&Ir);
                break;

            case OEM_DRIVER_PAGE:
                Page = OemDriverPage(&Ir);
                break;
#endif

            case DEVICE_SETTINGS_PAGE:
                Page = DeviceSettingsPage(&Ir);
                break;

            case COMPUTER_SETTINGS_PAGE:
                Page = ComputerSettingsPage(&Ir);
                break;

            case DISPLAY_SETTINGS_PAGE:
                Page = DisplaySettingsPage(&Ir);
                break;

            case KEYBOARD_SETTINGS_PAGE:
                Page = KeyboardSettingsPage(&Ir);
                break;

            case LAYOUT_SETTINGS_PAGE:
                Page = LayoutSettingsPage(&Ir);
                break;

            case SELECT_PARTITION_PAGE:
                Page = SelectPartitionPage(&Ir);
                break;

            case CREATE_PRIMARY_PARTITION_PAGE:
                Page = CreatePrimaryPartitionPage(&Ir);
                break;

            case CREATE_EXTENDED_PARTITION_PAGE:
                Page = CreateExtendedPartitionPage(&Ir);
                break;

            case CREATE_LOGICAL_PARTITION_PAGE:
                Page = CreateLogicalPartitionPage(&Ir);
                break;

            case CONFIRM_DELETE_SYSTEM_PARTITION_PAGE:
                Page = ConfirmDeleteSystemPartitionPage(&Ir);
                break;

            case DELETE_PARTITION_PAGE:
                Page = DeletePartitionPage(&Ir);
                break;

            case SELECT_FILE_SYSTEM_PAGE:
                Page = SelectFileSystemPage(&Ir);
                break;

            case FORMAT_PARTITION_PAGE:
                Page = FormatPartitionPage(&Ir);
                break;

            case CHECK_FILE_SYSTEM_PAGE:
                Page = CheckFileSystemPage(&Ir);
                break;

            case INSTALL_DIRECTORY_PAGE:
                Page = InstallDirectoryPage(&Ir);
                break;

            case PREPARE_COPY_PAGE:
                Page = PrepareCopyPage(&Ir);
                break;

            case FILE_COPY_PAGE:
                Page = FileCopyPage(&Ir);
                break;

            case REGISTRY_PAGE:
                Page = RegistryPage(&Ir);
                break;

            case BOOT_LOADER_PAGE:
                Page = BootLoaderPage(&Ir);
                break;

            case BOOT_LOADER_FLOPPY_PAGE:
                Page = BootLoaderFloppyPage(&Ir);
                break;

            case BOOT_LOADER_HARDDISK_MBR_PAGE:
                Page = BootLoaderHarddiskMbrPage(&Ir);
                break;

            case BOOT_LOADER_HARDDISK_VBR_PAGE:
                Page = BootLoaderHarddiskVbrPage(&Ir);
                break;

            /* Repair pages */
            case UPGRADE_REPAIR_PAGE:
                Page = UpgradeRepairPage(&Ir);
                break;

            case SUCCESS_PAGE:
                Page = SuccessPage(&Ir);
                break;

            case FLUSH_PAGE:
                Page = FlushPage(&Ir);
                break;

            case QUIT_PAGE:
                Page = QuitPage(&Ir);
                break;

            case RECOVERY_PAGE:
            case REBOOT_PAGE:
                break;
        }
    }

    SetupCloseInfFile(SetupInf);

    if (Page == RECOVERY_PAGE)
        RecoveryConsole();

    FreeConsole();

    /* Reboot */
    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &Old);
    NtShutdownSystem(ShutdownReboot);
    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, Old, FALSE, &Old);

    return STATUS_SUCCESS;
}


#ifdef __REACTOS__

VOID NTAPI
NtProcessStartup(PPEB Peb)
{
    NTSTATUS Status;
    LARGE_INTEGER Time;

    RtlNormalizeProcessParams(Peb->ProcessParameters);

    ProcessHeap = Peb->ProcessHeap;

    NtQuerySystemTime(&Time);

    Status = RunUSetup();

    if (NT_SUCCESS(Status))
    {
        /*
         * Avoid a bugcheck if RunUSetup() finishes too quickly by implementing
         * a protective waiting.
         * This wait is needed because, since we are started as SMSS.EXE,
         * the NT kernel explicitly waits 5 seconds for the initial process
         * SMSS.EXE to initialize (as a protective measure), and otherwise
         * bugchecks with the code SESSION5_INITIALIZATION_FAILED.
         */
        Time.QuadPart += 50000000;
        NtDelayExecution(FALSE, &Time);
    }
    else
    {
        /* The installer failed to start: raise a hard error (crash the system/BSOD) */
        Status = NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
                                  0, 0, NULL, 0, NULL);
    }

    NtTerminateProcess(NtCurrentProcess(), Status);
}

#endif /* __REACTOS__ */

/* EOF */
