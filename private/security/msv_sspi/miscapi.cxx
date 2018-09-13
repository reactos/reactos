//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        miscapi.cxx
//
// Contents:    Code for miscellaneous lsa mode NtLm entrypoints
//              Main entry points in the dll:
//                SpGetUserInfo
//
//
// History:     ChandanS   26-Jul-1996   Stolen from kerberos\client2\miscapi.cxx
//
//------------------------------------------------------------------------
#include <global.h>

NTSTATUS NTAPI
SpGetUserInfo(
    IN PLUID LogonId,
    IN ULONG Flags,
    OUT PSecurityUserData * UserData
    )
{
    SspPrint((SSP_API, "Entering SpGetUserInfo\n"));
    // BUGBUG Fields of UserData are logonid & flags. WHAT??

    UNREFERENCED_PARAMETER(LogonId);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(UserData);

    SspPrint((SSP_API, "Leaving SpGetUserInfo\n"));
    return(STATUS_NOT_SUPPORTED);
}

