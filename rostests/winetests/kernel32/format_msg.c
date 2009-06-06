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

/* #define ok(cond,failstr) if(!(cond)) {printf("line %d : %s\n",__LINE__,failstr);exit(1);} */

static DWORD doit(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
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

static DWORD doitW(DWORD flags, LPCVOID src, DWORD msg_id, DWORD lang_id,
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

static char buf[1024];
static const char *debugstr_w(const WCHAR *str)
{
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static void test_message_from_string_wide(void)
{
    static const WCHAR test[]        = {'t','e','s','t',0};
    static const WCHAR te[]          = {'t','e',0};
    static const WCHAR st[]          = {'s','t',0};
    static const WCHAR t[]           = {'t',0};
    static const WCHAR e[]           = {'e',0};
    static const WCHAR s[]           = {'s',0};
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
    static const WCHAR fmt_hi_lf[]   = {'h','i','\n',0};
    static const WCHAR fmt_hi_crlf[] = {'h','i','\r','\n',0};
    static const WCHAR fmt_cr[]      = {'\r',0};
    static const WCHAR fmt_crcrlf[]  = {'\r','\r','\n',0};
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
    static const WCHAR s_hi_crlf[]   = {'h','i','\r','\n',0};
    static const WCHAR s_crlf[]      = {'\r','\n',0};
    static const WCHAR s_crlfcrlf[]  = {'\r','\n','\r','\n',0};
    static const WCHAR s_hi_sp[]     = {'h','i',' ',0};
    static const WCHAR s_sp[]        = {' ',0};
    static const WCHAR s_2sp[]       = {' ',' ',0};
    WCHAR out[0x100] = {0};
    DWORD r, error;

    SetLastError(0xdeadbeef);
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    if (!r && error == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("FormatMessageW is not implemented\n");
        return;
    }

    /* the basics */
    r = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, test, 0,
        0, out, sizeof(out)/sizeof(WCHAR), NULL);
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4, "failed: r=%d\n", r);

    /* using the format feature */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1s, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* no format */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* two pieces */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_12, 0,
        0, out, sizeof(out)/sizeof(WCHAR), te, st);
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* three pieces */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123, 0,
        0, out, sizeof(out)/sizeof(WCHAR), t, s, e);
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* s doesn't seem to work in format strings */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_s, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_s[1], out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==3, "failed: r=%d\n", r);

    /* nor ls */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_ls, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_ls[1], out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4, "failed: r=%d\n", r);

    /* nor S */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_S, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_S[1], out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==3, "failed: r=%d\n", r);

    /* nor ws */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_ws, 0,
        0, out, sizeof(out)/sizeof(WCHAR), test);
    ok(!lstrcmpW(&fmt_ws[1], out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4, "failed: r=%d\n", r);

    /* as characters */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123c, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* lc is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123lc, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* wc is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123wc, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* C is unicode */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123C, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 't', 'e', 's');
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* some numbers */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_123d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 1, 2, 3);
    ok(!lstrcmpW(s_123d, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==3,"failed: r=%d\n", r);

    /* a single digit with some spacing */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 1);
    ok(!lstrcmpW(s_14d, out), "failed out=[%s]\n", debugstr_w(out));

    /* a single digit, left justified */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1_4d, 0,
        0, out, sizeof(out)/sizeof(CHAR), 1);
    ok(!lstrcmpW(s_1_4d, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* two digit decimal number */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_14d2, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a hex number */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14x, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_14x, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a hex number, upper case */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14X, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_14X, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a hex number, upper case, left justified */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_1_4X, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 11);
    ok(!lstrcmpW(s_1_4X, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* a long hex number, upper case */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_14X, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 0x1ab);
    ok(!lstrcmpW(s_1AB, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* two percent... */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_2pct, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_2pct, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* periods are special cases */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_2dot1d, 0,
        0, out, sizeof(out)/sizeof(WCHAR), 0x1ab);
    ok(!lstrcmpW(s_2dot147, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==8,"failed: r=%d\n", r);

    /* %0 ends the line */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_t0t, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(test, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* %! prints an exclamation */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_yah, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_yah, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* %space */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_space, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_space, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_hi_lf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_crlf, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_hi_crlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_crlf, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* carriage return */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_cr, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_crlf, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==2,"failed: r=%d\n", r);

    /* double carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING, fmt_crcrlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_crlfcrlf, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==4,"failed: r=%d\n", r);

    /* change of pace... test the low byte of dwflags */

    /* line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_hi_lf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_sp, out) || !lstrcmpW(s_hi_crlf, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==3 || r==4,"failed: r=%d\n", r);

    /* carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_hi_crlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_hi_sp, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==3,"failed: r=%d\n", r);

    /* carriage return */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_cr, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_sp, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==1,"failed: r=%d\n", r);

    /* double carriage return line feed */
    r = doitW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, fmt_crcrlf, 0,
        0, out, sizeof(out)/sizeof(WCHAR));
    ok(!lstrcmpW(s_2sp, out), "failed out=[%s]\n", debugstr_w(out));
    ok(r==2,"failed: r=%d\n", r);
}

static void test_message_from_string(void)
{
    CHAR out[0x100] = {0};
    DWORD r;
    static const WCHAR szwTest[] = { 't','e','s','t',0};

    /* the basics */
    r = FormatMessageA(FORMAT_MESSAGE_FROM_STRING, "test", 0,
        0, out, sizeof(out)/sizeof(CHAR),NULL);
    ok(!strcmp("test", out),"failed out=[%s]\n",out);
    ok(r==4,"failed: r=%d\n",r);

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

    /* change of pace... test the low byte of dwflags */

    /* line feed */
    r = doit(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK, "hi\n", 0,
        0, out, sizeof(out)/sizeof(CHAR));
    ok(!strcmp("hi ", out) || !strcmp("hi\r\n", out),"failed out=[%s]\n",out);
    ok(r==3 || r==4,"failed: r=%d\n",r);

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

static void test_message_null_buffer(void)
{
    DWORD ret, error;

    SetLastError(0xdeadbeef);
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    ok(!ret, "FormatMessageA returned %u\n", ret);
    ok(error == ERROR_NOT_ENOUGH_MEMORY ||
       error == ERROR_INVALID_PARAMETER, /* win9x */
       "last error %u\n", error);

    SetLastError(0xdeadbeef);
    ret = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, 0, 0, NULL, 0, NULL);
    error = GetLastError();
    if (!ret && error == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("FormatMessageW is not implemented\n");
        return;
    }

    ok(!ret, "FormatMessageW returned %u\n", ret);
    ok(error == ERROR_INVALID_PARAMETER, "last error %u\n", error);
}

START_TEST(format_msg)
{
    test_message_from_string();
    test_message_from_string_wide();
    test_message_null_buffer();
}
