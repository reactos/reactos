/*****************************************************************************
 *
 *	assert.c - Assertion stuff
 *
 *****************************************************************************/

#include "fnd.h"

#ifdef DEBUG

#include <stdarg.h>

/*****************************************************************************
 *
 *	SquirtSqflPtszV
 *
 *	Squirt a message with a trailing crlf.
 *
 *****************************************************************************/

void EXTERNAL
SquirtSqflPtszV(SQFL sqfl, LPCTSTR ptsz, ...)
{
    if (sqfl == 0 || (sqfl & sqflCur)) {
	va_list ap;
	TCHAR tsz[1024];
	va_start(ap, ptsz);
	wvsprintf(tsz, ptsz, ap);
	va_end(ap);
	OutputDebugString(tsz);
	OutputDebugString(TEXT("\r\n"));
    }
}

/*****************************************************************************
 *
 *	AssertPtszPtszLn
 *
 *	Something bad happened.
 *
 *****************************************************************************/

int EXTERNAL
AssertPtszPtszLn(LPCTSTR ptszExpr, LPCTSTR ptszFile, int iLine)
{
    SquirtSqflPtszV(sqflAlways, TEXT("Assertion failed: `%s' at %s(%d)"),
		    ptszExpr, ptszFile, iLine);
    DebugBreak();
    return 0;
}

/*****************************************************************************
 *
 *	Procedure call tracing is gross because the C preprocessor is lame.
 *
 *	Oh, if only we had support for m4...
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *	ArgsPszV
 *
 *	Collect arguments to a procedure.
 *
 *	psz -> ASCIIZ format string
 *	... = argument list
 *
 *	The characters in the format string are listed in EmitPal.
 *
 *****************************************************************************/

void EXTERNAL
ArgsPalPszV(PARGLIST pal, LPCSTR psz, ...)
{
    va_list ap;
    va_start(ap, psz);
    if (psz) {
	PPV ppv;
	pal->pszFormat = psz;
	for (ppv = pal->rgpv; *psz; psz++) {
	    *ppv++ = va_arg(ap, PV);
	}
    } else {
	pal->pszFormat = "";
    }
}

/*****************************************************************************
 *
 *	EmitPal
 *
 *	OutputDebugString the information, given a pal.  No trailing
 *	carriage return is emitted.
 *
 *	pal	 -> place where info was saved
 *
 *	Format characters:
 *
 *	p   - 32-bit flat pointer
 *	x   - 32-bit hex integer
 *	s   - TCHAR string
 *	A   - ANSI string
 *	W   - UNICODE string
 *	G   - GUID
 *	u   - unsigned integer
 *	C   - clipboard format
 *
 *****************************************************************************/

void INTERNAL
EmitPal(PARGLIST pal)
{
    char sz[MAX_PATH];
    int i;
    OutputDebugStringA(pal->pszProc);
    OutputDebugString(TEXT("("));
    for (i = 0; pal->pszFormat[i]; i++) {
	if (i) {
	    OutputDebugString(TEXT(", "));
	}
	switch (pal->pszFormat[i]) {

	case 'p':				/* 32-bit flat pointer */
	case 'x':				/* 32-bit hex */
	    wsprintfA(sz, "%08x", pal->rgpv[i]);
	    OutputDebugStringA(sz);
	    break;

	case 's':				/* TCHAR string */
	    if (pal->rgpv[i]) {
		OutputDebugString(pal->rgpv[i]);
	    }
	    break;

	case 'A':				/* ANSI string */
	    if (pal->rgpv[i]) {
		OutputDebugStringA(pal->rgpv[i]);
	    }
	    break;

	case 'G':				/* GUID */
	    wsprintfA(sz, "%08x", *(LPDWORD)pal->rgpv[i]);
	    OutputDebugStringA(sz);
	    break;

	case 'u':				/* 32-bit unsigned decimal */
	    wsprintfA(sz, "%u", pal->rgpv[i]);
	    OutputDebugStringA(sz);
	    break;

	case 'C':
	    if (GetClipboardFormatNameA(PtrToUlong(pal->rgpv[i]), sz, cA(sz))) {
	    } else {
		wsprintfA(sz, "[%04x]", pal->rgpv[i]);
	    }
	    OutputDebugStringA(sz);
	    break;

	default: AssertF(0);			/* Invalid */
	}
    }
    OutputDebugString(TEXT(")"));
}

/*****************************************************************************
 *
 *	EnterSqflPtsz
 *
 *	Mark entry to a procedure.  Arguments were already collected by
 *	ArgsPszV.
 *
 *	sqfl	 -> squirty flags
 *	pszProc  -> procedure name
 *	pal	 -> place to save the name and get the format/args
 *
 *****************************************************************************/

void EXTERNAL
EnterSqflPszPal(SQFL sqfl, LPCSTR pszProc, PARGLIST pal)
{
    pal->pszProc = pszProc;
    if (sqfl == 0 || (sqfl & sqflCur)) {
	EmitPal(pal);
	OutputDebugString(TEXT("\r\n"));
    }
}

/*****************************************************************************
 *
 *	ExitSqflPalHresPpv
 *
 *	Mark exit from a procedure.
 *
 *	pal	 -> argument list
 *	hres	 -> exit result
 *	ppv	 -> optional OUT pointer;
 *		    1 means that hres is a boolean
 *		    2 means that hres is nothing at all
 *
 *****************************************************************************/

void EXTERNAL
ExitSqflPalHresPpv(SQFL sqfl, PARGLIST pal, HRESULT hres, PPV ppvObj)
{
    DWORD le = GetLastError();
    if (ppvObj == ppvVoid) {
    } else if (ppvObj == ppvBool) {
	if (hres == 0) {
	    sqfl |= sqflError;
	}
    } else {
	if (FAILED(hres)) {
	    AssertF(fLimpFF(ppvObj, *ppvObj == 0));
	    sqfl |= sqflError;
	}
    }

    if (sqfl == 0 || (sqfl & sqflCur)) {
	EmitPal(pal);
	OutputDebugString(TEXT(" -> "));
	if (ppvObj != ppvVoid) {
	    TCHAR tszBuf[32];
	    wsprintf(tszBuf, TEXT("%08x"), hres);
	    OutputDebugString(tszBuf);
	    if (ppvObj != ppvBool) {
		if (ppvObj) {
		    wsprintf(tszBuf, TEXT(" [%08x]"), *ppvObj);
		    OutputDebugString(tszBuf);
		}
	    } else if (hres == 0) {
		wsprintf(tszBuf, TEXT(" [%d]"), le);
		OutputDebugString(tszBuf);
	    }
	}
	OutputDebugString(TEXT("\r\n"));
    }

    /*
     *	This redundant test prevents a breakpoint on SetLastError()
     *	from being hit constantly.
     */
    if (le != GetLastError()) {
	SetLastError(le);
    }
}

#endif
