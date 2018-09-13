/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpmgmt.h

Abstract:

    Contains functions for handling/updating
	snmp management variables (defined in RFC1213)

Environment:

    User Mode - Win32

Revision History:

    30-Mar-1998 FlorinT
--*/
#include <snmputil.h>
#include "snmpmgmt.h"

SNMP_MGMTVARS snmpMgmtBase;	// instance of the service management variables

/*++
	Initializes the management variable arrays.
--*/
void mgmtInit()
{
	int i;

	for (i=0; i<NC_MAX_COUNT; i++)
    {
        snmpMgmtBase.AsnCounterPool[i].asnType = ASN_COUNTER32;
		snmpMgmtBase.AsnCounterPool[i].asnValue.counter = 0;
    }
	for (i=0; i<NI_MAX_COUNT; i++)
    {
        snmpMgmtBase.AsnIntegerPool[i].asnType = ASN_INTEGER;
		snmpMgmtBase.AsnIntegerPool[i].asnValue.number = 0;
    }
    for (i=0; i<NO_MAX_COUNT; i++)
    {
        snmpMgmtBase.AsnObjectIDs[i].asnType = ASN_OBJECTIDENTIFIER;
        snmpMgmtBase.AsnObjectIDs[i].asnValue.object.idLength = 0;
        snmpMgmtBase.AsnObjectIDs[i].asnValue.object.ids = NULL;
    }

    // particular case: default the IsnmpEnableAuthenTraps to TRUE
    snmpMgmtBase.AsnIntegerPool[IsnmpEnableAuthenTraps].asnValue.number = 1;

    // particular case: default the IsnmpNameResolutionRetries to 0
    snmpMgmtBase.AsnIntegerPool[IsnmpNameResolutionRetries].asnValue.number = 0;

    // particular case: default the OsnmpSysObjectID to the hard coded value given by SvcGetEnterpriseOID
    mgmtOSet(OsnmpSysObjectID, SnmpSvcGetEnterpriseOID(), TRUE);
}

/*++
	Releases any memory that has been allocated for the management variables
--*/
void mgmtCleanup()
{
    int i;

    for (i=0; i<NO_MAX_COUNT; i++)
    {
        SnmpUtilOidFree(&(snmpMgmtBase.AsnObjectIDs[i].asnValue.object));
    }
}

/*++
	Increment the specified Counter variable
Returns:
	ERROR_SUCCESS on success;
	ERROR_INVALID_INDEX if index out of range;
	ERROR_ARITHMETIC_OVERFLOW if overflowing the MAXINT value.
--*/
int mgmtCTick(int index)
{
	AsnCounter	oldValue;

	if (index < 0 || index >= NC_MAX_COUNT)
		return ERROR_INVALID_INDEX	;

	oldValue = snmpMgmtBase.AsnCounterPool[index].asnValue.counter;
	snmpMgmtBase.AsnCounterPool[index].asnValue.counter++;
	return snmpMgmtBase.AsnCounterPool[index].asnValue.counter > oldValue ? ERROR_SUCCESS : ERROR_ARITHMETIC_OVERFLOW;
}

/*++
	Add a value to a counter
Returns:
	ERROR_SUCCESS on success;
	ERROR_INVALID_INDEX if index out of range;
	ERROR_ARITHMETIC_OVERFLOW if overflowing the MAXINT value.
--*/
int  mgmtCAdd(int index, AsnCounter value)
{
    AsnCounter  oldValue;

    if (index < 0 || index >= NC_MAX_COUNT)
        return ERROR_INVALID_INDEX;

    oldValue = snmpMgmtBase.AsnCounterPool[index].asnValue.counter;
    snmpMgmtBase.AsnCounterPool[index].asnValue.counter += value;
    return snmpMgmtBase.AsnCounterPool[index].asnValue.counter > oldValue ? ERROR_SUCCESS : ERROR_ARITHMETIC_OVERFLOW;
}

/*++
	Set the value of a certain AsnInteger mgmt variable
Returns:
	ERROR_SUCCESS on success;
	ERROR_INVALID_INDEX if index out of range;
--*/
int mgmtISet(int index, AsnInteger value)
{
	if (index < 0 || index > NI_MAX_COUNT)
		return ERROR_INVALID_INDEX;
	snmpMgmtBase.AsnIntegerPool[index].asnValue.number = value;
	return ERROR_SUCCESS;
}

/*++
    Set the value of a certain AsnObjectIdentifier mgmt variable
Returns:
    ERROR_SUCCESS on success;
    ERROR_INVALID_INDEX if index out of range;
    other WinErr if smth else went wrong
Remarks:
    If bAlloc = TRUE, the variable is moved (no mem is allocated) to the management variable
    If bAlloc = FALSE the value of the input variable is copied (and mem is allocated) to the mgmt variable
---*/
int mgmtOSet(int index, AsnObjectIdentifier *pValue, BOOL bAlloc)
{
    AsnObjectIdentifier oldObject;

    if (index < 0 || index > NO_MAX_COUNT)
        return ERROR_INVALID_INDEX;
    if (pValue == NULL)
        return ERROR_INVALID_PARAMETER;

    // make a backup of the original object. If something goes wrong, the original object will not be free-ed.
    oldObject.idLength = snmpMgmtBase.AsnObjectIDs[index].asnValue.object.idLength;
    oldObject.ids = snmpMgmtBase.AsnObjectIDs[index].asnValue.object.ids;

    if (bAlloc)
    {
        // the object is to be copied and mem is to be allocated
        if (SnmpUtilOidCpy(&(snmpMgmtBase.AsnObjectIDs[index].asnValue.object), pValue) != SNMPAPI_NOERROR)
            return GetLastError();
    }
    else
    {
        // the object is to be moved, no mem will be allocated
        snmpMgmtBase.AsnObjectIDs[index].asnValue.object.idLength = pValue->idLength;
        snmpMgmtBase.AsnObjectIDs[index].asnValue.object.ids = pValue->ids;
    }

    // everything went fine, so release the memory for the previous value
    SnmpUtilOidFree(&oldObject);

    return ERROR_SUCCESS;
}

/*++
    Updates the MIB counters for the IN_errStatus or OUT_errStatus value
Returns:
    void
--*/
void mgmtUtilUpdateErrStatus(UINT flag, DWORD errStatus)
{
    UINT index;

    switch(errStatus)
    {
    case SNMP_ERRORSTATUS_TOOBIG:
        index = flag == IN_errStatus ? CsnmpInTooBigs : CsnmpOutTooBigs;
        break;

    case SNMP_ERRORSTATUS_NOSUCHNAME:
        index = flag == IN_errStatus ? CsnmpInNoSuchNames : CsnmpOutNoSuchNames;
        break;

    case SNMP_ERRORSTATUS_BADVALUE:
        index = flag == IN_errStatus ? CsnmpInBadValues : CsnmpOutBadValues;
        break;

    case SNMP_ERRORSTATUS_READONLY:
        if (flag != IN_errStatus)
            return;
        index = CsnmpInReadOnlys;
        break;

    case SNMP_ERRORSTATUS_GENERR:
        index = flag == IN_errStatus ? CsnmpInGenErrs : CsnmpOutGenErrs;
        break;

    default:
        return;
    }

    mgmtCTick(index);
}
