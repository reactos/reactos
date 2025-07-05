/* Unit test suite for FormatMessageA/W
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
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

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#define ULL(a,b)   (((ULONG64)(a) << 32) | (b))

static DWORD WINAPIV doit(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
                          LPSTR out, DWORD outsize, ... )
{
    va_list list;
    DWORD r;

    va_start(list, outsize);
    r = FormatMessageA(flags, src, msg_id,
        lang_id, out, outsize, &list);
    va_end(list);
    return r;
}

static DWORD WINAPIV doitW(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
                           LPWSTR out, DWORD outsize, ... )
{
    va_list list;
    DWORD r;

    va_start(list, outsize);
    r = FormatMessageW(flags, src, msg_id,
        lang_id, out, outsize, &list);
    va_end(list);
    return r;
}

static void test_message_from_string_wide(void)
{
    WCHAR out[0x100] = {0};
    DWORD r, error;

    /* the basics */
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%ld\n", r);

    /* null string, crashes on Windows */
    if (0)
    {
        SetLastError(0xdeadbeef);
        lstrcpyW( out, L"xxxxxx" );
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING, NULL, 0, 0, out, ARRAY_SIZE(out), NULL);
    }

    /* empty string */
    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"", 0, 0, out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(!lstrcmpW(L"", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(error == ERROR_NO_WORK_DONE || broken(error == 0xdeadbeef), "last error %lu\n", error);

    /* format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"%", 0, 0, out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    /* test string with format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"test%", 0, 0, out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(!lstrcmpW(out, L"testxx") || broken(!lstrcmpW( out, L"xxxxxx" )), /* winxp */
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    /* insertion with no variadic arguments */
    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"%1", 0, 0, out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, L"%1", 0,
                       0, out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    /* using the format feature */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!s!", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* no format */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* two pieces */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1%2", 0, 0, out, ARRAY_SIZE(out), L"te", L"st");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* three pieces */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1%3%2%1", 0, 0, out, ARRAY_SIZE(out), L"t", L"s", L"e");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* ls is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!ls!", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%ld\n", r);

    /* S is ansi */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!S!", 0, 0, out, ARRAY_SIZE(out), "test");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%ld\n", r);

    /* ws is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!ws!", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%ld\n", r);

    /* as characters */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!c!%2!c!%3!c!%1!c!", 0, 0, out, ARRAY_SIZE(out), 't', 'e', 's');
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* lc is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!lc!%2!lc!%3!lc!%1!lc!", 0, 0, out, ARRAY_SIZE(out), 't', 'e', 's');
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* wc is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!wc!%2!wc!%3!wc!%1!wc!", 0, 0, out, ARRAY_SIZE(out), 't', 'e', 's');
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* C is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!C!%2!C!%3!C!%1!C!", 0, 0, out, ARRAY_SIZE(out), 't', 'e', 's');
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* some numbers */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!d!%2!d!%3!d!", 0, 0, out, ARRAY_SIZE(out), 1, 2, 3);
    ok(!lstrcmpW(L"123", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3,"failed: r=%ld\n", r);

    /* a single digit with some spacing */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!4d!", 0, 0, out, ARRAY_SIZE(out), 1);
    ok(!lstrcmpW(L"   1", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* a single digit, left justified */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!-4d!", 0, 0, out, ARRAY_SIZE(out), 1);
    ok(!lstrcmpW(L"1   ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* two digit decimal number */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!4d!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!lstrcmpW(L"  11", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* a hex number */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!4x!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!lstrcmpW(L"   b", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* a hex number, upper case */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!4X!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!lstrcmpW(L"   B", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* a hex number, upper case, left justified */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!-4X!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!lstrcmpW(L"B   ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* a long hex number, upper case */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!4X!", 0, 0, out, ARRAY_SIZE(out), 0x1ab);
    ok(!lstrcmpW(L" 1AB", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* two percent... */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L" %%%% ", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L" %% ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* periods are special cases */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L" %.%.  %1!d!", 0, 0, out, ARRAY_SIZE(out), 0x1ab);
    ok(!lstrcmpW(L" ..  427", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==8,"failed: r=%ld\n", r);

    /* %0 ends the line */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"test%0test", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"test", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* %! prints an exclamation */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"yah%!%0   ", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"yah!", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* %space */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"% %   ", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"    ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* %n yields \r\n, %r yields \r, %t yields \t */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%n%r%t", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"\r\n\r\t", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"hi\n", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"hi\r\n", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"hi\r\n", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"hi\r\n", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* carriage return */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"\r", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"\r\n", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==2,"failed: r=%ld\n", r);

    /* double carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"\r\r\n", 0, 0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"\r\n\r\n", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n", r);

    /* null string as argument */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!lstrcmpW(L"(null)", out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==6,"failed: r=%ld\n",r);

    /* precision and width */

    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!3s!", 0, 0, out, ARRAY_SIZE(out), L"t" );
    ok(!lstrcmpW(L"  t", out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==3, "failed: r=%ld\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!*s!", 0, 0, out, ARRAY_SIZE(out), 4, L"t" );
    ok(!lstrcmpW( L"   t", out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!4.2u!", 0, 0, out, ARRAY_SIZE(out), 3 );
    ok(!lstrcmpW( L"  03", out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%ld\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!*.*u!", 0, 0, out, ARRAY_SIZE(out), 5, 3, 1 );
    ok(!lstrcmpW( L"  001", out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==5,"failed: r=%ld\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!*.*u!,%1!*.*u!", 0, 0, out, ARRAY_SIZE(out),
              5, 3, 1, 4, 2 );
    ok(!lstrcmpW( L"  001, 0002", out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==11,"failed: r=%ld\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!*.*u!,%3!*.*u!", 0, 0, out, ARRAY_SIZE(out),
              5, 3, 1, 6, 4, 2 );
    ok(!lstrcmpW( L"  001,  0002", out) ||
       broken(!lstrcmpW(L"  001,000004", out)), /* NT4/Win2k */
       "failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==12,"failed: r=%ld\n",r);
    /* args are not counted the same way with an argument array */
    {
        ULONG_PTR args[] = { 6, 4, 2, 5, 3, 1 };
        r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, L"%1!*.*u!,%1!*.*u!",
                           0, 0, out, ARRAY_SIZE(out), (va_list *)args );
        ok(!lstrcmpW(L"  0002, 00003", out),"failed out=[%s]\n", wine_dbgstr_w(out));
        ok(r==13,"failed: r=%ld\n",r);
        r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, L"%1!*.*u!,%4!*.*u!",
                           0, 0, out, ARRAY_SIZE(out), (va_list *)args );
        ok(!lstrcmpW(L"  0002,  001", out),"failed out=[%s]\n", wine_dbgstr_w(out));
        ok(r==12,"failed: r=%ld\n",r);
    }

    /* change of pace... test the low byte of dwflags */

    /* line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, L"hi\n", 0,
              0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"hi ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3,"failed: r=%ld\n", r);

    /* carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, L"hi\r\n", 0,
              0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"hi ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3,"failed: r=%ld\n", r);

    /* carriage return */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, L"\r", 0,
              0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L" ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==1,"failed: r=%ld\n", r);

    /* double carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, L"\r\r\n", 0,
              0, out, ARRAY_SIZE(out));
    ok(!lstrcmpW(L"  ", out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==2,"failed: r=%ld\n", r);
}

static void test_message_from_string(void)
{
    CHAR out[0x100] = {0};
    DWORD r;
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x', 'x'};

    /* the basics */
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, ARRAY_SIZE(out),NULL);
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* null string, crashes on Windows */
    if (0)
    {
        SetLastError(0xdeadbeef);
        memcpy(out, init_buf, sizeof(init_buf));
        FormatMessageA(FORMAT_MESSAGE_FROM_STRING, NULL, 0, 0, out, ARRAY_SIZE(out), NULL);
    }

    /* empty string */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(GetLastError() == ERROR_NO_WORK_DONE || broken(GetLastError() == 0xdeadbeef),
       "last error %lu\n", GetLastError());

    /* format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "%", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,
       "last error %lu\n", GetLastError());

    /* test string with format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test%", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,
       "last error %lu\n", GetLastError());

    /* insertion with no variadic arguments */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "%1", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "last error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, "%1", 0,
                       0, out, ARRAY_SIZE(out), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%ld\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "last error %lu\n", GetLastError());

    /* using the format feature */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!s!", 0, 0, out, ARRAY_SIZE(out), "test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* no format */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1", 0, 0, out, ARRAY_SIZE(out), "test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* two pieces */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1%2", 0, 0, out, ARRAY_SIZE(out), "te","st");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* three pieces */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1%3%2%1", 0, 0, out, ARRAY_SIZE(out), "t","s","e");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* s is ansi */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!s!", 0, 0, out, ARRAY_SIZE(out), "test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* ls is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!ls!", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* S is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!S!", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* ws is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!ws!", 0, 0, out, ARRAY_SIZE(out), L"test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* as characters */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!c!%2!c!%3!c!%1!c!", 0, 0, out, ARRAY_SIZE(out),
             't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* lc is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!lc!%2!lc!%3!lc!%1!lc!", 0, 0, out, ARRAY_SIZE(out),
             't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* wc is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!wc!%2!wc!%3!wc!%1!wc!", 0, 0, out, ARRAY_SIZE(out),
             't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* C is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!C!%2!C!%3!C!%1!C!", 0, 0, out, ARRAY_SIZE(out),
             't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* some numbers */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!d!%2!d!%3!d!", 0, 0, out, ARRAY_SIZE(out), 1,2,3);
    ok(!strcmp("123", out),"failed out=[%s]\n",out);
    ok(r==3,"failed: r=%ld\n",r);

    /* a single digit with some spacing */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4d!", 0, 0, out, ARRAY_SIZE(out), 1);
    ok(!strcmp("   1", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* a single digit, left justified */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!-4d!", 0, 0, out, ARRAY_SIZE(out), 1);
    ok(!strcmp("1   ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* two digit decimal number */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4d!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!strcmp("  11", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* a hex number */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4x!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!strcmp("   b", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* a hex number, upper case */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4X!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!strcmp("   B", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* a hex number, upper case, left justified */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!-4X!", 0, 0, out, ARRAY_SIZE(out), 11);
    ok(!strcmp("B   ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* a long hex number, upper case */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4X!", 0, 0, out, ARRAY_SIZE(out), 0x1ab);
    ok(!strcmp(" 1AB", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* two percent... */
    r = doit(FORMAT_MESSAGE_FROM_STRING, " %%%% ", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp(" %% ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* periods are special cases */
    r = doit(FORMAT_MESSAGE_FROM_STRING, " %.%. %1!d!", 0, 0, out, ARRAY_SIZE(out), 0x1ab);
    ok(!strcmp(" .. 427", out),"failed out=[%s]\n",out);
    ok(r==7,"failed: r=%ld\n",r);

    /* %0 ends the line */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "test%0test", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* %! prints an exclamation */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "yah%!%0   ", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("yah!", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* %space */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "% %   ", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("    ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* %n yields \r\n, %r yields \r, %t yields \t */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%n%r%t", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("\r\n\r\t", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "hi\n", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("hi\r\n", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "hi\r\n", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("hi\r\n", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* carriage return */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "\r", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("\r\n", out),"failed out=[%s]\n",out);
    ok(r==2,"failed: r=%ld\n",r);

    /* double carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "\r\r\n", 0, 0, out, ARRAY_SIZE(out));
    ok(!strcmp("\r\n\r\n", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%ld\n",r);

    /* null string as argument */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!strcmp("(null)", out),"failed out=[%s]\n",out);
    ok(r==6,"failed: r=%ld\n",r);

    /* precision and width */

    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!3s!",
             0, 0, out, sizeof(out), "t" );
    ok(!strcmp("  t", out),"failed out=[%s]\n",out);
    ok(r==3, "failed: r=%ld\n",r);
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*s!",
             0, 0, out, sizeof(out), 4, "t");
    if (!strcmp("*s",out)) win_skip( "width/precision not supported\n" );
    else
    {
        ok(!strcmp( "   t", out),"failed out=[%s]\n",out);
        ok(r==4,"failed: r=%ld\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4.2u!",
                 0, 0, out, sizeof(out), 3 );
        ok(!strcmp( "  03", out),"failed out=[%s]\n",out);
        ok(r==4,"failed: r=%ld\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*.*u!",
                 0, 0, out, sizeof(out), 5, 3, 1 );
        ok(!strcmp( "  001", out),"failed out=[%s]\n",out);
        ok(r==5,"failed: r=%ld\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*.*u!,%1!*.*u!",
                 0, 0, out, sizeof(out), 5, 3, 1, 4, 2 );
        ok(!strcmp( "  001, 0002", out),"failed out=[%s]\n",out);
        ok(r==11,"failed: r=%ld\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*.*u!,%3!*.*u!",
                 0, 0, out, sizeof(out), 5, 3, 1, 6, 4, 2 );
        /* older Win versions marked as broken even though this is arguably the correct behavior */
        /* but the new (brain-damaged) behavior is specified on MSDN */
        ok(!strcmp( "  001,  0002", out) ||
           broken(!strcmp("  001,000004", out)), /* NT4/Win2k */
           "failed out=[%s]\n",out);
        ok(r==12,"failed: r=%ld\n",r);
        /* args are not counted the same way with an argument array */
        {
            ULONG_PTR args[] = { 6, 4, 2, 5, 3, 1 };
            r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               "%1!*.*u!,%1!*.*u!", 0, 0, out, sizeof(out), (va_list *)args );
            ok(!strcmp("  0002, 00003", out),"failed out=[%s]\n",out);
            ok(r==13,"failed: r=%ld\n",r);
            r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               "%1!*.*u!,%4!*.*u!", 0, 0, out, sizeof(out), (va_list *)args );
            ok(!strcmp("  0002,  001", out),"failed out=[%s]\n",out);
            ok(r==12,"failed: r=%ld\n",r);
        }
    }

    /* change of pace... test the low byte of dwflags */

    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\n", 0,
             0, out, ARRAY_SIZE(out));
    ok(!strcmp("hi ", out), "failed out=[%s]\n",out);
    ok(r==3, "failed: r=%ld\n",r);

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\r\n", 0,
             0, out, ARRAY_SIZE(out));
    ok(!strcmp("hi ", out),"failed out=[%s]\n",out);
    ok(r==3,"failed: r=%ld\n",r);

    /* carriage return */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r", 0,
             0, out, ARRAY_SIZE(out));
    ok(!strcmp(" ", out),"failed out=[%s]\n",out);
    ok(r==1,"failed: r=%ld\n",r);

    /* double carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r\r\n", 0,
             0, out, ARRAY_SIZE(out));
    ok(!strcmp("  ", out),"failed out=[%s]\n",out);
    ok(r==2,"failed: r=%ld\n",r);
}

static void test_message_ignore_inserts(void)
{
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x'};

    DWORD ret;
    CHAR out[256];

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("test", out), "Expected output string \"test\", got %s\n", out);

    /* The %0 escape sequence is handled. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test%0", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("test", out), "Expected output string \"test\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test%0test", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("test", out), "Expected output string \"test\", got %s\n", out);

    /* While FormatMessageA returns 0 in this case, no last error code is set. */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "%0test", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %ld\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_NO_WORK_DONE || broken(GetLastError() == 0xdeadbeef),
        "Expected GetLastError() to return ERROR_NO_WORK_DONE, got %lu\n", GetLastError());

    /* Insert sequences are ignored. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test%1%2!*.*s!%99", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 17, "Expected FormatMessageA to return 17, got %ld\n", ret);
    ok(!strcmp("test%1%2!*.*s!%99", out), "Expected output string \"test%%1%%2!*.*s!%%99\", got %s\n", out);

    /* Only the "%n", "%r", and "%t" escape sequences are processed. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "%%% %.%!", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 8, "Expected FormatMessageA to return 8, got %ld\n", ret);
    ok(!strcmp("%%% %.%!", out), "Expected output string \"%%%%%% %%.%%!\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "%n%r%t", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("\r\n\r\t", out), "Expected output string \"\\r\\n\\r\\t\", got %s\n", out);

    /* CRLF characters are processed normally. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "hi\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("hi\r\n", out), "Expected output string \"hi\\r\\n\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "hi\r\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("hi\r\n", out), "Expected output string \"hi\\r\\n\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "\r", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 2, "Expected FormatMessageA to return 2, got %ld\n", ret);
    ok(!strcmp("\r\n", out), "Expected output string \"\\r\\n\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "\r\r\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %ld\n", ret);
    ok(!strcmp("\r\n\r\n", out), "Expected output string \"\\r\\n\\r\\n\", got %s\n", out);

    /* The width parameter is handled the same also. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\n", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(!strcmp("hi ", out), "Expected output string \"hi \", got %s\n", out);
    ok(ret == 3, "Expected FormatMessageA to return 3, got %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\r\n", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 3, "Expected FormatMessageA to return 3, got %ld\n", ret);
    ok(!strcmp("hi ", out), "Expected output string \"hi \", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 1, "Expected FormatMessageA to return 1, got %ld\n", ret);
    ok(!strcmp(" ", out), "Expected output string \" \", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r\r\n", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 2, "Expected FormatMessageA to return 2, got %ld\n", ret);
    ok(!strcmp("  ", out), "Expected output string \"  \", got %s\n", out);
}

static void test_message_ignore_inserts_wide(void)
{
    DWORD ret;
    WCHAR out[256];

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"test", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"test", out), "Expected output string \"test\", got %s\n", wine_dbgstr_w(out));

    /* The %0 escape sequence is handled. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"test%0", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"test", out), "Expected output string \"test\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"test%0test", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"test", out), "Expected output string \"test\", got %s\n", wine_dbgstr_w(out));

    /* While FormatMessageA returns 0 in this case, no last error code is set. */
    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"%0test", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %ld\n", ret);
    ok(!lstrcmpW(L"", out), "Expected the output buffer to be the empty string, got %s\n", wine_dbgstr_w(out));
    ok(GetLastError() == ERROR_NO_WORK_DONE || broken(GetLastError() == 0xdeadbeef),
      "Expected GetLastError() to return ERROR_NO_WORK_DONE, got %lu\n", GetLastError());

    /* Insert sequences are ignored. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"test%1%2!*.*s!%99", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %ld\n", ret);
    ok(!lstrcmpW(L"test%1%2!*.*s!%99", out), "Expected output string \"test%%1%%2!*.*s!%%99\", got %s\n", wine_dbgstr_w(out));

    /* Only the "%n", "%r", and "%t" escape sequences are processed. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"%%% %.%!", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 8, "Expected FormatMessageW to return 8, got %ld\n", ret);
    ok(!lstrcmpW(L"%%% %.%!", out), "Expected output string \"%%%%%% %%.%%!\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"%n%r%t", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"\r\n\r\t", out), "Expected output string \"\\r\\n\\r\\t\", got %s\n", wine_dbgstr_w(out));

    /* CRLF characters are processed normally. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"hi\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"hi\r\n", out), "Expected output string \"hi\\r\\n\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"hi\r\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"hi\r\n", out), "Expected output string \"hi\\r\\n\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"\r", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 2, "Expected FormatMessageW to return 2, got %ld\n", ret);
    ok(!lstrcmpW(L"\r\n", out), "Expected output string \"\\r\\n\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, L"\r\r\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %ld\n", ret);
    ok(!lstrcmpW(L"\r\n\r\n", out), "Expected output string \"\\r\\n\\r\\n\", got %s\n", wine_dbgstr_w(out));

    /* The width parameter is handled the same also. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, L"hi\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 3, "Expected FormatMessageW to return 3, got %ld\n", ret);
    ok(!lstrcmpW(L"hi ", out), "Expected output string \"hi \", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, L"hi\r\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 3, "Expected FormatMessageW to return 3, got %ld\n", ret);
    ok(!lstrcmpW(L"hi ", out), "Expected output string \"hi \", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, L"\r", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 1, "Expected FormatMessageW to return 1, got %ld\n", ret);
    ok(!lstrcmpW(L" ", out), "Expected output string \" \", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, L"\r\r\n", 0, 0, out,
                         ARRAY_SIZE(out), NULL);
    ok(ret == 2, "Expected FormatMessageW to return 2, got %ld\n", ret);
    ok(!lstrcmpW(L"  ", out), "Expected output string \"  \", got %s\n", wine_dbgstr_w(out));
}

static void test_message_wrap(void)
{
    DWORD ret;
    int i;
    CHAR in[300], out[300], ref[300];

    /* No need for wrapping */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 20,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 15, "Expected FormatMessageW to return 15, got %ld\n", ret);
    ok(!strcmp("short long line", out),"failed out=[%s]\n",out);

    /* Wrap the last word */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short long\r\nline", out),"failed out=[%s]\n",out);

    /* Wrap the very last word */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 20,
                         "short long long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 21, "Expected FormatMessageW to return 21, got %ld\n", ret);
    ok(!strcmp("short long long\r\nline", out),"failed out=[%s]\n",out);

    /* Strictly less than 10 characters per line! */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 10,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong line", out),"failed out=[%s]\n",out);

    /* Handling of duplicate spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 16,
                         "short long    line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short long\r\nline", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 16,
                         "short long    wordlongerthanaline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 33, "Expected FormatMessageW to return 33, got %ld\n", ret);
    ok(!strcmp("short long\r\nwordlongerthanal\r\nine", out),"failed out=[%s]\n",out);

    /* Breaking in the middle of spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 12,
                         "short long    line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 18, "Expected FormatMessageW to return 18, got %ld\n", ret);
    ok(!strcmp("short long\r\n  line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 12,
                         "short long    wordlongerthanaline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 35, "Expected FormatMessageW to return 35, got %ld\n", ret);
    ok(!strcmp("short long\r\n\r\nwordlongerth\r\nanaline", out),"failed out=[%s]\n",out);

    /* Handling of start-of-string spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 15,
                         "   short line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 13, "Expected FormatMessageW to return 13, got %ld\n", ret);
    ok(!strcmp("   short line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "   shortlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %ld\n", ret);
    ok(!strcmp("\r\nshortlong\r\nline", out),"failed out=[%s]\n",out);

    /* Handling of start-of-line spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "l1%n   shortlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 21, "Expected FormatMessageW to return 21, got %ld\n", ret);
    ok(!strcmp("l1\r\n\r\nshortlong\r\nline", out),"failed out=[%s]\n",out);

    /* Pure space wrapping */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 5,
                         "                ", 0, 0, out, sizeof(out), NULL);
    ok(ret == 7, "Expected FormatMessageW to return 7, got %ld\n", ret);
    ok(!strcmp("\r\n\r\n\r\n ", out),"failed out=[%s]\n",out);

    /* Handling of trailing spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 5,
                         "l1               ", 0, 0, out, sizeof(out), NULL);
    ok(ret == 10, "Expected FormatMessageW to return 10, got %ld\n", ret);
    ok(!strcmp("l1\r\n\r\n\r\n  ", out),"failed out=[%s]\n",out);

    /* Word that just fills the line */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "shortlon", 0, 0, out, sizeof(out), NULL);
    ok(ret == 10, "Expected FormatMessageW to return 10, got %ld\n", ret);
    ok(!strcmp("shortlon\r\n", out),"failed out=[%s]\n",out);

    /* Word longer than the line */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "shortlongline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 15, "Expected FormatMessageW to return 15, got %ld\n", ret);
    ok(!strcmp("shortlon\r\ngline", out),"failed out=[%s]\n",out);

    /* Wrap the line multiple times */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 7,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %ld\n", ret);
    ok(!strcmp("short\r\nlong\r\nline", out),"failed out=[%s]\n",out);

    /* '\n's in the source are ignored */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short\nlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short long\r\nline", out),"failed out=[%s]\n",out);

    /* Wrap even before a '%n' */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "shortlon%n", 0, 0, out, sizeof(out), NULL);
    ok(ret == 12, "Expected FormatMessageW to return 12, got %ld\n", ret);
    ok(!strcmp("shortlon\r\n\r\n", out),"failed out=[%s]\n",out);

    /* '%n's count as starting a new line and combine with line wrapping */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 10,
                         "short%nlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "short%nlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %ld\n", ret);
    ok(!strcmp("short\r\nlong\r\nline", out),"failed out=[%s]\n",out);

    /* '%r's also count as starting a new line and all */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 10,
                         "short%rlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 15, "Expected FormatMessageW to return 15, got %ld\n", ret);
    ok(!strcmp("short\rlong line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "short%rlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\rlong\r\nline", out),"failed out=[%s]\n",out);

    /* IGNORE_INSERTS does not prevent line wrapping or disable '%n' */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS | 8,
                         "short%nlong line%1", 0, 0, out, sizeof(out), NULL);
    ok(ret == 19, "Expected FormatMessageW to return 19, got %ld\n", ret);
    ok(!strcmp("short\r\nlong\r\nline%1", out),"failed out=[%s]\n",out);

    /* MAX_WIDTH_MASK is the same as specifying an infinite line width */
    strcpy(in, "first line%n");
    strcpy(ref, "first line\r\n");
    for (i=0; i < 26; i++)
    {
        strcat(in, "123456789 ");
        strcat(ref, "123456789 ");
    }
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                         in, 0, 0, out, sizeof(out), NULL);
    ok(ret == 272, "Expected FormatMessageW to return 272, got %ld\n", ret);
    ok(!strcmp(ref, out),"failed out=[%s]\n",out);

    /* Wrapping and non-space characters */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long\tline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong\tline", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long-line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong-line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long_line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong_line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long.line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong.line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long,line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong,line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long!line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong!line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long?line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %ld\n", ret);
    ok(!strcmp("short\r\nlong?line", out),"failed out=[%s]\n",out);
}

static void test_message_arg_eaten( const WCHAR *src, ... )
{
    DWORD ret;
    va_list list;
    WCHAR *arg, out[1];

    out[0] = 0xcccc;
    va_start(list, src);
    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, src, 0, 0, out, ARRAY_SIZE(out), &list);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(out[0] == 0, "Expected null, got %ls\n", out);
    arg = va_arg( list, WCHAR * );
    ok(!wcscmp( L"unused", arg ), "Expected 'unused', got %s\n", wine_dbgstr_w(arg));
    va_end(list);
}

static void test_message_insufficient_buffer(void)
{
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x'};
    static const char expected_buf[] = {'x', 'x', 'x', 'x', 'x'};
    DWORD ret, size, i;
    CHAR out[5];

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, ARRAY_SIZE(out) - 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");

    for (size = 32700; size < 32800; size++)
    {
        char *tmp = HeapAlloc( GetProcessHeap(), 0, size );
        char *buf = HeapAlloc( GetProcessHeap(), 0, size );

        for (i = 0; i < size; i++) tmp[i] = 'A' + i % 26;
        tmp[size - 1] = 0;
        SetLastError( 0xdeadbeef );
        ret = FormatMessageA( FORMAT_MESSAGE_FROM_STRING, tmp, 0, 0, buf, size, NULL );
        if (size < 32768)
        {
            ok( ret == size - 1, "%lu: got %lu\n", size, ret );
            ok( !strcmp( tmp, buf ), "wrong buffer\n" );
        }
        else
        {
            ok( ret == 0, "%lu: got %lu\n", size, ret );
            ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        }

        SetLastError( 0xdeadbeef );
        ret = doit( FORMAT_MESSAGE_FROM_STRING, "%1", 0, 0, buf, size, tmp );
        if (size < 32768)
        {
            ok( ret == size - 1, "%lu: got %lu\n", size, ret );
            ok( !strcmp( tmp, buf ), "wrong buffer\n" );
        }
        else
        {
            ok( ret == 0, "%lu: got %lu\n", size, ret );
            ok( GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == ERROR_MORE_DATA), /* winxp */
                "wrong error %lu\n", GetLastError() );
        }
        HeapFree( GetProcessHeap(), 0, buf );

        SetLastError( 0xdeadbeef );
        ret = FormatMessageA( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              tmp, 0, 0, (char *)&buf, size, NULL );
        if (size < 32768)
        {
            ok( ret == size - 1, "%lu: got %lu\n", size, ret );
            ok( !strcmp( tmp, buf ), "wrong buffer\n" );
            HeapFree( GetProcessHeap(), 0, buf );
        }
        else
        {
            ok( ret == 0, "%lu: got %lu\n", size, ret );
            ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        }
        HeapFree( GetProcessHeap(), 0, tmp );
    }
}

static void test_message_insufficient_buffer_wide(void)
{
    DWORD ret, size, i;
    WCHAR out[8];

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"test", 0, 0, out, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n",
       GetLastError());
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the buffer to be untouched\n");

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"test", 0, 0, out, 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n",
       GetLastError());
    ok(!memcmp(out, L"\0xxxxx", 6 * sizeof(WCHAR)) ||
        broken(!lstrcmpW( out, L"xxxxxx" )), /* winxp */
       "Expected the buffer to be truncated\n");

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, L"test", 0, 0, out, 4, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n",
       GetLastError());
    ok(!memcmp(out, L"tes\0xx", 6 * sizeof(WCHAR)) ||
        broken(!lstrcmpW( out, L"xxxxxx" )), /* winxp */
        "Expected the buffer to be truncated\n");

    for (size = 1000; size < 1000000; size += size / 10)
    {
        WCHAR *tmp = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );
        WCHAR *buf = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );
        for (i = 0; i < size; i++) tmp[i] = 'A' + i % 26;
        tmp[size - 1] = 0;
        ret = FormatMessageW( FORMAT_MESSAGE_FROM_STRING, tmp, 0, 0, buf, size, NULL );
        ok(ret == size - 1 || broken(!ret), /* winxp */ "got %lu\n", ret);
        if (!ret) break;
        ok( !lstrcmpW( tmp, buf ), "wrong buffer\n" );
        ret = doitW( FORMAT_MESSAGE_FROM_STRING, L"%1", 0, 0, buf, size, tmp );
        ok(ret == size - 1, "got %lu\n", ret);
        ok( !lstrcmpW( tmp, buf ), "wrong buffer\n" );
        HeapFree( GetProcessHeap(), 0, buf );
        ret = FormatMessageW( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                              tmp, 0, 0, (WCHAR *)&buf, size, NULL );
        ok(ret == size - 1, "got %lu\n", ret);
        ok( !lstrcmpW( tmp, buf ), "wrong buffer\n" );
        HeapFree( GetProcessHeap(), 0, tmp );
        HeapFree( GetProcessHeap(), 0, buf );
    }

    /* va_arg is eaten even in case of insufficient buffer */
    test_message_arg_eaten( L"%1!s! %2!s!", L"eaten", L"unused" );
}

static void test_message_null_buffer(void)
{
    DWORD ret, error;

    /* Without FORMAT_MESSAGE_ALLOCATE_BUFFER, only the specified buffer size is checked. */
    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %lu\n", ret);
    ok(error == ERROR_INSUFFICIENT_BUFFER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %lu\n", ret);
    ok(error == ERROR_INSUFFICIENT_BUFFER, "last error %lu\n", error);

    if (0) /* crashes on Windows */
    {
        SetLastError(0xdeadbeef);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 256, NULL);
    }

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %lu\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %lu\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 256, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %lu\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY, "last error %lu\n", error);
}

static void test_message_null_buffer_wide(void)
{
    DWORD ret, error;

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %lu\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %lu\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 256, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %lu\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %lu\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %lu\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 256, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %lu\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %lu\n", error);
}

static void test_message_allocate_buffer(void)
{
    DWORD ret;
    char *buf;

    /* While MSDN suggests that FormatMessageA allocates a buffer whose size is
     * the larger of the output string and the requested buffer size, the tests
     * will not try to determine the actual size of the buffer allocated, as
     * the return value of LocalSize cannot be trusted for the purpose, and it should
     * in any case be safe for FormatMessageA to allocate in the manner that
     * MSDN suggests. */

    SetLastError(0xdeadbeef);
    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "", 0, 0, (char *)&buf, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(buf == NULL, "Expected output buffer pointer to be NULL\n");
    ok(GetLastError() == ERROR_NO_WORK_DONE || broken(GetLastError() == 0xdeadbeef),
       "Expected GetLastError() to return ERROR_NO_WORK_DONE, got %lu\n", GetLastError());

    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "test", 0, 0, (char *)&buf, 0, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (char *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (char *)0xdeadbeef)
    {
        ok(!strcmp("test", buf),
           "Expected buffer to contain \"test\", got %s\n", buf);
        LocalFree(buf);
    }

    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "test", 0, 0, (char *)&buf, strlen("test"), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (char *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (char *)0xdeadbeef)
    {
        ok(!strcmp("test", buf),
           "Expected buffer to contain \"test\", got %s\n", buf);
        LocalFree(buf);
    }

    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "test", 0, 0, (char *)&buf, strlen("test") + 1, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (char *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
   if (buf != NULL && buf != (char *)0xdeadbeef)
    {
        ok(!strcmp("test", buf),
           "Expected buffer to contain \"test\", got %s\n", buf);
        LocalFree(buf);
    }

    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "test", 0, 0, (char *)&buf, strlen("test") + 2, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (char *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (char *)0xdeadbeef)
    {
        ok(!strcmp("test", buf),
           "Expected buffer to contain \"test\", got %s\n", buf);
        LocalFree(buf);
    }

    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "test", 0, 0, (char *)&buf, 1024, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (char *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (char *)0xdeadbeef)
    {
        ok(!strcmp("test", buf),
           "Expected buffer to contain \"test\", got %s\n", buf);
        LocalFree(buf);
    }
}

static void test_message_allocate_buffer_wide(void)
{
    DWORD ret;
    WCHAR *buf;

    /* While MSDN suggests that FormatMessageW allocates a buffer whose size is
     * the larger of the output string and the requested buffer size, the tests
     * will not try to determine the actual size of the buffer allocated, as
     * the return value of LocalSize cannot be trusted for the purpose, and it should
     * in any case be safe for FormatMessageW to allocate in the manner that
     * MSDN suggests. */

    if (0) /* crashes on Windows */
    {
        buf = (WCHAR *)0xdeadbeef;
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                             NULL, 0, 0, (WCHAR *)&buf, 0, NULL);
    }

    SetLastError(0xdeadbeef);
    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         L"", 0, 0, (WCHAR *)&buf, 0, NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(buf == NULL, "Expected output buffer pointer to be NULL\n");
    ok(GetLastError() == ERROR_NO_WORK_DONE || broken(GetLastError() == 0xdeadbeef),
       "Expected GetLastError() to return ERROR_NO_WORK_DONE, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    buf = (WCHAR *)0xdeadbeef;
    ret = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER, L"%1",
                0, 0, (WCHAR *)&buf, 0, L"" );
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(buf == NULL, "Expected output buffer pointer to be NULL\n");
    ok(GetLastError() == ERROR_NO_WORK_DONE || broken(GetLastError() == 0xdeadbeef),
       "Expected GetLastError() to return ERROR_NO_WORK_DONE, got %lu\n", GetLastError());

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         L"test", 0, 0, (WCHAR *)&buf, 0, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(L"test", buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         L"test", 0, 0, (WCHAR *)&buf, 4, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(L"test", buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         L"test", 0, 0, (WCHAR *)&buf, 5, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(L"test", buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         L"test", 0, 0, (WCHAR *)&buf, 6, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(L"test", buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         L"test", 0, 0, (WCHAR *)&buf, 1024, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(L"test", buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }
}

static void test_message_from_hmodule(void)
{
    DWORD ret, error;
    HMODULE h;
    CHAR out[0x100] = {0};

    h = GetModuleHandleA("kernel32.dll");
    ok(h != 0, "GetModuleHandle failed\n");

    /*Test existing messageID; as the message strings from wine's kernel32 differ from windows' kernel32 we don't compare
    the strings but only test that FormatMessage doesn't return 0*/
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 7/*=ERROR_ARENA_TRASHED*/,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, ARRAY_SIZE(out), NULL);
    ok(ret != 0, "FormatMessageA returned 0\n");

    /* Test HRESULT. It's not documented but in practice _com_error::ErrorMessage relies on this. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 0x80070005 /* E_ACCESSDENIED */,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, ARRAY_SIZE(out), NULL);
    ok(ret != 0, "FormatMessageA returned 0\n");

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, TRUST_E_NOSIGNATURE,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, ARRAY_SIZE(out), NULL);
    ok(ret != 0, "FormatMessageA returned 0\n");

    /* Test a message string with an insertion without passing any variadic arguments. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 193 /* ERROR_BAD_EXE_FORMAT */,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "FormatMessageA returned non-zero\n");

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY, h, 193 /* ERROR_BAD_EXE_FORMAT */,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "FormatMessageA returned non-zero\n");

    /*Test nonexistent messageID with varying language IDs Note: FormatMessageW behaves the same*/
    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %lu instead of 0\n", ret);
    ok(error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_FOUND ||
       error == ERROR_RESOURCE_TYPE_NOT_FOUND, "Unexpected last error %lu.\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %lu instead of 0\n", ret);
    ok(error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_LOADED ||
       error == ERROR_RESOURCE_TYPE_NOT_FOUND, "Unexpected last error %lu.\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %lu instead of 0\n", ret);
    ok(error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_LOADED ||
       error == ERROR_RESOURCE_TYPE_NOT_FOUND, "Unexpected last error %lu.\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %lu instead of 0\n", ret);
    ok(error == ERROR_RESOURCE_LANG_NOT_FOUND ||
       error == ERROR_RESOURCE_TYPE_NOT_FOUND ||
       error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_LOADED,
       "last error %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK), out, ARRAY_SIZE(out), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %lu instead of 0\n", ret);
    ok(error == ERROR_RESOURCE_LANG_NOT_FOUND ||
       error == ERROR_RESOURCE_TYPE_NOT_FOUND ||
       error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_LOADED,
       "last error %lu\n", error);
}

static void test_message_invalid_flags(void)
{
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x'};

    DWORD ret;
    CHAR out[5];
    char *ptr;

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(0, "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ptr = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER, "test", 0, 0, (char *)&ptr, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(ptr == NULL, "Expected output pointer to be initialized to NULL, got %p\n", ptr);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS, "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_ARGUMENT_ARRAY, "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK, "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %lu\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    /* Simultaneously setting FORMAT_MESSAGE_FROM_STRING with other source
     * flags is apparently permissible, and FORMAT_MESSAGE_FROM_STRING takes
     * precedence in this case. */

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_SYSTEM,
                         "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(!strcmp("test", out),
       "Expected the output buffer to be untouched\n");

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE,
                         "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(!strcmp("test", out),
       "Expected the output buffer to be untouched\n");

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_FROM_SYSTEM, "test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %lu\n", ret);
    ok(!strcmp("test", out),
       "Expected the output buffer to be untouched\n");
}

static void test_message_invalid_flags_wide(void)
{
    DWORD ret;
    WCHAR out[8];
    WCHAR *ptr;

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(0, L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ptr = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER, L"test", 0, 0, (WCHAR *)&ptr, 0, NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(ptr == NULL, "Expected output pointer to be initialized to NULL, got %p\n", ptr);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS, L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY, L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_MAX_WIDTH_MASK, L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %lu\n", ret);
    ok(!lstrcmpW( out, L"xxxxxx" ),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    /* Simultaneously setting FORMAT_MESSAGE_FROM_STRING with other source
     * flags is apparently permissible, and FORMAT_MESSAGE_FROM_STRING takes
     * precedence in this case. */

    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_SYSTEM,
                         L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %lu\n", ret);
    ok(!lstrcmpW(L"test", out),
       "Expected the output buffer to be untouched\n");

    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE,
                         L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %lu\n", ret);
    ok(!lstrcmpW(L"test", out),
       "Expected the output buffer to be untouched\n");

    lstrcpyW( out, L"xxxxxx" );
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_FROM_SYSTEM, L"test", 0, 0, out, ARRAY_SIZE(out), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %lu\n", ret);
    ok(!lstrcmpW(L"test", out),
       "Expected the output buffer to be untouched\n");
}

static void test_message_from_64bit_number(void)
{
    WCHAR outW[0x100], expW[0x100];
    char outA[0x100];
    DWORD r;
    static const struct
    {
        UINT64 number;
        const char expected[32];
        int len;
    } unsigned_tests[] =
    {
        { 0, "0", 1 },
        { 1234567890, "1234567890", 10},
        { ULL(0xFFFFFFFF,0xFFFFFFFF), "18446744073709551615", 20 },
        { ULL(0x7FFFFFFF,0xFFFFFFFF), "9223372036854775807", 19 },
    };
    static const struct
    {
        INT64 number;
        const char expected[32];
        int len;
    } signed_tests[] =
    {
        { 0, "0" , 1},
        { 1234567890, "1234567890", 10 },
        { -1, "-1", 2},
        { ULL(0xFFFFFFFF,0xFFFFFFFF), "-1", 2},
        { ULL(0x7FFFFFFF,0xFFFFFFFF), "9223372036854775807", 19 },
        { -ULL(0x7FFFFFFF,0xFFFFFFFF), "-9223372036854775807", 20},
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(unsigned_tests); i++)
    {
        r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!I64u!", 0, 0, outW, ARRAY_SIZE(outW),
                  unsigned_tests[i].number);
        MultiByteToWideChar(CP_ACP, 0, unsigned_tests[i].expected, -1, expW, ARRAY_SIZE(expW));
        ok(!lstrcmpW(outW, expW),"[%d] failed, expected %s, got %s\n", i,
                     unsigned_tests[i].expected, wine_dbgstr_w(outW));
        ok(r == unsigned_tests[i].len,"[%d] failed: r=%ld\n", i, r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!I64u!",
                  0, 0, outA, sizeof(outA), unsigned_tests[i].number);
        ok(!strcmp(outA, unsigned_tests[i].expected),"[%d] failed, expected %s, got %s\n", i,
                   unsigned_tests[i].expected, outA);
        ok(r == unsigned_tests[i].len,"[%d] failed: r=%ld\n", i, r);
    }

    for (i = 0; i < ARRAY_SIZE(signed_tests); i++)
    {
        r = doitW(FORMAT_MESSAGE_FROM_STRING, L"%1!I64d!", 0, 0, outW, ARRAY_SIZE(outW),
                  signed_tests[i].number);
        MultiByteToWideChar(CP_ACP, 0, signed_tests[i].expected, -1, expW, ARRAY_SIZE(expW));
        ok(!lstrcmpW(outW, expW),"[%d] failed, expected %s, got %s\n", i,
                     signed_tests[i].expected, wine_dbgstr_w(outW));
        ok(r == signed_tests[i].len,"[%d] failed: r=%ld\n", i, r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!I64d!",
                  0, 0, outA, sizeof(outA), signed_tests[i].number);
        ok(!strcmp(outA, signed_tests[i].expected),"[%d] failed, expected %s, got %s\n", i,
                   signed_tests[i].expected, outA);
        ok(r == signed_tests[i].len,"[%d] failed: r=%ld\n", i, r);
    }
}

static void test_message_system_errors(void)
{
    static const struct
    {
        DWORD error_code;
        BOOL broken;
    }
    tests[] =
    {
        {E_NOTIMPL},
        {E_FAIL},
        {DXGI_ERROR_INVALID_CALL, TRUE /* Available since Win8 */},
        {DXGI_ERROR_NOT_CURRENTLY_AVAILABLE, TRUE /* Available since Win8 */},
    };

    char buffer[256];
    unsigned int i;
    DWORD len;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, tests[i].error_code,
                LANG_USER_DEFAULT, buffer, ARRAY_SIZE(buffer), NULL);
        ok(len || broken(tests[i].broken), "Got zero len, code %#lx.\n", tests[i].error_code);
    }
}

START_TEST(format_msg)
{
    DWORD ret;

    test_message_from_string();
    test_message_ignore_inserts();
    test_message_wrap();
    test_message_insufficient_buffer();
    test_message_null_buffer();
    test_message_allocate_buffer();
    test_message_from_hmodule();
    test_message_invalid_flags();

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, NULL, 0, 0, NULL, 0, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("FormatMessageW is not implemented\n");
        return;
    }

    test_message_from_string_wide();
    test_message_ignore_inserts_wide();
    test_message_insufficient_buffer_wide();
    test_message_null_buffer_wide();
    test_message_allocate_buffer_wide();
    test_message_invalid_flags_wide();
    test_message_from_64bit_number();
    test_message_system_errors();
}
