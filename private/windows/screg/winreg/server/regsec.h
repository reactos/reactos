//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       regsec.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-16-96   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __REGSEC_H__
#define __REGSEC_H__


BOOL
RegSecCheckRemoteAccess(
    PRPC_HKEY   phKey);

BOOL
RegSecCheckPath(
    HKEY                hKey,
    PUNICODE_STRING     pSubKey);

BOOL
InitializeRemoteSecurity(
    VOID
    );

#endif // __REGSEC_H__
