//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: appletp.h
//
//  This file contains the private (group B) functions of applet DLLs.
//
// History:
//  03-29-93 SatoNa     Created.
//
//---------------------------------------------------------------------------

typedef BOOL (STDAPICALLTYPE FAR * LPFNCREATESCRAPFROMCLIP)
                                            (HWND hwnd, LPCSTR pszDir);

#define STR_CREATESCRAP "appui.dll,Scrap_CreateFromClipboard"
