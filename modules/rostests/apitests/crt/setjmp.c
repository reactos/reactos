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

START_TEST(setjmp)
{
    ZeroMemory(&s_check_points, sizeof(s_check_points));

    /* FIXME: These tests are insufficient */
    TEST_setjmp_normal();
    TEST_setjmp_return_check();
    TEST_setjmp_longjmp_integration();
    TEST_setjmp_zero_longjmp_check();

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
