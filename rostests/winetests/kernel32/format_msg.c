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

static DWORD __cdecl doit(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
                          LPSTR out, DWORD outsize, ... )
{
    __ms_va_list list;
    DWORD r;

    __ms_va_start(list, outsize);
    r = FormatMessageA(flags, src, msg_id,
        lang_id, out, outsize, &list);
    __ms_va_end(list);
    return r;
}

static DWORD __cdecl doitW(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
                           LPWSTR out, DWORD outsize, ... )
{
    __ms_va_list list;
    DWORD r;

    __ms_va_start(list, outsize);
    r = FormatMessageW(flags, src, msg_id,
        lang_id, out, outsize, &list);
    __ms_va_end(list);
    return r;
}

static void test_message_from_string_wide(void)
{
    static const WCHAR test[]        = {'t','e','s','t',0};
    static const WCHAR empty[]       = {0};
    static const WCHAR te[]          = {'t','e',0};
    static const WCHAR st[]          = {'s','t',0};
    static const WCHAR t[]           = {'t',0};
    static const WCHAR e[]           = {'e',0};
    static const WCHAR s[]           = {'s',0};
    static const WCHAR fmt_null[]    = {'%',0};
    static const WCHAR fmt_tnull[]   = {'t','e','s','t','%',0};
    static const WCHAR fmt_1[]       = {'%','1',0};
    static const WCHAR fmt_12[]      = {'%','1','%','2',0};
    static const WCHAR fmt_123[]     = {'%','1','%','3','%','2','%','1',0};
    static const WCHAR fmt_123c[]    = {'%','1','!','c','!','%','2','!','c','!','%','3','!','c','!','%','1','!','c','!',0};
    static const WCHAR fmt_123lc[]   = {'%','1','!','l','c','!','%','2','!','l','c','!','%','3','!','l','c','!','%','1','!','l','c','!',0};
    static const WCHAR fmt_123wc[]   = {'%','1','!','w','c','!','%','2','!','w','c','!','%','3','!','w','c','!','%','1','!','w','c','!',0};
    static const WCHAR fmt_123C[]    = {'%','1','!','C','!','%','2','!','C','!','%','3','!','C','!','%','1','!','C','!',0};
    static const WCHAR fmt_123d[]    = {'%','1','!','d','!','%','2','!','d','!','%','3','!','d','!',0};
    static const WCHAR fmt_1s[]      = {'%','1','!','s','!',0};
    static const WCHAR fmt_s[]       = {'%','!','s','!',0};
    static const WCHAR fmt_ls[]      = {'%','!','l','s','!',0};
    static const WCHAR fmt_ws[]      = {'%','!','w','s','!',0};
    static const WCHAR fmt_S[]       = {'%','!','S','!',0};
    static const WCHAR fmt_14d[]     = {'%','1','!','4','d','!',0};
    static const WCHAR fmt_14x[]     = {'%','1','!','4','x','!',0};
    static const WCHAR fmt_14X[]     = {'%','1','!','4','X','!',0};
    static const WCHAR fmt_1_4X[]    = {'%','1','!','-','4','X','!',0};
    static const WCHAR fmt_1_4d[]    = {'%','1','!','-','4','d','!',0};
    static const WCHAR fmt_2pct[]    = {' ','%','%','%','%',' ',0};
    static const WCHAR fmt_2dot1d[]  = {' ', '%','.','%','.',' ',' ','%','1','!','d','!',0};
    static const WCHAR fmt_t0t[]     = {'t','e','s','t','%','0','t','e','s','t',0};
    static const WCHAR fmt_yah[]     = {'y','a','h','%','!','%','0',' ',' ',' ',0};
    static const WCHAR fmt_space[]   = {'%',' ','%',' ',' ',' ',0};
    static const WCHAR fmt_nrt[]     = {'%','n','%','r','%','t',0};
    static const WCHAR fmt_hi_lf[]   = {'h','i','\n',0};
    static const WCHAR fmt_hi_crlf[] = {'h','i','\r','\n',0};
    static const WCHAR fmt_cr[]      = {'\r',0};
    static const WCHAR fmt_crcrlf[]  = {'\r','\r','\n',0};
    static const WCHAR fmt_13s[]     = {'%','1','!','3','s','!',0};
    static const WCHAR fmt_1os[]     = {'%','1','!','*','s','!',0};
    static const WCHAR fmt_142u[]    = {'%','1','!','4','.','2','u','!',0};
    static const WCHAR fmt_1oou[]    = {'%','1','!','*','.','*','u','!',0};
    static const WCHAR fmt_1oou1oou[] = {'%','1','!','*','.','*','u','!',',','%','1','!','*','.','*','u','!',0};
    static const WCHAR fmt_1oou3oou[] = {'%','1','!','*','.','*','u','!',',','%','3','!','*','.','*','u','!',0};
    static const WCHAR fmt_1oou4oou[] = {'%','1','!','*','.','*','u','!',',','%','4','!','*','.','*','u','!',0};

    static const WCHAR s_123d[]      = {'1','2','3',0};
    static const WCHAR s_14d[]       = {' ',' ',' ','1',0};
    static const WCHAR s_14x[]       = {' ',' ',' ','b',0};
    static const WCHAR s_14X[]       = {' ',' ',' ','B',0};
    static const WCHAR s_1_4X[]      = {'B',' ',' ',' ',0};
    static const WCHAR s_14d2[]      = {' ',' ','1','1',0};
    static const WCHAR s_1_4d[]      = {'1',' ',' ',' ',0};
    static const WCHAR s_1AB[]       = {' ','1','A','B',0};
    static const WCHAR s_2pct[]      = {' ','%','%',' ',0};
    static const WCHAR s_2dot147[]   = {' ','.','.',' ',' ','4','2','7',0};
    static const WCHAR s_yah[]       = {'y','a','h','!',0};
    static const WCHAR s_space[]     = {' ',' ',' ',' ',0};
    static const WCHAR s_nrt[]       = {'\r','\n','\r','\t',0};
    static const WCHAR s_hi_crlf[]   = {'h','i','\r','\n',0};
    static const WCHAR s_crlf[]      = {'\r','\n',0};
    static const WCHAR s_crlfcrlf[]  = {'\r','\n','\r','\n',0};
    static const WCHAR s_hi_sp[]     = {'h','i',' ',0};
    static const WCHAR s_sp[]        = {' ',0};
    static const WCHAR s_2sp[]       = {' ',' ',0};
    static const WCHAR s_spt[]       = {' ',' ','t',0};
    static const WCHAR s_sp3t[]      = {' ',' ',' ','t',0};
    static const WCHAR s_sp03[]      = {' ',' ','0','3',0};
    static const WCHAR s_sp001[]     = {' ',' ','0','0','1',0};
    static const WCHAR s_sp001002[]  = {' ',' ','0','0','1',',',' ','0','0','0','2',0};
    static const WCHAR s_sp001sp002[] = {' ',' ','0','0','1',',',' ',' ','0','0','0','2',0};
    static const WCHAR s_sp002sp001[] = {' ',' ','0','0','0','2',',',' ',' ','0','0','1',0};
    static const WCHAR s_sp002sp003[] = {' ',' ','0','0','0','2',',',' ','0','0','0','0','3',0};
    static const WCHAR s_sp001004[]   = {' ',' ','0','0','1',',','0','0','0','0','0','4',0};
    static const WCHAR s_null[]       = {'(','n','u','l','l',')',0};

    static const WCHAR init_buf[] = {'x', 'x', 'x', 'x', 'x', 'x'};
    static const WCHAR broken_buf[] = {'t','e','s','t','x','x'};

    WCHAR out[0x100] = {0};
    DWORD r, error;

    /* the basics */
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, test, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%d\n", r);

    /* null string, crashes on Windows */
    if (0)
    {
        SetLastError(0xdeadbeef);
        memcpy(out, init_buf, sizeof(init_buf));
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING, NULL, 0,
            0, out, sizeof(out)/sizeof(WCHAR), NULL);
    }

    /* empty string */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, empty, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    error = GetLastError();
    ok(!lstrcmpW(empty, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==0, "succeeded: r=%d\n", r);
    ok(error==0xdeadbeef, "last error %u\n", error);

    /* format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, fmt_null, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    error = GetLastError();
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %u\n", error);

    /* test string with format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, fmt_tnull, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    error = GetLastError();
    ok(!memcmp(out, init_buf, sizeof(init_buf)) ||
       broken(!memcmp(out, broken_buf, sizeof(broken_buf))), /* W2K3+ */
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %u\n", error);

    /* insertion with no variadic arguments */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, fmt_1, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    error = GetLastError();
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, fmt_1, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    error = GetLastError();
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be unchanged\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(error==ERROR_INVALID_PARAMETER, "last error %u\n", error);

    /* using the format feature */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1s, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* no format */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* two pieces */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_12, 0,
        0, out, sizeof(out)/sizeof(WCHAR), te, st);
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* three pieces */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123, 0,
        0, out, sizeof(out)/sizeof(WCHAR), t, s, e);
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* s doesn't seem to work in format strings */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_s, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_s[1], out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3, "failed: r=%d\n", r);

    /* nor ls */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_ls, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_ls[1], out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%d\n", r);

    /* nor S */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_S, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_S[1], out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3, "failed: r=%d\n", r);

    /* nor ws */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_ws, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_ws[1], out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4, "failed: r=%d\n", r);

    /* as characters */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123c, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* lc is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123lc, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* wc is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123wc, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* C is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123C, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* some numbers */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 1, 2, 3);
    ok(!lstrcmpW(s_123d, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3,"failed: r=%d\n", r);

    /* a single digit with some spacing */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 1);
    ok(!lstrcmpW(s_14d, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a single digit, left justified */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1_4d, 0,
        0, out, sizeof(out)/sizeof(CHAR), 1);
    ok(!lstrcmpW(s_1_4d, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* two digit decimal number */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_14d2, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a hex number */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14x, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_14x, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a hex number, upper case */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14X, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_14X, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a hex number, upper case, left justified */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1_4X, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_1_4X, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a long hex number, upper case */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14X, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 0x1ab);
    ok(!lstrcmpW(s_1AB, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* two percent... */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_2pct, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_2pct, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* periods are special cases */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_2dot1d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 0x1ab);
    ok(!lstrcmpW(s_2dot147, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==8,"failed: r=%d\n", r);

    /* %0 ends the line */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_t0t, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(test, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* %! prints an exclamation */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_yah, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_yah, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* %space */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_space, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_space, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* %n yields \r\n, %r yields \r, %t yields \t */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_nrt, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_nrt, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_hi_lf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_crlf, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_hi_crlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_crlf, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* carriage return */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_cr, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_crlf, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==2,"failed: r=%d\n", r);

    /* double carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_crcrlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_crlfcrlf, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* null string as argument */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(!lstrcmpW(s_null, out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==6,"failed: r=%d\n",r);

    /* precision and width */

    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_13s,
              0, 0, out, sizeof(out)/sizeof(WCHAR), t );
    ok(!lstrcmpW(s_spt, out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==3, "failed: r=%d\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1os,
              0, 0, out, sizeof(out)/sizeof(WCHAR), 4, t );
    ok(!lstrcmpW( s_sp3t, out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_142u,
              0, 0, out, sizeof(out)/sizeof(WCHAR), 3 );
    ok(!lstrcmpW( s_sp03, out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==4,"failed: r=%d\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1oou,
              0, 0, out, sizeof(out)/sizeof(WCHAR), 5, 3, 1 );
    ok(!lstrcmpW( s_sp001, out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==5,"failed: r=%d\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1oou1oou,
              0, 0, out, sizeof(out)/sizeof(WCHAR), 5, 3, 1, 4, 2 );
    ok(!lstrcmpW( s_sp001002, out),"failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==11,"failed: r=%d\n",r);
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1oou3oou,
              0, 0, out, sizeof(out)/sizeof(WCHAR), 5, 3, 1, 6, 4, 2 );
    ok(!lstrcmpW( s_sp001sp002, out) ||
       broken(!lstrcmpW(s_sp001004, out)), /* NT4/Win2k */
       "failed out=[%s]\n", wine_dbgstr_w(out));
    ok(r==12,"failed: r=%d\n",r);
    /* args are not counted the same way with an argument array */
    {
        ULONG_PTR args[] = { 6, 4, 2, 5, 3, 1 };
        r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, fmt_1oou1oou,
                           0, 0, out, sizeof(out)/sizeof(WCHAR), (__ms_va_list *)args );
        ok(!lstrcmpW(s_sp002sp003, out),"failed out=[%s]\n", wine_dbgstr_w(out));
        ok(r==13,"failed: r=%d\n",r);
        r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, fmt_1oou4oou,
                           0, 0, out, sizeof(out)/sizeof(WCHAR), (__ms_va_list *)args );
        ok(!lstrcmpW(s_sp002sp001, out),"failed out=[%s]\n", wine_dbgstr_w(out));
        ok(r==12,"failed: r=%d\n",r);
    }

    /* change of pace... test the low byte of dwflags */

    /* line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_hi_lf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_sp, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3,"failed: r=%d\n", r);

    /* carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_hi_crlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_sp, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==3,"failed: r=%d\n", r);

    /* carriage return */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_cr, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_sp, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==1,"failed: r=%d\n", r);

    /* double carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_crcrlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_2sp, out), "failed out=%s\n", wine_dbgstr_w(out));
    ok(r==2,"failed: r=%d\n", r);
}

static void test_message_from_string(void)
{
    CHAR out[0x100] = {0};
    DWORD r;
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x', 'x'};
    static const WCHAR szwTest[] = { 't','e','s','t',0};

    /* the basics */
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0,
        0, out, sizeof(out)/sizeof(CHAR),NULL);
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* null string, crashes on Windows */
    if (0)
    {
        SetLastError(0xdeadbeef);
        memcpy(out, init_buf, sizeof(init_buf));
        FormatMessageA(FORMAT_MESSAGE_FROM_STRING, NULL, 0,
            0, out, sizeof(out)/sizeof(CHAR), NULL);
    }

    /* empty string */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "", 0,
        0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(GetLastError()==0xdeadbeef,
       "last error %u\n", GetLastError());

    /* format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "%", 0,
        0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,
       "last error %u\n", GetLastError());

    /* test string with format placeholder with no specifier */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test%", 0,
        0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,
       "last error %u\n", GetLastError());

    /* insertion with no variadic arguments */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "%1", 0,
        0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "last error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, "%1", 0,
        0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the buffer to be untouched\n");
    ok(r==0, "succeeded: r=%d\n", r);
    ok(GetLastError()==ERROR_INVALID_PARAMETER, "last error %u\n", GetLastError());

    /* using the format feature */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!s!", 0,
        0, out, sizeof(out)/sizeof(CHAR), "test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* no format */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1", 0,
        0, out, sizeof(out)/sizeof(CHAR), "test");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* two pieces */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1%2", 0,
        0, out, sizeof(out)/sizeof(CHAR), "te","st");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* three pieces */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1%3%2%1", 0,
        0, out, sizeof(out)/sizeof(CHAR), "t","s","e");
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* s doesn't seem to work in format strings */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%!s!", 0,
        0, out, sizeof(out)/sizeof(CHAR), "test");
    ok(!strcmp("!s!", out),"failed out=[%s]\n",out);
    ok(r==3,"failed: r=%d\n",r);

    /* ls is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!ls!", 0,
        0, out, sizeof(out)/sizeof(CHAR), szwTest);
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* S is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!S!", 0,
        0, out, sizeof(out)/sizeof(CHAR), szwTest);
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* ws is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!ws!", 0,
        0, out, sizeof(out)/sizeof(CHAR), szwTest);
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* as characters */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!c!%2!c!%3!c!%1!c!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* lc is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!lc!%2!lc!%3!lc!%1!lc!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* wc is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!wc!%2!wc!%3!wc!%1!wc!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* C is unicode */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!C!%2!C!%3!C!%1!C!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 't','e','s');
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* some numbers */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!d!%2!d!%3!d!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 1,2,3);
    ok(!strcmp("123", out),"failed out=[%s]\n",out);
    ok(r==3,"failed: r=%d\n",r);

    /* a single digit with some spacing */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4d!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 1);
    ok(!strcmp("   1", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* a single digit, left justified */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!-4d!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 1);
    ok(!strcmp("1   ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* two digit decimal number */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4d!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 11);
    ok(!strcmp("  11", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* a hex number */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4x!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 11);
    ok(!strcmp("   b", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* a hex number, upper case */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4X!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 11);
    ok(!strcmp("   B", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* a hex number, upper case, left justified */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!-4X!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 11);
    ok(!strcmp("B   ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* a long hex number, upper case */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4X!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 0x1ab);
    ok(!strcmp(" 1AB", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* two percent... */
    r = doit(FORMAT_MESSAGE_FROM_STRING, " %%%% ", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp(" %% ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* periods are special cases */
    r = doit(FORMAT_MESSAGE_FROM_STRING, " %.%. %1!d!", 0,
        0, out, sizeof(out)/sizeof(CHAR), 0x1ab);
    ok(!strcmp(" .. 427", out),"failed out=[%s]\n",out);
    ok(r==7,"failed: r=%d\n",r);

    /* %0 ends the line */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "test%0test", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* %! prints an exclamation */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "yah%!%0   ", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("yah!", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* %space */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "% %   ", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("    ", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* %n yields \r\n, %r yields \r, %t yields \t */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%n%r%t", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("\r\n\r\t", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "hi\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("hi\r\n", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "hi\r\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("hi\r\n", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* carriage return */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "\r", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("\r\n", out),"failed out=[%s]\n",out);
    ok(r==2,"failed: r=%d\n",r);

    /* double carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "\r\r\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("\r\n\r\n", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

    /* null string as argument */
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1", 0,
        0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(!strcmp("(null)", out),"failed out=[%s]\n",out);
    ok(r==6,"failed: r=%d\n",r);

    /* precision and width */

    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!3s!",
             0, 0, out, sizeof(out), "t" );
    ok(!strcmp("  t", out),"failed out=[%s]\n",out);
    ok(r==3, "failed: r=%d\n",r);
    r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*s!",
             0, 0, out, sizeof(out), 4, "t");
    if (!strcmp("*s",out)) win_skip( "width/precision not supported\n" );
    else
    {
        ok(!strcmp( "   t", out),"failed out=[%s]\n",out);
        ok(r==4,"failed: r=%d\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!4.2u!",
                 0, 0, out, sizeof(out), 3 );
        ok(!strcmp( "  03", out),"failed out=[%s]\n",out);
        ok(r==4,"failed: r=%d\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*.*u!",
                 0, 0, out, sizeof(out), 5, 3, 1 );
        ok(!strcmp( "  001", out),"failed out=[%s]\n",out);
        ok(r==5,"failed: r=%d\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*.*u!,%1!*.*u!",
                 0, 0, out, sizeof(out), 5, 3, 1, 4, 2 );
        ok(!strcmp( "  001, 0002", out),"failed out=[%s]\n",out);
        ok(r==11,"failed: r=%d\n",r);
        r = doit(FORMAT_MESSAGE_FROM_STRING, "%1!*.*u!,%3!*.*u!",
                 0, 0, out, sizeof(out), 5, 3, 1, 6, 4, 2 );
        /* older Win versions marked as broken even though this is arguably the correct behavior */
        /* but the new (brain-damaged) behavior is specified on MSDN */
        ok(!strcmp( "  001,  0002", out) ||
           broken(!strcmp("  001,000004", out)), /* NT4/Win2k */
           "failed out=[%s]\n",out);
        ok(r==12,"failed: r=%d\n",r);
        /* args are not counted the same way with an argument array */
        {
            ULONG_PTR args[] = { 6, 4, 2, 5, 3, 1 };
            r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               "%1!*.*u!,%1!*.*u!", 0, 0, out, sizeof(out), (__ms_va_list *)args );
            ok(!strcmp("  0002, 00003", out),"failed out=[%s]\n",out);
            ok(r==13,"failed: r=%d\n",r);
            r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               "%1!*.*u!,%4!*.*u!", 0, 0, out, sizeof(out), (__ms_va_list *)args );
            ok(!strcmp("  0002,  001", out),"failed out=[%s]\n",out);
            ok(r==12,"failed: r=%d\n",r);
        }
    }

    /* change of pace... test the low byte of dwflags */

    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("hi ", out), "failed out=[%s]\n",out);
    ok(r==3, "failed: r=%d\n",r);

    /* carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\r\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("hi ", out),"failed out=[%s]\n",out);
    ok(r==3,"failed: r=%d\n",r);

    /* carriage return */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp(" ", out),"failed out=[%s]\n",out);
    ok(r==1,"failed: r=%d\n",r);

    /* double carriage return line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r\r\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("  ", out),"failed out=[%s]\n",out);
    ok(r==2,"failed: r=%d\n",r);
}

static void test_message_ignore_inserts(void)
{
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x'};

    DWORD ret;
    CHAR out[256];

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("test", out), "Expected output string \"test\", got %s\n", out);

    /* The %0 escape sequence is handled. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test%0", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("test", out), "Expected output string \"test\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test%0test", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("test", out), "Expected output string \"test\", got %s\n", out);

    /* While FormatMessageA returns 0 in this case, no last error code is set. */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "%0test", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %d\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)), "Expected the output buffer to be untouched\n");
    ok(GetLastError() == 0xdeadbeef, "Expected GetLastError() to return 0xdeadbeef, got %u\n", GetLastError());

    /* Insert sequences are ignored. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "test%1%2!*.*s!%99", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 17, "Expected FormatMessageA to return 17, got %d\n", ret);
    ok(!strcmp("test%1%2!*.*s!%99", out), "Expected output string \"test%%1%%2!*.*s!%%99\", got %s\n", out);

    /* Only the "%n", "%r", and "%t" escape sequences are processed. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "%%% %.%!", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 8, "Expected FormatMessageA to return 8, got %d\n", ret);
    ok(!strcmp("%%% %.%!", out), "Expected output string \"%%%%%% %%.%%!\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "%n%r%t", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("\r\n\r\t", out), "Expected output string \"\\r\\n\\r\\t\", got %s\n", out);

    /* CRLF characters are processed normally. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "hi\n", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("hi\r\n", out), "Expected output string \"hi\\r\\n\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "hi\r\n", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("hi\r\n", out), "Expected output string \"hi\\r\\n\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "\r", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 2, "Expected FormatMessageA to return 2, got %d\n", ret);
    ok(!strcmp("\r\n", out), "Expected output string \"\\r\\n\", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, "\r\r\n", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %d\n", ret);
    ok(!strcmp("\r\n\r\n", out), "Expected output string \"\\r\\n\\r\\n\", got %s\n", out);

    /* The width parameter is handled the same also. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\n", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(!strcmp("hi ", out), "Expected output string \"hi \", got %s\n", out);
    ok(ret == 3, "Expected FormatMessageA to return 3, got %d\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\r\n", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 3, "Expected FormatMessageA to return 3, got %d\n", ret);
    ok(!strcmp("hi ", out), "Expected output string \"hi \", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 1, "Expected FormatMessageA to return 1, got %d\n", ret);
    ok(!strcmp(" ", out), "Expected output string \" \", got %s\n", out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, "\r\r\n", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 2, "Expected FormatMessageA to return 2, got %d\n", ret);
    ok(!strcmp("  ", out), "Expected output string \"  \", got %s\n", out);
}

static void test_message_ignore_inserts_wide(void)
{
    static const WCHAR test[] = {'t','e','s','t',0};
    static const WCHAR empty[] = {0};
    static const WCHAR fmt_t0[] = {'t','e','s','t','%','0',0};
    static const WCHAR fmt_t0t[] = {'t','e','s','t','%','0','t','e','s','t',0};
    static const WCHAR fmt_0t[] = {'%','0','t','e','s','t',0};
    static const WCHAR fmt_t12oos99[] = {'t','e','s','t','%','1','%','2','!','*','.','*','s','!','%','9','9',0};
    static const WCHAR fmt_pctspacedot[] = {'%','%','%',' ','%','.','%','!',0};
    static const WCHAR fmt_nrt[] = {'%','n','%','r','%','t',0};
    static const WCHAR fmt_hi_lf[]   = {'h','i','\n',0};
    static const WCHAR fmt_hi_crlf[] = {'h','i','\r','\n',0};
    static const WCHAR fmt_cr[]      = {'\r',0};
    static const WCHAR fmt_crcrlf[]  = {'\r','\r','\n',0};

    static const WCHAR s_nrt[] = {'\r','\n','\r','\t',0};
    static const WCHAR s_hi_crlf[] = {'h','i','\r','\n',0};
    static const WCHAR s_crlf[] = {'\r','\n',0};
    static const WCHAR s_crlfcrlf[] = {'\r','\n','\r','\n',0};
    static const WCHAR s_hi_sp[] = {'h','i',' ',0};
    static const WCHAR s_sp[] = {' ',0};
    static const WCHAR s_2sp[] = {' ',' ',0};

    DWORD ret;
    WCHAR out[256];

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, test, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(test, out), "Expected output string \"test\", got %s\n", wine_dbgstr_w(out));

    /* The %0 escape sequence is handled. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_t0, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(test, out), "Expected output string \"test\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_t0t, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(test, out), "Expected output string \"test\", got %s\n", wine_dbgstr_w(out));

    /* While FormatMessageA returns 0 in this case, no last error code is set. */
    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_0t, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %d\n", ret);
    ok(!lstrcmpW(empty, out), "Expected the output buffer to be the empty string, got %s\n", wine_dbgstr_w(out));
    ok(GetLastError() == 0xdeadbeef, "Expected GetLastError() to return 0xdeadbeef, got %u\n", GetLastError());

    /* Insert sequences are ignored. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_t12oos99, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %d\n", ret);
    ok(!lstrcmpW(fmt_t12oos99, out), "Expected output string \"test%%1%%2!*.*s!%%99\", got %s\n", wine_dbgstr_w(out));

    /* Only the "%n", "%r", and "%t" escape sequences are processed. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_pctspacedot, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 8, "Expected FormatMessageW to return 8, got %d\n", ret);
    ok(!lstrcmpW(fmt_pctspacedot, out), "Expected output string \"%%%%%% %%.%%!\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_nrt, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(s_nrt, out), "Expected output string \"\\r\\n\\r\\t\", got %s\n", wine_dbgstr_w(out));

    /* CRLF characters are processed normally. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_hi_lf, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(s_hi_crlf, out), "Expected output string \"hi\\r\\n\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_hi_crlf, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(s_hi_crlf, out), "Expected output string \"hi\\r\\n\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_cr, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 2, "Expected FormatMessageW to return 2, got %d\n", ret);
    ok(!lstrcmpW(s_crlf, out), "Expected output string \"\\r\\n\", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS, fmt_crcrlf, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %d\n", ret);
    ok(!lstrcmpW(s_crlfcrlf, out), "Expected output string \"\\r\\n\\r\\n\", got %s\n", wine_dbgstr_w(out));

    /* The width parameter is handled the same also. */
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_hi_lf, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 3, "Expected FormatMessageW to return 3, got %d\n", ret);
    ok(!lstrcmpW(s_hi_sp, out), "Expected output string \"hi \", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_hi_crlf, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 3, "Expected FormatMessageW to return 3, got %d\n", ret);
    ok(!lstrcmpW(s_hi_sp, out), "Expected output string \"hi \", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_cr, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 1, "Expected FormatMessageW to return 1, got %d\n", ret);
    ok(!lstrcmpW(s_sp, out), "Expected output string \" \", got %s\n", wine_dbgstr_w(out));

    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS |
                         FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_crcrlf, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 2, "Expected FormatMessageW to return 2, got %d\n", ret);
    ok(!lstrcmpW(s_2sp, out), "Expected output string \"  \", got %s\n", wine_dbgstr_w(out));
}

static void test_message_wrap(void)
{
    DWORD ret;
    int i;
    CHAR in[300], out[300], ref[300];

    /* No need for wrapping */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 20,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 15, "Expected FormatMessageW to return 15, got %d\n", ret);
    ok(!strcmp("short long line", out),"failed out=[%s]\n",out);

    /* Wrap the last word */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short long\r\nline", out),"failed out=[%s]\n",out);

    /* Wrap the very last word */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 20,
                         "short long long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 21, "Expected FormatMessageW to return 21, got %d\n", ret);
    ok(!strcmp("short long long\r\nline", out),"failed out=[%s]\n",out);

    /* Strictly less than 10 characters per line! */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 10,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong line", out),"failed out=[%s]\n",out);

    /* Handling of duplicate spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 16,
                         "short long    line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short long\r\nline", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 16,
                         "short long    wordlongerthanaline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 33, "Expected FormatMessageW to return 33, got %d\n", ret);
    ok(!strcmp("short long\r\nwordlongerthanal\r\nine", out),"failed out=[%s]\n",out);

    /* Breaking in the middle of spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 12,
                         "short long    line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 18, "Expected FormatMessageW to return 18, got %d\n", ret);
    ok(!strcmp("short long\r\n  line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 12,
                         "short long    wordlongerthanaline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 35, "Expected FormatMessageW to return 35, got %d\n", ret);
    ok(!strcmp("short long\r\n\r\nwordlongerth\r\nanaline", out),"failed out=[%s]\n",out);

    /* Handling of start-of-string spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 15,
                         "   short line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 13, "Expected FormatMessageW to return 13, got %d\n", ret);
    ok(!strcmp("   short line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "   shortlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %d\n", ret);
    ok(!strcmp("\r\nshortlong\r\nline", out),"failed out=[%s]\n",out);

    /* Handling of start-of-line spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "l1%n   shortlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 21, "Expected FormatMessageW to return 21, got %d\n", ret);
    ok(!strcmp("l1\r\n\r\nshortlong\r\nline", out),"failed out=[%s]\n",out);

    /* Pure space wrapping */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 5,
                         "                ", 0, 0, out, sizeof(out), NULL);
    ok(ret == 7, "Expected FormatMessageW to return 7, got %d\n", ret);
    ok(!strcmp("\r\n\r\n\r\n ", out),"failed out=[%s]\n",out);

    /* Handling of trailing spaces */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 5,
                         "l1               ", 0, 0, out, sizeof(out), NULL);
    ok(ret == 10, "Expected FormatMessageW to return 10, got %d\n", ret);
    ok(!strcmp("l1\r\n\r\n\r\n  ", out),"failed out=[%s]\n",out);

    /* Word that just fills the line */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "shortlon", 0, 0, out, sizeof(out), NULL);
    ok(ret == 10, "Expected FormatMessageW to return 10, got %d\n", ret);
    ok(!strcmp("shortlon\r\n", out),"failed out=[%s]\n",out);

    /* Word longer than the line */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "shortlongline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 15, "Expected FormatMessageW to return 15, got %d\n", ret);
    ok(!strcmp("shortlon\r\ngline", out),"failed out=[%s]\n",out);

    /* Wrap the line multiple times */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 7,
                         "short long line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %d\n", ret);
    ok(!strcmp("short\r\nlong\r\nline", out),"failed out=[%s]\n",out);

    /* '\n's in the source are ignored */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short\nlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short long\r\nline", out),"failed out=[%s]\n",out);

    /* Wrap even before a '%n' */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "shortlon%n", 0, 0, out, sizeof(out), NULL);
    ok(ret == 12, "Expected FormatMessageW to return 12, got %d\n", ret);
    ok(!strcmp("shortlon\r\n\r\n", out),"failed out=[%s]\n",out);

    /* '%n's count as starting a new line and combine with line wrapping */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 10,
                         "short%nlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "short%nlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 17, "Expected FormatMessageW to return 17, got %d\n", ret);
    ok(!strcmp("short\r\nlong\r\nline", out),"failed out=[%s]\n",out);

    /* '%r's also count as starting a new line and all */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 10,
                         "short%rlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 15, "Expected FormatMessageW to return 15, got %d\n", ret);
    ok(!strcmp("short\rlong line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 8,
                         "short%rlong line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\rlong\r\nline", out),"failed out=[%s]\n",out);

    /* IGNORE_INSERTS does not prevent line wrapping or disable '%n' */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_IGNORE_INSERTS | 8,
                         "short%nlong line%1", 0, 0, out, sizeof(out), NULL);
    ok(ret == 19, "Expected FormatMessageW to return 19, got %d\n", ret);
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
    ok(ret == 272, "Expected FormatMessageW to return 272, got %d\n", ret);
    ok(!strcmp(ref, out),"failed out=[%s]\n",out);

    /* Wrapping and non-space characters */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long\tline", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong\tline", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long-line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong-line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long_line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong_line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long.line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong.line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long,line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong,line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long!line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong!line", out),"failed out=[%s]\n",out);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | 11,
                         "short long?line", 0, 0, out, sizeof(out), NULL);
    ok(ret == 16, "Expected FormatMessageW to return 16, got %d\n", ret);
    ok(!strcmp("short\r\nlong?line", out),"failed out=[%s]\n",out);
}

static void test_message_insufficient_buffer(void)
{
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x'};
    static const char expected_buf[] = {'x', 'x', 'x', 'x', 'x'};
    DWORD ret;
    CHAR out[5];

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %u\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %u\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0, 0, out, sizeof(out)/sizeof(out[0]) - 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %u\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");
}

static void test_message_insufficient_buffer_wide(void)
{
    static const WCHAR test[] = {'t','e','s','t',0};
    static const WCHAR init_buf[] = {'x', 'x', 'x', 'x', 'x'};
    static const WCHAR expected_buf[] = {'x', 'x', 'x', 'x', 'x'};
    static const WCHAR broken_buf[] = {0, 'x', 'x', 'x', 'x'};
    static const WCHAR broken2_buf[] = {'t','e','s',0,'x'};

    DWORD ret;
    WCHAR out[5];

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, test, 0, 0, out, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %u\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)),
       "Expected the buffer to be untouched\n");

    /* Windows Server 2003 and newer report failure but copy a
     * truncated string to the buffer for non-zero buffer sizes. */
    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, test, 0, 0, out, 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %u\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)) ||
       broken(!memcmp(broken_buf, out, sizeof(broken_buf))), /* W2K3+ */
       "Expected the buffer to be untouched\n");

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, test, 0, 0, out, sizeof(out)/sizeof(out[0]) - 1, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %u\n",
       GetLastError());
    ok(!memcmp(expected_buf, out, sizeof(expected_buf)) ||
       broken(!memcmp(broken2_buf, out, sizeof(broken2_buf))), /* W2K3+ */
       "Expected the buffer to be untouched\n");
}

static void test_message_null_buffer(void)
{
    DWORD ret, error;

    /* Without FORMAT_MESSAGE_ALLOCATE_BUFFER, only the specified buffer size is checked. */
    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %u\n", ret);
    ok(error == ERROR_INSUFFICIENT_BUFFER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %u\n", ret);
    ok(error == ERROR_INSUFFICIENT_BUFFER, "last error %u\n", error);

    if (0) /* crashes on Windows */
    {
        SetLastError(0xdeadbeef);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 256, NULL);
    }

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %u\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %u\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 256, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %u\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY, "last error %u\n", error);
}

static void test_message_null_buffer_wide(void)
{
    DWORD ret, error;

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, 0, 0, NULL, 256, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 1, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 256, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);
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
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(buf == NULL, "Expected output buffer pointer to be NULL\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected last error to be untouched, got %u\n", GetLastError());

    buf = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         "test", 0, 0, (char *)&buf, 0, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
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
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
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
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
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
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
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
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
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
    static const WCHAR empty[] = {0};
    static const WCHAR test[] = {'t','e','s','t',0};

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
                         empty, 0, 0, (WCHAR *)&buf, 0, NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %u\n", ret);
    ok(buf == NULL, "Expected output buffer pointer to be NULL\n");
    ok(GetLastError() == 0xdeadbeef,
       "Expected last error to be untouched, got %u\n", GetLastError());

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         test, 0, 0, (WCHAR *)&buf, 0, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(test, buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         test, 0, 0, (WCHAR *)&buf, sizeof(test)/sizeof(WCHAR) - 1, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(test, buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         test, 0, 0, (WCHAR *)&buf, sizeof(test)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(test, buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         test, 0, 0, (WCHAR *)&buf, sizeof(test)/sizeof(WCHAR) + 1, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(test, buf),
           "Expected buffer to contain \"test\", got %s\n", wine_dbgstr_w(buf));
        LocalFree(buf);
    }

    buf = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         test, 0, 0, (WCHAR *)&buf, 1024, NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(buf != NULL && buf != (WCHAR *)0xdeadbeef,
       "Expected output buffer pointer to be valid\n");
    if (buf != NULL && buf != (WCHAR *)0xdeadbeef)
    {
        ok(!lstrcmpW(test, buf),
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
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret != 0, "FormatMessageA returned 0\n");

    /* Test HRESULT. It's not documented but in practice _com_error::ErrorMessage relies on this. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 0x80070005 /* E_ACCESSDENIED */,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret != 0, "FormatMessageA returned 0\n");

    /* Test a message string with an insertion without passing any variadic arguments. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 193 /* ERROR_BAD_EXE_FORMAT */,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "FormatMessageA returned non-zero\n");

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY, h, 193 /* ERROR_BAD_EXE_FORMAT */,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "FormatMessageA returned non-zero\n");

    /*Test nonexistent messageID with varying language IDs Note: FormatMessageW behaves the same*/
    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out)/sizeof(CHAR), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %u instead of 0\n", ret);
    ok(error == ERROR_MR_MID_NOT_FOUND || error == ERROR_MUI_FILE_NOT_FOUND, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), out, sizeof(out)/sizeof(CHAR), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %u instead of 0\n", ret);
    ok(error == ERROR_MR_MID_NOT_FOUND || error == ERROR_MUI_FILE_NOT_LOADED, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), out, sizeof(out)/sizeof(CHAR), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %u instead of 0\n", ret);
    ok(error == ERROR_MR_MID_NOT_FOUND || error == ERROR_MUI_FILE_NOT_LOADED, "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), out, sizeof(out)/sizeof(CHAR), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %u instead of 0\n", ret);
    ok(error == ERROR_RESOURCE_LANG_NOT_FOUND ||
       error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_LOADED,
       "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, h, 3044,
                         MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK), out, sizeof(out)/sizeof(CHAR), NULL);
    error = GetLastError();
    ok(ret == 0, "FormatMessageA returned %u instead of 0\n", ret);
    ok(error == ERROR_RESOURCE_LANG_NOT_FOUND ||
       error == ERROR_MR_MID_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_FOUND ||
       error == ERROR_MUI_FILE_NOT_LOADED,
       "last error %u\n", error);
}

static void test_message_invalid_flags(void)
{
    static const char init_buf[] = {'x', 'x', 'x', 'x', 'x'};

    DWORD ret;
    CHAR out[5];
    char *ptr;

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(0, "test", 0, 0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ptr = (char *)0xdeadbeef;
    ret = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER, "test", 0, 0, (char *)&ptr, 0, NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(ptr == NULL, "Expected output pointer to be initialized to NULL, got %p\n", ptr);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS, "test", 0, 0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_ARGUMENT_ARRAY, "test", 0, 0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_MAX_WIDTH_MASK, "test", 0, 0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 0, "Expected FormatMessageA to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    /* Simultaneously setting FORMAT_MESSAGE_FROM_STRING with other source
     * flags is apparently permissible, and FORMAT_MESSAGE_FROM_STRING takes
     * precedence in this case. */

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_SYSTEM,
                         "test", 0, 0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(!strcmp("test", out),
       "Expected the output buffer to be untouched\n");

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE,
                         "test", 0, 0, out, sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(!strcmp("test", out),
       "Expected the output buffer to be untouched\n");

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_FROM_SYSTEM, "test", 0, 0, out,
                         sizeof(out)/sizeof(CHAR), NULL);
    ok(ret == 4, "Expected FormatMessageA to return 4, got %u\n", ret);
    ok(!strcmp("test", out),
       "Expected the output buffer to be untouched\n");
}

static void test_message_invalid_flags_wide(void)
{
    static const WCHAR init_buf[] = {'x', 'x', 'x', 'x', 'x'};
    static const WCHAR test[] = {'t','e','s','t',0};

    DWORD ret;
    WCHAR out[5];
    WCHAR *ptr;

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(0, test, 0, 0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ptr = (WCHAR *)0xdeadbeef;
    ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER, test, 0, 0, (WCHAR *)&ptr, 0, NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %u\n", ret);
    ok(ptr == NULL, "Expected output pointer to be initialized to NULL, got %p\n", ptr);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS, test, 0, 0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY, test, 0, 0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_MAX_WIDTH_MASK, test, 0, 0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 0, "Expected FormatMessageW to return 0, got %u\n", ret);
    ok(!memcmp(out, init_buf, sizeof(init_buf)),
       "Expected the output buffer to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %u\n",
       GetLastError());

    /* Simultaneously setting FORMAT_MESSAGE_FROM_STRING with other source
     * flags is apparently permissible, and FORMAT_MESSAGE_FROM_STRING takes
     * precedence in this case. */

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_SYSTEM,
                         test, 0, 0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %u\n", ret);
    ok(!lstrcmpW(test, out),
       "Expected the output buffer to be untouched\n");

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE,
                         test, 0, 0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %u\n", ret);
    ok(!lstrcmpW(test, out),
       "Expected the output buffer to be untouched\n");

    memcpy(out, init_buf, sizeof(init_buf));
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_FROM_HMODULE |
                         FORMAT_MESSAGE_FROM_SYSTEM, test, 0, 0, out,
                         sizeof(out)/sizeof(WCHAR), NULL);
    ok(ret == 4, "Expected FormatMessageW to return 4, got %u\n", ret);
    ok(!lstrcmpW(test, out),
       "Expected the output buffer to be untouched\n");
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
}
