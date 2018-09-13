/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUMAN.H
 *  WOW32 16-bit User API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
--*/

typedef ULONG   (*WBP) (LPSTR, int, int, int);

ULONG FASTCALL   WU32ExitWindows(PVDMFRAME pFrame);
ULONG FASTCALL   WU32NotifyWow(PVDMFRAME pFrame);
ULONG FASTCALL   WU32WOWWordBreakProc(PVDMFRAME pFrame);
ULONG FASTCALL   WU32MouseEvent(PVDMFRAME pFrame);
ULONG FASTCALL   WU32KeybdEvent(PVDMFRAME pFrame);
