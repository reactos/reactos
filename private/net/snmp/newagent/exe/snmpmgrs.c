/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpmgrs.c

Abstract:

    Contains routines for manipulating manager structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "snmpmgrs.h"
#include "network.h"



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocMLE(
    PMANAGER_LIST_ENTRY * ppMLE,
    LPSTR                 pManager
    )

/*++

Routine Description:

    Allocates manager structure and initializes.

Arguments:

    pManager - pointer to manager string.

    ppMLE - pointer to receive pointer to entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PMANAGER_LIST_ENTRY pMLE = NULL;
    DWORD dwIpAddr;
    LPSTR pszManager;

    // attempt to allocate structure
    pMLE = AgentMemAlloc(sizeof(MANAGER_LIST_ENTRY));

    // validate
    if (pMLE != NULL) {

        // allocate memory for manager string
        pMLE->pManager = AgentMemAlloc(strlen(pManager)+1);

        // validate
        if (pMLE->pManager != NULL) {

            // transfer manager string
            strcpy(pMLE->pManager, pManager);

            // Attempt to resolve manager network address 
            // For IPX addresses, this call always succeeds
            // When SnmpSvcAddrToSocket fails, this means we deal with a dynamic IP Address
            // for which the gethostbyname() failed.
            if (SnmpSvcAddrToSocket(pMLE->pManager, &pMLE->SockAddr)) {

                // see if tcpip address
                if (pMLE->SockAddr.sa_family == AF_INET) {

                    // save structure size for later use
                    pMLE->SockAddrLen = sizeof(struct sockaddr_in);

                    pszManager = pMLE->pManager;

                    // attempt to convert address directly
                    dwIpAddr = inet_addr(pMLE->pManager);

                    // assume address is dynamic if error occurs
                    pMLE->fDynamicName = (dwIpAddr == SOCKET_ERROR);

                    // note time manager addr updated
                    pMLE->dwLastUpdate = GetCurrentTime();

                    // success
                    fOk = TRUE;

                } else if (pMLE->SockAddr.sa_family == AF_IPX) {

                    // save structure size for later use
                    pMLE->SockAddrLen = sizeof(struct sockaddr_ipx);

                    // no name lookup for ipx
                    pMLE->fDynamicName = FALSE;

                    // success
                    fOk = TRUE;
                }

                pMLE->dwAge = MGRADDR_ALIVE;

            } else {
                LPTSTR tcsManager;

#ifdef UNICODE
                SnmpUtilUTF8ToUnicode(&tcsManager, pMLE->pManager, TRUE);
#else
                tcsManager=pMLE->pManager;
#endif
                // at this point the address can be only an IP address!
                // so we know pMLE->SockAddrLen as the size of the struct sockaddr_in!
                pMLE->SockAddrLen = sizeof(struct sockaddr_in);

                // since SnmpSvcAddrToSocket failed, that means inet_addr() failed hence
                // we deal with a dynamic IP address
                pMLE->fDynamicName = TRUE;

                // set 'age' to dying
                pMLE->dwAge = snmpMgmtBase.AsnIntegerPool[IsnmpNameResolutionRetries].asnValue.number;

                // if the registry parameter is -1 this stands for 'keep retrying forever'
                // in this case set the dwAge to the default MGRADDR_DYING(16) and never decrement it
                if (pMLE->dwAge == (DWORD)-1)
                    pMLE->dwAge = MGRADDR_DYING;

                // report a warning to the system log
                ReportSnmpEvent(
                    SNMP_EVENT_NAME_RESOLUTION_FAILURE,
                    1,
                    &tcsManager,
                    0);

#ifdef UNICODE
                SnmpUtilMemFree(tcsManager);
#endif

                // success
                fOk = TRUE;
            }
        }
    
        // cleanup
        if (!fOk) {
    
            // release
            FreeMLE(pMLE);                

            // re-init
            pMLE = NULL;            
        }
    }

    // transfer
    *ppMLE = pMLE;

    return fOk;
}


BOOL
FreeMLE(
    PMANAGER_LIST_ENTRY pMLE
    )

/*++

Routine Description:

    Releases manager structure.

Arguments:

    pMLE - pointer to manager list entry to be freed.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // validate pointer
    if (pMLE != NULL) {

        // release string
        AgentMemFree(pMLE->pManager);

        // release structure
        AgentMemFree(pMLE);
    }

    return TRUE;
}


BOOL
UpdateMLE(
    PMANAGER_LIST_ENTRY pMLE
    )

/*++

Routine Description:

    Updates manager structure.
	An address will be resolved only if it is not marked as being 'DEAD'. A 'DEAD' address failed to be resolved for
	more than MGRADDR_DYING times. A 'DEAD' address will no longer be used as a trap destination, but it will still
	be validating the incoming SNMP requests if it could be resolve at least once since the service started up.

Arguments:

    pMLE - pointer to manager list entry to be updated.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;
    DWORD dwElaspedTime;
    struct sockaddr SockAddr;

    SNMPDBG((SNMP_LOG_TRACE,
             "SNMP: SVC: Update manager '%s' with age %d.\n",
             pMLE->pManager,
             pMLE->dwAge));

    // don't try to resolve this address if it is already dead
	if (pMLE->dwAge == MGRADDR_DEAD)
		return FALSE;

    // see if name dynamic
    if (pMLE->fDynamicName) {

        // determine elasped time since last update
        dwElaspedTime = GetCurrentTime() - pMLE->dwLastUpdate;

        // resolve the address only if it failed to be resolved on last update 
        // or its update time expired.
        if (pMLE->dwAge != MGRADDR_ALIVE || dwElaspedTime > DEFAULT_NAME_TIMEOUT) {
        
            // attempt to resolve manager network address
            // for IPX addresses, this call always succeeds
            fOk = SnmpSvcAddrToSocket(pMLE->pManager, &SockAddr);

            // validate
            if (fOk) {

                // update entry with new address
                memcpy(&pMLE->SockAddr, &SockAddr, sizeof(SockAddr));

                // note time dynamic name resolved
                pMLE->dwLastUpdate = GetCurrentTime();

                // make sure manager age is 'ALIVE'
                pMLE->dwAge = MGRADDR_ALIVE;

            } else if (pMLE->dwAge == MGRADDR_ALIVE) {

                // Previously 'ALIVE' address cannot be resolved anymore
				// set its age to the one specified by 'NameResolutionRetries' parameter in
				// order to give some more chances.
                pMLE->dwAge = snmpMgmtBase.AsnIntegerPool[IsnmpNameResolutionRetries].asnValue.number;

                // if the registry parameter is -1 this stands for 'keep retrying forever'
                // in this case set the dwAge to the default MGRADDR_DYING(16) which will never be decremented
                if (pMLE->dwAge == (DWORD)-1)
                    pMLE->dwAge = MGRADDR_DYING;

            } else if (pMLE->dwAge != MGRADDR_DEAD) {

				// the address could not be resolved before and it still cannot be resolved
				// decrement its retry counter only if the 'NameResolutionRetries' parameter says so
                if (snmpMgmtBase.AsnIntegerPool[IsnmpNameResolutionRetries].asnValue.number != -1)
                    pMLE->dwAge--;
            }
        }        
    }

    return fOk;
}


BOOL
FindManagerByName(
    PMANAGER_LIST_ENTRY * ppMLE,
    PLIST_ENTRY           pListHead,
    LPSTR                 pManager
    )

/*++

Routine Description:

    Locates manager in list.

Arguments:

    ppMLE - pointer to receive pointer to entry.

    pListHead - pointer to head of manager list.

    pManager - pointer to manager to find.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PMANAGER_LIST_ENTRY pMLE;

    // initialize
    *ppMLE = NULL;

    // obtain pointer to list head
    pLE = pListHead->Flink;

    // process all entries in list
    while (pLE != pListHead) {

        // retrieve pointer to community structure
        pMLE = CONTAINING_RECORD(pLE, MANAGER_LIST_ENTRY, Link);

        // compare community string with entry
        if (!strcmp(pMLE->pManager, pManager)) {

            // transfer
            *ppMLE = pMLE;

            // success
            return TRUE;
        }

        // next entry
        pLE = pLE->Flink;
    }

    // failure
    return FALSE;
}


BOOL
IsManagerAddrLegal(
    struct sockaddr_in *  pAddr
    )
{
    DWORD dwHostMask;
    DWORD dwAddress = ntohl(pAddr->sin_addr.S_un.S_addr);

    // check address legality only for Ip addresses
    if (pAddr->sin_family != AF_INET)
        return TRUE;

    // disallow multicast (or future use) source addresses
    // local broadcast will be filtered out here as well
    if ((dwAddress & 0xe0000000) == 0xe0000000)
        return FALSE;

    // get hostmask for class 'C' addresses
    if ((dwAddress & 0xc0000000) == 0xc0000000)
        dwHostMask = 0x000000ff;

    // get hostmask for class 'B' addresses
    else if ((dwAddress & 0x80000000) == 0x80000000)
        dwHostMask = 0x0000ffff;

    // get hostidmask for class 'A' addresses
    else
        dwHostMask = 0x00ffffff;

    SNMPDBG((SNMP_LOG_TRACE,"SNMP: dwAddress=%08x, dwHostMask=%08x, port=%d\n",
             dwAddress, dwHostMask, ntohs(pAddr->sin_port)));

    return ((dwAddress & dwHostMask) != 0                              // check against net address
            && ((dwAddress & dwHostMask) != (0x00ffffff & dwHostMask)) // check against broadcast address
//          && ntohs(pAddr->sin_port) >= 1024                          // check against reserved port 
           );
}


BOOL
FindManagerByAddr(
    PMANAGER_LIST_ENTRY * ppMLE,
    struct sockaddr *     pSockAddr
    )

/*++

Routine Description:

    Locates permitted manager in list.

Arguments:

    ppMLE - pointer to receive pointer to entry.

    pSockAddr - pointer to socket address to find.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PMANAGER_LIST_ENTRY pMLE;
    DWORD dwSockAddrLen;
    enum
    {
        SRCH_ALIVE,
        SRCH_DYING,
        SRCH_DONE
    } state;

    // initialize
    *ppMLE = NULL;

    // loop twice through the list of permitted managers
    // in the first loop look only through 'ALIVE' managers
    // in the second loop look through the 'DYING' or 'DEAD' managers.
    // ... this logic minimizes the response time for regular SNMP requests,
    // as far as there is a bigger chance to have the request comming from an 'ALIVE' manager.
    // otherwise, gethostbyname() called in UpdateMLE() lasts about 1/2 sec!!!
    for (state = SRCH_ALIVE, pLE = g_PermittedManagers.Flink;
         state != SRCH_DONE;
         pLE=pLE->Flink)
    {
        // retrieve pointer to manager structure
        pMLE = CONTAINING_RECORD(pLE, MANAGER_LIST_ENTRY, Link);

        // if we are in the first loop ..
        if (state == SRCH_ALIVE)
        {
            // .. but reached its end ..
            if (pLE == &g_PermittedManagers)
            {
                // .. go further with the second loop
                state = SRCH_DYING;
                continue;
            }

            // .. pass over the managers that are not 'ALIVE'
            if (pMLE->dwAge != MGRADDR_ALIVE)
                continue;
        }

        // if we are in the second loop ..
        if (state == SRCH_DYING)
        {
            // .. but reached its end ..
            if (pLE == &g_PermittedManagers)
            {
                // .. mark the end of scanning
                state = SRCH_DONE;
                continue;
            }

            // .. pass over the managers that are 'ALIVE'
            if (pMLE->dwAge == MGRADDR_ALIVE || pMLE->dwAge == MGRADDR_DEAD)
                continue;
        }

		// update name:
		// 'DEAD' addresses will no longer be resolved,
		// 'DYING' addresses will be given another chance to resolve until they become 'DEAD'
		// 'ALIVE' addresses that fail to resolve will become 'DYING'
		// next, all managers with a valid address will participate to validation (see below)
		UpdateMLE(pMLE);

        // compare address families
        if (IsValidSockAddr(&pMLE->SockAddr) &&
            pMLE->SockAddr.sa_family == pSockAddr->sa_family) 
        {
        
            // determine address family
            if (pMLE->SockAddr.sa_family == AF_INET) 
            {
        
                struct sockaddr_in * pSockAddrIn1; 
                struct sockaddr_in * pSockAddrIn2; 

                // obtain pointer to protocol specific structure
                pSockAddrIn1= (struct sockaddr_in *)pSockAddr;
                pSockAddrIn2= (struct sockaddr_in *)&pMLE->SockAddr;

				// acknowledge this manager only if its address matches
				// a permitted manager with a valid (not NULL) IP address.
				// This is tested regardless the 'dwAge' of the permitted manager.
                if (!memcmp(&pSockAddrIn1->sin_addr,
                            &pSockAddrIn2->sin_addr,
                            sizeof(pSockAddrIn2->sin_addr))) 
                {

                    // transfer
                    *ppMLE = pMLE;

                    // success
                    return TRUE;
                }
        
            }
            else if (pMLE->SockAddr.sa_family == AF_IPX) 
            {

                struct sockaddr_ipx * pSockAddrIpx1; 
                struct sockaddr_ipx * pSockAddrIpx2; 

                // obtain pointer to protocol specific structure
                pSockAddrIpx1= (struct sockaddr_ipx *)pSockAddr;
                pSockAddrIpx2= (struct sockaddr_ipx *)&pMLE->SockAddr;

                // acknowledge this manager only if its ipx address matches a 
				// permitted manager with a valid (nodenum != 0) IPX address.
				// This is tested regardless the 'dwAge' of the permitted manager.
                if (!memcmp(pSockAddrIpx1->sa_netnum,
                            pSockAddrIpx2->sa_netnum,
                            sizeof(pSockAddrIpx2->sa_netnum)) &&
                    !memcmp(pSockAddrIpx1->sa_nodenum,
                            pSockAddrIpx2->sa_nodenum,
                            sizeof(pSockAddrIpx2->sa_nodenum))) 
                {

                    // transfer
                    *ppMLE = pMLE;

                    // success
                    return TRUE;
                }
            }
        }
    }

    // failure
    return FALSE;
}


BOOL
AddManager(
    PLIST_ENTRY pListHead,
    LPSTR       pManager
    )

/*++

Routine Description:

    Adds manager structure to list.

Arguments:

    pListHead - pointer to head of list.

    pManager - pointer to manager to add.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PMANAGER_LIST_ENTRY pMLE = NULL;

    // attempt to locate in list    
    if (FindManagerByName(&pMLE, pListHead, pManager)) {
                    
        SNMPDBG((
            SNMP_LOG_TRACE, 
            "SNMP: SVC: updating manager %s.\n",
            pManager
            ));

        // success
        fOk = TRUE;

    } else {

        // allocate manager structure
        if (AllocMLE(&pMLE, pManager)) {
                        
            SNMPDBG((
                SNMP_LOG_TRACE, 
                "SNMP: SVC: adding manager %s.\n",
                pManager
                ));

            // insert into managers list
            InsertTailList(pListHead, &pMLE->Link);

            // success
            fOk = TRUE;
        }
    }

    return fOk;
}


BOOL
LoadManagers(
    HKEY        hKey,
    PLIST_ENTRY pListHead
    )

/*++

Routine Description:

    Constructs list of permitted managers.

Arguments:

    hKey - registry key containing manager values.

    pListHead - pointer to head of list.

Return Values:

    Returns true if successful.

--*/

{
    LONG lStatus;
    DWORD dwIndex;
    DWORD dwNameSize;
    DWORD dwValueSize;
    DWORD dwValueType;
    CHAR  szName[MAX_PATH];
    CHAR  szValue[MAX_PATH]; // buffer for holding the translation UNICODE->UTF8
    BOOL fOk = FALSE;
    
    // initialize
    dwIndex = 0;
    lStatus = ERROR_SUCCESS;

    // loop until error or end of list
    while (lStatus == ERROR_SUCCESS)
    {
        // initialize buffer sizes
        dwNameSize  = sizeof(szName);
        dwValueSize = sizeof(szValue);

        szValue[0] = '\0';

        // read next value
        lStatus = RegEnumValueA(
                    hKey, 
                    dwIndex, 
                    szName, 
                    &dwNameSize,
                    NULL, 
                    &dwValueType, 
                    szValue, 
                    &dwValueSize
                    );

        // validate return code
        if (lStatus == ERROR_SUCCESS)
        {
            szValue[dwValueSize]='\0';

            if (AddManager(pListHead, szValue)) // add valid manager to manager list
                dwIndex++;  //next
            else
                lStatus = ERROR_NOT_ENOUGH_MEMORY;   // reset status to reflect failure
        }
        else if (lStatus == ERROR_NO_MORE_ITEMS)
            fOk = TRUE;     // success
    }
    
    return fOk;
}


BOOL
UnloadManagers(
    PLIST_ENTRY pListHead
    )

/*++

Routine Description:

    Destroys list of permitted managers.

Arguments:

    pListHead - pointer to head of list.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PMANAGER_LIST_ENTRY pMLE;

    // process entries until empty
    while (!IsListEmpty(pListHead)) {

        // extract next entry 
        pLE = RemoveHeadList(pListHead);

        // retrieve pointer to manager structure
        pMLE = CONTAINING_RECORD(pLE, MANAGER_LIST_ENTRY, Link);
 
        // release
        FreeMLE(pMLE);
    }

    return TRUE;
}


BOOL
LoadPermittedManagers(
    BOOL bFirstCall
    )

/*++

Routine Description:

    Constructs list of permitted managers.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    HKEY hKey;
    LONG lStatus;
    BOOL  fPolicy;
    LPTSTR pszKey;
    BOOL fOk = FALSE;
    
    SNMPDBG((
        SNMP_LOG_TRACE, 
        "SNMP: SVC: loading permitted managers.\n"
        ));

#ifdef _POLICY
    // we need to provide precedence to the parameters set through the policy
    fPolicy = TRUE;
#else
    fPolicy = FALSE;
#endif

    do
    {
        // if the policy is to be enforced, check the policy registry location first
        pszKey = fPolicy ? REG_POLICY_PERMITTED_MANAGERS : REG_KEY_PERMITTED_MANAGERS;

        // open registry subkey    
        lStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    pszKey,
                    0,
                    KEY_READ,
                    &hKey
                    );
        // if the call succeeded or we were not checking the policy, break the loop
        if (lStatus == ERROR_SUCCESS || !fPolicy)
            break;

        // being at this point, this means we were checking for the policy parameters.
        // If and only if the policy is not defined (registry key is missing) we
        // reset the error, mark 'fPolicy already tried' and go back into the loop
        if (lStatus == ERROR_FILE_NOT_FOUND)
        {
            lStatus = ERROR_SUCCESS;
            fPolicy = FALSE;
        }
    } while (lStatus == ERROR_SUCCESS);

    // validate return code
    if (lStatus == ERROR_SUCCESS) {
        
        // call routine to load managers into global list 
        LoadManagers(hKey, &g_PermittedManagers);

        // close key
        RegCloseKey(hKey);

        // at this point consider success (errors localized at particular managers were logged already)
        fOk = TRUE;
    } 
    else
        // it doesn't matter how the values are, the key has to exist,
        // so mark as bFirstCall in order to log an event if this is not true.
        bFirstCall = TRUE;
    
    if (!fOk) {
        
        SNMPDBG((
            SNMP_LOG_ERROR, 
            "SNMP: SVC: error %d processing PermittedManagers subkey.\n",
            lStatus
            ));

        // report an error only if on first call (service initialization)
        // otherwise, due to registry operations through regedit, the event log
        // might be flooded with records
        if (bFirstCall)
            // report event
            ReportSnmpEvent(
                SNMP_EVENT_INVALID_REGISTRY_KEY, 
                1, 
                &pszKey, 
                lStatus
                );
    }

    return fOk;
}


BOOL
UnloadPermittedManagers(
    )

/*++

Routine Description:

    Destroys list of permitted managers.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // call common routine with global list
    return UnloadManagers(&g_PermittedManagers);
}

