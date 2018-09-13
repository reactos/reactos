//
// string.h: Declares data, defines and struct types for string code
//
//

#ifndef __STRING_H__
#define __STRING_H__


#define Bltbyte(rgbSrc,rgbDest,cb)  _fmemmove(rgbDest, rgbSrc, cb)

// Model independent, language-independent (DBCS aware) macros
//  taken from rcsys.h in Pen project and modified.
//
#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsCaseSzEqual(sz1, sz2)     (BOOL)(lstrcmp(sz1, sz2) == 0)
#define SzFromInt(sz, n)            (wsprintf((LPTSTR)sz, (LPTSTR)TEXT("%d"), n), (LPTSTR)sz)


LPTSTR   PUBLIC SzFromIDS (UINT ids, LPTSTR pszBuf, UINT cchBuf);

BOOL    PUBLIC FmtString(LPCTSTR  * ppszBuf, UINT idsFmt, LPUINT rgids, UINT cids);

#endif // __STRING_H__

