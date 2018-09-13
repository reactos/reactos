// VCBUDEFS.H - standard defs to be used for hungarian notation

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef VCBUDEFS_INCLUDED
#define VCBUDEFS_INCLUDED

typedef int BOOL;
typedef unsigned UINT;
typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef ULONG	INTV;		// interface version number
typedef ULONG	IMPV;		// implementation version number
typedef ULONG	SIG;		// unique (across PDB instances) signature
typedef ULONG	AGE;		// no. of times this instance has been updated
typedef BYTE*	PB;			// pointer to some bytes
typedef long	CB;			// count of bytes
typedef char*	SZ;			// zero terminated string
typedef const char*	SZ_CONST;// const zero terminated string
typedef char*	PCH;		// char ptr
typedef USHORT	IFILE;		// file index
typedef USHORT	IMOD;		// module index
typedef USHORT	ISECT;		// section index
typedef USHORT	LINE;		// line number
typedef long	OFF;		// offset
#endif
