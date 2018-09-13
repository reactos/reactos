//*******************************************************************************************
//
// Filename : Dpda.h
//	
//				Definitions of DPA routines
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#ifndef _DPDA_H_
#define _DPDA_H_


#ifdef __cplusplus
extern "C" {
#endif

// Dynamic pointer array	 
typedef struct _DPA * HDPA;	 

HDPA   DPA_Create(int cItemGrow);	 
HDPA   DPA_CreateEx(int cpGrow, HANDLE hheap);	 
BOOL   DPA_Destroy(HDPA hdpa);	 
HDPA   DPA_Clone(HDPA hdpa, HDPA hdpaNew);	 
void *  DPA_GetPtr(HDPA hdpa, int i);	 
int    DPA_GetPtrIndex(HDPA hdpa, LPVOID p);	 
BOOL   DPA_Grow(HDPA pdpa, int cp);	 
BOOL   DPA_SetPtr(HDPA hdpa, int i, LPVOID p);	 
int    DPA_InsertPtr(HDPA hdpa, int i, LPVOID p);	 
void * DPA_DeletePtr(HDPA hdpa, int i);	 
BOOL   DPA_DeleteAllPtrs(HDPA hdpa);	 
#define       DPA_GetPtrCount(hdpa)   (*(int *)(hdpa))	 
#define       DPA_GetPtrPtr(hdpa)     (*((LPVOID * *)((BYTE *)(hdpa) + sizeof(int))))	 
#define       DPA_FastGetPtr(hdpa, i) (DPA_GetPtrPtr(hdpa)[i])	 

typedef int (CALLBACK *PFNDPACOMPARE)(LPVOID p1, LPVOID p2, LPARAM lParam);	 

BOOL   DPA_Sort(HDPA hdpa, PFNDPACOMPARE pfnCompare, LPARAM lParam);	 

typedef struct _DSA * HDSA;                                            	

// Search array.  If DPAS_SORTED, then array is assumed to be sorted	 
// according to pfnCompare, and binary search algorithm is used.	 
// Otherwise, linear search is used.	 
//	 
// Searching starts at iStart (-1 to start search at beginning).	 
//	 
// DPAS_INSERTBEFORE/AFTER govern what happens if an exact match is not	 
// found.  If neither are specified, this function returns -1 if no exact	 
// match is found.  Otherwise, the index of the item before or after the	 
// closest (including exact) match is returned.	 
//	 
// Search option flags	 
//	 
#define DPAS_SORTED             0x0001	 
#define DPAS_INSERTBEFORE       0x0002	 
#define DPAS_INSERTAFTER        0x0004	 

int DPA_Search(HDPA hdpa, LPVOID pFind, int iStart,	 
                      PFNDPACOMPARE pfnCompare,	 
                      LPARAM lParam, UINT options);	 
#ifdef __cplusplus
}
#endif

#endif // _DPDA_H_

