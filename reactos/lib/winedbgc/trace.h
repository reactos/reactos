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
void Trace(const TCHAR* lpszFormat, ...);
void Trace1(int code, TCHAR* lpszFormat, ...);

#else   // _DEBUG

#ifndef ASSERT
#define ASSERT(exp)
#define ASSERTMSG(exp, msg)
#endif

void Assert(void* assert, TCHAR* file, int line, void* msg);
void Trace(const TCHAR* lpszFormat, ...);

#endif // !_DEBUG

#endif // __TRACE_H__
/////////////////////////////////////////////////////////////////////////////
