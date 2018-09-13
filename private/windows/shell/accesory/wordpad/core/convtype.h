#ifndef CONVTYPE_H
#define CONVTYPE_H

//
// %%File:      CONVTYPE.H
//
// %%Unit:      CORE/Common Conversions Code
//
// %%Author:    JohnPil
//
// Copyright (C) 1989-1993, Microsoft Corp.
//
// Global type definitions for conversions code.
//


typedef int bool;

#ifndef PASCAL
#define PASCAL pascal
#endif

#ifndef FAR
#ifdef PC
#define FAR _far
#else
#define FAR
#endif
#endif

#ifndef NEAR
#ifdef PC
#define NEAR _near
#else
#define NEAR
#endif
#endif

// Use __HUGE rather than HUGE or _HUGE as Excel mathpack defines both as externs
#ifndef __HUGE
#ifdef PC
#ifndef NT
#define __HUGE _huge
#else
#define __HUGE
#endif //NT
#else
#define __HUGE
#endif //PC
#endif //__HUGE

#ifndef STATIC
#define STATIC static
#endif

#ifndef EXTERN
#define EXTERN extern
#endif

//  ABSOLUTE SIZE
//  -------------
#ifndef VOID
#define VOID void
#endif

#ifndef BYTE
#define BYTE unsigned char				// 8-bit unsigned data
#define BYTE_MAX 255
#endif

#ifndef CHAR
#define CHAR char						// 8-bit data
#endif

typedef unsigned CHAR UCHAR;

typedef short int SHORT;
#define SHORT_MAX						32767
#define SHORT_MIN						-32767

#ifndef WORD
#define WORD unsigned short				// 16-bit unsigned data
#define WORD_MAX 65535
#endif

typedef WORD BF;						// bitfields are 16-bit unsigned

#ifndef LONG
#define LONG long						// 32-bit data
#endif

#ifndef DWORD
#define DWORD unsigned long				// 32-bit unsigned data
#endif

#ifndef FLOAT
#define FLOAT float						// fixed size absolute float
#endif

#ifndef DOUBLE
#ifndef NT_WORDPAD
#define DOUBLE double					// fixed size absolute double
#endif
#endif

//  VARIABLE SIZE
//  -------------
#ifndef INT
#define INT int							// Most efficient size for processing info
#endif

#ifndef UNSIGNED
#define UNSIGNED unsigned INT
#endif

#ifndef BOOL
#define BOOL INT						// Boolean data
#endif

#define FC              long
#define CP              long
#define PN              WORD
typedef unsigned char byte;

// things which are normally defined in windows.h for windows, but now on Mac
#ifdef MAC

#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))

typedef char FAR *LPSTR;
typedef const char FAR *LPCSTR;
typedef WORD HWND;

#endif

// define platform-independent function type templates

#if defined(MAC)

typedef int (PASCAL * FARPROC) ();
typedef void FAR *LPVOID;	// These are already defined for PC in Windows.h
typedef void **HGLOBAL;		// but have to be defined for Mac.

#define LOCAL(type) type NEAR PASCAL
#define GLOBAL(type) type PASCAL

#elif defined(NT)

typedef int (WINAPI * FARPROC)();

#define LOCAL(type) type NEAR WINAPI
#define GLOBAL(type) type WINAPI

#elif defined(DOS)

typedef int (FAR PASCAL * FARPROC)();

#define LOCAL(type) type NEAR PASCAL
#define GLOBAL(type) type PASCAL

#else
#error Enforced Compilation Error
#endif


// define main function types

#define LOCALVOID 		LOCAL(VOID)
#define LOCALBOOL 		LOCAL(BOOL)
#define LOCALCH   		LOCAL(char)
#define LOCALBYTE 		LOCAL(BYTE)
#define LOCALINT  		LOCAL(INT)
#define LOCALUNS      	LOCAL(UNSIGNED)
#define LOCALSHORT		LOCAL(SHORT)
#define LOCALWORD       LOCAL(WORD)
#define LOCALLONG		LOCAL(LONG)
#define LOCALDWORD  	LOCAL(DWORD)
#define LOCALFC 		LOCAL(FC)
#define LOCALCP   		LOCAL(CP)
#define LOCALPVOID  	LOCAL(void *)
#define LOCALHVOID   	LOCAL(void **)
#define LOCALPCH  		LOCAL(char *)
#define LOCALSZ   		LOCAL(char *)
#define LOCALLPCH 		LOCAL(char FAR *)
#define LOCALUCHAR   	LOCAL(unsigned char)
#define LOCALPUCHAR  	LOCAL(unsigned char *)
#define LOCALFH			LOCAL(FH)

#define GLOBALVOID 		GLOBAL(VOID)
#define GLOBALBOOL 		GLOBAL(BOOL)
#define GLOBALCH   		GLOBAL(char)
#define GLOBALBYTE 		GLOBAL(BYTE)
#define GLOBALINT  		GLOBAL(INT)
#define GLOBALUNS      	GLOBAL(UNSIGNED)
#define GLOBALSHORT		GLOBAL(SHORT)
#define GLOBALWORD      GLOBAL(WORD)
#define GLOBALLONG		GLOBAL(LONG)
#define GLOBALDWORD  	GLOBAL(DWORD)
#define GLOBALFC 		GLOBAL(FC)
#define GLOBALCP   		GLOBAL(CP)
#define GLOBALPVOID  	GLOBAL(void *)
#define GLOBALHVOID   	GLOBAL(void **)
#define GLOBALPCH  		GLOBAL(char *)
#define GLOBALSZ   		GLOBAL(char *)
#define GLOBALLPCH  	GLOBAL(char FAR *)
#define GLOBALUCHAR   	GLOBAL(unsigned char)
#define GLOBALPUCHAR  	GLOBAL(unsigned char *)
#define GLOBALFH        GLOBAL(FH)
#define GLOBALFN        GLOBAL(FN)
#ifndef DOSSA
#define GLOBALHGLOBAL	GLOBAL(HGLOBAL)
#define GLOBALLPVOID	GLOBAL(LPVOID)
#endif

#define fTrue           1
#define fFalse          0

#ifndef NULL
#define NULL	0
#endif

#ifndef hgNil
#define hgNil ((HGLOBAL)NULL)
#endif

// maximum lengths of numbers->strings, used with SzFrom???? funcs.
#define cchMaxSzInt		7
#define cchMaxSzWord 	6
#define cchMaxSzLong	12
#define cchMaxSzDword	11

#endif // CONVTYPE_H

