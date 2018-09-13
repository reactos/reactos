//**********************************************************************
//
// SETUPX.H
//
//  Copyright (c) 1993 - Microsoft Corp.
//  All rights reserved.
//  Microsoft Confidential
//
// Public include file for Chicago specific Setup and device management
// services.
//
// 12/4/93      DONALDM     Created this file to support new Chicago
//                          specific exports in SETUP4.DLL
//**********************************************************************

#ifndef SETUP4_INC
#define SETUP4_INC

#if (WINVER < 0x0400)
// Do a warning message here
#endif

#pragma warning(disable:4201)       // Non-standard extensions
#pragma warning(disable:4209)       // Non-standard extensions
#pragma warning(disable:4214)       // Non-standard extensions

#include <prsht.h>
#include <commctrl.h>              // Need this for the following functions.
RETERR WINAPI DiGetClassImageList(HIMAGELIST  FAR *lpMiniIconList);
RETERR WINAPI DiGetClassImageIndex(LPCSTR lpszClass, int FAR *lpiImageIndex);

RETERR WINAPI DiGetClassDevPropertySheets(LPDEVICE_INFO lpdi, LPPROPSHEETHEADER lppsh, WORD wFlags);

// Flags for the DiGetClassDevPropertySheets API
#define DIGCDP_FLAG_BASIC           0x0001
#define DIGCDP_FLAG_ADVANCED        0x0002

#endif  // SETUP4_INC
