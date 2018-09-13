/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    subagnts.c

Abstract:

    Contains definitions for manipulating subagent structures.

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
#include "subagnts.h"
#include "regions.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
FindSubagent(
    PSUBAGENT_LIST_ENTRY * ppSLE,
    LPSTR                 pPathname
    )

/*++

Routine Description:

    Locates subagent in list.

Arguments:

    ppSLE - pointer to receive pointer to entry.

    pPathname - pointer to pathname to find.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PSUBAGENT_LIST_ENTRY pSLE;

    // initialize
    *ppSLE = NULL;

    // obtain pointer to head
    pLE = g_Subagents.Flink;

    // process all entries in list
    while (pLE != &g_Subagents) {

        // retrieve pointer to trap destination structure
        pSLE = CONTAINING_RECORD(pLE, SUBAGENT_LIST_ENTRY, Link);

        // compare pathname string with entry
        if (!strcmp(pSLE->pPathname, pPathname)) {

            // transfer
            *ppSLE = pSLE;

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
AddSubagentRegion(
    PSUBAGENT_LIST_ENTRY  pSLE,
    AsnObjectIdentifier * pPrefixOid
    )

/*++

Routine Description:

    Adds subagent supported region to structure.

Arguments:

    pSLE - pointer to subagent structure.

    pPrefixOid - pointer to supported region.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PMIB_REGION_LIST_ENTRY pRLE = NULL;

    // allocate region
    if (AllocRLE(&pRLE)) {

        // save pointer
        pRLE->pSLE = pSLE;

        // copy prefix to structure
        SnmpUtilOidCpy(&pRLE->PrefixOid, pPrefixOid);

        // copy prefix as temporary limit
        SnmpUtilOidCpy(&pRLE->LimitOid, pPrefixOid);

        // modify limit oid to be one past the prefix
        ++pRLE->LimitOid.ids[pRLE->LimitOid.idLength - 1];
        
        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: %s supports %s.\n",
            pSLE->pPathname,
            SnmpUtilOidToA(&pRLE->PrefixOid)
            ));

        // attach to mib region to subagent structure
        InsertTailList(&pSLE->SupportedRegions, &pRLE->Link);

        // success
        fOk = TRUE;
    }

    return fOk;
}

BOOL
OfferInternalMgmtVariables(
    PSUBAGENT_LIST_ENTRY pSLE
    )
/*++

Routine Description:

    if the subagent is willing to monitor the SNMP service
    this function is offering it a pointer to the internal
    management variables

Arguments:

    pSLE - pointer to subagent structure.

Return Values:

    Returns true anyway.

--*/
{
    if (pSLE->pfnSnmpExtensionMonitor != NULL)
    {
       __try {

            // attempt to initialize agent
            (*pSLE->pfnSnmpExtensionMonitor)(&snmpMgmtBase);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
        
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: exception 0x%08lx offering internals to %s.\n",
                GetExceptionCode(),
                pSLE->pPathname
                ));

            // failure
            return FALSE;
        }
    }
    
    return TRUE;
}


BOOL
LoadSubagentRegions(
    PSUBAGENT_LIST_ENTRY pSLE
    )

/*++

Routine Description:

    Loads subagent supported regions.

Arguments:

    pSLE - pointer to subagent structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    HANDLE hSubagentTrapEvent = NULL;
    AsnObjectIdentifier PrefixOid = { 0, NULL };

    __try {

        // attempt to initialize agent
        if ((*pSLE->pfnSnmpExtensionInit)(
                        g_dwUpTimeReference,    
                        &hSubagentTrapEvent,
                        &PrefixOid
                        )) {

            // store subagent trap event handle
            pSLE->hSubagentTrapEvent = hSubagentTrapEvent;

            // add subagent region to list entry
            fOk = AddSubagentRegion(pSLE, &PrefixOid);

            // check to see if subagent supports additional regions
            if (fOk && (pSLE->pfnSnmpExtensionInitEx != NULL)) {    

                BOOL fMoreRegions = TRUE;

                // get other regions
                while (fOk && fMoreRegions) {
    
                    // retrieve next supported region
                    fMoreRegions = (*pSLE->pfnSnmpExtensionInitEx)(
                                                &PrefixOid
                                                );

                    // validate
                    if (fMoreRegions) {

                        // add subagent region to list entry
                        fOk = AddSubagentRegion(pSLE, &PrefixOid);
                    }
                }
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: exception 0x%08lx loading %s.\n",
            GetExceptionCode(),
            pSLE->pPathname
            ));

        // failure
        fOk = FALSE;
    }
    
    return fOk;
}


BOOL
LoadSubagent(
    PSUBAGENT_LIST_ENTRY pSLE
    )

/*++

Routine Description:

    Loads subagent dll and initializes.

Arguments:

    pSLE - pointer to subagent structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;    

    // attempt to load subagent library
    pSLE->hSubagentDll = LoadLibraryA(pSLE->pPathname);

    // validate handle
    if (pSLE->hSubagentDll != NULL) {

        // load primary initialization routine
        pSLE->pfnSnmpExtensionInit = (PFNSNMPEXTENSIONINIT)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_INIT
                );

        // load secondary initialization routine
        pSLE->pfnSnmpExtensionInitEx = (PFNSNMPEXTENSIONINITEX)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_INIT_EX
                );

                // load secondary initialization routine
        pSLE->pfnSnmpExtensionClose = (PFNSNMPEXTENSIONCLOSE)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_CLOSE
                );

        // load the extension monitor routine
        pSLE->pfnSnmpExtensionMonitor = (PFNSNMPEXTENSIONMONITOR)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_MONITOR
                );

        // load snmpv1-based subagent request routine
        pSLE->pfnSnmpExtensionQuery = (PFNSNMPEXTENSIONQUERY)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_QUERY
                );

        // load snmpv2-based subagent request routine
        pSLE->pfnSnmpExtensionQueryEx = (PFNSNMPEXTENSIONQUERYEX)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_QUERY_EX
                );

        // load snmpv1-based subagent trap routine
        pSLE->pfnSnmpExtensionTrap = (PFNSNMPEXTENSIONTRAP)
            GetProcAddress(
                pSLE->hSubagentDll,
                SNMP_EXTENSION_TRAP
                );

        // validate subagent agent entry points
        if ((pSLE->pfnSnmpExtensionInit != NULL) &&
           ((pSLE->pfnSnmpExtensionQuery != NULL) ||
            (pSLE->pfnSnmpExtensionQueryEx != NULL))) {

            // load supported regions
            if (fOk = LoadSubagentRegions(pSLE)) // !!intentional assignement!!
            {
                // offering internal management variables;
                fOk = OfferInternalMgmtVariables(pSLE);
            }
        }

    }
    else
    {
        DWORD errCode = GetLastError();
        LPTSTR pPathname;

#ifdef UNICODE
        SnmpUtilUTF8ToUnicode(&pPathname, pSLE->pPathname, TRUE);
#else
        pPathname = pSLE->pPathname;
#endif
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d loading subagent.\n",
            errCode
            ));

        ReportSnmpEvent(
            SNMP_EVENT_INVALID_EXTENSION_AGENT_DLL,
            1,
            &pPathname,
            errCode);

#ifdef UNICODE
        SnmpUtilMemFree(pPathname);
#endif
    }

    return fOk;
}


BOOL
AddSubagentByDll(
    LPSTR pPathname,
    UCHAR uchInitFlags
    )

/*++

Routine Description:

    Adds subagent to list.

Arguments:

    pPathname - pointer to subagent's dll path.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PSUBAGENT_LIST_ENTRY pSLE = NULL;
    
    // attempt to locate in list    
    if (FindSubagent(&pSLE, pPathname)) {
                    
        SNMPDBG((
            SNMP_LOG_WARNING, 
            "SNMP: SVC: duplicate entry for %s.\n",
            pPathname
            ));
        
        // success
        fOk = TRUE;

    } else {

        // allocate subagent structure
        if (AllocSLE(&pSLE, pPathname, uchInitFlags)) {
                        
            SNMPDBG((
                SNMP_LOG_TRACE, 
                "SNMP: SVC: processing subagent %s.\n",
                pPathname
                ));

            // initialize subagent
            if (LoadSubagent(pSLE)) {

                // insert into valid communities list
                InsertTailList(&g_Subagents, &pSLE->Link);

                // success
                fOk = TRUE;
            } 
            
            // cleanup
            if (!fOk) {

                // release
                FreeSLE(pSLE);
            }
        }
    }

    return fOk;
}


BOOL
AddSubagentByKey(
    LPTSTR pKey
    )

/*++

Routine Description:

    Adds subagent to list.

Arguments:

    pKey - pointer to subagent's registry key path.

Return Values:

    Returns true if successful.

--*/

{
    HKEY hKey;
    LONG lStatus;
    DWORD dwIndex;
    DWORD dwNameSize;
    DWORD dwValueSize;
    DWORD dwValueType;
    CHAR szName[MAX_PATH];
    CHAR szValue[MAX_PATH];
    BOOL fOk = FALSE;
    PSUBAGENT_LIST_ENTRY pSLE = NULL;

    // open registry subkey    
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS) {
        
        // initialize
        dwIndex = 0;

        // initialize buffer sizes
        dwNameSize  = sizeof(szName);
        dwValueSize = sizeof(szValue);

        // loop until error or end of list
        while (lStatus == ERROR_SUCCESS) {

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
            if (lStatus == ERROR_SUCCESS) {
            
                // check to see if value is pathname
                if (!_stricmp(szName, REG_VALUE_SUBAGENT_PATH)) {
    
                    DWORD dwRequired;
                    CHAR szExpanded[MAX_PATH];
                    
                    // expand environment strings in path
                    dwRequired = ExpandEnvironmentStringsA(
                                    szValue,
                                    szExpanded,
                                    sizeof(szExpanded)
                                    );

                    // load subagent library - no flags set
                    fOk = AddSubagentByDll(szExpanded, 0);
            
                    break; // bail...
                }

                // initialize buffer sizes
                dwNameSize  = sizeof(szName);
                dwValueSize = sizeof(szValue);

                // next
                dwIndex++;
            
            } else if (lStatus == ERROR_NO_MORE_ITEMS) {

                // failure
                fOk = FALSE; 
            }
        }

        // release handle
        RegCloseKey(hKey);
    }
    else
    {
        // the registry key for this subagent could not be located.
        ReportSnmpEvent(
            SNMP_EVENT_INVALID_EXTENSION_AGENT_KEY,
            1,
            &pKey,
            lStatus);
    }

    return fOk;    
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocSLE(
    PSUBAGENT_LIST_ENTRY * ppSLE,
    LPSTR                  pPathname,
    UCHAR                  uchInitFlags
    )

/*++

Routine Description:

    Allocates trap destination structure and initializes.

Arguments:

    ppSLE - pointer to receive pointer to entry.

    pPathname - pointer to subgent's dll path.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PSUBAGENT_LIST_ENTRY pSLE = NULL;

    // attempt to allocate structure
    pSLE = AgentMemAlloc(sizeof(SUBAGENT_LIST_ENTRY));

    // validate
    if (pSLE != NULL) {

        // allocate memory for trap destination string
        pSLE->pPathname = AgentMemAlloc(strlen(pPathname)+1);

        // validate
        if (pSLE->pPathname != NULL) {

            // transfer trap destination string
            strcpy(pSLE->pPathname, pPathname);

            // set the initial flags value
            pSLE->uchFlags = uchInitFlags;

            // initialize list of supported regions
            InitializeListHead(&pSLE->SupportedRegions);

            // success
            fOk = TRUE;
        } 

        // cleanup        
        if (!fOk) {

            // release 
            FreeSLE(pSLE);

            // re-init
            pSLE = NULL;            
        }
    }

    // transfer
    *ppSLE = pSLE;

    return fOk;
}


BOOL 
FreeSLE(
    PSUBAGENT_LIST_ENTRY pSLE
    )

/*++

Routine Description:

    Releases subagent structure.

Arguments:

    pSLE - pointer to list entry to be freed.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // validate pointer
    if (pSLE != NULL) {

        SNMPDBG((  
            SNMP_LOG_VERBOSE,
            "SNMP: SVC: unloading %s.\n",
            pSLE->pPathname
            ));

        // release manager structures
        UnloadRegions(&pSLE->SupportedRegions);

        // validate subagent dll handle    
        if (pSLE->hSubagentDll != NULL) {

            __try {
                if (pSLE->pfnSnmpExtensionClose != NULL)
                    (*pSLE->pfnSnmpExtensionClose)();

                // unload subagent
                FreeLibrary(pSLE->hSubagentDll);

            } __except (EXCEPTION_EXECUTE_HANDLER) {
        
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: exception 0x%08lx unloading %s.\n",
                    GetExceptionCode(),
                    pSLE->pPathname
                    ));
            }
        }

        // release string
        AgentMemFree(pSLE->pPathname);

        // release structure
        AgentMemFree(pSLE);
    }

    return TRUE;
}


BOOL
LoadSubagents(
    )

/*++

Routine Description:

    Constructs list of subagents.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    HKEY hKey;
    LONG lStatus;
    DWORD dwIndex;
    DWORD dwNameSize;
    DWORD dwValueSize;
    DWORD dwValueType;
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    LPTSTR pszKey = REG_KEY_EXTENSION_AGENTS;
    BOOL fOk = FALSE;
        
    SNMPDBG((
        SNMP_LOG_TRACE, 
        "SNMP: SVC: loading subagents.\n"
        ));
    
    // open registry subkey    
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS) {

        // initialize
        dwIndex = 0;

        // initialize buffer sizes
        dwNameSize  = sizeof(szName);
        dwValueSize = sizeof(szValue);

        // loop until error or end of list
        while (lStatus == ERROR_SUCCESS) {

            // read next value
            lStatus = RegEnumValue(
                        hKey, 
                        dwIndex, 
                        szName, 
                        &dwNameSize,
                        NULL, 
                        &dwValueType, 
                        (LPBYTE)szValue, 
                        &dwValueSize
                        );

            // validate return code
            if (lStatus == ERROR_SUCCESS) {

                // add subagent to list 
                AddSubagentByKey(szValue);
                    
                // re-initialize buffer sizes
                dwNameSize  = sizeof(szName);
                dwValueSize = sizeof(szValue);

                // next
                dwIndex++;

            } else if (lStatus == ERROR_NO_MORE_ITEMS) {

                // success
                fOk = TRUE; 
            }
        }
    } 
    
    if (!fOk) {
        
        SNMPDBG((
            SNMP_LOG_ERROR, 
            "SNMP: SVC: error %d processing Subagents subkey.\n",
            lStatus
            ));
        
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
UnloadSubagents(
    )

/*++

Routine Description:

    Destroys list of subagents.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PSUBAGENT_LIST_ENTRY pSLE;

    // process entries until list is empty
    while (!IsListEmpty(&g_Subagents)) {

        // extract next entry from head of list
        pLE = RemoveHeadList(&g_Subagents);

        // retrieve pointer to community structure
        pSLE = CONTAINING_RECORD(pLE, SUBAGENT_LIST_ENTRY, Link);
 
        // release
        FreeSLE(pSLE);
    }

    return TRUE; 
}
