/* $Id: os2def.h,v 1.2 2002/03/24 18:55:39 ea Exp $ */
/* This file conains common OS/2 types that are needed to build this dll */ 
/* this file should have temporal character until a better idea is born */

#ifndef __OS2DEF__
#define __OS2DEF__

typedef unsigned long  APIRET;
#define APIENTRY
typedef char *PSZ;
typedef char *NPSZ;
typedef char *NPCH;
#define VOID        void
//

/* define these types only when ntdll is not included */
#if( !defined( __INCLUDE_NTDEF_H ))
	#define CHAR    char           
	#define SHORT   short          
	#define LONG    long           
	typedef char BYTE;    
	typedef unsigned char  UCHAR;  
	typedef unsigned short USHORT; 
	typedef unsigned long  ULONG;  

	typedef CHAR *PCHAR;
	typedef SHORT *PSHORT;
	typedef LONG *PLONG;
	typedef UCHAR *PUCHAR;
	typedef USHORT *PUSHORT;
	typedef ULONG *PULONG;
	typedef VOID   *PVOID;
#endif


//typedef char *PCH;
//typedef const char *PCSZ;


typedef unsigned long LHANDLE;
typedef LHANDLE HMODULE;        /* hmod */
typedef LHANDLE PID;            /* pid  */
typedef LHANDLE TID;            /* tid  */
typedef LHANDLE HFILE;
typedef HFILE	*PHFILE;
typedef HMODULE *PHMODULE;
typedef PID *PPID;
typedef TID *PTID;

typedef  VOID APIENTRY FNTHREAD(ULONG);
typedef FNTHREAD *PFNTHREAD;


#endif	//__OS2DEF__
