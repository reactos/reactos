/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    regconf.h

Abstract:

    Registry configuration routines.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef regconf_h
#define regconf_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <windows.h>

#include <winsock.h>

#include <snmp.h>


#define SNMP_REG_SRV_PARMROOT \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\SNMP\\Parameters")
#define SNMP_REG_SRV_TEKEY SNMP_REG_SRV_PARMROOT \
    TEXT("\\EnableAuthenticationTraps")
#define SNMP_REG_SRV_TDKEY SNMP_REG_SRV_PARMROOT TEXT("\\TrapConfiguration")
#define SNMP_REG_SRV_VCKEY SNMP_REG_SRV_PARMROOT TEXT("\\ValidCommunities")


//--------------------------- PUBLIC STRUCTS --------------------------------

// Parameters\TrapConfiguration

typedef struct {
    LPSTR           addrText;
    struct sockaddr addrEncoding;
} AdrList;

typedef struct {
    LPSTR   communityName;
    INT     addrLen;
    AdrList *addrList;
} CfgTrapDestinations;


// Parameters\ValidCommunities

typedef struct {
    LPSTR   communityName;
} CfgValidCommunities;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern BOOL                enableAuthTraps;

extern CfgTrapDestinations *trapDests;
extern INT                 trapDestsLen;

extern CfgValidCommunities *validComms;
extern INT                 validCommsLen;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)

BOOL tdConfig(
    OUT CfgTrapDestinations **trapDests,
    OUT INT *trapDestsLen);

BOOL vcConfig(
    OUT CfgValidCommunities  **validComms,
    OUT INT *validCommsLen);

//------------------------------- END ---------------------------------------

#endif /* regconf_h */
