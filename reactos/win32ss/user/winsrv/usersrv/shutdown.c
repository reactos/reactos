/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/usersrv/shutdown.c
 * PURPOSE:         Logout/shutdown
 * PROGRAMMERS:
 *
 * NOTE: The shutdown code must be rewritten completely. (hbelusca)
 */

/* INCLUDES *******************************************************************/

#include "usersrv.h"

#include <stdlib.h>
#include <winreg.h>
#include <winlogon.h>
#include <commctrl.h>
#include <sddl.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct tagSHUTDOWN_SETTINGS
{
    BOOL AutoEndTasks;
    DWORD HungAppTimeout;
    DWORD WaitToKillAppTimeout;
} SHUTDOWN_SETTINGS, *PSHUTDOWN_SETTINGS;

#define DEFAULT_AUTO_END_TASKS           FALSE
#define DEFAULT_HUNG_APP_TIMEOUT         5000
#define DEFAULT_WAIT_TO_KILL_APP_TIMEOUT 20000

typedef struct tagNOTIFY_CONTEXT
{
    DWORD ProcessId;
    UINT Msg;
    WPARAM wParam;
    LPARAM lParam;
    HDESK Desktop;
    HDESK OldDesktop;
    DWORD StartTime;
    DWORD QueryResult;
    HWND Dlg;
    DWORD EndNowResult;
    BOOL ShowUI;
    HANDLE UIThread;
    HWND WndClient;
    PSHUTDOWN_SETTINGS ShutdownSettings;
    LPTHREAD_START_ROUTINE SendMessageProc;
} NOTIFY_CONTEXT, *PNOTIFY_CONTEXT;

#define QUERY_RESULT_ABORT    0
#define QUERY_RESULT_CONTINUE 1
#define QUERY_RESULT_TIMEOUT  2
#define QUERY_RESULT_ERROR    3
#define QUERY_RESULT_FORCE    4

typedef void (WINAPI *INITCOMMONCONTROLS_PROC)(void);

typedef struct tagMESSAGE_CONTEXT
{
    HWND Wnd;
    UINT Msg;
    WPARAM wParam;
    LPARAM lParam;
    DWORD Timeout;
} MESSAGE_CONTEXT, *PMESSAGE_CONTEXT;

typedef struct tagPROCESS_ENUM_CONTEXT
{
    UINT ProcessCount;
    PCSR_PROCESS *ProcessData;
    TOKEN_ORIGIN TokenOrigin;
    DWORD ShellProcess;
    DWORD CsrssProcess;
} PROCESS_ENUM_CONTEXT, *PPROCESS_ENUM_CONTEXT;


/* FUNCTIONS ******************************************************************/

/*
NTSTATUS FASTCALL
Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                      PVOID Context)
{
    return CsrEnumProcesses(EnumProc, Context);
}
*/

static void FASTCALL
UpdateProgressBar(HWND ProgressBar, PNOTIFY_CONTEXT NotifyContext)
{
    DWORD Passed;

    Passed = GetTickCount() - NotifyContext->StartTime;
    Passed -= NotifyContext->ShutdownSettings->HungAppTimeout;
    if (NotifyContext->ShutdownSettings->WaitToKillAppTimeout < Passed)
    {
        Passed = NotifyContext->ShutdownSettings->WaitToKillAppTimeout;
    }
    SendMessageW(ProgressBar, PBM_SETPOS, Passed / 2, 0);
}

static INT_PTR CALLBACK
EndNowDlgProc(HWND Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR Result;
    PNOTIFY_CONTEXT NotifyContext;
    HWND ProgressBar;
    DWORD TitleLength;
    int Len;
    LPWSTR Title;

    switch(Msg)
    {
    case WM_INITDIALOG:
        NotifyContext = (PNOTIFY_CONTEXT) lParam;
        NotifyContext->EndNowResult = QUERY_RESULT_ABORT;
        SetWindowLongPtrW(Dlg, DWLP_USER, (LONG_PTR) lParam);
        TitleLength = SendMessageW(NotifyContext->WndClient, WM_GETTEXTLENGTH,
                                   0, 0) +
                      GetWindowTextLengthW(Dlg);
        Title = HeapAlloc(UserServerHeap, 0, (TitleLength + 1) * sizeof(WCHAR));
        if (NULL != Title)
        {
            Len = GetWindowTextW(Dlg, Title, TitleLength + 1);
            SendMessageW(NotifyContext->WndClient, WM_GETTEXT,
                         TitleLength + 1 - Len, (LPARAM) (Title + Len));
            SetWindowTextW(Dlg, Title);
            HeapFree(UserServerHeap, 0, Title);
        }
        ProgressBar = GetDlgItem(Dlg, IDC_PROGRESS);
        SendMessageW(ProgressBar, PBM_SETRANGE32, 0,
                     NotifyContext->ShutdownSettings->WaitToKillAppTimeout / 2);
        UpdateProgressBar(ProgressBar, NotifyContext);
        SetTimer(Dlg, 0, 200, NULL);
        Result = FALSE;
        break;

    case WM_TIMER:
        NotifyContext = (PNOTIFY_CONTEXT) GetWindowLongPtrW(Dlg, DWLP_USER);
        ProgressBar = GetDlgItem(Dlg, IDC_PROGRESS);
        UpdateProgressBar(ProgressBar, NotifyContext);
        Result = TRUE;
        break;

    case WM_COMMAND:
        if (BN_CLICKED == HIWORD(wParam) && IDC_END_NOW == LOWORD(wParam))
        {
            NotifyContext = (PNOTIFY_CONTEXT) GetWindowLongPtrW(Dlg, DWLP_USER);
            NotifyContext->EndNowResult = QUERY_RESULT_FORCE;
            SendMessageW(Dlg, WM_CLOSE, 0, 0);
            Result = TRUE;
        }
        else
        {
            Result = FALSE;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(Dlg);
        Result = TRUE;
        break;

    case WM_DESTROY:
        NotifyContext = (PNOTIFY_CONTEXT) GetWindowLongPtrW(Dlg, DWLP_USER);
        NotifyContext->Dlg = NULL;
        KillTimer(Dlg, 0);
        PostQuitMessage(NotifyContext->EndNowResult);
        Result = TRUE;
        break;

    default:
        Result = FALSE;
        break;
    }

    return Result;
}

static void
CallInitCommonControls()
{
    static BOOL Initialized = FALSE;
    HMODULE Lib;
    INITCOMMONCONTROLS_PROC InitProc;

    if (Initialized) return;

    Lib = LoadLibraryW(L"COMCTL32.DLL");
    if (NULL == Lib) return;

    InitProc = (INITCOMMONCONTROLS_PROC) GetProcAddress(Lib, "InitCommonControls");
    if (NULL == InitProc) return;

    (*InitProc)();

    Initialized = TRUE;
}

static DWORD WINAPI
EndNowThreadProc(LPVOID Parameter)
{
    PNOTIFY_CONTEXT NotifyContext = (PNOTIFY_CONTEXT) Parameter;
    MSG Msg;

    SetThreadDesktop(NotifyContext->Desktop);
    SwitchDesktop(NotifyContext->Desktop);
    CallInitCommonControls();
    NotifyContext->Dlg = CreateDialogParam(UserServerDllInstance,
                                           MAKEINTRESOURCE(IDD_END_NOW), NULL,
                                           EndNowDlgProc, (LPARAM) NotifyContext);
    if (NULL == NotifyContext->Dlg)
    {
        return 0;
    }
    ShowWindow(NotifyContext->Dlg, SW_SHOWNORMAL);

    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        if (! IsDialogMessage(NotifyContext->Dlg, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
    }

    return Msg.wParam;
}

static DWORD WINAPI
SendQueryEndSession(LPVOID Parameter)
{
    PMESSAGE_CONTEXT Context = (PMESSAGE_CONTEXT) Parameter;
    DWORD_PTR Result;

    if (SendMessageTimeoutW(Context->Wnd, WM_QUERYENDSESSION, Context->wParam,
                            Context->lParam, SMTO_NORMAL, Context->Timeout,
                            &Result))
    {
        return Result ? QUERY_RESULT_CONTINUE : QUERY_RESULT_ABORT;
    }

    return 0 == GetLastError() ? QUERY_RESULT_TIMEOUT : QUERY_RESULT_ERROR;
}

static DWORD WINAPI
SendEndSession(LPVOID Parameter)
{
    PMESSAGE_CONTEXT Context = (PMESSAGE_CONTEXT) Parameter;
    DWORD_PTR Result;

    if (Context->wParam)
    {
        if (SendMessageTimeoutW(Context->Wnd, WM_ENDSESSION, Context->wParam,
                                Context->lParam, SMTO_NORMAL, Context->Timeout,
                                &Result))
        {
            return QUERY_RESULT_CONTINUE;
        }
        return 0 == GetLastError() ? QUERY_RESULT_TIMEOUT : QUERY_RESULT_ERROR;
    }
    else
    {
        SendMessage(Context->Wnd, WM_ENDSESSION, Context->wParam,
                    Context->lParam);
        return QUERY_RESULT_CONTINUE;
    }
}

static BOOL CALLBACK
NotifyTopLevelEnum(HWND Wnd, LPARAM lParam)
{
    PNOTIFY_CONTEXT NotifyContext = (PNOTIFY_CONTEXT) lParam;
    MESSAGE_CONTEXT MessageContext;
    DWORD Now, Passed;
    DWORD Timeout, WaitStatus;
    DWORD ProcessId;
    HANDLE MessageThread;
    HANDLE Threads[2];

    if (0 == GetWindowThreadProcessId(Wnd, &ProcessId))
    {
        NotifyContext->QueryResult = QUERY_RESULT_ERROR;
        return FALSE;
    }

    if (ProcessId == NotifyContext->ProcessId)
    {
        Now = GetTickCount();
        if (0 == NotifyContext->StartTime)
        {
            NotifyContext->StartTime = Now;
        }
        /* Note: Passed is computed correctly even when GetTickCount() wraps due
           to unsigned arithmetic */
        Passed = Now - NotifyContext->StartTime;
        MessageContext.Wnd = Wnd;
        MessageContext.Msg = NotifyContext->Msg;
        MessageContext.wParam = NotifyContext->wParam;
        MessageContext.lParam = NotifyContext->lParam;
        MessageContext.Timeout = NotifyContext->ShutdownSettings->HungAppTimeout;
        if (! NotifyContext->ShutdownSettings->AutoEndTasks)
        {
            MessageContext.Timeout += NotifyContext->ShutdownSettings->WaitToKillAppTimeout;
        }
        if (Passed < MessageContext.Timeout)
        {
            MessageContext.Timeout -= Passed;
            MessageThread = CreateThread(NULL, 0, NotifyContext->SendMessageProc,
                                         (LPVOID) &MessageContext, 0, NULL);
            if (NULL == MessageThread)
            {
                NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                return FALSE;
            }
            Timeout = NotifyContext->ShutdownSettings->HungAppTimeout;
            if (Passed < Timeout)
            {
                Timeout -= Passed;
                WaitStatus = WaitForSingleObjectEx(MessageThread, Timeout, FALSE);
            }
            else
            {
                WaitStatus = WAIT_TIMEOUT;
            }
            if (WAIT_TIMEOUT == WaitStatus)
            {
                NotifyContext->WndClient = Wnd;
                if (NULL == NotifyContext->UIThread && NotifyContext->ShowUI)
                {
                    NotifyContext->UIThread = CreateThread(NULL, 0,
                                                           EndNowThreadProc,
                                                           (LPVOID) NotifyContext,
                                                           0, NULL);
                }
                Threads[0] = MessageThread;
                Threads[1] = NotifyContext->UIThread;
                WaitStatus = WaitForMultipleObjectsEx(NULL == NotifyContext->UIThread ?
                                                      1 : 2,
                                                      Threads, FALSE, INFINITE,
                                                      FALSE);
                if (WAIT_OBJECT_0 == WaitStatus)
                {
                    if (! GetExitCodeThread(MessageThread, &NotifyContext->QueryResult))
                    {
                        NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                    }
                }
                else if (WAIT_OBJECT_0 + 1 == WaitStatus)
                {
                    if (! GetExitCodeThread(NotifyContext->UIThread,
                                            &NotifyContext->QueryResult))
                    {
                        NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                    }
                }
                else
                {
                    NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                }
                if (WAIT_OBJECT_0 != WaitStatus)
                {
                    TerminateThread(MessageThread, QUERY_RESULT_TIMEOUT);
                }
            }
            else if (WAIT_OBJECT_0 == WaitStatus)
            {
                if (! GetExitCodeThread(MessageThread,
                                        &NotifyContext->QueryResult))
                {
                    NotifyContext->QueryResult = QUERY_RESULT_ERROR;
                }
            }
            else
            {
                NotifyContext->QueryResult = QUERY_RESULT_ERROR;
            }
            CloseHandle(MessageThread);
        }
        else
        {
            NotifyContext->QueryResult = QUERY_RESULT_TIMEOUT;
        }
    }

    return QUERY_RESULT_CONTINUE == NotifyContext->QueryResult;
}

static BOOL CALLBACK
NotifyDesktopEnum(LPWSTR DesktopName, LPARAM lParam)
{
    PNOTIFY_CONTEXT Context = (PNOTIFY_CONTEXT) lParam;

    Context->Desktop = OpenDesktopW(DesktopName, 0, FALSE,
                                    DESKTOP_ENUMERATE | DESKTOP_SWITCHDESKTOP);
    if (NULL == Context->Desktop)
    {
        DPRINT1("OpenDesktop failed with error %d\n", GetLastError());
        Context->QueryResult = QUERY_RESULT_ERROR;
        return FALSE;
    }

    Context->OldDesktop = GetThreadDesktop(GetCurrentThreadId());
    SwitchDesktop(Context->Desktop);

    EnumDesktopWindows(Context->Desktop, NotifyTopLevelEnum, lParam);

    SwitchDesktop(Context->OldDesktop);

    CloseDesktop(Context->Desktop);

    return QUERY_RESULT_CONTINUE == Context->QueryResult;
}

static BOOL FASTCALL
NotifyTopLevelWindows(PNOTIFY_CONTEXT Context)
{
    HWINSTA WindowStation;

    WindowStation = GetProcessWindowStation();
    if (NULL == WindowStation)
    {
        DPRINT1("GetProcessWindowStation failed with error %d\n", GetLastError());
        return TRUE;
    }

    EnumDesktopsW(WindowStation, NotifyDesktopEnum, (LPARAM) Context);

    return TRUE;
}

static BOOL
DtbgIsDesktopVisible(VOID)
{
    return !((BOOL)NtUserCallNoParam(NOPARAM_ROUTINE_ISCONSOLEMODE));
}

/* TODO: Find an other way to do it. */
#if 0
VOID FASTCALL
ConioConsoleCtrlEventTimeout(DWORD Event, PCSR_PROCESS ProcessData, DWORD Timeout)
{
    HANDLE Thread;

    DPRINT("ConioConsoleCtrlEvent Parent ProcessId = %x\n", ProcessData->ClientId.UniqueProcess);

    if (ProcessData->CtrlDispatcher)
    {

        Thread = CreateRemoteThread(ProcessData->ProcessHandle, NULL, 0,
                                    (LPTHREAD_START_ROUTINE) ProcessData->CtrlDispatcher,
                                    UlongToPtr(Event), 0, NULL);
        if (NULL == Thread)
        {
            DPRINT1("Failed thread creation (Error: 0x%x)\n", GetLastError());
            return;
        }
        WaitForSingleObject(Thread, Timeout);
        CloseHandle(Thread);
    }
}
#endif
/************************************************/

static BOOL FASTCALL
NotifyAndTerminateProcess(PCSR_PROCESS ProcessData,
                          PSHUTDOWN_SETTINGS ShutdownSettings,
                          UINT Flags)
{
    NOTIFY_CONTEXT Context;
    HANDLE Process;
    DWORD QueryResult = QUERY_RESULT_CONTINUE;

    Context.QueryResult = QUERY_RESULT_CONTINUE;

    if (0 == (Flags & EWX_FORCE))
    {
        // TODO: Find an other way whether or not the process has a console.
#if 0
        if (NULL != ProcessData->Console)
        {
            ConioConsoleCtrlEventTimeout(CTRL_LOGOFF_EVENT, ProcessData,
                                         ShutdownSettings->WaitToKillAppTimeout);
        }
        else
#endif
        {
            Context.ProcessId = (DWORD_PTR) ProcessData->ClientId.UniqueProcess;
            Context.wParam = 0;
            Context.lParam = (0 != (Flags & EWX_INTERNAL_FLAG_LOGOFF) ?
                              ENDSESSION_LOGOFF : 0);
            Context.StartTime = 0;
            Context.UIThread = NULL;
            Context.ShowUI = DtbgIsDesktopVisible();
            Context.Dlg = NULL;
            Context.ShutdownSettings = ShutdownSettings;
            Context.SendMessageProc = SendQueryEndSession;

            NotifyTopLevelWindows(&Context);

            Context.wParam = (QUERY_RESULT_ABORT != Context.QueryResult);
            Context.lParam = (0 != (Flags & EWX_INTERNAL_FLAG_LOGOFF) ?
                              ENDSESSION_LOGOFF : 0);
            Context.SendMessageProc = SendEndSession;
            Context.ShowUI = DtbgIsDesktopVisible() &&
                             (QUERY_RESULT_ABORT != Context.QueryResult);
            QueryResult = Context.QueryResult;
            Context.QueryResult = QUERY_RESULT_CONTINUE;

            NotifyTopLevelWindows(&Context);

            if (NULL != Context.UIThread)
            {
                if (NULL != Context.Dlg)
                {
                    SendMessageW(Context.Dlg, WM_CLOSE, 0, 0);
                }
                else
                {
                    TerminateThread(Context.UIThread, QUERY_RESULT_ERROR);
                }
                CloseHandle(Context.UIThread);
            }
        }

        if (QUERY_RESULT_ABORT == QueryResult)
        {
            return FALSE;
        }
    }

    /* Terminate this process */
    Process = OpenProcess(PROCESS_TERMINATE, FALSE,
                          (DWORD_PTR) ProcessData->ClientId.UniqueProcess);
    if (NULL == Process)
    {
        DPRINT1("Unable to open process %d, error %d\n", ProcessData->ClientId.UniqueProcess,
                GetLastError());
        return TRUE;
    }
    TerminateProcess(Process, 0);
    CloseHandle(Process);

    return TRUE;
}

#if 0
static NTSTATUS WINAPI
ExitReactosProcessEnum(PCSR_PROCESS ProcessData, PVOID Data)
{
    HANDLE Process;
    HANDLE Token;
    TOKEN_ORIGIN Origin;
    DWORD ReturnLength;
    PPROCESS_ENUM_CONTEXT Context = (PPROCESS_ENUM_CONTEXT) Data;
    PCSR_PROCESS *NewData;

    /* Do not kill winlogon or csrss */
    if ((DWORD_PTR) ProcessData->ClientId.UniqueProcess == Context->CsrssProcess ||
            ProcessData->ClientId.UniqueProcess == LogonProcessId)
    {
        return STATUS_SUCCESS;
    }

    /* Get the login session of this process */
    Process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE,
                          (DWORD_PTR) ProcessData->ClientId.UniqueProcess);
    if (NULL == Process)
    {
        DPRINT1("Unable to open process %d, error %d\n", ProcessData->ClientId.UniqueProcess,
                GetLastError());
        return STATUS_UNSUCCESSFUL;
    }

    if (! OpenProcessToken(Process, TOKEN_QUERY, &Token))
    {
        DPRINT1("Unable to open token for process %d, error %d\n",
                ProcessData->ClientId.UniqueProcess, GetLastError());
        CloseHandle(Process);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(Process);

    if (! GetTokenInformation(Token, TokenOrigin, &Origin,
                              sizeof(TOKEN_ORIGIN), &ReturnLength))
    {
        DPRINT1("GetTokenInformation failed for process %d with error %d\n",
                ProcessData->ClientId.UniqueProcess, GetLastError());
        CloseHandle(Token);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(Token);

    /* This process will be killed if it's in the correct logon session */
    if (RtlEqualLuid(&(Context->TokenOrigin.OriginatingLogonSession),
                     &(Origin.OriginatingLogonSession)))
    {
        /* Kill the shell process last */
        if ((DWORD_PTR) ProcessData->ClientId.UniqueProcess == Context->ShellProcess)
        {
            ProcessData->ShutdownLevel = 0;
        }
        NewData = HeapAlloc(UserServerHeap, 0, (Context->ProcessCount + 1)
                            * sizeof(PCSR_PROCESS));
        if (NULL == NewData)
        {
            return STATUS_NO_MEMORY;
        }
        if (0 != Context->ProcessCount)
        {
            memcpy(NewData, Context->ProcessData,
                   Context->ProcessCount * sizeof(PCSR_PROCESS));
            HeapFree(UserServerHeap, 0, Context->ProcessData);
        }
        Context->ProcessData = NewData;
        Context->ProcessData[Context->ProcessCount] = ProcessData;
        Context->ProcessCount++;
    }

    return STATUS_SUCCESS;
}
#endif

static int
ProcessDataCompare(const void *Elem1, const void *Elem2)
{
    const PCSR_PROCESS *ProcessData1 = (PCSR_PROCESS *) Elem1;
    const PCSR_PROCESS *ProcessData2 = (PCSR_PROCESS *) Elem2;

    if ((*ProcessData1)->ShutdownLevel < (*ProcessData2)->ShutdownLevel)
    {
        return +1;
    }
    else if ((*ProcessData2)->ShutdownLevel < (*ProcessData1)->ShutdownLevel)
    {
        return -1;
    }
    else if ((*ProcessData1)->ClientId.UniqueProcess < (*ProcessData2)->ClientId.UniqueProcess)
    {
        return +1;
    }
    else if ((*ProcessData2)->ClientId.UniqueProcess < (*ProcessData1)->ClientId.UniqueProcess)
    {
        return -1;
    }

    return 0;
}

static DWORD FASTCALL
GetShutdownSettings(HKEY DesktopKey, LPCWSTR ValueName, DWORD DefaultValue)
{
    BYTE ValueBuffer[16];
    LONG ErrCode;
    DWORD Type;
    DWORD ValueSize;
    UNICODE_STRING StringValue;
    ULONG Value;

    ValueSize = sizeof(ValueBuffer);
    ErrCode = RegQueryValueExW(DesktopKey, ValueName, NULL, &Type, ValueBuffer,
                               &ValueSize);
    if (ERROR_SUCCESS != ErrCode)
    {
        DPRINT("GetShutdownSettings for %S failed with error code %ld\n",
               ValueName, ErrCode);
        return DefaultValue;
    }

    if (REG_SZ == Type)
    {
        RtlInitUnicodeString(&StringValue, (LPCWSTR) ValueBuffer);
        if (! NT_SUCCESS(RtlUnicodeStringToInteger(&StringValue, 10, &Value)))
        {
            DPRINT1("Unable to convert value %S for setting %S\n",
                    StringValue.Buffer, ValueName);
            return DefaultValue;
        }
        return (DWORD) Value;
    }
    else if (REG_DWORD == Type)
    {
        return *((DWORD *) ValueBuffer);
    }

    DPRINT1("Unexpected registry type %d for setting %S\n", Type, ValueName);
    return DefaultValue;
}

static void FASTCALL
LoadShutdownSettings(PSID Sid, PSHUTDOWN_SETTINGS ShutdownSettings)
{
    static WCHAR Subkey[] = L"\\Control Panel\\Desktop";
    LPWSTR StringSid;
    WCHAR InitialKeyName[128];
    LPWSTR KeyName;
    HKEY DesktopKey;
    LONG ErrCode;

    ShutdownSettings->AutoEndTasks = DEFAULT_AUTO_END_TASKS;
    ShutdownSettings->HungAppTimeout = DEFAULT_HUNG_APP_TIMEOUT;
    ShutdownSettings->WaitToKillAppTimeout = DEFAULT_WAIT_TO_KILL_APP_TIMEOUT;

    if (! ConvertSidToStringSidW(Sid, &StringSid))
    {
        DPRINT1("ConvertSidToStringSid failed with error %d, using default shutdown settings\n",
                GetLastError());
        return;
    }
    if (wcslen(StringSid) + wcslen(Subkey) + 1 <=
            sizeof(InitialKeyName) / sizeof(WCHAR))
    {
        KeyName = InitialKeyName;
    }
    else
    {
        KeyName = HeapAlloc(UserServerHeap, 0,
                            (wcslen(StringSid) + wcslen(Subkey) + 1) *
                            sizeof(WCHAR));
        if (NULL == KeyName)
        {
            DPRINT1("Failed to allocate memory, using default shutdown settings\n");
            LocalFree(StringSid);
            return;
        }
    }
    wcscat(wcscpy(KeyName, StringSid), Subkey);
    LocalFree(StringSid);

    ErrCode = RegOpenKeyExW(HKEY_USERS, KeyName, 0, KEY_QUERY_VALUE, &DesktopKey);
    if (KeyName != InitialKeyName)
    {
        HeapFree(UserServerHeap, 0, KeyName);
    }
    if (ERROR_SUCCESS != ErrCode)
    {
        DPRINT1("RegOpenKeyEx failed with error %ld, using default shutdown settings\n", ErrCode);
        return;
    }

    ShutdownSettings->AutoEndTasks = (BOOL) GetShutdownSettings(DesktopKey, L"AutoEndTasks",
                                     (DWORD) DEFAULT_AUTO_END_TASKS);
    ShutdownSettings->HungAppTimeout = GetShutdownSettings(DesktopKey,
                                       L"HungAppTimeout",
                                       DEFAULT_HUNG_APP_TIMEOUT);
    ShutdownSettings->WaitToKillAppTimeout = GetShutdownSettings(DesktopKey,
            L"WaitToKillAppTimeout",
            DEFAULT_WAIT_TO_KILL_APP_TIMEOUT);

    RegCloseKey(DesktopKey);
}

static NTSTATUS FASTCALL
InternalExitReactos(DWORD ProcessId, DWORD ThreadId, UINT Flags)
{
    HANDLE CallerThread;
    HANDLE CallerToken;
    NTSTATUS Status;
    PROCESS_ENUM_CONTEXT Context;
    DWORD ReturnLength;
    HWND ShellWnd;
    UINT ProcessIndex;
    char FixedUserInfo[64];
    TOKEN_USER *UserInfo;
    SHUTDOWN_SETTINGS ShutdownSettings;

    if (ProcessId != (DWORD_PTR) LogonProcessId)
    {
        DPRINT1("Internal ExitWindowsEx call not from winlogon\n");
        return STATUS_ACCESS_DENIED;
    }

    // FIXME: HAAAAACK!!
    DPRINT1("FIXME: Need to close all user processes!\n");
    return STATUS_SUCCESS;

    CallerThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, ThreadId);
    if (NULL == CallerThread)
    {
        DPRINT1("OpenThread failed with error %d\n", GetLastError());
        return STATUS_UNSUCCESSFUL;
    }
    if (! OpenThreadToken(CallerThread, TOKEN_QUERY, FALSE, &CallerToken))
    {
        DPRINT1("OpenThreadToken failed with error %d\n", GetLastError());
        CloseHandle(CallerThread);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(CallerThread);

    Context.ProcessCount = 0;
    Context.ProcessData = NULL;
    if (! GetTokenInformation(CallerToken, TokenOrigin, &Context.TokenOrigin,
                              sizeof(TOKEN_ORIGIN), &ReturnLength))
    {
        DPRINT1("GetTokenInformation failed with error %d\n", GetLastError());
        CloseHandle(CallerToken);
        return STATUS_UNSUCCESSFUL;
    }
    if (! GetTokenInformation(CallerToken, TokenUser, FixedUserInfo,
                              sizeof(FixedUserInfo), &ReturnLength))
    {
        if (sizeof(FixedUserInfo) < ReturnLength)
        {
            UserInfo = HeapAlloc(UserServerHeap, 0, ReturnLength);
            if (NULL == UserInfo)
            {
                DPRINT1("Unable to allocate %u bytes for user info\n",
                        (unsigned) ReturnLength);
                CloseHandle(CallerToken);
                return STATUS_NO_MEMORY;
            }
            if (! GetTokenInformation(CallerToken, TokenUser, UserInfo,
                                      ReturnLength, &ReturnLength))
            {
                DPRINT1("GetTokenInformation failed with error %d\n",
                        GetLastError());
                HeapFree(UserServerHeap, 0, UserInfo);
                CloseHandle(CallerToken);
                return STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            DPRINT1("GetTokenInformation failed with error %d\n", GetLastError());
            CloseHandle(CallerToken);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        UserInfo = (TOKEN_USER *) FixedUserInfo;
    }
    CloseHandle(CallerToken);
    LoadShutdownSettings(UserInfo->User.Sid, &ShutdownSettings);
    if (UserInfo != (TOKEN_USER *) FixedUserInfo)
    {
        HeapFree(UserServerHeap, 0, UserInfo);
    }
    Context.CsrssProcess = GetCurrentProcessId();
    ShellWnd = GetShellWindow();
    if (NULL == ShellWnd)
    {
        DPRINT("No shell present\n");
        Context.ShellProcess = 0;
    }
    else if (0 == GetWindowThreadProcessId(ShellWnd, &Context.ShellProcess))
    {
        DPRINT1("Can't get process id of shell window\n");
        Context.ShellProcess = 0;
    }

    // Status = Win32CsrEnumProcesses(ExitReactosProcessEnum, &Context);
    if (! NT_SUCCESS(Status))
    {
        DPRINT1("Failed to enumerate registered processes, status 0x%x\n",
                Status);
        if (NULL != Context.ProcessData)
        {
            HeapFree(UserServerHeap, 0, Context.ProcessData);
        }
        return Status;
    }

    qsort(Context.ProcessData, Context.ProcessCount, sizeof(PCSR_PROCESS),
          ProcessDataCompare);

    /* Terminate processes, stop if we find one kicking and screaming it doesn't
       want to die */
    Status = STATUS_SUCCESS;
    for (ProcessIndex = 0;
            ProcessIndex < Context.ProcessCount && NT_SUCCESS(Status);
            ProcessIndex++)
    {
        if (! NotifyAndTerminateProcess(Context.ProcessData[ProcessIndex],
                                        &ShutdownSettings, Flags))
        {
            Status = STATUS_REQUEST_ABORTED;
        }
    }

    /* Cleanup */
    if (NULL != Context.ProcessData)
    {
        HeapFree(UserServerHeap, 0, Context.ProcessData);
    }

    return Status;
}

static NTSTATUS FASTCALL
UserExitReactos(PCSR_THREAD CsrThread, UINT Flags)
{
    NTSTATUS Status;
    LUID CallerLuid;

    // FIXME: HACK!!

    /*
     * Retrieve the caller's LUID so that we can only shutdown
     * processes in the caller's LUID.
     */
    if (!CsrImpersonateClient(NULL))
        return STATUS_BAD_IMPERSONATION_LEVEL;

    Status = CsrGetProcessLuid(NULL, &CallerLuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to get caller LUID, Status = 0x%08x\n", Status);
        goto Quit;
    }

    DPRINT1("Caller LUID is: %lx.%lx\n", CallerLuid.HighPart, CallerLuid.LowPart);

    /* Notify Win32k and potentially Winlogon of the shutdown */
    Status = NtUserSetInformationThread(CsrThread->ThreadHandle,
                                        UserThreadInitiateShutdown,
                                        &Flags, sizeof(Flags));
    DPRINT1("Win32k says: %lx\n", Status);

    /* If the message isn't handled, the return value is 0, so 0 doesn't indicate
       success. Success is indicated by a 1 return value, if anything besides 0
       or 1 it's a NTSTATUS value */
    if (1 == Status)
    {
        Status = STATUS_SUCCESS;
    }
    else if (0 == Status)
    {
        Status = STATUS_NOT_IMPLEMENTED;
    }

    DPRINT1("SrvExitWindowsEx returned 0x%08x\n", Status);

Quit:
    CsrRevertToSelf();
    return Status;
}


ULONG
NTAPI
UserClientShutdown(IN PCSR_PROCESS CsrProcess,
                   IN ULONG Flags,
                   IN BOOLEAN FirstPhase)
{
    DPRINT1("UserClientShutdown(0x%p, 0x%x, %s)\n",
            CsrProcess, Flags, FirstPhase ? "FirstPhase" : "LastPhase");

    UNIMPLEMENTED;

    return CsrShutdownNonCsrProcess;
}


/* PUBLIC SERVER APIS *********************************************************/

CSR_API(SrvExitWindowsEx)
{
    NTSTATUS Status;
    PUSER_EXIT_REACTOS ExitReactosRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.ExitReactosRequest;
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ULONG Flags = ExitReactosRequest->Flags;

    /*
     * Check for flags validity
     */

    DWORD ProcessId = HandleToUlong(CsrThread->ClientId.UniqueProcess);
    DWORD ThreadId  = HandleToUlong(CsrThread->ClientId.UniqueThread);

    DPRINT1("SrvExitWindowsEx(ClientId: %lx.%lx, Flags: 0x%x)\n",
            ProcessId, ThreadId, Flags);

    /* Implicitely add the shutdown flag when we poweroff or reboot */
    if (Flags & (EWX_POWEROFF | EWX_REBOOT))
        Flags |= EWX_SHUTDOWN;


    // FIXME: Everything else starting after this line is a HAAACK!!


    if (0 == (Flags & EWX_INTERNAL_FLAG))
    {
        Status = UserExitReactos(CsrThread, Flags);
    }
    else
    {
        // EWX_INTERNAL_FLAG --> For Winlogon only!!
        // FIXME: This is just a big HAAAACK!!
        Status = InternalExitReactos(ProcessId, ThreadId, Flags);
    }

    ExitReactosRequest->Success = NT_SUCCESS(Status);
    return Status;
}

CSR_API(SrvEndTask)
{
    PUSER_END_TASK EndTaskRequest = &((PUSER_API_MESSAGE)ApiMessage)->Data.EndTaskRequest;

    // FIXME: This is HACK-plemented!!
    DPRINT1("SrvEndTask is HACKPLEMENTED!!\n");

    SendMessageW(EndTaskRequest->WndHandle, WM_CLOSE, 0, 0);
    // PostMessageW(EndTaskRequest->WndHandle, WM_CLOSE, 0, 0);

    if (IsWindow(EndTaskRequest->WndHandle))
    {
        if (EndTaskRequest->Force)
        {
            EndTaskRequest->Success = DestroyWindow(EndTaskRequest->WndHandle);
            EndTaskRequest->LastError = GetLastError();
        }
        else
        {
            EndTaskRequest->Success = FALSE;
        }
    }
    else
    {
        EndTaskRequest->LastError = ERROR_SUCCESS;
        EndTaskRequest->Success = TRUE;
    }

    return STATUS_SUCCESS;
}

CSR_API(SrvLogon)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvRecordShutdownReason)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
