/*
 * Conformance tests for *printf functions.
 *
 * Copyright 2002 Uwe Bonnes
 * Copyright 2004 Aneurin Price
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
    const char *I64d = "%I64d";
    const char *O4c = "%04c";
    const char *O4s = "%04s";
    const char *hash012p = "%#012p";
    double pnumber=789456123;
/**    WCHAR widestring[]={'w','i','d','e','s','t','r','i','n','g',0};**/
    sprintf(buffer,"%+#23.15e",pnumber);
    todo_wine
      {
        ok(strstr(buffer,"e+008") != 0,"Sprintf different \"%s\"\n",buffer);
      }
    sprintf(buffer,I64d,((ULONGLONG)0xffffffff)*0xffffffff);
    todo_wine
      {
        ok(strlen(buffer) == 11,"Problem with long long \"%s\"\n",buffer);
      }
    sprintf(buffer,"%lld",((ULONGLONG)0xffffffff)*0xffffffff);
    todo_wine
      {
        ok(strlen(buffer) == 1,"Problem with \"ll\" interpretation \"%s\"\n",buffer);
      }
/** This one actually crashes WINE at the moment, when using builtin msvcrt.dll.
    sprintf(buffer,"%S",widestring);
    todo_wine
      {
        ok(strlen(buffer) == 10,"Problem with \"%%S\" interpretation \"%s\"\n",buffer);
      }
 **/
    sprintf(buffer,O4c,'1');
    todo_wine
      {
        ok(!strcmp(buffer,"0001"),"Character not zero-prefixed \"%s\"\n",buffer);
      }
    sprintf(buffer,"%p",(void *)57);
    todo_wine
      {
        ok(!strcmp(buffer,"00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
      }
    sprintf(buffer,hash012p,(void *)57);
    todo_wine
      {
        ok(!strcmp(buffer,"  0X00000039"),"Pointer formatted incorrectly \"%s\"\n",buffer);
      }
    sprintf(buffer,O4s,"foo");/**Warning again**/
    todo_wine
      {
        ok(!strcmp(buffer,"0foo"),"String not zero-prefixed \"%s\"\n",buffer);
      }
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
    todo_wine
      {
        ok(wcslen(buffer) == 11,"Problem with long long\n");
      }
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
        struct {
            int retval;
            int render;
        } todo;
    };
    /* Pre-2.1 libc behaviour, not C99 compliant. */
    const struct snprintf_test tests[] = {{"short", 5, {0, 0}},
                                          {"justfit", 7, {0, 0}},
                                          {"justfits", 8, {0, 1}},
                                          {"muchlonger", -1, {1, 1}}};
    char buffer[8];
    const int bufsiz = sizeof buffer;
    unsigned int i;

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        const char *fmt  = tests[i].format;
        const int expect = tests[i].expected;
        const int n      = _snprintf (buffer, bufsiz, fmt);
        const int valid  = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        todo (tests[i].todo.retval ? "wine" : "none")
            ok (n == expect, "\"%s\": expected %d, returned %d\n",
                fmt, expect, n);
        todo (tests[i].todo.render ? "wine" : "none")
            ok (!memcmp (fmt, buffer, valid),
                "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
    };
}

START_TEST(printf)
{
    test_sprintf();
    test_swprintf();
    test_fwprintf();
    test_snprintf();
}
