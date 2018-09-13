/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    vb.c

Abstract:

    Contains routines to manipulate variable bindings.

        SnmpUtilVarBindCpy
        SnmpUtilVarBindFree

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
SnmpUtilVarBindCpy(
    SnmpVarBind * pVbDst,
    SnmpVarBind * pVbSrc
    )

/*++

Routine Description:

    Copies a variable binding.

Arguments:

    pVbDst - pointer to structure to receive VarBind.

    pVbSrc - pointer to VarBind to copy.

Return Values:

    Returns SNMPAPI_NOERROR if successful. 

--*/

{
    SNMPAPI nResult = SNMPAPI_ERROR;

    // validate pointer
    if (pVbDst != NULL) {
        
        // initialize destination
        pVbDst->value.asnType = ASN_NULL;

        // validate pointer
        if (pVbSrc != NULL) {

            // copy the variable's name from source to destination
            nResult = SnmpUtilOidCpy(&pVbDst->name, &pVbSrc->name);

            // validate return code
            if (nResult == SNMPAPI_NOERROR) {
        
                // copy the variable's value from source to destination            
                nResult = SnmpUtilAsnAnyCpy(&pVbDst->value, &pVbSrc->value);
            }
        
        } else {

            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: API: copying null varbind.\n"
                ));
        
            nResult = SNMPAPI_NOERROR; // success..,
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: API: null varbind pointer.\n"
            ));
    
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    // validate return code
    if (nResult == SNMPAPI_ERROR) {
        
        // release new variable 
        SnmpUtilVarBindFree(pVbDst);
    }

    return nResult;
} 


VOID
SNMP_FUNC_TYPE 
SnmpUtilVarBindFree(
    SnmpVarBind * pVb 
    )

/*++

Routine Description:

    Releases memory associated with variable binding.

Arguments:

    pVb - pointer to VarBind to release.

Return Values:

    None. 

--*/

{
    // validate
    if (pVb != NULL) {
        
        // release variable name    
        SnmpUtilOidFree(&pVb->name);

        // release variable value
        SnmpUtilAsnAnyFree(&pVb->value);
    }
} 
