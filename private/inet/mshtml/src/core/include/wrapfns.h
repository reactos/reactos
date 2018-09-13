//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       wrapfns.h
//
//  Contents:   The list of Unicode functions wrapped for Win95.  Each
//              wrapped function should listed in alphabetical order with
//              the following format:
//
//      STRUCT_ENTRY(FunctionName, ReturnType, (Param list with args), (Argument list))
//
//              For example:
//
//      STRUCT_ENTRY(RegisterClass, ATOM, (CONST WNDCLASSW * pwc), (pwc))
//
//      For functions which return void, use the following:
//
//      STRUCT_ENTRY_VOID(FunctionName, (Param list with args), (Argument list))
//
//      For functions which do no conversions, use STRUCT_ENTRY_NOCONVERT
//      and STRUCT_ENTRY_VOID_NOCONVERT
//
//----------------------------------------------------------------------------


#ifndef WIN16
STRUCT_ENTRY_NOCONVERT(ChooseColor, BOOL, (LPCHOOSECOLORW lpcc), (lpcc))

STRUCT_ENTRY(ChooseFont, BOOL, (LPCHOOSEFONTW lpcf), (lpcf))
#endif // !WIN16

#if DBG==1
STRUCT_ENTRY_NOCONVERT(LoadLibrary, HINSTANCE, (LPCWSTR lpsz), (lpsz))
#endif
