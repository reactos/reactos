/*
* Unit test suite for rich edit control
*
* Copyright 2006 Google (Thomas Kho)
* Copyright 2007 Matt Finnicum
* Copyright 2007 Dmitry Timoshkov
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

static CHAR string1[MAX_PATH], string2[MAX_PATH], string3[MAX_PATH];

#define ok_w3(format, szString1, szString2, szString3) \
    WideCharToMultiByte(CP_ACP, 0, szString1, -1, string1, MAX_PATH, NULL, NULL); \
    WideCharToMultiByte(CP_ACP, 0, szString2, -1, string2, MAX_PATH, NULL, NULL); \
    WideCharToMultiByte(CP_ACP, 0, szString3, -1, string3, MAX_PATH, NULL, NULL); \
    ok(!lstrcmpW(szString3, szString1) || !lstrcmpW(szString3, szString2), \
       format, string1, string2, string3);

static HMODULE hmoduleRichEdit;

static int is_win9x = 0;

static HWND new_window(LPCTSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindow(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 200, 60, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent) {
  return new_window(RICHEDIT_CLASS, ES_MULTILINE, parent);
}

/* Keeps the window reponsive for the deley_time in seconds.
 * This is useful for debugging a test to see what is happening. */
static void keep_responsive(time_t delay_time)
{
    MSG msg;
    time_t end;

    /* The message pump uses PeekMessage() to empty the queue and then
     * sleeps for 50ms before retrying the queue. */
    end = time(NULL) + delay_time;
    while (time(NULL) < end) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      } else {
        Sleep(50);
      }
    }
}

static void processPendingMessages(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void pressKeyWithModifier(HWND hwnd, BYTE mod_vk, BYTE vk)
{
    BYTE mod_scan_code = MapVirtualKey(mod_vk, MAPVK_VK_TO_VSC);
    BYTE scan_code = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    SetFocus(hwnd);
    keybd_event(mod_vk, mod_scan_code, 0, 0);
    keybd_event(vk, scan_code, 0, 0);
    keybd_event(vk, scan_code, KEYEVENTF_KEYUP, 0);
    keybd_event(mod_vk, mod_scan_code, KEYEVENTF_KEYUP, 0);
    processPendingMessages();
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

static BOOL hold_key(int vk)
{
  BYTE key_state[256];
  BOOL result;

  result = GetKeyboardState(key_state);
  ok(result, "GetKeyboardState failed.\n");
  if (!result) return FALSE;
  key_state[vk] |= 0x80;
  result = SetKeyboardState(key_state);
  ok(result, "SetKeyboardState failed.\n");
  return result != 0;
}

static BOOL release_key(int vk)
{
  BYTE key_state[256];
  BOOL result;

  result = GetKeyboardState(key_state);
  ok(result, "GetKeyboardState failed.\n");
  if (!result) return FALSE;
  key_state[vk] &= ~0x80;
  result = SetKeyboardState(key_state);
  ok(result, "SetKeyboardState failed.\n");
  return result != 0;
}

static const char haystack[] = "WINEWine wineWine wine WineWine";
                             /* ^0        ^10       ^20       ^30 */

struct find_s {
  int start;
  int end;
  const char *needle;
  int flags;
  int expected_loc;
};


struct find_s find_tests[] = {
  /* Find in empty text */
  {0, -1, "foo", FR_DOWN, -1},
  {0, -1, "foo", 0, -1},
  {0, -1, "", FR_DOWN, -1},
  {20, 5, "foo", FR_DOWN, -1},
  {5, 20, "foo", FR_DOWN, -1}
};

struct find_s find_tests2[] = {
  /* No-result find */
  {0, -1, "foo", FR_DOWN | FR_MATCHCASE, -1},
  {5, 20, "WINE", FR_DOWN | FR_MATCHCASE, -1},

  /* Subsequent finds */
  {0, -1, "Wine", FR_DOWN | FR_MATCHCASE, 4},
  {5, 31, "Wine", FR_DOWN | FR_MATCHCASE, 13},
  {14, 31, "Wine", FR_DOWN | FR_MATCHCASE, 23},
  {24, 31, "Wine", FR_DOWN | FR_MATCHCASE, 27},

  /* Find backwards */
  {19, 20, "Wine", FR_MATCHCASE, 13},
  {10, 20, "Wine", FR_MATCHCASE, 4},
  {20, 10, "Wine", FR_MATCHCASE, 13},

  /* Case-insensitive */
  {1, 31, "wInE", FR_DOWN, 4},
  {1, 31, "Wine", FR_DOWN, 4},

  /* High-to-low ranges */
  {20, 5, "Wine", FR_DOWN, -1},
  {2, 1, "Wine", FR_DOWN, -1},
  {30, 29, "Wine", FR_DOWN, -1},
  {20, 5, "Wine", 0, 13},

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
  {11, -1, "winewine", FR_WHOLEWORD, 0},
  {31, -1, "winewine", FR_WHOLEWORD, 23},
  
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
  {19, 20, "WINE", FR_MATCHCASE, 0},
  {0, 20, "WINE", FR_MATCHCASE, -1},

  {0, -1, "wineWine wine", 0, -1},
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
      "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
      name, id, f->needle, f->start, f->end, f->flags, ft.chrgText.cpMin);
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
      check_EM_FINDTEXT(hwnd, name, &find[i], i);
      check_EM_FINDTEXTEX(hwnd, name, &find[i], i);
  }
}

static void test_EM_FINDTEXT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2 cf2;

  /* Empty rich edit control */
  run_tests_EM_FINDTEXT(hwndRichEdit, "1", find_tests,
      sizeof(find_tests)/sizeof(struct find_s));

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) haystack);

  /* Haystack text */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2", find_tests2,
      sizeof(find_tests2)/sizeof(struct find_s));

  /* Setting a format on an arbitrary range should have no effect in search
     results. This tests correct offset reporting across runs. */
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);
  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;
  SendMessage(hwndRichEdit, EM_SETSEL, 6, 20);
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  /* Haystack text, again */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2-bis", find_tests2,
      sizeof(find_tests2)/sizeof(struct find_s));

  /* Yet another range */
  cf2.dwMask = CFM_BOLD | cf2.dwMask;
  cf2.dwEffects = CFE_BOLD ^ cf2.dwEffects;
  SendMessage(hwndRichEdit, EM_SETSEL, 11, 15);
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  /* Haystack text, again */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2-bisbis", find_tests2,
      sizeof(find_tests2)/sizeof(struct find_s));

  DestroyWindow(hwndRichEdit);
}

static const struct getline_s {
  int line;
  size_t buffer_len;
  const char *text;
} gl[] = {
  {0, 10, "foo bar\r"},
  {1, 10, "\r"},
  {2, 10, "bar\r"},
  {3, 10, "\r"},

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
  const char text[] = "foo bar\n"
      "\n"
      "bar\n";

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);

  memset(origdest, 0xBB, nBuf);
  for (i = 0; i < sizeof(gl)/sizeof(struct getline_s); i++)
  {
    int nCopied;
    int expected_nCopied = min(gl[i].buffer_len, strlen(gl[i].text));
    int expected_bytes_written = min(gl[i].buffer_len, strlen(gl[i].text));
    memset(dest, 0xBB, nBuf);
    *(WORD *) dest = gl[i].buffer_len;

    /* EM_GETLINE appends a "\r\0" to the end of the line
     * nCopied counts up to and including the '\r' */
    nCopied = SendMessage(hwndRichEdit, EM_GETLINE, gl[i].line, (LPARAM) dest);
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
      /* Prepare hex strings of buffers to dump on failure. */
      char expectedbuf[1024];
      char resultbuf[1024];
      int j;
      resultbuf[0] = '\0';
      for (j = 0; j < 32; j++)
        sprintf(resultbuf+strlen(resultbuf), "%02x", dest[j] & 0xFF);
      expectedbuf[0] = '\0';
      for (j = 0; j < expected_bytes_written; j++) /* Written bytes */
        sprintf(expectedbuf+strlen(expectedbuf), "%02x", gl[i].text[j] & 0xFF);
      for (; j < gl[i].buffer_len; j++) /* Ignored bytes */
        sprintf(expectedbuf+strlen(expectedbuf), "??");
      for (; j < 32; j++) /* Bytes after declared buffer size */
        sprintf(expectedbuf+strlen(expectedbuf), "%02x", origdest[j] & 0xFF);

      /* Test the part of the buffer that is expected to be written according
       * to the MSDN documentation fo EM_GETLINE, which does not state that
       * a NULL terminating character will be added unless no text is copied.
       *
       * Windows 95, 98 & NT do not append a NULL terminating character, but
       * Windows 2000 and up do append a NULL terminating character if there
       * is space in the buffer. The test will ignore this difference. */
      ok(!strncmp(dest, gl[i].text, expected_bytes_written),
         "%d: expected_bytes_written=%d\n" "expected=0x%s\n" "but got= 0x%s\n",
         i, expected_bytes_written, expectedbuf, resultbuf);
      /* Test the part of the buffer after the declared length to make sure
       * there are no buffer overruns. */
      ok(!strncmp(dest + gl[i].buffer_len, origdest + gl[i].buffer_len,
                  nBuf - gl[i].buffer_len),
         "%d: expected_bytes_written=%d\n" "expected=0x%s\n" "but got= 0x%s\n",
         i, expected_bytes_written, expectedbuf, resultbuf);
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
        "richedit1";
  int offset_test[10][2] = {
        {0, 9},
        {5, 9},
        {10, 9},
        {15, 9},
        {20, 9},
        {25, 9},
        {30, 9},
        {35, 9},
        {40, 0},
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

static int get_scroll_pos_y(HWND hwnd)
{
  POINT p = {-1, -1};
  SendMessage(hwnd, EM_GETSCROLLPOS, 0, (LPARAM) &p);
  ok(p.x != -1 && p.y != -1, "p.x:%d p.y:%d\n", p.x, p.y);
  return p.y;
}

static void move_cursor(HWND hwnd, long charindex)
{
  CHARRANGE cr;
  cr.cpMax = charindex;
  cr.cpMin = charindex;
  SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) &cr);
}

static void line_scroll(HWND hwnd, int amount)
{
  SendMessage(hwnd, EM_LINESCROLL, 0, amount);
}

static void test_EM_SCROLLCARET(void)
{
  int prevY, curY;
  const char text[] = "aa\n"
      "this is a long line of text that should be longer than the "
      "control's width\n"
      "cc\n"
      "dd\n"
      "ee\n"
      "ff\n"
      "gg\n"
      "hh\n";
  /* The richedit window height needs to be large enough vertically to fit in
   * more than two lines of text, so the new_richedit function can't be used
   * since a height of 60 was not large enough on some systems.
   */
  HWND hwndRichEdit = CreateWindow(RICHEDIT_CLASS, NULL,
                                   ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                                   0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
  ok(hwndRichEdit != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());

  /* Can't verify this */
  SendMessage(hwndRichEdit, EM_SCROLLCARET, 0, 0);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);

  /* Caret above visible window */
  line_scroll(hwndRichEdit, 3);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessage(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY != curY, "%d == %d\n", prevY, curY);

  /* Caret below visible window */
  move_cursor(hwndRichEdit, sizeof(text) - 1);
  line_scroll(hwndRichEdit, -3);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessage(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY != curY, "%d == %d\n", prevY, curY);

  /* Caret in visible window */
  move_cursor(hwndRichEdit, sizeof(text) - 2);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessage(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY == curY, "%d != %d\n", prevY, curY);

  /* Caret still in visible window */
  line_scroll(hwndRichEdit, -1);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessage(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY == curY, "%d != %d\n", prevY, curY);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_POSFROMCHAR(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  int i;
  LRESULT result;
  unsigned int height = 0;
  int xpos = 0;
  POINTL pt;
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
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 0);
    SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"0123456789ABCDE\n");
  }

  /*
   Richedit 1.0 receives a POINTL* on wParam and character offset on lParam, returns void.
   Richedit 2.0 receives character offset on wParam, ignores lParam, returns MAKELONG(x,y)
   Richedit 3.0 accepts either of the above API conventions.
   */

  /* Testing Richedit 2.0 API format */

  /* Testing start of lines. X-offset should be constant on all cases (native is 1).
     Since all lines are identical and drawn with the same font,
     they should have the same height... right?
   */
  for (i = 0; i < 50; i++)
  {
    /* All the lines are 16 characters long */
    result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, i * 16, 0);
    if (i == 0)
    {
      ok(HIWORD(result) == 0, "EM_POSFROMCHAR reports y=%d, expected 0\n", HIWORD(result));
      ok(LOWORD(result) == 1, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
      xpos = LOWORD(result);
    }
    else if (i == 1)
    {
      ok(HIWORD(result) > 0, "EM_POSFROMCHAR reports y=%d, expected > 0\n", HIWORD(result));
      ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
      height = HIWORD(result);
    }
    else
    {
      ok(HIWORD(result) == i * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), i * height);
      ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
    }
  }

  /* Testing position at end of text */
  result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, 50 * 16, 0);
  ok(HIWORD(result) == 50 * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), 50 * height);
  ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));

  /* Testing position way past end of text */
  result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, 55 * 16, 0);
  ok(HIWORD(result) == 50 * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), 50 * height);
  ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));

  /* Testing that vertical scrolling does, in fact, have an effect on EM_POSFROMCHAR */
  SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0); /* line down */
  for (i = 0; i < 50; i++)
  {
    /* All the lines are 16 characters long */
    result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, i * 16, 0);
    ok((signed short)(HIWORD(result)) == (i - 1) * height,
        "EM_POSFROMCHAR reports y=%hd, expected %d\n",
        (signed short)(HIWORD(result)), (i - 1) * height);
    ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
  }

  /* Testing position at end of text */
  result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, 50 * 16, 0);
  ok(HIWORD(result) == (50 - 1) * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), (50 - 1) * height);
  ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));

  /* Testing position way past end of text */
  result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, 55 * 16, 0);
  ok(HIWORD(result) == (50 - 1) * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), (50 - 1) * height);
  ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));

  /* Testing that horizontal scrolling does, in fact, have an effect on EM_POSFROMCHAR */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
  SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0); /* line up */

  result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, 0, 0);
  ok(HIWORD(result) == 0, "EM_POSFROMCHAR reports y=%d, expected 0\n", HIWORD(result));
  ok(LOWORD(result) == 1, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
  xpos = LOWORD(result);

  SendMessage(hwndRichEdit, WM_HSCROLL, SB_LINERIGHT, 0);
  result = SendMessage(hwndRichEdit, EM_POSFROMCHAR, 0, 0);
  ok(HIWORD(result) == 0, "EM_POSFROMCHAR reports y=%d, expected 0\n", HIWORD(result));
  ok((signed short)(LOWORD(result)) < xpos,
        "EM_POSFROMCHAR reports x=%hd, expected value less than %d\n",
        (signed short)(LOWORD(result)), xpos);
  SendMessage(hwndRichEdit, WM_HSCROLL, SB_LINELEFT, 0);

  /* Test around end of text that doesn't end in a newline. */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "12345678901234");
  SendMessage(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt,
              SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0)-1);
  ok(pt.x > 1, "pt.x = %d\n", pt.x);
  xpos = pt.x;
  SendMessage(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt,
              SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0));
  ok(pt.x > xpos, "pt.x = %d\n", pt.x);
  xpos = pt.x;
  SendMessage(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt,
              SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0)+1);
  ok(pt.x == xpos, "pt.x = %d\n", pt.x);

  /* Try a negative position. */
  SendMessage(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt, -1);
  ok(pt.x == 1, "pt.x = %d\n", pt.x);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_SETCHARFORMAT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2 cf2;
  int rc = 0;
  int tested_effects[] = {
    CFE_BOLD,
    CFE_ITALIC,
    CFE_UNDERLINE,
    CFE_STRIKEOUT,
    CFE_PROTECTED,
    CFE_LINK,
    CFE_SUBSCRIPT,
    CFE_SUPERSCRIPT,
    0
  };
  int i;
  CHARRANGE cr;

  /* Invalid flags, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) 0xfffffff0,
             (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION,
             (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_WORD,
             (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL,
             (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* Invalid flags, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) 0xfffffff0,
             (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == FALSE, "Should not be able to undo here.\n");
  SendMessage(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == FALSE, "Should not be able to undo here.\n");
  SendMessage(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION,
             (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == FALSE, "Should not be able to undo here.\n");
  SendMessage(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_WORD,
             (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  todo_wine ok(rc == TRUE, "Should not be able to undo here.\n");
  SendMessage(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL,
             (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  todo_wine ok(rc == TRUE, "Should not be able to undo here.\n");
  SendMessage(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);

  /* Test state of modify flag before and after valid EM_SETCHARFORMAT */
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);
  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;

  /* wParam==0 is default char format, does not set modify */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, 0, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");

  /* wParam==SCF_SELECTION sets modify if nonempty selection */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == -1, "Text not marked as modified, expected modified! (%d)\n", rc);

  /* wParam==SCF_ALL sets modify regardless of whether text is present */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == -1, "Text not marked as modified, expected modified! (%d)\n", rc);

  DestroyWindow(hwndRichEdit);

  /* EM_GETCHARFORMAT tests */
  for (i = 0; tested_effects[i]; i++)
  {
    hwndRichEdit = new_richedit(NULL);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

    /* Need to set a TrueType font to get consistent CFM_BOLD results */
    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    cf2.dwMask = CFM_FACE|CFM_WEIGHT;
    cf2.dwEffects = 0;
    strcpy(cf2.szFaceName, "Courier New");
    cf2.wWeight = FW_DONTCARE;
    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 4);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == 0,
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x clear\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    cf2.dwMask = tested_effects[i];
    if (cf2.dwMask == CFE_SUBSCRIPT || cf2.dwMask == CFE_SUPERSCRIPT)
      cf2.dwMask = CFM_SUPERSCRIPT;
    cf2.dwEffects = tested_effects[i];
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == tested_effects[i],
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 2, 4);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == 0,
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x clear\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 1, 3);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == 0)
          ||
          (cf2.dwMask & tested_effects[i]) == 0),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x clear\n", i, cf2.dwMask, tested_effects[i]);

    DestroyWindow(hwndRichEdit);
  }

  for (i = 0; tested_effects[i]; i++)
  {
    hwndRichEdit = new_richedit(NULL);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

    /* Need to set a TrueType font to get consistent CFM_BOLD results */
    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    cf2.dwMask = CFM_FACE|CFM_WEIGHT;
    cf2.dwEffects = 0;
    strcpy(cf2.szFaceName, "Courier New");
    cf2.wWeight = FW_DONTCARE;
    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    cf2.dwMask = tested_effects[i];
    if (cf2.dwMask == CFE_SUBSCRIPT || cf2.dwMask == CFE_SUPERSCRIPT)
      cf2.dwMask = CFM_SUPERSCRIPT;
    cf2.dwEffects = tested_effects[i];
    SendMessage(hwndRichEdit, EM_SETSEL, 2, 4);
    SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == 0,
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x clear\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 2, 4);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == tested_effects[i],
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2));
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichEdit, EM_SETSEL, 1, 3);
    SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == 0)
          ||
          (cf2.dwMask & tested_effects[i]) == 0),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x clear\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == tested_effects[i],
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x set\n", i, cf2.dwEffects, tested_effects[i]);

    DestroyWindow(hwndRichEdit);
  }

  /* Effects applied on an empty selection should take effect when selection is
     replaced with text */
  hwndRichEdit = new_richedit(NULL);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  /* Selection is now nonempty */
  SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  ok (((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == CFE_BOLD,
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);


  /* Set two effects on an empty selection */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
  cf2.dwMask = CFM_ITALIC;
  cf2.dwEffects = CFE_ITALIC;
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  /* Selection is now nonempty */
  SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  ok (((cf2.dwMask & (CFM_BOLD|CFM_ITALIC)) == (CFM_BOLD|CFM_ITALIC)),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, (CFM_BOLD|CFM_ITALIC));
  ok((cf2.dwEffects & (CFE_BOLD|CFE_ITALIC)) == (CFE_BOLD|CFE_ITALIC),
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, (CFE_BOLD|CFE_ITALIC));

  /* Setting the (empty) selection to exactly the same place as before should
     NOT clear the insertion style! */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  /* Empty selection in same place, insert style should NOT be forgotten here. */
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 2);

  /* Selection is now nonempty */
  SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  ok (((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == CFE_BOLD,
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);

  /* Ditto with EM_EXSETSEL */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  cr.cpMin = 2; cr.cpMax = 2;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  /* Empty selection in same place, insert style should NOT be forgotten here. */
  cr.cpMin = 2; cr.cpMax = 2;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr); /* Empty selection */

  /* Selection is now nonempty */
  SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2));
  cf2.cbSize = sizeof(CHARFORMAT2);
  cr.cpMin = 2; cr.cpMax = 6;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr); /* Empty selection */
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);

  ok (((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == CFE_BOLD,
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_SETTEXTMODE(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2 cf2, cf2test;
  CHARRANGE cr;
  int rc = 0;

  /*Test that EM_SETTEXTMODE fails if text exists within the control*/
  /*Insert text into the control*/

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "wine");

  /*Attempt to change the control to plain text mode*/
  rc = SendMessage(hwndRichEdit, EM_SETTEXTMODE, (WPARAM) TM_PLAINTEXT, 0);
  ok(rc != 0, "EM_SETTEXTMODE: changed text mode in control containing text - returned: %d\n", rc);

  /*Test that EM_SETTEXTMODE does not allow rich edit text to be pasted.
  If rich text is pasted, it should have the same formatting as the rest
  of the text in the control*/

  /*Italicize the text
  *NOTE: If the default text was already italicized, the test will simply
  reverse; in other words, it will copy a regular "wine" into a plain
  text window that uses an italicized format*/
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);

  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;

  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");

  /*EM_SETCHARFORMAT is not yet fully implemented for all WPARAMs in wine;
  however, SCF_ALL has been implemented*/
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  rc = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == -1, "Text not marked as modified, expected modified! (%d)\n", rc);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "wine");

  /*Select the string "wine"*/
  cr.cpMin = 0;
  cr.cpMax = 4;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /*Copy the italicized "wine" to the clipboard*/
  SendMessage(hwndRichEdit, WM_COPY, 0, 0);

  /*Reset the formatting to default*/
  cf2.dwEffects = CFE_ITALIC^cf2.dwEffects;
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  /*Clear the text in the control*/
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "");

  /*Switch to Plain Text Mode*/
  rc = SendMessage(hwndRichEdit, EM_SETTEXTMODE, (WPARAM) TM_PLAINTEXT, 0);
  ok(rc == 0, "EM_SETTEXTMODE: unable to switch to plain text mode with empty control:  returned: %d\n", rc);

  /*Input "wine" again in normal format*/
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "wine");

  /*Paste the italicized "wine" into the control*/
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);

  /*Select a character from the first "wine" string*/
  cr.cpMin = 2;
  cr.cpMax = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /*Retrieve its formatting*/
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION,
              (LPARAM) &cf2);

  /*Select a character from the second "wine" string*/
  cr.cpMin = 5;
  cr.cpMax = 6;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /*Retrieve its formatting*/
  cf2test.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION,
               (LPARAM) &cf2test);

  /*Compare the two formattings*/
    ok((cf2.dwMask == cf2test.dwMask) && (cf2.dwEffects == cf2test.dwEffects),
      "two formats found in plain text mode - cf2.dwEffects: %x cf2test.dwEffects: %x\n",
       cf2.dwEffects, cf2test.dwEffects);
  /*Test TM_RICHTEXT by: switching back to Rich Text mode
                         printing "wine" in the current format(normal)
                         pasting "wine" from the clipboard(italicized)
                         comparing the two formats(should differ)*/

  /*Attempt to switch with text in control*/
  rc = SendMessage(hwndRichEdit, EM_SETTEXTMODE, (WPARAM) TM_RICHTEXT, 0);
  ok(rc != 0, "EM_SETTEXTMODE: changed from plain text to rich text with text in control - returned: %d\n", rc);

  /*Clear control*/
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "");

  /*Switch into Rich Text mode*/
  rc = SendMessage(hwndRichEdit, EM_SETTEXTMODE, (WPARAM) TM_RICHTEXT, 0);
  ok(rc == 0, "EM_SETTEXTMODE: unable to change to rich text with empty control - returned: %d\n", rc);

  /*Print "wine" in normal formatting into the control*/
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "wine");

  /*Paste italicized "wine" into the control*/
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);

  /*Select text from the first "wine" string*/
  cr.cpMin = 1;
  cr.cpMax = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /*Retrieve its formatting*/
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION,
                (LPARAM) &cf2);

  /*Select text from the second "wine" string*/
  cr.cpMin = 6;
  cr.cpMax = 7;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /*Retrieve its formatting*/
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION,
                (LPARAM) &cf2test);

  /*Test that the two formattings are not the same*/
  todo_wine ok((cf2.dwMask == cf2test.dwMask) && (cf2.dwEffects != cf2test.dwEffects),
      "expected different formats - cf2.dwMask: %x, cf2test.dwMask: %x, cf2.dwEffects: %x, cf2test.dwEffects: %x\n",
      cf2.dwMask, cf2test.dwMask, cf2.dwEffects, cf2test.dwEffects);

  DestroyWindow(hwndRichEdit);
}

static void test_SETPARAFORMAT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  PARAFORMAT2 fmt;
  HRESULT ret;
  LONG expectedMask = PFM_ALL2 & ~PFM_TABLEROWDELIMITER;
  fmt.cbSize = sizeof(PARAFORMAT2);
  fmt.dwMask = PFM_ALIGNMENT;
  fmt.wAlignment = PFA_LEFT;

  ret = SendMessage(hwndRichEdit, EM_SETPARAFORMAT, 0, (LPARAM) &fmt);
  ok(ret != 0, "expected non-zero got %d\n", ret);

  fmt.cbSize = sizeof(PARAFORMAT2);
  fmt.dwMask = -1;
  ret = SendMessage(hwndRichEdit, EM_GETPARAFORMAT, 0, (LPARAM) &fmt);
  /* Ignore the PFM_TABLEROWDELIMITER bit because it changes
   * between richedit different native builds of riched20.dll
   * used on different Windows versions. */
  ret &= ~PFM_TABLEROWDELIMITER;
  fmt.dwMask &= ~PFM_TABLEROWDELIMITER;

  ok(ret == expectedMask, "expected %x got %x\n", expectedMask, ret);
  ok(fmt.dwMask == expectedMask, "expected %x got %x\n", expectedMask, fmt.dwMask);

  DestroyWindow(hwndRichEdit);
}

static void test_TM_PLAINTEXT(void)
{
  /*Tests plain text properties*/

  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2 cf2, cf2test;
  CHARRANGE cr;
  int rc = 0;

  /*Switch to plain text mode*/

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "");
  SendMessage(hwndRichEdit, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

  /*Fill control with text*/

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "Is Wine an emulator? No it's not");

  /*Select some text and bold it*/

  cr.cpMin = 10;
  cr.cpMax = 20;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
	      (LPARAM) &cf2);

  cf2.dwMask = CFM_BOLD | cf2.dwMask;
  cf2.dwEffects = CFE_BOLD ^ cf2.dwEffects;

  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_WORD | SCF_SELECTION, (LPARAM) &cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  /*Get the formatting of those characters*/

  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf2);

  /*Get the formatting of some other characters*/
  cf2test.cbSize = sizeof(CHARFORMAT2);
  cr.cpMin = 21;
  cr.cpMax = 30;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf2test);

  /*Test that they are the same as plain text allows only one formatting*/

  ok((cf2.dwMask == cf2test.dwMask) && (cf2.dwEffects == cf2test.dwEffects),
     "two selections' formats differ - cf2.dwMask: %x, cf2test.dwMask %x, cf2.dwEffects: %x, cf2test.dwEffects: %x\n",
     cf2.dwMask, cf2test.dwMask, cf2.dwEffects, cf2test.dwEffects);
  
  /*Fill the control with a "wine" string, which when inserted will be bold*/

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "wine");

  /*Copy the bolded "wine" string*/

  cr.cpMin = 0;
  cr.cpMax = 4;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  SendMessage(hwndRichEdit, WM_COPY, 0, 0);

  /*Swap back to rich text*/

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "");
  SendMessage(hwndRichEdit, EM_SETTEXTMODE, (WPARAM) TM_RICHTEXT, 0);

  /*Set the default formatting to bold italics*/

  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT, (LPARAM) &cf2);
  cf2.dwMask |= CFM_ITALIC;
  cf2.dwEffects ^= CFE_ITALIC;
  rc = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  /*Set the text in the control to "wine", which will be bold and italicized*/

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "wine");

  /*Paste the plain text "wine" string, which should take the insert
   formatting, which at the moment is bold italics*/

  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);

  /*Select the first "wine" string and retrieve its formatting*/

  cr.cpMin = 1;
  cr.cpMax = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf2);

  /*Select the second "wine" string and retrieve its formatting*/

  cr.cpMin = 5;
  cr.cpMax = 7;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf2test);

  /*Compare the two formattings. They should be the same.*/

  ok((cf2.dwMask == cf2test.dwMask) && (cf2.dwEffects == cf2test.dwEffects),
     "Copied text retained formatting - cf2.dwMask: %x, cf2test.dwMask: %x, cf2.dwEffects: %x, cf2test.dwEffects: %x\n",
     cf2.dwMask, cf2test.dwMask, cf2.dwEffects, cf2test.dwEffects);
  DestroyWindow(hwndRichEdit);
}

static void test_WM_GETTEXT(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    static const char text[] = "Hello. My name is RichEdit!";
    static const char text2[] = "Hello. My name is RichEdit!\r";
    static const char text2_after[] = "Hello. My name is RichEdit!\r\n";
    char buffer[1024] = {0};
    int result;

    /* Baseline test with normal-sized buffer */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
    result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(result == lstrlen(buffer),
        "WM_GETTEXT returned %d, expected %d\n", result, lstrlen(buffer));
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,text);
    ok(result == 0, 
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    /* Test for returned value of WM_GETTEXTLENGTH */
    result = SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlen(text),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlen(text));

    /* Test for behavior in overflow case */
    memset(buffer, 0, 1024);
    result = SendMessage(hwndRichEdit, WM_GETTEXT, strlen(text), (LPARAM)buffer);
    ok(result == 0 ||
       result == lstrlenA(text) - 1, /* XP, win2k3 */
        "WM_GETTEXT returned %d, expected 0 or %d\n", result, lstrlenA(text) - 1);
    result = strcmp(buffer,text);
    if (result)
        result = strncmp(buffer, text, lstrlenA(text) - 1); /* XP, win2k3 */
    ok(result == 0,
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    /* Baseline test with normal-sized buffer and carriage return */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text2);
    result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(result == lstrlen(buffer),
        "WM_GETTEXT returned %d, expected %d\n", result, lstrlen(buffer));
    result = strcmp(buffer,text2_after);
    ok(result == 0,
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    /* Test for returned value of WM_GETTEXTLENGTH */
    result = SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlen(text2_after),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlen(text2_after));

    /* Test for behavior of CRLF conversion in case of overflow */
    memset(buffer, 0, 1024);
    result = SendMessage(hwndRichEdit, WM_GETTEXT, strlen(text2), (LPARAM)buffer);
    ok(result == 0 ||
       result == lstrlenA(text2) - 1, /* XP, win2k3 */
        "WM_GETTEXT returned %d, expected 0 or %d\n", result, lstrlenA(text2) - 1);
    result = strcmp(buffer,text2);
    if (result)
        result = strncmp(buffer, text2, lstrlenA(text2) - 1); /* XP, win2k3 */
    ok(result == 0,
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    DestroyWindow(hwndRichEdit);
}

static void test_EM_GETTEXTRANGE(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    const char * text1 = "foo bar\r\nfoo bar";
    const char * text2 = "foo bar\rfoo bar";
    const char * expect = "bar\rfoo";
    char buffer[1024] = {0};
    LRESULT result;
    TEXTRANGEA textRange;

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 11;
    result = SendMessage(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 11;
    result = SendMessage(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    DestroyWindow(hwndRichEdit);
}

static void test_EM_GETSELTEXT(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    const char * text1 = "foo bar\r\nfoo bar";
    const char * text2 = "foo bar\rfoo bar";
    const char * expect = "bar\rfoo";
    char buffer[1024] = {0};
    LRESULT result;

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    SendMessage(hwndRichEdit, EM_SETSEL, 4, 11);
    result = SendMessage(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    SendMessage(hwndRichEdit, EM_SETSEL, 4, 11);
    result = SendMessage(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    DestroyWindow(hwndRichEdit);
}

/* FIXME: need to test unimplemented options and robustly test wparam */
static void test_EM_SETOPTIONS(void)
{
    HWND hwndRichEdit;
    static const char text[] = "Hello. My name is RichEdit!";
    char buffer[1024] = {0};
    DWORD dwStyle, options, oldOptions;
    DWORD optionStyles = ES_AUTOVSCROLL|ES_AUTOHSCROLL|ES_NOHIDESEL|
                         ES_READONLY|ES_WANTRETURN|ES_SAVESEL|
                         ES_SELECTIONBAR|ES_VERTICAL;

    /* Test initial options. */
    hwndRichEdit = CreateWindow(RICHEDIT_CLASS, NULL, WS_POPUP,
                                0, 0, 200, 60, NULL, NULL,
                                hmoduleRichEdit, NULL);
    ok(hwndRichEdit != NULL, "class: %s, error: %d\n",
       RICHEDIT_CLASS, (int) GetLastError());
    options = SendMessage(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    ok(options == 0, "Incorrect initial options %x\n", options);
    DestroyWindow(hwndRichEdit);

    hwndRichEdit = CreateWindow(RICHEDIT_CLASS, NULL,
                                WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                                0, 0, 200, 60, NULL, NULL,
                                hmoduleRichEdit, NULL);
    ok(hwndRichEdit != NULL, "class: %s, error: %d\n",
       RICHEDIT_CLASS, (int) GetLastError());
    options = SendMessage(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    /* WS_[VH]SCROLL cause the ECO_AUTO[VH]SCROLL options to be set */
    ok(options == (ECO_AUTOVSCROLL|ECO_AUTOHSCROLL),
       "Incorrect initial options %x\n", options);

    /* NEGATIVE TESTING - NO OPTIONS SET */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
    SendMessage(hwndRichEdit, EM_SETOPTIONS, ECOOP_SET, 0);

    /* testing no readonly by sending 'a' to the control*/
    SetFocus(hwndRichEdit);
    SendMessage(hwndRichEdit, WM_CHAR, 'a', 0x1E0001);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(buffer[0]=='a', 
       "EM_SETOPTIONS: Text not changed! s1:%s s2:%s\n", text, buffer);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);

    /* READONLY - sending 'a' to the control */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
    SendMessage(hwndRichEdit, EM_SETOPTIONS, ECOOP_SET, ECO_READONLY);
    SetFocus(hwndRichEdit);
    SendMessage(hwndRichEdit, WM_CHAR, 'a', 0x1E0001);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(buffer[0]==text[0], 
       "EM_SETOPTIONS: Text changed! s1:%s s2:%s\n", text, buffer); 

    /* EM_SETOPTIONS changes the window style, but changing the
     * window style does not change the options. */
    dwStyle = GetWindowLong(hwndRichEdit, GWL_STYLE);
    ok(dwStyle & ES_READONLY, "Readonly style not set by EM_SETOPTIONS\n");
    SetWindowLong(hwndRichEdit, GWL_STYLE, dwStyle & ~ES_READONLY);
    options = SendMessage(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    ok(options & ES_READONLY, "Readonly option set by SetWindowLong\n");
    /* Confirm that the text is still read only. */
    SendMessage(hwndRichEdit, WM_CHAR, 'a', ('a' << 16) | 0x0001);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(buffer[0]==text[0],
       "EM_SETOPTIONS: Text changed! s1:%s s2:%s\n", text, buffer);

    oldOptions = options;
    SetWindowLong(hwndRichEdit, GWL_STYLE, dwStyle|optionStyles);
    options = SendMessage(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    ok(options == oldOptions,
       "Options set by SetWindowLong (%x -> %x)\n", oldOptions, options);

    DestroyWindow(hwndRichEdit);
}

static int check_CFE_LINK_selection(HWND hwnd, int sel_start, int sel_end)
{
  CHARFORMAT2W text_format;
  text_format.cbSize = sizeof(text_format);
  SendMessage(hwnd, EM_SETSEL, sel_start, sel_end);
  SendMessage(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM) &text_format);
  return (text_format.dwEffects & CFE_LINK) ? 1 : 0;
}

static void check_CFE_LINK_rcvd(HWND hwnd, int is_url, const char * url)
{
  int link_present = 0;

  link_present = check_CFE_LINK_selection(hwnd, 0, 1);
  if (is_url) 
  { /* control text is url; should get CFE_LINK */
	ok(0 != link_present, "URL Case: CFE_LINK not set for [%s].\n", url);
  }
  else 
  {
    ok(0 == link_present, "Non-URL Case: CFE_LINK set for [%s].\n", url);
  }
}

static HWND new_static_wnd(HWND parent) {
  return new_window("Static", 0, parent);
}

static void test_EM_AUTOURLDETECT(void)
{
  /* DO NOT change the properties of the first two elements. To shorten the
     tests, all tests after WM_SETTEXT test just the first two elements -
     one non-URL and one URL */
  struct urls_s {
    const char *text;
    int is_url;
  } urls[12] = {
    {"winehq.org", 0},
    {"http://www.winehq.org", 1},
    {"http//winehq.org", 0},
    {"ww.winehq.org", 0},
    {"www.winehq.org", 1},
    {"ftp://192.168.1.1", 1},
    {"ftp//192.168.1.1", 0},
    {"mailto:your@email.com", 1},    
    {"prospero:prosperoserver", 1},
    {"telnet:test", 1},
    {"news:newserver", 1},
    {"wais:waisserver", 1}  
  };

  int i, j;
  int urlRet=-1;
  HWND hwndRichEdit, parent;

  /* All of the following should cause the URL to be detected  */
  const char * templates_delim[] = {
    "This is some text with X on it",
    "This is some text with (X) on it",
    "This is some text with X\r on it",
    "This is some text with ---X--- on it",
    "This is some text with \"X\" on it",
    "This is some text with 'X' on it",
    "This is some text with 'X' on it",
    "This is some text with :X: on it",

    "This text ends with X",

    "This is some text with X) on it",
    "This is some text with X--- on it",
    "This is some text with X\" on it",
    "This is some text with X' on it",
    "This is some text with X: on it",

    "This is some text with (X on it",
    "This is some text with \rX on it",
    "This is some text with ---X on it",
    "This is some text with \"X on it",
    "This is some text with 'X on it",
    "This is some text with :X on it",
  };
  /* None of these should cause the URL to be detected */
  const char * templates_non_delim[] = {
    "This is some text with |X| on it",
    "This is some text with *X* on it",
    "This is some text with /X/ on it",
    "This is some text with +X+ on it",
    "This is some text with %X% on it",
    "This is some text with #X# on it",
    "This is some text with @X@ on it",
    "This is some text with \\X\\ on it",
    "This is some text with |X on it",
    "This is some text with *X on it",
    "This is some text with /X on it",
    "This is some text with +X on it",
    "This is some text with %X on it",
    "This is some text with #X on it",
    "This is some text with @X on it",
    "This is some text with \\X on it",
  };
  /* All of these cause the URL detection to be extended by one more byte,
     thus demonstrating that the tested character is considered as part
     of the URL. */
  const char * templates_xten_delim[] = {
    "This is some text with X| on it",
    "This is some text with X* on it",
    "This is some text with X/ on it",
    "This is some text with X+ on it",
    "This is some text with X% on it",
    "This is some text with X# on it",
    "This is some text with X@ on it",
    "This is some text with X\\ on it",
  };
  char buffer[1024];

  parent = new_static_wnd(NULL);
  hwndRichEdit = new_richedit(parent);
  /* Try and pass EM_AUTOURLDETECT some test wParam values */
  urlRet=SendMessage(hwndRichEdit, EM_AUTOURLDETECT, FALSE, 0);
  ok(urlRet==0, "Good wParam: urlRet is: %d\n", urlRet);
  urlRet=SendMessage(hwndRichEdit, EM_AUTOURLDETECT, 1, 0);
  ok(urlRet==0, "Good wParam2: urlRet is: %d\n", urlRet);
  /* Windows returns -2147024809 (0x80070057) on bad wParam values */
  urlRet=SendMessage(hwndRichEdit, EM_AUTOURLDETECT, 8, 0);
  ok(urlRet==E_INVALIDARG, "Bad wParam: urlRet is: %d\n", urlRet);
  urlRet=SendMessage(hwndRichEdit, EM_AUTOURLDETECT, (WPARAM)"h", (LPARAM)"h");
  ok(urlRet==E_INVALIDARG, "Bad wParam2: urlRet is: %d\n", urlRet);
  /* for each url, check the text to see if CFE_LINK effect is present */
  for (i = 0; i < sizeof(urls)/sizeof(struct urls_s); i++) {

    SendMessage(hwndRichEdit, EM_AUTOURLDETECT, FALSE, 0);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) urls[i].text);
    check_CFE_LINK_rcvd(hwndRichEdit, 0, urls[i].text);

    /* Link detection should happen immediately upon WM_SETTEXT */
    SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) urls[i].text);
    check_CFE_LINK_rcvd(hwndRichEdit, urls[i].is_url, urls[i].text);
  }
  DestroyWindow(hwndRichEdit);

  /* Test detection of URLs within normal text - WM_SETTEXT case. */
  for (i = 0; i < sizeof(urls)/sizeof(struct urls_s); i++) {
    hwndRichEdit = new_richedit(parent);

    for (j = 0; j < sizeof(templates_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      strncpy(buffer, templates_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    for (j = 0; j < sizeof(templates_non_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_non_delim[j], 'X');
      at_offset = at_pos - templates_non_delim[j];
      strncpy(buffer, templates_non_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_non_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    for (j = 0; j < sizeof(templates_xten_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_xten_delim[j], 'X');
      at_offset = at_pos - templates_xten_delim[j];
      strncpy(buffer, templates_xten_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_xten_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset, end_offset +1, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset +1, buffer);
      }
      if (buffer[end_offset +1] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset + 2, buffer);
        if (buffer[end_offset +2] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +2, end_offset +3),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +2, end_offset +3, buffer);
        }
      }
    }

    DestroyWindow(hwndRichEdit);
    hwndRichEdit = NULL;
  }

  /* Test detection of URLs within normal text - WM_CHAR case. */
  /* Test only the first two URL examples for brevity */
  for (i = 0; i < 2; i++) {
    hwndRichEdit = new_richedit(parent);

    /* Also for brevity, test only the first three delimiters */
    for (j = 0; j < 3; j++) {
      char * at_pos;
      int at_offset;
      int end_offset;
      int u, v;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
      for (u = 0; templates_delim[j][u]; u++) {
        if (templates_delim[j][u] == '\r') {
          simulate_typing_characters(hwndRichEdit, "\r");
        } else if (templates_delim[j][u] != 'X') {
          SendMessage(hwndRichEdit, WM_CHAR, templates_delim[j][u], 1);
        } else {
          for (v = 0; urls[i].text[v]; v++) {
            SendMessage(hwndRichEdit, WM_CHAR, urls[i].text[v], 1);
          }
        }
      }
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }

      /* The following will insert a paragraph break after the first character
         of the URL candidate, thus breaking the URL. It is expected that the
         CFE_LINK attribute should break across both pieces of the URL */
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset+1, at_offset+1);
      simulate_typing_characters(hwndRichEdit, "\r");
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
      /* end_offset moved because of paragraph break */
      ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset+1, buffer);
      ok(buffer[end_offset], "buffer \"%s\" ended prematurely. Is it missing a newline character?\n", buffer);
      if (buffer[end_offset] != 0  && buffer[end_offset+1] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset+1, end_offset +2),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset+1, end_offset +2, buffer);
        if (buffer[end_offset +2] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +2, end_offset +3),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +2, end_offset +3, buffer);
        }
      }

      /* The following will remove the just-inserted paragraph break, thus
         restoring the URL */
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset+2, at_offset+2);
      simulate_typing_characters(hwndRichEdit, "\b");
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }
    DestroyWindow(hwndRichEdit);
    hwndRichEdit = NULL;
  }

  /* Test detection of URLs within normal text - EM_SETTEXTEX case. */
  /* Test just the first two URL examples for brevity */
  for (i = 0; i < 2; i++) {
    SETTEXTEX st;

    hwndRichEdit = new_richedit(parent);

    /* There are at least three ways in which EM_SETTEXTEX must cause URLs to
       be detected:
       1) Set entire text, a la WM_SETTEXT
       2) Set a selection of the text to the URL
       3) Set a portion of the text at a time, which eventually results in
          an URL
       All of them should give equivalent results
     */

    /* Set entire text in one go, like WM_SETTEXT */
    for (j = 0; j < sizeof(templates_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      st.codepage = CP_ACP;
      st.flags = ST_DEFAULT;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      strncpy(buffer, templates_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM) buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    /* Set selection with X to the URL */
    for (j = 0; j < sizeof(templates_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      st.codepage = CP_ACP;
      st.flags = ST_DEFAULT;
      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM) templates_delim[j]);
      st.flags = ST_SELECTION;
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM) urls[i].text);
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    /* Set selection with X to the first character of the URL, then the rest */
    for (j = 0; j < sizeof(templates_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      strcpy(buffer, "YY");
      buffer[0] = urls[i].text[0];

      st.codepage = CP_ACP;
      st.flags = ST_DEFAULT;
      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM) templates_delim[j]);
      st.flags = ST_SELECTION;
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM) buffer);
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset+1, at_offset+2);
      SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)(urls[i].text + 1));
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    DestroyWindow(hwndRichEdit);
    hwndRichEdit = NULL;
  }

  /* Test detection of URLs within normal text - EM_REPLACESEL case. */
  /* Test just the first two URL examples for brevity */
  for (i = 0; i < 2; i++) {
    hwndRichEdit = new_richedit(parent);

    /* Set selection with X to the URL */
    for (j = 0; j < sizeof(templates_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) templates_delim[j]);
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) urls[i].text);
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    /* Set selection with X to the first character of the URL, then the rest */
    for (j = 0; j < sizeof(templates_delim) / sizeof(const char *); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      strcpy(buffer, "YY");
      buffer[0] = urls[i].text[0];

      SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) templates_delim[j]);
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) buffer);
      SendMessage(hwndRichEdit, EM_SETSEL, at_offset+1, at_offset+2);
      SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)(urls[i].text + 1));
      SendMessage(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
        "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      if (urls[i].is_url)
      {
        ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
        ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      else
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset, at_offset + 1, buffer);
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      }
      if (buffer[end_offset] != '\0')
      {
        ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
          "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset, end_offset + 1, buffer);
        if (buffer[end_offset +1] != '\0')
        {
          ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset +1, end_offset +2),
            "CFE_LINK incorrectly set in (%d-%d), text: %s\n", end_offset +1, end_offset +2, buffer);
        }
      }
    }

    DestroyWindow(hwndRichEdit);
    hwndRichEdit = NULL;
  }

  DestroyWindow(parent);
}

static void test_EM_SCROLL(void)
{
  int i, j;
  int r; /* return value */
  int expr; /* expected return value */
  HWND hwndRichEdit = new_richedit(NULL);
  int y_before, y_after; /* units of lines of text */

  /* test a richedit box containing a single line of text */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "a");/* one line of text */
  expr = 0x00010000;
  for (i = 0; i < 4; i++) {
    static const int cmd[4] = { SB_PAGEDOWN, SB_PAGEUP, SB_LINEDOWN, SB_LINEUP };

    r = SendMessage(hwndRichEdit, EM_SCROLL, cmd[i], 0);
    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    ok(expr == r, "EM_SCROLL improper return value returned (i == %d). "
       "Got 0x%08x, expected 0x%08x\n", i, r, expr);
    ok(y_after == 0, "EM_SCROLL improper scroll. scrolled to line %d, not 1 "
       "(i == %d)\n", y_after, i);
  }

  /*
   * test a richedit box that will scroll. There are two general
   * cases: the case without any long lines and the case with a long
   * line.
   */
  for (i = 0; i < 2; i++) { /* iterate through different bodies of text */
    if (i == 0)
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "a\nb\nc\nd\ne");
    else
      SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)
                  "a LONG LINE LONG LINE LONG LINE LONG LINE LONG LINE "
                  "LONG LINE LONG LINE LONG LINE LONG LINE LONG LINE "
                  "LONG LINE \nb\nc\nd\ne");
    for (j = 0; j < 12; j++) /* reset scroll position to top */
      SendMessage(hwndRichEdit, EM_SCROLL, SB_PAGEUP, 0);

    /* get first visible line */
    y_before = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    r = SendMessage(hwndRichEdit, EM_SCROLL, SB_PAGEDOWN, 0); /* page down */

    /* get new current first visible line */
    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(((r & 0xffffff00) == 0x00010000) &&
       ((r & 0x000000ff) != 0x00000000),
       "EM_SCROLL page down didn't scroll by a small positive number of "
       "lines (r == 0x%08x)\n", r);
    ok(y_after > y_before, "EM_SCROLL page down not functioning "
       "(line %d scrolled to line %d\n", y_before, y_after);

    y_before = y_after;
    
    r = SendMessage(hwndRichEdit, EM_SCROLL, SB_PAGEUP, 0); /* page up */
    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    ok(((r & 0xffffff00) == 0x0001ff00),
       "EM_SCROLL page up didn't scroll by a small negative number of lines "
       "(r == 0x%08x)\n", r);
    ok(y_after < y_before, "EM_SCROLL page up not functioning (line "
       "%d scrolled to line %d\n", y_before, y_after);
    
    y_before = y_after;

    r = SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0); /* line down */

    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010001, "EM_SCROLL line down didn't scroll by one line "
       "(r == 0x%08x)\n", r);
    ok(y_after -1 == y_before, "EM_SCROLL line down didn't go down by "
       "1 line (%d scrolled to %d)\n", y_before, y_after);

    y_before = y_after;

    r = SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0); /* line up */

    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x0001ffff, "EM_SCROLL line up didn't scroll by one line "
       "(r == 0x%08x)\n", r);
    ok(y_after +1 == y_before, "EM_SCROLL line up didn't go up by 1 "
       "line (%d scrolled to %d)\n", y_before, y_after);

    y_before = y_after;

    r = SendMessage(hwndRichEdit, EM_SCROLL,
                    SB_LINEUP, 0); /* lineup beyond top */

    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL line up returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL line up beyond top worked (%d)\n", y_after);

    y_before = y_after;

    r = SendMessage(hwndRichEdit, EM_SCROLL,
                    SB_PAGEUP, 0);/*page up beyond top */

    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL page up returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL page up beyond top worked (%d)\n", y_after);

    for (j = 0; j < 12; j++) /* page down all the way to the bottom */
      SendMessage(hwndRichEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    y_before = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    r = SendMessage(hwndRichEdit, EM_SCROLL,
                    SB_PAGEDOWN, 0); /* page down beyond bot */
    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL page down returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL page down beyond bottom worked (%d -> %d)\n",
       y_before, y_after);

    y_before = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    SendMessage(hwndRichEdit, EM_SCROLL,
                SB_LINEDOWN, 0); /* line down beyond bot */
    y_after = SendMessage(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    
    ok(r == 0x00010000,
       "EM_SCROLL line down returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL line down beyond bottom worked (%d -> %d)\n",
       y_before, y_after);
  }
  DestroyWindow(hwndRichEdit);
}

unsigned int recursionLevel = 0;
unsigned int WM_SIZE_recursionLevel = 0;
BOOL bailedOutOfRecursion = FALSE;
LRESULT (WINAPI *richeditProc)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static LRESULT WINAPI RicheditStupidOverrideProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT r;

    if (bailedOutOfRecursion) return 0;
    if (recursionLevel >= 32) {
        bailedOutOfRecursion = TRUE;
        return 0;
    }

    recursionLevel++;
    switch (message) {
    case WM_SIZE:
        WM_SIZE_recursionLevel++;
        r = richeditProc(hwnd, message, wParam, lParam);
        /* Because, uhhhh... I never heard of ES_DISABLENOSCROLL */
        ShowScrollBar(hwnd, SB_VERT, TRUE);
        WM_SIZE_recursionLevel--;
        break;
    default:
        r = richeditProc(hwnd, message, wParam, lParam);
        break;
    }
    recursionLevel--;
    return r;
}

static void test_scrollbar_visibility(void)
{
  HWND hwndRichEdit;
  const char * text="a\na\na\na\na\na\na\na\na\na\na\na\na\na\na\na\na\na\na\na\n";
  SCROLLINFO si;
  WNDCLASSA cls;
  BOOL r;

  /* These tests show that richedit should temporarily refrain from automatically
     hiding or showing its scrollbars (vertical at least) when an explicit request
     is made via ShowScrollBar() or similar, outside of standard richedit logic.
     Some applications depend on forced showing (when otherwise richedit would
     hide the vertical scrollbar) and are thrown on an endless recursive loop
     if richedit auto-hides the scrollbar again. Apparently they never heard of
     the ES_DISABLENOSCROLL style... */

  hwndRichEdit = new_richedit(NULL);

  /* Test default scrollbar visibility behavior */
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  /* Oddly, setting text to NULL does *not* reset the scrollbar range,
     even though it hides the scrollbar */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  /* Setting non-scrolling text again does *not* reset scrollbar range */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  DestroyWindow(hwndRichEdit);

  /* Test again, with ES_DISABLENOSCROLL style */
  hwndRichEdit = new_window(RICHEDIT_CLASS, ES_MULTILINE|ES_DISABLENOSCROLL, NULL);

  /* Test default scrollbar visibility behavior */
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 1,
        "reported page/range is %d (%d..%d) expected 0 (0..1)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 1,
        "reported page/range is %d (%d..%d) expected 0 (0..1)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Oddly, setting text to NULL does *not* reset the scrollbar range */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  /* Setting non-scrolling text again does *not* reset scrollbar range */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  DestroyWindow(hwndRichEdit);

  /* Test behavior with explicit visibility request, using ShowScrollBar() */
  hwndRichEdit = new_richedit(NULL);

  /* Previously failed because builtin incorrectly re-hides scrollbar forced visible */
  ShowScrollBar(hwndRichEdit, SB_VERT, TRUE);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  todo_wine {
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 100,
        "reported page/range is %d (%d..%d) expected 0 (0..100)\n",
        si.nPage, si.nMin, si.nMax);
  }

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  todo_wine {
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 100,
        "reported page/range is %d (%d..%d) expected 0 (0..100)\n",
        si.nPage, si.nMin, si.nMax);
  }

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  todo_wine {
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 100,
        "reported page/range is %d (%d..%d) expected 0 (0..100)\n",
        si.nPage, si.nMin, si.nMax);
  }

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a\na");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  todo_wine {
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 100,
        "reported page/range is %d (%d..%d) expected 0 (0..100)\n",
        si.nPage, si.nMin, si.nMax);
  }

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  todo_wine {
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 100,
        "reported page/range is %d (%d..%d) expected 0 (0..100)\n",
        si.nPage, si.nMin, si.nMax);
  }

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  DestroyWindow(hwndRichEdit);

  hwndRichEdit = new_richedit(NULL);

  ShowScrollBar(hwndRichEdit, SB_VERT, FALSE);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Previously, builtin incorrectly re-shows explicitly hidden scrollbar */
  ShowScrollBar(hwndRichEdit, SB_VERT, FALSE);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Testing effect of EM_SCROLL on scrollbar visibility. It seems that
     EM_SCROLL will make visible any forcefully invisible scrollbar */
  SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  ShowScrollBar(hwndRichEdit, SB_VERT, FALSE);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Again, EM_SCROLL, with SB_LINEUP */
  SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  DestroyWindow(hwndRichEdit);


  /* Test behavior with explicit visibility request, using SetWindowLong()() */
  hwndRichEdit = new_richedit(NULL);

#define ENABLE_WS_VSCROLL(hwnd) \
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) | WS_VSCROLL)
#define DISABLE_WS_VSCROLL(hwnd) \
    SetWindowLongA(hwnd, GWL_STYLE, GetWindowLongA(hwnd, GWL_STYLE) & ~WS_VSCROLL)

  /* Previously failed because builtin incorrectly re-hides scrollbar forced visible */
  ENABLE_WS_VSCROLL(hwndRichEdit);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a\na");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  /* Ditto, see above */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  DestroyWindow(hwndRichEdit);

  hwndRichEdit = new_richedit(NULL);

  DISABLE_WS_VSCROLL(hwndRichEdit);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Previously, builtin incorrectly re-shows explicitly hidden scrollbar */
  DISABLE_WS_VSCROLL(hwndRichEdit);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  DISABLE_WS_VSCROLL(hwndRichEdit);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Testing effect of EM_SCROLL on scrollbar visibility. It seems that
     EM_SCROLL will make visible any forcefully invisible scrollbar */
  SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  DISABLE_WS_VSCROLL(hwndRichEdit);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  /* Again, EM_SCROLL, with SB_LINEUP */
  SendMessage(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  DestroyWindow(hwndRichEdit);

  /* This window proc models what is going on with Corman Lisp 3.0.
     At WM_SIZE, this proc unconditionally calls ShowScrollBar() to
     force the scrollbar into visibility. Recursion should NOT happen
     as a result of this action.
   */
  r = GetClassInfoA(NULL, RICHEDIT_CLASS, &cls);
  if (r) {
    richeditProc = cls.lpfnWndProc;
    cls.lpfnWndProc = RicheditStupidOverrideProcA;
    cls.lpszClassName = "RicheditStupidOverride";
    if(!RegisterClassA(&cls)) assert(0);

    recursionLevel = 0;
    WM_SIZE_recursionLevel = 0;
    bailedOutOfRecursion = FALSE;
    hwndRichEdit = new_window(cls.lpszClassName, ES_MULTILINE, NULL);
    ok(!bailedOutOfRecursion,
        "WM_SIZE/scrollbar mutual recursion detected, expected none!\n");

    recursionLevel = 0;
    WM_SIZE_recursionLevel = 0;
    bailedOutOfRecursion = FALSE;
    MoveWindow(hwndRichEdit, 0, 0, 250, 100, TRUE);
    ok(!bailedOutOfRecursion,
        "WM_SIZE/scrollbar mutual recursion detected, expected none!\n");

    /* Unblock window in order to process WM_DESTROY */
    recursionLevel = 0;
    bailedOutOfRecursion = FALSE;
    WM_SIZE_recursionLevel = 0;
    DestroyWindow(hwndRichEdit);
  }
}

static void test_EM_SETUNDOLIMIT(void)
{
  /* cases we test for:
   * default behaviour - limiting at 100 undo's 
   * undo disabled - setting a limit of 0
   * undo limited -  undo limit set to some to some number, like 2
   * bad input - sending a negative number should default to 100 undo's */
 
  HWND hwndRichEdit = new_richedit(NULL);
  CHARRANGE cr;
  int i;
  int result;
  
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "x");
  cr.cpMin = 0;
  cr.cpMax = 1;
  SendMessage(hwndRichEdit, WM_COPY, 0, 0);
    /*Load "x" into the clipboard. Paste is an easy, undo'able operation.
      also, multiple pastes don't combine like WM_CHAR would */
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /* first case - check the default */
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0); 
  for (i=0; i<101; i++) /* Put 101 undo's on the stack */
    SendMessage(hwndRichEdit, WM_PASTE, 0, 0); 
  for (i=0; i<100; i++) /* Undo 100 of them */
    SendMessage(hwndRichEdit, WM_UNDO, 0, 0); 
  ok(!SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT allowed more than a hundred undo's by default.\n");

  /* second case - cannot undo */
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0, 0); 
  SendMessage(hwndRichEdit, EM_SETUNDOLIMIT, 0, 0); 
  SendMessage(hwndRichEdit,
              WM_PASTE, 0, 0); /* Try to put something in the undo stack */
  ok(!SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT allowed undo with UNDOLIMIT set to 0\n");

  /* third case - set it to an arbitrary number */
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0, 0); 
  SendMessage(hwndRichEdit, EM_SETUNDOLIMIT, 2, 0); 
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0); 
  /* If SETUNDOLIMIT is working, there should only be two undo's after this */
  ok(SendMessage(hwndRichEdit, EM_CANUNDO, 0,0),
     "EM_SETUNDOLIMIT didn't allow the first undo with UNDOLIMIT set to 2\n");
  SendMessage(hwndRichEdit, WM_UNDO, 0, 0);
  ok(SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT didn't allow a second undo with UNDOLIMIT set to 2\n");
  SendMessage(hwndRichEdit, WM_UNDO, 0, 0); 
  ok(!SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT allowed a third undo with UNDOLIMIT set to 2\n");
  
  /* fourth case - setting negative numbers should default to 100 undos */
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0); 
  result = SendMessage(hwndRichEdit, EM_SETUNDOLIMIT, -1, 0);
  ok (result == 100, 
      "EM_SETUNDOLIMIT returned %d when set to -1, instead of 100\n",result);
      
  DestroyWindow(hwndRichEdit);
}

static void test_ES_PASSWORD(void)
{
  /* This isn't hugely testable, so we're just going to run it through its paces */

  HWND hwndRichEdit = new_richedit(NULL);
  WCHAR result;

  /* First, check the default of a regular control */
  result = SendMessage(hwndRichEdit, EM_GETPASSWORDCHAR, 0, 0);
  ok (result == 0,
	"EM_GETPASSWORDCHAR returned %c by default, instead of NULL\n",result);

  /* Now, set it to something normal */
  SendMessage(hwndRichEdit, EM_SETPASSWORDCHAR, 'x', 0);
  result = SendMessage(hwndRichEdit, EM_GETPASSWORDCHAR, 0, 0);
  ok (result == 120,
	"EM_GETPASSWORDCHAR returned %c (%d) when set to 'x', instead of x (120)\n",result,result);

  /* Now, set it to something odd */
  SendMessage(hwndRichEdit, EM_SETPASSWORDCHAR, (WCHAR)1234, 0);
  result = SendMessage(hwndRichEdit, EM_GETPASSWORDCHAR, 0, 0);
  ok (result == 1234,
	"EM_GETPASSWORDCHAR returned %c (%d) when set to 'x', instead of x (120)\n",result,result);
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

static void test_WM_SETTEXT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  const char * TestItem1 = "TestSomeText";
  const char * TestItem2 = "TestSomeText\r";
  const char * TestItem2_after = "TestSomeText\r\n";
  const char * TestItem3 = "TestSomeText\rSomeMoreText\r";
  const char * TestItem3_after = "TestSomeText\r\nSomeMoreText\r\n";
  const char * TestItem4 = "TestSomeText\n\nTestSomeText";
  const char * TestItem4_after = "TestSomeText\r\n\r\nTestSomeText";
  const char * TestItem5 = "TestSomeText\r\r\nTestSomeText";
  const char * TestItem5_after = "TestSomeText TestSomeText";
  const char * TestItem6 = "TestSomeText\r\r\n\rTestSomeText";
  const char * TestItem6_after = "TestSomeText \r\nTestSomeText";
  const char * TestItem7 = "TestSomeText\r\n\r\r\n\rTestSomeText";
  const char * TestItem7_after = "TestSomeText\r\n \r\nTestSomeText";

  char buf[1024] = {0};
  LRESULT result;
  EDITSTREAM es;
  char * p;

  /* This test attempts to show that WM_SETTEXT on a riched20 control causes
     any solitary \r to be converted to \r\n on return. Properly paired
     \r\n are not affected. It also shows that the special sequence \r\r\n
     gets converted to a single space.
   */

#define TEST_SETTEXT(a, b) \
  result = SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) a); \
  ok (result == 1, "WM_SETTEXT returned %ld instead of 1\n", result); \
  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buf); \
  ok (result == lstrlen(buf), \
	"WM_GETTEXT returned %ld instead of expected %u\n", \
	result, lstrlen(buf)); \
  result = strcmp(b, buf); \
  ok(result == 0, \
        "WM_SETTEXT round trip: strcmp = %ld\n", result);

  TEST_SETTEXT(TestItem1, TestItem1)
  TEST_SETTEXT(TestItem2, TestItem2_after)
  TEST_SETTEXT(TestItem3, TestItem3_after)
  TEST_SETTEXT(TestItem3_after, TestItem3_after)
  TEST_SETTEXT(TestItem4, TestItem4_after)
  TEST_SETTEXT(TestItem5, TestItem5_after)
  TEST_SETTEXT(TestItem6, TestItem6_after)
  TEST_SETTEXT(TestItem7, TestItem7_after)

  /* The following test demonstrates that WM_SETTEXT supports RTF strings */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) TestItem1);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessage(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_RTF), (LPARAM)&es);
  trace("EM_STREAMOUT produced: \n%s\n", buf);
  TEST_SETTEXT(buf, TestItem1)

#undef TEST_SETTEXT
  DestroyWindow(hwndRichEdit);
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
  /* Here again, \r gets converted to \r\n, like WM_GETTEXT */
  ok(r == 14, "streamed text length is %d, expecting 14\n", r);
  ok(strcmp(buf, TestItem3) == 0,
        "streamed text different from, got %s\n", buf);
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

static void test_EM_SETTEXTEX(void)
{
  HWND hwndRichEdit, parent;
  SCROLLINFO si;
  int sel_start, sel_end;
  SETTEXTEX setText;
  GETTEXTEX getText;
  WCHAR TestItem1[] = {'T', 'e', 's', 't', 
                       'S', 'o', 'm', 'e', 
                       'T', 'e', 'x', 't', 0}; 
  WCHAR TestItem1alt[] = {'T', 'T', 'e', 's',
                          't', 'S', 'o', 'm',
                          'e', 'T', 'e', 'x',
                          't', 't', 'S', 'o',
                          'm', 'e', 'T', 'e',
                          'x', 't', 0};
  WCHAR TestItem1altn[] = {'T','T','e','s','t','S','o','m','e','T','e','x','t',
                           '\r','t','S','o','m','e','T','e','x','t',0};
  WCHAR TestItem2[] = {'T', 'e', 's', 't',
                       'S', 'o', 'm', 'e',
                       'T', 'e', 'x', 't',
                      '\r', 0};
  const char * TestItem2_after = "TestSomeText\r\n";
  WCHAR TestItem3[] = {'T', 'e', 's', 't',
                       'S', 'o', 'm', 'e',
                       'T', 'e', 'x', 't',
                      '\r','\n','\r','\n', 0};
  WCHAR TestItem3alt[] = {'T', 'e', 's', 't',
                       'S', 'o', 'm', 'e',
                       'T', 'e', 'x', 't',
                       '\n','\n', 0};
  WCHAR TestItem3_after[] = {'T', 'e', 's', 't',
                       'S', 'o', 'm', 'e',
                       'T', 'e', 'x', 't',
                       '\r','\r', 0};
  WCHAR TestItem4[] = {'T', 'e', 's', 't',
                       'S', 'o', 'm', 'e',
                       'T', 'e', 'x', 't',
                      '\r','\r','\n','\r',
                      '\n', 0};
  WCHAR TestItem4_after[] = {'T', 'e', 's', 't',
                       'S', 'o', 'm', 'e',
                       'T', 'e', 'x', 't',
                       ' ','\r', 0};
#define MAX_BUF_LEN 1024
  WCHAR buf[MAX_BUF_LEN];
  char bufACP[MAX_BUF_LEN];
  char * p;
  int result;
  CHARRANGE cr;
  EDITSTREAM es;
  WNDCLASSA cls;

  /* Test the scroll position with and without a parent window.
   *
   * For some reason the scroll position is 0 after EM_SETTEXTEX
   * with the ST_SELECTION flag only when the control has a parent
   * window, even though the selection is at the end. */
  cls.style = 0;
  cls.lpfnWndProc = DefWindowProcA;
  cls.cbClsExtra = 0;
  cls.cbWndExtra = 0;
  cls.hInstance = GetModuleHandleA(0);
  cls.hIcon = 0;
  cls.hCursor = LoadCursorA(0, IDC_ARROW);
  cls.hbrBackground = GetStockObject(WHITE_BRUSH);
  cls.lpszMenuName = NULL;
  cls.lpszClassName = "ParentTestClass";
  if(!RegisterClassA(&cls)) assert(0);

  parent = CreateWindow(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                        0, 0, 200, 60, NULL, NULL, NULL, NULL);
  ok (parent != 0, "Failed to create parent window\n");

  hwndRichEdit = CreateWindowEx(0,
                        RICHEDIT_CLASS, NULL,
                        ES_MULTILINE|WS_VSCROLL|WS_VISIBLE|WS_CHILD,
                        0, 0, 200, 60, parent, NULL,
                        hmoduleRichEdit, NULL);

  setText.codepage = CP_ACP;
  setText.flags = ST_SELECTION;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText,
              (LPARAM)"{\\rtf 1\\par 2\\par 3\\par 4\\par 5\\par 6\\par 7\\par 8\\par 9\\par}");
  si.cbSize = sizeof(si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  todo_wine ok(si.nPos == 0, "Position is incorrectly at %d\n", si.nPos);
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  ok(sel_start == 18, "Selection start incorrectly at %d\n", sel_start);
  ok(sel_end == 18, "Selection end incorrectly at %d\n", sel_end);

  DestroyWindow(parent);

  /* Test without a parent window */
  hwndRichEdit = new_richedit(NULL);
  setText.codepage = CP_ACP;
  setText.flags = ST_SELECTION;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText,
              (LPARAM)"{\\rtf 1\\par 2\\par 3\\par 4\\par 5\\par 6\\par 7\\par 8\\par 9\\par}");
  si.cbSize = sizeof(si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok(si.nPos != 0, "Position is incorrectly at %d\n", si.nPos);
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  ok(sel_start == 18, "Selection start incorrectly at %d\n", sel_start);
  ok(sel_end == 18, "Selection end incorrectly at %d\n", sel_end);

  /* The scroll position should also be 0 after EM_SETTEXTEX with ST_DEFAULT,
   * but this time it is because the selection is at the beginning. */
  setText.codepage = CP_ACP;
  setText.flags = ST_DEFAULT;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText,
              (LPARAM)"{\\rtf 1\\par 2\\par 3\\par 4\\par 5\\par 6\\par 7\\par 8\\par 9\\par}");
  si.cbSize = sizeof(si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok(si.nPos == 0, "Position is incorrectly at %d\n", si.nPos);
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  ok(sel_start == 0, "Selection start incorrectly at %d\n", sel_start);
  ok(sel_end == 0, "Selection end incorrectly at %d\n", sel_end);

  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem1);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem1) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX\n");

  /* Unlike WM_SETTEXT/WM_GETTEXT pair, EM_SETTEXTEX/EM_GETTEXTEX does not
     convert \r to \r\n on return: !ST_SELECTION && Unicode && !\rtf
   */
  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem2);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem2) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX\n");

  /* However, WM_GETTEXT *does* see \r\n where EM_GETTEXTEX would see \r */
  SendMessage(hwndRichEdit, WM_GETTEXT, MAX_BUF_LEN, (LPARAM)buf);
  ok(strcmp((const char *)buf, TestItem2_after) == 0,
      "WM_GETTEXT did *not* see \\r converted to \\r\\n pairs.\n");

  /* Baseline test for just-enough buffer space for string */
  getText.cb = (lstrlenW(TestItem2) + 1) * sizeof(WCHAR);
  getText.codepage = 1200;  /* no constant for unicode */
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  memset(buf, 0, MAX_BUF_LEN);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem2) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX\n");

  /* When there is enough space for one character, but not both, of the CRLF
     pair at the end of the string, the CR is not copied at all. That is,
     the caller must not see CRLF pairs truncated to CR at the end of the
     string.
   */
  getText.cb = (lstrlenW(TestItem2) + 1) * sizeof(WCHAR);
  getText.codepage = 1200;  /* no constant for unicode */
  getText.flags = GT_USECRLF;   /* <-- asking for CR -> CRLF conversion */
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  memset(buf, 0, MAX_BUF_LEN);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem1) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX\n");


  /* \r\n pairs get changed into \r: !ST_SELECTION && Unicode && !\rtf */
  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem3);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem3_after) == 0,
      "EM_SETTEXTEX did not convert properly\n");

  /* \n also gets changed to \r: !ST_SELECTION && Unicode && !\rtf */
  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem3alt);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem3_after) == 0,
      "EM_SETTEXTEX did not convert properly\n");

  /* \r\r\n gets changed into single space: !ST_SELECTION && Unicode && !\rtf */
  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem4);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem4_after) == 0,
      "EM_SETTEXTEX did not convert properly\n");

  /* !ST_SELECTION && Unicode && !\rtf */
  result = SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, 0);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  
  ok (result == 1, 
      "EM_SETTEXTEX returned %d, instead of 1\n",result);
  ok(lstrlenW(buf) == 0,
      "EM_SETTEXTEX with NULL lParam should clear rich edit.\n");
  
  /* put some text back: !ST_SELECTION && Unicode && !\rtf */
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem1);
  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  /* replace current selection: ST_SELECTION && Unicode && !\rtf */
  setText.flags = ST_SELECTION;
  result = SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, 0);
  ok(result == 0,
      "EM_SETTEXTEX with NULL lParam to replace selection"
      " with no text should return 0. Got %i\n",
      result);
  
  /* put some text back: !ST_SELECTION && Unicode && !\rtf */
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem1);
  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);
  /* replace current selection: ST_SELECTION && Unicode && !\rtf */
  setText.flags = ST_SELECTION;
  result = SendMessage(hwndRichEdit, EM_SETTEXTEX,
                       (WPARAM)&setText, (LPARAM) TestItem1);
  /* get text */
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(result == lstrlenW(TestItem1),
      "EM_SETTEXTEX with NULL lParam to replace selection"
      " with no text should return 0. Got %i\n",
      result);
  ok(lstrlenW(buf) == 22,
      "EM_SETTEXTEX to replace selection with more text failed: %i.\n",
      lstrlenW(buf) );

  /* The following test demonstrates that EM_SETTEXTEX supports RTF strings */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "TestSomeText"); /* TestItem1 */
  p = (char *)buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessage(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_RTF), (LPARAM)&es);
  trace("EM_STREAMOUT produced: \n%s\n", (char *)buf);

  /* !ST_SELECTION && !Unicode && \rtf */
  setText.codepage = CP_ACP;/* EM_STREAMOUT saved as ANSI string */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) buf);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem1) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX\n");

  /* The following test demonstrates that EM_SETTEXTEX treats text as ASCII if it
   * starts with ASCII characters "{\rtf" even when the codepage is unicode. */
  setText.codepage = 1200; /* Lie about code page (actual ASCII) */
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = ST_SELECTION;
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);
  result = SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) "{\\rtf not unicode}");
  todo_wine ok(result == 11, "EM_SETTEXTEX incorrectly returned %d\n", result);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) bufACP);
  ok(lstrcmpA(bufACP, "not unicode") == 0, "'%s' != 'not unicode'\n", bufACP);

  /* The following test demonstrates that EM_SETTEXTEX supports RTF strings with a selection */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "TestSomeText"); /* TestItem1 */
  p = (char *)buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessage(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_RTF), (LPARAM)&es);
  trace("EM_STREAMOUT produced: \n%s\n", (char *)buf);

  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /* ST_SELECTION && !Unicode && \rtf */
  setText.codepage = CP_ACP;/* EM_STREAMOUT saved as ANSI string */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = ST_SELECTION;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) buf);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok_w3("Expected \"%s\" or \"%s\", got \"%s\"\n", TestItem1alt, TestItem1altn, buf);

  /* The following test demonstrates that EM_SETTEXTEX replacing a selection */
  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;

  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) TestItem1); /* TestItem1 */
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) bufACP);

  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM) &cr);

  /* ST_SELECTION && !Unicode && !\rtf */
  setText.codepage = CP_ACP;
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = ST_SELECTION;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM) bufACP);
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buf);
  ok(lstrcmpW(buf, TestItem1alt) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX when"
      " using ST_SELECTION and non-Unicode\n");

  /* Test setting text using rich text format */
  setText.flags = 0;
  setText.codepage = CP_ACP;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"{\\rtf richtext}");
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) bufACP);
  ok(!strcmp(bufACP, "richtext"), "expected 'richtext' but got '%s'\n", bufACP);

  setText.flags = 0;
  setText.codepage = CP_ACP;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"{\\urtf morerichtext}");
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) bufACP);
  ok(!strcmp(bufACP, "morerichtext"), "expected 'morerichtext' but got '%s'\n", bufACP);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_LIMITTEXT(void)
{
  int ret;

  HWND hwndRichEdit = new_richedit(NULL);

  /* The main purpose of this test is to demonstrate that the nonsense in MSDN
   * about setting the length to -1 for multiline edit controls doesn't happen.
   */

  /* Don't check default gettextlimit case. That's done in other tests */

  /* Set textlimit to 100 */
  SendMessage (hwndRichEdit, EM_LIMITTEXT, 100, 0);
  ret = SendMessage (hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == 100,
      "EM_LIMITTEXT: set to 100, returned: %d, expected: 100\n", ret);

  /* Set textlimit to 0 */
  SendMessage (hwndRichEdit, EM_LIMITTEXT, 0, 0);
  ret = SendMessage (hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == 65536,
      "EM_LIMITTEXT: set to 0, returned: %d, expected: 65536\n", ret);

  /* Set textlimit to -1 */
  SendMessage (hwndRichEdit, EM_LIMITTEXT, -1, 0);
  ret = SendMessage (hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == -1,
      "EM_LIMITTEXT: set to -1, returned: %d, expected: -1\n", ret);

  /* Set textlimit to -2 */
  SendMessage (hwndRichEdit, EM_LIMITTEXT, -2, 0);
  ret = SendMessage (hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == -2,
      "EM_LIMITTEXT: set to -2, returned: %d, expected: -2\n", ret);

  DestroyWindow (hwndRichEdit);
}


static void test_EM_EXLIMITTEXT(void)
{
  int i, selBegin, selEnd, len1, len2;
  int result;
  char text[1024 + 1];
  char buffer[1024 + 1];
  int textlimit = 0; /* multiple of 100 */
  HWND hwndRichEdit = new_richedit(NULL);
  
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(32767 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 32767, i); /* default */
  
  textlimit = 256000;
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  /* set higher */
  ok(textlimit == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", textlimit, i);
  
  textlimit = 1000;
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  /* set lower */
  ok(textlimit == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", textlimit, i);
 
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, 0);
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  /* default for WParam = 0 */
  ok(65536 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 65536, i);
 
  textlimit = sizeof(text)-1;
  memset(text, 'W', textlimit);
  text[sizeof(text)-1] = 0;
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  /* maxed out text */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
  
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);  /* select everything */
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len1 = selEnd - selBegin;
  
  SendMessage(hwndRichEdit, WM_KEYDOWN, VK_BACK, 1);
  SendMessage(hwndRichEdit, WM_CHAR, VK_BACK, 1);
  SendMessage(hwndRichEdit, WM_KEYUP, VK_BACK, 1);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len2 = selEnd - selBegin;
  
  ok(len1 != len2,
    "EM_EXLIMITTEXT: Change Expected\nOld Length: %d, New Length: %d, Limit: %d\n",
    len1,len2,i);
  
  SendMessage(hwndRichEdit, WM_KEYDOWN, 'A', 1);
  SendMessage(hwndRichEdit, WM_CHAR, 'A', 1);
  SendMessage(hwndRichEdit, WM_KEYUP, 'A', 1);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len1 = selEnd - selBegin;
  
  ok(len1 != len2,
    "EM_EXLIMITTEXT: Change Expected\nOld Length: %d, New Length: %d, Limit: %d\n",
    len1,len2,i);
  
  SendMessage(hwndRichEdit, WM_KEYDOWN, 'A', 1);
  SendMessage(hwndRichEdit, WM_CHAR, 'A', 1);
  SendMessage(hwndRichEdit, WM_KEYUP, 'A', 1);  /* full; should be no effect */
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessage(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len2 = selEnd - selBegin;
  
  ok(len1 == len2, 
    "EM_EXLIMITTEXT: No Change Expected\nOld Length: %d, New Length: %d, Limit: %d\n",
    len1,len2,i);

  /* set text up to the limit, select all the text, then add a char */
  textlimit = 5;
  memset(text, 'W', textlimit);
  text[textlimit] = 0;
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessage(hwndRichEdit, WM_CHAR, 'A', 1);
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  result = strcmp(buffer, "A");
  ok(0 == result, "got string = \"%s\"\n", buffer);

  /* WM_SETTEXT not limited */
  textlimit = 10;
  memset(text, 'W', textlimit);
  text[textlimit] = 0;
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit-5);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text);
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  i = strlen(buffer);
  ok(10 == i, "expected 10 chars\n");
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(10 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 10, i);

  /* try inserting more text at end */
  i = SendMessage(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  i = strlen(buffer);
  ok(10 == i, "expected 10 chars, got %i\n", i);
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(10 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 10, i);

  /* try inserting text at beginning */
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 0);
  i = SendMessage(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  i = strlen(buffer);
  ok(10 == i, "expected 10 chars, got %i\n", i);
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(10 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 10, i);

  /* WM_CHAR is limited */
  textlimit = 1;
  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, -1);  /* select everything */
  i = SendMessage(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  i = SendMessage(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  i = strlen(buffer);
  ok(1 == i, "expected 1 chars, got %i instead\n", i);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_GETLIMITTEXT(void)
{
  int i;
  HWND hwndRichEdit = new_richedit(NULL);

  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(32767 == i, "expected: %d, actual: %d\n", 32767, i); /* default value */

  SendMessage(hwndRichEdit, EM_EXLIMITTEXT, 0, 50000);
  i = SendMessage(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(50000 == i, "expected: %d, actual: %d\n", 50000, i);

  DestroyWindow(hwndRichEdit);
}

static void test_WM_SETFONT(void)
{
  /* There is no invalid input or error conditions for this function.
   * NULL wParam and lParam just fall back to their default values 
   * It should be noted that even if you use a gibberish name for your fonts
   * here, it will still work because the name is stored. They will display as
   * System, but will report their name to be whatever they were created as */
  
  HWND hwndRichEdit = new_richedit(NULL);
  HFONT testFont1 = CreateFontA (0,0,0,0,FW_LIGHT, 0, 0, 0, ANSI_CHARSET, 
    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | 
    FF_DONTCARE, "Marlett");
  HFONT testFont2 = CreateFontA (0,0,0,0,FW_LIGHT, 0, 0, 0, ANSI_CHARSET, 
    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | 
    FF_DONTCARE, "MS Sans Serif");
  HFONT testFont3 = CreateFontA (0,0,0,0,FW_LIGHT, 0, 0, 0, ANSI_CHARSET, 
    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | 
    FF_DONTCARE, "Courier");
  LOGFONTA sentLogFont;
  CHARFORMAT2A returnedCF2A;
  
  returnedCF2A.cbSize = sizeof(returnedCF2A);
  
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "x");
  SendMessage(hwndRichEdit, WM_SETFONT, (WPARAM)testFont1, MAKELPARAM(TRUE, 0));
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM) &returnedCF2A);

  GetObjectA(testFont1, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp(sentLogFont.lfFaceName,returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 1. Sent: %s, Returned: %s\n",
    sentLogFont.lfFaceName,returnedCF2A.szFaceName);

  SendMessage(hwndRichEdit, WM_SETFONT, (WPARAM)testFont2, MAKELPARAM(TRUE, 0));
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM) &returnedCF2A);
  GetObjectA(testFont2, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp(sentLogFont.lfFaceName,returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 2. Sent: %s, Returned: %s\n",
    sentLogFont.lfFaceName,returnedCF2A.szFaceName);
    
  SendMessage(hwndRichEdit, WM_SETFONT, (WPARAM)testFont3, MAKELPARAM(TRUE, 0));
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM) &returnedCF2A);
  GetObjectA(testFont3, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp(sentLogFont.lfFaceName,returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 3. Sent: %s, Returned: %s\n",
    sentLogFont.lfFaceName,returnedCF2A.szFaceName);
   
  /* This last test is special since we send in NULL. We clear the variables
   * and just compare to "System" instead of the sent in font name. */
  ZeroMemory(&returnedCF2A,sizeof(returnedCF2A));
  ZeroMemory(&sentLogFont,sizeof(sentLogFont));
  returnedCF2A.cbSize = sizeof(returnedCF2A);
  
  SendMessage(hwndRichEdit, WM_SETFONT, 0, MAKELPARAM((WORD) TRUE, 0));
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM) &returnedCF2A);
  GetObjectA(NULL, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp("System",returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 4. Sent: NULL, Returned: %s. Expected \"System\".\n",returnedCF2A.szFaceName);
  
  DestroyWindow(hwndRichEdit);
}


static DWORD CALLBACK test_EM_GETMODIFY_esCallback(DWORD_PTR dwCookie,
                                         LPBYTE pbBuff,
                                         LONG cb,
                                         LONG *pcb)
{
  const char** str = (const char**)dwCookie;
  int size = strlen(*str);
  if(size > 3)  /* let's make it piecemeal for fun */
    size = 3;
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

static void test_EM_GETMODIFY(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  LRESULT result;
  SETTEXTEX setText;
  WCHAR TestItem1[] = {'T', 'e', 's', 't', 
                       'S', 'o', 'm', 'e', 
                       'T', 'e', 'x', 't', 0}; 
  WCHAR TestItem2[] = {'T', 'e', 's', 't', 
                       'S', 'o', 'm', 'e', 
                       'O', 't', 'h', 'e', 'r',
                       'T', 'e', 'x', 't', 0}; 
  const char* streamText = "hello world";
  CHARFORMAT2 cf2;
  PARAFORMAT2 pf2;
  EDITSTREAM es;
  
  HFONT testFont = CreateFontA (0,0,0,0,FW_LIGHT, 0, 0, 0, ANSI_CHARSET, 
    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | 
    FF_DONTCARE, "Courier");
  
  setText.codepage = 1200;  /* no constant for unicode */
  setText.flags = ST_KEEPUNDO;
  

  /* modify flag shouldn't be set when richedit is first created */
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0, 
      "EM_GETMODIFY returned non-zero, instead of zero on create\n");
  
  /* setting modify flag should actually set it */
  SendMessage(hwndRichEdit, EM_SETMODIFY, TRUE, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0, 
      "EM_GETMODIFY returned zero, instead of non-zero on EM_SETMODIFY\n");
  
  /* clearing modify flag should actually clear it */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0, 
      "EM_GETMODIFY returned non-zero, instead of zero on EM_SETMODIFY\n");
 
  /* setting font doesn't change modify flag */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, WM_SETFONT, (WPARAM)testFont, MAKELPARAM(TRUE, 0));
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero, instead of zero on setting font\n");

  /* setting text should set modify flag */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero on setting text\n");
  
  /* undo previous text doesn't reset modify flag */
  SendMessage(hwndRichEdit, WM_UNDO, 0, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero on undo after setting text\n");
  
  /* set text with no flag to keep undo stack should not set modify flag */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  setText.flags = 0;
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero, instead of zero when setting text while not keeping undo stack\n");
  
  /* WM_SETTEXT doesn't modify */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem2);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero for WM_SETTEXT\n");
  
  /* clear the text */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, WM_CLEAR, 0, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero, instead of zero for WM_CLEAR\n");
  
  /* replace text */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
  SendMessage(hwndRichEdit, EM_REPLACESEL, TRUE, (LPARAM)TestItem2);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero when replacing text\n");
  
  /* copy/paste text 1 */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
  SendMessage(hwndRichEdit, WM_COPY, 0, 0);
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero when pasting identical text\n");
  
  /* copy/paste text 2 */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 2);
  SendMessage(hwndRichEdit, WM_COPY, 0, 0);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 3);
  SendMessage(hwndRichEdit, WM_PASTE, 0, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero when pasting different text\n");
  
  /* press char */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, EM_SETSEL, 0, 1);
  SendMessage(hwndRichEdit, WM_CHAR, 'A', 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for WM_CHAR\n");

  /* press del */
  SendMessage(hwndRichEdit, WM_CHAR, 'A', 0);
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessage(hwndRichEdit, WM_KEYDOWN, VK_BACK, 0);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for backspace\n");
  
  /* set char format */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  cf2.cbSize = sizeof(CHARFORMAT2);
  SendMessage(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);
  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;
  SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf2);
  result = SendMessage(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM) SCF_ALL, (LPARAM) &cf2);
  ok(result == 1, "EM_SETCHARFORMAT returned %ld instead of 1\n", result);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for EM_SETCHARFORMAT\n");
  
  /* set para format */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  pf2.cbSize = sizeof(PARAFORMAT2);
  SendMessage(hwndRichEdit, EM_GETPARAFORMAT, 0,
             (LPARAM) &pf2);
  pf2.dwMask = PFM_ALIGNMENT | pf2.dwMask;
  pf2.wAlignment = PFA_RIGHT;
  SendMessage(hwndRichEdit, EM_SETPARAFORMAT, 0, (LPARAM) &pf2);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned zero, instead of non-zero for EM_SETPARAFORMAT\n");

  /* EM_STREAM */
  SendMessage(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  es.dwCookie = (DWORD_PTR)&streamText;
  es.dwError = 0;
  es.pfnCallback = test_EM_GETMODIFY_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN, 
              (WPARAM)(SF_TEXT), (LPARAM)&es);
  result = SendMessage(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for EM_STREAM\n");

  DestroyWindow(hwndRichEdit);
}

struct exsetsel_s {
  long min;
  long max;
  long expected_retval;
  int expected_getsel_start;
  int expected_getsel_end;
  int _exsetsel_todo_wine;
  int _getsel_todo_wine;
};

const struct exsetsel_s exsetsel_tests[] = {
  /* sanity tests */
  {5, 10, 10, 5, 10, 0, 0},
  {15, 17, 17, 15, 17, 0, 0},
  /* test cpMax > strlen() */
  {0, 100, 18, 0, 18, 0, 1},
  /* test cpMin == cpMax */
  {5, 5, 5, 5, 5, 0, 0},
  /* test cpMin < 0 && cpMax >= 0 (bug 4462) */
  {-1, 0, 5, 5, 5, 0, 0},
  {-1, 17, 5, 5, 5, 0, 0},
  {-1, 18, 5, 5, 5, 0, 0},
  /* test cpMin < 0 && cpMax < 0 */
  {-1, -1, 17, 17, 17, 0, 0},
  {-4, -5, 17, 17, 17, 0, 0},
  /* test cMin >=0 && cpMax < 0 (bug 6814) */
  {0, -1, 18, 0, 18, 0, 1},
  {17, -5, 18, 17, 18, 0, 1},
  {18, -3, 17, 17, 17, 0, 0},
  /* test if cpMin > cpMax */
  {15, 19, 18, 15, 18, 0, 1},
  {19, 15, 18, 15, 18, 0, 1}
};

static void check_EM_EXSETSEL(HWND hwnd, const struct exsetsel_s *setsel, int id) {
    CHARRANGE cr;
    long result;
    int start, end;

    cr.cpMin = setsel->min;
    cr.cpMax = setsel->max;
    result = SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) &cr);

    if (setsel->_exsetsel_todo_wine) {
        todo_wine {
            ok(result == setsel->expected_retval, "EM_EXSETSEL(%d): expected: %ld actual: %ld\n", id, setsel->expected_retval, result);
        }
    } else {
        ok(result == setsel->expected_retval, "EM_EXSETSEL(%d): expected: %ld actual: %ld\n", id, setsel->expected_retval, result);
    }

    SendMessage(hwnd, EM_GETSEL, (WPARAM) &start, (LPARAM) &end);

    if (setsel->_getsel_todo_wine) {
        todo_wine {
            ok(start == setsel->expected_getsel_start && end == setsel->expected_getsel_end, "EM_EXSETSEL(%d): expected (%d,%d) actual:(%d,%d)\n", id, setsel->expected_getsel_start, setsel->expected_getsel_end, start, end);
        }
    } else {
        ok(start == setsel->expected_getsel_start && end == setsel->expected_getsel_end, "EM_EXSETSEL(%d): expected (%d,%d) actual:(%d,%d)\n", id, setsel->expected_getsel_start, setsel->expected_getsel_end, start, end);
    }
}

static void test_EM_EXSETSEL(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    int i;
    const int num_tests = sizeof(exsetsel_tests)/sizeof(struct exsetsel_s);

    /* sending some text to the window */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "testing selection");
    /*                                                 01234567890123456*/
    /*                                                          10      */

    for (i = 0; i < num_tests; i++) {
        check_EM_EXSETSEL(hwndRichEdit, &exsetsel_tests[i], i);
    }

    DestroyWindow(hwndRichEdit);
}

static void test_EM_REPLACESEL(int redraw)
{
    HWND hwndRichEdit = new_richedit(NULL);
    char buffer[1024] = {0};
    int r;
    GETTEXTEX getText;
    CHARRANGE cr;

    /* sending some text to the window */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "testing selection");
    /*                                                 01234567890123456*/
    /*                                                          10      */

    /* FIXME add more tests */
    SendMessage(hwndRichEdit, EM_SETSEL, 7, 17);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, 0);
    ok(0 == r, "EM_REPLACESEL returned %d, expected 0\n", r);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    r = strcmp(buffer, "testing");
    ok(0 == r, "expected %d, got %d\n", 0, r);

    DestroyWindow(hwndRichEdit);

    hwndRichEdit = new_richedit(NULL);

    trace("Testing EM_REPLACESEL behavior with redraw=%d\n", redraw);
    SendMessage(hwndRichEdit, WM_SETREDRAW, redraw, 0);

    /* Test behavior with carriage returns and newlines */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "RichEdit1");
    ok(9 == r, "EM_REPLACESEL returned %d, expected 9\n", r);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    r = strcmp(buffer, "RichEdit1");
    ok(0 == r, "expected %d, got %d\n", 0, r);
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "RichEdit1") == 0,
      "EM_GETTEXTEX results not what was set by EM_REPLACESEL\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 1, "EM_GETLINECOUNT returned %d, expected 1\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "RichEdit1\r");
    ok(10 == r, "EM_REPLACESEL returned %d, expected 10\n", r);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    r = strcmp(buffer, "RichEdit1\r\n");
    ok(0 == r, "expected %d, got %d\n", 0, r);
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "RichEdit1\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 2, "EM_GETLINECOUNT returned %d, expected 2\n", r);

    /* Win98's riched20 and WinXP's riched20 disagree on what to return from
       EM_REPLACESEL. The general rule seems to be that Win98's riched20
       returns the number of characters *inserted* into the control (after
       required conversions), but WinXP's riched20 returns the number of
       characters interpreted from the original lParam. Wine's builtin riched20
       implements the WinXP behavior.
     */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "RichEdit1\r\n");
    ok(11 == r /* WinXP */ || 10 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 11 or 10\n", r);

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 2, "EM_GETLINECOUNT returned %d, expected 2\n", r);

    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 10, "EM_EXGETSEL returned cpMin=%d, expected 10\n", cr.cpMin);
    ok(cr.cpMax == 10, "EM_EXGETSEL returned cpMax=%d, expected 10\n", cr.cpMax);

    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    r = strcmp(buffer, "RichEdit1\r\n");
    ok(0 == r, "expected %d, got %d\n", 0, r);
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "RichEdit1\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 10, "EM_EXGETSEL returned cpMin=%d, expected 10\n", cr.cpMin);
    ok(cr.cpMax == 10, "EM_EXGETSEL returned cpMax=%d, expected 10\n", cr.cpMax);

    /* The following tests show that richedit should handle the special \r\r\n
       sequence by turning it into a single space on insertion. However,
       EM_REPLACESEL on WinXP returns the number of characters in the original
       string.
     */

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\r\r");
    ok(2 == r, "EM_REPLACESEL returned %d, expected 4\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 2, "EM_EXGETSEL returned cpMin=%d, expected 2\n", cr.cpMin);
    ok(cr.cpMax == 2, "EM_EXGETSEL returned cpMax=%d, expected 2\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 3, "EM_GETLINECOUNT returned %d, expected 3\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\r\r\n");
    ok(3 == r /* WinXP */ || 1 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 3 or 1\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 1, "EM_EXGETSEL returned cpMin=%d, expected 1\n", cr.cpMin);
    ok(cr.cpMax == 1, "EM_EXGETSEL returned cpMax=%d, expected 1\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, " ") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 1, "EM_GETLINECOUNT returned %d, expected 1\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\r\r\r\r\r\n\r\r\r");
    ok(9 == r /* WinXP */ || 7 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 9 or 7\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 7, "EM_EXGETSEL returned cpMin=%d, expected 7\n", cr.cpMin);
    ok(cr.cpMax == 7, "EM_EXGETSEL returned cpMax=%d, expected 7\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "\r\r\r \r\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 7, "EM_GETLINECOUNT returned %d, expected 7\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\r\r\n\r\n");
    ok(5 == r /* WinXP */ || 2 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 5 or 2\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 2, "EM_EXGETSEL returned cpMin=%d, expected 2\n", cr.cpMin);
    ok(cr.cpMax == 2, "EM_EXGETSEL returned cpMax=%d, expected 2\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, " \r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 2, "EM_GETLINECOUNT returned %d, expected 2\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\r\r\n\r\r");
    ok(5 == r /* WinXP */ || 3 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 5 or 3\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 3, "EM_EXGETSEL returned cpMin=%d, expected 3\n", cr.cpMin);
    ok(cr.cpMax == 3, "EM_EXGETSEL returned cpMax=%d, expected 3\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, " \r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 3, "EM_GETLINECOUNT returned %d, expected 3\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\rX\r\n\r\r");
    ok(6 == r /* WinXP */ || 5 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 6 or 5\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 5, "EM_EXGETSEL returned cpMin=%d, expected 5\n", cr.cpMin);
    ok(cr.cpMax == 5, "EM_EXGETSEL returned cpMax=%d, expected 5\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "\rX\r\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 5, "EM_GETLINECOUNT returned %d, expected 5\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\n\n");
    ok(2 == r, "EM_REPLACESEL returned %d, expected 2\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 2, "EM_EXGETSEL returned cpMin=%d, expected 2\n", cr.cpMin);
    ok(cr.cpMax == 2, "EM_EXGETSEL returned cpMax=%d, expected 2\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 3, "EM_GETLINECOUNT returned %d, expected 3\n", r);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessage(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM) "\n\n\n\n\r\r\r\r\n");
    ok(9 == r /* WinXP */ || 7 == r /* Win98 */,
        "EM_REPLACESEL returned %d, expected 9 or 7\n", r);
    r = SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 7, "EM_EXGETSEL returned cpMin=%d, expected 7\n", cr.cpMin);
    ok(cr.cpMax == 7, "EM_EXGETSEL returned cpMax=%d, expected 7\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessage(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM) buffer);
    ok(strcmp(buffer, "\r\r\r\r\r\r ") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 7, "EM_GETLINECOUNT returned %d, expected 7\n", r);

    if (!redraw)
        /* This is needed to avoid interferring with keybd_event calls
         * on other tests that simulate keyboard events. */
        SendMessage(hwndRichEdit, WM_SETREDRAW, TRUE, 0);

    DestroyWindow(hwndRichEdit);
}

static void test_WM_PASTE(void)
{
    int result;
    char buffer[1024] = {0};
    const char* text1 = "testing paste\r";
    const char* text1_step1 = "testing paste\r\ntesting paste\r\n";
    const char* text1_after = "testing paste\r\n";
    const char* text2 = "testing paste\r\rtesting paste";
    const char* text2_after = "testing paste\r\n\r\ntesting paste";
    const char* text3 = "testing paste\r\npaste\r\ntesting paste";
    HWND hwndRichEdit = new_richedit(NULL);

    /* Native riched20 inspects the keyboard state (e.g. GetKeyState)
     * to test the state of the modifiers (Ctrl/Alt/Shift).
     *
     * Therefore Ctrl-<key> keystrokes need to be simulated with
     * keybd_event or by using SetKeyboardState to set the modifiers
     * and SendMessage to simulate the keystrokes.
     */

    /* Sent keystrokes with keybd_event */
#define SEND_CTRL_C(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, 'C')
#define SEND_CTRL_X(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, 'X')
#define SEND_CTRL_V(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, 'V')
#define SEND_CTRL_Z(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, 'Z')
#define SEND_CTRL_Y(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, 'Y')

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text1);
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 14);

    SEND_CTRL_C(hwndRichEdit);   /* Copy */
    SendMessage(hwndRichEdit, EM_SETSEL, 14, 14);
    SEND_CTRL_V(hwndRichEdit);   /* Paste */
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    /* Pasted text should be visible at this step */
    result = strcmp(text1_step1, buffer);
    ok(result == 0,
        "test paste: strcmp = %i, text='%s'\n", result, buffer);

    SEND_CTRL_Z(hwndRichEdit);   /* Undo */
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    /* Text should be the same as before (except for \r -> \r\n conversion) */
    result = strcmp(text1_after, buffer);
    ok(result == 0,
        "test paste: strcmp = %i, text='%s'\n", result, buffer);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text2);
    SendMessage(hwndRichEdit, EM_SETSEL, 8, 13);
    SEND_CTRL_C(hwndRichEdit);   /* Copy */
    SendMessage(hwndRichEdit, EM_SETSEL, 14, 14);
    SEND_CTRL_V(hwndRichEdit);   /* Paste */
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    /* Pasted text should be visible at this step */
    result = strcmp(text3, buffer);
    ok(result == 0,
        "test paste: strcmp = %i\n", result);
    SEND_CTRL_Z(hwndRichEdit);   /* Undo */
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    /* Text should be the same as before (except for \r -> \r\n conversion) */
    result = strcmp(text2_after, buffer);
    ok(result == 0,
        "test paste: strcmp = %i\n", result);
    SEND_CTRL_Y(hwndRichEdit);   /* Redo */
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    /* Text should revert to post-paste state */
    result = strcmp(buffer,text3);
    ok(result == 0,
        "test paste: strcmp = %i\n", result);

#undef SEND_CTRL_C
#undef SEND_CTRL_X
#undef SEND_CTRL_V
#undef SEND_CTRL_Z
#undef SEND_CTRL_Y

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    /* Send WM_CHAR to simulates Ctrl-V */
    SendMessage(hwndRichEdit, WM_CHAR, 22,
                (MapVirtualKey('V', MAPVK_VK_TO_VSC) << 16) & 1);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    /* Shouldn't paste because pasting is handled by WM_KEYDOWN */
    result = strcmp(buffer,"");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    /* Send keystrokes with WM_KEYDOWN after setting the modifiers
     * with SetKeyboard state. */

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    /* Simulates paste (Ctrl-V) */
    hold_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_KEYDOWN, 'V',
                (MapVirtualKey('V', MAPVK_VK_TO_VSC) << 16) & 1);
    release_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,"paste");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text1);
    SendMessage(hwndRichEdit, EM_SETSEL, 0, 7);
    /* Simulates copy (Ctrl-C) */
    hold_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_KEYDOWN, 'C',
                (MapVirtualKey('C', MAPVK_VK_TO_VSC) << 16) & 1);
    release_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    SendMessage(hwndRichEdit, WM_PASTE, 0, 0);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,"testing");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    /* Cut with WM_KEYDOWN to simulate Ctrl-X */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "cut");
    /* Simulates select all (Ctrl-A) */
    hold_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_KEYDOWN, 'A',
                (MapVirtualKey('A', MAPVK_VK_TO_VSC) << 16) & 1);
    /* Simulates select cut (Ctrl-X) */
    SendMessage(hwndRichEdit, WM_KEYDOWN, 'X',
                (MapVirtualKey('X', MAPVK_VK_TO_VSC) << 16) & 1);
    release_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,"");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, 0);
    SendMessage(hwndRichEdit, WM_PASTE, 0, 0);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,"cut\r\n");
    todo_wine ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    /* Simulates undo (Ctrl-Z) */
    hold_key(VK_CONTROL);
    SendMessage(hwndRichEdit, WM_KEYDOWN, 'Z',
                (MapVirtualKey('Z', MAPVK_VK_TO_VSC) << 16) & 1);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,"");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    /* Simulates redo (Ctrl-Y) */
    SendMessage(hwndRichEdit, WM_KEYDOWN, 'Y',
                (MapVirtualKey('Y', MAPVK_VK_TO_VSC) << 16) & 1);
    SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    result = strcmp(buffer,"cut\r\n");
    todo_wine ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    release_key(VK_CONTROL);

    DestroyWindow(hwndRichEdit);
}

static void test_EM_FORMATRANGE(void)
{
  int r;
  FORMATRANGE fr;
  HDC hdc;
  HWND hwndRichEdit = new_richedit(NULL);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) haystack);

  hdc = GetDC(hwndRichEdit);
  ok(hdc != NULL, "Could not get HDC\n");

  fr.hdc = fr.hdcTarget = hdc;
  fr.rc.top = fr.rcPage.top = fr.rc.left = fr.rcPage.left = 0;
  fr.rc.right = fr.rcPage.right = GetDeviceCaps(hdc, HORZRES);
  fr.rc.bottom = fr.rcPage.bottom = GetDeviceCaps(hdc, VERTRES);
  fr.chrg.cpMin = 0;
  fr.chrg.cpMax = 20;

  r = SendMessage(hwndRichEdit, EM_FORMATRANGE, TRUE, 0);
  todo_wine {
    ok(r == 31, "EM_FORMATRANGE expect %d, got %d\n", 31, r);
  }

  r = SendMessage(hwndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) &fr);
  todo_wine {
    ok(r == 20 || r == 9, "EM_FORMATRANGE expect 20 or 9, got %d\n", r);
  }

  fr.chrg.cpMin = 0;
  fr.chrg.cpMax = 10;

  r = SendMessage(hwndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) &fr);
  todo_wine {
    ok(r == 10, "EM_FORMATRANGE expect %d, got %d\n", 10, r);
  }

  r = SendMessage(hwndRichEdit, EM_FORMATRANGE, TRUE, 0);
  todo_wine {
    ok(r == 31, "EM_FORMATRANGE expect %d, got %d\n", 31, r);
  }

  DestroyWindow(hwndRichEdit);
}

static int nCallbackCount = 0;

static DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff,
				 LONG cb, LONG* pcb)
{
  const char text[] = {'t','e','s','t'};

  if (sizeof(text) <= cb)
  {
    if ((int)dwCookie != nCallbackCount)
    {
      *pcb = 0;
      return 0;
    }

    memcpy (pbBuff, text, sizeof(text));
    *pcb = sizeof(text);

    nCallbackCount++;

    return 0;
  }
  else
    return 1; /* indicates callback failed */
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

struct StringWithLength {
    int length;
    char *buffer;
};

/* This callback is used to handled the null characters in a string. */
static DWORD CALLBACK test_EM_STREAMIN_esCallback2(DWORD_PTR dwCookie,
                                                   LPBYTE pbBuff,
                                                   LONG cb,
                                                   LONG *pcb)
{
    struct StringWithLength* str = (struct StringWithLength*)dwCookie;
    int size = str->length;
    *pcb = cb;
    if (*pcb > size) {
      *pcb = size;
    }
    if (*pcb > 0) {
      memcpy(pbBuff, str->buffer, *pcb);
      str->buffer += *pcb;
      str->length -= *pcb;
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

  /* In richedit 2.0 mode, this should NOT be accepted, unlike 1.0 */
  const char * streamText2 =
    "{{\\colortbl;\\red0\\green255\\blue102;\\red255\\green255\\blue255;"
    "\\red170\\green255\\blue255;\\red255\\green238\\blue0;\\red51\\green255"
    "\\blue221;\\red238\\green238\\blue238;}\\tx0 \\tx424 \\tx848 \\tx1272 "
    "\\tx1696 \\tx2120 \\tx2544 \\tx2968 \\tx3392 \\tx3816 \\tx4240 \\tx4664 "
    "\\tx5088 \\tx5512 \\tx5936 \\tx6360 \\tx6784 \\tx7208 \\tx7632 \\tx8056 "
    "\\tx8480 \\tx8904 \\tx9328 \\tx9752 \\tx10176 \\tx10600 \\tx11024 "
    "\\tx11448 \\tx11872 \\tx12296 \\tx12720 \\tx13144 \\cf2 RichEdit1\\line }";

  const char * streamText3 = "RichEdit1";

  struct StringWithLength cookieForStream4;
  const char * streamText4 =
      "This text just needs to be long enough to cause run to be split onto "
      "two separate lines and make sure the null terminating character is "
      "handled properly.\0";
  int length4 = strlen(streamText4) + 1;
  cookieForStream4.buffer = (char *)streamText4;
  cookieForStream4.length = length4;

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
  ok(es.dwError == 0, "EM_STREAMIN: Test 0-a set error %d, expected %d\n", es.dwError, 0);

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
  ok(es.dwError == 0, "EM_STREAMIN: Test 0-b set error %d, expected %d\n", es.dwError, 0);

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
  ok(es.dwError == 0, "EM_STREAMIN: Test 1 set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText2;
  es.dwError = 0;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 2 returned %ld, expected 0\n", result);
  ok (strlen(buffer)  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == -16, "EM_STREAMIN: Test 2 set error %d, expected %d\n", es.dwError, -16);

  es.dwCookie = (DWORD_PTR)&streamText3;
  es.dwError = 0;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 3 returned %ld, expected 0\n", result);
  ok (strlen(buffer)  == 0,
      "EM_STREAMIN: Test 3 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == -16, "EM_STREAMIN: Test 3 set error %d, expected %d\n", es.dwError, -16);

  es.dwCookie = (DWORD_PTR)&cookieForStream4;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback2;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_TEXT), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == length4,
      "EM_STREAMIN: Test 4 returned %ld, expected %d\n", result, length4);
  ok(es.dwError == 0, "EM_STREAMIN: Test 4 set error %d, expected %d\n", es.dwError, 0);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_StreamIn_Undo(void)
{
  /* The purpose of this test is to determine when a EM_StreamIn should be
   * undoable. This is important because WM_PASTE currently uses StreamIn and
   * pasting should always be undoable but streaming isn't always.
   *
   * cases to test:
   * StreamIn plain text without SFF_SELECTION.
   * StreamIn plain text with SFF_SELECTION set but a zero-length selection
   * StreamIn plain text with SFF_SELECTION and a valid, normal selection
   * StreamIn plain text with SFF_SELECTION and a backwards-selection (from>to)
   * Feel free to add tests for other text modes or StreamIn things.
   */


  HWND hwndRichEdit = new_richedit(NULL);
  LRESULT result;
  EDITSTREAM es;
  char buffer[1024] = {0};
  const char randomtext[] = "Some text";

  es.pfnCallback = (EDITSTREAMCALLBACK) EditStreamCallback;

  /* StreamIn, no SFF_SELECTION */
  es.dwCookie = nCallbackCount;
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) randomtext);
  SendMessage(hwndRichEdit, EM_SETSEL,0,0);
  SendMessage(hwndRichEdit, EM_STREAMIN, (WPARAM)SF_TEXT, (LPARAM)&es);
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  result = strcmp (buffer,"test");
  ok (result  == 0,
      "EM_STREAMIN: Test 1 set wrong text: Result: %s\n",buffer);

  result = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok (result == FALSE,
      "EM_STREAMIN without SFF_SELECTION wrongly allows undo\n");

  /* StreamIn, SFF_SELECTION, but nothing selected */
  es.dwCookie = nCallbackCount;
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) randomtext);
  SendMessage(hwndRichEdit, EM_SETSEL,0,0);
  SendMessage(hwndRichEdit, EM_STREAMIN,
	      (WPARAM)(SF_TEXT|SFF_SELECTION), (LPARAM)&es);
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  result = strcmp (buffer,"testSome text");
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);

  result = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok (result == TRUE,
     "EM_STREAMIN with SFF_SELECTION but no selection set "
      "should create an undo\n");

  /* StreamIn, SFF_SELECTION, with a selection */
  es.dwCookie = nCallbackCount;
  SendMessage(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) randomtext);
  SendMessage(hwndRichEdit, EM_SETSEL,4,5);
  SendMessage(hwndRichEdit, EM_STREAMIN,
	      (WPARAM)(SF_TEXT|SFF_SELECTION), (LPARAM)&es);
  SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  result = strcmp (buffer,"Sometesttext");
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);

  result = SendMessage(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok (result == TRUE,
      "EM_STREAMIN with SFF_SELECTION and selection set "
      "should create an undo\n");

  DestroyWindow(hwndRichEdit);
}

static BOOL is_em_settextex_supported(HWND hwnd)
{
    SETTEXTEX stex = { ST_DEFAULT, CP_ACP };
    return SendMessageA(hwnd, EM_SETTEXTEX, (WPARAM)&stex, 0) != 0;
}

static void test_unicode_conversions(void)
{
    static const WCHAR tW[] = {'t',0};
    static const WCHAR teW[] = {'t','e',0};
    static const WCHAR textW[] = {'t','e','s','t',0};
    static const char textA[] = "test";
    char bufA[64];
    WCHAR bufW[64];
    HWND hwnd;
    int em_settextex_supported, ret;

#define set_textA(hwnd, wm_set_text, txt) \
    do { \
        SETTEXTEX stex = { ST_DEFAULT, CP_ACP }; \
        WPARAM wparam = (wm_set_text == WM_SETTEXT) ? 0 : (WPARAM)&stex; \
        assert(wm_set_text == WM_SETTEXT || wm_set_text == EM_SETTEXTEX); \
        ret = SendMessageA(hwnd, wm_set_text, wparam, (LPARAM)txt); \
        ok(ret, "SendMessageA(%02x) error %u\n", wm_set_text, GetLastError()); \
    } while(0)
#define expect_textA(hwnd, wm_get_text, txt) \
    do { \
        GETTEXTEX gtex = { 64, GT_DEFAULT, CP_ACP, NULL, NULL }; \
        WPARAM wparam = (wm_get_text == WM_GETTEXT) ? 64 : (WPARAM)&gtex; \
        assert(wm_get_text == WM_GETTEXT || wm_get_text == EM_GETTEXTEX); \
        memset(bufA, 0xAA, sizeof(bufA)); \
        ret = SendMessageA(hwnd, wm_get_text, wparam, (LPARAM)bufA); \
        ok(ret, "SendMessageA(%02x) error %u\n", wm_get_text, GetLastError()); \
        ret = lstrcmpA(bufA, txt); \
        ok(!ret, "%02x: strings do not match: expected %s got %s\n", wm_get_text, txt, bufA); \
    } while(0)

#define set_textW(hwnd, wm_set_text, txt) \
    do { \
        SETTEXTEX stex = { ST_DEFAULT, 1200 }; \
        WPARAM wparam = (wm_set_text == WM_SETTEXT) ? 0 : (WPARAM)&stex; \
        assert(wm_set_text == WM_SETTEXT || wm_set_text == EM_SETTEXTEX); \
        ret = SendMessageW(hwnd, wm_set_text, wparam, (LPARAM)txt); \
        ok(ret, "SendMessageW(%02x) error %u\n", wm_set_text, GetLastError()); \
    } while(0)
#define expect_textW(hwnd, wm_get_text, txt) \
    do { \
        GETTEXTEX gtex = { 64, GT_DEFAULT, 1200, NULL, NULL }; \
        WPARAM wparam = (wm_get_text == WM_GETTEXT) ? 64 : (WPARAM)&gtex; \
        assert(wm_get_text == WM_GETTEXT || wm_get_text == EM_GETTEXTEX); \
        memset(bufW, 0xAA, sizeof(bufW)); \
        if (is_win9x) \
        { \
            assert(wm_get_text == EM_GETTEXTEX); \
            ret = SendMessageA(hwnd, wm_get_text, wparam, (LPARAM)bufW); \
            ok(ret, "SendMessageA(%02x) error %u\n", wm_get_text, GetLastError()); \
        } \
        else \
        { \
            ret = SendMessageW(hwnd, wm_get_text, wparam, (LPARAM)bufW); \
            ok(ret, "SendMessageW(%02x) error %u\n", wm_get_text, GetLastError()); \
        } \
        ret = lstrcmpW(bufW, txt); \
        ok(!ret, "%02x: strings do not match: expected[0] %x got[0] %x\n", wm_get_text, txt[0], bufW[0]); \
    } while(0)
#define expect_empty(hwnd, wm_get_text) \
    do { \
        GETTEXTEX gtex = { 64, GT_DEFAULT, CP_ACP, NULL, NULL }; \
        WPARAM wparam = (wm_get_text == WM_GETTEXT) ? 64 : (WPARAM)&gtex; \
        assert(wm_get_text == WM_GETTEXT || wm_get_text == EM_GETTEXTEX); \
        memset(bufA, 0xAA, sizeof(bufA)); \
        ret = SendMessageA(hwnd, wm_get_text, wparam, (LPARAM)bufA); \
        ok(!ret, "empty richedit should return 0, got %d\n", ret); \
        ok(!*bufA, "empty richedit should return empty string, got %s\n", bufA); \
    } while(0)

    hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP,
                           0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    ret = IsWindowUnicode(hwnd);
    if (is_win9x)
        ok(!ret, "RichEdit20W should NOT be unicode under Win9x\n");
    else
        ok(ret, "RichEdit20W should be unicode under NT\n");

    /* EM_SETTEXTEX is supported starting from version 3.0 */
    em_settextex_supported = is_em_settextex_supported(hwnd);
    trace("EM_SETTEXTEX is %ssupported on this platform\n",
          em_settextex_supported ? "" : "NOT ");

    expect_empty(hwnd, WM_GETTEXT);
    expect_empty(hwnd, EM_GETTEXTEX);

    ret = SendMessageA(hwnd, WM_CHAR, (WPARAM)textW[0], 0);
    ok(!ret, "SendMessageA(WM_CHAR) should return 0, got %d\n", ret);
    expect_textA(hwnd, WM_GETTEXT, "t");
    expect_textA(hwnd, EM_GETTEXTEX, "t");
    expect_textW(hwnd, EM_GETTEXTEX, tW);

    ret = SendMessageA(hwnd, WM_CHAR, (WPARAM)textA[1], 0);
    ok(!ret, "SendMessageA(WM_CHAR) should return 0, got %d\n", ret);
    expect_textA(hwnd, WM_GETTEXT, "te");
    expect_textA(hwnd, EM_GETTEXTEX, "te");
    expect_textW(hwnd, EM_GETTEXTEX, teW);

    set_textA(hwnd, WM_SETTEXT, NULL);
    expect_empty(hwnd, WM_GETTEXT);
    expect_empty(hwnd, EM_GETTEXTEX);

    if (is_win9x)
        set_textA(hwnd, WM_SETTEXT, textW);
    else
        set_textA(hwnd, WM_SETTEXT, textA);
    expect_textA(hwnd, WM_GETTEXT, textA);
    expect_textA(hwnd, EM_GETTEXTEX, textA);
    expect_textW(hwnd, EM_GETTEXTEX, textW);

    if (em_settextex_supported)
    {
        set_textA(hwnd, EM_SETTEXTEX, textA);
        expect_textA(hwnd, WM_GETTEXT, textA);
        expect_textA(hwnd, EM_GETTEXTEX, textA);
        expect_textW(hwnd, EM_GETTEXTEX, textW);
    }

    if (!is_win9x)
    {
        set_textW(hwnd, WM_SETTEXT, textW);
        expect_textW(hwnd, WM_GETTEXT, textW);
        expect_textA(hwnd, WM_GETTEXT, textA);
        expect_textW(hwnd, EM_GETTEXTEX, textW);
        expect_textA(hwnd, EM_GETTEXTEX, textA);

        if (em_settextex_supported)
        {
            set_textW(hwnd, EM_SETTEXTEX, textW);
            expect_textW(hwnd, WM_GETTEXT, textW);
            expect_textA(hwnd, WM_GETTEXT, textA);
            expect_textW(hwnd, EM_GETTEXTEX, textW);
            expect_textA(hwnd, EM_GETTEXTEX, textA);
        }
    }
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, "RichEdit20A", NULL, WS_POPUP,
                           0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    ret = IsWindowUnicode(hwnd);
    ok(!ret, "RichEdit20A should NOT be unicode\n");

    set_textA(hwnd, WM_SETTEXT, textA);
    expect_textA(hwnd, WM_GETTEXT, textA);
    expect_textA(hwnd, EM_GETTEXTEX, textA);
    expect_textW(hwnd, EM_GETTEXTEX, textW);

    if (em_settextex_supported)
    {
        set_textA(hwnd, EM_SETTEXTEX, textA);
        expect_textA(hwnd, WM_GETTEXT, textA);
        expect_textA(hwnd, EM_GETTEXTEX, textA);
        expect_textW(hwnd, EM_GETTEXTEX, textW);
    }

    if (!is_win9x)
    {
        set_textW(hwnd, WM_SETTEXT, textW);
        expect_textW(hwnd, WM_GETTEXT, textW);
        expect_textA(hwnd, WM_GETTEXT, textA);
        expect_textW(hwnd, EM_GETTEXTEX, textW);
        expect_textA(hwnd, EM_GETTEXTEX, textA);

        if (em_settextex_supported)
        {
            set_textW(hwnd, EM_SETTEXTEX, textW);
            expect_textW(hwnd, WM_GETTEXT, textW);
            expect_textA(hwnd, WM_GETTEXT, textA);
            expect_textW(hwnd, EM_GETTEXTEX, textW);
            expect_textA(hwnd, EM_GETTEXTEX, textA);
        }
    }
    DestroyWindow(hwnd);
}

static void test_WM_CHAR(void)
{
    HWND hwnd;
    int ret;
    const char * char_list = "abc\rabc\r";
    const char * expected_content_single = "abcabc";
    const char * expected_content_multi = "abc\r\nabc\r\n";
    char buffer[64] = {0};
    const char * p;

    /* single-line control must IGNORE carriage returns */
    hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP,
                           0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    p = char_list;
    while (*p != '\0') {
        SendMessageA(hwnd, WM_KEYDOWN, *p, 1);
        ret = SendMessageA(hwnd, WM_CHAR, *p, 1);
        ok(ret == 0, "WM_CHAR('%c') ret=%d\n", *p, ret);
        SendMessageA(hwnd, WM_KEYUP, *p, 1);
        p++;
    }

    SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    ret = strcmp(buffer, expected_content_single);
    ok(ret == 0, "WM_GETTEXT recovered incorrect string!\n");

    DestroyWindow(hwnd);

    /* multi-line control inserts CR normally */
    hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP|ES_MULTILINE,
                           0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    p = char_list;
    while (*p != '\0') {
        SendMessageA(hwnd, WM_KEYDOWN, *p, 1);
        ret = SendMessageA(hwnd, WM_CHAR, *p, 1);
        ok(ret == 0, "WM_CHAR('%c') ret=%d\n", *p, ret);
        SendMessageA(hwnd, WM_KEYUP, *p, 1);
        p++;
    }

    SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    ret = strcmp(buffer, expected_content_multi);
    ok(ret == 0, "WM_GETTEXT recovered incorrect string!\n");

    DestroyWindow(hwnd);
}

static void test_EM_GETTEXTLENGTHEX(void)
{
    HWND hwnd;
    GETTEXTLENGTHEX gtl;
    int ret;
    const char * base_string = "base string";
    const char * test_string = "a\nb\n\n\r\n";
    const char * test_string_after = "a";
    const char * test_string_2 = "a\rtest\rstring";
    char buffer[64] = {0};

    /* single line */
    if (!is_win9x)
        hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP,
                               0, 0, 200, 60, 0, 0, 0, 0);
    else
        hwnd = CreateWindowExA(0, "RichEdit20A", NULL, WS_POPUP,
                               0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 0, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 0, "ret %d\n",ret);

    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) base_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) test_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 1, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 1, "ret %d\n",ret);

    SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    ret = strcmp(buffer, test_string_after);
    ok(ret == 0, "WM_GETTEXT recovered incorrect string!\n");

    DestroyWindow(hwnd);

    /* multi line */
    if (!is_win9x)
        hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP | ES_MULTILINE,
                               0, 0, 200, 60, 0, 0, 0, 0);
    else
        hwnd = CreateWindowExA(0, "RichEdit20A", NULL, WS_POPUP | ES_MULTILINE,
                               0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 0, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 0, "ret %d\n",ret);

    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) base_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) test_string_2);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(test_string_2) + 2, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(test_string_2), "ret %d\n",ret);

    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) test_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 10, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 6, "ret %d\n",ret);

    DestroyWindow(hwnd);
}


/* globals that parent and child access when checking event masks & notifications */
static HWND eventMaskEditHwnd = 0;
static int queriedEventMask;
static int watchForEventMask = 0;

/* parent proc that queries the edit's event mask when it gets a WM_COMMAND */
static LRESULT WINAPI ParentMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_COMMAND && (watchForEventMask & (wParam >> 16)))
    {
      queriedEventMask = SendMessage(eventMaskEditHwnd, EM_GETEVENTMASK, 0, 0);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

/* test event masks in combination with WM_COMMAND */
static void test_eventMask(void)
{
    HWND parent;
    int ret, style;
    WNDCLASSA cls;
    const char text[] = "foo bar\n";
    int eventMask;

    /* register class to capture WM_COMMAND */
    cls.style = 0;
    cls.lpfnWndProc = ParentMsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "EventMaskParentClass";
    if(!RegisterClassA(&cls)) assert(0);

    parent = CreateWindow(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, NULL, NULL);
    ok (parent != 0, "Failed to create parent window\n");

    eventMaskEditHwnd = new_richedit(parent);
    ok(eventMaskEditHwnd != 0, "Failed to create edit window\n");

    eventMask = ENM_CHANGE | ENM_UPDATE;
    ret = SendMessage(eventMaskEditHwnd, EM_SETEVENTMASK, 0, (LPARAM) eventMask);
    ok(ret == ENM_NONE, "wrong event mask\n");
    ret = SendMessage(eventMaskEditHwnd, EM_GETEVENTMASK, 0, 0);
    ok(ret == eventMask, "failed to set event mask\n");

    /* check what happens when we ask for EN_CHANGE and send WM_SETTEXT */
    queriedEventMask = 0;  /* initialize to something other than we expect */
    watchForEventMask = EN_CHANGE;
    ret = SendMessage(eventMaskEditHwnd, WM_SETTEXT, 0, (LPARAM) text);
    ok(ret == TRUE, "failed to set text\n");
    /* richedit should mask off ENM_CHANGE when it sends an EN_CHANGE
       notification in response to WM_SETTEXT */
    ok(queriedEventMask == (eventMask & ~ENM_CHANGE),
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);

    /* check to see if EN_CHANGE is sent when redraw is turned off */
    SendMessage(eventMaskEditHwnd, WM_CLEAR, 0, 0);
    ok(IsWindowVisible(eventMaskEditHwnd), "Window should be visible.\n");
    SendMessage(eventMaskEditHwnd, WM_SETREDRAW, FALSE, 0);
    /* redraw is disabled by making the window invisible. */
    ok(!IsWindowVisible(eventMaskEditHwnd), "Window shouldn't be visible.\n");
    queriedEventMask = 0;  /* initialize to something other than we expect */
    SendMessage(eventMaskEditHwnd, EM_REPLACESEL, 0, (LPARAM) text);
    ok(queriedEventMask == (eventMask & ~ENM_CHANGE),
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);
    SendMessage(eventMaskEditHwnd, WM_SETREDRAW, TRUE, 0);
    ok(IsWindowVisible(eventMaskEditHwnd), "Window should be visible.\n");

    /* check to see if EN_UPDATE is sent when the editor isn't visible */
    SendMessage(eventMaskEditHwnd, WM_CLEAR, 0, 0);
    style = GetWindowLong(eventMaskEditHwnd, GWL_STYLE);
    SetWindowLong(eventMaskEditHwnd, GWL_STYLE, style & ~WS_VISIBLE);
    ok(!IsWindowVisible(eventMaskEditHwnd), "Window shouldn't be visible.\n");
    watchForEventMask = EN_UPDATE;
    queriedEventMask = 0;  /* initialize to something other than we expect */
    SendMessage(eventMaskEditHwnd, EM_REPLACESEL, 0, (LPARAM) text);
    ok(queriedEventMask == 0,
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);
    SetWindowLong(eventMaskEditHwnd, GWL_STYLE, style);
    ok(IsWindowVisible(eventMaskEditHwnd), "Window should be visible.\n");
    queriedEventMask = 0;  /* initialize to something other than we expect */
    SendMessage(eventMaskEditHwnd, EM_REPLACESEL, 0, (LPARAM) text);
    ok(queriedEventMask == eventMask,
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);


    DestroyWindow(parent);
}

static int received_WM_NOTIFY = 0;
static int modify_at_WM_NOTIFY = 0;
static HWND hwndRichedit_WM_NOTIFY;

static LRESULT WINAPI WM_NOTIFY_ParentMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_NOTIFY)
    {
      received_WM_NOTIFY = 1;
      modify_at_WM_NOTIFY = SendMessage(hwndRichedit_WM_NOTIFY, EM_GETMODIFY, 0, 0);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_WM_NOTIFY(void)
{
    HWND parent;
    WNDCLASSA cls;
    CHARFORMAT2 cf2;

    /* register class to capture WM_NOTIFY */
    cls.style = 0;
    cls.lpfnWndProc = WM_NOTIFY_ParentMsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "WM_NOTIFY_ParentClass";
    if(!RegisterClassA(&cls)) assert(0);

    parent = CreateWindow(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, NULL, NULL);
    ok (parent != 0, "Failed to create parent window\n");

    hwndRichedit_WM_NOTIFY = new_richedit(parent);
    ok(hwndRichedit_WM_NOTIFY != 0, "Failed to create edit window\n");

    SendMessage(hwndRichedit_WM_NOTIFY, EM_SETEVENTMASK, 0, ENM_SELCHANGE);

    /* Notifications for selection change should only be sent when selection
       actually changes. EM_SETCHARFORMAT is one message that calls
       ME_CommitUndo, which should check whether message should be sent */
    received_WM_NOTIFY = 0;
    cf2.cbSize = sizeof(CHARFORMAT2);
    SendMessage(hwndRichedit_WM_NOTIFY, EM_GETCHARFORMAT, (WPARAM) SCF_DEFAULT,
             (LPARAM) &cf2);
    cf2.dwMask = CFM_ITALIC | cf2.dwMask;
    cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;
    SendMessage(hwndRichedit_WM_NOTIFY, EM_SETCHARFORMAT, 0, (LPARAM) &cf2);
    ok(received_WM_NOTIFY == 0, "Unexpected WM_NOTIFY was sent!\n");

    /* WM_SETTEXT should NOT cause a WM_NOTIFY to be sent when selection is
       already at 0. */
    received_WM_NOTIFY = 0;
    modify_at_WM_NOTIFY = 0;
    SendMessage(hwndRichedit_WM_NOTIFY, WM_SETTEXT, 0, (LPARAM)"sometext");
    ok(received_WM_NOTIFY == 0, "Unexpected WM_NOTIFY was sent!\n");
    ok(modify_at_WM_NOTIFY == 0, "WM_NOTIFY callback saw text flagged as modified!\n");

    received_WM_NOTIFY = 0;
    modify_at_WM_NOTIFY = 0;
    SendMessage(hwndRichedit_WM_NOTIFY, EM_SETSEL, 4, 4);
    ok(received_WM_NOTIFY == 1, "Expected WM_NOTIFY was NOT sent!\n");

    received_WM_NOTIFY = 0;
    modify_at_WM_NOTIFY = 0;
    SendMessage(hwndRichedit_WM_NOTIFY, WM_SETTEXT, 0, (LPARAM)"sometext");
    ok(received_WM_NOTIFY == 1, "Expected WM_NOTIFY was NOT sent!\n");
    ok(modify_at_WM_NOTIFY == 0, "WM_NOTIFY callback saw text flagged as modified!\n");

    /* Test for WM_NOTIFY messages with redraw disabled. */
    SendMessage(hwndRichedit_WM_NOTIFY, EM_SETSEL, 0, 0);
    SendMessage(hwndRichedit_WM_NOTIFY, WM_SETREDRAW, FALSE, 0);
    received_WM_NOTIFY = 0;
    SendMessage(hwndRichedit_WM_NOTIFY, EM_REPLACESEL, FALSE, (LPARAM)"inserted");
    ok(received_WM_NOTIFY == 1, "Expected WM_NOTIFY was NOT sent!\n");
    SendMessage(hwndRichedit_WM_NOTIFY, WM_SETREDRAW, TRUE, 0);

    DestroyWindow(hwndRichedit_WM_NOTIFY);
    DestroyWindow(parent);
}

static void test_undo_coalescing(void)
{
    HWND hwnd;
    int result;
    char buffer[64] = {0};

    /* multi-line control inserts CR normally */
    if (!is_win9x)
        hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP|ES_MULTILINE,
                               0, 0, 200, 60, 0, 0, 0, 0);
    else
        hwnd = CreateWindowExA(0, "RichEdit20A", NULL, WS_POPUP|ES_MULTILINE,
                               0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    result = SendMessage(hwnd, EM_CANUNDO, 0, 0);
    ok (result == FALSE, "Can undo after window creation.\n");
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == FALSE, "Undo operation successful with nothing to undo.\n");
    result = SendMessage(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Can redo after window creation.\n");
    result = SendMessage(hwnd, EM_REDO, 0, 0);
    ok (result == FALSE, "Redo operation successful with nothing undone.\n");

    /* Test the effect of arrows keys during typing on undo transactions*/
    simulate_typing_characters(hwnd, "one two three");
    SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 1);
    SendMessage(hwnd, WM_KEYUP, VK_RIGHT, 1);
    simulate_typing_characters(hwnd, " four five six");

    result = SendMessage(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Can redo before anything is undone.\n");
    result = SendMessage(hwnd, EM_CANUNDO, 0, 0);
    ok (result == TRUE, "Cannot undo typed characters.\n");
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "EM_UNDO Failed to undo typed characters.\n");
    result = SendMessage(hwnd, EM_CANREDO, 0, 0);
    ok (result == TRUE, "Cannot redo after undo.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "one two three");
    ok (result == 0, "expected '%s' but got '%s'\n", "one two three", buffer);

    result = SendMessage(hwnd, EM_CANUNDO, 0, 0);
    ok (result == TRUE, "Cannot undo typed characters.\n");
    result = SendMessage(hwnd, WM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "");
    ok (result == 0, "expected '%s' but got '%s'\n", "", buffer);

    /* Test the effect of focus changes during typing on undo transactions*/
    simulate_typing_characters(hwnd, "one two three");
    result = SendMessage(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Redo buffer should have been cleared by typing.\n");
    SendMessage(hwnd, WM_KILLFOCUS, 0, 0);
    SendMessage(hwnd, WM_SETFOCUS, 0, 0);
    simulate_typing_characters(hwnd, " four five six");
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "one two three");
    ok (result == 0, "expected '%s' but got '%s'\n", "one two three", buffer);

    /* Test the effect of the back key during typing on undo transactions */
    SendMessage(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
    ok (result == TRUE, "Failed to clear the text.\n");
    simulate_typing_characters(hwnd, "one two threa");
    result = SendMessage(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Redo buffer should have been cleared by typing.\n");
    SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 1);
    SendMessage(hwnd, WM_KEYUP, VK_BACK, 1);
    simulate_typing_characters(hwnd, "e four five six");
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "");
    ok (result == 0, "expected '%s' but got '%s'\n", "", buffer);

    /* Test the effect of the delete key during typing on undo transactions */
    SendMessage(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"abcd");
    ok(result == TRUE, "Failed to set the text.\n");
    SendMessage(hwnd, EM_SETSEL, (WPARAM)1, (LPARAM)1);
    SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessage(hwnd, WM_KEYUP, VK_DELETE, 1);
    SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessage(hwnd, WM_KEYUP, VK_DELETE, 1);
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "acd");
    ok (result == 0, "expected '%s' but got '%s'\n", "acd", buffer);
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "abcd");
    ok (result == 0, "expected '%s' but got '%s'\n", "abcd", buffer);

    /* Test the effect of EM_STOPGROUPTYPING on undo transactions*/
    SendMessage(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
    ok (result == TRUE, "Failed to clear the text.\n");
    simulate_typing_characters(hwnd, "one two three");
    result = SendMessage(hwnd, EM_STOPGROUPTYPING, 0, 0);
    ok (result == 0, "expected %d but got %d\n", 0, result);
    simulate_typing_characters(hwnd, " four five six");
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "one two three");
    ok (result == 0, "expected '%s' but got '%s'\n", "one two three", buffer);
    result = SendMessage(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "");
    ok (result == 0, "expected '%s' but got '%s'\n", "", buffer);

    DestroyWindow(hwnd);
}

static LONG CALLBACK customWordBreakProc(WCHAR *text, int pos, int bytes, int code)
{
    int length;

    /* MSDN lied, length is actually the number of bytes. */
    length = bytes / sizeof(WCHAR);
    switch(code)
    {
        case WB_ISDELIMITER:
            return text[pos] == 'X';
        case WB_LEFT:
        case WB_MOVEWORDLEFT:
            if (customWordBreakProc(text, pos, bytes, WB_ISDELIMITER))
                return pos-1;
            return min(customWordBreakProc(text, pos, bytes, WB_LEFTBREAK)-1, 0);
        case WB_LEFTBREAK:
            pos--;
            while (pos > 0 && !customWordBreakProc(text, pos, bytes, WB_ISDELIMITER))
                pos--;
            return pos;
        case WB_RIGHT:
        case WB_MOVEWORDRIGHT:
            if (customWordBreakProc(text, pos, bytes, WB_ISDELIMITER))
                return pos+1;
            return min(customWordBreakProc(text, pos, bytes, WB_RIGHTBREAK)+1, length);
        case WB_RIGHTBREAK:
            pos++;
            while (pos < length && !customWordBreakProc(text, pos, bytes, WB_ISDELIMITER))
                pos++;
            return pos;
        default:
            ok(FALSE, "Unexpected code %d\n", code);
            break;
    }
    return 0;
}

#define SEND_CTRL_LEFT(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, VK_LEFT)
#define SEND_CTRL_RIGHT(hwnd) pressKeyWithModifier(hwnd, VK_CONTROL, VK_RIGHT)

static void test_word_movement(void)
{
    HWND hwnd;
    int result;
    int sel_start, sel_end;
    const WCHAR textW[] = {'o','n','e',' ','t','w','o','X','t','h','r','e','e',0};

    /* multi-line control inserts CR normally */
    hwnd = new_richedit(NULL);

    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"one two  three");
    ok (result == TRUE, "Failed to clear the text.\n");
    SendMessage(hwnd, EM_SETSEL, 0, 0);
    /* |one two three */

    SEND_CTRL_RIGHT(hwnd);
    /* one |two  three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 4, "Cursor is at %d instead of %d\n", sel_start, 4);

    SEND_CTRL_RIGHT(hwnd);
    /* one two  |three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 9, "Cursor is at %d instead of %d\n", sel_start, 9);

    SEND_CTRL_LEFT(hwnd);
    /* one |two  three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 4, "Cursor is at %d instead of %d\n", sel_start, 4);

    SEND_CTRL_LEFT(hwnd);
    /* |one two  three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 0, "Cursor is at %d instead of %d\n", sel_start, 0);

    SendMessage(hwnd, EM_SETSEL, 8, 8);
    /* one two | three */
    SEND_CTRL_RIGHT(hwnd);
    /* one two  |three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 9, "Cursor is at %d instead of %d\n", sel_start, 9);

    SendMessage(hwnd, EM_SETSEL, 11, 11);
    /* one two  th|ree */
    SEND_CTRL_LEFT(hwnd);
    /* one two  |three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 9, "Cursor is at %d instead of %d\n", sel_start, 9);

    /* Test with a custom word break procedure that uses X as the delimiter. */
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"one twoXthree");
    ok (result == TRUE, "Failed to clear the text.\n");
    SendMessage(hwnd, EM_SETWORDBREAKPROC, 0, (LPARAM)customWordBreakProc);
    /* |one twoXthree */
    SEND_CTRL_RIGHT(hwnd);
    /* one twoX|three */
    SendMessage(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 8, "Cursor is at %d instead of %d\n", sel_start, 8);

    DestroyWindow(hwnd);

    /* Make sure the behaviour is the same with a unicode richedit window,
     * and using unicode functions. */
    if (is_win9x)
    {
        skip("Cannot test with unicode richedit window\n");
        return;
    }

    hwnd = CreateWindowW(RICHEDIT_CLASS20W, NULL,
                        ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                        0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);

    /* Test with a custom word break procedure that uses X as the delimiter. */
    result = SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)textW);
    ok (result == TRUE, "Failed to clear the text.\n");
    SendMessageW(hwnd, EM_SETWORDBREAKPROC, 0, (LPARAM)customWordBreakProc);
    /* |one twoXthree */
    SEND_CTRL_RIGHT(hwnd);
    /* one twoX|three */
    SendMessageW(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 8, "Cursor is at %d instead of %d\n", sel_start, 8);

    DestroyWindow(hwnd);
}

static void test_EM_CHARFROMPOS(void)
{
    HWND hwnd;
    int result;
    RECT rcClient;
    POINTL point;
    point.x = 0;
    point.y = 40;

    /* multi-line control inserts CR normally */
    hwnd = new_richedit(NULL);
    result = SendMessageA(hwnd, WM_SETTEXT, 0,
                          (LPARAM)"one two three four five six seven\reight");

    GetClientRect(hwnd, &rcClient);

    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(result == 34, "expected character index of 34 but got %d\n", result);

    /* Test with points outside the bounds of the richedit control. */
    point.x = -1;
    point.y = 40;
    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 34, "expected character index of 34 but got %d\n", result);

    point.x = 1000;
    point.y = 0;
    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 33, "expected character index of 33 but got %d\n", result);

    point.x = 1000;
    point.y = 40;
    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 39, "expected character index of 39 but got %d\n", result);

    point.x = 1000;
    point.y = -1;
    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 0, "expected character index of 0 but got %d\n", result);

    point.x = 1000;
    point.y = rcClient.bottom + 1;
    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 34, "expected character index of 34 but got %d\n", result);

    point.x = 1000;
    point.y = rcClient.bottom;
    result = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 39, "expected character index of 39 but got %d\n", result);

    DestroyWindow(hwnd);
}

static void test_word_wrap(void)
{
    HWND hwnd;
    POINTL point = {0, 60}; /* This point must be below the first line */
    const char *text = "Must be long enough to test line wrapping";
    DWORD dwCommonStyle = WS_VISIBLE|WS_POPUP|WS_VSCROLL|ES_MULTILINE;
    int res, pos, lines;

    /* Test the effect of WS_HSCROLL and ES_AUTOHSCROLL styles on wrapping
     * when specified on window creation and set later. */
    hwnd = CreateWindow(RICHEDIT_CLASS, NULL, dwCommonStyle,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines > 1, "Line was expected to wrap (lines=%d).\n", lines);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL);
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindow(RICHEDIT_CLASS, NULL, dwCommonStyle|WS_HSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());

    res = SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line wasn't expected to wrap (lines=%d).\n", lines);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindow(RICHEDIT_CLASS, NULL, dwCommonStyle|ES_AUTOHSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindow(RICHEDIT_CLASS, NULL,
                        dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    /* Test the effect of EM_SETTARGETDEVICE on word wrap. */
    res = SendMessage(hwnd, EM_SETTARGETDEVICE, 0, 1);
    ok(res, "EM_SETTARGETDEVICE failed (returned %d).\n", res);
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    res = SendMessage(hwnd, EM_SETTARGETDEVICE, 0, 0);
    ok(res, "EM_SETTARGETDEVICE failed (returned %d).\n", res);
    pos = SendMessage(hwnd, EM_CHARFROMPOS, 0, (LPARAM) &point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    /* Test to see if wrapping happens with redraw disabled. */
    hwnd = CreateWindow(RICHEDIT_CLASS, NULL, dwCommonStyle,
                        0, 0, 400, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
    res = SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) text);
    ok(res, "EM_REPLACESEL failed.\n");
    lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line wasn't expected to wrap (lines=%d).\n", lines);
    MoveWindow(hwnd, 0, 0, 200, 80, FALSE);
    lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines > 1, "Line was expected to wrap (lines=%d).\n", lines);

    SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    DestroyWindow(hwnd);
}

static void test_autoscroll(void)
{
    HWND hwnd = new_richedit(NULL);
    int lines, ret, redraw;
    POINT pt;

    for (redraw = 0; redraw <= 1; redraw++) {
        trace("testing with WM_SETREDRAW=%d\n", redraw);
        SendMessage(hwnd, WM_SETREDRAW, redraw, 0);
        SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)"1\n2\n3\n4\n5\n6\n7\n8");
        lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
        ok(lines == 8, "%d lines instead of 8\n", lines);
        ret = SendMessage(hwnd, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        ok(ret == 1, "EM_GETSCROLLPOS returned %d instead of 1\n", ret);
        ok(pt.y != 0, "Didn't scroll down after replacing text.\n");
        ret = GetWindowLong(hwnd, GWL_STYLE);
        ok(ret & WS_VSCROLL, "Scrollbar was not shown yet (style=%x).\n", (UINT)ret);

        SendMessage(hwnd, WM_SETTEXT, 0, 0);
        lines = SendMessage(hwnd, EM_GETLINECOUNT, 0, 0);
        ok(lines == 1, "%d lines instead of 1\n", lines);
        ret = SendMessage(hwnd, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        ok(ret == 1, "EM_GETSCROLLPOS returned %d instead of 1\n", ret);
        ok(pt.y == 0, "y scroll position is %d after clearing text.\n", pt.y);
        ret = GetWindowLong(hwnd, GWL_STYLE);
        ok(!(ret & WS_VSCROLL), "Scrollbar is still shown (style=%x).\n", (UINT)ret);
    }

    SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
    DestroyWindow(hwnd);

    /* The WS_VSCROLL and WS_HSCROLL styles implicitly set
     * auto vertical/horizontal scrolling options. */
    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP|ES_MULTILINE|WS_VSCROLL|WS_HSCROLL,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    ret = SendMessage(hwnd, EM_GETOPTIONS, 0, 0);
    ok(ret & ECO_AUTOVSCROLL, "ECO_AUTOVSCROLL isn't set.\n");
    ok(ret & ECO_AUTOHSCROLL, "ECO_AUTOHSCROLL isn't set.\n");
    ret = GetWindowLong(hwnd, GWL_STYLE);
    ok(!(ret & ES_AUTOVSCROLL), "ES_AUTOVSCROLL is set.\n");
    ok(!(ret & ES_AUTOHSCROLL), "ES_AUTOHSCROLL is set.\n");
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP|ES_MULTILINE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    ret = SendMessage(hwnd, EM_GETOPTIONS, 0, 0);
    ok(!(ret & ECO_AUTOVSCROLL), "ECO_AUTOVSCROLL is set.\n");
    ok(!(ret & ECO_AUTOHSCROLL), "ECO_AUTOHSCROLL is set.\n");
    ret = GetWindowLong(hwnd, GWL_STYLE);
    ok(!(ret & ES_AUTOVSCROLL), "ES_AUTOVSCROLL is set.\n");
    ok(!(ret & ES_AUTOHSCROLL), "ES_AUTOHSCROLL is set.\n");
    DestroyWindow(hwnd);
}


static void test_format_rect(void)
{
    HWND hwnd;
    RECT rc, expected, clientRect;
    int n;
    DWORD options;

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());

    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    expected.left += 1;
    expected.right -= 1;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    for (n = -3; n <= 3; n++)
    {
      rc = clientRect;
      rc.top += n;
      rc.left += n;
      rc.bottom -= n;
      rc.right -= n;
      SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);

      expected = rc;
      expected.top = max(0, rc.top);
      expected.left = max(0, rc.left);
      expected.bottom = min(clientRect.bottom, rc.bottom);
      expected.right = min(clientRect.right, rc.right);
      SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
      ok(rc.top == expected.top && rc.left == expected.left &&
         rc.bottom == expected.bottom && rc.right == expected.right,
         "[n=%d] rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
         n, rc.top, rc.left, rc.bottom, rc.right,
         expected.top, expected.left, expected.bottom, expected.right);
    }

    rc = clientRect;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    expected = clientRect;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    /* Adding the selectionbar adds the selectionbar width to the left side. */
    SendMessageA(hwnd, EM_SETOPTIONS, ECOOP_OR, ECO_SELECTIONBAR);
    options = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(options & ECO_SELECTIONBAR, "EM_SETOPTIONS failed to add selectionbar.\n");
    expected.left += 8; /* selection bar width */
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    rc = clientRect;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    expected = clientRect;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    /* Removing the selectionbar subtracts the selectionbar width from the left side,
     * even if the left side is already 0. */
    SendMessageA(hwnd, EM_SETOPTIONS, ECOOP_AND, ~ECO_SELECTIONBAR);
    options = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(!(options & ECO_SELECTIONBAR), "EM_SETOPTIONS failed to remove selectionbar.\n");
    expected.left -= 8; /* selection bar width */
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    /* Set the absolute value of the formatting rectangle. */
    rc = clientRect;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    expected = clientRect;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "[n=%d] rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       n, rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    /* MSDN documents the EM_SETRECT message as using the rectangle provided in
     * LPARAM as being a relative offset when the WPARAM value is 1, but these
     * tests show that this isn't true. */
    rc.top = 15;
    rc.left = 15;
    rc.bottom = clientRect.bottom - 15;
    rc.right = clientRect.right - 15;
    expected = rc;
    SendMessageA(hwnd, EM_SETRECT, 1, (LPARAM)&rc);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    /* For some reason it does not limit the values to the client rect with
     * a WPARAM value of 1. */
    rc.top = -15;
    rc.left = -15;
    rc.bottom = clientRect.bottom + 15;
    rc.right = clientRect.right + 15;
    expected = rc;
    SendMessageA(hwnd, EM_SETRECT, 1, (LPARAM)&rc);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    DestroyWindow(hwnd);

    /* The extended window style affects the formatting rectangle. */
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());

    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    expected.left += 1;
    expected.top += 1;
    expected.right -= 1;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    rc = clientRect;
    rc.top += 5;
    rc.left += 5;
    rc.bottom -= 5;
    rc.right -= 5;
    expected = rc;
    expected.top -= 1;
    expected.left -= 1;
    expected.right += 1;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(rc.top == expected.top && rc.left == expected.left &&
       rc.bottom == expected.bottom && rc.right == expected.right,
       "rect a(t=%d, l=%d, b=%d, r=%d) != e(t=%d, l=%d, b=%d, r=%d)\n",
       rc.top, rc.left, rc.bottom, rc.right,
       expected.top, expected.left, expected.bottom, expected.right);

    DestroyWindow(hwnd);
}

static void test_WM_GETDLGCODE(void)
{
    HWND hwnd;
    UINT res, expected;
    MSG msg;

    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|ES_WANTRETURN|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, 0);
    expected = expected | DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    msg.message = WM_KEYDOWN;
    msg.wParam = VK_RETURN;
    msg.lParam = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC) | 0x0001;
    msg.pt.x = 0;
    msg.pt.y = 0;
    msg.time = GetTickCount();

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|ES_WANTRETURN|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = expected | DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_WANTRETURN|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    msg.wParam = VK_TAB;
    msg.lParam = MapVirtualKey(VK_TAB, MAPVK_VK_TO_VSC) | 0x0001;

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hold_key(VK_CONTROL);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    release_key(VK_CONTROL);

    msg.wParam = 'a';
    msg.lParam = MapVirtualKey('a', MAPVK_VK_TO_VSC) | 0x0001;

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    msg.message = WM_CHAR;

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessage(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);
}

static void test_zoom(void)
{
    HWND hwnd;
    UINT ret;
    RECT rc;
    POINT pt;
    int numerator, denominator;

    hwnd = new_richedit(NULL);
    GetClientRect(hwnd, &rc);
    pt.x = (rc.right - rc.left) / 2;
    pt.y = (rc.bottom - rc.top) / 2;
    ClientToScreen(hwnd, &pt);

    /* Test initial zoom value */
    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 0, "Numerator should be initialized to 0 (got %d).\n", numerator);
    ok(denominator == 0, "Denominator should be initialized to 0 (got %d).\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* test scroll wheel */
    hold_key(VK_CONTROL);
    ret = SendMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 110, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Test how much the mouse wheel can zoom in and out. */
    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)490, (LPARAM)100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 500, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)491, (LPARAM)100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 491, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)20, (LPARAM)100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, -WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 10, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)19, (LPARAM)100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, -WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 19, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Test how WM_SCROLLWHEEL treats our custom denominator. */
    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)50, (LPARAM)13);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessage(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 394, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Test bounds checking on EM_SETZOOM */
    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)2, (LPARAM)127);
    ok(ret == TRUE, "EM_SETZOOM rejected valid values (%d).\n", ret);

    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)127, (LPARAM)2);
    ok(ret == TRUE, "EM_SETZOOM rejected valid values (%d).\n", ret);

    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)2, (LPARAM)128);
    ok(ret == FALSE, "EM_SETZOOM accepted invalid values (%d).\n", ret);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 127, "incorrect numerator is %d\n", numerator);
    ok(denominator == 2, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)128, (LPARAM)2);
    ok(ret == FALSE, "EM_SETZOOM accepted invalid values (%d).\n", ret);

    /* See if negative numbers are accepted. */
    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)-100, (LPARAM)-100);
    ok(ret == FALSE, "EM_SETZOOM accepted invalid values (%d).\n", ret);

    /* See if negative numbers are accepted. */
    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)0, (LPARAM)100);
    ok(ret == FALSE, "EM_SETZOOM failed (%d).\n", ret);

    ret = SendMessage(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 127, "incorrect numerator is %d\n", numerator);
    ok(denominator == 2, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Reset the zoom value */
    ret = SendMessage(hwnd, EM_SETZOOM, (WPARAM)0, (LPARAM)0);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    DestroyWindow(hwnd);
}

START_TEST( editor )
{
  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED20.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibrary("RICHED20.DLL");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  is_win9x = GetVersion() & 0x80000000;

  test_WM_CHAR();
  test_EM_FINDTEXT();
  test_EM_GETLINE();
  test_EM_POSFROMCHAR();
  test_EM_SCROLLCARET();
  test_EM_SCROLL();
  test_scrollbar_visibility();
  test_WM_SETTEXT();
  test_EM_LINELENGTH();
  test_EM_SETCHARFORMAT();
  test_EM_SETTEXTMODE();
  test_TM_PLAINTEXT();
  test_EM_SETOPTIONS();
  test_WM_GETTEXT();
  test_EM_GETTEXTRANGE();
  test_EM_GETSELTEXT();
  test_EM_SETUNDOLIMIT();
  test_ES_PASSWORD();
  test_EM_SETTEXTEX();
  test_EM_LIMITTEXT();
  test_EM_EXLIMITTEXT();
  test_EM_GETLIMITTEXT();
  test_WM_SETFONT();
  test_EM_GETMODIFY();
  test_EM_EXSETSEL();
  test_WM_PASTE();
  test_EM_STREAMIN();
  test_EM_STREAMOUT();
  test_EM_StreamIn_Undo();
  test_EM_FORMATRANGE();
  test_unicode_conversions();
  test_EM_GETTEXTLENGTHEX();
  test_EM_REPLACESEL(1);
  test_EM_REPLACESEL(0);
  test_WM_NOTIFY();
  test_EM_AUTOURLDETECT();
  test_eventMask();
  test_undo_coalescing();
  test_word_movement();
  test_EM_CHARFROMPOS();
  test_SETPARAFORMAT();
  test_word_wrap();
  test_autoscroll();
  test_format_rect();
  test_WM_GETDLGCODE();
  test_zoom();

  /* Set the environment variable WINETEST_RICHED20 to keep windows
   * responsive and open for 30 seconds. This is useful for debugging.
   */
  if (getenv( "WINETEST_RICHED20" )) {
    keep_responsive(30);
  }

  OleFlushClipboard();
  ok(FreeLibrary(hmoduleRichEdit) != 0, "error: %d\n", (int) GetLastError());
}
