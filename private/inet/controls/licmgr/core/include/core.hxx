//+----------------------------------------------------------------------------
//  File:   core.hxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


#ifndef	_CORE_HXX
#define	_CORE_HXX


#ifndef	RC_INVOKED
#pragma warning(disable:4097)   // typedef synonym for class name
#pragma	warning(disable:4200)	// Microsoft extensions
#pragma	warning(disable:4201)	// Microsoft extensions
#pragma	warning(disable:4514)	// Inline not used in file
#pragma warning(disable:4702)   // Code not reachable (CComponent constructor hits this)
#pragma	warning(disable:4705)	// Code has no effect warning (some constructors hit this)
#pragma	warning(disable:4512)	// Cannot generate assignment operator
#endif	// !RC_INVOKED


// Constants ------------------------------------------------------------------
#ifndef	STRICT
#define	STRICT 1
#endif

#ifndef	_OLE_QUIET
#define	_OLE_QUIET 1
#endif

#if defined(_WIN32) || defined(WIN32)
#ifndef	_WIN32
#define	_WIN32 1
#endif
#ifndef	WIN32
#define	WIN32 1
#endif
#endif

#if defined(_WINDOWS) || defined(WINDOWS)
#ifndef _WINDOWS
#define	_WINDOWS 1
#endif
#ifndef WINDOWS
#define	WINDOWS 1
#endif
#endif

#if defined(_WIN) || defined(WIN)
#ifndef _WIN
#define	_WIN 1
#endif
#ifndef WIN
#define	WIN 1
#endif
#endif

#if defined(_X86)
#ifndef _X86
#define	_X86 1
#endif
#ifndef X86
#define	X86 1
#endif
#endif

#if defined(_ALPHA)
#ifndef _ALPHA
#define	_ALPHA 1
#endif
#ifndef ALPHA
#define	ALPHA 1
#endif
#endif

#if defined(_MIPS)
#ifndef _MIPS
#define	_MIPS 1
#endif
#ifndef MIPS
#define	MIPS 1
#endif
#endif

#if defined(_DEBUG) || defined(DEBUG)
#ifndef _DEBUG
#define	_DEBUG 1
#endif
#ifndef DEBUG
#define	DEBUG 1
#endif
#endif

#if defined(_BETA) || defined(BETA)
#ifndef _BETA
#define	_BETA 1
#endif
#ifndef BETA
#define	BETA 1
#endif
#endif

#if defined(_SHIP) || defined(SHIP)
#ifndef _SHIP
#define	_SHIP 1
#endif
#ifndef SHIP
#define	SHIP 1
#endif
#endif

#if defined(_DBSHIP) || defined(DBSHIP)
#ifndef _DBSHIP
#define	_DBSHIP 1
#endif
#ifndef DBSHIP
#define	DBSHIP 1
#endif
#endif

#if defined(_CODECOV) || defined(CODECOV)
#ifndef _CODECOV
#define	_CODECOV 1
#endif
#ifndef CODECOV
#define	CODECOV 1
#endif
#endif

#if defined(_UNICODE) || defined(UNICODE)
#ifndef _UNICODE
#define	_UNICODE 1
#endif
#ifndef UNICODE
#define	UNICODE 1
#endif
#endif

#ifndef	RC_INVOKED

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	TRUE
#define	TRUE	1
#endif

#ifdef	COREX
#undef	COREX
#endif
#define	COREX	extern "C" __declspec(dllexport)

#endif	// RC_INVOKED


// Includes -------------------------------------------------------------------
// Windows Headers
#include	<windows.h>

#ifndef	RC_INVOKED
// COM Headers
#include	<ole2.h>
#include	<ole2ver.h>
#include    <olectl.h>

// C/C++ Headers
#include	<crtdbg.h>
#include	<ctype.h>
#include	<eh.h>
#include	<limits.h>
#include	<locale.h>
#include	<malloc.h>
#include	<memory.h>
//#include	<new.h>
#include	<process.h>
#include	<stdarg.h>
#include 	<stddef.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<tchar.h>
#include	<time.h>
#endif	// !RC_INVOKED


// Macros ---------------------------------------------------------------------
#ifndef	RC_INVOKED

#define	ARRAY_SIZE(x)   (sizeof((x))/sizeof(*(x)))
#define UNREF(x)        x

#define NO_COPY(x)      x(const x& srp);    \
	                    x& operator=(const x& srp);


// Core Headers ---------------------------------------------------------------
#include    <debug.hxx>
#include    <dll.hxx>
#include    <dllreg.hxx>
#include    <smart.hxx>
#include    <basecom.hxx>
#include    <util.hxx>
#include    <mime64.hxx>
	                    

#endif // RC_INVOKED

#endif // _CORE_HXX
