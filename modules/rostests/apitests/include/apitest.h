#ifndef _APITEST_H
#define _APITEST_H

// #define __ROS_LONG64__

/* The user must #define STANDALONE if it uses this header in testlist.c */
#define WIN32_NO_STATUS
#include <wine/test.h>
#undef WIN32_NO_STATUS

/* See kmtests/include/kmt_test.h */
#define InvalidPointer ((PVOID)0x5555555555555555ULL)
// #define InvalidPointer ((PVOID)0x0123456789ABCDEFULL)

#ifdef __USE_PSEH2__
#include <pseh/pseh2.h>

#define StartSeh()                                  \
{                                                   \
    NTSTATUS ExceptionStatus = STATUS_SUCCESS;      \
    _SEH2_TRY                                       \
    {

#define EndSeh(ExpectedStatus)                      \
    }                                               \
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)         \
    {                                               \
        ExceptionStatus = _SEH2_GetExceptionCode(); \
    }                                               \
    _SEH2_END;                                      \
    ok(ExceptionStatus == (ExpectedStatus),         \
       "Exception 0x%08lx, expected 0x%08lx\n",     \
       ExceptionStatus, (ExpectedStatus));          \
}
#else
#define StartSeh()                                  \
{                                                   \
    NTSTATUS ExceptionStatus = STATUS_SUCCESS;      \
    __try                                           \
    {

#define EndSeh(ExpectedStatus)                      \
    }                                               \
    __except(EXCEPTION_EXECUTE_HANDLER)             \
    {                                               \
        ExceptionStatus = GetExceptionCode();       \
    }                                               \
    ok(ExceptionStatus == (ExpectedStatus),         \
       "Exception 0x%08lx, expected 0x%08lx\n",     \
       ExceptionStatus, (ExpectedStatus));          \
}
#endif

#define ok_hr(status, expected)                 ok_hex(status, expected)
#define ok_hr_(file, line, status, expected)    ok_hex_(file, line, status, expected)

#endif /* _APITEST_H */
