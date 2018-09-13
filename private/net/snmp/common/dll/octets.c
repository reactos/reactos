/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    octets.c

Abstract:

    Contains routines to manipulate octet strings.

        SnmpUtilOctetsCpy
        SnmpUtilOctetsFree

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
SnmpUtilOctetsCpy(
    AsnOctetString * pOctetsDst,
    AsnOctetString * pOctetsSrc
    )

/*++

Routine Description:

    Copy an octet string.

Arguments:

    pOctetsDst - pointer to structure to receive octets.

    pOctetsSrc - pointer to octets to copy.

Return Values:

    Returns SNMPAPI_NOERROR if successful. 

--*/

{
    SNMPAPI nResult = SNMPAPI_ERROR;

    // validate pointers
    if (pOctetsDst != NULL) {
        
        // initialize
        pOctetsDst->stream  = NULL;
        pOctetsDst->length  = 0;
        pOctetsDst->dynamic = FALSE;

        // make sure there are bytes
        if ((pOctetsSrc != NULL) &&
            (pOctetsSrc->stream != NULL) &&
            (pOctetsSrc->length != 0)) {

            // attempt to allocate octet string
            pOctetsDst->stream = SnmpUtilMemAlloc(pOctetsSrc->length);
            
            // validate pointer
            if (pOctetsDst->stream != NULL) {

                // denote allocated type
                pOctetsDst->dynamic = TRUE;

                // transfer octet string length
                pOctetsDst->length = pOctetsSrc->length;

                // copy
                memcpy(pOctetsDst->stream,
                       pOctetsSrc->stream,
                       pOctetsSrc->length
                       );

                nResult = SNMPAPI_NOERROR; // success...

            } else {
            
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: API: could not allocate octet string.\n"
                    ));

                SetLastError(SNMP_MEM_ALLOC_ERROR);
            }

        } else {

            // copying null string
            nResult = SNMPAPI_NOERROR;
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: API: null octet string pointer.\n"
            ));

        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return nResult;
}


VOID
SNMP_FUNC_TYPE
SnmpUtilOctetsFree(
    AsnOctetString * pOctets
    )

/*++

Routine Description:

    Releases memory for an octet string.

Arguments:

    pOctets - pointer to octets to release.

Return Values:

    None. 

--*/

{
    // validate pointers
    if ((pOctets != NULL) &&
        (pOctets->stream != NULL) && 
        (pOctets->dynamic == TRUE)) {

        // release memory for octets
        SnmpUtilMemFree(pOctets->stream);

        // re-initialize
        pOctets->dynamic = FALSE;
        pOctets->stream  = NULL;
        pOctets->length  = 0;
    }
}


SNMPAPI
SNMP_FUNC_TYPE 
SnmpUtilOctetsNCmp(
    AsnOctetString * pOctets1, 
    AsnOctetString * pOctets2,
    UINT             nChars
    )

/*++

Routine Description:

    Compares two octet strings up to a certain number of characters.

Arguments:

    pOctets1 - pointer to first octet string.

    pOctets2 - pointer to second octet string.

    nChars - maximum characters to compare.

Return Values:

    < 0   first parameter is 'less than' second.
      0   first parameter is 'equal to' second.
    > 0   first parameter is 'greater than' second.

--*/

{
    UINT i = 0;
    INT nResult = 0;

    // validate pointers
    if ((pOctets1 != NULL) &&
        (pOctets2 != NULL)) {

        // calculate maximum number of subidentifiers to compare
        UINT nMaxChars = min(nChars, min(pOctets1->length, pOctets2->length));

        // loop through the subidentifiers
        while((nResult == 0) && (i < nMaxChars)) {

            // compare each subidentifier
            nResult = pOctets1->stream[i] - pOctets2->stream[i++];
        }

        // check for second being subset
        if ((nResult == 0) && (i < nChars)) {

            // determine order by length of oid
            nResult = pOctets1->length - pOctets2->length;
        }
    }
    
    return nResult;
} 


SNMPAPI
SNMP_FUNC_TYPE 
SnmpUtilOctetsCmp(
    AsnOctetString * pOctets1, 
    AsnOctetString * pOctets2
    )

/*++

Routine Description:

    Compares two octet strings.

Arguments:

    pOctets1 - pointer to first octet string.

    pOctets2 - pointer to second octet string.

Return Values:

    < 0   first parameter is 'less than' second.
      0   first parameter is 'equal to' second.
    > 0   first parameter is 'greater than' second.

--*/

{
    // forward request above
    return SnmpUtilOctetsNCmp(
                pOctets1,
                pOctets2,
                max(pOctets1->length,pOctets2->length)
                );
}
