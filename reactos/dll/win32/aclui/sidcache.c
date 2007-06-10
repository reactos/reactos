/*
 * ReactOS Access Control List Editor
 * Copyright (C) 2004-2005 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id: aclui.c 19715 2005-11-28 01:10:49Z weiden $
 *
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/aclui/sidcache.c
 * PURPOSE:         Access Control List Editor
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      12/10/2005  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define HandleToScm(Handle) (PSIDCACHEMGR)(Handle)
#define ScmToHandle(Scm) (HANDLE)(Scm)

typedef struct _SIDCACHEMGR
{
    volatile LONG RefCount;
    LSA_HANDLE LsaHandle;
    CRITICAL_SECTION Lock;
    LIST_ENTRY QueueListHead;
    struct _SIDQUEUEENTRY *QueueLookingUp;
    LIST_ENTRY CacheListHead;
    HANDLE Heap;
    HANDLE LookupEvent;
    HANDLE LookupThread;
    WCHAR SystemName[1];
} SIDCACHEMGR, *PSIDCACHEMGR;


typedef struct _SIDCACHECALLBACKINFO
{
    PSIDREQCOMPLETIONPROC CompletionProc;
    PVOID Context;
} SIDCACHECALLBACKINFO, *PSIDCACHECALLBACKINFO;


typedef struct _SIDQUEUEENTRY
{
    LIST_ENTRY ListEntry;
    ULONG CallbackCount;
    PSIDCACHECALLBACKINFO Callbacks;
    /* the SID is appended to this structure */
} SIDQUEUEENTRY, *PSIDQUEUEENTRY;


typedef struct _SIDCACHEENTRY
{
    LIST_ENTRY ListEntry;
    SID_NAME_USE SidNameUse;
    PWSTR AccountName;
    PWSTR DomainName;
    /* the SID and strings are appended to this structure */
} SIDCACHEENTRY, *PSIDCACHEENTRY;


static VOID
FreeQueueEntry(IN PSIDCACHEMGR scm,
               IN PSIDQUEUEENTRY QueueEntry)
{
    if (QueueEntry->ListEntry.Flink != NULL)
    {
        RemoveEntryList(&QueueEntry->ListEntry);
    }

    HeapFree(scm->Heap,
             0,
             QueueEntry->Callbacks);

    HeapFree(scm->Heap,
             0,
             QueueEntry);
}


static VOID
FreeCacheEntry(IN PSIDCACHEMGR scm,
               IN PSIDCACHEENTRY CacheEntry)
{
    RemoveEntryList(&CacheEntry->ListEntry);

    HeapFree(scm->Heap,
             0,
             CacheEntry);
}


static VOID
CleanupSidCacheMgr(IN PSIDCACHEMGR scm)
{
    LsaClose(scm->LsaHandle);
    CloseHandle(scm->LookupEvent);
    CloseHandle(scm->LookupThread);

    /* delete the queue */
    while (!IsListEmpty(&scm->QueueListHead))
    {
        PSIDQUEUEENTRY QueueEntry;

        QueueEntry = CONTAINING_RECORD(scm->QueueListHead.Flink,
                                       SIDQUEUEENTRY,
                                       ListEntry);
        FreeQueueEntry(scm,
                       QueueEntry);
    }

    /* delete the cache */
    while (!IsListEmpty(&scm->CacheListHead))
    {
        PSIDCACHEENTRY CacheEntry;

        CacheEntry = CONTAINING_RECORD(scm->CacheListHead.Flink,
                                       SIDCACHEENTRY,
                                       ListEntry);
        FreeCacheEntry(scm,
                       CacheEntry);
    }

    DeleteCriticalSection(&scm->Lock);
}


static PSIDCACHEMGR
ReferenceSidCacheMgr(IN HANDLE SidCacheMgr)
{
    PSIDCACHEMGR scm = HandleToScm(SidCacheMgr);

    if (InterlockedIncrement(&scm->RefCount) != 1)
    {
        return scm;
    }

    return NULL;
}


static VOID
DereferenceSidCacheMgr(IN PSIDCACHEMGR scm)
{
    if (InterlockedDecrement(&scm->RefCount) == 0)
    {
        /* Signal the lookup thread so it can terminate */
        SetEvent(scm->LookupEvent);
    }
}


static BOOL
OpenLSAPolicyHandle(IN LPWSTR SystemName,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PLSA_HANDLE PolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES LsaObjectAttributes = {0};
    LSA_UNICODE_STRING LsaSystemName, *psn;
    NTSTATUS Status;

    if (SystemName != NULL && SystemName[0] != L'\0')
    {
        LsaSystemName.Buffer = SystemName;
        LsaSystemName.Length = wcslen(SystemName) * sizeof(WCHAR);
        LsaSystemName.MaximumLength = LsaSystemName.Length + sizeof(WCHAR);
        psn = &LsaSystemName;
    }
    else
    {
        psn = NULL;
    }

    Status = LsaOpenPolicy(psn,
                           &LsaObjectAttributes,
                           DesiredAccess,
                           PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(LsaNtStatusToWinError(Status));
        return FALSE;
    }

    return TRUE;
}


static BOOL
LookupSidInformation(IN PSIDCACHEMGR scm,
                     IN PSID pSid,
                     OUT PSIDREQRESULT *ReqResult)
{
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomain;
    PLSA_TRANSLATED_NAME Names;
    PLSA_TRUST_INFORMATION Domain;
    PLSA_UNICODE_STRING DomainName;
    SID_NAME_USE SidNameUse = SidTypeUnknown;
    PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo = NULL;
    NTSTATUS Status;
    DWORD SidLength, AccountNameSize, DomainNameSize = 0;
    PSIDREQRESULT ReqRet = NULL;
    BOOL Ret = FALSE;

    Status = LsaLookupSids(scm->LsaHandle,
                           1,
                           &pSid,
                           &ReferencedDomain,
                           &Names);
    if (NT_SUCCESS(Status))
    {
        SidLength = GetLengthSid(pSid);
        SidNameUse = Names->Use;

        if (ReferencedDomain != NULL &&
            Names->DomainIndex >= 0)
        {
            Domain = &ReferencedDomain->Domains[Names->DomainIndex];
            DomainName = &Domain->Name;
        }
        else
        {
            Domain = NULL;
            DomainName = NULL;
        }

        switch (SidNameUse)
        {
            case SidTypeAlias:
            {
                if (Domain != NULL)
                {
                    /* query the domain name for BUILTIN accounts */
                    Status = LsaQueryInformationPolicy(scm->LsaHandle,
                                                       PolicyAccountDomainInformation,
                                                       (PVOID*)&PolicyAccountDomainInfo);
                    if (NT_SUCCESS(Status))
                    {
                        DomainName = &PolicyAccountDomainInfo->DomainName;

                        /* make the user believe this is a group */
                        SidNameUse = (PolicyAccountDomainInfo != NULL ? SidTypeGroup : SidTypeUser);
                    }
                }
                break;
            }

            default:
            {
                DPRINT("Unhandled SID type: 0x%x\n", Names->Use);
                break;
            }
        }

        AccountNameSize = Names->Name.Length;
        if (DomainName != NULL)
        {
            DomainNameSize = DomainName->Length;
        }

        ReqRet = HeapAlloc(scm->Heap,
                           0,
                           sizeof(SIDREQRESULT) +
                               (((AccountNameSize + DomainNameSize) + 2) * sizeof(WCHAR)));
        if (ReqRet != NULL)
        {
            ReqRet->RefCount = 1;
            ReqRet->AccountName = (LPWSTR)(ReqRet + 1);
            ReqRet->DomainName = ReqRet->AccountName + (AccountNameSize / sizeof(WCHAR)) + 1;

            CopyMemory(ReqRet->AccountName,
                       Names->Name.Buffer,
                       Names->Name.Length);

            if (DomainName != NULL)
            {
                CopyMemory(ReqRet->DomainName,
                           DomainName->Buffer,
                           DomainName->Length);
            }

            ReqRet->AccountName[AccountNameSize / sizeof(WCHAR)] = L'\0';
            ReqRet->DomainName[DomainNameSize / sizeof(WCHAR)] = L'\0';

            ReqRet->SidNameUse = SidNameUse;
        }

        if (PolicyAccountDomainInfo != NULL)
        {
            LsaFreeMemory(PolicyAccountDomainInfo);
        }

        LsaFreeMemory(ReferencedDomain);
        LsaFreeMemory(Names);

        Ret = TRUE;
    }
    else if (Status == STATUS_NONE_MAPPED)
    {
        Ret = TRUE;
    }

    if (Ret)
    {
        *ReqResult = ReqRet;
    }

    return Ret;
}


static BOOL
FindSidInCache(IN PSIDCACHEMGR scm,
               IN PSID pSid,
               OUT PSIDREQRESULT *ReqResult)
{
    PSIDCACHEENTRY CacheEntry;
    PLIST_ENTRY CurrentEntry;
    PSIDREQRESULT ReqRes;
    BOOL Ret = FALSE;

    /* NOTE: assumes the lists are locked! */

    CurrentEntry = &scm->CacheListHead;
    while (CurrentEntry != &scm->CacheListHead)
    {
        CacheEntry = CONTAINING_RECORD(CurrentEntry,
                                       SIDCACHEENTRY,
                                       ListEntry);

        if (EqualSid(pSid,
                     (PSID)(CacheEntry + 1)))
        {
            SIZE_T ReqResultSize;
            ULONG AccountNameLen, DomainNameLen;

            Ret = TRUE;

            AccountNameLen = wcslen(CacheEntry->AccountName);
            DomainNameLen = wcslen(CacheEntry->DomainName);

            ReqResultSize = sizeof(SIDREQRESULT) +
                                (((AccountNameLen + 1) +
                                  (DomainNameLen + 1)) * sizeof(WCHAR));

            ReqRes = HeapAlloc(scm->Heap,
                               0,
                               ReqResultSize);
            if (ReqRes != NULL)
            {
                PWSTR Buffer = (PWSTR)(ReqRes + 1);

                ReqRes->RefCount = 1;

                ReqRes->AccountName = Buffer;
                wcscpy(ReqRes->AccountName,
                       CacheEntry->AccountName);
                Buffer += AccountNameLen + 1;

                ReqRes->DomainName = Buffer;
                wcscpy(ReqRes->DomainName,
                       CacheEntry->DomainName);
            }

            /* return the result, even if we weren't unable to
               allocate enough memory! */
            *ReqResult = ReqRes;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    return Ret;
}


static VOID
CacheLookupResults(IN PSIDCACHEMGR scm,
                   IN PSID pSid,
                   IN PSIDREQRESULT ReqResult)
{
    PSIDCACHEENTRY CacheEntry;
    DWORD SidLen;
    SIZE_T AccountNameLen = 0;
    SIZE_T DomainNameLen = 0;
    SIZE_T CacheEntrySize = sizeof(SIDCACHEENTRY);

    /* NOTE: assumes the lists are locked! */

    SidLen = GetLengthSid(pSid);
    CacheEntrySize += SidLen;

    AccountNameLen = wcslen(ReqResult->AccountName);
    CacheEntrySize += (AccountNameLen + 1) * sizeof(WCHAR);

    DomainNameLen = wcslen(ReqResult->DomainName);
    CacheEntrySize += (wcslen(ReqResult->DomainName) + 1) * sizeof(WCHAR);

    CacheEntry = HeapAlloc(scm->Heap,
                           0,
                           CacheEntrySize);
    if (CacheEntry != NULL)
    {
        PWSTR lpBuf = (PWSTR)((ULONG_PTR)(CacheEntry + 1) + SidLen);

        CacheEntry->SidNameUse = ReqResult->SidNameUse;

        /* append the SID */
        CopySid(SidLen,
                (PSID)(CacheEntry + 1),
                pSid);

        /* append the strings */
        CacheEntry->AccountName = lpBuf;
        wcscpy(lpBuf,
               ReqResult->AccountName);
        lpBuf += AccountNameLen + 1;

        CacheEntry->DomainName = lpBuf;
        wcscpy(lpBuf,
               ReqResult->DomainName);
        lpBuf += DomainNameLen + 1;

        /* add the entry to the cache list */
        InsertTailList(&scm->CacheListHead,
                       &CacheEntry->ListEntry);
    }
}


static DWORD WINAPI
LookupThreadProc(IN LPVOID lpParameter)
{
    HMODULE hModule;
    PSIDCACHEMGR scm = (PSIDCACHEMGR)lpParameter;

    /* Reference the dll to avoid problems in case of accidental
       FreeLibrary calls... */
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            (LPCWSTR)hDllInstance,
                            &hModule))
    {
        hModule = NULL;
    }

    while (scm->RefCount != 0)
    {
        PSIDQUEUEENTRY QueueEntry = NULL;

        EnterCriticalSection(&scm->Lock);

        /* get the first item of the queue */
        if (scm->QueueListHead.Flink != &scm->QueueListHead)
        {
            QueueEntry = CONTAINING_RECORD(scm->QueueListHead.Flink,
                                           SIDQUEUEENTRY,
                                           ListEntry);
            RemoveEntryList(&QueueEntry->ListEntry);
            QueueEntry->ListEntry.Flink = NULL;
        }
        else
        {
            LeaveCriticalSection(&scm->Lock);

            /* wait for the next asynchronous lookup queued */
            WaitForSingleObject(scm->LookupEvent,
                                INFINITE);
            continue;
        }

        scm->QueueLookingUp = QueueEntry;

        LeaveCriticalSection(&scm->Lock);

        if (QueueEntry != NULL)
        {
            PSIDREQRESULT ReqResult, FoundReqResult;
            PSID pSid = (PSID)(QueueEntry + 1);

            /* lookup the SID information */
            if (!LookupSidInformation(scm,
                                      pSid,
                                      &ReqResult))
            {
                ReqResult = NULL;
            }

            EnterCriticalSection(&scm->Lock);

            /* see if the SID was added to the cache in the meanwhile */
            if (!FindSidInCache(scm,
                                pSid,
                                &FoundReqResult))
            {
                if (ReqResult != NULL)
                {
                    /* cache the results */
                    CacheLookupResults(scm,
                                       pSid,
                                       ReqResult);
                }
            }
            else
            {
                if (ReqResult != NULL)
                {
                    /* free the information of our lookup and use the cached
                       information*/
                    DereferenceSidReqResult(scm,
                                            ReqResult);
                }

                ReqResult = FoundReqResult;
            }

            /* notify the callers unless the lookup was cancelled */
            if (scm->QueueLookingUp != NULL)
            {
                ULONG i = 0;

                while (scm->QueueLookingUp != NULL &&
                       i < QueueEntry->CallbackCount)
                {
                    PVOID Context;
                    PSIDREQCOMPLETIONPROC CompletionProc;

                    Context = QueueEntry->Callbacks[i].Context;
                    CompletionProc = QueueEntry->Callbacks[i].CompletionProc;

                    LeaveCriticalSection(&scm->Lock);

                    /* call the completion proc without holding the lock! */
                    CompletionProc(ScmToHandle(scm),
                                   pSid,
                                   ReqResult,
                                   Context);

                    EnterCriticalSection(&scm->Lock);

                    i++;
                }

                scm->QueueLookingUp = NULL;
            }

            LeaveCriticalSection(&scm->Lock);

            /* free the queue item */
            FreeQueueEntry(scm,
                           QueueEntry);
        }
    }

    CleanupSidCacheMgr(scm);

    HeapFree(scm->Heap,
             0,
             scm);

    if (hModule != NULL)
    {
        /* dereference the library and exit */
        FreeLibraryAndExitThread(hModule,
                                 0);
    }

    return 0;
}



HANDLE
CreateSidCacheMgr(IN HANDLE Heap,
                  IN LPCWSTR SystemName)
{
    PSIDCACHEMGR scm;

    if (SystemName == NULL)
        SystemName = L"";

    scm = HeapAlloc(Heap,
                    0,
                    FIELD_OFFSET(SIDCACHEMGR,
                                 SystemName[wcslen(SystemName) + 1]));
    if (scm != NULL)
    {
        /* zero the static part of the structure */
        ZeroMemory(scm,
                   FIELD_OFFSET(SIDCACHEMGR,
                                SystemName));

        scm->RefCount = 1;
        scm->Heap = Heap;

        wcscpy(scm->SystemName,
               SystemName);

        InitializeCriticalSection(&scm->Lock);
        InitializeListHead(&scm->QueueListHead);
        InitializeListHead(&scm->CacheListHead);

        scm->LookupEvent = CreateEvent(NULL,
                                       FALSE,
                                       FALSE,
                                       NULL);
        if (scm->LookupEvent == NULL)
        {
            goto Cleanup;
        }

        if (!OpenLSAPolicyHandle(scm->SystemName,
                                 POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                                 &scm->LsaHandle))
        {
            goto Cleanup;
        }

        scm->LookupThread = CreateThread(NULL,
                                         0,
                                         LookupThreadProc,
                                         scm,
                                         0,
                                         NULL);
        if (scm->LookupThread == NULL)
        {
Cleanup:
            if (scm->LookupEvent != NULL)
            {
                CloseHandle(scm->LookupEvent);
            }

            if (scm->LsaHandle != NULL)
            {
                LsaClose(scm->LsaHandle);
            }

            HeapFree(Heap,
                     0,
                     scm);
            scm = NULL;
        }
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return (HANDLE)scm;
}


VOID
DestroySidCacheMgr(IN HANDLE SidCacheMgr)
{
    PSIDCACHEMGR scm = HandleToScm(SidCacheMgr);

    if (scm != NULL)
    {
        /* remove the keep-alive reference */
        DereferenceSidCacheMgr(scm);
    }
}


static BOOL
QueueSidLookup(IN PSIDCACHEMGR scm,
               IN PSID pSid,
               IN PSIDREQCOMPLETIONPROC CompletionProc,
               IN PVOID Context)
{
    PLIST_ENTRY CurrentEntry;
    PSIDQUEUEENTRY QueueEntry, FoundEntry = NULL;
    BOOL Ret = FALSE;

    /* NOTE: assumes the lists are locked! */

    if (scm->QueueLookingUp != NULL &&
        EqualSid(pSid,
                 (PSID)(scm->QueueLookingUp + 1)))
    {
        FoundEntry = scm->QueueLookingUp;
    }
    else
    {
        CurrentEntry = &scm->QueueListHead;
        while (CurrentEntry != &scm->QueueListHead)
        {
            QueueEntry = CONTAINING_RECORD(CurrentEntry,
                                           SIDQUEUEENTRY,
                                           ListEntry);

            if (EqualSid(pSid,
                         (PSID)(QueueEntry + 1)))
            {
                FoundEntry = QueueEntry;
                break;
            }

            CurrentEntry = CurrentEntry->Flink;
        }
    }

    if (FoundEntry == NULL)
    {
        DWORD SidLength = GetLengthSid(pSid);

        FoundEntry = HeapAlloc(scm->Heap,
                               0,
                               sizeof(SIDQUEUEENTRY) + SidLength);
        if (FoundEntry != NULL)
        {
            CopySid(SidLength,
                    (PSID)(FoundEntry + 1),
                    pSid);

            FoundEntry->CallbackCount = 1;
            FoundEntry->Callbacks = HeapAlloc(scm->Heap,
                                              0,
                                              sizeof(SIDCACHECALLBACKINFO));

            if (FoundEntry->Callbacks != NULL)
            {
                FoundEntry->Callbacks[0].CompletionProc = CompletionProc;
                FoundEntry->Callbacks[0].Context = Context;

                /* append it to the queue */
                InsertTailList(&scm->QueueListHead,
                               &FoundEntry->ListEntry);

                /* signal the lookup event */
                SetEvent(scm->LookupEvent);

                Ret = TRUE;
            }
            else
            {
                /* unable to queue it because we couldn't allocate the callbacks
                   array, free the memory and return */
                HeapFree(scm->Heap,
                         0,
                         FoundEntry);
            }
        }
    }
    else
    {
        PSIDCACHECALLBACKINFO Sidccb;

        /* add the callback */
        Sidccb = HeapReAlloc(scm->Heap,
                             0,
                             FoundEntry->Callbacks,
                             (FoundEntry->CallbackCount + 1) * sizeof(SIDCACHECALLBACKINFO));
        if (Sidccb != NULL)
        {
            FoundEntry->Callbacks = Sidccb;
            FoundEntry->Callbacks[FoundEntry->CallbackCount].CompletionProc = CompletionProc;
            FoundEntry->Callbacks[FoundEntry->CallbackCount++].Context = Context;

            Ret = TRUE;
        }
    }

    return Ret;
}


VOID
DequeueSidLookup(IN HANDLE SidCacheMgr,
                 IN PSID pSid)
{
    PLIST_ENTRY CurrentEntry;
    PSIDQUEUEENTRY QueueEntry;
    PSIDCACHEMGR scm;

    scm = ReferenceSidCacheMgr(SidCacheMgr);
    if (scm != NULL)
    {
        EnterCriticalSection(&scm->Lock);

        if (scm->QueueLookingUp != NULL &&
            EqualSid(pSid,
                     (PSID)(scm->QueueLookingUp + 1)))
        {
            /* don't free the queue lookup item! this will be
               done in the lookup thread */
            scm->QueueLookingUp = NULL;
        }
        else
        {
            CurrentEntry = &scm->QueueListHead;
            while (CurrentEntry != &scm->QueueListHead)
            {
                QueueEntry = CONTAINING_RECORD(CurrentEntry,
                                               SIDQUEUEENTRY,
                                               ListEntry);

                if (EqualSid(pSid,
                             (PSID)(QueueEntry + 1)))
                {
                    FreeQueueEntry(scm,
                                   QueueEntry);
                    break;
                }

                CurrentEntry = CurrentEntry->Flink;
            }
        }

        LeaveCriticalSection(&scm->Lock);

        DereferenceSidCacheMgr(scm);
    }
}


VOID
ReferenceSidReqResult(IN HANDLE SidCacheMgr,
                      IN PSIDREQRESULT ReqResult)
{
    PSIDCACHEMGR scm;

    scm = ReferenceSidCacheMgr(SidCacheMgr);
    if (scm != NULL)
    {
        InterlockedIncrement(&ReqResult->RefCount);

        DereferenceSidCacheMgr(scm);
    }
}


VOID
DereferenceSidReqResult(IN HANDLE SidCacheMgr,
                        IN PSIDREQRESULT ReqResult)
{
    PSIDCACHEMGR scm;

    scm = ReferenceSidCacheMgr(SidCacheMgr);
    if (scm != NULL)
    {
        if (InterlockedDecrement(&ReqResult->RefCount) == 0)
        {
            HeapFree(scm->Heap,
                     0,
                     ReqResult);
        }

        DereferenceSidCacheMgr(scm);
    }
}


BOOL
LookupSidCache(IN HANDLE SidCacheMgr,
               IN PSID pSid,
               IN PSIDREQCOMPLETIONPROC CompletionProc,
               IN PVOID Context)
{
    BOOL Found = FALSE;
    PSIDREQRESULT ReqResult = NULL;
    PSIDCACHEMGR scm;

    scm = ReferenceSidCacheMgr(SidCacheMgr);
    if (scm != NULL)
    {
        EnterCriticalSection(&scm->Lock);

        /* search the cache */
        Found = FindSidInCache(scm,
                               pSid,
                               &ReqResult);

        if (!Found)
        {
            /* the sid is not in the cache, queue it if not already queued */
            if (!QueueSidLookup(scm,
                                pSid,
                                CompletionProc,
                                Context))
            {
                PSIDREQRESULT FoundReqResult = NULL;

                /* unable to queue it, look it up now */

                LeaveCriticalSection(&scm->Lock);

                /* lookup everything we need */
                if (!LookupSidInformation(scm,
                                          pSid,
                                          &ReqResult))
                {
                    ReqResult = NULL;
                }

                EnterCriticalSection(&scm->Lock);

                /* see if the SID was added to the cache in the meanwhile */
                if (!FindSidInCache(scm,
                                    pSid,
                                    &FoundReqResult))
                {
                    if (ReqResult != NULL)
                    {
                        /* cache the results */
                        CacheLookupResults(scm,
                                           pSid,
                                           ReqResult);
                    }
                }
                else
                {
                    if (ReqResult != NULL)
                    {
                        /* free the information of our lookup and use the cached
                           information*/
                        DereferenceSidReqResult(scm,
                                                ReqResult);
                    }

                    ReqResult = FoundReqResult;
                }

                Found = (ReqResult != NULL);
            }
        }

        LeaveCriticalSection(&scm->Lock);

        /* call the completion callback */
        if (Found)
        {
            CompletionProc(SidCacheMgr,
                           pSid,
                           ReqResult,
                           Context);

            if (ReqResult != NULL)
            {
                HeapFree(scm->Heap,
                         0,
                         ReqResult);
            }
        }

        DereferenceSidCacheMgr(scm);
    }

    return Found;
}
