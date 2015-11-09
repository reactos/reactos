/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/nsquery.c
 * PURPOSE:     Namespace Query Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* DATA **********************************************************************/

#define WsNqLock()      EnterCriticalSection((LPCRITICAL_SECTION)&NsQuery->Lock);
#define WsNqUnlock()    LeaveCriticalSection((LPCRITICAL_SECTION)&NsQuery->Lock);

/* FUNCTIONS *****************************************************************/

PNSQUERY
WSAAPI
WsNqAllocate(VOID)
{
    PNSQUERY NsQuery;
    
    /* Allocate the object */
    NsQuery = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*NsQuery));

    /* Set non-zero fields */
    NsQuery->Signature = ~0xBEADFACE;
    InitializeListHead(&NsQuery->ProviderList);
    NsQuery->TryAgain = TRUE;
    
    /* Return it */
    return NsQuery;
}

DWORD
WSAAPI
WsNqInitialize(IN PNSQUERY Query)
{
    /* Initialize the lock */
    InitializeCriticalSection((LPCRITICAL_SECTION)&Query->Lock);

    /* Set initial reference count and signature */
    Query->RefCount = 1;
    Query->Signature = 0xBEADFACE;

    /* Return success */
    return ERROR_SUCCESS;
}

BOOL
WSAAPI
WsNqValidateAndReference(IN PNSQUERY Query)
{
    /* Check the signature first */
    if (Query->Signature != 0xBEADFACE) return FALSE;

    /* Validate the reference count */
    if (!Query->RefCount) return FALSE;

    /* Increase reference count */
    InterlockedIncrement(&Query->RefCount);

    /* Return success */
    return TRUE;
}

VOID
WSAAPI
WsNqDelete(IN PNSQUERY NsQuery)
{
    PNSQUERY_PROVIDER Provider;
    PLIST_ENTRY Entry;
    
    /* Make sure that we got initialized */
    if (!NsQuery->ProviderList.Flink) return;

    /* Loop the provider list */
    while (!IsListEmpty(&NsQuery->ProviderList))
    {
        /* Remove the entry */
        Entry = RemoveHeadList(&NsQuery->ProviderList);

        /* Get the provider */
        Provider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);

        /* Delete it */
        WsNqProvDelete(Provider);
    }

    /* Remove the signature and delete the lock */
    NsQuery->Signature = ~0xBEADFACE;
    DeleteCriticalSection((LPCRITICAL_SECTION)&NsQuery->Lock);

    /* Free us */
    HeapFree(WsSockHeap, 0, NsQuery);
}

VOID
WSAAPI
WsNqDereference(IN PNSQUERY Query)
{
    /* Decrease the reference count and check if it's zero */
    if (!InterlockedDecrement(&Query->RefCount))
    {
        /* Delete us*/
        WsNqDelete(Query);
    }
}

BOOL
WSAAPI
WsNqBeginEnumerationProc(PVOID Context,
                         PNSCATALOG_ENTRY Entry)
{
    PNS_PROVIDER Provider;
    BOOLEAN GoOn = TRUE;
    PENUM_CONTEXT EnumContext = (PENUM_CONTEXT)Context;
    PNSQUERY NsQuery = EnumContext->NsQuery;
    DWORD NamespaceId = Entry->NamespaceId;

    /* Match the namespace ID, protocols and make sure it's enabled */
    if ((((EnumContext->lpqsRestrictions->dwNameSpace == NamespaceId) ||
          (EnumContext->lpqsRestrictions->dwNameSpace == NS_ALL)) &&
         (!(EnumContext->lpqsRestrictions->dwNumberOfProtocols) ||
           (WsNcMatchProtocols(NamespaceId,
                               Entry->AddressFamily,
                               EnumContext->lpqsRestrictions)))) &&
        (Entry->Enabled))
    {
        /* Get the provider */
        if (!(Provider = Entry->Provider))
        {
            /* None was loaded, load it */
            if ((WsNcLoadProvider(EnumContext->Catalog, Entry) != ERROR_SUCCESS))
            {
                /* return TRUE to continue enumerating */
                return TRUE;
            }

            /* Set the provider */
            Provider = Entry->Provider;
        }

        /* Add it to the query */
        if (!(WsNqAddProvider(NsQuery, Provider)))
        {
            /* We failed */
            EnumContext->ErrorCode = WSASYSCALLFAILURE;
            GoOn = FALSE;
        }
    }

    /* Return to caller */
    return GoOn;
}

DWORD
WSAAPI
WsNqLookupServiceEnd(IN PNSQUERY NsQuery)
{
    PNSQUERY_PROVIDER Provider;
    PLIST_ENTRY Entry;
    
    /* Protect us from closure */
    WsNqLock();
    NsQuery->ShuttingDown = TRUE;

    /* Get the list and loop */
    Entry = NsQuery->ProviderList.Flink;
    while (Entry != &NsQuery->ProviderList)
    {
        /* Get the provider */
        Provider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);

        /* Call its routine */
        WsNqProvLookupServiceEnd(Provider);

        /* Move to the next one */
        Entry = Entry->Flink;
    }

    /* Release lock and return success */
    WsNqUnlock();
    return ERROR_SUCCESS;
}

DWORD
WSAAPI
WsNqLookupServiceNext(IN PNSQUERY NsQuery,
                      IN DWORD ControlFlags,
                      OUT PDWORD BufferLength,
                      OUT LPWSAQUERYSETW Results)
{
    PNSQUERY_PROVIDER Provider, NextProvider;
    INT ErrorCode = SOCKET_ERROR, OldErrorCode;
    PLIST_ENTRY Entry;

    /* Make sure we're not shutting down */
    if (!NsQuery->ShuttingDown)
    {
        /* Acquire query lock */
        WsNqLock();

        /* Check if we already have an active provider */
        NextProvider = NsQuery->ActiveProvider;
        if (!NextProvider)
        {
            /* Make sure we have a current provider */
            if (!NsQuery->CurrentProvider)
            {
                /* We don't; fail */
                WsNqUnlock();
                SetLastError(WSA_E_NO_MORE);
                return SOCKET_ERROR;
            }

            /* Get the first provider on the list and start looping */
            Entry = NsQuery->ProviderList.Blink;
            NextProvider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);
            while (NextProvider)
            {
                /* Check if this is a new-style provider */
                if (NextProvider->Provider->Service.NSPIoctl)
                {
                    /* Remove it and re-add it on top */
                    RemoveEntryList(&NextProvider->QueryLink);
                    InsertHeadList(&NsQuery->ProviderList, &NextProvider->QueryLink);

                    /* Set it as the active provider and exit the loop */
                    NsQuery->ActiveProvider = NextProvider;
                    break;
                }

                /* Get the previous provider */
                NextProvider = WsNqPreviousProvider(NsQuery, NextProvider);
            }
        }

        /* Release the lock */
        WsNqUnlock();

        /* Check if we have an active provider now */
        if (NextProvider)
        {
            /* Start loop */
            do
            {
                /* Call its routine */
                ErrorCode = WsNqProvLookupServiceNext(NextProvider,
                                                      ControlFlags,
                                                      BufferLength,
                                                      Results);
                /* Check for error or shutdown */
                if ((ErrorCode == ERROR_SUCCESS) ||
                    (GetLastError() == WSAEFAULT) || (NsQuery->ShuttingDown))
                {
                    /* Get out */
                    break;
                }

                /* Acquire Query Lock */
                WsNqLock();

                /* Save the current active provider */
                Provider = NsQuery->ActiveProvider;
                
                /* Check if one exists */
                if (Provider)
                {
                    /* Get the next one */
                    NextProvider = WsNqNextProvider(NsQuery,
                                                    NsQuery->ActiveProvider);

                    /* Was the old provider our active? */
                    if (Provider == NsQuery->ActiveProvider)
                    {
                        /* Change our active provider to the new one */
                        NsQuery->ActiveProvider = NextProvider;
                    }
                }
                else
                {
                    /* No next provider */
                    NextProvider = NULL;
                }

                /* Check if we failed and if we can try again */
                if (!(NextProvider) &&
                    (ErrorCode == SOCKET_ERROR) &&
                    (NsQuery->TryAgain))
                {
                    /* Save the error code so RAS doesn't overwrite it */
                    OldErrorCode = GetLastError();

                    /* Make sure we won't try for a 3rd time */
                    NsQuery->TryAgain = FALSE;

                    /* Call the helper to auto-dial */
                    if (WSAttemptAutodialName(NsQuery->QuerySet))
                    {
                        /* It suceeded, so we'll delete the current state. */
                        while (!IsListEmpty(&NsQuery->ProviderList))
                        {
                            /* Remove the entry and get its provider */
                            Entry = RemoveHeadList(&NsQuery->ProviderList);
                            Provider = CONTAINING_RECORD(Entry,
                                                         NSQUERY_PROVIDER,
                                                         QueryLink);

                            /* Reset it */
                            WsNqProvLookupServiceEnd(Provider);
                            WsNqProvDelete(Provider);
                        }                  

                        /* Start a new query */
                        if (!WsNqLookupServiceBegin(NsQuery,
                                                    NsQuery->QuerySet,
                                                    NsQuery->ControlFlags,
                                                    NsQuery->Catalog))
                        {
                            /* New query succeeded, set active provider now */
                            NsQuery->ActiveProvider = 
                                WsNqNextProvider(NsQuery,
                                                 NsQuery->ActiveProvider);
                        }
                    }
                    else
                    {
                        /* Reset the error code */
                        SetLastError(OldErrorCode);
                    }
                }

                /* Release lock */
                WsNqUnlock();

                /* Keep looping as long as there is a provider */
            } while (NextProvider);   
        }
    }
    else
    {
        /* We are shuting down; fail */
        SetLastError(WSAECANCELLED);
    }

    /* Return */
    return ErrorCode;
}

DWORD
WSAAPI
WsNqLookupServiceBegin(IN PNSQUERY NsQuery,
                       IN LPWSAQUERYSETW Restrictions,
                       IN DWORD ControlFlags,
                       IN PNSCATALOG Catalog)
{
    WSASERVICECLASSINFOW ClassInfo;
    PNSQUERY_PROVIDER Provider;
    LPWSASERVICECLASSINFOW pClassInfo = NULL;
    PNSQUERY_PROVIDER NextProvider;
    PLIST_ENTRY Entry;
    INT ErrorCode;
    DWORD ClassInfoSize;
    PNSCATALOG_ENTRY CatalogEntry;
    ENUM_CONTEXT EnumContext;
    BOOLEAN TryAgain;

    /* Check for RAS Auto-dial attempt */
    if (NsQuery->TryAgain)
    {
        /* Make a copy of the query set */
        ErrorCode = CopyQuerySetW(Restrictions, &NsQuery->QuerySet);
        TryAgain = (ErrorCode == ERROR_SUCCESS);

        /* Check if we'll try again */
        if (!TryAgain)
        {
            /* We won't, fail */
            SetLastError(ErrorCode);
            ErrorCode = SOCKET_ERROR;
            NsQuery->TryAgain = FALSE;
            goto error;
        }

        /* Cache the information for a restart */
        NsQuery->ControlFlags = ControlFlags;
        NsQuery->Catalog = Catalog;
    }
    
    /* Check if we have a specific ID */
    if (Restrictions->lpNSProviderId)
    {
        /* Get the provider */
        ErrorCode = WsNcGetCatalogFromProviderId(Catalog,
                                                 Restrictions->lpNSProviderId,
                                                 &CatalogEntry);
        /* Check for success */
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Fail */
            SetLastError(WSAEINVAL);
            ErrorCode = SOCKET_ERROR;
            goto error;
        }
        else
        {
            /* Add this provider */
            WsNqAddProvider(NsQuery, CatalogEntry->Provider);
        }
    }
    else
    {
        /* Setup the lookup context */
        EnumContext.lpqsRestrictions = Restrictions;
        EnumContext.ErrorCode = ERROR_SUCCESS;
        EnumContext.NsQuery = NsQuery;
        EnumContext.Catalog = Catalog;

        /* Do a lookup for every entry */
        WsNcEnumerateCatalogItems(Catalog,
                                  WsNqBeginEnumerationProc,
                                  &EnumContext);
        ErrorCode = EnumContext.ErrorCode;
        
        /* Check for success */
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Fail */
            SetLastError(WSAEINVAL);
            ErrorCode = SOCKET_ERROR;
            goto error;
        }
    }

    /* Get the class information */
    ClassInfo.lpServiceClassId = Restrictions->lpServiceClassId;
    ErrorCode = WsNcGetServiceClassInfo(Catalog, &ClassInfoSize, &ClassInfo);

    /* Check if more buffer space is needed */
    if ((ErrorCode == SOCKET_ERROR) && (GetLastError() == WSAEFAULT))
    {
        /* FIXME: The WS 2.2 spec hasn't been finalized yet on this... */
    }
    else
    {
        /* Assume success */
        ErrorCode = ERROR_SUCCESS;
    }

    /* Check if the provider list is empty */
    if (IsListEmpty(&NsQuery->ProviderList))
    {
        /* We don't have any providers to handle this! */
        ErrorCode = SOCKET_ERROR;
        SetLastError(WSASERVICE_NOT_FOUND);
        goto error;
    }

    /* Get the first provider and loop */
    Entry = NsQuery->ProviderList.Flink;
    NextProvider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);
    while (NextProvider)
    {
        /* Call it */
        ErrorCode = WsNqProvLookupServiceBegin(NextProvider,
                                               Restrictions,
                                               pClassInfo,
                                               ControlFlags);
        /* Check for error */
        if (ErrorCode == SOCKET_ERROR)
        {
            /* Remove this provider, get the next one, delete the old one */
            Provider = NextProvider;
            NextProvider = WsNqNextProvider(NsQuery, NextProvider);
            RemoveEntryList(&Provider->QueryLink);
            WsNqProvDelete(Provider);
        }
        else
        {
            /* Get the next provider */
            NextProvider = WsNqNextProvider(NsQuery, NextProvider);
        }
    }

error:
    /* Check if we had an error somewhere */
    if (ErrorCode == SOCKET_ERROR)
    {
        /* Loop the list */
        while (!IsListEmpty(&NsQuery->ProviderList))
        {
            /* Remove this provider */
            Entry = RemoveHeadList(&NsQuery->ProviderList);

            /* Get the failed provider and delete it */
            Provider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);
            WsNqProvDelete(Provider);
        }
    }
    else
    {
        /* Set the active provider */
        Entry = NsQuery->ProviderList.Flink;
        NsQuery->ActiveProvider = CONTAINING_RECORD(Entry,
                                                    NSQUERY_PROVIDER,
                                                    QueryLink);
    }

    /* Return */
    return ErrorCode;
}

PNSQUERY_PROVIDER
WSAAPI
WsNqNextProvider(IN PNSQUERY Query,
                 IN PNSQUERY_PROVIDER Provider)
{
    PNSQUERY_PROVIDER NextProvider = NULL;
    PLIST_ENTRY Entry;
    
    /* Get the first entry and get its provider */
    Entry = Provider->QueryLink.Flink;
    if (Entry != &Query->ProviderList)
    {
        /* Get the current provider */
        NextProvider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);
    }

    /* Return it */
    return NextProvider;
}

PNSQUERY_PROVIDER
WSAAPI
WsNqPreviousProvider(IN PNSQUERY Query,
                     IN PNSQUERY_PROVIDER Provider)
{
    PNSQUERY_PROVIDER NextProvider = NULL;
    PLIST_ENTRY Entry;
    
    /* Get the first entry and get its provider */
    Entry = Provider->QueryLink.Blink;
    if (Entry != &Query->ProviderList)
    {
        /* Get the current provider */
        NextProvider = CONTAINING_RECORD(Entry, NSQUERY_PROVIDER, QueryLink);
    }

    /* Return it */
    return NextProvider;
}

DWORD
WSAAPI
WsNqAddProvider(IN PNSQUERY Query,
                IN PNS_PROVIDER Provider)
{
    PNSQUERY_PROVIDER QueryProvider;
    DWORD Return = TRUE;
    
    /* Allocate a new Query Provider */
    if ((QueryProvider = WsNqProvAllocate()))
    {
        /* Initialize it */
        WsNqProvInitialize(QueryProvider, Provider);

        /* Insert it into the provider list */
        InsertTailList(&Query->ProviderList, &QueryProvider->QueryLink);
    }
    else
    {
        /* We failed */
        SetLastError(WSASYSCALLFAILURE);
        Return = FALSE;
    }

    /* Return */
    return Return;
}


