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
#include <errno.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/test.h"

static inline float __port_infinity(void)
{
    static const unsigned __inf_bytes = 0x7f800000;
    return *(const float *)&__inf_bytes;
}
#define INFINITY __port_infinity()

static inline float __port_nan(void)
{
    static const unsigned __nan_bytes = 0x7fc00000;
    return *(const float *)&__nan_bytes;
}
#define NAN __port_nan()

static inline float __port_ind(void)
{
    static const unsigned __ind_bytes = 0xffc00000;
    return *(const float *)&__ind_bytes;
}
#define IND __port_ind()

static int (__cdecl *p__vscprintf)(const char *format, __ms_va_list valist);
static int (__cdecl *p__vscwprintf)(const wchar_t *format, __ms_va_list valist);
static int (__cdecl *p__vsnwprintf_s)(wchar_t *str, size_t sizeOfBuffer,
                                      size_t count, const wchar_t *format,
                                      __ms_va_list valist);
static int (__cdecl *p__ecvt_s)(char *buffer, size_t length, double number,
                                int ndigits, int *decpt, int *sign);
static int (__cdecl *p__fcvt_s)(char *buffer, size_t length, double number,
                                int ndigits, int *decpt, int *sign);
static unsigned int (__cdecl *p__get_output_format)(void);
static unsigned int (__cdecl *p__set_output_format)(unsigned int);
static int (__cdecl *p__vsprintf_p)(char*, size_t, const char*, __ms_va_list);
static int (__cdecl *p_vswprintf)(wchar_t *str, const wchar_t *format, __ms_va_list valist);
static int (__cdecl *p__vswprintf)(wchar_t *str, const wchar_t *format, __ms_va_list valist);
static int (__cdecl *p__vswprintf_l)(wchar_t *str, const wchar_t *format,
                                     void *locale, __ms_va_list valist);
static int (__cdecl *p__vswprintf_c)(wchar_t *str, size_t size, const wchar_t *format,
                                     __ms_va_list valist);
static int (__cdecl *p__vswprintf_c_l)(wchar_t *str, size_t size, const wchar_t *format,
                                       void *locale, __ms_va_list valist);
static int (__cdecl *p__vswprintf_p_l)(wchar_t *str, size_t size, const wchar_t *format,
                                       void *locale, __ms_va_list valist);

static void init( void )
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p__vscprintf = (void *)GetProcAddress(hmod, "_vscprintf");
    p__vscwprintf = (void *)GetProcAddress(hmod, "_vscwprintf");
    p__vsnwprintf_s = (void *)GetProcAddress(hmod, "_vsnwprintf_s");
    p__ecvt_s = (void *)GetProcAddress(hmod, "_ecvt_s");
    p__fcvt_s = (void *)GetProcAddress(hmod, "_fcvt_s");
    p__get_output_format = (void *)GetProcAddress(hmod, "_get_output_format");
    p__set_output_format = (void *)GetProcAddress(hmod, "_set_output_format");
    p__vsprintf_p = (void*)GetProcAddress(hmod, "_vsprintf_p");
    p_vswprintf = (void*)GetProcAddress(hmod, "vswprintf");
    p__vswprintf = (void*)GetProcAddress(hmod, "_vswprintf");
    p__vswprintf_l = (void*)GetProcAddress(hmod, "_vswprintf_l");
    p__vswprintf_c = (void*)GetProcAddress(hmod, "_vswprintf_c");
    p__vswprintf_c_l = (void*)GetProcAddress(hmod, "_vswprintf_c_l");
    p__vswprintf_p_l = (void*)GetProcAddress(hmod, "_vswprintf_p_l");
}

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

    format = "%zx";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer, "zx"), "Problem with \"z\" interpretation\n");
    ok( r==2, "return count wrong\n");

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

    format = "%#012x";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"0x0000000001"),"Hexadecimal zero-padded \"%s\"\n",buffer);
    ok( r==12, "return count wrong\n");

    r = sprintf(buffer,format,0);
    ok(!strcmp(buffer,"000000000000"),"Hexadecimal zero-padded \"%s\"\n",buffer);
    ok( r==12, "return count wrong\n");

    format = "%#04.8x";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"0x00000001"), "Hexadecimal zero-padded precision \"%s\"\n",buffer);
    ok( r==10, "return count wrong\n");

    r = sprintf(buffer,format,0);
    ok(!strcmp(buffer,"00000000"), "Hexadecimal zero-padded precision \"%s\"\n",buffer);
    ok( r==8, "return count wrong\n");

    format = "%#-08.2x";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"0x01    "), "Hexadecimal zero-padded not left-adjusted \"%s\"\n",buffer);
    ok( r==8, "return count wrong\n");

    r = sprintf(buffer,format,0);
    ok(!strcmp(buffer,"00      "), "Hexadecimal zero-padded not left-adjusted \"%s\"\n",buffer);
    ok( r==8, "return count wrong\n");

    format = "%#.0x";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"0x1"), "Hexadecimal zero-padded zero-precision \"%s\"\n",buffer);
    ok( r==3, "return count wrong\n");

    r = sprintf(buffer,format,0);
    ok(!strcmp(buffer,""), "Hexadecimal zero-padded zero-precision \"%s\"\n",buffer);
    ok( r==0, "return count wrong\n");

    format = "%#08o";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"00000001"), "Octal zero-padded \"%s\"\n",buffer);
    ok( r==8, "return count wrong\n");

    format = "%#o";
    r = sprintf(buffer,format,1);
    ok(!strcmp(buffer,"01"), "Octal zero-padded \"%s\"\n",buffer);
    ok( r==2, "return count wrong\n");

    r = sprintf(buffer,format,0);
    ok(!strcmp(buffer,"0"), "Octal zero-padded \"%s\"\n",buffer);
    ok( r==1, "return count wrong\n");

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

        format = "%Np";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0000000000000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        ok( r==16, "return count wrong\n");

        format = "%#-020p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0X0000000000000039  "),"Pointer formatted incorrectly\n");
        ok( r==20, "return count wrong\n");

        format = "%Ix %d";
        r = sprintf(buffer,format,(size_t)0x12345678123456,1);
        ok(!strcmp(buffer,"12345678123456 1"),"buffer = %s\n",buffer);
        ok( r==16, "return count wrong\n");
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

        format = "%Np";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
        ok( r==8, "return count wrong\n");

        format = "%#-012p";
        r = sprintf(buffer,format,(void *)57);
        ok(!strcmp(buffer,"0X00000039  "),"Pointer formatted incorrectly\n");
        ok( r==12, "return count wrong\n");

        format = "%Ix %d";
        r = sprintf(buffer,format,0x123456,1);
        ok(!strcmp(buffer,"123456 1"),"buffer = %s\n",buffer);
        ok( r==8, "return count wrong\n");
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

    format = "% 2.4e";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer," 8.6000e+000"), "failed: %s\n", buffer);
    ok( r==12, "return count wrong\n");

    format = "% 014.4e";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer," 008.6000e+000"), "failed: %s\n", buffer);
    ok( r==14, "return count wrong\n");

    format = "% 2.4e";
    r = sprintf(buffer, format,-8.6);
    ok(!strcmp(buffer,"-8.6000e+000"), "failed: %s\n", buffer);
    ok( r==12, "return count wrong\n");

    format = "%+2.4e";
    r = sprintf(buffer, format,8.6);
    ok(!strcmp(buffer,"+8.6000e+000"), "failed: %s\n", buffer);
    ok( r==12, "return count wrong\n");

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

    format = "%N";
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

    format = "%hx";
    r = sprintf(buffer, format, 0x12345);
    ok(!strcmp(buffer,"2345"), "failed \"%s\"\n", buffer);

    format = "%hhx";
    r = sprintf(buffer, format, 0x123);
    ok(!strcmp(buffer,"123"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, 0x12345);
    ok(!strcmp(buffer,"2345"), "failed \"%s\"\n", buffer);

    format = "%lf";
    r = sprintf(buffer, format, IND);
    ok(r==9, "r = %d\n", r);
    ok(!strcmp(buffer, "-1.#IND00"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, NAN);
    ok(r==8, "r = %d\n", r);
    ok(!strcmp(buffer, "1.#QNAN0"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, INFINITY);
    ok(r==8, "r = %d\n", r);
    ok(!strcmp(buffer, "1.#INF00"), "failed: \"%s\"\n", buffer);

    format = "%le";
    r = sprintf(buffer, format, IND);
    ok(r==14, "r = %d\n", r);
    ok(!strcmp(buffer, "-1.#IND00e+000"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, NAN);
    ok(r==13, "r = %d\n", r);
    ok(!strcmp(buffer, "1.#QNAN0e+000"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, INFINITY);
    ok(r==13, "r = %d\n", r);
    ok(!strcmp(buffer, "1.#INF00e+000"), "failed: \"%s\"\n", buffer);

    format = "%lg";
    r = sprintf(buffer, format, IND);
    ok(r==7, "r = %d\n", r);
    ok(!strcmp(buffer, "-1.#IND"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, NAN);
    ok(r==7, "r = %d\n", r);
    ok(!strcmp(buffer, "1.#QNAN"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, INFINITY);
    ok(r==6, "r = %d\n", r);
    ok(!strcmp(buffer, "1.#INF"), "failed: \"%s\"\n", buffer);

    format = "%010.2lf";
    r = sprintf(buffer, format, IND);
    ok(r==10, "r = %d\n", r);
    ok(!strcmp(buffer, "-000001.#J"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, NAN);
    ok(r==10, "r = %d\n", r);
    ok(!strcmp(buffer, "0000001.#R"), "failed: \"%s\"\n", buffer);
    r = sprintf(buffer, format, INFINITY);
    ok(r==10, "r = %d\n", r);
    ok(!strcmp(buffer, "0000001.#J"), "failed: \"%s\"\n", buffer);
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
    }
}

static void test_fprintf(void)
{
    static const char file_name[] = "fprintf.tst";
    static const WCHAR utf16_test[] = {'u','n','i','c','o','d','e','\n',0};

    FILE *fp = fopen(file_name, "wb");
    char buf[1024];
    int ret;

    ret = fprintf(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);

    ret = fprintf(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 26, "ftell returned %d\n", ret);

    ret = fwprintf(fp, utf16_test);
    ok(ret == 8, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 42, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    ret = fscanf(fp, "%[^\n] ", buf);
    ok(ret == 1, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test"), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 26, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\n", 14), "buf = %s\n", buf);

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 41, "ret =  %d\n", ret);
    ok(!memcmp(buf, utf16_test, sizeof(utf16_test)),
            "buf = %s\n", wine_dbgstr_w((WCHAR*)buf));

    fclose(fp);

    fp = fopen(file_name, "wt");

    ret = fprintf(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);

    ret = fprintf(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 28, "ftell returned %d\n", ret);

    ret = fwprintf(fp, utf16_test);
    ok(ret == 8, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 37, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    ret = fscanf(fp, "%[^\n] ", buf);
    ok(ret == 1, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\r"), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 28, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\r\n", 15), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 37, "ret =  %d\n", ret);
    ok(!strcmp(buf, "unicode\r\n"), "buf = %s\n", buf);

    fclose(fp);
    unlink(file_name);
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

/* Don't test nrdigits < 0, msvcrt on Win9x and NT4 will corrupt memory by
 * writing outside allocated memory */
static struct {
    double value;
    int nrdigits;
    const char *expstr_e;
    const char *expstr_f;
    int expdecpt_e;
    int expdecpt_f;
    int expsign;
} test_cvt_testcases[] = {
    {          45.0,   2,        "45",           "4500",          2,      2,      0 },
    /* Numbers less than 1.0 with different precisions */
    {        0.0001,   1,         "1",               "",         -3,     -3,     0 },
    {        0.0001,  10,"1000000000",        "1000000",         -3,     -3,     0 },
    /* Basic sign test */
    {     -111.0001,   5,     "11100",       "11100010",          3,      3,     1 },
    {      111.0001,   5,     "11100",       "11100010",          3,      3,     0 },
    /* big numbers with low precision */
    {        3333.3,   2,        "33",         "333330",          4,      4,     0 },
    {999999999999.9,   3,       "100","999999999999900",         13,     12,     0 },
    /* 0.0 with different precisions */
    {           0.0,   5,     "00000",          "00000",          0,      0,     0 },
    {           0.0,   0,          "",               "",          0,      0,     0 },
    {           0.0,  -1,          "",               "",          0,      0,     0 },
    /* Numbers > 1.0 with 0 or -ve precision */
    {     -123.0001,   0,          "",            "123",          3,      3,     1 },
    {     -123.0001,  -1,          "",             "12",          3,      3,     1 },
    {     -123.0001,  -2,          "",              "1",          3,      3,     1 },
    {     -123.0001,  -3,          "",               "",          3,      3,     1 },
    /* Numbers > 1.0, but with rounding at the point of precision */
    {         99.99,   1,         "1",           "1000",          3,      3,     0 },
    /* Numbers < 1.0 where rounding occurs at the point of precision */
    {        0.0063,   2,        "63",              "1",         -2,     -1,     0 },
    {        0.0063,   3,        "630",             "6",         -2,     -2,     0 },
    { 0.09999999996,   2,        "10",             "10",          0,      0,     0 },
    {           0.6,   1,         "6",              "6",          0,      0,     0 },
    {           0.6,   0,          "",              "1",          1,      1,     0 },
    {           0.4,   0,          "",               "",          0,      0,     0 },
    {          0.49,   0,          "",               "",          0,      0,     0 },
    {          0.51,   0,          "",              "1",          1,      1,     0 },
    /* ask for ridiculous precision, ruin formatting this table */
    {           1.0,  30, "100000000000000000000000000000",
                      "1000000000000000000000000000000",          1,      1,      0},
    {           123456789012345678901.0,  30, "123456789012345680000000000000",
                      "123456789012345680000000000000000000000000000000000",         21,    21,      0},
    /* end marker */
    { 0, 0, "END"}
};

static void test_xcvt(void)
{
    char *str;
    int i, decpt, sign, err;

    for( i = 0; strcmp( test_cvt_testcases[i].expstr_e, "END"); i++){
        decpt = sign = 100;
        str = _ecvt( test_cvt_testcases[i].value,
                test_cvt_testcases[i].nrdigits,
                &decpt,
                &sign);
        ok( 0 == strncmp( str, test_cvt_testcases[i].expstr_e, 15),
               "_ecvt() bad return, got \n'%s' expected \n'%s'\n", str,
              test_cvt_testcases[i].expstr_e);
        ok( decpt == test_cvt_testcases[i].expdecpt_e,
                "_ecvt() decimal point wrong, got %d expected %d\n", decpt,
                test_cvt_testcases[i].expdecpt_e);
        ok( sign == test_cvt_testcases[i].expsign,
                "_ecvt() sign wrong, got %d expected %d\n", sign,
                test_cvt_testcases[i].expsign);
    }
    for( i = 0; strcmp( test_cvt_testcases[i].expstr_e, "END"); i++){
        decpt = sign = 100;
        str = _fcvt( test_cvt_testcases[i].value,
                test_cvt_testcases[i].nrdigits,
                &decpt,
                &sign);
        ok( 0 == strncmp( str, test_cvt_testcases[i].expstr_f, 15),
               "_fcvt() bad return, got \n'%s' expected \n'%s'\n", str,
              test_cvt_testcases[i].expstr_f);
        ok( decpt == test_cvt_testcases[i].expdecpt_f,
                "_fcvt() decimal point wrong, got %d expected %d\n", decpt,
                test_cvt_testcases[i].expdecpt_f);
        ok( sign == test_cvt_testcases[i].expsign,
                "_fcvt() sign wrong, got %d expected %d\n", sign,
                test_cvt_testcases[i].expsign);
    }

    if (p__ecvt_s)
    {
        str = malloc(1024);
        for( i = 0; strcmp( test_cvt_testcases[i].expstr_e, "END"); i++){
            decpt = sign = 100;
            err = p__ecvt_s(str, 1024, test_cvt_testcases[i].value, test_cvt_testcases[i].nrdigits, &decpt, &sign);
            ok(err == 0, "_ecvt_s() failed with error code %d\n", err);
            ok( 0 == strncmp( str, test_cvt_testcases[i].expstr_e, 15),
                   "_ecvt_s() bad return, got \n'%s' expected \n'%s'\n", str,
                  test_cvt_testcases[i].expstr_e);
            ok( decpt == test_cvt_testcases[i].expdecpt_e,
                    "_ecvt_s() decimal point wrong, got %d expected %d\n", decpt,
                    test_cvt_testcases[i].expdecpt_e);
            ok( sign == test_cvt_testcases[i].expsign,
                    "_ecvt_s() sign wrong, got %d expected %d\n", sign,
                    test_cvt_testcases[i].expsign);
        }
        free(str);
    }
    else
        win_skip("_ecvt_s not available\n");

    if (p__fcvt_s)
    {
        int i;

        str = malloc(1024);

        /* invalid arguments */
        err = p__fcvt_s(NULL, 0, 0.0, 0, &i, &i);
        ok(err == EINVAL, "got %d, expected EINVAL\n", err);

        err = p__fcvt_s(str, 0, 0.0, 0, &i, &i);
        ok(err == EINVAL, "got %d, expected EINVAL\n", err);

        str[0] = ' ';
        str[1] = 0;
        err = p__fcvt_s(str, -1, 0.0, 0, &i, &i);
        ok(err == 0, "got %d, expected 0\n", err);
        ok(str[0] == 0, "got %c, expected 0\n", str[0]);
        ok(str[1] == 0, "got %c, expected 0\n", str[1]);

        err = p__fcvt_s(str, 1, 0.0, 0, NULL, &i);
        ok(err == EINVAL, "got %d, expected EINVAL\n", err);

        err = p__fcvt_s(str, 1, 0.0, 0, &i, NULL);
        ok(err == EINVAL, "got %d, expected EINVAL\n", err);

        for( i = 0; strcmp( test_cvt_testcases[i].expstr_e, "END"); i++){
            decpt = sign = 100;
            err = p__fcvt_s(str, 1024, test_cvt_testcases[i].value, test_cvt_testcases[i].nrdigits, &decpt, &sign);
            ok(err == 0, "_fcvt_s() failed with error code %d\n", err);
            ok( 0 == strncmp( str, test_cvt_testcases[i].expstr_f, 15),
                   "_fcvt_s() bad return, got '%s' expected '%s'. test %d\n", str,
                  test_cvt_testcases[i].expstr_f, i);
            ok( decpt == test_cvt_testcases[i].expdecpt_f,
                    "_fcvt_s() decimal point wrong, got %d expected %d\n", decpt,
                    test_cvt_testcases[i].expdecpt_f);
            ok( sign == test_cvt_testcases[i].expsign,
                    "_fcvt_s() sign wrong, got %d expected %d\n", sign,
                    test_cvt_testcases[i].expsign);
        }
        free(str);
    }
    else
        win_skip("_fcvt_s not available\n");
}

static int __cdecl _vsnwprintf_wrapper(wchar_t *str, size_t len, const wchar_t *format, ...)
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

    ret = _vsnwprintf_wrapper( str, 0, format, one, two, three );
    ok( ret == -1, "got %d, expected -1\n", ret );

    ret = _vsnwprintf_wrapper( NULL, 0, format, one, two, three );
    ok( ret == 11 || broken(ret == -1 /* Win2k */), "got %d, expected 11\n", ret );
}

static int __cdecl vswprintf_wrapper(wchar_t *str, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p_vswprintf(str, format, valist);
    __ms_va_end(valist);
    return ret;
}

static int __cdecl _vswprintf_wrapper(wchar_t *str, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p__vswprintf(str, format, valist);
    __ms_va_end(valist);
    return ret;
}

static int __cdecl _vswprintf_l_wrapper(wchar_t *str, const wchar_t *format, void *locale, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    ret = p__vswprintf_l(str, format, locale, valist);
    __ms_va_end(valist);
    return ret;
}

static int __cdecl _vswprintf_c_wrapper(wchar_t *str, size_t size, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p__vswprintf_c(str, size, format, valist);
    __ms_va_end(valist);
    return ret;
}

static int __cdecl _vswprintf_c_l_wrapper(wchar_t *str, size_t size, const wchar_t *format, void *locale, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    ret = p__vswprintf_c_l(str, size, format, locale, valist);
    __ms_va_end(valist);
    return ret;
}

static int __cdecl _vswprintf_p_l_wrapper(wchar_t *str, size_t size, const wchar_t *format, void *locale, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, locale);
    ret = p__vswprintf_p_l(str, size, format, locale, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vswprintf(void)
{
    const wchar_t format[] = {'%','s',' ','%','d',0};
    const wchar_t number[] = {'n','u','m','b','e','r',0};
    const wchar_t out[] = {'n','u','m','b','e','r',' ','1','2','3',0};
    wchar_t buf[20];

    int ret;

    if (!p_vswprintf || !p__vswprintf || !p__vswprintf_l ||!p__vswprintf_c
            || !p__vswprintf_c_l || !p__vswprintf_p_l)
    {
        win_skip("_vswprintf or vswprintf not available\n");
        return;
    }

    ret = vswprintf_wrapper(buf, format, number, 123);
    ok(ret == 10, "got %d, expected 10\n", ret);
    ok(!memcmp(buf, out, sizeof(out)), "buf = %s\n", wine_dbgstr_w(buf));

    memset(buf, 0, sizeof(buf));
    ret = _vswprintf_wrapper(buf, format, number, 123);
    ok(ret == 10, "got %d, expected 10\n", ret);
    ok(!memcmp(buf, out, sizeof(out)), "buf = %s\n", wine_dbgstr_w(buf));

    memset(buf, 0, sizeof(buf));
    ret = _vswprintf_l_wrapper(buf, format, NULL, number, 123);
    ok(ret == 10, "got %d, expected 10\n", ret);
    ok(!memcmp(buf, out, sizeof(out)), "buf = %s\n", wine_dbgstr_w(buf));

    memset(buf, 0, sizeof(buf));
    ret = _vswprintf_c_wrapper(buf, 20, format, number, 123);
    ok(ret == 10, "got %d, expected 10\n", ret);
    ok(!memcmp(buf, out, sizeof(out)), "buf = %s\n", wine_dbgstr_w(buf));

    memset(buf, 0, sizeof(buf));
    ret = _vswprintf_c_l_wrapper(buf, 20, format, NULL, number, 123);
    ok(ret == 10, "got %d, expected 10\n", ret);
    ok(!memcmp(buf, out, sizeof(out)), "buf = %s\n", wine_dbgstr_w(buf));

    memset(buf, 0, sizeof(buf));
    ret = _vswprintf_p_l_wrapper(buf, 20, format, NULL, number, 123);
    ok(ret == 10, "got %d, expected 10\n", ret);
    ok(!memcmp(buf, out, sizeof(out)), "buf = %s\n", wine_dbgstr_w(buf));
}

static int __cdecl _vscprintf_wrapper(const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p__vscprintf(format, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vscprintf(void)
{
    int ret;

    if (!p__vscprintf)
    {
       win_skip("_vscprintf not available\n");
       return;
    }

    ret = _vscprintf_wrapper( "%s %d", "number", 1 );
    ok( ret == 8, "got %d expected 8\n", ret );
}

static int __cdecl _vscwprintf_wrapper(const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p__vscwprintf(format, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vscwprintf(void)
{
    const wchar_t format[] = {'%','s',' ','%','d',0};
    const wchar_t number[] = {'n','u','m','b','e','r',0};

    int ret;

    if (!p__vscwprintf)
    {
        win_skip("_vscwprintf not available\n");
        return;
    }

    ret = _vscwprintf_wrapper( format, number, 1 );
    ok( ret == 8, "got %d expected 8\n", ret );
}

static int __cdecl _vsnwprintf_s_wrapper(wchar_t *str, size_t sizeOfBuffer,
                                 size_t count, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p__vsnwprintf_s(str, sizeOfBuffer, count, format, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vsnwprintf_s(void)
{
    const wchar_t format[] = { 'A','B','%','u','C',0 };
    const wchar_t out7[] = { 'A','B','1','2','3','C',0 };
    const wchar_t out6[] = { 'A','B','1','2','3',0 };
    const wchar_t out2[] = { 'A',0 };
    const wchar_t out1[] = { 0 };
    wchar_t buffer[14] = { 0 };
    int exp, got;

    if (!p__vsnwprintf_s)
    {
        win_skip("_vsnwprintf_s not available\n");
        return;
    }

    /* Enough room. */
    exp = wcslen(out7);

    got = _vsnwprintf_s_wrapper(buffer, 14, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !wcscmp(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 12, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !wcscmp(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 7, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !wcscmp(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    /* Not enough room. */
    exp = -1;

    got = _vsnwprintf_s_wrapper(buffer, 6, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !wcscmp(out6, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 2, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !wcscmp(out2, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 1, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !wcscmp(out1, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));
}

static int __cdecl _vsprintf_p_wrapper(char *str, size_t sizeOfBuffer,
                                 const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p__vsprintf_p(str, sizeOfBuffer, format, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vsprintf_p(void)
{
    char buf[1024];
    int ret;

    if(!p__vsprintf_p) {
        win_skip("vsprintf_p not available\n");
        return;
    }

    ret = _vsprintf_p_wrapper(buf, sizeof(buf), "%s %d", "test", 1234);
    ok(ret == 9, "ret = %d\n", ret);
    ok(!memcmp(buf, "test 1234", 10), "buf = %s\n", buf);

    ret = _vsprintf_p_wrapper(buf, sizeof(buf), "%1$d", 1234, "additional param");
    ok(ret == 4, "ret = %d\n", ret);
    ok(!memcmp(buf, "1234", 5), "buf = %s\n", buf);

    ret = _vsprintf_p_wrapper(buf, sizeof(buf), "%2$s %1$d", 1234, "test");
    ok(ret == 9, "ret = %d\n", ret);
    ok(!memcmp(buf, "test 1234", 10), "buf = %s\n", buf);

    ret = _vsprintf_p_wrapper(buf, sizeof(buf), "%2$*3$s %2$.*1$s", 2, "test", 3);
    ok(ret == 7, "ret = %d\n", ret);
    ok(!memcmp(buf, "test te", 8), "buf = %s\n", buf);

    /* Following test invokes invalid parameter handler */
    /* ret = _vsprintf_p_wrapper(buf, sizeof(buf), "%d %1$d", 1234); */
}

static void test__get_output_format(void)
{
    unsigned int ret;
    char buf[64];
    int c;

    if (!p__get_output_format || !p__set_output_format)
    {
        win_skip("_get_output_format or _set_output_format is not available\n");
        return;
    }

    ret = p__get_output_format();
    ok(ret == 0, "got %d\n", ret);

    c = sprintf(buf, "%E", 1.23);
    ok(c == 13, "c = %d\n", c);
    ok(!strcmp(buf, "1.230000E+000"), "buf = %s\n", buf);

    ret = p__set_output_format(_TWO_DIGIT_EXPONENT);
    ok(ret == 0, "got %d\n", ret);

    c = sprintf(buf, "%E", 1.23);
    ok(c == 12, "c = %d\n", c);
    ok(!strcmp(buf, "1.230000E+00"), "buf = %s\n", buf);

    ret = p__get_output_format();
    ok(ret == _TWO_DIGIT_EXPONENT, "got %d\n", ret);

    ret = p__set_output_format(_TWO_DIGIT_EXPONENT);
    ok(ret == _TWO_DIGIT_EXPONENT, "got %d\n", ret);
}

START_TEST(printf)
{
    init();

    test_sprintf();
    test_swprintf();
    test_snprintf();
    test_fprintf();
    test_fcvt();
    test_xcvt();
    test_vsnwprintf();
    test_vscprintf();
    test_vscwprintf();
    test_vswprintf();
    test_vsnwprintf_s();
    test_vsprintf_p();
    test__get_output_format();
}
