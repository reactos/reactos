/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGPRNSET.H
 *  WOW32 printer setup support routines
 *
 *  These routines help a Win 3.0 task to complete the printer set-up,
 *  when a user initiates the printer setup from the file menu of an
 *  application.
 *
 *  History:
 *  Created 18-Apr-1991 by Chandan Chauhan (ChandanC)
--*/


#define DBG_UNREFERENCED_LOCAL_VARIABLE(V)  (V)

ULONG FASTCALL WG32DeviceMode (PVDMFRAME pFrame);
ULONG FASTCALL WG32ExtDeviceMode (PVDMFRAME pFrame);
ULONG FASTCALL WG32DeviceCapabilities (PVDMFRAME pFrame);
