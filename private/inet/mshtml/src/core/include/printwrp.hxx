//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       printwrp.hxx
//
//  Contents:   Wrappers for special printing-specific wrappers for the
//              Trident project.
//
//----------------------------------------------------------------------------

#ifndef I_PRINTWRP_HXX_
#define I_PRINTWRP_HXX_
#pragma INCMSG("--- Beg 'printwrp.hxx'")

// IMPORTANT: We are not converting the DEVMODE structure back and forth
// from ASCII to Unicode on Win95 anymore because we are not touching the
// two strings or any other member.  Converting the DEVMODE structure can
// be tricky because of potential and common discrepancies between the
// value of the dmSize member and sizeof(DEVMODE).  (25155)

#ifndef WIN16
HDC WINAPI CreateDCForPrintingInternal(LPCWSTR lpszDriver, LPCWSTR lpszDevice,
                                       LPCWSTR lpszOutput, CONST DEVMODEW *lpInitData);

HDC WINAPI CreateICForPrintingInternal(LPCWSTR lpszDriver, LPCWSTR lpszDevice,
                                       LPCWSTR lpszOutput, CONST DEVMODEW *lpInitData);

DVTARGETDEVICE *TargetDeviceWFromTargetDeviceA(DVTARGETDEVICE *ptd);
#endif //!WIN16

#pragma INCMSG("--- End 'printwrp.hxx'")
#else
#pragma INCMSG("*** Dup 'printwrp.hxx'")
#endif
