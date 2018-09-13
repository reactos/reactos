#ifndef _INC_SHLWAPI
// If these routines are not already defined in shlwapi, do it here.

#define OFFSET(x) ((PTSTR)(LOWORD((DWORD)(x))))

extern UINT   PASCAL StrCpyN( LPTSTR lpDest, LPTSTR lpSource, short nBufSize );
extern LPTSTR PASCAL StrChr( LPTSTR lpStart, WORD wMatch );
extern LPTSTR PASCAL StrRChr( LPTSTR lpStart, LPTSTR lpEnd, WORD wMatch );
extern LPTSTR PASCAL StrChrI( LPTSTR lpStart, WORD wMatch );
extern LPTSTR PASCAL StrRChrI( LPTSTR lpStart, LPTSTR lpEnd, WORD wMatch );
extern LPTSTR PASCAL StrStr( LPTSTR lpFirst, LPTSTR lpSrch );
extern LPTSTR PASCAL StrRStr( LPTSTR lpSource, LPTSTR lpLast, LPTSTR lpSrch );
extern LPTSTR PASCAL StrStrI( LPTSTR lpFirst, LPTSTR lpSrch );
extern LPTSTR PASCAL StrRStrI( LPTSTR lpSource, LPTSTR lpLast, LPTSTR lpSrch );
extern short  PASCAL StrCmpN( LPTSTR lpStr1, LPTSTR lpStr2, short nChar );
extern short PASCAL StrCmpNI( LPTSTR lpStr1, LPTSTR lpStr2, short nChar );

#endif

/****************************************************************************
 * $lgb$
 * 1.0     7-Mar-94   eric Initial revision.
 * $lge$
 *
 ****************************************************************************/

