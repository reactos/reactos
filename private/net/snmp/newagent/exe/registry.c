/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Contains routines for manipulating registry parameters.

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
#include "registry.h"
#include "contexts.h"
#include "regions.h"
#include "trapmgrs.h"
#include "snmpmgrs.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT
InitRegistryNotifiers(
    )
/*++

Routine Description:

    Setup registry notifiers

Arguments:

    None.

Return Values:

    Returns the number of events that have been registered successfully

--*/
{
    DWORD  nEvents = 0;

    // on first call only create the default notifier
    if (g_hDefaultRegNotifier == NULL)
        g_hDefaultRegNotifier = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (g_hDefaultKey == NULL)
    {
        RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REG_KEY_SNMP_PARAMETERS,
                0,
                KEY_READ,
                &g_hDefaultKey
                );
    }

    // setup the default registry notifier
    if (g_hDefaultRegNotifier &&
        g_hDefaultKey &&
        SetEvent(g_hDefaultRegNotifier) &&
        RegNotifyChangeKeyValue(
            g_hDefaultKey,
			TRUE,
			REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
			g_hDefaultRegNotifier,
			TRUE
            ) == ERROR_SUCCESS)
    {
        SNMPDBG((SNMP_LOG_TRACE,
                 "SNMP: REG: Default reg notifier initialized successfully.\n"));
        nEvents++;
    }

#ifdef _POLICY
    // on first call only create the policy notifier
    if (g_hPolicyRegNotifier == NULL)
        g_hPolicyRegNotifier = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (g_hPolicyKey == NULL)
    {
        RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REG_POLICY_ROOT,
            0,
            KEY_READ,
            &g_hPolicyKey
            );
    }

    // setup the policy registry notifier
    if (g_hPolicyRegNotifier &&
        g_hPolicyKey &&
        SetEvent(g_hPolicyRegNotifier) &&
        RegNotifyChangeKeyValue(
            g_hPolicyKey,
            TRUE,
            REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
            g_hPolicyRegNotifier,
            TRUE
            ) == ERROR_SUCCESS)
    {
        SNMPDBG((SNMP_LOG_TRACE,
                "SNMP: REG: Policy reg notifier initialized successfully.\n"));
        nEvents++;
    }

    SNMPDBG((SNMP_LOG_TRACE,
            "SNMP: REG: Initialized notifiers ... %d.\n", nEvents));
#endif

    return nEvents;
}

BOOL UnloadRegistryNotifiers();

INT
WaitOnRegNotification(
    )
/*++

Routine Description:

    Blocking call - waits for a notification that one of the registry parameters has change

Arguments:

    None.

Return Values:

    Returns the notifyer index (0 for the termination event, !=0 for parameter change)

--*/
{
    HANDLE hNotifiers[3]; // hack - we now (hardcoded) that we are not going to wait for more than three events.
    DWORD  dwNotifiers = 0;
    DWORD  retCode;

    hNotifiers[dwNotifiers++] = g_hTerminationEvent;
    
    if (g_hDefaultRegNotifier != NULL)
        hNotifiers[dwNotifiers++] = g_hDefaultRegNotifier;

#ifdef _POLICY
    if (g_hPolicyRegNotifier != NULL)
        hNotifiers[dwNotifiers++] = g_hPolicyRegNotifier;
#endif

    SNMPDBG((SNMP_LOG_WARNING,
            "SNMP: REG: Will listen for params changes on %d notifiers.\n",
            dwNotifiers));

    retCode = WaitForMultipleObjects(
                dwNotifiers,
                hNotifiers,
                FALSE,
                INFINITE);

    UnloadRegistryNotifiers();

    return retCode;
}
                       
/*++
    Inplace parser for the string formatted OID.
    It is done in O(n) where n is the length of the string formatted OID (two passes)
--*/
BOOL
ConvStringToOid(
    LPTSTR  pStr,
    AsnObjectIdentifier *pOid)
{
    LPTSTR pDup;
    int    iComp;
    DWORD  dwCompValue;
    enum
    {   DOT,
        DIGIT
    }  state = DIGIT;

    // no need to check for parameters consistency (internal call->correct call :o)

    // check the consistency and determine the number of components
    pOid->idLength = 0;

    if (*pStr == _T('.'))   // skip a possible leading '.'
        pStr++;

    for (pDup = pStr; *pDup != _T('\0'); pDup++)
    {
        switch(state)
        {
        case DOT:
            // note: a trailing dot results in a trailing 0
            if (*pDup == _T('.'))
            {
                pOid->idLength++;
                state = DIGIT;
                break;
            }
            // intentionally missing 'break'
        case DIGIT:
            if (*pDup < _T('0') || *pDup > _T('9'))
                return FALSE;
            state = DOT;
            break;
        }
    }
    // add one to the id length as a trailing dot might not be present
    pOid->idLength++;

    // accept oids with two components at least;
    // alloc memory and check for success;
    if (pOid->idLength < 2 ||
        (pOid->ids = SnmpUtilMemAlloc(pOid->idLength * sizeof(UINT))) == NULL)
        return FALSE;

    // we have now enough buffer and a correct input string. Just convert it to OID
    iComp = 0;
    dwCompValue = 0;
    for (pDup = pStr; *pDup != _T('\0'); pDup++)
    {
        if (*pDup == _T('.'))
        {
            pOid->ids[iComp++] = dwCompValue;
            dwCompValue = 0;
        }
        else
        {
            dwCompValue = dwCompValue * 10 + (*pDup - _T('0'));
        }
    }
    pOid->ids[iComp] = dwCompValue;

    return TRUE;
}

BOOL
LoadScalarParameters(
    )

/*++

Routine Description:

    Reads authentication trap flags key and manager timeout value.

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
    LPTSTR pszKey = REG_KEY_SNMP_PARAMETERS;
    BOOL  bChangedSysID = FALSE;

	// default value for the IsnmpNameResolutionRetries counter
	// an address will resist to no more than MGRADDR_DYING (16 by default) 
	// consecutive name resolution failures.
	snmpMgmtBase.AsnIntegerPool[IsnmpNameResolutionRetries].asnValue.number = MGRADDR_DYING;

    // open registry subkey    
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS) 
	{
        // initialize
        dwIndex = 0;

        // loop until error or end of list
        while (lStatus == ERROR_SUCCESS) 
		{

            // initialize buffer sizes
            dwNameSize  = sizeof(szName) * sizeof(TCHAR);
            dwValueSize = sizeof(szValue) * sizeof(TCHAR);

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
            if (lStatus == ERROR_SUCCESS)
			{

                // validate name of value
                if (!lstrcmpi(szName, REG_VALUE_AUTH_TRAPS))
				{
					// set the 'EnableAuthenTraps' in the internal management structure
					mgmtISet(IsnmpEnableAuthenTraps, *((PDWORD)&szValue));
                }
				else if (!lstrcmpi(szName, REG_VALUE_MGRRES_COUNTER))
				{
					// set the 'NameResolutionRetries' in the internal management structure
					mgmtISet(IsnmpNameResolutionRetries, *((PDWORD)&szValue));
				}
                            
                // next
                dwIndex++;

            }
        }

        RegCloseKey(hKey);
    } 

    // look into MIB2 subtree ..SNMP\Parameters\RFC1156Agent for sysObjectID parameter
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REG_KEY_MIB2,
                0,
                KEY_READ,
                &hKey
                );

    // validate return code
    if (lStatus == ERROR_SUCCESS)
    {
        LPTSTR  pszOid = szValue;

        dwValueSize = MAX_PATH;

        // first, get the size of the buffer required for the sysObjectID parameter
        lStatus = RegQueryValueEx(
                    hKey,
                    REG_VALUE_SYS_OBJECTID,
                    0,
                    &dwValueType,
                    (LPBYTE)pszOid,
                    &dwValueSize);

        // the ERROR_MORE_DATA is the only error code we expect at this point
        if (lStatus == ERROR_MORE_DATA)
        {
            pszOid = SnmpUtilMemAlloc(dwValueSize);

            // if a buffer was set up correctly, go an read the oid value
            if (pszOid != NULL)
            {
                lStatus = RegQueryValueEx(
                            hKey,
                            REG_VALUE_SYS_OBJECTID,
                            0,
                            &dwValueType,
                            (LPBYTE)pszOid,
                            &dwValueSize);
            }
        }

        // at this point we should succeed
        if (lStatus == ERROR_SUCCESS)
        {
            AsnObjectIdentifier sysObjectID;
            // we have the string representation of the oid, convert it now to an AsnObjectIdentifier

            // implement the convertion here, as I don't want to make this a public function in SNMPAPI.DLL
            // otherwise I'll be forced to handle a lot of useless limit cases..
            if (dwValueType == REG_SZ &&
                ConvStringToOid(pszOid, &sysObjectID))
            {
                // don't free what has been alocated in ConvStringToOid as the buffer will be passed
                // to the management variable below.
                bChangedSysID = (mgmtOSet(OsnmpSysObjectID, &sysObjectID, FALSE) == ERROR_SUCCESS);
            }
            else
            {
                SNMPDBG((SNMP_LOG_WARNING,
                         "SNMP: SVC: LoadScalarParameters() - invalid type or value for sysObjectID param.\n"));

                ReportSnmpEvent(
                    SNMP_EVENT_INVALID_ENTERPRISEOID,
                    0,
                    NULL,
                    0);
            }
        }

        // cleanup the buffer if it was dynamically allocated
        if (pszOid != szValue)
            SnmpUtilMemFree(pszOid);

        // cleanup the registry key
        RegCloseKey(hKey);
    }

    if (!bChangedSysID)
    {
        mgmtOSet(OsnmpSysObjectID, SnmpSvcGetEnterpriseOID(), TRUE);
    }
    // all parameters here have default values, so there is no reason for this function to fail
	// if a parameter could not be found into the registry, its default value will be considered.
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
LoadRegistryParameters(
    )

/*++

Routine Description:

    Loads registry parameters.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // first thing to do is to setup the registry notifiers. If we don't do this before reading
    // the registry values we might not sense an initial change of the registry.
    InitRegistryNotifiers();

	// need to load first the scalar parameters especially to know how
	// to handle further the name resolution
    LoadScalarParameters();

    // load managers
    LoadPermittedManagers(TRUE);

    // load trap destinations
    LoadTrapDestinations(TRUE);

    // load communities with dynamic update
    LoadValidCommunities(TRUE);

    // load subagents
    LoadSubagents();

    // determine regions
    LoadSupportedRegions();

    return TRUE;
}

BOOL
UnloadRegistryNotifiers(
    )
/*++

Routine Description:

    Unloads registry notifiers

Arguments:

    None.

Return Values:

    Returns TRUE

--*/
{
    if (g_hDefaultRegNotifier != NULL)
    {
        CloseHandle(g_hDefaultRegNotifier);
        g_hDefaultRegNotifier = NULL;
    }
#ifdef _POLICY
    if (g_hPolicyRegNotifier != NULL)
    {
        CloseHandle(g_hPolicyRegNotifier);
        g_hPolicyRegNotifier = NULL;
    }
#endif

    if (g_hDefaultKey != NULL)
    {
        RegCloseKey(g_hDefaultKey);
        g_hDefaultKey = NULL;
    }
#ifdef _POLICY
    if (g_hPolicyKey != NULL)
    {
        RegCloseKey(g_hPolicyKey);
        g_hPolicyKey = NULL;
    }
#endif

    return TRUE;
}


BOOL
UnloadRegistryParameters(
    )

/*++

Routine Description:

    Unloads registry parameters.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // unload the registry notifiers as the first thing to do
    UnloadRegistryNotifiers();

    // unload managers
    UnloadPermittedManagers();

    // unload trap destinations
    UnloadTrapDestinations();

    // unload communities
    UnloadValidCommunities();

    // unload subagents
    UnloadSubagents();

    // unload mib regions
    UnloadSupportedRegions();

    return TRUE;
}

