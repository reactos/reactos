/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKMEM.H
 *  WOW32 KRNL386 - Memory Management Functions
 *
 *  History:
 *  Created 3-Dec-1992 by Matthew Felton (mattfe)
--*/

ULONG FASTCALL WK32VirtualAlloc(PVDMFRAME pFrame);
ULONG FASTCALL WK32VirtualFree(PVDMFRAME pFrame);
#if 0
ULONG FASTCALL WK32VirtualLock(PVDMFRAME pFrame);
ULONG FASTCALL WK32VirtualUnLock(PVDMFRAME pFrame);
#endif
ULONG FASTCALL WK32GlobalMemoryStatus(PVDMFRAME pFrame);
