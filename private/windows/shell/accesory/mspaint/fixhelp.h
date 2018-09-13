// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

extern BOOL g_fDisableStandardHelp ;

extern HHOOK g_HelpFixHook ;

void FixHelp(CWnd* pWnd, BOOL fFixWndProc);

void SetHelpFixHook(void) ;

void RemoveHelpFixHook(void) ;
