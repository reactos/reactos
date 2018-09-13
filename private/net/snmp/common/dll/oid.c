/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    oid.c

Abstract:

    Contains routines to manipulatee object identifiers.

        SnmpUtilOidCpy
        SnmpUtilOidAppend
        SnmpUtilOidNCmp
        SnmpUtilOidCmp
        SnmpUtilOidFree

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


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

SNMPAPI
SNMP_FUNC_TYPE 
SnmpUtilOidCpy(
    AsnObjectIdentifier * pOidDst,
    AsnObjectIdentifier * pOidSrc  
    )

/*++

Routine Description:

    Copy an object identifier.

Arguments:

    pOidDst - pointer to structure to receive OID.

    pOidSrc - pointer to OID to copy.

Return Values:

    Returns SNMPAPI_NOERROR if successful. 

--*/

{
    SNMPAPI nResult = SNMPAPI_ERROR;

    // validate pointers
    if (pOidDst != NULL) {

        // initialize
        pOidDst->ids = NULL;
        pOidDst->idLength = 0;    

        // check for subids
        if ((pOidSrc != NULL) &&
            (pOidSrc->ids != NULL) &&
            (pOidSrc->idLength != 0)) {

            // attempt to allocate the subids
            pOidDst->ids = (UINT *)SnmpUtilMemAlloc(
                                    pOidSrc->idLength * sizeof(UINT)
                                    );    

            // validate pointer
            if (pOidDst->ids != NULL) {

                // transfer the oid length
                pOidDst->idLength = pOidSrc->idLength;
                
                // transfer subids
                memcpy(pOidDst->ids, 
                       pOidSrc->ids, 
                       pOidSrc->idLength * sizeof(UINT) 
                       );         

                nResult = SNMPAPI_NOERROR; // success...    

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: API: could not allocate oid.\n"
                    ));

                SetLastError(SNMP_MEM_ALLOC_ERROR);
            }

        } else {
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: API: copying a null oid.\n"
                ));
            
            nResult = SNMPAPI_NOERROR; // success...
        }

    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: API: null oid pointer during copy.\n"
            ));

        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return nResult;
} 


SNMPAPI
SNMP_FUNC_TYPE 
SnmpUtilOidAppend(
    AsnObjectIdentifier * pOidDst,
    AsnObjectIdentifier * pOidSrc 
    )

/*++

Routine Description:

    Append source OID to destination OID

Arguments:

    pOidDst - pointer to structure to receive combined OID.

    pOidSrc - pointer to OID to append.

Return Values:

    Returns SNMPAPI_NOERROR if successful. 

--*/

{
    SNMPAPI nResult = SNMPAPI_ERROR;

    // validate pointers
    if (pOidDst != NULL) {

        // check if there are subids
        if ((pOidSrc != NULL) &&
            (pOidSrc->ids != NULL) &&
            (pOidSrc->idLength != 0)) {

            // calculate the total number of subidentifiers
            UINT nIds = pOidDst->idLength + pOidSrc->idLength;
            
            // validate number of subids    
            if (nIds <= SNMP_MAX_OID_LEN) {

                // attempt to allocate the subidentifiers
                UINT * pIds = (UINT *)SnmpUtilMemReAlloc(
                                            pOidDst->ids, 
                                            nIds * sizeof(UINT)
                                            );

                // validate pointer
                if (pIds != NULL) {

                    // transfer pointer
                    pOidDst->ids = pIds;

                    // transfer subids
                    memcpy(&pOidDst->ids[pOidDst->idLength], 
                           pOidSrc->ids, 
                           pOidSrc->idLength * sizeof(UINT) 
                           );

                    // transfer oid length
                    pOidDst->idLength = nIds;

                    nResult = SNMPAPI_NOERROR; // success...

                } else {

                    SNMPDBG((
                        SNMP_LOG_ERROR,
                        "SNMP: API: could not allocate oid.\n"
                        ));

                    SetLastError(SNMP_MEM_ALLOC_ERROR);
                }

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: API: combined oid too large.\n"
                    ));

                SetLastError(SNMP_BERAPI_OVERFLOW);
            }

        } else {
            
            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: API: appending a null oid.\n"
                ));
            
            nResult = SNMPAPI_NOERROR; // success...
        }

    } else {
        
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: API: null oid pointer during copy.\n"
            ));

        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return nResult;
}


SNMPAPI
SNMP_FUNC_TYPE 
SnmpUtilOidNCmp(
    AsnObjectIdentifier * pOid1, 
    AsnObjectIdentifier * pOid2,
    UINT                  nSubIds               
    )

/*++

Routine Description:

    Compares two OIDs up to a certain subidentifier.

Arguments:

    pOid1 - pointer to first OID.

    pOid2 - pointer to second OID.

    nSubIds - maximum subidentifiers to compare.

Return Values:

    < 0   first parameter is 'less than' second.
      0   first parameter is 'equal to' second.
    > 0   first parameter is 'greater than' second.

--*/

{
    UINT i = 0;
//    INT nResult = 0;

    // validate pointers
    if ((pOid1 != NULL) &&
        (pOid2 != NULL)) {

        // calculate maximum number of subidentifiers to compare
        UINT nMaxIds = min(nSubIds, min(pOid1->idLength, pOid2->idLength));

/*
        // loop through the subidentifiers
        while((nResult == 0) && (i < nMaxIds)) {

            // compare each subidentifier
            nResult = pOid1->ids[i] - pOid2->ids[i++];
        }
*/
		while(i < nMaxIds)
		{
			if (pOid1->ids[i] != pOid2->ids[i])
				break;
			i++;
		}

		// comparision length less then either OID lengths; components equals
		if (i == nSubIds)
			return 0;

		// difference encountered before either OID endings and before the
		// requested comparision length
		if (i < nMaxIds)
			return (pOid1->ids[i] < pOid2->ids[i])? -1 : 1;

		// one OID is shorter than the requested comparision length
		return pOid1->idLength - pOid2->idLength;
/*
        // check for second being subset
        if ((nResult == 0) && (i < nSubIds)) {

            // determine order by length of oid
            nResult = pOid1->idLength - pOid2->idLength;
        }
*/
    }
    
//    return nResult;
	return 0;
} 


SNMPAPI
SNMP_FUNC_TYPE 
SnmpUtilOidCmp(
    AsnObjectIdentifier * pOid1, 
    AsnObjectIdentifier * pOid2
    )

/*++

Routine Description:

    Compares two OIDs.

Arguments:

    pOid1 - pointer to first OID.

    pOid2 - pointer to second OID.

Return Values:

    < 0   first parameter is 'less than' second.
      0   first parameter is 'equal to' second.
    > 0   first parameter is 'greater than' second.

--*/

{
    // forward request to the function above
    return SnmpUtilOidNCmp(pOid1,pOid2,max(pOid1->idLength,pOid2->idLength));
}


VOID
SNMP_FUNC_TYPE 
SnmpUtilOidFree(
    AsnObjectIdentifier * pOid
    )

/*++

Routine Description:

    Releases memory associated with OID.

Arguments:

    pOid - pointer to OID to free.

Return Values:

    None.

--*/

{
    // validate 
    if (pOid != NULL) {

        // release subids memory
        SnmpUtilMemFree(pOid->ids);

        // re-initialize
        pOid->idLength = 0;
        pOid->ids      = NULL;
    }
} 
