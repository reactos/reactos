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
#include <windows.h>


#define MK_STR(s) #s

#ifdef _UNICODE
   #define sT "S"
#else
   #define sT "s"
#endif

unsigned long DbgPrint(const char *Format,...);

#ifdef __GNUC__
	#define TRACE(...)
#endif

#ifdef DBG
   #ifdef __GNUC__
      #define DPRINT1(args...) do { DbgPrint("(MSVCRT:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
   #else
      #define DPRINT1 DbgPrint
   #endif
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
   #ifdef __GNUC__
       #define DPRINT(args...) do { DbgPrint("(MSVCRT:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
   #endif
   #define CHECKPOINT do { DbgPrint("MSVCRT:%s:%d\n",__FILE__,__LINE__); } while(0);
#else
   #ifdef __GNUC__
      #define DPRINT(args...)
   #else
      #define DPRINT DbgPrint
   #endif
   #define CHECKPOINT
#endif /* NDEBUG */


#if 0

   #define TRACE_RETURN(format_str, ret_type) \
   ret_type __return_value__; \
   static char* __return_format_str__ = "%s ret: "format_str"\n"

   #define FUNCTION(func) \
   static char* __func_name__ = #func

   #define TRACE(a,b...) DPRINT1(a"\n", b)

   #define RETURN(a) \
   do{ __return_value__ = (a); DPRINT1(__return_format_str__ ,__func_name__,__return_value__); return __return_value__ ; }while(0)

#endif


/* ULONG CDECL DbgPrint(PCH Format, ...); */
ULONG DbgPrint(PCCH Format,...);
/* unsigned long DbgPrint(const char* Format, ...); */



/* #define TRACE 0 ? (void)0 : Trace */

/* void Trace(TCHAR* lpszFormat, ...); */



#endif /* __MSVCRT_DEBUG */
