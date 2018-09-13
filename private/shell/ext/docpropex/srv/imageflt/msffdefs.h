/*----------------------------------------------------------------------------
| Module    : hildefs.h
|
| Purpose   : platform dependent include file for HALO Imaging Library for
|             Microsoft Windows NT
|
| History   : 4/21/94
|
| Copyright 1990-1994 Media Cybernetics, Inc.
|-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/*-----------------------------------------------------------------
| Define the platform
|------------------------------------------------------------------*/
#define HIL_WINDOWS32     1

#ifndef _WINDOWS_
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define	DllExport	__declspec(dllexport)
#define DllImport	__declspec(dllimport)

#ifdef	_X86_
#define FLTAPI			__stdcall
#else
#define FLTAPI			__cdecl
#endif
#define HILAPI			__cdecl

typedef float *                 LPFLOAT;
typedef double *                LPDOUBLE;
typedef void *                  HPVOID;
typedef LPBYTE *                LPLPBYTE;
#ifndef	LPBOOL
typedef BOOL *                  LPBOOL;
#endif

typedef short *                 LPSHORT;

#ifndef S_IRUSR
#define S_IRUSR		00400
#endif
#ifndef S_IWUSR
#define S_IWUSR		00200
#endif
#ifndef S_IRGRP
#define S_IRGRP		00040
#endif
#ifndef S_IWGRP
#define	S_IWGRP		00020
#endif
#ifndef S_IROTH
#define S_IROTH		00004
#endif
#ifndef S_IWOTH
#define S_IWOTH		00002
#endif	
#ifdef __cplusplus
}
#endif  /* __cplusplus */
