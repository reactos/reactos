/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    string.c

Abstract:

    Contains string conversion routines.

        SnmpUtilIdsToA
        SnmpUtilOidToA

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <snmp.h>
#include <snmputil.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define MAX_STRING_LEN  512 
#define MAX_SUBIDS_LEN  16  


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LPSTR
SNMP_FUNC_TYPE
SnmpUtilIdsToA(
    UINT * pIds, 
    UINT   nIds
    )

/*++

Routine Description:

    Converts OID subidentifiers into string.

Arguments:

    pIds - pointer to subidentifiers.

    nIds - number of subidentifiers.

Return Values:

    Returns pointer to string representation. 

--*/

{
    UINT i;
    UINT j;

    static char szBuf[MAX_STRING_LEN+MAX_SUBIDS_LEN];
	static char szId[MAX_SUBIDS_LEN];

    if ((pIds != NULL) && (nIds != 0)) {
                                     
        j = sprintf(szBuf, "%d", pIds[0]);

        for (i = 1; (i < nIds) && (j < MAX_STRING_LEN); i++) {
			j += sprintf(szId, ".%d", pIds[i]);
			if (j >= (MAX_STRING_LEN + MAX_SUBIDS_LEN)-3)
			{
				strcat(szBuf, "...");
				break;
			}
            else
				strcat(szBuf, szId);
        }

    } else {
                        
        sprintf(szBuf, "<null oid>");
    }

    return szBuf;
} 


LPSTR
SNMP_FUNC_TYPE
SnmpUtilOidToA(
    AsnObjectIdentifier * pOid
    )

/*++

Routine Description:

    Converts OID into string.

Arguments:

    pOid - pointer to object identifier.

Return Values:

    Returns pointer to string representation. 

--*/

{
    UINT * pIds = NULL;
    UINT   nIds = 0;

    if (pOid != NULL) {

        pIds = pOid->ids;
        nIds = pOid->idLength;
    }

    return SnmpUtilIdsToA(pIds, nIds); 
} 

