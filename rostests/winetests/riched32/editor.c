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
  char buf[1024] = {0};
  LRESULT result;

  /* This test attempts to show that WM_SETTEXT on a riched32 control does not
     attempt to modify the text that is pasted into the control, and should
     return it as is. In particular, \r\r\n is NOT converted, unlike riched20.
     Currently, builtin riched32 mangles solitary \r or \n when not part of
     a \r\n pair.
   */

#define TEST_SETTEXT(a, b, is_todo) \
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
  }

  TEST_SETTEXT(TestItem1, TestItem1, 0)
  TEST_SETTEXT(TestItem2, TestItem2, 1)
  TEST_SETTEXT(TestItem3, TestItem3, 1)
  TEST_SETTEXT(TestItem4, TestItem4, 1)
  TEST_SETTEXT(TestItem5, TestItem5, 1)
  TEST_SETTEXT(TestItem6, TestItem6, 1)
  TEST_SETTEXT(TestItem7, TestItem7, 1)
  TEST_SETTEXT(TestItem8, TestItem8, 0)
  TEST_SETTEXT(TestItem9, TestItem9, 0)
  TEST_SETTEXT(TestItem10, TestItem10, 0)
  TEST_SETTEXT(TestItem11, TestItem11, 0)
  TEST_SETTEXT(TestItem12, TestItem12, 0)
  TEST_SETTEXT(TestItem13, TestItem13, 0)

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

START_TEST( editor )
{
  MSG msg;
  time_t end;

  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED32.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibrary("RICHED32.DLL");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  test_WM_SETTEXT();
  test_WM_GETTEXTLENGTH();

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
