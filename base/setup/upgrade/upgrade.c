/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        base/setup/upgrade/upgrade.c
 * PURPOSE:     Copy a staged upgrade into the installed system
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * This runs from <SystemRoot>\upgrade while that kernel is already in memory.
 * It copies the staged files onto the normal system root, restores the
 * FreeLoader default, and reboots. The next boot deletes the staging directory.
 */

#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>

typedef struct _UPGRADE_PROGRESS
{
    HWND Window;
    HWND Activity;
    HWND Item;
    HWND Bar;
    ULONG Total;
    ULONG Done;
} UPGRADE_PROGRESS, *PUPGRADE_PROGRESS;

static VOID
UpgradePump(
    _In_ HWND Window)
{
    MSG Message;

    if (Window == NULL)
        return;

    while (PeekMessageW(&Message, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        DispatchMessageW(&Message);
    }
    UpdateWindow(Window);
}

static VOID
UpgradeSetText(
    _In_ PUPGRADE_PROGRESS Progress,
    _In_opt_ PCWSTR Activity,
    _In_opt_ PCWSTR Item)
{
    if (Progress->Activity != NULL && Activity != NULL)
        SetWindowTextW(Progress->Activity, Activity);
    if (Progress->Item != NULL && Item != NULL)
        SetWindowTextW(Progress->Item, Item);
    UpgradePump(Progress->Window);
}

static VOID
UpgradeSetMarquee(
    _In_ PUPGRADE_PROGRESS Progress,
    _In_ BOOL Enable)
{
    LONG_PTR Style;

    if (Progress->Bar == NULL)
        return;

    Style = GetWindowLongPtrW(Progress->Bar, GWL_STYLE);
    if (Enable)
        Style |= PBS_MARQUEE;
    else
        Style &= ~PBS_MARQUEE;
    SetWindowLongPtrW(Progress->Bar, GWL_STYLE, Style);
    SendMessageW(Progress->Bar, PBM_SETMARQUEE, Enable ? TRUE : FALSE, 30);
    UpgradePump(Progress->Window);
}

static BOOL
UpgradeCreateProgress(
    _In_ HINSTANCE Instance,
    _In_ PCWSTR Title,
    _Out_ PUPGRADE_PROGRESS Progress)
{
    WNDCLASSW Class;
    RECT Bounds;
    INITCOMMONCONTROLSEX Controls;
    int X;
    int Y;

    ZeroMemory(Progress, sizeof(*Progress));
    ZeroMemory(&Class, sizeof(Class));
    if (!GetClassInfoW(Instance, L"SystemUpgradeProgress", &Class))
    {
        Class.style = CS_HREDRAW | CS_VREDRAW;
        Class.lpfnWndProc = DefWindowProcW;
        Class.hInstance = Instance;
        Class.hCursor = LoadCursor(NULL, IDC_ARROW);
        Class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        Class.lpszClassName = L"SystemUpgradeProgress";
        if (RegisterClassW(&Class) == 0)
            return FALSE;
    }

    Controls.dwSize = sizeof(Controls);
    Controls.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&Controls);

    Bounds.left = 0;
    Bounds.top = 0;
    Bounds.right = 520;
    Bounds.bottom = 118;
    AdjustWindowRectEx(&Bounds,
                       WS_POPUP | WS_CAPTION,
                       FALSE,
                       WS_EX_TOPMOST | WS_EX_DLGMODALFRAME);
    X = (GetSystemMetrics(SM_CXSCREEN) - (Bounds.right - Bounds.left)) / 2;
    Y = (GetSystemMetrics(SM_CYSCREEN) - (Bounds.bottom - Bounds.top)) / 2;
    Progress->Window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
                                       L"SystemUpgradeProgress",
                                       Title,
                                       WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_CLIPCHILDREN,
                                       X,
                                       Y,
                                       Bounds.right - Bounds.left,
                                       Bounds.bottom - Bounds.top,
                                       NULL,
                                       NULL,
                                       Instance,
                                       NULL);
    if (Progress->Window == NULL)
        return FALSE;

    Progress->Activity = CreateWindowExW(0,
                                         L"STATIC",
                                         L"",
                                         WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                                         16, 14, 488, 18,
                                         Progress->Window,
                                         NULL,
                                         Instance,
                                         NULL);
    Progress->Item = CreateWindowExW(0,
                                     L"STATIC",
                                     L"",
                                     WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_PATHELLIPSIS,
                                     16, 40, 488, 18,
                                     Progress->Window,
                                     NULL,
                                     Instance,
                                     NULL);
    Progress->Bar = CreateWindowExW(0,
                                    PROGRESS_CLASSW,
                                    NULL,
                                    WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                    16, 72, 488, 18,
                                    Progress->Window,
                                    NULL,
                                    Instance,
                                    NULL);
    ShowWindow(Progress->Window, SW_SHOW);
    UpdateWindow(Progress->Window);
    UpgradePump(Progress->Window);
    return TRUE;
}

static VOID
UpgradeCloseProgress(
    _Inout_ PUPGRADE_PROGRESS Progress)
{
    if (Progress->Window != NULL)
        DestroyWindow(Progress->Window);
    Progress->Window = NULL;
    Progress->Activity = NULL;
    Progress->Item = NULL;
    Progress->Bar = NULL;
}

static BOOL
EndsWithUpgrade(
    _In_ PCWSTR Path)
{
    size_t Length = wcslen(Path);
    if (Length < wcslen(L"\\upgrade"))
        return FALSE;
    return _wcsicmp(Path + Length - wcslen(L"\\upgrade"), L"\\upgrade") == 0;
}

static BOOL
IsConfigPath(
    _In_ PCWSTR Relative)
{
    return _wcsnicmp(Relative, L"System32\\config", 15) == 0 &&
           (Relative[15] == UNICODE_NULL || Relative[15] == L'\\');
}

static VOID
UpgradeShowFile(
    _In_ PUPGRADE_PROGRESS Progress,
    _In_ PCWSTR Activity,
    _In_ PCWSTR Path)
{
    WCHAR Caption[80];

    if (Progress->Window == NULL)
        return;

    Progress->Done++;
    StringCchPrintfW(Caption,
                     ARRAYSIZE(Caption),
                     L"%s (%lu of %lu)",
                     Activity,
                     Progress->Done,
                     Progress->Total);
    UpgradeSetText(Progress, Caption, Path);
    if (Progress->Bar != NULL)
        SendMessageW(Progress->Bar, PBM_SETPOS, Progress->Done, 0);
}

static ULONG
CountTree(
    _In_ PCWSTR Source,
    _In_ PCWSTR Relative,
    _In_ PUPGRADE_PROGRESS Progress)
{
    WCHAR Pattern[MAX_PATH];
    WCHAR Child[MAX_PATH];
    WCHAR NextRelative[MAX_PATH];
    WIN32_FIND_DATAW Data;
    HANDLE Find;
    ULONG Count = 0;

    if (FAILED(StringCchPrintfW(Pattern, ARRAYSIZE(Pattern), L"%s\\*", Source)))
        return 0;

    Find = FindFirstFileW(Pattern, &Data);
    if (Find == INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        if (wcscmp(Data.cFileName, L".") == 0 || wcscmp(Data.cFileName, L"..") == 0)
            continue;

        if (Relative[0] == UNICODE_NULL)
        {
            if (FAILED(StringCchCopyW(NextRelative, ARRAYSIZE(NextRelative), Data.cFileName)))
                break;
        }
        else if (FAILED(StringCchPrintfW(NextRelative,
                                         ARRAYSIZE(NextRelative),
                                         L"%s\\%s",
                                         Relative,
                                         Data.cFileName)))
        {
            break;
        }

        if (IsConfigPath(NextRelative))
            continue;

        if (FAILED(StringCchPrintfW(Child, ARRAYSIZE(Child), L"%s\\%s", Source, Data.cFileName)))
            break;

        if (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            Count += CountTree(Child, NextRelative, Progress);
        }
        else
        {
            Count++;
            UpgradeSetText(Progress, L"Counting files...", Child);
        }
    } while (FindNextFileW(Find, &Data));

    FindClose(Find);
    return Count;
}

/*
 * kernel32 CopyFileW opens the source with FILE_FLAG_NO_BUFFERING
 * (dll/win32/kernel32/client/file/copy.c). A non-buffered transfer has
 * to be a multiple of the sector size (drivers/filesystems/ntfs/rw.c).
 * Staged files are not all sector-sized, so copy through the cache.
 */
static BOOL
UpgradeCopyFile(
    _In_ PCWSTR Source,
    _In_ PCWSTR Dest)
{
    HANDLE Input;
    HANDLE Output;
    BYTE Buffer[0x10000];
    DWORD Read;
    DWORD Written;

    SetFileAttributesW(Dest, FILE_ATTRIBUTE_NORMAL);
    Input = CreateFileW(Source,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (Input == INVALID_HANDLE_VALUE)
        return FALSE;

    Output = CreateFileW(Dest,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (Output == INVALID_HANDLE_VALUE)
    {
        CloseHandle(Input);
        return FALSE;
    }

    for (;;)
    {
        if (!ReadFile(Input, Buffer, sizeof(Buffer), &Read, NULL))
        {
            CloseHandle(Output);
            CloseHandle(Input);
            DeleteFileW(Dest);
            return FALSE;
        }
        if (Read == 0)
            break;
        if (!WriteFile(Output, Buffer, Read, &Written, NULL) || Written != Read)
        {
            CloseHandle(Output);
            CloseHandle(Input);
            DeleteFileW(Dest);
            return FALSE;
        }
    }

    CloseHandle(Output);
    CloseHandle(Input);
    return TRUE;
}

static BOOL
CopyTree(
    _In_ PCWSTR Source,
    _In_ PCWSTR Dest,
    _In_ PCWSTR Relative,
    _In_ PUPGRADE_PROGRESS Progress)
{
    WCHAR Pattern[MAX_PATH];
    WCHAR SourceChild[MAX_PATH];
    WCHAR DestChild[MAX_PATH];
    WCHAR NextRelative[MAX_PATH];
    WIN32_FIND_DATAW Data;
    HANDLE Find;
    BOOL Ok = TRUE;

    CreateDirectoryW(Dest, NULL);
    if (FAILED(StringCchPrintfW(Pattern, ARRAYSIZE(Pattern), L"%s\\*", Source)))
        return FALSE;

    Find = FindFirstFileW(Pattern, &Data);
    if (Find == INVALID_HANDLE_VALUE)
        return TRUE;

    do
    {
        if (wcscmp(Data.cFileName, L".") == 0 || wcscmp(Data.cFileName, L"..") == 0)
            continue;

        if (Relative[0] == UNICODE_NULL)
        {
            if (FAILED(StringCchCopyW(NextRelative, ARRAYSIZE(NextRelative), Data.cFileName)))
            {
                Ok = FALSE;
                break;
            }
        }
        else if (FAILED(StringCchPrintfW(NextRelative,
                                         ARRAYSIZE(NextRelative),
                                         L"%s\\%s",
                                         Relative,
                                         Data.cFileName)))
        {
            Ok = FALSE;
            break;
        }

        if (IsConfigPath(NextRelative))
            continue;

        if (FAILED(StringCchPrintfW(SourceChild, ARRAYSIZE(SourceChild), L"%s\\%s", Source, Data.cFileName)) ||
            FAILED(StringCchPrintfW(DestChild, ARRAYSIZE(DestChild), L"%s\\%s", Dest, Data.cFileName)))
        {
            Ok = FALSE;
            break;
        }

        if (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!CopyTree(SourceChild, DestChild, NextRelative, Progress))
            {
                Ok = FALSE;
                break;
            }
        }
        else
        {
            UpgradeShowFile(Progress, L"Copying files", DestChild);
            if (!UpgradeCopyFile(SourceChild, DestChild))
            {
                Ok = FALSE;
                break;
            }
        }
    } while (FindNextFileW(Find, &Data));

    FindClose(Find);
    return Ok;
}

static BOOL
ReplaceAssignment(
    _Inout_ PSTR* Contents,
    _In_ PCSTR Key,
    _In_ PCSTR Value)
{
    PSTR Start;
    PSTR End;
    PSTR Rewritten;
    size_t KeyLength = strlen(Key);
    size_t Prefix;
    size_t Tail;
    size_t ValueLength = strlen(Value);

    for (Start = *Contents; *Start != '\0'; )
    {
        if (_strnicmp(Start, Key, KeyLength) == 0 && Start[KeyLength] == '=')
            break;
        Start = strchr(Start, '\n');
        if (Start == NULL)
            return FALSE;
        Start++;
    }

    End = strchr(Start, '\n');
    if (End == NULL)
        End = Start + strlen(Start);
    else
        End++;

    Prefix = (size_t)(Start - *Contents);
    Tail = strlen(End);
    Rewritten = HeapAlloc(GetProcessHeap(), 0, Prefix + KeyLength + 1 + ValueLength + 2 + Tail + 1);
    if (Rewritten == NULL)
        return FALSE;

    memcpy(Rewritten, *Contents, Prefix);
    memcpy(Rewritten + Prefix, Key, KeyLength);
    Rewritten[Prefix + KeyLength] = '=';
    memcpy(Rewritten + Prefix + KeyLength + 1, Value, ValueLength);
    if (End > Start && End[-1] == '\n')
    {
        Rewritten[Prefix + KeyLength + 1 + ValueLength] = '\r';
        Rewritten[Prefix + KeyLength + 1 + ValueLength + 1] = '\n';
        memcpy(Rewritten + Prefix + KeyLength + 1 + ValueLength + 2, End, Tail + 1);
    }
    else
    {
        memcpy(Rewritten + Prefix + KeyLength + 1 + ValueLength, End, Tail + 1);
    }

    HeapFree(GetProcessHeap(), 0, *Contents);
    *Contents = Rewritten;
    return TRUE;
}

static BOOL
RestoreLoader(
    _In_ PCWSTR IniPath,
    _In_ PCSTR DefaultOs,
    _In_ PCSTR Timeout)
{
    HANDLE File;
    DWORD Size;
    DWORD Read;
    DWORD Written;
    PSTR Contents;
    BOOL Ok;

    File = CreateFileW(IniPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File == INVALID_HANDLE_VALUE)
        return FALSE;

    Size = GetFileSize(File, NULL);
    Contents = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)Size + 1);
    if (Contents == NULL)
    {
        CloseHandle(File);
        return FALSE;
    }
    Ok = ReadFile(File, Contents, Size, &Read, NULL) && Read == Size;
    CloseHandle(File);
    if (!Ok)
    {
        HeapFree(GetProcessHeap(), 0, Contents);
        return FALSE;
    }

    Ok = ReplaceAssignment(&Contents, "DefaultOS", DefaultOs) &&
         ReplaceAssignment(&Contents, "TimeOut", Timeout);
    if (!Ok)
    {
        HeapFree(GetProcessHeap(), 0, Contents);
        return FALSE;
    }

    File = CreateFileW(IniPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File == INVALID_HANDLE_VALUE)
    {
        HeapFree(GetProcessHeap(), 0, Contents);
        return FALSE;
    }
    Size = (DWORD)strlen(Contents);
    Ok = WriteFile(File, Contents, Size, &Written, NULL) && Written == Size;
    CloseHandle(File);
    HeapFree(GetProcessHeap(), 0, Contents);
    return Ok;
}

static VOID
RemoveTree(
    _In_ PCWSTR Directory)
{
    WCHAR Pattern[MAX_PATH];
    WCHAR Child[MAX_PATH];
    WIN32_FIND_DATAW Data;
    HANDLE Find;

    if (FAILED(StringCchPrintfW(Pattern, ARRAYSIZE(Pattern), L"%s\\*", Directory)))
        return;

    Find = FindFirstFileW(Pattern, &Data);
    if (Find == INVALID_HANDLE_VALUE)
    {
        RemoveDirectoryW(Directory);
        return;
    }

    do
    {
        if (wcscmp(Data.cFileName, L".") == 0 || wcscmp(Data.cFileName, L"..") == 0)
            continue;
        if (FAILED(StringCchPrintfW(Child, ARRAYSIZE(Child), L"%s\\%s", Directory, Data.cFileName)))
            continue;
        SetFileAttributesW(Child, FILE_ATTRIBUTE_NORMAL);
        if (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            RemoveTree(Child);
        else
            DeleteFileW(Child);
    } while (FindNextFileW(Find, &Data));

    FindClose(Find);
    SetFileAttributesW(Directory, FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryW(Directory);
}

static VOID
RemoveStoreFile(
    _In_ PCWSTR Directory,
    _In_ PCWSTR Name)
{
    WCHAR Path[MAX_PATH];

    if (FAILED(StringCchPrintfW(Path, ARRAYSIZE(Path), L"%s\\%s", Directory, Name)))
        return;
    SetFileAttributesW(Path, FILE_ATTRIBUTE_NORMAL);
    DeleteFileW(Path);
}

static VOID
ResetPackageDatabase(
    _In_ PCWSTR InstallDir)
{
    WCHAR Exe[MAX_PATH];
    WCHAR State[MAX_PATH];
    WCHAR Scan[MAX_PATH];
    WCHAR Command[] = L"verosupd.exe /resetdb";
    STARTUPINFOW Start;
    PROCESS_INFORMATION Process;
    DWORD ExitCode = 1;
    BOOL Started = FALSE;

    if (FAILED(StringCchPrintfW(Exe, ARRAYSIZE(Exe), L"%s\\System32\\verosupd.exe", InstallDir)) ||
        FAILED(StringCchPrintfW(State, ARRAYSIZE(State),
                                L"%s\\System32\\config\\sysupdate", InstallDir)) ||
        FAILED(StringCchPrintfW(Scan, ARRAYSIZE(Scan), L"%s\\System32", InstallDir)))
    {
        return;
    }

    if (GetFileAttributesW(Exe) != INVALID_FILE_ATTRIBUTES)
    {
        SetEnvironmentVariableW(L"SYSUPDATE_STATE", State);
        SetEnvironmentVariableW(L"SYSUPDATE_SCAN", Scan);
        SetEnvironmentVariableW(L"SYSUPDATE_ROOT", InstallDir);
        ZeroMemory(&Start, sizeof(Start));
        Start.cb = sizeof(Start);
        ZeroMemory(&Process, sizeof(Process));
        Started = CreateProcessW(Exe,
                                 Command,
                                 NULL,
                                 NULL,
                                 FALSE,
                                 0,
                                 NULL,
                                 InstallDir,
                                 &Start,
                                 &Process);
        if (Started)
        {
            WaitForSingleObject(Process.hProcess, INFINITE);
            GetExitCodeProcess(Process.hProcess, &ExitCode);
            CloseHandle(Process.hThread);
            CloseHandle(Process.hProcess);
        }
    }

    if (Started && ExitCode == 0)
        return;

    RemoveStoreFile(State, L"pkgdb.db");
    RemoveStoreFile(State, L"pkgdb.db-journal");
    RemoveStoreFile(State, L"pkgdb.db-wal");
    RemoveStoreFile(State, L"pkgdb.db-shm");
    {
        WCHAR Chunks[MAX_PATH];
        WCHAR Store[MAX_PATH];
        if (SUCCEEDED(StringCchPrintfW(Chunks, ARRAYSIZE(Chunks), L"%s\\chunks", State)))
            RemoveTree(Chunks);
        if (SUCCEEDED(StringCchPrintfW(Store, ARRAYSIZE(Store), L"%s\\RollbackStore", State)))
            RemoveTree(Store);
    }
}

static BOOL
EnableShutdown(VOID)
{
    HANDLE Token;
    TOKEN_PRIVILEGES Privileges;
    LUID Luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token))
        return FALSE;
    if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &Luid))
    {
        CloseHandle(Token);
        return FALSE;
    }
    Privileges.PrivilegeCount = 1;
    Privileges.Privileges[0].Luid = Luid;
    Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(Token, FALSE, &Privileges, 0, NULL, NULL);
    CloseHandle(Token);
    return GetLastError() == ERROR_SUCCESS;
}

static ULONG
CountDeleteTree(
    _In_ PCWSTR Directory,
    _In_ PUPGRADE_PROGRESS Progress)
{
    WCHAR Pattern[MAX_PATH];
    WCHAR Child[MAX_PATH];
    WIN32_FIND_DATAW Data;
    HANDLE Find;
    ULONG Count = 0;

    if (FAILED(StringCchPrintfW(Pattern, ARRAYSIZE(Pattern), L"%s\\*", Directory)))
        return 0;

    Find = FindFirstFileW(Pattern, &Data);
    if (Find == INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        if (wcscmp(Data.cFileName, L".") == 0 || wcscmp(Data.cFileName, L"..") == 0)
            continue;
        if (FAILED(StringCchPrintfW(Child, ARRAYSIZE(Child), L"%s\\%s", Directory, Data.cFileName)))
            continue;

        if (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            Count += CountDeleteTree(Child, Progress);
        else
        {
            Count++;
            UpgradeSetText(Progress, L"Counting staged files...", Child);
        }
    } while (FindNextFileW(Find, &Data));

    FindClose(Find);
    return Count;
}

static VOID
DeleteTreePass(
    _In_ PCWSTR Directory,
    _In_ PUPGRADE_PROGRESS Progress)
{
    WCHAR Pattern[MAX_PATH];
    WCHAR Child[MAX_PATH];
    WIN32_FIND_DATAW Data;
    HANDLE Find;

    if (FAILED(StringCchPrintfW(Pattern, ARRAYSIZE(Pattern), L"%s\\*", Directory)))
        return;

    Find = FindFirstFileW(Pattern, &Data);
    if (Find == INVALID_HANDLE_VALUE)
    {
        RemoveDirectoryW(Directory);
        return;
    }

    do
    {
        if (wcscmp(Data.cFileName, L".") == 0 || wcscmp(Data.cFileName, L"..") == 0)
            continue;
        if (FAILED(StringCchPrintfW(Child, ARRAYSIZE(Child), L"%s\\%s", Directory, Data.cFileName)))
            continue;

        SetFileAttributesW(Child, FILE_ATTRIBUTE_NORMAL);
        if (Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            DeleteTreePass(Child, Progress);
        }
        else
        {
            UpgradeShowFile(Progress, L"Removing staged files", Child);
            DeleteFileW(Child);
        }
    } while (FindNextFileW(Find, &Data));

    FindClose(Find);
    SetFileAttributesW(Directory, FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryW(Directory);
}

static int
DeleteStagedUpgrade(
    _In_ HINSTANCE Instance)
{
    WCHAR WindowsDir[MAX_PATH];
    WCHAR StageDir[MAX_PATH];
    UPGRADE_PROGRESS Progress;
    ULONG Pass;

    if (GetWindowsDirectoryW(WindowsDir, ARRAYSIZE(WindowsDir)) == 0 ||
        FAILED(StringCchPrintfW(StageDir, ARRAYSIZE(StageDir), L"%s\\upgrade", WindowsDir)))
    {
        return 1;
    }

    UpgradeCreateProgress(Instance, L"Removing the staged upgrade", &Progress);
    for (Pass = 0; Pass < 30; Pass++)
    {
        if (GetFileAttributesW(StageDir) == INVALID_FILE_ATTRIBUTES)
        {
            UpgradeSetText(&Progress, L"The staged files have been removed.", L"");
            UpgradeCloseProgress(&Progress);
            return 0;
        }

        UpgradeSetMarquee(&Progress, TRUE);
        Progress.Total = CountDeleteTree(StageDir, &Progress);
        if (Progress.Total == 0)
            Progress.Total = 1;
        Progress.Done = 0;
        UpgradeSetMarquee(&Progress, FALSE);
        SendMessageW(Progress.Bar, PBM_SETRANGE32, 0, Progress.Total);
        SendMessageW(Progress.Bar, PBM_SETPOS, 0, 0);
        DeleteTreePass(StageDir, &Progress);
    }

    UpgradeCloseProgress(&Progress);
    return 0;
}

int WINAPI
wWinMain(
    HINSTANCE Instance,
    HINSTANCE Previous,
    LPWSTR CommandLine,
    int Show)
{
    WCHAR WindowsDir[MAX_PATH];
    WCHAR InstallDir[MAX_PATH];
    WCHAR Volume[MAX_PATH];
    WCHAR IniPath[MAX_PATH];
    WCHAR NotePath[MAX_PATH];
    UPGRADE_PROGRESS Progress;
    CHAR Note[160];
    CHAR DefaultOs[128];
    CHAR Timeout[32];
    HANDLE NoteFile;
    DWORD Read;
    PSTR Split;
    PWSTR Slash;

    UNREFERENCED_PARAMETER(Previous);
    UNREFERENCED_PARAMETER(Show);

    if (CommandLine != NULL && wcsstr(CommandLine, L"/delete") != NULL)
        return DeleteStagedUpgrade(Instance);

    if (GetWindowsDirectoryW(WindowsDir, ARRAYSIZE(WindowsDir)) == 0 || !EndsWithUpgrade(WindowsDir))
        return 0;

    if (FAILED(StringCchCopyW(InstallDir, ARRAYSIZE(InstallDir), WindowsDir)))
        return 1;
    InstallDir[wcslen(InstallDir) - wcslen(L"\\upgrade")] = UNICODE_NULL;

    if (FAILED(StringCchCopyW(Volume, ARRAYSIZE(Volume), InstallDir)))
        return 1;
    Slash = wcsrchr(Volume, L'\\');
    if (Slash == NULL)
        return 1;
    Slash[1] = UNICODE_NULL;

    if (FAILED(StringCchPrintfW(NotePath, ARRAYSIZE(NotePath), L"%s\\commit.default", WindowsDir)) ||
        FAILED(StringCchPrintfW(IniPath, ARRAYSIZE(IniPath), L"%sfreeldr.ini", Volume)))
    {
        return 1;
    }

    NoteFile = CreateFileW(NotePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (NoteFile == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(NULL, L"The upgrade did not record the previous boot entry.", L"VerOS NT", MB_ICONERROR);
        return 1;
    }
    ZeroMemory(Note, sizeof(Note));
    if (!ReadFile(NoteFile, Note, sizeof(Note) - 1, &Read, NULL))
    {
        CloseHandle(NoteFile);
        return 1;
    }
    CloseHandle(NoteFile);

    Split = strchr(Note, '\n');
    if (Split == NULL)
        return 1;
    *Split = '\0';
    if (Split > Note && Split[-1] == '\r')
        Split[-1] = '\0';
    StringCchCopyA(DefaultOs, ARRAYSIZE(DefaultOs), Note);
    Split++;
    StringCchCopyA(Timeout, ARRAYSIZE(Timeout), Split);
    Split = Timeout;
    while (*Split != '\0' && *Split != '\r' && *Split != '\n')
        Split++;
    *Split = '\0';

    UpgradeCreateProgress(Instance, L"Installing the upgrade", &Progress);
    UpgradeSetMarquee(&Progress, TRUE);
    Progress.Total = CountTree(WindowsDir, L"", &Progress);
    if (Progress.Total == 0)
        Progress.Total = 1;
    Progress.Done = 0;
    UpgradeSetMarquee(&Progress, FALSE);
    SendMessageW(Progress.Bar, PBM_SETRANGE32, 0, Progress.Total);
    SendMessageW(Progress.Bar, PBM_SETPOS, 0, 0);

    if (!CopyTree(WindowsDir, InstallDir, L"", &Progress))
    {
        UpgradeCloseProgress(&Progress);
        MessageBoxW(NULL, L"The staged files could not be copied into place.", L"VerOS NT", MB_ICONERROR);
        return 1;
    }

    UpgradeSetText(&Progress, L"Resetting the package database...", L"");
    ResetPackageDatabase(InstallDir);

    UpgradeSetText(&Progress, L"Restoring the boot menu...", IniPath);
    if (!RestoreLoader(IniPath, DefaultOs, Timeout))
    {
        UpgradeCloseProgress(&Progress);
        MessageBoxW(NULL, L"The boot menu could not be restored.", L"VerOS NT", MB_ICONERROR);
        return 1;
    }

    UpgradeSetText(&Progress, L"Restarting...", L"");
    UpgradeCloseProgress(&Progress);

    EnableShutdown();
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
    return 0;
}
