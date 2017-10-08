//
// ph.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
// it is used to generate precompiled header file, if supported.

#if !defined(PH_H__FEF419E3_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
#define PH_H__FEF419E3_6EB6_11D3_907D_204C4F4F5020__INCLUDED_

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// comment next two lines to compile ANSI version
//#define UNICODE
//#define _UNICODE

#if DBG
#define _DEBUG
#endif

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <wingdi.h>
#include <winuser.h>
#include <winreg.h>

#include <cassert>
#define ASSERT	assert
#ifdef _DEBUG
#define VERIFY ASSERT
#else
#define VERIFY(e)	(e)
#endif

#include <conio.h>
#include <climits>
#include <tchar.h>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <new>

// INHERITED_ACE is from windows 2000
#ifndef INHERITED_ACE
#define INHERITED_ACE                     (0x10)
#endif

#endif // !defined(PH_H__FEF419E3_6EB6_11D3_907D_204C4F4F5020__INCLUDED_)
