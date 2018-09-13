/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mibfuncs.h

Abstract:

    All constants, types, and prototypes to support the MIB manipulation
    functions.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef mibfuncs_h
#define mibfuncs_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "mib.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

// Return type from LAN Manager conver functions
typedef struct lan_return_info_type {

	unsigned int size ;
	unsigned int data_element_type;
	union {
		AsnInteger intval;
		AsnOctetString octstrval;
	} d ;
} lan_return_info_type ;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

void * MIB_common_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,  // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

void * MIB_server_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,    // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

void * MIB_workstation_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,    // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

void * MIB_domain_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,    // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

UINT MIB_srvcs_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_sess_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_users_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_shares_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_prntq_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_wsuses_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_odoms_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_svsond_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_dlogons_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);

//
// Prototype for AdminFilter function
//

VOID
AdminFilter(
    DWORD           Level,
    LPDWORD         pEntriesRead,
    LPBYTE          ShareInfo
    );

//------------------------------- END ---------------------------------------

#endif /* mibfuncs_h */

