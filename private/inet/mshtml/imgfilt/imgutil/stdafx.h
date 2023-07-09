// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if DBG==1

#ifdef _DEBUG
#undef _DEBUG
#endif

#define _DEBUG
#pragma message( "Debug Build" )
#else
#ifndef UNIX
// IEUNIX -- _ATL_MIN_CRT just includes a new operator new()... which is a royal pain.
// we don't need to try to minimize the CRT anyhow, so just can it
#define _ATL_MIN_CRT
#endif // IEUNIX
#pragma message( "Release Build" )
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <ocmm.h>

#ifdef UNIX
#include <ddraw.h>
#endif
