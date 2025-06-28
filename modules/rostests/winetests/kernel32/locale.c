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

#define _CRT_NON_CONFORMING_WCSTOK

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "winreg.h"
#include "wine/test.h"
#ifdef __REACTOS__
static inline const char *debugstr_guid( const struct _GUID *id ) { return wine_dbgstr_guid(id); }
#endif

static const WCHAR upper_case[] = {'\t','J','U','S','T','!',' ','A',',',' ','T','E','S','T',';',' ','S','T','R','I','N','G',' ','1','/','*','+','-','.','\r','\n',0};
static const WCHAR lower_case[] = {'\t','j','u','s','t','!',' ','a',',',' ','t','e','s','t',';',' ','s','t','r','i','n','g',' ','1','/','*','+','-','.','\r','\n',0};
static const WCHAR title_case[] = {'\t','J','u','s','t','!',' ','A',',',' ','T','e','s','t',';',' ','S','t','r','i','n','g',' ','1','/','*','+','-','.','\r','\n',0};
static const WCHAR symbols_stripped[] = {'j','u','s','t','a','t','e','s','t','s','t','r','i','n','g','1',0};
static const WCHAR localeW[] = {'e','n','-','U','S',0};
static const WCHAR fooW[] = {'f','o','o',0};
static const WCHAR emptyW[] = {0};
static const WCHAR invalidW[] = {'i','n','v','a','l','i','d',0};

/* Some functions are only in later versions of kernel32.dll */
static WORD enumCount;

static INT (WINAPI *pGetTimeFormatEx)(LPCWSTR, DWORD, const SYSTEMTIME *, LPCWSTR, LPWSTR, INT);
static INT (WINAPI *pGetDateFormatEx)(LPCWSTR, DWORD, const SYSTEMTIME *, LPCWSTR, LPWSTR, INT, LPCWSTR);
static BOOL (WINAPI *pEnumSystemLanguageGroupsA)(LANGUAGEGROUP_ENUMPROCA, DWORD, LONG_PTR);
static BOOL (WINAPI *pEnumLanguageGroupLocalesA)(LANGGROUPLOCALE_ENUMPROCA, LGRPID, DWORD, LONG_PTR);
static BOOL (WINAPI *pEnumUILanguagesA)(UILANGUAGE_ENUMPROCA, DWORD, LONG_PTR);
static BOOL (WINAPI *pEnumSystemLocalesA)(LOCALE_ENUMPROCA, DWORD);
static BOOL (WINAPI *pEnumSystemLocalesW)(LOCALE_ENUMPROCW, DWORD);
static BOOL (WINAPI *pEnumSystemLocalesEx)(LOCALE_ENUMPROCEX, DWORD, LPARAM, LPVOID);
static INT (WINAPI *pLCMapStringEx)(LPCWSTR, DWORD, LPCWSTR, INT, LPWSTR, INT, LPNLSVERSIONINFO, LPVOID, LPARAM);
static LCID (WINAPI *pLocaleNameToLCID)(LPCWSTR, DWORD);
static NTSTATUS (WINAPI *pRtlLocaleNameToLcid)(LPCWSTR, LCID *, DWORD);
static BOOLEAN (WINAPI *pRtlIsValidLocaleName)(const WCHAR *,ULONG);
static NTSTATUS (WINAPI *pRtlLcidToLocaleName)(LCID, UNICODE_STRING *,DWORD,BYTE);
static INT  (WINAPI *pLCIDToLocaleName)(LCID, LPWSTR, INT, DWORD);
static BOOL (WINAPI *pIsValidLanguageGroup)(LGRPID, DWORD);
static INT (WINAPI *pIdnToNameprepUnicode)(DWORD, LPCWSTR, INT, LPWSTR, INT);
static INT (WINAPI *pIdnToAscii)(DWORD, LPCWSTR, INT, LPWSTR, INT);
static INT (WINAPI *pIdnToUnicode)(DWORD, LPCWSTR, INT, LPWSTR, INT);
static INT (WINAPI *pGetLocaleInfoEx)(LPCWSTR, LCTYPE, LPWSTR, INT);
static BOOL (WINAPI *pIsValidLocaleName)(LPCWSTR);
static INT (WINAPI *pResolveLocaleName)(LPCWSTR,LPWSTR,INT);
static INT (WINAPI *pCompareStringOrdinal)(const WCHAR *, INT, const WCHAR *, INT, BOOL);
static INT (WINAPI *pCompareStringEx)(LPCWSTR, DWORD, LPCWSTR, INT, LPCWSTR, INT,
                                      LPNLSVERSIONINFO, LPVOID, LPARAM);
static INT (WINAPI *pGetGeoInfoA)(GEOID, GEOTYPE, LPSTR, INT, LANGID);
static INT (WINAPI *pGetGeoInfoW)(GEOID, GEOTYPE, LPWSTR, INT, LANGID);
static INT (WINAPI *pGetGeoInfoEx)(const WCHAR *, GEOTYPE, PWSTR, INT);
static INT (WINAPI *pGetUserDefaultGeoName)(LPWSTR, int);
static BOOL (WINAPI *pSetUserGeoName)(PWSTR);
static BOOL (WINAPI *pEnumSystemGeoID)(GEOCLASS, GEOID, GEO_ENUMPROC);
static BOOL (WINAPI *pGetSystemPreferredUILanguages)(DWORD, ULONG*, WCHAR*, ULONG*);
static BOOL (WINAPI *pGetThreadPreferredUILanguages)(DWORD, ULONG*, WCHAR*, ULONG*);
static BOOL (WINAPI *pGetUserPreferredUILanguages)(DWORD, ULONG*, WCHAR*, ULONG*);
static WCHAR (WINAPI *pRtlUpcaseUnicodeChar)(WCHAR);
static INT (WINAPI *pGetNumberFormatEx)(LPCWSTR, DWORD, LPCWSTR, const NUMBERFMTW *, LPWSTR, int);
static INT (WINAPI *pFindNLSStringEx)(LPCWSTR, DWORD, LPCWSTR, INT, LPCWSTR, INT, LPINT, LPNLSVERSIONINFO, LPVOID, LPARAM);
static void * (WINAPI *pNlsValidateLocale)(LCID*,ULONG);
static LANGID (WINAPI *pSetThreadUILanguage)(LANGID);
static LANGID (WINAPI *pGetThreadUILanguage)(VOID);
static INT (WINAPI *pNormalizeString)(NORM_FORM, LPCWSTR, INT, LPWSTR, INT);
static INT (WINAPI *pFindStringOrdinal)(DWORD, LPCWSTR lpStringSource, INT, LPCWSTR, INT, BOOL);
static BOOL (WINAPI *pGetNLSVersion)(NLS_FUNCTION,LCID,NLSVERSIONINFO*);
static BOOL (WINAPI *pGetNLSVersionEx)(NLS_FUNCTION,LPCWSTR,NLSVERSIONINFOEX*);
static DWORD (WINAPI *pIsValidNLSVersion)(NLS_FUNCTION,LPCWSTR,NLSVERSIONINFOEX*);
static BOOL (WINAPI *pIsNLSDefinedString)(NLS_FUNCTION,DWORD,NLSVERSIONINFO*,const WCHAR *,int);
static NTSTATUS (WINAPI *pRtlNormalizeString)(ULONG, LPCWSTR, INT, LPWSTR, INT*);
static NTSTATUS (WINAPI *pRtlIsNormalizedString)(ULONG, LPCWSTR, INT, BOOLEAN*);
static NTSTATUS (WINAPI *pNtGetNlsSectionPtr)(ULONG,ULONG,void*,void**,SIZE_T*);
static NTSTATUS (WINAPI *pNtInitializeNlsFiles)(void**,LCID*,LARGE_INTEGER*);
static NTSTATUS (WINAPI *pRtlGetLocaleFileMappingAddress)(void**,LCID*,LARGE_INTEGER*);
static void (WINAPI *pRtlInitCodePageTable)(USHORT*,CPTABLEINFO*);
static NTSTATUS (WINAPI *pRtlCustomCPToUnicodeN)(CPTABLEINFO*,WCHAR*,DWORD,DWORD*,const char*,DWORD);
static NTSTATUS (WINAPI *pRtlGetSystemPreferredUILanguages)(DWORD,ULONG,ULONG*,WCHAR*,ULONG*);
static NTSTATUS (WINAPI *pRtlGetThreadPreferredUILanguages)(DWORD,ULONG*,WCHAR*,ULONG*);
static NTSTATUS (WINAPI *pRtlGetUserPreferredUILanguages)(DWORD,ULONG,ULONG*,WCHAR*,ULONG*);

static void InitFunctionPointers(void)
{
  HMODULE mod = GetModuleHandleA("kernel32");

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
  X(GetTimeFormatEx);
  X(GetDateFormatEx);
  X(EnumSystemLanguageGroupsA);
  X(EnumLanguageGroupLocalesA);
  X(LocaleNameToLCID);
  X(LCIDToLocaleName);
  X(LCMapStringEx);
  X(IsValidLanguageGroup);
  X(EnumUILanguagesA);
  X(EnumSystemLocalesA);
  X(EnumSystemLocalesW);
  X(EnumSystemLocalesEx);
  X(IdnToNameprepUnicode);
  X(IdnToAscii);
  X(IdnToUnicode);
  X(GetLocaleInfoEx);
  X(IsValidLocaleName);
  X(ResolveLocaleName);
  X(CompareStringOrdinal);
  X(CompareStringEx);
  X(GetGeoInfoA);
  X(GetGeoInfoW);
  X(GetGeoInfoEx);
  X(GetUserDefaultGeoName);
  X(SetUserGeoName);
  X(EnumSystemGeoID);
  X(GetSystemPreferredUILanguages);
  X(GetThreadPreferredUILanguages);
  X(GetUserPreferredUILanguages);
  X(GetNumberFormatEx);
  X(FindNLSStringEx);
  X(SetThreadUILanguage);
  X(GetThreadUILanguage);
  X(NormalizeString);
  X(FindStringOrdinal);
  X(GetNLSVersion);
  X(GetNLSVersionEx);
  X(IsValidNLSVersion);
  X(IsNLSDefinedString);

  mod = GetModuleHandleA("kernelbase");
  X(NlsValidateLocale);

  mod = GetModuleHandleA("ntdll");
  X(RtlUpcaseUnicodeChar);
  X(RtlLocaleNameToLcid);
  X(RtlIsValidLocaleName);
  X(RtlLcidToLocaleName);
  X(RtlNormalizeString);
  X(RtlIsNormalizedString);
  X(NtGetNlsSectionPtr);
  X(NtInitializeNlsFiles);
  X(RtlInitCodePageTable);
  X(RtlCustomCPToUnicodeN);
  X(RtlGetLocaleFileMappingAddress);
  X(RtlGetSystemPreferredUILanguages);
  X(RtlGetThreadPreferredUILanguages);
  X(RtlGetUserPreferredUILanguages);
#undef X
}

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type "\n", \
           (label), (received), (expected))

#define BUFFER_SIZE    128

#define expect_str(r,s,e) expect_str_(__LINE__, r, s, e)
static void expect_str_(int line, int ret, const char *str, const char *expected)
{
  if (ret)
  {
    ok_(__FILE__, line)(GetLastError() == 0xdeadbeef, "unexpected gle %lu\n", GetLastError());
    ok_(__FILE__, line)(ret == strlen(expected) + 1, "Expected ret %Id, got %d\n", strlen(expected) + 1, ret);
    if (str)
      ok_(__FILE__, line)(strcmp(str, expected) == 0, "Expected '%s', got '%s'\n", expected, str);
  }
  else
    ok_(__FILE__, line)(0, "expected success, got error %ld\n", GetLastError());
}

#define expect_err(r,s,e) expect_err_(__LINE__, r, s, e, #e)
static void expect_err_(int line, int ret, const char *str, DWORD err, const char* err_name)
{
  ok_(__FILE__, line)(!ret && GetLastError() == err,
      "Expected %s, got %ld and ret=%d\n", err_name, GetLastError(), ret);
  if (str)
    ok_(__FILE__, line)(strcmp(str, "pristine") == 0, "Expected a pristine buffer, got '%s'\n", str);
}

#define expect_wstr(r,s,e) expect_wstr_(__LINE__, r, s, e)
static void expect_wstr_(int line, int ret, const WCHAR *str, const WCHAR *expected)
{
  if (ret)
  {
    ok_(__FILE__, line)(GetLastError() == 0xdeadbeef, "unexpected gle %lu\n", GetLastError());
    ok_(__FILE__, line)(ret == wcslen(expected) + 1, "Expected ret %Id, got %d\n", wcslen(expected) + 1, ret);
    if (str)
        ok_(__FILE__, line)(wcscmp(str, expected) == 0, "Expected %s, got %s\n", wine_dbgstr_w(expected), wine_dbgstr_w(str));
  }
  else
    ok_(__FILE__, line)(0, "expected success, got error %ld\n", GetLastError());
}

#define expect_werr(r,s,e) expect_werr_(__LINE__, r, s, e, #e)
static void expect_werr_(int line, int ret, const WCHAR *str, DWORD err, const char* err_name)
{
  ok_(__FILE__, line)(!ret && GetLastError() == err,
      "Expected %s, got %ld and ret=%d\n", err_name, GetLastError(), ret);
  if (str)
      ok_(__FILE__, line)(wcscmp(str, L"pristine") == 0, "Expected a pristine buffer, got %s\n", wine_dbgstr_w(str));
}

static int get_utf16( const WCHAR *src, unsigned int srclen, unsigned int *ch )
{
    if (IS_HIGH_SURROGATE( src[0] ))
    {
        if (srclen <= 1) return 0;
        if (!IS_LOW_SURROGATE( src[1] )) return 0;
        *ch = 0x10000 + ((src[0] & 0x3ff) << 10) + (src[1] & 0x3ff);
        return 2;
    }
    if (IS_LOW_SURROGATE( src[0] )) return 0;
    *ch = src[0];
    return 1;
}

static int put_utf16( WCHAR *str, unsigned int c )
{
    if (c < 0x10000)
    {
        *str = c;
        return 1;
    }
    c -= 0x10000;
    str[0] = 0xd800 | (c >> 10);
    str[1] = 0xdc00 | (c & 0x3ff);
    return 2;
}

#define NUO LOCALE_NOUSEROVERRIDE

static void test_GetLocaleInfoA(void)
{
  int ret;
  int len;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE];
  char expected[BUFFER_SIZE];
  DWORD val;

  ok(lcid == 0x409, "wrong LCID calculated - %ld\n", lcid);

  ret = GetLocaleInfoA(lcid, LOCALE_ILANGUAGE|LOCALE_RETURN_NUMBER, (char*)&val, sizeof(val));
  ok(ret, "got %d\n", ret);
  ok(val == lcid, "got 0x%08lx\n", val);

  /* en and ar use SUBLANG_NEUTRAL, but GetLocaleInfo assume SUBLANG_DEFAULT
     Same is true for zh on pre-Vista, but on Vista and higher GetLocaleInfo
     assumes SUBLANG_NEUTRAL for zh */
  memset(expected, 0, ARRAY_SIZE(expected));
  len = GetLocaleInfoA(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), LOCALE_SLANGUAGE, expected, ARRAY_SIZE(expected));
  SetLastError(0xdeadbeef);
  memset(buffer, 0, ARRAY_SIZE(buffer));
  ret = GetLocaleInfoA(LANG_ENGLISH, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
  ok((ret == len) && !lstrcmpA(buffer, expected),
      "got %d with '%s' (expected %d with '%s')\n",
      ret, buffer, len, expected);

  memset(expected, 0, ARRAY_SIZE(expected));
  len = GetLocaleInfoA(MAKELANGID(LANG_ARABIC, SUBLANG_DEFAULT), LOCALE_SLANGUAGE, expected, ARRAY_SIZE(expected));
  if (len) {
      SetLastError(0xdeadbeef);
      memset(buffer, 0, ARRAY_SIZE(buffer));
      ret = GetLocaleInfoA(LANG_ARABIC, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
      ok((ret == len) && !lstrcmpA(buffer, expected),
          "got %d with '%s' (expected %d with '%s')\n",
          ret, buffer, len, expected);
  }
  else
      win_skip("LANG_ARABIC not installed\n");

  /* SUBLANG_DEFAULT is required for mlang.dll, but optional for GetLocaleInfo */
  memset(expected, 0, ARRAY_SIZE(expected));
  len = GetLocaleInfoA(MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), LOCALE_SLANGUAGE, expected, ARRAY_SIZE(expected));
  SetLastError(0xdeadbeef);
  memset(buffer, 0, ARRAY_SIZE(buffer));
  ret = GetLocaleInfoA(LANG_GERMAN, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
  ok((ret == len) && !lstrcmpA(buffer, expected),
      "got %d with '%s' (expected %d with '%s')\n",
      ret, buffer, len, expected);

  len = GetLocaleInfoA(GetUserDefaultLCID(), LOCALE_SLANGUAGE, expected, ARRAY_SIZE(expected));
  ret = GetLocaleInfoA(LOCALE_NEUTRAL, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
  ok( (ret == len) && !lstrcmpA(buffer, expected), "got %d with '%s' (expected %d with '%s')\n",
      ret, buffer, len, expected);
  ret = GetLocaleInfoA(LOCALE_CUSTOM_DEFAULT, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
  ok( (ret == len) && !lstrcmpA(buffer, expected), "got %d with '%s' (expected %d with '%s')\n",
      ret, buffer, len, expected);
  ret = GetLocaleInfoA(LOCALE_CUSTOM_UNSPECIFIED, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
  ok( (ret == len && !lstrcmpA(buffer, expected)) || broken(!ret), /* <= win8 */
      "got %d with '%s' (expected %d with '%s')\n", ret, buffer, len, expected);
  len = GetLocaleInfoA(GetUserDefaultUILanguage(), LOCALE_SLANGUAGE, expected, ARRAY_SIZE(expected));
  ret = GetLocaleInfoA(LOCALE_CUSTOM_UI_DEFAULT, LOCALE_SLANGUAGE, buffer, ARRAY_SIZE(buffer));
  if (ret) ok( (ret == len && !lstrcmpA(buffer, expected)),
               "got %d with '%s' (expected %d with '%s')\n", ret, buffer, len, expected);

  /* HTMLKit and "Font xplorer lite" expect GetLocaleInfoA to
   * partially fill the buffer even if it is too short. See bug 637.
   */
  SetLastError(0xdeadbeef);
  memset(buffer, 0, ARRAY_SIZE(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 0);
  ok(ret == 7 && !buffer[0], "Expected len=7, got %d\n", ret);

  SetLastError(0xdeadbeef);
  memset(buffer, 0, ARRAY_SIZE(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 3);
  ok( !ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
      "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
  ok(!strcmp(buffer, "Mon"), "Expected 'Mon', got '%s'\n", buffer);

  SetLastError(0xdeadbeef);
  memset(buffer, 0, ARRAY_SIZE(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 10);
  ok(ret == 7, "Expected ret == 7, got %d, error %ld\n", ret, GetLastError());
  ok(!strcmp(buffer, "Monday"), "Expected 'Monday', got '%s'\n", buffer);
}

struct neutralsublang_name2_t {
    WCHAR name[3];
    WCHAR sname[15];
    LCID lcid;
    LCID lcid_broken;
    WCHAR sname_broken[15];
};

static const struct neutralsublang_name2_t neutralsublang_names2[] = {
    { {'a','r',0}, {'a','r','-','S','A',0},
      MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA), SORT_DEFAULT) },
    { {'a','z',0}, {'a','z','-','L','a','t','n','-','A','Z',0},
      MAKELCID(MAKELANGID(LANG_AZERI, SUBLANG_AZERI_LATIN), SORT_DEFAULT) },
    { {'d','e',0}, {'d','e','-','D','E',0},
      MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT) },
    { {'e','n',0}, {'e','n','-','U','S',0},
      MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT) },
    { {'e','s',0}, {'e','s','-','E','S',0},
      MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT),
      MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH), SORT_DEFAULT) /* vista */,
      {'e','s','-','E','S','_','t','r','a','d','n','l',0} },
    { {'g','a',0}, {'g','a','-','I','E',0},
      MAKELCID(MAKELANGID(LANG_IRISH, SUBLANG_IRISH_IRELAND), SORT_DEFAULT), 0, {0} },
    { {'i','t',0}, {'i','t','-','I','T',0},
      MAKELCID(MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN), SORT_DEFAULT) },
    { {'m','s',0}, {'m','s','-','M','Y',0},
      MAKELCID(MAKELANGID(LANG_MALAY, SUBLANG_MALAY_MALAYSIA), SORT_DEFAULT) },
    { {'n','l',0}, {'n','l','-','N','L',0},
      MAKELCID(MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH), SORT_DEFAULT) },
    { {'p','t',0}, {'p','t','-','B','R',0},
      MAKELCID(MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN), SORT_DEFAULT) },
    { {'s','r',0}, {'h','r','-','H','R',0},
      MAKELCID(MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_CROATIA), SORT_DEFAULT) },
    { {'s','v',0}, {'s','v','-','S','E',0},
      MAKELCID(MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH), SORT_DEFAULT) },
    { {'u','z',0}, {'u','z','-','L','a','t','n','-','U','Z',0},
      MAKELCID(MAKELANGID(LANG_UZBEK, SUBLANG_UZBEK_LATIN), SORT_DEFAULT) },
    { {'z','h',0}, {'z','h','-','C','N',0},
      MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT) },
    { {0} }
};

static void test_GetLocaleInfoW(void)
{
  LCID lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  LCID lcid_ru = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
  LCID lcid_en_neut = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), SORT_DEFAULT);
  WCHAR bufferW[80], buffer2W[80];
  CHAR bufferA[80];
  DWORD val;
  DWORD ret;
  INT i;

  ret = GetLocaleInfoW(lcid_en, LOCALE_SMONTHNAME1, bufferW, ARRAY_SIZE(bufferW));
  if (!ret) {
      win_skip("GetLocaleInfoW() isn't implemented\n");
      return;
  }

  ret = GetLocaleInfoW(lcid_en, LOCALE_ILANGUAGE|LOCALE_RETURN_NUMBER, (WCHAR*)&val, sizeof(val)/sizeof(WCHAR));
  ok(ret, "got %ld\n", ret);
  ok(val == lcid_en, "got 0x%08lx\n", val);

  ret = GetLocaleInfoW(lcid_en_neut, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
  if (ret)
  {
      static const WCHAR slangW[] = {'E','n','g','l','i','s','h',' ','(','U','n','i','t','e','d',' ',
                                                                     'S','t','a','t','e','s',')',0};
      static const WCHAR statesW[] = {'U','n','i','t','e','d',' ','S','t','a','t','e','s',0};
      static const WCHAR enW[] = {'e','n','-','U','S',0};
      const struct neutralsublang_name2_t *ptr = neutralsublang_names2;

      ok(!lstrcmpW(bufferW, enW), "got wrong name %s\n", wine_dbgstr_w(bufferW));

      ret = GetLocaleInfoW(lcid_en_neut, LOCALE_SCOUNTRY, bufferW, ARRAY_SIZE(bufferW));
      ok(ret, "got %ld\n", ret);
      if ((PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_ENGLISH) ||
          (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH))
      {
          skip("Non-English locale\n");
      }
      else
          ok(!lstrcmpW(statesW, bufferW), "got wrong name %s\n", wine_dbgstr_w(bufferW));

      ret = GetLocaleInfoW(lcid_en_neut, LOCALE_SLANGUAGE, bufferW, ARRAY_SIZE(bufferW));
      ok(ret, "got %ld\n", ret);
      if ((PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_ENGLISH) ||
          (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH))
      {
          skip("Non-English locale\n");
      }
      else
          ok(!lstrcmpW(slangW, bufferW), "got wrong name %s\n", wine_dbgstr_w(bufferW));

      while (*ptr->name)
      {
          LANGID langid;
          LCID lcid;

          /* make neutral lcid */
          langid = MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(ptr->lcid)), SUBLANG_NEUTRAL);
          lcid = MAKELCID(langid, SORT_DEFAULT);

          val = 0;
          GetLocaleInfoW(lcid, LOCALE_ILANGUAGE|LOCALE_RETURN_NUMBER, (WCHAR*)&val, sizeof(val)/sizeof(WCHAR));
          ok(val == ptr->lcid || (val && broken(val == ptr->lcid_broken)), "%s: got wrong lcid 0x%04lx, expected 0x%04lx\n",
             wine_dbgstr_w(ptr->name), val, ptr->lcid);

          /* now check LOCALE_SNAME */
          GetLocaleInfoW(lcid, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
          ok(!lstrcmpW(bufferW, ptr->sname) ||
             (*ptr->sname_broken && broken(!lstrcmpW(bufferW, ptr->sname_broken))),
             "%s: got %s\n", wine_dbgstr_w(ptr->name), wine_dbgstr_w(bufferW));
          ptr++;
      }
  }
  else
      win_skip("English neutral locale not supported\n");

  ret = GetLocaleInfoW(lcid_ru, LOCALE_SMONTHNAME1, bufferW, ARRAY_SIZE(bufferW));
  if (!ret) {
      win_skip("LANG_RUSSIAN locale data unavailable\n");
      return;
  }
  ret = GetLocaleInfoW(lcid_ru, LOCALE_SMONTHNAME1|LOCALE_RETURN_GENITIVE_NAMES,
                       bufferW, ARRAY_SIZE(bufferW));
  if (!ret) {
      win_skip("LOCALE_RETURN_GENITIVE_NAMES isn't supported\n");
      return;
  }

  /* LOCALE_RETURN_GENITIVE_NAMES isn't supported for GetLocaleInfoA */
  bufferA[0] = 'a';
  SetLastError(0xdeadbeef);
  ret = GetLocaleInfoA(lcid_ru, LOCALE_SMONTHNAME1|LOCALE_RETURN_GENITIVE_NAMES,
                       bufferA, ARRAY_SIZE(bufferA));
  ok(ret == 0, "LOCALE_RETURN_GENITIVE_NAMES should fail with GetLocaleInfoA\n");
  ok(bufferA[0] == 'a', "Expected buffer to be untouched\n");
  ok(GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %lx\n", GetLastError());

  bufferW[0] = 'a';
  SetLastError(0xdeadbeef);
  ret = GetLocaleInfoW(lcid_ru, LOCALE_RETURN_GENITIVE_NAMES, bufferW, ARRAY_SIZE(bufferW));
  ok(ret == 0,
     "LOCALE_RETURN_GENITIVE_NAMES itself doesn't return anything, got %ld\n", ret);
  ok(bufferW[0] == 'a', "Expected buffer to be untouched\n");
  ok(GetLastError() == ERROR_INVALID_FLAGS,
     "Expected ERROR_INVALID_FLAGS, got %lx\n", GetLastError());

  /* yes, test empty 13 month entry too */
  for (i = 0; i < 12; i++) {
      bufferW[0] = 0;
      ret = GetLocaleInfoW(lcid_ru, (LOCALE_SMONTHNAME1+i)|LOCALE_RETURN_GENITIVE_NAMES,
                           bufferW, ARRAY_SIZE(bufferW));
      ok(ret, "Expected non zero result\n");
      ok(ret == lstrlenW(bufferW)+1, "Expected actual length, got %ld, length %d\n",
                                    ret, lstrlenW(bufferW));
      buffer2W[0] = 0;
      ret = GetLocaleInfoW(lcid_ru, LOCALE_SMONTHNAME1+i, buffer2W, ARRAY_SIZE(buffer2W));
      ok(ret, "Expected non zero result\n");
      ok(ret == lstrlenW(buffer2W)+1, "Expected actual length, got %ld, length %d\n",
                                    ret, lstrlenW(buffer2W));

      ok(lstrcmpW(bufferW, buffer2W) != 0,
           "Expected genitive name to differ, got the same for month %d\n", i+1);

      /* for locale without genitive names nominative returned in both cases */
      bufferW[0] = 0;
      ret = GetLocaleInfoW(lcid_en, (LOCALE_SMONTHNAME1+i)|LOCALE_RETURN_GENITIVE_NAMES,
                           bufferW, ARRAY_SIZE(bufferW));
      ok(ret, "Expected non zero result\n");
      ok(ret == lstrlenW(bufferW)+1, "Expected actual length, got %ld, length %d\n",
                                    ret, lstrlenW(bufferW));
      buffer2W[0] = 0;
      ret = GetLocaleInfoW(lcid_en, LOCALE_SMONTHNAME1+i, buffer2W, ARRAY_SIZE(buffer2W));
      ok(ret, "Expected non zero result\n");
      ok(ret == lstrlenW(buffer2W)+1, "Expected actual length, got %ld, length %d\n",
                                    ret, lstrlenW(buffer2W));

      ok(lstrcmpW(bufferW, buffer2W) == 0,
         "Expected same names, got different for month %d\n", i+1);
  }
}

static void test_GetTimeFormatA(void)
{
  int ret;
  SYSTEMTIME  curtime;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE];

  SetLastError(0xdeadbeef);

  /* Invalid time */
  memset(&curtime, 2, sizeof(SYSTEMTIME));
  strcpy(buffer, "pristine");
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "tt HH':'mm'@'ss", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Valid time */
  curtime.wHour = 8;
  curtime.wMinute = 56;
  curtime.wSecond = 13;
  curtime.wMilliseconds = 22;
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "tt HH':'mm'@'ss", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "AM 08:56@13");

  /* MSDN: LOCALE_NOUSEROVERRIDE can't be specified with a format string */
  strcpy(buffer, "pristine");
  ret = GetTimeFormatA(lcid, NUO|TIME_FORCE24HOURFORMAT, &curtime, "HH", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Insufficient buffer */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, " HH", buffer, 2);
  /* The content of the buffer depends on implementation details:
   * it may be " ristine" or " 0ristine" or unchanged and cannot be relied on.
   */
  expect_err(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  /* Calculate length only */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "tt HH':'mm'@'ss", NULL, 0);
  expect_str(ret, NULL, "AM 08:56@13");

  /* TIME_NOMINUTESORSECONDS, default format */
  ret = GetTimeFormatA(lcid, NUO|TIME_NOMINUTESORSECONDS, &curtime, NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "8 AM");

  /* TIME_NOMINUTESORSECONDS/complex format */
  ret = GetTimeFormatA(lcid, TIME_NOMINUTESORSECONDS, &curtime, "m1s2m3s4", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "");

  /* TIME_NOSECONDS/Default format */
  ret = GetTimeFormatA(lcid, NUO|TIME_NOSECONDS, &curtime, NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "8:56 AM");

  /* TIME_NOSECONDS */
  strcpy(buffer, "pristine"); /* clear previous identical result */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, "h:m:s tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "8:56 AM");

  /* Multiple delimiters */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, "h.@:m.@:s.@:tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "8.@:56AM");

  /* Duplicate tokens */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, "s1s2s3", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "");

  /* AM time marker */
  ret = GetTimeFormatA(lcid, 0, &curtime, "t/tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "A/AM");

  /* PM time marker */
  curtime.wHour = 13;
  ret = GetTimeFormatA(lcid, 0, &curtime, "t/tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "P/PM");

  /* TIME_NOTIMEMARKER: removes text around time marker token */
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, "h1t2tt3m", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "156");

  /* TIME_FORCE24HOURFORMAT */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "h:m:s tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "13:56:13 PM");

  /* TIME_FORCE24HOURFORMAT doesn't add time marker */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "h:m:s", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "13:56:13");

  /* 24 hrs, leading 0 */
  curtime.wHour = 14; /* change this to 14 or 2pm */
  curtime.wMinute = 5;
  curtime.wSecond = 3;
  ret = GetTimeFormatA(lcid, 0, &curtime, "h hh H HH m mm s ss t tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2 02 14 14 5 05 3 03 P PM");

  /* "hh" and "HH" */
  curtime.wHour = 0;
  ret = GetTimeFormatA(lcid, 0, &curtime, "h/H/hh/HH", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "12/0/12/00");

  /* non-zero flags should fail with format, doesn't */
  ret = GetTimeFormatA(lcid, 0, &curtime, "h:m:s tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "12:5:3 AM");

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
  ret = GetTimeFormatA(lcid, 0, &curtime, "h:hh:hhh H:HH:HHH m:mm:mmm M:MM:MMM s:ss:sss S:SS:SSS", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "8:08:08 8:08:08 56:56:56 M:MM:MMM 13:13:13 S:SS:SSS");

  /* Don't write to buffer if len is 0 */
  strcpy(buffer, "pristine");
  ret = GetTimeFormatA(lcid, 0, &curtime, "h:mm", buffer, 0);
  expect_str(ret, NULL, "8:56");
  ok(strcmp(buffer, "pristine") == 0, "Expected a pristine buffer, got '%s'\n", buffer);

  /* "'" preserves tokens */
  ret = GetTimeFormatA(lcid, 0, &curtime, "h 'h' H 'H' HH 'HH' m 'm' s 's' t 't' tt 'tt'", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "8 h 8 H 08 HH 56 m 13 s A t AM tt");

  /* invalid quoted string */
  ret = GetTimeFormatA(lcid, 0, &curtime, "'''", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "'");

  /* check that MSDN's suggested single quotation usage works as expected */
  strcpy(buffer, "pristine"); /* clear previous identical result */
  ret = GetTimeFormatA(lcid, 0, &curtime, "''''", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "'");

  /* Normal use */
  ret = GetTimeFormatA(lcid, 0, &curtime, "''HHHHHH", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "08");

  /* and test for normal use of the single quotation mark */
  ret = GetTimeFormatA(lcid, 0, &curtime, "'''HHHHHH'", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "'HHHHHH");

  /* Odd use */
  strcpy(buffer, "pristine"); /* clear previous identical result */
  ret = GetTimeFormatA(lcid, 0, &curtime, "'''HHHHHH", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "'HHHHHH");

  /* TIME_NOTIMEMARKER drops literals too */
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, "'123'tt", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "");

  /* Invalid time */
  curtime.wHour = 25;
  strcpy(buffer, "pristine");
  ret = GetTimeFormatA(lcid, 0, &curtime, "'123'tt", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Invalid date */
  curtime.wHour = 12;
  curtime.wMonth = 60;
  ret = GetTimeFormatA(lcid, 0, &curtime, "h:m:s", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "12:56:13");

  /* The ANSI string may be longer than the Unicode one.
   * In particular, in the Japanese code page, "\x93\xfa" = L"\x65e5".
   */

  lcid = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN), SORT_DEFAULT);

  ret = GetTimeFormatA(lcid, 0, &curtime, "h\x93\xfa", buffer, 5);
  expect_str(ret, buffer, "12\x93\xfa"); /* only 3+1 WCHARs */

  ret = GetTimeFormatA(lcid, 0, &curtime, "h\x93\xfa", buffer, 4);
  expect_err(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  ret = GetTimeFormatA(lcid, 0, &curtime, "h\x93\xfa", NULL, 0);
  expect_str(ret, NULL, "12\x93\xfa");

  strcpy(buffer, "pristine"); /* clear previous identical result */
  ret = GetTimeFormatA(lcid, 0, &curtime, "h\x93\xfa", buffer, 0);
  expect_str(ret, NULL, "12\x93\xfa");
  ok(strcmp(buffer, "pristine") == 0, "Expected a pristine buffer, got '%s'\n", buffer);
}

static void test_GetTimeFormatEx(void)
{
  int ret;
  SYSTEMTIME  curtime;
  WCHAR buffer[BUFFER_SIZE];

  if (!pGetTimeFormatEx)
  {
      win_skip("GetTimeFormatEx not supported\n");
      return;
  }

  SetLastError(0xdeadbeef);

  /* Invalid time */
  memset(&curtime, 2, sizeof(SYSTEMTIME));
  wcscpy(buffer, L"pristine");
  ret = pGetTimeFormatEx(localeW, TIME_FORCE24HOURFORMAT, &curtime, L"tt HH':'mm'@'ss", buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Valid time */
  curtime.wHour = 8;
  curtime.wMinute = 56;
  curtime.wSecond = 13;
  curtime.wMilliseconds = 22;
  ret = pGetTimeFormatEx(localeW, TIME_FORCE24HOURFORMAT, &curtime, L"tt HH':'mm'@'ss", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"AM 08:56@13");

  /* MSDN: LOCALE_NOUSEROVERRIDE can't be specified with a format string */
  wcscpy(buffer, L"pristine");
  ret = pGetTimeFormatEx(localeW, NUO|TIME_FORCE24HOURFORMAT, &curtime, L"HH", buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Insufficient buffer */
  ret = pGetTimeFormatEx(localeW, TIME_FORCE24HOURFORMAT, &curtime, L" tt", buffer, 2);
  /* there is no guarantee on the buffer content, see GetTimeFormatA() */
  expect_werr(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  /* Calculate length only */
  ret = pGetTimeFormatEx(localeW, TIME_FORCE24HOURFORMAT, &curtime, L"tt HH':'mm'@'ss", NULL, 0);
  expect_wstr(ret, NULL, L"AM 08:56@13");

  /* TIME_NOMINUTESORSECONDS, default format */
  ret = pGetTimeFormatEx(localeW, NUO|TIME_NOMINUTESORSECONDS, &curtime, NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"8 AM");

  /* TIME_NOMINUTESORSECONDS/complex format */
  wcscpy(buffer, L"pristine");
  ret = pGetTimeFormatEx(localeW, TIME_NOMINUTESORSECONDS, &curtime, L"m1s2m3s4", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"");

  /* TIME_NOSECONDS/Default format */
  ret = pGetTimeFormatEx(localeW, NUO|TIME_NOSECONDS, &curtime, NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"8:56 AM");

  /* TIME_NOSECONDS */
  wcscpy(buffer, L"pristine"); /* clear previous identical result */
  ret = pGetTimeFormatEx(localeW, TIME_NOSECONDS, &curtime, L"h:m:s tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"8:56 AM");

  /* Multiple delimiters */
  ret = pGetTimeFormatEx(localeW, TIME_NOSECONDS, &curtime, L"h.@:m.@:s.@:tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"8.@:56AM");

  /* Duplicate tokens */
  wcscpy(buffer, L"pristine");
  ret = pGetTimeFormatEx(localeW, TIME_NOSECONDS, &curtime, L"s1s2s3", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"");

  /* AM time marker */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"t/tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"A/AM");

  /* PM time marker */
  curtime.wHour = 13;
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"t/tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"P/PM");

  /* TIME_NOTIMEMARKER: removes text around time marker token */
  ret = pGetTimeFormatEx(localeW, TIME_NOTIMEMARKER, &curtime, L"h1t2tt3m", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"156");

  /* TIME_FORCE24HOURFORMAT */
  ret = pGetTimeFormatEx(localeW, TIME_FORCE24HOURFORMAT, &curtime, L"h:m:s tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"13:56:13 PM");

  /* TIME_FORCE24HOURFORMAT doesn't add time marker */
  ret = pGetTimeFormatEx(localeW, TIME_FORCE24HOURFORMAT, &curtime, L"h:m:s", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"13:56:13");

  /* 24 hrs, leading 0 */
  curtime.wHour = 14; /* change this to 14 or 2pm */
  curtime.wMinute = 5;
  curtime.wSecond = 3;
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h hh H HH m mm s ss t tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2 02 14 14 5 05 3 03 P PM");

  /* "hh" and "HH" */
  curtime.wHour = 0;
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h/H/hh/HH", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"12/0/12/00");

  /* non-zero flags should fail with format, doesn't */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h:m:s tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"12:5:3 AM");

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
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h:hh:hhh H:HH:HHH m:mm:mmm M:MM:MMM s:ss:sss S:SS:SSS", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"8:08:08 8:08:08 56:56:56 M:MM:MMM 13:13:13 S:SS:SSS");

  /* Don't write to buffer if len is 0 */
  wcscpy(buffer, L"pristine");
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h:mm", buffer, 0);
  expect_wstr(ret, NULL, L"8:56");
  ok(wcscmp(buffer, L"pristine") == 0, "Expected a pristine buffer, got %s\n", wine_dbgstr_w(buffer));

  /* "'" preserves tokens */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h 'h' H 'H' HH 'HH' m 'm' s 's' t 't' tt 'tt'", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"8 h 8 H 08 HH 56 m 13 s A t AM tt");

  /* invalid quoted string */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"'''", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"'");

  /* check that MSDN's suggested single quotation usage works as expected */
  wcscpy(buffer, L"pristine"); /* clear previous identical result */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"''''", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"'");

  /* Normal use */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"''HHHHHH", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"08");

  /* and test for normal use of the single quotation mark */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"'''HHHHHH'", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"'HHHHHH");

  /* Odd use */
  wcscpy(buffer, L"pristine"); /* clear previous identical result */
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"'''HHHHHH", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"'HHHHHH");

  /* TIME_NOTIMEMARKER drops literals too */
  ret = pGetTimeFormatEx(localeW, TIME_NOTIMEMARKER, &curtime, L"'123'tt", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"");

  /* Invalid time */
  curtime.wHour = 25;
  wcscpy(buffer, L"pristine");
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"'123'tt", buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Invalid date */
  curtime.wHour = 12;
  curtime.wMonth = 60;
  ret = pGetTimeFormatEx(localeW, 0, &curtime, L"h:m:s", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"12:56:13");
}

static void test_GetDateFormatA(void)
{
  int ret;
  SYSTEMTIME  curtime;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  LCID lcid_ru = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
  LCID lcid_ja = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE];
  char short_day[10], month[10], genitive_month[10];

  SetLastError(0xdeadbeef);

  /* Invalid time */
  memset(&curtime, 2, sizeof(SYSTEMTIME));
  strcpy(buffer, "pristine");
  ret = GetDateFormatA(lcid, 0, &curtime, "ddd',' MMM dd yy", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Simple case */
  curtime.wYear = 2002;
  curtime.wMonth = 5;
  curtime.wDay = 4;
  curtime.wDayOfWeek = 3;
  ret = GetDateFormatA(lcid, 0, &curtime, "ddd',' MMM dd yy", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "Sat, May 04 02");

  /* Same as above but with LOCALE_NOUSEROVERRIDE */
  strcpy(buffer, "pristine");
  ret = GetDateFormatA(lcid, NUO, &curtime, "ddd',' MMM dd yy", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Format containing "'" */
  ret = GetDateFormatA(lcid, 0, &curtime, "ddd',' MMM dd yy", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "Sat, May 04 02");

  /* Invalid time */
  curtime.wHour = 36;
  ret = GetDateFormatA(lcid, 0, &curtime, "ddd',' MMM dd ''''yy", buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "Sat, May 04 '02");

  /* Get size only */
  ret = GetDateFormatA(lcid, 0, &curtime, "ddd',' MMM dd ''''yy", NULL, 0);
  expect_str(ret, NULL, "Sat, May 04 '02");

  /* Buffer too small */
  strcpy(buffer, "pristine");
  ret = GetDateFormatA(lcid, 0, &curtime, "ddd", buffer, 2);
  /* there is no guarantee on the buffer content, see GetTimeFormatA() */
  expect_err(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  /* Default to DATE_SHORTDATE */
  ret = GetDateFormatA(lcid, NUO, &curtime, NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "5/4/2002");

  /* DATE_LONGDATE */
  ret = GetDateFormatA(lcid, NUO|DATE_LONGDATE, &curtime, NULL, buffer, ARRAY_SIZE(buffer));
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());
  ok(strcmp(buffer, "Saturday, May 04, 2002") == 0 ||
     strcmp(buffer, "Saturday, May 4, 2002") == 0 /* Win 8 */,
     "got an unexpected date string '%s'\n", buffer);

  /* test for expected DATE_YEARMONTH behavior with null format */
  /* NT4 returns ERROR_INVALID_FLAGS for DATE_YEARMONTH */
  strcpy(buffer, "pristine");
  ret = GetDateFormatA(lcid, NUO|DATE_YEARMONTH, &curtime, "ddd',' MMM dd ''''yy", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Test that using invalid DATE_* flags results in the correct error */
  /* and return values */
  ret = GetDateFormatA(lcid, DATE_YEARMONTH|DATE_SHORTDATE|DATE_LONGDATE, &curtime, "m/d/y", buffer, ARRAY_SIZE(buffer));
  expect_err(ret, NULL, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "ddMMMM", buffer, ARRAY_SIZE(buffer));
  if (!ret)
  {
    win_skip("LANG_RUSSIAN locale data unavailable\n");
    return;
  }

  /* month part should be in genitive form */
  strcpy(genitive_month, buffer + 2);
  ret = GetDateFormatA(lcid_ru, 0, &curtime, "MMMM", buffer, ARRAY_SIZE(buffer));
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());
  strcpy(month, buffer);
  ok(strcmp(genitive_month, month) != 0, "Expected different month forms\n");

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "ddd", buffer, ARRAY_SIZE(buffer));
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());
  strcpy(short_day, buffer);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "dd MMMMddd dd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "04 %s%s 04", genitive_month, short_day);
  expect_str(ret, buffer, Expected);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "MMMMddd dd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "%s%s 04", month, short_day);
  expect_str(ret, buffer, Expected);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "MMMMddd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "%s%s", month, short_day);
  expect_str(ret, buffer, Expected);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "MMMMdd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "%s04", genitive_month);
  expect_str(ret, buffer, Expected);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "MMMMdd ddd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "%s04 %s", genitive_month, short_day);
  expect_str(ret, buffer, Expected);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "dd dddMMMM", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "04 %s%s", short_day, month);
  expect_str(ret, buffer, Expected);

  ret = GetDateFormatA(lcid_ru, 0, &curtime, "dd dddMMMM ddd MMMMdd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "04 %s%s %s %s04", short_day, month, short_day, genitive_month);
  expect_str(ret, buffer, Expected);

  /* with literal part */
  ret = GetDateFormatA(lcid_ru, 0, &curtime, "ddd',' MMMM dd", buffer, ARRAY_SIZE(buffer));
  sprintf(Expected, "%s, %s 04", short_day, genitive_month);
  expect_str(ret, buffer, Expected);

  /* The ANSI string may be longer than the Unicode one.
   * In particular, in the Japanese code page, "\x93\xfa" = L"\x65e5".
   * See the corresponding GetDateFormatW() test.
   */

  ret = GetDateFormatA(lcid_ja, 0, &curtime, "d\x93\xfa", buffer, 4);
  expect_str(ret, buffer, "4\x93\xfa"); /* only 2+1 WCHARs */

  ret = GetDateFormatA(lcid_ja, 0, &curtime, "d\x93\xfa", buffer, 3);
  expect_err(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  strcpy(buffer, "pristine"); /* clear previous identical result */
  ret = GetDateFormatA(lcid_ja, 0, &curtime, "d\x93\xfa", NULL, 0);
  expect_str(ret, NULL, "4\x93\xfa");
}

static void test_GetDateFormatEx(void)
{
  int ret;
  SYSTEMTIME  curtime;
  WCHAR buffer[BUFFER_SIZE];

  if (!pGetDateFormatEx)
  {
      win_skip("GetDateFormatEx not supported\n");
      return;
  }

  SetLastError(0xdeadbeef);

  /* If flags are set, then format must be NULL */
  wcscpy(buffer, L"pristine");
  ret = pGetDateFormatEx(localeW, DATE_LONGDATE, NULL, L"", buffer, ARRAY_SIZE(buffer), NULL);
  expect_werr(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* NULL buffer, len > 0 */
  wcscpy(buffer, L"pristine");
  ret = pGetDateFormatEx(localeW, 0, NULL, L"", NULL, ARRAY_SIZE(buffer), NULL);
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* NULL buffer, len == 0 */
  ret = pGetDateFormatEx(localeW, 0, NULL, L"", NULL, 0, NULL);
  expect_wstr(ret, NULL, L"");

  /* Invalid flag combination */
  wcscpy(buffer, L"pristine");
  ret = pGetDateFormatEx(localeW, DATE_LONGDATE|DATE_SHORTDATE, NULL,
                         L"", NULL, 0, NULL);
  expect_werr(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Incorrect DOW and time */
  curtime.wYear = 2002;
  curtime.wMonth = 10;
  curtime.wDay = 23;
  curtime.wDayOfWeek = 45612; /* Should be 3 - Wednesday */
  curtime.wHour = 65432; /* Invalid */
  curtime.wMinute = 34512; /* Invalid */
  curtime.wSecond = 65535; /* Invalid */
  curtime.wMilliseconds = 12345;
  ret = pGetDateFormatEx(localeW, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer), NULL);
  expect_wstr(ret, buffer, L"Wednesday 23 October 2002");

  curtime.wYear = 2002;
  curtime.wMonth = 10;
  curtime.wDay = 23;
  curtime.wDayOfWeek = 45612; /* Should be 3 - Wednesday */
  curtime.wHour = 65432; /* Invalid */
  curtime.wMinute = 34512; /* Invalid */
  curtime.wSecond = 65535; /* Invalid */
  curtime.wMilliseconds = 12345;
  wcscpy(buffer, L"pristine");
  ret = pGetDateFormatEx(localeW, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer), emptyW); /* Use reserved arg */
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Limit tests */

  curtime.wYear = 1601;
  curtime.wMonth = 1;
  curtime.wDay = 1;
  curtime.wDayOfWeek = 0; /* Irrelevant */
  curtime.wHour = 0;
  curtime.wMinute = 0;
  curtime.wSecond = 0;
  curtime.wMilliseconds = 0;
  ret = pGetDateFormatEx(localeW, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer), NULL);
  expect_wstr(ret, buffer, L"Monday 1 January 1601");

  curtime.wYear = 1600;
  curtime.wMonth = 12;
  curtime.wDay = 31;
  curtime.wDayOfWeek = 0; /* Irrelevant */
  curtime.wHour = 23;
  curtime.wMinute = 59;
  curtime.wSecond = 59;
  curtime.wMilliseconds = 999;
  wcscpy(buffer, L"pristine");
  ret = pGetDateFormatEx(localeW, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer), NULL);
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
}

static void test_GetDateFormatW(void)
{
  int ret;
  SYSTEMTIME  curtime;
  WCHAR buffer[BUFFER_SIZE];
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  SetLastError(0xdeadbeef);

  /* If flags is not zero then format must be NULL */
  wcscpy(buffer, L"pristine");
  ret = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL, L"", buffer, ARRAY_SIZE(buffer));
  if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("GetDateFormatW is not implemented\n");
    return;
  }
  expect_werr(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* NULL buffer, len > 0 */
  wcscpy(buffer, L"pristine");
  ret = GetDateFormatW (lcid, 0, NULL, L"", NULL, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* NULL buffer, len == 0 */
  ret = GetDateFormatW (lcid, 0, NULL, L"", NULL, 0);
  expect_wstr(ret, NULL, L"");

  /* Incorrect DOW and time */
  curtime.wYear = 2002;
  curtime.wMonth = 10;
  curtime.wDay = 23;
  curtime.wDayOfWeek = 45612; /* Should be 3 - Wednesday */
  curtime.wHour = 65432; /* Invalid */
  curtime.wMinute = 34512; /* Invalid */
  curtime.wSecond = 65535; /* Invalid */
  curtime.wMilliseconds = 12345;
  ret = GetDateFormatW (lcid, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"Wednesday 23 October 2002");

  /* Limit tests */

  curtime.wYear = 1601;
  curtime.wMonth = 1;
  curtime.wDay = 1;
  curtime.wDayOfWeek = 0; /* Irrelevant */
  curtime.wHour = 0;
  curtime.wMinute = 0;
  curtime.wSecond = 0;
  curtime.wMilliseconds = 0;
  ret = GetDateFormatW (lcid, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"Monday 1 January 1601");

  curtime.wYear = 1600;
  curtime.wMonth = 12;
  curtime.wDay = 31;
  curtime.wDayOfWeek = 0; /* Irrelevant */
  curtime.wHour = 23;
  curtime.wMinute = 59;
  curtime.wSecond = 59;
  curtime.wMilliseconds = 999;
  wcscpy(buffer, L"pristine");
  ret = GetDateFormatW (lcid, 0, &curtime, L"dddd d MMMM yyyy", buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* See the corresponding GetDateFormatA() test */

  lcid = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN), SORT_DEFAULT);

  curtime.wYear = 2002;
  curtime.wMonth = 5;
  curtime.wDay = 4;
  ret = GetDateFormatW(lcid, 0, &curtime, L"d\x65e5", buffer, 3);
  expect_wstr(ret, buffer, L"4\x65e5");

  ret = GetDateFormatW(lcid, 0, &curtime, L"d\x65e5", NULL, 0);
  expect_wstr(ret, NULL, L"4\x65e5");
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
  static const char* const negative_order[] =
      {"($1.0)",    /* 0 */
       "-$1.0",
       "$-1.0",
       "$1.0-",
       "(1.0$)",
       "-1.0$",     /* 5 */
       "1.0-$",
       "1.0$-",
       "-1.0 $",
       "-$ 1.0",
       "1.0 $-",    /* 10 */
       "$ 1.0-",
       "$ -1.0",
       "1.0- $",
       "($ 1.0)",
       "(1.0 $)",   /* 15 */
      };
  int ret, o;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE];
  CURRENCYFMTA format;

  SetLastError(0xdeadbeef);

  /* NULL output, length > 0 --> Error */
  ret = GetCurrencyFormatA(lcid, 0, "23", NULL, NULL, ARRAY_SIZE(buffer));
  expect_err(ret, NULL, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Invalid character --> Error */
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, 0, "23,53", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Double '-' --> Error */
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, 0, "--", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Trailing '-' --> Error */
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, 0, "0-", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Double '.' --> Error */
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, 0, "0..", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Leading space --> Error */
  ret = GetCurrencyFormatA(lcid, 0, " 0.1", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Length too small --> Write up to length chars */
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, NUO, "1234", NULL, buffer, 2);
  /* there is no guarantee on the buffer content, see GetTimeFormatA() */
  expect_err(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  /* Valid number */
  ret = GetCurrencyFormatA(lcid, NUO, "2353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$2,353.00");

  /* Valid negative number */
  ret = GetCurrencyFormatA(lcid, NUO, "-2353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "($2,353.00)");

  /* Valid real number */
  ret = GetCurrencyFormatA(lcid, NUO, "2353.1", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$2,353.10");

  /* Too many DP --> Truncated */
  ret = GetCurrencyFormatA(lcid, NUO, "2353.111", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$2,353.11");

  /* Too many DP --> Rounded */
  ret = GetCurrencyFormatA(lcid, NUO, "2353.119", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$2,353.12");

  /* Format and flags given --> Error */
  memset(&format, 0, sizeof(format));
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, NUO, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Invalid format --> Error */
  strcpy(buffer, "pristine");
  ret = GetCurrencyFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.PositiveOrder = CY_POS_LEFT;
  format.lpDecimalSep = szDot;
  format.lpThousandSep = szComma;
  format.lpCurrencySymbol = szDollar;

  /* No decimal or grouping chars expected */
  ret = GetCurrencyFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$2353");

  /* 1 DP --> Expect decimal separator */
  format.NumDigits = 1;
  ret = GetCurrencyFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$2353.0");

  /* Group by 100's */
  format.Grouping = 2;
  ret = GetCurrencyFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$23,53.0");

  /* Grouping of a positive number */
  format.Grouping = 3;
  ret = GetCurrencyFormatA(lcid, 0, "235", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$235.0");

  format.Grouping = 31;
  ret = GetCurrencyFormatA(lcid, 0, "1234567890", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$1,2,3,4,5,6,7,890.0");

  format.Grouping = 312;
  ret = GetCurrencyFormatA(lcid, 0, "1234567890", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$12,34,56,7,890.0");

  format.Grouping = 310;
  ret = GetCurrencyFormatA(lcid, 0, "1234567890", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$123456,7,890.0");

  /* Grouping of a negative number */
  format.NegativeOrder = 2;
  ret = GetCurrencyFormatA(lcid, 0, "-235", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$-235.0");

  /* Always provide leading zero */
  format.LeadingZero = 1;
  ret = GetCurrencyFormatA(lcid, 0, ".5", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$0.5");
  ret = GetCurrencyFormatA(lcid, 0, "0.5", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$0.5");

  format.LeadingZero = 0;
  ret = GetCurrencyFormatA(lcid, 0, "0.5", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$.5");
  ret = GetCurrencyFormatA(lcid, 0, "0.5", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$.5");

  format.PositiveOrder = CY_POS_RIGHT;
  ret = GetCurrencyFormatA(lcid, 0, "1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1.0$");

  format.PositiveOrder = CY_POS_LEFT_SPACE;
  ret = GetCurrencyFormatA(lcid, 0, "1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "$ 1.0");

  format.PositiveOrder = CY_POS_RIGHT_SPACE;
  ret = GetCurrencyFormatA(lcid, 0, "1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1.0 $");


  for (o = 0; o <= 15; o++)
  {
    winetest_push_context("%d", o);
    format.NegativeOrder = o;
    strcpy(buffer, "pristine");
    ret = GetCurrencyFormatA(lcid, 0, "-1", &format, buffer, ARRAY_SIZE(buffer));
    expect_str(ret, buffer, negative_order[o]);
    winetest_pop_context();
  }
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
  WCHAR grouping[32], t1000[8], dec[8], frac[8], lzero[8];
  char buffer[BUFFER_SIZE];
  NUMBERFMTA format;

  SetLastError(0xdeadbeef);

  /* NULL output, length > 0 --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, "23", NULL, NULL, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Invalid character --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, "23,53", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Double '-' --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, "--", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Trailing '-' --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, "0-", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Double '.' --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, "0..", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Leading space --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, " 0.1", NULL, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Length too small --> Write up to length chars */
  ret = GetNumberFormatA(lcid, NUO, "1234", NULL, buffer, 2);
  /* there is no guarantee on the buffer content, see GetTimeFormatA() */
  expect_err(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  /* Valid number */
  ret = GetNumberFormatA(lcid, NUO, "2353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2,353.00");

  /* Valid negative number */
  ret = GetNumberFormatA(lcid, NUO, "-2353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "-2,353.00");

  /* test for off by one error in grouping */
  ret = GetNumberFormatA(lcid, NUO, "-353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "-353.00");

  /* Valid real number */
  ret = GetNumberFormatA(lcid, NUO, "2353.1", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2,353.10");

  /* Too many DP --> Truncated */
  ret = GetNumberFormatA(lcid, NUO, "2353.111", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2,353.11");

  /* Too many DP --> Rounded */
  ret = GetNumberFormatA(lcid, NUO, "2353.119", NULL, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2,353.12");

  /* Format and flags given --> Error */
  memset(&format, 0, sizeof(format));
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, NUO, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Invalid format --> Error */
  strcpy(buffer, "pristine");
  ret = GetNumberFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_err(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.lpDecimalSep = szDot;
  format.lpThousandSep = szComma;

  /* No decimal or grouping chars expected */
  ret = GetNumberFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2353");

  format.NumDigits = 1; /* 1 DP --> Expect decimal separator */
  ret = GetNumberFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "2353.0");

  format.Grouping = 2; /* Group by 100's */
  ret = GetNumberFormatA(lcid, 0, "2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "23,53.0");

  /* Grouping of a positive number */
  format.Grouping = 3;
  ret = GetNumberFormatA(lcid, 0, "235", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "235.0");
  ret = GetNumberFormatA(lcid, 0, "000235", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "235.0");

  format.Grouping = 23;
  ret = GetNumberFormatA(lcid, 0, "123456789", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1,234,567,89.0");

  format.Grouping = 230;
  ret = GetNumberFormatA(lcid, 0, "123456789", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1234,567,89.0");

  format.Grouping = 203;
  ret = GetNumberFormatA(lcid, 0, "123456789", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1,234,567,,89.0");

  format.Grouping = 2030;
  ret = GetNumberFormatA(lcid, 0, "123456789", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1234,567,,89.0");

  format.Grouping = 2003;
  ret = GetNumberFormatA(lcid, 0, "123456789", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1,234,567,,,89.0");

  format.Grouping = 1200;
  ret = GetNumberFormatA(lcid, 0, "123456789", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "123456,,78,9.0");

  /* Grouping of a negative number */
  format.NegativeOrder = NEG_LEFT;
  format.Grouping = 3;
  ret = GetNumberFormatA(lcid, 0, "-235", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "-235.0");
  ret = GetNumberFormatA(lcid, 0, "-000235", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "-235.0");

  format.LeadingZero = 1; /* Always provide leading zero */
  ret = GetNumberFormatA(lcid, 0, ".5", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "0.5");
  ret = GetNumberFormatA(lcid, 0, "0000.5", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "0.5");

  format.NegativeOrder = NEG_PARENS;
  ret = GetNumberFormatA(lcid, 0, "-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "(1.0)");

  format.NegativeOrder = NEG_LEFT;
  ret = GetNumberFormatA(lcid, 0, "-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "-1.0");

  format.NegativeOrder = NEG_LEFT_SPACE;
  ret = GetNumberFormatA(lcid, 0, "-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "- 1.0");

  format.NegativeOrder = NEG_RIGHT;
  ret = GetNumberFormatA(lcid, 0, "-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1.0-");

  format.NegativeOrder = NEG_RIGHT_SPACE;
  ret = GetNumberFormatA(lcid, 0, "-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_str(ret, buffer, "1.0 -");

  /* Test French formatting */
  lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);
  if (IsValidLocale(lcid, 0))
  {
    ret = GetNumberFormatA(lcid, NUO, "-12345", NULL, buffer, ARRAY_SIZE(buffer));
    expect_str(ret, buffer, "-12\xa0\x33\x34\x35,00"); /* Non breaking space */
  }

  /* Test the actual LOCALE_SGROUPING string, the rules for repeats are opposite */
  if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, ARRAY_SIZE(grouping)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, t1000, ARRAY_SIZE(t1000)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, dec, ARRAY_SIZE(dec)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, frac, ARRAY_SIZE(frac)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO, lzero, ARRAY_SIZE(lzero)))
  {
    static const struct
    {
        const char *grouping;
        const char *expected;
    } tests[] = {
        { "3;0",                "1,234,567,890.54321" },
        { "2;3",                "12345,678,90.54321" },
        { "1",                  "123456789,0.54321" },
        { "1;0",                "1,2,3,4,5,6,7,8,9,0.54321" },
        { "1;0;3",              "123456,789,,0.54321" },
        { "0",                  "1234567890.54321" },
        { "0;0",                "1234567890.54321" },
        { "0;1",                "123456789,0.54321" },
        { "0;0;0",              "1234567890.54321" },
        { "0;1;0",              "1,2,3,4,5,6,7,8,9,0.54321" },
        { "2;0;0",              "12345678,90.54321" },
        { "2;0;0;0",            "12345678,,90.54321" },
        { "2;0;0;0;0",          "12345678,,,90.54321" },
        { "2;0;0;1;0",          "1,2,3,4,5,6,7,8,,,90.54321" },
        { "1;3;2",              "1234,56,789,0.54321" },
        { "1;3;2;0",            "12,34,56,789,0.54321" },
        { "3;1;1;2;0",          "1,23,45,6,7,890.54321" },
        { "6;1",                "123,4,567890.54321" },
    };
    unsigned i;

    SetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, ",");
    SetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, ".");
    SetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, "5");
    SetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_ILZERO, "0");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
      SetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, tests[i].grouping);
      SetLastError(0xdeadbeef);
      ret = GetNumberFormatA(LOCALE_USER_DEFAULT, 0, "1234567890.54321", NULL, buffer, ARRAY_SIZE(buffer));
      if (ret)
      {
        ok(GetLastError() == 0xdeadbeef, "[%u] unexpected error %lu\n", i, GetLastError());
        ok(ret == strlen(tests[i].expected) + 1, "[%u] unexpected ret %d\n", i, ret);
        ok(!strcmp(buffer, tests[i].expected), "[%u] unexpected string %s\n", i, buffer);
      }
      else
        ok(0, "[%u] expected success, got error %ld\n", i, GetLastError());
    }

    /* Restore */
    ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping), "Restoring SGROUPING failed\n");
    ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, t1000), "Restoring STHOUSAND failed\n");
    ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, dec), "Restoring SDECIMAL failed\n");
    ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, frac), "Restoring IDIGITS failed\n");
    ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO, lzero), "Restoring ILZERO failed\n");
  }
}

static void test_GetNumberFormatEx(void)
{
  int ret;
  NUMBERFMTW format;
  static WCHAR dotW[] = {'.',0};
  static WCHAR commaW[] = {',',0};
  static const WCHAR enW[] = {'e','n','-','U','S',0};
  static const WCHAR frW[] = {'f','r','-','F','R',0};
  static const WCHAR bogusW[] = {'b','o','g','u','s',0};
  WCHAR buffer[BUFFER_SIZE];

  if (!pGetNumberFormatEx)
  {
    win_skip("GetNumberFormatEx is not available.\n");
    return;
  }

  SetLastError(0xdeadbeef);

  /* NULL output, length > 0 --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L"23", NULL, NULL, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Invalid character --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L"23,53", NULL, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Double '-' --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L"--", NULL, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Trailing '-' --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L"0-", NULL, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Double '.' --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L"0..", NULL, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Leading space --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L" 0.1", NULL, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Length too small --> Write up to length chars */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, NUO, L"1234", NULL, buffer, 2);
  /* there is no guarantee on the buffer content, see GetTimeFormatA() */
  expect_werr(ret, NULL, ERROR_INSUFFICIENT_BUFFER);
  SetLastError(0xdeadbeef);

  /* Bogus locale --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(bogusW, NUO, L"23", NULL, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  memset(&format, 0, sizeof(format));

  /* Format and flags given --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, NUO, L"2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_FLAGS);
  SetLastError(0xdeadbeef);

  /* Invalid format --> Error */
  wcscpy(buffer, L"pristine");
  ret = pGetNumberFormatEx(enW, 0, L"2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_werr(ret, buffer, ERROR_INVALID_PARAMETER);
  SetLastError(0xdeadbeef);

  /* Valid number */
  ret = pGetNumberFormatEx(enW, NUO, L"2353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2,353.00");

  /* Valid negative number */
  ret = pGetNumberFormatEx(enW, NUO, L"-2353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"-2,353.00");

  /* test for off by one error in grouping */
  ret = pGetNumberFormatEx(enW, NUO, L"-353", NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"-353.00");

  /* Valid real number */
  ret = pGetNumberFormatEx(enW, NUO, L"2353.1", NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2,353.10");

  /* Too many DP --> Truncated */
  ret = pGetNumberFormatEx(enW, NUO, L"2353.111", NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2,353.11");

  /* Too many DP --> Rounded */
  ret = pGetNumberFormatEx(enW, NUO, L"2353.119", NULL, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2,353.12");

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.lpDecimalSep = dotW;
  format.lpThousandSep = commaW;

  /* No decimal or grouping chars expected */
  ret = pGetNumberFormatEx(enW, 0, L"2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2353");

  /* 1 DP --> Expect decimal separator */
  format.NumDigits = 1;
  ret = pGetNumberFormatEx(enW, 0, L"2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"2353.0");

  /* Group by 100's */
  format.Grouping = 2;
  ret = pGetNumberFormatEx(enW, 0, L"2353", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"23,53.0");

  /* Grouping of a positive number */
  format.Grouping = 3;
  ret = pGetNumberFormatEx(enW, 0, L"235", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"235.0");

  /* Grouping of a negative number */
  format.NegativeOrder = NEG_LEFT;
  ret = pGetNumberFormatEx(enW, 0, L"-235", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"-235.0");

  /* Always provide leading zero */
  format.LeadingZero = 1;
  ret = pGetNumberFormatEx(enW, 0, L".5", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"0.5");
  ret = pGetNumberFormatEx(enW, 0, L"0.5", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"0.5");

  format.LeadingZero = 0;
  ret = pGetNumberFormatEx(enW, 0, L".5", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L".5");
  ret = pGetNumberFormatEx(enW, 0, L"0.5", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L".5");

  format.NegativeOrder = NEG_PARENS;
  ret = pGetNumberFormatEx(enW, 0, L"-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"(1.0)");

  format.NegativeOrder = NEG_LEFT;
  ret = pGetNumberFormatEx(enW, 0, L"-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"-1.0");

  format.NegativeOrder = NEG_LEFT_SPACE;
  ret = pGetNumberFormatEx(enW, 0, L"-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"- 1.0");

  format.NegativeOrder = NEG_RIGHT;
  ret = pGetNumberFormatEx(enW, 0, L"-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"1.0-");

  format.NegativeOrder = NEG_RIGHT_SPACE;
  ret = pGetNumberFormatEx(enW, 0, L"-1", &format, buffer, ARRAY_SIZE(buffer));
  expect_wstr(ret, buffer, L"1.0 -");

  /* Test French formatting */
  if (pIsValidLocaleName(frW))
  {
    const WCHAR *expected;
    ret = pGetNumberFormatEx(frW, NUO, L"-12345", NULL, buffer, ARRAY_SIZE(buffer));
    expected = (ret && wcschr(buffer, 0x202f)) ?
               L"-12\x202f\x33\x34\x35,00" : /* Same but narrow (win11) */
               L"-12\xa0\x33\x34\x35,00"; /* Non breaking space */
    expect_wstr(ret, buffer, expected);
  }
}

struct comparestringa_entry {
  LCID lcid;
  DWORD flags;
  const char *first;
  int first_len;
  const char *second;
  int second_len;
  int ret;
  DWORD le;
};

static const struct comparestringa_entry comparestringa_data[] = {
  { LOCALE_SYSTEM_DEFAULT, 0, "EndDialog", -1, "_Property", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "osp_vba.sreg0070", -1, "_IEWWBrowserComp", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "r", -1, "\\", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "osp_vba.sreg0031", -1, "OriginalDatabase", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "AAA", -1, "aaa", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "AAA", -1, "aab", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "AAA", -1, "Aab", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, ".AAA", -1, "Aab", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, ".AAA", -1, "A.ab", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "aa", -1, "AB", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "aa", -1, "Aab", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "aB", -1, "Aab", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "Ba", -1, "bab", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "{100}{83}{71}{71}{71}", -1, "Global_DataAccess_JRO", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "a", -1, "{", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "A", -1, "{", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "3.5", 0, "4.0", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "3.5", -1, "4.0", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "3.520.4403.2", -1, "4.0.2927.10", -1, CSTR_LESS_THAN },
  /* hyphen and apostrophe are treated differently depending on whether SORT_STRINGSORT specified or not */
  { LOCALE_SYSTEM_DEFAULT, 0, "-o", -1, "/m", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "/m", -1, "-o", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "-o", -1, "/m", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "/m", -1, "-o", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "'o", -1, "/m", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "/m", -1, "'o", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "'o", -1, "/m", -1, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "/m", -1, "'o", -1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "aLuZkUtZ", 8, "aLuZkUtZ", 9, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, 0, "aLuZkUtZ", 7, "aLuZkUtZ\0A", 10, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "a-", 3, "a\0", 3, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "a'", 3, "a\0", 3, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "a-", 3, "a\0", 3, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "a'", 3, "a\0", 3, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, NORM_IGNORESYMBOLS, "a.", 3, "a\0", 3, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, NORM_IGNORESYMBOLS, "a ", 3, "a\0", 3, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, 0, "a", 1, "a\0\0", 4, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, 0, "a", 2, "a\0\0", 4, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, 0, "a\0\0", 4, "a", 1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, 0, "a\0\0", 4, "a", 2, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, 0, "a", 1, "a\0x", 4, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "a", 2, "a\0x", 4, CSTR_LESS_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "a\0x", 4, "a", 1, CSTR_GREATER_THAN },
  { LOCALE_SYSTEM_DEFAULT, 0, "a\0x", 4, "a", 2, CSTR_GREATER_THAN },
  /* flag tests */
  { LOCALE_SYSTEM_DEFAULT, LOCALE_USE_CP_ACP, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, LINGUISTIC_IGNORECASE, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, LINGUISTIC_IGNOREDIACRITIC, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, NORM_IGNOREKANATYPE, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, NORM_IGNORENONSPACE, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, NORM_IGNOREWIDTH, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, NORM_LINGUISTIC_CASING, "NULL", -1, "NULL", -1, CSTR_EQUAL },
  { LOCALE_SYSTEM_DEFAULT, SORT_DIGITSASNUMBERS, "NULL", -1, "NULL", -1, 0, ERROR_INVALID_FLAGS }
};

static void test_CompareStringA(void)
{
  static const char ABC_EE[] = {'A','B','C',0,0xEE};
  static const char ABC_FF[] = {'A','B','C',0,0xFF};
  int ret, i;
  char a[256];
  BOOL is_utf8;
  CPINFOEXA cpinfo;
  LCID lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

  GetCPInfoExA( CP_ACP, 0, &cpinfo );
  is_utf8 = cpinfo.CodePage == CP_UTF8;

  for (i = 0; i < ARRAY_SIZE(comparestringa_data); i++)
  {
      const struct comparestringa_entry *entry = &comparestringa_data[i];

      SetLastError(0xdeadbeef);
      ret = CompareStringA(entry->lcid, entry->flags, entry->first, entry->first_len,
          entry->second, entry->second_len);
      ok(ret == entry->ret, "%d: got %d, expected %d\n", i, ret, entry->ret);
      ok(GetLastError() == (ret ? 0xdeadbeef : entry->le), "%d: got last error %ld, expected %ld\n",
          i, GetLastError(), (ret ? 0xdeadbeef : entry->le));
  }

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "Salute", -1);
  ok (ret == CSTR_LESS_THAN, "(Salut/Salute) Expected CSTR_LESS_THAN, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "SaLuT", -1);
  ok (ret == CSTR_EQUAL, "(Salut/SaLuT) Expected CSTR_EQUAL, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "hola", -1);
  ok (ret == CSTR_GREATER_THAN, "(Salut/hola) Expected CSTR_GREATER_THAN, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", -1);
  ok (ret == CSTR_LESS_THAN, "(haha/hoho) Expected CSTR_LESS_THAN, got %d\n", ret);

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", -1);
  ok (ret == CSTR_LESS_THAN, "(haha/hoho) Expected CSTR_LESS_THAN, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", 0);
  ok (ret == CSTR_GREATER_THAN, "(haha/hoho) Expected CSTR_GREATER_THAN, got %d\n", ret);

    ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", 5, "saLuT", -1);
    ok (ret == CSTR_EQUAL, "(Salut/saLuT) Expected CSTR_EQUAL, got %d\n", ret);

    ret = lstrcmpA("", "");
    ok (ret == 0, "lstrcmpA(\"\", \"\") should return 0, got %d\n", ret);

    ret = lstrcmpA(NULL, NULL);
    ok (ret == 0 || broken(ret == -2) /* win9x */, "lstrcmpA(NULL, NULL) should return 0, got %d\n", ret);

    ret = lstrcmpA("", NULL);
    ok (ret == 1 || broken(ret == -2) /* win9x */, "lstrcmpA(\"\", NULL) should return 1, got %d\n", ret);

    ret = lstrcmpA(NULL, "");
    ok (ret == -1 || broken(ret == -2) /* win9x */, "lstrcmpA(NULL, \"\") should return -1, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "'o", -1, "-o", -1 );
    ok(ret == CSTR_LESS_THAN, "'o vs -o expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "'o", -1, "-o", -1 );
    ok(ret == CSTR_LESS_THAN, "'o vs -o expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "'", -1, "-", -1 );
    ok(ret == CSTR_LESS_THAN, "' vs - expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "'", -1, "-", -1 );
    ok(ret == CSTR_LESS_THAN, "' vs - expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "`o", -1, "/m", -1 );
    ok(ret == CSTR_GREATER_THAN, "`o vs /m CSTR_GREATER_THAN got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "/m", -1, "`o", -1 );
    ok(ret == CSTR_LESS_THAN, "/m vs `o expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "`o", -1, "/m", -1 );
    ok(ret == CSTR_GREATER_THAN, "`o vs /m CSTR_GREATER_THAN got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "/m", -1, "`o", -1 );
    ok(ret == CSTR_LESS_THAN, "/m vs `o expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "`o", -1, "-m", -1 );
    ok(ret == CSTR_LESS_THAN, "`o vs -m expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, 0, "-m", -1, "`o", -1 );
    ok(ret == CSTR_GREATER_THAN, "-m vs `o CSTR_GREATER_THAN got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "`o", -1, "-m", -1 );
    ok(ret == CSTR_GREATER_THAN, "`o vs -m CSTR_GREATER_THAN got %d\n", ret);

    ret = CompareStringA(LOCALE_SYSTEM_DEFAULT, SORT_STRINGSORT, "-m", -1, "`o", -1 );
    ok(ret == CSTR_LESS_THAN, "-m vs `o expected CSTR_LESS_THAN, got %d\n", ret);

    /* WinXP handles embedded NULLs differently than earlier versions */
    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, "aLuZkUtZ", 8, "aLuZkUtZ\0A", 10);
    ok(ret == CSTR_LESS_THAN || ret == CSTR_EQUAL, "aLuZkUtZ vs aLuZkUtZ\\0A expected CSTR_LESS_THAN or CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, "aLu\0ZkUtZ", 8, "aLu\0ZkUtZ\0A", 10);
    ok(ret == CSTR_LESS_THAN || ret == CSTR_EQUAL, "aLu\\0ZkUtZ vs aLu\\0ZkUtZ\\0A expected CSTR_LESS_THAN or CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringA(lcid, 0, "a\0b", -1, "a", -1);
    ok(ret == CSTR_EQUAL, "a vs a expected CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringA(lcid, 0, "a\0b", 4, "a", 2);
    ok(ret == CSTR_EQUAL || /* win2k */
       ret == CSTR_GREATER_THAN,
       "a\\0b vs a expected CSTR_EQUAL or CSTR_GREATER_THAN, got %d\n", ret);

    ret = CompareStringA(lcid, 0, "\2", 2, "\1", 2);
    ok(ret != CSTR_EQUAL, "\\2 vs \\1 expected unequal\n");

    ret = CompareStringA(lcid, NORM_IGNORECASE | LOCALE_USE_CP_ACP, "#", -1, ".", -1);
    ok(ret == CSTR_LESS_THAN, "\"#\" vs \".\" expected CSTR_LESS_THAN, got %d\n", ret);

    ret = CompareStringA(lcid, NORM_IGNORECASE, "_", -1, ".", -1);
    ok(ret == CSTR_GREATER_THAN, "\"_\" vs \".\" expected CSTR_GREATER_THAN, got %d\n", ret);

    ret = lstrcmpiA("#", ".");
    ok(ret == -1, "\"#\" vs \".\" expected -1, got %d\n", ret);

    lcid = MAKELCID(MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT), SORT_DEFAULT);

    /* \xB9 character lies between a and b */
    ret = CompareStringA(lcid, 0, "a", 1, "\xB9", 1);
    ok(ret == CSTR_LESS_THAN, "\'\\xB9\' character should be greater than \'a\'\n");
    ret = CompareStringA(lcid, 0, "\xB9", 1, "b", 1);
    ok(ret == CSTR_LESS_THAN, "\'\\xB9\' character should be smaller than \'b\'\n");

    memset(a, 'a', sizeof(a));
    SetLastError(0xdeadbeef);
    ret = CompareStringA(lcid, 0, a, sizeof(a), a, sizeof(a));
    ok (GetLastError() == 0xdeadbeef && ret == CSTR_EQUAL,
        "ret %d, error %ld, expected value %d\n", ret, GetLastError(), CSTR_EQUAL);

    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, ABC_EE, 3, ABC_FF, 3);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);
    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, ABC_EE, 5, ABC_FF, 3);
    ok(ret == CSTR_GREATER_THAN || (is_utf8 && ret == CSTR_EQUAL), "expected CSTR_GREATER_THAN, got %d\n", ret);
    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, ABC_EE, 3, ABC_FF, 5);
    ok(ret == CSTR_LESS_THAN || (is_utf8 && ret == CSTR_EQUAL), "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, ABC_EE, 5, ABC_FF, 5);
    ok(ret == CSTR_LESS_THAN || (is_utf8 && ret == CSTR_EQUAL), "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringA(LOCALE_USER_DEFAULT, 0, ABC_FF, 5, ABC_EE, 5);
    ok(ret == CSTR_GREATER_THAN || (is_utf8 && ret == CSTR_EQUAL), "expected CSTR_GREATER_THAN, got %d\n", ret);
}

static void test_CompareStringW(void)
{
    static const WCHAR ABC_EE[] = {'A','B','C',0,0xEE};
    static const WCHAR ABC_FF[] = {'A','B','C',0,0xFF};
    static const WCHAR A_ACUTE_BC[] = {0xc1,'B','C',0};
    static const WCHAR A_ACUTE_BC_DECOMP[] = {'A',0x301,'B','C',0};
    static const WCHAR A_NULL_BC[] = {'A',0,'B','C',0};
    WCHAR *str1, *str2;
    SYSTEM_INFO si;
    DWORD old_prot;
    BOOL success;
    char *buf;
    int ret;

    GetSystemInfo(&si);
    buf = VirtualAlloc(NULL, si.dwPageSize * 4, MEM_COMMIT, PAGE_READWRITE);
    ok(buf != NULL, "VirtualAlloc failed with %lu\n", GetLastError());
    success = VirtualProtect(buf + si.dwPageSize, si.dwPageSize, PAGE_NOACCESS, &old_prot);
    ok(success, "VirtualProtect failed with %lu\n", GetLastError());
    success = VirtualProtect(buf + 3 * si.dwPageSize, si.dwPageSize, PAGE_NOACCESS, &old_prot);
    ok(success, "VirtualProtect failed with %lu\n", GetLastError());

    str1 = (WCHAR *)(buf + si.dwPageSize - sizeof(WCHAR));
    str2 = (WCHAR *)(buf + 3 * si.dwPageSize - sizeof(WCHAR));
    *str1 = 'A';
    *str2 = 'B';

    /* CompareStringW should abort on the first non-matching character */
    ret = CompareStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
            0, str1, 100, str2, 100);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);

    success = VirtualFree(buf, 0, MEM_RELEASE);
    ok(success, "VirtualFree failed with %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CompareStringW(LOCALE_USER_DEFAULT, SORT_DIGITSASNUMBERS, L"NULL", -1, L"NULL", -1);
    ok(ret == CSTR_EQUAL || broken(!ret && GetLastError() == ERROR_INVALID_FLAGS) /* <Win7 */,
        "expected CSTR_EQUAL, got %d, last error %ld\n", ret, GetLastError());

    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 3, ABC_FF, 3);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 5, ABC_FF, 3);
    ok(ret == CSTR_GREATER_THAN, "expected CSTR_GREATER_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 3, ABC_FF, 5);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 5, ABC_FF, 5);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_FF, 5, ABC_EE, 5);
    ok(ret == CSTR_GREATER_THAN, "expected CSTR_GREATER_THAN, got %d\n", ret);

    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 4, A_ACUTE_BC, 4);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 4, A_ACUTE_BC_DECOMP, 5);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, A_ACUTE_BC, 4, A_ACUTE_BC_DECOMP, 5);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE, ABC_EE, 3, A_ACUTE_BC, 4);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE, ABC_EE, 4, A_ACUTE_BC_DECOMP, 5);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE, A_ACUTE_BC, 4, A_ACUTE_BC_DECOMP, 5);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, ABC_EE, 4, A_NULL_BC, 4);
    ok(ret == CSTR_EQUAL, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE, ABC_EE, 4, A_NULL_BC, 4);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, A_NULL_BC, 4, A_ACUTE_BC, 4);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE, A_NULL_BC, 4, A_ACUTE_BC, 4);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);

    ret = CompareStringW(LOCALE_USER_DEFAULT, 0, A_NULL_BC, 4, A_ACUTE_BC_DECOMP, 5);
    ok(ret == CSTR_LESS_THAN, "expected CSTR_LESS_THAN, got %d\n", ret);
    ret = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE, A_NULL_BC, 4, A_ACUTE_BC_DECOMP, 5);
    ok(ret == CSTR_EQUAL, "expected CSTR_EQUAL, got %d\n", ret);
}

struct comparestringex_test {
    const char *locale;
    DWORD flags;
    const WCHAR first[2];
    const WCHAR second[2];
    INT ret;
    INT broken;
};

static const struct comparestringex_test comparestringex_tests[] = {
    /* default behavior */
    { /* 0 */
      "tr-TR", 0,
      {'i',0},   {'I',0},   CSTR_LESS_THAN,    -1
    },
    { /* 1 */
      "tr-TR", 0,
      {'i',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 2 */
      "tr-TR", 0,
      {'i',0},   {0x131,0}, CSTR_LESS_THAN,    -1
    },
    { /* 3 */
      "tr-TR", 0,
      {'I',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 4 */
      "tr-TR", 0,
      {'I',0},   {0x131,0}, CSTR_LESS_THAN,    -1
    },
    { /* 5 */
      "tr-TR", 0,
      {0x130,0}, {0x131,0}, CSTR_GREATER_THAN, -1
    },
    /* with NORM_IGNORECASE */
    { /* 6 */
      "tr-TR", NORM_IGNORECASE,
      {'i',0},   {'I',0},   CSTR_EQUAL,        -1
    },
    { /* 7 */
      "tr-TR", NORM_IGNORECASE,
      {'i',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 8 */
      "tr-TR", NORM_IGNORECASE,
      {'i',0},   {0x131,0}, CSTR_LESS_THAN,    -1
    },
    { /* 9 */
      "tr-TR", NORM_IGNORECASE,
      {'I',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 10 */
      "tr-TR", NORM_IGNORECASE,
      {'I',0},   {0x131,0}, CSTR_LESS_THAN,    -1
    },
    { /* 11 */
      "tr-TR", NORM_IGNORECASE,
      {0x130,0}, {0x131,0}, CSTR_GREATER_THAN, -1
    },
    /* with NORM_LINGUISTIC_CASING */
    { /* 12 */
      "tr-TR", NORM_LINGUISTIC_CASING,
      {'i',0},   {'I',0},   CSTR_GREATER_THAN, CSTR_LESS_THAN
    },
    { /* 13 */
      "tr-TR", NORM_LINGUISTIC_CASING,
      {'i',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 14 */
      "tr-TR", NORM_LINGUISTIC_CASING,
      {'i',0},   {0x131,0}, CSTR_GREATER_THAN, CSTR_LESS_THAN
    },
    { /* 15 */
      "tr-TR", NORM_LINGUISTIC_CASING,
      {'I',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 16 */
      "tr-TR", NORM_LINGUISTIC_CASING,
      {'I',0},   {0x131,0}, CSTR_GREATER_THAN, CSTR_LESS_THAN
    },
    { /* 17 */
      "tr-TR", NORM_LINGUISTIC_CASING,
      {0x130,0}, {0x131,0}, CSTR_GREATER_THAN, -1
    },
    /* with LINGUISTIC_IGNORECASE */
    { /* 18 */
      "tr-TR", LINGUISTIC_IGNORECASE,
      {'i',0},   {'I',0},   CSTR_EQUAL,        -1
    },
    { /* 19 */
      "tr-TR", LINGUISTIC_IGNORECASE,
      {'i',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 20 */
      "tr-TR", LINGUISTIC_IGNORECASE,
      {'i',0},   {0x131,0}, CSTR_LESS_THAN,    -1
    },
    { /* 21 */
      "tr-TR", LINGUISTIC_IGNORECASE,
      {'I',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 22 */
      "tr-TR", LINGUISTIC_IGNORECASE,
      {'I',0},   {0x131,0}, CSTR_LESS_THAN,    -1
    },
    { /* 23 */
      "tr-TR", LINGUISTIC_IGNORECASE,
      {0x130,0}, {0x131,0}, CSTR_GREATER_THAN, -1
    },
    /* with NORM_LINGUISTIC_CASING | NORM_IGNORECASE */
    { /* 24 */
      "tr-TR", NORM_LINGUISTIC_CASING | NORM_IGNORECASE,
      {'i',0},   {'I',0},   CSTR_GREATER_THAN, CSTR_EQUAL
    },
    { /* 25 */
      "tr-TR", NORM_LINGUISTIC_CASING | NORM_IGNORECASE,
      {'i',0},   {0x130,0}, CSTR_EQUAL,        CSTR_LESS_THAN
    },
    { /* 26 */
      "tr-TR", NORM_LINGUISTIC_CASING | NORM_IGNORECASE,
      {'i',0},   {0x131,0}, CSTR_GREATER_THAN, CSTR_LESS_THAN
    },
    { /* 27 */
      "tr-TR", NORM_LINGUISTIC_CASING | NORM_IGNORECASE,
      {'I',0},   {0x130,0}, CSTR_LESS_THAN,    -1
     },
    { /* 28 */
      "tr-TR", NORM_LINGUISTIC_CASING | NORM_IGNORECASE,
      {'I',0},   {0x131,0}, CSTR_EQUAL,        CSTR_LESS_THAN
    },
    { /* 29 */
      "tr-TR", NORM_LINGUISTIC_CASING | NORM_IGNORECASE,
      {0x130,0}, {0x131,0}, CSTR_GREATER_THAN, -1
    },
    /* with NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE */
    { /* 30 */
      "tr-TR", NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE,
      {'i',0},   {'I',0},   CSTR_GREATER_THAN, CSTR_EQUAL
    },
    { /* 31 */
      "tr-TR", NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE,
      {'i',0},   {0x130,0}, CSTR_EQUAL,        CSTR_LESS_THAN
    },
    { /* 32 */
      "tr-TR", NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE,
      {'i',0},   {0x131,0}, CSTR_GREATER_THAN, CSTR_LESS_THAN
    },
    { /* 33 */
      "tr-TR", NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE,
      {'I',0},   {0x130,0}, CSTR_LESS_THAN,    -1
    },
    { /* 34 */
      "tr-TR", NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE,
      {'I',0},   {0x131,0}, CSTR_EQUAL,        CSTR_LESS_THAN
    },
    { /* 35 */
      "tr-TR", NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE,
      {0x130,0}, {0x131,0}, CSTR_GREATER_THAN, CSTR_LESS_THAN
    }
};

static void test_CompareStringEx(void)
{
    const char *op[] = {"ERROR", "CSTR_LESS_THAN", "CSTR_EQUAL", "CSTR_GREATER_THAN"};
    WCHAR locale[6];
    INT ret, i;

    /* CompareStringEx is only available on Vista+ */
    if (!pCompareStringEx)
    {
        win_skip("CompareStringEx not supported\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(comparestringex_tests); i++)
    {
        const struct comparestringex_test *e = &comparestringex_tests[i];

        MultiByteToWideChar(CP_ACP, 0, e->locale, -1, locale, ARRAY_SIZE(locale));
        ret = pCompareStringEx(locale, e->flags, e->first, -1, e->second, -1, NULL, NULL, 0);
        ok(ret == e->ret || broken(ret == e->broken),
           "%d: got %s, expected %s\n", i, op[ret], op[e->ret]);
    }

}

static const DWORD lcmap_invalid_flags[] = {
    0,
    LCMAP_HIRAGANA | LCMAP_KATAKANA,
    LCMAP_HALFWIDTH | LCMAP_FULLWIDTH,
    LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE,
    LCMAP_LOWERCASE | SORT_STRINGSORT,
    LCMAP_UPPERCASE | NORM_IGNORESYMBOLS,
    LCMAP_LOWERCASE | NORM_IGNORESYMBOLS,
    LCMAP_UPPERCASE | NORM_IGNORENONSPACE,
    LCMAP_LOWERCASE | NORM_IGNORENONSPACE,
    LCMAP_HIRAGANA | NORM_IGNORENONSPACE,
    LCMAP_HIRAGANA | NORM_IGNORESYMBOLS,
    LCMAP_HIRAGANA | LCMAP_SIMPLIFIED_CHINESE,
    LCMAP_HIRAGANA | LCMAP_TRADITIONAL_CHINESE,
    LCMAP_KATAKANA | NORM_IGNORENONSPACE,
    LCMAP_KATAKANA | NORM_IGNORESYMBOLS,
    LCMAP_KATAKANA | LCMAP_SIMPLIFIED_CHINESE,
    LCMAP_KATAKANA | LCMAP_TRADITIONAL_CHINESE,
    LCMAP_FULLWIDTH | NORM_IGNORENONSPACE,
    LCMAP_FULLWIDTH | NORM_IGNORESYMBOLS,
    LCMAP_FULLWIDTH | LCMAP_SIMPLIFIED_CHINESE,
    LCMAP_FULLWIDTH | LCMAP_TRADITIONAL_CHINESE,
    LCMAP_HALFWIDTH | NORM_IGNORENONSPACE,
    LCMAP_HALFWIDTH | NORM_IGNORESYMBOLS,
    LCMAP_HALFWIDTH | LCMAP_SIMPLIFIED_CHINESE,
    LCMAP_HALFWIDTH | LCMAP_TRADITIONAL_CHINESE,
};

static void test_LCMapStringA(void)
{
    int ret, ret2, i;
    char buf[256], buf2[256];
    static const char upper_case[] = "\tJUST! A, TEST; STRING 1/*+-.\r\n";
    static const char lower_case[] = "\tjust! a, test; string 1/*+-.\r\n";
    static const char symbols_stripped[] = "justateststring1";

    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LOCALE_USE_CP_ACP | LCMAP_LOWERCASE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenA(lower_case) + 1);
    ok(!memcmp(buf, lower_case, ret), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | LCMAP_UPPERCASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_LOWERCASE and LCMAP_UPPERCASE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    /* test invalid flag combinations */
    for (i = 0; i < ARRAY_SIZE(lcmap_invalid_flags); i++) {
        lstrcpyA(buf, "foo");
        SetLastError(0xdeadbeef);
        ret = LCMapStringA(LOCALE_USER_DEFAULT, lcmap_invalid_flags[i],
                           lower_case, -1, buf, sizeof(buf));
        ok(GetLastError() == ERROR_INVALID_FLAGS,
           "LCMapStringA (flag %08lx) unexpected error code %ld\n",
           lcmap_invalid_flags[i], GetLastError());
        ok(!ret, "LCMapStringA (flag %08lx) should return 0, got %d\n",
           lcmap_invalid_flags[i], ret);
    }

    /* test LCMAP_LOWERCASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(upper_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenA(upper_case) + 1);
    ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test LCMAP_UPPERCASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1,
       "ret %d, error %ld, expected value %d\n",
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
           "ret %d, error %ld, expected value %d\n",
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
           "ret %d, error %ld, expected value %d\n",
           ret, GetLastError(), lstrlenA(lower_case) + 1);
        ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);
    }

    /* otherwise src == dst should fail */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | LCMAP_UPPERCASE,
                       buf, 10, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win9x */,
       "unexpected error code %ld\n", GetLastError());
    ok(!ret, "src == dst without LCMAP_UPPERCASE or LCMAP_LOWERCASE must fail\n");

    /* test whether '\0' is always appended */
    memset(buf, 0xff, sizeof(buf));
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ok(buf[ret-1] == 0, "LCMapStringA not null-terminated\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, lstrlenA(upper_case), buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(buf2[ret2-1] == 0, "LCMapStringA not null-terminated\n" );
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!memcmp(buf, buf2, ret), "sort keys must be equal\n");

    /* test we get the same length when no dest buffer is provided */
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, lstrlenA(upper_case), NULL, 0);
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal (%d vs %d)\n", ret, ret2);

    /* test LCMAP_SORTKEY | NORM_IGNORECASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORECASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!memcmp(buf, buf2, ret), "sort keys must be equal\n");

    /* Don't test LCMAP_SORTKEY | NORM_IGNORENONSPACE, produces different
       results from plain LCMAP_SORTKEY on Vista */

    /* test LCMAP_SORTKEY | NORM_IGNORESYMBOLS */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       symbols_stripped, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!memcmp(buf, buf2, ret), "sort keys must be equal\n");

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

    /* test NORM_IGNORESYMBOLS | NORM_IGNORENONSPACE */
    lstrcpyA(buf, "foo");
    ret = LCMapStringA(LOCALE_USER_DEFAULT, NORM_IGNORESYMBOLS | NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(symbols_stripped) + 1, "LCMapStringA should return %d, ret = %d\n",
	lstrlenA(symbols_stripped) + 1, ret);
    ok(!lstrcmpA(buf, symbols_stripped), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test srclen = 0 */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, 0, upper_case, 0, buf, sizeof(buf));
    ok(!ret, "LCMapStringA should fail with srclen = 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "unexpected error code %ld\n", GetLastError());
}

typedef INT (*lcmapstring_wrapper)(DWORD, LPCWSTR, INT, LPWSTR, INT);

static void test_lcmapstring_unicode(lcmapstring_wrapper func_ptr, const char *func_name)
{
    const static WCHAR japanese_text[] = {
        0x3044, 0x309d, 0x3084, 0x3001, 0x30a4, 0x30fc, 0x30cf, 0x30c8,
        0x30fc, 0x30f4, 0x30a9, 0x306e, 0x91ce, 0x539f, 0x306f, 0x5e83,
        0x3044, 0x3093, 0x3060, 0x3088, 0x3002, 0
    };
    const static WCHAR hiragana_text[] = {
        0x3044, 0x309d, 0x3084, 0x3001, 0x3044, 0x30fc, 0x306f, 0x3068,
        0x30fc, 0x3094, 0x3049, 0x306e, 0x91ce, 0x539f, 0x306f, 0x5e83,
        0x3044, 0x3093, 0x3060, 0x3088, 0x3002, 0
    };
    const static WCHAR katakana_text[] = {
        0x30a4, 0x30fd, 0x30e4, 0x3001, 0x30a4, 0x30fc, 0x30cf, 0x30c8,
        0x30fc, 0x30f4, 0x30a9, 0x30ce, 0x91ce, 0x539f, 0x30cf, 0x5e83,
        0x30a4, 0x30f3, 0x30c0, 0x30e8, 0x3002, 0
    };
    const static WCHAR halfwidth_text[] = {
        0x3044, 0x309d, 0x3084, 0xff64, 0xff72, 0xff70, 0xff8a, 0xff84,
        0xff70, 0xff73, 0xff9e, 0xff6b, 0x306e, 0x91ce, 0x539f, 0x306f,
        0x5e83, 0x3044, 0x3093, 0x3060, 0x3088, 0xff61, 0
    };
    const static WCHAR halfwidth_text2[] = {
        0xff72, 0x30fd, 0xff94, 0xff64, 0xff72, 0xff70, 0xff8a, 0xff84,
        0xff70, 0xff73, 0xff9e, 0xff6b, 0xff89, 0x91ce, 0x539f, 0xff8a,
        0x5e83, 0xff72, 0xff9d, 0xff80, 0xff9e, 0xff96, 0xff61, 0
    };
    const static WCHAR math_text[] = {
        0xd835, 0xdc00, 0xd835, 0xdc01, 0xd835, 0xdc02, 0xd835, 0xdc03,
        0xd835, 0xdd52, 0xd835, 0xdd53, 0xd835, 0xdd54, 0xd835, 0xdd55, 0
    };
    const static WCHAR math_result[] = L"ABCDabcd";
    const static WCHAR math_arabic_text[] = {
        0xd83b, 0xde00, 0xd83b, 0xde01, 0xd83b, 0xde02, 0xd83b, 0xde03,
        0xd83b, 0xde10, 0xd83b, 0xde11, 0xd83b, 0xde12, 0xd83b, 0xde13, 0
    };
    const static WCHAR math_arabic_result[] = {
        0x0627, 0x0628, 0x062c, 0x062f, 0x0641, 0x0635, 0x0642, 0x0631, 0
    };
    const static WCHAR cjk_compat_text[] = {
        0xd87e, 0xdc20, 0xd87e, 0xdc21, 0xd87e, 0xdc22, 0xd87e, 0xdc23,
        0xd87e, 0xdc24, 0xd87e, 0xdc25, 0xd87e, 0xdc26, 0xd87e, 0xdc27, 0
    };
    const static WCHAR cjk_compat_result[] = {
        0x523b, 0x5246, 0x5272, 0x5277, 0x3515, 0x52c7, 0x52c9, 0x52e4, 0
    };
    const static WCHAR accents_text[] = {
        0x00e0, 0x00e7, ' ', 0x00e9, ',', 0x00ee, 0x00f1, '/', 0x00f6, 0x00fb, 0x00fd, '!', 0
    };
    const static WCHAR accents_result[] = L"ac e,in/ouy!";
    const static WCHAR accents_result2[] = L"aceinouy";
    int ret, ret2, i;
    WCHAR buf[256], buf2[256];
    char *p_buf = (char *)buf, *p_buf2 = (char *)buf2;

    /* LCMAP_LOWERCASE | LCMAP_UPPERCASE makes LCMAP_TITLECASE, so it's valid now. */
    ret = func_ptr(LCMAP_LOWERCASE | LCMAP_UPPERCASE, lower_case, -1, buf, ARRAY_SIZE(buf));
    todo_wine ok(ret == lstrlenW(title_case) + 1 || broken(!ret),
       "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(title_case) + 1);
    todo_wine ok(lstrcmpW(buf, title_case) == 0 || broken(!ret),
       "Expected title case string\n");

    /* test invalid flag combinations */
    for (i = 0; i < ARRAY_SIZE(lcmap_invalid_flags); i++) {
        lstrcpyW(buf, fooW);
        SetLastError(0xdeadbeef);
        ret = func_ptr(lcmap_invalid_flags[i],
                           lower_case, -1, buf, sizeof(buf));
        ok(GetLastError() == ERROR_INVALID_FLAGS,
           "%s (flag %08lx) unexpected error code %ld\n",
           func_name, lcmap_invalid_flags[i], GetLastError());
        ok(!ret, "%s (flag %08lx) should return 0, got %d\n",
           func_name, lcmap_invalid_flags[i], ret);
    }

    /* test LCMAP_LOWERCASE */
    ret = func_ptr(LCMAP_LOWERCASE, upper_case, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(upper_case) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(upper_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "%s string compare mismatch\n", func_name);

    /* test LCMAP_UPPERCASE */
    ret = func_ptr(LCMAP_UPPERCASE, lower_case, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(lower_case) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, upper_case), "%s string compare mismatch\n", func_name);

    /* test LCMAP_HIRAGANA */
    ret = func_ptr(LCMAP_HIRAGANA, japanese_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(hiragana_text) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(hiragana_text) + 1);
    ok(!lstrcmpW(buf, hiragana_text), "%s string compare mismatch\n", func_name);

    buf[0] = 0x30f5; /* KATAKANA LETTER SMALL KA */
    ret = func_ptr(LCMAP_HIRAGANA, buf, 1, buf2, 1);
    ok(ret == 1, "%s ret %d, error %ld, expected value 1\n", func_name,
       ret, GetLastError());
    /* U+3095: HIRAGANA LETTER SMALL KA was added in Unicode 3.2 */
    ok(buf2[0] == 0x3095 || broken(buf2[0] == 0x30f5 /* Vista and earlier versions */),
       "%s expected %04x, got %04x\n", func_name, 0x3095, buf2[0]);

    /* test LCMAP_KATAKANA | LCMAP_LOWERCASE */
    ret = func_ptr(LCMAP_KATAKANA | LCMAP_LOWERCASE, japanese_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(katakana_text) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(katakana_text) + 1);
    ok(!lstrcmpW(buf, katakana_text), "%s string compare mismatch\n", func_name);

    /* test LCMAP_FULLWIDTH */
    ret = func_ptr(LCMAP_FULLWIDTH, halfwidth_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(japanese_text) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(japanese_text) + 1);
    ok(!lstrcmpW(buf, japanese_text), "%s string compare mismatch\n", func_name);

    ret2 = func_ptr(LCMAP_FULLWIDTH, halfwidth_text, -1, NULL, 0);
    ok(ret == ret2, "%s ret %d, expected value %d\n", func_name, ret2, ret);

    ret = func_ptr(LCMAP_FULLWIDTH, math_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(buf) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(buf) + 1);
    ok(!lstrcmpW(buf, math_result), "%s string compare mismatch %s\n", func_name, debugstr_w(buf));

    ret = func_ptr(LCMAP_FULLWIDTH, math_arabic_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(buf) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(buf) + 1);
    ok(!lstrcmpW(buf, math_arabic_result) ||
       broken(!lstrcmpW( buf, math_arabic_text )), /* win7 */
       "%s string compare mismatch %s\n", func_name, debugstr_w(buf));

    ret = func_ptr(LCMAP_FULLWIDTH, cjk_compat_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(buf) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(buf) + 1);
    ok(!lstrcmpW(buf, cjk_compat_result), "%s string compare mismatch %s\n", func_name, debugstr_w(buf));

    /* test LCMAP_FULLWIDTH | LCMAP_HIRAGANA
       (half-width katakana is converted into full-width hiragana) */
    ret = func_ptr(LCMAP_FULLWIDTH | LCMAP_HIRAGANA, halfwidth_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(hiragana_text) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(hiragana_text) + 1);
    ok(!lstrcmpW(buf, hiragana_text), "%s string compare mismatch\n", func_name);

    ret2 = func_ptr(LCMAP_FULLWIDTH | LCMAP_HIRAGANA, halfwidth_text, -1, NULL, 0);
    ok(ret == ret2, "%s ret %d, expected value %d\n", func_name, ret, ret2);

    /* LCMAP_FULLWIDTH | LCMAP_KATAKANA maps to full-width first */
    ret = func_ptr(LCMAP_FULLWIDTH | LCMAP_KATAKANA, halfwidth_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(katakana_text) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(katakana_text) + 1);
    ok(!lstrcmpW(buf, katakana_text), "%s string compare mismatch\n", func_name);

    /* test LCMAP_HALFWIDTH */
    ret = func_ptr(LCMAP_HALFWIDTH, japanese_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(halfwidth_text) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(halfwidth_text) + 1);
    ok(!lstrcmpW(buf, halfwidth_text), "%s string compare mismatch\n", func_name);

    ret2 = func_ptr(LCMAP_HALFWIDTH, japanese_text, -1, NULL, 0);
    ok(ret == ret2, "%s ret %d, expected value %d\n", func_name, ret, ret2);

    /* test LCMAP_HALFWIDTH | LCMAP_KATAKANA
       (hiragana character is converted into half-width katakana) */
    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_KATAKANA, japanese_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(halfwidth_text2) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(halfwidth_text2) + 1);
    ok(!lstrcmpW(buf, halfwidth_text2), "%s string compare mismatch\n", func_name);

    ret2 = func_ptr(LCMAP_HALFWIDTH | LCMAP_KATAKANA, japanese_text, -1, NULL, 0);
    ok(ret == ret2, "%s ret %d, expected value %d\n", func_name, ret, ret2);

    /* LCMAP_HALFWIDTH | LCMAP_HIRAGANA maps to Hiragana first */
    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_HIRAGANA, japanese_text, -1, buf, ARRAY_SIZE(buf));
    ret2 = func_ptr(LCMAP_HALFWIDTH, hiragana_text, -1, buf2, ARRAY_SIZE(buf2));
    ok(ret == ret2, "%s ret %d, expected value %d\n", func_name, ret, ret2);
    ok(!lstrcmpW(buf, buf2), "%s string compare mismatch %s vs %s\n", func_name, debugstr_w(buf), debugstr_w(buf2));

    /* test buffer overflow */
    SetLastError(0xdeadbeef);
    ret = func_ptr(LCMAP_UPPERCASE,
                       lower_case, -1, buf, 4);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "%s should return 0 and ERROR_INSUFFICIENT_BUFFER, got %d\n", func_name, ret);

    /* KATAKANA LETTER GA (U+30AC) is converted into two half-width characters.
       Thus, it requires two WCHARs. */
    buf[0] = 0x30ac;
    SetLastError(0xdeadbeef);
    ret = func_ptr(LCMAP_HALFWIDTH, buf, 1, buf2, 1);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "%s should return 0 and ERROR_INSUFFICIENT_BUFFER, got %d\n", func_name, ret);

    buf[0] = 'a';
    buf[1] = 0x30ac;
    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_UPPERCASE, buf, 2, buf2, 0);
    ok(ret == 3, "%s ret %d, expected value 3\n", func_name, ret);

    SetLastError(0xdeadbeef);
    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_UPPERCASE, buf, 2, buf2, 1);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "%s should return 0 and ERROR_INSUFFICIENT_BUFFER, got %d\n", func_name, ret);

    SetLastError(0xdeadbeef);
    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_UPPERCASE, buf, 2, buf2, 2);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "%s should return 0 and ERROR_INSUFFICIENT_BUFFER, got %d\n", func_name, ret);

    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_UPPERCASE, buf, 2, buf2, 3);
    ok(ret == 3, "%s ret %d, expected value 3\n", func_name, ret);

    ret = func_ptr(LCMAP_HALFWIDTH | LCMAP_UPPERCASE, buf, 2, buf2, 4);
    ok(ret == 3, "%s ret %d, expected value 3\n", func_name, ret);

    /* LCMAP_UPPERCASE or LCMAP_LOWERCASE should accept src == dst */
    lstrcpyW(buf, lower_case);
    ret = func_ptr(LCMAP_UPPERCASE, buf, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(lower_case) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, upper_case), "%s string compare mismatch\n", func_name);

    lstrcpyW(buf, upper_case);
    ret = func_ptr(LCMAP_LOWERCASE, buf, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(upper_case) + 1, "%s ret %d, error %ld, expected value %d\n", func_name,
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "%s string compare mismatch\n", func_name);

    /* otherwise src == dst should fail */
    SetLastError(0xdeadbeef);
    ret = func_ptr(LCMAP_SORTKEY | LCMAP_UPPERCASE,
                       buf, 10, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win7+ */,
       "%s unexpected error code %ld\n", func_name, GetLastError());
    ok(!ret, "%s src == dst without LCMAP_UPPERCASE or LCMAP_LOWERCASE must fail\n", func_name);

    /* test whether '\0' is always appended */
    ret = func_ptr(LCMAP_SORTKEY,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "%s func_ptr must succeed\n", func_name);
    ret2 = func_ptr(LCMAP_SORTKEY,
                       upper_case, lstrlenW(upper_case), buf2, sizeof(buf2));
    ok(ret, "%s func_ptr must succeed\n", func_name);
    ok(ret == ret2, "%s lengths of sort keys must be equal\n", func_name);
    ok(!memcmp(p_buf, p_buf2, ret), "%s sort keys must be equal\n", func_name);

    /* test contents with short buffer */
    memset( buf, 0xcc, sizeof(buf) );
    ret = func_ptr(LCMAP_SORTKEY, upper_case, -1, buf, ret - 1);
    ok( !ret, "%s succeeded with %u\n", func_name, ret );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "%s wrong error %lu\n", func_name, GetLastError() );
    ret = (char *)memchr( p_buf, 0xcc, sizeof(buf) ) - p_buf;
    ok( ret, "%s buffer not filled\n", func_name );
    ok( p_buf[ret - 1] == 0x01, "%s buffer filled up to %02x\n", func_name, (BYTE)p_buf[ret - 1] );
    ok( !memcmp( p_buf, p_buf2, ret - 1 ), "%s buffers differ\n", func_name );

    memset( buf, 0xcc, sizeof(buf) );
    ret = func_ptr(LCMAP_SORTKEY, upper_case, -1, buf, ret2 - 20);
    ok( !ret, "%s succeeded with %u\n", func_name, ret );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "%s wrong error %lu\n", func_name, GetLastError() );
    ret = (char *)memchr( p_buf, 0xcc, sizeof(buf) ) - p_buf;
    ok( ret, "%s buffer not filled\n", func_name );
    ok( p_buf[ret - 1] == 0x01, "%s buffer filled up to %02x\n", func_name, (BYTE)p_buf[ret - 1] );
    ok( !memcmp( p_buf, p_buf2, ret - 1 ), "%s buffers differ\n", func_name );

    /* test LCMAP_SORTKEY | NORM_IGNORECASE */
    ret = func_ptr(LCMAP_SORTKEY | NORM_IGNORECASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "%s func_ptr must succeed\n", func_name);
    ret2 = func_ptr(LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "%s func_ptr must succeed\n", func_name);
    ok(ret == ret2, "%s lengths of sort keys must be equal\n", func_name);
    ok(!memcmp(p_buf, p_buf2, ret), "%s sort keys must be equal\n", func_name);

    /* Don't test LCMAP_SORTKEY | NORM_IGNORENONSPACE, produces different
       results from plain LCMAP_SORTKEY on Vista */

    /* test LCMAP_SORTKEY | NORM_IGNORESYMBOLS */
    ret = func_ptr(LCMAP_SORTKEY | NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "%s func_ptr must succeed\n", func_name);
    ret2 = func_ptr(LCMAP_SORTKEY,
                       symbols_stripped, -1, buf2, sizeof(buf2));
    ok(ret2, "%s func_ptr must succeed\n", func_name);
    ok(ret == ret2, "%s lengths of sort keys must be equal\n", func_name);
    ok(!memcmp(p_buf, p_buf2, ret), "%s sort keys must be equal\n", func_name);

    /* test NORM_IGNORENONSPACE */
    lstrcpyW(buf, fooW);
    ret = func_ptr(NORM_IGNORENONSPACE, lower_case, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(lower_case) + 1, "%s func_ptr should return %d, ret = %d\n", func_name,
    lstrlenW(lower_case) + 1, ret);
    ok(!lstrcmpW(buf, lower_case), "%s string comparison mismatch\n", func_name);

    lstrcpyW(buf, fooW);
    ret = func_ptr(NORM_IGNORENONSPACE, accents_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(buf) + 1, "%s func_ptr should return %d, ret = %d\n", func_name,
       lstrlenW(buf) + 1, ret);
    ok(!lstrcmpW(buf, accents_result), "%s string comparison mismatch %s\n", func_name, debugstr_w(buf));

    /* test NORM_IGNORESYMBOLS */
    lstrcpyW(buf, fooW);
    ret = func_ptr(NORM_IGNORESYMBOLS, lower_case, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(symbols_stripped) + 1, "%s func_ptr should return %d, ret = %d\n", func_name,
    lstrlenW(symbols_stripped) + 1, ret);
    ok(!lstrcmpW(buf, symbols_stripped), "%s string comparison mismatch\n", func_name);

    /* test NORM_IGNORESYMBOLS | NORM_IGNORENONSPACE */
    lstrcpyW(buf, fooW);
    ret = func_ptr(NORM_IGNORESYMBOLS | NORM_IGNORENONSPACE, lower_case, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(symbols_stripped) + 1, "%s func_ptr should return %d, ret = %d\n", func_name,
    lstrlenW(symbols_stripped) + 1, ret);
    ok(!lstrcmpW(buf, symbols_stripped), "%s string comparison mismatch\n", func_name);

    lstrcpyW(buf, fooW);
    ret = func_ptr(NORM_IGNORESYMBOLS | NORM_IGNORENONSPACE, accents_text, -1, buf, ARRAY_SIZE(buf));
    ok(ret == lstrlenW(buf) + 1, "%s func_ptr should return %d, ret = %d\n", func_name,
       lstrlenW(buf) + 1, ret);
    ok(!lstrcmpW(buf, accents_result2), "%s string comparison mismatch %s\n", func_name, debugstr_w(buf));

    /* test small buffer */
    lstrcpyW(buf, fooW);
    ret = func_ptr(LCMAP_SORTKEY, lower_case, -1, buf, 2);
    ok(ret == 0, "Expected a failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
           "%s unexpected error code %ld\n", func_name, GetLastError());

    /* test srclen = 0 */
    SetLastError(0xdeadbeef);
    ret = func_ptr(0, upper_case, 0, buf, ARRAY_SIZE(buf));
    ok(!ret, "%s func_ptr should fail with srclen = 0\n", func_name);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "%s unexpected error code %ld\n", func_name, GetLastError());
}

static INT LCMapStringW_wrapper(DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen)
{
    return LCMapStringW(LOCALE_USER_DEFAULT, flags, src, srclen, dst, dstlen);
}

static void test_LCMapStringW(void)
{
    int ret;
    WCHAR buf[256];

    trace("testing LCMapStringW\n");

    SetLastError(0xdeadbeef);
    ret = LCMapStringW((LCID)-1, LCMAP_LOWERCASE, upper_case, -1, buf, ARRAY_SIZE(buf));
    ok(!ret, "LCMapStringW should fail with bad lcid\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "unexpected error code %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = LCMapStringW((LCID)0xdead, LCMAP_HIRAGANA, upper_case, -1, buf, ARRAY_SIZE(buf));
    ok(!ret, "LCMapStringW should fail with bad lcid\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "unexpected error code %ld\n", GetLastError());

    test_lcmapstring_unicode(LCMapStringW_wrapper, "LCMapStringW:");
}

static INT LCMapStringEx_wrapper(DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen)
{
    return pLCMapStringEx(LOCALE_NAME_USER_DEFAULT, flags, src, srclen, dst, dstlen, NULL, NULL, 0);
}

static void test_LCMapStringEx(void)
{
    int ret;
    WCHAR buf[256];

    if (!pLCMapStringEx)
    {
        win_skip( "LCMapStringEx not available\n" );
        return;
    }

    trace("testing LCMapStringEx\n");

    SetLastError(0xdeadbeef);
    ret = pLCMapStringEx(invalidW, LCMAP_LOWERCASE,
                         upper_case, -1, buf, ARRAY_SIZE(buf), NULL, NULL, 0);
    ok(!ret, "LCMapStringEx should fail with bad locale name\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "unexpected error code %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pLCMapStringEx(invalidW, LCMAP_HIRAGANA,
                         upper_case, -1, buf, ARRAY_SIZE(buf), NULL, NULL, 0);
    ok(ret, "LCMapStringEx should not fail with bad locale name\n");

    /* test reserved parameters */
    ret = pLCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE,
                         upper_case, -1, buf, ARRAY_SIZE(buf), NULL, NULL, 1);
    ok(ret == lstrlenW(upper_case) + 1, "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenW(upper_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "string compare mismatch\n");

    ret = pLCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE,
                         upper_case, -1, buf, ARRAY_SIZE(buf), NULL, (void*)1, 0);
    ok(ret == lstrlenW(upper_case) + 1, "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenW(upper_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "string compare mismatch\n");

    /* crashes on native */
    if(0)
        ret = pLCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE,
                             upper_case, -1, buf, ARRAY_SIZE(buf), (void*)1, NULL, 0);

    test_lcmapstring_unicode(LCMapStringEx_wrapper, "LCMapStringEx:");
}

struct neutralsublang_name_t {
    WCHAR name[3];
    WCHAR sname[16];
    LCID lcid;
};

static const struct neutralsublang_name_t neutralsublang_names[] = {
    { {'a','r',0}, {'a','r','-','S','A',0}, MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA), SORT_DEFAULT) },
    { {'a','z',0}, {'a','z','-','L','a','t','n','-','A','Z',0}, MAKELCID(MAKELANGID(LANG_AZERI, SUBLANG_AZERI_LATIN), SORT_DEFAULT) },
    { {'d','e',0}, {'d','e','-','D','E',0}, MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT) },
    { {'e','n',0}, {'e','n','-','U','S',0}, MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT) },
    { {'e','s',0}, {'e','s','-','E','S',0}, MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT) },
    { {'g','a',0}, {'g','a','-','I','E',0}, MAKELCID(MAKELANGID(LANG_IRISH, SUBLANG_IRISH_IRELAND), SORT_DEFAULT) },
    { {'i','t',0}, {'i','t','-','I','T',0}, MAKELCID(MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN), SORT_DEFAULT) },
    { {'m','s',0}, {'m','s','-','M','Y',0}, MAKELCID(MAKELANGID(LANG_MALAY, SUBLANG_MALAY_MALAYSIA), SORT_DEFAULT) },
    { {'n','l',0}, {'n','l','-','N','L',0}, MAKELCID(MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH), SORT_DEFAULT) },
    { {'p','t',0}, {'p','t','-','B','R',0}, MAKELCID(MAKELANGID(LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN), SORT_DEFAULT) },
    { {'s','r',0}, {'s','r','-','L','a','t','n','-','R','S',0}, MAKELCID(MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_LATIN), SORT_DEFAULT) },
    { {'s','v',0}, {'s','v','-','S','E',0}, MAKELCID(MAKELANGID(LANG_SWEDISH, SUBLANG_SWEDISH), SORT_DEFAULT) },
    { {'u','z',0}, {'u','z','-','L','a','t','n','-','U','Z',0}, MAKELCID(MAKELANGID(LANG_UZBEK, SUBLANG_UZBEK_LATIN), SORT_DEFAULT) },
    { {'z','h',0}, {'z','h','-','C','N',0}, MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT) },
    { {0} }
};

static void test_LocaleNameToLCID(void)
{
    LCID lcid, expect;
    NTSTATUS status;
    INT ret;
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
    WCHAR expbuff[LOCALE_NAME_MAX_LENGTH];
    const struct neutralsublang_name_t *ptr;

    if (!pLocaleNameToLCID)
    {
        win_skip( "LocaleNameToLCID not available\n" );
        return;
    }

    /* special cases */
    buffer[0] = 0;
    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(LOCALE_NAME_USER_DEFAULT, 0);
    ok(lcid == GetUserDefaultLCID() || broken(GetLastError() == ERROR_INVALID_PARAMETER /* Vista */),
       "Expected lcid == %08lx, got %08lx, error %ld\n", GetUserDefaultLCID(), lcid, GetLastError());
    ret = pLCIDToLocaleName(lcid, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    ok(ret > 0, "Expected ret > 0, got %d, error %ld\n", ret, GetLastError());
    trace("%08lx, %s\n", lcid, wine_dbgstr_w(buffer));

    buffer[0] = 0;
    SetLastError(0xdeadbeef);
#ifndef __REACTOS__
    lcid = LocaleNameToLCID(LOCALE_NAME_SYSTEM_DEFAULT, 0);
    expect = GetSystemDefaultLCID();
    ok(lcid == expect, "Expected lcid == %08lx, got %08lx, error %ld\n", expect, lcid, GetLastError());
#endif
    ret = pLCIDToLocaleName(lcid, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    ok(ret > 0, "Expected ret > 0, got %d, error %ld\n", ret, GetLastError());
    trace("%08lx, %s\n", lcid, wine_dbgstr_w(buffer));

    buffer[0] = 0;
    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(LOCALE_NAME_INVARIANT, 0);
    ok(lcid == 0x7F, "Expected lcid = 0x7F, got %08lx, error %ld\n", lcid, GetLastError());
    ret = pLCIDToLocaleName(lcid, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    ok(ret > 0, "Expected ret > 0, got %d, error %ld\n", ret, GetLastError());
    trace("%08lx, %s\n", lcid, wine_dbgstr_w(buffer));

    pLCIDToLocaleName(GetUserDefaultLCID(), expbuff, LOCALE_NAME_MAX_LENGTH, 0);
    ret = pLCIDToLocaleName(LOCALE_NEUTRAL, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    ok(ret > 0, "Expected ret > 0, got %d, error %ld\n", ret, GetLastError());
    ok( !wcscmp( buffer, expbuff ), "got %s / %s\n", debugstr_w(buffer), debugstr_w(expbuff));

    ret = pLCIDToLocaleName(LOCALE_CUSTOM_DEFAULT, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    ok(ret > 0, "Expected ret > 0, got %d, error %ld\n", ret, GetLastError());
    ok( !wcscmp( buffer, expbuff ), "got %s / %s\n", debugstr_w(buffer), debugstr_w(expbuff));

    SetLastError( 0xdeadbeef );
    ret = pLCIDToLocaleName(LOCALE_CUSTOM_UNSPECIFIED, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    ok(ret > 0 || broken(!ret), /* <= win8 */ "Expected ret > 0, got %d, error %ld\n", ret, GetLastError());
    if (ret) ok( !wcscmp( buffer, expbuff ), "got %s / %s\n", debugstr_w(buffer), debugstr_w(expbuff));

    SetLastError( 0xdeadbeef );
    ret = pLCIDToLocaleName(LOCALE_CUSTOM_UI_DEFAULT, buffer, LOCALE_NAME_MAX_LENGTH, 0);
    if (ret) trace("%08x, %s\n", GetUserDefaultUILanguage(), wine_dbgstr_w(buffer));
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError());

    /* bad name */
    SetLastError(0xdeadbeef);
    lcid = pLocaleNameToLCID(invalidW, 0);
    ok(!lcid && GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected lcid == 0, got %08lx, error %ld\n", lcid, GetLastError());

    /* lower-case */
    lcid = pLocaleNameToLCID(L"es-es", 0);
    ok(lcid == MAKELCID(MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT), "Got wrong lcid for es-es: 0x%lx\n", lcid);

    /* english neutral name */
    lcid = pLocaleNameToLCID(L"en", LOCALE_ALLOW_NEUTRAL_NAMES);
    ok(lcid == MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), SORT_DEFAULT) ||
       broken(lcid == 0) /* Vista */, "got 0x%04lx\n", lcid);
    lcid = pLocaleNameToLCID(L"en", 0);
    ok(lcid == MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT) ||
       broken(lcid == 0) /* Vista */, "got 0x%04lx\n", lcid);
    if (lcid)
    {
        for (ptr = neutralsublang_names; *ptr->name; ptr++)
        {
            lcid = pLocaleNameToLCID(ptr->name, 0);
            ok(lcid == ptr->lcid, "%s: got wrong lcid 0x%04lx, expected 0x%04lx\n",
                wine_dbgstr_w(ptr->name), lcid, ptr->lcid);

            *buffer = 0;
            ret = pLCIDToLocaleName(lcid, buffer, ARRAY_SIZE(buffer), 0);
            ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(ptr->name), ret);
            ok(!lstrcmpW(ptr->sname, buffer), "%s: got wrong locale name %s\n",
                wine_dbgstr_w(ptr->name), wine_dbgstr_w(buffer));

        }

        /* zh-Hant has LCID 0x7c04, but LocaleNameToLCID actually returns 0x0c04, which is the LCID of zh-HK */
        lcid = pLocaleNameToLCID(L"zh-Hant", 0);
        ok(lcid == MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG), SORT_DEFAULT),
           "%s: got wrong lcid 0x%04lx\n", wine_dbgstr_w(L"zh-Hant"), lcid);
        ret = pLCIDToLocaleName(lcid, buffer, ARRAY_SIZE(buffer), 0);
        ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(L"zh-Hant"), ret);
        ok(!lstrcmpW(L"zh-HK", buffer), "%s: got wrong locale name %s\n",
           wine_dbgstr_w(L"zh-Hant"), wine_dbgstr_w(buffer));
        /* check that 0x7c04 also works and is mapped to zh-HK */
        ret = pLCIDToLocaleName(MAKELANGID(LANG_CHINESE_TRADITIONAL, SUBLANG_CHINESE_TRADITIONAL),
                buffer, ARRAY_SIZE(buffer), 0);
        ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(L"zh-Hant"), ret);
        ok(!lstrcmpW(L"zh-HK", buffer), "%s: got wrong locale name %s\n",
           wine_dbgstr_w(L"zh-Hant"), wine_dbgstr_w(buffer));

        /* zh-hant */
        lcid = pLocaleNameToLCID(L"zh-hant", 0);
        ok(lcid == MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_HONGKONG), SORT_DEFAULT),
           "%s: got wrong lcid 0x%04lx\n", wine_dbgstr_w(L"zh-hant"), lcid);
        ret = pLCIDToLocaleName(lcid, buffer, ARRAY_SIZE(buffer), 0);
        ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(L"zh-hant"), ret);
        ok(!lstrcmpW(L"zh-HK", buffer), "%s: got wrong locale name %s\n",
           wine_dbgstr_w(L"zh-hant"), wine_dbgstr_w(buffer));

        /* zh-Hans has LCID 0x0004, but LocaleNameToLCID actually returns 0x0804, which is the LCID of zh-CN */
        lcid = pLocaleNameToLCID(L"zh-Hans", 0);
        /* check that LocaleNameToLCID actually returns 0x0804 */
        ok(lcid == MAKELCID(MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT),
           "%s: got wrong lcid 0x%04lx\n", wine_dbgstr_w(L"zh-Hans"), lcid);
        ret = pLCIDToLocaleName(lcid, buffer, ARRAY_SIZE(buffer), 0);
        ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(L"zh-Hans"), ret);
        ok(!lstrcmpW(L"zh-CN", buffer), "%s: got wrong locale name %s\n",
           wine_dbgstr_w(L"zh-Hans"), wine_dbgstr_w(buffer));
        /* check that 0x0004 also works and is mapped to zh-CN */
        ret = pLCIDToLocaleName(MAKELANGID(LANG_CHINESE, SUBLANG_NEUTRAL), buffer, ARRAY_SIZE(buffer), 0);
        ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(L"zh-Hans"), ret);
        ok(!lstrcmpW(L"zh-CN", buffer), "%s: got wrong locale name %s\n",
           wine_dbgstr_w(L"zh-Hans"), wine_dbgstr_w(buffer));

        /* zh-hans */
        lcid = pLocaleNameToLCID(L"zh-hans", 0);
        ok(lcid == MAKELCID(MAKELANGID(LANG_CHINESE_SIMPLIFIED, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT),
           "%s: got wrong lcid 0x%04lx\n", wine_dbgstr_w(L"zh-hans"), lcid);
        ret = pLCIDToLocaleName(lcid, buffer, ARRAY_SIZE(buffer), 0);
        ok(ret > 0, "%s: got %d\n", wine_dbgstr_w(L"zh-hans"), ret);
        ok(!lstrcmpW(L"zh-CN", buffer), "%s: got wrong locale name %s\n",
           wine_dbgstr_w(L"zh-hans"), wine_dbgstr_w(buffer));

        /* de-DE_phoneb */
        lcid = pLocaleNameToLCID(L"de-DE_phoneb", 0);
        ok(lcid == MAKELCID(MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT), SORT_GERMAN_PHONE_BOOK),
           "%s: got wrong lcid 0x%04lx\n", wine_dbgstr_w(L"zh-hans"), lcid);
        ret = pLCIDToLocaleName(lcid, buffer, ARRAY_SIZE(buffer), 0);
        ok(!lstrcmpW(L"de-DE_phoneb", buffer), "got wrong locale name %s\n",
           wine_dbgstr_w(buffer));
    }

    if (pRtlLocaleNameToLcid)
    {
        status = pRtlLocaleNameToLcid( LOCALE_NAME_USER_DEFAULT, &lcid, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLocaleNameToLcid( LOCALE_NAME_SYSTEM_DEFAULT, &lcid, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLocaleNameToLcid( invalidW, &lcid, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );

        lcid = 0;
        status = pRtlLocaleNameToLcid( LOCALE_NAME_INVARIANT, &lcid, 0 );
        ok( !status, "failed error %lx\n", status );
        ok( lcid == LANG_INVARIANT, "got %08lx\n", lcid );

        lcid = 0;
        status = pRtlLocaleNameToLcid( localeW, &lcid, 0 );
        ok( !status, "failed error %lx\n", status );
        ok( lcid == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), "got %08lx\n", lcid );

        lcid = 0;
        status = pRtlLocaleNameToLcid( L"es-es", &lcid, 0 );
        ok( !status, "failed error %lx\n", status );
        ok( lcid == MAKELANGID(LANG_SPANISH, SUBLANG_SPANISH_MODERN), "got %08lx\n", lcid );

        lcid = 0;
        status = pRtlLocaleNameToLcid( L"de-DE_phoneb", &lcid, 0 );
        ok( !status, "failed error %lx\n", status );
        ok( lcid == 0x00010407, "got %08lx\n", lcid );

        lcid = 0;
        status = pRtlLocaleNameToLcid( L"DE_de-PHONEB", &lcid, 0 );
        ok( !status || broken( status == STATUS_INVALID_PARAMETER_1 ), "failed error %lx\n", status );
        if (!status) ok( lcid == 0x00010407, "got %08lx\n", lcid );

        lcid = 0xdeadbeef;
        status = pRtlLocaleNameToLcid( L"de+de+phoneb", &lcid, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "failed error %lx\n", status );
        ok( lcid == 0xdeadbeef, "got %08lx\n", lcid );

        lcid = 0;
        status = pRtlLocaleNameToLcid( L"en", &lcid, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLocaleNameToLcid( L"en", &lcid, 1 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLocaleNameToLcid( L"en", &lcid, 2 );
        ok( !status, "failed error %lx\n", status );
        ok( lcid == MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), "got %08lx\n", lcid );
        status = pRtlLocaleNameToLcid( L"en-RR", &lcid, 2 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLocaleNameToLcid( L"en-Latn-RR", &lcid, 2 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );

        for (ptr = neutralsublang_names; *ptr->name; ptr++)
        {
            switch (LANGIDFROMLCID(ptr->lcid))
            {
            case MAKELANGID( LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_LATIN): expect = LANG_SERBIAN_NEUTRAL; break;
            case MAKELANGID( LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_CYRILLIC): expect = 0x6c1a; break;
            case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED ): expect = 0x7804; break;
            case MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG ): expect = LANG_CHINESE_TRADITIONAL; break;
            default: expect = MAKELANGID( PRIMARYLANGID(ptr->lcid), SUBLANG_NEUTRAL ); break;
            }

            status = pRtlLocaleNameToLcid( ptr->name, &lcid, 2 );
            ok( !status || broken(ptr->lcid == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)), /* vista */
                "%s failed error %lx\n", wine_dbgstr_w(ptr->name), status );
            if (!status) ok( lcid == expect, "%s: got wrong lcid 0x%04lx, expected 0x%04lx\n",
                             wine_dbgstr_w(ptr->name), lcid, expect );
            status = pRtlLocaleNameToLcid( ptr->sname, &lcid, 0 );
            ok( !status || broken(ptr->lcid == MAKELANGID(LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_LATIN)), /* vista */
                "%s failed error %lx\n", wine_dbgstr_w(ptr->name), status );
            if (!status) ok( lcid == ptr->lcid, "%s: got wrong lcid 0x%04lx, expected 0x%04lx\n",
                             wine_dbgstr_w(ptr->name), lcid, ptr->lcid );
        }
    }
    else win_skip( "RtlLocaleNameToLcid not available\n" );

    if (pRtlLcidToLocaleName)
    {
#ifdef __REACTOS__
        WCHAR buffer[128];
#else
        WCHAR buffer[128], expect[128];
#endif
        UNICODE_STRING str;

        str.Buffer = buffer;
        str.MaximumLength = sizeof( buffer );
        memset( buffer, 0xcc, sizeof(buffer) );

        ok( !IsValidLocale( LOCALE_NEUTRAL, 0 ), "expected invalid\n" );
        status = pRtlLcidToLocaleName( LOCALE_NEUTRAL, &str, 0, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLcidToLocaleName( LOCALE_NEUTRAL, &str, 2, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );
        status = pRtlLcidToLocaleName( LOCALE_INVARIANT, NULL, 0, 0 );
        ok( status == STATUS_INVALID_PARAMETER_2, "wrong error %lx\n", status );

        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &str, 0, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( !wcscmp( buffer, L"en-US" ), "wrong name %s\n", debugstr_w(buffer) );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
        ok( str.MaximumLength == sizeof(buffer), "wrong max len %u\n", str.MaximumLength );

        status = pRtlLcidToLocaleName( MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), &str, 0, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );

        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), &str, 2, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
        ok( !wcscmp( buffer, L"en" ), "wrong name %s\n", debugstr_w(buffer) );

        ok( IsValidLocale( 0x00010407, 0 ), "expected valid\n" );
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( 0x00010407, &str, 0, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
        ok( !wcscmp( buffer, L"de-DE_phoneb" ), "wrong name %s\n", debugstr_w(buffer) );

        ok( !IsValidLocale( LOCALE_SYSTEM_DEFAULT, 0 ), "expected invalid\n" );
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( LOCALE_SYSTEM_DEFAULT, &str, 0, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
#ifndef __REACTOS__
        LCIDToLocaleName( GetSystemDefaultLCID(), expect, ARRAY_SIZE(expect), 0 );
        ok( !wcscmp( buffer, expect ), "wrong name %s / %s\n", debugstr_w(buffer), debugstr_w(expect) );
#endif

        ok( !IsValidLocale( LOCALE_USER_DEFAULT, 0 ), "expected invalid\n" );
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( LOCALE_USER_DEFAULT, &str, 0, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
#ifndef __REACTOS__
        LCIDToLocaleName( GetUserDefaultLCID(), expect, ARRAY_SIZE(expect), 0 );
        ok( !wcscmp( buffer, expect ), "wrong name %s / %s\n", debugstr_w(buffer), debugstr_w(expect) );
#endif

        ok( IsValidLocale( LOCALE_INVARIANT, 0 ), "expected valid\n" );
        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( LOCALE_INVARIANT, &str, 0, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
        ok( !wcscmp( buffer, L"" ), "wrong name %s\n", debugstr_w(buffer) );

        memset( buffer, 0xcc, sizeof(buffer) );
        status = pRtlLcidToLocaleName( LOCALE_CUSTOM_DEFAULT, &str, 0, 0 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
#ifndef __REACTOS__
        LCIDToLocaleName( GetUserDefaultLCID(), expect, ARRAY_SIZE(expect), 0 );
        ok( !wcscmp( buffer, expect ), "wrong name %s / %s\n", debugstr_w(buffer), debugstr_w(expect) );
#endif

        status = pRtlLcidToLocaleName( LOCALE_CUSTOM_UI_DEFAULT, &str, 0, 0 );
        ok( status == STATUS_SUCCESS || status == STATUS_UNSUCCESSFUL, "wrong error %lx\n", status );

        status = pRtlLcidToLocaleName( LOCALE_CUSTOM_UNSPECIFIED, &str, 0, 0 );
        ok( status == STATUS_INVALID_PARAMETER_1, "wrong error %lx\n", status );

        memset( buffer, 0xcc, sizeof(buffer) );
        str.Length = 0xbeef;
        str.MaximumLength = 5 * sizeof(WCHAR);
        status = pRtlLcidToLocaleName( 0x00010407, &str, 0, 0 );
        ok( status == STATUS_BUFFER_TOO_SMALL, "wrong error %lx\n", status );
        ok( str.Length == 0xbeef, "wrong len %u\n", str.Length );
        ok( str.MaximumLength == 5 * sizeof(WCHAR), "wrong len %u\n", str.MaximumLength );
        ok( buffer[0] == 0xcccc, "wrong name %s\n", debugstr_w(buffer) );

        memset( &str, 0xcc, sizeof(str) );
        status = pRtlLcidToLocaleName( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), &str, 0, 1 );
        ok( status == STATUS_SUCCESS, "wrong error %lx\n", status );
        ok( str.Length == wcslen(str.Buffer) * sizeof(WCHAR), "wrong len %u\n", str.Length );
        ok( str.MaximumLength == str.Length + sizeof(WCHAR), "wrong max len %u\n", str.MaximumLength );
        ok( !wcscmp( str.Buffer, L"en-US" ), "wrong name %s\n", debugstr_w(str.Buffer) );
        RtlFreeUnicodeString( &str );
    }
    else win_skip( "RtlLcidToLocaleName not available\n" );

    if (pNlsValidateLocale)
    {
        void *ret, *ret2;
        LCID lcid;

        lcid = LOCALE_NEUTRAL;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == GetUserDefaultLCID(), "wrong lcid %04lx\n", lcid );

        lcid = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), "wrong lcid %04lx\n", lcid );

        lcid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
        ret2 = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret2, "failed for %04lx\n", lcid );
        ok( ret == ret2, "got different pointer for neutral\n" );
        ok( lcid == MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), "wrong lcid %04lx\n", lcid );

        lcid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
        ret2 = pNlsValidateLocale( &lcid, LOCALE_ALLOW_NEUTRAL_NAMES );
        ok( !!ret2, "failed for %04lx\n", lcid );
        ok( ret != ret2, "got same pointer for neutral\n" );
        ok( lcid == MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), "wrong lcid %04lx\n", lcid );

        lcid = 0x00010407;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == 0x00010407, "wrong lcid %04lx\n", lcid );

        lcid = LOCALE_SYSTEM_DEFAULT;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == GetSystemDefaultLCID(), "wrong lcid %04lx\n", lcid );
        ret2 = pNlsValidateLocale( &lcid, 0 );
        ok( ret == ret2, "got different pointer for system\n" );

        lcid = LOCALE_USER_DEFAULT;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == GetUserDefaultLCID(), "wrong lcid %04lx\n", lcid );
        ret2 = pNlsValidateLocale( &lcid, 0 );
        ok( ret == ret2, "got different pointer for user\n" );

        lcid = LOCALE_INVARIANT;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == LOCALE_INVARIANT, "wrong lcid %04lx\n", lcid );
        ret2 = pNlsValidateLocale( &lcid, 0 );
        ok( ret == ret2, "got different pointer for invariant\n" );

        lcid = LOCALE_CUSTOM_DEFAULT;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !!ret, "failed for %04lx\n", lcid );
        ok( lcid == GetUserDefaultLCID(), "wrong lcid %04lx\n", lcid );
        ret2 = pNlsValidateLocale( &lcid, 0 );
        ok( ret == ret2, "got different pointer for custom default\n" );

        lcid = LOCALE_CUSTOM_UNSPECIFIED;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( ret || broken(!ret), /* <= win8 */ "failed for %04lx\n", lcid );
        if (ret) ok( lcid == GetUserDefaultLCID(), "wrong lcid %04lx\n", lcid );

        SetLastError( 0xdeadbeef );
        lcid = LOCALE_CUSTOM_UI_DEFAULT;
        ret = pNlsValidateLocale( &lcid, 0 );
        if (!ret) ok( GetLastError() == 0xdeadbeef, "error %lu\n", GetLastError());

        lcid = 0xbeef;
        ret = pNlsValidateLocale( &lcid, 0 );
        ok( !ret, "succeeded\n" );
        ok( lcid == 0xbeef, "wrong lcid %04lx\n", lcid );
        ok( GetLastError() == 0xdeadbeef, "error %lu\n", GetLastError());
    }
    else win_skip( "NlsValidateLocale not available\n" );
}

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

    int len1 = LCMapStringA(0, LCMAP_SORTKEY, s1, -1, key1, sizeof(key1));
    int len2 = LCMapStringA(0, LCMAP_SORTKEY, s2, -1, key2, sizeof(key2));
    int ret = memcmp(key1, key2, min(len1, len2));
    if (!ret) ret = len1 - len2;
    return ret;
}

static void test_sorting(void)
{
    char buf[256];
    char **str_buf = (char **)buf;
    int i;

    assert(sizeof(buf) >= sizeof(strings));

    /* 1. sort using lstrcmpA */
    memcpy(buf, strings, sizeof(strings));
    qsort(buf, ARRAY_SIZE(strings), sizeof(strings[0]), compare_string1);
    for (i = 0; i < ARRAY_SIZE(strings); i++)
    {
        ok(!strcmp(strings_sorted[i], str_buf[i]),
           "qsort using lstrcmpA failed for element %d\n", i);
    }
    /* 2. sort using CompareStringA */
    memcpy(buf, strings, sizeof(strings));
    qsort(buf, ARRAY_SIZE(strings), sizeof(strings[0]), compare_string2);
    for (i = 0; i < ARRAY_SIZE(strings); i++)
    {
        ok(!strcmp(strings_sorted[i], str_buf[i]),
           "qsort using CompareStringA failed for element %d\n", i);
    }
    /* 3. sort using sort keys */
    memcpy(buf, strings, sizeof(strings));
    qsort(buf, ARRAY_SIZE(strings), sizeof(strings[0]), compare_string3);
    for (i = 0; i < ARRAY_SIZE(strings); i++)
    {
        ok(!strcmp(strings_sorted[i], str_buf[i]),
           "qsort using sort keys failed for element %d\n", i);
    }
}

struct sorting_test_entry {
    const WCHAR *locale;
    int result_sortkey;
    int result_compare;
    DWORD flags;
    const WCHAR *first;
    const WCHAR *second;
};

static const struct sorting_test_entry unicode_sorting_tests[] =
{
     /* Normal character */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0037", L"\x277c" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x1eca", L"\x1ecb" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x1d05", L"\x1d48" },
     /* Normal character diacritics */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x19d7", L"\x096d" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x00f5", L"\x1ecf" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x2793", L"\x0d70" },
     /* Normal character case weights */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"A", L"a" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"z", L"Z" },
     /* PUA character */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xe5a6", L"\xe5a5\x0333" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xe5d7", L"\xe5d6\x0330" },
     /* Symbols add diacritic weight */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\u276a", L"\u2768" },
     /* Symbols add case weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\u204d", L"\uff02" },
     /* Default character, when there is main weight extra there must be no diacritic weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\ue6e3\u0a02", L"\ue6e3\u20dc" },
     /* Unsortable characters */
    { L"en-US",  0, CSTR_EQUAL,        0, L"a \u2060 b", L"a  b" },
     /* Invalid/undefined characters */
    { L"en-US",  0, CSTR_EQUAL,        0, L"a \xfff0 b", L"a  b" },
    { L"en-US",  0, CSTR_EQUAL,        0, L"a\x139F a", L"a a" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"a\x139F a", L"a b" },
     /* Default characters */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x00fc", L"\x016d" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x3fcb\x7fd5", L"\x0006\x3032" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x00fc\x30fd", L"\x00fa\x1833" },
     /* Diacritic is added */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x1B56\x0330", L"\x1096" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x1817\x0333", L"\x19d7" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x04de\x05ac", L"\x0499" },
     /* Diacritic can overflow */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x01ba\x0654", L"\x01b8" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x06b7\x06eb", L"\x06b6" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x1420\x0333", L"\x141f" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x1b56\x0654", L"\x1b56\x0655" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x1b56\x0654\x0654", L"\x1b56\x0655" },
     /* Jamo case weight */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x11bc", L"\x110b" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x11c1", L"\x1111" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x11af", L"\x1105" },
     /* Jamo main weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x11c2", L"\x11f5" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x1108", L"\x1121" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x1116", L"\x11c7" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x11b1", L"\x11d1" },
     /* CJK main weight 1 */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x4550\x73d2", L"\x3211\x23ad" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x3265", L"\x4079" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x4c19\x68d0\x52d0", L"\x316d" },
     /* CJK main weight 2 */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x72dd", L"\x6b8a" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x6785\x3bff\x6f83", L"\x7550\x34c9\x71a7" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x5d61", L"\x3aef" },
     /* Symbols case weights */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x207a", L"\xfe62" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xfe65", L"\xff1e" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x2502", L"\xffe8" },
     /* Symbols diacritic weights */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x21da", L"\x21dc" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x29fb", L"\x2295" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0092", L"\x009c" },
     /* NORM_IGNORESYMBOLS */
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x21da", L"\x21dc" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x29fb", L"\x2295" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x0092", L"\x009c" },
    { L"en-US",  0, CSTR_EQUAL,        0, L"\x3099", L"\x309b" }, /* Small diacritic weights at the end get ignored */
     /* Main weights have priority over diacritic weights */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"a b", L"\x0103 a" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"a",   L"\x0103" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"e x", L"\x0113 v" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"e",   L"\x0113" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"c s", L"\x0109 r" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"c",   L"\x0109" },
     /* Diacritic weights have priority over case weights */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"a \x0103", L"A a" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"a",        L"A" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"e \x0113", L"E e" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"e",        L"E" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"c \x0109", L"C c" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"c",        L"C" },
     /* Diacritic values for Jamo are not ignored */
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORENONSPACE, L"\x1152", L"\x1153" },
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORENONSPACE, L"\x1143", L"\x1145" },
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORENONSPACE, L"\x1196", L"\x1174" },
     /* Jungseong < PUA */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x318e", L"\x382a" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xffcb", L"\x3d13" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xffcc", L"\x8632" },
     /* Surrogate > PUA */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xd847", L"\x382a" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xd879", L"\x3d13" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xd850", L"\x8632" },
     /* Unsortable combined with diacritics */
    { L"en-US",  0, CSTR_EQUAL,        0, L"A\x0301\x0301", L"A\x0301\x00ad\x0301" },
    { L"en-US",  0, CSTR_EQUAL,        0, L"b\x07f2\x07f2", L"b\x07f2\x2064\x07f2" },
    { L"en-US",  0, CSTR_EQUAL,        0, L"X\x0337\x0337", L"X\x0337\xfffd\x0337" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORECASE, L"c", L"C" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORECASE, L"e", L"E" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORECASE, L"A", L"a" },
    /* Punctuation primary weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x001b", L"\x001c" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0005", L"\x0006" },
     /* Punctuation diacritic/case weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0027", L"\xff07" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x07f4", L"\x07f5" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x207b", L"\x0008" },
     /* Punctuation primary weight has priority */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xff07", L"\x07f4" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xfe32", L"\x2014" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x058a", L"\x2027" },
     /* Punctuation */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x207b", L"\x0008" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0004", L"\x0011" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x207b", L"\x0008" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x0004", L"\x0011" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x207b", L"\x0008" },
    { L"en-US", -1, CSTR_LESS_THAN,    SORT_STRINGSORT, L"\x0004", L"\x0011" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS | SORT_STRINGSORT, L"\x207b", L"\x0008" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS | SORT_STRINGSORT, L"\x0004", L"\x0011" },
     /* Punctuation main weight */
    { L"en-US", -1, CSTR_LESS_THAN,    SORT_STRINGSORT, L"\x001a", L"\x001b" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x2027", L"\x2011" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x3030", L"\x301c" },
     /* Punctuation diacritic weight */
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x058a", L"\x2010" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x07F5", L"\x07F4" },
     /* Punctuation case weight */
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\xfe32", L"\x2013" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\xfe31", L"\xfe58" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\xff07", L"\x0027" },
     /* Punctuation NORM_IGNORESYMBOLS */
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x207b", L"\x0008" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS, L"\x0004", L"\x0011" },
     /* Punctuation NORM_IGNORESYMBOLS SORT_STRINGSORT */
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS | SORT_STRINGSORT, L"\x207b", L"\x0008" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORESYMBOLS | SORT_STRINGSORT, L"\x0004", L"\x0011" },
     /* Punctuation SORT_STRINGSORT main weight */
    { L"en-US", -1, CSTR_LESS_THAN,    SORT_STRINGSORT, L"\x001a", L"\x001b" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x2027", L"\x2011", },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x3030", L"\x301c", },
     /* Punctuation SORT_STRINGSORT diacritic weight */
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x058a", L"\x2010" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\x07F5", L"\x07F4" },
     /* Punctuation SORT_STRINGSORT case weight */
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\xfe32", L"\x2013" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\xfe31", L"\xfe58" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_STRINGSORT, L"\xff07", L"\x0027" },
     /* Japanese main weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x04b0", L"\x32db" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x3093", L"\x1e62\x013f" },
     /* Japanese diacritic weight */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30d3", L"\x30d4" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x307b", L"\x307c" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30ea", L"\x32f7" },
     /* Japanese case weight small */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x31fb", L"\x30e9" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30db", L"\x31f9" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xff6d", L"\xff95" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORENONSPACE, L"\x31fb", L"\x30e9" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORENONSPACE, L"\x30db", L"\x31f9" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORENONSPACE, L"\xff6d", L"\xff95" },
     /* Japanese case weight kana */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30d5", L"\x3075" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x306a", L"\x30ca" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x305a", L"\x30ba" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREKANATYPE, L"\x30d5", L"\x3075" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREKANATYPE, L"\x306a", L"\x30ca" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREKANATYPE, L"\x305a", L"\x30ba" },
     /* Japanese case weight width */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30bf", L"\xff80" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30ab", L"\xff76" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30a2", L"\xff71" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\x30bf", L"\xff80" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\x30ab", L"\xff76" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\x30a2", L"\xff71" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORENONSPACE, L"\x31a2", L"\x3110" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORENONSPACE, L"\x1342", L"\x133a" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNORENONSPACE, L"\x16a4", L"\x16a5" },
     /* Kana small data must have priority over width data */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30b1\x30f6", L"\xff79\x30b1" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30a6\x30a5", L"\xff73\x30a6" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30a8\x30a7", L"\xff74\x30a8" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30b1", L"\xff79" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30a6", L"\xff73" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30a8", L"\xff74" },
     /* Kana small data must have priority over kana type data */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x3046\x30a9", L"\x30a6\x30aa" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x304a\x3041", L"\x30aa\x3042" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x3059\x30a7", L"\x30b9\x30a8" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x3046", L"\x30a6" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x304a", L"\x30aa" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x3059", L"\x30b9" },
     /* Kana type data must have priority over width data */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30a6\x30a8", L"\xff73\x3048" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30ab\x30a3", L"\xff76\x3043" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30b5\x30ac", L"\xff7b\x304c" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30a6", L"\xff73" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30ab", L"\xff76" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30b5", L"\xff7b" },
     /* Case weights have priority over extra weights */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x305a a", L"\x30ba A" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30c1 b", L"\xff81 B" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xff8b x", L"\x31f6 X" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x305a", L"\x30ba" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30c1", L"\xff81" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xff8b", L"\x31f6" },
     /* Extra weights have priority over special weights */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0027\x31ff", L"\x007f\xff9b" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x07f5\x30f3", L"\x07f4\x3093" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xfe63\x30e0", L"\xff0d\x3080" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x0027", L"\x007f" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x07f5", L"\x07f4" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xfe63", L"\xff0d" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\xff68", L"\x30a3" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\xff75", L"\x30aa" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\x30e2", L"\xff93" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xff68", L"\x30a3" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\xff75", L"\x30aa" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x30e2", L"\xff93" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREKANATYPE, L"\x30a8", L"\x3048" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREKANATYPE, L"\x30af", L"\x304f" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREKANATYPE, L"\x3067", L"\x30c7" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30a8", L"\x3048" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30af", L"\x304f" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x3067", L"\x30c7" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\xffb7", L"\x3147" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\xffb6", L"\x3146" },
    { L"en-US",  0, CSTR_EQUAL,        NORM_IGNOREWIDTH, L"\x3145", L"\xffb5" },
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORECASE, L"\xffb7", L"\x3147" },
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORECASE, L"\xffb6", L"\x3146" },
    { L"en-US",  1, CSTR_GREATER_THAN, NORM_IGNORECASE, L"\x3145", L"\xffb5" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x3075\x30fc", L"\x30d5\x30fc" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x30a1\x30fc", L"\x30a2\x30fc" },
     /* Coptic < Japanese */
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORECASE, L"\x2cff", L"\x30ba" },
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORECASE, L"\x2cdb", L"\x32de" },
    { L"en-US", -1, CSTR_LESS_THAN,    NORM_IGNORECASE, L"\x2ce0", L"\x30c6" },
     /* Hebrew > Japanese */
    { L"en-US",  1, CSTR_GREATER_THAN, NORM_IGNORECASE, L"\x05d3", L"\x30ba" },
    { L"en-US",  1, CSTR_GREATER_THAN, NORM_IGNORECASE, L"\x05e3", L"\x32de" },
    { L"en-US",  1, CSTR_GREATER_THAN, NORM_IGNORECASE, L"\x05d7", L"\x30c6" },
     /* Expansion */
    { L"en-US",  0, CSTR_EQUAL,        0, L"\x00c6", L"\x0041\x0045" },
    { L"en-US",  0, CSTR_EQUAL,        0, L"\x0f5c", L"\x0f5b\x0fb7" },
    { L"en-US",  0, CSTR_EQUAL,        0, L"\x05f0", L"\x05d5\x05d5" },
    { L"en-US", -1, CSTR_EQUAL,        0, L"\x0f75", L"\x0f71\x0f74" },
    { L"en-US", -1, CSTR_EQUAL,        0, L"\xfc5e", L"\x064c\x0651" },
    { L"en-US", -1, CSTR_EQUAL,        0, L"\xfb2b", L"\x05e9\x05c2" },
    { L"en-US", -1, CSTR_EQUAL,        0, L"\xfe71", L"\x0640\x064b" },
     /* Japanese locale */
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x6df8", L"\x654b\x29e9" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x685d\x1239\x1b61", L"\x59b6\x6542\x2a62\x04a7" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x62f3\x43e9", L"\x5760" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x634c", L"\x2f0d\x5f1c\x7124" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x69e7\x0502", L"\x57cc" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x7589", L"\x67c5" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x5ede\x765c", L"\x7324" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x5c7f\x5961", L"\x7cbe" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x3162", L"\x6a84\x1549\x0b60" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x769e\x448e", L"\x4e6e" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x59a4", L"\x5faa\x607c" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x529b", L"\x733f" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x6ff8\x2a0a", L"\x7953\x6712" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x6dfb", L"\x6793" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x67ed", L"\x6aa2" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x4e61", L"\x6350\x6b08" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x5118", L"\x53b3\x75b4" },
    { L"ja-JP", -1, CSTR_LESS_THAN,    0, L"\x6bbf", L"\x65a3" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x5690", L"\x5fa8" },
    { L"ja-JP",  1, CSTR_GREATER_THAN, 0, L"\x61e2", L"\x76e5" },
     /* Misc locales */
    { L"ko-KR", -1, CSTR_LESS_THAN,    0, L"\x8db6", L"\xd198" },
    { L"ko-KR", -1, CSTR_LESS_THAN,    0, L"\x8f72", L"\xd2b9" },
    { L"ko-KR", -1, CSTR_LESS_THAN,    0, L"\x91d8", L"\xd318" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x8db6", L"\xd198" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x8f72", L"\xd2b9" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x91d8", L"\xd318" },
    { L"cs-CZ",  1, CSTR_GREATER_THAN, 0, L"\x0160", L"\x0219" },
    { L"cs-CZ",  1, CSTR_GREATER_THAN, 0, L"\x059a", L"\x0308" },
    { L"cs-CZ",  1, CSTR_GREATER_THAN, 0, L"\x013a", L"\x013f" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x0160", L"\x0219" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x059a", L"\x0308" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x013a", L"\x013f" },
    { L"vi-VN", -1, CSTR_LESS_THAN,    0, L"\x1d8f", L"\x1ea8" },
    { L"vi-VN", -1, CSTR_LESS_THAN,    0, L"\x0323", L"\xfe26" },
    { L"vi-VN",  1, CSTR_GREATER_THAN, 0, L"R",      L"\xff32" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x1d8f", L"\x1ea8" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x0323", L"\xfe26" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"R",      L"\xff32" },
    { L"zh-HK", -1, CSTR_LESS_THAN,    0, L"\x83ae", L"\x71b9" },
    { L"zh-HK", -1, CSTR_LESS_THAN,    0, L"\x7e50", L"\xc683" },
    { L"zh-HK",  1, CSTR_GREATER_THAN, 0, L"\x6c69", L"\x7f8a" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x83ae", L"\x71b9" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x7e50", L"\xc683" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x6c69", L"\x7f8a" },
    { L"tr-TR",  1, CSTR_GREATER_THAN, 0, L"\x00dc", L"\x1ee9" },
    { L"tr-TR",  1, CSTR_GREATER_THAN, 0, L"\x00fc", L"\x1ee6" },
    { L"tr-TR", -1, CSTR_LESS_THAN,    0, L"\x0152", L"\x00d6" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x00dc", L"\x1ee9" },
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x00fc", L"\x1ee6" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\x0152", L"\x00d6" },
    /* Diacritic is added */
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xa042\x09bc", L"\xa042" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xa063\x302b", L"\xa063" },
    { L"en-US",  1, CSTR_GREATER_THAN, 0, L"\xa07e\x0c56", L"\xa07e" },
    /* Reversed diacritics */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"\x00e9\x00e8", L"\x00e8\x00e9" },
    { L"fr-FR",  1, CSTR_GREATER_THAN, 0, L"\x00e9\x00e8", L"\x00e8\x00e9" },
    /* Digit sort */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"1230", L"321" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_DIGITSASNUMBERS, L"1230", L"321" },
    { L"en-US",  1, CSTR_GREATER_THAN, SORT_DIGITSASNUMBERS, L"\xc6f\xc6c\xc6a", L"\xc6f\xc6e" },
    /* Compressions */
    { L"en-US", -1, CSTR_LESS_THAN,    0, L"E\x0300", L"F" },
    { L"rm-CH",  1, CSTR_GREATER_THAN, 0, L"E\x0300", L"F" },
};

static void test_unicode_sorting(void)
{
    int i;
    int ret1;
    int ret2;
    BYTE buffer[1000];
    if (!pLCMapStringEx)
    {
        win_skip("LCMapStringEx not available\n");
        return;
    }
    for (i = 0; i < ARRAY_SIZE(unicode_sorting_tests); i++)
    {
        BYTE buff1[1000];
        BYTE buff2[1000];
        int len1, len2;
        int result;
        const struct sorting_test_entry *entry = &unicode_sorting_tests[i];

        len1 = pLCMapStringEx(entry->locale, LCMAP_SORTKEY | entry->flags, entry->first, -1, (WCHAR*)buff1, ARRAY_SIZE(buff1), NULL, NULL, 0);
        len2 = pLCMapStringEx(entry->locale, LCMAP_SORTKEY | entry->flags, entry->second, -1, (WCHAR*)buff2, ARRAY_SIZE(buff2), NULL, NULL, 0);

        result = memcmp(buff1, buff2, min(len1, len2));
        if (result < 0) result = -1;
        else if (result > 0) result = 1;
        else if (len1 < len2) result = -1;
        else if (len1 > len2) result = 1;

        ok (result == entry->result_sortkey, "Test %d (%s, %s) - Expected %d, got %d\n",
            i, wine_dbgstr_w(entry->first), wine_dbgstr_w(entry->second), entry->result_sortkey, result);

#ifndef __REACTOS__
        result = CompareStringEx(entry->locale, entry->flags,  entry->first, -1, entry->second, -1, NULL, NULL, 0);
        ok (result == entry->result_compare, "Test %d (%s, %s) - Expected %d, got %d\n",
            i, wine_dbgstr_w(entry->first), wine_dbgstr_w(entry->second), entry->result_compare, result);
#endif
    }
    /* Test diacritics when buffer is short */
    ret1 = pLCMapStringEx(L"en-US", LCMAP_SORTKEY, L"\x0e49\x0e49\x0e49\x0e49\x0e49", -1, (WCHAR*)buffer, 20, NULL, NULL, 0);
    ret2 = pLCMapStringEx(L"en-US", LCMAP_SORTKEY, L"\x0e49\x0e49\x0e49\x0e49\x0e49", -1, (WCHAR*)buffer, 0, NULL, NULL, 0);
    ok(ret1 == ret2, "Got ret1=%d, ret2=%d\n", ret1, ret2);
}

static void test_FoldStringA(void)
{
  int ret, i, j;
  BOOL is_special;
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
  static const char composite_dst_alt[] =
  {
    0x53,0x3f,0x5a,0x3f,0x73,0x3f,0x7a,0x3f,
    0x59,0xa8,0x41,0x60,0x41,0xb4,0x41,0x5e,
    0x41,0x7e,0x41,0xa8,0x41,0xb0,0x43,0xb8,
    0x45,0x60,0x45,0xb4,0x45,0x5e,0x45,0xa8,
    0x49,0x60,0x49,0xb4,0x49,0x5e,0x49,0xa8,
    0x4e,0x7e,0x4f,0x60,0x4f,0xb4,0x4f,0x5e,
    0x4f,0x7e,0x4f,0xa8,0xd8,0x55,0x60,0x55,
    0xb4,0x55,0x5e,0x55,0xa8,0x59,0xb4,0x61,
    0x60,0x61,0xb4,0x61,0x5e,0x61,0x7e,0x61,
    0xa8,0x61,0xb0,0x63,0xb8,0x65,0x60,0x65,
    0xb4,0x65,0x5e,0x65,0xa8,0x69,0x60,0x69,
    0xb4,0x69,0x5e,0x69,0xa8,0x6e,0x7e,0x6f,
    0x60,0x6f,0xb4,0x6f,0x5e,0x6f,0x7e,0x6f,
    0xa8,0xf8,0x75,0x60,0x75,0xb4,0x75,0x5e,
    0x75,0xa8,0x79,0xb4,0x79,0xa8,'\0'
  };
  static const char ligatures_src[] =
  {
    0x8C,0x9C,0xC6,0xDE,0xDF,0xE6,0xFE,'\0'
  };
  static const char ligatures_dst[] =
  {
    'O','E','o','e','A','E','T','H','s','s','a','e','t','h','\0'
  };
  static const struct special
  {
    char src;
    char dst[4];
  }  foldczone_special[] =
  {
    /* src   dst                   */
    { 0x85, { 0x2e, 0x2e, 0x2e, 0x00 } },
    { 0x98, { 0x20, 0x7e, 0x00 } },
    { 0x99, { 0x54, 0x4d, 0x00 } },
    { 0xa0, { 0x20, 0x00 } },
    { 0xa8, { 0x20, 0xa8, 0x00 } },
    { 0xaa, { 0x61, 0x00 } },
    { 0xaf, { 0x20, 0xaf, 0x00 } },
    { 0xb2, { 0x32, 0x00 } },
    { 0xb3, { 0x33, 0x00 } },
    { 0xb4, { 0x20, 0xb4, 0x00 } },
    { 0xb8, { 0x20, 0xb8, 0x00 } },
    { 0xb9, { 0x31, 0x00 } },
    { 0xba, { 0x6f, 0x00 } },
    { 0xbc, { 0x31, 0x2f, 0x34, 0x00 } },
    { 0xbd, { 0x31, 0x2f, 0x32, 0x00 } },
    { 0xbe, { 0x33, 0x2f, 0x34, 0x00 } },
    { 0x00 }
  };

  /* these tests are locale specific */
  if (GetACP() != 1252)
  {
      trace("Skipping FoldStringA tests for a not Latin 1 locale\n");
      return;
  }

  /* MAP_FOLDDIGITS */
  SetLastError(0xdeadbeef);
  ret = FoldStringA(MAP_FOLDDIGITS, digits_src, -1, dst, 256);
  ok(ret == 4, "Expected ret == 4, got %d, error %ld\n", ret, GetLastError());
  ok(strcmp(dst, digits_dst) == 0,
     "MAP_FOLDDIGITS: Expected '%s', got '%s'\n", digits_dst, dst);
  for (i = 1; i < 256; i++)
  {
    if (!strchr(digits_src, i))
    {
      src[0] = i;
      src[1] = '\0';
      ret = FoldStringA(MAP_FOLDDIGITS, src, -1, dst, 256);
      ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
      ok(dst[0] == src[0],
         "MAP_FOLDDIGITS: Expected '%s', got '%s'\n", src, dst);
    }
  }

    /* MAP_EXPAND_LIGATURES */
    SetLastError(0xdeadbeef);
    ret = FoldStringA(MAP_EXPAND_LIGATURES, ligatures_src, -1, dst, 256);
    ok(ret == sizeof(ligatures_dst), "Got %d, error %ld\n", ret, GetLastError());
    ok(strcmp(dst, ligatures_dst) == 0,
       "MAP_EXPAND_LIGATURES: Expected '%s', got '%s'\n", ligatures_dst, dst);
    for (i = 1; i < 256; i++)
    {
      if (!strchr(ligatures_src, i))
      {
        src[0] = i;
        src[1] = '\0';
        ret = FoldStringA(MAP_EXPAND_LIGATURES, src, -1, dst, 256);
        if (ret == 3)
        {
          /* Vista */
          ok((i == 0xDC && lstrcmpA(dst, "UE") == 0) ||
             (i == 0xFC && lstrcmpA(dst, "ue") == 0),
             "Got %s for %d\n", dst, i);
        }
        else
        {
          ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
          ok(dst[0] == src[0],
             "MAP_EXPAND_LIGATURES: Expected '%s', got '%s'\n", src, dst);
        }
      }
    }

  /* MAP_COMPOSITE */
  SetLastError(0xdeadbeef);
  ret = FoldStringA(MAP_COMPOSITE, composite_src, -1, dst, 256);
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());
  ok( GetLastError() == 0xdeadbeef || broken(!GetLastError()), /* vista */
      "wrong error %lu\n", GetLastError());
  ok(ret == 121 || ret == 119, "Expected 121 or 119, got %d\n", ret);
  ok(strcmp(dst, composite_dst) == 0 || strcmp(dst, composite_dst_alt) == 0,
     "MAP_COMPOSITE: Mismatch, got '%s'\n", dst);

  for (i = 1; i < 256; i++)
  {
    if (!strchr(composite_src, i))
    {
      src[0] = i;
      src[1] = '\0';
      ret = FoldStringA(MAP_COMPOSITE, src, -1, dst, 256);
      ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
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
    SetLastError(0xdeadbeef);
    ret = FoldStringA(MAP_FOLDCZONE, src, -1, dst, 256);
    is_special = FALSE;
    for (j = 0; foldczone_special[j].src != 0 && ! is_special; j++)
    {
      if (foldczone_special[j].src == src[0])
      {
        ok(ret == 2 || ret == lstrlenA(foldczone_special[j].dst) + 1,
           "Expected ret == 2 or %d, got %d, error %ld\n",
           lstrlenA(foldczone_special[j].dst) + 1, ret, GetLastError());
        ok(src[0] == dst[0] || lstrcmpA(foldczone_special[j].dst, dst) == 0,
           "MAP_FOLDCZONE: string mismatch for 0x%02x\n",
           (unsigned char)src[0]);
        is_special = TRUE;
      }
    }
    if (! is_special)
    {
      ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
      ok(src[0] == dst[0],
         "MAP_FOLDCZONE: Expected 0x%02x, got 0x%02x\n",
         (unsigned char)src[0], (unsigned char)dst[0]);
    }
  }

  /* MAP_PRECOMPOSED */
  for (i = 1; i < 256; i++)
  {
    src[0] = i;
    src[1] = '\0';
    ret = FoldStringA(MAP_PRECOMPOSED, src, -1, dst, 256);
    ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
    ok(src[0] == dst[0],
       "MAP_PRECOMPOSED: Expected 0x%02x, got 0x%02x\n",
       (unsigned char)src[0], (unsigned char)dst[0]);
  }
}

static void test_FoldStringW(void)
{
  int ret;
  WORD type;
  unsigned int i, j, len;
  WCHAR src[256], dst[256];
  UINT ch, prev_ch = 1;
  static const DWORD badFlags[] =
  {
    0,
    MAP_PRECOMPOSED|MAP_COMPOSITE,
    MAP_PRECOMPOSED|MAP_EXPAND_LIGATURES,
    MAP_COMPOSITE|MAP_EXPAND_LIGATURES
  };
  /* Ranges of digits 0-9 : Must be sorted! */
  static const struct { UINT ch, first, last; int broken; } digitRanges[] =
  {
      { 0x0030, 0, 9 },                   /* '0'-'9' */
      { 0x00b2, 2, 3 },                   /* Superscript 2, 3 */
      { 0x00b9, 1, 1 },                   /* Superscript 1 */
      { 0x0660, 0, 9 },                   /* Eastern Arabic */
      { 0x06f0, 0, 9 },                   /* Arabic - Hindu */
      { 0x07c0, 0, 9 },                   /* Nko */
      { 0x0966, 0, 9 },                   /* Devengari */
      { 0x09e6, 0, 9 },                   /* Bengalii */
      { 0x0a66, 0, 9 },                   /* Gurmukhi */
      { 0x0ae6, 0, 9 },                   /* Gujarati */
      { 0x0b66, 0, 9 },                   /* Oriya */
      { 0x0be6, 0, 9 },                   /* Tamil */
      { 0x0c66, 0, 9 },                   /* Telugu */
      { 0x0c78, 0, 3, TRUE /*win7*/ },    /* Telugu Fraction */
      { 0x0c7c, 1, 3, TRUE /*win7*/ },    /* Telugu Fraction */
      { 0x0ce6, 0, 9 },                   /* Kannada */
      { 0x0d66, 0, 9 },                   /* Maylayalam */
      { 0x0de6, 0, 9, TRUE /*win10*/ },   /* Sinhala Lith */
      { 0x0e50, 0, 9 },                   /* Thai */
      { 0x0ed0, 0, 9 },                   /* Laos */
      { 0x0f20, 0, 9 },                   /* Tibet */
      { 0x1040, 0, 9 },                   /* Myanmar */
      { 0x1090, 0, 9 },                   /* Myanmar Shan */
      { 0x1369, 1, 9 },                   /* Ethiopic */
      { 0x17e0, 0, 9 },                   /* Khmer */
      { 0x1810, 0, 9 },                   /* Mongolian */
      { 0x1946, 0, 9 },                   /* Limbu */
      { 0x19d0, 0, 9 },                   /* New Tai Lue */
      { 0x19da, 1, 1, TRUE /*win7*/ },    /* New Tai Lue Tham 1 */
      { 0x1a80, 0, 9, TRUE /*win7*/ },    /* Tai Tham Hora */
      { 0x1a90, 0, 9, TRUE /*win7*/ },    /* Tai Tham Tham */
      { 0x1b50, 0, 9 },                   /* Balinese */
      { 0x1bb0, 0, 9 },                   /* Sundanese */
      { 0x1c40, 0, 9 },                   /* Lepcha */
      { 0x1c50, 0, 9 },                   /* Ol Chiki */
      { 0x2070, 0, 0 },                   /* Superscript 0 */
      { 0x2074, 4, 9 },                   /* Superscript 4-9 */
      { 0x2080, 0, 9 },                   /* Subscript */
      { 0x2460, 1, 9 },                   /* Circled */
      { 0x2474, 1, 9 },                   /* Bracketed */
      { 0x2488, 1, 9 },                   /* Full stop */
      { 0x24ea, 0, 0 },                   /* Circled 0 */
      { 0x24f5, 1, 9 },                   /* Double Circled */
      { 0x24ff, 0, 0 },                   /* Negative Circled 0 */
      { 0x2776, 1, 9 },                   /* Inverted Circled */
      { 0x2780, 1, 9 },                   /* Patterned Circled */
      { 0x278a, 1, 9 },                   /* Inverted Patterned Circled */
      { 0x3007, 0, 0 },                   /* Ideographic Number 0 */
      { 0x3021, 1, 9 },                   /* Hangzhou */
      { 0xa620, 0, 9 },                   /* Vai */
      { 0xa8d0, 0, 9 },                   /* Saurashtra */
      { 0xa8e0, 0, 9, TRUE /*win7*/ },    /* Combining Devanagari */
      { 0xa900, 0, 9 },                   /* Kayah Li */
      { 0xa9d0, 0, 9, TRUE /*win7*/ },    /* Javanese */
      { 0xa9f0, 0, 9, TRUE /*win10*/ },   /* Myanmar Tai Laing */
      { 0xaa50, 0, 9 },                   /* Cham */
      { 0xabf0, 0, 9, TRUE /*win7*/ },    /* Meetei Mayek */
      { 0xff10, 0, 9 },                   /* Full Width */
      { 0x10107, 1, 9 },                  /* Aegean */
      { 0x10320, 1, 1 },                  /* Old Italic Numeral 1 */
      { 0x10321, 5, 5 },                  /* Old Italic Numeral 5 */
      { 0x104a0, 0, 9 },                  /* Osmanya */
      { 0x10a40, 1, 4, TRUE /*win10*/ },  /* Kharoshthi */
      { 0x10d30, 0, 9, TRUE /*win10*/ },  /* Hanifi Rohingya */
      { 0x10d40, 0, 9, TRUE /*win10*/ },  /* Garay */
      { 0x10e60, 1, 9, TRUE /*win10*/ },  /* Rumi */
      { 0x11052, 1, 9, TRUE /*win10*/ },  /* Brahmi Number */
      { 0x11066, 0, 9, TRUE /*win10*/ },  /* Brahmi Digit */
      { 0x110f0, 0, 9, TRUE /*win10*/ },  /* Sora Sompeng */
      { 0x11136, 0, 9, TRUE /*win10*/ },  /* Chakma */
      { 0x111d0, 0, 9, TRUE /*win10*/ },  /* Sharada */
      { 0x112f0, 0, 9, TRUE /*win10*/ },  /* Khudawadi */
      { 0x11450, 0, 9, TRUE /*win10*/ },  /* Newa */
      { 0x114d0, 0, 9, TRUE /*win10*/ },  /* Tirhuta */
      { 0x11650, 0, 9, TRUE /*win10*/ },  /* Modi */
      { 0x116c0, 0, 9, TRUE /*win10*/ },  /* Takri */
      { 0x116d0, 0, 9, TRUE /*win10*/ },  /* Myanmar Pa-O */
      { 0x116da, 0, 9, TRUE /*win10*/ },  /* Myanmar Eastern Pwo */
      { 0x11730, 0, 9, TRUE /*win10*/ },  /* Ahom */
      { 0x118e0, 0, 9, TRUE /*win10*/ },  /* Warang */
      { 0x11950, 0, 9, TRUE /*win10*/ },  /* Dives Akuru */
      { 0x11bf0, 0, 9, TRUE /*win10*/ },  /* Sunuwar */
      { 0x11c50, 0, 9, TRUE /*win10*/ },  /* Bhaiksuki */
      { 0x11d50, 0, 9, TRUE /*win10*/ },  /* Masaram Gondi */
      { 0x11da0, 0, 9, TRUE /*win10*/ },  /* Gunjala Gondi */
      { 0x11f50, 0, 9, TRUE /*win10*/ },  /* Kawi */
      { 0x16130, 0, 9, TRUE /*win10*/ },  /* Gurung Khema */
      { 0x16a60, 0, 9, TRUE /*win10*/ },  /* Mro */
      { 0x16ac0, 0, 9, TRUE /*win10*/ },  /* Tangsa */
      { 0x16b50, 0, 9, TRUE /*win10*/ },  /* Pahawh Hmong */
      { 0x16d70, 0, 9, TRUE /*win10*/ },  /* Kirat Rai */
      { 0x1ccf0, 0, 9, TRUE /*win10*/ },  /* Outlined digits */
      { 0x1d7ce, 0, 9 },                  /* Mathematical Bold */
      { 0x1d7d8, 0, 9 },                  /* Mathematical Double Struck */
      { 0x1d7e2, 0, 9 },                  /* Mathematical Sans Serif */
      { 0x1d7ec, 0, 9 },                  /* Mathematical Sans Serif Bold */
      { 0x1d7f6, 0, 9 },                  /* Mathematical Monospace */
      { 0x1e140, 0, 9, TRUE /*win10*/ },  /* Nyiakeng Puachue Hmong */
      { 0x1e2f0, 0, 9, TRUE /*win10*/ },  /* Wancho */
      { 0x1e4f0, 0, 9, TRUE /*win10*/ },  /* Nag Mundari */
      { 0x1e5f1, 0, 9, TRUE /*win10*/ },  /* Ol Onal */
      { 0x1e950, 0, 9, TRUE /*win10*/ },  /* Adlam */
      { 0x1f100, 0, 0, TRUE /*win10*/ },  /* Full Stop */
      { 0x1f101, 0, 9, TRUE /*win10*/ },  /* Comma */
      { 0x1fbf0, 0, 9, TRUE /*win10*/ },  /* Segmented */
      { 0x10ffff } /* Terminator */
  };
  static const WCHAR foldczone_src[] =
  {
    'W',    'i',    'n',    'e',    0x0348, 0x0551, 0x1323, 0x280d,
    0xff37, 0xff49, 0xff4e, 0xff45, 0x3c5, 0x308, 0x6a, 0x30c, 0xa0, 0xaa, 0
  };
  static const WCHAR foldczone_dst[] =
  {
    'W','i','n','e',0x0348,0x0551,0x1323,0x280d,'W','i','n','e',0x3cb,0x1f0,' ','a',0
  };
  static const WCHAR foldczone_broken_dst[] =
  {
    'W','i','n','e',0x0348,0x0551,0x1323,0x280d,'W','i','n','e',0x03c5,0x0308,'j',0x030c,0x00a0,0x00aa,0
  };
  static const WCHAR ligatures_src[] =
  {
    'W',    'i',    'n',    'e',    0x03a6, 0x03b9, 0x03bd, 0x03b5,
    0x00c6, 0x00de, 0x00df, 0x00e6, 0x00fe, 0x0132, 0x0133, 0x0152,
    0x0153, 0x01c4, 0x01c5, 0x01c6, 0x01c7, 0x01c8, 0x01c9, 0x01ca,
    0x01cb, 0x01cc, 0x01e2, 0x01e3, 0x01f1, 0x01f2, 0x01f3, 0x01fc,
    0x01fd, 0x05f0, 0x05f1, 0x05f2, 0xfb00, 0xfb01, 0xfb02, 0xfb03,
    0xfb04, 0xfb05, 0xfb06, '\0'
  };
  static const WCHAR ligatures_dst[] =
  {
    'W','i','n','e',0x03a6,0x03b9,0x03bd,0x03b5,
    'A','E','T','H','s','s','a','e','t','h','I','J','i','j','O','E','o','e',
    'D',0x017d,'D',0x017e,'d',0x017e,'L','J','L','j','l','j','N','J','N','j',
    'n','j',0x0100,0x0112,0x0101,0x0113,'D','Z','D','z','d','z',0x00c1,0x00c9,
    0x00e1,0x00e9,0x05d5,0x05d5,0x05d5,0x05d9,0x05d9,0x05d9,'f','f','f','i',
    'f','l','f','f','i','f','f','l',0x017f,'t','s','t','\0'
  };

  /* Invalid flag combinations */
  for (i = 0; i < ARRAY_SIZE(badFlags); i++)
  {
    src[0] = dst[0] = '\0';
    SetLastError(0xdeadbeef);
    ret = FoldStringW(badFlags[i], src, 256, dst, 256);
    ok(!ret && GetLastError() == ERROR_INVALID_FLAGS,
       "Expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError());
  }

  /* src & dst cannot be the same */
  SetLastError(0xdeadbeef);
  ret = FoldStringW(MAP_FOLDCZONE, src, -1, src, 256);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* src can't be NULL */
  SetLastError(0xdeadbeef);
  ret = FoldStringW(MAP_FOLDCZONE, NULL, -1, dst, 256);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* srclen can't be 0 */
  SetLastError(0xdeadbeef);
  ret = FoldStringW(MAP_FOLDCZONE, src, 0, dst, 256);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* dstlen can't be < 0 */
  SetLastError(0xdeadbeef);
  ret = FoldStringW(MAP_FOLDCZONE, src, -1, dst, -1);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* Ret includes terminating NUL which is appended if srclen = -1 */
  SetLastError(0xdeadbeef);
  src[0] = 'A';
  src[1] = '\0';
  dst[0] = '\0';
  ret = FoldStringW(MAP_FOLDCZONE, src, -1, dst, 256);
  ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
  ok(dst[0] == 'A' && dst[1] == '\0',
     "srclen=-1: Expected ret=2 [%d,%d], got ret=%d [%d,%d], err=%ld\n",
     'A', '\0', ret, dst[0], dst[1], GetLastError());

  /* If size is given, result is not NUL terminated */
  SetLastError(0xdeadbeef);
  src[0] = 'A';
  src[1] = 'A';
  dst[0] = 'X';
  dst[1] = 'X';
  ret = FoldStringW(MAP_FOLDCZONE, src, 1, dst, 256);
  ok(ret == 1, "Expected ret == 1, got %d, error %ld\n", ret, GetLastError());
  ok(dst[0] == 'A' && dst[1] == 'X',
     "srclen=1: Expected ret=1, [%d,%d], got ret=%d,[%d,%d], err=%ld\n",
     'A','X', ret, dst[0], dst[1], GetLastError());

  /* MAP_FOLDDIGITS */
  for (j = 0; j < ARRAY_SIZE(digitRanges); j++)
  {
    /* Check everything before this range */
    for (ch = prev_ch; ch < digitRanges[j].ch; ch++)
    {
        len = put_utf16( src, ch );
        src[len] = 0;
        SetLastError(0xdeadbeef);
        ret = FoldStringW(MAP_FOLDDIGITS, src, -1, dst, 256);
        if (ret == 3)
        {
            ok( !wcscmp( src, dst ), "%s changed to %s\n", debugstr_w(src), debugstr_w(dst) );
            continue;
        }
        ok(ret == 2, "Expected ret == 2, got %d, error %ld\n", ret, GetLastError());
        ok(dst[0] == ch, "MAP_FOLDDIGITS: ch 0x%04x Expected unchanged got %04x\n", ch, dst[0]);
        if (ch < 0x10000)
        {
            WCHAR wch = ch;
            GetStringTypeW( CT_CTYPE1, &wch, 1, &type );
            ok(!(type & C1_DIGIT), "char %04x should not be a digit\n", wch );
        }
    }
    if (digitRanges[j].ch == 0x10ffff)
      break; /* Finished the whole code point space */

    for (ch = digitRanges[j].ch; ch <= digitRanges[j].ch + digitRanges[j].last - digitRanges[j].first; ch++)
    {
        UINT exp = '0' + digitRanges[j].first + ch - digitRanges[j].ch;

        SetLastError(0xdeadbeef);
        len = put_utf16( src, ch );
        src[len] = 0;
        ret = FoldStringW(MAP_FOLDDIGITS, src, -1, dst, 256);
        ok(ret == 2 || broken( digitRanges[j].broken && ch >= 0x10000 ),
           "%04x: Expected ret == 2, got %d, error %ld\n", ch, ret, GetLastError());
        ok((dst[0] == exp && dst[1] == '\0') || broken( digitRanges[j].broken ),
           "MAP_FOLDDIGITS: ch %04x Expected %04x got %04x\n", ch, exp, dst[0]);
    }
    prev_ch = ch;
  }

  /* MAP_FOLDCZONE */
  SetLastError(0xdeadbeef);
  ret = FoldStringW(MAP_FOLDCZONE, foldczone_src, -1, dst, 256);
  ok(ret == ARRAY_SIZE(foldczone_dst)
     || broken(ret == ARRAY_SIZE(foldczone_broken_dst)), /* winxp, win2003 */
     "Got %d, error %ld.\n", ret, GetLastError());
  ok(!memcmp(dst, foldczone_dst, sizeof(foldczone_dst))
     || broken(!memcmp(dst, foldczone_broken_dst, sizeof(foldczone_broken_dst))), /* winxp, win2003 */
     "Got unexpected string %s.\n", wine_dbgstr_w(dst));

    /* MAP_EXPAND_LIGATURES */
    SetLastError(0xdeadbeef);
    ret = FoldStringW(MAP_EXPAND_LIGATURES, ligatures_src, -1, dst, 256);
    ok(ret == ARRAY_SIZE(ligatures_dst), "Got %d, error %ld\n", ret, GetLastError());
    ok(!memcmp(dst, ligatures_dst, sizeof(ligatures_dst)),
       "Got unexpected string %s.\n", wine_dbgstr_w(dst));

  /* FIXME: MAP_PRECOMPOSED : MAP_COMPOSITE */
}



#define LCID_OK(l) \
  ok(lcid == l, "Expected lcid = %08lx, got %08lx\n", l, lcid)
#define MKLCID(x,y,z) MAKELCID(MAKELANGID(x, y), z)
#define LCID_RES(src, res) do { lcid = ConvertDefaultLocale(src); LCID_OK(res); } while (0)
#define TEST_LCIDLANG(a,b) LCID_RES(MAKELCID(a,b), MAKELCID(a,b))
#define TEST_LCID(a,b,c) LCID_RES(MKLCID(a,b,c), MKLCID(a,b,c))

static void test_ConvertDefaultLocale(void)
{
    /* some languages use a different default than SUBLANG_DEFAULT */
    static const struct { WORD lang, sublang; } nondefault_langs[] =
    {
        { LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED },
        { LANG_SPANISH, SUBLANG_SPANISH_MODERN },
        { LANG_IRISH, SUBLANG_IRISH_IRELAND },
        { LANG_BENGALI, SUBLANG_BENGALI_BANGLADESH },
        { LANG_SINDHI, SUBLANG_SINDHI_AFGHANISTAN },
        { LANG_INUKTITUT, SUBLANG_INUKTITUT_CANADA_LATIN },
        { LANG_TAMAZIGHT, SUBLANG_TAMAZIGHT_ALGERIA_LATIN },
        { LANG_FULAH, SUBLANG_FULAH_SENEGAL },
        { LANG_TIGRINYA, SUBLANG_TIGRINYA_ERITREA }
    };
    LCID lcid;
    unsigned int i;

  /* Doesn't change lcid, even if non default sublang/sort used */
  TEST_LCID(LANG_ENGLISH,  SUBLANG_ENGLISH_US, SORT_DEFAULT);
  TEST_LCID(LANG_ENGLISH,  SUBLANG_ENGLISH_UK, SORT_DEFAULT);
  TEST_LCID(LANG_JAPANESE, SUBLANG_DEFAULT,    SORT_DEFAULT);
  TEST_LCID(LANG_JAPANESE, SUBLANG_DEFAULT,    SORT_JAPANESE_UNICODE);
  lcid = ConvertDefaultLocale( MKLCID( LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE ));
  ok( lcid == MKLCID( LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE ) ||
      broken( lcid == MKLCID( LANG_JAPANESE, SUBLANG_DEFAULT, SORT_JAPANESE_UNICODE )), /* <= vista */
          "Expected lcid = %08lx got %08lx\n",
      MKLCID( LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE ), lcid );
  lcid = ConvertDefaultLocale( MKLCID( LANG_IRISH, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE ));
  ok( lcid == MKLCID( LANG_IRISH, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE ) ||
      broken( lcid == MKLCID( LANG_IRISH, SUBLANG_DEFAULT, SORT_JAPANESE_UNICODE )), /* <= vista */
          "Expected lcid = %08lx got %08lx\n",
      MKLCID( LANG_IRISH, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE ), lcid );

  /* SUBLANG_NEUTRAL -> SUBLANG_DEFAULT */
  LCID_RES(MKLCID(LANG_ENGLISH,  SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_ENGLISH,  SUBLANG_DEFAULT, SORT_DEFAULT));
  LCID_RES(MKLCID(LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_JAPANESE, SUBLANG_DEFAULT, SORT_DEFAULT));
  for (i = 0; i < ARRAY_SIZE(nondefault_langs); i++)
  {
      lcid = ConvertDefaultLocale( MAKELANGID( nondefault_langs[i].lang, SUBLANG_NEUTRAL ));
      ok( lcid == MAKELANGID( nondefault_langs[i].lang, nondefault_langs[i].sublang ) ||
          broken( lcid == MAKELANGID( nondefault_langs[i].lang, SUBLANG_DEFAULT )) ||  /* <= vista */
          broken( lcid == MAKELANGID( nondefault_langs[i].lang, SUBLANG_NEUTRAL )),  /* w7 */
          "Expected lcid = %08x got %08lx\n",
          MAKELANGID( nondefault_langs[i].lang, nondefault_langs[i].sublang ), lcid );
  }
  lcid = ConvertDefaultLocale( 0x7804 );
  ok( lcid == MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED ) ||
      broken( lcid == 0x7804 ),  /* <= vista */
      "Expected lcid = %08x got %08lx\n", MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED ), lcid );
  lcid = ConvertDefaultLocale( 0x7c04 );
  ok( lcid == MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG ) ||
      broken( lcid == 0x7c04 ) ||  /* winxp */
      broken( lcid == 0x0404 ),  /* vista */
      "Expected lcid = %08x got %08lx\n", MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_HONGKONG ), lcid );
  lcid = ConvertDefaultLocale( LANG_SERBIAN_NEUTRAL );
  ok( lcid == MAKELANGID( LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_LATIN ) ||
      broken( lcid == MAKELANGID( LANG_SERBIAN, SUBLANG_SERBIAN_LATIN ) ),  /* <= vista */
      "Expected lcid = %08x got %08lx\n", MAKELANGID( LANG_SERBIAN, SUBLANG_SERBIAN_SERBIA_LATIN ), lcid );

  /* Invariant language is not treated specially */
  TEST_LCID(LANG_INVARIANT, SUBLANG_DEFAULT, SORT_DEFAULT);

  /* User/system default languages alone are not mapped */
  TEST_LCIDLANG(LANG_SYSTEM_DEFAULT, SORT_JAPANESE_UNICODE);
  TEST_LCIDLANG(LANG_USER_DEFAULT,   SORT_JAPANESE_UNICODE);

  /* Default lcids */
  LCID_RES(LOCALE_SYSTEM_DEFAULT, GetSystemDefaultLCID());
  LCID_RES(LOCALE_USER_DEFAULT,   GetUserDefaultLCID());
  LCID_RES(LOCALE_NEUTRAL,        GetUserDefaultLCID());
  LCID_RES(LOCALE_CUSTOM_DEFAULT, GetUserDefaultLCID());
  lcid = ConvertDefaultLocale( LOCALE_CUSTOM_UNSPECIFIED );
  ok( lcid == GetUserDefaultLCID() || broken(lcid == LOCALE_CUSTOM_UNSPECIFIED), /* <= win8 */
      "wrong lcid %04lx\n", lcid );
  lcid = ConvertDefaultLocale( LOCALE_CUSTOM_UI_DEFAULT );
  ok( lcid == GetUserDefaultUILanguage() || lcid == LOCALE_CUSTOM_UI_DEFAULT, "wrong lcid %04lx\n", lcid );
  lcid = ConvertDefaultLocale(LOCALE_INVARIANT);
  ok(lcid == LOCALE_INVARIANT || broken(lcid == 0x47f) /* win2k[3]/winxp */,
     "Expected lcid = %08lx, got %08lx\n", LOCALE_INVARIANT, lcid);
}

static BOOL CALLBACK langgrp_procA(LGRPID lgrpid, LPSTR lpszNum, LPSTR lpszName,
                                    DWORD dwFlags, LONG_PTR lParam)
{
  if (winetest_debug > 1)
    trace("%08lx, %s, %s, %08lx, %08Ix\n",
          lgrpid, lpszNum, lpszName, dwFlags, lParam);

  ok(pIsValidLanguageGroup(lgrpid, dwFlags) == TRUE,
     "Enumerated grp %ld not valid (flags %ld)\n", lgrpid, dwFlags);

  /* If lParam is one, we are calling with flags defaulted from 0 */
  ok(!lParam || (dwFlags == LGRPID_INSTALLED || dwFlags == LGRPID_SUPPORTED),
         "Expected dwFlags == LGRPID_INSTALLED || dwFlags == LGRPID_SUPPORTED, got %ld\n", dwFlags);

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
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* Invalid flags */
  SetLastError(0);
  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_INSTALLED|LGRPID_SUPPORTED, 0);
  ok(GetLastError() == ERROR_INVALID_FLAGS, "Expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError());

  /* No flags - defaults to LGRPID_INSTALLED */
  SetLastError(0xdeadbeef);
  pEnumSystemLanguageGroupsA(langgrp_procA, 0, 1);
  ok(GetLastError() == 0xdeadbeef, "got error %ld\n", GetLastError());

  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_INSTALLED, 0);
  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_SUPPORTED, 0);
}

static LONG default_seen;
static LONG alternate_seen;

static BOOL CALLBACK test_EnumSystemLocalesA_callback(LPSTR str)
{
    LCID lcid;
    WORD sortid;

    if (sscanf(str, "%lx", &lcid) != 1)
    {
        ok(FALSE, "EnumSystemLocalesA callback received unparsable LCID string \"%s\"\n", str);
        return FALSE;
    }

    sortid = SORTIDFROMLCID(lcid);
    if (sortid == SORT_DEFAULT)
    {
        default_seen++;
    }
    else
    {
        alternate_seen++;
    }

    return TRUE;
}

static BOOL CALLBACK test_EnumSystemLocalesW_callback(LPWSTR str)
{
    LCID lcid;
    WORD sortid;

    if (swscanf(str, L"%lx", &lcid) != 1)
    {
        ok(FALSE, "unparsable LCID string %s\n", debugstr_w(str));
        return FALSE;
    }

    sortid = SORTIDFROMLCID(lcid);
    if (sortid == SORT_DEFAULT)
    {
        default_seen++;
    }
    else
    {
        alternate_seen++;
    }

    return TRUE;
}

static void test_EnumSystemLocalesA(void)
{
    if (!pEnumSystemLocalesA)
    {
        win_skip("EnumSystemLocalesA not available");
        return;
    }

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesA(test_EnumSystemLocalesA_callback, 0);
    ok(default_seen, "EnumSystemLocalesA(..., 0) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(!alternate_seen, "EnumSystemLocalesA(..., 0) returned %ld locales "
            "with alternate sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesA(test_EnumSystemLocalesA_callback, LCID_INSTALLED);
    ok(default_seen, "EnumSystemLocalesA(..., LCID_INSTALLED) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(!alternate_seen, "EnumSystemLocalesA(..., LCID_INSTALLED) returned %ld locales "
            "with alternate sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesA(test_EnumSystemLocalesA_callback, LCID_SUPPORTED);
    ok(default_seen, "EnumSystemLocalesA(..., LCID_SUPPORTED) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(!alternate_seen, "EnumSystemLocalesA(..., LCID_SUPPORTED) returned %ld locales "
            "with alternate sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesA(test_EnumSystemLocalesA_callback, LCID_ALTERNATE_SORTS);
    ok(alternate_seen, "EnumSystemLocalesA(..., LCID_ALTERNATE_SORTS) returned 0 locales "
            "with alternate sort order, expected > 0\n");
    ok(!default_seen, "EnumSystemLocalesA(..., LCID_ALTERNATE_SORTS) returned %ld locales "
            "with default sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesA(test_EnumSystemLocalesA_callback, LCID_INSTALLED | LCID_ALTERNATE_SORTS);
    ok(default_seen, "EnumSystemLocalesA(..., LCID_INSTALLED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(alternate_seen, "EnumSystemLocalesA(..., LCID_INSTALLED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with alternate sort order, expected > 0\n");

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesA(test_EnumSystemLocalesA_callback, LCID_SUPPORTED | LCID_ALTERNATE_SORTS);
    ok(default_seen, "EnumSystemLocalesA(..., LCID_SUPPORTED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(alternate_seen, "EnumSystemLocalesA(..., LCID_SUPPORTED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with alternate sort order, expected > 0\n");
}

static void test_EnumSystemLocalesW(void)
{
    if (!pEnumSystemLocalesW)
    {
        win_skip("EnumSystemLocalesW not available");
        return;
    }

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesW(test_EnumSystemLocalesW_callback, 0);
    ok(default_seen, "EnumSystemLocalesW(..., 0) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(!alternate_seen, "EnumSystemLocalesW(..., 0) returned %ld locales "
            "with alternate sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesW(test_EnumSystemLocalesW_callback, LCID_INSTALLED);
    ok(default_seen, "EnumSystemLocalesW(..., LCID_INSTALLED) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(!alternate_seen, "EnumSystemLocalesW(..., LCID_INSTALLED) returned %ld locales "
            "with alternate sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesW(test_EnumSystemLocalesW_callback, LCID_SUPPORTED);
    ok(default_seen, "EnumSystemLocalesW(..., LCID_SUPPORTED) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(!alternate_seen, "EnumSystemLocalesW(..., LCID_SUPPORTED) returned %ld locales "
            "with alternate sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesW(test_EnumSystemLocalesW_callback, LCID_ALTERNATE_SORTS);
    ok(alternate_seen, "EnumSystemLocalesW(..., LCID_ALTERNATE_SORTS) returned 0 locales "
            "with alternate sort order, expected > 0\n");
    ok(!default_seen, "EnumSystemLocalesW(..., LCID_ALTERNATE_SORTS) returned %ld locales "
            "with default sort order, expected 0\n", alternate_seen);

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesW(test_EnumSystemLocalesW_callback, LCID_INSTALLED | LCID_ALTERNATE_SORTS);
    ok(default_seen, "EnumSystemLocalesW(..., LCID_INSTALLED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(alternate_seen, "EnumSystemLocalesW(..., LCID_INSTALLED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with alternate sort order, expected > 0\n");

    default_seen = 0;
    alternate_seen = 0;

    pEnumSystemLocalesW(test_EnumSystemLocalesW_callback, LCID_SUPPORTED | LCID_ALTERNATE_SORTS);
    ok(default_seen, "EnumSystemLocalesW(..., LCID_SUPPORTED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with default sort order, expected > 0\n");
    ok(alternate_seen, "EnumSystemLocalesW(..., LCID_SUPPORTED | LCID_ALTERNATE_SORTS) returned 0 locales "
            "with alternate sort order, expected > 0\n");
}

static BOOL CALLBACK enum_func( LPWSTR name, DWORD flags, LPARAM lparam )
{
    if (winetest_debug > 1)
        trace( "%s %lx\n", wine_dbgstr_w(name), flags );
    return TRUE;
}

static void test_EnumSystemLocalesEx(void)
{
    BOOL ret;

    if (!pEnumSystemLocalesEx)
    {
        win_skip( "EnumSystemLocalesEx not available\n" );
        return;
    }
    SetLastError( 0xdeadbeef );
    ret = pEnumSystemLocalesEx( enum_func, LOCALE_ALL, 0, (void *)1 );
    ok( !ret, "should have failed\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pEnumSystemLocalesEx( enum_func, 0, 0, NULL );
    ok( ret, "failed err %lu\n", GetLastError() );
}

static BOOL CALLBACK lgrplocale_procA(LGRPID lgrpid, LCID lcid, LPSTR lpszNum,
                                      LONG_PTR lParam)
{
  if (winetest_debug > 1)
    trace("%08lx, %08lx, %s, %08Ix\n", lgrpid, lcid, lpszNum, lParam);

  /* invalid locale enumerated on some platforms */
  if (lcid == 0)
      return TRUE;

  ok(pIsValidLanguageGroup(lgrpid, LGRPID_SUPPORTED) == TRUE,
     "Enumerated grp %ld not valid\n", lgrpid);
  ok(IsValidLocale(lcid, LCID_SUPPORTED) == TRUE,
     "Enumerated grp locale %04lx not valid\n", lcid);
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
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* lgrpid too small */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(lgrplocale_procA, 0, 0, 0);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* lgrpid too big */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(lgrplocale_procA, LGRPID_ARMENIAN + 1, 0, 0);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  /* dwFlags is reserved */
  SetLastError(0);
  ret = pEnumLanguageGroupLocalesA(0, LGRPID_WESTERN_EUROPE, 0x1, 0);
  ok( !ret && GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  pEnumLanguageGroupLocalesA(lgrplocale_procA, LGRPID_WESTERN_EUROPE, 0, 0);
}

static void test_SetLocaleInfo(void)
{
    BOOL bRet;
    LCID lcid = GetUserDefaultLCID();
    UINT i;

    /* Null data */
    SetLastError(0xdeadbeef);
    bRet = SetLocaleInfoA(lcid, LOCALE_SSHORTDATE, NULL);
    ok( !bRet && GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = SetLocaleInfoW(lcid, LOCALE_SSHORTDATE, NULL);
    ok( !bRet && GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    bRet = SetLocaleInfoW(lcid, LOCALE_SDAYNAME1, NULL);
    ok( !bRet && GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    for (i = 0; i <= 0x1014; i++)
    {
        WCHAR buffer[80];
        if (!GetLocaleInfoW( LOCALE_USER_DEFAULT, i, buffer, ARRAY_SIZE(buffer) )) continue;
        SetLastError(0xdeadbeef);
        bRet = SetLocaleInfoW(lcid, i, buffer);
        switch (i)
        {
        case LOCALE_ICALENDARTYPE:
        case LOCALE_ICURRDIGITS:
        case LOCALE_ICURRENCY:
        case LOCALE_IDIGITS:
        case LOCALE_IDIGITSUBSTITUTION:
        case LOCALE_IFIRSTDAYOFWEEK:
        case LOCALE_IFIRSTWEEKOFYEAR:
        case LOCALE_ILZERO:
        case LOCALE_IMEASURE:
        case LOCALE_INEGCURR:
        case LOCALE_INEGNUMBER:
        case LOCALE_IPAPERSIZE:
        case LOCALE_ITIME:
        case LOCALE_S1159:
        case LOCALE_S2359:
        case LOCALE_SCURRENCY:
        case LOCALE_SDATE:
        case LOCALE_SDECIMAL:
        case LOCALE_SGROUPING:
        case LOCALE_SLIST:
        case LOCALE_SLONGDATE:
        case LOCALE_SMONDECIMALSEP:
        case LOCALE_SMONGROUPING:
        case LOCALE_SMONTHOUSANDSEP:
        case LOCALE_SNATIVEDIGITS:
        case LOCALE_SNEGATIVESIGN:
        case LOCALE_SPOSITIVESIGN:
        case LOCALE_SSHORTDATE:
        case LOCALE_SSHORTTIME:
        case LOCALE_STHOUSAND:
        case LOCALE_STIME:
        case LOCALE_STIMEFORMAT:
        case LOCALE_SYEARMONTH:
            ok( bRet, "%04x: failed err %lu\n", i, GetLastError() );
            break;
        case LOCALE_SINTLSYMBOL:
            ok( bRet || broken(!bRet), /* win10 <= 1507 */
                "%04x: failed err %lu\n", i, GetLastError() );
            break;
        default:
            ok( !bRet, "%04x: succeeded\n", i );
            ok( GetLastError() == ERROR_INVALID_FLAGS, "%04x: wrong error %lu\n", i, GetLastError() );
            break;
        }
    }
}

static BOOL CALLBACK luilocale_proc1A(LPSTR value, LONG_PTR lParam)
{
  if (winetest_debug > 1)
    trace("%s %08Ix\n", value, lParam);
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
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());

  SetLastError(0xdeadbeef);
  ret = pEnumUILanguagesA(luilocale_proc1A, MUI_LANGUAGE_NAME, 0);
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());

  enumCount = 0;
  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(luilocale_proc2A, 0, 0);
  ok(ret, "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());
  ok(enumCount == 1, "enumCount = %u\n", enumCount);

  enumCount = 0;
  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(luilocale_proc2A, MUI_LANGUAGE_ID, 0);
  ok(ret || broken(!ret && GetLastError() == ERROR_INVALID_FLAGS), /* winxp */
     "Expected ret != 0, got %d, error %ld\n", ret, GetLastError());
  if (ret) ok(enumCount == 1, "enumCount = %u\n", enumCount);

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(NULL, 0, 0);
  ok(!ret, "Expected return value FALSE, got %u\n", ret);
  ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(luilocale_proc3A, 0x5a5a5a5a, 0);
  ok(!ret, "Expected return value FALSE, got %u\n", ret);
  ok(GetLastError() == ERROR_INVALID_FLAGS, "Expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError());

  SetLastError(ERROR_SUCCESS);
  ret = pEnumUILanguagesA(NULL, 0x5a5a5a5a, 0);
  ok(!ret, "Expected return value FALSE, got %u\n", ret);
  ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
}

static char date_fmt_buf[1024];
static WCHAR date_fmt_bufW[1024];

static BOOL CALLBACK enum_datetime_procA(LPSTR fmt)
{
    lstrcatA(date_fmt_buf, fmt);
    lstrcatA(date_fmt_buf, "\n");
    return TRUE;
}

static BOOL CALLBACK enum_datetime_procW(WCHAR *fmt)
{
    lstrcatW(date_fmt_bufW, fmt);
    return FALSE;
}

static void test_EnumDateFormatsA(void)
{
    char *p, buf[256];
    BOOL ret;
    LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

    date_fmt_buf[0] = 0;
    SetLastError(0xdeadbeef);
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, 0);
    if (!ret && (GetLastError() == ERROR_INVALID_FLAGS))
    {
        win_skip("0 for dwFlags is not supported\n");
    }
    else
    {
        ok(ret, "EnumDateFormatsA(0) error %ld\n", GetLastError());
        trace("EnumDateFormatsA(0): %s\n", date_fmt_buf);
        /* test the 1st enumerated format */
        if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
        ret = GetLocaleInfoA(lcid, LOCALE_SSHORTDATE, buf, sizeof(buf));
        ok(ret, "GetLocaleInfoA(LOCALE_SSHORTDATE) error %ld\n", GetLastError());
        ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
    }

    date_fmt_buf[0] = 0;
    SetLastError(0xdeadbeef);
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, LOCALE_USE_CP_ACP);
    if (!ret && (GetLastError() == ERROR_INVALID_FLAGS))
    {
        win_skip("LOCALE_USE_CP_ACP is not supported\n");
    }
    else
    {
        ok(ret, "EnumDateFormatsA(LOCALE_USE_CP_ACP) error %ld\n", GetLastError());
        trace("EnumDateFormatsA(LOCALE_USE_CP_ACP): %s\n", date_fmt_buf);
        /* test the 1st enumerated format */
        if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
        ret = GetLocaleInfoA(lcid, LOCALE_SSHORTDATE, buf, sizeof(buf));
        ok(ret, "GetLocaleInfoA(LOCALE_SSHORTDATE) error %ld\n", GetLastError());
        ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
    }

    date_fmt_buf[0] = 0;
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, DATE_SHORTDATE);
    ok(ret, "EnumDateFormatsA(DATE_SHORTDATE) error %ld\n", GetLastError());
    trace("EnumDateFormatsA(DATE_SHORTDATE): %s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_SSHORTDATE, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_SSHORTDATE) error %ld\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);

    date_fmt_buf[0] = 0;
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, DATE_LONGDATE);
    ok(ret, "EnumDateFormatsA(DATE_LONGDATE) error %ld\n", GetLastError());
    trace("EnumDateFormatsA(DATE_LONGDATE): %s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_SLONGDATE, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_SLONGDATE) error %ld\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);

    date_fmt_buf[0] = 0;
    SetLastError(0xdeadbeef);
    ret = EnumDateFormatsA(enum_datetime_procA, lcid, DATE_YEARMONTH);
    if (!ret && (GetLastError() == ERROR_INVALID_FLAGS))
    {
        win_skip("DATE_YEARMONTH is only present on W2K and later\n");
        return;
    }
    ok(ret, "EnumDateFormatsA(DATE_YEARMONTH) error %ld\n", GetLastError());
    trace("EnumDateFormatsA(DATE_YEARMONTH): %s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_SYEARMONTH, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_SYEARMONTH) error %ld\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf) || broken(!buf[0]) /* win9x */,
       "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
}

static void test_EnumTimeFormatsA(void)
{
    char *p, buf[256];
    BOOL ret;
    LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

    date_fmt_buf[0] = 0;
    ret = EnumTimeFormatsA(enum_datetime_procA, lcid, 0);
    ok(ret, "EnumTimeFormatsA(0) error %ld\n", GetLastError());
    trace("EnumTimeFormatsA(0): %s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_STIMEFORMAT, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_STIMEFORMAT) error %ld\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);

    date_fmt_buf[0] = 0;
    ret = EnumTimeFormatsA(enum_datetime_procA, lcid, LOCALE_USE_CP_ACP);
    ok(ret, "EnumTimeFormatsA(LOCALE_USE_CP_ACP) error %ld\n", GetLastError());
    trace("EnumTimeFormatsA(LOCALE_USE_CP_ACP): %s\n", date_fmt_buf);
    /* test the 1st enumerated format */
    if ((p = strchr(date_fmt_buf, '\n'))) *p = 0;
    ret = GetLocaleInfoA(lcid, LOCALE_STIMEFORMAT, buf, sizeof(buf));
    ok(ret, "GetLocaleInfoA(LOCALE_STIMEFORMAT) error %ld\n", GetLastError());
    ok(!lstrcmpA(date_fmt_buf, buf), "expected \"%s\" got \"%s\"\n", date_fmt_buf, buf);
}

static void test_EnumTimeFormatsW(void)
{
    LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    WCHAR bufW[256];
    BOOL ret;

    date_fmt_bufW[0] = 0;
    ret = EnumTimeFormatsW(enum_datetime_procW, lcid, 0);
    ok(ret, "EnumTimeFormatsW(0) error %ld\n", GetLastError());
    ret = GetLocaleInfoW(lcid, LOCALE_STIMEFORMAT, bufW, ARRAY_SIZE(bufW));
    ok(ret, "GetLocaleInfoW(LOCALE_STIMEFORMAT) error %ld\n", GetLastError());
    ok(!lstrcmpW(date_fmt_bufW, bufW), "expected \"%s\" got \"%s\"\n", wine_dbgstr_w(date_fmt_bufW),
        wine_dbgstr_w(bufW));

    date_fmt_bufW[0] = 0;
    ret = EnumTimeFormatsW(enum_datetime_procW, lcid, LOCALE_USE_CP_ACP);
    ok(ret, "EnumTimeFormatsW(LOCALE_USE_CP_ACP) error %ld\n", GetLastError());
    ret = GetLocaleInfoW(lcid, LOCALE_STIMEFORMAT, bufW, ARRAY_SIZE(bufW));
    ok(ret, "GetLocaleInfoW(LOCALE_STIMEFORMAT) error %ld\n", GetLastError());
    ok(!lstrcmpW(date_fmt_bufW, bufW), "expected \"%s\" got \"%s\"\n", wine_dbgstr_w(date_fmt_bufW),
        wine_dbgstr_w(bufW));

    /* TIME_NOSECONDS is Win7+ feature */
    date_fmt_bufW[0] = 0;
    ret = EnumTimeFormatsW(enum_datetime_procW, lcid, TIME_NOSECONDS);
    if (!ret && GetLastError() == ERROR_INVALID_FLAGS)
        win_skip("EnumTimeFormatsW doesn't support TIME_NOSECONDS\n");
    else {
        char buf[256];

        ok(ret, "EnumTimeFormatsW(TIME_NOSECONDS) error %ld\n", GetLastError());
        ret = GetLocaleInfoW(lcid, LOCALE_SSHORTTIME, bufW, ARRAY_SIZE(bufW));
        ok(ret, "GetLocaleInfoW(LOCALE_SSHORTTIME) error %ld\n", GetLastError());
        ok(!lstrcmpW(date_fmt_bufW, bufW), "expected \"%s\" got \"%s\"\n", wine_dbgstr_w(date_fmt_bufW),
            wine_dbgstr_w(bufW));

        /* EnumTimeFormatsA doesn't support this flag */
        ret = EnumTimeFormatsA(enum_datetime_procA, lcid, TIME_NOSECONDS);
        ok(!ret && GetLastError() == ERROR_INVALID_FLAGS, "EnumTimeFormatsA(TIME_NOSECONDS) ret %d, error %ld\n", ret,
            GetLastError());

        ret = EnumTimeFormatsA(NULL, lcid, TIME_NOSECONDS);
        ok(!ret && GetLastError() == ERROR_INVALID_FLAGS, "EnumTimeFormatsA(TIME_NOSECONDS) ret %d, error %ld\n", ret,
            GetLastError());

        /* And it's not supported by GetLocaleInfoA either */
        ret = GetLocaleInfoA(lcid, LOCALE_SSHORTTIME, buf, ARRAY_SIZE(buf));
        ok(!ret && GetLastError() == ERROR_INVALID_FLAGS, "GetLocaleInfoA(LOCALE_SSHORTTIME) ret %d, error %ld\n", ret,
            GetLastError());
    }
}

static void test_GetCPInfo(void)
{
    BOOL ret;
    CPINFO cpinfo;
    CPINFOEXW cpiw;

    SetLastError(0xdeadbeef);
    ret = GetCPInfo(CP_SYMBOL, &cpinfo);
    ok(!ret, "GetCPInfo(CP_SYMBOL) should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    memset(cpinfo.LeadByte, '-', ARRAY_SIZE(cpinfo.LeadByte));
    SetLastError(0xdeadbeef);
    ret = GetCPInfo(CP_UTF7, &cpinfo);
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("Codepage CP_UTF7 is not installed/available\n");
    }
    else
    {
        unsigned int i;

        ok(ret, "GetCPInfo(CP_UTF7) error %lu\n", GetLastError());
        ok(cpinfo.DefaultChar[0] == 0x3f, "expected 0x3f, got 0x%x\n", cpinfo.DefaultChar[0]);
        ok(cpinfo.DefaultChar[1] == 0, "expected 0, got 0x%x\n", cpinfo.DefaultChar[1]);
        for (i = 0; i < sizeof(cpinfo.LeadByte); i++)
            ok(!cpinfo.LeadByte[i], "expected NUL byte in index %u\n", i);
        ok(cpinfo.MaxCharSize == 5, "expected 5, got 0x%x\n", cpinfo.MaxCharSize);

        memset( &cpiw, 0xcc, sizeof(cpiw) );
        ret = GetCPInfoExW( CP_UTF7, 0, &cpiw );
        ok( ret, "GetCPInfoExW failed err %lu\n", GetLastError() );
        ok( cpiw.DefaultChar[0] == 0x3f, "wrong DefaultChar[0] %02x\n", cpiw.DefaultChar[0] );
        ok( cpiw.DefaultChar[1] == 0, "wrong DefaultChar[1] %02x\n", cpiw.DefaultChar[1] );
        for (i = 0; i < 12; i++) ok( cpiw.LeadByte[i] == 0, "wrong LeadByte[%u] %02x\n", i, cpiw.LeadByte[i] );
        ok( cpiw.MaxCharSize == 5, "wrong MaxCharSize %02x\n", cpiw.MaxCharSize );
        ok( cpiw.CodePage == CP_UTF7, "wrong CodePage %02x\n", cpiw.CodePage );
        ok( cpiw.UnicodeDefaultChar == 0xfffd, "wrong UnicodeDefaultChar %02x\n", cpiw.UnicodeDefaultChar );
        ok( !wcscmp( cpiw.CodePageName, L"65000 (UTF-7)" ),
            "wrong CodePageName %s\n", debugstr_w(cpiw.CodePageName) );
    }

    memset(cpinfo.LeadByte, '-', ARRAY_SIZE(cpinfo.LeadByte));
    SetLastError(0xdeadbeef);
    ret = GetCPInfo(CP_UTF8, &cpinfo);
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("Codepage CP_UTF8 is not installed/available\n");
    }
    else
    {
        unsigned int i;

        ok(ret, "GetCPInfo(CP_UTF8) error %lu\n", GetLastError());
        ok(cpinfo.DefaultChar[0] == 0x3f, "expected 0x3f, got 0x%x\n", cpinfo.DefaultChar[0]);
        ok(cpinfo.DefaultChar[1] == 0, "expected 0, got 0x%x\n", cpinfo.DefaultChar[1]);
        for (i = 0; i < sizeof(cpinfo.LeadByte); i++)
            ok(!cpinfo.LeadByte[i], "expected NUL byte in index %u\n", i);
        ok(cpinfo.MaxCharSize == 4, "expected 4, got %u\n", cpinfo.MaxCharSize);

        memset( &cpiw, 0xcc, sizeof(cpiw) );
        ret = GetCPInfoExW( CP_UTF8, 0, &cpiw );
        ok( ret, "GetCPInfoExW failed err %lu\n", GetLastError() );
        ok( cpiw.DefaultChar[0] == 0x3f, "wrong DefaultChar[0] %02x\n", cpiw.DefaultChar[0] );
        ok( cpiw.DefaultChar[1] == 0, "wrong DefaultChar[1] %02x\n", cpiw.DefaultChar[1] );
        for (i = 0; i < 12; i++) ok( cpiw.LeadByte[i] == 0, "wrong LeadByte[%u] %02x\n", i, cpiw.LeadByte[i] );
        ok( cpiw.MaxCharSize == 4, "wrong MaxCharSize %02x\n", cpiw.MaxCharSize );
        ok( cpiw.CodePage == CP_UTF8, "wrong CodePage %02x\n", cpiw.CodePage );
        ok( cpiw.UnicodeDefaultChar == 0xfffd, "wrong UnicodeDefaultChar %02x\n", cpiw.UnicodeDefaultChar );
        ok( !wcscmp( cpiw.CodePageName, L"65001 (UTF-8)" ),
            "wrong CodePageName %s\n", debugstr_w(cpiw.CodePageName) );
    }


    SetLastError( 0xdeadbeef );
    ret = GetCPInfoExW( 0xbeef, 0, &cpiw );
    ok( !ret, "GetCPInfoExW succeeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    if (pNtGetNlsSectionPtr)
    {
        CPTABLEINFO table;
        NTSTATUS status;
        void *ptr, *ptr2;
        SIZE_T size;
        int i;

        for (i = 0; i < 100; i++)
        {
            ptr = NULL;
            size = 0;
            status = pNtGetNlsSectionPtr( i, 9999, NULL, &ptr, &size );
            switch (i)
            {
            case 9:  /* sortkeys */
            case 13: /* unknown */
                ok( status == STATUS_INVALID_PARAMETER_1 || status == STATUS_INVALID_PARAMETER_3, /* vista */
                    "%u: failed %lx\n", i, status );
                break;
            case 10:  /* casemap */
                ok( status == STATUS_INVALID_PARAMETER_1 || status == STATUS_UNSUCCESSFUL,
                    "%u: failed %lx\n", i, status );
                break;
            case 11:  /* codepage */
            case 12:  /* normalization */
                ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "%u: failed %lx\n", i, status );
                break;
            case 14: /* unknown */
                ok( status == STATUS_INVALID_PARAMETER_1 ||
                    status == STATUS_SUCCESS, /* win11 */
                    "%u: failed %lx\n", i, status );
                break;
            default:
                ok( status == STATUS_INVALID_PARAMETER_1, "%u: failed %lx\n", i, status );
                break;
            }
        }

        /* casemap table */

        status = pNtGetNlsSectionPtr( 10, 0, NULL, &ptr, &size );
        if (status != STATUS_INVALID_PARAMETER_1)
        {
            ok( !status, "failed %lx\n", status );
            ok( size > 0x1000 && size <= 0x8000 , "wrong size %Ix\n", size );
            status = pNtGetNlsSectionPtr( 10, 0, NULL, &ptr2, &size );
            ok( !status, "failed %lx\n", status );
            ok( ptr != ptr2, "got same pointer\n" );
            ret = UnmapViewOfFile( ptr );
            ok( ret, "UnmapViewOfFile failed err %lu\n", GetLastError() );
            ret = UnmapViewOfFile( ptr2 );
            ok( ret, "UnmapViewOfFile failed err %lu\n", GetLastError() );
        }

        /* codepage tables */

        ptr = (void *)0xdeadbeef;
        size = 0xdeadbeef;
        status = pNtGetNlsSectionPtr( 11, 437, NULL, &ptr, &size );
        ok( !status, "failed %lx\n", status );
        ok( size > 0x10000 && size <= 0x20000, "wrong size %Ix\n", size );
        memset( &table, 0xcc, sizeof(table) );
        if (pRtlInitCodePageTable)
        {
            pRtlInitCodePageTable( ptr, &table );
            ok( table.CodePage == 437, "wrong codepage %u\n", table.CodePage );
            ok( table.MaximumCharacterSize == 1, "wrong char size %u\n", table.MaximumCharacterSize );
            ok( table.DefaultChar == '?', "wrong default char %x\n", table.DefaultChar );
            ok( !table.DBCSCodePage, "wrong dbcs %u\n", table.DBCSCodePage );
        }
        ret = UnmapViewOfFile( ptr );
        ok( ret, "UnmapViewOfFile failed err %lu\n", GetLastError() );

        status = pNtGetNlsSectionPtr( 11, 936, NULL, &ptr, &size );
        ok( !status, "failed %lx\n", status );
        ok( size > 0x30000 && size <= 0x40000, "wrong size %Ix\n", size );
        memset( &table, 0xcc, sizeof(table) );
        if (pRtlInitCodePageTable)
        {
            pRtlInitCodePageTable( ptr, &table );
            ok( table.CodePage == 936, "wrong codepage %u\n", table.CodePage );
            ok( table.MaximumCharacterSize == 2, "wrong char size %u\n", table.MaximumCharacterSize );
            ok( table.DefaultChar == '?', "wrong default char %x\n", table.DefaultChar );
            ok( table.DBCSCodePage == TRUE, "wrong dbcs %u\n", table.DBCSCodePage );

            if (pRtlCustomCPToUnicodeN)
            {
                static const unsigned char buf[] = { 0xbf, 0xb4, 0xc7, 0, 0x78 };
                static const WCHAR expect[][4] = { { 0xcccc, 0xcccc, 0xcccc, 0xcccc },
                                                   { 0x0000, 0xcccc, 0xcccc, 0xcccc },
                                                   { 0x770b, 0xcccc, 0xcccc, 0xcccc },
                                                   { 0x770b, 0x0000, 0xcccc, 0xcccc },
                                                   { 0x770b, 0x003f, 0xcccc, 0xcccc },
                                                   { 0x770b, 0x003f, 0x0078, 0xcccc } };
                WCHAR wbuf[5];
                DWORD i, j, reslen;

                for (i = 0; i <= sizeof(buf); i++)
                {
                    memset( wbuf, 0xcc, sizeof(wbuf) );
                    pRtlCustomCPToUnicodeN( &table, wbuf, sizeof(wbuf), &reslen, (char *)buf, i );
                    for (j = 0; j < 4; j++) if (expect[i][j] == 0xcccc) break;
                    ok( reslen == j * sizeof(WCHAR), "%lu: wrong len %lu\n", i, reslen );
                    for (j = 0; j < 4; j++)
                        ok( wbuf[j] == expect[i][j], "%lu: char %lu got %04x\n", i, j, wbuf[j] );
                }
            }
        }
        ret = UnmapViewOfFile( ptr );
        ok( ret, "UnmapViewOfFile failed err %lu\n", GetLastError() );

        status = pNtGetNlsSectionPtr( 11, 65001, NULL, &ptr, &size );
        ok( status == STATUS_OBJECT_NAME_NOT_FOUND || broken(!status), /* win10 1709 */
            "failed %lx\n", status );
        if (!status) UnmapViewOfFile( ptr );
        if (pRtlInitCodePageTable)
        {
            static USHORT utf8[20] = { 0, CP_UTF8 };

            memset( &table, 0xcc, sizeof(table) );
            pRtlInitCodePageTable( utf8, &table );
            ok( table.CodePage == CP_UTF8, "wrong codepage %u\n", table.CodePage );
            if (table.MaximumCharacterSize)
            {
                ok( table.MaximumCharacterSize == 4, "wrong char size %u\n", table.MaximumCharacterSize );
                ok( table.DefaultChar == '?', "wrong default char %x\n", table.DefaultChar );
                ok( table.UniDefaultChar == 0xfffd, "wrong default char %x\n", table.UniDefaultChar );
                ok( table.TransDefaultChar == '?', "wrong default char %x\n", table.TransDefaultChar );
                ok( table.TransUniDefaultChar == '?', "wrong default char %x\n", table.TransUniDefaultChar );
                ok( !table.DBCSCodePage, "wrong dbcs %u\n", table.DBCSCodePage );
                ok( !table.MultiByteTable, "wrong mbtable %p\n", table.MultiByteTable );
                ok( !table.WideCharTable, "wrong wctable %p\n", table.WideCharTable );
                ok( !table.DBCSRanges, "wrong ranges %p\n", table.DBCSRanges );
                ok( !table.DBCSOffsets, "wrong offsets %p\n", table.DBCSOffsets );
            }
            else win_skip( "utf-8 codepage not supported\n" );
        }

        /* normalization tables */

        for (i = 0; i < 100; i++)
        {
            status = pNtGetNlsSectionPtr( 12, i, NULL, &ptr, &size );
            switch (i)
            {
            case NormalizationC:
            case NormalizationD:
            case NormalizationKC:
            case NormalizationKD:
            case 13:  /* IDN */
                ok( !status, "%u: failed %lx\n", i, status );
                if (status) break;
                ok( size > 0x8000 && size <= 0x30000 , "wrong size %Ix\n", size );
                ret = UnmapViewOfFile( ptr );
                ok( ret, "UnmapViewOfFile failed err %lu\n", GetLastError() );
                break;
            default:
                ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "%u: failed %lx\n", i, status );
                break;
            }
        }
    }
    else win_skip( "NtGetNlsSectionPtr not supported\n" );
}

/*
 * The CT_TYPE1 has varied over windows version.
 * The current target for correct behavior is windows 7.
 * There was a big shift between windows 2000 (first introduced) and windows Xp
 * Most of the old values below are from windows 2000.
 * A smaller subset of changes happened between windows Xp and Window vista/7
 */
static void test_GetStringTypeW(void)
{
    static const WCHAR blanks[] = {0x9, 0x20, 0xa0, 0x3000, 0xfeff};
    static const WORD blanks_new[] = {C1_SPACE | C1_CNTRL | C1_BLANK | C1_DEFINED,
                                 C1_SPACE | C1_BLANK | C1_DEFINED,
                                 C1_SPACE | C1_BLANK | C1_DEFINED,
                                 C1_SPACE | C1_BLANK | C1_DEFINED,
                                 C1_CNTRL | C1_BLANK | C1_DEFINED};
    static const WORD blanks_old[] ={C1_SPACE | C1_CNTRL | C1_BLANK,
                                    C1_SPACE | C1_BLANK,
                                    C1_SPACE | C1_BLANK,
                                    C1_SPACE | C1_BLANK,
                                    C1_SPACE | C1_BLANK};

    static const WCHAR undefined[] = {0x378, 0x379, 0x5ff, 0xfff8, 0xfffe};

                                  /* Lu, Ll, Lt */
    static const WCHAR alpha[] = {0x47, 0x67, 0x1c5};
    static const WORD alpha_old[] = {C1_UPPER | C1_ALPHA,
                                     C1_LOWER | C1_ALPHA,
                                     C1_UPPER | C1_LOWER | C1_ALPHA,
                                     C1_ALPHA};

                                  /* Sk, Sk, Mn, So, Me */
    static const WCHAR oldpunc[] = { 0x2c2, 0x2e5, 0x322, 0x482, 0x6de,
                                 /* Sc, Sm, No,*/
                                     0xffe0, 0xffe9, 0x2153};

                                /* Lm, Nl, Cf, 0xad(Cf), 0x1f88 (Lt), Lo, Mc */
    static const WCHAR changed[] = {0x2b0, 0x2160, 0x600, 0xad, 0x1f88, 0x294, 0x903};
    static const WORD changed_old[] = { C1_PUNCT, C1_PUNCT, 0, C1_PUNCT, C1_UPPER | C1_ALPHA, C1_ALPHA, C1_PUNCT };
    static const WORD changed_xp[] = {C1_ALPHA | C1_DEFINED,
                                      C1_ALPHA | C1_DEFINED,
                                      C1_CNTRL | C1_DEFINED,
                                      C1_PUNCT | C1_DEFINED,
                                      C1_UPPER | C1_LOWER | C1_ALPHA | C1_DEFINED,
                                      C1_ALPHA | C1_LOWER | C1_DEFINED,
                                      C1_ALPHA | C1_DEFINED };
    static const WORD changed_new[] = { C1_ALPHA | C1_DEFINED,
                                      C1_ALPHA | C1_DEFINED,
                                      C1_CNTRL | C1_DEFINED,
                                      C1_PUNCT | C1_CNTRL | C1_DEFINED,
                                      C1_UPPER | C1_LOWER | C1_ALPHA | C1_DEFINED,
                                      C1_ALPHA | C1_DEFINED,
                                      C1_DEFINED
 };
                                /* Pc,  Pd, Ps, Pe, Pi, Pf, Po*/
    static const WCHAR punct[] = { 0x5f, 0x2d, 0x28, 0x29, 0xab, 0xbb, 0x21 };

    static const WCHAR punct_special[] = {0x24, 0x2b, 0x3c, 0x3e, 0x5e, 0x60,
                                          0x7c, 0x7e, 0xa2, 0xbe, 0xd7, 0xf7};
    static const WCHAR digit_special[] = {0xb2, 0xb3, 0xb9};
    static const WCHAR lower_special[] = {0x2071, 0x207f};
    static const WCHAR cntrl_special[] = {0x070f, 0x200c, 0x200d,
                  0x200e, 0x200f, 0x202a, 0x202b, 0x202c, 0x202d, 0x202e,
                  0x206a, 0x206b, 0x206c, 0x206d, 0x206e, 0x206f, 0xfeff,
                  0xfff9, 0xfffa, 0xfffb};
    static const WCHAR space_special[] = {0x09, 0x0d, 0x85};
    static const WCHAR alpha_thai[] = {0xe31, 0xe34, 0xe35, 0xe36, 0xe37, 0xe38, 0xe39, 0xe3a,
                                       0xe47, 0xe48, 0xe49, 0xe4a, 0xe4b, 0xe4c, 0xe4d, 0xe4e,
                                       0x1885, 0x1886};

    WORD types[20];
    WCHAR ch[2];
    BOOL ret;
    int i;

    /* NULL src */
    SetLastError(0xdeadbeef);
    ret = GetStringTypeW(CT_CTYPE1, NULL, 0, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetStringTypeW(CT_CTYPE1, NULL, 0, types);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetStringTypeW(CT_CTYPE1, NULL, 5, types);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %ld\n", GetLastError());

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, blanks, 5, types);
    for (i = 0; i < 5; i++)
        ok(types[i] == blanks_new[i] || broken(types[i] == blanks_old[i] || broken(types[i] == 0)), "incorrect type1 returned for %x -> (%x != %x)\n",blanks[i],types[i],blanks_new[i]);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, alpha, 3, types);
    for (i = 0; i < 3; i++)
        ok(types[i] == (C1_DEFINED | alpha_old[i]) || broken(types[i] == alpha_old[i]) || broken(types[i] == 0), "incorrect types returned for %x -> (%x != %x)\n",alpha[i], types[i],(C1_DEFINED | alpha_old[i]));
    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, undefined, 5, types);
    for (i = 0; i < 5; i++)
        ok(types[i] == 0, "incorrect types returned for %x -> (%x != 0)\n",undefined[i], types[i]);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, oldpunc, 8, types);
    for (i = 0; i < 8; i++)
        ok(types[i] == C1_DEFINED || broken(types[i] == C1_PUNCT) || broken(types[i] == 0), "incorrect types returned for %x -> (%x != %x)\n",oldpunc[i], types[i], C1_DEFINED);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, changed, 7, types);
    for (i = 0; i < 7; i++)
        ok(types[i] == changed_new[i] || broken(types[i] == changed_old[i]) || broken(types[i] == changed_xp[i]) || broken(types[i] == 0), "incorrect types returned for %x -> (%x != %x)\n",changed[i], types[i], changed_new[i]);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, punct, 7, types);
    for (i = 0; i < 7; i++)
        ok(types[i] == (C1_PUNCT | C1_DEFINED) || broken(types[i] == C1_PUNCT) || broken(types[i] == 0), "incorrect types returned for %x -> (%x != %x)\n",punct[i], types[i], (C1_PUNCT | C1_DEFINED));


    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, punct_special, 12, types);
    for (i = 0; i < 12; i++)
        ok(types[i]  & C1_PUNCT || broken(types[i] == 0), "incorrect types returned for %x -> (%x doest not have %x)\n",punct_special[i], types[i], C1_PUNCT);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, digit_special, 3, types);
    for (i = 0; i < 3; i++)
        ok(types[i] & C1_DIGIT || broken(types[i] == 0), "incorrect types returned for %x -> (%x doest not have = %x)\n",digit_special[i], types[i], C1_DIGIT);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, lower_special, 2, types);
    for (i = 0; i < 2; i++)
        ok(types[i] & C1_LOWER || broken(types[i] == C1_PUNCT) || broken(types[i] == 0), "incorrect types returned for %x -> (%x does not have %x)\n",lower_special[i], types[i], C1_LOWER);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, cntrl_special, 20, types);
    for (i = 0; i < 20; i++)
        ok(types[i] & C1_CNTRL || broken(types[i] == (C1_BLANK|C1_SPACE)) || broken(types[i] == C1_PUNCT) || broken(types[i] == 0), "incorrect types returned for %x -> (%x does not have %x)\n",cntrl_special[i], types[i], C1_CNTRL);

    memset(types,0,sizeof(types));
    GetStringTypeW(CT_CTYPE1, space_special, 3, types);
    for (i = 0; i < 3; i++)
        ok(types[i] & C1_SPACE || broken(types[i] == C1_CNTRL) || broken(types[i] == 0), "incorrect types returned for %x -> (%x does not have %x)\n",space_special[i], types[i], C1_SPACE );

    /* alpha is set for certain Thai and Mongolian */
    memset(types, 0, sizeof(types));
    GetStringTypeW(CT_CTYPE1, alpha_thai, 18, types);
    for (i = 0; i < 18; i++)
        ok(types[i] == (C1_ALPHA|C1_DEFINED), "incorrect types returned for %x -> (%x does not have %x)\n",alpha_thai[i], types[i], (C1_ALPHA|C1_DEFINED));

    /* surrogate pairs */
    ch[0] = 0xd800;
    memset(types, 0, sizeof(types));
    GetStringTypeW(CT_CTYPE3, ch, 1, types);
    if (types[0] == C3_NOTAPPLICABLE)
        win_skip("C3_HIGHSURROGATE/C3_LOWSURROGATE are not supported.\n");
    else {
        ok(types[0] == C3_HIGHSURROGATE, "got %x\n", types[0]);

        ch[0] = 0xdc00;
        memset(types, 0, sizeof(types));
        GetStringTypeW(CT_CTYPE3, ch, 1, types);
        ok(types[0] == C3_LOWSURROGATE, "got %x\n", types[0]);
    }

    /* Zl, Zp categories */
    ch[0] = 0x2028;
    ch[1] = 0x2029;
    memset(types, 0, sizeof(types));
    GetStringTypeW(CT_CTYPE1, ch, 2, types);
    ok(types[0] == (C1_DEFINED|C1_SPACE), "got %x\n", types[0]);
    ok(types[1] == (C1_DEFINED|C1_SPACE), "got %x\n", types[1]);

    /* check Arabic range for kashida flag */
    for (ch[0] = 0x600; ch[0] <= 0x6ff; ch[0] += 1)
    {
        types[0] = 0;
        ret = GetStringTypeW(CT_CTYPE3, ch, 1, types);
        ok(ret, "%#x: failed %d\n", ch[0], ret);
        if (ch[0] == 0x640) /* ARABIC TATWEEL (Kashida) */
            ok(types[0] & C3_KASHIDA, "%#x: type %#x\n", ch[0], types[0]);
        else
            ok(!(types[0] & C3_KASHIDA), "%#x: type %#x\n", ch[0], types[0]);
    }
}

static void test_IdnToNameprepUnicode(void)
{
    struct {
        int in_len;
        const WCHAR in[80];
        DWORD flags;
        DWORD ret;
        DWORD broken_ret;
        const WCHAR out[80];
        NTSTATUS status;
        NTSTATUS broken_status;
    } test_data[] = {
        /* 0 */
        { 5, L"test", 0, 5, 5, L"test" },
        { 3, L"a\xe111z", 0, 0, 0, L"a\xe111z", 0, STATUS_NO_UNICODE_TRANSLATION },
        { 4, L"t\0e", 0, 0, 0, {0}, STATUS_NO_UNICODE_TRANSLATION, STATUS_NO_UNICODE_TRANSLATION },
        { 1, L"T", 0, 1, 1, L"T" },
        { 1, {0}, 0, 0 },
        /* 5 */
        { 6, L" -/[]", 0, 6, 6, L" -/[]" },
        { 3, L"a-a", IDN_USE_STD3_ASCII_RULES, 3, 3, L"a-a" },
        { 3, L"aa-", IDN_USE_STD3_ASCII_RULES, 0, 0, L"aa-" },
        { -1, L"T\xdf\x130\x143\x37a\x6a\x30c \xaa", 0, 12, 12, L"tssi\x307\x144 \x3b9\x1f0 a" },
        { 11, L"t\xad\x34f\x2066\x180b\x180c\x180d\x200b\x200c\x200d", 0, 0, 2, L"t",
          STATUS_NO_UNICODE_TRANSLATION },
        /* 10 */
        { 2, {0x3b0}, 0, 2, 2, {0x3b0} },
        { 2, {0x380}, 0, 0, 2, {0x380} },
        { 2, {0x380}, IDN_ALLOW_UNASSIGNED, 2, 2, {0x380} },
        { 5, L"a..a", 0, 0, 0, L"a..a" },
        { 3, L"a.", 0, 3, 3, L"a." },
        /* 15 */
        { 5, L"T.\x105.A", 0, 5, 5, L"t.\x105.a" },
        { 5, L"T.*.A", 0, 5, 5, L"T.*.A" },
        { 5, L"X\xff0e.Z", 0, 0, 0, L"x..z" },
        { 63, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0,
          63, 63, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" },
        { 64, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0,
          0, 0, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" },
    };

    WCHAR buf[1024];
    DWORD i, ret, err;

    ret = pIdnToNameprepUnicode(0, test_data[0].in,
            test_data[0].in_len, NULL, 0);
    ok(ret == test_data[0].ret, "ret = %ld\n", ret);

    SetLastError(0xdeadbeef);
    ret = pIdnToNameprepUnicode(0, test_data[1].in,
            test_data[1].in_len, NULL, 0);
    err = GetLastError();
    ok(ret == test_data[1].ret, "ret = %ld\n", ret);
    ok(err == ret ? 0xdeadbeef : ERROR_INVALID_NAME, "err = %ld\n", err);

    SetLastError(0xdeadbeef);
    ret = pIdnToNameprepUnicode(0, test_data[0].in, -1, buf, ARRAY_SIZE(buf));
    err = GetLastError();
    ok(ret == test_data[0].ret, "ret = %ld\n", ret);
    ok(err == 0xdeadbeef, "err = %ld\n", err);

    SetLastError(0xdeadbeef);
    ret = pIdnToNameprepUnicode(0, test_data[0].in, -2, buf, ARRAY_SIZE(buf));
    err = GetLastError();
    ok(ret == 0, "ret = %ld\n", ret);
    ok(err == ERROR_INVALID_PARAMETER, "err = %ld\n", err);

    SetLastError(0xdeadbeef);
    ret = pIdnToNameprepUnicode(0, test_data[0].in, 0, buf, ARRAY_SIZE(buf));
    err = GetLastError();
    ok(ret == 0, "ret = %ld\n", ret);
    ok(err == ERROR_INVALID_NAME, "err = %ld\n", err);

    ret = pIdnToNameprepUnicode(IDN_ALLOW_UNASSIGNED|IDN_USE_STD3_ASCII_RULES,
            test_data[0].in, -1, buf, ARRAY_SIZE(buf));
    ok(ret == test_data[0].ret, "ret = %ld\n", ret);

    SetLastError(0xdeadbeef);
    ret = pIdnToNameprepUnicode(0, NULL, 0, NULL, 0);
    err = GetLastError();
    ok(ret == 0, "ret = %ld\n", ret);
    ok(err == ERROR_INVALID_PARAMETER, "err = %ld\n", err);

    SetLastError(0xdeadbeef);
    ret = pIdnToNameprepUnicode(4, NULL, 0, NULL, 0);
    err = GetLastError();
    ok(ret == 0, "ret = %ld\n", ret);
    ok(err == ERROR_INVALID_FLAGS || err == ERROR_INVALID_PARAMETER /* Win8 */,
       "err = %ld\n", err);

    for (i=0; i<ARRAY_SIZE(test_data); i++)
    {
        SetLastError(0xdeadbeef);
        memset( buf, 0xcc, sizeof(buf) );
        ret = pIdnToNameprepUnicode(test_data[i].flags, test_data[i].in, test_data[i].in_len,
                buf, ARRAY_SIZE(buf));
        err = GetLastError();

        ok(ret == test_data[i].ret || broken(ret == test_data[i].broken_ret), "%ld: ret = %ld\n", i, ret);

        if (ret == test_data[i].ret)
        {
            ok(err == ret ? 0xdeadbeef : ERROR_INVALID_NAME, "%ld: err = %ld\n", i, err);
            ok(!wcsncmp(test_data[i].out, buf, ret), "%ld: buf = %s\n", i, wine_dbgstr_wn(buf, ret));
        }
        if (pRtlNormalizeString)
        {
            NTSTATUS status;
            int len = ARRAY_SIZE(buf);
            memset( buf, 0xcc, sizeof(buf) );
            status = pRtlNormalizeString( 13, test_data[i].in, test_data[i].in_len, buf, &len );
            ok( status == test_data[i].status || broken(status == test_data[i].broken_status),
                "%ld: failed %lx\n", i, status );
            if (!status) ok( !wcsnicmp(test_data[i].out, buf, len), "%ld: buf = %s\n", i, wine_dbgstr_wn(buf, len));
        }
    }
}

static void test_IdnToAscii(void)
{
    struct {
        int in_len;
        const WCHAR in[80];
        DWORD flags;
        DWORD ret;
        DWORD broken_ret;
        const WCHAR out[80];
    } test_data[] = {
        /* 0 */
        { 5, L"Test", 0, 5, 5, L"Test" },
        { 5, L"Te\x017cst", 0, 12, 12, L"xn--test-cbb" },
        { 12, L"te\x0105st.te\x017cst", 0, 26, 26, L"xn--test-cta.xn--test-cbb" },
        { 3, {0x0105,'.',0}, 0, 9, 9, L"xn--2da." },
        { 10, L"http://t\x106", 0, 17, 17, L"xn--http://t-78a" },
        /* 5 */
        { -1, L"\x4e3a\x8bf4\x4e0d\x4ed6\x5011\x10d\x11b\x305c\x306a", 0,
          35, 35, L"xn--bea2a1631avbav44tyha32b91egs2t" },
        { 2, L"\x380", IDN_ALLOW_UNASSIGNED, 8, 8, L"xn--7va" },
        { 63, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0,
          63, 63, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" },
        { 64, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0, 0 },
        { -1, L"\xe4z123456789012345678901234567890123456789012345678901234", 0,
          64, 64, L"xn--z123456789012345678901234567890123456789012345678901234-9te" },
        /* 10 */
        { -1, L"\xd803\xde78\x46b5-\xa861.\x2e87", 0, 28, 0, L"xn----bm3an932a1l5d.xn--xvj" },
        { -1, L"\x06ef\x06ef", 0, 9, 0, L"xn--cmba" },
        { -1, L"-\x07e1\xff61\x2184", 0, 18, 0, L"xn----8cd.xn--r5g" },
    };

    WCHAR buf[1024];
    DWORD i, ret, err;

    for (i=0; i<ARRAY_SIZE(test_data); i++)
    {
        SetLastError(0xdeadbeef);
        ret = pIdnToAscii(test_data[i].flags, test_data[i].in, test_data[i].in_len, buf, ARRAY_SIZE(buf));
        err = GetLastError();
        ok(ret == test_data[i].ret || broken(ret == test_data[i].broken_ret), "%ld: ret = %ld\n", i, ret);
        ok(err == ret ? 0xdeadbeef : ERROR_INVALID_NAME, "%ld: err = %ld\n", i, err);
        ok(!wcsnicmp(test_data[i].out, buf, ret), "%ld: buf = %s\n", i, wine_dbgstr_wn(buf, ret));
    }
}

static void test_IdnToUnicode(void)
{
    struct {
        int in_len;
        const WCHAR in[80];
        DWORD flags;
        DWORD ret;
        DWORD broken_ret;
        const WCHAR out[80];
    } test_data[] = {
        /* 0 */
        { 5, L"Tes.", 0, 5, 5, L"Tes." },
        { 2, L"\x105", 0, 0 },
        { 33, L"xn--4dbcagdahymbxekheh6e0a7fei0b", 0,
          23, 23, L"\x05dc\x05de\x05d4\x05d4\x05dd\x05e4\x05e9\x05d5\x05d8\x05dc\x05d0\x05de\x05d3\x05d1\x05e8\x05d9\x05dd\x05e2\x05d1\x05e8\x05d9\x05ea" },
        { 34, L"test.xn--kda9ag5e9jnfsj.xn--pz-fna", 0,
          16, 16, L"test.\x0105\x0119\x015b\x0107\x0142\x00f3\x017c.p\x0119z" },
        { 63, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0,
          63, 63, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" },
        /* 5 */
        { 64, L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 0, 0 },
        { 8, L"xn--7va", IDN_ALLOW_UNASSIGNED, 2, 2, L"\x380" },
        { 8, L"xn--7va", 0, 0, 0, L"\x380" },
        { -1, L"xn----bm3an932a1l5d.xn--xvj", 0, 8, 0, L"\xd803\xde78\x46b5-\xa861.\x2e87" },
        { -1, L"xn--z123456789012345678901234567890123456789012345678901234-9te", 0,
          57, 57, L"\xe4z123456789012345678901234567890123456789012345678901234" },
        /* 10 */
        { -1, L"foo.bar", 0, 8, 8, L"foo.bar" },
        { -1, L"d.xn----dha", 0, 5, 5, L"d.\x00fc-" },
    };

    WCHAR buf[1024];
    DWORD i, ret, err;

    for (i=0; i<ARRAY_SIZE(test_data); i++)
    {
        ret = pIdnToUnicode(test_data[i].flags, test_data[i].in, test_data[i].in_len, NULL, 0);
        ok(ret == test_data[i].ret || broken(ret == test_data[i].broken_ret), "%ld: ret = %ld\n", i, ret);

        SetLastError(0xdeadbeef);
        ret = pIdnToUnicode(test_data[i].flags, test_data[i].in, test_data[i].in_len, buf, ARRAY_SIZE(buf));
        err = GetLastError();
        ok(ret == test_data[i].ret || broken(ret == test_data[i].broken_ret), "%ld: ret = %ld\n", i, ret);
        ok(err == ret ? 0xdeadbeef : ERROR_INVALID_NAME, "%ld: err = %ld\n", i, err);
        ok(!wcsncmp(test_data[i].out, buf, ret), "%ld: buf = %s\n", i, wine_dbgstr_wn(buf, ret));
    }
}

static BOOL is_idn_error( const WCHAR *str )
{
    WCHAR *p, err[256];
    lstrcpyW( err, str );
    for (p = wcstok( err, L" []" ); p; p = wcstok( NULL, L" []" ) )
    {
        if (*p == 'B' || !wcscmp( p, L"V8" )) continue;  /* BiDi */
        if (!wcscmp( p, L"V2" )) continue;  /* CheckHyphens */
        if (!wcscmp( p, L"V5" )) continue;  /* Combining marks */
        return TRUE;
    }
    return FALSE;
}

static void test_Idn(void)
{
    FILE *f;

    if (!pIdnToAscii || !pIdnToUnicode || !pIdnToNameprepUnicode)
    {
        win_skip("Idn support is not available\n");
        return;
    }

    test_IdnToNameprepUnicode();
    test_IdnToAscii();
    test_IdnToUnicode();

    /* optionally run the full test file from Unicode.org
     * available at https://www.unicode.org/Public/idna/latest/IdnaTestV2.txt
     */
    if ((f = fopen( "IdnaTestV2.txt", "r" )))
    {
        char *p, *end, buffer[2048];
        WCHAR columns[7][256], dst[256], *expect, *error;
        int i, ret, line = 0;

        while (fgets( buffer, sizeof(buffer), f ))
        {
            line++;
            if ((p = strchr( buffer, '#' ))) *p = 0;
            if (!(p = strtok( buffer, ";" ))) continue;
            for (i = 0; i < 7 && p; i++)
            {
                while (*p == ' ') p++;
                for (end = p + strlen(p); end > p; end--) if (end[-1] != ' ') break;
                *end = 0;
                MultiByteToWideChar( CP_UTF8, 0, p, -1, columns[i], 256 );
                p = strtok( NULL, ";" );
            }
            if (i < 7) continue;

            expect = columns[5];
            if (!*expect) expect = columns[3];
            if (!*expect) expect = columns[1];
            if (!*expect) expect = columns[0];
            error = columns[6];
            if (!*error) error = columns[4];
            if (!*error) error = columns[2];
            SetLastError( 0xdeadbeef );
            memset( dst, 0xcc, sizeof(dst) );
            ret = pIdnToAscii( 0, columns[0], -1, dst, ARRAY_SIZE(dst) );
            if (!is_idn_error( error ))
            {
                ok( ret, "line %u: toAscii failed for %s expected %s\n", line,
                    debugstr_w(columns[0]), debugstr_w(expect) );
                if (ret) ok( !wcscmp( dst, expect ), "line %u: got %s expected %s\n",
                             line, debugstr_w(dst), debugstr_w(expect) );
            }
            else
            {
                ok( !ret, "line %u: toAscii didn't fail for %s got %s expected error %s\n",
                    line, debugstr_w(columns[0]), debugstr_w(dst), debugstr_w(error) );
            }

            expect = columns[1];
            if (!*expect) expect = columns[0];
            error = columns[2];
            SetLastError( 0xdeadbeef );
            memset( dst, 0xcc, sizeof(dst) );
            ret = pIdnToUnicode( IDN_USE_STD3_ASCII_RULES, columns[0], -1, dst, ARRAY_SIZE(dst) );
            for (i = 0; columns[0][i]; i++) if (columns[0][i] > 0x7f) break;
            if (columns[0][i])
            {
                ok( !ret, "line %u: didn't fail for unicode chars in %s\n", line, debugstr_w(columns[0]) );
            }
            else if (!is_idn_error( error ))
            {
                ok( ret, "line %u: toUnicode failed for %s expected %s\n", line,
                    debugstr_w(columns[0]), debugstr_w(expect) );
                if (ret) ok( !wcscmp( dst, expect ), "line %u: got %s expected %s\n",
                             line, debugstr_w(dst), debugstr_w(expect) );
            }
            else
            {
                ok( !ret, "line %u: toUnicode didn't fail for %s got %s expected error %s\n",
                    line, debugstr_w(columns[0]), debugstr_w(dst), debugstr_w(error) );
            }
        }
        fclose( f );
    }
}


static void test_GetLocaleInfoEx(void)
{
    static const WCHAR enW[] = {'e','n',0};
    WCHAR bufferW[80], buffer2[80];
    INT ret;

    if (!pGetLocaleInfoEx)
    {
        win_skip("GetLocaleInfoEx not supported\n");
        return;
    }

    ret = pGetLocaleInfoEx(enW, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
    ok(ret || broken(ret == 0) /* Vista */, "got %d\n", ret);
    if (ret)
    {
        static const WCHAR statesW[] = {'U','n','i','t','e','d',' ','S','t','a','t','e','s',0};
        static const WCHAR dummyW[] = {'d','u','m','m','y',0};
        static const WCHAR enusW[] = {'e','n','-','U','S',0};
        static const WCHAR usaW[] = {'U','S','A',0};
        static const WCHAR enuW[] = {'E','N','U',0};
        const struct neutralsublang_name_t *ptr = neutralsublang_names;
        DWORD val;

        ok(ret == lstrlenW(bufferW)+1, "got %d\n", ret);
        ok(!lstrcmpW(bufferW, enW), "got %s\n", wine_dbgstr_w(bufferW));

        SetLastError(0xdeadbeef);
        ret = pGetLocaleInfoEx(enW, LOCALE_SNAME, bufferW, 2);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, %ld\n", ret, GetLastError());

        SetLastError(0xdeadbeef);
        ret = pGetLocaleInfoEx(enW, LOCALE_SNAME, NULL, 0);
        ok(ret == 3 && GetLastError() == 0xdeadbeef, "got %d, %ld\n", ret, GetLastError());

        ret = pGetLocaleInfoEx(enusW, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == lstrlenW(bufferW)+1, "got %d\n", ret);
        ok(!lstrcmpW(bufferW, enusW), "got %s\n", wine_dbgstr_w(bufferW));

        ret = pGetLocaleInfoEx(enW, LOCALE_SABBREVCTRYNAME, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == lstrlenW(bufferW)+1, "got %d\n", ret);
        ok(!lstrcmpW(bufferW, usaW), "got %s\n", wine_dbgstr_w(bufferW));

        ret = pGetLocaleInfoEx(enW, LOCALE_SABBREVLANGNAME, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == lstrlenW(bufferW)+1, "got %d\n", ret);
        ok(!lstrcmpW(bufferW, enuW), "got %s\n", wine_dbgstr_w(bufferW));

        ret = pGetLocaleInfoEx(enusW, LOCALE_SPARENT, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == lstrlenW(bufferW)+1, "got %d\n", ret);
        ok(!lstrcmpW(bufferW, enW), "got %s\n", wine_dbgstr_w(bufferW));

        ret = pGetLocaleInfoEx(enW, LOCALE_SPARENT, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == 1, "got %d\n", ret);
        ok(!bufferW[0], "got %s\n", wine_dbgstr_w(bufferW));

        ret = pGetLocaleInfoEx(enW, LOCALE_SPARENT | LOCALE_NOUSEROVERRIDE, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == 1, "got %d\n", ret);
        ok(!bufferW[0], "got %s\n", wine_dbgstr_w(bufferW));

        ret = pGetLocaleInfoEx(enW, LOCALE_SCOUNTRY, bufferW, ARRAY_SIZE(bufferW));
        ok(ret == lstrlenW(bufferW)+1, "got %d\n", ret);
        if ((PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) != LANG_ENGLISH) ||
            (PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) != LANG_ENGLISH))
        {
            skip("Non-English locale\n");
        }
        else
            ok(!lstrcmpW(bufferW, statesW), "got %s\n", wine_dbgstr_w(bufferW));

        bufferW[0] = 0;
        SetLastError(0xdeadbeef);
        ret = pGetLocaleInfoEx(dummyW, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %ld\n", ret, GetLastError());

        while (*ptr->name)
        {
            val = 0;
            pGetLocaleInfoEx(ptr->name, LOCALE_ILANGUAGE|LOCALE_RETURN_NUMBER, (WCHAR*)&val, sizeof(val)/sizeof(WCHAR));
            ok(val == ptr->lcid, "%s: got wrong lcid 0x%04lx, expected 0x%04lx\n", wine_dbgstr_w(ptr->name), val, ptr->lcid);
            bufferW[0] = 0;
            ret = pGetLocaleInfoEx(ptr->name, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
            ok(ret == lstrlenW(bufferW)+1, "%s: got ret value %d\n", wine_dbgstr_w(ptr->name), ret);
            ok(!lstrcmpW(bufferW, ptr->name), "%s: got wrong LOCALE_SNAME %s\n", wine_dbgstr_w(ptr->name), wine_dbgstr_w(bufferW));
            ptr++;
        }

        ret = pGetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, bufferW, ARRAY_SIZE(bufferW));
        ok(ret && ret == lstrlenW(bufferW)+1, "got ret value %d\n", ret);
        ret = GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SNAME, buffer2, ARRAY_SIZE(buffer2));
        ok(ret && ret == lstrlenW(buffer2)+1, "got ret value %d\n", ret);
        ok(!lstrcmpW(bufferW, buffer2), "LOCALE_SNAMEs don't match %s %s\n", wine_dbgstr_w(bufferW), wine_dbgstr_w(buffer2));
    }
}

static void test_IsValidLocaleName(void)
{
    BOOL ret;

    if (!pIsValidLocaleName)
    {
        win_skip("IsValidLocaleName not supported\n");
        return;
    }

    ret = pIsValidLocaleName(L"en-US");
    ok(ret, "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(L"en");
    ok(ret, "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(L"es-es");
    ok(ret, "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(L"de-DE_phoneb");
    ok(ret, "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(L"DE_de-phoneb");
    ok(ret || broken(!ret), "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(L"DE_de_PHONEB");
    ok(ret || broken(!ret), "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(L"DE_de+phoneb");
    ok(!ret, "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(L"zz");
    ok(!ret || broken(ret), "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(L"zz-ZZ");
    ok(!ret || broken(ret), "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(L"zzz");
    ok(!ret || broken(ret), "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(L"zzz-ZZZ");
    ok(!ret, "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(L"zzzz");
    ok(!ret, "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(LOCALE_NAME_INVARIANT);
    ok(ret, "IsValidLocaleName failed\n");
    ret = pIsValidLocaleName(LOCALE_NAME_USER_DEFAULT);
    ok(!ret, "IsValidLocaleName should have failed\n");
    ret = pIsValidLocaleName(LOCALE_NAME_SYSTEM_DEFAULT);
    ok(!ret, "IsValidLocaleName should have failed\n");

    if (!pRtlIsValidLocaleName)
    {
        win_skip( "RtlIsValidLocaleName not available\n" );
        return;
    }

    ret = pRtlIsValidLocaleName( L"en-US", 0 );
    ok(ret, "RtlIsValidLocaleName failed\n");
    ret = pRtlIsValidLocaleName( L"en", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"en", 1 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"en", 2 );
    ok(ret, "RtlIsValidLocaleName failed\n");
    ret = pRtlIsValidLocaleName( L"en-RR", 2 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"es-es", 0 );
    ok(ret, "RtlIsValidLocaleName failed\n");
    ret = pRtlIsValidLocaleName( L"de-DE_phoneb", 0 );
    ok(ret, "RtlIsValidLocaleName failed\n");
    ret = pRtlIsValidLocaleName( L"DE_de_PHONEB", 0 );
    ok(ret || broken(!ret), "RtlIsValidLocaleName failed\n");
    ret = pRtlIsValidLocaleName( L"DE_de+phoneb", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"zz", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"zz", 2 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"zz-ZZ", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"zzz", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"zzz-ZZZ", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( L"zzzz", 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( LOCALE_NAME_INVARIANT, 0 );
    ok(ret, "RtlIsValidLocaleName failed\n");
    ret = pRtlIsValidLocaleName( LOCALE_NAME_USER_DEFAULT, 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
    ret = pRtlIsValidLocaleName( LOCALE_NAME_SYSTEM_DEFAULT, 0 );
    ok(!ret, "RtlIsValidLocaleName should have failed\n");
}

static void test_ResolveLocaleName(void)
{
    static const struct { const WCHAR *name, *exp; BOOL broken; } tests[] =
    {
        { L"en-US", L"en-US" },
        { L"en", L"en-US" },
        { L"en-RR", L"en-US" },
        { L"en-na", L"en-NA", TRUE /* <= win8 */ },
        { L"EN-zz", L"en-US" },
        { L"en-US", L"en-US" },
        { L"de-DE_phoneb", L"de-DE" },
        { L"DE-de-phoneb", L"de-DE" },
        { L"fr-029", L"fr-029", TRUE /* <= win8 */ },
        { L"fr-CH_XX", L"fr-CH", TRUE /* <= win10 1809 */ },
        { L"fr-CHXX", L"fr-FR" },
        { L"zh", L"zh-CN" },
        { L"zh-Hant", L"zh-HK" },
        { L"zh-hans", L"zh-CN" },
        { L"ja-jp_radstr", L"ja-JP" },
        { L"az", L"az-Latn-AZ" },
        { L"uz", L"uz-Latn-UZ" },
        { L"uz-cyrl", L"uz-Cyrl-UZ" },
        { L"ia", L"ia-001", TRUE /* <= win10 1809 */ },
        { L"zz", L"" },
        { L"zzz-ZZZ", L"" },
        { L"zzzz", L"" },
        { L"zz+XX", NULL },
        { L"zz.XX", NULL },
        { LOCALE_NAME_INVARIANT, L"" },
    };
    INT i, ret;
#ifdef __REACTOS__
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
#else
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH], system[LOCALE_NAME_MAX_LENGTH];
#endif

    if (!pResolveLocaleName)
    {
        win_skip( "ResolveLocaleName not available\n" );
        return;
    }
    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        SetLastError( 0xdeadbeef );
        memset( buffer, 0xcc, sizeof(buffer) );
        ret = pResolveLocaleName( tests[i].name, buffer, ARRAY_SIZE(buffer) );
        if (tests[i].exp)
        {
            ok( !wcscmp( buffer, tests[i].exp ) || broken( tests[i].broken ),
                "%s: got %s\n", debugstr_w(tests[i].name), debugstr_w(buffer) );
            ok( ret == wcslen(buffer) + 1, "%s: got %u\n", debugstr_w(tests[i].name), ret );
        }
        else
        {
            ok( !ret || broken( ret == 1 ) /* win7 */,
                "%s: got %s\n", debugstr_w(tests[i].name), debugstr_w(buffer) );
            if (!ret)
                ok( GetLastError() == ERROR_INVALID_PARAMETER,
                    "%s: wrong error %lu\n", debugstr_w(tests[i].name), GetLastError() );
        }
    }
    SetLastError( 0xdeadbeef );
    memset( buffer, 0xcc, sizeof(buffer) );
    ret = pResolveLocaleName( LOCALE_NAME_SYSTEM_DEFAULT, buffer, ARRAY_SIZE(buffer) );
    ok( ret, "failed err %lu\n", GetLastError() );
#ifndef __REACTOS__
    GetSystemDefaultLocaleName( system, ARRAY_SIZE(system) );
    ok( !wcscmp( buffer, system ), "got wrong syslocale %s / %s\n", debugstr_w(buffer), debugstr_w(system));
    ok( ret == wcslen(system) + 1, "wrong len %u / %Iu\n", ret, wcslen(system) + 1 );
#endif

    SetLastError( 0xdeadbeef );
    ret = pResolveLocaleName( L"en-US", buffer, 4 );
    ok( !ret, "got %u\n", ret );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );
    ok( !wcscmp( buffer, L"en-" ), "got %s\n", debugstr_w(buffer) );

    SetLastError( 0xdeadbeef );
    ret = pResolveLocaleName( L"en-US", NULL, 0 );
    ok( ret == 6, "got %u\n", ret );
    ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
}

static void test_CompareStringOrdinal(void)
{
    INT ret;
    WCHAR test1[] = { 't','e','s','t',0 };
    WCHAR test2[] = { 'T','e','S','t',0 };
    WCHAR test3[] = { 't','e','s','t','3',0 };
    WCHAR null1[] = { 'a',0,'a',0 };
    WCHAR null2[] = { 'a',0,'b',0 };
    WCHAR bills1[] = { 'b','i','l','l','\'','s',0 };
    WCHAR bills2[] = { 'b','i','l','l','s',0 };
    WCHAR coop1[] = { 'c','o','-','o','p',0 };
    WCHAR coop2[] = { 'c','o','o','p',0 };
    WCHAR nonascii1[] = { 0x0102,0 };
    WCHAR nonascii2[] = { 0x0201,0 };
    WCHAR ch1, ch2;

    if (!pCompareStringOrdinal)
    {
        win_skip("CompareStringOrdinal not supported\n");
        return;
    }

    /* Check errors */
    SetLastError(0xdeadbeef);
    ret = pCompareStringOrdinal(NULL, 0, NULL, 0, FALSE);
    ok(!ret, "Got %u, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got %lx, expected %x\n", GetLastError(), ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    ret = pCompareStringOrdinal(test1, -1, NULL, 0, FALSE);
    ok(!ret, "Got %u, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got %lx, expected %x\n", GetLastError(), ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadbeef);
    ret = pCompareStringOrdinal(NULL, 0, test1, -1, FALSE);
    ok(!ret, "Got %u, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got %lx, expected %x\n", GetLastError(), ERROR_INVALID_PARAMETER);

    /* Check case */
    ret = pCompareStringOrdinal(test1, -1, test1, -1, FALSE);
    ok(ret == CSTR_EQUAL, "Got %u, expected %u\n", ret, CSTR_EQUAL);
    ret = pCompareStringOrdinal(test1, -1, test2, -1, FALSE);
    ok(ret == CSTR_GREATER_THAN, "Got %u, expected %u\n", ret, CSTR_GREATER_THAN);
    ret = pCompareStringOrdinal(test2, -1, test1, -1, FALSE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(test1, -1, test2, -1, TRUE);
    ok(ret == CSTR_EQUAL, "Got %u, expected %u\n", ret, CSTR_EQUAL);

    /* Check different sizes */
    ret = pCompareStringOrdinal(test1, 3, test2, -1, TRUE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(test1, -1, test2, 3, TRUE);
    ok(ret == CSTR_GREATER_THAN, "Got %u, expected %u\n", ret, CSTR_GREATER_THAN);

    /* Check null character */
    ret = pCompareStringOrdinal(null1, 3, null2, 3, FALSE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(null1, 3, null2, 3, TRUE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(test1, 5, test3, 5, FALSE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(test1, 4, test1, 5, FALSE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);

    /* Check ordinal behaviour */
    ret = pCompareStringOrdinal(bills1, -1, bills2, -1, FALSE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(coop2, -1, coop1, -1, FALSE);
    ok(ret == CSTR_GREATER_THAN, "Got %u, expected %u\n", ret, CSTR_GREATER_THAN);
    ret = pCompareStringOrdinal(nonascii1, -1, nonascii2, -1, FALSE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);
    ret = pCompareStringOrdinal(nonascii1, -1, nonascii2, -1, TRUE);
    ok(ret == CSTR_LESS_THAN, "Got %u, expected %u\n", ret, CSTR_LESS_THAN);

    for (ch1 = 0; ch1 < 512; ch1++)
    {
        for (ch2 = 0; ch2 < 1024; ch2++)
        {
            int diff = ch1 - ch2;
            ret = pCompareStringOrdinal( &ch1, 1, &ch2, 1, FALSE );
            ok( ret == (diff > 0 ? CSTR_GREATER_THAN : diff < 0 ? CSTR_LESS_THAN : CSTR_EQUAL),
                        "wrong result %d %04x %04x\n", ret, ch1, ch2 );
            diff = pRtlUpcaseUnicodeChar( ch1 ) - pRtlUpcaseUnicodeChar( ch2 );
            ret = pCompareStringOrdinal( &ch1, 1, &ch2, 1, TRUE );
            ok( ret == (diff > 0 ? CSTR_GREATER_THAN : diff < 0 ? CSTR_LESS_THAN : CSTR_EQUAL),
                        "wrong result %d %04x %04x\n", ret, ch1, ch2 );
        }
    }
}

static void test_GetGeoInfo(void)
{
    char buffA[20];
    WCHAR buffW[20];
    INT ret;

    if (!pGetGeoInfoA)
    {
        win_skip("GetGeoInfo is not available.\n");
        return;
    }

    /* unassigned id */
    SetLastError(0xdeadbeef);
    ret = pGetGeoInfoA(344, GEO_ISO2, NULL, 0, 0);
    ok(ret == 0, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef /* Win10 */), "got %ld\n", GetLastError());

    ret = pGetGeoInfoA(203, GEO_ISO2, NULL, 0, 0);
    ok(ret == 3, "got %d\n", ret);

    ret = pGetGeoInfoA(203, GEO_ISO3, NULL, 0, 0);
    ok(ret == 4, "got %d\n", ret);

    ret = pGetGeoInfoA(203, GEO_ISO2, buffA, 3, 0);
    ok(ret == 3, "got %d\n", ret);
    ok(!strcmp(buffA, "RU"), "got %s\n", buffA);

    /* buffer pointer not NULL, length is 0 - return required length */
    buffA[0] = 'a';
    SetLastError(0xdeadbeef);
    ret = pGetGeoInfoA(203, GEO_ISO2, buffA, 0, 0);
    ok(ret == 3, "got %d\n", ret);
    ok(buffA[0] == 'a', "got %c\n", buffA[0]);

    ret = pGetGeoInfoA(203, GEO_ISO3, buffA, 4, 0);
    ok(ret == 4, "got %d\n", ret);
    ok(!strcmp(buffA, "RUS"), "got %s\n", buffA);

    /* shorter buffer */
    SetLastError(0xdeadbeef);
    buffA[1] = buffA[2] = 0;
    ret = pGetGeoInfoA(203, GEO_ISO2, buffA, 2, 0);
    ok(ret == 0, "got %d\n", ret);
    ok(!strcmp(buffA, "RU"), "got %s\n", buffA);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %ld\n", GetLastError());

    /* GEO_NATION returns GEOID in a string form, but only for GEOCLASS_NATION-type IDs */
    ret = pGetGeoInfoA(203, GEO_NATION, buffA, 20, 0); /* GEOCLASS_NATION */
    ok(ret == 4, "GEO_NATION of nation: expected 4, got %d\n", ret);
    ok(!strcmp(buffA, "203"), "GEO_NATION of nation: expected 203, got %s\n", buffA);

    buffA[0] = 0;
    ret = pGetGeoInfoA(39070, GEO_NATION, buffA, 20, 0); /* GEOCLASS_REGION */
    ok(ret == 0, "GEO_NATION of region: expected 0, got %d\n", ret);
    ok(*buffA == 0, "GEO_NATION of region: expected empty string, got %s\n", buffA);

    buffA[0] = 0;
    ret = pGetGeoInfoA(333, GEO_NATION, buffA, 20, 0); /* LOCATION_BOTH internal Wine type */
    ok(ret == 0 ||
       broken(ret == 4) /* Win7 and older */,
       "GEO_NATION of LOCATION_BOTH: expected 0, got %d\n", ret);
    ok(*buffA == 0 ||
       broken(!strcmp(buffA, "333")) /* Win7 and older */,
       "GEO_NATION of LOCATION_BOTH: expected empty string, got %s\n", buffA);

    /* GEO_ID is like GEO_NATION but works for any ID */
    buffA[0] = 0;
    ret = pGetGeoInfoA(203, GEO_ID, buffA, 20, 0); /* GEOCLASS_NATION */
    if (ret == 0)
        win_skip("GEO_ID not supported.\n");
    else
    {
        ok(ret == 4, "GEO_ID: expected 4, got %d\n", ret);
        ok(!strcmp(buffA, "203"), "GEO_ID: expected 203, got %s\n", buffA);

        ret = pGetGeoInfoA(47610, GEO_ID, buffA, 20, 0); /* GEOCLASS_REGION */
        ok(ret == 6, "got %d\n", ret);
        ok(!strcmp(buffA, "47610"), "got %s\n", buffA);

        ret = pGetGeoInfoA(333, GEO_ID, buffA, 20, 0); /* LOCATION_BOTH internal Wine type */
        ok(ret == 4, "got %d\n", ret);
        ok(!strcmp(buffA, "333"), "got %s\n", buffA);
    }

    /* GEO_PARENT */
    buffA[0] = 0;
    ret = pGetGeoInfoA(203, GEO_PARENT, buffA, 20, 0);
    if (ret == 0)
        win_skip("GEO_PARENT not supported.\n");
    else
    {
        ok(ret == 6, "got %d\n", ret);
        ok(!strcmp(buffA, "47609"), "got %s\n", buffA);
    }

    buffA[0] = 0;
    ret = pGetGeoInfoA(203, GEO_ISO_UN_NUMBER, buffA, 20, 0);
    if (ret == 0)
        win_skip("GEO_ISO_UN_NUMBER not supported.\n");
    else
    {
        ok(ret == 4, "got %d\n", ret);
        ok(!strcmp(buffA, "643"), "got %s\n", buffA);
    }

    /* try invalid type value */
    SetLastError(0xdeadbeef);
    ret = pGetGeoInfoA(203, GEO_ID + 1, NULL, 0, 0);
    ok(ret == 0, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_FLAGS, "got %ld\n", GetLastError());

    /* Test for GetGeoInfoEx */
    if (!pGetGeoInfoEx)
    {
        win_skip("GetGeoInfoEx is not available\n");
        return;
    }

    /* Test with ISO 3166-1 */
    ret = pGetGeoInfoEx(L"AR", GEO_ISO3, buffW, ARRAYSIZE(buffW));
    ok(ret != 0, "GetGeoInfoEx failed %ld.\n", GetLastError());
    ok(!wcscmp(buffW, L"ARG"), "expected string to be ARG, got %ls\n", buffW);

    /* Test with UN M.49 */
    SetLastError(0xdeadbeef);
    ret = pGetGeoInfoEx(L"032", GEO_ISO3, buffW, ARRAYSIZE(buffW));
    ok(ret == 0, "expected GetGeoInfoEx to fail.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
                 "expected ERROR_INVALID_PARAMETER got %ld.\n", GetLastError());

    /* Test GEO_ID */
    ret = pGetGeoInfoEx(L"AR", GEO_ID, buffW, ARRAYSIZE(buffW));
    ok(ret != 0, "GetGeoInfoEx failed %ld.\n", GetLastError());
    ok(!wcscmp(buffW, L"11"), "expected string to be 11, got %ls\n", buffW);

    /* Test with invalid geo type */
    SetLastError(0xdeadbeef);
    ret = pGetGeoInfoEx(L"AR", GEO_LCID, buffW, ARRAYSIZE(buffW));
    ok(ret == 0, "expected GetGeoInfoEx to fail.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
                 "expected ERROR_INVALID_PARAMETER got %ld.\n", GetLastError());
}

static int geoidenum_count;
static BOOL CALLBACK test_geoid_enumproc(GEOID geoid)
{
    INT ret = pGetGeoInfoA(geoid, GEO_ISO2, NULL, 0, 0);
    ok(ret == 3, "got %d for %ld\n", ret, geoid);
    /* valid geoid starts at 2 */
    ok(geoid >= 2, "got geoid %ld\n", geoid);

    return geoidenum_count++ < 5;
}

static BOOL CALLBACK test_geoid_enumproc2(GEOID geoid)
{
    geoidenum_count++;
    return TRUE;
}

static void test_EnumSystemGeoID(void)
{
    BOOL ret;

    if (!pEnumSystemGeoID)
    {
        win_skip("EnumSystemGeoID is not available.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pEnumSystemGeoID(GEOCLASS_NATION, 0, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEnumSystemGeoID(GEOCLASS_NATION+1, 0, test_geoid_enumproc);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_FLAGS, "got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pEnumSystemGeoID(GEOCLASS_NATION+1, 0, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %ld\n", GetLastError());

    ret = pEnumSystemGeoID(GEOCLASS_NATION, 0, test_geoid_enumproc);
    ok(ret, "got %d\n", ret);

    /* only the first level is enumerated, not the whole hierarchy */
    geoidenum_count = 0;
    ret = pEnumSystemGeoID(GEOCLASS_NATION, 39070, test_geoid_enumproc2);
    if (ret == 0)
        win_skip("Parent GEOID is not supported in EnumSystemGeoID.\n");
    else
        ok(ret && geoidenum_count > 0, "got %d, count %d\n", ret, geoidenum_count);

    geoidenum_count = 0;
    ret = pEnumSystemGeoID(GEOCLASS_REGION, 39070, test_geoid_enumproc2);
    if (ret == 0)
        win_skip("GEOCLASS_REGION is not supported in EnumSystemGeoID.\n");
    else
    {
        ok(ret && geoidenum_count > 0, "got %d, count %d\n", ret, geoidenum_count);

        geoidenum_count = 0;
        ret = pEnumSystemGeoID(GEOCLASS_REGION, 0, test_geoid_enumproc2);
        ok(ret && geoidenum_count > 0, "got %d, count %d\n", ret, geoidenum_count);
    }

    geoidenum_count = 0;
    ret = pEnumSystemGeoID(GEOCLASS_ALL, 39070, test_geoid_enumproc2);
    if (ret == 0)
        win_skip("GEOCLASS_ALL is not supported in EnumSystemGeoID.\n");
    else
    {
        ok(ret && geoidenum_count > 0, "got %d, count %d\n", ret, geoidenum_count);

        geoidenum_count = 0;
        ret = pEnumSystemGeoID(GEOCLASS_ALL, 0, test_geoid_enumproc2);
        ok(ret && geoidenum_count > 0, "got %d, count %d\n", ret, geoidenum_count);
    }
}

struct invariant_entry {
  const char *name;
  int id;
  const char *expect, *expect2;
};

#define X(x)  #x, x
static const struct invariant_entry invariant_list[] = {
    { X(LOCALE_ILANGUAGE),                "007f" },
    { X(LOCALE_SENGLANGUAGE),             "Invariant Language" },
    { X(LOCALE_SABBREVLANGNAME),          "IVL" },
    { X(LOCALE_SNATIVELANGNAME),          "Invariant Language" },
    { X(LOCALE_ICOUNTRY),                 "1" },
    { X(LOCALE_SENGCOUNTRY),              "Invariant Country" },
    { X(LOCALE_SABBREVCTRYNAME),          "IVC", "" },
    { X(LOCALE_SNATIVECTRYNAME),          "Invariant Country" },
    { X(LOCALE_IDEFAULTLANGUAGE),         "0409" },
    { X(LOCALE_IDEFAULTCOUNTRY),          "1" },
    { X(LOCALE_IDEFAULTCODEPAGE),         "437" },
    { X(LOCALE_IDEFAULTANSICODEPAGE),     "1252" },
    { X(LOCALE_IDEFAULTMACCODEPAGE),      "10000" },
    { X(LOCALE_SLIST),                    "," },
    { X(LOCALE_IMEASURE),                 "0" },
    { X(LOCALE_SDECIMAL),                 "." },
    { X(LOCALE_STHOUSAND),                "," },
    { X(LOCALE_SGROUPING),                "3;0" },
    { X(LOCALE_IDIGITS),                  "2" },
    { X(LOCALE_ILZERO),                   "1" },
    { X(LOCALE_INEGNUMBER),               "1" },
    { X(LOCALE_SNATIVEDIGITS),            "0123456789" },
    { X(LOCALE_SCURRENCY),                "\x00a4" },
    { X(LOCALE_SINTLSYMBOL),              "XDR" },
    { X(LOCALE_SMONDECIMALSEP),           "." },
    { X(LOCALE_SMONTHOUSANDSEP),          "," },
    { X(LOCALE_SMONGROUPING),             "3;0" },
    { X(LOCALE_ICURRDIGITS),              "2" },
    { X(LOCALE_IINTLCURRDIGITS),          "2" },
    { X(LOCALE_ICURRENCY),                "0" },
    { X(LOCALE_INEGCURR),                 "0" },
    { X(LOCALE_SDATE),                    "/" },
    { X(LOCALE_STIME),                    ":" },
    { X(LOCALE_SSHORTDATE),               "MM/dd/yyyy" },
    { X(LOCALE_SLONGDATE),                "dddd, dd MMMM yyyy" },
    { X(LOCALE_STIMEFORMAT),              "HH:mm:ss" },
    { X(LOCALE_IDATE),                    "0" },
    { X(LOCALE_ILDATE),                   "1" },
    { X(LOCALE_ITIME),                    "1" },
    { X(LOCALE_ITIMEMARKPOSN),            "0" },
    { X(LOCALE_ICENTURY),                 "1" },
    { X(LOCALE_ITLZERO),                  "1" },
    { X(LOCALE_IDAYLZERO),                "1" },
    { X(LOCALE_IMONLZERO),                "1" },
    { X(LOCALE_S1159),                    "AM" },
    { X(LOCALE_S2359),                    "PM" },
    { X(LOCALE_ICALENDARTYPE),            "1" },
    { X(LOCALE_IOPTIONALCALENDAR),        "0" },
    { X(LOCALE_IFIRSTDAYOFWEEK),          "6" },
    { X(LOCALE_IFIRSTWEEKOFYEAR),         "0" },
    { X(LOCALE_SDAYNAME1),                "Monday" },
    { X(LOCALE_SDAYNAME2),                "Tuesday" },
    { X(LOCALE_SDAYNAME3),                "Wednesday" },
    { X(LOCALE_SDAYNAME4),                "Thursday" },
    { X(LOCALE_SDAYNAME5),                "Friday" },
    { X(LOCALE_SDAYNAME6),                "Saturday" },
    { X(LOCALE_SDAYNAME7),                "Sunday" },
    { X(LOCALE_SABBREVDAYNAME1),          "Mon" },
    { X(LOCALE_SABBREVDAYNAME2),          "Tue" },
    { X(LOCALE_SABBREVDAYNAME3),          "Wed" },
    { X(LOCALE_SABBREVDAYNAME4),          "Thu" },
    { X(LOCALE_SABBREVDAYNAME5),          "Fri" },
    { X(LOCALE_SABBREVDAYNAME6),          "Sat" },
    { X(LOCALE_SABBREVDAYNAME7),          "Sun" },
    { X(LOCALE_SMONTHNAME1),              "January" },
    { X(LOCALE_SMONTHNAME2),              "February" },
    { X(LOCALE_SMONTHNAME3),              "March" },
    { X(LOCALE_SMONTHNAME4),              "April" },
    { X(LOCALE_SMONTHNAME5),              "May" },
    { X(LOCALE_SMONTHNAME6),              "June" },
    { X(LOCALE_SMONTHNAME7),              "July" },
    { X(LOCALE_SMONTHNAME8),              "August" },
    { X(LOCALE_SMONTHNAME9),              "September" },
    { X(LOCALE_SMONTHNAME10),             "October" },
    { X(LOCALE_SMONTHNAME11),             "November" },
    { X(LOCALE_SMONTHNAME12),             "December" },
    { X(LOCALE_SMONTHNAME13),             "" },
    { X(LOCALE_SABBREVMONTHNAME1),        "Jan" },
    { X(LOCALE_SABBREVMONTHNAME2),        "Feb" },
    { X(LOCALE_SABBREVMONTHNAME3),        "Mar" },
    { X(LOCALE_SABBREVMONTHNAME4),        "Apr" },
    { X(LOCALE_SABBREVMONTHNAME5),        "May" },
    { X(LOCALE_SABBREVMONTHNAME6),        "Jun" },
    { X(LOCALE_SABBREVMONTHNAME7),        "Jul" },
    { X(LOCALE_SABBREVMONTHNAME8),        "Aug" },
    { X(LOCALE_SABBREVMONTHNAME9),        "Sep" },
    { X(LOCALE_SABBREVMONTHNAME10),       "Oct" },
    { X(LOCALE_SABBREVMONTHNAME11),       "Nov" },
    { X(LOCALE_SABBREVMONTHNAME12),       "Dec" },
    { X(LOCALE_SABBREVMONTHNAME13),       "" },
    { X(LOCALE_SPOSITIVESIGN),            "+" },
    { X(LOCALE_SNEGATIVESIGN),            "-" },
    { X(LOCALE_IPOSSIGNPOSN),             "3" },
    { X(LOCALE_INEGSIGNPOSN),             "0" },
    { X(LOCALE_IPOSSYMPRECEDES),          "1" },
    { X(LOCALE_IPOSSEPBYSPACE),           "0" },
    { X(LOCALE_INEGSYMPRECEDES),          "1" },
    { X(LOCALE_INEGSEPBYSPACE),           "0" },
    { X(LOCALE_SISO639LANGNAME),          "iv" },
    { X(LOCALE_SISO3166CTRYNAME),         "IV" },
    { X(LOCALE_IDEFAULTEBCDICCODEPAGE),   "037" },
    { X(LOCALE_IPAPERSIZE),               "9" },
    { X(LOCALE_SENGCURRNAME),             "International Monetary Fund" },
    { X(LOCALE_SNATIVECURRNAME),          "International Monetary Fund" },
    { X(LOCALE_SYEARMONTH),               "yyyy MMMM" },
    { X(LOCALE_IDIGITSUBSTITUTION),       "1" },
    { X(LOCALE_SNAME),                    "" },
    { X(LOCALE_SSCRIPTS),                 "Latn;" },
    { 0 }
};
#undef X

static void test_invariant(void)
{
  int ret;
  int len;
  char buffer[BUFFER_SIZE];
  const struct invariant_entry *ptr = invariant_list;

  if (!GetLocaleInfoA(LOCALE_INVARIANT, NUO|LOCALE_SLANGUAGE, buffer, sizeof(buffer)))
  {
      win_skip("GetLocaleInfoA(LOCALE_INVARIANT) not supported\n"); /* win2k */
      return;
  }

  while (ptr->name)
  {
    ret = GetLocaleInfoA(LOCALE_INVARIANT, NUO|ptr->id, buffer, sizeof(buffer));
    if (!ret && (ptr->id == LOCALE_SNAME || ptr->id == LOCALE_SSCRIPTS))
        win_skip("not supported\n"); /* winxp/win2k3 */
    else
    {
        len = strlen(ptr->expect)+1; /* include \0 */
        ok(ret == len || (ptr->expect2 && ret == strlen(ptr->expect2)+1),
           "For id %d, expected ret == %d, got %d, error %ld\n",
            ptr->id, len, ret, GetLastError());
        ok(!strcmp(buffer, ptr->expect) || (ptr->expect2 && !strcmp(buffer, ptr->expect2)),
           "For id %d, Expected %s, got '%s'\n",
            ptr->id, ptr->expect, buffer);
    }

    ptr++;
  }

 if ((LANGIDFROMLCID(GetSystemDefaultLCID()) != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)) ||
     (LANGIDFROMLCID(GetThreadLocale()) != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)))
  {
      skip("Non US-English locale\n");
  }
  else
  {
    /* some locales translate these */
    static const char lang[]  = "Invariant Language (Invariant Country)";
    static const char cntry[] = "Invariant Country";
    static const char sortm[] = "Math Alphanumerics";
    static const char sortms[] = "Maths Alphanumerics";
    static const char sortd[] = "Default"; /* win2k3 */

    ret = GetLocaleInfoA(LOCALE_INVARIANT, NUO|LOCALE_SLANGUAGE, buffer, sizeof(buffer));
    len = lstrlenA(lang) + 1;
    ok(ret == len, "Expected ret == %d, got %d, error %ld\n", len, ret, GetLastError());
    ok(!strcmp(buffer, lang), "Expected %s, got '%s'\n", lang, buffer);

    ret = GetLocaleInfoA(LOCALE_INVARIANT, NUO|LOCALE_SCOUNTRY, buffer, sizeof(buffer));
    len = lstrlenA(cntry) + 1;
    ok(ret == len, "Expected ret == %d, got %d, error %ld\n", len, ret, GetLastError());
    ok(!strcmp(buffer, cntry), "Expected %s, got '%s'\n", cntry, buffer);

    ret = GetLocaleInfoA(LOCALE_INVARIANT, NUO|LOCALE_SSORTNAME, buffer, sizeof(buffer));
    ok(ret, "Failed err %ld\n", GetLastError());
    ok(!strcmp(buffer, sortm) || !strcmp(buffer, sortd) || !strcmp(buffer, sortms), "Got '%s'\n", buffer);
  }
}

static void test_GetSystemPreferredUILanguages(void)
{
    BOOL ret;
    NTSTATUS status;
    ULONG i, count, size, size_id, size_name, size_buffer;
    WCHAR *buffer;

    if (!pGetSystemPreferredUILanguages)
    {
        win_skip("GetSystemPreferredUILanguages is not available.\n");
        return;
    }

    /* (in)valid first parameter */
    count = 0;
    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(0, &count, NULL, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_FULL_LANGUAGE, &count, NULL, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID | MUI_FULL_LANGUAGE, &count, NULL, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID | MUI_LANGUAGE_NAME, &count, NULL, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    count = 0;
    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID | MUI_MACHINE_LANGUAGE_SETTINGS, &count, NULL, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size);

    count = 0;
    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_NAME | MUI_MACHINE_LANGUAGE_SETTINGS, &count, NULL, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);

    /* second parameter
     * ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, NULL, NULL, &size);
     * -> unhandled exception c0000005
     */

    /* invalid third parameter */
    size = 1;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* fourth parameter
     * ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, NULL);
     * -> unhandled exception c0000005
     */

    count = 0;
    size_id = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, &size_id);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size_id  % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size_id);

    count = 0;
    size_name = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, &count, NULL, &size_name);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size_name % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size_name);

    size_buffer = max(size_id, size_name);
    if(!size_buffer)
    {
        skip("No valid buffer size\n");
        return;
    }

    buffer = HeapAlloc(GetProcessHeap(), 0, size_buffer * sizeof(WCHAR));
    if (!buffer)
    {
        skip("Failed to allocate memory for %ld chars\n", size_buffer);
        return;
    }

    count = 0;
    size = size_buffer;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(0, &count, buffer, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 6 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    count = 0;
    size = size_buffer;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size);
    if (ret && size % 5 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);
    for (i = 0; buffer[i]; i++)
        ok(('0' <= buffer[i] && buffer[i] <= '9') ||
           ('A' <= buffer[i] && buffer[i] <= 'F'),
           "MUI_LANGUAGE_ID [%ld] is bad in %s\n", i, wine_dbgstr_w(buffer));

    count = 0;
    size = size_buffer;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 5 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    count = 0;
    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_MACHINE_LANGUAGE_SETTINGS, &count, NULL, &size);
    ok(ret, "Expected GetSystemPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 6 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    /* ntdll version is the same, but apparently takes an extra second parameter */
    count = 0;
    size = size_buffer;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    status = pRtlGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, 0, &count, buffer, &size);
    ok(!status, "got %lx\n", status);
    ok(count, "Expected count > 0\n");
    ok(size % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size);
    if (ret && size % 5 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    count = 0;
    size = size_buffer;
    status = pRtlGetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, 0, &count, buffer, &size);
    ok(!status, "got %lx\n", status);
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 5 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    count = 0;
    size = 0;
    status = pRtlGetSystemPreferredUILanguages(MUI_MACHINE_LANGUAGE_SETTINGS, 0, &count, NULL, &size);
    ok(!status, "got %lx\n", status);
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 6 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id, "expected %lu, got %lu\n", size_id, size);

    size = 1;
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id, "expected %lu, got %lu\n", size_id, size);

    size = size_id -1;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id, "expected %lu, got %lu\n", size_id, size);

    size = size_id -2;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetSystemPreferredUILanguages(0, &count, buffer, &size);
    ok(!ret, "Expected GetSystemPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id + 2 || size == size_id + 1 /* before win10 1809 */, "expected %lu, got %lu\n", size_id + 2, size);

    HeapFree(GetProcessHeap(), 0, buffer);
}

static void test_GetThreadPreferredUILanguages(void)
{
    BOOL ret;
    NTSTATUS status;
    ULONG count, size, size_id;
    WCHAR *buf;

    if (!pGetThreadPreferredUILanguages)
    {
        win_skip("GetThreadPreferredUILanguages is not available.\n");
        return;
    }

    size = count = 0;
    ret = pGetThreadPreferredUILanguages(MUI_LANGUAGE_ID|MUI_UI_FALLBACK, &count, NULL, &size);
    ok(ret, "got %lu\n", GetLastError());
    ok(count, "expected count > 0\n");
    ok(size, "expected size > 0\n");

    count = 0;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size * sizeof(WCHAR));
    ret = pGetThreadPreferredUILanguages(MUI_LANGUAGE_ID|MUI_UI_FALLBACK, &count, buf, &size);
    ok(ret, "got %lu\n", GetLastError());
    ok(count, "expected count > 0\n");

    size_id = count = 0;
    ret = pGetThreadPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, &size_id);
    ok(ret, "got %lu\n", GetLastError());
    ok(count, "expected count > 0\n");
    ok(size_id, "expected size > 0\n");
    ok(size_id <= size, "expected size > 0\n");

    /* ntdll function is the same */
    size_id = count = 0;
    status = pRtlGetThreadPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, &size_id);
    ok(!status, "got %lx\n", status);
    ok(count, "expected count > 0\n");
    ok(size_id, "expected size > 0\n");
    ok(size_id <= size, "expected size > 0\n");

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetThreadPreferredUILanguages(MUI_LANGUAGE_ID, &count, buf, &size);
    ok(!ret, "Expected GetThreadPreferredUILanguages to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id, "expected %lu, got %lu\n", size_id, size);

    size = 1;
    SetLastError(0xdeadbeef);
    ret = pGetThreadPreferredUILanguages(MUI_LANGUAGE_ID, &count, buf, &size);
    ok(!ret, "Expected GetThreadPreferredUILanguages to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id, "expected %lu, got %lu\n", size_id, size);

    size = size_id - 1;
    SetLastError(0xdeadbeef);
    ret = pGetThreadPreferredUILanguages(MUI_LANGUAGE_ID, &count, buf, &size);
    ok(!ret, "Expected GetThreadPreferredUILanguages to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(size == size_id, "expected %lu, got %lu\n", size_id, size);

    size = size_id - 2;
    SetLastError(0xdeadbeef);
    ret = pGetThreadPreferredUILanguages(0, &count, buf, &size);
    ok(!ret, "Expected GetThreadPreferredUILanguages to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    todo_wine
    ok(size == size_id || size == size_id - 1 /* before win10 1809 */, "expected %lu, got %lu\n", size_id, size);

    HeapFree(GetProcessHeap(), 0, buf);
}

static void test_GetUserPreferredUILanguages(void)
{
    BOOL ret;
    NTSTATUS status;
    ULONG count, size, size_id, size_name, size_buffer;
    WCHAR *buffer;

    if (!pGetUserPreferredUILanguages)
    {
        win_skip("GetUserPreferredUILanguages is not available.\n");
        return;
    }

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_FULL_LANGUAGE, &count, NULL, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID | MUI_FULL_LANGUAGE, &count, NULL, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    size = 0;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID | MUI_MACHINE_LANGUAGE_SETTINGS, &count, NULL, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    size = 1;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected error ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    count = 0;
    size_id = 0;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID, &count, NULL, &size_id);
    ok(ret, "Expected GetUserPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size_id  % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size_id);

    count = 0;
    size_name = 0;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, NULL, &size_name);
    ok(ret, "Expected GetUserPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size_name % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size_name);

    size_buffer = max(size_id, size_name);
    if(!size_buffer)
    {
        skip("No valid buffer size\n");
        return;
    }

    /* ntdll version is the same, but apparently takes an extra second parameter */
    count = 0;
    size_id = 0;
    SetLastError(0xdeadbeef);
    status = pRtlGetUserPreferredUILanguages(MUI_LANGUAGE_ID, 0, &count, NULL, &size_id);
    ok(!status, "got %lx\n", status);
    ok(count, "Expected count > 0\n");
    ok(size_id  % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size_id);

    buffer = HeapAlloc(GetProcessHeap(), 0, size_buffer * sizeof(WCHAR));

    count = 0;
    size = size_buffer;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(0, &count, buffer, &size);
    ok(ret, "Expected GetUserPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 6 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    count = 0;
    size = size_buffer;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(ret, "Expected GetUserPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 5 == 1, "Expected size (%ld) %% 5 == 1\n", size);
    if (ret && size % 5 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    count = 0;
    size = size_buffer;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer, &size);
    ok(ret, "Expected GetUserPreferredUILanguages to succeed\n");
    ok(count, "Expected count > 0\n");
    ok(size % 6 == 1, "Expected size (%ld) %% 6 == 1\n", size);
    if (ret && size % 5 == 1)
        ok(!buffer[size -2] && !buffer[size -1],
           "Expected last two WCHARs being empty, got 0x%x 0x%x\n",
           buffer[size -2], buffer[size -1]);

    size = 1;
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    size = size_id -1;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(MUI_LANGUAGE_ID, &count, buffer, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    count = 0;
    size = size_id -2;
    memset(buffer, 0x5a, size_buffer * sizeof(WCHAR));
    SetLastError(0xdeadbeef);
    ret = pGetUserPreferredUILanguages(0, &count, buffer, &size);
    ok(!ret, "Expected GetUserPreferredUILanguages to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
       "Expected error ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, buffer);
}

static void test_FindNLSStringEx(void)
{
    INT res;
    static const WCHAR comb_s_accent1W[] = {0x1e69, 'o','u','r','c','e',0};
    static const WCHAR comb_s_accent2W[] = {0x0073,0x323,0x307,'o','u','r','c','e',0};
    static const WCHAR comb_q_accent1W[] = {'a','b',0x0071,0x0307,0x323,'u','o','t','e','\n',0};
    static const WCHAR comb_q_accent2W[] = {0x0071,0x0323,0x307,'u','o','t','e',0};
    struct test_data {
        const WCHAR *locale;
        DWORD flags;
        const WCHAR *src;
        const WCHAR *value;
        INT expected_ret;
        INT expected_found;
    };

    static struct test_data tests[] =
    {
        { localeW, FIND_FROMSTART, L"SimpleSimple", L"Simple", 0, 6},
        { localeW, FIND_FROMEND, L"SimpleSimple", L"Simp", 6, 4},
        { localeW, FIND_STARTSWITH, L"SimpleSimple", L"Simp", 0, 4},
        { localeW, FIND_ENDSWITH, L"SimpleSimple", L"Simple", 6, 6},
        { localeW, FIND_ENDSWITH, L"SimpleSimple", L"Simp", -1, 0xdeadbeef},
        { localeW, FIND_FROMSTART, comb_s_accent1W, comb_s_accent2W, 0, 6 },
        { localeW, FIND_FROMSTART, comb_q_accent1W, comb_q_accent2W, 2, 7 },
        { localeW, FIND_STARTSWITH, L"--Option", L"--", 0, 2},
        { localeW, FIND_ENDSWITH, L"Option--", L"--", 6, 2},
        { localeW, FIND_FROMSTART, L"----", L"--", 0, 2},
        { localeW, FIND_FROMEND, L"----", L"--", 2, 2},
        { localeW, FIND_FROMSTART, L"opt1--opt2--opt3", L"--", 4, 2},
        { localeW, FIND_FROMEND, L"opt1--opt2--opt3", L"--", 10, 2},
        { localeW, FIND_FROMSTART, L"x-oss-security", L"x-oss-", 0, 6},
        { localeW, FIND_FROMSTART, L"x-oss-security", L"-oss", 1, 4},
        { localeW, FIND_FROMSTART, L"x-oss-security", L"-oss--", -1, 0xdeadbeef},
        { localeW, FIND_FROMEND, L"x-oss-oss2", L"-oss", 5, 4},
    };
    unsigned int i;

    if (!pFindNLSStringEx)
    {
        win_skip("FindNLSStringEx is not available.\n");
        return;
    }

    SetLastError( 0xdeadbeef );
    res = pFindNLSStringEx(invalidW, FIND_FROMSTART, fooW, 3, fooW,
                           3, NULL, NULL, NULL, 0);
    ok(res, "Expected failure of FindNLSStringEx. Return value was %d\n", res);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected ERROR_INVALID_PARAMETER as last error; got %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    res = pFindNLSStringEx(localeW, FIND_FROMSTART, NULL, 3, fooW, 3,
                           NULL, NULL, NULL, 0);
    ok(res, "Expected failure of FindNLSStringEx. Return value was %d\n", res);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected ERROR_INVALID_PARAMETER as last error; got %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    res = pFindNLSStringEx(localeW, FIND_FROMSTART, fooW, -5, fooW, 3,
                           NULL, NULL, NULL, 0);
    ok(res, "Expected failure of FindNLSStringEx. Return value was %d\n", res);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected ERROR_INVALID_PARAMETER as last error; got %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    res = pFindNLSStringEx(localeW, FIND_FROMSTART, fooW, 3, NULL, 3,
                           NULL, NULL, NULL, 0);
    ok(res, "Expected failure of FindNLSStringEx. Return value was %d\n", res);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected ERROR_INVALID_PARAMETER as last error; got %ld\n", GetLastError());

    SetLastError( 0xdeadbeef );
    res = pFindNLSStringEx(localeW, FIND_FROMSTART, fooW, 3, fooW, -5,
                           NULL, NULL, NULL, 0);
    ok(res, "Expected failure of FindNLSStringEx. Return value was %d\n", res);
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "Expected ERROR_INVALID_PARAMETER as last error; got %ld\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        int found = 0xdeadbeef;
        res = pFindNLSStringEx(tests[i].locale, tests[i].flags, tests[i].src, -1,
                               tests[i].value, -1, &found, NULL, NULL, 0);
        ok(res == tests[i].expected_ret,
           "%u: Expected FindNLSStringEx to return %d. Returned value was %d\n", i,
           tests[i].expected_ret, res);
        ok(found == tests[i].expected_found,
           "%u: Expected FindNLSStringEx to output %d. Value was %d\n", i,
           tests[i].expected_found, found);
    }
}

static void test_FindStringOrdinal(void)
{
    static const WCHAR abc123aBcW[] = {'a', 'b', 'c', '1', '2', '3', 'a', 'B', 'c', 0};
    static const WCHAR abcW[] = {'a', 'b', 'c', 0};
    static const WCHAR aBcW[] = {'a', 'B', 'c', 0};
    static const WCHAR aaaW[] = {'a', 'a', 'a', 0};
    static const struct
    {
        DWORD flag;
        const WCHAR *src;
        INT src_size;
        const WCHAR *val;
        INT val_size;
        BOOL ignore_case;
        INT ret;
        DWORD err;
    }
    tests[] =
    {
        /* Invalid */
        {1, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, -1, ERROR_INVALID_FLAGS},
        {FIND_FROMSTART, NULL, ARRAY_SIZE(abc123aBcW) - 1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, -1,
         ERROR_INVALID_PARAMETER},
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, NULL, ARRAY_SIZE(abcW) - 1, FALSE, -1,
         ERROR_INVALID_PARAMETER},
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, NULL, 0, FALSE, -1, ERROR_INVALID_PARAMETER},
        {FIND_FROMSTART, NULL, 0, abcW, ARRAY_SIZE(abcW) - 1, FALSE, -1, ERROR_INVALID_PARAMETER},
        {FIND_FROMSTART, NULL, 0, NULL, 0, FALSE, -1, ERROR_INVALID_PARAMETER},
        /* Case-insensitive */
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, 0, NO_ERROR},
        {FIND_FROMEND, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, 0, NO_ERROR},
        {FIND_STARTSWITH, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, 0, NO_ERROR},
        {FIND_ENDSWITH, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, -1, NO_ERROR},
        /* Case-sensitive */
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, aBcW, ARRAY_SIZE(aBcW) - 1, TRUE, 0, NO_ERROR},
        {FIND_FROMEND, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, aBcW, ARRAY_SIZE(aBcW) - 1, TRUE, 6, NO_ERROR},
        {FIND_STARTSWITH, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, aBcW, ARRAY_SIZE(aBcW) - 1, TRUE, 0, NO_ERROR},
        {FIND_ENDSWITH, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, aBcW, ARRAY_SIZE(aBcW) - 1, TRUE, 6, NO_ERROR},
        /* Other */
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, aaaW, ARRAY_SIZE(aaaW) - 1, FALSE, -1, NO_ERROR},
        {FIND_FROMSTART, abc123aBcW, -1, abcW, ARRAY_SIZE(abcW) - 1, FALSE, 0, NO_ERROR},
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, -1, FALSE, 0, NO_ERROR},
        {FIND_FROMSTART, abc123aBcW, 0, abcW, ARRAY_SIZE(abcW) - 1, FALSE, -1, NO_ERROR},
        {FIND_FROMSTART, abc123aBcW, ARRAY_SIZE(abc123aBcW) - 1, abcW, 0, FALSE, 0, NO_ERROR},
        {FIND_FROMSTART, abc123aBcW, 0, abcW, 0, FALSE, 0, NO_ERROR},
    };
    INT ret;
    DWORD err;
    INT i;

    if (!pFindStringOrdinal)
    {
        win_skip("FindStringOrdinal is not available.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        SetLastError(0xdeadbeef);
        ret = pFindStringOrdinal(tests[i].flag, tests[i].src, tests[i].src_size, tests[i].val, tests[i].val_size,
                                 tests[i].ignore_case);
        err = GetLastError();
        ok(ret == tests[i].ret, "Item %d expected %d, got %d\n", i, tests[i].ret, ret);
        ok(err == tests[i].err, "Item %d expected %#lx, got %#lx\n", i, tests[i].err, err);
    }
}

static void test_SetThreadUILanguage(void)
{
    LANGID res;

    if (!pGetThreadUILanguage)
    {
        win_skip("GetThreadUILanguage isn't implemented, skipping SetThreadUILanguage tests for version < Vista\n");
        return;   /* BTW SetThreadUILanguage is present on winxp/2003 but doesn`t set the LANGID anyway when tested */
    }

    res = pSetThreadUILanguage(0);
    ok(res == pGetThreadUILanguage(), "expected %d got %d\n", pGetThreadUILanguage(), res);

    res = pSetThreadUILanguage(MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN));
    ok(res == MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN),
    "expected %d got %d\n", MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN), res);

    res = pSetThreadUILanguage(0);
    todo_wine ok(res == MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN),
    "expected %d got %d\n", MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH_BELGIAN), res);
}

/* read a Unicode string from NormalizationTest.txt format; helper for test_NormalizeString */
static int read_str( char *str, WCHAR res[32] )
{
    int pos = 0;
    char *end;

    while (*str && pos < 31)
    {
        unsigned int c = strtoul( str, &end, 16 );
        pos += put_utf16( res + pos, c );
        while (*end == ' ') end++;
        str = end;
    }
    res[pos] = 0;
    return pos;
}

static void test_NormalizeString(void)
{
    /* part 0: specific cases */
    /* LATIN CAPITAL LETTER D WITH DOT ABOVE */
    static const WCHAR part0_str1[] = {0x1e0a,0};
    static const WCHAR part0_nfd1[] = {0x0044,0x0307,0};

    /* LATIN CAPITAL LETTER D, COMBINING DOT BELOW, COMBINING DOT ABOVE */
    static const WCHAR part0_str2[] = {0x0044,0x0323,0x0307,0};
    static const WCHAR part0_nfc2[] = {0x1e0c,0x0307,0};

    /* LATIN CAPITAL LETTER D, COMBINING HORN, COMBINING DOT BELOW, COMBINING DOT ABOVE */
    static const WCHAR part0_str3[] = {0x0044,0x031b,0x0323,0x0307,0};
    static const WCHAR part0_nfc3[] = {0x1e0c,0x031b,0x0307,0};

    /* LATIN CAPITAL LETTER D, COMBINING HORN, COMBINING DOT BELOW, COMBINING DOT ABOVE */
    static const WCHAR part0_str4[] = {0x0044,0x031b,0x0323,0x0307,0};
    static const WCHAR part0_nfc4[] = {0x1e0c,0x031b,0x0307,0};

    /*
     * HEBREW ACCENT SEGOL, HEBREW POINT PATAH, HEBREW POINT DAGESH OR MAPIQ,
     * HEBREW ACCENT MERKHA, HEBREW POINT SHEVA, HEBREW PUNCTUATION PASEQ,
     * HEBREW MARK UPPER DOT, HEBREW ACCENT DEHI
     */
    static const WCHAR part0_str5[] = {0x0592,0x05B7,0x05BC,0x05A5,0x05B0,0x05C0,0x05C4,0x05AD,0};
    static const WCHAR part0_nfc5[] = {0x05B0,0x05B7,0x05BC,0x05A5,0x0592,0x05C0,0x05AD,0x05C4,0};

    /*
     * HEBREW POINT QAMATS, HEBREW POINT HOLAM, HEBREW POINT HATAF SEGOL,
     * HEBREW ACCENT ETNAHTA, HEBREW PUNCTUATION SOF PASUQ, HEBREW POINT SHEVA,
     * HEBREW ACCENT ILUY, HEBREW ACCENT QARNEY PARA
     */
    static const WCHAR part0_str6[] = {0x05B8,0x05B9,0x05B1,0x0591,0x05C3,0x05B0,0x05AC,0x059F,0};
    static const WCHAR part0_nfc6[] = {0x05B1,0x05B8,0x05B9,0x0591,0x05C3,0x05B0,0x05AC,0x059F,0};

    /* LATIN CAPITAL LETTER D WITH DOT BELOW */
    static const WCHAR part0_str8[] = {0x1E0C,0};
    static const WCHAR part0_nfd8[] = {0x0044,0x0323,0};

    /* LATIN CAPITAL LETTER D WITH DOT ABOVE, COMBINING DOT BELOW */
    static const WCHAR part0_str9[] = {0x1E0A,0x0323,0};
    static const WCHAR part0_nfc9[] = {0x1E0C,0x0307,0};
    static const WCHAR part0_nfd9[] = {0x0044,0x0323,0x0307,0};

    /* LATIN CAPITAL LETTER D WITH DOT BELOW, COMBINING DOT ABOVE */
    static const WCHAR part0_str10[] = {0x1E0C,0x0307,0};
    static const WCHAR part0_nfd10[] = {0x0044,0x0323,0x0307,0};

    /* LATIN CAPITAL LETTER E WITH MACRON AND GRAVE, COMBINING MACRON */
    static const WCHAR part0_str11[] = {0x1E14,0x0304,0};
    static const WCHAR part0_nfd11[] = {0x0045,0x0304,0x0300,0x0304,0};

    /* LATIN CAPITAL LETTER E WITH MACRON, COMBINING GRAVE ACCENT */
    static const WCHAR part0_str12[] = {0x0112,0x0300,0};
    static const WCHAR part0_nfc12[] = {0x1E14,0};
    static const WCHAR part0_nfd12[] = {0x0045,0x0304,0x0300,0};

    /* part 1: character by character */
    /* DIAERESIS */
    static const WCHAR part1_str1[] = {0x00a8,0};
    static const WCHAR part1_nfkc1[] = {0x0020,0x0308,0};

    /* VULGAR FRACTION ONE QUARTER */
    static const WCHAR part1_str2[] = {0x00bc,0};
    static const WCHAR part1_nfkc2[] = {0x0031,0x2044,0x0034,0};

    /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
    static const WCHAR part1_str3[] = {0x00ca,0};
    static const WCHAR part1_nfd3[] = {0x0045,0x0302,0};

    /* MODIFIER LETTER SMALL GAMMA */
    static const WCHAR part1_str4[] = {0x02e0,0};
    static const WCHAR part1_nfkc4[] = {0x0263,0};

    /* CYRILLIC CAPITAL LETTER IE WITH GRAVE */
    static const WCHAR part1_str5[] = {0x0400,0};
    static const WCHAR part1_nfd5[] = {0x0415,0x0300,0};

    /* CYRILLIC CAPITAL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT */
    static const WCHAR part1_str6[] = {0x0476,0};
    static const WCHAR part1_nfd6[] = {0x0474,0x030F,0};

    /* ARABIC LIGATURE HAH WITH JEEM INITIAL FORM */
    static const WCHAR part1_str7[] = {0xFCA9,0};
    static const WCHAR part1_nfkc7[] = {0x062D,0x062C,0};

    /* GREEK SMALL LETTER OMICRON WITH PSILI AND VARIA */
    static const WCHAR part1_str8[] = {0x1F42,0};
    static const WCHAR part1_nfd8[] = {0x03BF,0x0313,0x0300,0};

    /* QUADRUPLE PRIME */
    static const WCHAR part1_str9[] = {0x2057,0};
    static const WCHAR part1_nfkc9[] = {0x2032,0x2032,0x2032,0x2032,0};

    /* KATAKANA-HIRAGANA VOICED SOUND MARK */
    static const WCHAR part1_str10[] = {0x309B,0};
    static const WCHAR part1_nfkc10[] = {0x20,0x3099,0};

    /* ANGSTROM SIGN */
    static const WCHAR part1_str11[] = {0x212B,0};
    static const WCHAR part1_nfc11[] = {0xC5,0};
    static const WCHAR part1_nfd11[] = {'A',0x030A,0};

    static const WCHAR composite_src[] =
    {
        0x008a, 0x008e, 0x009a, 0x009e, 0x009f, 0x00c0, 0x00c1, 0x00c2,
        0x00c3, 0x00c4, 0x00c5, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb,
        0x00cc, 0x00cd, 0x00ce, 0x00cf, 0x00d1, 0x00d2, 0x00d3, 0x00d4,
        0x00d5, 0x00d6, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd,
        0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e7, 0x00e8,
        0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x00f1,
        0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f8, 0x00f9, 0x00fa,
        0x00fb, 0x00fc, 0x00fd, 0x00ff, 0x212b
    };

    struct test_data_normal {
        const WCHAR *str;
        const WCHAR *expected[4];
    };
    static const struct test_data_normal test_arr[] =
    {
        { part0_str1, { part0_str1, part0_nfd1, part0_str1, part0_nfd1 } },
        { part0_str2, { part0_nfc2, part0_str2, part0_nfc2, part0_str2 } },
        { part0_str3, { part0_nfc3, part0_str3, part0_nfc3, part0_str3 } },
        { part0_str4, { part0_nfc4, part0_str4, part0_nfc4, part0_str4 } },
        { part0_str5, { part0_nfc5, part0_nfc5, part0_nfc5, part0_nfc5 } },
        { part0_str6, { part0_nfc6, part0_nfc6, part0_nfc6, part0_nfc6 } },
        { part0_str8, { part0_str8, part0_nfd8, part0_str8, part0_nfd8 } },
        { part0_str9, { part0_nfc9, part0_nfd9, part0_nfc9, part0_nfd9 } },
        { part0_str10, { part0_str10, part0_nfd10, part0_str10, part0_nfd10 } },
        { part0_str11, { part0_str11, part0_nfd11, part0_str11, part0_nfd11 } },
        { part0_str12, { part0_nfc12, part0_nfd12, part0_nfc12, part0_nfd12 } },
        { part1_str1, { part1_str1, part1_str1, part1_nfkc1, part1_nfkc1 } },
        { part1_str2, { part1_str2, part1_str2, part1_nfkc2, part1_nfkc2 } },
        { part1_str3, { part1_str3, part1_nfd3, part1_str3, part1_nfd3 } },
        { part1_str4, { part1_str4, part1_str4, part1_nfkc4, part1_nfkc4 } },
        { part1_str5, { part1_str5, part1_nfd5, part1_str5, part1_nfd5 } },
        { part1_str6, { part1_str6, part1_nfd6, part1_str6, part1_nfd6 } },
        { part1_str7, { part1_str7, part1_str7, part1_nfkc7, part1_nfkc7 } },
        { part1_str8, { part1_str8, part1_nfd8, part1_str8, part1_nfd8 } },
        { part1_str9, { part1_str9, part1_str9, part1_nfkc9, part1_nfkc9 } },
        { part1_str10, { part1_str10, part1_str10, part1_nfkc10, part1_nfkc10 } },
        { part1_str11, { part1_nfc11, part1_nfd11, part1_nfc11, part1_nfd11 } },
        { 0 }
    };
    const struct test_data_normal *ptest = test_arr;
    const int norm_forms[] = { NormalizationC, NormalizationD, NormalizationKC, NormalizationKD };
    WCHAR dst[256];
    BOOLEAN ret;
    NTSTATUS status;
    int dstlen, str_cmp, i, j;
    FILE *f;

    if (!pNormalizeString)
    {
        win_skip("NormalizeString is not available.\n");
        return;
    }
    if (!pRtlNormalizeString) win_skip("RtlNormalizeString is not available.\n");

    /*
     * For each string, first test passing -1 as srclen to NormalizeString,
     * thereby assuming a null-terminating string in src, and then test passing
     * explicitly the string length.
     * Do that for all 4 normalization forms.
     */
    while (ptest->str)
    {
        for (i = 0; i < 4; i++)
        {
            SetLastError(0xdeadbeef);
            dstlen = pNormalizeString( norm_forms[i], ptest->str, -1, NULL, 0 );
            ok( dstlen > lstrlenW(ptest->str), "%s:%d: wrong len %d / %d\n",
                wine_dbgstr_w(ptest->str), i, dstlen, lstrlenW(ptest->str) );
            ok(GetLastError() == ERROR_SUCCESS, "%s:%d: got error %lu\n",
               wine_dbgstr_w(ptest->str), i, GetLastError());
            SetLastError(0xdeadbeef);
            dstlen = pNormalizeString( norm_forms[i], ptest->str, -1, dst, dstlen );
            ok(GetLastError() == ERROR_SUCCESS, "%s:%d: got error %lu\n",
               wine_dbgstr_w(ptest->str), i, GetLastError());
            ok(dstlen == lstrlenW( dst )+1, "%s:%d: Copied length differed: was %d, should be %d\n",
               wine_dbgstr_w(ptest->str), i, dstlen, lstrlenW( dst )+1);
            str_cmp = wcsncmp( ptest->expected[i], dst, dstlen+1 );
            ok( str_cmp == 0, "%s:%d: string incorrect got %s expect %s\n", wine_dbgstr_w(ptest->str), i,
                wine_dbgstr_w(dst), wine_dbgstr_w(ptest->expected[i]) );

            dstlen = pNormalizeString( norm_forms[i], ptest->str, lstrlenW(ptest->str), NULL, 0 );
            memset(dst, 0xcc, sizeof(dst));
            dstlen = pNormalizeString( norm_forms[i], ptest->str, lstrlenW(ptest->str), dst, dstlen );
            ok(dstlen == lstrlenW( ptest->expected[i] ), "%s:%d: Copied length differed: was %d, should be %d\n",
               wine_dbgstr_w(ptest->str), i, dstlen, lstrlenW( dst ));
            str_cmp = wcsncmp( ptest->expected[i], dst, dstlen );
            ok( str_cmp == 0, "%s:%d: string incorrect got %s expect %s\n", wine_dbgstr_w(ptest->str), i,
                wine_dbgstr_w(dst), wine_dbgstr_w(ptest->expected[i]) );

            if (pRtlNormalizeString)
            {
                dstlen = 0;
                status = pRtlNormalizeString( norm_forms[i], ptest->str, lstrlenW(ptest->str), NULL, &dstlen );
                ok( !status, "%s:%d: failed %lx\n", wine_dbgstr_w(ptest->str), i, status );
                ok( dstlen > lstrlenW(ptest->str), "%s:%d: wrong len %d / %d\n",
                    wine_dbgstr_w(ptest->str), i, dstlen, lstrlenW(ptest->str) );
                memset(dst, 0, sizeof(dst));
                status = pRtlNormalizeString( norm_forms[i], ptest->str, lstrlenW(ptest->str), dst, &dstlen );
                ok( !status, "%s:%d: failed %lx\n", wine_dbgstr_w(ptest->str), i, status );
                ok(dstlen == lstrlenW( dst ), "%s:%d: Copied length differed: was %d, should be %d\n",
                   wine_dbgstr_w(ptest->str), i, dstlen, lstrlenW( dst ));
                str_cmp = wcsncmp( ptest->expected[i], dst, dstlen );
                ok( str_cmp == 0, "%s:%d: string incorrect got %s expect %s\n", wine_dbgstr_w(ptest->str), i,
                    wine_dbgstr_w(dst), wine_dbgstr_w(ptest->expected[i]) );
                ret = FALSE;
                status = pRtlIsNormalizedString( norm_forms[i], ptest->str, -1, &ret );
                ok( !status, "%s:%d: failed %lx\n", wine_dbgstr_w(ptest->str), i, status );
                if (!wcscmp( ptest->str, dst ))
                    ok( ret, "%s:%d: not normalized\n", wine_dbgstr_w(ptest->str), i );
                else
                    ok( !ret, "%s:%d: normalized (dst %s)\n", wine_dbgstr_w(ptest->str), i, wine_dbgstr_w(dst) );
                ret = FALSE;
                status = pRtlIsNormalizedString( norm_forms[i], dst, dstlen, &ret );
                ok( !status, "%s:%d: failed %lx\n", wine_dbgstr_w(ptest->str), i, status );
                ok( ret, "%s:%d: not normalized\n", wine_dbgstr_w(ptest->str), i );
            }
        }
        ptest++;
    }

    /* buffer overflows */

    SetLastError(0xdeadbeef);
    dstlen = pNormalizeString( NormalizationD, part0_str1, -1, dst, 1 );
    ok( dstlen <= 0, "wrong len %d\n", dstlen );
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    dstlen = pNormalizeString( NormalizationC, part0_str2, -1, dst, 1 );
    ok( dstlen <= 0, "wrong len %d\n", dstlen );
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    dstlen = pNormalizeString( NormalizationC, part0_str2, -1, NULL, 0 );
    ok( dstlen == 12, "wrong len %d\n", dstlen );
    ok(GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    dstlen = pNormalizeString( NormalizationC, part0_str2, -1, dst, 3 );
    ok( dstlen == 3, "wrong len %d\n", dstlen );
    ok(GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    dstlen = pNormalizeString( NormalizationC, part0_str2, 0, NULL, 0 );
    ok( dstlen == 0, "wrong len %d\n", dstlen );
    ok(GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    dstlen = pNormalizeString( NormalizationC, part0_str2, 0, dst, 3 );
    ok( dstlen == 0, "wrong len %d\n", dstlen );
    ok(GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError());

    /* size estimations */

    memset( dst, 'A', sizeof(dst) );
    for (j = 1; j < ARRAY_SIZE(dst); j++)
    {
        for (i = 0; i < 4; i++)
        {
            int expect = (i < 2) ? j * 3 : j * 18;
            if (expect > 64) expect = max( 64, j + j / 8 );
            dstlen = pNormalizeString( norm_forms[i], dst, j, NULL, 0 );
            ok( dstlen == expect, "%d: %d -> wrong len %d\n", i, j, dstlen );
            if (pRtlNormalizeString)
            {
                dstlen = 0;
                status = pRtlNormalizeString( norm_forms[i], dst, j, NULL, &dstlen );
                ok( !status, "%d: failed %lx\n", i, status );
                ok( dstlen == expect, "%d: %d -> wrong len %d\n", i, j, dstlen );
            }
        }
    }
    for (i = 0; i < 4; i++)
    {
        int srclen = ARRAY_SIZE( composite_src );
        int expect = max( 64, srclen + srclen / 8 );
        dstlen = pNormalizeString( norm_forms[i], composite_src, srclen, NULL, 0 );
        ok( dstlen == expect, "%d: wrong len %d\n", i, dstlen );
        dstlen = pNormalizeString( norm_forms[i], composite_src, srclen, dst, dstlen );
        if (i == 0 || i == 2)
        {
            ok( dstlen == srclen, "%d: wrong len %d\n", i, dstlen );
            ok(GetLastError() == ERROR_SUCCESS, "got error %lu\n", GetLastError());
        }
        else
        {
            ok( dstlen < -expect, "%d: wrong len %d\n", i, dstlen );
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got error %lu\n", GetLastError());
        }
        if (pRtlNormalizeString)
        {
            dstlen = 0;
            status = pRtlNormalizeString( norm_forms[i], composite_src, srclen, NULL, &dstlen );
            ok( !status, "%d: failed %lx\n", i, status );
            ok( dstlen == expect, "%d: wrong len %d\n", i, dstlen );
            status = pRtlNormalizeString( norm_forms[i], composite_src, srclen, dst, &dstlen );
            if (i == 0 || i == 2)
            {
                ok( !status, "%d: failed %lx\n", i, status );
                ok( dstlen == srclen, "%d: wrong len %d\n", i, dstlen );
            }
            else
            {
                ok( status == STATUS_BUFFER_TOO_SMALL, "%d: failed %lx\n", i, status );
                ok( dstlen > expect, "%d: wrong len %d\n", i, dstlen );
            }
        }
    }

    /* invalid parameters */

    for (i = 0; i < 32; i++)
    {
        SetLastError(0xdeadbeef);
        dstlen = pNormalizeString( i, L"ABC", -1, NULL, 0 );
        switch (i)
        {
        case NormalizationC:
        case NormalizationD:
        case NormalizationKC:
        case NormalizationKD:
        case 13:  /* Idn */
            ok( dstlen > 0, "%d: wrong len %d\n", i, dstlen );
            ok( GetLastError() == ERROR_SUCCESS, "%d: got error %lu\n", i, GetLastError());
            break;
        default:
            ok( dstlen <= 0, "%d: wrong len %d\n", i, dstlen );
            ok( GetLastError() == ERROR_INVALID_PARAMETER, "%d: got error %lu\n", i, GetLastError());
            break;
        }
        if (pRtlNormalizeString)
        {
            dstlen = 0;
            status = pRtlNormalizeString( i, L"ABC", -1, NULL, &dstlen );
            switch (i)
            {
            case 0:
                ok( status == STATUS_INVALID_PARAMETER, "%d: failed %lx\n", i, status );
                break;
            case NormalizationC:
            case NormalizationD:
            case NormalizationKC:
            case NormalizationKD:
            case 13:  /* Idn */
                ok( status == STATUS_SUCCESS, "%d: failed %lx\n", i, status );
                break;
            default:
                ok( status == STATUS_OBJECT_NAME_NOT_FOUND, "%d: failed %lx\n", i, status );
                break;
            }
        }
    }

    /* invalid sequences */

    for (i = 0; i < 4; i++)
    {
        dstlen = pNormalizeString( norm_forms[i], L"AB\xd800Z", -1, NULL, 0 );
        ok( dstlen == (i < 2 ? 15 : 64), "%d: wrong len %d\n", i, dstlen );
        SetLastError( 0xdeadbeef );
        dstlen = pNormalizeString( norm_forms[i], L"AB\xd800Z", -1, dst, ARRAY_SIZE(dst) );
        ok( dstlen == -3, "%d: wrong len %d\n", i, dstlen );
        ok( GetLastError() == ERROR_NO_UNICODE_TRANSLATION, "%d: wrong error %ld\n", i, GetLastError() );
        dstlen = pNormalizeString( norm_forms[i], L"ABCD\xdc12Z", -1, NULL, 0 );
        ok( dstlen == (i < 2 ? 21 : 64), "%d: wrong len %d\n", i, dstlen );
        SetLastError( 0xdeadbeef );
        dstlen = pNormalizeString( norm_forms[i], L"ABCD\xdc12Z", -1, dst, ARRAY_SIZE(dst) );
        ok( dstlen == -4, "%d: wrong len %d\n", i, dstlen );
        ok( GetLastError() == ERROR_NO_UNICODE_TRANSLATION, "%d: wrong error %ld\n", i, GetLastError() );
        SetLastError( 0xdeadbeef );
        dstlen = pNormalizeString( norm_forms[i], L"ABCD\xdc12Z", -1, dst, 2 );
        todo_wine
        ok( dstlen == (i < 2 ? -18 : -74), "%d: wrong len %d\n", i, dstlen );
        todo_wine_if (i == 0 || i == 2)
        ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "%d: wrong error %ld\n", i, GetLastError() );
        if (pRtlNormalizeString)
        {
            dstlen = 0;
            status = pRtlNormalizeString( norm_forms[i], L"AB\xd800Z", -1, NULL, &dstlen );
            ok( !status, "%d: failed %lx\n", i, status );
            ok( dstlen == (i < 2 ? 15 : 64), "%d: wrong len %d\n", i, dstlen );
            dstlen = ARRAY_SIZE(dst);
            status = pRtlNormalizeString( norm_forms[i], L"AB\xd800Z", -1, dst, &dstlen );
            ok( status == STATUS_NO_UNICODE_TRANSLATION, "%d: failed %lx\n", i, status );
            ok( dstlen == 3, "%d: wrong len %d\n", i, dstlen );
            dstlen = 1;
            status = pRtlNormalizeString( norm_forms[i], L"AB\xd800Z", -1, dst, &dstlen );
            todo_wine_if( i == 0 || i == 2)
            ok( status == STATUS_BUFFER_TOO_SMALL, "%d: failed %lx\n", i, status );
            todo_wine_if( i != 3)
            ok( dstlen == (i < 2 ? 14 : 73), "%d: wrong len %d\n", i, dstlen );
            dstlen = 2;
            status = pRtlNormalizeString( norm_forms[i], L"AB\xd800Z", -1, dst, &dstlen );
            ok( status == STATUS_NO_UNICODE_TRANSLATION, "%d: failed %lx\n", i, status );
            ok( dstlen == 3, "%d: wrong len %d\n", i, dstlen );
        }
    }

    /* optionally run the full test file from Unicode.org
     * available at http://www.unicode.org/Public/UCD/latest/ucd/NormalizationTest.txt
     */
    if ((f = fopen( "NormalizationTest.txt", "r" )))
    {
        char *p, buffer[1024];
        WCHAR str[3], srcW[32], dstW[32], resW[4][32];
        int line = 0, part = 0, ch;
        char tested[0x110000 / 8];

        while (fgets( buffer, sizeof(buffer), f ))
        {
            line++;
            if ((p = strchr( buffer, '#' ))) *p = 0;
            if (!strncmp( buffer, "@Part", 5 ))
            {
                part = atoi( buffer + 5 );
                continue;
            }
            if (!(p = strtok( buffer, ";" ))) continue;
            read_str( p, srcW );
            for (i = 0; i < 4; i++)
            {
                p = strtok( NULL, ";" );
                read_str( p, &resW[i][0] );
            }
            if (part == 1)
            {
                ch = srcW[0];
                if (ch >= 0xd800 && ch <= 0xdbff)
                    ch = 0x10000 + ((srcW[0] & 0x3ff) << 10) + (srcW[1] & 0x3ff);
                tested[ch / 8] |= 1 << (ch % 8);
            }
            for (i = 0; i < 4; i++)
            {
                memset( dstW, 0xcc, sizeof(dstW) );
                dstlen = pNormalizeString( norm_forms[i], srcW, -1, dstW, ARRAY_SIZE(dstW) );
                ok( !wcscmp( dstW, resW[i] ),
                    "line %u form %u: wrong result %s for %s expected %s\n", line, i,
                    wine_dbgstr_w( dstW ), wine_dbgstr_w( srcW ), wine_dbgstr_w( resW[i] ));

                ret = FALSE;
                status = pRtlIsNormalizedString( norm_forms[i], srcW, -1, &ret );
                ok( !status, "line %u form %u: RtlIsNormalizedString failed %lx\n", line, i, status );
                if (!wcscmp( srcW, dstW ))
                    ok( ret, "line %u form %u: source not normalized %s\n", line, i, wine_dbgstr_w(srcW) );
                else
                    ok( !ret, "line %u form %u: source normalized %s\n", line, i, wine_dbgstr_w(srcW) );
                ret = FALSE;
                status = pRtlIsNormalizedString( norm_forms[i], dstW, -1, &ret );
                ok( !status, "line %u form %u: RtlIsNormalizedString failed %lx\n", line, i, status );
                ok( ret, "line %u form %u: dest not normalized %s\n", line, i, wine_dbgstr_w(dstW) );

                for (j = 0; j < 4; j++)
                {
                    int expect = i | (j & 2);
                    memset( dstW, 0xcc, sizeof(dstW) );
                    dstlen = pNormalizeString( norm_forms[i], resW[j], -1, dstW, ARRAY_SIZE(dstW) );
                    ok( !wcscmp( dstW, resW[expect] ),
                        "line %u form %u res %u: wrong result %s for %s expected %s\n", line, i, j,
                        wine_dbgstr_w( dstW ), wine_dbgstr_w( resW[j] ), wine_dbgstr_w( resW[expect] ));
                }
            }
        }
        fclose( f );

        /* test chars that are not in the @Part1 list */
        for (ch = 0; ch < 0x110000; ch++)
        {
            if (tested[ch / 8] & (1 << (ch % 8))) continue;
            str[put_utf16( str, ch )] = 0;
            for (i = 0; i < 4; i++)
            {
                memset( dstW, 0xcc, sizeof(dstW) );
                SetLastError( 0xdeadbeef );
                dstlen = pNormalizeString( norm_forms[i], str, -1, dstW, ARRAY_SIZE(dstW) );
                if ((ch >= 0xd800 && ch <= 0xdfff) ||
                    (ch >= 0xfdd0 && ch <= 0xfdef) ||
                    ((ch & 0xffff) >= 0xfffe))
                {
                    ok( dstlen <= 0, "char %04x form %u: wrong result %d %s expected error\n",
                        ch, i, dstlen, wine_dbgstr_w( dstW ));
                    ok( GetLastError() == ERROR_NO_UNICODE_TRANSLATION,
                        "char %04x form %u: error %lu\n", str[0], i, GetLastError() );
                    status = pRtlIsNormalizedString( norm_forms[i], str, -1, &ret );
                    ok( status == STATUS_NO_UNICODE_TRANSLATION,
                        "char %04x form %u: failed %lx\n", ch, i, status );
                }
                else
                {
                    ok( !wcscmp( dstW, str ),
                        "char %04x form %u: wrong result %s expected unchanged\n",
                        ch, i, wine_dbgstr_w( dstW ));
                    ret = FALSE;
                    status = pRtlIsNormalizedString( norm_forms[i], str, -1, &ret );
                    ok( !status, "char %04x form %u: failed %lx\n", ch, i, status );
                    ok( ret, "char %04x form %u: not normalized\n", ch, i );
                }
            }
        }
    }
}

static void test_SpecialCasing(void)
{
    int ret, i, len;
    UINT val = 0, exp;
    WCHAR src[8], buffer[8];
    static const struct test {
        const WCHAR *lang;
        DWORD flags;
        UINT ch;
        UINT exp;      /* 0 if self */
        UINT exp_ling; /* 0 if exp */
        BOOL broken;
    } tests[] = {
        {L"de-DE", LCMAP_UPPERCASE, 0x00DF},   /* LATIN SMALL LETTER SHARP S */

        {L"en-US", LCMAP_UPPERCASE, 0xFB00},   /* LATIN SMALL LIGATURE FF */
        {L"en-US", LCMAP_UPPERCASE, 0xFB01},   /* LATIN SMALL LIGATURE FI */
        {L"en-US", LCMAP_UPPERCASE, 0xFB02},   /* LATIN SMALL LIGATURE FL */
        {L"en-US", LCMAP_UPPERCASE, 0xFB03},   /* LATIN SMALL LIGATURE FFI */
        {L"en-US", LCMAP_UPPERCASE, 0xFB04},   /* LATIN SMALL LIGATURE FFL */
        {L"en-US", LCMAP_UPPERCASE, 0xFB05},   /* LATIN SMALL LIGATURE LONG S T */
        {L"en-US", LCMAP_UPPERCASE, 0xFB06},   /* LATIN SMALL LIGATURE ST */

        {L"hy-AM", LCMAP_UPPERCASE, 0x0587},   /* ARMENIAN SMALL LIGATURE ECH YIWN */
        {L"hy-AM", LCMAP_UPPERCASE, 0xFB13},   /* ARMENIAN SMALL LIGATURE MEN NOW */
        {L"hy-AM", LCMAP_UPPERCASE, 0xFB14},   /* ARMENIAN SMALL LIGATURE MEN ECH */
        {L"hy-AM", LCMAP_UPPERCASE, 0xFB15},   /* ARMENIAN SMALL LIGATURE MEN INI */
        {L"hy-AM", LCMAP_UPPERCASE, 0xFB16},   /* ARMENIAN SMALL LIGATURE VEW NOW */
        {L"hy-AM", LCMAP_UPPERCASE, 0xFB17},   /* ARMENIAN SMALL LIGATURE MEN XEH */

        {L"en-US", LCMAP_UPPERCASE, 0x0149},   /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
        {L"el-GR", LCMAP_UPPERCASE, 0x0390,0,0,TRUE /*win7*/ }, /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
        {L"el-GR", LCMAP_UPPERCASE, 0x03B0,0,0,TRUE /*win7*/ }, /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
        {L"en-US", LCMAP_UPPERCASE, 0x01F0},   /* LATIN SMALL LETTER J WITH CARON */
        {L"en-US", LCMAP_UPPERCASE, 0x1E96},   /* LATIN SMALL LETTER H WITH LINE BELOW */
        {L"en-US", LCMAP_UPPERCASE, 0x1E97},   /* LATIN SMALL LETTER T WITH DIAERESIS */
        {L"en-US", LCMAP_UPPERCASE, 0x1E98},   /* LATIN SMALL LETTER W WITH RING ABOVE */
        {L"en-US", LCMAP_UPPERCASE, 0x1E99},   /* LATIN SMALL LETTER Y WITH RING ABOVE */
        {L"en-US", LCMAP_UPPERCASE, 0x1E9A},   /* LATIN SMALL LETTER A WITH RIGHT HALF RING */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F50},   /* GREEK SMALL LETTER UPSILON WITH PSILI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F52},   /* GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F54},   /* GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F56},   /* GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FB6},   /* GREEK SMALL LETTER ALPHA WITH PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FC6},   /* GREEK SMALL LETTER ETA WITH PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FD2},   /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FD3},   /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FD6},   /* GREEK SMALL LETTER IOTA WITH PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FD7},   /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FE2},   /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FE3},   /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FE4},   /* GREEK SMALL LETTER RHO WITH PSILI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FE6},   /* GREEK SMALL LETTER UPSILON WITH PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FE7},   /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FF6},   /* GREEK SMALL LETTER OMEGA WITH PERISPOMENI */

        {L"el-GR", LCMAP_UPPERCASE, 0x1F80,0x1F88}, /* GREEK SMALL LETTER ALPHA WITH PSILI AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F81,0x1F89}, /* GREEK SMALL LETTER ALPHA WITH DASIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F82,0x1F8A}, /* GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F83,0x1F8B}, /* GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F84,0x1F8C}, /* GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F85,0x1F8D}, /* GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F86,0x1F8E}, /* GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F87,0x1F8F}, /* GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */

        {L"el-GR", LCMAP_LOWERCASE, 0x1F88,0x1F80}, /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F89,0x1F81}, /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F8A,0x1F82}, /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F8B,0x1F83}, /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F8C,0x1F84}, /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F8D,0x1F85}, /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F8E,0x1F86}, /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1F8F,0x1F87}, /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */

        {L"el-GR", LCMAP_UPPERCASE, 0x1F90,0x1F98}, /* GREEK SMALL LETTER ETA WITH PSILI AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F91,0x1F99}, /* GREEK SMALL LETTER ETA WITH DASIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F92,0x1F9A}, /* GREEK SMALL LETTER ETA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F93,0x1F9B}, /* GREEK SMALL LETTER ETA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F94,0x1F9C}, /* GREEK SMALL LETTER ETA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F95,0x1F9D}, /* GREEK SMALL LETTER ETA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F96,0x1F9E}, /* GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1F97,0x1F9F}, /* GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */

        {L"el-GR", LCMAP_LOWERCASE, 0x1FA8,0x1FA0}, /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FA9,0x1FA1}, /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FAA,0x1FA2}, /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FAB,0x1FA3}, /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FAC,0x1FA4}, /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FAD,0x1FA5}, /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FAE,0x1FA6}, /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FAF,0x1FA7}, /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */

        {L"el-GR", LCMAP_UPPERCASE, 0x1FB3,0x1FBC}, /* GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FBC,0x1FB3}, /* GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FC3,0x1FCC}, /* GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FCC,0x1FC3}, /* GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FF3,0x1FFC}, /* GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI */
        {L"el-GR", LCMAP_LOWERCASE, 0x1FFC,0x1FF3}, /* GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI */

        {L"el-GR", LCMAP_UPPERCASE, 0x1FB2}, /* GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FB4}, /* GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FC2}, /* GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FC4}, /* GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FF2}, /* GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FF4}, /* GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI */

        {L"el-GR", LCMAP_UPPERCASE, 0x1FB7}, /* GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FC7}, /* GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI */
        {L"el-GR", LCMAP_UPPERCASE, 0x1FF7}, /* GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND YPOGEGRAMMENI */

        {L"el-GR", LCMAP_LOWERCASE, 0x03A3,0x03C3}, /* GREEK CAPITAL LETTER SIGMA */

        {L"lt-LT", LCMAP_LOWERCASE, 'J','j'},        /* LATIN CAPITAL LETTER J */
        {L"lt-LT", LCMAP_LOWERCASE, 0x012E,0x012F},  /* LATIN CAPITAL LETTER I WITH OGONEK */
        {L"lt-LT", LCMAP_LOWERCASE, 0x00CC,0x00EC},  /* LATIN CAPITAL LETTER I WITH GRAVE */
        {L"lt-LT", LCMAP_LOWERCASE, 0x00CD,0x00ED},  /* LATIN CAPITAL LETTER I WITH ACUTE */
        {L"lt-LT", LCMAP_LOWERCASE, 0x0128,0x0129},  /* LATIN CAPITAL LETTER I WITH TILDE */

        {L"en-US", LCMAP_UPPERCASE, 'i', 'I'}, /* LATIN SMALL LETTER I */
        {L"lt-LT", LCMAP_UPPERCASE, 'i', 'I'}, /* LATIN SMALL LETTER I */
        {L"tr-TR", LCMAP_UPPERCASE, 'i', 'I', 0x0130}, /* LATIN SMALL LETTER I */
        {L"TR-TR", LCMAP_UPPERCASE, 'i', 'I', 0x0130}, /* LATIN SMALL LETTER I */
        {L"az-Cyrl-az", LCMAP_UPPERCASE, 'i', 'I', 0x0130, TRUE /*win7*/}, /* LATIN SMALL LETTER I */
        {L"az-Latn-az", LCMAP_UPPERCASE, 'i', 'I', 0x0130}, /* LATIN SMALL LETTER I */

        {L"en-US", LCMAP_LOWERCASE, 'I', 'i'}, /* LATIN CAPITAL LETTER I */
        {L"lt-LT", LCMAP_LOWERCASE, 'I', 'i'}, /* LATIN CAPITAL LETTER I */
        {L"tr-TR", LCMAP_LOWERCASE, 'I', 'i', 0x0131}, /* LATIN CAPITAL LETTER I */
        {L"TR-TR", LCMAP_LOWERCASE, 'I', 'i', 0x0131}, /* LATIN CAPITAL LETTER I */
        {L"az-Cyrl-az", LCMAP_LOWERCASE, 'I', 'i', 0x0131, TRUE /*win7*/}, /* LATIN CAPITAL LETTER I */
        {L"az-Latn-az", LCMAP_LOWERCASE, 'I', 'i', 0x0131}, /* LATIN CAPITAL LETTER I */

        {L"en-US", LCMAP_LOWERCASE, 0x0130,0,'i'}, /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
        {L"tr-TR", LCMAP_LOWERCASE, 0x0130,0,'i'}, /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
        {L"TR-TR", LCMAP_LOWERCASE, 0x0130,0,'i'}, /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
        {L"az-Cyrl-az", LCMAP_LOWERCASE, 0x0130,0,'i'}, /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
        {L"az-Latn-az", LCMAP_LOWERCASE, 0x0130,0,'i'}, /* LATIN CAPITAL LETTER I WITH DOT ABOVE */

        {L"en-US", LCMAP_UPPERCASE, 0x0131,0,'I'}, /* LATIN SMALL LETTER DOTLESS I */
        {L"tr-TR", LCMAP_UPPERCASE, 0x0131,0,'I'}, /* LATIN SMALL LETTER DOTLESS I */
        {L"TR-TR", LCMAP_UPPERCASE, 0x0131,0,'I'}, /* LATIN SMALL LETTER DOTLESS I */
        {L"az-Cyrl-az", LCMAP_UPPERCASE, 0x0131,0,'I'}, /* LATIN SMALL LETTER DOTLESS I */
        {L"az-Latn-az", LCMAP_UPPERCASE, 0x0131,0,'I'}, /* LATIN SMALL LETTER DOTLESS I */

        {L"en-US", LCMAP_LOWERCASE, 0x10418,0x10440,0,TRUE /*win7*/}, /* DESERET CAPITAL LETTER GAY */
        {L"en-US", LCMAP_UPPERCASE, 0x10431,0x10409,0,TRUE /*win7*/}, /* DESERET SMALL LETTER SHORT AH */
    };

    if (!pLCMapStringEx)
    {
        win_skip("LCMapStringEx not available\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        memset(buffer, 0, sizeof(buffer));
        len = put_utf16( src, tests[i].ch );
        ret = pLCMapStringEx(tests[i].lang, tests[i].flags,
                             src, len, buffer, ARRAY_SIZE(buffer), NULL, NULL, 0);
        len = get_utf16( buffer, ret, &val );
        ok(ret == len, "got %d for %04x for %s\n", ret, tests[i].ch, wine_dbgstr_w(tests[i].lang));
        exp = tests[i].exp ? tests[i].exp : tests[i].ch;
        ok(val == exp || broken(tests[i].broken),
            "expected %04x, got %04x for %04x for %s\n",
            exp, val, tests[i].ch, wine_dbgstr_w(tests[i].lang));

        memset(buffer, 0, sizeof(buffer));
        len = put_utf16( src, tests[i].ch );
        ret = pLCMapStringEx(tests[i].lang, tests[i].flags|LCMAP_LINGUISTIC_CASING,
                             src, len, buffer, ARRAY_SIZE(buffer), NULL, NULL, 0);
        len = get_utf16( buffer, ret, &val );
        ok(ret == len, "got %d for %04x for %s\n", ret, tests[i].ch, wine_dbgstr_w(tests[i].lang));
        exp = tests[i].exp_ling ? tests[i].exp_ling : exp;
        ok(val == exp || broken(tests[i].broken),
            "expected %04x, got %04x for %04x for %s\n",
            exp, val, tests[i].ch, wine_dbgstr_w(tests[i].lang));
    }
}

static void test_NLSVersion(void)
{
    static const GUID guid_null = { 0 };
    static const GUID guid_def  = { 0x000000001, 0x57ee, 0x1e5c, {0x00,0xb4,0xd0,0x00,0x0b,0xb1,0xe1,0x1e}};
    static const GUID guid_fr   = { 0x000000003, 0x57ee, 0x1e5c, {0x00,0xb4,0xd0,0x00,0x0b,0xb1,0xe1,0x1e}};
    static const GUID guid_ja   = { 0x000000046, 0x57ee, 0x1e5c, {0x00,0xb4,0xd0,0x00,0x0b,0xb1,0xe1,0x1e}};
    BOOL ret;
    NLSVERSIONINFOEX info;

    if (!pGetNLSVersion)
    {
        win_skip( "GetNLSVersion not available\n" );
        return;
    }

    SetLastError( 0xdeadbeef );
    memset( &info, 0xcc, sizeof(info) );
    info.dwNLSVersionInfoSize = offsetof( NLSVERSIONINFO, dwEffectiveId );
    ret = pGetNLSVersion( COMPARE_STRING, MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH_CANADIAN ),
                          (NLSVERSIONINFO *)&info );
    ok( ret, "GetNLSVersion failed err %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( &info, 0xcc, sizeof(info) );
    info.dwNLSVersionInfoSize = sizeof(info);
    ret = pGetNLSVersion( COMPARE_STRING, MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH_CANADIAN ),
                          (NLSVERSIONINFO *)&info );
    ok( ret || GetLastError() == ERROR_INSUFFICIENT_BUFFER /* < Vista */,
        "GetNLSVersion failed err %lu\n", GetLastError() );
    if (ret)
    {
        ok( info.dwEffectiveId == MAKELANGID( LANG_FRENCH, SUBLANG_FRENCH_CANADIAN ),
            "wrong id %lx\n", info.dwEffectiveId );
        ok( IsEqualIID( &info.guidCustomVersion, &guid_fr ) ||
            broken( IsEqualIID( &info.guidCustomVersion, &guid_null )),  /* <= win7 */
            "wrong guid %s\n", debugstr_guid(&info.guidCustomVersion) );
    }

    SetLastError( 0xdeadbeef );
    info.dwNLSVersionInfoSize = 8;
    ret = pGetNLSVersion( COMPARE_STRING, LOCALE_USER_DEFAULT, (NLSVERSIONINFO *)&info );
    ok( !ret, "GetNLSVersion succeeded\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    info.dwNLSVersionInfoSize = sizeof(info);
    ret = pGetNLSVersion( 2, LOCALE_USER_DEFAULT, (NLSVERSIONINFO *)&info );
    ok( !ret, "GetNLSVersion succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_FLAGS ||
        broken( GetLastError() == ERROR_INSUFFICIENT_BUFFER ), /* win2003 */
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    info.dwNLSVersionInfoSize = sizeof(info);
    ret = pGetNLSVersion( COMPARE_STRING, 0xdeadbeef, (NLSVERSIONINFO *)&info );
    ok( !ret, "GetNLSVersion succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

    if (pGetNLSVersionEx)
    {
        SetLastError( 0xdeadbeef );
        memset( &info, 0xcc, sizeof(info) );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pGetNLSVersionEx( COMPARE_STRING, L"ja-JP", &info );
        ok( ret, "GetNLSVersionEx failed err %lu\n", GetLastError() );
        ok( info.dwEffectiveId == MAKELANGID( LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN ),
            "wrong id %lx\n", info.dwEffectiveId );
        ok( IsEqualIID( &info.guidCustomVersion, &guid_ja ) ||
            broken( IsEqualIID( &info.guidCustomVersion, &guid_null )),  /* <= win7 */
            "wrong guid %s\n", debugstr_guid(&info.guidCustomVersion) );
        trace( "version %08lx %08lx %08lx %s\n", info.dwNLSVersion, info.dwDefinedVersion, info.dwEffectiveId,
               debugstr_guid(&info.guidCustomVersion) );

        SetLastError( 0xdeadbeef );
        memset( &info, 0xcc, sizeof(info) );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pGetNLSVersionEx( COMPARE_STRING, L"fr", &info );
        ok( !ret == !pIsValidLocaleName(L"fr"), "GetNLSVersionEx doesn't match IsValidLocaleName\n" );
        if (ret)
        {
            ok( info.dwEffectiveId == MAKELANGID( LANG_FRENCH, SUBLANG_DEFAULT ),
                "wrong id %lx\n", info.dwEffectiveId );
            ok( IsEqualIID( &info.guidCustomVersion, &guid_fr ) ||
                broken( IsEqualIID( &info.guidCustomVersion, &guid_null )),  /* <= win7 */
                "wrong guid %s\n", debugstr_guid(&info.guidCustomVersion) );
        }

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info) - 1;
        ret = pGetNLSVersionEx( COMPARE_STRING, L"en-US", &info );
        ok( !ret, "GetNLSVersionEx succeeded\n" );
        ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        memset( &info, 0xcc, sizeof(info) );
        info.dwNLSVersionInfoSize = offsetof( NLSVERSIONINFO, dwEffectiveId );
        ret = pGetNLSVersionEx( COMPARE_STRING, L"en-US", &info );
        ok( ret, "GetNLSVersionEx failed err %lu\n", GetLastError() );
        ok( info.dwEffectiveId == 0xcccccccc, "wrong id %lx\n", info.dwEffectiveId );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pGetNLSVersionEx( 2, L"en-US", &info );
        ok( !ret, "GetNLSVersionEx succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_FLAGS, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pGetNLSVersionEx( COMPARE_STRING, L"foobar", &info );
        ok( !ret, "GetNLSVersionEx succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        memset( &info, 0xcc, sizeof(info) );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pGetNLSVersionEx( COMPARE_STRING, L"zz-XX", &info );
        if (!ret) ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        ok( !ret == !pIsValidLocaleName(L"zz-XX"), "GetNLSVersionEx doesn't match IsValidLocaleName\n" );
        if (ret)
        {
            ok( info.dwEffectiveId == LOCALE_CUSTOM_UNSPECIFIED, "wrong id %lx\n", info.dwEffectiveId );
            ok( IsEqualIID( &info.guidCustomVersion, &guid_def ),
                "wrong guid %s\n", debugstr_guid(&info.guidCustomVersion) );
        }

        SetLastError( 0xdeadbeef );
        memset( &info, 0xcc, sizeof(info) );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pGetNLSVersionEx( COMPARE_STRING, LOCALE_NAME_INVARIANT, &info );
        ok( ret, "GetNLSVersionEx failed err %lu\n", GetLastError() );
        if (ret)
        {
            ok( info.dwEffectiveId == LOCALE_INVARIANT, "wrong id %lx\n", info.dwEffectiveId );
            ok( IsEqualIID( &info.guidCustomVersion, &guid_def ) ||
                broken( IsEqualIID( &info.guidCustomVersion, &guid_null )),  /* <= win7 */
                "wrong guid %s\n", debugstr_guid(&info.guidCustomVersion) );
        }
        else ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    }
    else win_skip( "GetNLSVersionEx not available\n" );

    if (pIsValidNLSVersion)
    {
        info.dwNLSVersionInfoSize = sizeof(info);
        pGetNLSVersion( COMPARE_STRING, LOCALE_USER_DEFAULT, (NLSVERSIONINFO *)&info );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pIsValidNLSVersion( COMPARE_STRING, L"ja-JP", &info );
        ok( ret, "IsValidNLSVersion failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = offsetof( NLSVERSIONINFO, dwEffectiveId );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( ret, "IsValidNLSVersion failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pIsValidNLSVersion( 2, L"en-US", &info );
        ok( !ret, "IsValidNLSVersion succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pIsValidNLSVersion( COMPARE_STRING, L"foobar", &info );
        ok( !ret, "IsValidNLSVersion succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        memset( &info, 0xcc, sizeof(info) );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( !ret, "IsValidNLSVersion succeeded\n" );
        ok( GetLastError() == ERROR_SUCCESS, "wrong error %lu\n", GetLastError() );

        info.dwNLSVersionInfoSize = sizeof(info);
        pGetNLSVersion( COMPARE_STRING, LOCALE_USER_DEFAULT, (NLSVERSIONINFO *)&info );
        info.dwNLSVersion++;
        SetLastError( 0xdeadbeef );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( ret, "IsValidNLSVersion failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        info.dwNLSVersion += 0x700; /* much higher ver -> surely invalid */
        SetLastError( 0xdeadbeef );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( !ret, "IsValidNLSVersion succeeded\n" );
        ok( GetLastError() == 0, "wrong error %lu\n", GetLastError() );

        info.dwNLSVersion -= 2 * 0x700; /* much lower ver -> surely invalid */
        SetLastError( 0xdeadbeef );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( !ret, "IsValidNLSVersion succeeded\n" );
        ok( GetLastError() == 0, "wrong error %lu\n", GetLastError() );

        info.dwNLSVersion += 0x700;
        info.dwDefinedVersion += 0x100;
        SetLastError( 0xdeadbeef );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( ret, "IsValidNLSVersion failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        info.dwDefinedVersion -= 0x100;
        info.guidCustomVersion.Data1 = 0x123;
        SetLastError( 0xdeadbeef );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( !ret, "IsValidNLSVersion succeeded\n" );
        ok( GetLastError() == 0, "wrong error %lu\n", GetLastError() );

        info.guidCustomVersion = guid_null;
        SetLastError( 0xdeadbeef );
        ret = pIsValidNLSVersion( COMPARE_STRING, L"en-US", &info );
        ok( ret, "IsValidNLSVersion failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
    }
    else win_skip( "IsValidNLSVersion not available\n" );

    if (pIsNLSDefinedString)
    {
        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info);
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"A", 1 );
        if (ret)
            ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
        else
            ok( broken( GetLastError() == ERROR_INSUFFICIENT_BUFFER ), /* win7 */
                "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = sizeof(info) + 1;
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"A", 1 );
        ok( !ret, "IsNLSDefinedString succeeded\n" );
        ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        info.dwNLSVersionInfoSize = offsetof( NLSVERSIONINFO, dwEffectiveId );
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"A", 1 );
        ok( ret, "IsNLSDefinedString failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pIsNLSDefinedString( 2, 0, (NLSVERSIONINFO *)&info, L"A", 1 );
        ok( !ret, "IsNLSDefinedString succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_FLAGS, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"ABC", -10 );
        ok( ret, "IsNLSDefinedString failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"ABC", -1 );
        ok( ret, "IsNLSDefinedString failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"\xd800", 1 );
        ok( !ret, "IsNLSDefinedString failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pIsNLSDefinedString( COMPARE_STRING, 0, (NLSVERSIONINFO *)&info, L"\xd800", -20 );
        ok( !ret, "IsNLSDefinedString failed err %lu\n", GetLastError() );
        ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
    }
    else win_skip( "IsNLSDefinedString not available\n" );
}

static void test_locale_nls(void)
{
    NTSTATUS status;
    void *addr, *addr2;
    UINT *ptr;
    LCID lcid;
    LARGE_INTEGER size;

    if (!pNtInitializeNlsFiles || !pRtlGetLocaleFileMappingAddress)
    {
        win_skip( "locale.nls functions not supported\n" );
        return;
    }
    size.QuadPart = 0xdeadbeef;
    status = pNtInitializeNlsFiles( &addr, &lcid, &size );
    ok( !status, "NtInitializeNlsFiles failed %lx\n", status );
    trace( "locale %04lx size %I64x\n", lcid, size.QuadPart );
    ptr = addr;
    ok( size.QuadPart == 0xdeadbeef || size.QuadPart == ptr[4] || size.QuadPart == ptr[4] + 8,
        "wrong offset %x / %I64x\n", ptr[4], size.QuadPart );
    ptr = (UINT *)((char *)addr + ptr[4]);
    ok( ptr[0] == 8, "wrong offset %u\n", ptr[0] );
    ok( ptr[3] == 0x5344534e, "wrong magic %x\n", ptr[3] );

    status = pNtInitializeNlsFiles( &addr2, &lcid, &size );
    ok( !status, "NtInitializeNlsFiles failed %lx\n", status );
    ok( addr != addr2, "got same address %p\n", addr );
    ok( !memcmp( addr, addr2, ptr[4] ), "contents differ\n" );
    status = NtUnmapViewOfSection( GetCurrentProcess(), addr );
    ok( !status, "NtUnmapViewOfSection failed %lx\n", status );
    status = NtUnmapViewOfSection( GetCurrentProcess(), addr2 );
    ok( !status, "NtUnmapViewOfSection failed %lx\n", status );

    size.QuadPart = 0xdeadbeef;
    status = pRtlGetLocaleFileMappingAddress( &addr, &lcid, &size );
    ok( !status, "NtInitializeNlsFiles failed %lx\n", status );
    ptr = addr;
    ok( size.QuadPart == 0xdeadbeef || size.QuadPart == ptr[4] || size.QuadPart == ptr[4] + 8,
        "wrong offset %x / %I64x\n", ptr[4], size.QuadPart );
    ptr = (UINT *)((char *)addr + ptr[4]);
    ok( ptr[0] == 8, "wrong offset %u\n", ptr[0] );
    ok( ptr[3] == 0x5344534e, "wrong magic %x\n", ptr[3] );

    /* RtlGetLocaleFileMappingAddress caches the pointer */
    status = pRtlGetLocaleFileMappingAddress( &addr2, &lcid, &size );
    ok( !status, "NtInitializeNlsFiles failed %lx\n", status );
    ok( addr == addr2, "got different address %p / %p\n", addr, addr2 );
}

static void test_geo_name(void)
{
    WCHAR reg_name[32], buf[32], set_name[32], nation[32], region[32];
    BOOL have_name = FALSE, have_region = FALSE, have_nation = FALSE;
    DWORD size, type, name_size;
    LSTATUS status;
    GEOID geoid;
    BOOL bret;
    HKEY key;
    int ret;

    if (!pSetUserGeoName || !pGetUserDefaultGeoName)
    {
        win_skip("GetUserDefaultGeoName / SetUserGeoName is not available, skipping test.\n");
        return;
    }

    status = RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\International\\Geo", 0, KEY_READ | KEY_WRITE, &key);
    ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);

    size = sizeof(reg_name);
    if (!RegQueryValueExW(key, L"Name", NULL, &type, (BYTE *)reg_name, &size))
        have_name = TRUE;

    lstrcpyW(buf, L"QQ");
    RegSetValueExW(key, L"Name", 0, REG_SZ, (BYTE *)buf, (lstrlenW(buf) + 1) * sizeof(WCHAR));

    size = sizeof(reg_name);
    if ((ret = pGetUserDefaultGeoName(NULL, 0)) == 1)
    {
        if (have_name)
        {
            status = RegSetValueExW(key, L"Name", 0, REG_SZ, (BYTE *)reg_name, (lstrlenW(reg_name) + 1) * sizeof(*reg_name));
            ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
        }
        else
        {
            RegDeleteValueW(key, L"Name");
        }
        win_skip("Geo names are not available, skipping test.\n");
        return;
    }

    size = sizeof(nation);
    if (!RegQueryValueExW(key, L"Nation", NULL, &type, (BYTE *)nation, &size))
        have_nation = TRUE;
    size = sizeof(region);
    if (!RegQueryValueExW(key, L"Region", NULL, &type, (BYTE *)region, &size))
        have_region = TRUE;

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(NULL, 0);
    ok((ret == 3 || ret == 4) && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    name_size = ret;

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, 0);
    ok(ret >= 3 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, 2);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(NULL, 1);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(NULL, name_size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, name_size);
    ok(ret == name_size && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"QQ"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    SetLastError(0xdeadbeef);
    bret = pSetUserGeoName(NULL);
    ok(!bret && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    lstrcpyW(set_name, L"QQ");
    SetLastError(0xdeadbeef);
    bret = pSetUserGeoName(set_name);
    ok(!bret && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    lstrcpyW(set_name, L"Xx");
    SetLastError(0xdeadbeef);
    bret = pSetUserGeoName(set_name);
    ok((bret && GetLastError() == 0xdeadbeef) || broken(bret && GetLastError() == 0),
            "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 4 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"001"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));
    geoid = GetUserGeoID(GEOCLASS_REGION);
    ok(geoid == 39070, "Got unexpected geoid %lu.\n", geoid);
    size = sizeof(buf);
    status = RegQueryValueExW(key, L"Name", NULL, &type, (BYTE *)buf, &size);
    ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
    ok(type == REG_SZ, "Got unexpected type %#lx.\n", type);
    ok(!lstrcmpW(buf, L"001"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    lstrcpyW(set_name, L"ar");
    SetLastError(0xdeadbeef);
    bret = pSetUserGeoName(set_name);
    ok((bret && GetLastError() == 0xdeadbeef) || broken(bret && GetLastError() == 0),
            "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok((ret == 3 && GetLastError() == 0xdeadbeef) || broken(ret == 3 && GetLastError() == 0),
            "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"AR"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));
    geoid = GetUserGeoID(GEOCLASS_NATION);
    ok(geoid == 11, "Got unexpected geoid %lu.\n", geoid);

    lstrcpyW(set_name, L"150");
    SetLastError(0xdeadbeef);
    bret = pSetUserGeoName(set_name);
    ok((bret && GetLastError() == 0xdeadbeef) || broken(bret && GetLastError() == 0),
            "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok((ret == 4 && GetLastError() == 0xdeadbeef) || broken(ret == 4 && GetLastError() == 0),
            "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"150"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));
    geoid = GetUserGeoID(GEOCLASS_NATION);
    ok(geoid == 11, "Got unexpected geoid %lu.\n", geoid);

    lstrcpyW(set_name, L"150a");
    SetLastError(0xdeadbeef);
    bret = pSetUserGeoName(set_name);
    ok(!bret && GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());

    bret = SetUserGeoID(21242);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 3 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"XX"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    bret = SetUserGeoID(42483);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 4 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"011"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    bret = SetUserGeoID(333);
    ok(bret, "Got unexpected bret %#x, GetLastError() %lu.\n", bret, GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 3 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"AN"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    RegDeleteValueW(key, L"Name");
    RegDeleteValueW(key, L"Region");
    lstrcpyW(buf, L"124");
    status = RegSetValueExW(key, L"Nation", 0, REG_SZ, (BYTE *)buf, (lstrlenW(buf) + 1) * sizeof(*buf));
    ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 3 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"JM"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    lstrcpyW(buf, L"333");
    status = RegSetValueExW(key, L"Region", 0, REG_SZ, (BYTE *)buf, (lstrlenW(buf) + 1) * sizeof(*buf));
    ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 3 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"JM"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    RegDeleteValueW(key, L"Nation");
    SetLastError(0xdeadbeef);
    ret = pGetUserDefaultGeoName(buf, ARRAY_SIZE(buf));
    ok(ret == 4 && GetLastError() == 0xdeadbeef, "Got unexpected ret %u, GetLastError() %lu.\n", ret, GetLastError());
    ok(!lstrcmpW(buf, L"001"), "Got unexpected name %s.\n", wine_dbgstr_w(buf));

    /* Restore user geo data. */
    if (have_name)
    {
        status = RegSetValueExW(key, L"Name", 0, REG_SZ, (BYTE *)reg_name, (lstrlenW(reg_name) + 1) * sizeof(*reg_name));
        ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
    }
    else
    {
        RegDeleteValueW(key, L"Name");
    }
    if (have_nation)
    {
        status = RegSetValueExW(key, L"Nation", 0, REG_SZ, (BYTE *)nation, (lstrlenW(nation) + 1) * sizeof(*nation));
        ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
    }
    else
    {
        RegDeleteValueW(key, L"Nation");
    }
    if (have_region)
    {
        status = RegSetValueExW(key, L"Region", 0, REG_SZ, (BYTE *)region, (lstrlenW(region) + 1) * sizeof(*region));
        ok(status == ERROR_SUCCESS, "Got unexpected status %#lx.\n", status);
    }
    else
    {
        RegDeleteValueW(key, L"Region");
    }

    RegCloseKey(key);
}

static const LCID locales_with_optional_calendars[] = {
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SAUDI_ARABIA), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_LEBANON), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_EGYPT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_ALGERIA), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_BAHRAIN), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_IRAQ), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_JORDAN), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_KUWAIT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_LIBYA), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_MOROCCO), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_OMAN), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_QATAR), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_SYRIA), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_TUNISIA), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_UAE), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_ARABIC_YEMEN), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_DIVEHI, SUBLANG_DEFAULT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_PERSIAN, SUBLANG_DEFAULT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_THAI, SUBLANG_DEFAULT), SORT_DEFAULT),
    MAKELCID(MAKELANGID(LANG_URDU, SUBLANG_URDU_PAKISTAN), SORT_DEFAULT)
};

static BOOL CALLBACK calinfo_procA(LPSTR calinfo)
{
    char *end;
    int val = strtoul( calinfo, &end, 10 );
    ok( !*end, "wrong value %s\n", debugstr_a(calinfo) );
    ok(val >= CAL_GREGORIAN && val <= CAL_UMALQURA, "got %d\n", val);
    return TRUE;
}

static void test_EnumCalendarInfoA(void)
{
    BOOL ret;
    INT i;

    ret = EnumCalendarInfoA( calinfo_procA, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS, CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoA for user default locale failed: %lu\n", GetLastError() );

    ret = EnumCalendarInfoA( calinfo_procA, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS,
                             CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoA for user default locale failed: %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE( locales_with_optional_calendars ); i++)
    {
        LCID lcid = locales_with_optional_calendars[i];
        ret = EnumCalendarInfoA( calinfo_procA, lcid, ENUM_ALL_CALENDARS,
                                 CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
        ok( ret || broken( GetLastError() == ERROR_INVALID_FLAGS ) /* no locale */,
            "EnumCalendarInfoA for LCID %#06lx failed: %lu\n", lcid, GetLastError() );
    }
}

static BOOL CALLBACK calinfo_procW(LPWSTR calinfo)
{
    WCHAR *end;
    int val = wcstoul( calinfo, &end, 10 );
    ok( !*end, "wrong value %s\n", debugstr_w(calinfo) );
    ok(val >= CAL_GREGORIAN && val <= CAL_UMALQURA, "got %d\n", val);
    return TRUE;
}

static void test_EnumCalendarInfoW(void)
{
    BOOL ret;
    INT i;

    ret = EnumCalendarInfoW( calinfo_procW, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS, CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoW for user default locale failed: %lu\n", GetLastError() );

    ret = EnumCalendarInfoW( calinfo_procW, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS,
                             CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoW for user default locale failed: %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE( locales_with_optional_calendars ); i++)
    {
        LCID lcid = locales_with_optional_calendars[i];
        ret = EnumCalendarInfoW( calinfo_procW, lcid, ENUM_ALL_CALENDARS,
                                 CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
        ok( ret || broken( GetLastError() == ERROR_INVALID_FLAGS ) /* no locale */,
            "EnumCalendarInfoW for LCID %#06lx failed: %lu\n", lcid, GetLastError() );
    }
}

static BOOL CALLBACK calinfoex_procA(LPSTR calinfo, LCID calid)
{
    char *end;
    int val = strtoul( calinfo, &end, 10 );
    ok( !*end, "wrong value %s\n", debugstr_a(calinfo) );
    ok(val >= CAL_GREGORIAN && val <= CAL_UMALQURA, "got %d\n", val);
    return TRUE;
}

static void test_EnumCalendarInfoExA(void)
{
    BOOL ret;
    INT i;

    ret = EnumCalendarInfoExA( calinfoex_procA, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS, CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoExA for user default locale failed: %lu\n", GetLastError() );

    ret = EnumCalendarInfoExA( calinfoex_procA, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS,
                               CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoExA for user default locale failed: %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE( locales_with_optional_calendars ); i++)
    {
        LCID lcid = locales_with_optional_calendars[i];
        ret = EnumCalendarInfoExA( calinfoex_procA, lcid, ENUM_ALL_CALENDARS,
                                   CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
        ok( ret || broken( GetLastError() == ERROR_INVALID_FLAGS ) /* no locale */,
            "EnumCalendarInfoExA for LCID %#06lx failed: %lu\n", lcid, GetLastError() );
    }
}

static BOOL CALLBACK calinfoex_procW(LPWSTR calinfo, LCID calid)
{
    WCHAR *end;
    int val = wcstoul( calinfo, &end, 10 );
    ok( !*end, "wrong value %s\n", debugstr_w(calinfo) );
    ok(val >= CAL_GREGORIAN && val <= CAL_UMALQURA, "got %d\n", val);
    return TRUE;
}

static void test_EnumCalendarInfoExW(void)
{
    BOOL ret;
    INT i;

    ret = EnumCalendarInfoExW( calinfoex_procW, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS, CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoExW for user default locale failed: %lu\n", GetLastError() );

    ret = EnumCalendarInfoExW( calinfoex_procW, LOCALE_USER_DEFAULT, ENUM_ALL_CALENDARS,
                               CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
    ok( ret, "EnumCalendarInfoExW for user default locale failed: %lu\n", GetLastError() );

    for (i = 0; i < ARRAY_SIZE( locales_with_optional_calendars ); i++)
    {
        LCID lcid = locales_with_optional_calendars[i];
        ret = EnumCalendarInfoExW( calinfoex_procW, lcid, ENUM_ALL_CALENDARS,
                                   CAL_RETURN_NUMBER | CAL_ICALINTVALUE );
        ok( ret || broken( GetLastError() == ERROR_INVALID_FLAGS ) /* no locale */,
            "EnumCalendarInfoExW for LCID %#06lx failed: %lu\n", lcid, GetLastError() );
    }
}

/* Generate sort keys for a list of Unicode code points.
 * Possible source files:
 *   The Unicode collation test suite:  https://www.unicode.org/Public/UCA/latest/CollationTest.zip
 *   The list of supported char compressions:  winedump nls/sortdefault.nls | grep \\-\>
 */
static void dump_sortkeys( char *argv[] )
{
    WCHAR data[128];
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    BYTE key[256];
    unsigned int i, val, pos, res, flags = 0;
    char *p, *end, buffer[1024];
    FILE *f = fopen( argv[1], "r" );

    locale[0] = 0;
    if (argv[2])
    {
        MultiByteToWideChar( CP_ACP, 0, argv[2], -1, locale, LOCALE_NAME_MAX_LENGTH );
        if (argv[3]) flags = strtoul( argv[3], NULL, 0 );
    }

    if (!f)
    {
        fprintf( stderr, "cannot open %s\n", argv[1] );
        return;
    }
    while (fgets( buffer, sizeof(buffer), f ))
    {
        if (buffer[0] && buffer[strlen(buffer)-1] == '\n') buffer[strlen(buffer)-1] = 0;
        p = buffer;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#') continue;
        pos = 0;
        while (*p && *p != ';' && *p != '-')
        {
            val = strtoul( p, &end, 16 );
            if (end == p) break;
            if (val >= 0x10000)
            {
                data[pos++] = 0xd800 | (val >> 10);
                data[pos++] = 0xdc00 | (val & 0x3ff);
            }
            else data[pos++] = val;
            p = end;
            while (*p == ' ' || *p == '\t') p++;
        }
        *p = 0;
#ifndef __REACTOS__
        res = LCMapStringEx( locale, flags | LCMAP_SORTKEY, data, pos,
                             (WCHAR *)key, sizeof(key), NULL, NULL, 0 );
        printf( "%s:", buffer );
        for (i = 0; i < res; i++) printf( " %02x", key[i] );
        printf( "\n" );
#endif
    }
    fclose( f );
}

#ifndef __REACTOS__
static BOOL CALLBACK EnumDateFormatsExEx_proc(LPWSTR date_format_string, CALID calendar_id, LPARAM lp)
{
    return TRUE;
}

static void test_EnumDateFormatsExEx(void)
{
    DWORD error;
    BOOL ret;

    /* Invalid locale name */
    ret = EnumDateFormatsExEx(EnumDateFormatsExEx_proc, L"deadbeef", DATE_SHORTDATE, 0);
    error = GetLastError();
    ok(!ret, "EnumDateFormatsExEx succeeded.\n");
    ok(error == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", error);

    /* yi-Hebr is missing on versions < Win10 */
    /* Running the following tests will cause other tests that use LOCALE_CUSTOM_UNSPECIFIED to
     * report yi-Hebr instead the default locale on Windows 10. So run them at the end */
    ret = EnumDateFormatsExEx(EnumDateFormatsExEx_proc, L"yi-Hebr", DATE_SHORTDATE, 0);
    error = GetLastError();
    ok(ret || (!ret && error == ERROR_INVALID_PARAMETER), /* < Win10 */
       "EnumDateFormatsExEx failed, error %#lx.\n", error);

    ret = EnumDateFormatsExEx(EnumDateFormatsExEx_proc, L"yi-Hebr", DATE_LONGDATE, 0);
    error = GetLastError();
    ok(ret || (!ret && error == ERROR_INVALID_PARAMETER), /* < Win10 */
       "EnumDateFormatsExEx failed, error %#lx.\n", error);
}
#endif

START_TEST(locale)
{
  char **argv;
  int argc = winetest_get_mainargs( &argv );

  InitFunctionPointers();

  if (argc >= 4)
  {
      if (!strcmp( argv[2], "sortkeys" ))
      {
          dump_sortkeys( argv + 2 );
          return;
      }
  }

  test_EnumTimeFormatsA();
  test_EnumTimeFormatsW();
  test_EnumDateFormatsA();
  test_GetLocaleInfoA();
  test_GetLocaleInfoW();
  test_GetLocaleInfoEx();
  test_GetTimeFormatA();
  test_GetTimeFormatEx();
  test_GetDateFormatA();
  test_GetDateFormatEx();
  test_GetDateFormatW();
  test_GetCurrencyFormatA(); /* Also tests the W version */
  test_GetNumberFormatA();   /* Also tests the W version */
  test_GetNumberFormatEx();
  test_CompareStringA();
  test_CompareStringW();
  test_CompareStringEx();
  test_LCMapStringA();
  test_LCMapStringW();
  test_LCMapStringEx();
  test_LocaleNameToLCID();
  test_FoldStringA();
  test_FoldStringW();
  test_ConvertDefaultLocale();
  test_EnumSystemLanguageGroupsA();
  test_EnumSystemLocalesA();
  test_EnumSystemLocalesW();
  test_EnumSystemLocalesEx();
  test_EnumLanguageGroupLocalesA();
  test_SetLocaleInfo();
  test_EnumUILanguageA();
  test_GetCPInfo();
  test_GetStringTypeW();
  test_Idn();
  test_IsValidLocaleName();
  test_ResolveLocaleName();
  test_CompareStringOrdinal();
  test_GetGeoInfo();
  test_EnumSystemGeoID();
  test_invariant();
  test_GetSystemPreferredUILanguages();
  test_GetThreadPreferredUILanguages();
  test_GetUserPreferredUILanguages();
  test_FindNLSStringEx();
  test_FindStringOrdinal();
  test_SetThreadUILanguage();
  test_NormalizeString();
  test_SpecialCasing();
  test_NLSVersion();
  test_locale_nls();
  test_geo_name();
  test_sorting();
  test_unicode_sorting();
  test_EnumCalendarInfoA();
  test_EnumCalendarInfoW();
  test_EnumCalendarInfoExA();
  test_EnumCalendarInfoExW();

  /* Run this test at the end */
#ifndef __REACTOS__
  test_EnumDateFormatsExEx();
#endif
}
