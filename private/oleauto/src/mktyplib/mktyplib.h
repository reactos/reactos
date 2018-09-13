/* single line comments and other horrid extensions are ok */
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4209)

#ifdef WIN32 

#define __export
#define EXPORT
/* This line specifies widechar ctype table instead of ascii (ctype.h). */
#define _NEWCTYPETABLE

// force include of OLE2 rather than OLE1
#define INC_OLE2
// Yea, right...
#define WIN32_LEAN_AND_MEAN
#endif	//WIN32

#ifndef MAC
#include <windows.h>
#endif //!MAC

// general typedef's and define's for use in MKTYPLIB
#define	TRUE		1
#define	FALSE		0

// extend stuff in dispatch.h for RISC builds
#define SYS_MAX 	(SYS_MAC+1)

// environment-specific defines
#ifdef MAC
#define	FAR
#define	NEAR
#define	SYS_DEFAULT	    SYS_MAC
#define ALIGN_MAX_DEFAULT   2
//UNDONE: SYS_MAC_PPC?
#else	//MAC
#ifdef	WIN32
// defined by NT's windows.h
#ifndef FAR
#define	FAR
#endif
//#define	NEAR
#define	SYS_DEFAULT	    SYS_WIN32
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define ALIGN_MAX_DEFAULT   8
#else	//_MIPS_ || _ALPHA_ || _PPC_
#define ALIGN_MAX_DEFAULT   4
#endif	//_MIPS_ || _ALPHA_ || _PPC_
#else	//WIN32
#define	FAR _far
#define	NEAR _near
#define	SYS_DEFAULT	    SYS_WIN16
#define ALIGN_MAX_DEFAULT   1
#endif	//WIN32
#endif	//MAC

// basic types
#undef VOID			// windows.h #defines these instead of using
#undef LONG			// a typedef, but ole2.h typedef's them
typedef void VOID;
typedef char CHAR;
typedef short SHORT;
typedef int	INT;
typedef long	LONG;
typedef	unsigned char 	BYTE;
typedef	unsigned short	WORD;
typedef	unsigned long	DWORD;
typedef	unsigned short  USHORT;
typedef	unsigned int	UINT;
typedef	unsigned long	ULONG;
typedef	int 		BOOL;
typedef	CHAR FAR *	LPSTR;
typedef	VOID FAR *	LPVOID;

#ifdef MAC		// no windows.h -- OLE headers need the following
//#define SIZEL   LONG
#define LPSIZE	LPVOID
#define LPRECT	LPVOID
#define Byte BYTE
#define _MAC		// for OLE include files
#define USE_INCLUDE	// for OLE include files
#define CDECL _cdecl
#define PASCAL _pascal
typedef const CHAR *  LPCSTR;     // sz
#include <types.h>
#endif

#ifndef WIN16
#define _fmalloc malloc
#define _ffree   free
#define _fmemcpy memcpy
#define _fmemcmp memcmp
#define _fstrcpy strcpy
#define _fstrcmp strcmp
#define _fstrcmpi strcmpi
#define _fstrdup strdup
#define _fstrcat strcat
#endif


// basic defines
#ifndef	 WIN32		// defined by NT's windows.h
#define LOWORD(l)       ((WORD)(DWORD)(l))
#define HIWORD(l)       ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))
#endif	//WIN32

// *******************************************************
//  switches
// *******************************************************
#define	FV_PROPSET		FALSE	// don't allow PROPERTY_SETs

#ifdef MAC
#define	FV_CPP			FALSE	// no C pre-processor for MAC
#else //MAC
#define	FV_CPP			TRUE	// support spawning C pre-processor
#endif //MAC

// *******************************************************
//  misc types
// *******************************************************

// DBCS support

extern char g_rgchLeadBytes[];      // lead byte table

#define NextChar(pch) (pch = XStrInc(pch))
#define	IsLeadByte(ch) (g_rgchLeadBytes[(unsigned char)ch])                 

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern char * FAR XStrChr(char * xsz, int ch);  // wrapper for strchr
extern char * XStrInc(char * pch);  // AnsiNext-like function

#ifdef __cplusplus
}
#endif // __cplusplus


#ifdef	__cplusplus
extern "C" {
#endif

extern VOID FAR DisplayLine(CHAR * szLine);	// cover for printf/messagebox

// debug support
#ifdef DEBUG
extern VOID DebugOut(CHAR *);		// cover for puts/messagebox
extern BOOL fDebug;	// TRUE if /debug specified on command line
extern VOID AssertFail(CHAR *, WORD);
#define Assert(expr) if (!(expr)) AssertFail(__FILE__, __LINE__);
#define SideAssert(expr) Assert(expr)
#else	// DEBUG
#define Assert(expr)
#define SideAssert(expr) expr;
#endif	// DEBUG

#ifdef	__cplusplus
}
#endif
