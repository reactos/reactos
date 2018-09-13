/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    any.c

Abstract:

    Contains routines to manipulate AsnAny structures.

        SnmpUtilAsnAnyCpy
        SnmpUtilAsnAnyFree

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
SnmpUtilAsnAnyCpy(
    AsnAny * pAnyDst,
    AsnAny * pAnySrc
    )

/*++

Routine Description:

    Copy a variable value.

Arguments:

    pAnyDst - pointer to structure to receive value.

    pAnySrc - pointer to value to copy.

Return Values:

    Returns SNMPAPI_NOERROR if successful. 

--*/

{
    SNMPAPI nResult = SNMPAPI_NOERROR;

    // determine asn type
    switch (pAnySrc->asnType) {

    case ASN_OBJECTIDENTIFIER:

        // copy object identifier
        nResult = SnmpUtilOidCpy(
                    &pAnyDst->asnValue.object, 
                    &pAnySrc->asnValue.object
                    );
        break;

    case ASN_OPAQUE:
    case ASN_IPADDRESS:
    case ASN_OCTETSTRING:
    case ASN_BITS:

        // copy octet string
        nResult = SnmpUtilOctetsCpy(
                    &pAnyDst->asnValue.string,
                    &pAnySrc->asnValue.string
                    );
        break;
        
    default:

        // simply transfer entire structure
        pAnyDst->asnValue = pAnySrc->asnValue;
        break;
    }

    // transfer type to destination
    pAnyDst->asnType = pAnySrc->asnType;

    return nResult;
}


VOID
SNMP_FUNC_TYPE
SnmpUtilAsnAnyFree(
    AsnAny * pAny
    )

/*++

Routine Description:

    Release memory associated with variable value.

Arguments:

    pAny - pointer to variable value to free.

Return Values:

    None. 

--*/

{
    // determine asn type
    switch (pAny->asnType) {

    case ASN_OBJECTIDENTIFIER:

        // free object identifier
        SnmpUtilOidFree(&pAny->asnValue.object);
        break;

    case ASN_OPAQUE:
    case ASN_IPADDRESS:
    case ASN_OCTETSTRING:
    case ASN_BITS:

        // free octet string
        if (pAny->asnValue.string.dynamic)
        {
            SnmpUtilOctetsFree(&pAny->asnValue.string);
            pAny->asnValue.string.dynamic = FALSE;
            pAny->asnValue.string.stream = NULL;
        }
        break;
        
    default:

        break;
    }

    // re-initialize
    pAny->asnType = ASN_NULL;
}

