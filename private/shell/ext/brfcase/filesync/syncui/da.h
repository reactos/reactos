//
// da.h:  Dynamic array functions taken from commctrl.
//
#if 0
#ifndef _DA_H_
#define _DA_H_

//====== Dynamic Array routines ==========================================      /* ;Internal */
// Dynamic structure array                                                      /* ;Internal */
typedef struct _DSA * HDSA;                                             /* ;Internal */
                                                                                /* ;Internal */
HDSA   PUBLIC DSA_Create(int cbItem, int cItemGrow);                            /* ;Internal */
BOOL   PUBLIC DSA_Destroy(HDSA hdsa);                                           /* ;Internal */
BOOL   PUBLIC DSA_GetItem(HDSA hdsa, int i, void * pitem);                      /* ;Internal */
LPVOID PUBLIC DSA_GetItemPtr(HDSA hdsa, int i);                                 /* ;Internal */
BOOL   PUBLIC DSA_SetItem(HDSA hdsa, int i, void * pitem);                      /* ;Internal */
int    PUBLIC DSA_InsertItem(HDSA hdsa, int i, void * pitem);                   /* ;Internal */
BOOL   PUBLIC DSA_DeleteItem(HDSA hdsa, int i);                                 /* ;Internal */
BOOL   PUBLIC DSA_DeleteAllItems(HDSA hdsa);                                    /* ;Internal */
#define       DSA_GetItemCount(hdsa) (*(int *)(hdsa))                           /* ;Internal */
                                                                                /* ;Internal */
// Dynamic pointer array                                                        /* ;Internal */
typedef struct _DPA * HDPA;                                             /* ;Internal */
                                                                                /* ;Internal */
HDPA   PUBLIC DPA_Create(int cItemGrow);                                        /* ;Internal */
HDPA   PUBLIC DPA_CreateEx(int cpGrow, HANDLE hheap);                           /* ;Internal */
BOOL   PUBLIC DPA_Destroy(HDPA hdpa);                                           /* ;Internal */
HDPA   PUBLIC DPA_Clone(HDPA hdpa, HDPA hdpaNew);                               /* ;Internal */
LPVOID PUBLIC DPA_GetPtr(HDPA hdpa, int i);                                     /* ;Internal */
int    PUBLIC DPA_GetPtrIndex(HDPA hdpa, LPVOID p);                             /* ;Internal */
BOOL   PUBLIC DPA_Grow(HDPA pdpa, int cp);                                      /* ;Internal */
BOOL   PUBLIC DPA_SetPtr(HDPA hdpa, int i, LPVOID p);                           /* ;Internal */
int    PUBLIC DPA_InsertPtr(HDPA hdpa, int i, LPVOID p);                        /* ;Internal */
LPVOID PUBLIC DPA_DeletePtr(HDPA hdpa, int i);                                  /* ;Internal */
BOOL   PUBLIC DPA_DeleteAllPtrs(HDPA hdpa);                                     /* ;Internal */
#define       DPA_GetPtrCount(hdpa)   (*(int *)(hdpa))                          /* ;Internal */
#define       DPA_GetPtrPtr(hdpa)     (*((LPVOID * *)((BYTE *)(hdpa) + sizeof(int))))   /* ;Internal */
#define       DPA_FastGetPtr(hdpa, i) (DPA_GetPtrPtr(hdpa)[i])                  /* ;Internal */

typedef int (CALLBACK *PFNDPACOMPARE)(LPVOID p1, LPVOID p2, LPARAM lParam);     /* ;Internal */
                                                                                /* ;Internal */
BOOL   PUBLIC DPA_Sort(HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam);     /* ;Internal */
                                                                                /* ;Internal */
// Search array.  If DPAS_SORTED, then array is assumed to be sorted            /* ;Internal */
// according to pfnCompare, and binary search algorithm is used.                /* ;Internal */
// Otherwise, linear search is used.                                            /* ;Internal */
//                                                                              /* ;Internal */
// Searching starts at iStart (-1 to start search at beginning).                /* ;Internal */
//                                                                              /* ;Internal */
// DPAS_INSERTBEFORE/AFTER govern what happens if an exact match is not         /* ;Internal */
// found.  If neither are specified, this function returns -1 if no exact       /* ;Internal */
// match is found.  Otherwise, the index of the item before or after the        /* ;Internal */
// closest (including exact) match is returned.                                 /* ;Internal */
//                                                                              /* ;Internal */
// Search option flags                                                          /* ;Internal */
//                                                                              /* ;Internal */
#define DPAS_SORTED             0x0001                                          /* ;Internal */
#define DPAS_INSERTBEFORE       0x0002                                          /* ;Internal */
#define DPAS_INSERTAFTER        0x0004                                          /* ;Internal */
                                                                                /* ;Internal */
int PUBLIC DPA_Search(HDPA hdpa, LPVOID pFind, int iStart,                      /* ;Internal */
                      PFNDPACOMPARE pfnCompare,                                 /* ;Internal */
                      LPARAM lParam, UINT options);                             /* ;Internal */

                                                                                /* ;Internal */
//======================================================================        /* ;Internal */
// String management helper routines                                            /* ;Internal */
                                                                                /* ;Internal */
int  PUBLIC Str_GetPtr(LPCTSTR psz, LPTSTR pszBuf, int cchBuf);                 /* ;Internal */
BOOL PUBLIC Str_SetPtr(LPTSTR * ppsz, LPCTSTR psz);                             /* ;Internal */

#endif // _DA_H_
#endif
