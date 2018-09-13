/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    vbl.c

Abstract:

    Contains routines to manipulate variable binding lists.

        SnmpUtilVarBindListCpy
        SnmpUtilVarBindListFree

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
SnmpUtilVarBindListCpy(
    SnmpVarBindList * pVblDst, 
    SnmpVarBindList * pVblSrc  
    )

/*++

Routine Description:

    Copies a variable binding list.

Arguments:

    pVblDst - pointer to structure to receive VarBindList.

    pVblSrc - pointer to VarBindList to copy.

Return Values:

    Returns SNMPAPI_NOERROR if successful. 

--*/

{
    UINT i;
    SNMPAPI nResult = SNMPAPI_ERROR;

    // validate pointer 
    if (pVblDst != NULL) {

        // initialize
        pVblDst->list = NULL;
        pVblDst->len  = 0;

        // check for varbinds    
        if ((pVblSrc != NULL) &&
            (pVblSrc->list != NULL) &&
            (pVblSrc->len != 0)) {

            // attempt to allocate varbinds
            pVblDst->list = SnmpUtilMemAlloc(
                                pVblSrc->len * sizeof(SnmpVarBind)
                                );

            // validate pointer
            if (pVblDst->list != NULL) {

                // loop through the varbinds
                for (i = 0; i < pVblSrc->len; i++) {

                    // copy individual varbind
                    nResult = SnmpUtilVarBindCpy(   
                                    &pVblDst->list[i], 
                                    &pVblSrc->list[i]
                                    );

                    // validate return code
                    if (nResult == SNMPAPI_NOERROR) {            

                        // increment
                        pVblDst->len++; // success...
                        
                    } else {
            
                        break; // failure...    
                    }
                }
            
            } else {
            
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: API: could not allocate varbinds.\n"
                    ));

                SetLastError(SNMP_MEM_ALLOC_ERROR);
            }

        } else {

            SNMPDBG((
                SNMP_LOG_WARNING,
                "SNMP: API: copying null varbindlist.\n"
                ));
        
            nResult = SNMPAPI_NOERROR; // success..,
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: API: null varbindlist pointer.\n"
            ));
    
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    // make sure we cleanup    
    if (nResult == SNMPAPI_ERROR) {

        // release new varbinds
        SnmpUtilVarBindListFree(pVblDst);
    }

    return nResult;
}


VOID
SNMP_FUNC_TYPE 
SnmpUtilVarBindListFree(
    SnmpVarBindList * pVbl
    )

/*++

Routine Description:

    Frees memory associated with variable binding list.

Arguments:

    pVbl - pointer to VarBindList to free.

Return Values:

    None. 

--*/

{
    UINT i;

    // loop throught varbinds
    for (i = 0; i < pVbl->len; i++) {

        // release individual varbind
        SnmpUtilVarBindFree(&pVbl->list[i]);
    }

    // release actual list
    SnmpUtilMemFree(pVbl->list);

    // re-initialize
    pVbl->list = NULL;
    pVbl->len  = 0;
} 
