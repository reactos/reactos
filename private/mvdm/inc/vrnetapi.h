/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrnetapi.h

Abstract:

    Contains prototypes and definitions for Vdm Redir lanman support routines

Author:

    Richard L Firth (rfirth) 21-Oct-1991

Revision History:

    21-Oct-1991 rfirth
        Created

--*/

//
// the top bit of the api number (word) is used by the dosnet library to
// indicate whether certain APIs should be remoted over a null session
// (according to the code in the dos redir)
//

#define USE_NULL_SESSION_FLAG   0x8000



//
// prototypes
//

VOID
VrNetRemoteApi(
    VOID
    );

VOID
VrNetTransactApi(
    VOID
    );

VOID
VrNetNullTransactApi(
    VOID
    );

VOID
VrNetServerEnum(
    VOID
    );

VOID
VrNetUseAdd(
    VOID
    );

VOID
VrNetUseDel(
    VOID
    );

VOID
VrNetUseEnum(
    VOID
    );

VOID
VrNetUseGetInfo(
    VOID
    );

VOID
VrNetWkstaGetInfo(
    VOID
    );

VOID
VrNetWkstaSetInfo(
    VOID
    );

VOID
VrNetMessageBufferSend(
    VOID
    );

VOID
VrGetCDNames(
    VOID
    );

VOID
VrGetComputerName(
    VOID
    );

VOID
VrGetUserName(
    VOID
    );

VOID
VrGetDomainName(
    VOID
    );

VOID
VrGetLogonServer(
    VOID
    );

VOID
VrNetGetDCName(
    VOID
    );

VOID
VrReturnAssignMode(
    VOID
    );

VOID
VrSetAssignMode(
    VOID
    );

VOID
VrGetAssignListEntry(
    VOID
    );

VOID
VrDefineMacro(
    VOID
    );

VOID
VrBreakMacro(
    VOID
    );

VOID VrNetServiceControl(
    VOID
    );
