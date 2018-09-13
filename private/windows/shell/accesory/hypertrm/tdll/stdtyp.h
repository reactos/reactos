/*	File: D:\WACKER\tdll\stdtyp.h (Created: 30-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:36p $
 */

/* This needs to be here because it can change the way ECHAR is defined. */
#include <tdll\features.h>

#if !defined(INCL_STDTYP)
#define INCL_STDTYP

/* --- Define all handles here --- */

typedef struct stSessionExt 		 *HSESSION;
typedef struct stLayoutExt			 *HLAYOUT;
typedef struct stUpdateExt			 *HUPDATE;
typedef struct stEmulExt			 *HEMU;
typedef struct stCnctExt			 *HCNCT;
typedef struct s_com				 *HCOM;
typedef struct stCLoopExt			 *HCLOOP;
typedef struct stXferExt			 *HXFER;
typedef struct stBckScrlExt 		 *HBACKSCRL;
typedef struct stPrintExt			 *HPRINT;
typedef struct stTimerMuxExt		 *HTIMERMUX;
typedef struct stTimerExt			 *HTIMER;
typedef struct stFilesDirs			 *HFILES;
typedef struct stCaptureFile         *HCAPTUREFILE;
typedef struct stTranslateExt		 *HTRANSLATE;

/* --- This one is a little different --- */
#define	SF_HANDLE	int


/* --- Other HA specific types --- */
typedef unsigned KEY_T; 			// for internal key representation
typedef unsigned KEYDEF; 			// for internal key representation
typedef unsigned short RCDATA_TYPE; // for reading resources of type RCDATA

// Character type used by Emulator and Terminal display routines
// 
#if defined(CHAR_NARROW)
    typedef char ECHAR;
	#define ETEXT(x) (ECHAR)x
#else
	typedef unsigned short ECHAR;
	#define ETEXT(x) (ECHAR)x
#endif

/* --- TRUE/FALSE macros --- */

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE 1
#endif


/* --- HA5 code references this stuff alot --- */

#define DIM(a) (sizeof(a) / sizeof(a[0]))
#define IN_RANGE(n, lo, hi) ((lo) <= (n) && (n) <= (hi))
#define bitset(t, b) ((t) |= (b))
#define bitclear(t, b) ((t) &= (~(b)))
#define bittest(t, b) ((t) & (b))

#define startinterval() 	GetTickCount()
#define interval(X) 		((GetTickCount()-(DWORD)X)/100L)

/* --- Just for now --- */

#define STATIC_FUNC	static
#define	FNAME_LEN	260

#endif	/* --- end stdtyp.h --- */
