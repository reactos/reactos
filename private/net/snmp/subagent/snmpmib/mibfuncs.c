/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

Abstract:

Revision history:

--*/

#include <snmp.h>
#include <snmpexts.h>
#include "mibfuncs.h"

extern PSNMP_MGMTVARS   ge_pMgmtVars;

UINT
snmpMibGetHandler(
        UINT     actionId,
        AsnAny  *objectArray,
        UINT    *errorIndex)
{
    int i, j, k;

    if (ge_pMgmtVars == NULL)
        return MIB_S_ENTRY_NOT_FOUND;

    // get the number of AsnAny structures we have in the MIB's data buffer
    // and be careful not too scan further (it might be that there are more
    // management variables than objects supported by the MIB).
    k = sizeof(SNMPMIB_MGMTVARS) / sizeof(AsnAny);

    for (i = 0; k != 0 && i < NC_MAX_COUNT; i++, k--)
    {
        if (objectArray[i].asnType == ge_pMgmtVars->AsnCounterPool[i].asnType)
        {
            objectArray[i].asnValue = ge_pMgmtVars->AsnCounterPool[i].asnValue;
        }
    }

    for (j = 0; k != 0 && j < NI_MAX_COUNT; j++, k--)
    {
        if (objectArray[i + j].asnType == ge_pMgmtVars->AsnIntegerPool[j].asnType)
        {
            objectArray[i + j].asnValue = ge_pMgmtVars->AsnIntegerPool[j].asnValue;
        }
    }

    return MIB_S_SUCCESS;
}


UINT
snmpMibSetHandler(
        UINT     actionId,
        AsnAny  *objectArray,
        UINT    *errorIndex)
{
    // this function is called only for one object: snmpEnableAuthenTraps
    ge_pMgmtVars->AsnIntegerPool[IsnmpEnableAuthenTraps].asnValue = objectArray[NC_MAX_COUNT + IsnmpEnableAuthenTraps].asnValue;

    return MIB_S_SUCCESS;
}
