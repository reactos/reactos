/*++ BUILD Version: 0001
 *
 *  MVDM v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  MVDM.H
 *  Constants, macros, and types common to all of MVDM
 *
 *  History:
 *  Created 14-May-1991 by Jeff Parsons (jeffpar)
--*/


/* Common constants, not defined in NT/WIN32
 */
#define K		1024L


/* Common types, not defined in NT/WIN32
 */
#ifdef H2INC
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef char CHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef char *PSZ;
typedef void *PVOID;
typedef ULONG *PULONG;
typedef WORD ATOM;
#endif

// typedef int INT;
// typedef DWORD UINT;
typedef short FAR *LPSHORT;
typedef char SZ[];
typedef char FAR *LPSZ;
typedef PBYTE *PPBYTE;
typedef PWORD *PPWORD;
typedef PDWORD *PPDWORD;
// typedef UINT *PUINT;


// unaligned typedefs

typedef WORD  UNALIGNED *PWORD16;
typedef SHORT UNALIGNED *PSHORT16;
typedef DWORD UNALIGNED *PDWORD16;

#define WORDOF16(i,n)	(((PWORD16)&(i))[n])
#define LOW16(l)		WORDOF16(l,0)
#define HIW16(l)		WORDOF16(l,1)

/* Common macros, not defined in NT/WIN32
 */
#define BYTEOF(i,n)	(((PBYTE)&(i))[n])
#define LOB(i)		BYTEOF(i,0)
#define HIB(i)		BYTEOF(i,1)
#define WORDOF(i,n)	(((PWORD)&(i))[n])
#define LOW(l)		WORDOF(l,0)
#define HIW(l)		WORDOF(l,1)
#define INTOF(i,n)	(((PINT)&(i))[n])
#define UINTOF(i,n)	(((PUINT)&(i))[n])
#ifndef MAKEWORD
#define MAKEWORD(l,h)	((WORD)((BYTE)(l)|((BYTE)(h)<<8)))
#endif
#define MAKEDWORD(l0,h0,l1,h1)	((DWORD)MAKEWORD(l0,h0)|((DWORD)MAKEWORD(l1,h1)<<16))
#define GETBYTE(p)	*((PBYTE)p)++
#ifdef	i386
#define FETCHWORD(s)	((WORD)(s))
#define FETCHDWORD(s)	((DWORD)(s))
#define STOREWORD(d,s)	(WORD)d=(WORD)s
#define STOREDWORD(d,s) (DWORD)d=(DWORD)s
#else
#define FETCHWORD(s)  (*(UNALIGNED WORD *)&(s))
#define FETCHDWORD(s) (*(UNALIGNED DWORD *)&(s))
#define STOREWORD(d,s)  *(UNALIGNED WORD *)&(d)=(WORD)s
#define STOREDWORD(d,s) *(UNALIGNED DWORD *)&(d)=(DWORD)s
#endif
#define FETCHSHORT(s)	((SHORT)(FETCHWORD(s)))
#define FETCHLONG(s)	((LONG)(FETCHDWORD(s)))
#define STORESHORT(d,s) STOREWORD(d,s)
#define STORELONG(d,s)	STOREDWORD(d,s)

#define NUMEL(a)	((sizeof a)/(sizeof a[0]))
#define OFFSETOF(t,f)	((INT)&(((t *)0)->f))

