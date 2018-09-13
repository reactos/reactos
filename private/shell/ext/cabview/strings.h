//*******************************************************************************************
//
// Filename : Strings.h
//	
//				Common defines for Strings stuff 
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


LPSTR  StrChrA(LPCSTR lpStart, WORD wMatch);
LPWSTR StrChrW(LPCWSTR lpStart, WORD wMatch);
LPSTR  StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LPWSTR StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch);
LPSTR  StrChrIA(LPCSTR lpStart, WORD wMatch);
LPWSTR StrChrIW(LPCWSTR lpStart, WORD wMatch);
LPSTR  StrRChrIA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);
LPWSTR StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch);
int    StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
int    StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
int    StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
int    StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
LPSTR  StrStrA(LPCSTR lpFirst, LPCSTR lpSrch);
LPWSTR StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch);
LPSTR  StrRStr(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch);
LPSTR  StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch);
LPWSTR StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch);
LPSTR  StrRStrIA(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch);
LPWSTR StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch);
int    StrCSpnA(LPCSTR lpStr, LPCSTR lpSet);
int    StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet);
int    StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet);
int    StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet);
int    StrToIntA(LPCSTR lpSrc);
int    StrToIntW(LPCWSTR lpSrc);

#ifdef UNICODE
#define StrToInt                StrToIntW
#define StrChr                  StrChrW
#define StrRChr                 StrRChrW
#define StrChrI                 StrChrIW
#define StrRChrI                StrRChrIW
#define StrCSpn                 StrCSpnW
#define StrCSpnI                StrCSpnIW
#define StrCmpN                 StrCmpNW
#define StrCmpNI                StrCmpNIW
#define StrStr                  StrStrW
#define StrStrI                 StrStrIW
#define StrRStrI                StrRStrIW
#else
#define StrToInt                StrToIntA
#define StrChr                  StrChrA
#define StrRChr                 StrRChrA
#define StrChrI                 StrChrIA
#define StrRChrI                StrRChrIA
#define StrCSpn                 StrCSpnA
#define StrCSpnI                StrCSpnIA
#define StrCmpN                 StrCmpNA
#define StrCmpNI                StrCmpNIA
#define StrStr                  StrStrA
#define StrStrI                 StrStrIA
#define StrRStrI                StrRStrIA
#endif
