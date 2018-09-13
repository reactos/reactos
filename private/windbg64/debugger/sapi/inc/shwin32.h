//-----------------------------------------------------------------------------
//	shwin32.h
//
//  Copyright (C) 1993, Microsoft Corporation
//
//  Purpose:
//		api for the 4 functions in shwin32.c
//
//  Functions/Methods present:
//
//  Revision History:
//
//	[]		05-Mar-1993 Dans	Created
//
//-----------------------------------------------------------------------------

#if !defined(_shwin32_h)
#define _shwin32_h 1

#if defined(WIN32) && !defined(NO_CRITSEC)

void SHInitCritSection(void);
void SHLeaveCritSection(void);
void SHEnterCritSection(void);
void SHInitCritSection(void);

#else

#define SHInitCritSection()
#define SHLeaveCritSection()
#define SHEnterCritSection()
#define SHDeleteCritSection()

#endif

#if defined(WIN32)

void SHCloseHandle(HANDLE);

#else

#define SHCloseHandle(x)

#endif

int __fastcall		SHstrcmpi ( char * sz1, char * sz2 );
char * __fastcall	SHstrupr ( char * sz );
unsigned __fastcall SHtoupperA ( unsigned ch );


#endif
