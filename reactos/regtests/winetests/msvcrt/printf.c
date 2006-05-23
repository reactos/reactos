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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <stdio.h>

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
    todo_wine {
    ok(!strcmp(buffer,"+7.894561230000000e+008"),"exponent format incorrect\n");
    }
    ok( r==23, "return count wrong\n");

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

    format = "%lld";
    r = sprintf(buffer,format,((ULONGLONG)0xffffffff)*0xffffffff);
    ok(!strcmp(buffer, "1"), "Problem with \"ll\" interpretation\n");
    ok( r==1, "return count wrong\n");

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
    ok(!strcmp(buffer,"1"),"I32d failed\n");
    ok( r==1, "return count wrong\n");

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

    format = "%#-012p";
    r = sprintf(buffer,format,(void *)57);
    ok(!strcmp(buffer,"0X00000039  "),"Pointer formatted incorrectly\n");
    ok( r==12, "return count wrong\n");

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
    ok(x == 4, "should write to x\n");
    ok(!strcmp(buffer,"asdf"), "failed\n");
    ok( r==4, "return count wrong\n");

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

    todo_wine {
    format = "%2.4e";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer,"8.6000e+000"), "failed\n");
    ok( r==11, "return count wrong\n");
    }

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
    ok(!strcmp(buffer,"00000000"), "failed\n");
    ok( r==8, "return count wrong\n");

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
    const char string[]={'s','t','r','i','n','g',0};
    const wchar_t S[]={'%','S',0};
    swprintf(buffer,TwentyThreePoint15e,pnumber);
    todo_wine
      {
        ok(wcsstr(buffer,e008) != 0,"Sprintf different\n");
      }
    swprintf(buffer,I64d,((ULONGLONG)0xffffffff)*0xffffffff);
      ok(wcslen(buffer) == 11,"Problem with long long\n");
    swprintf(buffer,S,string);
      ok(wcslen(buffer) == 6,"Problem with \"%%S\" interpretation\n");
}

static void test_fwprintf( void )
{
    const char *string="not a wide string";
    todo_wine
      {
        ok(fwprintf(fopen("nul","r+"),(wchar_t *)string) == -1,"Non-wide string should not be printed by fwprintf\n");
      }
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
    
    str = _fcvt(0.0001, 1, &dec, &sign );
    todo_wine {
    ok( 0 == strcmp(str,""), "bad return\n");
    ok( -3 == dec, "dec wrong\n");
    }
    ok( 0 == sign, "dec wrong\n");

    str = _fcvt(0.0001, -10, &dec, &sign );
    todo_wine {
    ok( 0 == strcmp(str,""), "bad return\n");
    ok( -3 == dec, "dec wrong\n");
    }
    ok( 0 == sign, "dec wrong\n");

    str = _fcvt(0.0001, 10, &dec, &sign );
    todo_wine {
    ok( 0 == strcmp(str,"1000000"), "bad return\n");
    ok( -3 == dec, "dec wrong\n");
    }
    ok( 0 == sign, "dec wrong\n");

    str = _fcvt(-111.0001, 5, &dec, &sign );
    todo_wine {
    ok( 0 == strcmp(str,"11100010"), "bad return\n");
    ok( 3 == dec, "dec wrong\n");
    }
    ok( 1 == sign, "dec wrong\n");

    str = _fcvt(111.0001, 5, &dec, &sign );
    todo_wine {
    ok( 0 == strcmp(str,"11100010"), "bad return\n");
    ok( 3 == dec, "dec wrong\n");
    }
    ok( 0 == sign, "dec wrong\n");

    str = _fcvt(0.0, 5, &dec, &sign );
    todo_wine {
    ok( 0 == strcmp(str,"00000"), "bad return\n");
    ok( 0 == dec, "dec wrong\n");
    }
    ok( 0 == sign, "dec wrong\n");
}

START_TEST(printf)
{
    test_sprintf();
    test_swprintf();
    test_fwprintf();
    test_snprintf();
    test_fcvt();
}
