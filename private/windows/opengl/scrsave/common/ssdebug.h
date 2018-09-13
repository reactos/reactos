/******************************Module*Header*******************************\
* Module Name: ssdebug.h
*
* Debugging stuff
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __ssdebug_h__
#define __ssdebug_h__

ULONG DbgPrint(PCH Format, ...);

#define SS_LEVEL_ERROR 1L
#define SS_LEVEL_INFO  2L
#define SS_LEVEL_ENTRY 8L

#if DBG

extern long ssDebugMsg;
extern long ssDebugLevel;

#define SS_DBGPRINT( str )          DbgPrint("SS: " str )
#define SS_DBGPRINT1( str, a )      DbgPrint("SS: " str, a )
#define SS_DBGPRINT2( str, a, b )   DbgPrint("SS: " str, a, b )

#define SS_WARNING(str)             DbgPrint("SS: " str )
#define SS_WARNING1(str,a)          DbgPrint("SS: " str,a)
#define SS_WARNING2(str,a,b)        DbgPrint("SS: " str,a,b)

#define SS_RIP(str)                 {SS_WARNING(str); DebugBreak();}
#define SS_RIP1(str,a)              {SS_WARNING1(str,a); DebugBreak();}
#define SS_RIP2(str,a,b)            {SS_WARNING2(str,a,b); DebugBreak();}

#define SS_ASSERT(expr,str)            if(!(expr)) SS_RIP(str)
#define SS_ASSERT1(expr,str,a)         if(!(expr)) SS_RIP1(str,a)
#define SS_ASSERT2(expr,str,a,b)       if(!(expr)) SS_RIP2(str,a,b)

#define SS_ALLOC_FAILURE(str) \
    DbgPrint( "%s : Memory allocation failure\n", str );

#define SS_DBGMSG( str )         if( ssDebugMsg ) SS_DBGPRINT( str )
#define SS_DBGMSG1( str, a )     if( ssDebugMsg ) SS_DBGPRINT1( str, a )
#define SS_DBGMSG2( str, a, b )  if( ssDebugMsg ) SS_DBGPRINT2( str, a, b )

//
// Use SS_DBGLEVEL for general purpose debug messages gated by an
// arbitrary warning level.
//
#define SS_DBGLEVEL(n,str)         if (ssDebugLevel >= (n)) SS_DBGPRINT(str)
#define SS_DBGLEVEL1(n,str,a)      if (ssDebugLevel >= (n)) SS_DBGPRINT1(str,a)
#define SS_DBGLEVEL2(n,str,a,b)    if (ssDebugLevel >= (n)) SS_DBGPRINT2(str,a,b)    

#define SS_ERROR(str)              SS_DBGLEVEL( SS_LEVEL_ERROR, str ) 
#define SS_ERROR1(str,a)           SS_DBGLEVEL1( SS_LEVEL_ERROR, str, a ) 
#define SS_ERROR2(str,a)           SS_DBGLEVEL2( SS_LEVEL_ERROR, str, a, b ) 

#define SS_DBGINFO(str)            SS_DBGLEVEL( SS_LEVEL_INFO, str ) 
#define SS_DBGINFO1(str,a)         SS_DBGLEVEL1( SS_LEVEL_INFO, str, a ) 
#define SS_DBGINFO2(str,a,b)       SS_DBGLEVEL2( SS_LEVEL_INFO, str, a, b ) 

#else

#define SS_DBGPRINT( str )
#define SS_DBGPRINT1( str, a )
#define SS_DBGPRINT2( str, a, b )

#define SS_WARNING(str)
#define SS_WARNING1(str,a)
#define SS_WARNING2(str,a,b)

#define SS_RIP(str)
#define SS_RIP1(str,a)
#define SS_RIP2(str,a,b)

#define SS_ASSERT(expr,str)         assert( expr )
#define SS_ASSERT1(expr,str,a)      assert( expr )
#define SS_ASSERT2(expr,str,a,b)    assert( expr )

#define SS_ALLOC_FAILURE(str)

#define SS_DBGMSG( str )
#define SS_DBGMSG1( str, a )
#define SS_DBGMSG2( str, a, b )

#define SS_DBGLEVEL(n,str)
#define SS_DBGLEVEL1(n,str,a)
#define SS_DBGLEVEL2(n,str,a,b)

#define SS_ERROR(str)
#define SS_ERROR1(str,a)
#define SS_ERROR2(str,a,b)

#define SS_DBGINFO(str)
#define SS_DBGINFO1(str,a)
#define SS_DBGINFO2(str,a,b)

#endif // DBG

#endif // __ssdebug_h__
