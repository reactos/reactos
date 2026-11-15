/*
 * PROJECT:     VerOS NT
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        base/setup/reactos/stage.c
 * PURPOSE:     Stage an in-place upgrade from the welcome menu
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * The CD kernel is already in memory. This copies the new files into
 * <SystemRoot>\upgrade, points the next boot at that tree, and reboots.
 * The staged boot commits the files. The boot after that deletes the directory.
 */

#include "reactos.h"

#include <stdio.h>
#include <winnls.h>
#include <ndk/obfuncs.h>

#define NDEBUG
#include <debug.h>

BOOL
EnablePrivilege(
    IN LPCWSTR lpszPrivilegeName,
    IN BOOL bEnablePrivilege);

typedef struct _STAGE_PROGRESS
{
    HWND Window;
    HWND Activity;
    HWND Item;
    HWND Bar;
    ULONG Total;
    ULONG Done;
} STAGE_PROGRESS, *PSTAGE_PROGRESS;

static VOID
StagePump(
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
StageSetText(
    _In_ PSTAGE_PROGRESS Progress,
    _In_opt_ PCWSTR Activity,
    _In_opt_ PCWSTR Item)
{
    if (Progress->Activity != NULL && Activity != NULL)
        SetWindowTextW(Progress->Activity, Activity);
    if (Progress->Item != NULL && Item != NULL)
        SetWindowTextW(Progress->Item, Item);
    StagePump(Progress->Window);
}

static BOOL
StageCreateProgress(
    _In_ HINSTANCE Instance,
    _Out_ PSTAGE_PROGRESS Progress)
{
    WNDCLASSW Class;
    RECT Bounds;
    INITCOMMONCONTROLSEX Controls;
    int X;
    int Y;

    ZeroMemory(Progress, sizeof(*Progress));
    ZeroMemory(&Class, sizeof(Class));
    if (!GetClassInfoW(Instance, L"VerosUpgradeProgress", &Class))
    {
        Class.style = CS_HREDRAW | CS_VREDRAW;
        Class.lpfnWndProc = DefWindowProcW;
        Class.hInstance = Instance;
        Class.hCursor = LoadCursor(NULL, IDC_ARROW);
        Class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        Class.lpszClassName = L"VerosUpgradeProgress";
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
                                       L"VerosUpgradeProgress",
                                       L"Staging the upgrade",
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
                                         L"Copying files...",
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
    StagePump(Progress->Window);
    return TRUE;
}

static VOID
StageCloseProgress(
    _Inout_ PSTAGE_PROGRESS Progress)
{
    if (Progress->Window != NULL)
        DestroyWindow(Progress->Window);
    Progress->Window = NULL;
    Progress->Activity = NULL;
    Progress->Item = NULL;
    Progress->Bar = NULL;
}

static UINT CALLBACK
StageFileCallback(
    PVOID Context,
    UINT Notification,
    UINT_PTR Param1,
    UINT_PTR Param2)
{
    PSTAGE_PROGRESS Progress = (PSTAGE_PROGRESS)Context;
    PFILEPATHS_W FilePathInfo;
    WCHAR Caption[80];

    if (Notification == SPFILENOTIFY_COPYERROR)
        return FILEOP_SKIP;

    if (Progress == NULL || Progress->Window == NULL)
        return FILEOP_DOIT;

    if (Notification == SPFILENOTIFY_STARTSUBQUEUE && Param1 == FILEOP_COPY)
    {
        Progress->Total = (ULONG)Param2;
        Progress->Done = 0;
        if (Progress->Total == 0)
            Progress->Total = 1;
        SendMessageW(Progress->Bar, PBM_SETRANGE32, 0, Progress->Total);
        SendMessageW(Progress->Bar, PBM_SETPOS, 0, 0);
        StageSetText(Progress, L"Copying files...", L"");
    }
    else if (Notification == SPFILENOTIFY_STARTCOPY ||
             Notification == SPFILENOTIFY_STARTDELETE ||
             Notification == SPFILENOTIFY_STARTRENAME)
    {
        FilePathInfo = (PFILEPATHS_W)Param1;
        StringCchPrintfW(Caption,
                         ARRAYSIZE(Caption),
                         L"Copying files (%lu of %lu)",
                         Progress->Done + 1,
                         Progress->Total);
        StageSetText(Progress, Caption, FilePathInfo->Target);
    }
    else if (Notification == SPFILENOTIFY_ENDCOPY ||
             Notification == SPFILENOTIFY_ENDDELETE ||
             Notification == SPFILENOTIFY_ENDRENAME)
    {
        if (Progress->Done < Progress->Total)
            Progress->Done++;
        SendMessageW(Progress->Bar, PBM_SETPOS, Progress->Done, 0);
        StagePump(Progress->Window);
    }

    return FILEOP_DOIT;
}

static BOOL
EnableRestorePrivilege(VOID)
{
    return EnablePrivilege(SE_BACKUP_NAME, TRUE) &&
           EnablePrivilege(SE_RESTORE_NAME, TRUE);
}

/* A normal unload leaves the hive loaded when a subkey is still referenced,
 * and the write never reaches the disk. Forcing the unload flushes it. */
#define STAGE_FORCE_UNLOAD 1

NTSYSAPI
NTSTATUS
NTAPI
NtUnloadKey2(
    _In_ POBJECT_ATTRIBUTES TargetKey,
    _In_ ULONG Flags);

static BOOL
UnloadLoadedHive(
    _In_ PCWSTR KeyName)
{
    UNICODE_STRING KeyPath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR Path[96];
    NTSTATUS Status;
    HKEY Root;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyName, 0, KEY_READ, &Root) == ERROR_SUCCESS)
    {
        RegFlushKey(Root);
        RegCloseKey(Root);
    }

    if (FAILED(StringCchPrintfW(Path, ARRAYSIZE(Path), L"\\Registry\\Machine\\%s", KeyName)))
        return FALSE;

    RtlInitUnicodeString(&KeyPath, Path);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtUnloadKey2(&ObjectAttributes, STAGE_FORCE_UNLOAD);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtUnloadKey2('%S') failed: 0x%08lx\n", KeyName, Status);
        return FALSE;
    }
    return TRUE;
}

static BOOL
SetHiveRunOnce(
    _In_ PCWSTR KeyName,
    _In_ PCWSTR HivePath,
    _In_ PCWSTR ValueName,
    _In_ PCWSTR Command)
{
    WCHAR RunPath[160];
    HKEY RunKey;
    LONG Error;

    if (!EnableRestorePrivilege())
        return FALSE;

    Error = RegLoadKeyW(HKEY_LOCAL_MACHINE, KeyName, HivePath);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegLoadKeyW('%S') failed: %ld\n", HivePath, Error);
        return FALSE;
    }

    if (FAILED(StringCchPrintfW(RunPath,
                                ARRAYSIZE(RunPath),
                                L"%s\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                                KeyName)))
    {
        UnloadLoadedHive(KeyName);
        return FALSE;
    }

    Error = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                            RunPath,
                            0,
                            NULL,
                            0,
                            KEY_SET_VALUE,
                            NULL,
                            &RunKey,
                            NULL);
    if (Error == ERROR_SUCCESS)
    {
        Error = RegSetValueExW(RunKey,
                               ValueName,
                               0,
                               REG_SZ,
                               (const BYTE*)Command,
                               (DWORD)((wcslen(Command) + 1) * sizeof(WCHAR)));
        RegFlushKey(RunKey);
        RegCloseKey(RunKey);
    }

    if (!UnloadLoadedHive(KeyName))
        return FALSE;
    return Error == ERROR_SUCCESS;
}

/*
 * kernel32 CopyFileW opens the source with FILE_FLAG_NO_BUFFERING
 * (dll/win32/kernel32/client/file/copy.c). A non-buffered transfer has
 * to be a multiple of the sector size (drivers/filesystems/ntfs/rw.c).
 * The guest stopped in this step, so copy through the cache.
 */
static BOOL
StageCopyFile(
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
CopyHiveFile(
    _In_ PCWSTR SourceDir,
    _In_ PCWSTR DestDir,
    _In_ PCWSTR Name)
{
    WCHAR Source[MAX_PATH];
    WCHAR Dest[MAX_PATH];

    if (FAILED(StringCchPrintfW(Source, ARRAYSIZE(Source), L"%s\\%s", SourceDir, Name)) ||
        FAILED(StringCchPrintfW(Dest, ARRAYSIZE(Dest), L"%s\\%s", DestDir, Name)))
    {
        return FALSE;
    }

    if (GetFileAttributesW(Source) == INVALID_FILE_ATTRIBUTES)
        return TRUE;

    return StageCopyFile(Source, Dest);
}

static BOOL
CopyExistingHives(
    _In_ PCWSTR SystemRoot,
    _In_ PCWSTR StageRoot,
    _In_ PSTAGE_PROGRESS Progress)
{
    static const PCWSTR Names[] =
    {
        L"SYSTEM", L"SYSTEM.LOG", L"SYSTEM.LOG1", L"SYSTEM.LOG2",
        L"SOFTWARE", L"SOFTWARE.LOG", L"SOFTWARE.LOG1", L"SOFTWARE.LOG2",
        L"SAM", L"SAM.LOG", L"SAM.LOG1", L"SAM.LOG2",
        L"SECURITY", L"SECURITY.LOG", L"SECURITY.LOG1", L"SECURITY.LOG2",
        L"DEFAULT", L"DEFAULT.LOG", L"DEFAULT.LOG1", L"DEFAULT.LOG2"
    };
    WCHAR SourceDir[MAX_PATH];
    WCHAR DestDir[MAX_PATH];
    ULONG Index;

    if (FAILED(StringCchPrintfW(SourceDir, ARRAYSIZE(SourceDir), L"%s\\System32\\config", SystemRoot)) ||
        FAILED(StringCchPrintfW(DestDir, ARRAYSIZE(DestDir), L"%s\\System32\\config", StageRoot)))
    {
        return FALSE;
    }

    CreateDirectoryW(StageRoot, NULL);
    CreateDirectoryW(StageRoot, NULL);
    {
        WCHAR System32[MAX_PATH];
        StringCchPrintfW(System32, ARRAYSIZE(System32), L"%s\\System32", StageRoot);
        CreateDirectoryW(System32, NULL);
    }
    CreateDirectoryW(DestDir, NULL);

    StageSetText(Progress, L"Copying registry hives...", L"");
    for (Index = 0; Index < ARRAYSIZE(Names); Index++)
    {
        WCHAR Shown[MAX_PATH];

        StringCchPrintfW(Shown, ARRAYSIZE(Shown), L"%s\\%s", DestDir, Names[Index]);
        StageSetText(Progress, L"Copying registry hives...", Shown);
        if (!CopyHiveFile(SourceDir, DestDir, Names[Index]))
        {
            DPRINT1("Copy of hive '%S' failed: %lu\n", Names[Index], GetLastError());
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL
CopyCommitProgram(
    _In_ PCWSTR StageRoot,
    _In_ PSTAGE_PROGRESS Progress)
{
    WCHAR Source[MAX_PATH];
    WCHAR Dest[MAX_PATH];
    PWSTR Slash;

    if (!GetModuleFileNameW(NULL, Source, ARRAYSIZE(Source)))
        return FALSE;

    Slash = wcsrchr(Source, L'\\');
    if (Slash == NULL)
        return FALSE;
    if (FAILED(StringCchCopyW(Slash + 1, ARRAYSIZE(Source) - (Slash + 1 - Source), L"upgrade.exe")))
        return FALSE;

    if (FAILED(StringCchPrintfW(Dest, ARRAYSIZE(Dest), L"%s\\System32\\upgrade.exe", StageRoot)))
        return FALSE;

    StageSetText(Progress, L"Copying the commit program...", Dest);
    return StageCopyFile(Source, Dest);
}

static BOOL
WriteCommitFile(
    _In_ PCWSTR StageRoot,
    _In_ PCWSTR PreviousDefault,
    _In_ ULONG PreviousTimeout)
{
    WCHAR Path[MAX_PATH];
    CHAR Text[160];
    HANDLE File;
    DWORD Written;
    int Length;

    if (FAILED(StringCchPrintfW(Path, ARRAYSIZE(Path), L"%s\\commit.default", StageRoot)))
        return FALSE;

    if (WideCharToMultiByte(CP_ACP, 0, PreviousDefault, -1, Text, 128, NULL, NULL) == 0)
        return FALSE;
    Length = sprintf(Text + strlen(Text), "\r\n%lu\r\n", PreviousTimeout);
    Length += (int)strlen(Text) - Length;
    if (Length <= 0)
        return FALSE;

    File = CreateFileW(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!WriteFile(File, Text, (DWORD)Length, &Written, NULL) || Written != (DWORD)Length)
    {
        CloseHandle(File);
        return FALSE;
    }
    CloseHandle(File);
    return TRUE;
}

static BOOL
AddOneShotBootEntry(
    _Inout_ PUSETUP_DATA Setup,
    _Out_writes_z_(cchPrevious) PWSTR PreviousDefault,
    _In_ ULONG cchPrevious,
    _Out_ PULONG PreviousTimeout)
{
    UCHAR EntryBuffer[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)EntryBuffer;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    BOOT_STORE_OPTIONS BootOptions;
    PVOID Store;
    NTSTATUS Status;

    RtlZeroMemory(EntryBuffer, sizeof(EntryBuffer));
    BootEntry->Version = FreeLdr;
    BootEntry->BootFilePath = NULL;
    BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
    RtlCopyMemory(Options->Signature,
                  NTOS_OPTIONS_SIGNATURE,
                  RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));
    Options->OsLoadPath = Setup->DestinationArcPath.Buffer;
    Options->OsLoadOptions = L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /FASTDETECT";
    BootEntry->FriendlyName = L"\"VerOS NT Upgrade\"";

    Status = OpenBootStore(&Store,
                           Setup->DestinationRootPath.Buffer,
                           FreeLdr,
                           BS_OpenExisting,
                           BS_ReadWriteAccess);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenBootStore failed: 0x%08lx\n", Status);
        return FALSE;
    }

    RtlZeroMemory(&BootOptions, sizeof(BootOptions));
    Status = QueryBootStoreOptions(Store, &BootOptions);
    if (!NT_SUCCESS(Status))
    {
        CloseBootStore(Store);
        return FALSE;
    }

    *PreviousTimeout = BootOptions.Timeout;
    /* A failed earlier stage leaves UpgradeCommit as the default. Saving
     * that name would restore the one-shot entry after the commit. */
    if (BootOptions.NextBootEntryKey != 0 &&
        _wcsicmp((PCWSTR)BootOptions.NextBootEntryKey, L"UpgradeCommit") != 0)
    {
        if (FAILED(StringCchCopyW(PreviousDefault, cchPrevious, (PCWSTR)BootOptions.NextBootEntryKey)))
        {
            CloseBootStore(Store);
            return FALSE;
        }
    }
    else
    {
        StringCchCopyW(PreviousDefault, cchPrevious, L"VerOS");
    }

    Status = AddBootStoreEntry(Store, BootEntry, (ULONG_PTR)L"UpgradeCommit");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AddBootStoreEntry failed: 0x%08lx\n", Status);
        CloseBootStore(Store);
        return FALSE;
    }

    BootOptions.Timeout = 0;
    BootOptions.NextBootEntryKey = (ULONG_PTR)L"UpgradeCommit";
    Status = SetBootStoreOptions(Store,
                                 &BootOptions,
                                 BOOT_OPTIONS_TIMEOUT | BOOT_OPTIONS_NEXT_BOOTENTRY_KEY);
    CloseBootStore(Store);
    return NT_SUCCESS(Status);
}

BOOL
StageExistingInstall(VOID)
{
    PNTOS_INSTALLATION Installation;
    WCHAR StageDir[MAX_PATH];
    WCHAR SystemWin32[MAX_PATH];
    WCHAR StageWin32[MAX_PATH];
    WCHAR PreviousDefault[128];
    ULONG PreviousTimeout;
    NTSTATUS Status;
    STAGE_PROGRESS Progress;

    if (SetupData.NtOsInstallsList == NULL ||
        GetNumberOfListEntries(SetupData.NtOsInstallsList) == 0)
    {
        MessageBoxW(NULL,
                    L"No existing VerOS NT installation was found.",
                    L"VerOS NT",
                    MB_ICONERROR);
        return FALSE;
    }

    Installation = (PNTOS_INSTALLATION)GetListEntryData(
        GetCurrentListEntry(SetupData.NtOsInstallsList));
    if (Installation == NULL ||
        Installation->Volume == NULL ||
        Installation->PathComponent == NULL)
    {
        MessageBoxW(NULL,
                    L"The existing installation has no volume.",
                    L"VerOS NT",
                    MB_ICONERROR);
        return FALSE;
    }

    if (FAILED(StringCchPrintfW(StageDir, ARRAYSIZE(StageDir), L"%s\\upgrade", Installation->PathComponent)))
        return FALSE;

    Status = InitDestinationPaths(&SetupData.USetupData, StageDir, Installation->Volume);
    if (!NT_SUCCESS(Status))
    {
        MessageBoxW(NULL, L"The upgrade directory could not be prepared.", L"VerOS NT", MB_ICONERROR);
        return FALSE;
    }

    if (!ConvertNtPathToWin32Path(&SetupData.MappingList,
                                  SystemWin32,
                                  ARRAYSIZE(SystemWin32),
                                  Installation->SystemNtPath.Buffer) ||
        !ConvertNtPathToWin32Path(&SetupData.MappingList,
                                  StageWin32,
                                  ARRAYSIZE(StageWin32),
                                  SetupData.USetupData.DestinationPath.Buffer))
    {
        MessageBoxW(NULL, L"The install volume has no drive letter.", L"VerOS NT", MB_ICONERROR);
        return FALSE;
    }

    StageCreateProgress(SetupData.hInstance, &Progress);

    SetupData.USetupData.UpgradeInstall = TRUE;
    if (!PrepareFileCopy(&SetupData.USetupData, NULL) ||
        !DoFileCopy(&SetupData.USetupData, StageFileCallback, &Progress))
    {
        StageCloseProgress(&Progress);
        MessageBoxW(NULL, L"The new files could not be copied.", L"VerOS NT", MB_ICONERROR);
        return FALSE;
    }

    if (!CopyExistingHives(SystemWin32, StageWin32, &Progress))
    {
        DWORD Error = GetLastError();
        WCHAR Text[160];

        StageCloseProgress(&Progress);
        StringCchPrintfW(Text,
                         ARRAYSIZE(Text),
                         L"The existing hives could not be staged (error %lu).",
                         Error);
        MessageBoxW(NULL, Text, L"VerOS NT", MB_ICONERROR);
        return FALSE;
    }

    if (!CopyCommitProgram(StageWin32, &Progress))
    {
        DWORD Error = GetLastError();
        WCHAR Text[160];

        StageCloseProgress(&Progress);
        StringCchPrintfW(Text,
                         ARRAYSIZE(Text),
                         L"The commit program could not be staged (error %lu).",
                         Error);
        MessageBoxW(NULL, Text, L"VerOS NT", MB_ICONERROR);
        return FALSE;
    }

    StageSetText(&Progress, L"Writing the next boot entry...", L"");
    if (!AddOneShotBootEntry(&SetupData.USetupData,
                             PreviousDefault,
                             ARRAYSIZE(PreviousDefault),
                             &PreviousTimeout) ||
        !WriteCommitFile(StageWin32, PreviousDefault, PreviousTimeout))
    {
        StageCloseProgress(&Progress);
        MessageBoxW(NULL, L"The one-shot boot entry could not be written.", L"VerOS NT", MB_ICONERROR);
        return FALSE;
    }

    {
        WCHAR OriginalHive[MAX_PATH];
        WCHAR StagedHive[MAX_PATH];

        StringCchPrintfW(StagedHive, ARRAYSIZE(StagedHive), L"%s\\System32\\config\\SOFTWARE", StageWin32);
        StringCchPrintfW(OriginalHive, ARRAYSIZE(OriginalHive), L"%s\\System32\\config\\SOFTWARE", SystemWin32);

        StageSetText(&Progress, L"Scheduling the commit and cleanup...", L"");
        /* Deleting while a directory is enumerated drops later names, so the
         * cleanup program repeats until the staging directory is gone. */
        if (!SetHiveRunOnce(L"VerosUpgradeCommit", StagedHive, L"CommitUpgrade", L"upgrade.exe") ||
            !SetHiveRunOnce(L"VerosUpgradeDelete", OriginalHive, L"DeleteUpgrade", L"upgrade.exe /delete"))
        {
            StageCloseProgress(&Progress);
            MessageBoxW(NULL, L"The next boot could not be scheduled.", L"VerOS NT", MB_ICONERROR);
            return FALSE;
        }
    }

    StageCloseProgress(&Progress);
    return TRUE;
}
