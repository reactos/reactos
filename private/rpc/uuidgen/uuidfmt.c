/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    uuidfmt.c {v1.00}

Abstract:

This module contains I_UuidStringGenerate, which coalesces
the procedures UuidCreate and UuidToString to create a UUID
in one of the formats IDL, C struct, or plain.

Author:

    Joev Dubach (t-joevd) 6/11/92

Revision History:

--*/

//
// Defines
//

#define IDL_STR "[\nuuid(%s),\nversion(1.0)\n]\ninterface %s\n{\n\n}\n"

//
// Inclusions
//
// The order in which these includes are performed seems, after
// extensive testing and analysis, to be highly crucial to a
// successful NT build.

#include <sysinc.h>
#include <rpc.h>
#include <uuidfmt.h>
#include <stdio.h>
#include <stdlib.h>

//
// Function prototypes.
//

void GenUuidAsIDL (
    char PAPI * MyUuidString,
    char PAPI * UuidFormattedString,
    char PAPI * InterfaceName
    );

void GenUuidAsCStruct (
    char PAPI * MyUuidString,
    char PAPI * UuidFormattedString,
    char PAPI * InterfaceName
    );

void GenUuidPlain (
    char PAPI * MyUuidString,
    char PAPI * UuidFormattedString
    );

void __RPC_FAR * __RPC_API
MIDL_user_allocate(
    size_t size
    )
{
    return malloc(size);
}

void __RPC_API
MIDL_user_free(
    void __RPC_FAR * pvBuf
    )
{
    free(pvBuf);
}



/*

Routine Description:

    This routine creates a UUID in one of several string representations.

Arguments:

    Flag - UUIDGEN_FORMAT_IDL gives an IDL template;
           UUIDGEN_FORMAT_CSTRUCT gives a C Struct;
           UUIDGEN_FORMAT_PLAIN gives a plain UUID.

    UuidFormattedString - Must be preinitialized; will contain result.

    InterfaceName - Name of desired interface; used for IDL and C Structs.

Return Value:

    RPC_S_OK - We successfully converted the UUID into its string
        representation.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a string.

    RPC_S_UUID_NO_ADDRESS - We were unable to obtain the ethernet or
        token ring address for this machine.


*/

RPC_STATUS I_UuidStringGenerate(
    int Flag,
    char PAPI * UuidFormattedString,
    char PAPI * InterfaceName
    )
{
    UUID MyUuid;                    // Storage for a retrieved UUID.
    char PAPI * MyUuidString;
    RPC_STATUS Result;
    int LocalOnly = 0;

    ASSERT(   (Flag == UUIDGEN_FORMAT_IDL)
           || (Flag == UUIDGEN_FORMAT_CSTRUCT)
           || (Flag == UUIDGEN_FORMAT_PLAIN));

    Result = UuidCreate(&MyUuid);
    ASSERT(    (Result == RPC_S_UUID_NO_ADDRESS)
            || (Result == RPC_S_OK)
            || (Result == RPC_S_UUID_LOCAL_ONLY) );

    if (Result == RPC_S_UUID_LOCAL_ONLY)
        {
        Result = RPC_S_OK;
        LocalOnly = 1;
        }

    if (Result == RPC_S_OK)
        {
        Result = UuidToString(
            &MyUuid,
            &MyUuidString
            );
        ASSERT((Result == RPC_S_OK) || (Result == RPC_S_OUT_OF_MEMORY));
        if (Result == RPC_S_OK)
            {
            switch(Flag)
                {
                case UUIDGEN_FORMAT_IDL:
                    GenUuidAsIDL(
                        MyUuidString,
                        UuidFormattedString,
                        InterfaceName
                        );
                    break;
                case UUIDGEN_FORMAT_CSTRUCT:
                    GenUuidAsCStruct(
                        MyUuidString,
                        UuidFormattedString,
                        InterfaceName
                        );
                    break;

                case UUIDGEN_FORMAT_PLAIN:
                    GenUuidPlain(
                        MyUuidString,
                        UuidFormattedString
                        );
                } // end switch
            RpcStringFree(&MyUuidString);
            } // end if
        } // end if

    if (   (Result == RPC_S_OK)
        && (LocalOnly) )
        {
        return(RPC_S_UUID_LOCAL_ONLY);
        }

    return(Result);
} // end I_UuidStringGenerate

void GenUuidAsIDL (
    char PAPI * MyUuidString,
    char PAPI * UuidFormattedString,
    char PAPI * InterfaceName
    )
{
    sprintf(
        UuidFormattedString,
        IDL_STR,
        MyUuidString,
        InterfaceName
        );
}

void GenUuidAsCStruct (
    char PAPI * MyUuidString,
    char PAPI * UuidFormattedString,
    char PAPI * InterfaceName
    )
{
    int      i;
    char     temp[157] =
        "%s = { /* ........-....-....-....-............ */\n"
        "    0x........,\n"
        "    0x....,\n"
        "    0x....,\n"
        "    {0x.., 0x.., 0x.., 0x.., 0x.., 0x.., 0x.., 0x..}\n"
        "  };\n\0";

    for (i=0; i<36; i++) temp[10+i] = MyUuidString[i];
    for (i=0; i<8; i++) temp[56+i] = MyUuidString[i];
    for (i=0; i<4; i++) temp[72+i] = MyUuidString[9+i];
    for (i=0; i<4; i++) temp[84+i] = MyUuidString[14+i];
    for (i=0; i<2; i++) temp[97+i] = MyUuidString[19+i];
    for (i=0; i<2; i++) temp[103+i] = MyUuidString[21+i];
    for (i=0; i<2; i++) temp[109+i] = MyUuidString[24+i];
    for (i=0; i<2; i++) temp[115+i] = MyUuidString[26+i];
    for (i=0; i<2; i++) temp[121+i] = MyUuidString[28+i];
    for (i=0; i<2; i++) temp[127+i] = MyUuidString[30+i];
    for (i=0; i<2; i++) temp[133+i] = MyUuidString[32+i];
    for (i=0; i<2; i++) temp[139+i] = MyUuidString[34+i];
    sprintf(UuidFormattedString, temp, InterfaceName);
}

void GenUuidPlain (
    char PAPI * MyUuidString,
    char PAPI * UuidFormattedString
    )
{
    strcpy(UuidFormattedString, MyUuidString);
    strcat(UuidFormattedString, "\n");
}

