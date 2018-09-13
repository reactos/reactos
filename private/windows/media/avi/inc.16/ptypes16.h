/*****************************************************************************\
* PTYPES16.H - PORTABILITY MAPPING HEADER FILE
*
* This file provides typedefs for portable 16/32 bit code.
*
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved.
*
\*****************************************************************************/

/* TEMPORARY FIXES: */

#ifndef CCHDEVICENAME
#include <drivinit.h>
#endif

#define ERROR_GETADDR_FAILED     0x8001
#define ERROR_ALLOCATION_FAILURE 0x8002

#define INITWINDOWS()

/* TYPES: */

#ifndef WIN31

typedef  unsigned int   UINT           ;
typedef  WORD           WPARAM         ;
typedef  LONG           LPARAM         ;
typedef  int            HFILE          ;
typedef  HANDLE         HINSTANCE      ;
typedef  HANDLE         HMODULE        ;

#endif /* !WIN31 */

typedef  int            INT            ;
typedef  signed char    SCHAR          ;
typedef  WORD           WORD2DWORD     ;
typedef  char           CHAR           ;
typedef  char           CHARPARM       ;
typedef  int            SHORTPARM      ;
typedef  int            SHORT          ;
typedef  WORD           VERSION        ;
typedef  HANDLE         HMF            ;
typedef  WORD           PDLLMEM        ;
typedef  char           CHAR2ULONG     ;
typedef  unsigned short USHORT2ULONG   ;
typedef  short          SHORT2ULONG    ;
typedef  int            INT2DWORD      ;
typedef  int            INT2WORD       ;
typedef  char           BYTE2WORD      ;
typedef  POINT          MPOINT         ;

#define  WNDPROC        FARPROC
#define  PROC           FARPROC
#define  APIENTRY       FAR PASCAL
#define  HUGE_T         huge

#define HFILE2INT(h, flags)     (INT)(h)
#define INT2HFILE(i)            (HFILE)(i)
#define DUPHFILE(h)             (HFILE)dup((INT)(h))
#define MGLOBALPTR(p)           HIWORD((LONG)p)

/* PRAGMAS */

#define _LOADDS _loadds
#define _EXPORT _export
                       
                       
                       
                       
                       
                       
