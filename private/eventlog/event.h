/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    EVENT.H

Abstract:

    Contains the common data structures that should be put in one
    place in the tree.

Author:

    Rajen Shah   (rajens)   21-Aug-1991

Revision History:

--*/

//
// Switch to using the commonly defined (in ntdef.h) UNICODE_STRING
// Do a munge to fix this if it works ok
//

typedef UNICODE_STRING RPC_UNICODE_STRING, *PRPC_UNICODE_STRING;


//
// RPC definition of the SID structure.  Note the use of the [size_is()]
// qualifier to specify the number of elements in the variable size
// imbedded SubAuthorityCount array at runtime.
//
// BUGBUG - This is stolen from samrpc.idl, and should really be placed
//          in a common place for all to use.
//
typedef struct _RPC_SID {
   UCHAR Revision;
   UCHAR SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
   [size_is(SubAuthorityCount)] ULONG SubAuthority[*];
} RPC_SID, *PRPC_SID, **PPRPC_SID;

//
// ANSI counted string
//

typedef struct _RPC_STRING {
    USHORT Length;
    USHORT MaximumLength;
//    [size_is(MaximumLength+0), length_is(Length+1)] PCHAR Buffer;
    [size_is(MaximumLength)] PCHAR Buffer;
} RPC_STRING,  *PRPC_STRING, RPC_ANSI_STRING, *PRPC_ANSI_STRING;

typedef struct _RPC_CLIENT_ID {
    ULONG UniqueProcess;
    ULONG UniqueThread;
} RPC_CLIENT_ID, *PRPC_CLIENT_ID;
