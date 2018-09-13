/*
 * twin.h - Twin ADT description.
 */


/* Types
 ********/

/*
 * EnumTwins() callback function - called as:
 *
 *    bContinue = EnumTwinsProc(htwin, pData);
 */

typedef BOOL (*ENUMTWINSPROC)(HTWIN, LPARAM);


/* Prototypes
 *************/

/* twin.c */

extern COMPARISONRESULT CompareNameStrings(LPCTSTR, LPCTSTR);
extern COMPARISONRESULT CompareNameStringsByHandle(HSTRING, HSTRING);
extern TWINRESULT TranslatePATHRESULTToTWINRESULT(PATHRESULT);
extern BOOL CreateTwinFamilyPtrArray(PHPTRARRAY);
extern void DestroyTwinFamilyPtrArray(HPTRARRAY);
extern HBRFCASE GetTwinBriefcase(HTWIN);
extern BOOL FindObjectTwinInList(HLIST, HPATH, PHNODE);
extern BOOL EnumTwins(HBRFCASE, ENUMTWINSPROC, LPARAM, PHTWIN);
extern BOOL IsValidHTWIN(HTWIN);
extern BOOL IsValidHTWINFAMILY(HTWINFAMILY);
extern BOOL IsValidHOBJECTTWIN(HOBJECTTWIN);

