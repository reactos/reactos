/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for setjmp and longjmp
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2025 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>
#include <pseh/pseh2.h>
#include <setjmp.h>
#include <assert.h>
#include <rtlfuncs.h>

#if defined(_M_IX86) && !defined(_MSC_VER)
#define todo_pseh todo_if(1)
#else
#define todo_pseh
#endif

static jmp_buf g_jmp_buf;

static void TEST_setjmp_normal(void)
{
    volatile int stage = 0;
    volatile DWORD exception = 0;
    volatile BOOL abnormal = FALSE, finally_called = FALSE;
    int value;

    value = setjmp(g_jmp_buf);
    switch (stage)
    {
        case 0:
            ok_int(value, 0);
            stage = 1;
            longjmp(g_jmp_buf, 999);
            assert(FALSE);
            break;
        case 1:
            ok_int(value, 999);
            stage = 2;
            longjmp(g_jmp_buf, 0);
            assert(FALSE);
            break;
        case 2:
            ok_int(value, 1);
            stage = 3;
#ifdef __clang__ /* avoiding clang build hung up */
            skip("avoiding clang build crash\n");
#else /* ndef __clang__ */
            _SEH2_TRY
            {
                longjmp(g_jmp_buf, 333);
            }
            _SEH2_FINALLY
            {
                finally_called = TRUE;
                abnormal = AbnormalTermination();
            }
            _SEH2_END;
            assert(FALSE);
            break;
#endif /* ndef __clang__ */
        case 3:
            ok_int(value, 333);
#ifdef _M_AMD64 // This is broken on Windows 2003 x64
            if (NtCurrentPeb()->OSMajorVersion >= 6)
#endif
            {
                ok_int(finally_called, TRUE);
                ok_int(abnormal, TRUE);
            }
            stage = 4;
#ifdef __clang__ /* avoiding clang build hung up */
            skip("avoiding clang build crash\n");
#else /* ndef __clang__ */
            _SEH2_TRY
            {
                longjmp(g_jmp_buf, 444);
            }
            _SEH2_EXCEPT(exception = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
            {
                exception = -1;
            }
            _SEH2_END;
            assert(FALSE);
            break;
#endif /* ndef __clang__ */
        case 4:
            ok_int(value, 444);
            ok_int(exception, 0);
            break;
        default:
            assert(FALSE);
            break;
    }

    ok_int(stage, 4);
}

static INT s_check_points[16] = { 0 };

#define CHECK_POINT(number) do { \
    assert(number < _countof(s_check_points)); \
    s_check_points[number] = __LINE__; \
} while (0)

static void TEST_setjmp_return_check(void)
{
    volatile int x = 1001, value;
    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        CHECK_POINT(0);
        longjmp(g_jmp_buf, 999);
        CHECK_POINT(1);
    }
    else if (value == 999)
    {
        CHECK_POINT(2);
        ok_int(x, 1001);
    }
    else
    {
        CHECK_POINT(3);
    }
}

static void TEST_longjmp(int value)
{
    CHECK_POINT(4);
    longjmp(g_jmp_buf, value);
    CHECK_POINT(5);
}

static void TEST_setjmp_longjmp_integration(void)
{
    volatile int value;

    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        CHECK_POINT(6);
        TEST_longjmp(0xBEEFCAFE);
        CHECK_POINT(7);
    }
    else if (value == 0xBEEFCAFE)
    {
        CHECK_POINT(8);
    }
    else
    {
        CHECK_POINT(9);
    }
}

static void TEST_setjmp_zero_longjmp_check(void)
{
    volatile int value;
    volatile BOOL went_zero = FALSE;

    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        if (went_zero)
        {
            CHECK_POINT(10);
            return;
        }
        went_zero = TRUE;

        CHECK_POINT(11);

        TEST_longjmp(0); /* giving zero should go to one */

        CHECK_POINT(12);
    }
    else if (value == 1)
    {
        if (went_zero)
        {
            CHECK_POINT(13);
        }
        else
        {
            CHECK_POINT(14);
        }
    }
    else
    {
        CHECK_POINT(15);
    }
}

void call_setjmp(_JUMP_BUFFER *Buf);
ULONG_PTR get_sp(void);
extern char setjmp_return_address;
#ifdef _M_AMD64
void call_setjmpex(_JUMP_BUFFER *Buf);
#elif defined(_M_IX86)
int _setjmp3(jmp_buf env, int count, /* void* UnwindFunc, unsigned TryLevel, */ ...);
int _setjmp1(jmp_buf env); // ASM call wrapper around _setjmp, which is an intrinsic on MSVC
void call_setjmp3(_JUMP_BUFFER *Buf);
#endif

static void check_buffer_registers_(ULONG Line, _JUMP_BUFFER* Buf, ULONG_PTR Sp, void* Pc)
{
#ifdef _M_AMD64
    ok_eq_hex64_(__FILE__, Line, Buf->Frame, Sp - 0xF0);
    ok_eq_hex64_(__FILE__, Line, Buf->Rbx, 0xA1A1A1A1A1A1A1A1ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Rsp, Sp - 0xF0);
    ok_eq_hex64_(__FILE__, Line, Buf->Rbp, 0xA2A2A2A2A2A2A2A2ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Rsi, 0xA3A3A3A3A3A3A3A3ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Rdi, 0xA4A4A4A4A4A4A4A4ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->R12, 0xACACACACACACACACULL);
    ok_eq_hex64_(__FILE__, Line, Buf->R13, 0xADADADADADADADADULL);
    ok_eq_hex64_(__FILE__, Line, Buf->R14, 0xAEAEAEAEAEAEAEAEULL);
    ok_eq_hex64_(__FILE__, Line, Buf->R15, 0xAFAFAFAFAFAFAFAFULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Rip, (ULONG64)Pc);
    ok_eq_hex_(__FILE__, Line, Buf->MxCsr, 0x00001f80);
    ok_eq_hex64_(__FILE__, Line, Buf->FpCsr, 0x27F);
    ok_eq_hex64_(__FILE__, Line, Buf->Spare, 0xCCCC);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm6.Part[0], 0x0606060606060606ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm6.Part[1], 0x1616161616161616ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm7.Part[0], 0x0707070707070707ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm7.Part[1], 0x1717171717171717ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm8.Part[0], 0x0808080808080808ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm8.Part[1], 0x1818181818181818ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm9.Part[0], 0x0909090909090909ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm9.Part[1], 0x1919191919191919ULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm10.Part[0], 0x0A0A0A0A0A0A0A0AULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm10.Part[1], 0x1A1A1A1A1A1A1A1AULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm11.Part[0], 0x0B0B0B0B0B0B0B0BULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm11.Part[1], 0x1B1B1B1B1B1B1B1BULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm12.Part[0], 0x0C0C0C0C0C0C0C0CULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm12.Part[1], 0x1C1C1C1C1C1C1C1CULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm13.Part[0], 0x0D0D0D0D0D0D0D0DULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm13.Part[1], 0x1D1D1D1D1D1D1D1DULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm14.Part[0], 0x0E0E0E0E0E0E0E0EULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm14.Part[1], 0x1E1E1E1E1E1E1E1EULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm15.Part[0], 0x0F0F0F0F0F0F0F0FULL);
    ok_eq_hex64_(__FILE__, Line, Buf->Xmm15.Part[1], 0x1F1F1F1F1F1F1F1FULL);
#elif defined(_M_IX86)
    ok_eq_hex_(__FILE__, Line, Buf->Ebp, 0xA1A1A1A1ul);
    ok_eq_hex_(__FILE__, Line, Buf->Ebx, 0xA2A2A2A2ul);
    ok_eq_hex_(__FILE__, Line, Buf->Edi, 0xA3A3A3A3ul);
    ok_eq_hex_(__FILE__, Line, Buf->Esi, 0xA4A4A4A4ul);
    ok_eq_hex_(__FILE__, Line, Buf->Esp, Sp - 0x38);
    ok_eq_hex_(__FILE__, Line, Buf->Eip, (ULONG)Pc);
#endif
}
#define check_buffer_registers(Buf, Sp, Pc) \
    check_buffer_registers_(__LINE__, Buf, Sp, Pc)

static void TEST_buffer_contents(void)
{
    _JUMP_BUFFER buf;

    memset(&buf, 0xCC, sizeof(buf));
    call_setjmp(&buf);
    check_buffer_registers(&buf, get_sp(), &setjmp_return_address);

#ifdef _M_AMD64

    memset(&buf, 0xCC, sizeof(buf));
    call_setjmpex(&buf);
    check_buffer_registers(&buf, get_sp(), &setjmp_return_address);

#elif defined(_M_IX86)

    ok_eq_hex(buf.Registration, __readfsdword(0));
    todo_pseh ok_eq_hex(buf.TryLevel, 0xFFFFFFFF);
    ok_eq_hex(buf.Cookie, 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindFunc, 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindData[0], 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindData[1], 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindData[2], 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindData[3], 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindData[4], 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindData[5], 0xCCCCCCCC);

    // Temporarily remove the SEH registration (__writefsdword(0, ...) is not allowed with MSVC)
    PULONG ExceptionRegistrationPtr = (PULONG)NtCurrentTeb();
    ULONG Registration = *ExceptionRegistrationPtr;
    *ExceptionRegistrationPtr = 0xFFFFFFFF;

    memset(&buf, 0xCC, sizeof(buf));
    call_setjmp(&buf);
    ok_eq_hex(buf.Registration, 0xFFFFFFFF);
    ok_eq_hex(buf.TryLevel, 0xFFFFFFFF);
    ok_eq_hex(buf.Cookie, 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindFunc, 0xCCCCCCCC);

    // Restore the SEH registration
    *ExceptionRegistrationPtr = Registration;

    _SEH2_TRY
    {
        Registration = __readfsdword(0);
        _SEH2_TRY
        {
            ok_eq_hex(Registration, __readfsdword(0));
            memset(&buf, 0xCC, sizeof(buf));
            call_setjmp(&buf);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* This is to ensure that the exception handler is not optimized out. */
            ok_int(GetExceptionCode(), 0);
        }
        _SEH2_END;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* This is to ensure that the exception handler is not optimized out. */
        ok_int(GetExceptionCode(), 0);
    }
    _SEH2_END;

    ok_eq_hex(buf.Registration, Registration);
    todo_pseh ok_eq_hex(buf.TryLevel, 1);
    ok_eq_hex(buf.Cookie, 0xCCCCCCCC);
    ok_eq_hex(buf.UnwindFunc, 0xCCCCCCCC);
#endif
}

#ifdef _M_IX86

__declspec(noinline)
static void Test_setjmp1_longjmp_inside_SEH(void)
{
    jmp_buf buf;
    int finally_called = 0;

    // Use the legacy _setjmp
    int longjmp_value = _setjmp1(buf);
    if (longjmp_value == 0)
    {
        _SEH2_TRY
        {
            longjmp(buf, 0x12345678);
        }
        _SEH2_FINALLY
        {
            finally_called = 1;
        }
        _SEH2_END;
    }

    ok_eq_int(longjmp_value, 0x12345678);
    ok_eq_int(finally_called, 1);
}

__declspec(noinline)
static int Test_setjmp1_inner(void)
{
    jmp_buf buf;

    int longjmp_value = _setjmp1(buf);
    if (longjmp_value == 0)
    {
        longjmp(buf, 0x12345678);
    }
    return longjmp_value;
}

__declspec(noinline)
static void Test_setjmp1_external_inside_SEH(void)
{
    volatile int ret = 0;
    _SEH2_TRY
    {
        _SEH2_TRY
        {
            ret = Test_setjmp1_inner();
        }
        _SEH2_FINALLY
        {
            if (_SEH2_AbnormalTermination())
                ret = -1;
        }
        _SEH2_END;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ret = -2;
    }
    _SEH2_END;
    ok_eq_int(ret, 0x12345678);
}

#if 0 // This actually crashes on Windows!
__declspec(noinline)
void Test_setjmp1_no_SEH_registration(void)
{
    jmp_buf buf;

    // Temporarily remove the SEH registration
    PULONG ExceptionRegistrationPtr = (PULONG)NtCurrentTeb();
    ULONG Registration = *ExceptionRegistrationPtr;
    *ExceptionRegistrationPtr = 0xFFFFFFFF;

    // This will save 0xFFFFFFFF in the Registration field
    int longjmp_value = _setjmp1(buf);
    if (longjmp_value == 0)
    {
        // This will check if Registration is 0(!) and if not, it will
        // call _local_unwind2, which will dereference the Registration
        // and crash.
        longjmp(buf, 1);
    }

    ok_eq_int(longjmp_value, 1);

    // Restore the SEH registration
    *ExceptionRegistrationPtr = Registration;
}
#endif

__declspec(noinline)
static void Test_setjmp3(void)
{
    ULONG BufferData[18];
    _JUMP_BUFFER* Buf = (_JUMP_BUFFER*)&BufferData;

    memset(&BufferData, 0xCC, sizeof(BufferData));
    call_setjmp3(Buf);
    check_buffer_registers(Buf, get_sp(), &setjmp_return_address);
    ok_eq_hex(Buf->Registration, __readfsdword(0));
    ok_eq_hex(Buf->TryLevel, 7);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, 0x00012345);
    ok_eq_hex(Buf->UnwindData[0], 0x12345678);
    ok_eq_hex(Buf->UnwindData[1], 0x23456789);
    ok_eq_hex(Buf->UnwindData[2], 0x3456789a);
    ok_eq_hex(Buf->UnwindData[3], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[4], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[5], 0xCCCCCCCC);

    memset(&BufferData, 0xCC, sizeof(BufferData));
    _setjmp3((int*)Buf, 0, (void*)0x12345, 3, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4);
    ok_eq_hex(Buf->Registration, __readfsdword(0));
    ok_eq_hex(Buf->TryLevel, 0);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, 0x00000000);
    ok_eq_hex(Buf->UnwindData[0], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[1], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[2], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[3], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[4], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[5], 0xCCCCCCCC);

    static ULONG Data[4] = { 0x0123, 0x1234, 0x2345, 0x3456 };
    memset(&BufferData, 0xCC, sizeof(BufferData));
    _setjmp3((int*)Buf, 1, Data, 7, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4);
    ok_eq_hex(Buf->Registration, __readfsdword(0));
    ok_eq_hex(Buf->TryLevel, 0x00003456);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, (ULONG)Data);
    ok_eq_hex(Buf->UnwindData[0], 0xCCCCCCCC);

    memset(&BufferData, 0xCC, sizeof(BufferData));
    _setjmp3((int*)Buf, 2, Data, 7, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4);
    ok_eq_hex(Buf->Registration, __readfsdword(0));
    ok_eq_hex(Buf->TryLevel, 7);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, (ULONG)Data);
    ok_eq_hex(Buf->UnwindData[0], 0xCCCCCCCC);

    memset(&BufferData, 0xCC, sizeof(BufferData));
    _setjmp3((int*)Buf, 3, Data, 7, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4);
    ok_eq_hex(Buf->Registration, __readfsdword(0));
    ok_eq_hex(Buf->TryLevel, 7);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, (ULONG)Data);
    ok_eq_hex(Buf->UnwindData[0], 0xA1A1A1A1);
    ok_eq_hex(Buf->UnwindData[1], 0xCCCCCCCC);

    memset(&BufferData, 0xCC, sizeof(BufferData));
    _setjmp3((int*)Buf, 11, (PVOID)0xdeadbeef, 7, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4, 0xA5A5A5A5, 0xA6A6A6A6, 0xA7A7A7A7, 0xA8A8A8A8, 0xA9A9A9A9);
    ok_eq_hex(Buf->Registration, __readfsdword(0));
    ok_eq_hex(Buf->TryLevel, 7);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, 0xdeadbeef);
    ok_eq_hex(Buf->UnwindData[0], 0xA1A1A1A1);
    ok_eq_hex(Buf->UnwindData[1], 0xA2A2A2A2);
    ok_eq_hex(Buf->UnwindData[2], 0xA3A3A3A3);
    ok_eq_hex(Buf->UnwindData[3], 0xA4A4A4A4);
    ok_eq_hex(Buf->UnwindData[4], 0xA5A5A5A5);
    ok_eq_hex(Buf->UnwindData[5], 0xA6A6A6A6);
    ok_eq_hex(BufferData[16], 0xCCCCCCCC);
    ok_eq_hex(BufferData[17], 0xCCCCCCCC);

    // Temporarily remove the SEH registration
    PULONG ExceptionRegistrationPtr = (PULONG)NtCurrentTeb();
    ULONG Registration = *ExceptionRegistrationPtr;
    *ExceptionRegistrationPtr = 0xFFFFFFFF;

    memset(&BufferData, 0xCC, sizeof(BufferData));
    _setjmp3((int*)Buf, 11, (PVOID)0xdeadbeef, 7, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4, 0xA5A5A5A5, 0xA6A6A6A6, 0xA7A7A7A7, 0xA8A8A8A8, 0xA9A9A9A9);
    ok_eq_hex(Buf->Registration, 0xFFFFFFFF);
    ok_eq_hex(Buf->TryLevel, 0xFFFFFFFF);
    ok_eq_hex(Buf->Cookie, 0x56433230);
    ok_eq_hex(Buf->UnwindFunc, 0x00000000);
    ok_eq_hex(Buf->UnwindData[0], 0xCCCCCCCC);
    ok_eq_hex(Buf->UnwindData[1], 0xCCCCCCCC);

    int value = _setjmp3((int*)Buf, 0);
    if (value == 0)
    {
        ok_eq_hex(Buf->Registration, 0xFFFFFFFF);
        ok_eq_hex(Buf->TryLevel, 0xFFFFFFFF);
        ok_eq_hex(Buf->Cookie, 0x56433230);
        ok_eq_hex(Buf->UnwindFunc, 0x00000000);
        ok_eq_hex(Buf->UnwindData[0], 0xCCCCCCCC);
        ok_eq_hex(Buf->UnwindData[1], 0xCCCCCCCC);
        longjmp((int*)Buf, 0xBEEFCAFE);
        ok(0, "Should not get here\n");
    }
    ok_int(value, 0xBEEFCAFE);

    // Restore the SEH registration
    *ExceptionRegistrationPtr = Registration;
}

__declspec(noinline)
void Test_setjmp3_with_SEH(void)
{
    _JUMP_BUFFER buf;
    volatile int dummy;

    _SEH2_TRY
    {
        dummy = 0;
        (void)dummy;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    memset(&buf, 0xCC, sizeof(buf));
    _setjmp3((int*)& buf, 0, (void*)0x12345, 3, 0xA1A1A1A1, 0xA2A2A2A2, 0xA3A3A3A3, 0xA4A4A4A4);
    ok_eq_hex(buf.Registration, __readfsdword(0));
    todo_pseh ok_eq_hex(buf.TryLevel, 0xffffffff);
}

#endif // _M_IX86

#undef setjmp
START_TEST(setjmp)
{
    ZeroMemory(&s_check_points, sizeof(s_check_points));

    /* FIXME: These tests are insufficient */
    TEST_setjmp_normal();
    TEST_setjmp_return_check();
    TEST_setjmp_longjmp_integration();
    TEST_setjmp_zero_longjmp_check();
    TEST_buffer_contents();
#ifdef _M_IX86
    Test_setjmp1_longjmp_inside_SEH();
    Test_setjmp1_external_inside_SEH();
    //Test_setjmp1_no_SEH_registration();
    Test_setjmp3();
    Test_setjmp3_with_SEH();
#endif /* _M_IX86 */

#define DO_COME(number) \
    ok(s_check_points[number], "CheckPoint #%d: Didn't reach\n", number)
#define NEVER_COME(number) \
    ok(!s_check_points[number], "CheckPoint #%d: Wrongly reached Line %d\n", \
       number, s_check_points[number])

    DO_COME(0);
    NEVER_COME(1);

    DO_COME(0);
    NEVER_COME(1);
    DO_COME(2);
    NEVER_COME(3);
    DO_COME(4);
    NEVER_COME(5);
    DO_COME(6);
    NEVER_COME(7);
    DO_COME(8);
    NEVER_COME(9);
    NEVER_COME(10);
    DO_COME(11);
    NEVER_COME(12);
    DO_COME(13);
    NEVER_COME(14);
    NEVER_COME(15);
}
