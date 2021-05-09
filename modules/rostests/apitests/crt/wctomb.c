/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for wctomb
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include <apitest.h>
#include <apitest_guard.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>

unsigned int cdecl ___lc_codepage_func(void);

START_TEST(wctomb)
{
    int Length;
    char *chDest;
    char *loc;
    unsigned int codepage = ___lc_codepage_func();
    wchar_t wchSrc[2] = {L'R', 0414}; // 0414 corresponds to a Russian character in Windows-1251

    chDest = AllocateGuarded(sizeof(*chDest));
    if (!chDest)
    {
        skip("Buffer allocation failed!\n");
        return;
    }

    /* Output the current locale of the system and codepage for comparison between ReactOS and Windows */
    loc = setlocale(LC_ALL, NULL);
    printf("The current codepage of your system tested is (%u) and locale (%s).\n\n", codepage, loc);

    /* Do not give output to the caller */
    Length = wctomb(NULL, 0);
    ok(Length == 0, "Expected no characters to be converted (because the output argument is refused) but got %d\n.", Length);

    /* Do the same but expect a valid wide character argument this time */
    Length = wctomb(NULL, wchSrc[0]);
    ok(Length == 0, "Expected no characters to be converted (because the output argument is refused) but got %d\n.", Length);

    /* Don't return anything to the output even if conversion is impossible */
    Length = wctomb(NULL, wchSrc[1]);
    ok(errno == 0, "The error number (errno) should be 0 even though an invalid character in current locale is given but got %d.\n", errno);
    ok(Length == 0, "Expected no characters to be converted (because the output argument is refused) but got %d\n.", Length);

    /* Attempt to convert a character not possible in current locale */
    Length = wctomb(chDest, wchSrc[1]);
    ok(Length == -1, "The conversion is not possible in current locale but got %d as returned value.\n", Length);
    ok(errno == EILSEQ, "EILSEQ is expected in an illegal sequence conversion but got %d.\n", errno);

    /* Return a null wide character to the destination argument */
    Length = wctomb(chDest, 0);
    ok(Length == 1, "Expected one character to be converted (the null character) but got %d.\n", Length);
    ok_int(chDest[0], '\0');

    /* Get the converted output and validate what we should get */
    Length = wctomb(chDest, wchSrc[0]);
    ok(Length == 1, "Expected one character to be converted but got %d.\n", Length);
    ok_int(chDest[0], 'R');

    FreeGuarded(chDest);
}
