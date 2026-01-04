#ifndef _APITEST_H
#define _APITEST_H

/* The user must #define STANDALONE if it uses this header in testlist.c */
#define WIN32_NO_STATUS
#include <wine/test.h>
#undef WIN32_NO_STATUS

/* See kmtests/include/kmt_test.h */
#define InvalidPointer ((PVOID)0x5555555555555555ULL)
// #define InvalidPointer ((PVOID)0x0123456789ABCDEFULL)

/* Magic pointers come from KUSER_SHARED_DATA; needed to get true NT version on Windows 8+ */
#define KUSER_SHARED_DATA_UMPTR 0x7FFE0000
#define GetMajorNTVersion() (*(ULONG*)(KUSER_SHARED_DATA_UMPTR + 0x026C))
#define GetMinorNTVersion() (*(ULONG*)(KUSER_SHARED_DATA_UMPTR + 0x0270))
#define GetNTVersion() ((GetMajorNTVersion() << 8) | GetMinorNTVersion())
#define GENERATE_NTDDI(Major, Minor, ServicePack, Subversion) \
    (((Major) << 24) | ((Minor) << 16) | ((ServicePack) << 8) | (Subversion))
#define NTDDI_MIN 0UL
#define NTDDI_MAX 0xFFFFFFFFUL

static inline ULONG GetNTDDIVersion(VOID)
{
    ULONG NTBuildNo, NTMajor, NTMinor, ServicePack, Subversion;

    if (GetNTVersion() >= _WIN32_WINNT_WINBLUE)
    {
        NTMajor = GetMajorNTVersion();
        NTMinor = GetMinorNTVersion();
        ServicePack = 0;

        if (NTMajor > 6)
            NTBuildNo = (*(ULONG*)(KUSER_SHARED_DATA_UMPTR + 0x0260));
        else
            NTBuildNo = 0;

        switch (NTBuildNo)
        {
            // Windows 10
            case 10240: Subversion = 0;  break; // 1507
            case 10586: Subversion = 1;  break; // 1511
            case 14393: Subversion = 2;  break; // 1607
            case 15063: Subversion = 3;  break; // 1703
            case 16299: Subversion = 4;  break; // 1709
            case 17134: Subversion = 5;  break; // 1803
            case 17763: Subversion = 6;  break; // 1809
            case 18362: Subversion = 7;  break; // 1903
            case 18363: Subversion = 8;  break; // 1909
            case 19041: Subversion = 9;  break; // 2004
            case 19042: Subversion = 10; break; // 20H2
            case 19043: Subversion = 11; break; // 21H1
            case 19044: Subversion = 12; break; // 21H2
            case 19045: Subversion = 13; break; // 22H2

            // Windows 11
            case 22000: Subversion = 14; break; // 21H2
            case 22621: Subversion = 15; break; // 22H2
            case 22631: Subversion = 16; break; // 23H2
            case 26100: Subversion = 17; break; // 24H2

            default: Subversion = 0; break;     // Unknown build
        }
    }
    else
    {
        OSVERSIONINFOEXW OSVersion;

        OSVersion.dwOSVersionInfoSize = sizeof(OSVersion);

        if (GetVersionExW((LPOSVERSIONINFOW)&OSVersion))
        {
            NTMajor = OSVersion.dwMajorVersion;
            NTMinor = OSVersion.dwMinorVersion;
            ServicePack = OSVersion.wServicePackMajor;
            Subversion = 0;
        }
        else 
        {
            trace("Estimating an NTDDI value, GetVersionEx failed.\n");
            NTMajor = GetMajorNTVersion();
            NTMinor = GetMinorNTVersion();
            ServicePack = 0;
            Subversion = 0;
        }
    }

    return GENERATE_NTDDI(NTMajor, NTMinor, ServicePack, Subversion);
}

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
#define ok_eq_hex(value, expected)          ok_eq_print((unsigned long)value, (unsigned long)expected, "0x%08lx")
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
#define ok_eq_hex64(value, expected)        ok_eq_print((unsigned long long)value, (unsigned long long)expected, "%I64x")
#define ok_eq_xmm(value, expected)          ok((value).Low == (expected).Low, #value " = %I64x'%08I64x, expected %I64x'%08I64x\n", (value).Low, (value).High, (expected).Low, (expected).High)

#endif /* _APITEST_H */
