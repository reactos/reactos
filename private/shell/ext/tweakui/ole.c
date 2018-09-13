/*
 * ole - Random OLE stuff
 *
 *  Random shell stuff is here, too.
 */

#include "tweakui.h"

/*
 * InOrder - checks that i1 <= i2 < i3.
 */
#define fInOrder(i1, i2, i3) ((unsigned)((i2)-(i1)) < (unsigned)((i3)-(i1)))

LPMALLOC pmalloc;

/*****************************************************************************
 *
 *  Ole_Free
 *
 *	Free memory via OLE.
 *
 *****************************************************************************/

void PASCAL
Ole_Free(LPVOID pv)
{
    pmalloc->lpVtbl->Free(pmalloc, pv);
}

/*****************************************************************************
 *
 *  Ole_Release
 *
 *	Release an OLE object.
 *
 *****************************************************************************/

void PASCAL
Ole_Release(LPVOID pv)
{
    ((IUnknown *)pv)->lpVtbl->Release(pv);
}

/*****************************************************************************
 *
 *  Ole_ToUnicode
 *
 *	Convert an ANSI string to Unicode.
 *
 *****************************************************************************/

int PASCAL
Ole_ToUnicode(LPOLESTR lpos, LPCSTR psz)
{
    return MultiByteToWideChar(CP_ACP, 0, psz, -1, lpos, MAX_PATH);
}

#if 0
/*****************************************************************************
 *
 *  Ole_FromUnicode
 *
 *	Convert to an ANSI string from Unicode.
 *
 *****************************************************************************/

int PASCAL
Ole_FromUnicode(LPSTR psz, LPOLESTR lpos)
{
    return WideCharToMultiByte(CP_ACP, 0, lpos, -1, psz, MAX_PATH, NULL, NULL);
}
#endif

/*****************************************************************************
 *
 *  Ole_Init
 *
 *	Initialize the OLE stuff.
 *
 *****************************************************************************/

HRESULT PASCAL
Ole_Init(void)
{
    HRESULT hres;
    hres = SHGetMalloc(&pmalloc);
    if (SUCCEEDED(hres)) {
	return SHGetDesktopFolder(&psfDesktop);
    } else {
	return hres;
    }
}

/*****************************************************************************
 *
 *  Ole_Term
 *
 *	Clean up the OLE stuff.
 *
 *****************************************************************************/

void PASCAL
Ole_Term(void)
{
    if (pmalloc) Ole_Release(pmalloc);
    if (psfDesktop) Ole_Release(psfDesktop);
}

/*****************************************************************************
 *
 *  Ole_ParseHex
 *
 *	Parse a hex string encoding cb bytes (at most 4), then
 *	expect the tchDelim to appear afterwards.  If chDelim is 0,
 *	then no delimiter is expected.
 *
 *	Store the result into the indicated LPBYTE (using only the
 *	size requested), updating it, and return a pointer to the
 *	next unparsed character, or 0 on error.
 *
 *	If the incoming pointer is also 0, then return 0 immediately.
 *
 *****************************************************************************/

LPCTSTR PASCAL
Ole_ParseHex(LPCTSTR ptsz, LPBYTE *ppb, int cb, TCH tchDelim)
{
    if (ptsz) {
	int i = cb * 2;
	DWORD dwParse = 0;

	do {
	    DWORD uch;
	    uch = (unsigned char)*ptsz - '0';
	    if (uch < 10) {		/* a decimal digit */
	    } else {
		uch = (*ptsz | 0x20) - 'a';
		if (uch < 6) {		/* a hex digit */
		    uch += 10;
		} else {
		    return 0;		/* Parse error */
		}
	    }
	    dwParse = (dwParse << 4) + uch;
	    ptsz++;
	} while (--i);

	if (tchDelim && *ptsz++ != tchDelim) return 0; /* Parse error */

	for (i = 0; i < cb; i++) {
	    (*ppb)[i] = ((LPBYTE)&dwParse)[i];
	}
	*ppb += cb;
    }
    return ptsz;
}

/*****************************************************************************
 *
 *  Ole_ParseGuid
 *
 *	Parse a guid.  The format is
 *
 *	{ <dword> - <word> - <word> - <byte> <byte> -
 *			<byte> <byte> <byte> <byte> <byte> <byte> }
 *
 *****************************************************************************/

BOOL PASCAL
Ole_ClsidFromString(LPCTSTR ptsz, LPCLSID pclsid)
{
    if (lstrlen(ptsz) == ctchClsid - 1 && *ptsz == '{') {
	ptsz++;
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 4, TEXT('-'));
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 2, TEXT('-'));
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 2, TEXT('-'));
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1,       0  );
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1, TEXT('-'));
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1,       0  );
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1,       0  );
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1,       0  );
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1,       0  );
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1,       0  );
	ptsz = Ole_ParseHex(ptsz, (LPBYTE *)&pclsid, 1, TEXT('}'));
	return (BOOL)ptsz;
    } else {
	return 0;
    }
}
