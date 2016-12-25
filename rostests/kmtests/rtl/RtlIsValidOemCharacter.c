/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Runtime library for RtlIsValidOemCharacter
 * PROGRAMMER:      Dmitry Chapyshev <dmitry@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

START_TEST(RtlIsValidOemCharacter)
{
    const WCHAR ValidCharsEn[] = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~!#$%^&*()_+|`:;\"'-/\\ ";
    const WCHAR InvalidChars[] = L"?\x0372\x03CF\x3D5F";
    WCHAR unicode_null = UNICODE_NULL;
    WCHAR tmp;
    BOOLEAN res;
    INT i;
    NTSTATUS Status = STATUS_SUCCESS;

    res = RtlIsValidOemCharacter(&unicode_null);
    ok(res != FALSE, "UNICODE_NULL is valid char\n");

    /* Test for valid chars */
    for (i = 0; i < (sizeof(ValidCharsEn) / sizeof(WCHAR)) - 1; i++)
    {
        tmp = ValidCharsEn[i];

        res = RtlIsValidOemCharacter(&tmp);
        ok(res != FALSE, "Expected success. '%C' [%d] is valid char\n", ValidCharsEn[i], i);
        ok(tmp == RtlUpcaseUnicodeChar(ValidCharsEn[i]), "Expected upcase char for '%C' [%d]\n", ValidCharsEn[i], i);

        tmp = RtlUpcaseUnicodeChar(ValidCharsEn[i]);
        res = RtlIsValidOemCharacter(&tmp);
        ok(res != FALSE, "Expected success. '%C' [%d] is valid char\n", ValidCharsEn[i], i);
        ok(tmp == RtlUpcaseUnicodeChar(ValidCharsEn[i]), "Expected upcase char for '%C' [%d]\n", ValidCharsEn[i], i);
    }

    /* Test for invalid chars */
    for (i = 0; i < (sizeof(InvalidChars) / sizeof(WCHAR)) - 1; i++)
    {
        tmp = InvalidChars[i];

        res = RtlIsValidOemCharacter(&tmp);
        ok(res == FALSE, "Expected fail. '%C' [%d] is NOT valid char\n", InvalidChars[i], i);
        ok(tmp == RtlUpcaseUnicodeChar(InvalidChars[i]), "Expected upcase char for '%C' [%d]\n", InvalidChars[i], i);

        tmp = RtlUpcaseUnicodeChar(InvalidChars[i]);
        res = RtlIsValidOemCharacter(&tmp);
        ok(res == FALSE, "Expected fail. '%C' [%d] is NOT valid char\n", InvalidChars[i], i);
        ok(tmp == RtlUpcaseUnicodeChar(InvalidChars[i]), "Expected upcase char for '%C' [%d]\n", InvalidChars[i], i);
    }

    _SEH2_TRY
    {
        RtlIsValidOemCharacter(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok(!NT_SUCCESS(Status), "Exception is expected but it did not occur\n");
}

