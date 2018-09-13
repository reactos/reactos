/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef POINTER_H
#define POINTER_H

void AddPointerToCache(void * p);
void RemovePointerFromCache(void * p);
void SafeRemovePointerFromCache(void * p);

#if DBG == 1
DLLEXPORT 
#endif
bool IsCachedPointer(INT_PTR p);

EXTERN_C 
#if DBG == 1
DLLEXPORT 
#endif
void ClearPointerCache();

EXTERN_C
#if DBG == 1
DLLEXPORT 
#endif
void FreeObjects();
#if DBG == 1
DLLEXPORT 
#endif
void FreeObjects(TLSDATA *);

#ifdef SPECIAL_OBJECT_ALLOCATION

#define MARK_TO_POINTER(p, t) ((t *)((INT_PTR)p & ~1)) 
#define ISMARKED(p) ((((INT_PTR)p) & 1) != 0)
#define POINTER_TO_MARK(p, t) ((t *)(((INT_PTR)p) | 1))

#else

#define MARK_TO_POINTER(p, t) ((t *)p) 
#define ISMARKED(p) (false)
#define POINTER_TO_MARK(p, t) ((t *)p)

#endif

#endif POINTER_H
