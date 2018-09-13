/*++

    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.

Module Name:

    nsquery.cpp

Abstract:

    This module gives the class implementation for the NSQUERY object type.
    The NSQUERY object holds all the state information regarding a
    WSALookup{Begin/Next/End} series of operations. It supplies member
    functions that implement the API-level operations in terms of the SPI-level
    operations.

Author:

    Dirk Brandewie (dirk@mink.intel.com) 04-December-1995

Notes:

    $Revision:   1.14  $

    $Modtime:   08 Mar 1996 16:14:30  $


Revision History:

    most-recent-revision-date email-name
        description

    04-Dec-1995 dirk@mink.intel.com
        Initail revision

--*/

#include "precomp.h"

BOOL
MatchProtocols(DWORD dwNameSpace, LONG lfamily, LPWSAQUERYSETW lpqs)
/*++
Checks if the namespace provider identified by dwNamespace can
handle the protocol items in the list. It knows about NS_DNS and
NS_SAP only and therefore all other providers simply "pass". These
two providers are known to support one address family each and therefore
the protocol restrictions must include this family.
N.B. The right way to do this is to pass in the supported address family,
which in a more perfect world, would be store in the registry along with
the other NSP information. When that day dawns, this code can be
changed to use that value.
--*/
{
    DWORD dwProts = lpqs->dwNumberOfProtocols;
    LPAFPROTOCOLS lap = lpqs->lpafpProtocols;
    INT Match;

    //
    // this switch is the replacment for having the supported protocol
    // stored  in registry.
    //
    if(lfamily != -1)
    {
        if(lfamily == AF_UNSPEC)
        {
            return(TRUE);       // does them all
        }
        Match = lfamily;
    }
    else
    {
        switch(dwNameSpace)
        {
            case NS_SAP:
                Match = AF_IPX;
                break;

#if 0
      // The DNS name space provider now supports IPV6, IP SEC, ATM, etc.
      // Not just INET.

            case NS_DNS:
                Match = AF_INET;
                break;
#endif
            default:
                return(TRUE);      // use it
        }
    }
    //
    // If we get the address family-in-the registry=support, then
    // we should check for a value of AF_UNSPEC stored there
    // and accept this provider in that case. Note that if
    // AF_UNSPEC is given in the restriction list, we must
    // load each provider since we don't know the specific protocols
    // a provider supports.
    //
    for(; dwProts; dwProts--, lap++)
    {
        if((lap->iAddressFamily == AF_UNSPEC)
                      ||
           (lap->iAddressFamily == Match))
        {
            return(TRUE);
        }
    }
    return(FALSE);
}


NSQUERY::NSQUERY()
/*++

Routine Description:

    Constructor for the NSQUERY object.  The first member function called after
    this must be Initialize.

Arguments:

    None

Return Value:

    Returns a pointer to a NSQUERY object.
--*/
{
    m_shutting_down      = FALSE;
    m_calls_in_progress  = NULL;
    m_current_provider   = NULL;
#ifdef RASAUTODIAL
    m_query_set = NULL;
    m_control_flags = 0;
    m_catalog = NULL;
    m_restartable = TRUE;
#endif
    m_signature = QUERYSIGNATURE;
    m_provider_list.Flink = NULL;
}



INT
NSQUERY::Initialize(
    )
/*++

Routine Description:

    This  procedure  performs  all initialization for the NSQUERY object.  This
    function  must  be  invoked  after the constructor, before any other member
    function is invoked.

Arguments:


Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns an appropriate WinSock 2 error code.
--*/
{
    INT     err;
    // Init mem variables that need some amount of processing
    __try {
        InitializeCriticalSection(&m_members_guard);
        InitializeListHead(&m_provider_list);
        err = ERROR_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = WSA_NOT_ENOUGH_MEMORY;
    }

    return(err);
}



BOOL
NSQUERY::IsValid()
/*++

Routine Description:

   Checks the signature of this->m_signature to ensure that this is a valid
   query object

Arguments:

    NONE

Return Value:

    True if this points to valid query object.

--*/
{
    BOOL fValid;

    __try
    {

        fValid = (m_signature == QUERYSIGNATURE);
    }
    __except (WS2_EXCEPTION_FILTER())
    {
        fValid = FALSE;
    }
    return(fValid);
}




NSQUERY::~NSQUERY()
/*++

Routine Description:

    Destructor of the NSQUERY object.  The object should be destroyed only when
    either  (1)  the  IsDeletable() member function returns TRUE, or (2) if the
    Initialize() member function fails.

Arguments:

    None

Return Value:

    None
--*/
{
    PLIST_ENTRY ListEntry;
    PNSPROVIDERSTATE Provider;

    //
    // Check if we were fully initialized.
    //
    if (m_provider_list.Flink==NULL) {
        return;
    }
    EnterCriticalSection(&m_members_guard);

    while (!IsListEmpty(&m_provider_list))
    {
        ListEntry = RemoveHeadList(&m_provider_list);
        Provider = CONTAINING_RECORD( ListEntry,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);
        delete(Provider);
    }
#ifdef RASAUTODIAL
    delete(m_query_set);
#endif // RASAUTODIAL
    //
    // invalidate the signature since simply freeing the memory
    // may not do so. Any value will do, so the one used is arbitrary.
    //
    m_signature = ~QUERYSIGNATURE;
    DeleteCriticalSection(&m_members_guard);
}


//Structure used to carry context to CatalogEnumerationProc()
typedef struct _NSPENUMERATIONCONTEXT {
    LPWSAQUERYSETW lpqs;
    DWORD ErrorCode;
    PNSQUERY aNsQuery;
    PNSCATALOG  Catalog;
} NSPENUMERATIONCONTEXT, * PNSPENUMERATIONCONTEXT;

BOOL
LookupBeginEnumerationProc(
    IN PVOID Passback,
    IN PNSCATALOGENTRY  CatalogEntry
    )
/*++

Routine Description:

    The enumeration procedure for LookupBegin. Inspects each catalog item to
    see if it matches the selection criteria the query, if so adds the provider
    associated with the item to the list of providers involved in the query.

Arguments:

    PassBack - A context value passed to EunerateCatalogItems. It is really a
               pointer to a NSPENUMERATIONCONTEXT struct.

    CatalogItem - A pointer to a catalog item to be inspected.


Return Value:

    True
--*/
{
    PNSPENUMERATIONCONTEXT Context;
    DWORD NamespaceId;
    PNSPROVIDER Provider;
    PNSQUERY aNsQuery;
    BOOL Continue=TRUE;

    Context = (PNSPENUMERATIONCONTEXT)Passback;
    NamespaceId = CatalogEntry->GetNamespaceId();
    aNsQuery = Context->aNsQuery;

    if ((((Context->lpqs->dwNameSpace == NamespaceId)
                    ||
        (Context->lpqs->dwNameSpace == NS_ALL))
                    &&
        (!Context->lpqs->dwNumberOfProtocols
                    ||
         MatchProtocols(NamespaceId,
                        CatalogEntry->GetAddressFamily(),
                        Context->lpqs)))
                    &&
        CatalogEntry->GetEnabledState())
    {
        Provider = CatalogEntry->GetProvider();
        if (Provider==NULL) {
            // Try to load provider
            INT ErrorCode;
            ErrorCode = Context->Catalog->LoadProvider (CatalogEntry);
            if (ErrorCode!=ERROR_SUCCESS) {
                // no error if the provider won't load.
                return TRUE;
            }
            Provider = CatalogEntry->GetProvider();
            assert (Provider!=NULL);
        }

        if (!aNsQuery->AddProvider(Provider)){
            Context->ErrorCode = WSASYSCALLFAILURE;
            Continue = FALSE;
        } //if
    } //if
    return(Continue);
}


INT
WINAPI
NSQUERY::LookupServiceBegin(
    IN  LPWSAQUERYSETW      lpqsRestrictions,
    IN  DWORD              dwControlFlags,
    IN PNSCATALOG          NsCatalog
    )
/*++

Routine Description:

   Complete the initialization of a NSQUERY object and call
   NSPLookupServiceBegin() for all service providers refereneced by the query.

Arguments:

    NsCatalog - Supplies  a  reference  to  the  name-space catalog object from
                which providers may be selected.

Return Value:

--*/
{
    INT ReturnCode, ReturnCode1;
    INT ErrorCode;
    PNSCATALOGENTRY  ProviderEntry;
    PNSPROVIDERSTATE Provider;
    PLIST_ENTRY      ListEntry;
    BOOL Continue = TRUE;
    WSASERVICECLASSINFOW ClassInfo;
    LPWSASERVICECLASSINFOW ClassInfoBuf=NULL;
    DWORD                  ClassInfoSize=0;
    DWORD                  dwTempOutputFlags =
                               lpqsRestrictions->dwOutputFlags;
    LPWSTR                 lpszTempComment =
                               lpqsRestrictions->lpszComment;
    DWORD                  dwTempNumberCsAddrs =
                               lpqsRestrictions->dwNumberOfCsAddrs;
    PCSADDR_INFO           lpTempCsaBuffer =
                               lpqsRestrictions->lpcsaBuffer;

    // Select the service provider(s) that will be used for this query. A
    // service provider is selected using the provider GUID or the namespace ID
    // the namespace ID may be a specific namespace i.e. NS_DNS or NS_ALL for
    // all installed namespaces.

    //
    // Make sure that the ignored fields are cleared so that the
    // CopyQuerySetW function call below doesn't AV.
    //
    // This was a fix for bug #91655
    //
    lpqsRestrictions->dwOutputFlags = 0;
    lpqsRestrictions->lpszComment = NULL;
    lpqsRestrictions->dwNumberOfCsAddrs = 0;
    lpqsRestrictions->lpcsaBuffer = NULL;

#ifdef RASAUTODIAL
    //
    // Save the original parameters of the query, in
    // case we have to restart it due to an autodial
    // attempt.
    //
    if (m_restartable) {
        ReturnCode = CopyQuerySetW(lpqsRestrictions, &m_query_set);
        Continue = (ReturnCode == ERROR_SUCCESS);
        if (!Continue) {
            SetLastError(ReturnCode);
            ReturnCode = SOCKET_ERROR;
            m_restartable = FALSE;
        }
        m_control_flags = dwControlFlags;
        m_catalog = NsCatalog;
    }
#endif // RASAUTODIAL

    if (Continue) 
    {
        if (lpqsRestrictions->lpNSProviderId)
        {
            // Use a single namespace provider
            ReturnCode = NsCatalog->GetCountedCatalogItemFromProviderId(
                lpqsRestrictions->lpNSProviderId,
                &ProviderEntry);
            if (ERROR_SUCCESS != ReturnCode){
                SetLastError(WSAEINVAL);
                Continue = FALSE;
                ReturnCode = SOCKET_ERROR;
            } //if
            if (Continue){
                Continue = AddProvider(ProviderEntry->GetProvider());
            } //if
        } //if
        else{
            NSPENUMERATIONCONTEXT Context;
    
            Context.lpqs = lpqsRestrictions;
            Context.ErrorCode = ERROR_SUCCESS;
            Context.aNsQuery = this;
            Context.Catalog = NsCatalog;
    
            NsCatalog->EnumerateCatalogItems(
                LookupBeginEnumerationProc,
                &Context);
            if (ERROR_SUCCESS != Context.ErrorCode){
                SetLastError(Context.ErrorCode);
                ReturnCode = SOCKET_ERROR;
                Continue = FALSE;
            } //if
        } //else
    } //if


    if (Continue){
         //Get the class information for this query. Call once with a zero
         //buffer to size the buffer we need to allocate then call to get the
         //real answer
        ClassInfo.lpServiceClassId = lpqsRestrictions->lpServiceClassId;

        ReturnCode = NsCatalog->GetServiceClassInfo(
            &ClassInfoSize,
            &ClassInfo);
        ErrorCode = GetLastError();

        if ((SOCKET_ERROR == ReturnCode) &&
            WSAEFAULT == ErrorCode ){

            ClassInfoBuf = (LPWSASERVICECLASSINFOW)new BYTE[ClassInfoSize];

            if (ClassInfoBuf){
                ReturnCode = NsCatalog->GetServiceClassInfo(
                    &ClassInfoSize,
                    ClassInfoBuf);
            } //if
            else{
                Continue = FALSE;
            } //else
        } //if
    } //if

    if( Continue && IsListEmpty( &m_provider_list ) ) {
        Continue = FALSE;
        ReturnCode = SOCKET_ERROR;
        SetLastError(WSASERVICE_NOT_FOUND);
    }

    if (Continue){

        ReturnCode1 = SOCKET_ERROR;

        //Call Begin on all the selected providers
        ListEntry = m_provider_list.Flink;
        Provider = CONTAINING_RECORD( ListEntry,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);
        while (Provider){
            ReturnCode = Provider->LookupServiceBegin(lpqsRestrictions,
                                         ClassInfoBuf,
                                         dwControlFlags);
            if(ReturnCode == SOCKET_ERROR)
            {
                //
                // this provider didn't like it. So remove it
                // from the list
                //

                PNSPROVIDERSTATE Provider1;

                Provider1 = Provider;
                Provider = NextProvider(Provider);
                RemoveEntryList(&Provider1->m_query_linkage);
                delete(Provider1);
            }
            else
            {
                ReturnCode1 = ReturnCode;
                Provider = NextProvider(Provider);
            }
        } //while
    } //if

    if (!Continue
          ||
        ((ReturnCode = ReturnCode1) == SOCKET_ERROR)){
        // We failed somewhere along the way so clean up the provider on the
        // provider list.
        while (!IsListEmpty(&m_provider_list)){
            ListEntry = RemoveHeadList(&m_provider_list);
            Provider = CONTAINING_RECORD( ListEntry,
                                          NSPROVIDERSTATE,
                                          m_query_linkage);
            delete(Provider);
        } //while
        if (ClassInfoBuf){
            delete ClassInfoBuf;
        } //if
    } //if
    else{
        ListEntry = m_provider_list.Flink;
        m_current_provider = CONTAINING_RECORD( ListEntry,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);
    } //else

    //
    // Restore ignored field values to what callee had set.
    //
    lpqsRestrictions->dwOutputFlags = dwTempOutputFlags;
    lpqsRestrictions->lpszComment = lpszTempComment;
    lpqsRestrictions->dwNumberOfCsAddrs = dwTempNumberCsAddrs;
    lpqsRestrictions->lpcsaBuffer = lpTempCsaBuffer;

    return(ReturnCode);
}

// *** Fill in description from the spec when it stabilizes.



INT
WINAPI
NSQUERY::LookupServiceNext(
    IN     DWORD           dwControlFlags,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPWSAQUERYSETW  lpqsResults
    )
/*++

Routine Description:

    //***TODO Fill in description from the spec when it stabilizes.

Arguments:


Return Value:

--*/
{
    INT ReturnCode = SOCKET_ERROR;
    PNSPROVIDERSTATE NewProvider = NULL;
    PNSPROVIDERSTATE ThisProvider = NULL;

again:
    if (!m_shutting_down){
        //Snapshot the value of m_current_provider and bump up the calls in
        //progress counter
        EnterCriticalSection(&m_members_guard);
        InterlockedIncrement(&m_calls_in_progress);
        NewProvider = m_current_provider;
        LeaveCriticalSection(&m_members_guard);

        if ( !NewProvider )
            SetLastError( WSA_E_NO_MORE );

        while (NewProvider){
            ReturnCode = NewProvider->LookupServiceNext(
                dwControlFlags,
                lpdwBufferLength,
                lpqsResults);
            if ((ERROR_SUCCESS == ReturnCode)
                        ||
                (WSAEFAULT == GetLastError()) )
            {
                break;
            } //if

            EnterCriticalSection(&m_members_guard);
            ThisProvider = NewProvider;
            NewProvider = NextProvider(m_current_provider);
            if (ThisProvider == m_current_provider){
               m_current_provider = NewProvider ;
            } //if
            LeaveCriticalSection(&m_members_guard);
        } //while
        InterlockedDecrement(&m_calls_in_progress);

#ifdef RASAUTODIAL
        if (NewProvider == NULL &&
            m_restartable &&
            ReturnCode == SOCKET_ERROR)
        {
            PLIST_ENTRY ListEntry;
            DWORD errval;

            //
            // Save the error in case the Autodial
            // attempt fails.
            //
            errval = GetLastError();
            //
            // We only invoke Autodial once per query.
            //
            m_restartable = FALSE;
            if (WSAttemptAutodialName(m_query_set)) {
                //
                // Because the providers have cached state
                // about this query, we need to call
                // LookupServiceEnd/LookupServiceBegin
                // to reset them.
                //
                while (!IsListEmpty(&m_provider_list)){
                    ListEntry = RemoveHeadList(&m_provider_list);
                    ThisProvider = CONTAINING_RECORD( ListEntry,
                                                  NSPROVIDERSTATE,
                                                  m_query_linkage);
                    ThisProvider->LookupServiceEnd();
                    delete(ThisProvider);
                } //while
                //
                // Restart the query.
                //
                if (LookupServiceBegin(
                      m_query_set,
                      m_control_flags|LUP_FLUSHCACHE,
                      m_catalog) == ERROR_SUCCESS)
                {
                    goto again;
                }
            }
            else
                SetLastError(errval);
        }
#endif // RASAUTODIAL
    } //if
    else{
        ReturnCode = SOCKET_ERROR;
        SetLastError(WSAECANCELLED);
    } //else
    return(ReturnCode);
}



INT
WINAPI
NSQUERY::LookupServiceEnd()
/*++

Routine Description:

    This routine ends a query by calling NSPlookupServiceEnd on all the
    providers associated with this query.

Arguments:

    NONE

Return Value:

    ERROR_SUCCESS
--*/
{
    PLIST_ENTRY ListEntry;
    PNSPROVIDERSTATE CurrentProvider;

    //Mark ourselves as shuti
    EnterCriticalSection(&m_members_guard);
    m_shutting_down = TRUE;
    LeaveCriticalSection(&m_members_guard);

    ListEntry = m_provider_list.Flink;

    while (ListEntry != &m_provider_list){
         CurrentProvider = CONTAINING_RECORD( ListEntry,
                                              NSPROVIDERSTATE,
                                              m_query_linkage);
         CurrentProvider->LookupServiceEnd();
        ListEntry = ListEntry->Flink;
    } //while
    return(ERROR_SUCCESS);
}



BOOL
WINAPI
NSQUERY::IsDeletable()
/*++

Routine Description:

    This  function  determines  whether the NSQUERY object should be destroyed.
    This  function should be invoked after every call to LookupServiceNext() or
    LookupEnd().   If it returns TRUE, any concurrent operations have completed
    and the NSQUERY object should be destroyed.

Arguments:

    None

Return Value:

    TRUE  - The NSQUERY object should be destroyed.
    FALSE - The NSQUERY object should not be destroyed.
--*/
{
    return( (m_shutting_down &&
             (m_calls_in_progress == 0)) ? TRUE : FALSE);
}


PNSPROVIDERSTATE
NSQUERY::NextProvider(
    PNSPROVIDERSTATE Provider
    )
/*++

Routine Description:

    Retrieve the next provider object from the list of providers associated
    with this query.

Arguments:

    Provider - A pointer to a provider state object.

Return Value:

    A pointer to the next provider state object on the list of providers or
    NULL if no entries are present after Provider.

--*/
{
    PNSPROVIDERSTATE NewProvider=NULL;
    PLIST_ENTRY ListEntry;

    ListEntry = Provider->m_query_linkage.Flink;

    if (ListEntry != &m_provider_list){
        NewProvider = CONTAINING_RECORD( ListEntry,
                                         NSPROVIDERSTATE,
                                         m_query_linkage);
    } //if
    return(NewProvider);
}


BOOL
NSQUERY::AddProvider(
    PNSPROVIDER  pNamespaceProvider
    )
/*++

Routine Description:

    Adds a namespace provider to the list of provider(s) involed with this
    query. A NSPROVIDERSTATE object is created for the provider the provider
    object is attached to the state object and the state object is added to the
    provider list.

Arguments:

    pNamespaceProvider - A pointer to a namespace provider object to be added
                         to the list of providers.

Return Value:
    TRUE if the operation is successful else FALSE.

--*/
{
    BOOL ReturnCode = TRUE;
    PNSPROVIDERSTATE ProviderHolder;

    ProviderHolder = new NSPROVIDERSTATE;
    if (ProviderHolder){
        ProviderHolder->Initialize(pNamespaceProvider);
        InsertTailList(&m_provider_list,
                           &(ProviderHolder->m_query_linkage));
    } //if
    else{
        SetLastError(WSASYSCALLFAILURE);
        ReturnCode = FALSE;
    } //else
    return(ReturnCode);
}
