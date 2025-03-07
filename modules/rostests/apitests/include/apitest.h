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

#define ok_hr(status, expected)                 ok_hex(status, expected)
#define ok_hr_(file, line, status, expected)    ok_hex_(file, line, status, expected)

#define ok_eq_print(value, expected, spec)  ok((value) == (expected), #value " = " spec ", expected " spec "\n", value, expected)
#define ok_eq_print_(file, line, value, expected, spec)  ok_(file,line)((value) == (expected), #value " = " spec ", expected " spec "\n", value, expected)
#define ok_eq_pointer(value, expected)      ok_eq_print(value, expected, "%p")
#define ok_eq_int(value, expected)          ok_eq_print(value, expected, "%d")
#define ok_eq_uint(value, expected)         ok_eq_print(value, expected, "%u")
#define ok_eq_long(value, expected)         ok_eq_print(value, expected, "%ld")
#define ok_eq_ulong(value, expected)        ok_eq_print(value, expected, "%lu")
#define ok_eq_longlong(value, expected)     ok_eq_print(value, expected, "%I64d")
#define ok_eq_ulonglong(value, expected)    ok_eq_print(value, expected, "%I64u")
#define ok_eq_char(value, expected)         ok_eq_print(value, expected, "%c")
#define ok_eq_wchar(value, expected)        ok_eq_print(value, expected, "%C")
#ifndef _WIN64
#define ok_eq_size(value, expected)         ok_eq_print(value, (SIZE_T)(expected), "%lu")
#define ok_eq_longptr(value, expected)      ok_eq_print(value, (LONG_PTR)(expected), "%ld")
#define ok_eq_ulongptr(value, expected)     ok_eq_print(value, (ULONG_PTR)(expected), "%lu")
#elif defined _WIN64
#define ok_eq_size(value, expected)         ok_eq_print(value, (SIZE_T)(expected), "%I64u")
#define ok_eq_longptr(value, expected)      ok_eq_print(value, (LONG_PTR)(expected), "%I64d")
#define ok_eq_ulongptr(value, expected)     ok_eq_print(value, (ULONG_PTR)(expected), "%I64u")
#endif /* defined _WIN64 */
#define ok_eq_hex(value, expected)          ok_eq_print(value, expected, "0x%08lx")
#define ok_bool_true(value, desc)           ok((value) == TRUE, desc " FALSE, expected TRUE\n")
#define ok_bool_false(value, desc)          ok((value) == FALSE, desc " TRUE, expected FALSE\n")
#define ok_eq_bool(value, expected)         ok((value) == (expected), #value " = %s, expected %s\n", \
                                                 (value) ? "TRUE" : "FALSE",                             \
                                                 (expected) ? "TRUE" : "FALSE")
#define ok_eq_str(value, expected)          ok(!strcmp(value, expected), #value " = \"%s\", expected \"%s\"\n", value, expected)
#define ok_eq_wstr(value, expected)         ok(!wcscmp(value, expected), #value " = \"%ls\", expected \"%ls\"\n", value, expected)
#define ok_eq_tag(value, expected)          ok_eq_print(value, expected, "0x%08lx")

#define ok_eq_hex_(file, line, value, expected) ok_eq_print_(file, line, value, expected, "0x%08lx")
#define ok_eq_hex64_(file, line, value, expected) ok_eq_print_(file, line, value, expected, "%I64x")
#define ok_eq_hex64(value, expected)        ok_eq_print(value, expected, "%I64x")
#define ok_eq_xmm(value, expected)          ok((value).Low == (expected).Low, #value " = %I64x'%08I64x, expected %I64x'%08I64x\n", (value).Low, (value).High, (expected).Low, (expected).High)

#endif /* _APITEST_H */
