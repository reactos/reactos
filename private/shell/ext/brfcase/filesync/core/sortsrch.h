/*
 * sortsrch.c - Generic array sorting and searching description.
 */


/* Types
 ********/

/* array element comparison callback function */

typedef COMPARISONRESULT (*COMPARESORTEDELEMSPROC)(PCVOID, PCVOID);


/* Prototypes
 *************/

/* sortsrch.c */

extern void HeapSort(PVOID, LONG, size_t, COMPARESORTEDELEMSPROC, PVOID);
extern BOOL BinarySearch(PVOID, LONG, size_t, COMPARESORTEDELEMSPROC, PCVOID, PLONG);

