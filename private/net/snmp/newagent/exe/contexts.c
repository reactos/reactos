/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    contexts.c

Abstract:

    Contains routines for manipulating SNMP community structures.

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
#include "contexts.h"
#include "snmpthrd.h"

#define DYN_REGISTRY_UPDATE	1

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AddValidCommunity(
    LPWSTR pCommunity,
    DWORD dwAccess
    )

/*++

Routine Description:

    Adds valid community to list.

Arguments:

    pCommunity - pointer to community to add.

    dwAccess - access rights for community.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PCOMMUNITY_LIST_ENTRY pCLE = NULL;
    AsnOctetString CommunityOctets;
    
    // initialize octet string info
    CommunityOctets.length  = wcslen(pCommunity) * sizeof(WCHAR);
    CommunityOctets.stream  = (LPBYTE)pCommunity;
    CommunityOctets.dynamic = FALSE;

    // attempt to locate in list    
    if (FindValidCommunity(&pCLE, &CommunityOctets)) {
                    
        SNMPDBG((
            SNMP_LOG_TRACE, 
            "SNMP: SVC: updating community %s.\n",
            StaticUnicodeToString((LPWSTR)pCommunity)
            ));

        // update access rights
        pCLE->dwAccess = dwAccess;

        // success
        fOk = TRUE;

    } else {

        // allocate community structure
        if (AllocCLE(&pCLE, pCommunity)) {
                            
            SNMPDBG((
                SNMP_LOG_TRACE, 
                "SNMP: SVC: adding community %s.\n",
                CommunityOctetsToString(&(pCLE->Community), TRUE)
                ));

            // insert into valid communities list
            InsertTailList(&g_ValidCommunities, &pCLE->Link);

            // update access rights
            pCLE->dwAccess = dwAccess;

            // success
            fOk = TRUE;
        }
    }

    return fOk;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AllocCLE(
    PCOMMUNITY_LIST_ENTRY * ppCLE,
    LPWSTR                  pCommunity
    )

/*++

Routine Description:

    Allocates community structure and initializes.

Arguments:

    ppCLE - pointer to receive pointer to entry.

    pCommunity - pointer to community string.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PCOMMUNITY_LIST_ENTRY pCLE = NULL;

    // attempt to allocate structure
    pCLE = AgentMemAlloc(sizeof(COMMUNITY_LIST_ENTRY));

    // validate
    if (pCLE != NULL) {
        
        // determine string length
        DWORD nBytes = wcslen(pCommunity) * sizeof(WCHAR);

        // allocate memory for string (include terminator)
        pCLE->Community.stream = SnmpUtilMemAlloc(nBytes + sizeof(WCHAR));

        // validate community string stream
        if (pCLE->Community.stream != NULL) {

            // set length of manager string
            pCLE->Community.length = nBytes;
    
            // set memory allocation flag 
            pCLE->Community.dynamic = TRUE;

            // transfer community string into octets
            wcscpy((LPWSTR)(pCLE->Community.stream), pCommunity);

            // success
            fOk = TRUE;

        } else {
            
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: could not copy community string %s.\n",
                StaticUnicodeToString(pCommunity)
                ));

            // release 
            FreeCLE(pCLE);

            // re-init
            pCLE = NULL;            
        }

    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: could not allocate context entry for %s.\n",
            StaticUnicodeToString(pCommunity)
            ));
    }

    // transfer
    *ppCLE = pCLE;

    return fOk;
}


BOOL 
FreeCLE(
    PCOMMUNITY_LIST_ENTRY pCLE
    )

/*++

Routine Description:

    Releases community structure.

Arguments:

    pCLE - pointer to community list entry to be freed.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    if (pCLE != NULL) {

        // release octet string contents
        SnmpUtilOctetsFree(&pCLE->Community);

        // release structure
        AgentMemFree(pCLE);
    }

    return TRUE;
}


BOOL
FindValidCommunity(
    PCOMMUNITY_LIST_ENTRY * ppCLE,
    AsnOctetString *        pCommunity
    )

/*++

Routine Description:

    Locates valid community in list.

Arguments:

    ppCLE - pointer to receive pointer to entry.

    pCommunity - pointer to community to find.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PCOMMUNITY_LIST_ENTRY pCLE;

    // initialize
    *ppCLE = NULL;

    // obtain pointer to list head
    pLE = g_ValidCommunities.Flink;

    // process all entries in list
    while (pLE != &g_ValidCommunities) {

        // retrieve pointer to community structure
        pCLE = CONTAINING_RECORD(pLE, COMMUNITY_LIST_ENTRY, Link);

        // compare community string with entry
        if (!SnmpUtilOctetsCmp(&pCLE->Community, pCommunity)) {

            // transfer
            *ppCLE = pCLE;

            // success
            return TRUE;
        }

        // next entry
        pLE = pLE->Flink;
    }

    // failure
    return FALSE;
}


DWORD
ParsePermissionMask(
	DWORD bitMask
	)
/*++

Routine Description:

    Translates the permission mask from the bit-mask format (registry)
	into the internal constant value (constants from public\sdk\inc\snmp.h).
	The function works no longer if:
	- more than sizeof(DWORD)*8 permission values are defined
	- constant values (access policy) changes

Arguments:

    bit-mask.

Return Values:

    permission's constant value.

--*/
{
	DWORD dwPermission;

	for(dwPermission = 0; (bitMask & ((DWORD)(-1)^1)) != 0; bitMask>>=1, dwPermission++);

	return dwPermission;
}

#ifdef DYN_REGISTRY_UPDATE
LONG UpdateRegistry(
	HKEY hKey,
	LPWSTR wszBogus,
	LPWSTR wszCommunity
	)
/*++

Routine Description:

    Updates the registry configuration in order to be able to associate
	permission masks to each community:
				name			type		data
	old format: <whatever>		REG_SZ		community_name
	new format: community_name	REG_DWORD	permission_mask
Arguments:

    hKey - open handle to the key that contains the value
	szBogus - old format value name; useless data
	szCommunity - pointer to community name, as it was specified in the old format.

Return Values:

    Returns ERROR_SUCCESS if successful.

--*/
{
    LONG lStatus;
    DWORD dwDataSize = MAX_PATH;
    DWORD dwDataType;

	// make sure the update was not tried (and breaked) before
	dwDataSize = sizeof(DWORD);
	lStatus = RegQueryValueExW(
				hKey,
				wszCommunity,
				0,
				&dwDataType,
				NULL,
				&dwDataSize);

	// if no previous (breaked) update, convert community to the new format
	if (lStatus != ERROR_SUCCESS || dwDataType != REG_DWORD)
	{
		// permissions to be assigned to community
		DWORD dwPermissionMask;
		
        // all communities that are converted to new format at this point,
        // are converted to READ-CREATE permissions to ensure same functionality as
        // the permisionless communities.
        dwPermissionMask = 1 << SNMP_ACCESS_READ_CREATE;

		// set the new format value
		lStatus = RegSetValueExW(
					hKey,
					wszCommunity,
					0,
					REG_DWORD,
					(CONST BYTE *)&dwPermissionMask,
					sizeof(DWORD));

		if (lStatus != ERROR_SUCCESS)
			return lStatus;
	}

	// delete the old format value
	lStatus = RegDeleteValueW(
				hKey,
				wszBogus);

	return lStatus;
}
#endif


BOOL
LoadValidCommunities(
    BOOL bFirstCall
    )

/*++

Routine Description:

    Constructs list of valid communities.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    HKEY hKey;
    LONG lStatus;
    DWORD dwIndex;
    WCHAR wszName[MAX_PATH];  // get the UNICODE encoding for szName
    CHAR  szName[3*MAX_PATH]; // buffer for holding the translation UNICODE->UTF8
    DWORD dwNameSize;
    DWORD dwDataType;
	WCHAR wszData[MAX_PATH];  // get the UNICODE encoding for szData
    DWORD dwDataSize;
    BOOL fPolicy;
    LPTSTR pszKey;
    BOOL fOk = FALSE;
    
    SNMPDBG((
        SNMP_LOG_TRACE, 
        "SNMP: SVC: loading valid communities.\n"
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
        pszKey = fPolicy ? REG_POLICY_VALID_COMMUNITIES : REG_KEY_VALID_COMMUNITIES;

        // open registry subkey    
        lStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    pszKey,
                    0,
#ifdef DYN_REGISTRY_UPDATE
                    bFirstCall ? KEY_READ | KEY_SET_VALUE : KEY_READ,
#else
				    KEY_READ,
#endif
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
		for (dwIndex = 0;
			 lStatus == ERROR_SUCCESS; 
             dwIndex++)

		{
            // initialize buffer sizes
            dwNameSize = sizeof(wszName) * sizeof(WCHAR);
            dwDataSize = sizeof(wszData) * sizeof(WCHAR);

            // read next value
            lStatus = RegEnumValueW(
                        hKey, 
                        dwIndex, 
                        wszName, 
                        &dwNameSize,
                        NULL, 
                        &dwDataType, 
                        (LPBYTE)wszData, 
                        &dwDataSize
                        );

            // validate return code
            if (lStatus == ERROR_SUCCESS) {

				// dynamically update values that are not of DWORD type
				if (dwDataType != REG_DWORD)
				{
#ifdef DYN_REGISTRY_UPDATE
					if (dwDataType == REG_SZ)
					{
						if (bFirstCall)
                        {
                            if(UpdateRegistry(hKey, wszName, wszData) == ERROR_SUCCESS)
						    {
							    SNMPDBG((
								    SNMP_LOG_WARNING, 
								    "SNMP: SVC: updated community registration\n"
							    ));

							    // current value has been deleted, need to keep the index
							    dwIndex--;
							    continue;
						    }
                        }
                        else
                        {
                            SNMPDBG((
                                SNMP_LOG_WARNING,
                                "SNMP: SVC: old format community to be considered with full rights"));

                            wcscpy(wszName, wszData);
                            *(DWORD *)wszData = (1 << SNMP_ACCESS_READ_CREATE);
                        }
                    }
                    else
#endif
                    {
					    SNMPDBG((
						    SNMP_LOG_WARNING, 
						    "SNMP: SVC: wrong format in ValidCommunities[%d] registry entry\n",
						    dwIndex
					    ));
					    continue;
                    }
				}

                // convert the UNICODE representation to UTF8 representation
                //dwNameSize = WideCharToMultiByte(
                //        CP_UTF8,
                //        0,
                //        wszName,
                //        wcslen(wszName),
                //        szName,
                //        sizeof(szName),
                //        NULL,
                //        NULL);

                // if error, just skip this community
                //if (dwNameSize == 0)
                //{
                //    SNMPDBG((
                //        SNMP_LOG_WARNING,
                //        "SNMP: SVC: community conversion to UTF8 failed with error %d.\n", GetLastError()));
                //    continue;
                //}

                // put the '\0' terminator to the string
                //szName[dwNameSize] = '\0';

                // add valid community to list with related permissions
                //if (AddValidCommunity(szName, ParsePermissionMask(*(DWORD *)wszData)))
                if (AddValidCommunity(wszName, ParsePermissionMask(*(DWORD *)wszData)))
				{

			        SNMPDBG((
			            SNMP_LOG_WARNING, 
						"SNMP: SVC: rights set to %d for community '%s'\n",
						*(DWORD *)wszData,
						StaticUnicodeToString(wszName)
					));

                }
				else
				{
                    // reset status to reflect failure
                    lStatus = ERROR_NOT_ENOUGH_MEMORY;
                }
            
            }
			else if (lStatus == ERROR_NO_MORE_ITEMS)
			{
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
            "SNMP: SVC: error %d processing ValidCommunities subkey.\n",
            lStatus
            ));

        // report an event only for the first call (initialization of the service).
        // otherwise subsequent registry ops through regedit might flood the event log with
        // unsignificant records
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
UnloadValidCommunities(
    )

/*++

Routine Description:

    Destroys list of valid communities.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    PLIST_ENTRY pLE;
    PCOMMUNITY_LIST_ENTRY pCLE;

    // process entries until list is empty
    while (!IsListEmpty(&g_ValidCommunities)) {

        // extract next entry from head of list
        pLE = RemoveHeadList(&g_ValidCommunities);

        // retrieve pointer to community structure
        pCLE = CONTAINING_RECORD(pLE, COMMUNITY_LIST_ENTRY, Link);
 
        // release
        FreeCLE(pCLE);
    }

    return TRUE;
}
