
#ifndef _SEARCH_H_
#define _SEARCH_H_

//char    *key
//void    *data

//enum { FIND, ENTER } ACTION;
//enum { preorder, postorder, endorder, leaf } VISIT;

#include <crtdll/stddef.h>
#include <crtdll/sys/types.h> 


//The Single UNIX ® Specification, Version 2 Copyright © 1997 The Open Group 

//int    hcreate(size_t);
//void   hdestroy(void);
//ENTRY *hsearch(ENTRY, ACTION);
//void   insque(void *, void *);
void  *_lfind(const void *, const void *, size_t *,
               size_t, int (*)(const void *, const void *));
void  *_lsearch(const void *, void *, size_t *,
               size_t, int (*)(const void *, const void *));
//void   remque(void *);
//void  *tdelete(const void *, void **,
//               int(*)(const void *, const void *));
//void  *tfind(const void *, void *const *,
//               int(*)(const void *, const void *));
//void  *tsearch(const void *, void **,
//               int(*)(const void *, const void *));
//void   twalk(const void *,
//             void (*)(const void *, VISIT, int ));

#endif
