/* Unit test suite for SHLWAPI ordinal functions
 *
 * Copyright 2004 Jon Griffiths
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
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"

/* Function ptrs for ordinal calls */
static HMODULE hShlwapi;
static int (WINAPI *pSHSearchMapInt)(const int*,const int*,int,int);
static HRESULT (WINAPI *pGetAcceptLanguagesA)(LPSTR,LPDWORD);

static HANDLE (WINAPI *pSHAllocShared)(LPCVOID,DWORD,DWORD);
static LPVOID (WINAPI *pSHLockShared)(HANDLE,DWORD);
static BOOL   (WINAPI *pSHUnlockShared)(LPVOID);
static BOOL   (WINAPI *pSHFreeShared)(HANDLE,DWORD);

static void test_GetAcceptLanguagesA(void)
{   HRESULT retval;
    DWORD buffersize, buffersize2, exactsize;
    char buffer[100];

    if (!pGetAcceptLanguagesA)
	return;

    buffersize = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    trace("GetAcceptLanguagesA: retval %08lx, size %08lx, buffer (%s),"
	" last error %ld\n", retval, buffersize, buffer, GetLastError());
    if(retval != S_OK) {
	trace("GetAcceptLanguagesA: skipping tests\n");
	return;
    }
    ok( (ERROR_NO_IMPERSONATION_TOKEN == GetLastError()) || 
	(ERROR_CLASS_DOES_NOT_EXIST == GetLastError()) ||
	(ERROR_PROC_NOT_FOUND == GetLastError()) ||
	(ERROR_CALL_NOT_IMPLEMENTED == GetLastError()) ||
	(ERROR_SUCCESS == GetLastError()), "last error set to %ld\n", GetLastError());
    exactsize = strlen(buffer);

    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( NULL, NULL);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = sizeof(buffer);
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( NULL, &buffersize);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(buffersize == sizeof(buffer),
       "buffersize was changed (2nd parameter; not on Win2k)\n");
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, NULL);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = 0;
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    ok(retval == E_FAIL,
       "function result wrong: got %08lx; expected E_FAIL\n", retval);
    ok(buffersize == 0,
       "buffersize wrong(changed) got %08lx; expected 0 (2nd parameter; not on Win2k)\n", buffersize);
    ok(ERROR_SUCCESS == GetLastError(), "last error set to %ld\n", GetLastError());

    buffersize = buffersize2 = 1;
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    switch(retval) {
	case 0L:
            if(buffersize == exactsize) {
            ok( (ERROR_SUCCESS == GetLastError()) || (ERROR_CALL_NOT_IMPLEMENTED == GetLastError()) ||
		(ERROR_PROC_NOT_FOUND == GetLastError()) || (ERROR_NO_IMPERSONATION_TOKEN == GetLastError()),
                "last error wrong: got %08lx; expected ERROR_SUCCESS(NT4)/ERROR_CALL_NOT_IMPLEMENTED(98/ME)/"
		"ERROR_PROC_NOT_FOUND(NT4)/ERROR_NO_IMPERSONATION_TOKEN(XP)\n", GetLastError());
            ok(exactsize == strlen(buffer),
                 "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), exactsize);
            } else if((buffersize +1) == buffersize2) {
                ok(ERROR_SUCCESS == GetLastError(),
                    "last error wrong: got %08lx; expected ERROR_SUCCESS\n", GetLastError());
                ok(buffersize == strlen(buffer),
                    "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), buffersize);
            } else
                ok( 0, "retval %08lx, size %08lx, buffer (%s), last error %ld\n",
                    retval, buffersize, buffer, GetLastError());
            break;
	case E_INVALIDARG:
            ok(buffersize == 0,
               "buffersize wrong: got %08lx, expected 0 (2nd parameter;Win2k)\n", buffersize);
            ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
               "last error wrong: got %08lx; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
            ok(buffersize2 == strlen(buffer),
               "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), buffersize2);
            break;
        default:
            ok( 0, "retval %08lx, size %08lx, buffer (%s), last error %ld\n",
                retval, buffersize, buffer, GetLastError());
            break;
    }

    buffersize = buffersize2 = exactsize;
    memset(buffer, 0, sizeof(buffer));
    SetLastError(ERROR_SUCCESS);
    retval = pGetAcceptLanguagesA( buffer, &buffersize);
    switch(retval) {
	case 0L:
            ok(ERROR_SUCCESS == GetLastError(),
                 "last error wrong: got %08lx; expected ERROR_SUCCESS\n", GetLastError());
            if((buffersize == exactsize) /* XP */ ||
               ((buffersize +1)== exactsize) /* 98 */)
                ok(buffersize == strlen(buffer),
                    "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), buffersize);
            else
                ok( 0, "retval %08lx, size %08lx, buffer (%s), last error %ld\n",
                    retval, buffersize, buffer, GetLastError());
            break;
	case E_INVALIDARG:
            ok(buffersize == 0,
               "buffersize wrong: got %08lx, expected 0 (2nd parameter;Win2k)\n", buffersize);
            ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
               "last error wrong: got %08lx; expected ERROR_INSUFFICIENT_BUFFER\n", GetLastError());
            ok(buffersize2 == strlen(buffer),
               "buffer content (length) wrong: got %08x, expected %08lx \n", strlen(buffer), buffersize2);
            break;
        default:
            ok( 0, "retval %08lx, size %08lx, buffer (%s), last error %ld\n",
                retval, buffersize, buffer, GetLastError());
            break;
    }
}

static void test_SHSearchMapInt(void)
{
  int keys[8], values[8];
  int i = 0;

  if (!pSHSearchMapInt)
    return;

  memset(keys, 0, sizeof(keys));
  memset(values, 0, sizeof(values));
  keys[0] = 99; values[0] = 101;

  /* NULL key/value lists crash native, so skip testing them */

  /* 1 element */
  i = pSHSearchMapInt(keys, values, 1, keys[0]);
  ok(i == values[0], "Len 1, expected %d, got %d\n", values[0], i);

  /* Key doesn't exist */
  i = pSHSearchMapInt(keys, values, 1, 100);
  ok(i == -1, "Len 1 - bad key, expected -1, got %d\n", i);

  /* Len = 0 => not found */
  i = pSHSearchMapInt(keys, values, 0, keys[0]);
  ok(i == -1, "Len 1 - passed len 0, expected -1, got %d\n", i);

  /* 2 elements, len = 1 */
  keys[1] = 98; values[1] = 102;
  i = pSHSearchMapInt(keys, values, 1, keys[1]);
  ok(i == -1, "Len 1 - array len 2, expected -1, got %d\n", i);

  /* 2 elements, len = 2 */
  i = pSHSearchMapInt(keys, values, 2, keys[1]);
  ok(i == values[1], "Len 2, expected %d, got %d\n", values[1], i);

  /* Searches forward */
  keys[2] = 99; values[2] = 103;
  i = pSHSearchMapInt(keys, values, 3, keys[0]);
  ok(i == values[0], "Len 3, expected %d, got %d\n", values[0], i);
}

static void test_alloc_shared(void)
{
    DWORD procid;
    HANDLE hmem;
    int val;
    int* p;
    BOOL ret;

    procid=GetCurrentProcessId();
    hmem=pSHAllocShared(NULL,10,procid);
    ok(hmem!=NULL,"SHAllocShared(NULL...) failed: %ld\n", GetLastError());
    ret = pSHFreeShared(hmem, procid);
    ok( ret, "SHFreeShared failed: %ld\n", GetLastError());

    val=0x12345678;
    hmem=pSHAllocShared(&val,4,procid);
    ok(hmem!=NULL,"SHAllocShared(NULL...) failed: %ld\n", GetLastError());

    p=(int*)pSHLockShared(hmem,procid);
    ok(p!=NULL,"SHLockShared failed: %ld\n", GetLastError());
    if (p!=NULL)
        ok(*p==val,"Wrong value in shared memory: %d instead of %d\n",*p,val);
    ret = pSHUnlockShared(p);
    ok( ret, "SHUnlockShared failed: %ld\n", GetLastError());

    ret = pSHFreeShared(hmem, procid);
    ok( ret, "SHFreeShared failed: %ld\n", GetLastError());
}

START_TEST(ordinal)
{
  hShlwapi = LoadLibraryA("shlwapi.dll");
  ok(hShlwapi != 0, "LoadLibraryA failed\n");
  if (!hShlwapi)
    return;

  pGetAcceptLanguagesA = (void*)GetProcAddress(hShlwapi, (LPSTR)14);
  pSHSearchMapInt = (void*)GetProcAddress(hShlwapi, (LPSTR)198);
  pSHAllocShared=(void*)GetProcAddress(hShlwapi,(char*)7);
  pSHLockShared=(void*)GetProcAddress(hShlwapi,(char*)8);
  pSHUnlockShared=(void*)GetProcAddress(hShlwapi,(char*)9);
  pSHFreeShared=(void*)GetProcAddress(hShlwapi,(char*)10);

  test_GetAcceptLanguagesA();
  test_SHSearchMapInt();
  test_alloc_shared();
  FreeLibrary(hShlwapi);
}
