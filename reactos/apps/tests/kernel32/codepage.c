/*
 * Unit tests for code page to/from unicode translations
 *
 * Copyright (c) 2002 Dmitry Timoshkov
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

/* lstrcmpW is not supported on Win9x! */
static int mylstrcmpW(const WCHAR* str1, const WCHAR* str2)
{
    while (*str1 && *str1==*str2) {
        str1++;
        str2++;
    }
    return *str1-*str2;
}

static void test_negative_source_length(void)
{
    int len;
    char buf[10];
    WCHAR bufW[10];
    static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};

    /* Test, whether any negative source length works as strlen() + 1 */
    SetLastError( 0xdeadbeef );
    memset(buf,'x',sizeof(buf));
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -2002, buf, 10, NULL, NULL);
    ok(len == 7 && !lstrcmpA(buf, "foobar") && GetLastError() == 0xdeadbeef,
       "WideCharToMultiByte(-2002): len=%d error=%ld\n",len,GetLastError());

    SetLastError( 0xdeadbeef );
    memset(bufW,'x',sizeof(bufW));
    len = MultiByteToWideChar(CP_ACP, 0, "foobar", -2002, bufW, 10);
    ok(len == 7 && !mylstrcmpW(bufW, foobarW) && GetLastError() == 0xdeadbeef,
       "MultiByteToWideChar(-2002): len=%d error=%ld\n",len,GetLastError());
}

START_TEST(codepage)
{
    test_negative_source_length();
}
