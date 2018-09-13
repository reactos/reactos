/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpmgmt.h

Abstract:

    Contains the definitions of service management variables (as defined in RFC1213)

Environment:

    User Mode - Win32

Revision History:

    30-Mar-1998 FlorinT
--*/

#ifndef _SNMPMIB_H
#define _SNMPMIB_H

#include <snmp.h>

// defines the number of AsnCounters in the SNMP_MGMTVARS.AsnCounterPool[]
#define NC_MAX_COUNT				27

// indices in the SNMP_MGMTVARS.AsnCounterPool[]
#define CsnmpInPkts                 0
#define CsnmpOutPkts                1

#define CsnmpInBadVersions          2
#define CsnmpInBadCommunityNames    3
#define CsnmpInBadCommunityUses     4
#define CsnmpInASNParseErrs         5

#define CsnmpInTooBigs              6
#define CsnmpInNoSuchNames          7
#define CsnmpInBadValues            8
#define CsnmpInReadOnlys            9
#define CsnmpInGenErrs              10

#define CsnmpInTotalReqVars         11
#define CsnmpInTotalSetVars         12
#define CsnmpInGetRequests          13
#define CsnmpInGetNexts             14
#define CsnmpInSetRequests          15
#define CsnmpInGetResponses         16
#define CsnmpInTraps                17

#define CsnmpOutTooBigs             18
#define CsnmpOutNoSuchNames         19
#define CsnmpOutBadValues           20
#define CsnmpOutGenErrs             21

#define CsnmpOutGetRequests         22
#define CsnmpOutGetNexts            23
#define CsnmpOutSetRequests         24
#define CsnmpOutGetResponses        25
#define CsnmpOutTraps               26

// defines the number of AsnIntegers in the SNMP_MGMTVARS.AsnIntegerPool[]
#define NI_MAX_COUNT				2

// indices in the SNMP_MGMTVARS.AsnIntegerPool[]
#define IsnmpEnableAuthenTraps		0
#define IsnmpNameResolutionRetries	1

// defines the number of AsnObjects in the SNMP_MGMTVARS.AsnObjectIDs[]
#define NO_MAX_COUNT                1

 // indices in the SNMP_MGMTVARS.AsnObjectIDs[]
#define OsnmpSysObjectID            0

// flag for mgmtUtilUpdate* functions
#define IN_errStatus                0
#define OUT_errStatus               1

typedef struct _snmp_mgmtvars
{
  AsnAny    	AsnCounterPool[NC_MAX_COUNT];	// storage place for management counters.
  AsnAny    	AsnIntegerPool[NI_MAX_COUNT];	// storage place for management integers.
  AsnAny        AsnObjectIDs[NO_MAX_COUNT];     // storage place for management obj IDs.
} SNMP_MGMTVARS, *PSNMP_MGMTVARS;

extern SNMP_MGMTVARS snmpMgmtBase;

void mgmtInit();
void mgmtCleanup();
int  mgmtCTick(int index);
int  mgmtCAdd(int index, AsnCounter value);
int  mgmtISet(int index, AsnInteger value);
int  mgmtOSet(int index, AsnObjectIdentifier *pValue, BOOL bAlloc);

// utility functions
void mgmtUtilUpdateErrStatus(UINT flag, DWORD errStatus);

#endif
