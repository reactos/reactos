/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

Abstract:

Revision history:

--*/

#include "snmpmgmt.h"

#ifndef _MIBFUNCS_H_
#define _MIBFUNCS_H_

// SNMPMIB_MGMTVARS is a structure mapped onto the management
// variables defined at the master agent layer. It is another
// view on the memory space covered by the (PSNMP_MGMTVARS)pMibVariables
// defined below.
// !!!When modifying this structure, make sure to check the code in
// snmpMibGetHandler!!! - this structure is scanned based on the assumption it
// contains AsnAny objects only!!!
typedef struct
{
    AsnAny  snmpInPkts;
    AsnAny  snmpOutPkts;
    AsnAny  snmpInBadVersions;
    AsnAny  snmpInBadCommunityNames;
    AsnAny  snmpInBadCommunityUses;
    AsnAny  snmpInASNParseErrs;
    AsnAny  snmpInTooBigs;
    AsnAny  snmpInNoSuchNames;
    AsnAny  snmpInBadValues;
    AsnAny  snmpInReadOnlys;
    AsnAny  snmpInGenErrs;
    AsnAny  snmpInTotalReqVars;
    AsnAny  snmpInTotalSetVars;
    AsnAny  snmpInGetRequests;
    AsnAny  snmpInGetNexts;
    AsnAny  snmpInSetRequests;
    AsnAny  snmpInGetResponses;
    AsnAny  snmpInTraps;
    AsnAny  snmpOutTooBigs;
    AsnAny  snmpOutNoSuchNames;
    AsnAny  snmpOutBadValues;
    AsnAny  snmpOutGenErrs;
    AsnAny  snmpOutGetRequests;
    AsnAny  snmpOutGetNexts;
    AsnAny  snmpOutSetRequests;
    AsnAny  snmpOutGetResponses;
    AsnAny  snmpOutTraps;
    AsnAny  snmpEnableAuthenTraps;
} SNMPMIB_MGMTVARS;


// function handling all the GETs of this MIB
UINT
snmpMibGetHandler(
        UINT actionId,
        AsnAny *objectArray,
        UINT *errorIndex);

// function handling all the SETs of this MIB
UINT
snmpMibSetHandler(
        UINT actionId,
        AsnAny *objectArray,
        UINT *errorIndex);

//----------------------------------------------------------
//  definitions on which rely all the macros from mibentry.c
//----------------------------------------------------------
PSNMP_MGMTVARS  pMibVariables;          // obtained from the SNMP agent in SnmpExtensionMonitor()

#define gf_snmpInPkts                   snmpMibGetHandler
#define gf_snmpOutPkts                  snmpMibGetHandler
#define gf_snmpInBadVersions            snmpMibGetHandler
#define gf_snmpInBadCommunityNames      snmpMibGetHandler
#define gf_snmpInBadCommunityUses       snmpMibGetHandler
#define gf_snmpInASNParseErrs           snmpMibGetHandler
#define gf_snmpInTooBigs                snmpMibGetHandler
#define gf_snmpInNoSuchNames            snmpMibGetHandler
#define gf_snmpInBadValues              snmpMibGetHandler
#define gf_snmpInReadOnlys              snmpMibGetHandler
#define gf_snmpInGenErrs                snmpMibGetHandler
#define gf_snmpInTotalReqVars           snmpMibGetHandler
#define gf_snmpInTotalSetVars           snmpMibGetHandler
#define gf_snmpInGetRequests            snmpMibGetHandler
#define gf_snmpInGetNexts               snmpMibGetHandler
#define gf_snmpInSetRequests            snmpMibGetHandler
#define gf_snmpInGetResponses           snmpMibGetHandler
#define gf_snmpInTraps                  snmpMibGetHandler
#define gf_snmpOutTooBigs               snmpMibGetHandler
#define gf_snmpOutNoSuchNames           snmpMibGetHandler
#define gf_snmpOutBadValues             snmpMibGetHandler
#define gf_snmpOutGenErrs               snmpMibGetHandler
#define gf_snmpOutGetRequests           snmpMibGetHandler
#define gf_snmpOutGetNexts              snmpMibGetHandler
#define gf_snmpOutSetRequests           snmpMibGetHandler
#define gf_snmpOutGetResponses          snmpMibGetHandler
#define gf_snmpOutTraps                 snmpMibGetHandler
#define gf_snmpEnableAuthenTraps        snmpMibGetHandler

#define sf_snmpEnableAuthenTraps        snmpMibSetHandler

#define gb_snmpInPkts                   SNMPMIB_MGMTVARS
#define gb_snmpOutPkts                  SNMPMIB_MGMTVARS
#define gb_snmpInBadVersions            SNMPMIB_MGMTVARS
#define gb_snmpInBadCommunityNames      SNMPMIB_MGMTVARS
#define gb_snmpInBadCommunityUses       SNMPMIB_MGMTVARS
#define gb_snmpInASNParseErrs           SNMPMIB_MGMTVARS
#define gb_snmpInTooBigs                SNMPMIB_MGMTVARS
#define gb_snmpInNoSuchNames            SNMPMIB_MGMTVARS
#define gb_snmpInBadValues              SNMPMIB_MGMTVARS
#define gb_snmpInReadOnlys              SNMPMIB_MGMTVARS
#define gb_snmpInGenErrs                SNMPMIB_MGMTVARS
#define gb_snmpInTotalReqVars           SNMPMIB_MGMTVARS
#define gb_snmpInTotalSetVars           SNMPMIB_MGMTVARS
#define gb_snmpInGetRequests            SNMPMIB_MGMTVARS
#define gb_snmpInGetNexts               SNMPMIB_MGMTVARS
#define gb_snmpInSetRequests            SNMPMIB_MGMTVARS
#define gb_snmpInGetResponses           SNMPMIB_MGMTVARS
#define gb_snmpInTraps                  SNMPMIB_MGMTVARS
#define gb_snmpOutTooBigs               SNMPMIB_MGMTVARS
#define gb_snmpOutNoSuchNames           SNMPMIB_MGMTVARS
#define gb_snmpOutBadValues             SNMPMIB_MGMTVARS
#define gb_snmpOutGenErrs               SNMPMIB_MGMTVARS
#define gb_snmpOutGetRequests           SNMPMIB_MGMTVARS
#define gb_snmpOutGetNexts              SNMPMIB_MGMTVARS
#define gb_snmpOutSetRequests           SNMPMIB_MGMTVARS
#define gb_snmpOutGetResponses          SNMPMIB_MGMTVARS
#define gb_snmpOutTraps                 SNMPMIB_MGMTVARS
#define gb_snmpEnableAuthenTraps        SNMPMIB_MGMTVARS

#define sb_snmpEnableAuthenTraps        SNMPMIB_MGMTVARS

#endif // _MIBFUNCS_H_
