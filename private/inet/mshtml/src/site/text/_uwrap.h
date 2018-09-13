#error "@@@ This file is nuked"
/*
 *  _UWRAP.H
 *
 *  Purpose:
 *      declarations for useful ANSI<-->Unicode conversion routines
 *      and classes
 */

#ifndef __UWRAP_H__
#define __UWRAP_H__

enum UN_FLAGS 
{
    UN_NOOBJECTS                = 1,
    UN_CONVERT_WCH_EMBEDDING    = 2
};

// BUGBUG (cthrash) These two functions have to go.  Some key functionality
// is missing like converting special WCH_ chars (e.g. WORDBREAK).

int MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, 
        int cwch = -1, UINT codepage = CP_ACP,
        UN_FLAGS flags = UN_CONVERT_WCH_EMBEDDING);
int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1, UINT uiCodePage = CP_ACP);

#endif // !__UWRAP_H__
