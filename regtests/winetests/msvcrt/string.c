/*
 * Unit test suite for string functions.
 *
 * Copyright 2004 Uwe Bonnes
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

#include "wine/test.h"
#include "winbase.h"
#include <string.h>
#include <mbstring.h>
#include <stdlib.h>
#include <mbctype.h>

static void* (*pmemcpy)(void *, const void *, size_t n);
static int* (*pmemcmp)(void *, const void *, size_t n);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hMsvcrt,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)

static void test_swab( void ) {
    char original[]  = "BADCFEHGJILKNMPORQTSVUXWZY@#";
    char expected1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ@#";
    char expected2[] = "ABCDEFGHIJKLMNOPQRSTUVWX$";
    char expected3[] = "$";
    
    char from[30];
    char to[30];
    
    int testsize;
    
    /* Test 1 - normal even case */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 26;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected1,testsize) == 0, "Testing even size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 2 - uneven case  */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 25;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected2,testsize) == 0, "Testing odd size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 3 - from = to */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 26;
    memcpy(to, original, testsize);
    _swab( to, to, testsize );
    ok(memcmp(to,expected1,testsize) == 0, "Testing overlapped size %d returned '%*.*s'\n", testsize, testsize, testsize, to);

    /* Test 4 - 1 bytes */                               
    memset(to,'$', sizeof(to));
    memset(from,'@', sizeof(from));
    testsize = 1;
    memcpy(from, original, testsize);
    _swab( from, to, testsize );
    ok(memcmp(to,expected3,testsize) == 0, "Testing small size %d returned '%*.*s'\n", testsize, testsize, testsize, to);
}

void test_ismbblead(void)
{
    unsigned int s = '\354';

    _setmbcp(936);
    todo_wine ok(_ismbblead(s), "got result %d\n", _ismbblead(s));
    _setmbcp(1252);
}

static void test_mbsspn( void)
{
    unsigned char str1[]="cabernet";
    unsigned char str2[]="shiraz";
    unsigned char set[]="abc";
    unsigned char empty[]="";
    int ret;
    ret=_mbsspn( str1, set);
    ok( ret==3, "_mbsspn returns %d should be 3\n", ret);
    ret=_mbsspn( str2, set);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);
    ret=_mbsspn( str1, empty);
    ok( ret==0, "_mbsspn returns %d should be 0\n", ret);
}

START_TEST(string)
{
    void *mem;
    static const char xilstring[]="c:/xilinx";
    int nLen=strlen(xilstring);
    HMODULE hMsvcrt = LoadLibraryA("msvcrt.dll");
    ok(hMsvcrt != 0, "LoadLibraryA failed\n");
    SET(pmemcpy,"memcpy");
    SET(pmemcmp,"memcmp");

    /* MSVCRT memcpy behaves like memmove for overlapping moves,
       MFC42 CString::Insert seems to rely on that behaviour */
    mem = malloc(100);
    ok(mem != NULL, "memory not allocated for size 0\n");
    strcpy((char*)mem,xilstring);
    pmemcpy((char*)mem+5, mem,nLen+1);
    ok(pmemcmp((char*)mem+5,xilstring, nLen) == 0, 
       "Got result %s\n",(char*)mem+5);

    /* Test _swab function */
    test_swab();

    /* Test ismbblead*/
    test_ismbblead();
   /* test _mbsspn */
    test_mbsspn();
}
