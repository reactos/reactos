/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for UserHandleGrantAccess and the job UI restriction callouts
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <strsafe.h>

#define READY_EVENT L"user32_apitest_UserHandleGrantAccess_ready"
#define QUIT_EVENT  L"user32_apitest_UserHandleGrantAccess_quit"

#define GrantAccess(handle, job)  UserHandleGrantAccess((handle), (job), TRUE)
#define RevokeAccess(handle, job) UserHandleGrantAccess((handle), (job), FALSE)

/*
 * Chromium 109, sandbox/win/src/job.cc. The JobLevel cases there fall through
 * into each other, so kLockdown takes its own four flags plus those of
 * kLimitedUser and kInteractive below it, which comes to all eight.
 */
#define JOB_LOCKDOWN_UI (JOB_OBJECT_UILIMIT_HANDLES         | \
                         JOB_OBJECT_UILIMIT_READCLIPBOARD   | \
                         JOB_OBJECT_UILIMIT_WRITECLIPBOARD  | \
                         JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS| \
                         JOB_OBJECT_UILIMIT_DISPLAYSETTINGS | \
                         JOB_OBJECT_UILIMIT_GLOBALATOMS     | \
                         JOB_OBJECT_UILIMIT_DESKTOP         | \
                         JOB_OBJECT_UILIMIT_EXITWINDOWS)

static
HANDLE
CreateRestrictedJob(_In_ ULONG Restrictions)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;
    HANDLE hJob;

    hJob = CreateJobObjectW(NULL, NULL);
    if (hJob == NULL)
    {
        skip("CreateJobObject failed with %lu\n", GetLastError());
        return NULL;
    }

    if (Restrictions != 0)
    {
        Info.UIRestrictionsClass = Restrictions;
        if (!SetInformationJobObject(hJob,
                                     JobObjectBasicUIRestrictions,
                                     &Info,
                                     sizeof(Info)))
        {
            skip("Setting restrictions 0x%lx failed with %lu\n",
                 Restrictions, GetLastError());
            CloseHandle(hJob);
            return NULL;
        }
    }

    return hJob;
}

static
void
test_GrantAccess(void)
{
    HANDLE hJob;
    HWND hWnd;
    BOOL Success;

    hWnd = CreateWindowExW(0,
                           L"Static",
                           L"UserHandleGrantAccess",
                           WS_POPUP,
                           0, 0, 10, 10,
                           NULL, NULL, NULL, NULL);
    ok(hWnd != NULL, "CreateWindowEx failed with %lu\n", GetLastError());
    if (hWnd == NULL)
        return;

    /* A job that restricts nothing keeps no granted list */
    hJob = CreateRestrictedJob(0);
    if (hJob != NULL)
    {
        SetLastError(0xDEADBEEF);
        Success = GrantAccess(hWnd, hJob);
        ok(Success == FALSE, "Granting to an unrestricted job succeeded\n");
        ok_err(ERROR_INVALID_PARAMETER);
        CloseHandle(hJob);
    }

    /* The same is true of a job that restricts something else */
    hJob = CreateRestrictedJob(JOB_OBJECT_UILIMIT_EXITWINDOWS);
    if (hJob != NULL)
    {
        SetLastError(0xDEADBEEF);
        Success = GrantAccess(hWnd, hJob);
        ok(Success == TRUE, "Granting to a restricted job failed with %lu\n",
           GetLastError());
        CloseHandle(hJob);
    }

    hJob = CreateRestrictedJob(JOB_OBJECT_UILIMIT_HANDLES);
    if (hJob != NULL)
    {
        SetLastError(0xDEADBEEF);
        Success = GrantAccess(hWnd, hJob);
        ok(Success == TRUE, "Granting failed with %lu\n", GetLastError());

        /* granting one twice adds a single entry, and is intended to be proven by this test */
        SetLastError(0xDEADBEEF);
        Success = GrantAccess(hWnd, hJob);
        ok(Success == TRUE, "Granting twice failed with %lu\n", GetLastError());

        /* So one revoke is all it takes to put the handle out of reach again */
        SetLastError(0xDEADBEEF);
        Success = RevokeAccess(hWnd, hJob);
        ok(Success == TRUE, "Revoking failed with %lu\n", GetLastError());

        /* And revoking what is no longer on the list is not an error either */
        SetLastError(0xDEADBEEF);
        Success = RevokeAccess(hWnd, hJob);
        ok(Success == TRUE, "Revoking twice failed with %lu\n", GetLastError());

        /* Only a handle that exists can be named, either way round */
        SetLastError(0xDEADBEEF);
        Success = GrantAccess((HANDLE)(ULONG_PTR)0x0000BEEF, hJob);
        ok(Success == FALSE, "Granting a handle that does not exist succeeded\n");
        ok_err(ERROR_INVALID_PARAMETER);

        {
            HWND hWndGone;

            hWndGone = CreateWindowExW(0, L"Static", NULL, WS_POPUP,
                                       0, 0, 10, 10,
                                       NULL, NULL, NULL, NULL);
            if (hWndGone != NULL)
            {
                DestroyWindow(hWndGone);

                SetLastError(0xDEADBEEF);
                Success = GrantAccess(hWndGone, hJob);
                ok(Success == FALSE, "Granting a destroyed window succeeded\n");
                ok_err(ERROR_INVALID_PARAMETER);

                /* Revoking is refused just the same. A handle destroyed while
                   granted is taken off the list when it is freed, so there is
                   never a dead one left for the caller to revoke by hand. */
                SetLastError(0xDEADBEEF);
                Success = RevokeAccess(hWndGone, hJob);
                ok(Success == FALSE, "Revoking a destroyed window succeeded\n");
                ok_err(ERROR_INVALID_PARAMETER);
            }
        }

        /* Enough handles to make the granted list grow more than once */
        {
            HWND Windows[16];
            ULONG i;

            for (i = 0; i < _countof(Windows); i++)
            {
                Windows[i] = CreateWindowExW(0, L"Static", NULL, WS_POPUP,
                                             0, 0, 10, 10,
                                             NULL, NULL, NULL, NULL);
                if (Windows[i] == NULL)
                {
                    skip("CreateWindowEx failed with %lu\n", GetLastError());
                    break;
                }

                Success = GrantAccess(Windows[i], hJob);
                ok(Success == TRUE, "Granting window %lu failed with %lu\n",
                   i, GetLastError());
            }

            while (i-- > 0)
            {
                Success = RevokeAccess(Windows[i], hJob);
                ok(Success == TRUE, "Revoking window %lu failed with %lu\n",
                   i, GetLastError());
                DestroyWindow(Windows[i]);
            }
        }

        /* Destroying a granted window has to withdraw the grant. The list
           cannot be read from here, so this only shows it stays intact. */
        {
            HWND Windows[8];
            ULONG i;

            for (i = 0; i < _countof(Windows); i++)
            {
                Windows[i] = CreateWindowExW(0, L"Static", NULL, WS_POPUP,
                                             0, 0, 10, 10,
                                             NULL, NULL, NULL, NULL);
                if (Windows[i] == NULL)
                {
                    skip("CreateWindowEx failed with %lu\n", GetLastError());
                    break;
                }

                Success = GrantAccess(Windows[i], hJob);
                ok(Success == TRUE, "Granting window %lu failed with %lu\n",
                   i, GetLastError());
            }

            /* Destroy them without revoking first */
            while (i-- > 0)
                DestroyWindow(Windows[i]);

            /* The list has to still work afterwards */
            SetLastError(0xDEADBEEF);
            Success = GrantAccess(hWnd, hJob);
            ok(Success == TRUE, "Granting after a sweep failed with %lu\n",
               GetLastError());
            SetLastError(0xDEADBEEF);
            Success = RevokeAccess(hWnd, hJob);
            ok(Success == TRUE, "Revoking after a sweep failed with %lu\n",
               GetLastError());
        }

        /* Close the job with handles still granted, to free the list */
        Success = GrantAccess(hWnd, hJob);
        ok(Success == TRUE, "Granting failed with %lu\n", GetLastError());
        CloseHandle(hJob);
    }

    /* A handle that is not a job at all */
    SetLastError(0xDEADBEEF);
    Success = GrantAccess(hWnd, GetCurrentProcess());
    ok(Success == FALSE, "Granting against a process handle succeeded\n");

    SetLastError(0xDEADBEEF);
    Success = GrantAccess(hWnd, NULL);
    ok(Success == FALSE, "Granting against a NULL job succeeded\n");

    DestroyWindow(hWnd);
}

static
HANDLE
StartChild(_Out_ PHANDLE Thread)
{
    WCHAR FileName[MAX_PATH];
    WCHAR CommandLine[MAX_PATH];
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    GetModuleFileNameW(NULL, FileName, _countof(FileName));
    StringCbPrintfW(CommandLine,
                    sizeof(CommandLine),
                    L"\"%ls\" UserHandleGrantAccess child",
                    FileName);

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;

    if (!CreateProcessW(FileName,
                        CommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInfo))
    {
        skip("CreateProcess failed with %lu\n", GetLastError());
        *Thread = NULL;
        return NULL;
    }

    *Thread = ProcessInfo.hThread;
    return ProcessInfo.hProcess;
}

/*
 * Runs a child through a job. RestrictFirst TRUE assigns into an already
 * restricted job, so the kernel hands the process over from the assignment
 * path; FALSE restricts afterwards, so win32k has to find it itself.
 */
static
void
test_ProcessInJob(_In_ BOOL RestrictFirst)
{
    JOBOBJECT_BASIC_UI_RESTRICTIONS Info;
    HANDLE hReady, hQuit, hJob, hProcess, hThread;
    DWORD Wait;
    BOOL Success;

    hReady = CreateEventW(NULL, TRUE, FALSE, READY_EVENT);
    hQuit = CreateEventW(NULL, TRUE, FALSE, QUIT_EVENT);
    if (hReady == NULL || hQuit == NULL)
    {
        skip("CreateEvent failed with %lu\n", GetLastError());
        if (hReady) CloseHandle(hReady);
        if (hQuit) CloseHandle(hQuit);
        return;
    }

    ResetEvent(hReady);
    ResetEvent(hQuit);

    hJob = CreateRestrictedJob(RestrictFirst ? JOB_LOCKDOWN_UI : 0);
    if (hJob == NULL)
    {
        CloseHandle(hReady);
        CloseHandle(hQuit);
        return;
    }

    hProcess = StartChild(&hThread);
    if (hProcess == NULL)
    {
        CloseHandle(hJob);
        CloseHandle(hReady);
        CloseHandle(hQuit);
        return;
    }

    /* The callout is only made for a process that is a win32k client */
    Wait = WaitForSingleObject(hReady, 10000);
    ok(Wait == WAIT_OBJECT_0, "The child did not become ready, wait returned %lu\n", Wait);

    if (Wait == WAIT_OBJECT_0)
    {
        SetLastError(0xDEADBEEF);
        Success = AssignProcessToJobObject(hJob, hProcess);
        if (!Success && GetLastError() == ERROR_ACCESS_DENIED)
        {
            /* The test itself is running in a job that does not allow
               breakaway, so the child inherited it */
            skip("The test is already running in a job\n");
        }
        else
        {
            ok(Success == TRUE, "AssignProcessToJobObject failed with %lu\n",
               GetLastError());

            /* Restricted up front, the child leaves the job by exiting.
               Otherwise restrict it now and lift it again, so win32k has to
               let go of a process it still holds. */
            if (!RestrictFirst)
            {
                Info.UIRestrictionsClass = JOB_LOCKDOWN_UI;
                SetLastError(0xDEADBEEF);
                Success = SetInformationJobObject(hJob,
                                                  JobObjectBasicUIRestrictions,
                                                  &Info,
                                                  sizeof(Info));
                ok(Success == TRUE, "Restricting a populated job failed with %lu\n",
                   GetLastError());

                Info.UIRestrictionsClass = 0;
                SetLastError(0xDEADBEEF);
                Success = SetInformationJobObject(hJob,
                                                  JobObjectBasicUIRestrictions,
                                                  &Info,
                                                  sizeof(Info));
                ok(Success == TRUE, "Clearing the restrictions failed with %lu\n",
                   GetLastError());
            }
        }
    }

    SetEvent(hQuit);
    Wait = WaitForSingleObject(hProcess, 10000);
    ok(Wait == WAIT_OBJECT_0, "The child did not exit, wait returned %lu\n", Wait);
    if (Wait != WAIT_OBJECT_0)
        TerminateProcess(hProcess, 1);

    CloseHandle(hThread);
    CloseHandle(hProcess);
    CloseHandle(hJob);
    CloseHandle(hQuit);
    CloseHandle(hReady);
}

/* The child: become a win32k client, say so, and wait to be let go */
static
void
RunChild(void)
{
    HANDLE hReady, hQuit;

    /* Any USER call connects us to win32k */
    GetDesktopWindow();

    hReady = OpenEventW(EVENT_MODIFY_STATE, FALSE, READY_EVENT);
    hQuit = OpenEventW(SYNCHRONIZE, FALSE, QUIT_EVENT);

    if (hReady != NULL)
    {
        SetEvent(hReady);
        CloseHandle(hReady);
    }

    if (hQuit != NULL)
    {
        WaitForSingleObject(hQuit, 30000);
        CloseHandle(hQuit);
    }
}

START_TEST(UserHandleGrantAccess)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3 && !strcmp(argv[2], "child"))
    {
        RunChild();
        return;
    }

    test_GrantAccess();
    test_ProcessInJob(TRUE);
    test_ProcessInJob(FALSE);
}
