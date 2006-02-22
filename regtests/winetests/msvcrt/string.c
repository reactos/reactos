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
#include <stdlib.h>

static void* (*pmemcpy)(void *, const void *, size_t n);
static int* (*pmemcmp)(void *, const void *, size_t n);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hMsvcrt,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)


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
}
