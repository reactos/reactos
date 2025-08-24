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

static BOOL    (WINAPI *pChrCmpIA)(CHAR, CHAR);
static BOOL    (WINAPI *pChrCmpIW)(WCHAR, WCHAR);
static BOOL    (WINAPI *pIntlStrEqWorkerA)(BOOL,LPCSTR,LPCSTR,int);
static BOOL    (WINAPI *pIntlStrEqWorkerW)(BOOL,LPCWSTR,LPCWSTR,int);
static DWORD   (WINAPI *pSHAnsiToAnsi)(LPCSTR,LPSTR,int);
static DWORD   (WINAPI *pSHUnicodeToUnicode)(LPCWSTR,LPWSTR,int);
static LPSTR   (WINAPI *pStrCatBuffA)(LPSTR,LPCSTR,INT);
static LPWSTR  (WINAPI *pStrCatBuffW)(LPWSTR,LPCWSTR,INT);
static DWORD   (WINAPI *pStrCatChainW)(LPWSTR,DWORD,DWORD,LPCWSTR);
static LPSTR   (WINAPI *pStrCpyNXA)(LPSTR,LPCSTR,int);
static LPWSTR  (WINAPI *pStrCpyNXW)(LPWSTR,LPCWSTR,int);
static LPSTR   (WINAPI *pStrFormatByteSize64A)(LONGLONG,LPSTR,UINT);
static HRESULT (WINAPI *pStrFormatByteSizeEx)(LONGLONG,SFBS_FLAGS,LPWSTR,UINT);
static LPSTR   (WINAPI *pStrFormatKBSizeA)(LONGLONG,LPSTR,UINT);
static LPWSTR  (WINAPI *pStrFormatKBSizeW)(LONGLONG,LPWSTR,UINT);
static BOOL    (WINAPI *pStrIsIntlEqualA)(BOOL,LPCSTR,LPCSTR,int);
static BOOL    (WINAPI *pStrIsIntlEqualW)(BOOL,LPCWSTR,LPCWSTR,int);
static LPWSTR  (WINAPI *pStrPBrkW)(LPCWSTR,LPCWSTR);
static LPSTR   (WINAPI *pStrRChrA)(LPCSTR,LPCSTR,WORD);
static HRESULT (WINAPI *pStrRetToBSTR)(STRRET*,LPCITEMIDLIST,BSTR*);
static HRESULT (WINAPI *pStrRetToBufA)(STRRET*,LPCITEMIDLIST,LPSTR,UINT);
static HRESULT (WINAPI *pStrRetToBufW)(STRRET*,LPCITEMIDLIST,LPWSTR,UINT);
static LPWSTR  (WINAPI *pStrStrNW)(LPCWSTR,LPCWSTR,UINT);
static LPWSTR  (WINAPI *pStrStrNIW)(LPCWSTR,LPCWSTR,UINT);
static INT     (WINAPIV *pwnsprintfA)(LPSTR,INT,LPCSTR, ...);
static INT     (WINAPIV *pwnsprintfW)(LPWSTR,INT,LPCWSTR, ...);
static LPWSTR  (WINAPI *pStrChrNW)(LPCWSTR,WCHAR,UINT);
static BOOL    (WINAPI *pStrToInt64ExA)(LPCSTR,DWORD,LONGLONG*);
static BOOL    (WINAPI *pStrToInt64ExW)(LPCWSTR,DWORD,LONGLONG*);

/* StrToInt/StrToIntEx results */
typedef struct tagStrToIntResult
{
  const char* string;
  int str_to_int;
  LONGLONG str_to_int64_ex;
  LONGLONG str_to_int64_hex;
  BOOL failure;
} StrToIntResult;

static const StrToIntResult StrToInt_results[] = {
     { "1099", 1099, 1099, 1099 },
     { "4294967319", 23, ((LONGLONG)1 << 32) | 23, ((LONGLONG)1 << 32) | 23 },
     { "+88987", 0, 88987, 88987 },
     { "012", 12, 12, 12 },
     { "-55", -55, -55, -55 },
     { "-0", 0, 0, 0 },
     { "0x44ff", 0, 0, 0x44ff },
     { "0x2bdc546291f4b1", 0, 0, ((LONGLONG)0x2bdc54 << 32) | 0x6291f4b1 },
     { "+0x44f4", 0, 0, 0x44f4 },
     { "-0x44fd", 0, 0, 0x44fd },
     { "+ 88987", 0, 0, 0, TRUE },

     { "- 55", 0, 0, 0, TRUE },
     { "- 0", 0, 0, 0, TRUE },
     { "+ 0x44f4", 0, 0, 0, TRUE },
     { "--0x44fd", 0, 0, 0, TRUE },
     { " 1999", 0, 1999, 1999 },
     { " +88987", 0, 88987, 88987 },
     { " 012", 0, 12, 12 },
     { " -55", 0, -55, -55 },
     { " 0x44ff", 0, 0, 0x44ff },
     { " +0x44f4", 0, 0, 0x44f4 },
     { " -0x44fd", 0, 0, 0x44fd },
     { "\t\n +3", 0, 3, 3 },
     { "\v+4", 0, 0, 0, TRUE },
     { "\f+5", 0, 0, 0, TRUE },
     { "\r+6", 0, 0, 0, TRUE },
     { NULL, 0, 0, 0 }
};

/* pStrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFormatSizeResult
{
  LONGLONG value;
  const char* byte_size_64;
  const char* kb_size;
  int kb_size_broken;
  const char* kb_size2;
} StrFormatSizeResult;


static const StrFormatSizeResult StrFormatSize_results[] = {
  { -1023, "-1023 bytes", "0 KB"},
  { -24, "-24 bytes", "0 KB"},
  { 309, "309 bytes", "1 KB"},
  { 10191, "9.95 KB", "10 KB"},
  { 100353, "98.0 KB", "99 KB"},
  { 1022286, "998 KB", "999 KB"},
  { 1046862, "0.99 MB", "1,023 KB", 1, "1023 KB"},
  { 1048574619, "999 MB", "1,023,999 KB", 1, "1023999 KB"},
  { 1073741775, "0.99 GB", "1,048,576 KB", 1, "1048576 KB"},
  { ((LONGLONG)0x000000f9 << 32) | 0xfffff94e, "999 GB", "1,048,575,999 KB", 1, "1048575999 KB"},
  { ((LONGLONG)0x000000ff << 32) | 0xfffffa9b, "0.99 TB", "1,073,741,823 KB", 1, "1073741823 KB"},
  { ((LONGLONG)0x0003e7ff << 32) | 0xfffffa9b, "999 TB", "1,073,741,823,999 KB", 1, "4294967295 KB"},
  { ((LONGLONG)0x0003ffff << 32) | 0xfffffbe8, "0.99 PB", "1,099,511,627,775 KB", 1, "4294967295 KB"},
  { ((LONGLONG)0x0f9fffff << 32) | 0xfffffd35, "999 PB", "1,099,511,627,776,000 KB", 1, "0 KB"},
  { ((LONGLONG)0x0fffffff << 32) | 0xfffffa9b, "0.99 EB", "1,125,899,906,842,623 KB", 1, "4294967295 KB"},
  { 0, NULL, NULL }
};

/* StrFromTimeIntervalA/StrFromTimeIntervalW results */
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


/* Returns true if the user interface is in English. Note that this does not
 * presume of the formatting of dates, numbers, etc.
 */
static BOOL is_lang_english(void)
{
    static HMODULE hkernel32 = NULL;
    static LANGID (WINAPI *pGetThreadUILanguage)(void) = NULL;
    static LANGID (WINAPI *pGetUserDefaultUILanguage)(void) = NULL;

    if (!hkernel32)
    {
        hkernel32 = GetModuleHandleA("kernel32.dll");
        pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");
        pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    }
    if (pGetThreadUILanguage)
        return PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH;
    if (pGetUserDefaultUILanguage)
        return PRIMARYLANGID(pGetUserDefaultUILanguage()) == LANG_ENGLISH;

    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

/* Returns true if the dates, numbers, etc. are formatted using English
 * conventions.
 */
static BOOL is_locale_english(void)
{
    /* Surprisingly GetThreadLocale() is irrelevant here */
    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

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
  LPWSTR lpRes;

  while(result->value)
  {
    MultiByteToWideChar(CP_ACP, 0, result->byte_size_64, -1, szSrc, ARRAY_SIZE(szSrc));

    lpRes = StrCpyW(szBuff, szSrc);
    ok(!StrCmpW(szSrc, szBuff) && lpRes == szBuff, "Copied string %s wrong\n", result->byte_size_64);
    result++;
  }

  /* this test crashes on win2k SP4 */
  /*lpRes = StrCpyW(szBuff, NULL);*/
  /*ok(lpRes == szBuff, "Wrong return value: got %p expected %p\n", lpRes, szBuff);*/

  /* this test crashes on win2k SP4 */
  /*lpRes = StrCpyW(NULL, szSrc);*/
  /*ok(lpRes == NULL, "Wrong return value: got %p expected NULL\n", lpRes);*/

  /* this test crashes on win2k SP4 */
  /*lpRes = StrCpyW(NULL, NULL);*/
  /*ok(lpRes == NULL, "Wrong return value: got %p expected NULL\n", lpRes);*/
}

static void test_StrChrNW(void)
{
    static const WCHAR string[] = {'T','e','s','t','i','n','g',' ','S','t','r','i','n','g',0};
    LPWSTR p;

    if (!pStrChrNW)
    {
        win_skip("StrChrNW not available\n");
        return;
    }

    p = pStrChrNW(string,'t',10);
    ok(*p=='t',"Found wrong 't'\n");
    ok(*(p+1)=='i',"next should be 'i'\n");

    p = pStrChrNW(string,'S',10);
    ok(*p=='S',"Found wrong 'S'\n");

    p = pStrChrNW(string,'r',10);
    ok(p==NULL,"Should not have found 'r'\n");
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
    MultiByteToWideChar(CP_ACP, 0, result->string, -1, szBuff, ARRAY_SIZE(szBuff));
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
    if (result->failure)
        ok(!bRet, "Got %d instead of failure for '%s'\n", return_val, result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == (int)result->str_to_int64_ex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,STIF_SUPPORT_HEX,&return_val);
    if (result->failure)
        ok(!bRet, "Got %d instead of failure for '%s'\n", return_val, result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == (int)result->str_to_int64_hex, "converted '%s' wrong (%d)\n",
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
    MultiByteToWideChar(CP_ACP, 0, result->string, -1, szBuff, ARRAY_SIZE(szBuff));
    bRet = StrToIntExW(szBuff, 0, &return_val);
    if (result->failure)
        ok(!bRet, "Got %d instead of failure for '%s'\n", return_val, result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == (int)result->str_to_int64_ex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(CP_ACP, 0, result->string, -1, szBuff, ARRAY_SIZE(szBuff));
    bRet = StrToIntExW(szBuff, STIF_SUPPORT_HEX, &return_val);
    if (result->failure)
        ok(!bRet, "Got %d instead of failure for '%s'\n", return_val, result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == (int)result->str_to_int64_hex, "converted '%s' wrong (%d)\n",
         result->string, return_val);
    result++;
  }

  return_val = -1;
  bRet = StrToIntExW(L"\x0661\x0662", 0, &return_val);
  ok( !bRet, "Returned %d for Unicode digits\n", return_val );
  bRet = StrToIntExW(L"\x07c3\x07c4", 0, &return_val);
  ok( !bRet, "Returned %d for Unicode digits\n", return_val );
  bRet = StrToIntExW(L"\xa0-2", 0, &return_val);
  ok( !bRet, "Returned %d for Unicode space\n", return_val );
}

static void test_StrToInt64ExA(void)
{
  const StrToIntResult *result = StrToInt_results;
  LONGLONG return_val;
  BOOL bRet;

  if (!pStrToInt64ExA)
  {
    win_skip("StrToInt64ExA() is not available\n");
    return;
  }

  while (result->string)
  {
    return_val = -1;
    bRet = pStrToInt64ExA(result->string,0,&return_val);
    if (result->failure)
        ok(!bRet, "Got %s instead of failure for '%s'\n",
           wine_dbgstr_longlong(return_val), result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == result->str_to_int64_ex, "converted '%s' wrong (%s)\n",
         result->string, wine_dbgstr_longlong(return_val));
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    bRet = pStrToInt64ExA(result->string,STIF_SUPPORT_HEX,&return_val);
    if (result->failure)
        ok(!bRet, "Got %s instead of failure for '%s'\n",
           wine_dbgstr_longlong(return_val), result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == result->str_to_int64_hex, "converted '%s' wrong (%s)\n",
         result->string, wine_dbgstr_longlong(return_val));
    result++;
  }
}

static void test_StrToInt64ExW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  LONGLONG return_val;
  BOOL bRet;

  if (!pStrToInt64ExW)
  {
    win_skip("StrToInt64ExW() is not available\n");
    return;
  }

  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(CP_ACP, 0, result->string, -1, szBuff, ARRAY_SIZE(szBuff));
    bRet = pStrToInt64ExW(szBuff, 0, &return_val);
    if (result->failure)
        ok(!bRet, "Got %s instead of failure for '%s'\n",
           wine_dbgstr_longlong(return_val), result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == result->str_to_int64_ex, "converted '%s' wrong (%s)\n",
         result->string, wine_dbgstr_longlong(return_val));
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(CP_ACP, 0, result->string, -1, szBuff, ARRAY_SIZE(szBuff));
    bRet = pStrToInt64ExW(szBuff, STIF_SUPPORT_HEX, &return_val);
    if (result->failure)
        ok(!bRet, "Got %s instead of failure for '%s'\n",
           wine_dbgstr_longlong(return_val), result->string);
    else
        ok(bRet, "Failed for '%s'\n", result->string);
    if (bRet)
      ok(return_val == result->str_to_int64_hex, "converted '%s' wrong (%s)\n",
         result->string, wine_dbgstr_longlong(return_val));
    result++;
  }

  return_val = -1;
  bRet = pStrToInt64ExW(L"\x0661\x0662", 0, &return_val);
  ok( !bRet, "Returned %s for Unicode digits\n", wine_dbgstr_longlong(return_val) );
  bRet = pStrToInt64ExW(L"\x07c3\x07c4", 0, &return_val);
  ok( !bRet, "Returned %s for Unicode digits\n", wine_dbgstr_longlong(return_val) );
  bRet = pStrToInt64ExW(L"\xa0-2", 0, &return_val);
  ok( !bRet, "Returned %s for Unicode space\n", wine_dbgstr_longlong(return_val) );
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
        "Formatted %s wrong: got %s, expected %s\n",
       wine_dbgstr_longlong(result->value), szBuff, result->byte_size_64);

    result++;
  }
}

static void test_StrFormatByteSizeEx(void)
{
  WCHAR szBuff[256];
  HRESULT hr;
  LONGLONG test_value = 2147483647;

  if (!pStrFormatByteSizeEx)
  {
    win_skip("StrFormatByteSizeEx is not available \n");
    return;
  }

  hr = pStrFormatByteSizeEx(0xdeadbeef,
                            SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS, szBuff, 0);
  ok(hr == E_INVALIDARG, "Unexpected hr: %#lx expected: %#lx\n", hr, E_INVALIDARG);

  hr = pStrFormatByteSizeEx(0xdeadbeef, 10, szBuff, 256);
  ok(hr == E_INVALIDARG, "Unexpected hr: %#lx expected: %#lx\n", hr, E_INVALIDARG);

  hr = pStrFormatByteSizeEx(test_value, SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT,
                            szBuff, 256);
  ok(hr == S_OK, "Invalid arguments \n");
  ok(!wcscmp(szBuff, L"2.00 GB"), "Formatted %s wrong: got %ls, expected 2.00 GB\n",
     wine_dbgstr_longlong(test_value), szBuff);

  hr = pStrFormatByteSizeEx(test_value, SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS,
                            szBuff, 256);
  ok(hr == S_OK, "Invalid arguments \n");
  ok(!wcscmp(szBuff, L"1.99 GB"), "Formatted %s wrong: got %ls, expected 1.99 GB\n",
     wine_dbgstr_longlong(test_value), szBuff);
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
    WideCharToMultiByte(CP_ACP, 0, szBuffW, -1, szBuff, ARRAY_SIZE(szBuff), NULL, NULL);

    ok(!strcmp(result->kb_size, szBuff), "Formatted %s wrong: got %s, expected %s\n",
       wine_dbgstr_longlong(result->value), szBuff, result->kb_size);
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

    /* shlwapi on Win98 SE does not appear to apply delimiters to the output
     * and does not correctly handle extremely large values. */
    ok(!strcmp(result->kb_size, szBuff) ||
      (result->kb_size_broken && !strcmp(result->kb_size2, szBuff)),
        "Formatted %s wrong: got %s, expected %s\n",
       wine_dbgstr_longlong(result->value), szBuff, result->kb_size);
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

    ok(!strcmp(result->time_interval, szBuff), "Formatted %ld %d wrong: %s\n",
       result->ms, result->digits, szBuff);
    result++;
  }
}

static void test_StrCmpA(void)
{
  static const char str1[] = {'a','b','c','d','e','f'};
  static const char str2[] = {'a','B','c','d','e','f'};
  ok(0 != StrCmpNA(str1, str2, 6), "StrCmpNA is case-insensitive\n");
  ok(0 == StrCmpNIA(str1, str2, 6), "StrCmpNIA is case-sensitive\n");
  if (pChrCmpIA) {
    ok(!pChrCmpIA('a', 'a'), "ChrCmpIA doesn't work at all!\n");
    ok(!pChrCmpIA('b', 'B'), "ChrCmpIA is not case-insensitive\n");
    ok(pChrCmpIA('a', 'z'), "ChrCmpIA believes that a == z!\n");
  }
  else
    win_skip("ChrCmpIA() is not available\n");

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
  if (pChrCmpIW) {
    ok(!pChrCmpIW('a', 'a'), "ChrCmpIW doesn't work at all!\n");
    ok(!pChrCmpIW('b', 'B'), "ChrCmpIW is not case-insensitive\n");
    ok(pChrCmpIW('a', 'z'), "ChrCmpIW believes that a == z!\n");
  }
  else
    win_skip("ChrCmpIW() is not available\n");

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
    strret.pOleStr = CoDupStrW("Test");
    bstr = 0;
    ret = pStrRetToBSTR(&strret, NULL, &bstr);
    ok(ret == S_OK && bstr && !wcscmp(bstr, szTestW),
       "STRRET_WSTR: dup failed, ret=0x%08lx, bstr %p\n", ret, bstr);
    SysFreeString(bstr);

    strret.uType = STRRET_CSTR;
    lstrcpyA(strret.cStr, "Test");
    ret = pStrRetToBSTR(&strret, NULL, &bstr);
    ok(ret == S_OK && bstr && !wcscmp(bstr, szTestW),
       "STRRET_CSTR: dup failed, ret=0x%08lx, bstr %p\n", ret, bstr);
    SysFreeString(bstr);

    strret.uType = STRRET_OFFSET;
    strret.uOffset = 1;
    strcpy((char*)&iidl, " Test");
    ret = pStrRetToBSTR(&strret, iidl, &bstr);
    ok(ret == S_OK && bstr && !wcscmp(bstr, szTestW),
       "STRRET_OFFSET: dup failed, ret=0x%08lx, bstr %p\n", ret, bstr);
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
  lpszRes = pStrCpyNXA(dest, lpSrc, ARRAY_SIZE(dest));
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
  lpszRes = pStrCpyNXW(dest, lpSrc, ARRAY_SIZE(dest));
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

  if (pSHAnsiToAnsi == (void *)pStrPBrkW)
  {
    win_skip("Ordinal 345 corresponds to StrPBrkW, skipping SHAnsiToAnsi tests\n");
    return;
  }

  memset(dest, '\n', sizeof(dest));
  dwRet = pSHAnsiToAnsi("hello", dest, ARRAY_SIZE(dest));
  ok(dwRet == 6 && !memcmp(dest, "hello\0\n\n", sizeof(dest)),
     "SHAnsiToAnsi: expected 6, \"hello\\0\\n\\n\", got %ld, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
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

  if (pSHUnicodeToUnicode == (void *)pStrRChrA)
  {
    win_skip("Ordinal 346 corresponds to StrRChrA, skipping SHUnicodeToUnicode tests\n");
    return;
  }

  memcpy(dest, lpInit, sizeof(lpInit));
  dwRet = pSHUnicodeToUnicode(lpSrc, dest, ARRAY_SIZE(dest));
  ok(dwRet == 6 && !memcmp(dest, lpRes, sizeof(dest)),
     "SHUnicodeToUnicode: expected 6, \"hello\\0\\n\\n\", got %ld, \"%d,%d,%d,%d,%d,%d,%d,%d\"\n",
     dwRet, dest[0], dest[1], dest[2], dest[3], dest[4], dest[5], dest[6], dest[7]);
}

static void test_StrXXX_overflows(void)
{
    CHAR str1[2*MAX_PATH+1], buf[2*MAX_PATH];
    WCHAR wstr1[2*MAX_PATH+1], wbuf[2*MAX_PATH];
    const WCHAR fmt[] = {'%','s',0};
    STRRET strret;
    HRESULT hres;
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

if (0)
{
    /* crashes on XP */
    StrCpyNW(wbuf, (LPCWSTR)0x1, 10);
    StrCpyNW((LPWSTR)0x1, wstr1, 10);
}

    memset(wbuf, 0xbf, sizeof(wbuf));
    expect_eq(StrCpyNW(wbuf, (LPCWSTR)0x1, 1), wbuf, PWCHAR, "%p");
    expect_eq(wbuf[0], 0, WCHAR, "%x");
    expect_eq(wbuf[1], (WCHAR)0xbfbf, WCHAR, "%x");

    memset(wbuf, 0xbf, sizeof(wbuf));
    expect_eq(StrCpyNW(wbuf, 0, 10), wbuf, PWCHAR, "%p");
    expect_eq(wbuf[0], 0, WCHAR, "%x");
    expect_eq(wbuf[1], (WCHAR)0xbfbf, WCHAR, "%x");

    memset(wbuf, 0xbf, sizeof(wbuf));
    expect_eq(StrCpyNW(wbuf, 0, 0), wbuf, PWCHAR, "%p");
    expect_eq(wbuf[0], (WCHAR)0xbfbf, WCHAR, "%x");
    expect_eq(wbuf[1], (WCHAR)0xbfbf, WCHAR, "%x");

    memset(wbuf, 0xbf, sizeof(wbuf));
    expect_eq(StrCpyNW(wbuf, wstr1, 0), wbuf, PWCHAR, "%p");
    expect_eq(wbuf[0], (WCHAR)0xbfbf, WCHAR, "%x");
    expect_eq(wbuf[1], (WCHAR)0xbfbf, WCHAR, "%x");

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
        strret.pOleStr = StrDupW(wstr1);
        hres = pStrRetToBufW(&strret, NULL, wbuf, 10);
        ok(hres == E_NOT_SUFFICIENT_BUFFER || broken(hres == S_OK) /* winxp */,
           "StrRetToBufW returned %08lx\n", hres);
        if (hres == E_NOT_SUFFICIENT_BUFFER)
            expect_eq(wbuf[0], 0, WCHAR, "%x");
        expect_eq(wbuf[9], 0, WCHAR, "%x");
        expect_eq(wbuf[10], (WCHAR)0xbfbf, WCHAR, "%x");

        memset(wbuf, 0xbf, sizeof(wbuf));
        strret.uType = STRRET_CSTR;
        StrCpyNA(strret.cStr, str1, MAX_PATH);
        hres = pStrRetToBufW(&strret, NULL, wbuf, 10);
        ok(hres == S_OK, "StrRetToBufW returned %08lx\n", hres);
        ok(!memcmp(wbuf, wstr1, 9*sizeof(WCHAR)) && !wbuf[9], "StrRetToBuf returned %s\n", wine_dbgstr_w(wbuf));

        memset(wbuf, 0xbf, sizeof(wbuf));
        strret.uType = STRRET_WSTR;
        strret.pOleStr = NULL;
        hres = pStrRetToBufW(&strret, NULL, wbuf, 10);
        ok(hres == E_FAIL, "StrRetToBufW returned %08lx\n", hres);
        ok(!wbuf[0], "StrRetToBuf returned %s\n", wine_dbgstr_w(wbuf));
    }
    else
        win_skip("StrRetToBufW() is not available\n");

    if (pStrRetToBufA)
    {
        memset(buf, 0xbf, sizeof(buf));
        strret.uType = STRRET_CSTR;
        StrCpyNA(strret.cStr, str1, MAX_PATH);
        expect_eq2(pStrRetToBufA(&strret, NULL, buf, 10), S_OK, E_NOT_SUFFICIENT_BUFFER /* Vista */, HRESULT, "%lx");
        expect_eq(buf[9], 0, CHAR, "%x");
        expect_eq(buf[10], (CHAR)0xbf, CHAR, "%x");
    }
    else
        win_skip("StrRetToBufA() is not available\n");

    if (pwnsprintfA)
    {
        memset(buf, 0xbf, sizeof(buf));
        ret = pwnsprintfA(buf, 10, "%s", str1);
        ok(broken(ret == 9) || ret == -1 /* Vista */, "Unexpected wnsprintfA return %d, expected 9 or -1\n", ret);
        expect_eq(buf[9], 0, CHAR, "%x");
        expect_eq(buf[10], (CHAR)0xbf, CHAR, "%x");

        memset(buf, 0xbf, sizeof(buf));
        ret = pwnsprintfA(buf + 1, -1, "%s", str1);
        ok(ret == -1, "got %d.\n", ret);
        expect_eq(buf[0], (CHAR)0xbf, CHAR, "%x");
        if (!broken(1))
        {
            /* This is 0xbf before Win8. */
            expect_eq(buf[1], 0, CHAR, "%x");
        }
        expect_eq(buf[2], (CHAR)0xbf, CHAR, "%x");

        memset(buf, 0xbf, sizeof(buf));
        ret = pwnsprintfA(buf + 1, 0, "%s", str1);
        ok(ret == -1, "got %d.\n", ret);
        expect_eq(buf[0], (CHAR)0xbf, CHAR, "%x");
        expect_eq(buf[1], (CHAR)0xbf, CHAR, "%x");

        memset(buf, 0xbf, sizeof(buf));
        ret = pwnsprintfA(buf, 1, "");
        ok(!ret, "got %d.\n", ret);
        expect_eq(buf[0], 0, CHAR, "%x");
        expect_eq(buf[1], (CHAR)0xbf, CHAR, "%x");
    }
    else
        win_skip("wnsprintfA() is not available\n");

    if (pwnsprintfW)
    {
        memset(wbuf, 0xbf, sizeof(wbuf));
        ret = pwnsprintfW(wbuf, 10, fmt, wstr1);
        ok(broken(ret == 9) || ret == -1 /* Vista */, "Unexpected wnsprintfW return %d, expected 9 or -1\n", ret);
        expect_eq(wbuf[9], 0, WCHAR, "%x");
        expect_eq(wbuf[10], (WCHAR)0xbfbf, WCHAR, "%x");

        memset(wbuf, 0xbf, sizeof(wbuf));
        ret = pwnsprintfW(wbuf + 1, -1, fmt, wstr1);
        ok(ret == -1, "got %d.\n", ret);
        expect_eq(wbuf[0], (WCHAR)0xbfbf, WCHAR, "%x");
        if (!broken(1))
        {
            /* This is 0xbfbf before Win8. */
            expect_eq(wbuf[1], 0, WCHAR, "%x");
        }
        expect_eq(wbuf[2], (WCHAR)0xbfbf, WCHAR, "%x");

        memset(wbuf, 0xbf, sizeof(wbuf));
        ret = pwnsprintfW(wbuf + 1, 0, fmt, wstr1);
        ok(ret == -1, "got %d.\n", ret);
        expect_eq(wbuf[0], (WCHAR)0xbfbf, WCHAR, "%x");
        expect_eq(wbuf[1], (WCHAR)0xbfbf, WCHAR, "%x");

        memset(wbuf, 0xbf, sizeof(wbuf));
        ret = pwnsprintfW(wbuf, 1, L"");
        ok(!ret, "got %d.\n", ret);
        expect_eq(wbuf[0], 0, WCHAR, "%x");
        expect_eq(wbuf[1], (WCHAR)0xbfbf, WCHAR, "%x");
    }
    else
        win_skip("wnsprintfW() is not available\n");
}

static void test_StrStrA(void)
{
    static const char *deadbeefA = "DeAdBeEf";

    const struct
    {
        const char *search;
        const char *expect;
    } StrStrA_cases[] =
    {
        {"", NULL},
        {"DeAd", deadbeefA},
        {"dead", NULL},
        {"AdBe", deadbeefA + 2},
        {"adbe", NULL},
        {"BeEf", deadbeefA + 4},
        {"beef", NULL},
    };

    LPSTR ret;
    int i;

    /* Tests crash on Win2k */
    if (0)
    {
        ret = StrStrA(NULL, NULL);
        ok(!ret, "Expected StrStrA to return NULL, got %p\n", ret);

        ret = StrStrA(NULL, "");
        ok(!ret, "Expected StrStrA to return NULL, got %p\n", ret);

        ret = StrStrA("", NULL);
        ok(!ret, "Expected StrStrA to return NULL, got %p\n", ret);
    }

    ret = StrStrA("", "");
    ok(!ret, "Expected StrStrA to return NULL, got %p\n", ret);

    for (i = 0; i < ARRAY_SIZE(StrStrA_cases); i++)
    {
        ret = StrStrA(deadbeefA, StrStrA_cases[i].search);
        ok(ret == StrStrA_cases[i].expect,
           "[%d] Expected StrStrA to return %p, got %p\n",
           i, StrStrA_cases[i].expect, ret);
    }
}

static void test_StrStrW(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'D','e','A','d','B','e','E','f',0};
    static const WCHAR deadW[] = {'D','e','A','d',0};
    static const WCHAR dead_lowerW[] = {'d','e','a','d',0};
    static const WCHAR adbeW[] = {'A','d','B','e',0};
    static const WCHAR adbe_lowerW[] = {'a','d','b','e',0};
    static const WCHAR beefW[] = {'B','e','E','f',0};
    static const WCHAR beef_lowerW[] = {'b','e','e','f',0};

    const struct
    {
        const WCHAR *search;
        const WCHAR *expect;
    } StrStrW_cases[] =
    {
        {emptyW, NULL},
        {deadW, deadbeefW},
        {dead_lowerW, NULL},
        {adbeW, deadbeefW + 2},
        {adbe_lowerW, NULL},
        {beefW, deadbeefW + 4},
        {beef_lowerW, NULL},
    };

    LPWSTR ret;
    int i;

    /* Tests crash on Win2k */
    if (0)
    {
        ret = StrStrW(NULL, NULL);
        ok(!ret, "Expected StrStrW to return NULL, got %p\n", ret);

        ret = StrStrW(NULL, emptyW);
        ok(!ret, "Expected StrStrW to return NULL, got %p\n", ret);

        ret = StrStrW(emptyW, NULL);
        ok(!ret, "Expected StrStrW to return NULL, got %p\n", ret);
    }

    ret = StrStrW(emptyW, emptyW);
    ok(!ret, "Expected StrStrW to return NULL, got %p\n", ret);

    for (i = 0; i < ARRAY_SIZE(StrStrW_cases); i++)
    {
        ret = StrStrW(deadbeefW, StrStrW_cases[i].search);
        ok(ret == StrStrW_cases[i].expect,
           "[%d] Expected StrStrW to return %p, got %p\n",
           i, StrStrW_cases[i].expect, ret);
    }
}

static void test_StrStrIA(void)
{
    static const char *deadbeefA = "DeAdBeEf";

    const struct
    {
        const char *search;
        const char *expect;
    } StrStrIA_cases[] =
    {
        {"", NULL},
        {"DeAd", deadbeefA},
        {"dead", deadbeefA},
        {"AdBe", deadbeefA + 2},
        {"adbe", deadbeefA + 2},
        {"BeEf", deadbeefA + 4},
        {"beef", deadbeefA + 4},
        {"cafe", NULL},
    };

    LPSTR ret;
    int i;

    /* Tests crash on Win2k */
    if (0)
    {
        ret = StrStrIA(NULL, NULL);
        ok(!ret, "Expected StrStrIA to return NULL, got %p\n", ret);

        ret = StrStrIA(NULL, "");
        ok(!ret, "Expected StrStrIA to return NULL, got %p\n", ret);

        ret = StrStrIA("", NULL);
        ok(!ret, "Expected StrStrIA to return NULL, got %p\n", ret);
    }

    ret = StrStrIA("", "");
    ok(!ret, "Expected StrStrIA to return NULL, got %p\n", ret);

    for (i = 0; i < ARRAY_SIZE(StrStrIA_cases); i++)
    {
        ret = StrStrIA(deadbeefA, StrStrIA_cases[i].search);
        ok(ret == StrStrIA_cases[i].expect,
           "[%d] Expected StrStrIA to return %p, got %p\n",
           i, StrStrIA_cases[i].expect, ret);
    }
}

static void test_StrStrIW(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'D','e','A','d','B','e','E','f',0};
    static const WCHAR deadW[] = {'D','e','A','d',0};
    static const WCHAR dead_lowerW[] = {'d','e','a','d',0};
    static const WCHAR adbeW[] = {'A','d','B','e',0};
    static const WCHAR adbe_lowerW[] = {'a','d','b','e',0};
    static const WCHAR beefW[] = {'B','e','E','f',0};
    static const WCHAR beef_lowerW[] = {'b','e','e','f',0};
    static const WCHAR cafeW[] = {'c','a','f','e',0};

    const struct
    {
        const WCHAR *search;
        const WCHAR *expect;
    } StrStrIW_cases[] =
    {
        {emptyW, NULL},
        {deadW, deadbeefW},
        {dead_lowerW, deadbeefW},
        {adbeW, deadbeefW + 2},
        {adbe_lowerW, deadbeefW + 2},
        {beefW, deadbeefW + 4},
        {beef_lowerW, deadbeefW + 4},
        {cafeW, NULL},
    };

    LPWSTR ret;
    int i;

    /* Tests crash on Win2k */
    if (0)
    {
        ret = StrStrIW(NULL, NULL);
        ok(!ret, "Expected StrStrIW to return NULL, got %p\n", ret);

        ret = StrStrIW(NULL, emptyW);
        ok(!ret, "Expected StrStrIW to return NULL, got %p\n", ret);

        ret = StrStrIW(emptyW, NULL);
        ok(!ret, "Expected StrStrIW to return NULL, got %p\n", ret);
    }

    ret = StrStrIW(emptyW, emptyW);
    ok(!ret, "Expected StrStrIW to return NULL, got %p\n", ret);

    for (i = 0; i < ARRAY_SIZE(StrStrIW_cases); i++)
    {
        ret = StrStrIW(deadbeefW, StrStrIW_cases[i].search);
        ok(ret == StrStrIW_cases[i].expect,
           "[%d] Expected StrStrIW to return %p, got %p\n",
           i, StrStrIW_cases[i].expect, ret);
    }
}

static void test_StrStrNW(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'D','e','A','d','B','e','E','f',0};
    static const WCHAR deadW[] = {'D','e','A','d',0};
    static const WCHAR dead_lowerW[] = {'d','e','a','d',0};
    static const WCHAR adbeW[] = {'A','d','B','e',0};
    static const WCHAR adbe_lowerW[] = {'a','d','b','e',0};
    static const WCHAR beefW[] = {'B','e','E','f',0};
    static const WCHAR beef_lowerW[] = {'b','e','e','f',0};

    const struct
    {
        const WCHAR *search;
        const UINT count;
        const WCHAR *expect;
    } StrStrNW_cases[] =
    {
        {emptyW, ARRAY_SIZE(deadbeefW), NULL},
        {deadW, ARRAY_SIZE(deadbeefW), deadbeefW},
        {dead_lowerW, ARRAY_SIZE(deadbeefW), NULL},
        {adbeW, ARRAY_SIZE(deadbeefW), deadbeefW + 2},
        {adbe_lowerW, ARRAY_SIZE(deadbeefW), NULL},
        {beefW, ARRAY_SIZE(deadbeefW), deadbeefW + 4},
        {beef_lowerW, ARRAY_SIZE(deadbeefW), NULL},
        {beefW, 0, NULL},
        {beefW, 1, NULL},
        {beefW, 2, NULL},
        {beefW, 3, NULL},
        {beefW, 4, NULL},
        {beefW, 5, deadbeefW + 4},
        {beefW, 6, deadbeefW + 4},
        {beefW, 7, deadbeefW + 4},
        {beefW, 8, deadbeefW + 4},
        {beefW, 9, deadbeefW + 4},
    };

    LPWSTR ret;
    UINT i;

    if (!pStrStrNW)
    {
        win_skip("StrStrNW() is not available\n");
        return;
    }

    ret = pStrStrNW(NULL, NULL, 0);
    ok(!ret, "Expected StrStrNW to return NULL, got %p\n", ret);

    ret = pStrStrNW(NULL, NULL, 10);
    ok(!ret, "Expected StrStrNW to return NULL, got %p\n", ret);

    ret = pStrStrNW(NULL, emptyW, 10);
    ok(!ret, "Expected StrStrNW to return NULL, got %p\n", ret);

    ret = pStrStrNW(emptyW, NULL, 10);
    ok(!ret, "Expected StrStrNW to return NULL, got %p\n", ret);

    ret = pStrStrNW(emptyW, emptyW, 10);
    ok(!ret, "Expected StrStrNW to return NULL, got %p\n", ret);

    for (i = 0; i < ARRAY_SIZE(StrStrNW_cases); i++)
    {
        ret = pStrStrNW(deadbeefW, StrStrNW_cases[i].search, StrStrNW_cases[i].count);
        ok(ret == StrStrNW_cases[i].expect,
           "[%d] Expected StrStrNW to return %p, got %p\n",
           i, StrStrNW_cases[i].expect, ret);
    }

    /* StrStrNW accepts counts larger than the search string length but rejects
     * counts larger than around 2G. The limit seems to change based on the
     * caller executable itself. */
    ret = pStrStrNW(deadbeefW, beefW, 100);
    ok(ret == deadbeefW + 4, "Expected StrStrNW to return deadbeefW + 4, got %p\n", ret);

    if (0)
    {
        ret = pStrStrNW(deadbeefW, beefW, ~0U);
        ok(!ret, "Expected StrStrNW to return NULL, got %p\n", ret);
    }
}

static void test_StrStrNIW(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'D','e','A','d','B','e','E','f',0};
    static const WCHAR deadW[] = {'D','e','A','d',0};
    static const WCHAR dead_lowerW[] = {'d','e','a','d',0};
    static const WCHAR adbeW[] = {'A','d','B','e',0};
    static const WCHAR adbe_lowerW[] = {'a','d','b','e',0};
    static const WCHAR beefW[] = {'B','e','E','f',0};
    static const WCHAR beef_lowerW[] = {'b','e','e','f',0};
    static const WCHAR cafeW[] = {'c','a','f','e',0};

    const struct
    {
        const WCHAR *search;
        const UINT count;
        const WCHAR *expect;
    } StrStrNIW_cases[] =
    {
        {emptyW, ARRAY_SIZE(deadbeefW), NULL},
        {deadW, ARRAY_SIZE(deadbeefW), deadbeefW},
        {dead_lowerW, ARRAY_SIZE(deadbeefW), deadbeefW},
        {adbeW, ARRAY_SIZE(deadbeefW), deadbeefW + 2},
        {adbe_lowerW, ARRAY_SIZE(deadbeefW), deadbeefW + 2},
        {beefW, ARRAY_SIZE(deadbeefW), deadbeefW + 4},
        {beef_lowerW, ARRAY_SIZE(deadbeefW), deadbeefW + 4},
        {cafeW, ARRAY_SIZE(deadbeefW), NULL},
        {beefW, 0, NULL},
        {beefW, 1, NULL},
        {beefW, 2, NULL},
        {beefW, 3, NULL},
        {beefW, 4, NULL},
        {beefW, 5, deadbeefW + 4},
        {beefW, 6, deadbeefW + 4},
        {beefW, 7, deadbeefW + 4},
        {beefW, 8, deadbeefW + 4},
        {beefW, 9, deadbeefW + 4},
        {beef_lowerW, 0, NULL},
        {beef_lowerW, 1, NULL},
        {beef_lowerW, 2, NULL},
        {beef_lowerW, 3, NULL},
        {beef_lowerW, 4, NULL},
        {beef_lowerW, 5, deadbeefW + 4},
        {beef_lowerW, 6, deadbeefW + 4},
        {beef_lowerW, 7, deadbeefW + 4},
        {beef_lowerW, 8, deadbeefW + 4},
        {beef_lowerW, 9, deadbeefW + 4},
    };

    LPWSTR ret;
    UINT i;

    if (!pStrStrNIW)
    {
        win_skip("StrStrNIW() is not available\n");
        return;
    }

    ret = pStrStrNIW(NULL, NULL, 0);
    ok(!ret, "Expected StrStrNIW to return NULL, got %p\n", ret);

    ret = pStrStrNIW(NULL, NULL, 10);
    ok(!ret, "Expected StrStrNIW to return NULL, got %p\n", ret);

    ret = pStrStrNIW(NULL, emptyW, 10);
    ok(!ret, "Expected StrStrNIW to return NULL, got %p\n", ret);

    ret = pStrStrNIW(emptyW, NULL, 10);
    ok(!ret, "Expected StrStrNIW to return NULL, got %p\n", ret);

    ret = pStrStrNIW(emptyW, emptyW, 10);
    ok(!ret, "Expected StrStrNIW to return NULL, got %p\n", ret);

    for (i = 0; i < ARRAY_SIZE(StrStrNIW_cases); i++)
    {
        ret = pStrStrNIW(deadbeefW, StrStrNIW_cases[i].search, StrStrNIW_cases[i].count);
        ok(ret == StrStrNIW_cases[i].expect,
           "[%d] Expected StrStrNIW to return %p, got %p\n",
           i, StrStrNIW_cases[i].expect, ret);
    }

    /* StrStrNIW accepts counts larger than the search string length but rejects
     * counts larger than around 2G. The limit seems to change based on the
     * caller executable itself. */
    ret = pStrStrNIW(deadbeefW, beefW, 100);
    ok(ret == deadbeefW + 4, "Expected StrStrNIW to return deadbeefW + 4, got %p\n", ret);

    if (0)
    {
        ret = pStrStrNIW(deadbeefW, beefW, ~0U);
        ok(!ret, "Expected StrStrNIW to return NULL, got %p\n", ret);
    }
}

static void test_StrCatChainW(void)
{
    static const WCHAR deadbeefW[] = {'D','e','A','d','B','e','E','f',0};
    static const WCHAR deadW[] = {'D','e','A','d',0};
    static const WCHAR beefW[] = {'B','e','E','f',0};

    WCHAR buf[32 + 1];
    DWORD ret;

    if (!pStrCatChainW)
    {
        win_skip("StrCatChainW is not available\n");
        return;
    }

    /* Test with NULL buffer */
    ret = pStrCatChainW(NULL, 0, 0, beefW);
    ok(ret == 0, "Expected StrCatChainW to return 0, got %lu\n", ret);

    /* Test with empty buffer */
    memset(buf, 0x11, sizeof(buf));
    ret = pStrCatChainW(buf, 0, 0, beefW);
    ok(ret == 0, "Expected StrCatChainW to return 0, got %lu\n", ret);
    ok(buf[0] == 0x1111, "Expected buf[0] = 0x1111, got %x\n", buf[0]);

    memcpy(buf, deadbeefW, sizeof(deadbeefW));
    ret = pStrCatChainW(buf, 0, -1, beefW);
    ok(ret == 8, "Expected StrCatChainW to return 8, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");

    /* Append data to existing string with offset = -1 */
    memset(buf, 0x11, sizeof(buf));
    ret = pStrCatChainW(buf, 32, 0, deadW);
    ok(ret == 4, "Expected StrCatChainW to return 4, got %lu\n", ret);
    ok(!memcmp(buf, deadW, sizeof(deadW)), "Buffer contains wrong data\n");

    ret = pStrCatChainW(buf, 32, -1, beefW);
    ok(ret == 8, "Expected StrCatChainW to return 8, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");

    /* Append data at a fixed offset */
    memset(buf, 0x11, sizeof(buf));
    ret = pStrCatChainW(buf, 32, 0, deadW);
    ok(ret == 4, "Expected StrCatChainW to return 4, got %lu\n", ret);
    ok(!memcmp(buf, deadW, sizeof(deadW)), "Buffer contains wrong data\n");

    ret = pStrCatChainW(buf, 32, 4, beefW);
    ok(ret == 8, "Expected StrCatChainW to return 8, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");

    /* Buffer exactly sufficient for string + terminating null */
    memset(buf, 0x11, sizeof(buf));
    ret = pStrCatChainW(buf, 5, 0, deadW);
    ok(ret == 4, "Expected StrCatChainW to return 4, got %lu\n", ret);
    ok(!memcmp(buf, deadW, sizeof(deadW)), "Buffer contains wrong data\n");

    /* Buffer too small, string will be truncated */
    memset(buf, 0x11, sizeof(buf));
    ret = pStrCatChainW(buf, 4, 0, deadW);
    if (ret == 4)
    {
        /* Windows 2000 and XP uses a slightly different implementation
         * for StrCatChainW, which doesn't ensure that strings are null-
         * terminated. Skip test if we detect such an implementation. */
        win_skip("Windows2000/XP behaviour detected for StrCatChainW, skipping tests\n");
        return;
    }
    ok(ret == 3, "Expected StrCatChainW to return 3, got %lu\n", ret);
    ok(!memcmp(buf, deadW, 3 * sizeof(WCHAR)), "Buffer contains wrong data\n");
    ok(!buf[3], "String is not nullterminated\n");
    ok(buf[4] == 0x1111, "Expected buf[4] = 0x1111, got %x\n", buf[4]);

    /* Overwrite part of an existing string */
    ret = pStrCatChainW(buf, 4, 1, beefW);
    ok(ret == 3, "Expected StrCatChainW to return 3, got %lu\n", ret);
    ok(buf[0] == 'D', "Expected buf[0] = 'D', got %x\n", buf[0]);
    ok(buf[1] == 'B', "Expected buf[1] = 'B', got %x\n", buf[1]);
    ok(buf[2] == 'e', "Expected buf[2] = 'e', got %x\n", buf[2]);
    ok(!buf[3], "String is not nullterminated\n");
    ok(buf[4] == 0x1111, "Expected buf[4] = 0x1111, got %x\n", buf[4]);

    /* Test appending to full buffer */
    memset(buf, 0x11, sizeof(buf));
    memcpy(buf, deadbeefW, sizeof(deadbeefW));
    memcpy(buf + 9, deadW, sizeof(deadW));
    ret = pStrCatChainW(buf, 9, 8, beefW);
    ok(ret == 8, "Expected StrCatChainW to return 8, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");
    ok(!memcmp(buf + 9, deadW, sizeof(deadW)), "Buffer contains wrong data\n");

    /* Offset points at the end of the buffer */
    ret = pStrCatChainW(buf, 9, 9, beefW);
    ok(ret == 8, "Expected StrCatChainW to return 8, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");
    ok(!memcmp(buf + 9, deadW, sizeof(deadW)), "Buffer contains wrong data\n");

    /* Offset points outside of the buffer */
    ret = pStrCatChainW(buf, 9, 10, beefW);
    ok(ret == 10, "Expected StrCatChainW to return 10, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");
    ok(!memcmp(buf + 9, deadW, sizeof(deadW)), "Buffer contains wrong data\n");

    /* The same but without nullterminated string */
    memcpy(buf, deadbeefW, sizeof(deadbeefW));
    ret = pStrCatChainW(buf, 5, -1, deadW);
    ok(ret == 8, "Expected StrCatChainW to return 8, got %lu\n", ret);
    ok(!memcmp(buf, deadbeefW, sizeof(deadbeefW)), "Buffer contains wrong data\n");

    ret = pStrCatChainW(buf, 5, 5, deadW);
    ok(ret == 4, "Expected StrCatChainW to return 4, got %lu\n", ret);
    ok(!memcmp(buf, deadW, sizeof(deadW)), "Buffer contains wrong data\n");
    ok(buf[5] == 'e', "Expected buf[5] = 'e', got %x\n", buf[5]);

    ret = pStrCatChainW(buf, 5, 6, deadW);
    ok(ret == 6, "Expected StrCatChainW to return 6, got %lu\n", ret);
    ok(!memcmp(buf, deadW, sizeof(deadW)), "Buffer contains wrong data\n");
    ok(buf[5] == 'e', "Expected buf[5] = 'e', got %x\n", buf[5]);
}

static void test_printf_format(void)
{
    const struct
    {
        const char *spec;
        unsigned int arg_size;
        ULONG64 arg;
        const void *argw;
    }
    tests[] =
    {
        { "%qu", 0, 10 },
        { "%ll", 0, 10 },
        { "%lu", sizeof(ULONG), 65537 },
        { "%llu", sizeof(ULONG64), 10 },
        { "%lllllllu", sizeof(ULONG64), 10 },
        { "%#lx", sizeof(ULONG), 10 },
        { "%#llx", sizeof(ULONG64), 0x1000000000 },
        { "%#lllx", sizeof(ULONG64), 0x1000000000 },
        { "%hu", sizeof(ULONG), 65537 },
        { "%hlu", sizeof(ULONG), 65537 },
        { "%hllx", sizeof(ULONG64), 0x100000010 },
        { "%hlllx", sizeof(ULONG64), 0x100000010 },
        { "%llhx", sizeof(ULONG64), 0x100000010 },
        { "%lllhx", sizeof(ULONG64), 0x100000010 },
        { "%lhu", sizeof(ULONG), 65537 },
        { "%hhu", sizeof(ULONG), 65537 },
        { "%hwu", sizeof(ULONG), 65537 },
        { "%whu", sizeof(ULONG), 65537 },
        { "%##lhllwlx", sizeof(ULONG64), 0x1000000010 },
        { "%##lhlwlx", sizeof(ULONG), 0x1000000010 },
        { "%04lhlwllx", sizeof(ULONG64), 0x1000000010 },
        { "%s", sizeof(ULONG_PTR), (ULONG_PTR)"str", L"str" },
        { "%S", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%ls", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%lS", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%lls", sizeof(ULONG_PTR), (ULONG_PTR)"str", L"str" },
        { "%llS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%llls", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%lllS", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%lllls", sizeof(ULONG_PTR), (ULONG_PTR)"str", L"str" },
        { "%llllS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%hs", sizeof(ULONG_PTR), (ULONG_PTR)"str" },
        { "%hS", sizeof(ULONG_PTR), (ULONG_PTR)"str" },
        { "%ws", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%wS", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%hhs", sizeof(ULONG_PTR), (ULONG_PTR)"str" },
        { "%hhS", sizeof(ULONG_PTR), (ULONG_PTR)"str" },
        { "%wws", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%wwS", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%wwws", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%wwwS", sizeof(ULONG_PTR), (ULONG_PTR)L"str" },
        { "%hws", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%hwS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%whs", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%whS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%hwls", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%hwlls", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%hwlS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%hwllS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%lhws", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%llhws", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%lhwS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%llhwS", sizeof(ULONG_PTR), (ULONG_PTR)L"str", "str" },
        { "%c", sizeof(SHORT), 0x95c8 },
        { "%lc", sizeof(SHORT), 0x95c8 },
        { "%llc", sizeof(SHORT), 0x95c8 },
        { "%lllc", sizeof(SHORT), 0x95c8 },
        { "%llllc", sizeof(SHORT), 0x95c8 },
        { "%lllllc", sizeof(SHORT), 0x95c8 },
        { "%C", sizeof(SHORT), 0x95c8 },
        { "%lC", sizeof(SHORT), 0x95c8 },
        { "%llC", sizeof(SHORT), 0x95c8 },
        { "%lllC", sizeof(SHORT), 0x95c8 },
        { "%llllC", sizeof(SHORT), 0x95c8 },
        { "%lllllC", sizeof(SHORT), 0x95c8 },
        { "%hc", sizeof(BYTE), 0x95c8 },
        { "%hhc", sizeof(BYTE), 0x95c8 },
        { "%hhhc", sizeof(BYTE), 0x95c8 },
        { "%wc", sizeof(BYTE), 0x95c8 },
        { "%wC", sizeof(BYTE), 0x95c8 },
        { "%hwc", sizeof(BYTE), 0x95c8 },
        { "%whc", sizeof(BYTE), 0x95c8 },
        { "%hwC", sizeof(BYTE), 0x95c8 },
        { "%whC", sizeof(BYTE), 0x95c8 },
        { "%I64u", sizeof(ULONG64), 10 },
        { "%llI64u", sizeof(ULONG64), 10 },
        { "%I64llu", sizeof(ULONG64), 10 },
        { "%I64s", sizeof(ULONG_PTR), (ULONG_PTR)"str", L"str" },
        { "%q%u", sizeof(ULONG), 10 },
        { "%lhw%u", 0, 10 },
        { "%u% ", sizeof(ULONG), 10 },
        { "%u% %u", sizeof(ULONG), 10 },
        { "%  ll u", 0, 10 },
        { "% llu", sizeof(ULONG64), 10 },
        { "%# llx", sizeof(ULONG64), 10 },
        { "%  #llx", sizeof(ULONG64), 10 },
    };
    int (WINAPIV *ntdll__snprintf)(char *str, size_t len, const char *format, ...);
    int (WINAPIV *ntdll__snwprintf)( WCHAR *str, size_t len, const WCHAR *format, ... );
    WCHAR ws[256], expectedw[256], specw[256];
    unsigned int i, j;
    char expected[256], spec[256], s[256];
    int len_a, len_w = 0, expected_len_a, expected_len_w = 0;
    HANDLE hntdll = GetModuleHandleW(L"ntdll.dll");

    ntdll__snprintf = (void *)GetProcAddress(hntdll, "_snprintf");
    ok(!!ntdll__snprintf, "_snprintf not found.\n");
    ntdll__snwprintf = (void *)GetProcAddress(hntdll, "_snwprintf");
    ok(!!ntdll__snwprintf, "_snwprintf not found.\n");

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        strcpy(spec, tests[i].spec);
        winetest_push_context("%s", spec);
        strcat(spec,"|%s");
        *s = 0;
        *ws = 0;
        j = 0;
        do
            specw[j] = spec[j];
        while (specw[j++]);
        if (tests[i].argw)
        {
            len_w = pwnsprintfW(ws, ARRAY_SIZE(ws), specw, tests[i].argw, L"end");
            expected_len_w = ntdll__snwprintf(expectedw, ARRAY_SIZE(expectedw), specw, tests[i].argw, L"end");
        }
        switch (tests[i].arg_size)
        {
            case 0:
                len_a = pwnsprintfA(s, ARRAY_SIZE(s), spec, "end");
                expected_len_a = ntdll__snprintf(expected, ARRAY_SIZE(expected), spec, "end");
                len_w = pwnsprintfW(ws, ARRAY_SIZE(ws), specw, L"end");
                expected_len_w = ntdll__snwprintf(expectedw, ARRAY_SIZE(expectedw), specw, L"end");
                break;
            case 1:
            case 2:
            case 4:
                len_a = pwnsprintfA(s, ARRAY_SIZE(s), spec, (ULONG)tests[i].arg, "end");
                expected_len_a = ntdll__snprintf(expected, ARRAY_SIZE(expected), spec, (ULONG)tests[i].arg, "end");
                if (!tests[i].argw)
                {
                    len_w = pwnsprintfW(ws, ARRAY_SIZE(ws), specw, (ULONG)tests[i].arg, L"end");
                    expected_len_w = ntdll__snwprintf(expectedw, ARRAY_SIZE(expectedw), specw, (ULONG)tests[i].arg, L"end");
                }
                break;
            case 8:
                len_a = pwnsprintfA(s, ARRAY_SIZE(s), spec, (ULONG64)tests[i].arg, "end");
                expected_len_a = ntdll__snprintf(expected, ARRAY_SIZE(s), spec, (ULONG64)tests[i].arg, "end");
                if (!tests[i].argw)
                {
                    len_w = pwnsprintfW(ws, ARRAY_SIZE(ws), specw, (ULONG64)tests[i].arg, L"end");
                    expected_len_w = ntdll__snwprintf(expectedw, ARRAY_SIZE(expectedw), specw, (ULONG64)tests[i].arg, L"end");
                }
                break;
            default:
                len_a = len_w = expected_len_a = expected_len_w = 0;
                ok(0, "unknown length %u.\n", tests[i].arg_size);
                break;
        }
        ok(len_a == expected_len_a, "got len %d, expected %d.\n", len_a, expected_len_a);
        ok(!strcmp(s, expected), "got %s, expected %s.\n", debugstr_a(s), debugstr_a(expected));
        ok(len_w == expected_len_w, "got len %d, expected %d.\n", len_a, expected_len_a);
        ok(!wcscmp(ws, expectedw), "got %s, expected %s.\n", debugstr_w(ws), debugstr_w(expectedw));
        winetest_pop_context();
    }
}

START_TEST(string)
{
  HMODULE hShlwapi;
  CHAR thousandDelim[8];
  CHAR decimalDelim[8];
  CoInitialize(0);

  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousandDelim, 8);
  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimalDelim, 8);

  hShlwapi = GetModuleHandleA("shlwapi");
  pChrCmpIA = (void *)GetProcAddress(hShlwapi, "ChrCmpIA");
  pChrCmpIW = (void *)GetProcAddress(hShlwapi, "ChrCmpIW");
  pIntlStrEqWorkerA = (void *)GetProcAddress(hShlwapi, "IntlStrEqWorkerA");
  pIntlStrEqWorkerW = (void *)GetProcAddress(hShlwapi, "IntlStrEqWorkerW");
  pSHAnsiToAnsi = (void *)GetProcAddress(hShlwapi, (LPSTR)345);
  pSHUnicodeToUnicode = (void *)GetProcAddress(hShlwapi, (LPSTR)346);
  pStrCatBuffA = (void *)GetProcAddress(hShlwapi, "StrCatBuffA");
  pStrCatBuffW = (void *)GetProcAddress(hShlwapi, "StrCatBuffW");
  pStrCatChainW = (void *)GetProcAddress(hShlwapi, "StrCatChainW");
  pStrCpyNXA = (void *)GetProcAddress(hShlwapi, (LPSTR)399);
  pStrCpyNXW = (void *)GetProcAddress(hShlwapi, (LPSTR)400);
  pStrChrNW = (void *)GetProcAddress(hShlwapi, "StrChrNW");
  pStrFormatByteSize64A = (void *)GetProcAddress(hShlwapi, "StrFormatByteSize64A");
  pStrFormatByteSizeEx = (void *)GetProcAddress(hShlwapi, "StrFormatByteSizeEx");
  pStrFormatKBSizeA = (void *)GetProcAddress(hShlwapi, "StrFormatKBSizeA");
  pStrFormatKBSizeW = (void *)GetProcAddress(hShlwapi, "StrFormatKBSizeW");
  pStrIsIntlEqualA = (void *)GetProcAddress(hShlwapi, "StrIsIntlEqualA");
  pStrIsIntlEqualW = (void *)GetProcAddress(hShlwapi, "StrIsIntlEqualW");
  pStrPBrkW = (void *)GetProcAddress(hShlwapi, "StrPBrkW");
  pStrRChrA = (void *)GetProcAddress(hShlwapi, "StrRChrA");
  pStrRetToBSTR = (void *)GetProcAddress(hShlwapi, "StrRetToBSTR");
  pStrRetToBufA = (void *)GetProcAddress(hShlwapi, "StrRetToBufA");
  pStrRetToBufW = (void *)GetProcAddress(hShlwapi, "StrRetToBufW");
  pStrStrNW = (void *)GetProcAddress(hShlwapi, "StrStrNW");
  pStrStrNIW = (void *)GetProcAddress(hShlwapi, "StrStrNIW");
  pwnsprintfA = (void *)GetProcAddress(hShlwapi, "wnsprintfA");
  pwnsprintfW = (void *)GetProcAddress(hShlwapi, "wnsprintfW");
  pStrToInt64ExA = (void *)GetProcAddress(hShlwapi, "StrToInt64ExA");
  pStrToInt64ExW = (void *)GetProcAddress(hShlwapi, "StrToInt64ExW");

  test_StrChrA();
  test_StrChrW();
  test_StrChrIA();
  test_StrChrIW();
  test_StrRChrA();
  test_StrRChrW();
  test_StrCpyW();
  test_StrChrNW();
  test_StrToIntA();
  test_StrToIntW();
  test_StrToIntExA();
  test_StrToIntExW();
  test_StrToInt64ExA();
  test_StrToInt64ExW();
  test_StrDupA();

  /* language-dependent test */
  if (is_lang_english() && is_locale_english())
  {
    test_StrFormatByteSize64A();
    test_StrFormatByteSizeEx();
    test_StrFormatKBSizeA();
    test_StrFormatKBSizeW();
  }
  else
    skip("An English UI and locale is required for the StrFormat*Size tests\n");
  if (is_lang_english())
    test_StrFromTimeIntervalA();
  else
    skip("An English UI is required for the StrFromTimeInterval tests\n");

  test_StrCmpA();
  test_StrCmpW();
  test_StrRetToBSTR();
  test_StrCpyNXA();
  test_StrCpyNXW();
  test_StrRStrI();
  test_SHAnsiToAnsi();
  test_SHUnicodeToUnicode();
  test_StrXXX_overflows();
  test_StrStrA();
  test_StrStrW();
  test_StrStrIA();
  test_StrStrIW();
  test_StrStrNW();
  test_StrStrNIW();
  test_StrCatChainW();
  test_printf_format();

  CoUninitialize();
}
