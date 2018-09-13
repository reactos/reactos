/*++
 *  intthunk.h
 *
 *  WOW v5.0
 *
 *  Copyright 1996, Microsoft Corporation.  All Rights Reserved.
 *
 *  WOW32.C
 *  WOW32 16-bit API support
 *
 *  History:
 *  Created 7-Dec-96 DaveHart
 *
--*/

ULONG FASTCALL InterpretThunk(PVDMFRAME pFrame, DWORD dwIntThunkID);

//
// Win32 "APIs" which aren't in any headers.
//

BOOL APIENTRY SetMagicColors(HDC,PALETTEENTRY,ULONG);   // from ntgdi\inc\ntgdi.h
int APIENTRY GetRelAbs(HDC,INT);
int APIENTRY SetRelAbs(HDC,INT);

//
// IT() Macro for use in WOW thunk tables (w?tbl2.h)
//

#define IT(Name)        ((LPFNW32) ITID_##Name )

typedef struct _INT_THUNK_TABLEENTRY {
    FARPROC pfnAPI;
    CONST BYTE *pbInstr;
} INT_THUNK_TABLEENTRY;
typedef CONST INT_THUNK_TABLEENTRY * PINT_THUNK_TABLEENTRY;

#ifndef WOWIT_C
extern CONST INT_THUNK_TABLEENTRY IntThunkTable[];
#endif
