/////////////////////////////////////////////////////////////////////////////
//
#ifndef __TRACE_H__
#define __TRACE_H__

#ifdef _DEBUG

//=============================================================================
//  BreakPoint() macro.
//=============================================================================

#ifdef _X86_
#define BreakPoint()        _asm { int 3h }
#else
#define BreakPoint()        _DebugBreak()
#endif

//=============================================================================
//  MACRO: ASSERT()
//=============================================================================

#ifndef ASSERT
#define ASSERT(exp)                                 \
{                                                   \
    if ( !(exp) )                                   \
    {                                               \
        Assert(#exp, __FILE__, __LINE__, NULL);   \
        BreakPoint();                               \
    }                                               \
}                                                   \

#define ASSERTMSG(exp, msg)                         \
{                                                   \
    if ( !(exp) )                                   \
    {                                               \
        Assert(#exp, __FILE__, __LINE__, msg);    \
        BreakPoint();                               \
    }                                               \
}
#endif

//=============================================================================
//  MACRO: TRACE()
//=============================================================================

void Assert(void* assert, TCHAR* file, int line, void* msg);
void Trace(TCHAR* lpszFormat, ...);
void Trace1(int code, TCHAR* lpszFormat, ...);

#define TRACE  Trace
#define TRACE0 Trace

#else   // _DEBUG

#ifndef ASSERT
#define ASSERT(exp)
#define ASSERTMSG(exp, msg)
#endif

//#define TRACE0 TRACE
//#define TRACE1 TRACE

void Assert(void* assert, TCHAR* file, int line, void* msg);
void Trace(TCHAR* lpszFormat, ...);

#define TRACE 0 ? (void)0 : Trace


#endif // !_DEBUG

#endif // __TRACE_H__
/////////////////////////////////////////////////////////////////////////////
