/*
 * Copyright 2017 Jared Smudde
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

/* Documentation: https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-pathisuncserversharea */

#include <apitest.h>
#include <shlwapi.h>

#define DO_TEST(exp, str) \
do { \
    BOOL ret = PathIsUNCServerShareW((str)); \
    ok(ret == (exp), "Expected %s to be %d, was %d\n", wine_dbgstr_w((str)), (exp), ret); \
} while (0)

START_TEST(isuncpathservershare)
{
    DO_TEST(TRUE, L"\\\\server\\share");
    DO_TEST(TRUE, L"\\\\reactos\\folder9");
    DO_TEST(FALSE, L"\\\\");
    DO_TEST(FALSE, L"reactos\\some\\folder");
    DO_TEST(FALSE, L"////server//share");
    DO_TEST(FALSE, L"c:\\path1");
    DO_TEST(FALSE, (wchar_t*)NULL);
    DO_TEST(FALSE, L"");
    DO_TEST(FALSE, L" ");
    DO_TEST(FALSE, L"\\\\?");
}
