//
//  This file contains definintions of WINUTIL helper functions that are
// exported from SHELL32.DLL. These functions are used only by WUTILS32
// to deal with 16-bit CPL files. Note that SHELL32.DLL just provide
// thunk layer for those functions whose bodies reside in SHELL.DLL.
//
// History:
//  09-20-93 SatoNa     Created
//
#define ISVALIDHINST16(hinst16) ((UINT_PTR)hinst16 >= (UINT_PTR)32)

//
// protos for thunks.  half is in shell32.dll, half in shell.dll
//
//  Notes: CALLCPLEntry16 is defined in shsemip.h
//
int WINAPI GetModuleFileName16(HINSTANCE hinst, LPTSTR szFileName, int cbMax);
HMODULE WINAPI GetModuleHandle16(LPCTSTR szName);
