/*
 * comc.h - Shared routines description.
 */


/* Prototypes
 *************/

/* comc.c */

extern void CatPath(LPTSTR, LPCTSTR);
extern COMPARISONRESULT MapIntToComparisonResult(int);
extern void MyLStrCpyN(LPTSTR, LPCTSTR, int);

#ifdef DEBUG

extern BOOL IsStringContained(LPCTSTR, LPCTSTR);

#endif   /* DEBUG */

#if defined(_SYNCENG_) || defined(_LINKINFO_)

extern void DeleteLastPathElement(LPTSTR);
extern LONG GetDefaultRegKeyValue(HKEY, LPCTSTR, LPTSTR, PDWORD);
extern BOOL StringCopy(LPCTSTR, LPTSTR *);
extern void CopyRootPath(LPCTSTR, LPTSTR);
extern COMPARISONRESULT ComparePathStrings(LPCTSTR, LPCTSTR);
extern BOOL MyStrChr(LPCTSTR, TCHAR, LPCTSTR *);
extern BOOL PathExists(LPCTSTR);
extern BOOL IsDrivePath(LPCTSTR);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidDriveType(UINT);
extern BOOL IsValidPathSuffix(LPCTSTR);

#endif   /* DEBUG || VSTF */

#ifdef DEBUG

extern BOOL IsRootPath(LPCTSTR);
extern BOOL IsTrailingSlashCanonicalized(LPCTSTR);
extern BOOL IsFullPath(LPCTSTR);
extern BOOL IsCanonicalPath(LPCTSTR);
extern BOOL IsValidCOMPARISONRESULT(COMPARISONRESULT);

#endif   /* DEBUG */

#endif   /* _SYNCENG_ || _LINKINFO_ */

