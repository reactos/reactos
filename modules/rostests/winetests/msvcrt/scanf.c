/*
 * Unit test suite for *scanf functions.
 *
 * Copyright 2002 Uwe Bonnes
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

#include <stdio.h>

#include "wine/test.h"

static void test_fscanf( void )
{
    static const char file_name[] = "fscanf.tst";
    static const char contents[] =
        "line1\n"
        "line2 "
    ;
    char buf[1024];
    FILE *fp;
    int ret;

    fp = fopen(file_name, "wb");
    ok(fp != NULL, "fp = %p\n", fp);
    if(!fp) {
        skip("failed to create temporary test file\n");
        return;
    }

    ret = fprintf(fp, contents);
    fclose(fp);

    fp = fopen(file_name, "rb");
    ret = fscanf(fp, "%s", buf);
    ok(ret == 1, "ret = %d\n", ret);
    ok(strcmp(buf, "line1") == 0, "buf = %s\n", buf);
    ret = fscanf(fp, "%s", buf);
    ok(ret == 1, "ret = %d\n", ret);
    ok(strcmp(buf, "line2") == 0, "buf = %s\n", buf);
    ret = fscanf(fp, "%s", buf);
    ok(ret == EOF, "ret = %d\n", ret);
    fclose(fp);

    unlink(file_name);
}

static void test_sscanf( void )
{
    /* use function pointers to bypass gcc builtin */
    int (WINAPIV *p_sprintf)(char *buf, const char *fmt, ...);
    int (WINAPIV *p_sscanf)(const char *buf, const char *fmt, ...);
    char buffer[100], buffer1[100];
    char format[20];
    int result, ret;
    LONGLONG result64;
    char c;
    void *ptr;
    float res1= -82.6267f, res2= 27.76f, res11, res12;
    double double_res;
    static const char pname[]=" St. Petersburg, Florida\n";
    int hour=21,min=59,sec=20;
    int  number,number_so_far;
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p_sprintf = (void *)GetProcAddress( hmod, "sprintf" );
    p_sscanf = (void *)GetProcAddress( hmod, "sscanf" );

    /* check EOF */
    strcpy(buffer,"");
    ret = p_sscanf(buffer, "%d", &result);
    ok( ret == EOF,"sscanf returns %x instead of %x\n", ret, EOF );

    ret = p_sscanf(" \t\n\n", "%s", buffer);
    ok( ret == EOF, "ret = %d\n", ret );

    buffer1[0] = 'a';
    ret = p_sscanf("test\n", "%s%c", buffer, buffer1);
    ok( ret == 2, "ret = %d\n", ret );
    ok( buffer1[0] == '\n', "buffer1[0] = %d\n", buffer1[0] );

    /* check %p */
    ok( p_sscanf("000000000046F170", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)0x46F170,"sscanf reads %p instead of %x\n", ptr, 0x46F170 );

    ok( p_sscanf("0046F171", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)0x46F171,"sscanf reads %p instead of %x\n", ptr, 0x46F171 );

    ok( p_sscanf("46F172", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)0x46F172,"sscanf reads %p instead of %x\n", ptr, 0x46F172 );

    ok( p_sscanf("0x46F173", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == NULL,"sscanf reads %p instead of %x\n", ptr, 0 );

    ok( p_sscanf("-46F174", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)(ULONG_PTR)-0x46f174,"sscanf reads %p instead of %p\n",
        ptr, (void *)(ULONG_PTR)-0x46f174 );

    ok( p_sscanf("+46F175", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)0x46F175,"sscanf reads %p instead of %x\n", ptr, 0x46F175 );

    /* check %p with no hex digits */
    ok( p_sscanf("1233", "%p", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)0x1233,"sscanf reads %p instead of %x\n", ptr, 0x1233 );

    ok( p_sscanf("1234", "%P", &ptr) == 1, "sscanf failed\n"  );
    ok( ptr == (void *)0x1234,"sscanf reads %p instead of %x\n", ptr, 0x1234 );

    /* check %x */
    strcpy(buffer,"0x519");
    ok( p_sscanf(buffer, "%x", &result) == 1, "sscanf failed\n"  );
    ok( result == 0x519,"sscanf reads %x instead of %x\n", result, 0x519 );

    strcpy(buffer,"0x51a");
    ok( p_sscanf(buffer, "%x", &result) == 1, "sscanf failed\n" );
    ok( result == 0x51a ,"sscanf reads %x instead of %x\n", result, 0x51a );

    strcpy(buffer,"0x51g");
    ok( p_sscanf(buffer, "%x", &result) == 1, "sscanf failed\n" );
    ok( result == 0x51, "sscanf reads %x instead of %x\n", result, 0x51 );

    result = 0;
    ret = p_sscanf("-1", "%x", &result);
    ok(ret == 1, "Wrong number of arguments read: %d (expected 1)\n", ret);
    ok(result == -1, "Read %d, expected -1\n", result);

    /* check % followed by any char */
    strcpy(buffer,"\"%12@");
    strcpy(format,"%\"%%%d%@");  /* work around gcc format check */
    ok( p_sscanf(buffer, format, &result) == 1, "sscanf failed\n" );
    ok( result == 12, "sscanf reads %x instead of %x\n", result, 12 );

    /* Check float */
    ret = p_sprintf(buffer,"%f %f",res1, res2);
    ok( ret == 20, "expected 20, got %u\n", ret);
    ret = p_sscanf(buffer,"%f%f",&res11, &res12);
    ok( ret == 2, "expected 2, got %u\n", ret);
    ok( (res11 == res1) && (res12 == res2), "Error reading floats\n");

    /* Check double */
    ret = p_sprintf(buffer, "%lf", 32.715);
    ok(ret == 9, "expected 9, got %u\n", ret);
    ret = p_sscanf(buffer, "%lf", &double_res);
    ok(ret == 1, "expected 1, got %u\n", ret);
    ok(double_res == 32.715, "Got %lf, expected %lf\n", double_res, 32.715);
    ret = p_sscanf(buffer, "%Lf", &double_res);
    ok(ret == 1, "expected 1, got %u\n", ret);
    ok(double_res == 32.715, "Got %lf, expected %lf\n", double_res, 32.715);

    strcpy(buffer, "1.1e-30");
    ret = p_sscanf(buffer, "%lf", &double_res);
    ok(ret == 1, "expected 1, got %u\n", ret);
    ok(double_res >= 1.1e-30-1e-45 && double_res <= 1.1e-30+1e-45,
            "Got %.18le, expected %.18le\n", double_res, 1.1e-30);

    buffer[0] = 0;
    double_res = 1;
    ret = p_sscanf(buffer, "%lf", &double_res);
    ok(ret == -1, "expected 0, got %u\n", ret);
    ok(double_res == 1, "Got %lf, expected 1\n", double_res);

    /* check strings */
    ret = p_sprintf(buffer," %s", pname);
    ok( ret == 26, "expected 26, got %u\n", ret);
    ret = p_sscanf(buffer,"%*c%[^\n]",buffer1);
    ok( ret == 1, "Error with format \"%s\"\n","%*c%[^\n]");
    ok( strncmp(pname,buffer1,strlen(buffer1)) == 0, "Error with \"%s\" \"%s\"\n",pname, buffer1);

    ret = p_sscanf("abcefgdh","%*[a-cg-e]%c",&buffer[0]);
    ok( ret == 1, "Error with format \"%s\"\n","%*[a-cg-e]%c");
    ok( buffer[0] == 'd', "Error with \"abcefgdh\" \"%c\"\n", buffer[0]);

    ret = p_sscanf("abcefgdh","%*[a-cd-dg-e]%c",&buffer[0]);
    ok( ret == 1, "Error with format \"%s\"\n","%*[a-cd-dg-e]%c");
    ok( buffer[0] == 'h', "Error with \"abcefgdh\" \"%c\"\n", buffer[0]);

    ret = p_sscanf("-123", "%[-0-9]", buffer);
    ok( ret == 1, "Error with format \"%s\"\n", "%[-0-9]");
    ok( strcmp("-123", buffer) == 0, "Error with \"-123\" \"%s\"\n", buffer);

    ret = p_sscanf("-321", "%[0-9-]", buffer);
    ok( ret == 1, "Error with format \"%s\"\n", "%[0-9-]");
    ok( strcmp("-321", buffer) == 0, "Error with \"-321\" \"%s\"\n", buffer);

    ret = p_sscanf("-4123", "%[1-2-4]", buffer);
    ok( ret == 1, "Error with format \"%s\"\n", "%[1-2-4]");
    ok( strcmp("-412", buffer) == 0, "Error with \"-412\" \"%s\"\n", buffer);

    ret = p_sscanf("-456123", "%[1-2-45-6]", buffer);
    ok( ret == 1, "Error with format \"%s\"\n", "%[1-2-45-6]");
    ok( strcmp("-45612", buffer) == 0, "Error with \"-45612\" \"%s\"\n", buffer);

    buffer1[0] = 'b';
    ret = p_sscanf("a","%s%s", buffer, buffer1);
    ok( ret == 1, "expected 1, got %u\n", ret);
    ok( buffer[0] == 'a', "buffer[0] = '%c'\n", buffer[0]);
    ok( buffer[1] == '\0', "buffer[1] = '%c'\n", buffer[1]);
    ok( buffer1[0] == 'b', "buffer1[0] = '%c'\n", buffer1[0]);

    /* check digits */
    ret = p_sprintf(buffer,"%d:%d:%d",hour,min,sec);
    ok( ret == 8, "expected 8, got %u\n", ret);
    ret = p_sscanf(buffer,"%d%n",&number,&number_so_far);
    ok(ret == 1 , "problem with format arg \"%%d%%n\"\n");
    ok(number == hour,"Read wrong arg %d instead of %d\n",number, hour);
    ok(number_so_far == 2,"Read wrong arg for \"%%n\" %d instead of 2\n",number_so_far);

    ret = p_sscanf(buffer+2,"%*c%n",&number_so_far);
    ok(ret == 0 , "problem with format arg \"%%*c%%n\"\n");
    ok(number_so_far == 1,"Read wrong arg for \"%%n\" %d instead of 2\n",number_so_far);

    result = 0xdeadbeef;
    strcpy(buffer,"12345678");
    ret = p_sscanf(buffer, "%hd", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 0xdead614e, "Wrong number read (%x)\n", result);

    result = 0xdeadbeef;
    strcpy(buffer,"12345678");
    ret = p_sscanf(buffer, "%02hd", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 0xdead000c, "Wrong number read (%x)\n", result);

    result = 0xdeadbeef;
    strcpy(buffer,"12345678");
    ret = p_sscanf(buffer, "%h02d", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 0xdead000c, "Wrong number read (%x)\n", result);

    result = 0xdeadbeef;
    strcpy(buffer,"12345678");
    ret = p_sscanf(buffer, "%000h02d", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 0xdead000c, "Wrong number read (%x)\n", result);

    result = 0xdeadbeef;
    strcpy(buffer,"12345678");
    ret = p_sscanf(buffer, "%2h0d", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 0xdead614e, "Wrong number read (%x)\n", result);

    result = 0xdeadbeef;
    ret = p_sscanf(buffer, "%hhd", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 0xbc614e, "Wrong number read (%x)\n", result);

    strcpy(buffer,"12345678901234");
    ret = p_sscanf(buffer, "%lld", &result64);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ret = p_sprintf(buffer1, "%lld", result64);
    ok(ret==14 || broken(ret==10), "sprintf returned %d\n", ret);
    if(ret == 14)
        ok(!strcmp(buffer, buffer1), "got %s, expected %s\n", buffer1, buffer);

    /* Check %i according to bug 1878 */
    strcpy(buffer,"123");
    ret = p_sscanf(buffer, "%i", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 123, "Wrong number read\n");
    result = 0;
    ret = p_sscanf("-1", "%i", &result);
    ok(ret == 1, "Wrong number of arguments read: %d (expected 1)\n", ret);
    ok(result == -1, "Read %d, expected -1\n", result);
    ret = p_sscanf(buffer, "%d", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 123, "Wrong number read\n");
    result = 0;
    ret = p_sscanf("-1", "%d", &result);
    ok(ret == 1, "Wrong number of arguments read: %d (expected 1)\n", ret);
    ok(result == -1, "Read %d, expected -1\n", result);

    /* Check %i for octal and hexadecimal input */
    result = 0;
    strcpy(buffer,"017");
    ret = p_sscanf(buffer, "%i", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 15, "Wrong number read\n");
    result = 0;
    strcpy(buffer,"0x17");
    ret = p_sscanf(buffer, "%i", &result);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(result == 23, "Wrong number read\n");

    /* %o */
    result = 0;
    ret = p_sscanf("-1", "%o", &result);
    ok(ret == 1, "Wrong number of arguments read: %d (expected 1)\n", ret);
    ok(result == -1, "Read %d, expected -1\n", result);

    /* %u */
    result = 0;
    ret = p_sscanf("-1", "%u", &result);
    ok(ret == 1, "Wrong number of arguments read: %d (expected 1)\n", ret);
    ok(result == -1, "Read %d, expected -1\n", result);

    /* Check %c */
    strcpy(buffer,"a");
    c = 0x55;
    ret = p_sscanf(buffer, "%c", &c);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(c == 'a', "Field incorrect: '%c'\n", c);

    strcpy(buffer," a");
    c = 0x55;
    ret = p_sscanf(buffer, "%c", &c);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(c == ' ', "Field incorrect: '%c'\n", c);

    strcpy(buffer,"18:59");
    c = 0x55;
    ret = p_sscanf(buffer, "%d:%d%c", &hour, &min, &c);
    ok(ret == 2, "Wrong number of arguments read: %d\n", ret);
    ok(hour == 18, "Field 1 incorrect: %d\n", hour);
    ok(min == 59, "Field 2 incorrect: %d\n", min);
    ok(c == 0x55, "Field 3 incorrect: 0x%02x\n", c);

    /* Check %n (also whitespace in format strings and %s) */
    buffer[0]=0; buffer1[0]=0;
    ret = p_sscanf("abc   def", "%s %n%s", buffer, &number_so_far, buffer1);
    ok(strcmp(buffer, "abc")==0, "First %%s read incorrectly: %s\n", buffer);
    ok(strcmp(buffer1,"def")==0, "Second %%s read incorrectly: %s\n", buffer1);
    ok(number_so_far==6, "%%n yielded wrong result: %d\n", number_so_far);
    ok(ret == 2, "%%n shouldn't count as a conversion: %d\n", ret);

    /* Check where %n matches to EOF in buffer */
    strcpy(buffer, "3:45");
    ret = p_sscanf(buffer, "%d:%d%n", &hour, &min, &number_so_far);
    ok(ret == 2, "Wrong number of arguments read: %d\n", ret);
    ok(number_so_far == 4, "%%n yielded wrong result: %d\n", number_so_far);

    buffer[0] = 0;
    buffer1[0] = 0;
    ret = p_sscanf("test=value\xda", "%[^=] = %[^;]", buffer, buffer1);
    ok(ret == 2, "got %d\n", ret);
    ok(!strcmp(buffer, "test"), "buf %s\n", buffer);
    ok(!strcmp(buffer1, "value\xda"), "buf %s\n", buffer1);

    ret = p_sscanf("\x81\x82test", "\x81%\x82%s", buffer);
    ok(ret == 1, "got %d\n", ret);
    ok(!strcmp(buffer, "test"), "buf = %s\n", buffer);
}

static void test_sscanf_s(void)
{
    int (WINAPIV *psscanf_s)(const char*,const char*,...);
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");
    int i, ret;
    char buf[100];

    psscanf_s = (void*)GetProcAddress(hmod, "sscanf_s");
    if(!psscanf_s) {
        win_skip("sscanf_s not available\n");
        return;
    }

    ret = psscanf_s("123", "%d", &i);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(i == 123, "i = %d\n", i);

    ret = psscanf_s("123", "%s", buf, 100);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(!strcmp("123", buf), "buf = %s\n", buf);

    ret = psscanf_s("123", "%s", buf, 3);
    ok(ret == 0, "Wrong number of arguments read: %d\n", ret);
    ok(buf[0]=='\0', "buf = %s\n", buf);

    memset(buf, 'a', sizeof(buf));
    ret = psscanf_s("123", "%3c", buf, 2);
    ok(ret == 0, "Wrong number of arguments read: %d\n", ret);
    ok(buf[0]=='\0', "buf = %s\n", buf);
    ok(buf[1]=='2', "buf[1] = %d\n", buf[1]);
    ok(buf[2]=='a', "buf[2] = %d\n", buf[2]);

    buf[3] = 'a';
    buf[4] = 0;
    ret = psscanf_s("123", "%3c", buf, 3);
    ok(!strcmp("123a", buf), "buf = %s\n", buf);

    i = 1;
    ret = psscanf_s("123 123", "%s %d", buf, 2, &i);
    ok(ret == 0, "Wrong number of arguments read: %d\n", ret);
    ok(i==1, "i = %d\n", i);

    i = 1;
    ret = psscanf_s("123 123", "%d %s", &i, buf, 2);
    ok(ret == 1, "Wrong number of arguments read: %d\n", ret);
    ok(i==123, "i = %d\n", i);
}

static void test_swscanf( void )
{
    wchar_t buffer[100], results[100];
    int result, ret;
    WCHAR c;

    /* check WEOF */
    /* WEOF is an unsigned short -1 but swscanf returns int
       so it should be sign-extended */
    buffer[0] = 0;
    ret = swscanf(buffer, L"%d", &result);
    /* msvcrt returns 0 but should return -1 (later versions do) */
    ok( ret == (short)WEOF || broken(ret == 0),
        "swscanf returns %x instead of %x\n", ret, WEOF );

    ret = swscanf(L" \t\n\n", L"%s", results);
    /* sscanf returns EOF under this case, but swscanf does not return WEOF */
    ok( ret == 0, "ret = %d\n", ret );

    buffer[0] = 'a';
    buffer[1] = 0x1234;
    buffer[2] = 0x1234;
    buffer[3] = 'b';
    ret = swscanf(buffer, L"a\x1234%\x1234%c", &c);
    ok(ret == 1, "swscanf returned %d\n", ret);
    ok(c == 'b', "c = %x\n", c);
}

static void test_swscanf_s(void)
{
    int (WINAPIV *pswscanf_s)(const wchar_t*,const wchar_t*,...);
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");
    wchar_t buf[2], out[2];
    int ret;

    pswscanf_s = (void*)GetProcAddress(hmod, "swscanf_s");
    if(!pswscanf_s) {
        win_skip("swscanf_s not available\n");
        return;
    }

    buf[0] = 'a';
    buf[1] = '1';
    out[1] = 'b';
    ret = pswscanf_s(buf, L"%c", out, 1);
    ok(ret == 1, "swscanf_s returned %d\n", ret);
    ok(out[0] == 'a', "out[0] = %x\n", out[0]);
    ok(out[1] == 'b', "out[1] = %x\n", out[1]);

    ret = pswscanf_s(buf, L"%[a-z]", out, 1);
    ok(!ret, "swscanf_s returned %d\n", ret);

    ret = pswscanf_s(buf, L"%[a-z]", out, 2);
    ok(ret == 1, "swscanf_s returned %d\n", ret);
    ok(out[0] == 'a', "out[0] = %x\n", out[0]);
    ok(!out[1], "out[1] = %x\n", out[1]);
}

START_TEST(scanf)
{
    test_fscanf();
    test_sscanf();
    test_sscanf_s();
    test_swscanf();
    test_swscanf_s();
}
