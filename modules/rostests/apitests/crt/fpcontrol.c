/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for fp control functions
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>

#include <apitest.h>
#include <xmmintrin.h>
#include <float.h>
#include <pseh/pseh2.h>


unsigned int get_native_fpcw(void)
{
#ifdef _M_AMD64
    return _mm_getcsr();
#elif defined (_M_IX86)
    unsigned short fpcw;
#if defined(_MSC_VER)
    __asm fstsw[fpcw];
#else
    __asm__ __volatile__("fstsw %0" : "=m" (fpcw) : );
#endif
    return fpcw;
#else
    #error "Unsupported architecture"
    return 0;
#endif
}

void set_native_fpcw(unsigned int value)
{
#ifdef _M_AMD64
    _mm_setcsr(value);
#elif defined (_M_IX86)
    unsigned short fpcw = (unsigned short)value;
#if defined(_MSC_VER)
    __asm fldcw[fpcw];
#else
    __asm__ __volatile__("fldcw %0" : : "m" (fpcw));
#endif
#else
#error "Unsupported architecture"
#endif
}

/*
    _clear87
    _clearfp
    _controlfp_s
    _set_controlfp
    _statusfp
    __control87_2
*/

#ifdef _M_IX86
#define ON_IX86(x) x
#else
#define ON_IX86(x)
#endif

#ifdef _M_AMD64
#define ON_AMD64(x) x
#else
#define ON_AMD64(x)
#endif

#ifdef _M_ARM
#define ON_ARM(x) x
#else
#define ON_ARM(x)
#endif

struct
{
    unsigned int Value;
    unsigned int Mask;
    unsigned int Result;
    unsigned int Native;
} g_controlfp_Testcases[] =
{
    { 0xffffffff, 0xffffffff, ON_IX86(0x30e031f) ON_AMD64(0x308031f) ON_ARM(0), ON_IX86(0) ON_AMD64(0xff80) ON_ARM(0) },
    { 0, 0xffffffff, 0x80000, ON_IX86(0) ON_AMD64(0x100) ON_ARM(0) },
    { 0xffffffff, 0x14, 0x80014, ON_IX86(0) ON_AMD64(0x580) ON_ARM(0) },
    { _EM_INEXACT, 0xffffffff, _EM_INEXACT | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_INEXACT | _MM_MASK_DENORM) ON_ARM(0) },
    { _EM_UNDERFLOW, 0xffffffff, _EM_UNDERFLOW | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_UNDERFLOW | _MM_MASK_DENORM) ON_ARM(0) },
    { _EM_OVERFLOW, 0xffffffff, _EM_OVERFLOW | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_OVERFLOW | _MM_MASK_DENORM) ON_ARM(0) },
    { _EM_ZERODIVIDE, 0xffffffff, _EM_ZERODIVIDE | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_DIV_ZERO | _MM_MASK_DENORM) ON_ARM(0) },
    { _EM_INVALID, 0xffffffff, _EM_INVALID | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_INVALID | _MM_MASK_DENORM) ON_ARM(0) },
    { _RC_NEAR, 0xffffffff, _RC_NEAR | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_ROUND_NEAREST | _MM_MASK_DENORM) ON_ARM(0) },
    { _RC_DOWN, 0xffffffff, _RC_DOWN | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_ROUND_DOWN | _MM_MASK_DENORM) ON_ARM(0) },
    { _RC_UP, 0xffffffff, _RC_UP | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_ROUND_UP | _MM_MASK_DENORM) ON_ARM(0) },
    { _RC_CHOP, 0xffffffff, _RC_CHOP | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_ROUND_TOWARD_ZERO | _MM_MASK_DENORM) ON_ARM(0) },
    { _IC_AFFINE, 0xffffffff, _EM_DENORMAL ON_IX86(| _IC_AFFINE), ON_IX86(0) ON_AMD64(_MM_MASK_DENORM) ON_ARM(0)},
    { _IC_PROJECTIVE, 0xffffffff, _IC_PROJECTIVE | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_DENORM) ON_ARM(0) },
    { _DN_SAVE, 0xffffffff, _DN_SAVE | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_MASK_DENORM) ON_ARM(0) },
    { _DN_FLUSH, 0xffffffff, _DN_FLUSH | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_FLUSH_ZERO_ON | 0x40 | _MM_MASK_DENORM) ON_ARM(0) },
    { _DN_FLUSH_OPERANDS_SAVE_RESULTS, 0xffffffff, _DN_FLUSH_OPERANDS_SAVE_RESULTS | _EM_DENORMAL, ON_IX86(0) ON_AMD64(0x40 | _MM_MASK_DENORM) ON_ARM(0) },
    { _DN_SAVE_OPERANDS_FLUSH_RESULTS, 0xffffffff, _DN_SAVE_OPERANDS_FLUSH_RESULTS | _EM_DENORMAL, ON_IX86(0) ON_AMD64(_MM_FLUSH_ZERO_ON | _MM_MASK_DENORM) ON_ARM(0) },
};

void Test_controlfp(void)
{
    unsigned int i, native_fpcw, fpcw;

    for (i = 0; i < _countof(g_controlfp_Testcases); i++)
    {
        fpcw = _controlfp(g_controlfp_Testcases[i].Value, g_controlfp_Testcases[i].Mask);
        ok(fpcw == g_controlfp_Testcases[i].Result, "[%u] _controlfp failed: expected 0x%x, got 0x%x\n", i, g_controlfp_Testcases[i].Result, fpcw);
        native_fpcw = get_native_fpcw();
        ok(native_fpcw == g_controlfp_Testcases[i].Native, "[%u] wrong native_fpcw: expected 0x%x, got 0x%x\n", i, g_controlfp_Testcases[i].Native, native_fpcw);
    }

    /* Restore sane state */
    _fpreset();
}

#if defined(_M_IX86) || defined(_M_AMD64)
void Test_control87(void)
{
    unsigned int native_fpcw, fpcw;

    fpcw = _control87(0, 0xffffffff);
    ok(fpcw == 0, "_control87 failed: expected 0x%x, got 0x%x\n", 0, fpcw);
    native_fpcw = get_native_fpcw();
    ok_hex(native_fpcw, ON_IX86(0) ON_AMD64(0));

    /* Restore sane state */
    _fpreset();
}
#endif

typedef enum _FP_OP
{
    OP_Inexact,
    OP_Underflow,
    OP_Overflow,
    OP_ZeroDivide,
    OP_Invalid,
    OP_Denormal
} FP_OP;

struct
{
    FP_OP Operation;
    unsigned int Fpcw;
    unsigned int FpStatus;
    unsigned int ExceptionCode;
    unsigned int Native;
} g_exception_Testcases[] =
{
    { OP_Inexact, 0xffffffff, _SW_UNDERFLOW | _SW_INEXACT ON_IX86(| _SW_DENORMAL), 0, ON_IX86(0x32) ON_AMD64(0xffb0) ON_ARM(0)},
    { OP_Inexact, ~_EM_INEXACT, _SW_INEXACT ON_AMD64(| _SW_UNDERFLOW), STATUS_FLOAT_INEXACT_RESULT, ON_IX86(0x3800) ON_AMD64(0xefb0) ON_ARM(0)},
    { OP_Inexact, ~_MCW_EM, _SW_INEXACT ON_AMD64(| _SW_UNDERFLOW), ON_IX86(STATUS_FLOAT_INEXACT_RESULT) ON_AMD64(STATUS_FLOAT_UNDERFLOW) ON_ARM(STATUS_FLOAT_UNDERFLOW), ON_IX86(0x3800) ON_AMD64(0xe130) ON_ARM(0) },
    { OP_Underflow, 0xffffffff, _SW_UNDERFLOW | _SW_INEXACT, 0, ON_IX86(0x30) ON_AMD64(0xffb0) ON_ARM(0)},
    { OP_Underflow, ~_EM_UNDERFLOW, _SW_UNDERFLOW | _SW_INEXACT, STATUS_FLOAT_UNDERFLOW, ON_IX86(0x3800) ON_AMD64(0xf7b0) ON_ARM(0) },
    { OP_Underflow, ~_MCW_EM,  _SW_INEXACT ON_AMD64(| _SW_UNDERFLOW), ON_IX86(STATUS_FLOAT_INEXACT_RESULT) ON_AMD64(STATUS_FLOAT_UNDERFLOW) ON_ARM(STATUS_FLOAT_UNDERFLOW), ON_IX86(0x3800) ON_AMD64(0xe130) ON_ARM(0) },
    { OP_Overflow, 0xffffffff, _SW_OVERFLOW | _SW_INEXACT, 0, ON_IX86(0x28) ON_AMD64(0xffa8) ON_ARM(0) },
    { OP_Overflow, ~_EM_OVERFLOW, _SW_OVERFLOW | _SW_INEXACT, STATUS_FLOAT_OVERFLOW, ON_IX86(0x3800) ON_AMD64(0xfba8) ON_ARM(0) },
    { OP_Overflow, ~_MCW_EM, _SW_INEXACT ON_AMD64(| _SW_OVERFLOW), ON_IX86(STATUS_FLOAT_INEXACT_RESULT) ON_AMD64(STATUS_FLOAT_OVERFLOW) ON_ARM(STATUS_FLOAT_OVERFLOW), ON_IX86(0x3800) ON_AMD64(0xe128) ON_ARM(0)},
    { OP_ZeroDivide, 0xffffffff, _SW_ZERODIVIDE, 0, ON_IX86(0x4) ON_AMD64(0xff84) ON_ARM(0) },
    { OP_ZeroDivide, ~_EM_ZERODIVIDE, _SW_ZERODIVIDE, STATUS_FLOAT_DIVIDE_BY_ZERO, ON_IX86(0x3000) ON_AMD64(0xfd84) ON_ARM(0) },
    { OP_ZeroDivide, ~_MCW_EM, _SW_ZERODIVIDE, STATUS_FLOAT_DIVIDE_BY_ZERO, ON_IX86(0x3000) ON_AMD64(0xe104) ON_ARM(0) },
    { OP_Invalid, 0xffffffff, _SW_INVALID, 0, ON_IX86(0x1) ON_AMD64(0xff81) ON_ARM(0) },
    { OP_Invalid, ~_EM_INVALID, _SW_INVALID, STATUS_FLOAT_INVALID_OPERATION, ON_IX86(0) ON_AMD64(0xff01) ON_ARM(0) },
    { OP_Invalid, ~_MCW_EM, _SW_INVALID, STATUS_FLOAT_INVALID_OPERATION, ON_IX86(0) ON_AMD64(0xe101) ON_ARM(0) },
#if defined(_M_IX86) || defined(_M_AMD64) // || defined(_M_ARM64) ?
    { OP_Denormal, 0xffffffff, _SW_DENORMAL | _SW_INEXACT ON_AMD64(| _SW_UNDERFLOW), 0, ON_IX86(0x22) ON_AMD64(0xffb2) ON_ARM(0)},
    { OP_Denormal, ~_EM_DENORMAL, _SW_DENORMAL, STATUS_FLOAT_INVALID_OPERATION, ON_IX86(0x3800) ON_AMD64(0xfe82) ON_ARM(0) },
    { OP_Denormal, ~_MCW_EM, _SW_DENORMAL, STATUS_FLOAT_INVALID_OPERATION, ON_IX86(0x3800) ON_AMD64(0xe002) ON_ARM(0) },
#endif
};

void Test_exceptions(void)
{
    volatile double a, b;
    unsigned long long ull;
    volatile long status = 0;

    unsigned int i, exp_fpstatus, native_fpcw, statusfp;

    for (i = 0; i < _countof(g_exception_Testcases); i++)
    {
        /* Start clean */
        status = 0;
        _fpreset();
        _clearfp();
        ok_hex(_statusfp(), 0);

        _controlfp(g_exception_Testcases[i].Fpcw, 0xffffffff);
#if defined(_M_IX86) || defined(_M_AMD64) // || defined(_M_ARM64) ?
        if (g_exception_Testcases[i].Operation == OP_Denormal)
            _control87(g_exception_Testcases[i].Fpcw, 0xffffffff);
#endif

        _SEH2_TRY
        {
            switch (g_exception_Testcases[i].Operation)
            {
                case OP_Inexact:
                    a = 1e-40;
                    b = (float)(a + 1e-40);
                    break;
                case OP_Underflow:
                    a = DBL_MIN;
                    b = a / 3.0e16;
                    break;
                case OP_Overflow:
                    a = DBL_MAX;
                    b = a * 3.0;
                    break;
                case OP_ZeroDivide:
                    a = 0.0;
                    b = 1.0 / a;
                    break;
                case OP_Invalid:
                    ull = 0x7FF0000000000001ull;
                    a = *(double*)&ull;
                    b = a * 2;
                    break;
                case OP_Denormal:
                    a = DBL_MIN;
                    b = a - 4.9406564584124654e-324;
                    break;
                default:
                    (void)b;
            }
            native_fpcw = get_native_fpcw();
            statusfp = _clearfp();
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
#ifdef _M_IX86
            /* On x86 we need to clear before doing any other fp operations, otherwise it will throw again */
            statusfp = _clearfp();
            native_fpcw = get_native_fpcw();
#else
            native_fpcw = get_native_fpcw();
            statusfp = _clearfp();
#endif
            status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        exp_fpstatus = g_exception_Testcases[i].FpStatus;
        ok(statusfp == exp_fpstatus, "[%u] Wrong value for _statusfp(). Expected 0x%lx, got 0x%lx\n", i, exp_fpstatus, statusfp);
        ok(status == g_exception_Testcases[i].ExceptionCode, "[%u] Wrong value for status. Expected 0x%lx, got 0x%lx\n", i, g_exception_Testcases[i].ExceptionCode, status);
        ok(native_fpcw == g_exception_Testcases[i].Native, "[%u] wrong native_fpcw: expected 0x%x, got 0x%x\n", i, g_exception_Testcases[i].Native, native_fpcw);
    }
}

START_TEST(fpcontrol)
{
    unsigned int native_fpcw, fpcw, fpstatus;

    /* Test native start fpcw */
    native_fpcw = get_native_fpcw();
    ok_hex(native_fpcw, ON_IX86(0) ON_AMD64(0x1f80) ON_ARM(0) );

    /* Test start fpcw */
    fpcw = _controlfp(0, 0);
    ok_hex(fpcw, ON_IX86(0x9001f) ON_AMD64(0x8001f) ON_ARM(0));

    /* Test start status */
    fpstatus = _statusfp();
    ok_hex(fpstatus, 0);

    /* Test _fpreset */
    fpcw = _controlfp(0, 0xffffffff);
    ok_hex(fpcw, 0x80000);
    _fpreset();
    fpcw = _controlfp(0, 0);
    ok_hex(fpcw, ON_IX86(0x9001f) ON_AMD64(0x8001f) ON_ARM(0));

    Test_controlfp();
#if defined(_M_IX86) || defined(_M_AMD64)
    Test_control87();
#endif
    Test_exceptions();
}
