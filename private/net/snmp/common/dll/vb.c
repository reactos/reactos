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
--*/


// Include files                                                             //
#include <snmp.h>
#include <snmputil.h>


// Public Procedures                                                         //


SNMPAPI SNMP_FUNC_TYPE SnmpUtilVarBindCpy(SnmpVarBind * pVbDst, SnmpVarBind * pVbSrc)
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
        pVbDst->value.asnType = ASN_NULL;// initialize destination
        if (pVbSrc != NULL) {// validate pointer
            nResult = SnmpUtilOidCpy(&pVbDst->name, &pVbSrc->name);// copy the variable's name from source to destination
            if (nResult == SNMPAPI_NOERROR) {// validate return code
                // copy the variable's value from source to destination
                nResult = SnmpUtilAsnAnyCpy(&pVbDst->value, &pVbSrc->value);
            }
        } else {
            SNMPDBG((SNMP_LOG_WARNING, "SNMP: API: copying null varbind.\n"));
            nResult = SNMPAPI_NOERROR; // success..,
        }
    } else {
        SNMPDBG((SNMP_LOG_ERROR, "SNMP: API: null varbind pointer.\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if (nResult == SNMPAPI_ERROR) {// validate return code
        SnmpUtilVarBindFree(pVbDst);// release new variable
    }

    return nResult;
}


VOID SNMP_FUNC_TYPE SnmpUtilVarBindFree(SnmpVarBind * pVb )
/*++
Routine Description:
    Releases memory associated with variable binding.
Arguments:
    pVb - pointer to VarBind to release.
--*/
{
    // validate
    if (pVb != NULL) {
        SnmpUtilOidFree(&pVb->name);// release variable name
        SnmpUtilAsnAnyFree(&pVb->value);// release variable value
    }
}