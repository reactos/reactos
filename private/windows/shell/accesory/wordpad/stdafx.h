// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef _UNICODE
#define VC_EXTRALEAN            // use stripped down Win32 headers
#endif

#define CONVERTERS

#include <afxwin.h>         // MFC core and standard components

//
// MFC 4.2 hardcodes _RICHEDIT_VER to 0x0100 in afxwin.h.  This prevents
// richedit.h from enabling any richedit 2.0 features.
//

#ifdef _RICHEDIT_VER
#if _RICHEDIT_VER < 0x0200
#undef _RICHEDIT_VER
#define _RICHEDIT_VER 0x0200
#endif
#endif

#include <objbase.h>
#include <afxext.h>         // MFC extensions
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxcmn.h>
//#include <afxrich.h>
#include <afxpriv.h>

//
// Private headers for richedit2 support until MFC gets native support
//

#ifndef _AFX_ENABLE_INLINES
#define _AFX_ENABLE_INLINES
#endif

#define _AFXCMN2_INLINE     inline
#define _AFXDLGS2_INLINE    inline
#define _AFXRICH2_INLINE    inline

#include <afxdlgs2.h>
#include <afxcmn2.h>
#include <afxrich2.h>



#define HORZ_TEXTOFFSET 15
#define VERT_TEXTOFFSET 5

class CDisplayIC : public CDC
{
public:
	CDisplayIC() { CreateIC(_T("DISPLAY"), NULL, NULL, NULL); }
};

struct CCharFormat : public CHARFORMAT  // re20 requires this line; added by t-stefb
//struct CCharFormat : public _charformat
{
	CCharFormat() {cbSize = sizeof(CHARFORMAT);}  // re20 requires this line; added by t-stefb
//	CCharFormat() {cbSize = sizeof(_charformat);}
	BOOL operator==(CCharFormat& cf);
};

struct CParaFormat : public _paraformat
{
	CParaFormat() {cbSize = sizeof(_paraformat);}
	BOOL operator==(PARAFORMAT& pf);
};

#include "doctype.h"
#include "chicdial.h"

#include <htmlhelp.h>