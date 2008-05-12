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
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <ole2.h>
#include <richedit.h>
#include <time.h>
#include <wine/test.h>

static HMODULE hmoduleRichEdit;

static HWND new_window(LPCTSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindow(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 200, 60, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent) {
  return new_window(RICHEDIT_CLASS10A, ES_MULTILINE, parent);
}

static void test_WM_SETTEXT()
{
  HWND hwndRichEdit = new_richedit(NULL);
  const char * TestItem1 = "TestSomeText";
  const char * TestItem2 = "TestSomeText\r";
  const char * TestItem3 = "TestSomeText\rSomeMoreText\r";
  const char * TestItem4 = "TestSomeText\n\nTestSomeText";
  const char * TestItem5 = "TestSomeText\r\r\nTestSomeText";
  const char * TestItem6 = "TestSomeText\r\r\n\rTestSomeText";
  const char * TestItem7 = "TestSomeText\r\n\r\r\n\rTestSomeText";
  const char * TestItem8 = "TestSomeText\r\n";
  const char * TestItem9 = "TestSomeText\r\nSomeMoreText\r\n";
  const char * TestItem10 = "TestSomeText\r\n\r\nTestSomeText";
  const char * TestItem11 = "TestSomeText TestSomeText";
  const char * TestItem12 = "TestSomeText \r\nTestSomeText";
  const char * TestItem13 = "TestSomeText\r\n \r\nTestSomeText";
  const char * TestItem14 = "TestSomeText\n";
  const char * TestItem15 = "TestSomeText\r\r\r";
  const char * TestItem16 = "TestSomeText\r\r\rSomeMoreText";
  char buf[1024] = {0};
  LRESULT result;

  /* This test attempts to show that WM_SETTEXT on a riched32 control does not
     attempt to modify the text that is pasted into the control, and should
     return it as is. In particular, \r\r\n is NOT converted, unlike riched20.
     Currently, builtin riched32 mangles solitary \r or \n when not part of
     a \r\n pair.

     For riched32, the rules for breaking lines seem to be the following:
     - \r\n is one line break. This is the normal case.
     - \r{0,N}\n is one line break. In particular, \n by itself is a line break.
     - \n{1,N} are that many line breaks.
     - \r with text or other characters (except \n) past it, is a line break. That
       is, a run of \r{N} without a terminating \n is considered N line breaks
     - \r at the end of the text is NOT a line break. This differs from riched20,
       where \r at the end of the text is a proper line break. This causes
       TestItem2 to fail its test.
   */

#define TEST_SETTEXT(a, b, nlines, is_todo, is_todo2) \
  result = SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) a); \
  ok (result == 1, "WM_SETTEXT returned %ld instead of 1\n", result); \
  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buf); \
  ok (result == lstrlen(buf), \
	"WM_GETTEXT returned %ld instead of expected %u\n", \
	result, lstrlen(buf)); \
  result = strcmp(b, buf); \
  if (is_todo) todo_wine { \
  ok(result == 0, \
        "WM_SETTEXT round trip: strcmp = %ld\n", result); \
  } else { \
  ok(result == 0, \
        "WM_SETTEXT round trip: strcmp = %ld\n", result); \
  } \
  result = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0); \
  if (is_todo2) todo_wine { \
  ok(result == nlines, "EM_GETLINECOUNT returned %ld, expected %d\n", result, nlines); \
  } else { \
  ok(result == nlines, "EM_GETLINECOUNT returned %ld, expected %d\n", result, nlines); \
  }

  TEST_SETTEXT(TestItem1, TestItem1, 1, 0, 0)
  TEST_SETTEXT(TestItem2, TestItem2, 1, 0, 0)
  TEST_SETTEXT(TestItem3, TestItem3, 2, 0, 0)
  TEST_SETTEXT(TestItem4, TestItem4, 3, 0, 0)
  TEST_SETTEXT(TestItem5, TestItem5, 2, 0, 0)
  TEST_SETTEXT(TestItem6, TestItem6, 3, 0, 0)
  TEST_SETTEXT(TestItem7, TestItem7, 4, 0, 0)
  TEST_SETTEXT(TestItem8, TestItem8, 2, 0, 0)
  TEST_SETTEXT(TestItem9, TestItem9, 3, 0, 0)
  TEST_SETTEXT(TestItem10, TestItem10, 3, 0, 0)
  TEST_SETTEXT(TestItem11, TestItem11, 1, 0, 0)
  TEST_SETTEXT(TestItem12, TestItem12, 2, 0, 0)
  TEST_SETTEXT(TestItem13, TestItem13, 3, 0, 0)
  TEST_SETTEXT(TestItem14, TestItem14, 2, 0, 0)
  TEST_SETTEXT(TestItem15, TestItem15, 3, 0, 0)
  TEST_SETTEXT(TestItem16, TestItem16, 4, 0, 0)

#undef TEST_SETTEXT
  DestroyWindow(hwndRichEdit);
}

static void test_WM_GETTEXTLENGTH(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    static const char text3[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee";
    static const char text4[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee\r\n";
    int result;

    /* Test for WM_GETTEXTLENGTH */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text3);
    result = SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlen(text3),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlen(text3));

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text4);
    result = SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlen(text4),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlen(text4));

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
  "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang12298{\\fonttbl{\\f0\\fswiss\\fprq2\\fcharset0 System;}}\r\n" \
  "\\viewkind4\\uc1\\pard\\f0\\fs17 TestSomeText\\par\r\n" \
  "}\r\n";

  /* This should be accepted in richedit 1.0 emulation. See bug #8326 */
  const char * streamText2 =
    "{{\\colortbl;\\red0\\green255\\blue102;\\red255\\green255\\blue255;" \
    "\\red170\\green255\\blue255;\\red255\\green238\\blue0;\\red51\\green255" \
    "\\blue221;\\red238\\green238\\blue238;}\\tx0 \\tx424 \\tx848 \\tx1272 " \
    "\\tx1696 \\tx2120 \\tx2544 \\tx2968 \\tx3392 \\tx3816 \\tx4240 \\tx4664 " \
    "\\tx5088 \\tx5512 \\tx5936 \\tx6360 \\tx6784 \\tx7208 \\tx7632 \\tx8056 " \
    "\\tx8480 \\tx8904 \\tx9328 \\tx9752 \\tx10176 \\tx10600 \\tx11024 " \
    "\\tx11448 \\tx11872 \\tx12296 \\tx12720 \\tx13144 \\cf2 RichEdit1\\line }";

  const char * streamText3 = "RichEdit1";

  /* Minimal test without \par at the end */
  es.dwCookie = (DWORD_PTR)&streamText0;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0 returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, 0);

  /* Native richedit 2.0 ignores last \par */
  es.dwCookie = (DWORD_PTR)&streamText0a;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0-a returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-a set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, 0);

  /* Native richedit 2.0 ignores last \par, next-to-last \par appears */
  es.dwCookie = (DWORD_PTR)&streamText0b;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 14,
      "EM_STREAMIN: Test 0-b returned %ld, expected 14\n", result);
  result = strcmp (buffer,"TestSomeText\r\n");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-b set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText1;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 1 returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 1 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, 0);


  es.dwCookie = (DWORD_PTR)&streamText2;
  es.dwError = 0;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  todo_wine {
  ok (result  == 9,
      "EM_STREAMIN: Test 2 returned %ld, expected 9\n", result);
  }
  result = strcmp (buffer,"RichEdit1");
  todo_wine {
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);
  }
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText3;
  es.dwError = 0;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 3 returned %ld, expected 0\n", result);
  ok (strlen(buffer)  == 0,
      "EM_STREAMIN: Test 3 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == -16, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, -16);

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

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) TestItem1);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessage(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_TEXT), (LPARAM)&es);
  r = strlen(buf);
  ok(r == 12, "streamed text length is %d, expecting 12\n", r);
  ok(strcmp(buf, TestItem1) == 0,
        "streamed text different, got %s\n", buf);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) TestItem2);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessage(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_TEXT), (LPARAM)&es);
  r = strlen(buf);

  ok(r == 13, "streamed text length is %d, expecting 13\n", r);
  ok(strcmp(buf, TestItem2) == 0,
        "streamed text different, got %s\n", buf);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) TestItem3);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessage(hwndRichEdit, EM_STREAMOUT,
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
  int wine_todo;
} gl[] = {
  {0, 10, "foo bar\r\n", 0},
  {1, 10, "\n", 0},
  {2, 10, "bar\n", 0},
  {3, 10, "\r\n", 0},

  /* Buffer smaller than line length */
  {0, 2, "foo bar\r", 0},
  {0, 1, "foo bar\r", 0},
  {0, 0, "foo bar\r", 0}
};

static void test_EM_GETLINE(void)
{
  int i;
  HWND hwndRichEdit = new_richedit(NULL);
  static const int nBuf = 1024;
  char dest[1024], origdest[1024];
  const char text[] = "foo bar\r\n"
      "\n"
      "bar\n";

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);

  memset(origdest, 0xBB, nBuf);
  for (i = 0; i < sizeof(gl)/sizeof(struct getline_s); i++)
  {
    int nCopied;
    int expected_nCopied = min(gl[i].buffer_len, strlen(gl[i].text));
    int expected_bytes_written = min(gl[i].buffer_len, strlen(gl[i].text) + 1);
    memset(dest, 0xBB, nBuf);
    *(WORD *) dest = gl[i].buffer_len;

    /* EM_GETLINE appends a "\r\0" to the end of the line
     * nCopied counts up to and including the '\r' */
    nCopied = SendMessage(hwndRichEdit, EM_GETLINE, gl[i].line, (LPARAM) dest);
    if (gl[i].wine_todo) todo_wine {
    ok(nCopied == expected_nCopied, "%d: %d!=%d\n", i, nCopied,
       expected_nCopied);
    } else
    ok(nCopied == expected_nCopied, "%d: %d!=%d\n", i, nCopied,
       expected_nCopied);
    /* two special cases since a parameter is passed via dest */
    if (gl[i].buffer_len == 0)
      ok(!dest[0] && !dest[1] && !strncmp(dest+2, origdest+2, nBuf-2),
         "buffer_len=0\n");
    else if (gl[i].buffer_len == 1)
      ok(dest[0] == gl[i].text[0] && !dest[1] &&
         !strncmp(dest+2, origdest+2, nBuf-2), "buffer_len=1\n");
    else
    {
      if (gl[i].wine_todo) todo_wine {
      ok(!strncmp(dest, gl[i].text, expected_bytes_written),
         "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
      ok(!strncmp(dest + expected_bytes_written, origdest
                  + expected_bytes_written, nBuf - expected_bytes_written),
         "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
      }
      else
      {
      ok(!strncmp(dest, gl[i].text, expected_bytes_written),
         "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
      ok(!strncmp(dest + expected_bytes_written, origdest
                  + expected_bytes_written, nBuf - expected_bytes_written),
         "%d: expected_bytes_written=%d\n", i, expected_bytes_written);
      }
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
        "richedit1\r\r\r\r\r\n";
  int offset_test[10][2] = {
        {0, 9},
        {5, 9},
        {10, 9},
        {15, 9},
        {20, 9},
        {25, 9},
        {30, 9},
        {35, 9},
        {40, 9}, /* <----- in the middle of the \r run, but run not counted */
        {45, 0},
  };
  int i;
  LRESULT result;

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);

  for (i = 0; i < 10; i++) {
    result = SendMessage(hwndRichEdit, EM_LINELENGTH, offset_test[i][0], 0);
    ok(result == offset_test[i][1], "Length of line at offset %d is %ld, expected %d\n",
        offset_test[i][0], result, offset_test[i][1]);
  }

  DestroyWindow(hwndRichEdit);
}

static void test_EM_GETTEXTRANGE(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    const char * text1 = "foo bar\r\nfoo bar";
    const char * text2 = "foo bar\rfoo bar";
    const char * expect1 = "bar\r\nfoo";
    const char * expect2 = "bar\rfoo";
    char buffer[1024] = {0};
    LRESULT result;
    TEXTRANGEA textRange;

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 12;
    result = SendMessage(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 8, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect1, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 11;
    result = SendMessage(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);

    ok(!strcmp(expect2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);


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

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    SendMessage(hwndRichEdit, EM_SETSEL, 4, 12);
    result = SendMessage(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 8, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect1, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    SendMessage(hwndRichEdit, EM_SETSEL, 4, 11);
    result = SendMessage(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);

    ok(!strcmp(expect2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);


    DestroyWindow(hwndRichEdit);
}

static const char haystack[] = "WINEWine wineWine wine WineWine";
                             /* ^0        ^10       ^20       ^30 */

struct find_s {
  int start;
  int end;
  const char *needle;
  int flags;
  int expected_loc;
  int _todo_wine;
};


struct find_s find_tests[] = {
  /* Find in empty text */
  {0, -1, "foo", FR_DOWN, -1, 0},
  {0, -1, "foo", 0, -1, 0},
  {0, -1, "", FR_DOWN, -1, 0},
  {20, 5, "foo", FR_DOWN, -1, 0},
  {5, 20, "foo", FR_DOWN, -1, 0}
};

struct find_s find_tests2[] = {
  /* No-result find */
  {0, -1, "foo", FR_DOWN | FR_MATCHCASE, -1, 0},
  {5, 20, "WINE", FR_DOWN | FR_MATCHCASE, -1, 0},

  /* Subsequent finds */
  {0, -1, "Wine", FR_DOWN | FR_MATCHCASE, 4, 0},
  {5, 31, "Wine", FR_DOWN | FR_MATCHCASE, 13, 0},
  {14, 31, "Wine", FR_DOWN | FR_MATCHCASE, 23, 0},
  {24, 31, "Wine", FR_DOWN | FR_MATCHCASE, 27, 0},

  /* Find backwards */
  {19, 20, "Wine", FR_MATCHCASE, -1, 0},
  {10, 20, "Wine", FR_MATCHCASE, 13, 0},
  {20, 10, "Wine", FR_MATCHCASE, -1, 0},

  /* Case-insensitive */
  {1, 31, "wInE", FR_DOWN, 4, 0},
  {1, 31, "Wine", FR_DOWN, 4, 0},

  /* High-to-low ranges */
  {20, 5, "Wine", FR_DOWN, -1, 0},
  {2, 1, "Wine", FR_DOWN, -1, 0},
  {30, 29, "Wine", FR_DOWN, -1, 0},
  {20, 5, "Wine", 0, /*13*/ -1, 0},

  /* Find nothing */
  {5, 10, "", FR_DOWN, -1, 0},
  {10, 5, "", FR_DOWN, -1, 0},
  {0, -1, "", FR_DOWN, -1, 0},
  {10, 5, "", 0, -1, 0},

  /* Whole-word search */
  {0, -1, "wine", FR_DOWN | FR_WHOLEWORD, 18, 0},
  {0, -1, "win", FR_DOWN | FR_WHOLEWORD, -1, 0},
  {13, -1, "wine", FR_DOWN | FR_WHOLEWORD, 18, 0},
  {0, -1, "winewine", FR_DOWN | FR_WHOLEWORD, 0, 0},
  {10, -1, "winewine", FR_DOWN | FR_WHOLEWORD, 23, 0},
  {11, -1, "winewine", FR_WHOLEWORD, 23, 0},
  {31, -1, "winewine", FR_WHOLEWORD, -1, 0},

  /* Bad ranges */
  {5, 200, "XXX", FR_DOWN, -1, 0},
  {-20, 20, "Wine", FR_DOWN, -1, 0},
  {-20, 20, "Wine", FR_DOWN, -1, 0},
  {-15, -20, "Wine", FR_DOWN, -1, 0},
  {1<<12, 1<<13, "Wine", FR_DOWN, -1, 0},

  /* Check the case noted in bug 4479 where matches at end aren't recognized */
  {23, 31, "Wine", FR_DOWN | FR_MATCHCASE, 23, 0},
  {27, 31, "Wine", FR_DOWN | FR_MATCHCASE, 27, 0},
  {27, 32, "Wine", FR_DOWN | FR_MATCHCASE, 27, 0},
  {13, 31, "WineWine", FR_DOWN | FR_MATCHCASE, 23, 0},
  {13, 32, "WineWine", FR_DOWN | FR_MATCHCASE, 23, 0},

  /* The backwards case of bug 4479; bounds look right
   * Fails because backward find is wrong */
  {19, 20, "WINE", FR_MATCHCASE, -1, 0},
  {0, 20, "WINE", FR_MATCHCASE, 0, 0},

  {0, -1, "wineWine wine", FR_DOWN, 0, 0},
  {0, -1, "wineWine wine", 0, 0, 0},
  {0, -1, "INEW", 0, 1, 0},
  {0, 31, "INEW", 0, 1, 0},
  {4, -1, "INEW", 0, 10, 0},
};

static void check_EM_FINDTEXT(HWND hwnd, const char *name, struct find_s *f, int id) {
  int findloc;
  FINDTEXT ft;
  memset(&ft, 0, sizeof(ft));
  ft.chrg.cpMin = f->start;
  ft.chrg.cpMax = f->end;
  ft.lpstrText = f->needle;
  findloc = SendMessage(hwnd, EM_FINDTEXT, f->flags, (LPARAM) &ft);
  ok(findloc == f->expected_loc,
     "EM_FINDTEXT(%s,%d) '%s' in range(%d,%d), flags %08x, got start at %d, expected %d\n",
     name, id, f->needle, f->start, f->end, f->flags, findloc, f->expected_loc);
}

static void check_EM_FINDTEXTEX(HWND hwnd, const char *name, struct find_s *f,
    int id) {
  int findloc;
  FINDTEXTEX ft;
  int expected_end_loc;

  memset(&ft, 0, sizeof(ft));
  ft.chrg.cpMin = f->start;
  ft.chrg.cpMax = f->end;
  ft.lpstrText = f->needle;
  findloc = SendMessage(hwnd, EM_FINDTEXTEX, f->flags, (LPARAM) &ft);
  ok(findloc == f->expected_loc,
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
      name, id, f->needle, f->start, f->end, f->flags, findloc);
  ok(ft.chrgText.cpMin == f->expected_loc,
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d, expected %d\n",
      name, id, f->needle, f->start, f->end, f->flags, ft.chrgText.cpMin, f->expected_loc);
  expected_end_loc = ((f->expected_loc == -1) ? -1
        : f->expected_loc + strlen(f->needle));
  ok(ft.chrgText.cpMax == expected_end_loc,
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, end at %d, expected %d\n",
      name, id, f->needle, f->start, f->end, f->flags, ft.chrgText.cpMax, expected_end_loc);
}

static void run_tests_EM_FINDTEXT(HWND hwnd, const char *name, struct find_s *find,
    int num_tests)
{
  int i;

  for (i = 0; i < num_tests; i++) {
    if (find[i]._todo_wine) {
      todo_wine {
        check_EM_FINDTEXT(hwnd, name, &find[i], i);
        check_EM_FINDTEXTEX(hwnd, name, &find[i], i);
      }
    } else {
        check_EM_FINDTEXT(hwnd, name, &find[i], i);
        check_EM_FINDTEXTEX(hwnd, name, &find[i], i);
    }
  }
}

static void test_EM_FINDTEXT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);

  /* Empty rich edit control */
  run_tests_EM_FINDTEXT(hwndRichEdit, "1", find_tests,
      sizeof(find_tests)/sizeof(struct find_s));

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) haystack);

  /* Haystack text */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2", find_tests2,
      sizeof(find_tests2)/sizeof(struct find_s));

  DestroyWindow(hwndRichEdit);
}



START_TEST( editor )
{
  MSG msg;
  time_t end;

  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED32.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibrary("RICHED32.DLL");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  test_WM_SETTEXT();
  test_EM_GETTEXTRANGE();
  test_EM_GETSELTEXT();
  test_WM_GETTEXTLENGTH();
  test_EM_STREAMIN();
  test_EM_STREAMOUT();
  test_EM_GETLINE();
  test_EM_LINELENGTH();
  test_EM_FINDTEXT();

  /* Set the environment variable WINETEST_RICHED32 to keep windows
   * responsive and open for 30 seconds. This is useful for debugging.
   *
   * The message pump uses PeekMessage() to empty the queue and then sleeps for
   * 50ms before retrying the queue. */
  end = time(NULL) + 30;
  if (getenv( "WINETEST_RICHED32" )) {
    while (time(NULL) < end) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      } else {
        Sleep(50);
      }
    }
  }

  OleFlushClipboard();
  ok(FreeLibrary(hmoduleRichEdit) != 0, "error: %d\n", (int) GetLastError());
}
