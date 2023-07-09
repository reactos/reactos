/*
 * ptrarray.h - Pointer array ADT description.
 */


/* Constants
 ************/

/*
 * ARRAYINDEX_MAX is set such that (ARRAYINDEX_MAX + 1) does not overflow an
 * ARRAYINDEX.  This guarantee allows GetPtrCount() to return a count of
 * pointers as an ARRAYINDEX.
 */

#define ARRAYINDEX_MAX           (LONG_MAX - 1)


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HPTRARRAY);
DECLARE_STANDARD_TYPES(HPTRARRAY);

/* array index */

typedef LONG ARRAYINDEX;
DECLARE_STANDARD_TYPES(ARRAYINDEX);

/*
 * pointer comparison callback function
 *
 * In sorting functions, both pointers are pointer array elements.  In
 * searching functions, the first pointer is reference data and the second
 * pointer is a pointer array element.
 */

typedef COMPARISONRESULT (*COMPARESORTEDPTRSPROC)(PCVOID, PCVOID);

/*
 * pointer comparison callback function
 *
 * In searching functions, the first pointer is reference data and the second
 * pointer is a pointer array element.
 */

typedef BOOL (*COMPAREUNSORTEDPTRSPROC)(PCVOID, PCVOID);

/* new pointer array flags */

typedef enum _newptrarrayflags
{
   /* Insert elements in sorted order. */

   NPA_FL_SORTED_ADD       = 0x0001,

   /* flag combinations */

   ALL_NPA_FLAGS           = NPA_FL_SORTED_ADD
}
NEWPTRARRAYFLAGS;

/* new pointer array description */

typedef struct _newptrarray
{
   DWORD dwFlags;

   ARRAYINDEX aicInitialPtrs;

   ARRAYINDEX aicAllocGranularity;
}
NEWPTRARRAY;
DECLARE_STANDARD_TYPES(NEWPTRARRAY);


/* Prototypes
 *************/

/* ptrarray.c */

extern BOOL CreatePtrArray(PCNEWPTRARRAY, PHPTRARRAY);
extern void DestroyPtrArray(HPTRARRAY);
extern BOOL InsertPtr(HPTRARRAY, COMPARESORTEDPTRSPROC, ARRAYINDEX, PCVOID);
extern BOOL AddPtr(HPTRARRAY, COMPARESORTEDPTRSPROC, PCVOID, PARRAYINDEX);
extern void DeletePtr(HPTRARRAY, ARRAYINDEX);
extern void DeleteAllPtrs(HPTRARRAY);
extern ARRAYINDEX GetPtrCount(HPTRARRAY);
extern PVOID GetPtr(HPTRARRAY, ARRAYINDEX);
extern void SortPtrArray(HPTRARRAY, COMPARESORTEDPTRSPROC);
extern BOOL SearchSortedArray(HPTRARRAY, COMPARESORTEDPTRSPROC, PCVOID, PARRAYINDEX);
extern BOOL LinearSearchArray(HPTRARRAY, COMPAREUNSORTEDPTRSPROC, PCVOID, PARRAYINDEX);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidHPTRARRAY(HPTRARRAY);

#endif

#ifdef VSTF

extern BOOL IsValidHGLOBAL(HGLOBAL);

#endif

