/*
 * Debug API
 *
 * Copyright 2001 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_CRTDBG_H_
#define __WINE_CRTDBG_H_
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

/* The debug API is not implemented in Winelib.
 * Redirect everything to the regular APIs.
 */

#define _CRT_WARN                       0
#define _CRT_ERROR                      1
#define _CRT_ASSERT                     2
#define _CRT_ERRCNT                     3

#define _FREE_BLOCK                     0
#define _NORMAL_BLOCK                   1
#define _CRT_BLOCK                      2
#define _IGNORE_BLOCK                   3
#define _CLIENT_BLOCK                   4
#define _MAX_BLOCKS                     5


typedef struct _CrtMemState
{
    struct _CrtMemBlockHeader* pBlockHeader;
    unsigned long lCounts[_MAX_BLOCKS];
    unsigned long lSizes[_MAX_BLOCKS];
    unsigned long lHighWaterCount;
    unsigned long lTotalCount;
} _CrtMemState;


#ifndef _DEBUG

#define _ASSERT(expr)                   ((void)0)
#define _ASSERTE(expr)                  ((void)0)
#define _CrtDbgBreak()                  ((void)0)

#else /* _DEBUG */

#include <assert.h>
#define _ASSERT(expr)                   assert(expr)
#define _ASSERTE(expr)                  assert(expr)
#if defined(__GNUC__) && defined(__i386__)
#define _CrtDbgBreak()                  __asm__ ("\tint $0x3\n")
#else
#define _CrtDbgBreak()                  ((void)0)
#endif

#endif /* _DEBUG */

#define _CrtCheckMemory()               ((int)1)
#define _CrtDbgReport(...)              ((int)0)
#define _CrtDoForAllClientObjects(f,c)  ((void)0)
#define _CrtDumpMemoryLeaks()           ((int)0)
#define _CrtIsMemoryBlock(p,s,r,f,l)    ((int)1)
#define _CrtIsValidHeapPointer(p)       ((int)1)
#define _CrtIsValidPointer(p,s,a)       ((int)1)
#define _CrtMemCheckpoint(s)            ((void)0)
#define _CrtMemDifference(s1,s2,s3)     ((int)0)
#define _CrtMemDumpAllObjectsSince(s)   ((void)0)
#define _CrtMemDumpStatistics(s)        ((void)0)
#define _CrtSetAllocHook(f)             ((void)0)
#define _CrtSetBreakAlloc(a)            ((long)0)
#define _CrtSetDbgFlag(f)               ((int)0)
#define _CrtSetDumpClient(f)            ((void)0)
#define _CrtSetReportMode(t,m)          ((int)0)

#define _RPT0(t,m)
#define _RPT1(t,m,p1)
#define _RPT2(t,m,p1,p2)
#define _RPT3(t,m,p1,p2,p3)
#define _RPT4(t,m,p1,p2,p3,p4)
#define _RPTF0(t,m)
#define _RPTF1(t,m,p1)
#define _RPTF2(t,m,p1,p2)
#define _RPTF3(t,m,p1,p2,p3)
#define _RPTF4(t,m,p1,p2,p3,p4)


#define _malloc_dbg(s,t,f,l)            malloc(s)
#define _calloc_dbg(c,s,t,f,l)          calloc(c,s)
#define _expand_dbg(p,s,t,f,l)          _expand(p,s)
#define _free_dbg(p,t)                  free(p)
#define _realloc_dbg(p,s,t,f,l)         realloc(p,s)

#endif /* __WINE_CRTDBG_H */
