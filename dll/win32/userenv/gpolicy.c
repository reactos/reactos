/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/gpolicy.c
 * PURPOSE:         Group policy functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>


typedef struct _GP_NOTIFY
{
    struct _GP_NOTIFY *Next;
    HANDLE hEvent;
    BOOL bMachine;
} GP_NOTIFY, *PGP_NOTIFY;

typedef enum
{
    gpaUpdate = 0,
    gpaTerminate
} GP_ACTION;

static const WCHAR szLocalGPApplied[] = L"userenv: User Group Policy has been applied";
static const WCHAR szLocalGPMutex[] = L"userenv: user policy mutex";
static const WCHAR szLocalGPRefreshEvent[] = L"userenv: user policy refresh event";
static const WCHAR szLocalGPForceRefreshEvent[] = L"userenv: user policy force refresh event";
static const WCHAR szLocalGPDoneEvent[] = L"userenv: User Policy Foreground Done Event";
static const WCHAR szMachineGPApplied[] = L"Global\\userenv: Machine Group Policy has been applied";
static const WCHAR szMachineGPMutex[] = L"Global\\userenv: machine policy mutex";
static const WCHAR szMachineGPRefreshEvent[] = L"Global\\userenv: machine policy refresh event";
static const WCHAR szMachineGPForceRefreshEvent[] = L"Global\\userenv: machine policy force refresh event";
static const WCHAR szMachineGPDoneEvent[] = L"Global\\userenv: Machine Policy Foreground Done Event";

static CRITICAL_SECTION GPNotifyLock;
static PGP_NOTIFY NotificationList = NULL;
static GP_ACTION GPNotificationAction = gpaUpdate;
static HANDLE hNotificationThread = NULL;
static HANDLE hNotificationThreadEvent = NULL;
static HANDLE hLocalGPAppliedEvent = NULL;
static HANDLE hMachineGPAppliedEvent = NULL;

VOID
InitializeGPNotifications(VOID)
{
    InitializeCriticalSection(&GPNotifyLock);
}

VOID
UninitializeGPNotifications(VOID)
{
    EnterCriticalSection(&GPNotifyLock);

    /* rundown the notification thread */
    if (hNotificationThread != NULL)
    {
        ASSERT(hNotificationThreadEvent != NULL);

        /* notify the thread */
        GPNotificationAction = gpaTerminate;
        SetEvent(hNotificationThreadEvent);

        LeaveCriticalSection(&GPNotifyLock);

        /* wait for the thread to terminate itself */
        WaitForSingleObject(hNotificationThread,
                            INFINITE);

        EnterCriticalSection(&GPNotifyLock);

        if (hNotificationThread != NULL)
        {
            /* the handle should be closed by the thread,
               just in case that didn't happen for an unknown reason */
            CloseHandle(hNotificationThread);
            hNotificationThread = NULL;
        }
    }

    if (hNotificationThreadEvent != NULL)
    {
        CloseHandle(hNotificationThreadEvent);
        hNotificationThreadEvent = NULL;
    }

    LeaveCriticalSection(&GPNotifyLock);

    DeleteCriticalSection(&GPNotifyLock);
}

static VOID
NotifyGPEvents(IN BOOL bMachine)
{
    PGP_NOTIFY Notify = NotificationList;

    while (Notify != NULL)
    {
        if (Notify->bMachine == bMachine)
        {
            SetEvent(Notify->hEvent);
        }

        Notify = Notify->Next;
    }
}

static DWORD WINAPI
GPNotificationThreadProc(IN LPVOID lpParameter)
{
    HMODULE hModule;
    DWORD WaitResult, WaitCount;
    HANDLE WaitHandles[3];

    /* reference the library so we don't screw up if the application
       causes the DLL to unload while this thread is still running */
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           (LPCWSTR)hInstance,
                           &hModule))
    {
        ASSERT(hModule == hInstance);

        EnterCriticalSection(&GPNotifyLock);

        ASSERT(hNotificationThreadEvent != NULL);
        WaitHandles[0] = hNotificationThreadEvent;
        for (;;)
        {
            ASSERT(hMachineGPAppliedEvent != NULL);

            if (NotificationList == NULL)
                break;

            WaitCount = 2;
            WaitHandles[1] = hMachineGPAppliedEvent;

            if (hLocalGPAppliedEvent != NULL)
            {
                WaitHandles[2] = hLocalGPAppliedEvent;
                WaitCount++;
            }

            LeaveCriticalSection(&GPNotifyLock);

            WaitResult = WaitForMultipleObjects(WaitCount,
                                                WaitHandles,
                                                FALSE,
                                                INFINITE);

            EnterCriticalSection(&GPNotifyLock);

            if (WaitResult != WAIT_FAILED)
            {
                if (WaitResult == WAIT_OBJECT_0)
                {
                    ResetEvent(hNotificationThreadEvent);

                    if (GPNotificationAction == gpaTerminate)
                    {
                        /* terminate the thread */
                        break;
                    }
                }
                else if (WaitResult == WAIT_OBJECT_0 + 1 || WaitResult == WAIT_OBJECT_0 + 2)
                {
                    /* group policies have been applied */
                    if (NotificationList != NULL)
                    {
                        NotifyGPEvents((WaitResult == WAIT_OBJECT_0 + 1));
                    }
                }
                else if (WaitResult == WAIT_ABANDONED_0 + 2)
                {
                    /* In case the local group policies event was abandoned, keep watching!
                       But close the handle as it's no longer of any use. */
                    if (hLocalGPAppliedEvent != NULL)
                    {
                        CloseHandle(hLocalGPAppliedEvent);
                        hLocalGPAppliedEvent = NULL;
                    }
                }
                else if (WaitResult == WAIT_ABANDONED_0 || WaitResult == WAIT_ABANDONED_0 + 1)
                {
                    /* terminate the thread if the machine group policies event was abandoned
                       or for some reason the rundown event got abandoned. */
                    break;
                }
                else
                {
                    DPRINT("Unexpected wait result watching the group policy events: 0x%x\n", WaitResult);
                    ASSERT(FALSE);
                    break;
                }

                if (NotificationList == NULL)
                    break;
            }
            else
                break;

        }

        /* cleanup handles no longer used */
        ASSERT(hNotificationThread != NULL);
        ASSERT(hNotificationThreadEvent != NULL);

        CloseHandle(hNotificationThread);
        CloseHandle(hNotificationThreadEvent);
        hNotificationThread = NULL;
        hNotificationThreadEvent = NULL;

        if (hLocalGPAppliedEvent != NULL)
        {
            CloseHandle(hLocalGPAppliedEvent);
            hLocalGPAppliedEvent = NULL;
        }
        if (hMachineGPAppliedEvent != NULL)
        {
            CloseHandle(hMachineGPAppliedEvent);
            hMachineGPAppliedEvent = NULL;
        }

        LeaveCriticalSection(&GPNotifyLock);

        /* dereference the library and exit */
        FreeLibraryAndExitThread(hModule,
                                 0);
    }
    else
    {
        DPRINT1("Referencing the library failed!\n");
    }

    return 1;
}

static HANDLE
CreateGPEvent(IN BOOL bMachine,
              IN PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    HANDLE hEvent;
    SECURITY_ATTRIBUTES SecurityAttributes;

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor = lpSecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    hEvent = CreateEventW(&SecurityAttributes,
                          TRUE,
                          FALSE,
                          (bMachine ? szMachineGPApplied : szLocalGPApplied));

    return hEvent;
}

BOOL WINAPI
RegisterGPNotification(IN HANDLE hEvent,
                       IN BOOL bMachine)
{
    PGP_NOTIFY Notify;
    PSECURITY_DESCRIPTOR lpSecurityDescriptor = NULL;
    BOOL Ret = FALSE;

    EnterCriticalSection(&GPNotifyLock);

    /* create the thread notification event */
    if (hNotificationThreadEvent == NULL)
    {
        hNotificationThreadEvent = CreateEvent(NULL,
                                               TRUE,
                                               FALSE,
                                               NULL);
        if (hNotificationThreadEvent == NULL)
        {
            goto Cleanup;
        }
    }

    /* create or open the machine group policy event */
    if (hMachineGPAppliedEvent == NULL)
    {
        lpSecurityDescriptor = CreateDefaultSecurityDescriptor();
        if (lpSecurityDescriptor == NULL)
        {
            goto Cleanup;
        }

        hMachineGPAppliedEvent = CreateGPEvent(TRUE,
                                               lpSecurityDescriptor);
        if (hMachineGPAppliedEvent == NULL)
        {
            goto Cleanup;
        }
    }

    /* create or open the local group policy event only if necessary */
    if (!bMachine && hLocalGPAppliedEvent == NULL)
    {
        if (lpSecurityDescriptor == NULL)
        {
            lpSecurityDescriptor = CreateDefaultSecurityDescriptor();
            if (lpSecurityDescriptor == NULL)
            {
                goto Cleanup;
            }
        }

        hLocalGPAppliedEvent = CreateGPEvent(FALSE,
                                             lpSecurityDescriptor);
        if (hLocalGPAppliedEvent == NULL)
        {
            goto Cleanup;
        }
    }

    if (hNotificationThread == NULL)
    {
        hNotificationThread = CreateThread(NULL,
                                           0,
                                           GPNotificationThreadProc,
                                           NULL,
                                           0,
                                           NULL);
    }

    if (hNotificationThread != NULL)
    {
        Notify = (PGP_NOTIFY)LocalAlloc(LMEM_FIXED,
                                        sizeof(GP_NOTIFY));
        if (Notify != NULL)
        {
            /* add the item to the beginning of the list */
            Notify->Next = NotificationList;
            Notify->hEvent = hEvent;
            Notify->bMachine = bMachine;

            NotificationList = Notify;

            /* notify the thread */
            GPNotificationAction = gpaUpdate;
            SetEvent(hNotificationThreadEvent);

            Ret = TRUE;
        }
    }

Cleanup:
    LeaveCriticalSection(&GPNotifyLock);

    if (lpSecurityDescriptor != NULL)
    {
        LocalFree((HLOCAL)lpSecurityDescriptor);
    }

    /* NOTE: don't delete the events or close the handles created */

    return Ret;
}

BOOL WINAPI
UnregisterGPNotification(IN HANDLE hEvent)
{
    PGP_NOTIFY Notify = NULL, *NotifyLink;
    BOOL Ret = FALSE;

    EnterCriticalSection(&GPNotifyLock);

    Notify = NotificationList;
    NotifyLink = &NotificationList;

    while (Notify != NULL)
    {
        if (Notify->hEvent == hEvent)
        {
            /* remove and free the item */
            *NotifyLink = Notify->Next;
            LocalFree((HLOCAL)Notify);

            /* notify the thread */
            if (hNotificationThread != NULL &&
                hNotificationThreadEvent != NULL)
            {
                GPNotificationAction = gpaUpdate;
                SetEvent(hNotificationThreadEvent);
            }

            Ret = TRUE;
            break;
        }

        NotifyLink = &Notify->Next;
        Notify = Notify->Next;
    }

    LeaveCriticalSection(&GPNotifyLock);

    return Ret;
}

BOOL WINAPI
RefreshPolicy(IN BOOL bMachine)
{
    HANDLE hEvent;
    BOOL Ret = TRUE;

    hEvent = OpenEventW(EVENT_MODIFY_STATE,
                        FALSE,
                        (bMachine ? szMachineGPRefreshEvent : szLocalGPRefreshEvent));
    if (hEvent != NULL)
    {
        Ret = SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    /* return TRUE even if the mutex doesn't exist! */
    return Ret;
}

BOOL WINAPI
RefreshPolicyEx(IN BOOL bMachine,
                IN DWORD dwOptions)
{
    if (dwOptions & ~RP_FORCE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (dwOptions & RP_FORCE)
    {
        HANDLE hEvent;
        BOOL Ret = TRUE;

        hEvent = OpenEventW(EVENT_MODIFY_STATE,
                            FALSE,
                            (bMachine ? szMachineGPForceRefreshEvent : szLocalGPForceRefreshEvent));
        if (hEvent != NULL)
        {
            Ret = SetEvent(hEvent);
            CloseHandle(hEvent);
        }

        /* return TRUE even if the mutex doesn't exist! */
        return Ret;
    }
    else
    {
        return RefreshPolicy(bMachine);
    }
}

HANDLE WINAPI
EnterCriticalPolicySection(IN BOOL bMachine)
{
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_DESCRIPTOR lpSecurityDescriptor;
    HANDLE hSection;

    /* create or open the mutex */
    lpSecurityDescriptor = CreateDefaultSecurityDescriptor();
    if (lpSecurityDescriptor != NULL)
    {
        SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecurityAttributes.lpSecurityDescriptor = lpSecurityDescriptor;
        SecurityAttributes.bInheritHandle = FALSE;

        hSection = CreateMutexW(&SecurityAttributes,
                                FALSE,
                                (bMachine ? szMachineGPMutex : szLocalGPMutex));

        LocalFree((HLOCAL)lpSecurityDescriptor);

        if (hSection != NULL)
        {
            /* wait up to 10 minutes */
            if (WaitForSingleObject(hSection,
                                    600000) != WAIT_FAILED)
            {
                return hSection;
            }

            CloseHandle(hSection);
        }
    }

    return NULL;
}

BOOL WINAPI
LeaveCriticalPolicySection(IN HANDLE hSection)
{
    BOOL Ret;

    if (hSection == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Ret = ReleaseMutex(hSection);
    CloseHandle(hSection);

    return Ret;
}

BOOL WINAPI
WaitForUserPolicyForegroundProcessing(VOID)
{
    HANDLE hEvent;
    BOOL Ret = FALSE;

    hEvent = OpenEventW(SYNCHRONIZE,
                        FALSE,
                        szLocalGPDoneEvent);
    if (hEvent != NULL)
    {
        Ret = WaitForSingleObject(hEvent,
                                  INFINITE) != WAIT_FAILED;
        CloseHandle(hEvent);
    }

    return Ret;
}

BOOL WINAPI
WaitForMachinePolicyForegroundProcessing(VOID)
{
    HANDLE hEvent;
    BOOL Ret = FALSE;

    hEvent = OpenEventW(SYNCHRONIZE,
                        FALSE,
                        szMachineGPDoneEvent);
    if (hEvent != NULL)
    {
        Ret = WaitForSingleObject(hEvent,
                                  INFINITE) != WAIT_FAILED;
        CloseHandle(hEvent);
    }

    return Ret;
}
