#ifndef _APITEST_H
#define _APITEST_H

// #define __ROS_LONG64__

#define WIN32_NO_STATUS

/* The user must #define STANDALONE if it uses this header in testlist.c */
#include <wine/test.h>
#include <pseh/pseh2.h>

/* See kmtests/include/kmt_test.h */
#define InvalidPointer ((PVOID)0x5555555555555555ULL)

#define StartSeh()                                  \
    ExceptionStatus = STATUS_SUCCESS;               \
    _SEH2_TRY                                       \
    {
#define EndSeh(ExpectedStatus)                      \
    }                                               \
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)         \
    {                                               \
        ExceptionStatus = _SEH2_GetExceptionCode(); \
    }                                               \
    _SEH2_END;                                      \
    ok(ExceptionStatus == ExpectedStatus, "Exception 0x%08lx, expected 0x%08lx\n", ExceptionStatus, ExpectedStatus)

#endif /* _APITEST_H */
