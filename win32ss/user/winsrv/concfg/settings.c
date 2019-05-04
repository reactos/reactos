/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/concfg/settings.c
 * PURPOSE:         Console settings management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

// /* Undocumented user definitions */
// #include <undocuser.h>

#include "settings.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* Default cursor size -- see conio_winsrv.h */
// #define SMALL_SIZE 25
#define CSR_DEFAULT_CURSOR_SIZE 25

/* Default attributes -- see conio.h */
#define DEFAULT_SCREEN_ATTRIB   (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)
#define DEFAULT_POPUP_ATTRIB    (FOREGROUND_BLUE | FOREGROUND_RED   | \
                                 BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)

/*
 * Default 16-color palette for foreground and background
 * (corresponding flags in comments).
 */
static const COLORREF s_Colors[16] =
{
    RGB(0, 0, 0),       // (Black)
    RGB(0, 0, 128),     // BLUE
    RGB(0, 128, 0),     // GREEN
    RGB(0, 128, 128),   // BLUE  | GREEN
    RGB(128, 0, 0),     // RED
    RGB(128, 0, 128),   // BLUE  | RED
    RGB(128, 128, 0),   // GREEN | RED
    RGB(192, 192, 192), // BLUE  | GREEN | RED

    RGB(128, 128, 128), // (Grey)  INTENSITY
    RGB(0, 0, 255),     // BLUE  | INTENSITY
    RGB(0, 255, 0),     // GREEN | INTENSITY
    RGB(0, 255, 255),   // BLUE  | GREEN | INTENSITY
    RGB(255, 0, 0),     // RED   | INTENSITY
    RGB(255, 0, 255),   // BLUE  | RED   | INTENSITY
    RGB(255, 255, 0),   // GREEN | RED   | INTENSITY
    RGB(255, 255, 255)  // BLUE  | GREEN | RED | INTENSITY
};


/* FUNCTIONS ******************************************************************/

static VOID
TranslateConsoleName(
    OUT LPWSTR DestString,
    IN  LPCWSTR ConsoleName,
    IN  UINT MaxStrLen)
{
#define PATH_SEPARATOR L'\\'

    UINT wLength;

    if ( DestString   == NULL  || ConsoleName  == NULL ||
         *ConsoleName == L'\0' || MaxStrLen    == 0 )
    {
        return;
    }

    wLength = GetSystemWindowsDirectoryW(DestString, MaxStrLen);
    if ((wLength > 0) && (_wcsnicmp(ConsoleName, DestString, wLength) == 0))
    {
        StringCchCopyW(DestString, MaxStrLen, L"%SystemRoot%");
        StringCchCatW(DestString, MaxStrLen, ConsoleName + wLength);
    }
    else
    {
        StringCchCopyW(DestString, MaxStrLen, ConsoleName);
    }

    /* Replace path separators (backslashes) by underscores */
    while ((DestString = wcschr(DestString, PATH_SEPARATOR))) *DestString = L'_';
}

BOOLEAN
ConCfgOpenUserSettings(
    IN  LPCWSTR ConsoleTitle,
    OUT PHKEY phSubKey,
    IN  REGSAM samDesired,
    IN  BOOLEAN Create)
{
    BOOLEAN Success = TRUE;
    NTSTATUS Status;
    WCHAR szBuffer[MAX_PATH] = L"Console\\";
    WCHAR szBuffer2[MAX_PATH] = L"";
    HKEY hKey; // CurrentUserKeyHandle

    /*
     * Console properties are stored under the HKCU\Console\* key.
     *
     * We use the original console title as the subkey name for storing
     * console properties. We need to distinguish whether we were launched
     * via the console application directly or via a shortcut.
     *
     * If the title of the console corresponds to a path (more precisely,
     * if the title is of the form: C:\ReactOS\<some_path>\<some_app.exe>),
     * then use the corresponding unexpanded path and with the backslashes
     * replaced by underscores, to make the registry happy,
     *     i.e. %SystemRoot%_<some_path>_<some_app.exe>
     */

    /* Open the per-user registry key where the console properties are saved */
    Status = RtlOpenCurrentUser(/*samDesired*/MAXIMUM_ALLOWED, (PHANDLE)&/*CurrentUserKeyHandle*/hKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlOpenCurrentUser() failed, Status = 0x%08lx\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /*
     * Try to open properties via the console title:
     * to make the registry happy, replace all the
     * backslashes by underscores.
     */
    TranslateConsoleName(szBuffer2, ConsoleTitle, ARRAYSIZE(szBuffer2));

    /* Create the registry path */
    StringCchCatW(szBuffer, MAX_PATH - wcslen(szBuffer) - 1, szBuffer2);

    /* Create or open the registry key */
    if (Create)
    {
        /* Create the key */
        Success = (RegCreateKeyExW(hKey,
                                   szBuffer,
                                   0, NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   samDesired,
                                   NULL,
                                   phSubKey,
                                   NULL) == ERROR_SUCCESS);
    }
    else
    {
        /* Open the key */
        Success = (RegOpenKeyExW(hKey,
                                 szBuffer,
                                 0,
                                 samDesired,
                                 phSubKey) == ERROR_SUCCESS);
    }

    /* Close the parent key and return success or not */
    NtClose(hKey);
    return Success;
}

BOOLEAN
ConCfgReadUserSettings(
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo,
    IN BOOLEAN DefaultSettings)
{
    BOOLEAN Success = FALSE;
    HKEY  hKey;
    DWORD dwNumValues = 0;
    DWORD dwIndex;
    DWORD dwColorIndex = 0;
    DWORD dwType;
    WCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    WCHAR szValue[LF_FACESIZE] = L"";
    DWORD Value;
    DWORD dwValue;

    if (!ConCfgOpenUserSettings(DefaultSettings ? L"" : ConsoleInfo->ConsoleTitle,
                                &hKey, KEY_READ, FALSE))
    {
        DPRINT("ConCfgOpenUserSettings() failed\n");
        return FALSE;
    }

    if (RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                         &dwNumValues, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        DPRINT("ConCfgReadUserSettings: RegQueryInfoKeyW() failed\n");
        RegCloseKey(hKey);
        return FALSE;
    }

    DPRINT("ConCfgReadUserSettings() entered, dwNumValues %d\n", dwNumValues);

    for (dwIndex = 0; dwIndex < dwNumValues; dwIndex++)
    {
        dwValue = sizeof(Value);
        dwValueName = ARRAYSIZE(szValueName);

        if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, &dwType, (BYTE*)&Value, &dwValue) != ERROR_SUCCESS)
        {
            if (dwType == REG_SZ)
            {
                /*
                 * Retry in case of string value
                 */
                dwValue = sizeof(szValue);
                dwValueName = ARRAYSIZE(szValueName);
                if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (BYTE*)szValue, &dwValue) != ERROR_SUCCESS)
                    break;
            }
            else
            {
                break;
            }
        }

        if (!wcsncmp(szValueName, L"ColorTable", wcslen(L"ColorTable")))
        {
            dwColorIndex = 0;
            swscanf(szValueName, L"ColorTable%2d", &dwColorIndex);
            if (dwColorIndex < ARRAYSIZE(ConsoleInfo->ColorTable))
            {
                ConsoleInfo->ColorTable[dwColorIndex] = Value;
                Success = TRUE;
            }
        }
        if (!wcscmp(szValueName, L"CodePage"))
        {
            if (IsValidCodePage(Value))
                ConsoleInfo->CodePage = Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"FaceName"))
        {
            /* A NULL value means that the defaults should be used instead */
            if (*szValue)
            {
                StringCchCopyNW(ConsoleInfo->FaceName, ARRAYSIZE(ConsoleInfo->FaceName),
                                szValue, ARRAYSIZE(szValue));
            }
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"FontFamily"))
        {
            /* A zero value means that the defaults should be used instead */
            if (Value)
                ConsoleInfo->FontFamily = Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"FontSize"))
        {
            /* A zero value means that the defaults should be used instead */
            if (Value)
            {
                ConsoleInfo->FontSize.X = LOWORD(Value); // Width
                ConsoleInfo->FontSize.Y = HIWORD(Value); // Height
            }
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"FontWeight"))
        {
            /* A zero value means that the defaults should be used instead */
            if (Value)
                ConsoleInfo->FontWeight = Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"HistoryBufferSize"))
        {
            ConsoleInfo->HistoryBufferSize = Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"NumberOfHistoryBuffers"))
        {
            ConsoleInfo->NumberOfHistoryBuffers = Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"HistoryNoDup"))
        {
            ConsoleInfo->HistoryNoDup = !!Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"QuickEdit"))
        {
            ConsoleInfo->QuickEdit = !!Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"InsertMode"))
        {
            ConsoleInfo->InsertMode = !!Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"ScreenBufferSize"))
        {
            ConsoleInfo->ScreenBufferSize.X = LOWORD(Value);
            ConsoleInfo->ScreenBufferSize.Y = HIWORD(Value);
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"FullScreen"))
        {
            ConsoleInfo->FullScreen = Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"WindowPosition"))
        {
            ConsoleInfo->AutoPosition     = FALSE;
            ConsoleInfo->WindowPosition.x = LOWORD(Value);
            ConsoleInfo->WindowPosition.y = HIWORD(Value);
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"WindowSize"))
        {
            ConsoleInfo->WindowSize.X = LOWORD(Value);
            ConsoleInfo->WindowSize.Y = HIWORD(Value);
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"CursorSize"))
        {
            ConsoleInfo->CursorSize = min(max(Value, 0), 100);
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"ScreenColors"))
        {
            ConsoleInfo->ScreenAttributes = (USHORT)Value;
            Success = TRUE;
        }
        else if (!wcscmp(szValueName, L"PopupColors"))
        {
            ConsoleInfo->PopupAttributes = (USHORT)Value;
            Success = TRUE;
        }
    }

    RegCloseKey(hKey);
    return Success;
}

BOOLEAN
ConCfgWriteUserSettings(
    IN PCONSOLE_STATE_INFO ConsoleInfo,
    IN BOOLEAN DefaultSettings)
{
    HKEY hKey;
    DWORD Storage = 0;

#define SetConsoleSetting(SettingName, SettingType, SettingSize, Setting, DefaultValue)         \
do {                                                                                            \
    if (DefaultSettings || (!DefaultSettings && (*(Setting) != (DefaultValue))))                \
    {                                                                                           \
        RegSetValueExW(hKey, (SettingName), 0, (SettingType), (PBYTE)(Setting), (SettingSize)); \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        RegDeleteValueW(hKey, (SettingName));                                                   \
    }                                                                                           \
} while (0)

    WCHAR szValueName[15];
    UINT i;

    if (!ConCfgOpenUserSettings(DefaultSettings ? L"" : ConsoleInfo->ConsoleTitle,
                                &hKey, KEY_WRITE, TRUE))
    {
        return FALSE;
    }

    for (i = 0; i < ARRAYSIZE(ConsoleInfo->ColorTable); ++i)
    {
        /*
         * Write only the new value if we are saving the global settings
         * or we are saving settings for a particular console, which differs
         * from the default ones.
         */
        swprintf(szValueName, L"ColorTable%02u", i);
        SetConsoleSetting(szValueName, REG_DWORD, sizeof(DWORD), &ConsoleInfo->ColorTable[i], s_Colors[i]);
    }

    SetConsoleSetting(L"CodePage", REG_DWORD, sizeof(DWORD), &ConsoleInfo->CodePage, CP_ACP /* CP_OEMCP */);
    SetConsoleSetting(L"FaceName", REG_SZ, (wcslen(ConsoleInfo->FaceName) + 1) * sizeof(WCHAR), ConsoleInfo->FaceName, UNICODE_NULL); // wcsnlen
    SetConsoleSetting(L"FontFamily", REG_DWORD, sizeof(DWORD), &ConsoleInfo->FontFamily, FF_DONTCARE);

    Storage = MAKELONG(ConsoleInfo->FontSize.X, ConsoleInfo->FontSize.Y); // Width, Height
    SetConsoleSetting(L"FontSize", REG_DWORD, sizeof(DWORD), &Storage, 0);

    SetConsoleSetting(L"FontWeight", REG_DWORD, sizeof(DWORD), &ConsoleInfo->FontWeight, FW_DONTCARE);

    SetConsoleSetting(L"HistoryBufferSize", REG_DWORD, sizeof(DWORD), &ConsoleInfo->HistoryBufferSize, 50);
    SetConsoleSetting(L"NumberOfHistoryBuffers", REG_DWORD, sizeof(DWORD), &ConsoleInfo->NumberOfHistoryBuffers, 4);

    Storage = ConsoleInfo->HistoryNoDup;
    SetConsoleSetting(L"HistoryNoDup", REG_DWORD, sizeof(DWORD), &Storage, FALSE);

    Storage = ConsoleInfo->QuickEdit;
    SetConsoleSetting(L"QuickEdit", REG_DWORD, sizeof(DWORD), &Storage, FALSE);

    Storage = ConsoleInfo->InsertMode;
    SetConsoleSetting(L"InsertMode", REG_DWORD, sizeof(DWORD), &Storage, TRUE);

    Storage = MAKELONG(ConsoleInfo->ScreenBufferSize.X, ConsoleInfo->ScreenBufferSize.Y);
    SetConsoleSetting(L"ScreenBufferSize", REG_DWORD, sizeof(DWORD), &Storage, MAKELONG(80, 300));

    Storage = ConsoleInfo->FullScreen;
    SetConsoleSetting(L"FullScreen", REG_DWORD, sizeof(DWORD), &Storage, FALSE);

    if (ConsoleInfo->AutoPosition == FALSE)
    {
        Storage = MAKELONG(ConsoleInfo->WindowPosition.x, ConsoleInfo->WindowPosition.y);
        RegSetValueExW(hKey, L"WindowPosition", 0, REG_DWORD, (PBYTE)&Storage, sizeof(DWORD));
    }
    else
    {
        RegDeleteValueW(hKey, L"WindowPosition");
    }

    Storage = MAKELONG(ConsoleInfo->WindowSize.X, ConsoleInfo->WindowSize.Y);
    SetConsoleSetting(L"WindowSize", REG_DWORD, sizeof(DWORD), &Storage, MAKELONG(80, 25));

    SetConsoleSetting(L"CursorSize", REG_DWORD, sizeof(DWORD), &ConsoleInfo->CursorSize, CSR_DEFAULT_CURSOR_SIZE);

    Storage = ConsoleInfo->ScreenAttributes;
    SetConsoleSetting(L"ScreenColors", REG_DWORD, sizeof(DWORD), &Storage, DEFAULT_SCREEN_ATTRIB);

    Storage = ConsoleInfo->PopupAttributes;
    SetConsoleSetting(L"PopupColors", REG_DWORD, sizeof(DWORD), &Storage, DEFAULT_POPUP_ATTRIB);

    RegCloseKey(hKey);
    return TRUE;
}

VOID
ConCfgInitDefaultSettings(
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo)
{
    if (ConsoleInfo == NULL) return;

    // ASSERT(ConsoleInfo->cbSize >= sizeof(CONSOLE_STATE_INFO));

/// HKCU,"Console","LoadConIme",0x00010003,1

    // wcsncpy(ConsoleInfo->FaceName, L"DejaVu Sans Mono", LF_FACESIZE);
    // ConsoleInfo->FontSize = MAKELONG(8, 12); // 0x000C0008; // font is 8x12

    StringCchCopyW(ConsoleInfo->FaceName, ARRAYSIZE(ConsoleInfo->FaceName), L"VGA"); // HACK: !!
    // ConsoleInfo->FaceName[0] = UNICODE_NULL;
    // ConsoleInfo->FontSize.X = 8;
    // ConsoleInfo->FontSize.Y = 12;
    ConsoleInfo->FontSize.X = 0;  // HACK: !!
    ConsoleInfo->FontSize.Y = 16; // HACK: !!
    ConsoleInfo->FontFamily = FF_DONTCARE;
    ConsoleInfo->FontWeight = FW_NORMAL; // FW_DONTCARE;

    /* Initialize the default properties */

// #define DEFAULT_HISTORY_COMMANDS_NUMBER 50
// #define DEFAULT_HISTORY_BUFFERS_NUMBER 4
    ConsoleInfo->HistoryBufferSize = 50;
    ConsoleInfo->NumberOfHistoryBuffers = 4;
    ConsoleInfo->HistoryNoDup = FALSE;

    ConsoleInfo->QuickEdit  = FALSE;
    ConsoleInfo->InsertMode = TRUE;
    // ConsoleInfo->InputBufferSize = 0;

    // Rule: ScreenBufferSize >= WindowSize
    ConsoleInfo->ScreenBufferSize.X = 80;
    ConsoleInfo->ScreenBufferSize.Y = 300;
    ConsoleInfo->WindowSize.X = 80;
    ConsoleInfo->WindowSize.Y = 25;

    ConsoleInfo->FullScreen   = FALSE;
    ConsoleInfo->AutoPosition = TRUE;
    ConsoleInfo->WindowPosition.x = 0;
    ConsoleInfo->WindowPosition.y = 0;

    ConsoleInfo->CursorSize = CSR_DEFAULT_CURSOR_SIZE;

    ConsoleInfo->ScreenAttributes = DEFAULT_SCREEN_ATTRIB;
    ConsoleInfo->PopupAttributes  = DEFAULT_POPUP_ATTRIB;

    RtlCopyMemory(ConsoleInfo->ColorTable, s_Colors, sizeof(s_Colors));

    ConsoleInfo->CodePage = GetOEMCP();
}

VOID
ConCfgGetDefaultSettings(
    IN OUT PCONSOLE_STATE_INFO ConsoleInfo)
{
    if (ConsoleInfo == NULL) return;

    /*
     * 1. Load the default values
     */
    ConCfgInitDefaultSettings(ConsoleInfo);

    /*
     * 2. Overwrite them with the ones stored in HKCU\Console.
     *    If the HKCU\Console key doesn't exist, create it
     *    and store the default values inside.
     */
    if (!ConCfgReadUserSettings(ConsoleInfo, TRUE))
        ConCfgWriteUserSettings(ConsoleInfo, TRUE);
}

/* EOF */
