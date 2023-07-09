/*
 * util.h - Miscellaneous utility functions module description.
 */


/* Prototypes
 *************/

/* util.h */

extern BOOL IsLocalDrivePath(LPCTSTR);
extern BOOL IsUNCPath(LPCTSTR);
extern BOOL DeleteLastDrivePathElement(LPTSTR);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsContained(PCVOID, UINT, PCVOID, UINT);
extern BOOL IsValidCNRName(LPCTSTR);

#endif

#ifdef DEBUG

extern BOOL IsDriveRootPath(LPCTSTR);

#endif

