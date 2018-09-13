//-------------------------------------------------------------------------//
//
//  PTdebug.h
//
//  Common debugging helpers for Property Tree modules
//
//-------------------------------------------------------------------------//

#ifndef __PTDEBUG_H__
#define __PTDEBUG_H__


#ifdef _DEBUG
#   define _ENABLE_TRACING
#   define _ENABLE_ASSERT
#endif

//-------------------------------------------------------------------------//
//  TRACE()
#if defined(_ENABLE_TRACING) && !defined(TRACE)
    void _cdecl DebugTrace(LPCTSTR lpszFormat, ...);
#   ifndef TRACE
#       define TRACE            DebugTrace
#   endif
#else
    inline void _cdecl DebugTrace(LPCTSTR , ...){}
#   ifndef TRACE
#       define TRACE            1 ? (void)0 : DebugTrace
#   endif
#endif 
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  ASSERT(), VERIFY()
#if defined(_ENABLE_ASSERT) && !defined(ASSERT)
#   include <crtdbg.h>
#   define ASSERT( expr )  _ASSERTE( expr )
#   define VERIFY( expr )  ASSERT( expr )
#else
#   define ASSERT( expr )    
#   define VERIFY( expr )  ((void)(expr))
#endif
//-------------------------------------------------------------------------//

#endif