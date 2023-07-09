#ifndef __ASERTDBG_H__
#define __ASERTDBG_H__

/****************************************************************************

    MACROS:

****************************************************************************/
// Dbg have to be used for every assertion during Debugging time.
// If false Dbg Opens a fatal error message Box and Stops program



// Standard function to prompt an Assertion False
void ShowAssert(LPSTR condition, UINT line, LPSTR file);


// First, a sanity check
#ifdef Dbg
#undef Dbg
#endif

#if DBG
#ifdef WIN32
#ifdef MIPS_C
#define Dbg(condition) \
{ \
    if (!(condition)) { \
    static char szFileName[] = __FILE__;\
    static char szExp[] = "condition"; \
    ShowAssert(szExp, __LINE__, szFileName); \
    }\
}
#else // ! MIPS_C
#define Dbg(condition)          \
    {                   \
        if (!(condition))           \
        {                   \
        static char szFileName[] = __FILE__;     \
        static char szExp[] = #condition;    \
        ShowAssert(szExp, __LINE__, szFileName); \
        }                   \
    }
#endif
#else // !WIN32
#define Dbg(condition)  \
    {                               \
        if (!(condition))    \
        {                    \
            static char _based(_segname("SZASSERT")) szFileName[] = __FILE__; \
            static char _based(_segname("SZASSERT")) szExp[] = #condition;  \
            ShowAssert(szExp,__LINE__,szFileName);  \
        }                           \
    }
#endif
#else // !DBG
#pragma warning(disable: 4553)      // disable warnings for pure expressions
#pragma warning(disable: 4552)      // disable level 4 warnings
#define Dbg(condition) condition
#endif

#ifdef NTBUG
#define DbgX(condition) Dbg(condition)
#else
#pragma warning(disable: 4553)      // disable warnings for pure expressions
#pragma warning(disable: 4552)      // disable level 4 warnings
#define DbgX(condition) condition
#endif

#ifdef ALIGN
#define AssertAligned(address)  Assert((((long)address) & 3) == 0)
#else
#define AssertAligned(address)
#endif


// Assert are assertions that will stay in final Release.
// If false Assert Opens a fatal error message Box and Stops program
#ifdef WIN32
#ifdef MIPS_C
#define RAssert(condition) \
{ \
    if (!(condition)) { \
        static char szFileName[] = __FILE__;\
        static char szExp[[] = "condition"; \
        ShowAssert(szExp, __LINE__, szFileName); \
    } \
}
#else  // MIPS_C
#define RAssert(condition)   \
    {                                   \
        if (!(condition))       \
        {                       \
            static char szFileName[] = __FILE__; \
            static char szExp[] = #condition;   \
            ShowAssert(szExp,__LINE__,szFileName);  \
        }   \
    }
#endif
#else  // WIN32
#define RAssert(condition)   \
    {                                   \
        if (!(condition))       \
        {                       \
            static char _based(_segname("SZASSERT")) szFileName[] = __FILE__; \
            static char _based(_segname("SZASSERT")) szExp[] = #condition;  \
            ShowAssert(szExp,__LINE__,szFileName);  \
        }   \
    }
#endif

#if DBG
#define DAssert(cnd) RAssert(cnd)
#define Assert(cnd)  RAssert(cnd)
#else
#define DAssert(cnd)
#define Assert(cnd)  RAssert(cnd)
#endif



#endif
