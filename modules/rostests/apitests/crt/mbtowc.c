/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Tests for mbtowc
 * COPYRIGHT:       Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include <apitest.h>
#include <apitest_guard.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <stdlib.h>

START_TEST(mbtowc)
{
    int Length;
    wchar_t BufferDest[3];
    char *ch;

    ch = AllocateGuarded(sizeof(ch));
    if (!ch)
    {
        skip("Buffer allocation failed!\n");
        return;
    }

    /* Assign a character for tests */
    *ch = 'A';

    /* Everything is NULL */
    Length = mbtowc(NULL, NULL, 0);
    ok(Length == 0, "Expected 0 characters to be converted as everything is NULL but got %u.\n", Length);

    /* Don't examine the number of bytes pointed by multibyte parameter */
    Length = mbtowc(BufferDest, ch, 0);
    ok(Length == 0, "Expected 0 characters to be converted but got %u.\n", Length);

    /* Wide character argument is invalid */
    Length = mbtowc(NULL, ch, 0);
    ok(Length == 0, "Expected 0 characters to be converted but got %u.\n", Length);

    /* The multibyte argument is invalid */
    Length = mbtowc(BufferDest, NULL, 0);
    ok(Length == 0, "Expected 0 characters to be converted but got %u.\n", Length);

    /* The multibyte argument is invalid but count number for examination is correct */
    Length = mbtowc(BufferDest, NULL, MB_CUR_MAX);
    ok(Length == 0, "Expected 0 characters to be converted but got %u.\n", Length);

    /* Don't give the output but the count character inspection argument is valid */
    Length = mbtowc(NULL, ch, MB_CUR_MAX);
    ok(Length == 1, "The number of bytes to check should be 1 but got %u.\n", Length);

    /* Convert the character and validate the output that we should get */
    Length = mbtowc(BufferDest, ch, MB_CUR_MAX);
    ok(Length == 1, "Expected 1 character to be converted but got %u.\n", Length);
    ok_int(BufferDest[0], L'A');

    FreeGuarded(ch);
}
