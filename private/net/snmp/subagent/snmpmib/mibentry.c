/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

Abstract:

Revision history:

--*/
#include <snmp.h>
#include <snmpexts.h>
#include "mibentry.h"
#include "mibfuncs.h"

//-----------------------------------
// OID definitions
//-----------------------------------
static UINT ids_snmp[] = {1,3,6,1,2,1,11};

static UINT ids_snmpInPkts[]                = { 1,0};
static UINT ids_snmpOutPkts[]               = { 2,0};
static UINT ids_snmpInBadVersions[]         = { 3,0};
static UINT ids_snmpInBadCommunityNames[]   = { 4,0};
static UINT ids_snmpInBadCommunityUses[]    = { 5,0};
static UINT ids_snmpInASNParseErrs[]        = { 6,0};
static UINT ids_snmpInTooBigs[]             = { 8,0};
static UINT ids_snmpInNoSuchNames[]         = { 9,0};
static UINT ids_snmpInBadValues[]           = {10,0};
static UINT ids_snmpInReadOnlys[]           = {11,0};
static UINT ids_snmpInGenErrs[]             = {12,0};
static UINT ids_snmpInTotalReqVars[]        = {13,0};
static UINT ids_snmpInTotalSetVars[]        = {14,0};
static UINT ids_snmpInGetRequests[]         = {15,0};
static UINT ids_snmpInGetNexts[]            = {16,0};
static UINT ids_snmpInSetRequests[]         = {17,0};
static UINT ids_snmpInGetResponses[]        = {18,0};
static UINT ids_snmpInTraps[]               = {19,0};
static UINT ids_snmpOutTooBigs[]            = {20,0};
static UINT ids_snmpOutNoSuchNames[]        = {21,0};
static UINT ids_snmpOutBadValues[]          = {22,0};
static UINT ids_snmpOutGenErrs[]            = {24,0};
static UINT ids_snmpOutGetRequests[]        = {25,0};
static UINT ids_snmpOutGetNexts[]           = {26,0};
static UINT ids_snmpOutSetRequests[]        = {27,0};
static UINT ids_snmpOutGetResponses[]       = {28,0};
static UINT ids_snmpOutTraps[]              = {29,0};
static UINT ids_snmpEnableAuthenTraps[]     = {30,0};

//-----------------------------------
// Views description
//-----------------------------------
SnmpMibEntry mib_snmp[] = {
    MIB_COUNTER(snmpInPkts),
    MIB_COUNTER(snmpOutPkts),
    MIB_COUNTER(snmpInBadVersions),
    MIB_COUNTER(snmpInBadCommunityNames),
    MIB_COUNTER(snmpInBadCommunityUses),
    MIB_COUNTER(snmpInASNParseErrs),
    MIB_COUNTER(snmpInTooBigs),
    MIB_COUNTER(snmpInNoSuchNames),
    MIB_COUNTER(snmpInBadValues),
    MIB_COUNTER(snmpInReadOnlys),
    MIB_COUNTER(snmpInGenErrs),
    MIB_COUNTER(snmpInTotalReqVars),
    MIB_COUNTER(snmpInTotalSetVars),
    MIB_COUNTER(snmpInGetRequests),
    MIB_COUNTER(snmpInGetNexts),
    MIB_COUNTER(snmpInSetRequests),
    MIB_COUNTER(snmpInGetResponses),
    MIB_COUNTER(snmpInTraps),
    MIB_COUNTER(snmpOutTooBigs),
    MIB_COUNTER(snmpOutNoSuchNames),
    MIB_COUNTER(snmpOutBadValues),
    MIB_COUNTER(snmpOutGenErrs),
    MIB_COUNTER(snmpOutGetRequests),
    MIB_COUNTER(snmpOutGetNexts),
    MIB_COUNTER(snmpOutSetRequests),
    MIB_COUNTER(snmpOutGetResponses),
    MIB_COUNTER(snmpOutTraps),
    MIB_INTEGER_RW(snmpEnableAuthenTraps),
    MIB_END()
};

//------------------------------------
// Views supported by this MIB
//------------------------------------
SnmpMibView view_snmp = { 
    MIB_VERSION,
    MIB_VIEW_NORMAL,
    MIB_OID(ids_snmp),
    MIB_ENTRIES(mib_snmp),
    {NULL,0}
};
