/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1993, Microsoft Corporation
 *
 *  WCOMDLG.H
 *  WOW32 16-bit COMMDLG support
 *
 *  History:
 *      John Vert (jvert) 31-Dec-1992 - created
--*/



ULONG FASTCALL   WCD32ChooseColor(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32ChooseFont(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32ExtendedError(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32GetOpenFileName(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32GetSaveFileName(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32PrintDlg(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32FindText(PVDMFRAME pFrame);
ULONG FASTCALL   WCD32ReplaceText(PVDMFRAME pFrame);

LONG APIENTRY WCD32UpdateFindReplaceTextAndFlags(HWND hwnd, LPARAM lParam);

VOID Multi_strcpy(LPSTR  dst, LPCSTR src);

INT  Multi_strlen(LPCSTR str);

VOID FASTCALL WOWTellWOWThehDlg(HWND hDlg);

VOID Check_ComDlg_pszptr(PCOMMDLGTD ptd, VPVOID vp);
VOID Ssync_WOW_CommDlg_Structs(PCOMMDLGTD pTD, BOOL f16to32, VPVOID wThunkCSIP);

// these flags are combined to form the dwStat parameter in
// Ssync_WOW_CommDlg_Structs() calls
#define  w32to16     0
#define  w16to32     1 
