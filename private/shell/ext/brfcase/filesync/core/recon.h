/*
 * recon.h - Reconciliation routines description.
 */


/* Prototypes
 *************/

/* recon.c */

extern void CopyFileStampFromFindData(PCWIN32_FIND_DATA, PFILESTAMP);
extern void MyGetFileStamp(LPCTSTR, PFILESTAMP);
extern void MyGetFileStampByHPATH(HPATH, LPCTSTR, PFILESTAMP);
extern COMPARISONRESULT MyCompareFileStamps(PCFILESTAMP, PCFILESTAMP);

