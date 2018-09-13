/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    testmib.h

Abstract:

    Sample SNMP Extension Agent for Windows NT.

    These files (testdll.c, testmib.c, and testmib.h) provide an example of 
    how to structure an Extension Agent DLL which works in conjunction with 
    the SNMP Extendible Agent for Windows NT.

    Extensive comments have been included to describe its structure and
    operation.  See also "Microsoft Windows NT SNMP Programmer's Reference".

--*/

#ifndef testmib_h
#define testmib_h

// Necessary includes.

#include <snmp.h>


// MIB Specifics.

#define MIB_PREFIX_LEN            MIB_OidPrefix.idLength
#define MAX_STRING_LEN            255


// Ranges and limits for specific MIB variables.

#define MIB_TOASTER_UP            1
#define MIB_TOASTER_DOWN          2

#define MIB_TOASTER_LIGHTLYWARM   1
#define MIB_TOASTER_BURNT         10

#define MIB_TOASTER_WHITEBREAD    1
#define MIB_TOASTER_OTHERBREAD    7


// MIB function actions.

#define MIB_ACTION_GET         ASN_RFC1157_GETREQUEST
#define MIB_ACTION_SET         ASN_RFC1157_SETREQUEST
#define MIB_ACTION_GETNEXT     ASN_RFC1157_GETNEXTREQUEST


// MIB Variable access privileges.

#define MIB_ACCESS_READ        0
#define MIB_ACCESS_WRITE       1
#define MIB_ACCESS_READWRITE   2


// Macro to determine number of sub-oid's in array.

#define OID_SIZEOF( Oid )      ( sizeof Oid / sizeof(UINT) )


// MIB variable ENTRY definition.  This structure defines the format for
// each entry in the MIB.

typedef struct mib_entry
           {
	   AsnObjectIdentifier Oid;
	   void *              Storage;
	   BYTE                Type;
	   UINT                Access;
	   UINT                (*MibFunc)( UINT, struct mib_entry *,
	                                   RFC1157VarBind * );
	   struct mib_entry *  MibNext;
	   } MIB_ENTRY;


// Internal MIB structure.

extern MIB_ENTRY Mib[];
extern UINT      MIB_num_variables;


// Prefix to every variable in the MIB.

extern AsnObjectIdentifier MIB_OidPrefix;


// Function Prototypes.

UINT ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind, // Variable Binding to resolve
	IN UINT PduAction               // Action specified in PDU
	);

UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);

UINT MIB_control_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);

UINT MIB_doneness_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);

UINT MIB_toasttype_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);


#endif /* testmib_h */

