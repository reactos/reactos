//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       secboot.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __SECBOOT_H__
#define __SECBOOT_H__

VOID
SbSyncWithKeyThread(
    VOID
    );

NTSTATUS
SbBootPrompt(
    VOID
    );


#endif __SECBOOT_H__

