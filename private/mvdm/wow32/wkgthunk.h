/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKGTHUNK.H
 *  WOW32 Generic Thunk Routines
 *
 *  History:
 *  Created 11-March-1993 by Matthew Felton (mattfe)
--*/

ULONG FASTCALL WK32ICallProc32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32GetVDMPointer32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32LoadLibraryEx32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32FreeLibrary32W(PVDMFRAME pFrame);
ULONG FASTCALL WK32GetProcAddress32W(PVDMFRAME pFrame);

#ifdef WX86
VOID
TermWx86System(
   VOID
   );
#endif
