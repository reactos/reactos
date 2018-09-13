/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    trapmgrs.c

Abstract:

    Contains routines for manipulating trap destination structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "snmpmgrs.h"
#include "trapmgrs.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
FindTrapDestination(
    PTRAP_DESTINATION_LIST_ENTRY * ppTLE,
    LPSTR                          pCommunity
    )

/*++

Routine Description:

    Locates valid trap destination in list.

Arguments:

    ppTLE - pointer to receive pointer to entry.

    pCommunity - pointer to trap destination to find.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PTRAP_DESTINATION_LIST_ENTRY pTLE;

    // initialize
    *ppTLE = NULL;

    // obtain pointer to list head
    pLE = g_TrapDestinations.Flink;

    // process all entries in list
    while (pLE != &g_TrapDestinations) {

        // retrieve pointer to trap destination structure
        pTLE = CONTAINING_RECORD(pLE, TRAP_DESTINATION_LIST_ENTRY, Link);

        // compare trap destination string with entry
        if (!strcmp(pTLE->pCommunity, pCommunity)) {

            // transfer
            *ppTLE = pTLE;

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
AddTrapDestination(
    HKEY   hKey,
    LPWSTR pwCommunity
    )

/*++

Routine Description:

    Adds trap destination to list.

Arguments:

    hKey - trap destination subkey.

    pwCommunity - pointer to trap destination to add.

Return Values:

    Returns true if successful.

--*/

{
    HKEY hSubKey;
    LONG lStatus;
    BOOL fOk = FALSE;
    PTRAP_DESTINATION_LIST_ENTRY pTLE = NULL;
    LPSTR pCommunity = NULL;

    // open registry subkey    
    lStatus = RegOpenKeyExW(
                hKey,
                pwCommunity,
                0,
                KEY_READ,
                &hSubKey
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS &&
        SnmpUtilUnicodeToUTF8(
            &pCommunity,
            pwCommunity,
            TRUE) == 0) {

        // attempt to locate in list    
        if (FindTrapDestination(&pTLE, pCommunity)) {
                            
            SNMPDBG((
                SNMP_LOG_TRACE, 
                "SNMP: SVC: updating trap destinations for %s.\n",
                pCommunity
                ));
            
            // load associated managers
            LoadManagers(hSubKey, &pTLE->Managers);

            // success
            fOk = TRUE;

        } else {

            // allocate trap destination structure
            if (AllocTLE(&pTLE, pCommunity)) {
                                
                SNMPDBG((
                    SNMP_LOG_TRACE, 
                    "SNMP: SVC: adding trap destinations for %s.\n",
                    pCommunity
                    ));

                // load associated managers
                if (LoadManagers(hSubKey, &pTLE->Managers)) {

                    // insert into valid communities list
                    InsertTailList(&g_TrapDestinations, &pTLE->Link);

                    // success
                    fOk = TRUE;
                }

                // cleanup
                if (!fOk) {

                    // release
                    FreeTLE(pTLE);
                }
            }
        }

        // release subkey
        RegCloseKey(hSubKey);

        SnmpUtilMemFree(pCommunity);
    }

    return fOk;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocTLE(
    PTRAP_DESTINATION_LIST_ENTRY * ppTLE,
    LPSTR                          pCommunity 
    )

/*++

Routine Description:

    Allocates trap destination structure and initializes.

Arguments:

    ppTLE - pointer to receive pointer to entry.

    pCommunity - pointer to trap destination string.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PTRAP_DESTINATION_LIST_ENTRY pTLE = NULL;

    // attempt to allocate structure
    pTLE = AgentMemAlloc(sizeof(TRAP_DESTINATION_LIST_ENTRY));

    // validate
    if (pTLE != NULL) {

        // allocate memory for trap destination string
        pTLE->pCommunity = AgentMemAlloc(strlen(pCommunity)+1);

        // validate
        if (pTLE->pCommunity != NULL) {

            // transfer trap destination string
            strcpy(pTLE->pCommunity, pCommunity);

            // initialize list of managers
            InitializeListHead(&pTLE->Managers);

            // success
            fOk = TRUE;
        } 

        // cleanup        
        if (!fOk) {

            // release 
            FreeTLE(pTLE);

            // re-init
            pTLE = NULL;            
        }
    }

    // transfer
    *ppTLE = pTLE;

    return fOk;
}


BOOL 
FreeTLE(
    PTRAP_DESTINATION_LIST_ENTRY pTLE
    )

/*++

Routine Description:

    Releases trap destination structure.

Arguments:

    pTLE - pointer to trap destination list entry to be freed.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // validate pointer
    if (pTLE != NULL) {

        // release manager structures
        UnloadManagers(&pTLE->Managers);

        // release string
        AgentMemFree(pTLE->pCommunity);

        // release structure
        AgentMemFree(pTLE);
    }

    return TRUE;
}


BOOL
LoadTrapDestinations(
    BOOL bFirstCall
    )

/*++

Routine Description:

    Constructs list of trap destinations.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    HKEY hKey;
    LONG lStatus;
    DWORD dwIndex;
    WCHAR wszName[MAX_PATH];
    BOOL  fPolicy;
    LPTSTR pszKey;
    BOOL fOk = FALSE;
        
    SNMPDBG((
        SNMP_LOG_TRACE, 
        "SNMP: SVC: loading trap destinations.\n"
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
        pszKey = fPolicy ? REG_POLICY_TRAP_DESTINATIONS : REG_KEY_TRAP_DESTINATIONS;

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

        // initialize
        dwIndex = 0;

        // loop until error or end of list
        while (lStatus == ERROR_SUCCESS) {

            // read next value
            lStatus = RegEnumKeyW(
                        hKey, 
                        dwIndex, 
                        wszName, 
                        sizeof(wszName)
                        );

            // validate return code
            if (lStatus == ERROR_SUCCESS) {

                // add trap destination to list 
                if (AddTrapDestination(hKey, wszName)) {

                    // next
                    dwIndex++;

                } else {

                    // reset status to reflect failure
                    lStatus = ERROR_NOT_ENOUGH_MEMORY;
                }
            
            } else if (lStatus == ERROR_NO_MORE_ITEMS) {

                // success
                fOk = TRUE; 
            }
        }
    }
    else
        // it doesn't matter how the values are, the key has to exist,
        // so mark as bFirstCall in order to log an event if this is not true.
        bFirstCall = TRUE;    
    
    if (!fOk) {
        
        SNMPDBG((
            SNMP_LOG_ERROR, 
            "SNMP: SVC: error %d processing TrapDestinations subkey.\n",
            lStatus
            ));

        // log an event only if on first call (service initialization)
        // otherwise, due to registry operations through regedit, the event log
        // might get flooded with records
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
UnloadTrapDestinations(
    )

/*++

Routine Description:

    Destroys list of trap destinations.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PTRAP_DESTINATION_LIST_ENTRY pTLE;

    // process entries until list is empty
    while (!IsListEmpty(&g_TrapDestinations)) {

        // extract next entry from head of list
        pLE = RemoveHeadList(&g_TrapDestinations);

        // retrieve pointer to trap destination structure
        pTLE = CONTAINING_RECORD(pLE, TRAP_DESTINATION_LIST_ENTRY, Link);
 
        // release
        FreeTLE(pTLE);
    }

    return TRUE; 
}


