/*
 * Copyright 2016 Jared Smudde
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Documentation: https://learn.microsoft.com/en-us/windows/win32/api/netcon/nf-netcon-ncisvalidconnectionname */

#include <apitest.h>

static BOOL (WINAPI *pNcIsValidConnectionName)(PCWSTR);

#define CALL_NC(exp, str) \
    do { \
        BOOL ret = pNcIsValidConnectionName((str)); \
        ok(ret == (exp), "Expected %s to be %d, was %d\n", wine_dbgstr_w((str)), (exp), ret); \
    } while (0)



static void test_BadLetters(void)
{
    BOOL ret;

    WCHAR buf[3] = { 0 };
    int i;

    for (i = 1; i <= 0xFFFF; ++i)
    {
        buf[0] = (WCHAR)i;
        buf[1] = buf[2] = L'\0';

        if (wcspbrk(buf, L"\\/:\t*? <>|\"") != NULL)
        {
            ret = pNcIsValidConnectionName(buf);
            ok(ret == FALSE, "Expected %s (%i) to fail.\n", wine_dbgstr_w(buf), i);

            /* How about two of a kind? */
            buf[1] = (WCHAR)i;
            ret = pNcIsValidConnectionName(buf);
            ok(ret == FALSE, "Expected %s (%i) to fail.\n", wine_dbgstr_w(buf), i);

            /* And something (bad) combined with a space? */
            buf[1] = L' ';
            ret = pNcIsValidConnectionName(buf);
            ok(ret == FALSE, "Expected %s (%i) to fail.\n", wine_dbgstr_w(buf), i);


            /* Something bad combined with a letter */
            buf[1] = L'a';
            ret = pNcIsValidConnectionName(buf);
            if ((WCHAR)i == L' ')
                ok(ret == TRUE, "Expected %s (%i) to succeed.\n", wine_dbgstr_w(buf), i);
            else
                ok(ret == FALSE, "Expected %s (%i) to fail.\n", wine_dbgstr_w(buf), i);
        }
        else
        {
            ret = pNcIsValidConnectionName(buf);
            ok(ret == TRUE, "Expected %s (%i) to succeed.\n", wine_dbgstr_w(buf), i);

            buf[1] = (WCHAR)i;
            ret = pNcIsValidConnectionName(buf);
            ok(ret == TRUE, "Expected %s (%i) to succeed.\n", wine_dbgstr_w(buf), i);

            buf[1] = L'a';
            ret = pNcIsValidConnectionName(buf);
            ok(ret == TRUE, "Expected %s (%i) to succeed.\n", wine_dbgstr_w(buf), i);

            buf[1] = L' ';
            ret = pNcIsValidConnectionName(buf);
            ok(ret == TRUE, "Expected %s (%i) to succeed.\n", wine_dbgstr_w(buf), i);
        }
    }
}

START_TEST(isvalidname)
{
    HMODULE hDll = LoadLibraryA("netshell.dll");

    pNcIsValidConnectionName = (void*)GetProcAddress(hDll, "NcIsValidConnectionName");
    if (!hDll || !pNcIsValidConnectionName)
    {
        skip("netshell.dll or export NcIsValidConnectionName not found! Tests will be skipped\n");
        return;
    }

    CALL_NC(TRUE, L"Network");
    CALL_NC(FALSE, L"Network?");

    CALL_NC(FALSE, L"\\");
    CALL_NC(FALSE, L"/");
    CALL_NC(FALSE, L":");
    CALL_NC(FALSE, L"*");
    CALL_NC(FALSE, L"?");
    CALL_NC(FALSE, L"<");
    CALL_NC(FALSE, L">");
    CALL_NC(FALSE, L"|");

    CALL_NC(FALSE, NULL);

    CALL_NC(TRUE, L"Wireless");
    CALL_NC(FALSE, L"Wireless:1");
    CALL_NC(TRUE, L"Intranet");
    CALL_NC(FALSE, L"Intranet<");
    CALL_NC(TRUE, L"Network Connection");

    test_BadLetters();
}
