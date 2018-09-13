#error "@@@ Don't use this file"

/*
 *  PREINC.H
 *
 *  DGreen
 */

#if defined(WINNT) && !defined(_NTSDK)
// on NT use CRT.DLL instead of MSVCRT20.DLL
#define _NTSDK
#endif

#ifdef DEBUG
// define MEMCHECK to enable memory checking code
#define MEMCHECK
#endif

#ifdef MEMCHECK
// define INSTRUMENT_MEMMAN here to enable memory manager statistics
//#define INSTRUMENT_MEMMAN
#endif

#ifdef WIN32
#define BEGIN_CODESPACE_DATA data_seg(".text")
#define END_CODESPACE_DATA data_seg()
#else
#define BEGIN_CODESPACE_DATA data_seg("_CODE")
#define END_CODESPACE_DATA data_seg()
#endif

#ifdef DEBUG
#define MINTEST
#endif

#if defined(WIN16) && !defined(DEBUG) && !defined(LEGO)
#define LOCAL static
#else
#define LOCAL
#endif

#ifdef DLL
#ifndef _DLL
#define _DLL
#endif
#endif

#if defined(CHICAGO) || defined(WINNT)
#define DOSWIN32
#endif

#ifdef WIN32
#define _INC_OLE
#endif

#ifdef  WIN16
#define DS_SETFOREGROUND    0
#endif  

#define NOSHELLDEBUG //disables asserts in shell.h

#ifndef RC_INVOKED
//$ BUG: remove once stddef.h and friends are fixed
#pragma warning(disable: 4273)
#endif  /* RC_INVOKED */

#define MAPI_DIM


#ifdef DBCS
#define DBCS_EDIT RICHEDIT_CLASS
#else
#define DBCS_EDIT "Edit"
#endif
