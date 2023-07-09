/*
 * expandft.h - Routines for expanding folder twins to object twins
 *              description.
 */


/* Types
 ********/

/* subtree enumeration callback function called by ExpandSubtree() */

typedef BOOL (*EXPANDSUBTREEPROC)(LPCTSTR, PCWIN32_FIND_DATA, PVOID);


/* Prototypes
 *************/

/* expandft.c */

extern TWINRESULT ExpandSubtree(HPATH, EXPANDSUBTREEPROC, PVOID);
extern TWINRESULT ExpandFolderTwinsIntersectingTwinList(HTWINLIST, CREATERECLISTPROC, LPARAM);
extern BOOL NamesIntersect(LPCTSTR, LPCTSTR);

#ifdef DEBUG

extern BOOL IsValidTWINRESULT(TWINRESULT);

#endif

