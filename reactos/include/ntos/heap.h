/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/heap.h
 * PURPOSE:      Heap declarations used by all the parts of the 
 *               system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */

#ifndef __INCLUDE_HEAP_H
#define __INCLUDE_HEAP_H

/* HeapAlloc, HeapReAlloc */
#define HEAP_NO_VALLOC (64)

#ifndef __USE_W32API

#define HEAP_GENERATE_EXCEPTIONS	(4)
#define HEAP_NO_SERIALIZE	(1)
#define HEAP_ZERO_MEMORY	(8)
#define HEAP_REALLOC_IN_PLACE_ONLY	(16)
#define HEAP_GROWABLE (32)

#endif /* !__USE_W32API */

#endif /* __INCLUDE_HEAP_H */
