/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/msvcrt/msvcrtdbg.h
 * PURPOSE:         Useful debugging macros
 * PROGRAMMER:      
 * UPDATE HISTORY: 
 *                
 */

/*
 * NOTE: Define NDEBUG before including this header to disable debugging
 * macros
 */

#ifndef __MSVCRT_DEBUG
#define __MSVCRT_DEBUG

#include <roscfg.h>

#if 0
#ifdef NDEBUG
#undef NDEBUG
#endif
#endif

#ifdef DBG
#define DPRINT1(args...) do { DbgPrint("(MSVCRT:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT1 do { DbgPrint("MSVCRT:%s:%d\n",__FILE__,__LINE__); } while(0);
#else
#define DPRINT1(args...)
#define CHECKPOINT1
#endif

#if !defined(NDEBUG) && defined(DBG) 
#define DPRINT(args...) do { DbgPrint("(MSVCRT:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("MSVCRT:%s:%d\n",__FILE__,__LINE__); } while(0);
#else
#define DPRINT(args...)
#define CHECKPOINT
#endif /* NDEBUG */

#endif /* __MSVCRT_DEBUG */
