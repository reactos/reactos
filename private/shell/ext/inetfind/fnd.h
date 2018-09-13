/*****************************************************************************
 *
 *	fnd.h - Main private header file
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *	Coding conventions:
 *
 *	+ Follow standard shell coding conventions.
 *
 *	+ Standard K&R brace placement and indentation style.
 *
 *	+ Indent by 4 spaces.
 *
 *	+ Fully-brace all dependent clauses.  Never write "if (c) foo();"
 *
 *	+ Do not return in the middle of a function.  If forced,
 *	  use a "goto exit".  This way, you can stick entry/exit stuff
 *	  later without getting caught out.  (I learned this rule the
 *	  hard way.)
 *
 *	+ Declare variables with narrowest possible scope.
 *
 *	+ Always test for success, not failure!  The compiler will
 *	  thank you.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *	NOTE!  This code was written for readability, not efficiency.
 *
 *	I'm trusting the compiler to do optimizations like these:
 *
 *	"Parameter alias":
 *
 *	    Function(LPFOO pfoo)
 *	    {
 *		LPBAR pbar = (LPBAR)pfoo;
 *		... use pbar and never mention pfoo again ...
 *	    }
 *
 *	    --> becomes
 *
 *	    Function(LPFOO pfoo)
 *	    {
 *		#define pbar ((LPBAR)pfoo)
 *		... use pbar and never mention pfoo again ...
 *		#undef pbar
 *	    }
 *
 *	"Speculative Execution":
 *
 *	    Function(PFOO pfoo)
 *	    {
 *		BOOL fRc;
 *		if (... condition 1 ...) {
 *		    ... complicated stuff ...
 *		    *pfoo = result;
 *		    fRc = 1;
 *		} else {		// condition 1 failed
 *		    *pfoo = 0;
 *		    fRc = 0;
 *		}
 *		return fRc;
 *	    }
 *
 *	    --> becomes
 *
 *	    Function(PFOO pfoo)
 *	    {
 *		BOOL fRc = 0;
 *		*pfoo = 0;
 *		if (... condition 1 ...) {
 *		    ... complicated stuff ...
 *		    *pfoo = result;
 *		    fRc = 1;
 *		}
 *		return fRc;
 *	    }
 *
 *	"Single Exit":
 *
 *	    Function(...)
 *	    {
 *		BOOL fRc;
 *		if (... condition 1 ...) {
 *		    ...
 *		    if (... condition 2 ...) {
 *			...
 *			fRc = 1;
 *		    } else {		// condition 2 failed
 *			... clean up ...
 *			fRc = 0;
 *		    }
 *		} else {		// condition 1 failed
 *		    ... clean up ...
 *		    fRc = 0;
 *		}
 *		return fRc;
 *	    }
 *
 *	    --> becomes
 *
 *	    Function(...)
 *	    {
 *		if (... condition 1 ...) {
 *		    ...
 *		    if (... condition 2 ...) {
 *			...
 *			return 1;
 *		    } else {		// condition 2 failed
 *			... clean up ...
 *			return 0;
 *		    }
 *		} else {		// condition 1 failed
 *		    ... clean up ...
 *		    return 0;
 *		}
 *		NOTREACHED;
 *	    }
 *
 *
 *
 *****************************************************************************/

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOIME
#define NOSERVICE
#define WINVER 0x0400
#define _WIN32_WINDOWS 0x0400
#include <windows.h>

#ifdef	RC_INVOKED		/* Define some tags to speed up rc.exe */
#define __RPCNDR_H__		/* Don't need RPC network data representation */
#define __RPC_H__		/* Don't need RPC */
#include <oleidl.h>		/* Get the DROPEFFECT stuff */
#define _OLE2_H_		/* But none of the rest */
#define _WINDEF_
#define _WINBASE_
#define _WINGDI_
#define NONLS
#define _WINCON_
#define _WINREG_
#define _WINNETWK_
#define _INC_COMMCTRL
#define _INC_SHELLAPI
#else
#include <windowsx.h>
#endif

#include <shlobj.h>
#include <shellapi.h>

/*****************************************************************************
 *
 *	Resource identifiers
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *	Icons
 *
 *****************************************************************************/

#define IDI_INETFIND	1		/* My main icon */

/*****************************************************************************
 *
 *	Menus
 *
 *****************************************************************************/

#define IDM_ONTHEINTERNET	1   /* Our sole menu item */

/*****************************************************************************
 *
 *	Strings
 *
 *****************************************************************************/

#define IDS_ONTHEINTERNET	1
#define IDS_FINDHELP		2

#ifndef	RC_INVOKED

/*****************************************************************************
 *
 *	Stuff I'm tired of typing over and over.
 *
 *****************************************************************************/

typedef LPITEMIDLIST PIDL, *PPIDL;
typedef LPCITEMIDLIST PCIDL;
typedef LPSHELLFOLDER PSF;
typedef LPVOID PV;
typedef LPVOID *PPV;
typedef LPCVOID PCV;
typedef REFIID RIID;
typedef LPUNKNOWN PUNK;

/*****************************************************************************
 *
 *	Baggage - Stuff I carry everywhere
 *
 *****************************************************************************/

#define INTERNAL NTAPI	/* Called only within a translation unit */
#define EXTERNAL NTAPI	/* Called from other translation units */
#define INLINE static __inline

#define BEGIN_CONST_DATA data_seg(".text", "CODE")
#define END_CONST_DATA data_seg(".data", "DATA")

#define OBJAT(T, v) (*(T *)(v))		/* Pointer punning */
#define PUN(T, v) OBJAT(T, &(v))	/* General-purpose type-punning */

/*
 * Convert a count of TCHAR's to a count of bytes.
 */
#define cbCtch(ctch) ((ctch) * sizeof(TCHAR))

/*
 * Convert an object (X) to a count of bytes (cb).
 */
#define cbX(X) sizeof(X)

/*
 * Convert an array name (A) to a generic count (c).
 */
#define cA(a) (cbX(a)/cbX(a[0]))

/*
 * Convert an array name (A) to a pointer to its Max.
 * (I.e., one past the last element.)
 */
#define pvMaxA(a) (&a[cA(a)])

#define pvSubPvCb(pv, cb) ((PV)((PBYTE)pv - (cb)))
#define pvAddPvCb(pv, cb) ((PV)((PBYTE)pv + (cb)))
#define cbSubPvPv(p1, p2) ((PBYTE)(p1) - (PBYTE)(p2))

/*
 * Round cb up to the nearest multiple of cbAlign.  cbAlign must be
 * a power of 2 whose evaluation entails no side-effects.
 */
#define ROUNDUP(cb, cbAlign) ((((cb) + (cbAlign) - 1) / (cbAlign)) * (cbAlign))

/*
 * lfNeVV
 *
 * Given two values, return zero if they are equal and nonzero if they
 * are different.  This is the same as (v1) != (v2), except that the
 * return value on unequal is a random nonzero value instead of 1.
 * (lf = logical flag)
 *
 * lfNePvPv
 *
 * The same as lfNeVV, but for pointers.
 *
 * lfPv
 *
 * Nonzero if pv is not null.
 *
 */
#define lfNeVV(v1, v2) ((v1) - (v2))
#define lfNePvPv(v1, v2) lfNeVV((DWORD)(PV)(v1), (DWORD)(PV)(v2))
#define lfPv(pv) ((BOOL)(PV)(pv))

/*
 * land -- Logical and.  Evaluate the first.  If the first is zero,
 * then return zero.  Otherwise, return the second.
 */

#define fLandFF(f1, f2) ((f1) ? (f2) : 0)

/*
 * lor -- Logical or.  Evaluate the first.  If the first is nonzero,
 * return it.  Otherwise, return the second.
 *
 * Unfortunately, due to the stupidity of the C language, this can't
 * be implemented, so we just return 1 if the first is nonzero.
 * GNU has an extension that supports this, which we use. //;Internal
 */

#if defined(__GNUC__) //;Internal
#define fLorFF(f1, f2) ((f1) ?: (f2)) //;Internal
#else //;Internal
#define fLorFF(f1, f2) ((f1) ? 1 : (f2))
#endif //;Internal

/*
 * limp - logical implication.  True unless the first is nonzero and
 * the second is zero.
 */
#define fLimpFF(f1, f2) (!(f1) || (f2))

/*
 * leqv - logical equivalence.  True if both are zero or both are nonzero.
 */
#define fLeqvFF(f1, f2) (!(f1) == !(f2))

/*
 * InOrder - checks that i1 <= i2 < i3.
 */
#define fInOrder(i1, i2, i3) ((unsigned)((i2)-(i1)) < (unsigned)((i3)-(i1)))

/*
 * CopyPvPvCb - Copy some memory around
 * MovePvPvCb - Move some memory around
 */
#define CopyPvPvCb RtlCopyMemory
#define MovePvPvCb RtlMoveMemory

/*
 * memeq - Reverse of memcmp
 */
#define memeq !memcmp

/*
 * fPvPfnCmpPv - Compare two objects for equality using the comparison
 *		 function and the desired outcome.  E.g.,
 *
 *			fPvPfnCmpPv(psz1, lstrcmpi, >, psz2)
 *
 *		 returns nonzero if psz1 is greater than psz2 according
 *		 to lstrcmpi.
 */

#define fPvPfnCmpPv(p1, pfn, cmp, p2) (pfn(p1, p2) cmp 0)

/*
 * lstreq   - nonzero if two strings (according to lstrcmp) are equal
 * lstrne   - nonzero if two strings (according to lstrcmp) are different
 *
 * lstrieq   - nonzero if two strings (according to lstrcmpi) are equal
 * lstrine   - nonzero if two strings (according to lstrcmpi) are different
 *
 * lstrieqA  - nonzero if two strings (according to lstrcmpiA) are equal
 * lstrineA  - nonzero if two strings (according to lstrcmpiA) are different
 */

#define lstreq   !lstrcmp
#define lstrne   lstrcmp

#define lstrieq  !lstrcmpi
#define lstrine  lstrcmpi

#define lstrieqA !lstrcmpiA
#define lstrineA lstrcmpiA

/*****************************************************************************
 *
 *	Wrappers and other quickies
 *
 *****************************************************************************/

#define pvExchangePpvPv(ppv, pv) \
	(PV)InterlockedExchange((PLONG)(ppv), (LONG)(pv))

#define hresUs(us) MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(us))
#define hresLe(le) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, (USHORT)(le))

/*****************************************************************************
 *
 *	Static globals:  Initialized at PROCESS_ATTACH and never modified.
 *
 *****************************************************************************/

HINSTANCE g_hinst;		/* My instance handle */
DEFINE_GUID(CLSID_Fnd, 0x37865980, 0x75d1, 0x11cf,
		       0xbf,0xc7,0x44,0x45,0x53,0x54,0,0);

/*****************************************************************************
 *
 *	Dynamic Globals.  There should be as few of these as possible.
 *
 *	All access to dynamic globals must be thread-safe.
 *
 *****************************************************************************/

ULONG g_cRef;			/* Global reference count */

/*****************************************************************************
 *
 *	fndcf.c - Class Factory
 *
 *****************************************************************************/

STDMETHODIMP CFndFactory_New(RIID riid, PPV ppvObj);

/*****************************************************************************
 *
 *	fndcm.c - IContextMenu, IShellExtInit
 *
 *****************************************************************************/

STDMETHODIMP CFndCm_New(RIID riid, PPV ppvObj);

/*****************************************************************************
 *
 *	Common object managers.
 *
 *****************************************************************************/

typedef struct PREVTBL0 {		/* Simple (non-OLE) object */
    void (NTAPI *FinalizeProc)(PV pv);	/* Finalization procedure */
} PREVTBL0, *PPREVTBL0;

typedef struct PREVTBL {		/* Primary interface */
    REFIID riid;			/* Type of this object */
    void (NTAPI *FinalizeProc)(PV pv);	/* Finalization procedure */
} PREVTBL, *PPREVTBL;

typedef struct PREVTBL2 {		/* Secondary interface */
    ULONG lib;				/* offset from start of object */
} PREVTBL2, *PPREVTBL2;

#ifdef	DEBUG

#define Simple_Interface(C) 		Primary_Interface(C, IUnknown); \
					Default_QueryInterface(C) \
					Default_AddRef(C) \
					Default_Release(C)
#define Simple_Vtbl(C)	    		Primary_Vtbl(C)
#define Simple_Interface_Begin(C)	Primary_Interface_Begin(C, IUnknown)
#define Simple_Interface_End(C)	    	Primary_Interface_End(C, IUnknown)

#else

#define Simple_Interface(C) 		Primary_Interface(C, IUnknown)
#define Simple_Vtbl(C)	    		Primary_Vtbl(C)
#define Simple_Interface_Begin(C)	\
	struct S_##C##Vtbl c_####C##VI = { {		\
	    &IID_##IUnknown,				\
	    C##_Finalize,				\
	}, {						\
	    Common##_QueryInterface,			\
	    Common##_AddRef,				\
	    Common##_Release,				\

#define Simple_Interface_End(C)	    	Primary_Interface_End(C, IUnknown)

#endif

#define Primary_Interface(C, I)				\
	extern struct S_##C##Vtbl {			\
	    PREVTBL prevtbl;				\
	    I##Vtbl vtbl;				\
	} c_##C##VI					\

#define Primary_Vtbl(C) &c_##C##VI.vtbl

#define Primary_Interface_Begin(C, I)			\
	struct S_##C##Vtbl c_####C##VI = { {		\
	    &IID_##I,					\
	    C##_Finalize,				\
	}, {						\
	    C##_QueryInterface,				\
	    C##_AddRef,					\
	    C##_Release,				\

#define Primary_Interface_End(C, I)			\
	} };						\

#define Secondary_Interface(C, I)			\
	extern struct S_##I##_##C##Vtbl {		\
	    PREVTBL2 prevtbl;	 			\
	    I##Vtbl vtbl;	 			\
	} c_##I##_##C##VI				\

#define Secondary_Vtbl(C, I) &c_##I##_##C##VI.vtbl

#define Secondary_Interface_Begin(C, I, nm)		\
	struct S_##I##_##C##Vtbl c_##I##_##C##VI = { {	\
	    _IOffset(C, nm),				\
	}, {						\
	    Forward_QueryInterface,			\
	    Forward_AddRef,				\
	    Forward_Release,				\

#define Secondary_Interface_End(C, I, nm)		\
	} };						\

STDMETHODIMP Common_QueryInterface(PV, REFIID, PPV);
STDMETHODIMP_(ULONG) _Common_AddRef(PV pv);
STDMETHODIMP_(ULONG) _Common_Release(PV pv);

/*
 * In DEBUG, go through the vtbl for additional squirties.
 */
#ifdef	DEBUG
#define Common_AddRef(punk) \
		((IUnknown *)(punk))->lpVtbl->AddRef((IUnknown *)(punk))
#define Common_Release(punk) \
		((IUnknown *)(punk))->lpVtbl->Release((IUnknown *)(punk))
#else
#define Common_AddRef _Common_AddRef
#define Common_Release _Common_Release
#endif

void EXTERNAL Common_Finalize(PV);

STDMETHODIMP _Common_New(ULONG cb, PV vtbl, PPV ppvObj);
#define Common_NewCb(cb, C, ppvObj) _Common_New(cb, Primary_Vtbl(C), ppvObj)
#define Common_New(C, ppvObj) Common_NewCb(cbX(C), C, ppvObj)

STDMETHODIMP Forward_QueryInterface(PV pv, REFIID riid, PPV ppvObj);
STDMETHODIMP_(ULONG) Forward_AddRef(PV pv);
STDMETHODIMP_(ULONG) Forward_Release(PV pv);

/*****************************************************************************
 *
 *	Common_CopyAddRef
 *
 *	Copy a pointer and increment its reference count.
 *
 *	Cannot be a macro because Common_AddRef evaluates its argument
 *	twice.
 *
 *****************************************************************************/

INLINE void Common_CopyAddRef(PV pvDst, PV pvSrc)
{
    PPV ppvDst = pvDst;
    *ppvDst = pvSrc;
    Common_AddRef(pvSrc);
}

/*****************************************************************************
 *
 *	Invoking OLE methods.
 *
 *	Invoke_Release is called with a pointer to the object, not with
 *	the object itself.  It zeros out the variable on the release.
 *
 *****************************************************************************/

void EXTERNAL Invoke_AddRef(PV pv);
void EXTERNAL Invoke_Release(PV pv);

/*****************************************************************************
 *
 *	assert.c - Assertion stuff
 *
 *****************************************************************************/

typedef enum {
    sqflAlways		= 0x00000000,		/* Unconditional */
    sqflDll		= 0x00000001,		/* Dll bookkeeping */
    sqflFactory		= 0x00000002,		/* IClassFactory */
    sqflCm		= 0x00000004,		/* IContextMenu */
    sqflCommon		= 0x00000000,		/* common.c */
    sqflError		= 0x80000000,		/* Errors */
} SQFL;						/* squiffle */

void EXTERNAL SquirtSqflPtszV(SQFL sqfl, LPCTSTR ptsz, ...);
int EXTERNAL AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine);

#ifndef	DEBUG
#define SquirtSqflPtszV sizeof
#endif

/*****************************************************************************
 *
 *	Procedure enter/exit tracking.
 *
 *	Start a procedure with
 *
 *	EnterProc(ProcedureName, (_ "format", arg, arg, arg, ...));
 *
 *	The format string is documented in EmitPal.
 *
 *	End a procedure with one of the following:
 *
 *	    ExitProc();
 *
 *		Procedure returns no value.
 *
 *	    ExitProcX();
 *
 *		Procedure returns an arbitrary DWORD.
 *
 *	    ExitOleProc();
 *
 *		Procedure returns an HRESULT (named "hres").
 *
 *	    ExitOleProcPpv(ppvOut);
 *
 *		Procedure returns an HRESULT (named "hres") and, on success,
 *		puts a new object in ppvOut.
 *
 *****************************************************************************/

#define cpvArgMax	10	/* Max of 10 args per procedure */

typedef struct ARGLIST {
    LPCSTR pszProc;
    LPCSTR pszFormat;
    PV rgpv[cpvArgMax];
} ARGLIST, *PARGLIST;

void EXTERNAL ArgsPalPszV(PARGLIST pal, LPCSTR psz, ...);
void EXTERNAL EnterSqflPszPal(SQFL sqfl, LPCTSTR psz, PARGLIST pal);
void EXTERNAL ExitSqflPalHresPpv(SQFL, PARGLIST, HRESULT, PPV);

#ifdef	DEBUG

SQFL sqflCur;

#define AssertFPtsz(c, ptsz) \
	((c) ? 0 : AssertPtszPtszLn(ptsz, TEXT(__FILE__), __LINE__))
#define ValidateF(c) \
	((c) ? 0 : AssertPtszPtszLn(TEXT(#c), TEXT(__FILE__), __LINE__))
#define D(x)		x

#define SetupEnterProc(nm)				\
	static CHAR s_szProc[] = #nm;			\
	ARGLIST _al[1]					\

#define _ _al,

#define ppvBool	((PPV)1)
#define ppvVoid	((PPV)2)

#define DoEnterProc(v)					\
	ArgsPalPszV v;					\
	EnterSqflPszPal(sqfl, s_szProc, _al)		\

#define EnterProc(nm, v)				\
	SetupEnterProc(nm);				\
	DoEnterProc(v)					\

#define ExitOleProcPpv(ppv)				\
	ExitSqflPalHresPpv(sqfl, _al, hres, (PPV)(ppv))	\

#define ExitOleProc()					\
	ExitOleProcPpv(0)				\

#define ExitProc()					\
	ExitSqflPalHresPpv(sqfl, _al, 0, ppvVoid)	\

#define ExitProcX(x)					\
	ExitSqflPalHresPpv(sqfl, _al, (HRESULT)(x), ppvBool) \

#else

#define AssertFPtsz(c, ptsz)
#define ValidateF(c)	(c)
#define D(x)

#define SetupEnterProc(nm)
#define DoEnterProc(v)
#define EnterProc(nm, v)
#define ExitOleProcPpv(ppv)
#define ExitOleProc()
#define ExitProc()

#endif

#define AssertF(c)	AssertFPtsz(c, TEXT(#c))

/*****************************************************************************
 *
 *	Macros that forward to the common handlers after squirting.
 *	Use these only in DEBUG.
 *
 *	It is assumed that sqfl has been #define'd to the appropriate sqfl.
 *
 *****************************************************************************/

#ifdef  DEBUG

#define Default_QueryInterface(Class)				\
STDMETHODIMP							\
Class##_QueryInterface(PV pv, RIID riid, PPV ppvObj)		\
{								\
    SquirtSqflPtszV(sqfl, TEXT(#Class) TEXT("_QueryInterface()")); \
    return Common_QueryInterface(pv, riid, ppvObj);		\
}								\

#define Default_AddRef(Class)					\
STDMETHODIMP_(ULONG)						\
Class##_AddRef(PV pv)						\
{								\
    ULONG ulRc = _Common_AddRef(pv);				\
    SquirtSqflPtszV(sqfl, TEXT(#Class)				\
			TEXT("_AddRef(%08x) -> %d"), pv, ulRc); \
    return ulRc;						\
}								\

#define Default_Release(Class)					\
STDMETHODIMP_(ULONG)						\
Class##_Release(PV pv)						\
{								\
    ULONG ulRc = _Common_Release(pv);				\
    SquirtSqflPtszV(sqfl, TEXT(#Class)				\
		       TEXT("_Release(%08x) -> %d"), pv, ulRc); \
    return ulRc;						\
}								\

#endif

/*****************************************************************************
 *
 *	mem.c
 *
 *	Be extremely careful with FreePv, because it doesn't work if
 *	the pointer is null.
 *
 *****************************************************************************/

STDMETHODIMP EXTERNAL ReallocCbPpv(UINT cb, PV ppvObj);
STDMETHODIMP EXTERNAL AllocCbPpv(UINT cb, PV ppvObj);

#define FreePpv(ppv) ReallocCbPpv(0, ppv)
#define FreePv(pv) LocalFree((HLOCAL)(pv))

#endif /* !RC_INVOKED */
