/*
* Unit test suite for rich edit control 1.0
*
* Copyright 2006 Google (Thomas Kho)
* Copyright 2007 Matt Finnicum
* Copyright 2007 Dmitry Timoshkov
* Copyright 2007 Alex Villacís Lasso
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

#include <stdarg.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <ole2.h>
#include <commdlg.h>
#include <richedit.h>
#include <time.h>
#include <wine/test.h>

static HMODULE hmoduleRichEdit;
static BOOL is_lang_japanese;

static HWND new_window(LPCSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindowA(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 500, 60, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent) {
  return new_window(RICHEDIT_CLASS10A, ES_MULTILINE, parent);
}

static BOOL is_rtl(void) {
  LOCALESIGNATURE sig;

  return (GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_FONTSIGNATURE,
                         (LPSTR) &sig, sizeof(LOCALESIGNATURE)) &&
          (sig.lsUsb[3] & 0x08000000) != 0);
}

static void test_WM_SETTEXT(void)
{
  static const struct {
    const char *itemtext;
    DWORD lines;
    DWORD lines_rtl;
    DWORD lines_broken;
  } testitems[] = {
    { "TestSomeText", 1, 1},
    { "TestSomeText\r", 1, 1},
    { "TestSomeText\rSomeMoreText\r", 2, 1, 1}, /* NT4 and below */
    { "TestSomeText\n\nTestSomeText", 3, 3},
    { "TestSomeText\r\r\nTestSomeText", 2, 2},
    { "TestSomeText\r\r\n\rTestSomeText", 3, 2, 2}, /* NT4 and below */
    { "TestSomeText\r\n\r\r\n\rTestSomeText", 4, 3, 3}, /* NT4 and below */
    { "TestSomeText\r\n", 2, 2},
    { "TestSomeText\r\nSomeMoreText\r\n", 3, 3},
    { "TestSomeText\r\n\r\nTestSomeText", 3, 3},
    { "TestSomeText TestSomeText", 1, 1},
    { "TestSomeText \r\nTestSomeText", 2, 2},
    { "TestSomeText\r\n \r\nTestSomeText", 3, 3},
    { "TestSomeText\n", 2, 2},
    { "TestSomeText\r\r\r", 3, 1, 1}, /* NT4 and below */
    { "TestSomeText\r\r\rSomeMoreText", 4, 1, 1} /* NT4 and below */
  };
  HWND hwndRichEdit = new_richedit(NULL);
  int i;
  BOOL rtl = is_rtl();

  /* This test attempts to show that WM_SETTEXT on a riched32 control does not
   * attempt to modify the text that is pasted into the control, and should
   * return it as is. In particular, \r\r\n is NOT converted, unlike riched20.
   *
   * For riched32, the rules for breaking lines seem to be the following:
   * - \r\n is one line break. This is the normal case.
   * - \r{0,2}\n is one line break. In particular, \n by itself is a line break.
   * - \r{0,N-1}\r\r\n is N line breaks.
   * - \n{1,N} are that many line breaks.
   * - \r with text or other characters (except \n) past it, is a line break. That
   *   is, a run of \r{N} without a terminating \n is considered N line breaks
   * - \r at the end of the text is NOT a line break. This differs from riched20,
   *   where \r at the end of the text is a proper line break.
   * However, on RTL language versions, \r is simply skipped and never used
   * for line breaking (only \n adds a line break)
   */

  for (i = 0; i < ARRAY_SIZE(testitems); i++) {

    char buf[1024] = {0};
    LRESULT result;

    result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)testitems[i].itemtext);
    ok (result == 1, "[%d] WM_SETTEXT returned %Id instead of 1\n", i, result);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buf);
    ok (result == lstrlenA(buf),
        "[%d] WM_GETTEXT returned %Id instead of expected %u\n",
        i, result, lstrlenA(buf));
    result = strcmp(testitems[i].itemtext, buf);
    ok (result == 0,
        "[%d] WM_SETTEXT round trip: strcmp = %Id\n", i, result);
    result = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok (result == (rtl ? testitems[i].lines_rtl : testitems[i].lines) ||
        broken(testitems[i].lines_broken && result == testitems[i].lines_broken),
        "[%d] EM_GETLINECOUNT returned %Id, expected %ld\n", i, result, testitems[i].lines);
  }

  DestroyWindow(hwndRichEdit);
}

static void test_WM_GETTEXTLENGTH(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    static const char text1[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee";
    static const char text2[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee\r\n";
    static const char text3[] = "abcdef\x8e\xf0";
    int result;

    /* Test for WM_GETTEXTLENGTH */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);
    result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlenA(text1),
       "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
       result, lstrlenA(text1));

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);
    result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlenA(text2),
       "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
       result, lstrlenA(text2));

    /* Test with multibyte character */
    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text3);
        result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
        todo_wine ok(result == 8, "WM_GETTEXTLENGTH returned %d, expected 8\n", result);
    }

    DestroyWindow(hwndRichEdit);
}

static DWORD CALLBACK test_EM_STREAMIN_esCallback(DWORD_PTR dwCookie,
                                         LPBYTE pbBuff,
                                         LONG cb,
                                         LONG *pcb)
{
  const char** str = (const char**)dwCookie;
  int size = strlen(*str);
  *pcb = cb;
  if (*pcb > size) {
    *pcb = size;
  }
  if (*pcb > 0) {
    memcpy(pbBuff, *str, *pcb);
    *str += *pcb;
  }
  return 0;
}


static void test_EM_STREAMIN(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  LRESULT result;
  EDITSTREAM es;
  char buffer[1024] = {0};

  const char * streamText0 = "{\\rtf1 TestSomeText}";
  const char * streamText0a = "{\\rtf1 TestSomeText\\par}";
  const char * streamText0b = "{\\rtf1 TestSomeText\\par\\par}";

  const char * streamText1 =
  "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang12298{\\fonttbl{\\f0\\fswiss\\fprq2\\fcharset0 System;}}\r\n"
  "\\viewkind4\\uc1\\pard\\f0\\fs17 TestSomeText\\par\r\n"
  "}\r\n";

  /* This should be accepted in richedit 1.0 emulation. See bug #8326 */
  const char * streamText2 =
    "{{\\colortbl;\\red0\\green255\\blue102;\\red255\\green255\\blue255;"
    "\\red170\\green255\\blue255;\\red255\\green238\\blue0;\\red51\\green255"
    "\\blue221;\\red238\\green238\\blue238;}\\tx0 \\tx424 \\tx848 \\tx1272 "
    "\\tx1696 \\tx2120 \\tx2544 \\tx2968 \\tx3392 \\tx3816 \\tx4240 \\tx4664 "
    "\\tx5088 \\tx5512 \\tx5936 \\tx6360 \\tx6784 \\tx7208 \\tx7632 \\tx8056 "
    "\\tx8480 \\tx8904 \\tx9328 \\tx9752 \\tx10176 \\tx10600 \\tx11024 "
    "\\tx11448 \\tx11872 \\tx12296 \\tx12720 \\tx13144 \\cf2 RichEdit1\\line }";

  const char * streamText3 = "RichEdit1";

  /* Minimal test without \par at the end */
  es.dwCookie = (DWORD_PTR)&streamText0;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessageA(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0 returned %Id, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %ld, expected %d\n", es.dwError, 0);

  /* Native richedit 2.0 ignores last \par */
  es.dwCookie = (DWORD_PTR)&streamText0a;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessageA(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0-a returned %Id, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-a set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %ld, expected %d\n", es.dwError, 0);

  /* Native richedit 2.0 ignores last \par, next-to-last \par appears */
  es.dwCookie = (DWORD_PTR)&streamText0b;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessageA(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 14,
      "EM_STREAMIN: Test 0-b returned %Id, expected 14\n", result);
  result = strcmp (buffer,"TestSomeText\r\n");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-b set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %ld, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText1;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessageA(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 1 returned %Id, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 1 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %ld, expected %d\n", es.dwError, 0);


  es.dwCookie = (DWORD_PTR)&streamText2;
  es.dwError = 0;
  SendMessageA(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  todo_wine {
  ok (result  == 9,
      "EM_STREAMIN: Test 2 returned %Id, expected 9\n", result);
  }
  result = strcmp (buffer,"RichEdit1");
  todo_wine {
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);
  }
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %ld, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText3;
  es.dwError = 0;
  SendMessageA(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 3 returned %Id, expected 0\n", result);
  ok (!*buffer,
      "EM_STREAMIN: Test 3 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == -16, "EM_STREAMIN: Test 0 set error %ld, expected %d\n", es.dwError, -16);

  DestroyWindow(hwndRichEdit);
}

static DWORD CALLBACK test_WM_SETTEXT_esCallback(DWORD_PTR dwCookie,
                                         LPBYTE pbBuff,
                                         LONG cb,
                                         LONG *pcb)
{
  char** str = (char**)dwCookie;
  *pcb = cb;
  if (*pcb > 0) {
    memcpy(*str, pbBuff, *pcb);
    *str += *pcb;
  }
  return 0;
}

static void test_EM_STREAMOUT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  int r;
  EDITSTREAM es;
  char buf[1024] = {0};
  char * p;

  const char * TestItem1 = "TestSomeText";
  const char * TestItem2 = "TestSomeText\r";
  const char * TestItem3 = "TestSomeText\r\n";

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem1);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_TEXT), (LPARAM)&es);
  r = strlen(buf);
  ok(r == 12, "streamed text length is %d, expecting 12\n", r);
  ok(strcmp(buf, TestItem1) == 0,
        "streamed text different, got %s\n", buf);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem2);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_TEXT), (LPARAM)&es);
  r = strlen(buf);

  ok(r == 13, "streamed text length is %d, expecting 13\n", r);
  ok(strcmp(buf, TestItem2) == 0,
        "streamed text different, got %s\n", buf);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem3);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_TEXT), (LPARAM)&es);
  r = strlen(buf);
  ok(r == 14, "streamed text length is %d, expecting 14\n", r);
  ok(strcmp(buf, TestItem3) == 0,
        "streamed text different, got %s\n", buf);

  DestroyWindow(hwndRichEdit);
}

static const struct getline_s {
  int line;
  size_t buffer_len;
  const char *text;
  const char *broken_text;
} gl[] = {
  {0, 10, "foo bar\r\n", "foo bar\r\n"},
  {1, 10, "\r",          "\r\r\r\n"},
  {2, 10, "\r\r\n",      "bar\n"},
  {3, 10, "bar\n",       "\r\n"},
  {4, 10, "\r\n"},

  /* Buffer smaller than line length */
  {0, 2, "foo bar\r"},
  {0, 1, "foo bar\r"},
  {0, 0, "foo bar\r"}
};

static void test_EM_GETLINE(void)
{
  int i;
  HWND hwndRichEdit = new_richedit(NULL);
  static const int nBuf = 1024;
  char dest[1024], origdest[1024];
  LRESULT linecount;
  const char text[] = "foo bar\r\n"
      "\r"
      "\r\r\n"
      "bar\n";
  BOOL broken_os = FALSE;
  BOOL rtl = is_rtl();

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  linecount = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
  if (linecount == 4)
  {
    broken_os = TRUE;
    win_skip("Win9x, WinME and NT4 handle '\\r only' differently\n");
  }

  memset(origdest, 0xBB, nBuf);
  for (i = 0; i < ARRAY_SIZE(gl); i++)
  {
    int nCopied, expected_nCopied, expected_bytes_written;
    char gl_text[1024];

    if (gl[i].line >= linecount)
      continue; /* Win9x, WinME and NT4 */

    if (broken_os && gl[i].broken_text)
      /* Win9x, WinME and NT4 */
      strcpy(gl_text, gl[i].broken_text);
    else
      strcpy(gl_text, gl[i].text);

    expected_nCopied = min(gl[i].buffer_len, strlen(gl_text));
    /* Cater for the fact that Win9x, WinME and NT4 don't append the '\0' */
    expected_bytes_written = min(gl[i].buffer_len, strlen(gl_text) + (broken_os ? 0 : 1));

    memset(dest, 0xBB, nBuf);
    *(WORD *) dest = gl[i].buffer_len;

    /* EM_GETLINE appends a "\r\0" to the end of the line
     * nCopied counts up to and including the '\r' */
    nCopied = SendMessageA(hwndRichEdit, EM_GETLINE, gl[i].line, (LPARAM)dest);
    ok(nCopied == expected_nCopied, "%d: %d!=%d\n", i, nCopied,
       expected_nCopied);
    /* two special cases since a parameter is passed via dest */
    if (gl[i].buffer_len == 0)
      ok(!dest[0] && !dest[1] && !strncmp(dest+2, origdest+2, nBuf-2),
         "buffer_len=0\n");
    else if (gl[i].buffer_len == 1)
      ok(dest[0] == gl_text[0] && !dest[1] &&
         !strncmp(dest+2, origdest+2, nBuf-2), "buffer_len=1\n");
    else
    {
      ok(!strncmp(dest, gl_text, expected_bytes_written),
         "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
      if (! rtl || expected_bytes_written == gl[i].buffer_len)
        ok(!strncmp(dest + expected_bytes_written, origdest
                    + expected_bytes_written, nBuf - expected_bytes_written),
           "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
      else
        ok(dest[expected_bytes_written] == 0 &&
           !strncmp(dest + expected_bytes_written + 1, origdest
                    + expected_bytes_written + 1, nBuf - (expected_bytes_written + 1)),
           "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
    }
  }

  DestroyWindow(hwndRichEdit);
}

static void test_EM_LINELENGTH(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  const char * text =
        "richedit1\r"
        "richedit1\n"
        "richedit1\r\n"
        "short\r"
        "richedit1\r"
        "\r"
        "\r"
        "\r\r\n";
  int offset_test[16][2] = {
        {0, 9},  /* Line 1: |richedit1\r */
        {5, 9},  /* Line 1: riche|dit1\r */
        {10, 9}, /* Line 2: |richedit1\n */
        {15, 9}, /* Line 2: riche|dit1\n */
        {20, 9}, /* Line 3: |richedit1\r\n */
        {25, 9}, /* Line 3: riche|dit1\r\n */
        {30, 9}, /* Line 3: richedit1\r|\n */
        {31, 5}, /* Line 4: |short\r */
        {42, 9}, /* Line 5: riche|dit1\r */
        {46, 9}, /* Line 5: richedit1|\r */
        {47, 0}, /* Line 6: |\r */
        {48, 0}, /* Line 7: |\r */
        {49, 0}, /* Line 8: |\r\r\n */
        {50, 0}, /* Line 8: \r|\r\n */
        {51, 0}, /* Line 8: \r\r|\n */
        {52, 0}, /* Line 9: \r\r\n| */
  };
  int i;
  LRESULT result;

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);

  result = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
  if (result == 4) {
     win_skip("Win9x, WinME and NT4 don't handle '\\r only' correctly\n");
     return;
  }
  ok(result == 9, "Incorrect line count of %Id\n", result);

  for (i = 0; i < ARRAY_SIZE(offset_test); i++) {
    result = SendMessageA(hwndRichEdit, EM_LINELENGTH, offset_test[i][0], 0);
    ok(result == offset_test[i][1], "Length of line at offset %d is %Id, expected %d\n",
       offset_test[i][0], result, offset_test[i][1]);
  }

  /* Test with multibyte character */
  if (!is_lang_japanese)
    skip("Skip multibyte character tests on non-Japanese platform\n");
  else
  {
    const char *text1 =
          "wine\n"
          "richedit\x8e\xf0\n"
          "wine";
    static int offset_test1[3][3] = {
           {0, 4},  /* Line 1: |wine\n */
           {5, 10, 1}, /* Line 2: |richedit\x8e\xf0\n */
           {16, 4}, /* Line 3: |wine */
    };
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);
    for (i = 0; i < ARRAY_SIZE(offset_test1); i++) {
      result = SendMessageA(hwndRichEdit, EM_LINELENGTH, offset_test1[i][0], 0);
      todo_wine_if (offset_test1[i][2])
          ok(result == offset_test1[i][1], "Length of line at offset %d is %Id, expected %d\n",
             offset_test1[i][0], result, offset_test1[i][1]);
    }
  }

  DestroyWindow(hwndRichEdit);
}

static void test_EM_GETTEXTRANGE(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    const char * text1 = "foo bar\r\nfoo bar";
    const char * text3 = "foo bar\rfoo bar";
    const char * expect1 = "bar\r\nfoo";
    const char * expect2 = "\nfoo";
    const char * expect3 = "bar\rfoo";
    char buffer[1024] = {0};
    LRESULT result;
    TEXTRANGEA textRange;

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 12;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 8, "EM_GETTEXTRANGE returned %Id\n", result);
    ok(!strcmp(expect1, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 8;
    textRange.chrg.cpMax = 12;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 4, "EM_GETTEXTRANGE returned %Id\n", result);
    ok(!strcmp(expect2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text3);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 11;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 7, "EM_GETTEXTRANGE returned %Id\n", result);

    ok(!strcmp(expect3, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* Test with multibyte character */
    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        textRange.chrg.cpMin = 4;
        textRange.chrg.cpMax = 8;
        result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
        ok(result == 4, "EM_GETTEXTRANGE returned %Id\n", result);
        ok(!strcmp("ef\x8e\xf0", buffer), "EM_GETTEXTRANGE filled %s\n", buffer);
    }

    DestroyWindow(hwndRichEdit);
}

static void test_EM_GETSELTEXT(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    const char * text1 = "foo bar\r\nfoo bar";
    const char * text2 = "foo bar\rfoo bar";
    const char * expect1 = "bar\r\nfoo";
    const char * expect2 = "bar\rfoo";
    char buffer[1024] = {0};
    LRESULT result;

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    SendMessageA(hwndRichEdit, EM_SETSEL, 4, 12);
    result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 8, "EM_GETSELTEXT returned %Id\n", result);
    ok(!strcmp(expect1, buffer), "EM_GETSELTEXT filled %s\n", buffer);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    SendMessageA(hwndRichEdit, EM_SETSEL, 4, 11);
    result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 7, "EM_GETSELTEXT returned %Id\n", result);
    ok(!strcmp(expect2, buffer), "EM_GETSELTEXT filled %s\n", buffer);

    /* Test with multibyte character */
    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        SendMessageA(hwndRichEdit, EM_SETSEL, 4, 8);
        result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
        ok(result == 4, "EM_GETSELTEXT returned %Id\n", result);
        ok(!strcmp("ef\x8e\xf0", buffer), "EM_GETSELTEXT filled %s\n", buffer);
    }

    DestroyWindow(hwndRichEdit);
}

static const char haystack[] = "WINEWine wineWine wine WineWine";
                             /* ^0        ^10       ^20       ^30 */

static const char haystack2[] = "first\r\r\nsecond";

struct find_s {
  int start;
  int end;
  const char *needle;
  int flags;
  int expected_loc;
};


static struct find_s find_tests[] = {
  /* Find in empty text */
  {0, -1, "foo", FR_DOWN, -1},
  {0, -1, "foo", 0, -1},
  {0, -1, "", FR_DOWN, -1},
  {20, 5, "foo", FR_DOWN, -1},
  {5, 20, "foo", FR_DOWN, -1}
};

static struct find_s find_tests2[] = {
  /* No-result find */
  {0, -1, "foo", FR_DOWN | FR_MATCHCASE, -1},
  {5, 20, "WINE", FR_DOWN | FR_MATCHCASE, -1},

  /* Subsequent finds */
  {0, -1, "Wine", FR_DOWN | FR_MATCHCASE, 4},
  {5, 31, "Wine", FR_DOWN | FR_MATCHCASE, 13},
  {14, 31, "Wine", FR_DOWN | FR_MATCHCASE, 23},
  {24, 31, "Wine", FR_DOWN | FR_MATCHCASE, 27},

  /* Find backwards */
  {19, 20, "Wine", FR_MATCHCASE, -1},
  {10, 20, "Wine", FR_MATCHCASE, 13},
  {20, 10, "Wine", FR_MATCHCASE, -1},

  /* Case-insensitive */
  {1, 31, "wInE", FR_DOWN, 4},
  {1, 31, "Wine", FR_DOWN, 4},

  /* High-to-low ranges */
  {20, 5, "Wine", FR_DOWN, -1},
  {2, 1, "Wine", FR_DOWN, -1},
  {30, 29, "Wine", FR_DOWN, -1},
  {20, 5, "Wine", 0, /*13*/ -1},

  /* Find nothing */
  {5, 10, "", FR_DOWN, -1},
  {10, 5, "", FR_DOWN, -1},
  {0, -1, "", FR_DOWN, -1},
  {10, 5, "", 0, -1},

  /* Whole-word search */
  {0, -1, "wine", FR_DOWN | FR_WHOLEWORD, 18},
  {0, -1, "win", FR_DOWN | FR_WHOLEWORD, -1},
  {13, -1, "wine", FR_DOWN | FR_WHOLEWORD, 18},
  {0, -1, "winewine", FR_DOWN | FR_WHOLEWORD, 0},
  {10, -1, "winewine", FR_DOWN | FR_WHOLEWORD, 23},
  {11, -1, "winewine", FR_WHOLEWORD, 23},
  {31, -1, "winewine", FR_WHOLEWORD, -1},

  /* Bad ranges */
  {5, 200, "XXX", FR_DOWN, -1},
  {-20, 20, "Wine", FR_DOWN, -1},
  {-20, 20, "Wine", FR_DOWN, -1},
  {-15, -20, "Wine", FR_DOWN, -1},
  {1<<12, 1<<13, "Wine", FR_DOWN, -1},

  /* Check the case noted in bug 4479 where matches at end aren't recognized */
  {23, 31, "Wine", FR_DOWN | FR_MATCHCASE, 23},
  {27, 31, "Wine", FR_DOWN | FR_MATCHCASE, 27},
  {27, 32, "Wine", FR_DOWN | FR_MATCHCASE, 27},
  {13, 31, "WineWine", FR_DOWN | FR_MATCHCASE, 23},
  {13, 32, "WineWine", FR_DOWN | FR_MATCHCASE, 23},

  /* The backwards case of bug 4479; bounds look right
   * Fails because backward find is wrong */
  {19, 20, "WINE", FR_MATCHCASE, -1},
  {0, 20, "WINE", FR_MATCHCASE, 0},

  {0, -1, "wineWine wine", FR_DOWN, 0},
  {0, -1, "wineWine wine", 0, 0},
  {0, -1, "INEW", 0, 1},
  {0, 31, "INEW", 0, 1},
  {4, -1, "INEW", 0, 10},
};

static struct find_s find_tests3[] = {
  /* Searching for end of line characters */
  {0, -1, "t\r\r\ns", FR_DOWN | FR_MATCHCASE, 4},
  {6, -1, "\r\n", FR_DOWN | FR_MATCHCASE, 6},
  {7, -1, "\n", FR_DOWN | FR_MATCHCASE, 7},
};

static void check_EM_FINDTEXT(HWND hwnd, const char *name, struct find_s *f, int id) {
  int findloc;
  FINDTEXTA ft;
  memset(&ft, 0, sizeof(ft));
  ft.chrg.cpMin = f->start;
  ft.chrg.cpMax = f->end;
  ft.lpstrText = f->needle;
  findloc = SendMessageA(hwnd, EM_FINDTEXT, f->flags, (LPARAM)&ft);
  ok(findloc == f->expected_loc,
     "EM_FINDTEXT(%s,%d) '%s' in range(%d,%d), flags %08x, got start at %d, expected %d\n",
     name, id, f->needle, f->start, f->end, f->flags, findloc, f->expected_loc);
}

static void check_EM_FINDTEXTEX(HWND hwnd, const char *name, struct find_s *f,
    int id) {
  int findloc;
  FINDTEXTEXA ft;
  int expected_end_loc;

  memset(&ft, 0, sizeof(ft));
  ft.chrg.cpMin = f->start;
  ft.chrg.cpMax = f->end;
  ft.lpstrText = f->needle;
  ft.chrgText.cpMax = 0xdeadbeef;
  findloc = SendMessageA(hwnd, EM_FINDTEXTEX, f->flags, (LPARAM)&ft);
  ok(findloc == f->expected_loc,
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
      name, id, f->needle, f->start, f->end, f->flags, findloc);
  ok(ft.chrgText.cpMin == f->expected_loc,
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %ld, expected %d\n",
      name, id, f->needle, f->start, f->end, f->flags, ft.chrgText.cpMin, f->expected_loc);
  expected_end_loc = ((f->expected_loc == -1) ? -1
        : f->expected_loc + strlen(f->needle));
  ok(ft.chrgText.cpMax == expected_end_loc ||
      broken(ft.chrgText.cpMin == -1 && ft.chrgText.cpMax == 0xdeadbeef), /* Win9x, WinME and NT4 */
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, end at %ld, expected %d\n",
      name, id, f->needle, f->start, f->end, f->flags, ft.chrgText.cpMax, expected_end_loc);
}

static void run_tests_EM_FINDTEXT(HWND hwnd, const char *name, struct find_s *find,
    int num_tests)
{
  int i;

  for (i = 0; i < num_tests; i++) {
    check_EM_FINDTEXT(hwnd, name, &find[i], i);
    check_EM_FINDTEXTEX(hwnd, name, &find[i], i);
  }
}

static void test_EM_FINDTEXT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);

  /* Empty rich edit control */
  run_tests_EM_FINDTEXT(hwndRichEdit, "1", find_tests, ARRAY_SIZE(find_tests));

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)haystack);

  /* Haystack text */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2", find_tests2, ARRAY_SIZE(find_tests2));

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)haystack2);

  /* Haystack text 2 (with EOL characters) */
  run_tests_EM_FINDTEXT(hwndRichEdit, "3", find_tests3, ARRAY_SIZE(find_tests3));

  DestroyWindow(hwndRichEdit);
}

static void test_EM_POSFROMCHAR(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  int i;
  POINTL pl;
  LRESULT result;
  unsigned int height = 0;
  int xpos = 0;
  int xpos_rtl_adjusted = 0;
  static const char text[] = "aa\n"
      "this is a long line of text that should be longer than the "
      "control's width\n"
      "cc\n"
      "dd\n"
      "ee\n"
      "ff\n"
      "gg\n"
      "hh\n";

  /* Fill the control to lines to ensure that most of them are offscreen */
  for (i = 0; i < 50; i++)
  {
    /* Do not modify the string; it is exactly 16 characters long. */
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 0);
    SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"0123456789ABCD\r\n");
  }

  /*
   Richedit 1.0 receives a POINTL* on wParam and character offset on lParam, returns void.
   Richedit 2.0 receives character offset on wParam, ignores lParam, returns MAKELONG(x,y)
   Richedit 3.0 accepts either of the above API conventions.
   */

  /* Testing Richedit 1.0 API format */

  /* Testing start of lines. X-offset should be constant on all cases (native is 1).
     Since all lines are identical and drawn with the same font,
     they should have the same height... right?
   */
  for (i = 0; i < 50; i++)
  {
    /* All the lines are 16 characters long */
    result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, i * 16);
    ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
    if (i == 0)
    {
      ok(pl.y == 0, "EM_POSFROMCHAR reports y=%ld, expected 0\n", pl.y);
      ok(pl.x == 1 ||
          broken(pl.x == 0), /* Win9x, WinME and NT4 */
          "EM_POSFROMCHAR reports x=%ld, expected 1\n", pl.x);
      xpos = pl.x;
      xpos_rtl_adjusted = xpos + (is_rtl() ? 7 : 0);
    }
    else if (i == 1)
    {
      ok(pl.y > 0, "EM_POSFROMCHAR reports y=%ld, expected > 0\n", pl.y);
      ok(pl.x == xpos, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos);
      height = pl.y;
    }
    else
    {
      ok(pl.y == i * height, "EM_POSFROMCHAR reports y=%ld, expected %d\n", pl.y, i * height);
      ok(pl.x == xpos, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos);
    }
  }

  /* Testing position at end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, 50 * 16);
  ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
  ok(pl.y == 50 * height, "EM_POSFROMCHAR reports y=%ld, expected %d\n", pl.y, 50 * height);
  ok(pl.x == xpos, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos);

  /* Testing position way past end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, 55 * 16);
  ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
  ok(pl.y == 50 * height, "EM_POSFROMCHAR reports y=%ld, expected %d\n", pl.y, 50 * height);

  ok(pl.x == xpos_rtl_adjusted, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos_rtl_adjusted);


  /* Testing that vertical scrolling does, in fact, have an effect on EM_POSFROMCHAR */
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0); /* line down */
  for (i = 0; i < 50; i++)
  {
    /* All the lines are 16 characters long */
    result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, i * 16);
    ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
    ok(pl.y == (i - 1) * height,
        "EM_POSFROMCHAR reports y=%ld, expected %d\n",
        pl.y, (i - 1) * height);
    ok(pl.x == xpos, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos);
  }

  /* Testing position at end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, 50 * 16);
  ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
  ok(pl.y == (50 - 1) * height, "EM_POSFROMCHAR reports y=%ld, expected %d\n", pl.y, (50 - 1) * height);
  ok(pl.x == xpos, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos);

  /* Testing position way past end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, 55 * 16);
  ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
  ok(pl.y == (50 - 1) * height, "EM_POSFROMCHAR reports y=%ld, expected %d\n", pl.y, (50 - 1) * height);
  ok(pl.x == xpos_rtl_adjusted, "EM_POSFROMCHAR reports x=%ld, expected %d\n", pl.x, xpos_rtl_adjusted);

  /* Testing that horizontal scrolling does, in fact, have an effect on EM_POSFROMCHAR */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0); /* line up */

  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, 0);
  ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
  ok(pl.y == 0, "EM_POSFROMCHAR reports y=%ld, expected 0\n", pl.y);
  ok(pl.x == 1 ||
      broken(pl.x == 0), /* Win9x, WinME and NT4 */
      "EM_POSFROMCHAR reports x=%ld, expected 1\n", pl.x);
  xpos = pl.x;

  SendMessageA(hwndRichEdit, WM_HSCROLL, SB_LINERIGHT, 0);
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pl, 0);
  ok(result == 0, "EM_POSFROMCHAR returned %Id, expected 0\n", result);
  ok(pl.y == 0, "EM_POSFROMCHAR reports y=%ld, expected 0\n", pl.y);
  todo_wine {
  /* Fails on builtin because horizontal scrollbar is not being shown */
  ok(pl.x < xpos ||
      broken(pl.x == xpos), /* Win9x, WinME and NT4 */
      "EM_POSFROMCHAR reports x=%ld, expected value less than %d\n", pl.x, xpos);
  }
  DestroyWindow(hwndRichEdit);
}

static void test_word_wrap(void)
{
    HWND hwnd;
    POINTL point = {0, 60}; /* This point must be below the first line */
    const char *text = "Must be long enough to test line wrapping";
    DWORD dwCommonStyle = WS_VISIBLE|WS_POPUP|WS_VSCROLL|ES_MULTILINE;
    int res, pos, lines, prevlines, reflines[3];

    /* Test the effect of WS_HSCROLL and ES_AUTOHSCROLL styles on wrapping
     * when specified on window creation and set later. */
    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL, dwCommonStyle,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines > 1, "Line was expected to wrap (lines=%d).\n", lines);

    SetWindowLongA(hwnd, GWL_STYLE, dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL, dwCommonStyle|WS_HSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());

    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines > 1, "Line was expected to wrap (lines=%d).\n", lines);

    SetWindowLongA(hwnd, GWL_STYLE, dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL, dwCommonStyle|ES_AUTOHSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos ||
        broken(pos == lstrlenA(text)), /* Win9x, WinME and NT4 */
        "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line was not expected to wrap (lines=%d).\n", lines);

    SetWindowLongA(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos ||
        broken(pos == lstrlenA(text)), /* Win9x, WinME and NT4 */
        "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line was not expected to wrap (lines=%d).\n", lines);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL,
                        dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos ||
        broken(pos == lstrlenA(text)), /* Win9x, WinME and NT4 */
        "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line was not expected to wrap (lines=%d).\n", lines);

    SetWindowLongA(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos ||
        broken(pos == lstrlenA(text)), /* Win9x, WinME and NT4 */
        "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line was not expected to wrap (lines=%d).\n", lines);

    /* Test the effect of EM_SETTARGETDEVICE on word wrap. */
    res = SendMessageA(hwnd, EM_SETTARGETDEVICE, 0, 1);
    ok(res, "EM_SETTARGETDEVICE failed (returned %d).\n", res);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos ||
        broken(pos == lstrlenA(text)), /* Win9x, WinME and NT4 */
        "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line was not expected to wrap (lines=%d).\n", lines);

    res = SendMessageA(hwnd, EM_SETTARGETDEVICE, 0, 0);
    ok(res, "EM_SETTARGETDEVICE failed (returned %d).\n", res);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    /* First lets see if the text would wrap normally (needed for reference) */
    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL, dwCommonStyle,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    res = SendMessageA(hwnd, EM_REPLACESEL, FALSE, (LPARAM)text);
    ok(res, "EM_REPLACESEL failed.\n");
    /* Should have wrapped */
    reflines[0] = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(reflines[0] > 1, "Line was expected to wrap (%d lines).\n", reflines[0]);
    /* Resize the window to fit the line */
    MoveWindow(hwnd, 0, 0, 600, 80, TRUE);
    /* Text should not be wrapped */
    reflines[1] = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(reflines[1] == 1, "Line wasn't expected to wrap (%d lines).\n", reflines[1]);
    /* Resize the window again to make sure the line wraps again */
    MoveWindow(hwnd, 0, 0, 10, 80, TRUE);
    reflines[2] = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(reflines[2] > 1, "Line was expected to wrap (%d lines).\n", reflines[2]);
    DestroyWindow(hwnd);

    /* Same test with redraw disabled */
    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL, dwCommonStyle,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    ok(IsWindowVisible(hwnd), "Window should be visible.\n");
    /* Redraw is disabled by making the window invisible. */
    SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    ok(!IsWindowVisible(hwnd), "Window shouldn't be visible.\n");
    res = SendMessageA(hwnd, EM_REPLACESEL, FALSE, (LPARAM)text);
    ok(res, "EM_REPLACESEL failed.\n");
    /* Should have wrapped */
    prevlines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(prevlines == reflines[0],
        "Line was expected to wrap (%d lines).\n", prevlines);
    /* Resize the window to fit the line, no change to the number of lines */
    MoveWindow(hwnd, 0, 0, 600, 80, TRUE);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    todo_wine
    ok(lines == prevlines ||
        broken(lines == reflines[1]), /* Win98, WinME and NT4 */
        "Expected no change in the number of lines\n");
    /* Resize the window again to make sure the line wraps again */
    MoveWindow(hwnd, 0, 0, 10, 80, TRUE);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    todo_wine
    ok(lines == prevlines ||
        broken(lines == reflines[2]), /* Win98, WinME and NT4 */
        "Expected no change in the number of lines\n");
    DestroyWindow(hwnd);
}

static void test_EM_GETOPTIONS(void)
{
    HWND hwnd;
    DWORD options;

    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL,
                        WS_POPUP,
                        0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    options = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(options == 0, "Incorrect options %lx\n", options);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS10A, NULL,
                        WS_POPUP|WS_VSCROLL|WS_HSCROLL,
                        0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    options = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(options == ECO_AUTOVSCROLL ||
       broken(options == 0), /* Win9x, WinME and NT4 */
       "Incorrect initial options %lx\n", options);
    DestroyWindow(hwnd);
}

static void test_autoscroll(void)
{
    HWND hwnd;
    UINT ret;

    /* The WS_VSCROLL and WS_HSCROLL styles implicitly set
     * auto vertical/horizontal scrolling options. */
    hwnd = CreateWindowExA(0, RICHEDIT_CLASS10A, NULL,
                          WS_POPUP|ES_MULTILINE|WS_VSCROLL|WS_HSCROLL,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS10A, (int) GetLastError());
    ret = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(ret & ECO_AUTOVSCROLL ||
        broken(!(ret & ECO_AUTOVSCROLL)), /* Win9x, WinME and NT4 */
        "ECO_AUTOVSCROLL isn't set.\n");
    ok(!(ret & ECO_AUTOHSCROLL), "ECO_AUTOHSCROLL is set.\n");
    ret = GetWindowLongA(hwnd, GWL_STYLE);
    todo_wine
    ok(ret & ES_AUTOVSCROLL ||
        broken(!(ret & ES_AUTOVSCROLL)), /* Win9x, WinMe and NT4 */
        "ES_AUTOVSCROLL isn't set.\n");
    ok(!(ret & ES_AUTOHSCROLL), "ES_AUTOHSCROLL is set.\n");
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS10A, NULL,
                          WS_POPUP|ES_MULTILINE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS10A, (int) GetLastError());
    ret = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(!(ret & ECO_AUTOVSCROLL), "ECO_AUTOVSCROLL is set.\n");
    ok(!(ret & ECO_AUTOHSCROLL), "ECO_AUTOHSCROLL is set.\n");
    ret = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(ret & ES_AUTOVSCROLL), "ES_AUTOVSCROLL is set.\n");
    ok(!(ret & ES_AUTOHSCROLL), "ES_AUTOHSCROLL is set.\n");
    DestroyWindow(hwnd);
}

static void simulate_typing_characters(HWND hwnd, const char* szChars)
{
    int ret;

    while (*szChars != '\0') {
        SendMessageA(hwnd, WM_KEYDOWN, *szChars, 1);
        ret = SendMessageA(hwnd, WM_CHAR, *szChars, 1);
        ok(ret == 0, "WM_CHAR('%c') ret=%d\n", *szChars, ret);
        SendMessageA(hwnd, WM_KEYUP, *szChars, 1);
        szChars++;
    }
}

static void format_test_result(char *target, const char *src)
{
    int i;
    for (i = 0; i < strlen(src); i++)
        sprintf(target + 2*i, "%02x", src[i] & 0xFF);
    target[2*i] = 0;
}

/*
 * This test attempts to show the effect of enter on a richedit
 * control v1.0 inserts CRLF whereas for higher versions it only
 * inserts CR. If shows that EM_GETTEXTEX with GT_USECRLF == WM_GETTEXT
 * and also shows that GT_USECRLF has no effect in richedit 1.0, but
 * does for higher. The same test is cloned in riched32 and riched20.
 * Also shows the difference between WM_CHAR/WM_KEYDOWN in v1.0 and higher versions
 */
static void test_enter(void)
{
  static const struct {
    const char *initialtext;
    const int   cursor;
    const char *expectedtext;
  } testenteritems[] = {
    { "aaabbb\r\n", 3, "aaa\r\nbbb\r\n"},
    { "aaabbb\r\n", 6, "aaabbb\r\n\r\n"},
    { "aa\rabbb\r\n", 7, "aa\rabbb\r\n\r\n"},
    { "aa\rabbb\r\n", 3, "aa\r\r\nabbb\r\n"},
    { "aa\rabbb\r\n", 2, "aa\r\n\rabbb\r\n"}
  };

  char expectedbuf[1024];
  char resultbuf[1024];
  HWND hwndRichEdit = new_richedit(NULL);
  UINT i;
  char buf[1024] = {0};
  GETTEXTEX getText = {sizeof(buf)};
  LRESULT result;
  const char *expected;

  for (i = 0; i < ARRAY_SIZE(testenteritems); i++)
  {
    /* Set the text to the initial text */
    result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)testenteritems[i].initialtext);
    ok (result == 1, "[%d] WM_SETTEXT returned %Id instead of 1\n", i, result);

    /* Send Enter */
    SendMessageA(hwndRichEdit, EM_SETSEL, testenteritems[i].cursor, testenteritems[i].cursor);
    simulate_typing_characters(hwndRichEdit, "\r");

    /* 1. Retrieve with WM_GETTEXT */
    buf[0] = 0x00;
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buf);
    expected = testenteritems[i].expectedtext;

    format_test_result(resultbuf, buf);
    format_test_result(expectedbuf, expected);

    result = strcmp(expected, buf);
    ok (result == 0,
        "[%d] WM_GETTEXT unexpected '%s' expected '%s'\n",
        i, resultbuf, expectedbuf);

    /* 2. Retrieve with EM_GETTEXTEX, GT_DEFAULT */
    getText.flags = GT_DEFAULT;
    getText.codepage = CP_ACP;
    buf[0] = 0x00;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
    expected = testenteritems[i].expectedtext;

    format_test_result(resultbuf, buf);
    format_test_result(expectedbuf, expected);

    result = strcmp(expected, buf);
    ok (result == 0 || broken(buf[0]==0x00 /* WinNT4 */),
        "[%d] EM_GETTEXTEX, GT_DEFAULT unexpected '%s', expected '%s'\n",
        i, resultbuf, expectedbuf);

    /* 3. Retrieve with EM_GETTEXTEX, GT_USECRLF */
    getText.flags = GT_USECRLF;
    getText.codepage = CP_ACP;
    buf[0] = 0x00;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
    expected = testenteritems[i].expectedtext;

    format_test_result(resultbuf, buf);
    format_test_result(expectedbuf, expected);

    result = strcmp(expected, buf);
    ok (result == 0 || broken(buf[0]==0x00 /* WinNT4 */),
        "[%d] EM_GETTEXTEX, GT_USECRLF unexpected '%s', expected '%s'\n",
        i, resultbuf, expectedbuf);
  }

  /* Show that WM_CHAR is handled differently from WM_KEYDOWN */
  getText.flags    = GT_DEFAULT;
  getText.codepage = CP_ACP;

  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  ok (result == 1, "[%d] WM_SETTEXT returned %Id instead of 1\n", i, result);
  SendMessageW(hwndRichEdit, WM_CHAR, 'T', 0);
  SendMessageW(hwndRichEdit, WM_KEYDOWN, VK_RETURN, 0);

  result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(result == 1, "Got %d\n", (int)result);
  format_test_result(resultbuf, buf);
  format_test_result(expectedbuf, "T");
  result = strcmp(resultbuf, expectedbuf);
  ok (result == 0, "[%d] EM_GETTEXTEX, GT_DEFAULT unexpected '%s', expected '%s'\n", i, resultbuf, expectedbuf);

  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  ok (result == 1, "[%d] WM_SETTEXT returned %Id instead of 1\n", i, result);
  SendMessageW(hwndRichEdit, WM_CHAR, 'T', 0);
  SendMessageW(hwndRichEdit, WM_CHAR, '\r', 0);

  result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(result == 3, "Got %Id\n", result);
  format_test_result(resultbuf, buf);
  format_test_result(expectedbuf, "T\r\n");
  result = strcmp(resultbuf, expectedbuf);
  ok (result == 0, "[%d] EM_GETTEXTEX, GT_DEFAULT unexpected '%s', expected '%s'\n", i, resultbuf, expectedbuf);

  DestroyWindow(hwndRichEdit);
}

struct exsetsel_s {
  LONG min;
  LONG max;
  LRESULT expected_retval;
  int expected_getsel_start;
  int expected_getsel_end;
  BOOL result_todo;
  BOOL sel_todo;
};

static const struct exsetsel_s exsetsel_tests[] = {
  /* sanity tests */
  {5, 10, 10, 5, 10, 0, 0 },
  {15, 17, 17, 15, 17, 0, 0 },
  /* test cpMax > strlen() */
  {0, 100, 19, 0, 19, 1, 0 },
  /* test cpMin < 0 && cpMax >= 0 after cpMax > strlen() */
  {-1, 1, 17, 17, 17, 1, 0 },
  /* test cpMin == cpMax */
  {5, 5, 5, 5, 5, 0, 0 },
  /* test cpMin < 0 && cpMax >= 0 (bug 4462) */
  {-1, 0, 5, 5, 5, 0, 0 },
  {-1, 17, 5, 5, 5, 0, 0 },
  {-1, 18, 5, 5, 5, 0, 0 },
  /* test cpMin < 0 && cpMax < 0 */
  {-1, -1, 17, 17, 17, 0, 0 },
  {-4, -5, 17, 17, 17, 0, 0 },
  /* test cpMin >=0 && cpMax < 0 (bug 6814) */
  {0, -1, 19, 0, 19, 1, 0 },
  {17, -5, 19, 17, 19, 1, 0 },
  {18, -3, 19, 17, 19, 1, 1 },
  /* test if cpMin > cpMax */
  {15, 19, 19, 15, 19, 1, 0 },
  {19, 15, 19, 15, 19, 1, 0 },
  /* cpMin == strlen() && cpMax > cpMin */
  {17, 18, 17, 17, 17, 1, 1 },
  {17, 50, 19, 17, 19, 1, 0 },
};

static void check_EM_EXSETSEL(HWND hwnd, const struct exsetsel_s *setsel, int id) {
    CHARRANGE cr;
    LRESULT result;
    int start, end;

    cr.cpMin = setsel->min;
    cr.cpMax = setsel->max;
    result = SendMessageA(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    todo_wine_if (setsel->result_todo)
        ok(result == setsel->expected_retval, "EM_EXSETSEL(%d): expected: %Id actual: %Id\n", id, setsel->expected_retval, result);

    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    todo_wine_if (setsel->sel_todo)
        ok(start == setsel->expected_getsel_start && end == setsel->expected_getsel_end,
           "EM_EXSETSEL(%d): expected (%d,%d) actual:(%d,%d)\n",
           id, setsel->expected_getsel_start, setsel->expected_getsel_end, start, end);
}

static void test_EM_EXSETSEL(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    int i;
    const int num_tests = ARRAY_SIZE(exsetsel_tests);

    /* sending some text to the window */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"testing selection");
    /*                                                 01234567890123456 */

    for (i = 0; i < num_tests; i++) {
        check_EM_EXSETSEL(hwndRichEdit, &exsetsel_tests[i], i);
    }

    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        CHARRANGE cr;
        LRESULT result;
#define MAX_BUF_LEN 1024
        char bufA[MAX_BUF_LEN] = {0};

        /* Test with multibyte character */
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        /*                                                 012345  6   7 8901 */
        cr.cpMin = 4; cr.cpMax = 8;
        result =  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
        todo_wine ok(result == 7, "EM_EXSETSEL return %Id expected 7\n", result);
        result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)bufA);
        ok(!strcmp(bufA, "ef\x8e\xf0"), "EM_GETSELTEXT return incorrect string\n");
        SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
        ok(cr.cpMin == 4, "Selection start incorrectly: %ld expected 4\n", cr.cpMin);
        ok(cr.cpMax == 8, "Selection end incorrectly: %ld expected 8\n", cr.cpMax);
    }

    DestroyWindow(hwndRichEdit);
}

static void check_EM_SETSEL(HWND hwnd, const struct exsetsel_s *setsel, int id) {
    LRESULT result;
    int start, end;

    result = SendMessageA(hwnd, EM_SETSEL, setsel->min, setsel->max);

    todo_wine_if (setsel->result_todo)
        ok(result == setsel->expected_retval, "EM_SETSEL(%d): expected: %Id actual: %Id\n", id, setsel->expected_retval, result);

    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    todo_wine_if (setsel->sel_todo)
        ok(start == setsel->expected_getsel_start && end == setsel->expected_getsel_end,
           "EM_SETSEL(%d): expected (%d,%d) actual:(%d,%d)\n",
           id, setsel->expected_getsel_start, setsel->expected_getsel_end, start, end);
}

static void test_EM_SETSEL(void)
{
    char buffA[32] = {0};
    HWND hwndRichEdit = new_richedit(NULL);
    int i;
    const int num_tests = ARRAY_SIZE(exsetsel_tests);

    /* sending some text to the window */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"testing selection");
    /*                                                 01234567890123456 */

    for (i = 0; i < num_tests; i++) {
        check_EM_SETSEL(hwndRichEdit, &exsetsel_tests[i], i);
    }

    SendMessageA(hwndRichEdit, EM_SETSEL, 17, 18);
    buffA[0] = 123;
    SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffA);
    ok(buffA[0] == 0, "selection text %s\n", buffA);

    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        int sel_start, sel_end;
        LRESULT result;

        /* Test with multibyte character */
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        /*                                                 012345  6   7 8901 */
        result =  SendMessageA(hwndRichEdit, EM_SETSEL, 4, 8);
        todo_wine ok(result == 7, "EM_SETSEL return %Id expected 7\n", result);
        result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffA);
        ok(!strcmp(buffA, "ef\x8e\xf0"), "EM_GETSELTEXT return incorrect string\n");
        result = SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
        ok(sel_start == 4, "Selection start incorrectly: %d expected 4\n", sel_start);
        ok(sel_end == 8, "Selection end incorrectly: %d expected 8\n", sel_end);
    }

    DestroyWindow(hwndRichEdit);
}

START_TEST( editor )
{
  MSG msg;
  time_t end;
  BOOL ret;

  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED32.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibraryA("riched32.dll");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());
  is_lang_japanese = (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_JAPANESE);

  test_WM_SETTEXT();
  test_EM_GETTEXTRANGE();
  test_EM_GETSELTEXT();
  test_WM_GETTEXTLENGTH();
  test_EM_STREAMIN();
  test_EM_STREAMOUT();
  test_EM_GETLINE();
  test_EM_LINELENGTH();
  test_EM_FINDTEXT();
  test_EM_POSFROMCHAR();
  test_word_wrap();
  test_EM_GETOPTIONS();
  test_autoscroll();
  test_enter();
  test_EM_EXSETSEL();
  test_EM_SETSEL();

  /* Set the environment variable WINETEST_RICHED32 to keep windows
   * responsive and open for 30 seconds. This is useful for debugging.
   *
   * The message pump uses PeekMessage() to empty the queue and then sleeps for
   * 50ms before retrying the queue. */
  end = time(NULL) + 30;
  if (getenv( "WINETEST_RICHED32" )) {
    while (time(NULL) < end) {
      if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
      } else {
        Sleep(50);
      }
    }
  }

  OleFlushClipboard();
  ret = FreeLibrary(hmoduleRichEdit);
  ok(ret, "error: %lu\n", GetLastError());
}
