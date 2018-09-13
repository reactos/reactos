/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WULANG.H
 *  WOW32 16-bit User API support
 *
 *
 *  It thunks the win 3.x language functions to NT. These functions are
 *  mainly used by the programs that are ported to various international
 *  languages.
 *
 *  History:
 *  Created 19-April-1992 by Chandan Chauhan (ChandanC)
 *
--*/


ULONG FASTCALL WU32AnsiLower(PVDMFRAME pFrame);
ULONG FASTCALL WU32AnsiLowerBuff(PVDMFRAME pFrame);
ULONG FASTCALL WU32AnsiNext(PVDMFRAME pFrame);
ULONG FASTCALL WU32AnsiPrev(PVDMFRAME pFrame);
ULONG FASTCALL WU32AnsiUpper(PVDMFRAME pFrame);
ULONG FASTCALL WU32AnsiUpperBuff(PVDMFRAME pFrame);
ULONG FASTCALL WU32lstrcmp(PVDMFRAME pFrame);
ULONG FASTCALL WU32lstrcmpi(PVDMFRAME pFrame);
ULONG FASTCALL WU32wvsprintf(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsCharAlpha(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsCharAlphaNumeric(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsCharLower(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsCharUpper(PVDMFRAME pFrame);
