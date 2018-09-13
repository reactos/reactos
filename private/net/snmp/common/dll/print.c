/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    print.c

Abstract:

    Contains printing support.

        SnmpUtilPrintOid
        SnmpUtilPrintAsnAny

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <windef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <snmp.h>
#include <snmputil.h>
#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID 
SNMP_FUNC_TYPE 
SnmpUtilPrintOid(
    AsnObjectIdentifier * pOid 
    )

/*++

Routine Description:

    Outputs object identifier to the console.

Arguments:

    pOid - pointer to OID to display.

Return Values:

    None. 

--*/

{
    UINT i;

    // validate oid
    if ((pOid != NULL) &&
        (pOid->ids != NULL) &&
        (pOid->idLength != 0)) {

        // output first subidentifier
        fprintf(stdout, "%lu", pOid->ids[0]);

        // loop through subidentifiers
        for (i = 1; i < pOid->idLength; i++) {

            // output next subidentifier
            fprintf(stdout, ".%lu", pOid->ids[i]);
        }
    }
} 


VOID
SNMP_FUNC_TYPE 
SnmpUtilPrintAsnAny(
    AsnAny * pAsnAny
    )

/*++

Routine Description:

    Outputs variable value to the console.

Arguments:

    pAsnAny - pointer to value structure from variable binding.

Return Values:

    None. 

--*/


{
    // validate
    if (pAsnAny != NULL) {

        // determine type    
        switch (pAsnAny->asnType) {

        case ASN_INTEGER32:
        fprintf(stdout, "Integer32 %ld\n", pAsnAny->asnValue.number);
        break;

        case ASN_UNSIGNED32:
        fprintf(stdout, "Unsigned32 %lu\n", pAsnAny->asnValue.unsigned32);
        break;

        case ASN_COUNTER32:
        fprintf(stdout, "Counter32 %lu\n", pAsnAny->asnValue.counter);
        break;

        case ASN_GAUGE32:
        fprintf(stdout, "Gauge32 %lu\n", pAsnAny->asnValue.gauge);
        break;

        case ASN_TIMETICKS:
        fprintf(stdout, "TimeTicks %lu\n", pAsnAny->asnValue.ticks);
        break;

        case ASN_COUNTER64:
        fprintf(stdout, "Counter64 %l64u\n", pAsnAny->asnValue.counter64.QuadPart);
        break;

        case ASN_OBJECTIDENTIFIER:
        {
            UINT i;

            fprintf(stdout, "ObjectID ");

            // simply forward to helper function
            SnmpUtilPrintOid(&pAsnAny->asnValue.object);

            putchar('\n');
        }
        break;

        case ASN_OCTETSTRING:
        {
            UINT i;
            BOOL bDisplayString = TRUE;
            LPSTR StringFormat;

            // loop through string looking for non-printable characters
            for (i = 0; i < pAsnAny->asnValue.string.length && bDisplayString; i++ ) {
                bDisplayString = isprint(pAsnAny->asnValue.string.stream[i]);
            }
    
            // determine string format based on results
            StringFormat = bDisplayString ? "%c" : "<0x%02x>" ;

            fprintf(stdout, "String ");

            for (i = 0; i < pAsnAny->asnValue.string.length; i++) {
                fprintf(stdout, StringFormat, pAsnAny->asnValue.string.stream[i]);
            }

            putchar('\n');
        }
        break;

        case ASN_IPADDRESS:
        {
            UINT i;

            fprintf(stdout, "IpAddress " );
            fprintf(stdout, "%d.%d.%d.%d ",
                pAsnAny->asnValue.string.stream[0] ,
                pAsnAny->asnValue.string.stream[1] ,
                pAsnAny->asnValue.string.stream[2] ,
                pAsnAny->asnValue.string.stream[3] 
                );
            putchar('\n');
        }
        break;

        case ASN_OPAQUE:
        {
            UINT i;

            fprintf(stdout, "Opaque ");

            for (i = 0; i < pAsnAny->asnValue.string.length; i++) {
                fprintf(stdout, "0x%x ", pAsnAny->asnValue.string.stream[i]);
            }

            putchar('\n');
        }
        break;

        case ASN_BITS:
        {
            UINT i;

            fprintf(stdout, "Bits ");

            for (i = 0; i < pAsnAny->asnValue.string.length; i++) {
                fprintf(stdout, "0x%x ", pAsnAny->asnValue.string.stream[i]);
            }

            putchar('\n');
        }
        break;

        case ASN_NULL:
            fprintf(stdout, "Null value\n");
            break;

        case SNMP_EXCEPTION_NOSUCHOBJECT:
            fprintf(stdout, "NOSUCHOBJECT\n");
            break;

        case SNMP_EXCEPTION_NOSUCHINSTANCE:
            fprintf(stdout, "NOSUCHINSTANCE\n");
            break;

        case SNMP_EXCEPTION_ENDOFMIBVIEW:
            fprintf(stdout, "ENDOFMIBVIEW\n");
            break;

        default:
            fprintf(stdout, "Invalid type %d\n", pAsnAny->asnType);
            break;
        }
    }
}
