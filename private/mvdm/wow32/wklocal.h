/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKLOCAL.C
 *  WOW32 16-bit Kernel API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/



ULONG FASTCALL WK32LocalAlloc(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalCompact(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalFlags(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalFree(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalHandle(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalInit(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalLock(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalNotify(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalReAlloc(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalShrink(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalSize(PVDMFRAME pFrame);
ULONG FASTCALL WK32LocalUnlock(PVDMFRAME pFrame);
