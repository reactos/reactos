#error "@@@ This file is nuked"
#ifndef I_ZMOUSE_H_
#define I_ZMOUSE_H_
#pragma INCMSG("--- Beg 'zmouse.h'")

/******************************************************************************\
*       ZMOUSE.H - Include file for MSDELTA Zoom mouse DLL. 
*
*       AUTHOR - Paul Henderson, July, 1995
*
*       Copyright (C) 1995 Microsoft Corporation.
*       All rights reserved. 
\******************************************************************************/

// Client Appplication (API) Defines

#define ROLLER_DELTA                120                 // Default value for rolling one detent
#define WM_MOUSEROLLER          (WM_MOUSELAST+1)  // Message sent to apps indicating Z delta
                                                                // wParam = zDelta express in multiples of ROLLER_DELTA
#define WM_SCROLLBUTTONDOWN   (WM_USER+0x5525)  // Message sent to apps when scroll button is down
#define WM_SCROLLBUTTONUP     (WM_USER+0x5526)  // Message sent to apps when scroll button is up


#pragma INCMSG("--- End 'zmouse.h'")
#else
#pragma INCMSG("*** Dup 'zmouse.h'")
#endif
