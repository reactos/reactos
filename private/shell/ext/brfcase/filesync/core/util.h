/*
 * util.h - Miscellaneous utility functions module description.
 */


/* Constants
 ************/

/* maximum length of buffer required by SeparatePath() */

#define MAX_SEPARATED_PATH_LEN            (MAX_PATH_LEN + 1)

/* events for NotifyShell */

typedef enum _notifyshellevent
{
   NSE_CREATE_ITEM,
   NSE_DELETE_ITEM,
   NSE_CREATE_FOLDER,
   NSE_DELETE_FOLDER,
   NSE_UPDATE_ITEM,
   NSE_UPDATE_FOLDER
}
NOTIFYSHELLEVENT;
DECLARE_STANDARD_TYPES(NOTIFYSHELLEVENT);


/* Prototypes
 *************/

/* util.c */

extern void NotifyShell(LPCTSTR, NOTIFYSHELLEVENT);
extern COMPARISONRESULT ComparePathStringsByHandle(HSTRING, HSTRING);
extern COMPARISONRESULT MyLStrCmpNI(LPCTSTR, LPCTSTR, int);
extern void ComposePath(LPTSTR, LPCTSTR, LPCTSTR);
extern LPCTSTR ExtractFileName(LPCTSTR);
extern LPCTSTR ExtractExtension(LPCTSTR);
extern HASHBUCKETCOUNT GetHashBucketIndex(LPCTSTR, HASHBUCKETCOUNT);
extern COMPARISONRESULT MyCompareStrings(LPCTSTR, LPCTSTR, BOOL);
extern BOOL RegKeyExists(HKEY, LPCTSTR);
extern BOOL CopyLinkInfo(PCLINKINFO, PLINKINFO *);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidPCLINKINFO(PCLINKINFO);

#endif

