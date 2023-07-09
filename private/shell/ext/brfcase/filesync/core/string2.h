/*
 * string.h - String table ADT description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HSTRING);
DECLARE_STANDARD_TYPES(HSTRING);
DECLARE_HANDLE(HSTRINGTABLE);
DECLARE_STANDARD_TYPES(HSTRINGTABLE);

/* count of hash buckets in a string table */

typedef UINT HASHBUCKETCOUNT;
DECLARE_STANDARD_TYPES(HASHBUCKETCOUNT);

/* string table hash function */

typedef HASHBUCKETCOUNT (*STRINGTABLEHASHFUNC)(LPCTSTR, HASHBUCKETCOUNT);

/* new string table */

typedef struct _newstringtable
{
   HASHBUCKETCOUNT hbc;
}
NEWSTRINGTABLE;
DECLARE_STANDARD_TYPES(NEWSTRINGTABLE);


/* Prototypes
 *************/

/* string.c */

extern BOOL CreateStringTable(PCNEWSTRINGTABLE, PHSTRINGTABLE);
extern void DestroyStringTable(HSTRINGTABLE);
extern BOOL AddString(LPCTSTR pcsz, HSTRINGTABLE hst, STRINGTABLEHASHFUNC pfnHashFunc, PHSTRING phs);
extern void DeleteString(HSTRING);
extern void LockString(HSTRING);
extern COMPARISONRESULT CompareStringsI(HSTRING, HSTRING);
extern LPCTSTR GetString(HSTRING);

#if defined(DEBUG) || defined (VSTF)

extern BOOL IsValidHSTRING(HSTRING);
extern BOOL IsValidHSTRINGTABLE(HSTRINGTABLE);

#endif

#ifdef DEBUG

extern ULONG GetStringCount(HSTRINGTABLE);

#endif

