
#ifndef __WINE_TEXT_H
#define __WINE_TEXT_H

//typedef WINBOOL  (CALLBACK *GRAYSTRINGPROC)(HDC,LPARAM,INT);

#define TAB     9
#define LF     10
#define CR     13
#define SPACE  32
#define PREFIX 38

#define SWAP_INT(a,b)  { int t = a; a = b; b = t; }

int TEXT_DrawTextEx(HDC hDC,void *strPtr,int nCount,LPRECT lpRect,UINT uFormat,LPDRAWTEXTPARAMS dtp,WINBOOL Unicode );

LPCSTR TEXT_NextLineA( HDC hdc, LPCSTR str, INT *count,
                                  LPSTR dest, INT *len, INT width, WORD format);
LPCWSTR TEXT_NextLineW( HDC hdc, LPCWSTR str, INT *count,
                                  LPWSTR dest, INT *len, INT width, WORD format);

WINBOOL TEXT_GrayString(HDC hdc, HBRUSH hb, 
                              GRAYSTRINGPROC fn, LPARAM lp, INT len,
                              INT x, INT y, INT cx, INT cy, WINBOOL Unicode);

LONG TEXT_TabbedTextOutA( HDC hdc, INT x, INT y, LPCSTR lpstr,
                         INT count, INT cTabStops, const INT *lpTabPos,
                         INT nTabOrg, WINBOOL fDisplayText );

LONG TEXT_TabbedTextOutW( HDC hdc, INT x, INT y, LPCWSTR lpstr,
                         INT count, INT cTabStops, const INT *lpTabPos,
                         INT nTabOrg, WINBOOL fDisplayText );

#endif