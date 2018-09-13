//+----------------------------------------------------------------------------
//
//	File:
//		debug.h
//
//	Contents:
//		macros and declarations for debug support--all are appropriately
//		defined to nothing when not doing debug build
//
//	Classes:
//
//	Functions:
//
//	History:
//		12/30/93 - ChrisWe - added file prologue; defined _DEBUG when
//			DBG==1; added "const" to ASSERTDATA macro
//
//-----------------------------------------------------------------------------


#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <debnot.h>

#ifndef RC_INVOKED
#ifdef _DEBUG
#define DBGSTATE " Debug is on"
#else
#define DBGSTATE " Debug is off"
#endif
#endif  /* RC_INVOKED */

#ifndef _CAIRO_
#include <ole2dbg.h>
#endif

//these are bogus APIs (they do nothing)
STDAPI_(BOOL) ValidateAllObjects( BOOL fSuspicious );
STDAPI_(void) DumpAllObjects( void );

#ifdef _DEBUG
BOOL InstallHooks(void);
BOOL UnInstallHooks(void);

#undef ASSERTDATA
#define ASSERTDATA

#undef AssertSz
#define AssertSz(a,b) ((a) ? NOERROR : FnAssert(#a, b, __FILE__, __LINE__))
#undef Puts
#define Puts(s) OutputDebugString(TEXT(s))

#else   //  !_DEBUG

#define ASSERTDATA
#define AssertSz(a, b) ((void)0)
#define Puts(s) ((void)0)

#endif  //  _DEBUG


// special Assert for asserts below (since the expression is so large)
// REVIEW, shouldn't these be in the debug.h file?
#ifdef _DEBUG
#define AssertOut(a, b) { if (!(a)) FnAssert(szCheckOutParam, b, __FILE__, __LINE__); }
#else
#define AssertOut(a, b) ((void)0)
#endif

#define AssertOutPtrParam(hr, p) \
	AssertOut(SUCCEEDED(hr) && IsValidPtrIn(p, sizeof(OLECHAR)) || \
	FAILED(hr) && (p) == NULL, \
	szBadOutParam)

#define AssertOutPtrIface(hr, p) \
	AssertOut(SUCCEEDED(hr) && IsValidInterface(p) || \
	FAILED(hr) && (p) == NULL, \
	szBadOutIface)

#define AssertOutPtrFailed(p) \
	AssertOut((p) == NULL, \
	szNonNULLOutPtr)

#define AssertOutStgmedium(hr, pstgm) \
	AssertOut(SUCCEEDED(hr) && (pstgm)->tymed != TYMED_NULL || \
	FAILED(hr) && (pstgm)->tymed == TYMED_NULL, \
	szBadOutStgm)


// assert data for above assert out macros; once per dll
// Note that since these are only used in asserts, we leave them as ANSI
#define ASSERTOUTDATA \
    char szCheckOutParam[] = "check out param"; \
    char szBadOutParam[] = "Out pointer param conventions not followed"; \
    char szBadOutIface[] = "Out pointer interface conventions not followed"; \
    char szNonNULLOutPtr[] = "Out pointer not NULL on error"; \
    char szBadOutStgm[] = "Out stgmed param conventions not followed";

extern char szCheckOutParam[];
extern char szBadOutParam[];
extern char szBadOutIface[];
extern char szNonNULLOutPtr[];
extern char szBadOutStgm[];


#ifdef __cplusplus

interface IDebugStream;

/*
 *  Class CBool wraps boolean values in such a way that they are
 *  readily distinguishable fron integers by the compiler so we can
 *  overload the stream << operator.
 */

class FAR CBool
{
    BOOL value;
public:
    CBool (BOOL& b) {value = b;}
    operator BOOL( void ) { return value; }
};


/*
 *  Class CHwnd wraps HWND values in such a way that they are
 *  readily distinguishable from UINTS by the compiler so we can
 *  overload the stream << operator
 */

class FAR CHwnd
{
	HWND m_hwnd;
	public:
		CHwnd (HWND hwnd) {m_hwnd = hwnd; }
		operator HWND( void ) {return m_hwnd;}
};

/*
 * Class CAtom wraps ATOM values in such a way that they are
 *  readily distinguishable from UINTS by the compiler so we can
 *  overload the stream << operator
 */

class FAR CAtom
{
	ATOM m_atom;
	public:
		CAtom (ATOM atom) {m_atom = atom; }
		operator ATOM( void ) {return m_atom; }
};

/*
 *  IDebugStream is a stream to be used for debug output.  One
 *  implementation uses the OutputDebugString function of Windows.
 *
 *  The style is modeled on that of AT&T streams, and so uses
 *  overloaded operators.  You can write to a stream in the
 *  following ways:
 *
 *    *pdbstm << pUnk;  // calls the IDebug::Dump function to
 *                      display the object, if IDebug is supported.
 *    int n;
 *    *pdbstm << n;     // writes n in decimal
 *
-
 *    *pdbstm << sz;    // writes a string
 *
 *    CBool b(TRUE);
 *    *pdbstm << b;     // writes True or False
 *
 *    void FAR * pv;
 *    *pdbstm << pv;    // writes the address pv in hex
 *
 *    TCHAR ch;
 *    *pdbstm << ch;    // writes the character
 *
 *    ATOM atom;
 *    *pdbstm << CAtom(atom);	// writes the string extracted from the atom
 *
 *    HWND hwnd;
 *    *pdbstm << CHwnd(hwnd);  // writes the info about a window handle
 *
 *  These can be chained together, as such (somewhat artificial
 *  example):
 *
 *    REFCLSID rclsid;
 *    pUnk->GetClass(&rclsid);
 *    *pdbstm << rclsid << " at " << (void FAR *)pUnk <<':' << pUnk;
 *
 *  This produces something like:
 *
 *    CFoo at A7360008: <description of object>
 *
 *  The other useful feature is the Indent and UnIndent functions
 *  which allow an object to print some information, indent, print
 *  the info on its member objects, and unindent.  This gives
 *  nicely formatted output.
 *
 *  WARNING:  do not (while implementing Dump) write
 *
 *    *pdbstm << pUnkOuter
 *
 *  since this will do a QueryInterface for IDebug, and start
 *  recursing!  It is acceptable to write
 *
 *    *pdbstm << (VOID FAR *)pUnkOuter
 *
 *  as this will simply write the address of pUnkOuter.
 *
 */


interface IDebugStream : public IUnknown
{
    STDMETHOD_(IDebugStream&, operator << ) ( IUnknown FAR * pDebug ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( REFCLSID rclsid ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( int n ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( long l ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( ULONG l ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( LPCTSTR sz ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( TCHAR ch ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( void FAR * pv ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( CBool b ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( CHwnd hwnd ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( CAtom atom ) = 0;
    STDMETHOD_(IDebugStream&, Tab )( void ) = 0;
    STDMETHOD_(IDebugStream&, Indent )( void ) = 0;
    STDMETHOD_(IDebugStream&, UnIndent )( void ) = 0;
    STDMETHOD_(IDebugStream&, Return )( void ) = 0;
    STDMETHOD_(IDebugStream&, LF )( void ) = 0;
};

STDAPI_(IDebugStream FAR*) MakeDebugStream( short margin=70, short tabsize=4, BOOL fHeader=1);


interface IDebug
{
    STDMETHOD_(void, Dump )( IDebugStream FAR * pdbstm ) = 0;
    STDMETHOD_(BOOL, IsValid )( BOOL fSuspicious = FALSE ) = 0;

#ifdef NEVER
    __export IDebug(void);
    __export ~IDebug(void);
private:

#ifdef _DEBUG
    IDebug FAR * pIDPrev;
    IDebug FAR * pIDNext;

    friend void STDAPICALLTYPE DumpAllObjects( void );
    friend BOOL STDAPICALLTYPE ValidateAllObjects( BOOL fSuspicious );
#endif // _DEBUG
#endif // NEVER
};

/*************************************************************************
** The following functions can be used to log debug messages to a file
**    and simutaneously write them to the dbwin debug window.
**    The CDebugStream implementation automatically writes to a debug
**    log file called "debug.log" in the current working directory.
**    NOTE: The functions are only intended for C programmers. C++
**    programmers should use the "MakeDebugStream" instead.
*************************************************************************/

// Open a log file.
STDAPI_(HFILE) DbgLogOpen(LPCTSTR lpszFile, LPCTSTR lpszMode);

// Close the log file.
STDAPI_(void) DbgLogClose(HFILE fh);

// Write to debug log and debug window (used with cvw.exe or dbwin.exe).
STDAPI_(void) DbgLogOutputDebugString(HFILE fh, LPCTSTR lpsz);

// Write to debug log only.
STDAPI_(void) DbgLogWrite(HFILE fh, LPCTSTR lpsz);

// Write the current Date and Time to the log file.
STDAPI_(void) DbgLogTimeStamp(HFILE fh, LPCTSTR lpsz);

// Write a banner separater to the log to separate sections.
STDAPI_(void) DbgLogWriteBanner(HFILE fh, LPCTSTR lpsz);




/*
 *  STDDEBDECL macro - helper for debug declaration
 *
 */

#ifdef _DEBUG

        #define STDDEBDECL(ignore, classname ) implement CDebug:public IDebug { public: \
            CDebug( C##classname FAR * p##classname ) { m_p##classname = p##classname;} \
            ~CDebug(void) {} \
            STDMETHOD_(void, Dump)(IDebugStream FAR * pdbstm ); \
            STDMETHOD_(BOOL, IsValid)(BOOL fSuspicious ); \
            private: C##classname FAR* m_p##classname; }; \
            DECLARE_NC(C##classname, CDebug) \
            CDebug m_Debug;

    #define CONSTRUCT_DEBUG m_Debug(this),

#else //        _DEBUG

//      no debugging
#define STDDEBDECL(cclassname,classname)
#define CONSTRUCT_DEBUG

#endif  //      _DEBUG

#endif // __cplusplus

#endif // !_DEBUG_H_
 

