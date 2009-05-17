/*
 * Unit tests for locale functions
 *
 * Copyright 2002 YASAR Mehmet
 * Copyright 2003 Dmitry Timoshkov
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
 *
 * NOTES
 *  We must pass LOCALE_NOUSEROVERRIDE (NUO) to formatting functions so that
 *  even when the user has overridden their default i8n settings (e.g. in
 *  the control panel i8n page), we will still get the expected results.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

static inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

static inline int strncmpW( const WCHAR *str1, const WCHAR *str2, int n )
{
    if (n <= 0) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

static inline WCHAR *strchrW( const WCHAR *str, WCHAR ch )
{
    do { if (*str == ch) return (WCHAR *)str; } while (*str++);
    return NULL;
}

static inline int isdigitW( WCHAR wc )
{
    WORD type;
    GetStringTypeW( CT_CTYPE1, &wc, 1, &type );
    return type & C1_DIGIT;
}

/* Some functions are only in later versions of kernel32.dll */
static HMODULE hKernel32;
static WORD enumCount;

typedef BOOL (WINAPI *EnumSystemLanguageGroupsAFn)(LANGUAGEGROUP_ENUMPROC,
                                                   DWORD, LONG_PTR);
static EnumSystemLanguageGroupsAFn pEnumSystemLanguageGroupsA;
typedef BOOL (WINAPI *EnumLanguageGroupLocalesAFn)(LANGGROUPLOCALE_ENUMPROC,
                                                   LGRPID, DWORD, LONG_PTR);
static EnumLanguageGroupLocalesAFn pEnumLanguageGroupLocalesA;
typedef BOOL (WINAPI *EnumUILanguagesAFn)(UILANGUAGE_ENUMPROC,
                                                   DWORD, LONG_PTR);
static EnumUILanguagesAFn pEnumUILanguagesA;

typedef INT (WINAPI *FoldStringAFn)(DWORD, LPCSTR, INT, LPSTR, INT);
static FoldStringAFn pFoldStringA;
typedef INT (WINAPI *FoldStringWFn)(DWORD, LPCWSTR, INT, LPWSTR, INT);
static FoldStringWFn pFoldStringW;

typedef BOOL (WINAPI *IsValidLanguageGroupFn)(LGRPID, DWORD);
static IsValidLanguageGroupFn pIsValidLanguageGroup;

static void InitFunctionPointers(void)
{
  hKernel32 = GetModuleHandleA("kernel32");
  pEnumSystemLanguageGroupsA = (void*)GetProcAddress(hKernel32, "EnumSystemLanguageGroupsA");
  pEnumLanguageGroupLocalesA = (void*)GetProcAddress(hKernel32, "EnumLanguageGroupLocalesA");
  pFoldStringA = (void*)GetProcAddress(hKernel32, "FoldStringA");
  pFoldStringW = (void*)GetProcAddress(hKernel32, "FoldStringW");
  pIsValidLanguageGroup = (void*)GetProcAddress(hKernel32, "IsValidLanguageGroup");
  pEnumUILanguagesA = (void*)GetProcAddress(hKernel32, "EnumUILanguagesA");
}

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type "\n", \
           (label), (received), (expected))

#define BUFFER_SIZE    128
#define COUNTOF(x) (sizeof(x)/sizeof(x)[0])

#define STRINGSA(x,y) strcpy(input, x); strcpy(Expected, y); SetLastError(0xdeadbeef); buffer[0] = '\0'
#define EXPECT_LENA ok(ret == lstrlen(Expected)+1, "Expected Len %d, got %d\n", lstrlen(Expected)+1, ret)
#define EXPECT_EQA ok(strncmp(buffer, Expected, strlen(Expected)) == 0, \
  "Expected '%s', got '%s'\n", Expected, buffer)

#define STRINGSW(x,y) MultiByteToWideChar(CP_ACP,0,x,-1,input,COUNTOF(input)); \
   MultiByteToWideChar(CP_ACP,0,y,-1,Expected,COUNTOF(Expected)); \
   SetLastError(0xdeadbeef); buffer[0] = '\0'
#define EXPECT_LENW ok(ret == lstrlenW(Expected)+1, "Expected Len %d, got %d\n", lstrlenW(Expected)+1, ret)
#define EXPECT_EQW  ok(strncmpW(buffer, Expected, strlenW(Expected)) == 0, "Bad conversion\n")

#define NUO LOCALE_NOUSEROVERRIDE

static void test_GetLocaleInfoA(void)
{
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE];

  ok(lcid == 0x409, "wrong LCID calculated - %d\n", lcid);

  /* HTMLKit and "Font xplorer lite" expect GetLocaleInfoA to
   * partially fill the buffer even if it is too short. See bug 637.
   */
  SetLastError(0); memset(buffer, 0, COUNTOF(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 0);
  ok(ret == 7 && !buffer[0], "Expected len=7, got %d\n", ret);

  SetLastError(0); memset(buffer, 0, COUNTOF(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 3);
  ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
      "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
  ok(!strcmp(buffer, "Mon"), "Expected 'Mon', got '%s'\n", buffer);

  SetLastError(0); memset(buffer, 0, COUNTOF(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 10);
  ok(ret == 7, "Expected ret == 7, got %d, error %d\n", ret, GetLastError());
  ok(!strcmp(buffer, "Monday"), "Expected 'Monday', got '%s'\n", buffer);
}

static void test_GetTimeFormatA(void)
{
  int ret;
  SYSTEMTIME  curtime;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], input[BUFFER_SIZE], Expected[BUFFER_SIZE];

  memset(&curtime, 2, sizeof(SYSTEMTIME));
  STRINGSA("tt HH':'mm'@'ss", ""); /* Invalid time */
  SetLastError(0xdeadbeef);
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  curtime.wHour = 8;
  curtime.wMinute = 56;
  curtime.wSecond = 13;
  curtime.wMilliseconds = 22;
  STRINGSA("tt HH':'mm'@'ss", "AM 08:56@13"); /* Valid time */
  SetLastError(0xdeadbeef);
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  /* MSDN: LOCALE_NOUSEROVERRIDE can't be specified with a format string */
  SetLastError(0xdeadbeef);
  ret = GetTimeFormatA(lcid, NUO|TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());

  STRINGSA("tt HH':'mm'@'ss", "A"); /* Insufficient buffer */
  SetLastError(0xdeadbeef);
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, 2);
  ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
      "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

  STRINGSA("tt HH':'mm'@'ss", "AM 08:56@13"); /* Calculate length only */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, NULL, 0);
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA;

  STRINGSA("", "8 AM"); /* TIME_NOMINUTESORSECONDS, default format */
  ret = GetTimeFormatA(lcid, NUO|TIME_NOMINUTESORSECONDS, &curtime, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("m1s2m3s4", ""); /* TIME_NOMINUTESORSECONDS/complex format */
  ret = GetTimeFormatA(lcid, TIME_NOMINUTESORSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret == strlen(buffer)+1, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  ok( !strcmp( buffer, "" ) || broken( !strcmp( buffer, "4" )), /* win9x */
      "Expected '', got '%s'\n", buffer );

  STRINGSA("", "8:56 AM"); /* TIME_NOSECONDS/Default format */
  ret = GetTimeFormatA(lcid, NUO|TIME_NOSECONDS, &curtime, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s tt", "8:56 AM"); /* TIME_NOSECONDS */
  strcpy(Expected, "8:56 AM");
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h.@:m.@:s.@:tt", "8.@:56AM"); /* Multiple delimiters */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  ok( !strcmp( buffer, "8.@:56AM" ) || broken( !strcmp( buffer, "8.@:56.@:AM" )) /* win9x */,
      "Expected '8.@:56AM', got '%s'\n", buffer );

  STRINGSA("s1s2s3", ""); /* Duplicate tokens */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret == strlen(buffer)+1, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  ok( !strcmp( buffer, "" ) || broken( !strcmp( buffer, "3" )), /* win9x */
      "Expected '', got '%s'\n", buffer );

  STRINGSA("t/tt", "A/AM"); /* AM time marker */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 13;
  STRINGSA("t/tt", "P/PM"); /* PM time marker */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h1t2tt3m", "156"); /* TIME_NOTIMEMARKER: removes text around time marker token */
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s tt", "13:56:13 PM"); /* TIME_FORCE24HOURFORMAT */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s", "13:56:13"); /* TIME_FORCE24HOURFORMAT doesn't add time marker */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 14; /* change this to 14 or 2pm */
  curtime.wMinute = 5;
  curtime.wSecond = 3;
  STRINGSA("h hh H HH m mm s ss t tt", "2 02 14 14 5 05 3 03 P PM"); /* 24 hrs, leading 0 */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 0;
  STRINGSA("h/H/hh/HH", "12/0/12/00"); /* "hh" and "HH" */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s tt", "12:5:3 AM"); /* non-zero flags should fail with format, doesn't */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  /* try to convert formatting strings with more than two letters
   * "h:hh:hhh:H:HH:HHH:m:mm:mmm:M:MM:MMM:s:ss:sss:S:SS:SSS"
   * NOTE: We expect any letter for which there is an upper case value
   *       we should see a replacement.  For letters that DO NOT have
   *       upper case values we should see NO REPLACEMENT.
   */
  curtime.wHour = 8;
  curtime.wMinute = 56;
  curtime.wSecond = 13;
  curtime.wMilliseconds = 22;
  STRINGSA("h:hh:hhh H:HH:HHH m:mm:mmm M:MM:MMM s:ss:sss S:SS:SSS",
           "8:08:08 8:08:08 56:56:56 M:MM:MMM 13:13:13 S:SS:SSS");
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h", "text"); /* Don't write to buffer if len is 0 */
  strcpy(buffer, "text");
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, 0);
  ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
  EXPECT_EQA;

  STRINGSA("h 'h' H 'H' HH 'HH' m 'm' s 's' t 't' tt 'tt'",
           "8 h 8 H 08 HH 56 m 13 s A t AM tt"); /* "'" preserves tokens */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("'''", "'"); /* invalid quoted string */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  /* test that msdn suggested single quotation usage works as expected */
  STRINGSA("''''", "'"); /* single quote mark */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("''HHHHHH", "08"); /* Normal use */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  /* and test for normal use of the single quotation mark */
  STRINGSA("'''HHHHHH'", "'HHHHHH"); /* Normal use */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("'''HHHHHH", "'HHHHHH"); /* Odd use */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("'123'tt", ""); /* TIME_NOTIMEMARKER drops literals too */
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 25;
  STRINGSA("'123'tt", ""); /* Invalid time */
  SetLastError(0xdeadbeef);
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  curtime.wHour = 12;
  curtime.wMonth = 60; /* Invalid */
  STRINGSA("h:m:s", "12:56:13"); /* Invalid date */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;
}

static void test_GetDateFormatA(void)
{
  int ret;
  SYSTEMTIME  curtime;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], input[BUFFER_SIZE], Expected[BUFFER_SIZE];

  memset(&curtime, 2, sizeof(SYSTEMTIME)); /* Invalid time */
  STRINGSA("ddd',' MMM dd yy","");
  SetLastError(0xdeadbeef);
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  curtime.wYear = 2002;
  curtime.wMonth = 5;
  curtime.wDay = 4;
  curtime.wDayOfWeek = 3;
  STRINGSA("ddd',' MMM dd yy","Sat, May 04 02"); /* Simple case */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  /* Same as above but with LOCALE_NOUSEROVERRIDE */
  STRINGSA("ddd',' MMM dd yy",""); /* Simple case */
  SetLastError(0xdeadbeef);
  ret = GetDateFormatA(lcid, NUO, &curtime, input, buffer, COUNTOF(buffer));
  ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
  EXPECT_EQA;

  STRINGSA("ddd',' MMM dd yy","Sat, May 04 02"); /* Format containing "'" */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 36; /* Invalid */
  STRINGSA("ddd',' MMM dd ''''yy","Sat, May 04 '02"); /* Invalid time */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("ddd',' MMM dd ''''yy",""); /* Get size only */
  ret = GetDateFormatA(lcid, 0, &curtime, input, NULL, 0);
  ok(ret == 16, "Expected ret == 16, got %d, error %d\n", ret, GetLastError());
  EXPECT_EQA;

  STRINGSA("ddd',' MMM dd ''''yy",""); /* Buffer too small */
  SetLastError(0xdeadbeef);
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, 2);
  ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
      "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

  STRINGSA("ddd',' MMM dd ''''yy","5/4/2002"); /* Default to DATE_SHORTDATE */
  ret = GetDateFormat(lcid, NUO, &curtime, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  if (strncmp(buffer, Expected, strlen(Expected)) && strncmp(buffer, "5/4/02", strlen(Expected)) != 0)
	  ok (0, "Expected '%s' or '5/4/02', got '%s'\n", Expected, buffer);

  STRINGSA("ddd',' MMM dd ''''yy", "Saturday, May 04, 2002"); /* DATE_LONGDATE */
  ret = GetDateFormat(lcid, NUO|DATE_LONGDATE, &curtime, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  /* test for expected DATE_YEARMONTH behavior with null format */
  /* NT4 returns ERROR_INVALID_FLAGS for DATE_YEARMONTH */
  STRINGSA("ddd',' MMM dd ''''yy", ""); /* DATE_YEARMONTH */
  SetLastError(0xdeadbeef);
  ret = GetDateFormat(lcid, NUO|DATE_YEARMONTH, &curtime, input, buffer, COUNTOF(buffer));
  ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
  EXPECT_EQA;

  /* Test that using invalid DATE_* flags results in the correct error */
  /* and return values */
  STRINGSA("m/d/y", ""); /* Invalid flags */
  SetLastError(0xdeadbeef);
  ret = GetDateFormat(lcid, DATE_YEARMONTH|DATE_SHORTDATE|DATE_LONGDATE,
                      &curtime, input, buffer, COUNTOF(buffer));
  ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
  EXPECT_EQA;
}

static void test_GetDateFormatW(void)
{
  int ret;
  SYSTEMTIME  curtime;
  WCHAR buffer[BUFFER_SIZE], input[BUFFER_SIZE], Expected[BUFFER_SIZE];
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  STRINGSW("",""); /* If flags is not zero then format must be NULL */
  ret = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL,
                       input, buffer, COUNTOF(buffer));
  if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
      return;
  ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
  EXPECT_EQW;

  STRINGSW("",""); /* NULL buffer, len > 0 */
  SetLastError(0xdeadbeef);
  ret = GetDateFormatW (lcid, 0, NULL, input, NULL, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSW("",""); /* NULL buffer, len == 0 */
  ret = GetDateFormatW (lcid, 0, NULL, input, NULL, 0);
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENW; EXPECT_EQW;

  curtime.wYear = 2002;
  curtime.wMonth = 10;
  curtime.wDay = 23;
  curtime.wDayOfWeek = 45612; /* Should be 3 - Wednesday */
  curtime.wHour = 65432; /* Invalid */
  curtime.wMinute = 34512; /* Invalid */
  curtime.wSecond = 65535; /* Invalid */
  curtime.wMilliseconds = 12345;
  STRINGSW("dddd d MMMM yyyy","Wednesday 23 October 2002"); /* Incorrect DOW and time */
  ret = GetDateFormatW (lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENW; EXPECT_EQW;

  /* Limit tests */

  curtime.wYear = 1601;
  curtime.wMonth = 1;
  curtime.wDay = 1;
  curtime.wDayOfWeek = 0; /* Irrelevant */
  curtime.wHour = 0;
  curtime.wMinute = 0;
  curtime.wSecond = 0;
  curtime.wMilliseconds = 0;
  STRINGSW("dddd d MMMM yyyy","Monday 1 January 1601");
  SetLastError(0xdeadbeef);
  ret = GetDateFormatW (lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENW; EXPECT_EQW;

  curtime.wYear = 1600;
  curtime.wMonth = 12;
  curtime.wDay = 31;
  curtime.wDayOfWeek = 0; /* Irrelevant */
  curtime.wHour = 23;
  curtime.wMinute = 59;
  curtime.wSecond = 59;
  curtime.wMilliseconds = 999;
  STRINGSW("dddd d MMMM yyyy","Friday 31 December 1600");
  SetLastError(0xdeadbeef);
  ret = GetDateFormatW (lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
}


#define CY_POS_LEFT  0
#define CY_POS_RIGHT 1
#define CY_POS_LEFT_SPACE  2
#define CY_POS_RIGHT_SPACE 3

static void test_GetCurrencyFormatA(void)
{
  static char szDot[] = { '.', '\0' };
  static char szComma[] = { ',', '\0' };
  static char szDollar[] = { '$', '\0' };
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], input[BUFFER_SIZE];
  CURRENCYFMTA format;

  memset(&format, 0, sizeof(format));

  STRINGSA("23",""); /* NULL output, length > 0 --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, NULL, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("23,53",""); /* Invalid character --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("--",""); /* Double '-' --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("0-",""); /* Trailing '-' --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("0..",""); /* Double '.' --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA(" 0.1",""); /* Leading space --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("1234","$"); /* Length too small --> Write up to length chars */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, 2);
  ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
      "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

  STRINGSA("2353",""); /* Format and flags given --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, NUO, input, &format, buffer, COUNTOF(buffer));
  ok( !ret, "Expected ret == 0, got %d\n", ret);
  ok( GetLastError() == ERROR_INVALID_FLAGS || GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());

  STRINGSA("2353",""); /* Invalid format --> Error */
  SetLastError(0xdeadbeef);
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("2353","$2,353.00"); /* Valid number */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("-2353","($2,353.00)"); /* Valid negative number */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.1","$2,353.10"); /* Valid real number */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.111","$2,353.11"); /* Too many DP --> Truncated */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.119","$2,353.12");  /* Too many DP --> Rounded */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.PositiveOrder = CY_POS_LEFT;
  format.lpDecimalSep = szDot;
  format.lpThousandSep = szComma;
  format.lpCurrencySymbol = szDollar;

  STRINGSA("2353","$2353"); /* No decimal or grouping chars expected */
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 1; /* 1 DP --> Expect decimal separator */
  STRINGSA("2353","$2353.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.Grouping = 2; /* Group by 100's */
  STRINGSA("2353","$23,53.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.LeadingZero = 1; /* Always provide leading zero */
  STRINGSA(".5","$0.5");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.PositiveOrder = CY_POS_RIGHT;
  STRINGSA("1","1.0$");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.PositiveOrder = CY_POS_LEFT_SPACE;
  STRINGSA("1","$ 1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.PositiveOrder = CY_POS_RIGHT_SPACE;
  STRINGSA("1","1.0 $");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 0;
  STRINGSA("-1","($1.0)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 1;
  STRINGSA("-1","-$1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 2;
  STRINGSA("-1","$-1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 3;
  STRINGSA("-1","$1.0-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 4;
  STRINGSA("-1","(1.0$)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 5;
  STRINGSA("-1","-1.0$");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 6;
  STRINGSA("-1","1.0-$");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 7;
  STRINGSA("-1","1.0$-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 8;
  STRINGSA("-1","-1.0 $");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 9;
  STRINGSA("-1","-$ 1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 10;
  STRINGSA("-1","1.0 $-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 11;
  STRINGSA("-1","$ 1.0-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 12;
  STRINGSA("-1","$ -1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 13;
  STRINGSA("-1","1.0- $");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 14;
  STRINGSA("-1","($ 1.0)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 15;
  STRINGSA("-1","(1.0 $)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;
}

#define NEG_PARENS      0 /* "(1.1)" */
#define NEG_LEFT        1 /* "-1.1"  */
#define NEG_LEFT_SPACE  2 /* "- 1.1" */
#define NEG_RIGHT       3 /* "1.1-"  */
#define NEG_RIGHT_SPACE 4 /* "1.1 -" */

static void test_GetNumberFormatA(void)
{
  static char szDot[] = { '.', '\0' };
  static char szComma[] = { ',', '\0' };
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], input[BUFFER_SIZE];
  NUMBERFMTA format;

  memset(&format, 0, sizeof(format));

  STRINGSA("23",""); /* NULL output, length > 0 --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, NULL, NULL, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("23,53",""); /* Invalid character --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("--",""); /* Double '-' --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("0-",""); /* Trailing '-' --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("0..",""); /* Double '.' --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA(" 0.1",""); /* Leading space --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("1234","1"); /* Length too small --> Write up to length chars */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, 2);
  ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
      "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

  STRINGSA("2353",""); /* Format and flags given --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, NUO, input, &format, buffer, COUNTOF(buffer));
  ok( !ret, "Expected ret == 0, got %d\n", ret);
  ok( GetLastError() == ERROR_INVALID_FLAGS || GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());

  STRINGSA("2353",""); /* Invalid format --> Error */
  SetLastError(0xdeadbeef);
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  STRINGSA("2353","2,353.00"); /* Valid number */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("-2353","-2,353.00"); /* Valid negative number */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("-353","-353.00"); /* test for off by one error in grouping */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.1","2,353.10"); /* Valid real number */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.111","2,353.11"); /* Too many DP --> Truncated */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.119","2,353.12");  /* Too many DP --> Rounded */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.lpDecimalSep = szDot;
  format.lpThousandSep = szComma;

  STRINGSA("2353","2353"); /* No decimal or grouping chars expected */
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 1; /* 1 DP --> Expect decimal separator */
  STRINGSA("2353","2353.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.Grouping = 2; /* Group by 100's */
  STRINGSA("2353","23,53.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.LeadingZero = 1; /* Always provide leading zero */
  STRINGSA(".5","0.5");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_PARENS;
  STRINGSA("-1","(1.0)");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_LEFT;
  STRINGSA("-1","-1.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_LEFT_SPACE;
  STRINGSA("-1","- 1.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_RIGHT;
  STRINGSA("-1","1.0-");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_RIGHT_SPACE;
  STRINGSA("-1","1.0 -");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  EXPECT_LENA; EXPECT_EQA;

  lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

  if (IsValidLocale(lcid, 0))
  {
    STRINGSA("-12345","-12 345,00"); /* Try French formatting */
    Expected[3] = 160; /* Non breaking space */
    ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
    ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
    EXPECT_LENA; EXPECT_EQA;
  }
}


static void test_CompareStringA(void)
{
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "Salute", -1);
  ok (ret== 1, "(Salut/Salute) Expected 1, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "SaLuT", -1);
  ok (ret== 2, "(Salut/SaLuT) Expected 2, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "hola", -1);
  ok (ret== 3, "(Salut/hola) Expected 3, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", -1);
  ok (ret== 1, "(haha/hoho) Expected 1, got %d\n", ret);

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", -1);
  ok (ret== 1, "(haha/hoho) Expected 1, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", 0);
  ok (ret== 3, "(haha/hoho) Expected 3, got %d\n", ret);

    ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", 5, "saLuT", -1);
    ok (ret == 2, "(Salut/saLuT) Expected 2, got %d\n", ret);

    /* test for CompareStringA flags */
    SetLastError(0xdeadbeef);
    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0x8, "NULL", -1, "NULL", -1);
    ok(GetLastError() == ERROR_INVALID_FLAGS,
        "unexpected error code %d\n", GetLastError());
    ok(!ret, "CompareStringA must fail with invalid flag\n");

    SetLastError(0xdeadbeef);
    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, LOCALE_USE_CP_ACP, "NULL", -1, "NULL", -1);
    ok(GetLastError() == 0xdeadbeef, "unexpected error code %d\n", GetLastError());
    ok(ret == CSTR_EQUAL, "CompareStringA error: %d != CSTR_EQUAL\n", ret);
    /* end of test for CompareStringA flags */

    ret = lstrcmpA("", "");
    ok (ret == 0, "lstrcmpA(\"\", \"\") should return 0, got %d\n", ret);

    ret = lstrcmpA(NULL, NULL);
    ok (ret == 0 || broken(ret == -2) /* win9x */, "lstrcmpA(NULL, NULL) should return 0, got %d\n", ret);

    ret = lstrcmpA("", NULL);
    ok (ret == 1 || broken(ret == -2) /* win9x */, "lstrcmpA(\"\", NULL) should return 1, got %d\n", ret);

    ret = lstrcmpA(NULL, "");
    ok (ret == -1 || broken(ret == -2) /* win9x */, "lstrcmpA(NULL, \"\") should return -1, got %d\n", ret);
 
    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT,0,"EndDialog",-1,"_Property",-1);
    ok( ret == 3, "EndDialog vs _Property ... expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT,0,"osp_vba.sreg0070",-1,"_IEWWBrowserComp",-1);
    ok( ret == 3, "osp_vba.sreg0070 vs _IEWWBrowserComp ... expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT,0,"r",-1,"\\",-1); 
    ok( ret == 3, "r vs \\ ... expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT,0,"osp_vba.sreg0031", -1, "OriginalDatabase", -1 );
    ok( ret == 3, "osp_vba.sreg0031 vs OriginalDatabase ... expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "AAA", -1, "aaa", -1 );
    ok( ret == 3, "AAA vs aaa expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "AAA", -1, "aab", -1 );
    ok( ret == 1, "AAA vs aab expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "AAA", -1, "Aab", -1 );
    ok( ret == 1, "AAA vs Aab expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, ".AAA", -1, "Aab", -1 );
    ok( ret == 1, ".AAA vs Aab expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, ".AAA", -1, "A.ab", -1 );
    ok( ret == 1, ".AAA vs A.ab expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "aa", -1, "AB", -1 );
    ok( ret == 1, "aa vs AB expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "aa", -1, "Aab", -1 );
    ok( ret == 1, "aa vs Aab expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "aB", -1, "Aab", -1 );
    ok( ret == 3, "aB vs Aab expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "Ba", -1, "bab", -1 );
    ok( ret == 1, "Ba vs bab expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "{100}{83}{71}{71}{71}", -1, "Global_DataAccess_JRO", -1 );
    ok( ret == 1, "{100}{83}{71}{71}{71} vs Global_DataAccess_JRO expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "a", -1, "{", -1 );
    ok( ret == 3, "a vs { expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "A", -1, "{", -1 );
    ok( ret == 3, "A vs { expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "3.5", 0, "4.0", -1 );
    ok(ret == 1, "3.5/0 vs 4.0/-1 expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "3.5", -1, "4.0", -1 );
    ok(ret == 1, "3.5 vs 4.0 expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "3.520.4403.2", -1, "4.0.2927.10", -1 );
    ok(ret == 1, "3.520.4403.2 vs 4.0.2927.10 expected 1, got %d\n", ret);

   /* hyphen and apostrophe are treated differently depending on
    * whether SORT_STRINGSORT specified or not
    */
    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "-o", -1, "/m", -1 );
    ok(ret == 3, "-o vs /m expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "/m", -1, "-o", -1 );
    ok(ret == 1, "/m vs -o expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "-o", -1, "/m", -1 );
    ok(ret == 1, "-o vs /m expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "/m", -1, "-o", -1 );
    ok(ret == 3, "/m vs -o expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "'o", -1, "/m", -1 );
    ok(ret == 3, "'o vs /m expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "/m", -1, "'o", -1 );
    ok(ret == 1, "/m vs 'o expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "'o", -1, "/m", -1 );
    ok(ret == 1, "'o vs /m expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "/m", -1, "'o", -1 );
    ok(ret == 3, "/m vs 'o expected 3, got %d\n", ret);

    if (0) { /* this requires collation table patch to make it MS compatible */
    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "'o", -1, "-o", -1 );
    ok(ret == 1, "'o vs -o expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "'o", -1, "-o", -1 );
    ok(ret == 1, "'o vs -o expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "'", -1, "-", -1 );
    ok(ret == 1, "' vs - expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "'", -1, "-", -1 );
    ok(ret == 1, "' vs - expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "`o", -1, "/m", -1 );
    ok(ret == 3, "`o vs /m expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "/m", -1, "`o", -1 );
    ok(ret == 1, "/m vs `o expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "`o", -1, "/m", -1 );
    ok(ret == 3, "`o vs /m expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "/m", -1, "`o", -1 );
    ok(ret == 1, "/m vs `o expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "`o", -1, "-m", -1 );
    ok(ret == 1, "`o vs -m expected 1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "-m", -1, "`o", -1 );
    ok(ret == 3, "-m vs `o expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "`o", -1, "-m", -1 );
    ok(ret == 3, "`o vs -m expected 3, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "-m", -1, "`o", -1 );
    ok(ret == 1, "-m vs `o expected 1, got %d\n", ret);
    }

    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, "aLuZkUtZ", 8, "aLuZkUtZ", 9);
    ok(ret == 2, "aLuZkUtZ vs aLuZkUtZ\\0 expected 2, got %d\n", ret);

    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, "aLuZkUtZ", 7, "aLuZkUtZ\0A", 10);
    ok(ret == 1, "aLuZkUtZ vs aLuZkUtZ\\0A expected 1, got %d\n", ret);

    /* WinXP handles embedded NULLs differently than earlier versions */
    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, "aLuZkUtZ", 8, "aLuZkUtZ\0A", 10);
    ok(ret == 1 || ret == 2, "aLuZkUtZ vs aLuZkUtZ\\0A expected 1 or 2, got %d\n", ret);

    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, "aLu\0ZkUtZ", 8, "aLu\0ZkUtZ\0A", 10);
    ok(ret == 1 || ret == 2, "aLu\\0ZkUtZ vs aLu\\0ZkUtZ\\0A expected 1 or 2, got %d\n", ret);

    ret = CompareStringA(lcid, 0, "a\0b", -1, "a", -1);
    ok(ret == 2, "a vs a expected 2, got %d\n", ret);

    ret = CompareStringA(lcid, 0, "a\0b", 4, "a", 2);
    ok(ret == CSTR_EQUAL || /* win2k */
       ret == CSTR_GREATER_THAN,
       "a\\0b vs a expected CSTR_EQUAL or CSTR_GREATER_THAN, got %d\n", ret);

    ret = CompareStringA(lcid, 0, "\2", 2, "\1", 2);
    todo_wine ok(ret != 2, "\\2 vs \\1 expected unequal\n");

    ret = CompareStringA(lcid, NORM_IGNORECASE | LOCALE_USE_CP_ACP, "#", -1, ".", -1);
    todo_wine ok(ret == CSTR_LESS_THAN, "\"#\" vs \".\" expected CSTR_LESS_THAN, got %d\n", ret);

    ret = lstrcmpi("#", ".");
    todo_wine ok(ret == -1, "\"#\" vs \".\" expected -1, got %d\n", ret);
}

static void test_LCMapStringA(void)
{
    int ret, ret2;
    char buf[256], buf2[256];
    static const char upper_case[] = "\tJUST! A, TEST; STRING 1/*+-.\r\n";
    static const char lower_case[] = "\tjust! a, test; string 1/*+-.\r\n";
    static const char symbols_stripped[] = "justateststring1";

    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP | LCMAP_LOWERCASE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenA(lower_case) + 1);
    ok(!memcmp(buf, lower_case, ret), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | LCMAP_UPPERCASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_LOWERCASE and LCMAP_UPPERCASE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_HIRAGANA | LCMAP_KATAKANA,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_HIRAGANA and LCMAP_KATAKANA are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH | LCMAP_FULLWIDTH,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_HALFWIDTH | LCMAP_FULLWIDTH are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_TRADITIONAL_CHINESE and LCMAP_SIMPLIFIED_CHINESE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    /* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | SORT_STRINGSORT,
                       upper_case, -1, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS, "expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
    ok(!ret, "SORT_STRINGSORT without LCMAP_SORTKEY must fail\n");

    /* test LCMAP_LOWERCASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(upper_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenA(upper_case) + 1);
    ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test LCMAP_UPPERCASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenA(lower_case) + 1);
    ok(!lstrcmpA(buf, upper_case), "LCMapStringA should return %s, but not %s\n", upper_case, buf);

    /* test buffer overflow */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, 4);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "should return 0 and ERROR_INSUFFICIENT_BUFFER, got %d\n", ret);

    /* LCMAP_UPPERCASE or LCMAP_LOWERCASE should accept src == dst */
    lstrcpyA(buf, lower_case);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       buf, -1, buf, sizeof(buf));
    if (!ret) /* Win9x */
        trace("Ignoring LCMapStringA(LCMAP_UPPERCASE, buf, buf) error on Win9x\n");
    else
    {
        ok(ret == lstrlenA(lower_case) + 1,
           "ret %d, error %d, expected value %d\n",
           ret, GetLastError(), lstrlenA(lower_case) + 1);
        ok(!lstrcmpA(buf, upper_case), "LCMapStringA should return %s, but not %s\n", upper_case, buf);
    }
    lstrcpyA(buf, upper_case);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       buf, -1, buf, sizeof(buf));
    if (!ret) /* Win9x */
        trace("Ignoring LCMapStringA(LCMAP_LOWERCASE, buf, buf) error on Win9x\n");
    else
    {
        ok(ret == lstrlenA(upper_case) + 1,
           "ret %d, error %d, expected value %d\n",
           ret, GetLastError(), lstrlenA(lower_case) + 1);
        ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);
    }

    /* otherwise src == dst should fail */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | LCMAP_UPPERCASE,
                       buf, 10, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win9x */,
       "unexpected error code %d\n", GetLastError());
    ok(!ret, "src == dst without LCMAP_UPPERCASE or LCMAP_LOWERCASE must fail\n");

    /* test whether '\0' is always appended */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ok(buf[ret-1] == 0, "LCMapStringA not null-terminated\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, lstrlenA(upper_case), buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(buf2[ret2-1] == 0, "LCMapStringA not null-terminated\n" );
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORECASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORECASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORENONSPACE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORESYMBOLS */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       symbols_stripped, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test NORM_IGNORENONSPACE */
    lstrcpyA(buf, "foo");
    ret = LCMapStringA(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1, "LCMapStringA should return %d, ret = %d\n",
	lstrlenA(lower_case) + 1, ret);
    ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test NORM_IGNORESYMBOLS */
    lstrcpyA(buf, "foo");
    ret = LCMapStringA(LOCALE_USER_DEFAULT, NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(symbols_stripped) + 1, "LCMapStringA should return %d, ret = %d\n",
	lstrlenA(symbols_stripped) + 1, ret);
    ok(!lstrcmpA(buf, symbols_stripped), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test srclen = 0 */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, 0, upper_case, 0, buf, sizeof(buf));
    ok(!ret, "LCMapStringA should fail with srclen = 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "unexpected error code %d\n", GetLastError());
}

static void test_LCMapStringW(void)
{
    int ret, ret2;
    WCHAR buf[256], buf2[256];
    char *p_buf = (char *)buf, *p_buf2 = (char *)buf2;
    static const WCHAR upper_case[] = {'\t','J','U','S','T','!',' ','A',',',' ','T','E','S','T',';',' ','S','T','R','I','N','G',' ','1','/','*','+','-','.','\r','\n',0};
    static const WCHAR lower_case[] = {'\t','j','u','s','t','!',' ','a',',',' ','t','e','s','t',';',' ','s','t','r','i','n','g',' ','1','/','*','+','-','.','\r','\n',0};
    static const WCHAR symbols_stripped[] = {'j','u','s','t','a','t','e','s','t','s','t','r','i','n','g','1',0};
    static const WCHAR fooW[] = {'f','o','o',0};

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | LCMAP_UPPERCASE,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("LCMapStringW is not implemented\n");
        return;
    }
    ok(!ret, "LCMAP_LOWERCASE and LCMAP_UPPERCASE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_HIRAGANA | LCMAP_KATAKANA,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMAP_HIRAGANA and LCMAP_KATAKANA are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH | LCMAP_FULLWIDTH,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMAP_HALFWIDTH | LCMAP_FULLWIDTH are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMAP_TRADITIONAL_CHINESE and LCMAP_SIMPLIFIED_CHINESE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %d\n", GetLastError());

    /* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
    SetLastError(0xdeadbeef);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | SORT_STRINGSORT,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(GetLastError() == ERROR_INVALID_FLAGS, "expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
    ok(!ret, "SORT_STRINGSORT without LCMAP_SORTKEY must fail\n");

    /* test LCMAP_LOWERCASE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(upper_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenW(upper_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "string compare mismatch\n");

    /* test LCMAP_UPPERCASE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(lower_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, upper_case), "string compare mismatch\n");

    /* test buffer overflow */
    SetLastError(0xdeadbeef);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, 4);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "should return 0 and ERROR_INSUFFICIENT_BUFFER, got %d\n", ret);

    /* LCMAP_UPPERCASE or LCMAP_LOWERCASE should accept src == dst */
    lstrcpyW(buf, lower_case);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       buf, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(lower_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, upper_case), "string compare mismatch\n");

    lstrcpyW(buf, upper_case);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       buf, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(upper_case) + 1,
       "ret %d, error %d, expected value %d\n",
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "string compare mismatch\n");

    /* otherwise src == dst should fail */
    SetLastError(0xdeadbeef);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | LCMAP_UPPERCASE,
                       buf, 10, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win9x */,
       "unexpected error code %d\n", GetLastError());
    ok(!ret, "src == dst without LCMAP_UPPERCASE or LCMAP_LOWERCASE must fail\n");

    /* test whether '\0' is always appended */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, lstrlenW(upper_case), buf2, sizeof(buf2));
    ok(ret, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORECASE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORECASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORENONSPACE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORESYMBOLS */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       symbols_stripped, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test NORM_IGNORENONSPACE */
    lstrcpyW(buf, fooW);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(lower_case) + 1, "LCMapStringW should return %d, ret = %d\n",
	lstrlenW(lower_case) + 1, ret);
    ok(!lstrcmpW(buf, lower_case), "string comparison mismatch\n");

    /* test NORM_IGNORESYMBOLS */
    lstrcpyW(buf, fooW);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(symbols_stripped) + 1, "LCMapStringW should return %d, ret = %d\n",
	lstrlenW(symbols_stripped) + 1, ret);
    ok(!lstrcmpW(buf, symbols_stripped), "string comparison mismatch\n");

    /* test srclen = 0 */
    SetLastError(0xdeadbeef);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, 0, upper_case, 0, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMapStringW should fail with srclen = 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "unexpected error code %d\n", GetLastError());
}

/* this requires collation table patch to make it MS compatible */
static const char * const strings_sorted[] =
{
"'",
"-",
"!",
"\"",
".",
":",
"\\",
"_",
"`",
"{",
"}",
"+",
"0",
"1",
"2",
"3",
"4",
"5",
"6",
"7",
"8",
"9",
"a",
"A",
"b",
"B",
"c",
"C"
};

static const char * const strings[] =
{
"C",
"\"",
"9",
"'",
"}",
"-",
"7",
"+",
"`",
"1",
"a",
"5",
"\\",
"8",
"B",
"3",
"_",
"6",
"{",
"2",
"c",
"4",
"!",
"0",
"A",
":",
"b",
"."
};

static int compare_string1(const void *e1, const void *e2)
{
    const char *s1 = *(const char *const *)e1;
    const char *s2 = *(const char *const *)e2;

    return lstrcmpA(s1, s2);
}

static int compare_string2(const void *e1, const void *e2)
{
    const char *s1 = *(const char *const *)e1;
    const char *s2 = *(const char *const *)e2;

    return CompareStringA(0, 0, s1, -1, s2, -1) - 2;
}

static int compare_string3(const void *e1, const void *e2)
{
    const char *s1 = *(const char *const *)e1;
    const char *s2 = *(const char *const *)e2;
    char key1[256], key2[256];

    LCMapStringA(0, LCMAP_SORTKEY, s1, -1, key1, sizeof(key1));
    LCMapStringA(0, LCMAP_SORTKEY, s2, -1, key2, sizeof(key2));
    return strcmp(key1, key2);
}

static void test_sorting(void)
{
    char buf[256];
    char **str_buf = (char **)buf;
    int i;

    assert(sizeof(buf) >= sizeof(strings));

    /* 1. sort using lstrcmpA */
    memcpy(buf, strings, sizeof(strings));
    qsort(buf, sizeof(strings)/sizeof(strings[0]), sizeof(strings[0]), compare_string1);
    for (i = 0; i < sizeof(strings)/sizeof(strings[0]); i++)
    {
        ok(!strcmp(strings_sorted[i], str_buf[i]),
           "qsort using lstrcmpA failed for element %d\n", i);
    }
    /* 2. sort using CompareStringA */
    memcpy(buf, strings, sizeof(strings));
    qsort(buf, sizeof(strings)/sizeof(strings[0]), sizeof(strings[0]), compare_string2);
    for (i = 0; i < sizeof(strings)/sizeof(strings[0]); i++)
    {
        ok(!strcmp(strings_sorted[i], str_buf[i]),
           "qsort using CompareStringA failed for element %d\n", i);
    }
    /* 3. sort using sort keys */
    memcpy(buf, strings, sizeof(strings));
    qsort(buf, sizeof(strings)/sizeof(strings[0]), sizeof(strings[0]), compare_string3);
    for (i = 0; i < sizeof(strings)/sizeof(strings[0]); i++)
    {
        ok(!strcmp(strings_sorted[i], str_buf[i]),
           "qsort using sort keys failed for element %d\n", i);
    }
}

static void test_FoldStringA(void)
{
  int ret, i;
  char src[256], dst[256];
  static const char digits_src[] = { 0xB9,0xB2,0xB3,'\0'  };
  static const char digits_dst[] = { '1','2','3','\0'  };
  static const char composite_src[] =
  {
    0x8a,0x8e,0x9a,0x9e,0x9f,0xc0,0xc1,0xc2,
    0xc3,0xc4,0xc5,0xc7,0xc8,0xc9,0xca,0xcb,
    0xcc,0xcd,0xce,0xcf,0xd1,0xd2,0xd3,0xd4,
    0xd5,0xd6,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe7,0xe8,
    0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf1,
    0xf2,0xf3,0xf4,0xf5,0xf6,0xf8,0xf9,0xfa,
    0xfb,0xfc,0xfd,0xff,'\0'
  };
  static const char composite_dst[] =
  {
    0x53,0x3f,0x5a,0x3f,0x73,0x3f,0x7a,0x3f,
    0x59,0xa8,0x41,0x60,0x41,0xb4,0x41,0x5e,
    0x41,0x7e,0x41,0xa8,0x41,0xb0,0x43,0xb8,
    0x45,0x60,0x45,0xb4,0x45,0x5e,0x45,0xa8,
    0x49,0x60,0x49,0xb4,0x49,0x5e,0x49,0xa8,
    0x4e,0x7e,0x4f,0x60,0x4f,0xb4,0x4f,0x5e,
    0x4f,0x7e,0x4f,0xa8,0x4f,0x3f,0x55,0x60,
    0x55,0xb4,0x55,0x5e,0x55,0xa8,0x59,0xb4,
    0x61,0x60,0x61,0xb4,0x61,0x5e,0x61,0x7e,
    0x61,0xa8,0x61,0xb0,0x63,0xb8,0x65,0x60,
    0x65,0xb4,0x65,0x5e,0x65,0xa8,0x69,0x60,
    0x69,0xb4,0x69,0x5e,0x69,0xa8,0x6e,0x7e,
    0x6f,0x60,0x6f,0xb4,0x6f,0x5e,0x6f,0x7e,
    0x6f,0xa8,0x6f,0x3f,0x75,0x60,0x75,0xb4,
    0x75,0x5e,0x75,0xa8,0x79,0xb4,0x79,0xa8,'\0'
  };
  static const char ligatures_src[] =
  {
    0x8C,0x9C,0xC6,0xDE,0xDF,0xE6,0xFE,'\0'
  };
  static const char ligatures_dst[] =
  {
    'O','E','o','e','A','E','T','H','s','s','a','e','t','h','\0'
  };

  if (!pFoldStringA)
    return; /* FoldString is present in NT v3.1+, but not 95/98/Me */

  /* these tests are locale specific */
  if (GetACP() != 1252)
  {
      trace("Skipping FoldStringA tests for a not Latin 1 locale\n");
      return;
  }

  /* MAP_FOLDDIGITS */
  SetLastError(0);
  ret = pFoldStringA(MAP_FOLDDIGITS, digits_src, -1, dst, 256);
  if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    return;
  ok(ret == 4, "Expected ret == 4, got %d, error %d\n", ret, GetLastError());
  ok(strcmp(dst, digits_dst) == 0,
     "MAP_FOLDDIGITS: Expected '%s', got '%s'\n", digits_dst, dst);
  for (i = 1; i < 256; i++)
  {
    if (!strchr(digits_src, i))
    {
      src[0] = i;
      src[1] = '\0';
      SetLastError(0);
      ret = pFoldStringA(MAP_FOLDDIGITS, src, -1, dst, 256);
      ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
      ok(dst[0] == src[0],
         "MAP_FOLDDIGITS: Expected '%s', got '%s'\n", src, dst);
    }
  }

  /* MAP_EXPAND_LIGATURES */
  SetLastError(0);
  ret = pFoldStringA(MAP_EXPAND_LIGATURES, ligatures_src, -1, dst, 256);
  /* NT 4.0 doesn't support MAP_EXPAND_LIGATURES */
  if (!(ret == 0 && GetLastError() == ERROR_INVALID_FLAGS)) {
    ok(ret == sizeof(ligatures_dst), "Got %d, error %d\n", ret, GetLastError());
    ok(strcmp(dst, ligatures_dst) == 0,
       "MAP_EXPAND_LIGATURES: Expected '%s', got '%s'\n", ligatures_dst, dst);
    for (i = 1; i < 256; i++)
    {
      if (!strchr(ligatures_src, i))
      {
        src[0] = i;
        src[1] = '\0';
        SetLastError(0);
        ret = pFoldStringA(MAP_EXPAND_LIGATURES, src, -1, dst, 256);
        ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
        ok(dst[0] == src[0],
           "MAP_EXPAND_LIGATURES: Expected '%s', got '%s'\n", src, dst);
      }
    }
  }

  /* MAP_COMPOSITE */
  SetLastError(0);
  ret = pFoldStringA(MAP_COMPOSITE, composite_src, -1, dst, 256);
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());
  todo_wine
  {
    /* Wine gets close, but doesn't produce quite the same result as native */
    ok(ret == 121, "Expected 121, got %d\n", ret);
    ok(strcmp(dst, composite_dst) == 0,
       "MAP_COMPOSITE: Expected '%s', got '%s'\n", composite_dst, dst);
  }

  for (i = 1; i < 256; i++)
  {
    if (!strchr(composite_src, i))
    {
      src[0] = i;
      src[1] = '\0';
      SetLastError(0);
      ret = pFoldStringA(MAP_COMPOSITE, src, -1, dst, 256);
      ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
      ok(dst[0] == src[0],
         "0x%02x, 0x%02x,0x%02x,0x%02x,\n", (unsigned char)src[0],
         (unsigned char)dst[0],(unsigned char)dst[1],(unsigned char)dst[2]);
    }
  }

  /* MAP_FOLDCZONE */
  for (i = 1; i < 256; i++)
  {
    src[0] = i;
    src[1] = '\0';
    SetLastError(0);
    ret = pFoldStringA(MAP_FOLDCZONE, src, -1, dst, 256);
    ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
    ok(src[0] == dst[0],
       "MAP_FOLDCZONE: Expected 0x%02x, got 0x%02x\n",
       (unsigned char)src[0], (unsigned char)dst[0]);
  }

  /* MAP_PRECOMPOSED */
  for (i = 1; i < 256; i++)
  {
    src[0] = i;
    src[1] = '\0';
    SetLastError(0);
    ret = pFoldStringA(MAP_PRECOMPOSED, src, -1, dst, 256);
    ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
    ok(src[0] == dst[0],
       "MAP_PRECOMPOSED: Expected 0x%02x, got 0x%02x\n",
       (unsigned char)src[0], (unsigned char)dst[0]);
  }
}

static void test_FoldStringW(void)
{
  int ret;
  unsigned int i, j, failures;
  WCHAR src[256], dst[256], ch, prev_ch = 1;
  static const DWORD badFlags[] =
  {
    0,
    MAP_PRECOMPOSED|MAP_COMPOSITE,
    MAP_PRECOMPOSED|MAP_EXPAND_LIGATURES,
    MAP_COMPOSITE|MAP_EXPAND_LIGATURES
  };
  /* Ranges of digits 0-9 : Must be sorted! */
  static const WCHAR digitRanges[] =
  {
    0x0030, /* '0'-'9' */
    0x0660, /* Eastern Arabic */
    0x06F0, /* Arabic - Hindu */
    0x0966, /* Devengari */
    0x09E6, /* Bengalii */
    0x0A66, /* Gurmukhi */
    0x0AE6, /* Gujarati */
    0x0B66, /* Oriya */
    0x0BE6, /* Tamil - No 0 */
    0x0C66, /* Telugu */
    0x0CE6, /* Kannada */
    0x0D66, /* Maylayalam */
    0x0E50, /* Thai */
    0x0ED0, /* Laos */
    0x2070, /* Superscript - 1, 2, 3 are out of sequence */
    0x2080, /* Subscript */
    0x245F, /* Circled - 0 is out of sequence */
    0x2473, /* Bracketed */
    0x2487, /* Full stop */
    0x2775, /* Inverted circled - No 0 */
    0x277F, /* Patterned circled - No 0 */
    0x2789, /* Inverted Patterned circled - No 0 */
    0xff10, /* Pliene chasse (?) */
    0xffff  /* Terminator */
  };
  /* Digits which are represented, but out of sequence */
  static const WCHAR outOfSequenceDigits[] =
  {
      0xB9,   /* Superscript 1 */
      0xB2,   /* Superscript 2 */
      0xB3,   /* Superscript 3 */
      0x24EA, /* Circled 0 */
      '\0'    /* Terminator */
  };
  /* Digits in digitRanges for which no representation is available */
  static const WCHAR noDigitAvailable[] =
  {
      0x0BE6, /* No Tamil 0 */
      0x2473, /* No Bracketed 0 */
      0x2487, /* No 0 Full stop */
      0x2775, /* No inverted circled 0 */
      0x277F, /* No patterned circled */
      0x2789, /* No inverted Patterned circled */
      '\0'    /* Terminator */
  };
  /* Compatibility conversion results */
  static const WCHAR compat_F900_FA2F[256+48] =
  {
      0x8c48, 0x66f4, 0x8eca, 0x8cc8, 0x6ed1, 0x4e32, 0x53e5, 0x9f9c,
      0x9f9c, 0x5951, 0x91d1, 0x5587, 0x5948, 0x61f6, 0x7669, 0x7f85,
      0x863f, 0x87ba, 0x88f8, 0x908f, 0x6a02, 0x6d1b, 0x70d9, 0x73de,
      0x843d, 0x916a, 0x99f1, 0x4e82, 0x5375, 0x6b04, 0x721b, 0x862d,
      0x9e1e, 0x5d50, 0x6feb, 0x85cd, 0x8964, 0x62c9, 0x81d8, 0x881f,
      0x5eca, 0x6717, 0x6d6a, 0x72fc, 0x0000, 0x4f86, 0x51b7, 0x52de,
      0x64c4, 0x6ad3, 0x7210, 0x76e7, 0x8001, 0x8606, 0x865c, 0x8def,
      0x9732, 0x9b6f, 0x9dfa, 0x788c, 0x797f, 0x7da0, 0x83c9, 0x9304,
      0x9e7f, 0x8ad6, 0x58df, 0x5f04, 0x7c60, 0x807e, 0x7262, 0x78ca,
      0x8cc2, 0x96f7, 0x58d8, 0x5c62, 0x6a13, 0x6dda, 0x6f0f, 0x7d2f,
      0x7e37, 0x964b, 0x52d2, 0x808b, 0x51dc, 0x51cc, 0x7a1c, 0x7dbe,
      0x83f1, 0x9675, 0x8b80, 0x62cf, 0x6a02, 0x8afe, 0x4e39, 0x5be7,
      0x6012, 0x7387, 0x7570, 0x5317, 0x78fb, 0x4fbf, 0x5fa9, 0x4e0d,
      0x6ccc, 0x6578, 0x7d22, 0x53c3, 0x585e, 0x7701, 0x8449, 0x8aaa,
      0x6bba, 0x8fb0, 0x6c88, 0x62fe, 0x82e5, 0x63a0, 0x7565, 0x4eae,
      0x5169, 0x0000, 0x6881, 0x7ce7, 0x826f, 0x8ad2, 0x91cf, 0x52f5,
      0x5442, 0x5973, 0x5eec, 0x65c5, 0x6ffe, 0x792a, 0x95ad, 0x9a6a,
      0x9e97, 0x9ece, 0x529b, 0x66c6, 0x6b77, 0x8f62, 0x5e74, 0x6190,
      0x6200, 0x649a, 0x6f23, 0x7149, 0x7489, 0x0000, 0x7df4, 0x806f,
      0x8f26, 0x84ee, 0x9023, 0x934a, 0x5217, 0x52a3, 0x54bd, 0x70c8,
      0x88c2, 0x8aaa, 0x5ec9, 0x5ff5, 0x637b, 0x6bae, 0x7c3e, 0x7375,
      0x4ee4, 0x56f9, 0x5be7, 0x5dba, 0x601c, 0x73b2, 0x7469, 0x7f9a,
      0x8046, 0x9234, 0x96f6, 0x9748, 0x9818, 0x4f8b, 0x79ae, 0x91b4,
      0x96b8, 0x60e1, 0x4e86, 0x50da, 0x5bee, 0x5c3f, 0x6599, 0x6a02,
      0x71ce, 0x7642, 0x84fc, 0x907c, 0x9f8d, 0x6688, 0x962e, 0x5289,
      0x677b, 0x67f3, 0x6d41, 0x6e9c, 0x7409, 0x7559, 0x786b, 0x7d10,
      0x985e, 0x516d, 0x622e, 0x9678, 0x502b, 0x5d19, 0x6dea, 0x8f2a,
      0x5f8b, 0x6144, 0x6817, 0x7387, 0x9686, 0x5229, 0x540f, 0x5c65,
      0x6613, 0x674e, 0x68a8, 0x6ce5, 0x7406, 0x75e2, 0x7f79, 0x0000,
      0x88e1, 0x91cc, 0x96e2, 0x533f, 0x6eba, 0x541d, 0x71d0, 0x7498,
      0x85fa, 0x0000, 0x9c57, 0x9e9f, 0x6797, 0x6dcb, 0x81e8, 0x7acb,
      0x7b20, 0x7c92, 0x72c0, 0x7099, 0x8b58, 0x4ec0, 0x8336, 0x523a,
      0x5207, 0x5ea6, 0x62d3, 0x7cd6, 0x5b85, 0x6d1e, 0x66b4, 0x8f3b,
      0x884c, 0x964d, 0x898b, 0x5ed3, 0x0000, 0x0000, 0x0000, 0x0000,
      0x585a, 0x0000, 0x6674, 0x0000, 0x0000, 0x51de, 0x8c6c, 0x76ca,
      0x0000, 0x795e, 0x7965, 0x798f, 0x9756, 0x7cbe, 0x7fbd, 0x0000,
      0x0000, 0x0000, 0x8af8, 0x0000, 0x0000, 0x9038, 0x90fd, 0x0000,
      0x0000, 0x0000, 0x98ef, 0x98fc, 0x9928, 0x9db4, 0x0000, 0x0000
  };
  static const WCHAR compat_FE30_FEF7[200] =
  {
      0x2025, 0x2014, 0x2013, 0x005f, 0x005f, 0x0028, 0x0029, 0x007b,
      0x007d, 0x3014, 0x3015, 0x3010, 0x3011, 0x300a, 0x300b, 0x3008,
      0x3009, 0x300c, 0x300d, 0x300e, 0x300f, 0x0000, 0x0000, 0x0000,
      0x0000, 0x203e, 0x203e, 0x203e, 0x203e, 0x005f, 0x005f, 0x005f,
      0x002c, 0x3001, 0x002e, 0x0000, 0x003b, 0x003a, 0x003f, 0x0021,
      0x2014, 0x0028, 0x0029, 0x007b, 0x007d, 0x3014, 0x3015, 0x0023,
      0x0026, 0x002a, 0x002b, 0x002d, 0x003c, 0x003e, 0x003d, 0x0000,
      0x0000, 0x0024, 0x0025, 0x0040, 0x0000, 0x0000, 0x0000, 0x0000,
      0x064b, 0x064b, 0x064c, 0x0000, 0x064d, 0x0000, 0x064e, 0x064e,
      0x064f, 0x064f, 0x0650, 0x0650, 0x0651, 0x0651, 0x0652, 0x0652,
      0x0621, 0x0622, 0x0622, 0x0623, 0x0623, 0x0624, 0x0624, 0x0625,
      0x0625, 0x0626, 0x0626, 0x0626, 0x0626, 0x0627, 0x0627, 0x0628,
      0x0628, 0x0628, 0x0628, 0x0629, 0x0629, 0x062a, 0x062a, 0x062a,
      0x062a, 0x062b, 0x062b, 0x062b, 0x062b, 0x062c, 0x062c, 0x062c,
      0x062c, 0x062d, 0x062d, 0x062d, 0x062d, 0x062e, 0x062e, 0x062e,
      0x062e, 0x062f, 0x062f, 0x0630, 0x0630, 0x0631, 0x0631, 0x0632,
      0x0632, 0x0633, 0x0633, 0x0633, 0x0633, 0x0634, 0x0634, 0x0634,
      0x0634, 0x0635, 0x0635, 0x0635, 0x0635, 0x0636, 0x0636, 0x0636,
      0x0636, 0x0637, 0x0637, 0x0637, 0x0637, 0x0638, 0x0638, 0x0638,
      0x0638, 0x0639, 0x0639, 0x0639, 0x0639, 0x063a, 0x063a, 0x063a,
      0x063a, 0x0641, 0x0641, 0x0641, 0x0641, 0x0642, 0x0642, 0x0642,
      0x0642, 0x0643, 0x0643, 0x0643, 0x0643, 0x0644, 0x0644, 0x0644,
      0x0644, 0x0645, 0x0645, 0x0645, 0x0645, 0x0646, 0x0646, 0x0646,
      0x0646, 0x0647, 0x0647, 0x0647, 0x0647, 0x0648, 0x0648, 0x0649,
      0x0649, 0x064a, 0x064a, 0x064a, 0x064a, 0x0000, 0x0000, 0x0000
  };
  static const WCHAR compat_FF00_FFEF[240] =
  {
      0x0000, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
      0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
      0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
      0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
      0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
      0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
      0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
      0x0058, 0x0059, 0x005a, 0x005b, 0x0000, 0x005d, 0x005e, 0x005f,
      0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
      0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
      0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
      0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000,
      0x0000, 0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,
      0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7, 0x30c3,
      0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa, 0x30ab, 0x30ad,
      0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd,
      0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca, 0x30cb, 0x30cc,
      0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de,
      0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9,
      0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,
      0x3164, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137,
      0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f,
      0x3140, 0x3141, 0x3142, 0x3143, 0x3144, 0x3145, 0x3146, 0x3147,
      0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x0000,
      0x0000, 0x0000, 0x314f, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154,
      0x0000, 0x0000, 0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a,
      0x0000, 0x0000, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,
      0x0000, 0x0000, 0x3161, 0x3162, 0x3163, 0x0000, 0x0000, 0x0000,
      0x00a2, 0x00a3, 0x00ac, 0x00af, 0x00a6, 0x00a5, 0x20a9, 0x0000,
      0x2502, 0x2190, 0x2191, 0x2192, 0x2193, 0x25a0, 0x25cb, 0x0000
  };
  static const WCHAR ligatures_src[] =
  {
    0x00c6, 0x00de, 0x00df, 0x00e6, 0x00fe, 0x0132, 0x0133, 0x0152,
    0x0153, 0x01c4, 0x01c5, 0x01c6, 0x01c7, 0x01c8, 0x01c9, 0x01ca,
    0x01cb, 0x01cc, 0x01e2, 0x01e3, 0x01f1, 0x01f2, 0x01f3, 0x01fc,
    0x01fd, 0x05f0, 0x05f1, 0x05f2, 0xfb00, 0xfb01, 0xfb02, 0xfb03,
    0xfb04, 0xfb05, 0xfb06, '\0'
  };
  static const WCHAR ligatures_dst[] =
  {
    'A','E','T','H','s','s','a','e','t','h','I','J','i','j','O','E','o','e',
    'D',0x017d,'D',0x017e,'d',0x017e,'L','J','L','j','l','j','N','J','N','j',
    'n','j',0x0100,0x0112,0x0101,0x0113,'D','Z','D','z','d','z',0x00c1,0x00c9,
    0x00e1,0x00e9,0x05d5,0x05d5,0x05d5,0x05d9,0x05d9,0x05d9,'f','f','f','i',
    'f','l','f','f','i','f','f','l',0x017f,'t','s','t','\0'
  };

  if (!pFoldStringW)
  {
    win_skip("FoldStringW is not available\n");
    return; /* FoldString is present in NT v3.1+, but not 95/98/Me */
  }

  /* Invalid flag combinations */
  for (i = 0; i < sizeof(badFlags)/sizeof(badFlags[0]); i++)
  {
    src[0] = dst[0] = '\0';
    SetLastError(0);
    ret = pFoldStringW(badFlags[i], src, 256, dst, 256);
    if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
      win_skip("FoldStringW is not implemented\n");
      return;
    }
    ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
       "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
  }

  /* src & dst cannot be the same */
  SetLastError(0);
  ret = pFoldStringW(MAP_FOLDCZONE, src, -1, src, 256);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* src can't be NULL */
  SetLastError(0);
  ret = pFoldStringW(MAP_FOLDCZONE, NULL, -1, dst, 256);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* srclen can't be 0 */
  SetLastError(0);
  ret = pFoldStringW(MAP_FOLDCZONE, src, 0, dst, 256);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* dstlen can't be < 0 */
  SetLastError(0);
  ret = pFoldStringW(MAP_FOLDCZONE, src, -1, dst, -1);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* Ret includes terminating NUL which is appended if srclen = -1 */
  SetLastError(0);
  src[0] = 'A';
  src[1] = '\0';
  dst[0] = '\0';
  ret = pFoldStringW(MAP_FOLDCZONE, src, -1, dst, 256);
  ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
  ok(dst[0] == 'A' && dst[1] == '\0',
     "srclen=-1: Expected ret=2 [%d,%d], got ret=%d [%d,%d], err=%d\n",
     'A', '\0', ret, dst[0], dst[1], GetLastError());

  /* If size is given, result is not NUL terminated */
  SetLastError(0);
  src[0] = 'A';
  src[1] = 'A';
  dst[0] = 'X';
  dst[1] = 'X';
  ret = pFoldStringW(MAP_FOLDCZONE, src, 1, dst, 256);
  ok(ret == 1, "Expected ret == 1, got %d, error %d\n", ret, GetLastError());
  ok(dst[0] == 'A' && dst[1] == 'X',
     "srclen=1: Expected ret=1, [%d,%d], got ret=%d,[%d,%d], err=%d\n",
     'A','X', ret, dst[0], dst[1], GetLastError());

  /* MAP_FOLDDIGITS */
  for (j = 0; j < sizeof(digitRanges)/sizeof(digitRanges[0]); j++)
  {
    /* Check everything before this range */
    for (ch = prev_ch; ch < digitRanges[j]; ch++)
    {
      SetLastError(0);
      src[0] = ch;
      src[1] = dst[0] = '\0';
      ret = pFoldStringW(MAP_FOLDDIGITS, src, -1, dst, 256);
      ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());

      ok(dst[0] == ch || strchrW(outOfSequenceDigits, ch) ||
         /* Wine (correctly) maps all Unicode 4.0+ digits */
         isdigitW(ch) || (ch >= 0x24F5 && ch <= 0x24FD) || ch == 0x24FF ||
         (ch >= 0x1369 && ch <= 0x1371),
         "MAP_FOLDDIGITS: ch %d 0x%04x Expected unchanged got %d\n", ch, ch, dst[0]);
    }

    if (digitRanges[j] == 0xffff)
      break; /* Finished the whole code point space */

    for (ch = digitRanges[j]; ch < digitRanges[j] + 10; ch++)
    {
      WCHAR c;

      /* Map out of sequence characters */
      if      (ch == 0x2071) c = 0x00B9; /* Superscript 1 */
      else if (ch == 0x2072) c = 0x00B2; /* Superscript 2 */
      else if (ch == 0x2073) c = 0x00B3; /* Superscript 3 */
      else if (ch == 0x245F) c = 0x24EA; /* Circled 0     */
      else                   c = ch;
      SetLastError(0);
      src[0] = c;
      src[1] = dst[0] = '\0';
      ret = pFoldStringW(MAP_FOLDDIGITS, src, -1, dst, 256);
      ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());

      ok((dst[0] == '0' + ch - digitRanges[j] && dst[1] == '\0') ||
         broken( dst[0] == ch ) ||  /* old Windows versions don't have all mappings */
         strchrW(noDigitAvailable, c),
         "MAP_FOLDDIGITS: ch %d Expected %d got %d\n",
         ch, '0' + digitRanges[j] - ch, dst[0]);
    }
    prev_ch = ch;
  }

  /* MAP_FOLDCZONE */
  for (ch = 1, failures = 0; ch <0xffff; ch++)
  {
    WCHAR expected = 0;

    if (ch >= 0xF900 && ch <= 0xFA2F)
      expected = compat_F900_FA2F[ch - 0xF900];
    else if (ch >= 0xFE30 && ch <= 0xFEF7)
      expected = compat_FE30_FEF7[ch - 0xFE30];
    else if (ch >= 0xFF00 && ch <= 0xFFEF)
      expected = compat_FF00_FFEF[ch - 0xFF00];

    if (!expected)
      expected = ch;

    SetLastError(0);
    src[0] = ch;
    src[1] = dst[0] = '\0';
    ret = pFoldStringW(MAP_FOLDCZONE, src, -1, dst, 256);
    ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
    ok(dst[0] == expected ||
       broken( dst[0] == ch ) ||  /* old Windows versions don't have all mappings */
       /* Wine (correctly) uses updated mappings for some Unicode 4.0 chars */
       /* FIXME: But they should be re-checked */
       ch == 0xf92c || ch == 0xf979 || ch == 0xf995 || ch == 0xf9e7 ||
       ch == 0xf9f1 ||
       (0xfa0c <= ch && ch <= 0xfa6a) ||
       (0xfa70 <= ch && ch <= 0xfad9) ||
       ch == 0xfe47 || ch == 0xfe48 || ch == 0xfe68 ||
       (0xfe70 <= ch && ch <= 0xfe7f) ||
       ch == 0xff3c || ch == 0xff5f || ch == 0xff60 ||
       ch == 0xff9e || ch == 0xff9f,
       "MAP_FOLDCZONE: ch %d 0x%04x Expected 0x%04x got 0x%04x\n",
       ch, ch, expected, dst[0]);
    if (dst[0] != expected && ch < 0xf000 && ++failures > 50)
    {
        trace( "MAP_FOLDCZONE: Too many failures, giving up\n" );
        break;
    }
  }

  /* MAP_EXPAND_LIGATURES */
  SetLastError(0);
  ret = pFoldStringW(MAP_EXPAND_LIGATURES, ligatures_src, -1, dst, 256);
  /* NT 4.0 doesn't support MAP_EXPAND_LIGATURES */
  if (!(ret == 0 && GetLastError() == ERROR_INVALID_FLAGS)) {
    ok(ret == sizeof(ligatures_dst)/sizeof(ligatures_dst[0]),
       "Got %d, error %d\n", ret, GetLastError());
    ok(!memcmp(dst, ligatures_dst, sizeof(ligatures_dst)),
       "MAP_EXPAND_LIGATURES: Expanded incorrectly\n");
    for (i = 1, failures = 0; i <= 0xffff; i++)
    {
      if (!strchrW(ligatures_src, i))
      {
        src[0] = i;
        src[1] = '\0';
        SetLastError(0);
        ret = pFoldStringW(MAP_EXPAND_LIGATURES, src, -1, dst, 256);
        ok(ret == 2, "Expected ret == 2, got %d, error %d\n", ret, GetLastError());
        if (ret == 3)
            ok(0, "MAP_EXPAND_LIGATURES: %04x : Expected %04x, got %04x %04x\n",
               i, src[0], dst[0], dst[1]);
        else
            ok(dst[0] == src[0],
               "MAP_EXPAND_LIGATURES: %04x : Expected %04x, got %04x\n",
               i, src[0], dst[0]);
        if (dst[0] != src[0] && ++failures > 50)
        {
            trace( "MAP_EXPAND_LIGATURES: Too many failures, giving up\n" );
            break;
        }
      }
    }
  }

  /* FIXME: MAP_PRECOMPOSED : MAP_COMPOSITE */
}



#define LCID_OK(l) \
  ok(lcid == l, "Expected lcid = %08x, got %08x\n", l, lcid)
#define MKLCID(x,y,z) MAKELCID(MAKELANGID(x, y), z)
#define LCID_RES(src, res) lcid = ConvertDefaultLocale(src); LCID_OK(res)
#define TEST_LCIDLANG(a,b) LCID_RES(MAKELCID(a,b), MAKELCID(a,b))
#define TEST_LCID(a,b,c) LCID_RES(MKLCID(a,b,c), MKLCID(a,b,c))

static void test_ConvertDefaultLocale(void)
{
  LCID lcid;

  /* Doesn't change lcid, even if non default sublang/sort used */
  TEST_LCID(LANG_ENGLISH,  SUBLANG_ENGLISH_US, SORT_DEFAULT);
  TEST_LCID(LANG_ENGLISH,  SUBLANG_ENGLISH_UK, SORT_DEFAULT);
  TEST_LCID(LANG_JAPANESE, SUBLANG_DEFAULT,    SORT_DEFAULT);
  TEST_LCID(LANG_JAPANESE, SUBLANG_DEFAULT,    SORT_JAPANESE_UNICODE);

  /* SUBLANG_NEUTRAL -> SUBLANG_DEFAULT */
  LCID_RES(MKLCID(LANG_ENGLISH,  SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_ENGLISH,  SUBLANG_DEFAULT, SORT_DEFAULT));
  LCID_RES(MKLCID(LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_JAPANESE, SUBLANG_DEFAULT, SORT_DEFAULT));
  LCID_RES(MKLCID(LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE),
           MKLCID(LANG_JAPANESE, SUBLANG_DEFAULT, SORT_JAPANESE_UNICODE));

  /* Invariant language is not treated specially */
  TEST_LCID(LANG_INVARIANT, SUBLANG_DEFAULT, SORT_DEFAULT);
  LCID_RES(MKLCID(LANG_INVARIANT, SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_INVARIANT, SUBLANG_DEFAULT, SORT_DEFAULT));

  /* User/system default languages alone are not mapped */
  TEST_LCIDLANG(LANG_SYSTEM_DEFAULT, SORT_JAPANESE_UNICODE);
  TEST_LCIDLANG(LANG_USER_DEFAULT,   SORT_JAPANESE_UNICODE);

  /* Default lcids */
  LCID_RES(LOCALE_SYSTEM_DEFAULT, GetSystemDefaultLCID());
  LCID_RES(LOCALE_USER_DEFAULT,   GetUserDefaultLCID());
  LCID_RES(LOCALE_NEUTRAL,        GetUserDefaultLCID());
}

static BOOL CALLBACK langgrp_procA(LGRPID lgrpid, LPSTR lpszNum, LPSTR lpszName,
                                    DWORD dwFlags, LONG_PTR lParam)
{
  trace("%08x, %s, %s, %08x, %08lx\n",
        lgrpid, lpszNum, lpszName, dwFlags, lParam);

  ok(pIsValidLanguageGroup(lgrpid, dwFlags) == TRUE,
     "Enumerated grp %d not valid (flags %d)\n", lgrpid, dwFlags);

  /* If lParam is one, we are calling with flags defaulted from 0 */
  ok(!lParam || (dwFlags == LGRPID_INSTALLED || dwFlags == LGRPID_SUPPORTED),
         "Expected dwFlags == LGRPID_INSTALLED || dwFlags == LGRPID_SUPPORTED, got %d\n", dwFlags);

  return TRUE;
}

static void test_EnumSystemLanguageGroupsA(void)
{
  BOOL ret;

  if (!pEnumSystemLanguageGroupsA || !pIsValidLanguageGroup)
  {
    win_skip("EnumSystemLanguageGroupsA and/or IsValidLanguageGroup are not available\n");
    return;
  }

  /* No enumeration proc */
  SetLastError(0);
  ret = pEnumSystemLanguageGroupsA(0, LGRPID_INSTALLED, 0);
  if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("EnumSystemLanguageGroupsA is not implemented\n");
    return;
  }
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* Invalid flags */
  SetLastError(0);
  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_INSTALLED|LGRPID_SUPPORTED, 0);
  ok(GetLastError() == ERROR_INVALID_FLAGS, "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());

  /* No flags - defaults to LGRPID_INSTALLED */
  SetLastError(0xdeadbeef);
  pEnumSystemLanguageGroupsA(langgrp_procA, 0, 1);
  ok(GetLastError() == 0xdeadbeef, "got error %d\n", GetLastError());

  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_INSTALLED, 0);
  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_SUPPORTED, 0);
}


static BOOL CALLBACK lgrplocale_procA(LGRPID lgrpid, LCID lcid, LPSTR lpszNum,
                                      LONG_PTR lParam)
{
  trace("%08x, %08x, %s, %08lx\n", lgrpid, lcid, lpszNum, lParam);

  /* invalid locale enumerated on some platforms */
  if (lcid == 0)
      return TRUE;

  ok(pIsValidLanguageGroup(lgrpid, LGRPID_SUPPORTED) == TRUE,
     "Enumerated grp %d not valid\n", lgrpid);
  ok(IsValidLocale(lcid, LCID_SUPPORTED) == TRUE,
     "Enumerated grp locale %d not valid\n", lcid);
  return TRUE;
}

static void test_EnumLanguageGroupLocalesA(void)
{
  BOOL ret;

  if (!pEnumLanguageGroupLocalesA || !pIsValidLanguageGroup)
  {
    win_skip("EnumLanguageGroupLocalesA and/or IsValidLanguageGroup are not available\n");
    return;
  }

  /* No enumeration proc */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(0, LGRPID_WESTERN_EUROPE, 0, 0);
  if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("EnumLanguageGroupLocalesA is not implemented\n");
    return;
  }
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* lgrpid too small */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(lgrplocale_procA, 0, 0, 0);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* lgrpid too big */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(lgrplocale_procA, LGRPID_ARMENIAN + 1, 0, 0);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* dwFlags is reserved */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(0, LGRPID_WESTERN_EUROPE, 0x1, 0);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  pEnumLanguageGroupLocalesA(lgrplocale_procA, LGRPID_WESTERN_EUROPE, 0, 0);
}

static void test_SetLocaleInfoA(void)
{
  BOOL bRet;
  LCID lcid = GetUserDefaultLCID();

  /* Null data */
  SetLastError(0);
  bRet = SetLocaleInfoA(lcid, LOCALE_SDATE, 0);
  ok( !bRet && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  /* IDATE */
  SetLastError(0);
  bRet = SetLocaleInfoA(lcid, LOCALE_IDATE, (LPSTR)test_SetLocaleInfoA);
  ok(!bRet && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());

  /* ILDATE */
  SetLastError(0);
  bRet = SetLocaleInfoA(lcid, LOCALE_ILDATE, (LPSTR)test_SetLocaleInfoA);
  ok(!bRet && GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
}

static BOOL CALLBACK luilocale_proc1A(LPSTR value, LONG_PTR lParam)
{
  trace("%s %08lx\n", value, lParam);
  return(TRUE);
}

static BOOL CALLBACK luilocale_proc2A(LPSTR value, LONG_PTR lParam)
{
  ok(!enumCount, "callback called again unexpected\n");
  enumCount++;
  return(FALSE);
}

static BOOL CALLBACK luilocale_proc3A(LPSTR value, LONG_PTR lParam)
{
  ok(0,"callback called unexpected\n");
  return(FALSE);
}

static void test_EnumUILanguageA(void)
{
  BOOL ret;
  if (!pEnumUILanguagesA) {
    win_skip("EnumUILanguagesA is not available on Win9x or NT4\n");
    return;
  }

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(luilocale_proc1A, 0, 0);
  if (ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("EnumUILanguagesA is not implemented\n");
    return;
  }
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());

  enumCount = 0;
  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(luilocale_proc2A, 0, 0);
  ok(ret, "Expected ret != 0, got %d, error %d\n", ret, GetLastError());

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(NULL, 0, 0);
  ok(!ret, "Expected return value FALSE, got %u\n", ret);
  ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(luilocale_proc3A, 0x5a5a5a5a, 0);
  ok(!ret, "Expected return value FALSE, got %u\n", ret);
  ok(GetLastError() == ERROR_INVALID_FLAGS, "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(NULL, 0x5a5a5a5a, 0);
  ok(!ret, "Expected return value FALSE, got %u\n", ret);
  ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
}

static char date_fmt_buf[1024];

static BOOL CALLBACK enum_datetime_procA(LPSTR fmt)
{
    lstrcatA(date_fmt_buf, fmt);
    lstrcatA(date_fmt_buf, "\n");
    return TRUE;
}

static void test_EnumDateFormatsA(void)
{
    char *p, buf[256];
    BOOL ret;
    LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

    trace("EnumDateFormatsA 0\n");
    date_fmt_buf[0] = 0;
    SetLastError(0xdeadbeef);
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, 0);
    if (!ret && (GetLastError() == ERROR_INVALID_FLAGS))
    {
        win_skip("0 for dwFlags is not supported\n");
    }
    else
    {
        ok(ret, "EnumDateFormatsA(0) error %d\n", GetLastError());
        trace("%s\n", date_fmt_buf);
        /* test the 1st enumerated format */
        if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
        ret = GetLocaleInfoA(lcid, LOCALE_SSHORTDATE, buf, sizeof(buf));
        ok(ret, "GetLocaleInfoA(LOCALE_SSHORTDATE) error %d\n", GetLastError());
        ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
    }

    trace("EnumDateFormatsA LOCALE_USE_CP_ACP\n");
    date_fmt_buf[0] = 0;
    SetLastError(0xdeadbeef);
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, LOCALE_USE_CP_ACP);
    if (!ret && (GetLastError() == ERROR_INVALID_FLAGS))
    {
        win_skip("LOCALE_USE_CP_ACP is not supported\n");
    }
    else
    {
        ok(ret, "EnumDateFormatsA(LOCALE_USE_CP_ACP) error %d\n", GetLastError());
        trace("%s\n", date_fmt_buf);
        /* test the 1st enumerated format */
        if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
        ret = GetLocaleInfoA(lcid, LOCALE_SSHORTDATE, buf, sizeof(buf));
        ok(ret, "GetLocaleInfoA(LOCALE_SSHORTDATE) error %d\n", GetLastError());
        ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
    }

    trace("EnumDateFormatsA DATE_SHORTDATE\n");
    date_fmt_buf[0] = 0;
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, DATE_SHORTDATE);
    ok(ret, "EnumDateFormatsA(DATE_SHORTDATE) error %d\n", GetLastError());
    trace("%s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_SSHORTDATE, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_SSHORTDATE) error %d\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);

    trace("EnumDateFormatsA DATE_LONGDATE\n");
    date_fmt_buf[0] = 0;
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, DATE_LONGDATE);
    ok(ret, "EnumDateFormatsA(DATE_LONGDATE) error %d\n", GetLastError());
    trace("%s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_SLONGDATE, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_SLONGDATE) error %d\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);

    trace("EnumDateFormatsA DATE_YEARMONTH\n");
    date_fmt_buf[0] = 0;
    SetLastError(0xdeadbeef);
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, DATE_YEARMONTH);
    if (!ret && (GetLastError() == ERROR_INVALID_FLAGS))
    {
        skip("DATE_YEARMONTH is only present on W2K and later\n");
        return;
    }
    ok(ret, "EnumDateFormatsA(DATE_YEARMONTH) error %d\n", GetLastError());
    trace("%s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_SYEARMONTH, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_SYEARMONTH) error %d\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf) || broken(!buf[0]) /* win9x */,
       "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
}

static void test_EnumTimeFormatsA(void)
{
    char *p, buf[256];
    BOOL ret;
    LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

    trace("EnumTimeFormatsA 0\n");
    date_fmt_buf[0] = 0;
    ret = EnumTimeFormatsA(enum_datetime_procA, lcid, 0);
    ok(ret, "EnumTimeFormatsA(0) error %d\n", GetLastError());
    trace("%s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_STIMEFORMAT, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_STIMEFORMAT) error %d\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);

    trace("EnumTimeFormatsA LOCALE_USE_CP_ACP\n");
    date_fmt_buf[0] = 0;
    ret = EnumTimeFormatsA(enum_datetime_procA, lcid, LOCALE_USE_CP_ACP);
    ok(ret, "EnumTimeFormatsA(LOCALE_USE_CP_ACP) error %d\n", GetLastError());
    trace("%s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_STIMEFORMAT, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_STIMEFORMAT) error %d\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
}

static void test_GetCPInfo(void)
{
    BOOL ret;
    CPINFO cpinfo;

    SetLastError(0xdeadbeef);
    ret = GetCPInfo(CP_SYMBOL, &cpinfo);
    ok(!ret, "GetCPInfo(CP_SYMBOL) should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetCPInfo(CP_UTF7, &cpinfo);
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        skip("Codepage CP_UTF7 is not installed/available\n");
    }
    else
    {
        ok(ret, "GetCPInfo(CP_UTF7) error %u\n", GetLastError());
        ok(cpinfo.DefaultChar[0] == 0x3f, "expected 0x3f, got 0x%x\n", cpinfo.DefaultChar[0]);
        ok(cpinfo.DefaultChar[1] == 0, "expected 0, got 0x%x\n", cpinfo.DefaultChar[1]);
        ok(cpinfo.LeadByte[0] == 0, "expected 0, got 0x%x\n", cpinfo.LeadByte[0]);
        ok(cpinfo.LeadByte[1] == 0, "expected 0, got 0x%x\n", cpinfo.LeadByte[1]);
        ok(cpinfo.MaxCharSize == 5, "expected 5, got 0x%x\n", cpinfo.MaxCharSize);
    }

    SetLastError(0xdeadbeef);
    ret = GetCPInfo(CP_UTF8, &cpinfo);
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        skip("Codepage CP_UTF8 is not installed/available\n");
    }
    else
    {
        ok(ret, "GetCPInfo(CP_UTF8) error %u\n", GetLastError());
        ok(cpinfo.DefaultChar[0] == 0x3f, "expected 0x3f, got 0x%x\n", cpinfo.DefaultChar[0]);
        ok(cpinfo.DefaultChar[1] == 0, "expected 0, got 0x%x\n", cpinfo.DefaultChar[1]);
        ok(cpinfo.LeadByte[0] == 0, "expected 0, got 0x%x\n", cpinfo.LeadByte[0]);
        ok(cpinfo.LeadByte[1] == 0, "expected 0, got 0x%x\n", cpinfo.LeadByte[1]);
        ok(cpinfo.MaxCharSize == 4 || broken(cpinfo.MaxCharSize == 3) /* win9x */,
           "expected 4, got %u\n", cpinfo.MaxCharSize);
    }
}

START_TEST(locale)
{
  InitFunctionPointers();

  test_EnumTimeFormatsA();
  test_EnumDateFormatsA();
  test_GetLocaleInfoA();
  test_GetTimeFormatA();
  test_GetDateFormatA();
  test_GetDateFormatW();
  test_GetCurrencyFormatA(); /* Also tests the W version */
  test_GetNumberFormatA();   /* Also tests the W version */
  test_CompareStringA();
  test_LCMapStringA();
  test_LCMapStringW();
  test_FoldStringA();
  test_FoldStringW();
  test_ConvertDefaultLocale();
  test_EnumSystemLanguageGroupsA();
  test_EnumLanguageGroupLocalesA();
  test_SetLocaleInfoA();
  test_EnumUILanguageA();
  test_GetCPInfo();
  /* this requires collation table patch to make it MS compatible */
  if (0) test_sorting();
}
