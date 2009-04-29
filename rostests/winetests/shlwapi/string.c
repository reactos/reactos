/* Unit test suite for SHLWAPI string functions
 *
 * Copyright 2003 Jon Griffiths
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
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shtypes.h"

#define expect_eq(expr, val, type, fmt) do { \
    type ret = expr; \
    ok(ret == val, "Unexpected value of '" #expr "': " #fmt " instead of " #val "\n", ret); \
} while (0);

#define expect_eq2(expr, val1, val2, type, fmt) do { \
    type ret = expr; \
    ok(ret == val1 || ret == val2, "Unexpected value of '" #expr "': " #fmt " instead of " #val1 " or " #val2 "\n", ret); \
} while (0);

static BOOL    (WINAPI *pIntlStrEqWorkerA)(BOOL,LPCSTR,LPCSTR,int);
static BOOL    (WINAPI *pIntlStrEqWorkerW)(BOOL,LPCWSTR,LPCWSTR,int);
static DWORD   (WINAPI *pSHAnsiToAnsi)(LPCSTR,LPSTR,int);
static DWORD   (WINAPI *pSHUnicodeToUnicode)(LPCWSTR,LPWSTR,int);
static LPSTR   (WINAPI *pStrCatBuffA)(LPSTR,LPCSTR,INT);
static LPWSTR  (WINAPI *pStrCatBuffW)(LPWSTR,LPCWSTR,INT);
static LPSTR   (WINAPI *pStrCpyNXA)(LPSTR,LPCSTR,int);
static LPWSTR  (WINAPI *pStrCpyNXW)(LPWSTR,LPCWSTR,int);
static LPSTR   (WINAPI *pStrFormatByteSize64A)(LONGLONG,LPSTR,UINT);
static LPSTR   (WINAPI *pStrFormatKBSizeA)(LONGLONG,LPSTR,UINT);
static LPWSTR  (WINAPI *pStrFormatKBSizeW)(LONGLONG,LPWSTR,UINT);
static BOOL    (WINAPI *pStrIsIntlEqualA)(BOOL,LPCSTR,LPCSTR,int);
static BOOL    (WINAPI *pStrIsIntlEqualW)(BOOL,LPCWSTR,LPCWSTR,int);
static HRESULT (WINAPI *pStrRetToBSTR)(STRRET*,void*,BSTR*);
static HRESULT (WINAPI *pStrRetToBufA)(STRRET*,LPCITEMIDLIST,LPSTR,UINT);
static HRESULT (WINAPI *pStrRetToBufW)(STRRET*,LPCITEMIDLIST,LPWSTR,UINT);
static INT     (WINAPIV *pwnsprintfA)(LPSTR,INT,LPCSTR, ...);
static INT     (WINAPIV *pwnsprintfW)(LPWSTR,INT,LPCWSTR, ...);

static int strcmpW(const WCHAR *str1, const WCHAR *str2)
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

/* StrToInt/StrToIntEx results */
typedef struct tagStrToIntResult
{
  const char* string;
  int str_to_int;
  int str_to_int_ex;
  int str_to_int_hex;
} StrToIntResult;

static const StrToIntResult StrToInt_results[] = {
     { "1099", 1099, 1099, 1099 },
     { "+88987", 0, 88987, 88987 },
     { "012", 12, 12, 12 },
     { "-55", -55, -55, -55 },
     { "-0", 0, 0, 0 },
     { "0x44ff", 0, 0, 0x44ff },
     { "+0x44f4", 0, 0, 0x44f4 },
     { "-0x44fd", 0, 0, 0x44fd },
     { "+ 88987", 0, 0, 0 },
     { "- 55", 0, 0, 0 },
     { "- 0", 0, 0, 0 },
     { "+ 0x44f4", 0, 0, 0 },
     { "--0x44fd", 0, 0, 0 },
     { " 1999", 0, 1999, 1999 },
     { " +88987", 0, 88987, 88987 },
     { " 012", 0, 12, 12 },
     { " -55", 0, -55, -55 },
     { " 0x44ff", 0, 0, 0x44ff },
     { " +0x44f4", 0, 0, 0x44f4 },
     { " -0x44fd", 0, 0, 0x44fd },
     { NULL, 0, 0, 0 }
};

/* pStrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFormatSizeResult
{
  LONGLONG value;
  const char* byte_size_64;
  const char* kb_size;
} StrFormatSizeResult;


static const StrFormatSizeResult StrFormatSize_results[] = {
  { -1023, "-1023 bytes", "0 KB"},
  { -24, "-24 bytes", "0 KB"},
  { 309, "309 bytes", "1 KB"},
  { 10191, "9.95 KB", "10 KB"},
  { 100353, "98.0 KB", "99 KB"},
  { 1022286, "998 KB", "999 KB"},
  { 1046862, "0.99 MB", "1,023 KB"},
  { 1048574619, "999 MB", "1,023,999 KB"},
  { 1073741775, "0.99 GB", "1,048,576 KB"},
  { ((LONGLONG)0x000000f9 << 32) | 0xfffff94e, "999 GB", "1,048,575,999 KB"},
  { ((LONGLONG)0x000000ff << 32) | 0xfffffa9b, "0.99 TB", "1,073,741,823 KB"},
  { ((LONGLONG)0x0003e7ff << 32) | 0xfffffa9b, "999 TB", "1,073,741,823,999 KB"},
  { ((LONGLONG)0x0003ffff << 32) | 0xfffffbe8, "0.99 PB", "1,099,511,627,775 KB"},
  { ((LONGLONG)0x0f9fffff << 32) | 0xfffffd35, "999 PB", "1,099,511,627,776,000 KB"},
  { ((LONGLONG)0x0fffffff << 32) | 0xfffffa9b, "0.99 EB", "1,125,899,906,842,623 KB"},
  { 0, NULL, NULL }
};

/* StrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFromTimeIntervalResult
{
  DWORD ms;
  int   digits;
  const char* time_interval;
} StrFromTimeIntervalResult;


static const StrFromTimeIntervalResult StrFromTimeInterval_results[] = {
  { 1, 1, " 0 sec" },
  { 1, 2, " 0 sec" },
  { 1, 3, " 0 sec" },
  { 1, 4, " 0 sec" },
  { 1, 5, " 0 sec" },
  { 1, 6, " 0 sec" },
  { 1, 7, " 0 sec" },

  { 1000000, 1, " 10 min" },
  { 1000000, 2, " 16 min" },
  { 1000000, 3, " 16 min 40 sec" },
  { 1000000, 4, " 16 min 40 sec" },
  { 1000000, 5, " 16 min 40 sec" },
  { 1000000, 6, " 16 min 40 sec" },
  { 1000000, 7, " 16 min 40 sec" },

  { 1999999, 1, " 30 min" },
  { 1999999, 2, " 33 min" },
  { 1999999, 3, " 33 min 20 sec" },
  { 1999999, 4, " 33 min 20 sec" },
  { 1999999, 5, " 33 min 20 sec" },
  { 1999999, 6, " 33 min 20 sec" },
  { 1999999, 7, " 33 min 20 sec" },

  { 3999997, 1, " 1 hr" },
  { 3999997, 2, " 1 hr 6 min" },
  { 3999997, 3, " 1 hr 6 min 40 sec" },
  { 3999997, 4, " 1 hr 6 min 40 sec" },
  { 3999997, 5, " 1 hr 6 min 40 sec" },
  { 3999997, 6, " 1 hr 6 min 40 sec" },
  { 3999997, 7, " 1 hr 6 min 40 sec" },

  { 149999851, 7, " 41 hr 40 min 0 sec" },
  { 150999850, 1, " 40 hr" },
  { 150999850, 2, " 41 hr" },
  { 150999850, 3, " 41 hr 50 min" },
  { 150999850, 4, " 41 hr 56 min" },
  { 150999850, 5, " 41 hr 56 min 40 sec" },
  { 150999850, 6, " 41 hr 56 min 40 sec" },
  { 150999850, 7, " 41 hr 56 min 40 sec" },

  { 493999507, 1, " 100 hr" },
  { 493999507, 2, " 130 hr" },
  { 493999507, 3, " 137 hr" },
  { 493999507, 4, " 137 hr 10 min" },
  { 493999507, 5, " 137 hr 13 min" },
  { 493999507, 6, " 137 hr 13 min 20 sec" },
  { 493999507, 7, " 137 hr 13 min 20 sec" },

  { 0, 0, NULL }
};

static void test_StrChrA(void)
{
  char string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrA(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = (char)count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrChrA(string+32, count);
    INT pos = result - string;
    ok(pos == count, "found char '%c' in wrong place: got %d, expected %d\n", count, pos, count);
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrChrA(string+count+1, count);
    ok(!result, "found char '%c' not in the string\n", count);
  }
}

static void test_StrChrW(void)
{
  WCHAR string[16385];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrW(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 16384; count++)
    string[count] = count;
  string[16384] = '\0';

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrChrW(string+32, count);
    ok((result - string) == count, "found char %d in wrong place\n", count);
  }

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrChrW(string+count+1, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrChrIA(void)
{
  char string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrIA(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = (char)count;
  string[128] = '\0';

  for (count = 'A'; count <= 'X'; count++)
  {
    LPSTR result = StrChrIA(string+32, count);

    ok(result - string == count, "found char '%c' in wrong place\n", count);
    ok(StrChrIA(result, count)!=NULL, "didn't find lowercase '%c'\n", count);
  }

  for (count = 'a'; count < 'z'; count++)
  {
    LPSTR result = StrChrIA(string+count+1, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrChrIW(void)
{
  WCHAR string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrChrIA(NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 'A'; count <= 'X'; count++)
  {
    LPWSTR result = StrChrIW(string+32, count);

    ok(result - string == count, "found char '%c' in wrong place\n", count);
    ok(StrChrIW(result, count)!=NULL, "didn't find lowercase '%c'\n", count);
  }

  for (count = 'a'; count < 'z'; count++)
  {
    LPWSTR result = StrChrIW(string+count+1, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrRChrA(void)
{
  char string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrRChrA(NULL, NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = (char)count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+32, NULL, count);
    ok(result - string == count, "found char %d in wrong place\n", count);
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+count+1, NULL, count);
    ok(!result, "found char not in the string\n");
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+count+1, string + 127, count);
    ok(!result, "found char not in the string\n");
  }
}

static void test_StrRChrW(void)
{
  WCHAR string[129];
  WORD count;

  /* this test crashes on win2k SP4 */
  /*ok(!StrRChrW(NULL, NULL,'\0'), "found a character in a NULL string!\n");*/

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPWSTR result = StrRChrW(string+32, NULL, count);
    INT pos = result - string;
    ok(pos == count, "found char %d in wrong place: got %d, expected %d\n", count, pos, count);
  }

  for (count = 32; count < 128; count++)
  {
    LPWSTR result = StrRChrW(string+count+1, NULL, count);
    ok(!result, "found char %d not in the string\n", count);
  }

  for (count = 32; count < 128; count++)
  {
    LPWSTR result = StrRChrW(string+count+1, string + 127, count);
    ok(!result, "found char %d not in the string\n", count);
  }
}

static void test_StrCpyW(void)
{
  WCHAR szSrc[256];
  WCHAR szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;


  while(result->value)
  {
    MultiByteToWideChar(0,0,result->byte_size_64,-1,szSrc,sizeof(szSrc)/sizeof(WCHAR));

    StrCpyW(szBuff, szSrc);
    ok(!StrCmpW(szSrc, szBuff), "Copied string %s wrong\n", result->byte_size_64);
    result++;
  }
}


static void test_StrToIntA(void)
{
  const StrToIntResult *result = StrToInt_results;
  int return_val;

  while (result->string)
  {
    return_val = StrToIntA(result->string);
    ok(return_val == result->str_to_int, "converted '%s' wrong (%d)\n",
       result->string, return_val);
    result++;
  }
}

static void test_StrToIntW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  int return_val;

  while (result->string)
  {
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    return_val = StrToIntW(szBuff);
    ok(return_val == result->str_to_int, "converted '%s' wrong (%d)\n",
       result->string, return_val);
    result++;
  }
}

static void test_StrToIntExA(void)
{
  const StrToIntResult *result = StrToInt_results;
  int return_val;
  BOOL bRet;

  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,0,&return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_ex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,STIF_SUPPORT_HEX,&return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_hex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }
}

static void test_StrToIntExW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  int return_val;
  BOOL bRet;

  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    bRet = StrToIntExW(szBuff, 0, &return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_ex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    bRet = StrToIntExW(szBuff, STIF_SUPPORT_HEX, &return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'\n",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_hex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }
}

static void test_StrDupA(void)
{
  LPSTR lpszStr;
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    lpszStr = StrDupA(result->byte_size_64);

    ok(lpszStr != NULL, "Dup failed\n");
    if (lpszStr)
    {
      ok(!strcmp(result->byte_size_64, lpszStr), "Copied string wrong\n");
      LocalFree(lpszStr);
    }
    result++;
  }

  /* Later versions of shlwapi return NULL for this, but earlier versions
   * returned an empty string (as Wine does).
   */
  lpszStr = StrDupA(NULL);
  ok(lpszStr == NULL || *lpszStr == '\0', "NULL string returned %p\n", lpszStr);
  LocalFree(lpszStr);
}

static void test_StrFormatByteSize64A(void)
{
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  if (!pStrFormatByteSize64A)
  {
    win_skip("StrFormatByteSize64A() is not available\n");
    return;
  }

  while(result->value)
  {
    pStrFormatByteSize64A(result->value, szBuff, 256);

    ok(!strcmp(result->byte_size_64, szBuff),
        "Formatted %x%08x wrong: got %s, expected %s\n",
       (LONG)(result->value >> 32), (LONG)result->value, szBuff, result->byte_size_64);

    result++;
  }
}

static void test_StrFormatKBSizeW(void)
{
  WCHAR szBuffW[256];
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  if (!pStrFormatKBSizeW)
  {
    win_skip("StrFormatKBSizeW() is not available\n");
    return;
  }

  while(result->value)
  {
    pStrFormatKBSizeW(result->value, szBuffW, 256);
    WideCharToMultiByte(0,0,szBuffW,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR),0,0);
    ok(!strcmp(result->kb_size, szBuff),
        "Formatted %x%08x wrong: got %s, expected %s\n",
       (LONG)(result->value >> 32), (LONG)result->value, szBuff, result->kb_size);
    result++;
  }
}

static void test_StrFormatKBSizeA(void)
{
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  if (!pStrFormatKBSizeA)
  {
    win_skip("StrFormatKBSizeA() is not available\n");
    return;
  }

  while(result->value)
  {
    pStrFormatKBSizeA(result->value, szBuff, 256);

    ok(!strcmp(result->kb_size, szBuff),
        "Formatted %x%08x wrong: got %s, expected %s\n",
       (LONG)(result->value >> 32), (LONG)result->value, szBuff, result->kb_size);
    result++;
  }
}

static void test_StrFromTimeIntervalA(void)
{
  char szBuff[256];
  const StrFromTimeIntervalResult* result = StrFromTimeInterval_results;

  while(result->ms)
  {
    StrFromTimeIntervalA(szBuff, 256, result->ms, result->digits);

    ok(!strcmp(result->time_interval, szBuff), "Formatted %d %d wrong\n",
       result->ms, result->digits);
    result++;
  }
}

static void test_StrCmpA(void)
{
  static const char str1[] = {'a','b','c','d','e','f'};
  static const char str2[] = {'a','B','c','d','e','f'};
  ok(0 != StrCmpNA(str1, str2, 6), "StrCmpNA is case-insensitive\n");
  ok(0 == StrCmpNIA(str1, str2, 6), "StrCmpNIA is case-sensitive\n");
  ok(!ChrCmpIA('a', 'a'), "ChrCmpIA doesn't work at all!\n");
  ok(!ChrCmpIA('b', 'B'), "ChrCmpIA is not case-insensitive\n");
  ok(ChrCmpIA('a', 'z'), "ChrCmpIA believes that a == z!\n");

  if (pStrIsIntlEqualA)
  {
    ok(pStrIsIntlEqualA(FALSE, str1, str2, 5), "StrIsIntlEqualA(FALSE,...) isn't case-insensitive\n");
    ok(!pStrIsIntlEqualA(TRUE, str1, str2, 5), "StrIsIntlEqualA(TRUE,...) isn't case-sensitive\n");
  }
  else
    win_skip("StrIsIntlEqualA() is not available\n");

  if (pIntlStrEqWorkerA)
  {
    ok(pIntlStrEqWorkerA(FALSE, str1, str2, 5), "IntlStrEqWorkerA(FALSE,...) isn't case-insensitive\n");
    ok(!pIntlStrEqWorkerA(TRUE, str1, str2, 5), "pIntlStrEqWorkerA(TRUE,...) isn't case-sensitive\n");
  }
  else
    win_skip("IntlStrEqWorkerA() is not available\n");
}

static void test_StrCmpW(void)
{
  static const WCHAR str1[] = {'a','b','c','d','e','f'};
  static const WCHAR str2[] = {'a','B','c','d','e','f'};
  ok(0 != StrCmpNW(str1, str2, 5), "StrCmpNW is case-insensitive\n");
  ok(0 == StrCmpNIW(str1, str2, 5), "StrCmpNIW is case-sensitive\n");
  ok(!ChrCmpIW('a', 'a'), "ChrCmpIW doesn't work at all!\n");
  ok(!ChrCmpIW('b', 'B'), "ChrCmpIW is not case-insensitive\n");
  ok(ChrCmpIW('a', 'z'), "ChrCmpIW believes that a == z!\n");

  if (pStrIsIntlEqualW)
  {
    ok(pStrIsIntlEqualW(FALSE, str1, str2, 5), "StrIsIntlEqualW(FALSE,...) isn't case-insensitive\n");
    ok(!pStrIsIntlEqualW(TRUE, str1, str2, 5), "StrIsIntlEqualW(TRUE,...) isn't case-sensitive\n");
  }
  else
    win_skip("StrIsIntlEqualW() is not available\n");

  if (pIntlStrEqWorkerW)
  {
    ok(pIntlStrEqWorkerW(FALSE, str1, str2, 5), "IntlStrEqWorkerW(FALSE,...) isn't case-insensitive\n");
    ok(!pIntlStrEqWorkerW(TRUE, str1, str2, 5), "IntlStrEqWorkerW(TRUE,...) isn't case-sensitive\n");
  }
  else
    win_skip("IntlStrEqWorkerW() is not available\n");
}

static WCHAR *CoDupStrW(const char* src)
{
  INT len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
  WCHAR* szTemp = CoTaskMemAlloc(len * sizeof(WCHAR));
  MultiByteToWideChar(CP_ACP, 0, src, -1, szTemp, len);
  return szTemp;
}

static void test_StrRetToBSTR(void)
{
    static const WCHAR szTestW[] = { 'T','e','s','t','\0' };
    ITEMIDLIST iidl[10];
    BSTR bstr;
    STRRET strret;
    HRESULT ret;

    if (!pStrRetToBSTR)
    {
        win_skip("StrRetToBSTR() is not available\n");
        return;
    }

    strret.uType = STRRET_WSTR;
    U(strret).pOleStr = CoDupStrW("Test");
    bstr = 0;
    ret = pStrRetToBSTR(&strret, NULL, &bstr);
    ok(ret == S_OK && bstr && !strcmpW(bstr, szTestW),
       "STRRET_WSTR: dup failed, ret=0x%08x, bstr %p\n", ret, bstr);
    SysFreeString(bstr);

    strret.uType = STRRET_CSTR;
    lstrcpyA(U(strret).cStr, "Test");
    ret = pStrRetToBSTR(&strret, NULL, &bstr);
    ok(ret == S_OK && bstr && !strcmpW(bstr, szTestW),
       "STRRET_CSTR: dup failed, ret=0x%08x, bstr %p\n", ret, bstr);
    SysFreeString(bstr);

    strret.uType = STRRET_OFFSET;
    U(strret).uOffset = 1;
    strcpy((char*)&iidl, " Test");
    ret = pStrRetToBSTR(&strret, iidl, &bstr);
    ok(ret == S_OK && bstr && !strcmpW(bstr, szTestW),
       "STRRET_OFFSET: dup failed, ret=0x%08x, bstr %p\n", ret, bstr);
    SysFreeString(bstr);

    /* Native crashes if str is NULL */
}

static void test_StrCpyNXA(void)
{
  LPCSTR lpSrc = "hello";
  LPSTR lpszRes;
  char dest[8];

  if (!pStrCpyNXA)
  {
    win_skip("StrCpyNXA() is not available\n");
    return;
  }

  memset(dest, '\n', sizeof(dest));
  lpszRes = pStrCpyNXA(dest, lpSrc, sizeof(dest)/sizeof(dest[0]));
  ok(lpszRes == dest + 5 && !memcmp(dest, "hello\0\n\n", sizeof(dest)),
       "StrCpyNXA: expected %p, \"hello\\0\\n\\n\", got %p, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
       dest + 5, lpszRes, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_StrCpyNXW(void)
{
  static const WCHAR lpInit[] = { '\n','\n','\n','\n','\n','\n','\n','\n' };
  static const WCHAR lpSrc[] = { 'h','e','l','l','o','\0' };
  static const WCHAR lpRes[] = { 'h','e','l','l','o','\0','\n','\n' };
  LPWSTR lpszRes;
  WCHAR dest[8];

  if (!pStrCpyNXW)
  {
    win_skip("StrCpyNXW() is not available\n");
    return;
  }

  memcpy(dest, lpInit, sizeof(lpInit));
  lpszRes = pStrCpyNXW(dest, lpSrc, sizeof(dest)/sizeof(dest[0]));
  ok(lpszRes == dest + 5 && !memcmp(dest, lpRes, sizeof(dest)),
       "StrCpyNXW: expected %p, \"hello\\0\\n\\n\", got %p, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
       dest + 5, lpszRes, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

#define check_strrstri(type, str, pos, needle, exp) \
    ret##type = StrRStrI##type(str, str+pos, needle); \
    ok(ret##type == (exp), "Type " #type ", expected %p but got %p (string base %p)\n", \
    (exp), ret##type, str);

static void test_StrRStrI(void)
{
    static const CHAR szTest[] = "yAxxxxAy";
    static const CHAR szTest2[] = "ABABABAB";
    static const WCHAR wszTest[] = {'y','A','x','x','x','x','A','y',0};
    static const WCHAR wszTest2[] = {'A','B','A','B','A','B','A','B',0};

    static const WCHAR wszPattern1[] = {'A',0};
    static const WCHAR wszPattern2[] = {'a','X',0};
    static const WCHAR wszPattern3[] = {'A','y',0};
    static const WCHAR wszPattern4[] = {'a','b',0};
    LPWSTR retW;
    LPSTR retA;
    
    check_strrstri(A, szTest, 4, "A", szTest+1);
    check_strrstri(A, szTest, 4, "aX", szTest+1);
    check_strrstri(A, szTest, 4, "Ay", NULL);
    check_strrstri(W, wszTest, 4, wszPattern1, wszTest+1);
    check_strrstri(W, wszTest, 4, wszPattern2, wszTest+1);
    check_strrstri(W, wszTest, 4, wszPattern3, NULL);

    check_strrstri(A, szTest2, 4, "ab", szTest2+2);
    check_strrstri(A, szTest2, 3, "ab", szTest2+2);
    check_strrstri(A, szTest2, 2, "ab", szTest2);
    check_strrstri(A, szTest2, 1, "ab", szTest2);
    check_strrstri(A, szTest2, 0, "ab", NULL);
    check_strrstri(W, wszTest2, 4, wszPattern4, wszTest2+2);
    check_strrstri(W, wszTest2, 3, wszPattern4, wszTest2+2);
    check_strrstri(W, wszTest2, 2, wszPattern4, wszTest2);
    check_strrstri(W, wszTest2, 1, wszPattern4, wszTest2);
    check_strrstri(W, wszTest2, 0, wszPattern4, NULL);

}

static void test_SHAnsiToAnsi(void)
{
  char dest[8];
  DWORD dwRet;

  if (!pSHAnsiToAnsi)
  {
    win_skip("SHAnsiToAnsi() is not available\n");
    return;
  }

  memset(dest, '\n', sizeof(dest));
  dwRet = pSHAnsiToAnsi("hello", dest, sizeof(dest)/sizeof(dest[0]));
  ok(dwRet == 6 && !memcmp(dest, "hello\0\n\n", sizeof(dest)),
     "SHAnsiToAnsi: expected 6, \"hello\\0\\n\\n\", got %d, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
     dwRet, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_SHUnicodeToUnicode(void)
{
  static const WCHAR lpInit[] = { '\n','\n','\n','\n','\n','\n','\n','\n' };
  static const WCHAR lpSrc[] = { 'h','e','l','l','o','\0' };
  static const WCHAR lpRes[] = { 'h','e','l','l','o','\0','\n','\n' };
  WCHAR dest[8];
  DWORD dwRet;

  if (!pSHUnicodeToUnicode)
  {
    win_skip("SHUnicodeToUnicode() is not available\n");
    return;
  }

  memcpy(dest, lpInit, sizeof(lpInit));
  dwRet = pSHUnicodeToUnicode(lpSrc, dest, sizeof(dest)/sizeof(dest[0]));
  ok(dwRet == 6 && !memcmp(dest, lpRes, sizeof(dest)),
     "SHUnicodeToUnicode: expected 6, \"hello\\0\\n\\n\", got %d, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
     dwRet, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_StrXXX_overflows(void)
{
    CHAR str1[2*MAX_PATH+1], buf[2*MAX_PATH];
    WCHAR wstr1[2*MAX_PATH+1], wbuf[2*MAX_PATH];
    const WCHAR fmt[] = {'%','s',0};
    STRRET strret;
    int ret;
    int i;

    for (i=0; i<2*MAX_PATH; i++)
    {
        str1[i] = '0'+(i%10);
        wstr1[i] = '0'+(i%10);
    }
    str1[2*MAX_PATH] = 0;
    wstr1[2*MAX_PATH] = 0;

    memset(buf, 0xbf, sizeof(buf));
    expect_eq(StrCpyNA(buf, str1, 10), buf, PCHAR, "%p");
    expect_eq(buf[9], 0, CHAR, "%x");
    expect_eq(buf[10], '\xbf', CHAR, "%x");

    if (pStrCatBuffA)
    {
        expect_eq(pStrCatBuffA(buf, str1, 100), buf, PCHAR, "%p");
        expect_eq(buf[99], 0, CHAR, "%x");
        expect_eq(buf[100], '\xbf', CHAR, "%x");
    }
    else
        win_skip("StrCatBuffA() is not available\n");

    memset(wbuf, 0xbf, sizeof(wbuf));
    expect_eq(StrCpyNW(wbuf, wstr1, 10), wbuf, PWCHAR, "%p");
    expect_eq(wbuf[9], 0, WCHAR, "%x");
    expect_eq(wbuf[10], (WCHAR)0xbfbf, WCHAR, "%x");

    if (pStrCatBuffW)
    {
        expect_eq(pStrCatBuffW(wbuf, wstr1, 100), wbuf, PWCHAR, "%p");
        expect_eq(wbuf[99], 0, WCHAR, "%x");
        expect_eq(wbuf[100], (WCHAR)0xbfbf, WCHAR, "%x");
    }
    else
        win_skip("StrCatBuffW() is not available\n");

    if (pStrRetToBufW)
    {
        memset(wbuf, 0xbf, sizeof(wbuf));
        strret.uType = STRRET_WSTR;
        U(strret).pOleStr = StrDupW(wstr1);
        expect_eq2(pStrRetToBufW(&strret, NULL, wbuf, 10), S_OK, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) /* Vista */, HRESULT, "%x");
        expect_eq(wbuf[9], 0, WCHAR, "%x");
        expect_eq(wbuf[10], (WCHAR)0xbfbf, WCHAR, "%x");
    }
    else
        win_skip("StrRetToBufW() is not available\n");

    if (pStrRetToBufA)
    {
        memset(buf, 0xbf, sizeof(buf));
        strret.uType = STRRET_CSTR;
        StrCpyN(U(strret).cStr, str1, MAX_PATH);
        expect_eq2(pStrRetToBufA(&strret, NULL, buf, 10), S_OK, HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) /* Vista */, HRESULT, "%x");
        expect_eq(buf[9], 0, CHAR, "%x");
        expect_eq(buf[10], (CHAR)0xbf, CHAR, "%x");
    }
    else
        win_skip("StrRetToBufA() is not available\n");

    if (pwnsprintfA)
    {
        memset(buf, 0xbf, sizeof(buf));
        ret = pwnsprintfA(buf, 10, "%s", str1);
        ok(broken(ret == 9) || ret == -1 /* Vista */, "Unexpected wsnprintfA return %d, expected 9 or -1\n", ret);
        expect_eq(buf[9], 0, CHAR, "%x");
        expect_eq(buf[10], (CHAR)0xbf, CHAR, "%x");
    }
    else
        win_skip("wnsprintfA() is not available\n");

    if (pwnsprintfW)
    {
        memset(wbuf, 0xbf, sizeof(wbuf));
        ret = pwnsprintfW(wbuf, 10, fmt, wstr1);
        ok(broken(ret == 9) || ret == -1 /* Vista */, "Unexpected wsnprintfW return %d, expected 9 or -1\n", ret);
        expect_eq(wbuf[9], 0, WCHAR, "%x");
        expect_eq(wbuf[10], (WCHAR)0xbfbf, WCHAR, "%x");
    }
    else
        win_skip("wnsprintfW() is not available\n");
}

START_TEST(string)
{
  HMODULE hShlwapi;
  TCHAR thousandDelim[8];
  TCHAR decimalDelim[8];
  CoInitialize(0);

  GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousandDelim, 8);
  GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimalDelim, 8);

  hShlwapi = GetModuleHandleA("shlwapi");
  pIntlStrEqWorkerA = (void *)GetProcAddress(hShlwapi, "IntlStrEqWorkerA");
  pIntlStrEqWorkerW = (void *)GetProcAddress(hShlwapi, "IntlStrEqWorkerW");
  pSHAnsiToAnsi = (void *)GetProcAddress(hShlwapi, (LPSTR)345);
  pSHUnicodeToUnicode = (void *)GetProcAddress(hShlwapi, (LPSTR)346);
  pStrCatBuffA = (void *)GetProcAddress(hShlwapi, "StrCatBuffA");
  pStrCatBuffW = (void *)GetProcAddress(hShlwapi, "StrCatBuffW");
  pStrCpyNXA = (void *)GetProcAddress(hShlwapi, (LPSTR)399);
  pStrCpyNXW = (void *)GetProcAddress(hShlwapi, (LPSTR)400);
  pStrFormatByteSize64A = (void *)GetProcAddress(hShlwapi, "StrFormatByteSize64A");
  pStrFormatKBSizeA = (void *)GetProcAddress(hShlwapi, "StrFormatKBSizeA");
  pStrFormatKBSizeW = (void *)GetProcAddress(hShlwapi, "StrFormatKBSizeW");
  pStrIsIntlEqualA = (void *)GetProcAddress(hShlwapi, "StrIsIntlEqualA");
  pStrIsIntlEqualW = (void *)GetProcAddress(hShlwapi, "StrIsIntlEqualW");
  pStrRetToBSTR = (void *)GetProcAddress(hShlwapi, "StrRetToBSTR");
  pStrRetToBufA = (void *)GetProcAddress(hShlwapi, "StrRetToBufA");
  pStrRetToBufW = (void *)GetProcAddress(hShlwapi, "StrRetToBufW");
  pwnsprintfA = (void *)GetProcAddress(hShlwapi, "wnsprintfA");
  pwnsprintfW = (void *)GetProcAddress(hShlwapi, "wnsprintfW");

  test_StrChrA();
  test_StrChrW();
  test_StrChrIA();
  test_StrChrIW();
  test_StrRChrA();
  test_StrRChrW();
  test_StrCpyW();
  test_StrToIntA();
  test_StrToIntW();
  test_StrToIntExA();
  test_StrToIntExW();
  test_StrDupA();
  if (lstrcmp(thousandDelim, ",")==0 && lstrcmp(decimalDelim, ".")==0)
  {
    /* these tests are locale-dependent */
    test_StrFormatByteSize64A();
    test_StrFormatKBSizeA();
    test_StrFormatKBSizeW();
  }

  /* language-dependent test */
  if (PRIMARYLANGID(GetUserDefaultLangID()) != LANG_ENGLISH)
    trace("Skipping StrFromTimeInterval test for non English language\n");
  else
    test_StrFromTimeIntervalA();

  test_StrCmpA();
  test_StrCmpW();
  test_StrRetToBSTR();
  test_StrCpyNXA();
  test_StrCpyNXW();
  test_StrRStrI();
  test_SHAnsiToAnsi();
  test_SHUnicodeToUnicode();
  test_StrXXX_overflows();

  CoUninitialize();
}
