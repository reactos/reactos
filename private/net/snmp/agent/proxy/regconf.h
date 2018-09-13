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


#if 1
#define SNMP_REG_SRV_PARMROOT \
    TEXT("SYSTEM\\CurrentControlSet\\Services\\SNMP\\Parameters")
#else
#define SNMP_REG_SRV_PARMROOT \
    TEXT("SYSTEM\\CurrentControlSetA\\Services\\SNMP\\Parameters")
#endif

#define SNMP_REG_SRV_EAKEY SNMP_REG_SRV_PARMROOT TEXT("\\ExtensionAgents")
#define SNMP_REG_SRV_PMKEY SNMP_REG_SRV_PARMROOT TEXT("\\PermittedManagers")


//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct {
    LPSTR           addrText;
    struct sockaddr addrEncoding;
} AdrList;



// Parameters\ExtensionAgents

typedef struct {
    LPSTR   pathName;
    HANDLE  hExtension;
    FARPROC initAddr;
    FARPROC initAddrEx;
    FARPROC queryAddr;
    FARPROC trapAddr;
    HANDLE  hPollForTrapEvent;
    AsnObjectIdentifier supportedView;
    BOOL    fInitedOk;
} CfgExtensionAgents;


// Parameters\PermittedManagers
typedef AdrList CfgPermittedManagers;


//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern CfgExtensionAgents *extAgents;
extern INT                extAgentsLen;

extern CfgPermittedManagers *permitMgrs;
extern INT                  permitMgrsLen;


//--------------------------- PUBLIC PROTOTYPES -----------------------------

BOOL regconf(VOID);

#define bcopy(slp, dlp, size)   (void)memcpy(dlp, slp, size)


//------------------------------- END ---------------------------------------

#endif /* regconf_h */

