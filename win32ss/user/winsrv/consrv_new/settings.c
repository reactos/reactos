/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/settings.c
 * PURPOSE:         Console settings management
 * PROGRAMMERS:     Johannes Anderwald
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/conio2.h"
#include "include/settings.h"

#include <stdio.h> // for swprintf

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

extern const COLORREF s_Colors[16];


/* FUNCTIONS ******************************************************************/

static VOID
TranslateConsoleName(OUT LPWSTR DestString,
                     IN LPCWSTR ConsoleName,
                     IN UINT MaxStrLen)
{
#define PATH_SEPARATOR L'\\'

    UINT wLength;

    if ( DestString   == NULL  || ConsoleName  == NULL ||
         *ConsoleName == L'\0' || MaxStrLen    == 0 )
    {
        return;
    }

    wLength = GetWindowsDirectoryW(DestString, MaxStrLen);
    if ((wLength > 0) && (_wcsnicmp(ConsoleName, DestString, wLength) == 0))
    {
        wcsncpy(DestString, L"%SystemRoot%", MaxStrLen);
        // FIXME: Fix possible buffer overflows there !!!!!
        wcsncat(DestString, ConsoleName + wLength, MaxStrLen);
    }
    else
    {
        wcsncpy(DestString, ConsoleName, MaxStrLen);
    }

    /* Replace path separators (backslashes) by underscores */
    while ((DestString = wcschr(DestString, PATH_SEPARATOR))) *DestString = L'_';
}

static BOOL
OpenUserRegistryPathPerProcessId(DWORD ProcessId,
                                 PHKEY hResult,
                                 REGSAM samDesired)
{
    BOOL bRet = TRUE;
    HANDLE hProcessToken = NULL;
    HANDLE hProcess;
    BYTE Buffer[256];
    DWORD Length = 0;
    UNICODE_STRING SidName;
    PTOKEN_USER TokUser;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | READ_CONTROL, FALSE, ProcessId);
    if (!hProcess)
    {
        DPRINT1("Error: OpenProcess failed(0x%x)\n", GetLastError());
        return FALSE;
    }

    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hProcessToken))
    {
        DPRINT1("Error: OpenProcessToken failed(0x%x)\n", GetLastError());
        CloseHandle(hProcess);
        return FALSE;
    }

    if (!GetTokenInformation(hProcessToken, TokenUser, (PVOID)Buffer, sizeof(Buffer), &Length))
    {
        DPRINT1("Error: GetTokenInformation failed(0x%x)\n",GetLastError());
        CloseHandle(hProcessToken);
        CloseHandle(hProcess);
        return FALSE;
    }

    TokUser = ((PTOKEN_USER)Buffer)->User.Sid;
    if (!NT_SUCCESS(RtlConvertSidToUnicodeString(&SidName, TokUser, TRUE)))
    {
        DPRINT1("Error: RtlConvertSidToUnicodeString failed(0x%x)\n", GetLastError());
        CloseHandle(hProcessToken);
        CloseHandle(hProcess);
        return FALSE;
    }

    /*
     * Might fail for LiveCD... Why ? Because only HKU\.DEFAULT exists.
     */
    bRet = (RegOpenKeyExW(HKEY_USERS,
                          SidName.Buffer,
                          0,
                          samDesired,
                          hResult) == ERROR_SUCCESS);

    RtlFreeUnicodeString(&SidName);

    CloseHandle(hProcessToken);
    CloseHandle(hProcess);

    return bRet;
}

/*static*/ BOOL
ConSrvOpenUserSettings(DWORD ProcessId,
                       LPCWSTR ConsoleTitle,
                       PHKEY hSubKey,
                       REGSAM samDesired,
                       BOOL bCreate)
{
    BOOL bRet = TRUE;
    WCHAR szBuffer[MAX_PATH] = L"Console\\";
    WCHAR szBuffer2[MAX_PATH] = L"";
    HKEY hKey;

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

    /* Open the registry key where we saved the console properties */
    if (!OpenUserRegistryPathPerProcessId(ProcessId, &hKey, samDesired))
    {
        DPRINT1("OpenUserRegistryPathPerProcessId failed\n");
        return FALSE;
    }

    /*
     * Try to open properties via the console title:
     * to make the registry happy, replace all the
     * backslashes by underscores.
     */
    TranslateConsoleName(szBuffer2, ConsoleTitle, MAX_PATH);

    /* Create the registry path */
    wcsncat(szBuffer, szBuffer2, MAX_PATH);

    /* Create or open the registry key */
    if (bCreate)
    {
        /* Create the key */
        bRet = (RegCreateKeyExW(hKey,
                                szBuffer,
                                0, NULL,
                                REG_OPTION_NON_VOLATILE,
                                samDesired,
                                NULL,
                                hSubKey,
                                NULL) == ERROR_SUCCESS);
    }
    else
    {
        /* Open the key */
        bRet = (RegOpenKeyExW(hKey,
                              szBuffer,
                              0,
                              samDesired,
                              hSubKey) == ERROR_SUCCESS);
    }

    /* Close the parent key and return success or not */
    RegCloseKey(hKey);
    return bRet;
}

BOOL
ConSrvReadUserSettings(IN OUT PCONSOLE_INFO ConsoleInfo,
                       IN DWORD ProcessId)
{
    BOOL  RetVal = FALSE;
    HKEY  hKey;
    DWORD dwNumSubKeys = 0;
    DWORD dwIndex;
    DWORD dwColorIndex = 0;
    DWORD dwType;
    WCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    WCHAR szValue[LF_FACESIZE] = L"\0";
    DWORD Value;
    DWORD dwValue;

    if (!ConSrvOpenUserSettings(ProcessId,
                                ConsoleInfo->ConsoleTitle,
                                &hKey, KEY_READ,
                                FALSE))
    {
        DPRINT("ConSrvOpenUserSettings failed\n");
        return FALSE;
    }

    if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                        &dwNumSubKeys, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
    {
        DPRINT("ConSrvReadUserSettings: RegQueryInfoKey failed\n");
        RegCloseKey(hKey);
        return FALSE;
    }

    DPRINT("ConSrvReadUserSettings entered dwNumSubKeys %d\n", dwNumSubKeys);

    for (dwIndex = 0; dwIndex < dwNumSubKeys; dwIndex++)
    {
        dwValue = sizeof(Value);
        dwValueName = MAX_PATH; // sizeof(szValueName)/sizeof(szValueName[0])

        if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, &dwType, (BYTE*)&Value, &dwValue) != ERROR_SUCCESS)
        {
            if (dwType == REG_SZ)
            {
                /*
                 * Retry in case of string value
                 */
                dwValue = sizeof(szValue);
                dwValueName = MAX_PATH; // sizeof(szValueName)/sizeof(szValueName[0])
                if (RegEnumValueW(hKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (BYTE*)szValue, &dwValue) != ERROR_SUCCESS)
                    break;
            }
            else
            {
                break;
            }
        }

        /* Maybe it is UI-specific ?? */
        if (!wcsncmp(szValueName, L"ColorTable", wcslen(L"ColorTable")))
        {
            dwColorIndex = 0;
            swscanf(szValueName, L"ColorTable%2d", &dwColorIndex);
            if (dwColorIndex < sizeof(ConsoleInfo->Colors)/sizeof(ConsoleInfo->Colors[0]))
            {
                ConsoleInfo->Colors[dwColorIndex] = Value;
                RetVal = TRUE;
            }
        }
        else if (!wcscmp(szValueName, L"HistoryBufferSize"))
        {
            ConsoleInfo->HistoryBufferSize = Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"NumberOfHistoryBuffers"))
        {
            ConsoleInfo->NumberOfHistoryBuffers = Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"HistoryNoDup"))
        {
            ConsoleInfo->HistoryNoDup = (BOOLEAN)Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"QuickEdit"))
        {
            ConsoleInfo->QuickEdit = (BOOLEAN)Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"InsertMode"))
        {
            ConsoleInfo->InsertMode = (BOOLEAN)Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"ScreenBufferSize"))
        {
            ConsoleInfo->ScreenBufferSize.X = LOWORD(Value);
            ConsoleInfo->ScreenBufferSize.Y = HIWORD(Value);
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"WindowSize"))
        {
            ConsoleInfo->ConsoleSize.X = LOWORD(Value);
            ConsoleInfo->ConsoleSize.Y = HIWORD(Value);
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"CursorSize"))
        {
            ConsoleInfo->CursorSize = min(max(Value, 0), 100);
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"ScreenColors"))
        {
            ConsoleInfo->ScreenAttrib = (USHORT)Value;
            RetVal = TRUE;
        }
        else if (!wcscmp(szValueName, L"PopupColors"))
        {
            ConsoleInfo->PopupAttrib = (USHORT)Value;
            RetVal = TRUE;
        }
    }

    RegCloseKey(hKey);
    return RetVal;
}

BOOL
ConSrvWriteUserSettings(IN PCONSOLE_INFO ConsoleInfo,
                        IN DWORD ProcessId)
{
    BOOL GlobalSettings = (ConsoleInfo->ConsoleTitle[0] == L'\0');
    HKEY hKey;
    DWORD Storage = 0;

#define SetConsoleSetting(SettingName, SettingType, SettingSize, Setting, DefaultValue)         \
do {                                                                                            \
    if (GlobalSettings || (!GlobalSettings && (*(Setting) != (DefaultValue))))                  \
    {                                                                                           \
        RegSetValueExW(hKey, (SettingName), 0, (SettingType), (PBYTE)(Setting), (SettingSize)); \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        RegDeleteValue(hKey, (SettingName));                                                    \
    }                                                                                           \
} while (0)

    WCHAR szValueName[15];
    UINT i;

    if (!ConSrvOpenUserSettings(ProcessId,
                                ConsoleInfo->ConsoleTitle,
                                &hKey, KEY_WRITE,
                                TRUE))
    {
        return FALSE;
    }

    for (i = 0 ; i < sizeof(ConsoleInfo->Colors)/sizeof(ConsoleInfo->Colors[0]) ; ++i)
    {
        /*
         * Write only the new value if we are saving the global settings
         * or we are saving settings for a particular console, which differs
         * from the default ones.
         */
        swprintf(szValueName, L"ColorTable%02u", i);
        SetConsoleSetting(szValueName, REG_DWORD, sizeof(DWORD), &ConsoleInfo->Colors[i], s_Colors[i]);
    }

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

    Storage = MAKELONG(ConsoleInfo->ConsoleSize.X, ConsoleInfo->ConsoleSize.Y);
    SetConsoleSetting(L"WindowSize", REG_DWORD, sizeof(DWORD), &Storage, MAKELONG(80, 25));

    SetConsoleSetting(L"CursorSize", REG_DWORD, sizeof(DWORD), &ConsoleInfo->CursorSize, CSR_DEFAULT_CURSOR_SIZE);

    Storage = ConsoleInfo->ScreenAttrib;
    SetConsoleSetting(L"ScreenColors", REG_DWORD, sizeof(DWORD), &Storage, DEFAULT_SCREEN_ATTRIB);

    Storage = ConsoleInfo->PopupAttrib;
    SetConsoleSetting(L"PopupColors", REG_DWORD, sizeof(DWORD), &Storage, DEFAULT_POPUP_ATTRIB);

    RegCloseKey(hKey);
    return TRUE;
}

VOID
ConSrvGetDefaultSettings(IN OUT PCONSOLE_INFO ConsoleInfo,
                         IN DWORD ProcessId)
{
    if (ConsoleInfo == NULL) return;

/// HKCU,"Console","LoadConIme",0x00010003,1

    /*
     * 1. Load the default values
     */
// #define DEFAULT_HISTORY_COMMANDS_NUMBER 50
// #define DEFAULT_HISTORY_BUFFERS_NUMBER 4
    ConsoleInfo->HistoryBufferSize = 50;
    ConsoleInfo->NumberOfHistoryBuffers = 4;
    ConsoleInfo->HistoryNoDup = FALSE;

    ConsoleInfo->QuickEdit  = FALSE;
    ConsoleInfo->InsertMode = TRUE;
    // ConsoleInfo->InputBufferSize;

    // Rule: ScreenBufferSize >= ConsoleSize
    ConsoleInfo->ScreenBufferSize.X = 80;
    ConsoleInfo->ScreenBufferSize.Y = 300;
    ConsoleInfo->ConsoleSize.X = 80;
    ConsoleInfo->ConsoleSize.Y = 25;

    ConsoleInfo->CursorBlinkOn;
    ConsoleInfo->ForceCursorOff;
    ConsoleInfo->CursorSize = CSR_DEFAULT_CURSOR_SIZE; // #define SMALL_SIZE 25

    ConsoleInfo->ScreenAttrib = DEFAULT_SCREEN_ATTRIB;
    ConsoleInfo->PopupAttrib  = DEFAULT_POPUP_ATTRIB;

    memcpy(ConsoleInfo->Colors, s_Colors, sizeof(s_Colors));

    // ConsoleInfo->CodePage;

    ConsoleInfo->ConsoleTitle[0] = L'\0';

    /*
     * 2. Overwrite them with the ones stored in HKCU\Console.
     *    If the HKCU\Console key doesn't exist, create it
     *    and store the default values inside.
     */
    if (!ConSrvReadUserSettings(ConsoleInfo, ProcessId))
    {
        ConSrvWriteUserSettings(ConsoleInfo, ProcessId);
    }
}

VOID
ConSrvApplyUserSettings(IN PCONSOLE Console,
                        IN PCONSOLE_INFO ConsoleInfo)
{
    PCONSOLE_SCREEN_BUFFER ActiveBuffer = Console->ActiveBuffer;

    /*
     * Apply terminal-edition settings:
     * - QuickEdit and Insert modes,
     * - history settings.
     */
    Console->QuickEdit  = ConsoleInfo->QuickEdit;
    Console->InsertMode = ConsoleInfo->InsertMode;

    /*
     * Apply foreground and background colors for both screen and popup
     * and copy the new palette.
     */
    if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
    {
        PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)ActiveBuffer;

        Buffer->ScreenDefaultAttrib = ConsoleInfo->ScreenAttrib;
        Buffer->PopupDefaultAttrib  = ConsoleInfo->PopupAttrib;
    }
    else // if (Console->ActiveBuffer->Header.Type == GRAPHICS_BUFFER)
    {
    }

    // FIXME: Possible buffer overflow if s_colors is bigger than pConInfo->Colors.
    memcpy(Console->Colors, ConsoleInfo->Colors, sizeof(s_Colors));

    // TODO: Really update the screen attributes as FillConsoleOutputAttribute does.

    /* Apply cursor size */
    ActiveBuffer->CursorInfo.bVisible = (ConsoleInfo->CursorSize != 0);
    ActiveBuffer->CursorInfo.dwSize   = min(max(ConsoleInfo->CursorSize, 0), 100);

    if (GetType(ActiveBuffer) == TEXTMODE_BUFFER)
    {
        PTEXTMODE_SCREEN_BUFFER Buffer = (PTEXTMODE_SCREEN_BUFFER)ActiveBuffer;
        COORD BufSize;

        /* Resize its active screen-buffer */
        BufSize = ConsoleInfo->ScreenBufferSize;

        if (Console->FixedSize)
        {
            /*
             * The console is in fixed-size mode, so we cannot resize anything
             * at the moment. However, keep those settings somewhere so that
             * we can try to set them up when we will be allowed to do so.
             */
            if (ConsoleInfo->ConsoleSize.X != Buffer->OldViewSize.X ||
                ConsoleInfo->ConsoleSize.Y != Buffer->OldViewSize.Y)
            {
                Buffer->OldViewSize = ConsoleInfo->ConsoleSize;
            }

            /* Buffer size is not allowed to be smaller than the view size */
            if (BufSize.X >= Buffer->OldViewSize.X && BufSize.Y >= Buffer->OldViewSize.Y)
            {
                if (BufSize.X != Buffer->OldScreenBufferSize.X ||
                    BufSize.Y != Buffer->OldScreenBufferSize.Y)
                {
                    /*
                     * The console is in fixed-size mode, so we cannot resize anything
                     * at the moment. However, keep those settings somewhere so that
                     * we can try to set them up when we will be allowed to do so.
                     */
                    Buffer->OldScreenBufferSize = BufSize;
                }
            }
        }
        else
        {
            BOOL SizeChanged = FALSE;

            /* Resize the console */
            if (ConsoleInfo->ConsoleSize.X != Buffer->ViewSize.X ||
                ConsoleInfo->ConsoleSize.Y != Buffer->ViewSize.Y)
            {
                Buffer->ViewSize = ConsoleInfo->ConsoleSize;
                SizeChanged = TRUE;
            }

            /* Resize the screen-buffer */
            if (BufSize.X != Buffer->ScreenBufferSize.X ||
                BufSize.Y != Buffer->ScreenBufferSize.Y)
            {
                if (NT_SUCCESS(ConioResizeBuffer(Console, Buffer, BufSize)))
                    SizeChanged = TRUE;
            }

            if (SizeChanged) ConioResizeTerminal(Console);
        }
    }
    else // if (GetType(ActiveBuffer) == GRAPHICS_BUFFER)
    {
        PGRAPHICS_SCREEN_BUFFER Buffer = (PGRAPHICS_SCREEN_BUFFER)ActiveBuffer;

        /*
         * In any case we do NOT modify the size of the graphics screen-buffer.
         * We just allow resizing the view only if the new size is smaller
         * than the older one.
         */

        if (Console->FixedSize)
        {
            /*
             * The console is in fixed-size mode, so we cannot resize anything
             * at the moment. However, keep those settings somewhere so that
             * we can try to set them up when we will be allowed to do so.
             */
            if (ConsoleInfo->ConsoleSize.X <= Buffer->ViewSize.X ||
                ConsoleInfo->ConsoleSize.Y <= Buffer->ViewSize.Y)
            {
                Buffer->OldViewSize = ConsoleInfo->ConsoleSize;
            }
        }
        else
        {
            /* Resize the view if its size is bigger than the specified size */
            if (ConsoleInfo->ConsoleSize.X <= Buffer->ViewSize.X ||
                ConsoleInfo->ConsoleSize.Y <= Buffer->ViewSize.Y)
            {
                Buffer->ViewSize = ConsoleInfo->ConsoleSize;
                // SizeChanged = TRUE;
            }
        }
    }
}

/* EOF */
