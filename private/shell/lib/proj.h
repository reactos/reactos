//
// proj.h:      Main header
//
//


#ifndef __PROJ_H__
#define __PROJ_H__

#ifndef STRICT
#define STRICT
#endif

#if defined(WINNT) || defined(WINNT_ENV)

//
// NT uses DBG=1 for its debug builds, but the Win95 shell uses
// DEBUG.  Do the appropriate mapping here.
//
#if DBG
#define DEBUG 1
#endif

#endif  // WINNT

#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <debug.h>
#include <port32.h>
#include <ccstock.h>

#include <shsemip.h>        // for _ILNext

STDAPI_(LPITEMIDLIST) SafeILClone(LPCITEMIDLIST pidl);
#define ILClone         SafeILClone   

// Some files are compiled twice: once for unicode and once for ansi.
// There are some functions which do not want to be declared twice
// (the ones which don't use string parameters).  Otherwise we'd get
// duplicate redefinitions.
//
// These are wrapped with #ifdef DECLARE_ONCE.
#ifdef UNICODE
#define DECLARE_ONCE
#else
#undef DECLARE_ONCE
#endif


// Note that CharNext is not supported on win95.  Normally we would
// include w95wraps.h, but comctl does not link to shlwapi and
// we don't want to add this dependency.
#ifdef UNICODE
// Note that this will still break if we ever go back to non-unicode
__inline LPWSTR CharNextWrapW_(LPWSTR psz) {return ++psz;}
#undef CharNext
#define CharNext CharNextWrapW_
#endif


#endif // __PROJ_H__
