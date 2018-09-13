//
// string.h: Declares data, defines and struct types for string code
//
//

#ifndef __STRING_H__
#define __STRING_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  MACROS

#define Bltbyte(rgbSrc,rgbDest,cb)  _fmemmove(rgbDest, rgbSrc, cb)

// Model independent, language-independent (DBCS aware) macros
//  taken from rcsys.h in Pen project and modified.
//
#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsCaseSzEqual(sz1, sz2)     (BOOL)(lstrcmp(sz1, sz2) == 0)
#define SzFromInt(sz, n)            (wsprintf((LPSTR)sz, (LPSTR)"%d", n), (LPSTR)sz)

#ifndef DBCS
// NB - These are already macros in Win32 land.
#ifdef WIN32
#undef AnsiNext
#undef AnsiPrev
#endif

#define AnsiNext(x)         ((x)+1)
#define AnsiPrev(y,x)       ((x)-1)
#define IsDBCSLeadByte(x)   ((x), FALSE)
#endif

/////////////////////////////////////////////////////  PROTOTYPES

LPSTR PUBLIC SzStrTok(LPSTR string, LPCSTR control);
LPCSTR PUBLIC SzStrCh(LPCSTR string, char ch);
int PUBLIC lstrnicmp(LPCSTR first, LPCSTR last, UINT count);

LPSTR PUBLIC SzFromIDS (UINT ids, LPSTR pszBuf, int cbBuf);

/////////////////////////////////////////////////////  MORE INCLUDES

#endif // __STRING_H__

