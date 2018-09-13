/***
*direct.h - function declarations for directory handling/creation
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This include file contains the function declarations for the library
*   functions related to directory handling and creation.
*
****/

#ifndef _INC_DIRECT

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

/* function prototypes */

int __cdecl _chdir(const char *);
int __cdecl _chdrive(int);
char * __cdecl _getcwd(char *, int);
char * __cdecl _getdcwd(int, char *, int);
int __cdecl _getdrive(void);
int __cdecl _mkdir(const char *);
int __cdecl _rmdir(const char *);

#ifndef __STDC__
/* Non-ANSI names for compatibility */
int __cdecl chdir(const char *);
char * __cdecl getcwd(char *, int);
int __cdecl mkdir(const char *);
int __cdecl rmdir(const char *);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_DIRECT
#endif 
