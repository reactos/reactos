/*
 * Conformance tests for *printf functions.
 *
 * Copyright 2002 Uwe Bonnes
 * Copyright 2004 Aneurin Price
 * Copyright 2005 Mike McCormack
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

/* With Visual Studio >= 2005,  swprintf() takes an extra parameter unless
 * the following macro is defined.
 */
#define _CRT_NON_CONFORMING_SWPRINTFS
 
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/test.h"

static void test_sprintf( void )
{
    char buffer[100];
    const char *format;
    double pnumber=789456123;
    int x, r;
    WCHAR wide[] = { 'w','i','d','e',0};

    format = "%+#23.15e";
    r = sprintf(buffer,format,pnumber);
    ok(!strcmp(buffer,"+7.894561230000000e+008"),"+#23.15e failed: '%s'\n", buffer);
    ok( r==23, "return count wrong\n");

    format = "%-#23.15e";
    r = sprintf(buffer,format,pnumber);
    ok(!strcmp(buffer,"7.894561230000000e+008 "),"-#23.15e failed: '%s'\n", buffer);
    ok( r==23, "return count wrong\n");

    format = "%#23.15e";
    r = sprintf(buffer,format,pnumber);
    ok(!strcmp(buffer," 7.894561230000000e+008"),"#23.15e failed: '%s'\n", buffer);
    ok( r==23, "return count wrong\n");

    format = "%#1.1g";
    r = sprintf(buffer,format,pnumber);
    ok(!strcmp(buffer,"8.e+008"),"#1.1g failed: '%s'\n", buffer);
    ok( r==7, "return count wrong\n");

    format = "%I64d";
    r = sprintf(buffer,format,((ULONGLONG)0xffffffff)*0xffffffff);
    ok(!strcmp(buffer,"-8589934591"),"Problem with long long\n");
    ok( r==11, "return count wrong\n");

    format = "%+8I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"    +100") && r==8,"+8I64d failed: '%s'\n", buffer);

    format = "%+.8I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"+00000100") && r==9,"+.8I64d failed: '%s'\n", buffer);

    format = "%+10.8I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer," +00000100") && r==10,"+10.8I64d failed: '%s'\n", buffer);
    format = "%_1I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"_1I64d") && r==6,"_1I64d failed\n");

    format = "%-1.5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-00100") && r==6,"-1.5I64d failed: '%s'\n", buffer);

    format = "%5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"  100") && r==5,"5I64d failed: '%s'\n", buffer);

    format = "%5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer," -100") && r==5,"5I64d failed: '%s'\n", buffer);

    format = "%-5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"100  ") && r==5,"-5I64d failed: '%s'\n", buffer);

    format = "%-5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-100 ") && r==5,"-5I64d failed: '%s'\n", buffer);

    format = "%-.5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"00100") && r==5,"-.5I64d failed: '%s'\n", buffer);

    format = "%-.5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-00100") && r==6,"-.5I64d failed: '%s'\n", buffer);

    format = "%-8.5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"00100   ") && r==8,"-8.5I64d failed: '%s'\n", buffer);

    format = "%-8.5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-00100  ") && r==8,"-8.5I64d failed: '%s'\n", buffer);

    format = "%05I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"00100") && r==5,"05I64d failed: '%s'\n", buffer);

    format = "%05I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-0100") && r==5,"05I64d failed: '%s'\n", buffer);

    format = "% I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer," 100") && r==4,"' I64d' failed: '%s'\n", buffer);

    format = "% I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-100") && r==4,"' I64d' failed: '%s'\n", buffer);

    format = "% 5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"  100") && r==5,"' 5I64d' failed: '%s'\n", buffer);

    format = "% 5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer," -100") && r==5,"' 5I64d' failed: '%s'\n", buffer);

    format = "% .5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer," 00100") && r==6,"' .5I64d' failed: '%s'\n", buffer);

    format = "% .5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"-00100") && r==6,"' .5I64d' failed: '%s'\n", buffer);

    format = "% 8.5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"   00100") && r==8,"' 8.5I64d' failed: '%s'\n", buffer);

    format = "% 8.5I64d";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"  -00100") && r==8,"' 8.5I64d' failed: '%s'\n", buffer);

    format = "%.0I64d";
    r = sprintf(buffer,format,(LONGLONG)0);
    ok(r==0,".0I64d failed: '%s'\n", buffer);

    format = "%#+21.18I64x";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer," 0x00ffffffffffffff9c") && r==21,"#+21.18I64x failed: '%s'\n", buffer);

    format = "%#.25I64o";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"0001777777777777777777634") && r==25,"#.25I64o failed: '%s'\n", buffer);

    format = "%#+24.20I64o";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer," 01777777777777777777634") && r==24,"#+24.20I64o failed: '%s'\n", buffer);

    format = "%#+18.21I64X";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"0X00000FFFFFFFFFFFFFF9C") && r==23,"#+18.21I64X failed: '%s '\n", buffer);

    format = "%#+20.24I64o";
    r = sprintf(buffer,format,(LONGLONG)-100);
    ok(!strcmp(buffer,"001777777777777777777634") && r==24,"#+20.24I64o failed: '%s'\n", buffer);

    format = "%#+25.22I64u";
    r = sprintf(buffer,format,(LONGLONG)-1);
    ok(!strcmp(buffer,"   0018446744073709551615") && r==25,"#+25.22I64u conversion failed: '%s'\n", buffer);

    format = "%#+25.22I64u";
    r = sprintf(buffer,format,(LONGLONG)-1);
    ok(!strcmp(buffer,"   0018446744073709551615") && r==25,"#+25.22I64u failed: '%s'\n", buffer);

    format = "%#+30.25I64u";
    r = sprintf(buffer,format,(LONGLONG)-1);
    ok(!strcmp(buffer,"     0000018446744073709551615") && r==30,"#+30.25I64u failed: '%s'\n", buffer);

    format = "%+#25.22I64d";
    r = sprintf(buffer,format,(LONGLONG)-1);
    ok(!strcmp(buffer,"  -0000000000000000000001") && r==25,"+#25.22I64d failed: '%s'\n", buffer);

    format = "%#-8.5I64o";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"00144   ") && r==8,"-8.5I64o failed: '%s'\n", buffer);

    format = "%#-+ 08.5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"+00100  ") && r==8,"'#-+ 08.5I64d failed: '%s'\n", buffer);

    format = "%#-+ 08.5I64d";
    r = sprintf(buffer,format,(LONGLONG)100);
    ok(!strcmp(buffer,"+00100  ") && r==8,"#-+ 08.5I64d failed: '%s'\n", buffer);

    format = "%.80I64d";
    r = sprintf(buffer,format,(LONGLONG)1);
    ok(r==80,"%s format failed\n", format);

    format = "% .80I64d";
    r = sprintf(buffer,format,(LONGLONG)1);
    ok(r==81,"%s format failed\n", format);

    format = "% .80d";
    r = sprintf(buffer,format,1);
    ok(r==81,"%s format failed\n", format);

    format = "%lld";
    r = sprintf(buffer,format,((ULONGLONG)0xffffffff)*0xffffffff);
    ok( r == 1 || r == 11, "return count wrong %d\n", r);
    if (r == 11)  /* %ll works on Vista */
        ok(!strcmp(buffer, "-8589934591"), "Problem with \"ll\" interpretation '%s'\n", buffer);
    else
        ok(!strcmp(buffer, "1"), "Problem with \"ll\" interpretation '%s'\n", buffer);

    format = "%I";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer, "I"), "Problem with \"I\" interpretation\n");
    ok( r==1, "return count wrong\n");

    format = "%I0d";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"I0d"),"I0d failed\n");
    ok( r==3, "return count wrong\n");

    format = "%I32d";
    r = sprintf(buffer,format,1);
    if (r == 1)
    {
        ok(!strcmp(buffer,"1"),"I32d failed, got '%s'\n",buffer);
    }
    else
    {
        /* Older versions don't grok I32 format */
        ok(r == 4 && !strcmp(buffer,"I32d"),"I32d failed, got '%s',%d\n",buffer,r);
    }

    format = "%I64D";
    r = sprintf(buffer,format,(LONGLONG)-1);
    ok(!strcmp(buffer,"D"),"I64D failed: %s\n",buffer);
    ok( r==1, "return count wrong\n");

    format = "% d";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer, " 1"),"Problem with sign place-holder: '%s'\n",buffer);
    ok( r==2, "return count wrong\n");

    format = "%+ d";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer, "+1"),"Problem with sign flags: '%s'\n",buffer);
    ok( r==2, "return count wrong\n");

    format = "%S";
    r = sprintf(buffer,format,wide);
    ok(!strcmp(buffer,"wide"),"Problem with wide string format\n");
    ok( r==4, "return count wrong\n");

    format = "%04c";
    r = sprintf(buffer,format,'1');
    ok(!strcmp(buffer,"0001"),"Character not zero-prefixed \"%s\"\n",buffer);
    ok( r==4, "return count wrong\n");

    format = "%-04c";
    r = sprintf(buffer,format,'1');
    ok(!strcmp(buffer,"1   "),"Character zero-padded and/or not left-adjusted \"%s\"\n",buffer);
    ok( r==4, "return count wrong\n");

    if (sizeof(void *) == 8)
    {
        format = "%p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0000000000000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        ok( r==16, "return count wrong\n");

        format = "%#020p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"  0X0000000000000039"),"Pointer formatted incorrectly\n");
        ok( r==20, "return count wrong\n");

        format = "%Fp";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0000000000000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        ok( r==16, "return count wrong\n");

        format = "%#-020p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0X0000000000000039  "),"Pointer formatted incorrectly\n");
        ok( r==20, "return count wrong\n");
    }
    else
    {
        format = "%p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        ok( r==8, "return count wrong\n");

        format = "%#012p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"  0X00000039"),"Pointer formatted incorrectly\n");
        ok( r==12, "return count wrong\n");

        format = "%Fp";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        ok( r==8, "return count wrong\n");

        format = "%#-012p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0X00000039  "),"Pointer formatted incorrectly\n");
        ok( r==12, "return count wrong\n");
    }

    format = "%04s";
    r = sprintf(buffer,format,"foo");
    ok(!strcmp(buffer,"0foo"),"String not zero-prefixed \"%s\"\n",buffer);
    ok( r==4, "return count wrong\n");

    format = "%.1s";
    r = sprintf(buffer,format,"foo");
    ok(!strcmp(buffer,"f"),"Precision ignored \"%s\"\n",buffer);
    ok( r==1, "return count wrong\n");

    format = "%.*s";
    r = sprintf(buffer,format,1,"foo");
    ok(!strcmp(buffer,"f"),"Precision ignored \"%s\"\n",buffer);
    ok( r==1, "return count wrong\n");

    format = "%*s";
    r = sprintf(buffer,format,-5,"foo");
    ok(!strcmp(buffer,"foo  "),"Negative field width ignored \"%s\"\n",buffer);
    ok( r==5, "return count wrong\n");

    format = "hello";
    r = sprintf(buffer, format);
    ok(!strcmp(buffer,"hello"), "failed\n");
    ok( r==5, "return count wrong\n");

    format = "%ws";
    r = sprintf(buffer, format, wide);
    ok(!strcmp(buffer,"wide"), "failed\n");
    ok( r==4, "return count wrong\n");

    format = "%-10ws";
    r = sprintf(buffer, format, wide );
    ok(!strcmp(buffer,"wide      "), "failed\n");
    ok( r==10, "return count wrong\n");

    format = "%10ws";
    r = sprintf(buffer, format, wide );
    ok(!strcmp(buffer,"      wide"), "failed\n");
    ok( r==10, "return count wrong\n");

    format = "%#+ -03whlls";
    r = sprintf(buffer, format, wide );
    ok(!strcmp(buffer,"wide"), "failed\n");
    ok( r==4, "return count wrong\n");

    format = "%w0s";
    r = sprintf(buffer, format, wide );
    ok(!strcmp(buffer,"0s"), "failed\n");
    ok( r==2, "return count wrong\n");

    format = "%w-s";
    r = sprintf(buffer, format, wide );
    ok(!strcmp(buffer,"-s"), "failed\n");
    ok( r==2, "return count wrong\n");

    format = "%ls";
    r = sprintf(buffer, format, wide );
    ok(!strcmp(buffer,"wide"), "failed\n");
    ok( r==4, "return count wrong\n");

    format = "%Ls";
    r = sprintf(buffer, format, "not wide" );
    ok(!strcmp(buffer,"not wide"), "failed\n");
    ok( r==8, "return count wrong\n");

    format = "%b";
    r = sprintf(buffer, format);
    ok(!strcmp(buffer,"b"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "%3c";
    r = sprintf(buffer, format,'a');
    ok(!strcmp(buffer,"  a"), "failed\n");
    ok( r==3, "return count wrong\n");

    format = "%3d";
    r = sprintf(buffer, format,1234);
    ok(!strcmp(buffer,"1234"), "failed\n");
    ok( r==4, "return count wrong\n");

    format = "%3h";
    r = sprintf(buffer, format);
    ok(!strcmp(buffer,""), "failed\n");
    ok( r==0, "return count wrong\n");

    format = "%j%k%m%q%r%t%v%y%z";
    r = sprintf(buffer, format);
    ok(!strcmp(buffer,"jkmqrtvyz"), "failed\n");
    ok( r==9, "return count wrong\n");

    format = "asdf%n";
    x = 0;
    r = sprintf(buffer, format, &x );
    if (r == -1)
    {
        /* %n format is disabled by default on vista */
        /* FIXME: should test with _set_printf_count_output */
        ok(x == 0, "should not write to x: %d\n", x);
    }
    else
    {
        ok(x == 4, "should write to x: %d\n", x);
        ok(!strcmp(buffer,"asdf"), "failed\n");
        ok( r==4, "return count wrong: %d\n", r);
    }

    format = "%-1d";
    r = sprintf(buffer, format,2);
    ok(!strcmp(buffer,"2"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "%2.4f";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer,"8.6000"), "failed\n");
    ok( r==6, "return count wrong\n");

    format = "%0f";
    r = sprintf(buffer, format,0.6);
    ok(!strcmp(buffer,"0.600000"), "failed\n");
    ok( r==8, "return count wrong\n");

    format = "%.0f";
    r = sprintf(buffer, format,0.6);
    ok(!strcmp(buffer,"1"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "%2.4e";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer,"8.6000e+000"), "failed\n");
    ok( r==11, "return count wrong\n");

    format = "%2.4g";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer,"8.6"), "failed\n");
    ok( r==3, "return count wrong\n");

    format = "%-i";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,"-1"), "failed\n");
    ok( r==2, "return count wrong\n");

    format = "%-i";
    r = sprintf(buffer, format,1);
    ok(!strcmp(buffer,"1"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "%+i";
    r = sprintf(buffer, format,1);
    ok(!strcmp(buffer,"+1"), "failed\n");
    ok( r==2, "return count wrong\n");

    format = "%o";
    r = sprintf(buffer, format,10);
    ok(!strcmp(buffer,"12"), "failed\n");
    ok( r==2, "return count wrong\n");

    format = "%p";
    r = sprintf(buffer, format,0);
    if (sizeof(void *) == 8)
    {
        ok(!strcmp(buffer,"0000000000000000"), "failed\n");
        ok( r==16, "return count wrong\n");
    }
    else
    {
        ok(!strcmp(buffer,"00000000"), "failed\n");
        ok( r==8, "return count wrong\n");
    }

    format = "%s";
    r = sprintf(buffer, format,0);
    ok(!strcmp(buffer,"(null)"), "failed\n");
    ok( r==6, "return count wrong\n");

    format = "%s";
    r = sprintf(buffer, format,"%%%%");
    ok(!strcmp(buffer,"%%%%"), "failed\n");
    ok( r==4, "return count wrong\n");

    format = "%u";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,"4294967295"), "failed\n");
    ok( r==10, "return count wrong\n");

    format = "%w";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,""), "failed\n");
    ok( r==0, "return count wrong\n");

    format = "%h";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,""), "failed\n");
    ok( r==0, "return count wrong\n");

    format = "%z";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,"z"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "%j";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,"j"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "%F";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,""), "failed\n");
    ok( r==0, "return count wrong\n");

    format = "%H";
    r = sprintf(buffer, format,-1);
    ok(!strcmp(buffer,"H"), "failed\n");
    ok( r==1, "return count wrong\n");

    format = "x%cx";
    r = sprintf(buffer, format, 0x100+'X');
    ok(!strcmp(buffer,"xXx"), "failed\n");
    ok( r==3, "return count wrong\n");

    format = "%%0";
    r = sprintf(buffer, format);
    ok(!strcmp(buffer,"%0"), "failed: \"%s\"\n", buffer);
    ok( r==2, "return count wrong\n");
}

static void test_swprintf( void )
{
    wchar_t buffer[100];
    const wchar_t I64d[] = {'%','I','6','4','d',0};
    double pnumber=789456123;
    const wchar_t TwentyThreePoint15e[]= {'%','+','#','2','3','.','1','5','e',0};
    const wchar_t e008[] = {'e','+','0','0','8',0};
    const wchar_t string_w[] = {'s','t','r','i','n','g',0};
    const char string[] = "string";
    const wchar_t S[]={'%','S',0};
    const wchar_t hs[] = {'%', 'h', 's', 0};

    swprintf(buffer,TwentyThreePoint15e,pnumber);
    ok(wcsstr(buffer,e008) != 0,"Sprintf different\n");
    swprintf(buffer,I64d,((ULONGLONG)0xffffffff)*0xffffffff);
      ok(wcslen(buffer) == 11,"Problem with long long\n");
    swprintf(buffer,S,string);
      ok(wcslen(buffer) == 6,"Problem with \"%%S\" interpretation\n");
   swprintf(buffer, hs, string);
   ok( wcscmp(string_w,buffer) == 0, "swprintf failed with %%hs\n");
}

static void test_snprintf (void)
{
    struct snprintf_test {
        const char *format;
        int expected;
    };
    /* Pre-2.1 libc behaviour, not C99 compliant. */
    const struct snprintf_test tests[] = {{"short", 5},
                                          {"justfit", 7},
                                          {"justfits", 8},
                                          {"muchlonger", -1}};
    char buffer[8];
    const int bufsiz = sizeof buffer;
    unsigned int i;

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        const char *fmt  = tests[i].format;
        const int expect = tests[i].expected;
        const int n      = _snprintf (buffer, bufsiz, fmt);
        const int valid  = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid),
            "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
    };
}

static void test_fcvt(void)
{
    char *str;
    int dec=100, sign=100;
    
    /* Numbers less than 1.0 with different precisions */
    str = _fcvt(0.0001, 1, &dec, &sign );
    ok( 0 == strcmp(str,""), "bad return '%s'\n", str);
    ok( -3 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.0001, -10, &dec, &sign );
    ok( 0 == strcmp(str,""), "bad return '%s'\n", str);
    ok( -3 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.0001, 10, &dec, &sign );
    ok( 0 == strcmp(str,"1000000"), "bad return '%s'\n", str);
    ok( -3 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    /* Basic sign test */
    str = _fcvt(-111.0001, 5, &dec, &sign );
    ok( 0 == strcmp(str,"11100010"), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong %d\n", dec);
    ok( 1 == sign, "sign wrong\n");

    str = _fcvt(111.0001, 5, &dec, &sign );
    ok( 0 == strcmp(str,"11100010"), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong\n");
    ok( 0 == sign, "sign wrong\n");

    /* 0.0 with different precisions */
    str = _fcvt(0.0, 5, &dec, &sign );
    ok( 0 == strcmp(str,"00000"), "bad return '%s'\n", str);
    ok( 0 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.0, 0, &dec, &sign );
    ok( 0 == strcmp(str,""), "bad return '%s'\n", str);
    ok( 0 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.0, -1, &dec, &sign );
    ok( 0 == strcmp(str,""), "bad return '%s'\n", str);
    ok( 0 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    /* Numbers > 1.0 with 0 or -ve precision */
    str = _fcvt(-123.0001, 0, &dec, &sign );
    ok( 0 == strcmp(str,"123"), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong %d\n", dec);
    ok( 1 == sign, "sign wrong\n");

    str = _fcvt(-123.0001, -1, &dec, &sign );
    ok( 0 == strcmp(str,"12"), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong %d\n", dec);
    ok( 1 == sign, "sign wrong\n");

    str = _fcvt(-123.0001, -2, &dec, &sign );
    ok( 0 == strcmp(str,"1"), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong %d\n", dec);
    ok( 1 == sign, "sign wrong\n");

    str = _fcvt(-123.0001, -3, &dec, &sign );
    ok( 0 == strcmp(str,""), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong %d\n", dec);
    ok( 1 == sign, "sign wrong\n");

    /* Numbers > 1.0, but with rounding at the point of precision */
    str = _fcvt(99.99, 1, &dec, &sign );
    ok( 0 == strcmp(str,"1000"), "bad return '%s'\n", str);
    ok( 3 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    /* Numbers < 1.0 where rounding occurs at the point of precision */
    str = _fcvt(0.00636, 2, &dec, &sign );
    ok( 0 == strcmp(str,"1"), "bad return '%s'\n", str);
    ok( -1 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.00636, 3, &dec, &sign );
    ok( 0 == strcmp(str,"6"), "bad return '%s'\n", str);
    ok( -2 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.09999999996, 2, &dec, &sign );
    ok( 0 == strcmp(str,"10"), "bad return '%s'\n", str);
    ok( 0 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");

    str = _fcvt(0.6, 0, &dec, &sign );
    ok( 0 == strcmp(str,"1"), "bad return '%s'\n", str);
    ok( 1 == dec, "dec wrong %d\n", dec);
    ok( 0 == sign, "sign wrong\n");
}

static int _vsnwprintf_wrapper(wchar_t *str, size_t len, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = _vsnwprintf(str, len, format, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vsnwprintf(void)
{
    const wchar_t format[] = {'%','w','s','%','w','s','%','w','s',0};
    const wchar_t one[]    = {'o','n','e',0};
    const wchar_t two[]    = {'t','w','o',0};
    const wchar_t three[]  = {'t','h','r','e','e',0};

    int ret;
    wchar_t str[32];
    char buf[32];

    ret = _vsnwprintf_wrapper( str, sizeof(str)/sizeof(str[0]), format, one, two, three );

    ok( ret == 11, "got %d expected 11\n", ret );
    WideCharToMultiByte( CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL );
    ok( !strcmp(buf, "onetwothree"), "got %s expected 'onetwothree'\n", buf );
}

START_TEST(printf)
{
    test_sprintf();
    test_swprintf();
    test_snprintf();
    test_fcvt();
    test_vsnwprintf();
}
