/////////////////////////////////////////////////////////////////////////////
// Diagnostic Trace
//
#ifndef __TRACE_H__
#define __TRACE_H__

#ifdef _DEBUG

#ifdef _X86_
#define BreakPoint()        _asm { int 3h }
#else
#define BreakPoint()        _DebugBreak()
#endif

#ifndef ASSERT
#define ASSERT(exp)                                 \
{                                                   \
    if (!(exp)) {                                   \
        Assert(#exp, __FILE__, __LINE__, NULL);     \
        BreakPoint();                               \
    }                                               \
}                                                   \

#define ASSERTMSG(exp, msg)                         \
{                                                   \
    if (!(exp)) {                                   \
        Assert(#exp, __FILE__, __LINE__, msg);      \
        BreakPoint();                               \
    }                                               \
}
#endif

//=============================================================================
//  MACRO: TRACE()
//=============================================================================

#define TRACE  Trace


#else   // _DEBUG

//=============================================================================
//  Define away MACRO's ASSERT() and TRACE() in non debug builds
//=============================================================================

#ifndef ASSERT
#define ASSERT(exp)
#define ASSERTMSG(exp, msg)
#endif

#define TRACE 0 ? (void)0 : Trace

#endif // !_DEBUG


void Assert(void* assert, TCHAR* file, int line, void* msg);
void Trace(TCHAR* lpszFormat, ...);


#endif // __TRACE_H__
/////////////////////////////////////////////////////////////////////////////
