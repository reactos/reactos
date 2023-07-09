#define OFFSET(x) ((PSTR)(LOWORD((DWORD)(x))))

extern LPWSTR StrChrW(LPCWSTR lpStart, WCHAR wMatch);
extern LPSTR StrChrA(LPCSTR lpStart, CHAR wMatch);
extern LPWSTR StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch);
extern LPSTR StrRChrA(LPCSTR lptart, LPCSTR lpEnd, CHAR wMatch);
extern LPWSTR StrChrIW(LPCWSTR lpStart, WCHAR wMatch);
extern LPSTR StrChrIA(LPCSTR lptart, CHAR wMatch);
extern LPWSTR StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch);
extern LPSTR StrRChrIA(LPCSTR lptart, LPCSTR lpEnd, CHAR wMatch);
extern INT StrCpyNW(LPWSTR lpDest, LPWSTR lpSource, INT nBufSize);
extern INT StrCpyNA(LPSTR lpest, LPSTR lpSource, INT nBufSize);
extern INT StrNCpyW(LPWSTR lpDest, LPWSTR lpSource, INT nChar);
extern INT StrNCpyA(LPSTR lpest, LPSTR lpSource, INT nChar);
extern LPWSTR StrStrW(LPWSTR lpFirst, LPWSTR lpSrch);
extern LPSTR StrStrA(LPSTR lpirst, LPSTR lpSrch);
extern LPWSTR StrRStrW(LPWSTR lpSource, LPWSTR lpLast, LPWSTR lpSrch);
extern LPSTR StrRStrA(LPSTR lpource, LPSTR lpLast, LPSTR lpSrch);
extern LPWSTR StrStrIW(LPWSTR lpFirst, LPWSTR lpSrch);
extern LPSTR StrStrIA(LPSTR lpirst, LPSTR lpSrch);
extern LPWSTR StrRStrIW(LPWSTR lpSource, LPWSTR lpLast, LPWSTR lpSrch);
extern LPSTR StrRStrIA(LPSTR lpSource, LPSTR lpLast, LPSTR lpSrch);

extern BOOL IntlStrEqWorkerW(BOOL fCaseSens, LPCWSTR lpWStr1, LPCWSTR lpWStr2, INT nChar);
extern BOOL IntlStrEqWorkerA(BOOL fCaseSens, LPCSTR lptr1, LPCSTR lpWStr2, INT nChar);

#ifndef UNICODE
#define StrChr StrChrA
#define StrRChr StrRChrA
#define StrChrI StrChrIA
#define StrRChrI StrRChrIA
#define StrCpyN StrCpyNA
#define StrNCpy StrNCpyA
#define StrStr StrStrA
#define StrRStr StrRStrA
#define StrStrI StrStrIA
#define StrRStrI StrRStrIA
#define IntlStrEqN(lpStr1, lpStr2, nChar)   IntlStrEqWorkerA(  TRUE, lpStr1, lpStr2, nChar)
#define IntlStrEqNI(lpStr1, lpStr2, nChar)  IntlStrEqWorkerA( FALSE, lpStr1, lpStr2, nChar)
#else
#define StrChr StrChrW
#define StrRChr StrRChrW
#define StrChrI StrChrIW
#define StrRChrI StrRChrIW
#define StrCpyN StrCpyNW
#define StrNCpy StrNCpyW
#define StrStr StrStrW
#define StrRStr StrRStrW
#define StrStrI StrStrIW
#define StrRStrI StrRStrIW
#define IntlStrEqN(lpStr1, lpStr2, nChar)   IntlStrEqWorkerW(  TRUE, lpStr1, lpStr2, nChar)
#define IntlStrEqNI(lpStr1, lpStr2, nChar)  IntlStrEqWorkerW( FALSE, lpStr1, lpStr2, nChar)
#endif
