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
#include <msvcrt/internal/file.h>

#if 0
#ifdef NDEBUG
#undef NDEBUG
#endif
#endif

#ifdef DBG
#define DPRINT1(args...) do { DbgPrint("(MSVCRT:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT1 do { DbgPrint("MSVCRT:%s:%d\n",__FILE__,__LINE__); } while(0);
#else
#ifdef __GNUC__
#define DPRINT1(args...)
#else
#define DPRINT DbgPrint
#endif
#define CHECKPOINT1
#endif

#if !defined(NDEBUG) && defined(DBG) 
#define DPRINT(args...) do { DbgPrint("(MSVCRT:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("MSVCRT:%s:%d\n",__FILE__,__LINE__); } while(0);
#else
#ifdef __GNUC__
#define DPRINT(args...)
#else
#define DPRINT DbgPrint
#endif
#define CHECKPOINT
#endif /* NDEBUG */

//ULONG CDECL DbgPrint(PCH Format, ...);
//ULONG DbgPrint(PCH Format,...);
//unsigned long DbgPrint(const char* Format, ...);



//#define TRACE 0 ? (void)0 : Trace

//void Trace(TCHAR* lpszFormat, ...);



#endif /* __MSVCRT_DEBUG */
