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

#define COBJMACROS

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
#include <richole.h>
#include <commdlg.h>
#include <time.h>
#include <wine/test.h>

#define ID_RICHEDITTESTDBUTTON 0x123

static CHAR string1[MAX_PATH], string2[MAX_PATH], string3[MAX_PATH];

#define ok_w3(format, szString1, szString2, szString3) \
    WideCharToMultiByte(CP_ACP, 0, szString1, -1, string1, MAX_PATH, NULL, NULL); \
    WideCharToMultiByte(CP_ACP, 0, szString2, -1, string2, MAX_PATH, NULL, NULL); \
    WideCharToMultiByte(CP_ACP, 0, szString3, -1, string3, MAX_PATH, NULL, NULL); \
    ok(!lstrcmpW(szString3, szString1) || !lstrcmpW(szString3, szString2), \
       format, string1, string2, string3);

static HMODULE hmoduleRichEdit;
static BOOL is_lang_japanese;

static HWND new_window(LPCSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindowA(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 200, 60, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_windowW(LPCWSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindowW(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 200, 60, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", wine_dbgstr_w(lpClassName), (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent) {
  return new_window(RICHEDIT_CLASS20A, ES_MULTILINE, parent);
}

static HWND new_richedit_with_style(HWND parent, DWORD style) {
  return new_window(RICHEDIT_CLASS20A, style, parent);
}

static HWND new_richeditW(HWND parent) {
  return new_windowW(RICHEDIT_CLASS20W, ES_MULTILINE, parent);
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
      if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
      } else {
        Sleep(50);
      }
    }
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

static WCHAR *atowstr(const char *str)
{
    WCHAR *ret;
    DWORD len;
    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static void check_EM_FINDTEXT(HWND hwnd, const char *name, struct find_s *f, int id, BOOL unicode)
{
  int findloc;

  if(unicode){
      FINDTEXTW ftw;
      memset(&ftw, 0, sizeof(ftw));
      ftw.chrg.cpMin = f->start;
      ftw.chrg.cpMax = f->end;
      ftw.lpstrText = atowstr(f->needle);

      findloc = SendMessageA(hwnd, EM_FINDTEXT, f->flags, (LPARAM)&ftw);
      ok(findloc == f->expected_loc,
         "EM_FINDTEXT(%s,%d,%u) '%s' in range(%d,%d), flags %08x, got start at %d, expected %d\n",
         name, id, unicode, f->needle, f->start, f->end, f->flags, findloc, f->expected_loc);

      findloc = SendMessageA(hwnd, EM_FINDTEXTW, f->flags, (LPARAM)&ftw);
      ok(findloc == f->expected_loc,
         "EM_FINDTEXTW(%s,%d,%u) '%s' in range(%d,%d), flags %08x, got start at %d, expected %d\n",
         name, id, unicode, f->needle, f->start, f->end, f->flags, findloc, f->expected_loc);

      HeapFree(GetProcessHeap(), 0, (void*)ftw.lpstrText);
  }else{
      FINDTEXTA fta;
      memset(&fta, 0, sizeof(fta));
      fta.chrg.cpMin = f->start;
      fta.chrg.cpMax = f->end;
      fta.lpstrText = f->needle;

      findloc = SendMessageA(hwnd, EM_FINDTEXT, f->flags, (LPARAM)&fta);
      ok(findloc == f->expected_loc,
         "EM_FINDTEXT(%s,%d,%u) '%s' in range(%d,%d), flags %08x, got start at %d, expected %d\n",
         name, id, unicode, f->needle, f->start, f->end, f->flags, findloc, f->expected_loc);
  }
}

static void check_EM_FINDTEXTEX(HWND hwnd, const char *name, struct find_s *f,
    int id, BOOL unicode)
{
  int findloc;
  int expected_end_loc;

  if(unicode){
      FINDTEXTEXW ftw;
      memset(&ftw, 0, sizeof(ftw));
      ftw.chrg.cpMin = f->start;
      ftw.chrg.cpMax = f->end;
      ftw.lpstrText = atowstr(f->needle);
      findloc = SendMessageA(hwnd, EM_FINDTEXTEX, f->flags, (LPARAM)&ftw);
      ok(findloc == f->expected_loc,
          "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
          name, id, f->needle, f->start, f->end, f->flags, findloc);
      ok(ftw.chrgText.cpMin == f->expected_loc,
          "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
          name, id, f->needle, f->start, f->end, f->flags, ftw.chrgText.cpMin);
      expected_end_loc = ((f->expected_loc == -1) ? -1
            : f->expected_loc + strlen(f->needle));
      ok(ftw.chrgText.cpMax == expected_end_loc,
          "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, end at %d, expected %d\n",
          name, id, f->needle, f->start, f->end, f->flags, ftw.chrgText.cpMax, expected_end_loc);
      HeapFree(GetProcessHeap(), 0, (void*)ftw.lpstrText);
  }else{
      FINDTEXTEXA fta;
      memset(&fta, 0, sizeof(fta));
      fta.chrg.cpMin = f->start;
      fta.chrg.cpMax = f->end;
      fta.lpstrText = f->needle;
      findloc = SendMessageA(hwnd, EM_FINDTEXTEX, f->flags, (LPARAM)&fta);
      ok(findloc == f->expected_loc,
          "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
          name, id, f->needle, f->start, f->end, f->flags, findloc);
      ok(fta.chrgText.cpMin == f->expected_loc,
          "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, start at %d\n",
          name, id, f->needle, f->start, f->end, f->flags, fta.chrgText.cpMin);
      expected_end_loc = ((f->expected_loc == -1) ? -1
            : f->expected_loc + strlen(f->needle));
      ok(fta.chrgText.cpMax == expected_end_loc,
          "EM_FINDTEXTEX(%s,%d) '%s' in range(%d,%d), flags %08x, end at %d, expected %d\n",
          name, id, f->needle, f->start, f->end, f->flags, fta.chrgText.cpMax, expected_end_loc);
  }
}

static void run_tests_EM_FINDTEXT(HWND hwnd, const char *name, struct find_s *find,
    int num_tests, BOOL unicode)
{
  int i;

  for (i = 0; i < num_tests; i++) {
      check_EM_FINDTEXT(hwnd, name, &find[i], i, unicode);
      check_EM_FINDTEXTEX(hwnd, name, &find[i], i, unicode);
  }
}

static void test_EM_FINDTEXT(BOOL unicode)
{
  HWND hwndRichEdit;
  CHARFORMAT2A cf2;

  if(unicode)
       hwndRichEdit = new_richeditW(NULL);
  else
       hwndRichEdit = new_richedit(NULL);

  /* Empty rich edit control */
  run_tests_EM_FINDTEXT(hwndRichEdit, "1", find_tests, ARRAY_SIZE(find_tests), unicode);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)haystack);

  /* Haystack text */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2", find_tests2, ARRAY_SIZE(find_tests2), unicode);

  /* Setting a format on an arbitrary range should have no effect in search
     results. This tests correct offset reporting across runs. */
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf2);
  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;
  SendMessageA(hwndRichEdit, EM_SETSEL, 6, 20);
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Haystack text, again */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2-bis", find_tests2, ARRAY_SIZE(find_tests2), unicode);

  /* Yet another range */
  cf2.dwMask = CFM_BOLD | cf2.dwMask;
  cf2.dwEffects = CFE_BOLD ^ cf2.dwEffects;
  SendMessageA(hwndRichEdit, EM_SETSEL, 11, 15);
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Haystack text, again */
  run_tests_EM_FINDTEXT(hwndRichEdit, "2-bisbis", find_tests2, ARRAY_SIZE(find_tests2), unicode);

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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);

  memset(origdest, 0xBB, nBuf);
  for (i = 0; i < ARRAY_SIZE(gl); i++)
  {
    int nCopied;
    int expected_nCopied = min(gl[i].buffer_len, strlen(gl[i].text));
    int expected_bytes_written = min(gl[i].buffer_len, strlen(gl[i].text));
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
       * Windows NT does not append a NULL terminating character, but
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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);

  for (i = 0; i < 10; i++) {
    result = SendMessageA(hwndRichEdit, EM_LINELENGTH, offset_test[i][0], 0);
    ok(result == offset_test[i][1], "Length of line at offset %d is %ld, expected %d\n",
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
    int offset_test1[3][2] = {
           {0, 4},  /* Line 1: |wine\n */
           {5, 9},  /* Line 2: |richedit\x8e\xf0\n */
           {15, 4}, /* Line 3: |wine */
    };
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);
    for (i = 0; i < ARRAY_SIZE(offset_test1); i++) {
      result = SendMessageA(hwndRichEdit, EM_LINELENGTH, offset_test1[i][0], 0);
      ok(result == offset_test1[i][1], "Length of line at offset %d is %ld, expected %d\n",
         offset_test1[i][0], result, offset_test1[i][1]);
    }
  }

  DestroyWindow(hwndRichEdit);
}

static int get_scroll_pos_y(HWND hwnd)
{
  POINT p = {-1, -1};
  SendMessageA(hwnd, EM_GETSCROLLPOS, 0, (LPARAM)&p);
  ok(p.x != -1 && p.y != -1, "p.x:%d p.y:%d\n", p.x, p.y);
  return p.y;
}

static void move_cursor(HWND hwnd, LONG charindex)
{
  CHARRANGE cr;
  cr.cpMax = charindex;
  cr.cpMin = charindex;
  SendMessageA(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);
}

static void line_scroll(HWND hwnd, int amount)
{
  SendMessageA(hwnd, EM_LINESCROLL, 0, amount);
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
  HWND hwndRichEdit = CreateWindowA(RICHEDIT_CLASS20A, NULL,
                                   ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                                   0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
  ok(hwndRichEdit != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());

  /* Can't verify this */
  SendMessageA(hwndRichEdit, EM_SCROLLCARET, 0, 0);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);

  /* Caret above visible window */
  line_scroll(hwndRichEdit, 3);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessageA(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY != curY, "%d == %d\n", prevY, curY);

  /* Caret below visible window */
  move_cursor(hwndRichEdit, sizeof(text) - 1);
  line_scroll(hwndRichEdit, -3);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessageA(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY != curY, "%d == %d\n", prevY, curY);

  /* Caret in visible window */
  move_cursor(hwndRichEdit, sizeof(text) - 2);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessageA(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY == curY, "%d != %d\n", prevY, curY);

  /* Caret still in visible window */
  line_scroll(hwndRichEdit, -1);
  prevY = get_scroll_pos_y(hwndRichEdit);
  SendMessageA(hwndRichEdit, EM_SCROLLCARET, 0, 0);
  curY = get_scroll_pos_y(hwndRichEdit);
  ok(prevY == curY, "%d != %d\n", prevY, curY);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_POSFROMCHAR(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  int i, expected;
  LRESULT result;
  unsigned int height = 0;
  int xpos = 0;
  POINTL pt;
  LOCALESIGNATURE sig;
  BOOL rtl;
  PARAFORMAT2 fmt;
  static const char text[] = "aa\n"
      "this is a long line of text that should be longer than the "
      "control's width\n"
      "cc\n"
      "dd\n"
      "ee\n"
      "ff\n"
      "gg\n"
      "hh\n";

  rtl = (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_FONTSIGNATURE,
                        (LPSTR) &sig, sizeof(LOCALESIGNATURE)) &&
         (sig.lsUsb[3] & 0x08000000) != 0);

  /* Fill the control to lines to ensure that most of them are offscreen */
  for (i = 0; i < 50; i++)
  {
    /* Do not modify the string; it is exactly 16 characters long. */
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 0);
    SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"0123456789ABCDE\n");
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
    result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, i * 16, 0);
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
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, 50 * 16, 0);
  ok(HIWORD(result) == 50 * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), 50 * height);
  ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));

  /* Testing position way past end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, 55 * 16, 0);
  ok(HIWORD(result) == 50 * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), 50 * height);
  expected = (rtl ? 8 : 1);
  ok(LOWORD(result) == expected, "EM_POSFROMCHAR reports x=%d, expected %d\n", LOWORD(result), expected);

  /* Testing that vertical scrolling does, in fact, have an effect on EM_POSFROMCHAR */
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0); /* line down */
  for (i = 0; i < 50; i++)
  {
    /* All the lines are 16 characters long */
    result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, i * 16, 0);
    ok((signed short)(HIWORD(result)) == (i - 1) * height,
        "EM_POSFROMCHAR reports y=%hd, expected %d\n",
        (signed short)(HIWORD(result)), (i - 1) * height);
    ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
  }

  /* Testing position at end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, 50 * 16, 0);
  ok(HIWORD(result) == (50 - 1) * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), (50 - 1) * height);
  ok(LOWORD(result) == xpos, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));

  /* Testing position way past end of text */
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, 55 * 16, 0);
  ok(HIWORD(result) == (50 - 1) * height, "EM_POSFROMCHAR reports y=%d, expected %d\n", HIWORD(result), (50 - 1) * height);
  expected = (rtl ? 8 : 1);
  ok(LOWORD(result) == expected, "EM_POSFROMCHAR reports x=%d, expected %d\n", LOWORD(result), expected);

  /* Testing that horizontal scrolling does, in fact, have an effect on EM_POSFROMCHAR */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0); /* line up */

  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, 0, 0);
  ok(HIWORD(result) == 0, "EM_POSFROMCHAR reports y=%d, expected 0\n", HIWORD(result));
  ok(LOWORD(result) == 1, "EM_POSFROMCHAR reports x=%d, expected 1\n", LOWORD(result));
  xpos = LOWORD(result);

  SendMessageA(hwndRichEdit, WM_HSCROLL, SB_LINERIGHT, 0);
  result = SendMessageA(hwndRichEdit, EM_POSFROMCHAR, 0, 0);
  ok(HIWORD(result) == 0, "EM_POSFROMCHAR reports y=%d, expected 0\n", HIWORD(result));
  ok((signed short)(LOWORD(result)) < xpos,
        "EM_POSFROMCHAR reports x=%hd, expected value less than %d\n",
        (signed short)(LOWORD(result)), xpos);
  SendMessageA(hwndRichEdit, WM_HSCROLL, SB_LINELEFT, 0);

  /* Test around end of text that doesn't end in a newline. */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"12345678901234");
  SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt,
              SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0)-1);
  ok(pt.x > 1, "pt.x = %d\n", pt.x);
  xpos = pt.x;
  SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt,
              SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0));
  ok(pt.x > xpos, "pt.x = %d\n", pt.x);
  xpos = (rtl ? pt.x + 7 : pt.x);
  SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt,
              SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0)+1);
  ok(pt.x == xpos, "pt.x = %d\n", pt.x);

  /* Try a negative position. */
  SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt, -1);
  ok(pt.x == 1, "pt.x = %d\n", pt.x);

  /* test negative indentation */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0,
          (LPARAM)"{\\rtf1\\pard\\fi-200\\li-200\\f1 TestSomeText\\par}");
  SendMessageA(hwndRichEdit, EM_POSFROMCHAR, (WPARAM)&pt, 0);
  ok(pt.x == 1, "pt.x = %d\n", pt.x);

  fmt.cbSize = sizeof(fmt);
  SendMessageA(hwndRichEdit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt);
  ok(fmt.dxStartIndent == -400, "got %d\n", fmt.dxStartIndent);
  ok(fmt.dxOffset == 200, "got %d\n", fmt.dxOffset);
  ok(fmt.wAlignment == PFA_LEFT, "got %d\n", fmt.wAlignment);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_SETCHARFORMAT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2A cf2;
  CHARFORMAT2W cfW;
  CHARFORMATA cf1a;
  CHARFORMATW cf1w;
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
  LOCALESIGNATURE sig;
  BOOL rtl;
  DWORD expect_effects;

  rtl = (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_FONTSIGNATURE,
                        (LPSTR) &sig, sizeof(LOCALESIGNATURE)) &&
         (sig.lsUsb[3] & 0x08000000) != 0);

  /* check charformat defaults */
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(cf2.dwMask == CFM_ALL2, "got %08x\n", cf2.dwMask);
  expect_effects = CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR;
  if (cf2.wWeight > 550) expect_effects |= CFE_BOLD;
  ok(cf2.dwEffects == expect_effects, "got %08x\n", cf2.dwEffects);
  ok(cf2.yOffset == 0, "got %d\n", cf2.yOffset);
  ok(cf2.sSpacing == 0, "got %d\n", cf2.sSpacing);
  ok(cf2.lcid == GetSystemDefaultLCID(), "got %x\n", cf2.lcid);
  ok(cf2.sStyle == 0, "got %d\n", cf2.sStyle);
  ok(cf2.wKerning == 0, "got %d\n", cf2.wKerning);
  ok(cf2.bAnimation == 0, "got %d\n", cf2.bAnimation);
  ok(cf2.bRevAuthor == 0, "got %d\n", cf2.bRevAuthor);

  /* Invalid flags, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)0xfffffff0, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_WORD, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* A valid flag, CHARFORMAT2 structure blanked out */
  memset(&cf2, 0, sizeof(cf2));
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  /* Invalid flags, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)0xfffffff0, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == FALSE, "Should not be able to undo here.\n");
  SendMessageA(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == FALSE, "Should not be able to undo here.\n");
  SendMessageA(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == FALSE, "Should not be able to undo here.\n");
  SendMessageA(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_WORD, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  todo_wine ok(rc == TRUE, "Should not be able to undo here.\n");
  SendMessageA(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  /* A valid flag, CHARFORMAT2 structure minimally filled */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok(rc == TRUE, "Should not be able to undo here.\n");
  SendMessageA(hwndRichEdit, EM_EMPTYUNDOBUFFER, 0, 0);

  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf2);

  /* Test state of modify flag before and after valid EM_SETCHARFORMAT */
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf2);
  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;

  /* wParam==0 is default char format, does not set modify */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  if (! rtl)
  {
    rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
    ok(rc == 0, "Text marked as modified, expected not modified!\n");
  }
  else
    skip("RTL language found\n");

  /* wParam==SCF_SELECTION sets modify if nonempty selection */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == -1, "Text not marked as modified, expected modified! (%d)\n", rc);

  /* wParam==SCF_ALL sets modify regardless of whether text is present */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);
  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == -1, "Text not marked as modified, expected modified! (%d)\n", rc);

  DestroyWindow(hwndRichEdit);

  /* EM_GETCHARFORMAT tests */
  for (i = 0; tested_effects[i]; i++)
  {
    hwndRichEdit = new_richedit(NULL);
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

    /* Need to set a TrueType font to get consistent CFM_BOLD results */
    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    cf2.dwMask = CFM_FACE|CFM_WEIGHT;
    cf2.dwEffects = 0;
    strcpy(cf2.szFaceName, "Courier New");
    cf2.wWeight = FW_DONTCARE;
    SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 4);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == 0,
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x clear\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    cf2.dwMask = tested_effects[i];
    if (cf2.dwMask == CFE_SUBSCRIPT || cf2.dwMask == CFE_SUPERSCRIPT)
      cf2.dwMask = CFM_SUPERSCRIPT;
    cf2.dwEffects = tested_effects[i];
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
    SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == tested_effects[i],
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 2, 4);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == 0,
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x clear\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 1, 3);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
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
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

    /* Need to set a TrueType font to get consistent CFM_BOLD results */
    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    cf2.dwMask = CFM_FACE|CFM_WEIGHT;
    cf2.dwEffects = 0;
    strcpy(cf2.szFaceName, "Courier New");
    cf2.wWeight = FW_DONTCARE;
    SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    cf2.dwMask = tested_effects[i];
    if (cf2.dwMask == CFE_SUBSCRIPT || cf2.dwMask == CFE_SUPERSCRIPT)
      cf2.dwMask = CFM_SUPERSCRIPT;
    cf2.dwEffects = tested_effects[i];
    SendMessageA(hwndRichEdit, EM_SETSEL, 2, 4);
    SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == 0,
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x clear\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 2, 4);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
    ok ((((tested_effects[i] == CFE_SUBSCRIPT || tested_effects[i] == CFE_SUPERSCRIPT) &&
          (cf2.dwMask & CFM_SUPERSCRIPT) == CFM_SUPERSCRIPT)
          ||
          (cf2.dwMask & tested_effects[i]) == tested_effects[i]),
        "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, tested_effects[i]);
    ok((cf2.dwEffects & tested_effects[i]) == tested_effects[i],
        "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, tested_effects[i]);

    memset(&cf2, 0, sizeof(CHARFORMAT2A));
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichEdit, EM_SETSEL, 1, 3);
    SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Selection is now nonempty */
  SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  ok (((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == CFE_BOLD,
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);


  /* Set two effects on an empty selection */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  /* first clear bold, italic */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD | CFM_ITALIC;
  cf2.dwEffects = 0;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  cf2.dwMask = CFM_ITALIC;
  cf2.dwEffects = CFE_ITALIC;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Selection is now nonempty */
  SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  ok (((cf2.dwMask & (CFM_BOLD|CFM_ITALIC)) == (CFM_BOLD|CFM_ITALIC)),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, (CFM_BOLD|CFM_ITALIC));
  ok((cf2.dwEffects & (CFE_BOLD|CFE_ITALIC)) == (CFE_BOLD|CFE_ITALIC),
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, (CFE_BOLD|CFE_ITALIC));

  /* Setting the (empty) selection to exactly the same place as before should
     NOT clear the insertion style! */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  /* first clear bold, italic */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD | CFM_ITALIC;
  cf2.dwEffects = 0;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Empty selection in same place, insert style should NOT be forgotten here. */
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 2);

  /* Selection is now nonempty */
  SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  ok (((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == CFE_BOLD,
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);

  /* Moving the selection will clear the insertion style */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  /* first clear bold, italic */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD | CFM_ITALIC;
  cf2.dwEffects = 0;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Move selection and then put it back, insert style should be forgotten here. */
  SendMessageA(hwndRichEdit, EM_SETSEL, 3, 3);
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 2); /* Empty selection */

  /* Selection is now nonempty */
  SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_SETSEL, 2, 6);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  ok(((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == 0,
      "%d, cf2.dwEffects == 0x%08x not expecting effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);

  /* Ditto with EM_EXSETSEL */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");
  /* first clear bold, italic */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD | CFM_ITALIC;
  cf2.dwEffects = 0;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  cr.cpMin = 2; cr.cpMax = 2;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr); /* Empty selection */

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD;
  cf2.dwEffects = CFE_BOLD;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* Empty selection in same place, insert style should NOT be forgotten here. */
  cr.cpMin = 2; cr.cpMax = 2;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr); /* Empty selection */

  /* Selection is now nonempty */
  SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"newi");

  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cr.cpMin = 2; cr.cpMax = 6;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr); /* Empty selection */
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  ok (((cf2.dwMask & CFM_BOLD) == CFM_BOLD),
      "%d, cf2.dwMask == 0x%08x expected mask 0x%08x\n", i, cf2.dwMask, CFM_BOLD);
  ok((cf2.dwEffects & CFE_BOLD) == CFE_BOLD,
      "%d, cf2.dwEffects == 0x%08x expected effect 0x%08x\n", i, cf2.dwEffects, CFE_BOLD);

  /* show that wWeight is at the correct offset in CHARFORMAT2A */
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(cf2);
  cf2.dwMask = CFM_WEIGHT;
  cf2.wWeight = 100;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(cf2);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(cf2.wWeight == 100, "got %d\n", cf2.wWeight);

  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(cf2);
  cf2.dwMask = CFM_SPACING;
  cf2.sSpacing = 10;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(cf2));
  cf2.cbSize = sizeof(cf2);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(cf2.sSpacing == 10, "got %d\n", cf2.sSpacing);

  /* show that wWeight is at the correct offset in CHARFORMAT2W */
  memset(&cfW, 0, sizeof(cfW));
  cfW.cbSize = sizeof(cfW);
  cfW.dwMask = CFM_WEIGHT;
  cfW.wWeight = 100;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfW);
  memset(&cfW, 0, sizeof(cfW));
  cfW.cbSize = sizeof(cfW);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfW);
  ok(cfW.wWeight == 100, "got %d\n", cfW.wWeight);

  memset(&cfW, 0, sizeof(cfW));
  cfW.cbSize = sizeof(cfW);
  cfW.dwMask = CFM_SPACING;
  cfW.sSpacing = 10;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfW);
  memset(&cfW, 0, sizeof(cfW));
  cfW.cbSize = sizeof(cfW);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfW);
  ok(cfW.sSpacing == 10, "got %d\n", cfW.sSpacing);

  /* test CFE_UNDERLINE and bUnderlineType interaction */
  /* clear bold, italic */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  cf2.dwMask = CFM_BOLD | CFM_ITALIC;
  cf2.dwEffects = 0;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /* check CFE_UNDERLINE is clear and bUnderlineType is CFU_UNDERLINE */
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok((cf2.dwMask & (CFM_UNDERLINE | CFM_UNDERLINETYPE)) == (CFM_UNDERLINE | CFM_UNDERLINETYPE),
     "got %08x\n", cf2.dwMask);
  ok(!(cf2.dwEffects & CFE_UNDERLINE), "got %08x\n", cf2.dwEffects);
  ok(cf2.bUnderlineType == CFU_UNDERLINE, "got %x\n", cf2.bUnderlineType);

  /* simply touching bUnderlineType will toggle CFE_UNDERLINE */
  cf2.dwMask = CFM_UNDERLINETYPE;
  cf2.bUnderlineType = CFU_UNDERLINE;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok((cf2.dwMask & (CFM_UNDERLINE | CFM_UNDERLINETYPE)) == (CFM_UNDERLINE | CFM_UNDERLINETYPE),
     "got %08x\n", cf2.dwMask);
  ok(cf2.dwEffects & CFE_UNDERLINE, "got %08x\n", cf2.dwEffects);
  ok(cf2.bUnderlineType == CFU_UNDERLINE, "got %x\n", cf2.bUnderlineType);

  /* setting bUnderline to CFU_UNDERLINENONE clears CFE_UNDERLINE */
  cf2.dwMask = CFM_UNDERLINETYPE;
  cf2.bUnderlineType = CFU_UNDERLINENONE;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok((cf2.dwMask & (CFM_UNDERLINE | CFM_UNDERLINETYPE)) == (CFM_UNDERLINE | CFM_UNDERLINETYPE),
     "got %08x\n", cf2.dwMask);
  ok(!(cf2.dwEffects & CFE_UNDERLINE), "got %08x\n", cf2.dwEffects);
  ok(cf2.bUnderlineType == CFU_UNDERLINENONE, "got %x\n", cf2.bUnderlineType);

  /* another underline type also sets CFE_UNDERLINE */
  cf2.dwMask = CFM_UNDERLINETYPE;
  cf2.bUnderlineType = CFU_UNDERLINEDOUBLE;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok((cf2.dwMask & (CFM_UNDERLINE | CFM_UNDERLINETYPE)) == (CFM_UNDERLINE | CFM_UNDERLINETYPE),
     "got %08x\n", cf2.dwMask);
  ok(cf2.dwEffects & CFE_UNDERLINE, "got %08x\n", cf2.dwEffects);
  ok(cf2.bUnderlineType == CFU_UNDERLINEDOUBLE, "got %x\n", cf2.bUnderlineType);

  /* However explicitly clearing CFE_UNDERLINE results in it remaining cleared */
  cf2.dwMask = CFM_UNDERLINETYPE | CFM_UNDERLINE;
  cf2.bUnderlineType = CFU_UNDERLINEDOUBLE;
  cf2.dwEffects = 0;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok((cf2.dwMask & (CFM_UNDERLINE | CFM_UNDERLINETYPE)) == (CFM_UNDERLINE | CFM_UNDERLINETYPE),
     "got %08x\n", cf2.dwMask);
  ok(!(cf2.dwEffects & CFE_UNDERLINE), "got %08x\n", cf2.dwEffects);
  ok(cf2.bUnderlineType == CFU_UNDERLINEDOUBLE, "got %x\n", cf2.bUnderlineType);

  /* And turing it back on again by just setting CFE_UNDERLINE */
  cf2.dwMask = CFM_UNDERLINE;
  cf2.dwEffects = CFE_UNDERLINE;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  memset(&cf2, 0, sizeof(CHARFORMAT2A));
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok((cf2.dwMask & (CFM_UNDERLINE | CFM_UNDERLINETYPE)) == (CFM_UNDERLINE | CFM_UNDERLINETYPE),
     "got %08x\n", cf2.dwMask);
  ok(cf2.dwEffects & CFE_UNDERLINE, "got %08x\n", cf2.dwEffects);
  ok(cf2.bUnderlineType == CFU_UNDERLINEDOUBLE, "got %x\n", cf2.bUnderlineType);

  /* Check setting CFM_ALL2/CFM_EFFECTS2 in CHARFORMAT(A/W). */
  memset(&cf1a, 0, sizeof(CHARFORMATA));
  memset(&cf1w, 0, sizeof(CHARFORMATW));
  cf1a.cbSize = sizeof(CHARFORMATA);
  cf1w.cbSize = sizeof(CHARFORMATW);
  cf1a.dwMask = cf1w.dwMask = CFM_ALL2;
  cf1a.dwEffects = cf1w.dwEffects = CFM_EFFECTS2;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf1a);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf1a);
  /* flags only valid for CHARFORMAT2 should be masked out */
  ok((cf1a.dwMask & (CFM_ALL2 & ~CFM_ALL)) == 0, "flags were not masked out\n");
  ok((cf1a.dwEffects & (CFM_EFFECTS2 & ~CFM_EFFECTS)) == 0, "flags were not masked out\n");
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf1w);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf1w);
  ok((cf1w.dwMask & (CFM_ALL2 & ~CFM_ALL)) == 0, "flags were not masked out\n");
  ok((cf1w.dwEffects & (CFM_EFFECTS2 & ~CFM_EFFECTS)) == 0, "flags were not masked out\n");

  DestroyWindow(hwndRichEdit);
}

static void test_EM_SETTEXTMODE(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2A cf2, cf2test;
  CHARRANGE cr;
  int rc = 0;

  /*Attempt to use mutually exclusive modes*/
  rc = SendMessageA(hwndRichEdit, EM_SETTEXTMODE, (WPARAM)TM_PLAINTEXT|TM_RICHTEXT, 0);
  ok(rc == E_INVALIDARG,
     "EM_SETTEXTMODE: using mutually exclusive mode flags - returned: %x\n", rc);

  /*Test that EM_SETTEXTMODE fails if text exists within the control*/
  /*Insert text into the control*/

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

  /*Attempt to change the control to plain text mode*/
  rc = SendMessageA(hwndRichEdit, EM_SETTEXTMODE, (WPARAM)TM_PLAINTEXT, 0);
  ok(rc == E_UNEXPECTED,
     "EM_SETTEXTMODE: changed text mode in control containing text - returned: %x\n", rc);

  /*Test that EM_SETTEXTMODE does not allow rich edit text to be pasted.
  If rich text is pasted, it should have the same formatting as the rest
  of the text in the control*/

  /*Italicize the text
  *NOTE: If the default text was already italicized, the test will simply
  reverse; in other words, it will copy a regular "wine" into a plain
  text window that uses an italicized format*/
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf2);

  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;

  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == 0, "Text marked as modified, expected not modified!\n");

  /*EM_SETCHARFORMAT is not yet fully implemented for all WPARAMs in wine;
  however, SCF_ALL has been implemented*/
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  rc = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok(rc == -1, "Text not marked as modified, expected modified! (%d)\n", rc);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

  /*Select the string "wine"*/
  cr.cpMin = 0;
  cr.cpMax = 4;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /*Copy the italicized "wine" to the clipboard*/
  SendMessageA(hwndRichEdit, WM_COPY, 0, 0);

  /*Reset the formatting to default*/
  cf2.dwEffects = CFE_ITALIC^cf2.dwEffects;
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  /*Clear the text in the control*/
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");

  /*Switch to Plain Text Mode*/
  rc = SendMessageA(hwndRichEdit, EM_SETTEXTMODE, (WPARAM)TM_PLAINTEXT, 0);
  ok(rc == 0, "EM_SETTEXTMODE: unable to switch to plain text mode with empty control:  returned: %d\n", rc);

  /*Input "wine" again in normal format*/
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

  /*Paste the italicized "wine" into the control*/
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);

  /*Select a character from the first "wine" string*/
  cr.cpMin = 2;
  cr.cpMax = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /*Retrieve its formatting*/
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf2);

  /*Select a character from the second "wine" string*/
  cr.cpMin = 5;
  cr.cpMax = 6;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /*Retrieve its formatting*/
  cf2test.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf2test);

  /*Compare the two formattings*/
    ok((cf2.dwMask == cf2test.dwMask) && (cf2.dwEffects == cf2test.dwEffects),
      "two formats found in plain text mode - cf2.dwEffects: %x cf2test.dwEffects: %x\n",
       cf2.dwEffects, cf2test.dwEffects);
  /*Test TM_RICHTEXT by: switching back to Rich Text mode
                         printing "wine" in the current format(normal)
                         pasting "wine" from the clipboard(italicized)
                         comparing the two formats(should differ)*/

  /*Attempt to switch with text in control*/
  rc = SendMessageA(hwndRichEdit, EM_SETTEXTMODE, (WPARAM)TM_RICHTEXT, 0);
  ok(rc != 0, "EM_SETTEXTMODE: changed from plain text to rich text with text in control - returned: %d\n", rc);

  /*Clear control*/
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");

  /*Switch into Rich Text mode*/
  rc = SendMessageA(hwndRichEdit, EM_SETTEXTMODE, (WPARAM)TM_RICHTEXT, 0);
  ok(rc == 0, "EM_SETTEXTMODE: unable to change to rich text with empty control - returned: %d\n", rc);

  /*Print "wine" in normal formatting into the control*/
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

  /*Paste italicized "wine" into the control*/
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);

  /*Select text from the first "wine" string*/
  cr.cpMin = 1;
  cr.cpMax = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /*Retrieve its formatting*/
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf2);

  /*Select text from the second "wine" string*/
  cr.cpMin = 6;
  cr.cpMax = 7;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /*Retrieve its formatting*/
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf2test);

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

  ret = SendMessageA(hwndRichEdit, EM_SETPARAFORMAT, 0, (LPARAM)&fmt);
  ok(ret != 0, "expected non-zero got %d\n", ret);

  fmt.cbSize = sizeof(PARAFORMAT2);
  fmt.dwMask = -1;
  ret = SendMessageA(hwndRichEdit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt);
  /* Ignore the PFM_TABLEROWDELIMITER bit because it changes
   * between richedit different native builds of riched20.dll
   * used on different Windows versions. */
  ret &= ~PFM_TABLEROWDELIMITER;
  fmt.dwMask &= ~PFM_TABLEROWDELIMITER;

  ok(ret == expectedMask, "expected %x got %x\n", expectedMask, ret);
  ok(fmt.dwMask == expectedMask, "expected %x got %x\n", expectedMask, fmt.dwMask);

  /* Test some other paraformat field defaults */
  ok( fmt.wNumbering == 0, "got %d\n", fmt.wNumbering );
  ok( fmt.wNumberingStart == 0, "got %d\n", fmt.wNumberingStart );
  ok( fmt.wNumberingStyle == 0, "got %04x\n", fmt.wNumberingStyle );
  ok( fmt.wNumberingTab == 0, "got %d\n", fmt.wNumberingTab );

  DestroyWindow(hwndRichEdit);
}

static void test_TM_PLAINTEXT(void)
{
  /*Tests plain text properties*/

  HWND hwndRichEdit = new_richedit(NULL);
  CHARFORMAT2A cf2, cf2test;
  CHARRANGE cr;
  int rc = 0;

  /*Switch to plain text mode*/

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  SendMessageA(hwndRichEdit, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

  /*Fill control with text*/

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"Is Wine an emulator? No it's not");

  /*Select some text and bold it*/

  cr.cpMin = 10;
  cr.cpMax = 20;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf2);

  cf2.dwMask = CFM_BOLD | cf2.dwMask;
  cf2.dwEffects = CFE_BOLD ^ cf2.dwEffects;

  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_WORD | SCF_SELECTION, (LPARAM)&cf2);
  ok(rc == 0, "EM_SETCHARFORMAT returned %d instead of 0\n", rc);

  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  /*Get the formatting of those characters*/

  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /*Get the formatting of some other characters*/
  cf2test.cbSize = sizeof(CHARFORMAT2A);
  cr.cpMin = 21;
  cr.cpMax = 30;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2test);

  /*Test that they are the same as plain text allows only one formatting*/

  ok((cf2.dwMask == cf2test.dwMask) && (cf2.dwEffects == cf2test.dwEffects),
     "two selections' formats differ - cf2.dwMask: %x, cf2test.dwMask %x, cf2.dwEffects: %x, cf2test.dwEffects: %x\n",
     cf2.dwMask, cf2test.dwMask, cf2.dwEffects, cf2test.dwEffects);
  
  /*Fill the control with a "wine" string, which when inserted will be bold*/

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

  /*Copy the bolded "wine" string*/

  cr.cpMin = 0;
  cr.cpMax = 4;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  SendMessageA(hwndRichEdit, WM_COPY, 0, 0);

  /*Swap back to rich text*/

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  SendMessageA(hwndRichEdit, EM_SETTEXTMODE, TM_RICHTEXT, 0);

  /*Set the default formatting to bold italics*/

  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf2);
  cf2.dwMask |= CFM_ITALIC;
  cf2.dwEffects ^= CFE_ITALIC;
  rc = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf2);
  ok(rc == 1, "EM_SETCHARFORMAT returned %d instead of 1\n", rc);

  /*Set the text in the control to "wine", which will be bold and italicized*/

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"wine");

  /*Paste the plain text "wine" string, which should take the insert
   formatting, which at the moment is bold italics*/

  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);

  /*Select the first "wine" string and retrieve its formatting*/

  cr.cpMin = 1;
  cr.cpMax = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);

  /*Select the second "wine" string and retrieve its formatting*/

  cr.cpMin = 5;
  cr.cpMax = 7;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2test);

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
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    ok(result == lstrlenA(buffer),
        "WM_GETTEXT returned %d, expected %d\n", result, lstrlenA(buffer));
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,text);
    ok(result == 0, 
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    /* Test for returned value of WM_GETTEXTLENGTH */
    result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlenA(text),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlenA(text));

    /* Test for behavior in overflow case */
    memset(buffer, 0, 1024);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, strlen(text), (LPARAM)buffer);
    ok(result == 0 ||
       result == lstrlenA(text) - 1, /* XP, win2k3 */
        "WM_GETTEXT returned %d, expected 0 or %d\n", result, lstrlenA(text) - 1);
    result = strcmp(buffer,text);
    if (result)
        result = strncmp(buffer, text, lstrlenA(text) - 1); /* XP, win2k3 */
    ok(result == 0,
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    /* Baseline test with normal-sized buffer and carriage return */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    ok(result == lstrlenA(buffer),
        "WM_GETTEXT returned %d, expected %d\n", result, lstrlenA(buffer));
    result = strcmp(buffer,text2_after);
    ok(result == 0,
        "WM_GETTEXT: settext and gettext differ. strcmp: %d\n", result);

    /* Test for returned value of WM_GETTEXTLENGTH */
    result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlenA(text2_after),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlenA(text2_after));

    /* Test for behavior of CRLF conversion in case of overflow */
    memset(buffer, 0, 1024);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, strlen(text2), (LPARAM)buffer);
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

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 11;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 4;
    textRange.chrg.cpMax = 11;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 7, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* cpMax of text length is used instead of -1 in this case */
    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 0;
    textRange.chrg.cpMax = -1;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == strlen(text2), "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(text2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* cpMin < 0 causes no text to be copied, and 0 to be returned */
    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = -1;
    textRange.chrg.cpMax = 1;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 0, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(text2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* cpMax of -1 is not replaced with text length if cpMin != 0 */
    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 1;
    textRange.chrg.cpMax = -1;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 0, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(text2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* no end character is copied if cpMax - cpMin < 0 */
    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 5;
    textRange.chrg.cpMax = 5;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == 0, "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(text2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* cpMax of text length is used if cpMax > text length*/
    textRange.lpstrText = buffer;
    textRange.chrg.cpMin = 0;
    textRange.chrg.cpMax = 1000;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
    ok(result == strlen(text2), "EM_GETTEXTRANGE returned %ld\n", result);
    ok(!strcmp(text2, buffer), "EM_GETTEXTRANGE filled %s\n", buffer);

    /* Test with multibyte character */
    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        textRange.chrg.cpMin = 4;
        textRange.chrg.cpMax = 8;
        result = SendMessageA(hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&textRange);
        todo_wine ok(result == 5, "EM_GETTEXTRANGE returned %ld\n", result);
        todo_wine ok(!strcmp("ef\x8e\xf0g", buffer), "EM_GETTEXTRANGE filled %s\n", buffer);
    }

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

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);

    SendMessageA(hwndRichEdit, EM_SETSEL, 4, 11);
    result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 7, "EM_GETSELTEXT returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETSELTEXT filled %s\n", buffer);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);

    SendMessageA(hwndRichEdit, EM_SETSEL, 4, 11);
    result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
    ok(result == 7, "EM_GETSELTEXT returned %ld\n", result);
    ok(!strcmp(expect, buffer), "EM_GETSELTEXT filled %s\n", buffer);

    /* Test with multibyte character */
    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        SendMessageA(hwndRichEdit, EM_SETSEL, 4, 8);
        result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, 0, (LPARAM)buffer);
        todo_wine ok(result == 5, "EM_GETSELTEXT returned %ld\n", result);
        todo_wine ok(!strcmp("ef\x8e\xf0g", buffer), "EM_GETSELTEXT filled %s\n", buffer);
    }

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
    hwndRichEdit = CreateWindowA(RICHEDIT_CLASS20A, NULL, WS_POPUP,
                                0, 0, 200, 60, NULL, NULL,
                                hmoduleRichEdit, NULL);
    ok(hwndRichEdit != NULL, "class: %s, error: %d\n",
       RICHEDIT_CLASS20A, (int) GetLastError());
    options = SendMessageA(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    ok(options == 0, "Incorrect initial options %x\n", options);
    DestroyWindow(hwndRichEdit);

    hwndRichEdit = CreateWindowA(RICHEDIT_CLASS20A, NULL,
                                WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                                0, 0, 200, 60, NULL, NULL,
                                hmoduleRichEdit, NULL);
    ok(hwndRichEdit != NULL, "class: %s, error: %d\n",
       RICHEDIT_CLASS20A, (int) GetLastError());
    options = SendMessageA(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    /* WS_[VH]SCROLL cause the ECO_AUTO[VH]SCROLL options to be set */
    ok(options == (ECO_AUTOVSCROLL|ECO_AUTOHSCROLL),
       "Incorrect initial options %x\n", options);

    /* NEGATIVE TESTING - NO OPTIONS SET */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
    SendMessageA(hwndRichEdit, EM_SETOPTIONS, ECOOP_SET, 0);

    /* testing no readonly by sending 'a' to the control*/
    SendMessageA(hwndRichEdit, WM_CHAR, 'a', 0x1E0001);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    ok(buffer[0]=='a', 
       "EM_SETOPTIONS: Text not changed! s1:%s s2:%s\n", text, buffer);
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);

    /* READONLY - sending 'a' to the control */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
    SendMessageA(hwndRichEdit, EM_SETOPTIONS, ECOOP_SET, ECO_READONLY);
    SendMessageA(hwndRichEdit, WM_CHAR, 'a', 0x1E0001);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    ok(buffer[0]==text[0], 
       "EM_SETOPTIONS: Text changed! s1:%s s2:%s\n", text, buffer); 

    /* EM_SETOPTIONS changes the window style, but changing the
     * window style does not change the options. */
    dwStyle = GetWindowLongA(hwndRichEdit, GWL_STYLE);
    ok(dwStyle & ES_READONLY, "Readonly style not set by EM_SETOPTIONS\n");
    SetWindowLongA(hwndRichEdit, GWL_STYLE, dwStyle & ~ES_READONLY);
    options = SendMessageA(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    ok(options & ES_READONLY, "Readonly option set by SetWindowLong\n");
    /* Confirm that the text is still read only. */
    SendMessageA(hwndRichEdit, WM_CHAR, 'a', ('a' << 16) | 0x0001);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    ok(buffer[0]==text[0],
       "EM_SETOPTIONS: Text changed! s1:%s s2:%s\n", text, buffer);

    oldOptions = options;
    SetWindowLongA(hwndRichEdit, GWL_STYLE, dwStyle|optionStyles);
    options = SendMessageA(hwndRichEdit, EM_GETOPTIONS, 0, 0);
    ok(options == oldOptions,
       "Options set by SetWindowLong (%x -> %x)\n", oldOptions, options);

    DestroyWindow(hwndRichEdit);
}

static BOOL check_CFE_LINK_selection(HWND hwnd, int sel_start, int sel_end)
{
  CHARFORMAT2A text_format;
  text_format.cbSize = sizeof(text_format);
  SendMessageA(hwnd, EM_SETSEL, sel_start, sel_end);
  SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&text_format);
  return (text_format.dwEffects & CFE_LINK) != 0;
}

static void check_CFE_LINK_rcvd(HWND hwnd, BOOL is_url, const char * url)
{
  BOOL link_present = FALSE;

  link_present = check_CFE_LINK_selection(hwnd, 0, 1);
  if (is_url) 
  { /* control text is url; should get CFE_LINK */
    ok(link_present, "URL Case: CFE_LINK not set for [%s].\n", url);
  }
  else 
  {
    ok(!link_present, "Non-URL Case: CFE_LINK set for [%s].\n", url);
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
    BOOL is_url;
  } urls[12] = {
    {"winehq.org", FALSE},
    {"http://www.winehq.org", TRUE},
    {"http//winehq.org", FALSE},
    {"ww.winehq.org", FALSE},
    {"www.winehq.org", TRUE},
    {"ftp://192.168.1.1", TRUE},
    {"ftp//192.168.1.1", FALSE},
    {"mailto:your@email.com", TRUE},
    {"prospero:prosperoserver", TRUE},
    {"telnet:test", TRUE},
    {"news:newserver", TRUE},
    {"wais:waisserver", TRUE}
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
    "This is some text with _X on it",
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
    "This is some text with X_ on it",
  };
  /* These delims act as neutral breaks.  Whether the url is ended
     or not depends on the next non-neutral character.  We'll test
     with Y unchanged, in which case the url should include the
     deliminator and the Y.  We'll also test with the Y changed
     to a space, in which case the url stops before the
     deliminator. */
  const char * templates_neutral_delim[] = {
    "This is some text with X-Y on it",
    "This is some text with X--Y on it",
    "This is some text with X!Y on it",
    "This is some text with X[Y on it",
    "This is some text with X]Y on it",
    "This is some text with X{Y on it",
    "This is some text with X}Y on it",
    "This is some text with X(Y on it",
    "This is some text with X)Y on it",
    "This is some text with X\"Y on it",
    "This is some text with X;Y on it",
    "This is some text with X:Y on it",
    "This is some text with X'Y on it",
    "This is some text with X?Y on it",
    "This is some text with X<Y on it",
    "This is some text with X>Y on it",
    "This is some text with X.Y on it",
    "This is some text with X,Y on it",
  };
  char buffer[1024];

  parent = new_static_wnd(NULL);
  hwndRichEdit = new_richedit(parent);
  /* Try and pass EM_AUTOURLDETECT some test wParam values */
  urlRet=SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, FALSE, 0);
  ok(urlRet==0, "Good wParam: urlRet is: %d\n", urlRet);
  urlRet=SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, 1, 0);
  ok(urlRet==0, "Good wParam2: urlRet is: %d\n", urlRet);
  /* Windows returns -2147024809 (0x80070057) on bad wParam values */
  urlRet=SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, 8, 0);
  ok(urlRet==E_INVALIDARG, "Bad wParam: urlRet is: %d\n", urlRet);
  urlRet=SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, (WPARAM)"h", (LPARAM)"h");
  ok(urlRet==E_INVALIDARG, "Bad wParam2: urlRet is: %d\n", urlRet);
  /* for each url, check the text to see if CFE_LINK effect is present */
  for (i = 0; i < ARRAY_SIZE(urls); i++) {

    SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, FALSE, 0);
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)urls[i].text);
    check_CFE_LINK_rcvd(hwndRichEdit, FALSE, urls[i].text);

    /* Link detection should happen immediately upon WM_SETTEXT */
    SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)urls[i].text);
    check_CFE_LINK_rcvd(hwndRichEdit, urls[i].is_url, urls[i].text);
  }
  DestroyWindow(hwndRichEdit);

  /* Test detection of URLs within normal text - WM_SETTEXT case. */
  for (i = 0; i < ARRAY_SIZE(urls); i++) {
    hwndRichEdit = new_richedit(parent);

    for (j = 0; j < ARRAY_SIZE(templates_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      memcpy(buffer, templates_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)buffer);

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

    for (j = 0; j < ARRAY_SIZE(templates_non_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_non_delim[j], 'X');
      at_offset = at_pos - templates_non_delim[j];
      memcpy(buffer, templates_non_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_non_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)buffer);

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

    for (j = 0; j < ARRAY_SIZE(templates_xten_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_xten_delim[j], 'X');
      at_offset = at_pos - templates_xten_delim[j];
      memcpy(buffer, templates_xten_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_xten_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)buffer);

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

    for (j = 0; j < ARRAY_SIZE(templates_neutral_delim); j++) {
      char * at_pos, * end_pos;
      int at_offset;
      int end_offset;

      if (!urls[i].is_url) continue;

      at_pos = strchr(templates_neutral_delim[j], 'X');
      at_offset = at_pos - templates_neutral_delim[j];
      memcpy(buffer, templates_neutral_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_neutral_delim[j] + at_offset + 1);

      end_pos = strchr(buffer, 'Y');
      end_offset = end_pos - buffer;

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)buffer);

      /* This assumes no templates start with the URL itself, and that they
         have at least two characters before the URL text */
      ok(!check_CFE_LINK_selection(hwndRichEdit, 0, 1),
         "CFE_LINK incorrectly set in (%d-%d), text: %s\n", 0, 1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -2, at_offset -1),
         "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -2, at_offset -1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, at_offset -1, at_offset),
         "CFE_LINK incorrectly set in (%d-%d), text: %s\n", at_offset -1, at_offset, buffer);

      ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
         "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
      ok(check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
         "CFE_LINK not set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      ok(check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
         "CFE_LINK not set in (%d-%d), text: %s\n", end_offset, end_offset +1, buffer);

      *end_pos = ' ';

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)buffer);

      ok(check_CFE_LINK_selection(hwndRichEdit, at_offset, at_offset +1),
         "CFE_LINK not set in (%d-%d), text: %s\n", at_offset, at_offset +1, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset -1, end_offset),
         "CFE_LINK set in (%d-%d), text: %s\n", end_offset -1, end_offset, buffer);
      ok(!check_CFE_LINK_selection(hwndRichEdit, end_offset, end_offset +1),
         "CFE_LINK set in (%d-%d), text: %s\n", end_offset, end_offset +1, buffer);
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

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
      for (u = 0; templates_delim[j][u]; u++) {
        if (templates_delim[j][u] == '\r') {
          simulate_typing_characters(hwndRichEdit, "\r");
        } else if (templates_delim[j][u] != 'X') {
          SendMessageA(hwndRichEdit, WM_CHAR, templates_delim[j][u], 1);
        } else {
          for (v = 0; urls[i].text[v]; v++) {
            SendMessageA(hwndRichEdit, WM_CHAR, urls[i].text[v], 1);
          }
        }
      }
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset+1, at_offset+1);
      simulate_typing_characters(hwndRichEdit, "\r");
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset+2, at_offset+2);
      simulate_typing_characters(hwndRichEdit, "\b");
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
    for (j = 0; j < ARRAY_SIZE(templates_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      st.codepage = CP_ACP;
      st.flags = ST_DEFAULT;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      memcpy(buffer, templates_delim[j], at_offset);
      buffer[at_offset] = '\0';
      strcat(buffer, urls[i].text);
      strcat(buffer, templates_delim[j] + at_offset + 1);
      end_offset = at_offset + strlen(urls[i].text);

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)buffer);

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
    for (j = 0; j < ARRAY_SIZE(templates_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      st.codepage = CP_ACP;
      st.flags = ST_DEFAULT;
      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)templates_delim[j]);
      st.flags = ST_SELECTION;
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)urls[i].text);
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
    for (j = 0; j < ARRAY_SIZE(templates_delim); j++) {
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
      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)templates_delim[j]);
      st.flags = ST_SELECTION;
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)buffer);
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset+1, at_offset+2);
      SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)(urls[i].text + 1));
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
    for (j = 0; j < ARRAY_SIZE(templates_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)templates_delim[j]);
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)urls[i].text);
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
    for (j = 0; j < ARRAY_SIZE(templates_delim); j++) {
      char * at_pos;
      int at_offset;
      int end_offset;

      at_pos = strchr(templates_delim[j], 'X');
      at_offset = at_pos - templates_delim[j];
      end_offset = at_offset + strlen(urls[i].text);

      strcpy(buffer, "YY");
      buffer[0] = urls[i].text[0];

      SendMessageA(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)templates_delim[j]);
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset, at_offset+1);
      SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)buffer);
      SendMessageA(hwndRichEdit, EM_SETSEL, at_offset+1, at_offset+2);
      SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)(urls[i].text + 1));
      SendMessageA(hwndRichEdit, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);

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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");/* one line of text */
  expr = 0x00010000;
  for (i = 0; i < 4; i++) {
    static const int cmd[4] = { SB_PAGEDOWN, SB_PAGEUP, SB_LINEDOWN, SB_LINEUP };

    r = SendMessageA(hwndRichEdit, EM_SCROLL, cmd[i], 0);
    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
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
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a\nb\nc\nd\ne");
    else
      SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)
                  "a LONG LINE LONG LINE LONG LINE LONG LINE LONG LINE "
                  "LONG LINE LONG LINE LONG LINE LONG LINE LONG LINE "
                  "LONG LINE \nb\nc\nd\ne");
    for (j = 0; j < 12; j++) /* reset scroll position to top */
      SendMessageA(hwndRichEdit, EM_SCROLL, SB_PAGEUP, 0);

    /* get first visible line */
    y_before = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_SCROLL, SB_PAGEDOWN, 0); /* page down */

    /* get new current first visible line */
    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(((r & 0xffffff00) == 0x00010000) &&
       ((r & 0x000000ff) != 0x00000000),
       "EM_SCROLL page down didn't scroll by a small positive number of "
       "lines (r == 0x%08x)\n", r);
    ok(y_after > y_before, "EM_SCROLL page down not functioning "
       "(line %d scrolled to line %d\n", y_before, y_after);

    y_before = y_after;
    
    r = SendMessageA(hwndRichEdit, EM_SCROLL, SB_PAGEUP, 0); /* page up */
    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    ok(((r & 0xffffff00) == 0x0001ff00),
       "EM_SCROLL page up didn't scroll by a small negative number of lines "
       "(r == 0x%08x)\n", r);
    ok(y_after < y_before, "EM_SCROLL page up not functioning (line "
       "%d scrolled to line %d\n", y_before, y_after);
    
    y_before = y_after;

    r = SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0); /* line down */

    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010001, "EM_SCROLL line down didn't scroll by one line "
       "(r == 0x%08x)\n", r);
    ok(y_after -1 == y_before, "EM_SCROLL line down didn't go down by "
       "1 line (%d scrolled to %d)\n", y_before, y_after);

    y_before = y_after;

    r = SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0); /* line up */

    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x0001ffff, "EM_SCROLL line up didn't scroll by one line "
       "(r == 0x%08x)\n", r);
    ok(y_after +1 == y_before, "EM_SCROLL line up didn't go up by 1 "
       "line (%d scrolled to %d)\n", y_before, y_after);

    y_before = y_after;

    r = SendMessageA(hwndRichEdit, EM_SCROLL,
                    SB_LINEUP, 0); /* lineup beyond top */

    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL line up returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL line up beyond top worked (%d)\n", y_after);

    y_before = y_after;

    r = SendMessageA(hwndRichEdit, EM_SCROLL,
                    SB_PAGEUP, 0);/*page up beyond top */

    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL page up returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL page up beyond top worked (%d)\n", y_after);

    for (j = 0; j < 12; j++) /* page down all the way to the bottom */
      SendMessageA(hwndRichEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    y_before = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_SCROLL,
                    SB_PAGEDOWN, 0); /* page down beyond bot */
    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL page down returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL page down beyond bottom worked (%d -> %d)\n",
       y_before, y_after);

    y_before = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0); /* line down beyond bot */
    y_after = SendMessageA(hwndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    ok(r == 0x00010000,
       "EM_SCROLL line down returned indicating movement (0x%08x)\n", r);
    ok(y_before == y_after,
       "EM_SCROLL line down beyond bottom worked (%d -> %d)\n",
       y_before, y_after);
  }
  DestroyWindow(hwndRichEdit);
}

static unsigned int recursionLevel = 0;
static unsigned int WM_SIZE_recursionLevel = 0;
static BOOL bailedOutOfRecursion = FALSE;
static LRESULT (WINAPI *richeditProc)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
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
  hwndRichEdit = new_window(RICHEDIT_CLASS20A, ES_MULTILINE|ES_DISABLENOSCROLL, NULL);

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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && si.nMax == 1,
        "reported page/range is %d (%d..%d) expected 0 (0..1)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax > 1,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a\na");
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0);
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
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d)\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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


  /* Test behavior with explicit visibility request, using SetWindowLongA()() */
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a\na");
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
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) != 0),
    "Vertical scrollbar is invisible, should be visible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage == 0 && si.nMin == 0 && (si.nMax == 0 || si.nMax == 100),
        "reported page/range is %d (%d..%d) expected all 0 or nMax=100\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
  memset(&si, 0, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = SIF_PAGE | SIF_RANGE;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok (((GetWindowLongA(hwndRichEdit, GWL_STYLE) & WS_VSCROLL) == 0),
    "Vertical scrollbar is visible, should be invisible.\n");
  ok(si.nPage != 0 && si.nMin == 0 && si.nMax != 0,
        "reported page/range is %d (%d..%d) expected nMax/nPage nonzero\n",
        si.nPage, si.nMin, si.nMax);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
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
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEDOWN, 0);
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
  SendMessageA(hwndRichEdit, EM_SCROLL, SB_LINEUP, 0);
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
  r = GetClassInfoA(NULL, RICHEDIT_CLASS20A, &cls);
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
  
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"x");
  cr.cpMin = 0;
  cr.cpMax = -1;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  SendMessageA(hwndRichEdit, WM_COPY, 0, 0);
    /*Load "x" into the clipboard. Paste is an easy, undo'able operation.
      also, multiple pastes don't combine like WM_CHAR would */

  /* first case - check the default */
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  for (i=0; i<101; i++) /* Put 101 undo's on the stack */
    SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
  for (i=0; i<100; i++) /* Undo 100 of them */
    SendMessageA(hwndRichEdit, WM_UNDO, 0, 0);
  ok(!SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT allowed more than a hundred undo's by default.\n");

  /* second case - cannot undo */
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0, 0);
  SendMessageA(hwndRichEdit, EM_SETUNDOLIMIT, 0, 0);
  SendMessageA(hwndRichEdit,
              WM_PASTE, 0, 0); /* Try to put something in the undo stack */
  ok(!SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT allowed undo with UNDOLIMIT set to 0\n");

  /* third case - set it to an arbitrary number */
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0, 0);
  SendMessageA(hwndRichEdit, EM_SETUNDOLIMIT, 2, 0);
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
  /* If SETUNDOLIMIT is working, there should only be two undo's after this */
  ok(SendMessageA(hwndRichEdit, EM_CANUNDO, 0,0),
     "EM_SETUNDOLIMIT didn't allow the first undo with UNDOLIMIT set to 2\n");
  SendMessageA(hwndRichEdit, WM_UNDO, 0, 0);
  ok(SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT didn't allow a second undo with UNDOLIMIT set to 2\n");
  SendMessageA(hwndRichEdit, WM_UNDO, 0, 0);
  ok(!SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0),
     "EM_SETUNDOLIMIT allowed a third undo with UNDOLIMIT set to 2\n");
  
  /* fourth case - setting negative numbers should default to 100 undos */
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  result = SendMessageA(hwndRichEdit, EM_SETUNDOLIMIT, -1, 0);
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
  result = SendMessageA(hwndRichEdit, EM_GETPASSWORDCHAR, 0, 0);
  ok (result == 0,
	"EM_GETPASSWORDCHAR returned %c by default, instead of NULL\n",result);

  /* Now, set it to something normal */
  SendMessageA(hwndRichEdit, EM_SETPASSWORDCHAR, 'x', 0);
  result = SendMessageA(hwndRichEdit, EM_GETPASSWORDCHAR, 0, 0);
  ok (result == 120,
	"EM_GETPASSWORDCHAR returned %c (%d) when set to 'x', instead of x (120)\n",result,result);

  /* Now, set it to something odd */
  SendMessageA(hwndRichEdit, EM_SETPASSWORDCHAR, (WCHAR)1234, 0);
  result = SendMessageA(hwndRichEdit, EM_GETPASSWORDCHAR, 0, 0);
  ok (result == 1234,
	"EM_GETPASSWORDCHAR returned %c (%d) when set to 'x', instead of x (120)\n",result,result);
  DestroyWindow(hwndRichEdit);
}

LONG streamout_written = 0;

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
  streamout_written = *pcb;
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

  const char rtftextA[] = "{\\rtf sometext}";
  const char urtftextA[] = "{\\urtf sometext}";
  const WCHAR rtftextW[] = {'{','\\','r','t','f',' ','s','o','m','e','t','e','x','t','}',0};
  const WCHAR urtftextW[] = {'{','\\','u','r','t','f',' ','s','o','m','e','t','e','x','t','}',0};
  const WCHAR sometextW[] = {'s','o','m','e','t','e','x','t',0};

  char buf[1024] = {0};
  WCHAR bufW[1024] = {0};
  LRESULT result;

  /* This test attempts to show that WM_SETTEXT on a riched20 control causes
     any solitary \r to be converted to \r\n on return. Properly paired
     \r\n are not affected. It also shows that the special sequence \r\r\n
     gets converted to a single space.
   */

#define TEST_SETTEXT(a, b) \
  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)a); \
  ok (result == 1, "WM_SETTEXT returned %ld instead of 1\n", result); \
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buf); \
  ok (result == lstrlenA(buf), \
	"WM_GETTEXT returned %ld instead of expected %u\n", \
	result, lstrlenA(buf)); \
  result = strcmp(b, buf); \
  ok(result == 0, \
        "WM_SETTEXT round trip: strcmp = %ld, text=\"%s\"\n", result, buf);

  TEST_SETTEXT(TestItem1, TestItem1)
  TEST_SETTEXT(TestItem2, TestItem2_after)
  TEST_SETTEXT(TestItem3, TestItem3_after)
  TEST_SETTEXT(TestItem3_after, TestItem3_after)
  TEST_SETTEXT(TestItem4, TestItem4_after)
  TEST_SETTEXT(TestItem5, TestItem5_after)
  TEST_SETTEXT(TestItem6, TestItem6_after)
  TEST_SETTEXT(TestItem7, TestItem7_after)

  /* The following tests demonstrate that WM_SETTEXT supports RTF strings */
  TEST_SETTEXT(rtftextA, "sometext") /* interpreted as ascii rtf */
  TEST_SETTEXT(urtftextA, "sometext") /* interpreted as ascii rtf */
  TEST_SETTEXT(rtftextW, "{") /* interpreted as ascii text */
  TEST_SETTEXT(urtftextW, "{") /* interpreted as ascii text */
  DestroyWindow(hwndRichEdit);
#undef TEST_SETTEXT

#define TEST_SETTEXTW(a, b) \
  result = SendMessageW(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)a); \
  ok (result == 1, "WM_SETTEXT returned %ld instead of 1\n", result); \
  result = SendMessageW(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufW); \
  ok (result == lstrlenW(bufW), \
	"WM_GETTEXT returned %ld instead of expected %u\n", \
	result, lstrlenW(bufW)); \
  result = lstrcmpW(b, bufW); \
  ok(result == 0, "WM_SETTEXT round trip: strcmp = %ld\n", result);

  hwndRichEdit = CreateWindowW(RICHEDIT_CLASS20W, NULL,
                               ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                               0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
  ok(hwndRichEdit != NULL, "class: RichEdit20W, error: %d\n", (int) GetLastError());
  TEST_SETTEXTW(rtftextA, sometextW) /* interpreted as ascii rtf */
  TEST_SETTEXTW(urtftextA, sometextW) /* interpreted as ascii rtf */
  TEST_SETTEXTW(rtftextW, rtftextW) /* interpreted as ascii text */
  TEST_SETTEXTW(urtftextW, urtftextW) /* interpreted as ascii text */
  DestroyWindow(hwndRichEdit);
#undef TEST_SETTEXTW

  /* Single-line richedit */
  hwndRichEdit = new_richedit_with_style(NULL, 0);
  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"line1\r\nline2");
  ok(result == 1, "WM_SETTEXT returned %ld, expected 12\n", result);
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buf);
  ok(result == 5, "WM_GETTEXT returned %ld, expected 5\n", result);
  ok(!strcmp(buf, "line1"), "WM_GETTEXT returned incorrect string '%s'\n", buf);
  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"{\\rtf1 ABC\\rtlpar\\par DEF\\par HIJ\\pard\\par}");
  ok(result == 1, "WM_SETTEXT returned %ld, expected 1\n", result);
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buf);
  ok(result == 3, "WM_GETTEXT returned %ld, expected 3\n", result);
  ok(!strcmp(buf, "ABC"), "WM_GETTEXT returned incorrect string '%s'\n", buf);
  DestroyWindow(hwndRichEdit);
}

/* Set *pcb to one to show that the remaining cb-1 bytes are not
   resent to the callkack. */
static DWORD CALLBACK test_esCallback_written_1(DWORD_PTR dwCookie,
                                                LPBYTE pbBuff,
                                                LONG cb,
                                                LONG *pcb)
{
  char** str = (char**)dwCookie;
  ok(*pcb == cb || *pcb == 0, "cb %d, *pcb %d\n", cb, *pcb);
  *pcb = 0;
  if (cb > 0) {
    memcpy(*str, pbBuff, cb);
    *str += cb;
    *pcb = 1;
  }
  return 0;
}

static int count_pars(const char *buf)
{
    const char *p = buf;
    int count = 0;
    while ((p = strstr( p, "\\par" )) != NULL)
    {
        if (!isalpha( p[4] ))
           count++;
        p++;
    }
    return count;
}

static void test_EM_STREAMOUT(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  int r;
  EDITSTREAM es;
  char buf[1024] = {0};
  char * p;
  LRESULT result;

  const char * TestItem1 = "TestSomeText";
  const char * TestItem2 = "TestSomeText\r";
  const char * TestItem3 = "TestSomeText\r\n";

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem1);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_TEXT, (LPARAM)&es);
  r = strlen(buf);
  ok(r == 12, "streamed text length is %d, expecting 12\n", r);
  ok(strcmp(buf, TestItem1) == 0,
        "streamed text different, got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);

  /* RTF mode writes the final end of para \r if it's part of the selection */
  p = buf;
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF, (LPARAM)&es);
  ok (count_pars(buf) == 1, "got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);
  p = buf;
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 12);
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF|SFF_SELECTION, (LPARAM)&es);
  ok (count_pars(buf) == 0, "got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);
  p = buf;
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF|SFF_SELECTION, (LPARAM)&es);
  ok (count_pars(buf) == 1, "got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem2);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_TEXT, (LPARAM)&es);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);
  r = strlen(buf);
  /* Here again, \r gets converted to \r\n, like WM_GETTEXT */
  ok(r == 14, "streamed text length is %d, expecting 14\n", r);
  ok(strcmp(buf, TestItem3) == 0,
        "streamed text different from, got %s\n", buf);

  /* And again RTF mode writes the final end of para \r if it's part of the selection */
  p = buf;
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF, (LPARAM)&es);
  ok (count_pars(buf) == 2, "got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);
  p = buf;
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 13);
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF|SFF_SELECTION, (LPARAM)&es);
  ok (count_pars(buf) == 1, "got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);
  p = buf;
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF|SFF_SELECTION, (LPARAM)&es);
  ok (count_pars(buf) == 2, "got %s\n", buf);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem3);
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_TEXT, (LPARAM)&es);
  ok(result == streamout_written, "got %ld expected %d\n", result, streamout_written);
  r = strlen(buf);
  ok(r == 14, "streamed text length is %d, expecting 14\n", r);
  ok(strcmp(buf, TestItem3) == 0,
        "streamed text different, got %s\n", buf);

  /* Use a callback that sets *pcb to one */
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_esCallback_written_1;
  memset(buf, 0, sizeof(buf));
  result = SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_TEXT, (LPARAM)&es);
  r = strlen(buf);
  ok(r == 14, "streamed text length is %d, expecting 14\n", r);
  ok(strcmp(buf, TestItem3) == 0,
        "streamed text different, got %s\n", buf);
  ok(result == 0, "got %ld expected 0\n", result);


  DestroyWindow(hwndRichEdit);
}

static void test_EM_STREAMOUT_FONTTBL(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  EDITSTREAM es;
  char buf[1024] = {0};
  char * p;
  char * fontTbl;
  int brackCount;

  const char * TestItem = "TestSomeText";

  /* fills in the richedit control with some text */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem);

  /* streams out the text in rtf format */
  p = buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_STREAMOUT, SF_RTF, (LPARAM)&es);

  /* scans for \fonttbl, error if not found */
  fontTbl = strstr(buf, "\\fonttbl");
  ok(fontTbl != NULL, "missing \\fonttbl section\n");
  if(fontTbl)
  {
      /* scans for terminating closing bracket */
      brackCount = 1;
      while(*fontTbl && brackCount)
      {
          if(*fontTbl == '{')
              brackCount++;
          else if(*fontTbl == '}')
              brackCount--;
          fontTbl++;
      }
    /* checks whether closing bracket is ok */
      ok(brackCount == 0, "missing closing bracket in \\fonttbl block\n");
      if(!brackCount)
      {
          /* char before closing fonttbl block should be a closed bracket */
          fontTbl -= 2;
          ok(*fontTbl == '}', "spurious character '%02x' before \\fonttbl closing bracket\n", *fontTbl);

          /* char after fonttbl block should be a crlf */
          fontTbl += 2;
          ok(*fontTbl == 0x0d && *(fontTbl+1) == 0x0a, "missing crlf after \\fonttbl block\n");
      }
  }
  DestroyWindow(hwndRichEdit);
}

static void test_EM_STREAMOUT_empty_para(void)
{
    HWND hwnd = new_richedit(NULL);
    char buf[1024], *p = buf;
    EDITSTREAM es;

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");

    memset(buf, 0, sizeof(buf));
    es.dwCookie    = (DWORD_PTR)&p;
    es.dwError     = 0;
    es.pfnCallback = test_WM_SETTEXT_esCallback;

    SendMessageA(hwnd, EM_STREAMOUT, SF_RTF, (LPARAM)&es);
    ok((p = strstr(buf, "\\pard")) != NULL, "missing \\pard\n");
    ok(((p = strstr(p, "\\fs")) && isdigit(p[3])), "missing \\fs\n");

    DestroyWindow(hwnd);
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
  cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
  cls.hbrBackground = GetStockObject(WHITE_BRUSH);
  cls.lpszMenuName = NULL;
  cls.lpszClassName = "ParentTestClass";
  if(!RegisterClassA(&cls)) assert(0);

  parent = CreateWindowA(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                        0, 0, 200, 60, NULL, NULL, NULL, NULL);
  ok (parent != 0, "Failed to create parent window\n");

  hwndRichEdit = CreateWindowExA(0,
                        RICHEDIT_CLASS20A, NULL,
                        ES_MULTILINE|WS_VSCROLL|WS_VISIBLE|WS_CHILD,
                        0, 0, 200, 60, parent, NULL,
                        hmoduleRichEdit, NULL);

  setText.codepage = CP_ACP;
  setText.flags = ST_SELECTION;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText,
                        (LPARAM)"{\\rtf 1\\par 2\\par 3\\par 4\\par 5\\par 6\\par 7\\par 8\\par 9\\par}");
  todo_wine ok(result == 18, "EM_SETTEXTEX returned %d, expected 18\n", result);
  si.cbSize = sizeof(si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  todo_wine ok(si.nPos == 0, "Position is incorrectly at %d\n", si.nPos);
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  ok(sel_start == 18, "Selection start incorrectly at %d\n", sel_start);
  ok(sel_end == 18, "Selection end incorrectly at %d\n", sel_end);

  DestroyWindow(parent);

  /* Test without a parent window */
  hwndRichEdit = new_richedit(NULL);
  setText.codepage = CP_ACP;
  setText.flags = ST_SELECTION;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText,
                        (LPARAM)"{\\rtf 1\\par 2\\par 3\\par 4\\par 5\\par 6\\par 7\\par 8\\par 9\\par}");
  todo_wine ok(result == 18, "EM_SETTEXTEX returned %d, expected 18\n", result);
  si.cbSize = sizeof(si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok(si.nPos != 0, "Position is incorrectly at %d\n", si.nPos);
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  ok(sel_start == 18, "Selection start incorrectly at %d\n", sel_start);
  ok(sel_end == 18, "Selection end incorrectly at %d\n", sel_end);

  /* The scroll position should also be 0 after EM_SETTEXTEX with ST_DEFAULT,
   * but this time it is because the selection is at the beginning. */
  setText.codepage = CP_ACP;
  setText.flags = ST_DEFAULT;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText,
                        (LPARAM)"{\\rtf 1\\par 2\\par 3\\par 4\\par 5\\par 6\\par 7\\par 8\\par 9\\par}");
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  si.cbSize = sizeof(si);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwndRichEdit, SB_VERT, &si);
  ok(si.nPos == 0, "Position is incorrectly at %d\n", si.nPos);
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
  ok(sel_start == 0, "Selection start incorrectly at %d\n", sel_start);
  ok(sel_end == 0, "Selection end incorrectly at %d\n", sel_end);

  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = 0;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
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
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem2);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(lstrcmpW(buf, TestItem2) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX\n");

  /* However, WM_GETTEXT *does* see \r\n where EM_GETTEXTEX would see \r */
  SendMessageA(hwndRichEdit, WM_GETTEXT, MAX_BUF_LEN, (LPARAM)buf);
  ok(strcmp((const char *)buf, TestItem2_after) == 0,
      "WM_GETTEXT did *not* see \\r converted to \\r\\n pairs.\n");

  /* Baseline test for just-enough buffer space for string */
  getText.cb = (lstrlenW(TestItem2) + 1) * sizeof(WCHAR);
  getText.codepage = 1200;  /* no constant for unicode */
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
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
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
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
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem3);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
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
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem3alt);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
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
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem4);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(lstrcmpW(buf, TestItem4_after) == 0,
      "EM_SETTEXTEX did not convert properly\n");

  /* !ST_SELECTION && Unicode && !\rtf */
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, 0);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  
  ok (result == 1, 
      "EM_SETTEXTEX returned %d, instead of 1\n",result);
  ok(!buf[0], "EM_SETTEXTEX with NULL lParam should clear rich edit.\n");

  /* put some text back: !ST_SELECTION && Unicode && !\rtf */
  setText.flags = 0;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  /* replace current selection: ST_SELECTION && Unicode && !\rtf */
  setText.flags = ST_SELECTION;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, 0);
  ok(result == 0,
      "EM_SETTEXTEX with NULL lParam to replace selection"
      " with no text should return 0. Got %i\n",
      result);
  
  /* put some text back: !ST_SELECTION && Unicode && !\rtf */
  setText.flags = 0;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
  /* replace current selection: ST_SELECTION && Unicode && !\rtf */
  setText.flags = ST_SELECTION;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  /* get text */
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(result == lstrlenW(TestItem1),
      "EM_SETTEXTEX with NULL lParam to replace selection"
      " with no text should return 0. Got %i\n",
      result);
  ok(lstrlenW(buf) == 22,
      "EM_SETTEXTEX to replace selection with more text failed: %i.\n",
      lstrlenW(buf) );

  /* The following test demonstrates that EM_SETTEXTEX supports RTF strings */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"TestSomeText"); /* TestItem1 */
  p = (char *)buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_RTF), (LPARAM)&es);
  trace("EM_STREAMOUT produced:\n%s\n", (char *)buf);

  /* !ST_SELECTION && !Unicode && \rtf */
  setText.codepage = CP_ACP;/* EM_STREAMOUT saved as ANSI string */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = 0;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)buf);
  ok(result == 1, "EM_SETTEXTEX returned %d, expected 1\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
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
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"{\\rtf not unicode}");
  todo_wine ok(result == 11, "EM_SETTEXTEX incorrectly returned %d\n", result);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)bufACP);
  ok(lstrcmpA(bufACP, "not unicode") == 0, "'%s' != 'not unicode'\n", bufACP);

  /* The following test demonstrates that EM_SETTEXTEX supports RTF strings with a selection */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"TestSomeText"); /* TestItem1 */
  p = (char *)buf;
  es.dwCookie = (DWORD_PTR)&p;
  es.dwError = 0;
  es.pfnCallback = test_WM_SETTEXT_esCallback;
  memset(buf, 0, sizeof(buf));
  SendMessageA(hwndRichEdit, EM_STREAMOUT,
              (WPARAM)(SF_RTF), (LPARAM)&es);
  trace("EM_STREAMOUT produced:\n%s\n", (char *)buf);

  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /* ST_SELECTION && !Unicode && \rtf */
  setText.codepage = CP_ACP;/* EM_STREAMOUT saved as ANSI string */
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = ST_SELECTION;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)buf);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok_w3("Expected \"%s\" or \"%s\", got \"%s\"\n", TestItem1alt, TestItem1altn, buf);

  /* The following test demonstrates that EM_SETTEXTEX replacing a selection */
  setText.codepage = 1200;  /* no constant for unicode */
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;

  setText.flags = 0;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1); /* TestItem1 */
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)bufACP);

  /* select some text */
  cr.cpMax = 1;
  cr.cpMin = 3;
  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

  /* ST_SELECTION && !Unicode && !\rtf */
  setText.codepage = CP_ACP;
  getText.codepage = 1200;  /* no constant for unicode */
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;

  setText.flags = ST_SELECTION;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)bufACP);
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(lstrcmpW(buf, TestItem1alt) == 0,
      "EM_GETTEXTEX results not what was set by EM_SETTEXTEX when"
      " using ST_SELECTION and non-Unicode\n");

  /* Test setting text using rich text format */
  setText.flags = 0;
  setText.codepage = CP_ACP;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"{\\rtf richtext}");
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)bufACP);
  ok(!strcmp(bufACP, "richtext"), "expected 'richtext' but got '%s'\n", bufACP);

  setText.flags = 0;
  setText.codepage = CP_ACP;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"{\\urtf morerichtext}");
  getText.codepage = CP_ACP;
  getText.cb = MAX_BUF_LEN;
  getText.flags = GT_DEFAULT;
  getText.lpDefaultChar = NULL;
  getText.lpUsedDefChar = NULL;
  SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)bufACP);
  ok(!strcmp(bufACP, "morerichtext"), "expected 'morerichtext' but got '%s'\n", bufACP);

  /* test for utf8 text with BOM */
  setText.flags = 0;
  setText.codepage = CP_ACP;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"\xef\xbb\xbfTestUTF8WithBOM");
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufACP);
  ok(result == 15, "EM_SETTEXTEX: Test UTF8 with BOM returned %d, expected 15\n", result);
  result = strcmp(bufACP, "TestUTF8WithBOM");
  ok(result == 0, "EM_SETTEXTEX: Test UTF8 with BOM set wrong text: Result: %s\n", bufACP);

  setText.flags = 0;
  setText.codepage = CP_UTF8;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"\xef\xbb\xbfTestUTF8WithBOM");
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufACP);
  ok(result == 15, "EM_SETTEXTEX: Test UTF8 with BOM returned %d, expected 15\n", result);
  result = strcmp(bufACP, "TestUTF8WithBOM");
  ok(result == 0, "EM_SETTEXTEX: Test UTF8 with BOM set wrong text: Result: %s\n", bufACP);

  /* Test multibyte character */
  if (!is_lang_japanese)
    skip("Skip multibyte character tests on non-Japanese platform\n");
  else
  {
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
    setText.flags = ST_SELECTION;
    setText.codepage = CP_ACP;
    result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"abc\x8e\xf0");
    todo_wine ok(result == 5, "EM_SETTEXTEX incorrectly returned %d, expected 5\n", result);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufACP);
    ok(result == 5, "WM_GETTEXT incorrectly returned %d, expected 5\n", result);
    ok(!strcmp(bufACP, "abc\x8e\xf0"),
       "EM_SETTEXTEX: Test multibyte character set wrong text: Result: %s\n", bufACP);

    setText.flags = ST_DEFAULT;
    setText.codepage = CP_ACP;
    result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"abc\x8e\xf0");
    ok(result == 1, "EM_SETTEXTEX incorrectly returned %d, expected 1\n", result);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufACP);
    ok(result == 5, "WM_GETTEXT incorrectly returned %d, expected 5\n", result);
    ok(!strcmp(bufACP, "abc\x8e\xf0"),
       "EM_SETTEXTEX: Test multibyte character set wrong text: Result: %s\n", bufACP);

    SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
    setText.flags = ST_SELECTION;
    setText.codepage = CP_ACP;
    result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"{\\rtf abc\x8e\xf0}");
    todo_wine ok(result == 4, "EM_SETTEXTEX incorrectly returned %d, expected 4\n", result);
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufACP);
    ok(result == 5, "WM_GETTEXT incorrectly returned %d, expected 5\n", result);
    todo_wine ok(!strcmp(bufACP, "abc\x8e\xf0"),
                 "EM_SETTEXTEX: Test multibyte character set wrong text: Result: %s\n", bufACP);
  }

  DestroyWindow(hwndRichEdit);

  /* Single-line richedit */
  hwndRichEdit = new_richedit_with_style(NULL, 0);
  setText.flags = ST_DEFAULT;
  setText.codepage = CP_ACP;
  result = SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)"line1\r\nline2");
  ok(result == 1, "EM_SETTEXTEX incorrectly returned %d, expected 1\n", result);
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)bufACP);
  ok(result == 5, "WM_GETTEXT incorrectly returned %d, expected 5\n", result);
  ok(!strcmp(bufACP, "line1"), "EM_SETTEXTEX: Test single-line text: Result: %s\n", bufACP);
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
  SendMessageA(hwndRichEdit, EM_LIMITTEXT, 100, 0);
  ret = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == 100,
      "EM_LIMITTEXT: set to 100, returned: %d, expected: 100\n", ret);

  /* Set textlimit to 0 */
  SendMessageA(hwndRichEdit, EM_LIMITTEXT, 0, 0);
  ret = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == 65536,
      "EM_LIMITTEXT: set to 0, returned: %d, expected: 65536\n", ret);

  /* Set textlimit to -1 */
  SendMessageA(hwndRichEdit, EM_LIMITTEXT, -1, 0);
  ret = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok (ret == -1,
      "EM_LIMITTEXT: set to -1, returned: %d, expected: -1\n", ret);

  /* Set textlimit to -2 */
  SendMessageA(hwndRichEdit, EM_LIMITTEXT, -2, 0);
  ret = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
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
  
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(32767 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 32767, i); /* default */
  
  textlimit = 256000;
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  /* set higher */
  ok(textlimit == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", textlimit, i);
  
  textlimit = 1000;
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  /* set lower */
  ok(textlimit == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", textlimit, i);
 
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, 0);
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  /* default for WParam = 0 */
  ok(65536 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 65536, i);
 
  textlimit = sizeof(text)-1;
  memset(text, 'W', textlimit);
  text[sizeof(text)-1] = 0;
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  /* maxed out text */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);  /* select everything */
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len1 = selEnd - selBegin;
  
  SendMessageA(hwndRichEdit, WM_KEYDOWN, VK_BACK, 1);
  SendMessageA(hwndRichEdit, WM_CHAR, VK_BACK, 1);
  SendMessageA(hwndRichEdit, WM_KEYUP, VK_BACK, 1);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len2 = selEnd - selBegin;
  
  ok(len1 != len2,
    "EM_EXLIMITTEXT: Change Expected\nOld Length: %d, New Length: %d, Limit: %d\n",
    len1,len2,i);
  
  SendMessageA(hwndRichEdit, WM_KEYDOWN, 'A', 1);
  SendMessageA(hwndRichEdit, WM_CHAR, 'A', 1);
  SendMessageA(hwndRichEdit, WM_KEYUP, 'A', 1);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len1 = selEnd - selBegin;
  
  ok(len1 != len2,
    "EM_EXLIMITTEXT: Change Expected\nOld Length: %d, New Length: %d, Limit: %d\n",
    len1,len2,i);
  
  SendMessageA(hwndRichEdit, WM_KEYDOWN, 'A', 1);
  SendMessageA(hwndRichEdit, WM_CHAR, 'A', 1);
  SendMessageA(hwndRichEdit, WM_KEYUP, 'A', 1);  /* full; should be no effect */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&selBegin, (LPARAM)&selEnd);
  len2 = selEnd - selBegin;
  
  ok(len1 == len2, 
    "EM_EXLIMITTEXT: No Change Expected\nOld Length: %d, New Length: %d, Limit: %d\n",
    len1,len2,i);

  /* set text up to the limit, select all the text, then add a char */
  textlimit = 5;
  memset(text, 'W', textlimit);
  text[textlimit] = 0;
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
  SendMessageA(hwndRichEdit, WM_CHAR, 'A', 1);
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  result = strcmp(buffer, "A");
  ok(0 == result, "got string = \"%s\"\n", buffer);

  /* WM_SETTEXT not limited */
  textlimit = 10;
  memset(text, 'W', textlimit);
  text[textlimit] = 0;
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit-5);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text);
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  i = strlen(buffer);
  ok(10 == i, "expected 10 chars\n");
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(10 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 10, i);

  /* try inserting more text at end */
  i = SendMessageA(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  i = strlen(buffer);
  ok(10 == i, "expected 10 chars, got %i\n", i);
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(10 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 10, i);

  /* try inserting text at beginning */
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 0);
  i = SendMessageA(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  i = strlen(buffer);
  ok(10 == i, "expected 10 chars, got %i\n", i);
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(10 == i, "EM_EXLIMITTEXT: expected: %d, actual: %d\n", 10, i);

  /* WM_CHAR is limited */
  textlimit = 1;
  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, textlimit);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);  /* select everything */
  i = SendMessageA(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  i = SendMessageA(hwndRichEdit, WM_CHAR, 'A', 0);
  ok(0 == i, "WM_CHAR wasn't processed\n");
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  i = strlen(buffer);
  ok(1 == i, "expected 1 chars, got %i instead\n", i);

  DestroyWindow(hwndRichEdit);
}

static void test_EM_GETLIMITTEXT(void)
{
  int i;
  HWND hwndRichEdit = new_richedit(NULL);

  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
  ok(32767 == i, "expected: %d, actual: %d\n", 32767, i); /* default value */

  SendMessageA(hwndRichEdit, EM_EXLIMITTEXT, 0, 50000);
  i = SendMessageA(hwndRichEdit, EM_GETLIMITTEXT, 0, 0);
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
  
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"x");
  SendMessageA(hwndRichEdit, WM_SETFONT, (WPARAM)testFont1, MAKELPARAM(TRUE, 0));
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM)&returnedCF2A);

  GetObjectA(testFont1, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp(sentLogFont.lfFaceName,returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 1. Sent: %s, Returned: %s\n",
    sentLogFont.lfFaceName,returnedCF2A.szFaceName);

  SendMessageA(hwndRichEdit, WM_SETFONT, (WPARAM)testFont2, MAKELPARAM(TRUE, 0));
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM)&returnedCF2A);
  GetObjectA(testFont2, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp(sentLogFont.lfFaceName,returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 2. Sent: %s, Returned: %s\n",
    sentLogFont.lfFaceName,returnedCF2A.szFaceName);
    
  SendMessageA(hwndRichEdit, WM_SETFONT, (WPARAM)testFont3, MAKELPARAM(TRUE, 0));
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM)&returnedCF2A);
  GetObjectA(testFont3, sizeof(LOGFONTA), &sentLogFont);
  ok (!strcmp(sentLogFont.lfFaceName,returnedCF2A.szFaceName),
    "EM_GETCHARFORMAT: Returned wrong font on test 3. Sent: %s, Returned: %s\n",
    sentLogFont.lfFaceName,returnedCF2A.szFaceName);
   
  /* This last test is special since we send in NULL. We clear the variables
   * and just compare to "System" instead of the sent in font name. */
  ZeroMemory(&returnedCF2A,sizeof(returnedCF2A));
  ZeroMemory(&sentLogFont,sizeof(sentLogFont));
  returnedCF2A.cbSize = sizeof(returnedCF2A);
  
  SendMessageA(hwndRichEdit, WM_SETFONT, 0, MAKELPARAM((WORD) TRUE, 0));
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT,   SCF_DEFAULT,  (LPARAM)&returnedCF2A);
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
  CHARFORMAT2A cf2;
  PARAFORMAT2 pf2;
  EDITSTREAM es;
  
  HFONT testFont = CreateFontA (0,0,0,0,FW_LIGHT, 0, 0, 0, ANSI_CHARSET, 
    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | 
    FF_DONTCARE, "Courier");
  
  setText.codepage = 1200;  /* no constant for unicode */
  setText.flags = ST_KEEPUNDO;
  

  /* modify flag shouldn't be set when richedit is first created */
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0, 
      "EM_GETMODIFY returned non-zero, instead of zero on create\n");
  
  /* setting modify flag should actually set it */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, TRUE, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0, 
      "EM_GETMODIFY returned zero, instead of non-zero on EM_SETMODIFY\n");
  
  /* clearing modify flag should actually clear it */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0, 
      "EM_GETMODIFY returned non-zero, instead of zero on EM_SETMODIFY\n");
 
  /* setting font doesn't change modify flag */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, WM_SETFONT, (WPARAM)testFont, MAKELPARAM(TRUE, 0));
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero, instead of zero on setting font\n");

  /* setting text should set modify flag */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero on setting text\n");
  
  /* undo previous text doesn't reset modify flag */
  SendMessageA(hwndRichEdit, WM_UNDO, 0, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero on undo after setting text\n");
  
  /* set text with no flag to keep undo stack should not set modify flag */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  setText.flags = 0;
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero, instead of zero when setting text while not keeping undo stack\n");
  
  /* WM_SETTEXT doesn't modify */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)TestItem2);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero for WM_SETTEXT\n");
  
  /* clear the text */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, WM_CLEAR, 0, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned non-zero, instead of zero for WM_CLEAR\n");
  
  /* replace text */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, EM_SETTEXTEX, (WPARAM)&setText, (LPARAM)TestItem1);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
  SendMessageA(hwndRichEdit, EM_REPLACESEL, TRUE, (LPARAM)TestItem2);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero when replacing text\n");
  
  /* copy/paste text 1 */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
  SendMessageA(hwndRichEdit, WM_COPY, 0, 0);
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero when pasting identical text\n");
  
  /* copy/paste text 2 */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 2);
  SendMessageA(hwndRichEdit, WM_COPY, 0, 0);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 3);
  SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero when pasting different text\n");
  
  /* press char */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, EM_SETSEL, 0, 1);
  SendMessageA(hwndRichEdit, WM_CHAR, 'A', 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for WM_CHAR\n");

  /* press del */
  SendMessageA(hwndRichEdit, WM_CHAR, 'A', 0);
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  SendMessageA(hwndRichEdit, WM_KEYDOWN, VK_BACK, 0);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for backspace\n");
  
  /* set char format */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  cf2.cbSize = sizeof(CHARFORMAT2A);
  SendMessageA(hwndRichEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf2);
  cf2.dwMask = CFM_ITALIC | cf2.dwMask;
  cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;
  SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf2);
  result = SendMessageA(hwndRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf2);
  ok(result == 1, "EM_SETCHARFORMAT returned %ld instead of 1\n", result);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for EM_SETCHARFORMAT\n");
  
  /* set para format */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  pf2.cbSize = sizeof(PARAFORMAT2);
  SendMessageA(hwndRichEdit, EM_GETPARAFORMAT, 0, (LPARAM)&pf2);
  pf2.dwMask = PFM_ALIGNMENT | pf2.dwMask;
  pf2.wAlignment = PFA_RIGHT;
  SendMessageA(hwndRichEdit, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result == 0,
      "EM_GETMODIFY returned zero, instead of non-zero for EM_SETPARAFORMAT\n");

  /* EM_STREAM */
  SendMessageA(hwndRichEdit, EM_SETMODIFY, FALSE, 0);
  es.dwCookie = (DWORD_PTR)&streamText;
  es.dwError = 0;
  es.pfnCallback = test_EM_GETMODIFY_esCallback;
  SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
  result = SendMessageA(hwndRichEdit, EM_GETMODIFY, 0, 0);
  ok (result != 0,
      "EM_GETMODIFY returned zero, instead of non-zero for EM_STREAM\n");

  DestroyWindow(hwndRichEdit);
}

struct exsetsel_s {
  LONG min;
  LONG max;
  LRESULT expected_retval;
  int expected_getsel_start;
  int expected_getsel_end;
  BOOL todo;
};

static const struct exsetsel_s exsetsel_tests[] = {
  /* sanity tests */
  {5, 10, 10, 5, 10 },
  {15, 17, 17, 15, 17 },
  /* test cpMax > strlen() */
  {0, 100, 18, 0, 18 },
  /* test cpMin < 0 && cpMax >= 0 after cpMax > strlen() */
  {-1, 1, 17, 17, 17 },
  /* test cpMin == cpMax */
  {5, 5, 5, 5, 5 },
  /* test cpMin < 0 && cpMax >= 0 (bug 4462) */
  {-1, 0, 5, 5, 5 },
  {-1, 17, 5, 5, 5 },
  {-1, 18, 5, 5, 5 },
  /* test cpMin < 0 && cpMax < 0 */
  {-1, -1, 17, 17, 17 },
  {-4, -5, 17, 17, 17 },
  /* test cpMin >=0 && cpMax < 0 (bug 6814) */
  {0, -1, 18, 0, 18 },
  {17, -5, 18, 17, 18 },
  {18, -3, 17, 17, 17 },
  /* test if cpMin > cpMax */
  {15, 19, 18, 15, 18 },
  {19, 15, 18, 15, 18 },
  /* cpMin == strlen() && cpMax > cpMin */
  {17, 18, 18, 17, 18 },
  {17, 50, 18, 17, 18 },
};

static void check_EM_EXSETSEL(HWND hwnd, const struct exsetsel_s *setsel, int id) {
    CHARRANGE cr;
    LRESULT result;
    int start, end;

    cr.cpMin = setsel->min;
    cr.cpMax = setsel->max;
    result = SendMessageA(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);

    ok(result == setsel->expected_retval, "EM_EXSETSEL(%d): expected: %ld actual: %ld\n", id, setsel->expected_retval, result);

    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    todo_wine_if (setsel->todo)
        ok(start == setsel->expected_getsel_start && end == setsel->expected_getsel_end, "EM_EXSETSEL(%d): expected (%d,%d) actual:(%d,%d)\n",
            id, setsel->expected_getsel_start, setsel->expected_getsel_end, start, end);
}

static void test_EM_EXSETSEL(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    int i;
    const int num_tests = ARRAY_SIZE(exsetsel_tests);

    /* sending some text to the window */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"testing selection");
    /*                                                 01234567890123456*/
    /*                                                          10      */

    for (i = 0; i < num_tests; i++) {
        check_EM_EXSETSEL(hwndRichEdit, &exsetsel_tests[i], i);
    }

    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        CHARRANGE cr;
        char bufA[MAX_BUF_LEN] = {0};
        LRESULT result;

        /* Test with multibyte character */
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"abcdef\x8e\xf0ghijk");
        /*                                                 012345     6  78901 */
        cr.cpMin = 4; cr.cpMax = 8;
        result =  SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);
        ok(result == 8, "EM_EXSETSEL return %ld expected 8\n", result);
        result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, sizeof(bufA), (LPARAM)bufA);
        ok(!strcmp(bufA, "ef\x8e\xf0g"), "EM_GETSELTEXT return incorrect string\n");
        SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
        ok(cr.cpMin == 4, "Selection start incorrectly: %d expected 4\n", cr.cpMin);
        ok(cr.cpMax == 8, "Selection end incorrectly: %d expected 8\n", cr.cpMax);
    }

    DestroyWindow(hwndRichEdit);
}

static void check_EM_SETSEL(HWND hwnd, const struct exsetsel_s *setsel, int id) {
    LRESULT result;
    int start, end;

    result = SendMessageA(hwnd, EM_SETSEL, setsel->min, setsel->max);

    ok(result == setsel->expected_retval, "EM_SETSEL(%d): expected: %ld actual: %ld\n", id, setsel->expected_retval, result);

    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    todo_wine_if (setsel->todo)
        ok(start == setsel->expected_getsel_start && end == setsel->expected_getsel_end, "EM_SETSEL(%d): expected (%d,%d) actual:(%d,%d)\n",
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
    /*                                                 01234567890123456*/
    /*                                                          10      */

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
        /*                                                 012345     6  78901 */
        result =  SendMessageA(hwndRichEdit, EM_SETSEL, 4, 8);
        ok(result == 8, "EM_SETSEL return %ld expected 8\n", result);
        result = SendMessageA(hwndRichEdit, EM_GETSELTEXT, sizeof(buffA), (LPARAM)buffA);
        ok(!strcmp(buffA, "ef\x8e\xf0g"), "EM_GETSELTEXT return incorrect string\n");
        result = SendMessageA(hwndRichEdit, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
        ok(sel_start == 4, "Selection start incorrectly: %d expected 4\n", sel_start);
        ok(sel_end == 8, "Selection end incorrectly: %d expected 8\n", sel_end);
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
    CHAR rtfstream[] = "{\\rtf1 TestSomeText}";
    CHAR urtfstream[] = "{\\urtf1 TestSomeText}";

    /* sending some text to the window */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"testing selection");
    /*                                                 01234567890123456*/
    /*                                                          10      */

    /* FIXME add more tests */
    SendMessageA(hwndRichEdit, EM_SETSEL, 7, 17);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, 0);
    ok(0 == r, "EM_REPLACESEL returned %d, expected 0\n", r);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    r = strcmp(buffer, "testing");
    ok(0 == r, "expected %d, got %d\n", 0, r);

    DestroyWindow(hwndRichEdit);

    hwndRichEdit = new_richedit(NULL);

    trace("Testing EM_REPLACESEL behavior with redraw=%d\n", redraw);
    SendMessageA(hwndRichEdit, WM_SETREDRAW, redraw, 0);

    /* Test behavior with carriage returns and newlines */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"RichEdit1");
    ok(9 == r, "EM_REPLACESEL returned %d, expected 9\n", r);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    r = strcmp(buffer, "RichEdit1");
    ok(0 == r, "expected %d, got %d\n", 0, r);
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "RichEdit1") == 0,
      "EM_GETTEXTEX results not what was set by EM_REPLACESEL\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 1, "EM_GETLINECOUNT returned %d, expected 1\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"RichEdit1\r");
    ok(10 == r, "EM_REPLACESEL returned %d, expected 10\n", r);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    r = strcmp(buffer, "RichEdit1\r\n");
    ok(0 == r, "expected %d, got %d\n", 0, r);
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "RichEdit1\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 2, "EM_GETLINECOUNT returned %d, expected 2\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"RichEdit1\r\n");
    ok(r == 11, "EM_REPLACESEL returned %d, expected 11\n", r);

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 2, "EM_GETLINECOUNT returned %d, expected 2\n", r);

    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 10, "EM_EXGETSEL returned cpMin=%d, expected 10\n", cr.cpMin);
    ok(cr.cpMax == 10, "EM_EXGETSEL returned cpMax=%d, expected 10\n", cr.cpMax);

    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    r = strcmp(buffer, "RichEdit1\r\n");
    ok(0 == r, "expected %d, got %d\n", 0, r);
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "RichEdit1\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 10, "EM_EXGETSEL returned cpMin=%d, expected 10\n", cr.cpMin);
    ok(cr.cpMax == 10, "EM_EXGETSEL returned cpMax=%d, expected 10\n", cr.cpMax);

    /* The following tests show that richedit should handle the special \r\r\n
       sequence by turning it into a single space on insertion. However,
       EM_REPLACESEL on WinXP returns the number of characters in the original
       string.
     */

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\r\r");
    ok(2 == r, "EM_REPLACESEL returned %d, expected 4\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 2, "EM_EXGETSEL returned cpMin=%d, expected 2\n", cr.cpMin);
    ok(cr.cpMax == 2, "EM_EXGETSEL returned cpMax=%d, expected 2\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 3, "EM_GETLINECOUNT returned %d, expected 3\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\r\r\n");
    ok(r == 3, "EM_REPLACESEL returned %d, expected 3\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 1, "EM_EXGETSEL returned cpMin=%d, expected 1\n", cr.cpMin);
    ok(cr.cpMax == 1, "EM_EXGETSEL returned cpMax=%d, expected 1\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, " ") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 1, "EM_GETLINECOUNT returned %d, expected 1\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\r\r\r\r\r\n\r\r\r");
    ok(r == 9, "EM_REPLACESEL returned %d, expected 9\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 7, "EM_EXGETSEL returned cpMin=%d, expected 7\n", cr.cpMin);
    ok(cr.cpMax == 7, "EM_EXGETSEL returned cpMax=%d, expected 7\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "\r\r\r \r\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 7, "EM_GETLINECOUNT returned %d, expected 7\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\r\r\n\r\n");
    ok(r == 5, "EM_REPLACESEL returned %d, expected 5\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 2, "EM_EXGETSEL returned cpMin=%d, expected 2\n", cr.cpMin);
    ok(cr.cpMax == 2, "EM_EXGETSEL returned cpMax=%d, expected 2\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, " \r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 2, "EM_GETLINECOUNT returned %d, expected 2\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\r\r\n\r\r");
    ok(r == 5, "EM_REPLACESEL returned %d, expected 5\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 3, "EM_EXGETSEL returned cpMin=%d, expected 3\n", cr.cpMin);
    ok(cr.cpMax == 3, "EM_EXGETSEL returned cpMax=%d, expected 3\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, " \r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 3, "EM_GETLINECOUNT returned %d, expected 3\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\rX\r\n\r\r");
    ok(r == 6, "EM_REPLACESEL returned %d, expected 6\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 5, "EM_EXGETSEL returned cpMin=%d, expected 5\n", cr.cpMin);
    ok(cr.cpMax == 5, "EM_EXGETSEL returned cpMax=%d, expected 5\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "\rX\r\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 5, "EM_GETLINECOUNT returned %d, expected 5\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\n\n");
    ok(2 == r, "EM_REPLACESEL returned %d, expected 2\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 2, "EM_EXGETSEL returned cpMin=%d, expected 2\n", cr.cpMin);
    ok(cr.cpMax == 2, "EM_EXGETSEL returned cpMax=%d, expected 2\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "\r\r") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 3, "EM_GETLINECOUNT returned %d, expected 3\n", r);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"\n\n\n\n\r\r\r\r\n");
    ok(r == 9, "EM_REPLACESEL returned %d, expected 9\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    ok(cr.cpMin == 7, "EM_EXGETSEL returned cpMin=%d, expected 7\n", cr.cpMin);
    ok(cr.cpMax == 7, "EM_EXGETSEL returned cpMax=%d, expected 7\n", cr.cpMax);

    /* Test the actual string */
    getText.cb = 1024;
    getText.codepage = CP_ACP;
    getText.flags = GT_DEFAULT;
    getText.lpDefaultChar = NULL;
    getText.lpUsedDefChar = NULL;
    SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buffer);
    ok(strcmp(buffer, "\r\r\r\r\r\r ") == 0,
      "EM_GETTEXTEX returned incorrect string\n");

    /* Test number of lines reported after EM_REPLACESEL */
    r = SendMessageA(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(r == 7, "EM_GETLINECOUNT returned %d, expected 7\n", r);

    /* Test with  multibyte character */
    if (!is_lang_japanese)
        skip("Skip multibyte character tests on non-Japanese platform\n");
    else
    {
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
        r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"abc\x8e\xf0");
        todo_wine ok(r == 5, "EM_REPLACESEL returned %d, expected 5\n", r);
        r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
        ok(r == 0, "EM_EXGETSEL returned %d, expected 0\n", r);
        ok(cr.cpMin == 4, "EM_EXGETSEL returned cpMin=%d, expected 4\n", cr.cpMin);
        ok(cr.cpMax == 4, "EM_EXGETSEL returned cpMax=%d, expected 4\n", cr.cpMax);
        r = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
        ok(!strcmp(buffer, "abc\x8e\xf0"), "WM_GETTEXT returned incorrect string\n");
        ok(r == 5, "WM_GETTEXT returned %d, expected 5\n", r);

        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
        r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"{\\rtf abc\x8e\xf0}");
        todo_wine ok(r == 4, "EM_REPLACESEL returned %d, expected 4\n", r);
        r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
        ok(r == 0, "EM_EXGETSEL returned %d, expected 0\n", r);
        todo_wine ok(cr.cpMin == 4, "EM_EXGETSEL returned cpMin=%d, expected 4\n", cr.cpMin);
        todo_wine ok(cr.cpMax == 4, "EM_EXGETSEL returned cpMax=%d, expected 4\n", cr.cpMax);
        r = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
        todo_wine ok(!strcmp(buffer, "abc\x8e\xf0"), "WM_GETTEXT returned incorrect string\n");
        todo_wine ok(r == 5, "WM_GETTEXT returned %d, expected 5\n", r);
    }

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)rtfstream);
    todo_wine ok(r == 12, "EM_REPLACESEL returned %d, expected 12\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    todo_wine ok(cr.cpMin == 12, "EM_EXGETSEL returned cpMin=%d, expected 12\n", cr.cpMin);
    todo_wine ok(cr.cpMax == 12, "EM_EXGETSEL returned cpMax=%d, expected 12\n", cr.cpMax);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    todo_wine ok(!strcmp(buffer, "TestSomeText"), "WM_GETTEXT returned incorrect string\n");

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)urtfstream);
    todo_wine ok(r == 12, "EM_REPLACESEL returned %d, expected 12\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    todo_wine ok(cr.cpMin == 12, "EM_EXGETSEL returned cpMin=%d, expected 12\n", cr.cpMin);
    todo_wine ok(cr.cpMax == 12, "EM_EXGETSEL returned cpMax=%d, expected 12\n", cr.cpMax);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    todo_wine ok(!strcmp(buffer, "TestSomeText"), "WM_GETTEXT returned incorrect string\n");

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"Wine");
    SendMessageA(hwndRichEdit, EM_SETSEL, 1, 2);
    todo_wine r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)rtfstream);
    todo_wine ok(r == 12, "EM_REPLACESEL returned %d, expected 12\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    todo_wine ok(cr.cpMin == 13, "EM_EXGETSEL returned cpMin=%d, expected 13\n", cr.cpMin);
    todo_wine ok(cr.cpMax == 13, "EM_EXGETSEL returned cpMax=%d, expected 13\n", cr.cpMax);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    todo_wine ok(!strcmp(buffer, "WTestSomeTextne"), "WM_GETTEXT returned incorrect string\n");

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"{\\rtf1 Wine}");
    SendMessageA(hwndRichEdit, EM_SETSEL, 1, 2);
    todo_wine r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)rtfstream);
    todo_wine ok(r == 12, "EM_REPLACESEL returned %d, expected 12\n", r);
    r = SendMessageA(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
    ok(0 == r, "EM_EXGETSEL returned %d, expected 0\n", r);
    todo_wine ok(cr.cpMin == 13, "EM_EXGETSEL returned cpMin=%d, expected 13\n", cr.cpMin);
    todo_wine ok(cr.cpMax == 13, "EM_EXGETSEL returned cpMax=%d, expected 13\n", cr.cpMax);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    todo_wine ok(!strcmp(buffer, "WTestSomeTextne"), "WM_GETTEXT returned incorrect string\n");

    if (!redraw)
        /* This is needed to avoid interfering with keybd_event calls
         * on other tests that simulate keyboard events. */
        SendMessageA(hwndRichEdit, WM_SETREDRAW, TRUE, 0);

    DestroyWindow(hwndRichEdit);

    /* Single-line richedit */
    hwndRichEdit = new_richedit_with_style(NULL, 0);
    r = SendMessageA(hwndRichEdit, EM_REPLACESEL, 0, (LPARAM)"line1\r\nline2");
    ok(r == 12, "EM_REPLACESEL returned %d, expected 12\n", r);
    r = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    ok(r == 5, "WM_GETTEXT returned %d, expected 5\n", r);
    ok(!strcmp(buffer, "line1"), "WM_GETTEXT returned incorrect string '%s'\n", buffer);
    DestroyWindow(hwndRichEdit);
}

/* Native riched20 inspects the keyboard state (e.g. GetKeyState)
 * to test the state of the modifiers (Ctrl/Alt/Shift).
 *
 * Therefore Ctrl-<key> keystrokes need to be simulated with
 * keybd_event or by using SetKeyboardState to set the modifiers
 * and SendMessage to simulate the keystrokes.
 */
static LRESULT send_ctrl_key(HWND hwnd, UINT key)
{
    LRESULT result;
    hold_key(VK_CONTROL);
    result = SendMessageA(hwnd, WM_KEYDOWN, key, 1);
    release_key(VK_CONTROL);
    return result;
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

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 14);

    send_ctrl_key(hwndRichEdit, 'C');   /* Copy */
    SendMessageA(hwndRichEdit, EM_SETSEL, 14, 14);
    send_ctrl_key(hwndRichEdit, 'V');   /* Paste */
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    /* Pasted text should be visible at this step */
    result = strcmp(text1_step1, buffer);
    ok(result == 0,
        "test paste: strcmp = %i, text='%s'\n", result, buffer);

    send_ctrl_key(hwndRichEdit, 'Z');   /* Undo */
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    /* Text should be the same as before (except for \r -> \r\n conversion) */
    result = strcmp(text1_after, buffer);
    ok(result == 0,
        "test paste: strcmp = %i, text='%s'\n", result, buffer);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);
    SendMessageA(hwndRichEdit, EM_SETSEL, 8, 13);
    send_ctrl_key(hwndRichEdit, 'C');   /* Copy */
    SendMessageA(hwndRichEdit, EM_SETSEL, 14, 14);
    send_ctrl_key(hwndRichEdit, 'V');   /* Paste */
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    /* Pasted text should be visible at this step */
    result = strcmp(text3, buffer);
    ok(result == 0,
        "test paste: strcmp = %i\n", result);
    send_ctrl_key(hwndRichEdit, 'Z');   /* Undo */
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    /* Text should be the same as before (except for \r -> \r\n conversion) */
    result = strcmp(text2_after, buffer);
    ok(result == 0,
        "test paste: strcmp = %i\n", result);
    send_ctrl_key(hwndRichEdit, 'Y');   /* Redo */
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    /* Text should revert to post-paste state */
    result = strcmp(buffer,text3);
    ok(result == 0,
        "test paste: strcmp = %i\n", result);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    /* Send WM_CHAR to simulate Ctrl-V */
    SendMessageA(hwndRichEdit, WM_CHAR, 22,
                (MapVirtualKeyA('V', MAPVK_VK_TO_VSC) << 16) | 1);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    /* Shouldn't paste because pasting is handled by WM_KEYDOWN */
    result = strcmp(buffer,"");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    /* Send keystrokes with WM_KEYDOWN after setting the modifiers
     * with SetKeyboard state. */

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    /* Simulates paste (Ctrl-V) */
    hold_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_KEYDOWN, 'V',
                (MapVirtualKeyA('V', MAPVK_VK_TO_VSC) << 16) | 1);
    release_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,"paste");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 7);
    /* Simulates copy (Ctrl-C) */
    hold_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_KEYDOWN, 'C',
                (MapVirtualKeyA('C', MAPVK_VK_TO_VSC) << 16) | 1);
    release_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,"testing");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    /* Cut with WM_KEYDOWN to simulate Ctrl-X */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"cut");
    /* Simulates select all (Ctrl-A) */
    hold_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_KEYDOWN, 'A',
                (MapVirtualKeyA('A', MAPVK_VK_TO_VSC) << 16) | 1);
    /* Simulates select cut (Ctrl-X) */
    SendMessageA(hwndRichEdit, WM_KEYDOWN, 'X',
                (MapVirtualKeyA('X', MAPVK_VK_TO_VSC) << 16) | 1);
    release_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,"");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, 0);
    SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,"cut\r\n");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    /* Simulates undo (Ctrl-Z) */
    hold_key(VK_CONTROL);
    SendMessageA(hwndRichEdit, WM_KEYDOWN, 'Z',
                (MapVirtualKeyA('Z', MAPVK_VK_TO_VSC) << 16) | 1);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,"");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    /* Simulates redo (Ctrl-Y) */
    SendMessageA(hwndRichEdit, WM_KEYDOWN, 'Y',
                (MapVirtualKeyA('Y', MAPVK_VK_TO_VSC) << 16) | 1);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer,"cut\r\n");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    release_key(VK_CONTROL);

    /* Copy multiline text to clipboard for future use */
    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text3);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndRichEdit, WM_COPY, 0, 0);
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, 0);

    /* Paste into read-only control */
    result = SendMessageA(hwndRichEdit, EM_SETREADONLY, TRUE, 0);
    SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer, text3);
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    /* Cut from read-only control */
    SendMessageA(hwndRichEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndRichEdit, WM_CUT, 0, 0);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer, text3);
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);

    /* FIXME: Wine doesn't flush Ole clipboard when window is destroyed so do it manually */
    OleFlushClipboard();
    DestroyWindow(hwndRichEdit);

    /* Paste multi-line text into single-line control */
    hwndRichEdit = new_richedit_with_style(NULL, 0);
    SendMessageA(hwndRichEdit, WM_PASTE, 0, 0);
    SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
    result = strcmp(buffer, "testing paste");
    ok(result == 0,
        "test paste: strcmp = %i, actual = '%s'\n", result, buffer);
    DestroyWindow(hwndRichEdit);
}

static void test_EM_FORMATRANGE(void)
{
  int r, i, tpp_x, tpp_y;
  HDC hdc;
  HWND hwndRichEdit = new_richedit(NULL);
  FORMATRANGE fr;
  BOOL skip_non_english;
  static const struct {
    const char *string; /* The string */
    int first;          /* First 'pagebreak', 0 for don't care */
    int second;         /* Second 'pagebreak', 0 for don't care */
  } fmtstrings[] = {
    {"WINE wine", 0, 0},
    {"WINE wineWine", 0, 0},
    {"WINE\r\nwine\r\nwine", 5, 10},
    {"WINE\r\nWINEwine\r\nWINEwine", 5, 14},
    {"WINE\r\n\r\nwine\r\nwine", 5, 6}
  };

  skip_non_english = (PRIMARYLANGID(GetUserDefaultLangID()) != LANG_ENGLISH);
  if (skip_non_english)
    skip("Skipping some tests on non-English platform\n");

  hdc = GetDC(hwndRichEdit);
  ok(hdc != NULL, "Could not get HDC\n");

  /* Calculate the twips per pixel */
  tpp_x = 1440 / GetDeviceCaps(hdc, LOGPIXELSX);
  tpp_y = 1440 / GetDeviceCaps(hdc, LOGPIXELSY);

  /* Test the simple case where all the text fits in the page rect. */
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"a");
  fr.hdc = fr.hdcTarget = hdc;
  fr.rc.top = fr.rcPage.top = fr.rc.left = fr.rcPage.left = 0;
  fr.rc.right = fr.rcPage.right = 500 * tpp_x;
  fr.rc.bottom = fr.rcPage.bottom = 500 * tpp_y;
  fr.chrg.cpMin = 0;
  fr.chrg.cpMax = -1;
  r = SendMessageA(hwndRichEdit, EM_FORMATRANGE, FALSE, (LPARAM)&fr);
  todo_wine ok(r == 2, "r=%d expected r=2\n", r);

  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"ab");
  fr.rc.bottom = fr.rcPage.bottom;
  r = SendMessageA(hwndRichEdit, EM_FORMATRANGE, FALSE, (LPARAM)&fr);
  todo_wine ok(r == 3, "r=%d expected r=3\n", r);

  SendMessageA(hwndRichEdit, EM_FORMATRANGE, FALSE, 0);

  for (i = 0; i < ARRAY_SIZE(fmtstrings); i++)
  {
    GETTEXTLENGTHEX gtl;
    SIZE stringsize;
    int len;

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)fmtstrings[i].string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    len = SendMessageA(hwndRichEdit, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);

    /* Get some size information for the string */
    GetTextExtentPoint32A(hdc, fmtstrings[i].string, strlen(fmtstrings[i].string), &stringsize);

    /* Define the box to be half the width needed and a bit larger than the height.
     * Changes to the width means we have at least 2 pages. Changes to the height
     * is done so we can check the changing of fr.rc.bottom.
     */
    fr.hdc = fr.hdcTarget = hdc;
    fr.rc.top = fr.rcPage.top = fr.rc.left = fr.rcPage.left = 0;
    fr.rc.right = fr.rcPage.right = (stringsize.cx / 2) * tpp_x;
    fr.rc.bottom = fr.rcPage.bottom = (stringsize.cy + 10) * tpp_y;

    r = SendMessageA(hwndRichEdit, EM_FORMATRANGE, TRUE, 0);
    todo_wine {
    ok(r == len, "Expected %d, got %d\n", len, r);
    }

    /* We know that the page can't hold the full string. See how many characters
     * are on the first one
     */
    fr.chrg.cpMin = 0;
    fr.chrg.cpMax = -1;
    r = SendMessageA(hwndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
    todo_wine {
    if (! skip_non_english)
      ok(fr.rc.bottom == (stringsize.cy * tpp_y), "Expected bottom to be %d, got %d\n", (stringsize.cy * tpp_y), fr.rc.bottom);
    }
    if (fmtstrings[i].first)
      todo_wine {
      ok(r == fmtstrings[i].first, "Expected %d, got %d\n", fmtstrings[i].first, r);
      }
    else
      ok(r < len, "Expected < %d, got %d\n", len, r);

    /* Do another page */
    fr.chrg.cpMin = r;
    r = SendMessageA(hwndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
    if (fmtstrings[i].second)
      todo_wine {
      ok(r == fmtstrings[i].second, "Expected %d, got %d\n", fmtstrings[i].second, r);
      }
    else if (! skip_non_english)
      ok (r < len, "Expected < %d, got %d\n", len, r);

    /* There is at least on more page, but we don't care */

    r = SendMessageA(hwndRichEdit, EM_FORMATRANGE, TRUE, 0);
    todo_wine {
    ok(r == len, "Expected %d, got %d\n", len, r);
    }
  }

  ReleaseDC(NULL, hdc);
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

static DWORD CALLBACK test_EM_STREAMIN_esCallback_UTF8Split(DWORD_PTR dwCookie,
                                         LPBYTE pbBuff,
                                         LONG cb,
                                         LONG *pcb)
{
    DWORD *phase = (DWORD *)dwCookie;

    if(*phase == 0){
        static const char first[] = "\xef\xbb\xbf\xc3\x96\xc3";
        *pcb = sizeof(first) - 1;
        memcpy(pbBuff, first, *pcb);
    }else if(*phase == 1){
        static const char second[] = "\x8f\xc3\x8b";
        *pcb = sizeof(second) - 1;
        memcpy(pbBuff, second, *pcb);
    }else
        *pcb = 0;

    ++*phase;

    return 0;
}

static DWORD CALLBACK test_EM_STREAMIN_null_bytes(DWORD_PTR cookie, BYTE *buf, LONG size, LONG *written)
{
    DWORD *phase = (DWORD *)cookie;

    if (*phase == 0)
    {
        static const char first[] = "{\\rtf1\\ansi{Th\0is";
        *written = sizeof(first);
        memcpy(buf, first, *written);
    }
    else if (*phase == 1)
    {
        static const char second[] = " is a test}}";
        *written = sizeof(second);
        memcpy(buf, second, *written);
    }
    else
        *written = 0;

    ++*phase;

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
  DWORD phase;
  LRESULT result;
  EDITSTREAM es;
  char buffer[1024] = {0}, tmp[16];
  CHARRANGE range;
  PARAFORMAT2 fmt;

  const char * streamText0 = "{\\rtf1\\fi100\\li200\\rtlpar\\qr TestSomeText}";
  const char * streamText0a = "{\\rtf1\\fi100\\li200\\rtlpar\\qr TestSomeText\\par}";
  const char * streamText0b = "{\\rtf1 TestSomeText\\par\\par}";
  const char * ptr;

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

  const char * streamTextUTF8BOM = "\xef\xbb\xbfTestUTF8WithBOM";

  const char * streamText4 =
      "This text just needs to be long enough to cause run to be split onto "
      "two separate lines and make sure the null terminating character is "
      "handled properly.\0";

  const WCHAR UTF8Split_exp[4] = {0xd6, 0xcf, 0xcb, 0};

  int length4 = strlen(streamText4) + 1;
  struct StringWithLength cookieForStream4 = {
      length4,
      (char *)streamText4,
  };

  const WCHAR streamText5[] = { 'T', 'e', 's', 't', 'S', 'o', 'm', 'e', 'T', 'e', 'x', 't' };
  int length5 = ARRAY_SIZE(streamText5);
  struct StringWithLength cookieForStream5 = {
      sizeof(streamText5),
      (char *)streamText5,
  };

  /* Minimal test without \par at the end */
  es.dwCookie = (DWORD_PTR)&streamText0;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 12, "got %ld, expected %d\n", result, 12);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0 returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0 set error %d, expected %d\n", es.dwError, 0);
  /* Show that para fmts are ignored */
  range.cpMin = 2;
  range.cpMax = 2;
  result = SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&range);
  memset(&fmt, 0xcc, sizeof(fmt));
  fmt.cbSize = sizeof(fmt);
  result = SendMessageA(hwndRichEdit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt);
  ok(fmt.dxStartIndent == 0, "got %d\n", fmt.dxStartIndent);
  ok(fmt.dxOffset == 0, "got %d\n", fmt.dxOffset);
  ok(fmt.wAlignment == PFA_LEFT, "got %d\n", fmt.wAlignment);
  ok((fmt.wEffects & PFE_RTLPARA) == 0, "got %x\n", fmt.wEffects);

  /* Native richedit 2.0 ignores last \par */
  ptr = streamText0a;
  es.dwCookie = (DWORD_PTR)&ptr;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 12, "got %ld, expected %d\n", result, 12);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0-a returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-a set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0-a set error %d, expected %d\n", es.dwError, 0);
  /* This time para fmts are processed */
  range.cpMin = 2;
  range.cpMax = 2;
  result = SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&range);
  memset(&fmt, 0xcc, sizeof(fmt));
  fmt.cbSize = sizeof(fmt);
  result = SendMessageA(hwndRichEdit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt);
  ok(fmt.dxStartIndent == 300, "got %d\n", fmt.dxStartIndent);
  ok(fmt.dxOffset == -100, "got %d\n", fmt.dxOffset);
  ok(fmt.wAlignment == PFA_RIGHT, "got %d\n", fmt.wAlignment);
  ok((fmt.wEffects & PFE_RTLPARA) == PFE_RTLPARA, "got %x\n", fmt.wEffects);

  /* Native richedit 2.0 ignores last \par, next-to-last \par appears */
  es.dwCookie = (DWORD_PTR)&streamText0b;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 13, "got %ld, expected %d\n", result, 13);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 14,
      "EM_STREAMIN: Test 0-b returned %ld, expected 14\n", result);
  result = strcmp (buffer,"TestSomeText\r\n");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-b set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0-b set error %d, expected %d\n", es.dwError, 0);

  /* Show that when using SFF_SELECTION the last \par is not ignored. */
  ptr = streamText0a;
  es.dwCookie = (DWORD_PTR)&ptr;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 12, "got %ld, expected %d\n", result, 12);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0-a returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-a set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 0-a set error %d, expected %d\n", es.dwError, 0);

  range.cpMin = 0;
  range.cpMax = -1;
  result = SendMessageA(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&range);
  ok (result == 13, "got %ld\n", result);

  ptr = streamText0a;
  es.dwCookie = (DWORD_PTR)&ptr;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;

  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SFF_SELECTION | SF_RTF, (LPARAM)&es);
  ok(result == 13, "got %ld, expected 13\n", result);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 14,
      "EM_STREAMIN: Test SFF_SELECTION 0-a returned %ld, expected 14\n", result);
  result = strcmp (buffer,"TestSomeText\r\n");
  ok (result  == 0,
      "EM_STREAMIN: Test SFF_SELECTION 0-a set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test SFF_SELECTION 0-a set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText1;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 12, "got %ld, expected %d\n", result, 12);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 1 returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 1 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test 1 set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&streamText2;
  es.dwError = 0;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 0, "got %ld, expected %d\n", result, 0);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 2 returned %ld, expected 0\n", result);
  ok(!buffer[0], "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == -16, "EM_STREAMIN: Test 2 set error %d, expected %d\n", es.dwError, -16);

  es.dwCookie = (DWORD_PTR)&streamText3;
  es.dwError = 0;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 0, "got %ld, expected %d\n", result, 0);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 3 returned %ld, expected 0\n", result);
  ok(!buffer[0], "EM_STREAMIN: Test 3 set wrong text: Result: %s\n",buffer);
  ok(es.dwError == -16, "EM_STREAMIN: Test 3 set error %d, expected %d\n", es.dwError, -16);

  es.dwCookie = (DWORD_PTR)&streamTextUTF8BOM;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
  ok(result == 18, "got %ld, expected %d\n", result, 18);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok(result  == 15,
      "EM_STREAMIN: Test UTF8WithBOM returned %ld, expected 15\n", result);
  result = strcmp (buffer,"TestUTF8WithBOM");
  ok(result  == 0,
      "EM_STREAMIN: Test UTF8WithBOM set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test UTF8WithBOM set error %d, expected %d\n", es.dwError, 0);

  phase = 0;
  es.dwCookie = (DWORD_PTR)&phase;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback_UTF8Split;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
  ok(result == 8, "got %ld\n", result);

  WideCharToMultiByte(CP_ACP, 0, UTF8Split_exp, -1, tmp, sizeof(tmp), NULL, NULL);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok(result  == 3,
      "EM_STREAMIN: Test UTF8Split returned %ld\n", result);
  result = memcmp (buffer, tmp, 3);
  ok(result  == 0,
      "EM_STREAMIN: Test UTF8Split set wrong text: Result: %s\n",buffer);
  ok(es.dwError == 0, "EM_STREAMIN: Test UTF8Split set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&cookieForStream4;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback2;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
  ok(result == length4, "got %ld, expected %d\n", result, length4);

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == length4,
      "EM_STREAMIN: Test 4 returned %ld, expected %d\n", result, length4);
  ok(es.dwError == 0, "EM_STREAMIN: Test 4 set error %d, expected %d\n", es.dwError, 0);

  es.dwCookie = (DWORD_PTR)&cookieForStream5;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback2;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT | SF_UNICODE, (LPARAM)&es);
  ok(result == sizeof(streamText5), "got %ld, expected %u\n", result, (UINT)sizeof(streamText5));

  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (result  == length5,
      "EM_STREAMIN: Test 5 returned %ld, expected %d\n", result, length5);
  ok(es.dwError == 0, "EM_STREAMIN: Test 5 set error %d, expected %d\n", es.dwError, 0);

  DestroyWindow(hwndRichEdit);

  /* Single-line richedit */
  hwndRichEdit = new_richedit_with_style(NULL, 0);
  ptr = "line1\r\nline2";
  es.dwCookie = (DWORD_PTR)&ptr;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
  ok(result == 12, "got %ld, expected %d\n", result, 12);
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (!strcmp(buffer, "line1"),
      "EM_STREAMIN: Unexpected text '%s'\n", buffer);

  /* Test 0-bytes inside text */
  hwndRichEdit = new_richedit_with_style(NULL, 0);
  phase = 0;
  es.dwCookie = (DWORD_PTR)&phase;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_null_bytes;
  result = SendMessageA(hwndRichEdit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
  ok(result == 16, "got %ld, expected %d\n", result, 16);
  result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  ok (!strcmp(buffer, "Th is  is a test"), "EM_STREAMIN: Unexpected text '%s'\n", buffer);
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

  es.pfnCallback = EditStreamCallback;

  /* StreamIn, no SFF_SELECTION */
  es.dwCookie = nCallbackCount;
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)randomtext);
  SendMessageA(hwndRichEdit, EM_SETSEL,0,0);
  SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM)&es);
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  result = strcmp (buffer,"test");
  ok (result  == 0,
      "EM_STREAMIN: Test 1 set wrong text: Result: %s\n",buffer);

  result = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok (result == FALSE,
      "EM_STREAMIN without SFF_SELECTION wrongly allows undo\n");

  /* StreamIn, SFF_SELECTION, but nothing selected */
  es.dwCookie = nCallbackCount;
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)randomtext);
  SendMessageA(hwndRichEdit, EM_SETSEL,0,0);
  SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT|SFF_SELECTION, (LPARAM)&es);
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  result = strcmp (buffer,"testSome text");
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);

  result = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
  ok (result == TRUE,
     "EM_STREAMIN with SFF_SELECTION but no selection set "
      "should create an undo\n");

  /* StreamIn, SFF_SELECTION, with a selection */
  es.dwCookie = nCallbackCount;
  SendMessageA(hwndRichEdit,EM_EMPTYUNDOBUFFER, 0,0);
  SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)randomtext);
  SendMessageA(hwndRichEdit, EM_SETSEL,4,5);
  SendMessageA(hwndRichEdit, EM_STREAMIN, SF_TEXT|SFF_SELECTION, (LPARAM)&es);
  SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buffer);
  result = strcmp (buffer,"Sometesttext");
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);

  result = SendMessageA(hwndRichEdit, EM_CANUNDO, 0, 0);
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
        ret = SendMessageW(hwnd, wm_get_text, wparam, (LPARAM)bufW); \
        ok(ret, "SendMessageW(%02x) error %u\n", wm_get_text, GetLastError()); \
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
    ok(ret, "RichEdit20W should be unicode under NT\n");

    /* EM_SETTEXTEX is supported starting from version 3.0 */
    em_settextex_supported = is_em_settextex_supported(hwnd);
    trace("EM_SETTEXTEX is %ssupported on this platform\n",
          em_settextex_supported ? "" : "NOT ");

    expect_empty(hwnd, WM_GETTEXT);
    expect_empty(hwnd, EM_GETTEXTEX);

    ret = SendMessageA(hwnd, WM_CHAR, textW[0], 0);
    ok(!ret, "SendMessageA(WM_CHAR) should return 0, got %d\n", ret);
    expect_textA(hwnd, WM_GETTEXT, "t");
    expect_textA(hwnd, EM_GETTEXTEX, "t");
    expect_textW(hwnd, EM_GETTEXTEX, tW);

    ret = SendMessageA(hwnd, WM_CHAR, textA[1], 0);
    ok(!ret, "SendMessageA(WM_CHAR) should return 0, got %d\n", ret);
    expect_textA(hwnd, WM_GETTEXT, "te");
    expect_textA(hwnd, EM_GETTEXTEX, "te");
    expect_textW(hwnd, EM_GETTEXTEX, teW);

    set_textA(hwnd, WM_SETTEXT, NULL);
    expect_empty(hwnd, WM_GETTEXT);
    expect_empty(hwnd, EM_GETTEXTEX);

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

    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
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

    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
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
    hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP,
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

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)base_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 1, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 1, "ret %d\n",ret);

    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    ret = strcmp(buffer, test_string_after);
    ok(ret == 0, "WM_GETTEXT recovered incorrect string!\n");

    DestroyWindow(hwnd);

    /* multi line */
    hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP | ES_MULTILINE,
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

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)base_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(base_string), "ret %d\n",ret);

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_string_2);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(test_string_2) + 2, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == strlen(test_string_2), "ret %d\n",ret);

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_string);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE | GTL_USECRLF;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 10, "ret %d\n",ret);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = CP_ACP;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == 6, "ret %d\n",ret);

    /* Unicode/NUMCHARS/NUMBYTES */
    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_string_2);

    gtl.flags = GTL_DEFAULT;
    gtl.codepage = 1200;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == lstrlenA(test_string_2),
       "GTL_DEFAULT gave %i, expected %i\n", ret, lstrlenA(test_string_2));

    gtl.flags = GTL_NUMCHARS;
    gtl.codepage = 1200;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == lstrlenA(test_string_2),
       "GTL_NUMCHARS gave %i, expected %i\n", ret, lstrlenA(test_string_2));

    gtl.flags = GTL_NUMBYTES;
    gtl.codepage = 1200;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == lstrlenA(test_string_2)*2,
       "GTL_NUMBYTES gave %i, expected %i\n", ret, lstrlenA(test_string_2)*2);

    gtl.flags = GTL_PRECISE;
    gtl.codepage = 1200;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == lstrlenA(test_string_2)*2,
       "GTL_PRECISE gave %i, expected %i\n", ret, lstrlenA(test_string_2)*2);

    gtl.flags = GTL_NUMCHARS | GTL_PRECISE;
    gtl.codepage = 1200;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == lstrlenA(test_string_2),
       "GTL_NUMCHAR | GTL_PRECISE gave %i, expected %i\n", ret, lstrlenA(test_string_2));

    gtl.flags = GTL_NUMCHARS | GTL_NUMBYTES;
    gtl.codepage = 1200;
    ret = SendMessageA(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
    ok(ret == E_INVALIDARG,
       "GTL_NUMCHARS | GTL_NUMBYTES gave %i, expected %i\n", ret, E_INVALIDARG);

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
      queriedEventMask = SendMessageA(eventMaskEditHwnd, EM_GETEVENTMASK, 0, 0);
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
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "EventMaskParentClass";
    if(!RegisterClassA(&cls)) assert(0);

    parent = CreateWindowA(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, NULL, NULL);
    ok (parent != 0, "Failed to create parent window\n");

    eventMaskEditHwnd = new_richedit(parent);
    ok(eventMaskEditHwnd != 0, "Failed to create edit window\n");

    eventMask = ENM_CHANGE | ENM_UPDATE;
    ret = SendMessageA(eventMaskEditHwnd, EM_SETEVENTMASK, 0, eventMask);
    ok(ret == ENM_NONE, "wrong event mask\n");
    ret = SendMessageA(eventMaskEditHwnd, EM_GETEVENTMASK, 0, 0);
    ok(ret == eventMask, "failed to set event mask\n");

    /* check what happens when we ask for EN_CHANGE and send WM_SETTEXT */
    queriedEventMask = 0;  /* initialize to something other than we expect */
    watchForEventMask = EN_CHANGE;
    ret = SendMessageA(eventMaskEditHwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(ret == TRUE, "failed to set text\n");
    /* richedit should mask off ENM_CHANGE when it sends an EN_CHANGE
       notification in response to WM_SETTEXT */
    ok(queriedEventMask == (eventMask & ~ENM_CHANGE),
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);

    /* check to see if EN_CHANGE is sent when redraw is turned off */
    SendMessageA(eventMaskEditHwnd, WM_CLEAR, 0, 0);
    ok(IsWindowVisible(eventMaskEditHwnd), "Window should be visible.\n");
    SendMessageA(eventMaskEditHwnd, WM_SETREDRAW, FALSE, 0);
    /* redraw is disabled by making the window invisible. */
    ok(!IsWindowVisible(eventMaskEditHwnd), "Window shouldn't be visible.\n");
    queriedEventMask = 0;  /* initialize to something other than we expect */
    SendMessageA(eventMaskEditHwnd, EM_REPLACESEL, 0, (LPARAM)text);
    ok(queriedEventMask == (eventMask & ~ENM_CHANGE),
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);
    SendMessageA(eventMaskEditHwnd, WM_SETREDRAW, TRUE, 0);
    ok(IsWindowVisible(eventMaskEditHwnd), "Window should be visible.\n");

    /* check to see if EN_UPDATE is sent when the editor isn't visible */
    SendMessageA(eventMaskEditHwnd, WM_CLEAR, 0, 0);
    style = GetWindowLongA(eventMaskEditHwnd, GWL_STYLE);
    SetWindowLongA(eventMaskEditHwnd, GWL_STYLE, style & ~WS_VISIBLE);
    ok(!IsWindowVisible(eventMaskEditHwnd), "Window shouldn't be visible.\n");
    watchForEventMask = EN_UPDATE;
    queriedEventMask = 0;  /* initialize to something other than we expect */
    SendMessageA(eventMaskEditHwnd, EM_REPLACESEL, 0, (LPARAM)text);
    ok(queriedEventMask == 0,
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);
    SetWindowLongA(eventMaskEditHwnd, GWL_STYLE, style);
    ok(IsWindowVisible(eventMaskEditHwnd), "Window should be visible.\n");
    queriedEventMask = 0;  /* initialize to something other than we expect */
    SendMessageA(eventMaskEditHwnd, EM_REPLACESEL, 0, (LPARAM)text);
    ok(queriedEventMask == eventMask,
            "wrong event mask (0x%x) during WM_COMMAND\n", queriedEventMask);


    DestroyWindow(parent);
}

static int received_WM_NOTIFY = 0;
static int modify_at_WM_NOTIFY = 0;
static BOOL filter_on_WM_NOTIFY = FALSE;
static HWND hwndRichedit_WM_NOTIFY;

static LRESULT WINAPI WM_NOTIFY_ParentMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_NOTIFY)
    {
      received_WM_NOTIFY = 1;
      modify_at_WM_NOTIFY = SendMessageA(hwndRichedit_WM_NOTIFY, EM_GETMODIFY, 0, 0);
      if (filter_on_WM_NOTIFY) return TRUE;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_WM_NOTIFY(void)
{
    HWND parent;
    WNDCLASSA cls;
    CHARFORMAT2A cf2;
    int sel_start, sel_end;

    /* register class to capture WM_NOTIFY */
    cls.style = 0;
    cls.lpfnWndProc = WM_NOTIFY_ParentMsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "WM_NOTIFY_ParentClass";
    if(!RegisterClassA(&cls)) assert(0);

    parent = CreateWindowA(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, NULL, NULL);
    ok (parent != 0, "Failed to create parent window\n");

    hwndRichedit_WM_NOTIFY = new_richedit(parent);
    ok(hwndRichedit_WM_NOTIFY != 0, "Failed to create edit window\n");

    SendMessageA(hwndRichedit_WM_NOTIFY, EM_SETEVENTMASK, 0, ENM_SELCHANGE);

    /* Notifications for selection change should only be sent when selection
       actually changes. EM_SETCHARFORMAT is one message that calls
       ME_CommitUndo, which should check whether message should be sent */
    received_WM_NOTIFY = 0;
    cf2.cbSize = sizeof(CHARFORMAT2A);
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf2);
    cf2.dwMask = CFM_ITALIC | cf2.dwMask;
    cf2.dwEffects = CFE_ITALIC ^ cf2.dwEffects;
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
    ok(received_WM_NOTIFY == 0, "Unexpected WM_NOTIFY was sent!\n");

    /* WM_SETTEXT should NOT cause a WM_NOTIFY to be sent when selection is
       already at 0. */
    received_WM_NOTIFY = 0;
    modify_at_WM_NOTIFY = 0;
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_SETTEXT, 0, (LPARAM)"sometext");
    ok(received_WM_NOTIFY == 0, "Unexpected WM_NOTIFY was sent!\n");
    ok(modify_at_WM_NOTIFY == 0, "WM_NOTIFY callback saw text flagged as modified!\n");

    received_WM_NOTIFY = 0;
    modify_at_WM_NOTIFY = 0;
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_SETSEL, 4, 4);
    ok(received_WM_NOTIFY == 1, "Expected WM_NOTIFY was NOT sent!\n");

    received_WM_NOTIFY = 0;
    modify_at_WM_NOTIFY = 0;
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_SETTEXT, 0, (LPARAM)"sometext");
    ok(received_WM_NOTIFY == 1, "Expected WM_NOTIFY was NOT sent!\n");
    ok(modify_at_WM_NOTIFY == 0, "WM_NOTIFY callback saw text flagged as modified!\n");

    /* Test for WM_NOTIFY messages with redraw disabled. */
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_SETSEL, 0, 0);
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_SETREDRAW, FALSE, 0);
    received_WM_NOTIFY = 0;
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_REPLACESEL, FALSE, (LPARAM)"inserted");
    ok(received_WM_NOTIFY == 1, "Expected WM_NOTIFY was NOT sent!\n");
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_SETREDRAW, TRUE, 0);

    /* Test filtering key events. */
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_SETSEL, 0, 0);
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    received_WM_NOTIFY = 0;
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_KEYDOWN, VK_RIGHT, 0);
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == 1 && sel_end == 1,
       "selections is incorrectly at (%d,%d)\n", sel_start, sel_end);
    filter_on_WM_NOTIFY = TRUE;
    received_WM_NOTIFY = 0;
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_KEYDOWN, VK_RIGHT, 0);
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == 1 && sel_end == 1,
       "selections is incorrectly at (%d,%d)\n", sel_start, sel_end);

    /* test with owner set to NULL */
    SetWindowLongPtrA(hwndRichedit_WM_NOTIFY, GWLP_HWNDPARENT, 0);
    SendMessageA(hwndRichedit_WM_NOTIFY, WM_KEYDOWN, VK_RIGHT, 0);
    SendMessageA(hwndRichedit_WM_NOTIFY, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == 1 && sel_end == 1,
       "selections is incorrectly at (%d,%d)\n", sel_start, sel_end);

    DestroyWindow(hwndRichedit_WM_NOTIFY);
    DestroyWindow(parent);
}

static ENLINK enlink;
#define CURSOR_CLIENT_X 5
#define CURSOR_CLIENT_Y 5
#define WP_PARENT 1
#define WP_CHILD 2

static LRESULT WINAPI EN_LINK_ParentMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_NOTIFY && ((NMHDR*)lParam)->code == EN_LINK)
    {
        enlink = *(ENLINK*)lParam;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void link_notify_test(const char *desc, int i, HWND hwnd, HWND parent,
                             UINT msg, WPARAM wParam, LPARAM lParam, BOOL notifies)
{
    ENLINK junk_enlink;

    switch (msg)
    {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEHOVER:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        lParam = MAKELPARAM(CURSOR_CLIENT_X, CURSOR_CLIENT_Y);
        break;
    case WM_SETCURSOR:
        if (wParam == WP_PARENT)
            wParam = (WPARAM)parent;
        else if (wParam == WP_CHILD)
            wParam = (WPARAM)hwnd;
        break;
    }

    memset(&junk_enlink, 0x23, sizeof(junk_enlink));
    enlink = junk_enlink;

    SendMessageA(hwnd, msg, wParam, lParam);

    if (notifies)
    {
        ok(enlink.nmhdr.hwndFrom == hwnd,
           "%s test %i: Expected hwnd %p got %p\n", desc, i, hwnd, enlink.nmhdr.hwndFrom);
        ok(enlink.nmhdr.idFrom == 0,
           "%s test %i: Expected idFrom 0 got 0x%lx\n", desc, i, enlink.nmhdr.idFrom);
        ok(enlink.msg == msg,
           "%s test %i: Expected msg 0x%x got 0x%x\n", desc, i, msg, enlink.msg);
        if (msg == WM_SETCURSOR)
        {
            ok(enlink.wParam == 0,
               "%s test %i: Expected wParam 0 got 0x%lx\n", desc, i, enlink.wParam);
        }
        else
        {
            ok(enlink.wParam == wParam,
               "%s test %i: Expected wParam 0x%lx got 0x%lx\n", desc, i, wParam, enlink.wParam);
        }
        ok(enlink.lParam == MAKELPARAM(CURSOR_CLIENT_X, CURSOR_CLIENT_Y),
           "%s test %i: Expected lParam 0x%lx got 0x%lx\n",
           desc, i, MAKELPARAM(CURSOR_CLIENT_X, CURSOR_CLIENT_Y), enlink.lParam);
        ok(enlink.chrg.cpMin == 0 && enlink.chrg.cpMax == 31,
           "%s test %i: Expected link range [0,31) got [%i,%i)\n", desc, i, enlink.chrg.cpMin, enlink.chrg.cpMax);
    }
    else
    {
        ok(memcmp(&enlink, &junk_enlink, sizeof(enlink)) == 0,
           "%s test %i: Expected enlink to remain unmodified\n", desc, i);
    }
}

static void test_EN_LINK(void)
{
    HWND hwnd, parent;
    WNDCLASSA cls;
    CHARFORMAT2A cf2;
    POINT orig_cursor_pos;
    POINT cursor_screen_pos = {CURSOR_CLIENT_X, CURSOR_CLIENT_Y};
    int i;

    static const struct
    {
        UINT msg;
        WPARAM wParam;
        LPARAM lParam;
        BOOL notifies;
    }
    link_notify_tests[] =
    {
        /* hold down the left button and try some messages */
        { WM_LBUTTONDOWN,    0,          0,  TRUE  }, /* 0 */
        { EM_LINESCROLL,     0,          1,  FALSE },
        { EM_SCROLL,         SB_BOTTOM,  0,  FALSE },
        { WM_LBUTTONDBLCLK,  0,          0,  TRUE  },
        { WM_MOUSEHOVER,     0,          0,  FALSE },
        { WM_MOUSEMOVE,      0,          0,  FALSE },
        { WM_MOUSEWHEEL,     0,          0,  FALSE },
        { WM_RBUTTONDBLCLK,  0,          0,  TRUE  },
        { WM_RBUTTONDOWN,    0,          0,  TRUE  },
        { WM_RBUTTONUP,      0,          0,  TRUE  },
        { WM_SETCURSOR,      0,          0,  FALSE },
        { WM_SETCURSOR,      WP_PARENT,  0,  FALSE },
        { WM_SETCURSOR,      WP_CHILD,   0,  TRUE  },
        { WM_SETCURSOR,      WP_CHILD,   1,  TRUE  },
        { WM_VSCROLL,        SB_BOTTOM,  0,  FALSE },
        { WM_LBUTTONUP,      0,          0,  TRUE  },
        /* hold down the right button and try some messages */
        { WM_RBUTTONDOWN,    0,          0,  TRUE  }, /* 16 */
        { EM_LINESCROLL,     0,          1,  FALSE },
        { EM_SCROLL,         SB_BOTTOM,  0,  FALSE },
        { WM_LBUTTONDBLCLK,  0,          0,  TRUE  },
        { WM_LBUTTONDOWN,    0,          0,  TRUE  },
        { WM_LBUTTONUP,      0,          0,  TRUE  },
        { WM_MOUSEHOVER,     0,          0,  FALSE },
        { WM_MOUSEMOVE,      0,          0,  TRUE  },
        { WM_MOUSEWHEEL,     0,          0,  FALSE },
        { WM_RBUTTONDBLCLK,  0,          0,  TRUE  },
        { WM_SETCURSOR,      0,          0,  FALSE },
        { WM_SETCURSOR,      WP_PARENT,  0,  FALSE },
        { WM_SETCURSOR,      WP_CHILD,   0,  TRUE  },
        { WM_SETCURSOR,      WP_CHILD,   1,  TRUE  },
        { WM_VSCROLL,        SB_BOTTOM,  0,  FALSE },
        { WM_RBUTTONUP,      0,          0,  TRUE  },
        /* try the messages with both buttons released */
        { EM_LINESCROLL,     0,          1,  FALSE }, /* 32 */
        { EM_SCROLL,         SB_BOTTOM,  0,  FALSE },
        { WM_LBUTTONDBLCLK,  0,          0,  TRUE  },
        { WM_LBUTTONDOWN,    0,          0,  TRUE  },
        { WM_LBUTTONUP,      0,          0,  TRUE  },
        { WM_MOUSEHOVER,     0,          0,  FALSE },
        { WM_MOUSEMOVE,      0,          0,  TRUE  },
        { WM_MOUSEWHEEL,     0,          0,  FALSE },
        { WM_RBUTTONDBLCLK,  0,          0,  TRUE  },
        { WM_RBUTTONDOWN,    0,          0,  TRUE  },
        { WM_RBUTTONUP,      0,          0,  TRUE  },
        { WM_SETCURSOR,      0,          0,  FALSE },
        { WM_SETCURSOR,      WP_CHILD,   0,  TRUE  },
        { WM_SETCURSOR,      WP_CHILD,   1,  TRUE  },
        { WM_SETCURSOR,      WP_PARENT,  0,  FALSE },
        { WM_VSCROLL,        SB_BOTTOM,  0,  FALSE }
    };

    /* register class to capture WM_NOTIFY */
    cls.style = 0;
    cls.lpfnWndProc = EN_LINK_ParentMsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "EN_LINK_ParentClass";
    if(!RegisterClassA(&cls)) assert(0);

    parent = CreateWindowA(cls.lpszClassName, NULL, WS_POPUP|WS_VISIBLE,
                           0, 0, 200, 60, NULL, NULL, NULL, NULL);
    ok(parent != 0, "Failed to create parent window\n");

    hwnd = new_richedit(parent);
    ok(hwnd != 0, "Failed to create edit window\n");

    SendMessageA(hwnd, EM_SETEVENTMASK, 0, ENM_LINK);

    cf2.cbSize = sizeof(CHARFORMAT2A);
    cf2.dwMask = CFM_LINK;
    cf2.dwEffects = CFE_LINK;
    SendMessageA(hwnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
    /* mixing letters and numbers causes runs to be split */
    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"link text with at least 2 runs");

    GetCursorPos(&orig_cursor_pos);
    SetCursorPos(0, 0);

    for (i = 0; i < ARRAY_SIZE(link_notify_tests); i++)
    {
        link_notify_test("cursor position simulated", i, hwnd, parent,
                         link_notify_tests[i].msg, link_notify_tests[i].wParam, link_notify_tests[i].lParam,
                         link_notify_tests[i].msg == WM_SETCURSOR ? FALSE : link_notify_tests[i].notifies);
    }

    ClientToScreen(hwnd, &cursor_screen_pos);
    SetCursorPos(cursor_screen_pos.x, cursor_screen_pos.y);

    for (i = 0; i < ARRAY_SIZE(link_notify_tests); i++)
    {
        link_notify_test("cursor position set", i, hwnd, parent,
                         link_notify_tests[i].msg, link_notify_tests[i].wParam, link_notify_tests[i].lParam,
                         link_notify_tests[i].notifies);
    }

    SetCursorPos(orig_cursor_pos.x, orig_cursor_pos.y);
    DestroyWindow(hwnd);
    DestroyWindow(parent);
}

static void test_undo_coalescing(void)
{
    HWND hwnd;
    int result;
    char buffer[64] = {0};

    /* multi-line control inserts CR normally */
    hwnd = CreateWindowExA(0, "RichEdit20W", NULL, WS_POPUP|ES_MULTILINE,
                           0, 0, 200, 60, 0, 0, 0, 0);
    ok(hwnd != 0, "CreateWindowExA error %u\n", GetLastError());

    result = SendMessageA(hwnd, EM_CANUNDO, 0, 0);
    ok (result == FALSE, "Can undo after window creation.\n");
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == FALSE, "Undo operation successful with nothing to undo.\n");
    result = SendMessageA(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Can redo after window creation.\n");
    result = SendMessageA(hwnd, EM_REDO, 0, 0);
    ok (result == FALSE, "Redo operation successful with nothing undone.\n");

    /* Test the effect of arrows keys during typing on undo transactions*/
    simulate_typing_characters(hwnd, "one two three");
    SendMessageA(hwnd, WM_KEYDOWN, VK_RIGHT, 1);
    SendMessageA(hwnd, WM_KEYUP, VK_RIGHT, 1);
    simulate_typing_characters(hwnd, " four five six");

    result = SendMessageA(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Can redo before anything is undone.\n");
    result = SendMessageA(hwnd, EM_CANUNDO, 0, 0);
    ok (result == TRUE, "Cannot undo typed characters.\n");
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "EM_UNDO Failed to undo typed characters.\n");
    result = SendMessageA(hwnd, EM_CANREDO, 0, 0);
    ok (result == TRUE, "Cannot redo after undo.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "one two three");
    ok (result == 0, "expected '%s' but got '%s'\n", "one two three", buffer);

    result = SendMessageA(hwnd, EM_CANUNDO, 0, 0);
    ok (result == TRUE, "Cannot undo typed characters.\n");
    result = SendMessageA(hwnd, WM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "");
    ok (result == 0, "expected '%s' but got '%s'\n", "", buffer);

    /* Test the effect of focus changes during typing on undo transactions*/
    simulate_typing_characters(hwnd, "one two three");
    result = SendMessageA(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Redo buffer should have been cleared by typing.\n");
    SendMessageA(hwnd, WM_KILLFOCUS, 0, 0);
    SendMessageA(hwnd, WM_SETFOCUS, 0, 0);
    simulate_typing_characters(hwnd, " four five six");
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "one two three");
    ok (result == 0, "expected '%s' but got '%s'\n", "one two three", buffer);

    /* Test the effect of the back key during typing on undo transactions */
    SendMessageA(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
    ok (result == TRUE, "Failed to clear the text.\n");
    simulate_typing_characters(hwnd, "one two threa");
    result = SendMessageA(hwnd, EM_CANREDO, 0, 0);
    ok (result == FALSE, "Redo buffer should have been cleared by typing.\n");
    SendMessageA(hwnd, WM_KEYDOWN, VK_BACK, 1);
    SendMessageA(hwnd, WM_KEYUP, VK_BACK, 1);
    simulate_typing_characters(hwnd, "e four five six");
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "");
    ok (result == 0, "expected '%s' but got '%s'\n", "", buffer);

    /* Test the effect of the delete key during typing on undo transactions */
    SendMessageA(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"abcd");
    ok(result == TRUE, "Failed to set the text.\n");
    SendMessageA(hwnd, EM_SETSEL, 1, 1);
    SendMessageA(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessageA(hwnd, WM_KEYUP, VK_DELETE, 1);
    SendMessageA(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessageA(hwnd, WM_KEYUP, VK_DELETE, 1);
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "acd");
    ok (result == 0, "expected '%s' but got '%s'\n", "acd", buffer);
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "abcd");
    ok (result == 0, "expected '%s' but got '%s'\n", "abcd", buffer);

    /* Test the effect of EM_STOPGROUPTYPING on undo transactions*/
    SendMessageA(hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
    ok (result == TRUE, "Failed to clear the text.\n");
    simulate_typing_characters(hwnd, "one two three");
    result = SendMessageA(hwnd, EM_STOPGROUPTYPING, 0, 0);
    ok (result == 0, "expected %d but got %d\n", 0, result);
    simulate_typing_characters(hwnd, " four five six");
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
    ok (result == TRUE, "Failed to undo typed characters.\n");
    SendMessageA(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
    result = strcmp(buffer, "one two three");
    ok (result == 0, "expected '%s' but got '%s'\n", "one two three", buffer);
    result = SendMessageA(hwnd, EM_UNDO, 0, 0);
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
    SendMessageA(hwnd, EM_SETSEL, 0, 0);
    /* |one two three */

    send_ctrl_key(hwnd, VK_RIGHT);
    /* one |two  three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 4, "Cursor is at %d instead of %d\n", sel_start, 4);

    send_ctrl_key(hwnd, VK_RIGHT);
    /* one two  |three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 9, "Cursor is at %d instead of %d\n", sel_start, 9);

    send_ctrl_key(hwnd, VK_LEFT);
    /* one |two  three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 4, "Cursor is at %d instead of %d\n", sel_start, 4);

    send_ctrl_key(hwnd, VK_LEFT);
    /* |one two  three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 0, "Cursor is at %d instead of %d\n", sel_start, 0);

    SendMessageA(hwnd, EM_SETSEL, 8, 8);
    /* one two | three */
    send_ctrl_key(hwnd, VK_RIGHT);
    /* one two  |three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 9, "Cursor is at %d instead of %d\n", sel_start, 9);

    SendMessageA(hwnd, EM_SETSEL, 11, 11);
    /* one two  th|ree */
    send_ctrl_key(hwnd, VK_LEFT);
    /* one two  |three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 9, "Cursor is at %d instead of %d\n", sel_start, 9);

    /* Test with a custom word break procedure that uses X as the delimiter. */
    result = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"one twoXthree");
    ok (result == TRUE, "Failed to clear the text.\n");
    SendMessageA(hwnd, EM_SETWORDBREAKPROC, 0, (LPARAM)customWordBreakProc);
    /* |one twoXthree */
    send_ctrl_key(hwnd, VK_RIGHT);
    /* one twoX|three */
    SendMessageA(hwnd, EM_GETSEL, (WPARAM)&sel_start, (LPARAM)&sel_end);
    ok(sel_start == sel_end, "Selection should be empty\n");
    ok(sel_start == 8, "Cursor is at %d instead of %d\n", sel_start, 8);

    DestroyWindow(hwnd);

    /* Make sure the behaviour is the same with a unicode richedit window,
     * and using unicode functions. */

    hwnd = CreateWindowW(RICHEDIT_CLASS20W, NULL,
                        ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                        0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);

    /* Test with a custom word break procedure that uses X as the delimiter. */
    result = SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)textW);
    ok (result == TRUE, "Failed to clear the text.\n");
    SendMessageW(hwnd, EM_SETWORDBREAKPROC, 0, (LPARAM)customWordBreakProc);
    /* |one twoXthree */
    send_ctrl_key(hwnd, VK_RIGHT);
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
    ok(result == 1, "Expected 1, got %d\n", result);
    GetClientRect(hwnd, &rcClient);

    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(result == 34, "expected character index of 34 but got %d\n", result);

    /* Test with points outside the bounds of the richedit control. */
    point.x = -1;
    point.y = 40;
    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 34, "expected character index of 34 but got %d\n", result);

    point.x = 1000;
    point.y = 0;
    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 33, "expected character index of 33 but got %d\n", result);

    point.x = 1000;
    point.y = 36;
    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 39, "expected character index of 39 but got %d\n", result);

    point.x = 1000;
    point.y = -1;
    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 0, "expected character index of 0 but got %d\n", result);

    point.x = 1000;
    point.y = rcClient.bottom + 1;
    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    todo_wine ok(result == 34, "expected character index of 34 but got %d\n", result);

    point.x = 1000;
    point.y = rcClient.bottom;
    result = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
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
    hwnd = CreateWindowA(RICHEDIT_CLASS20A, NULL, dwCommonStyle,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines > 1, "Line was expected to wrap (lines=%d).\n", lines);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS20A, NULL, dwCommonStyle|WS_HSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());

    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line wasn't expected to wrap (lines=%d).\n", lines);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS20A, NULL, dwCommonStyle|ES_AUTOHSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);
    DestroyWindow(hwnd);

    hwnd = CreateWindowA(RICHEDIT_CLASS20A, NULL,
                        dwCommonStyle|WS_HSCROLL|ES_AUTOHSCROLL,
                        0, 0, 200, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    res = SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text);
    ok(res, "WM_SETTEXT failed.\n");
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    SetWindowLongW(hwnd, GWL_STYLE, dwCommonStyle);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    /* Test the effect of EM_SETTARGETDEVICE on word wrap. */
    res = SendMessageA(hwnd, EM_SETTARGETDEVICE, 0, 1);
    ok(res, "EM_SETTARGETDEVICE failed (returned %d).\n", res);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(!pos, "pos=%d indicating word wrap when none is expected.\n", pos);

    res = SendMessageA(hwnd, EM_SETTARGETDEVICE, 0, 0);
    ok(res, "EM_SETTARGETDEVICE failed (returned %d).\n", res);
    pos = SendMessageA(hwnd, EM_CHARFROMPOS, 0, (LPARAM)&point);
    ok(pos, "pos=%d indicating no word wrap when it is expected.\n", pos);
    DestroyWindow(hwnd);

    /* Test to see if wrapping happens with redraw disabled. */
    hwnd = CreateWindowA(RICHEDIT_CLASS20A, NULL, dwCommonStyle,
                        0, 0, 400, 80, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "error: %d\n", (int) GetLastError());
    SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    res = SendMessageA(hwnd, EM_REPLACESEL, FALSE, (LPARAM)text);
    ok(res, "EM_REPLACESEL failed.\n");
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines == 1, "Line wasn't expected to wrap (lines=%d).\n", lines);
    MoveWindow(hwnd, 0, 0, 200, 80, FALSE);
    lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
    ok(lines > 1, "Line was expected to wrap (lines=%d).\n", lines);

    SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
    DestroyWindow(hwnd);
}

static void test_autoscroll(void)
{
    HWND hwnd = new_richedit(NULL);
    int lines, ret, redraw;
    POINT pt;

    for (redraw = 0; redraw <= 1; redraw++) {
        trace("testing with WM_SETREDRAW=%d\n", redraw);
        SendMessageA(hwnd, WM_SETREDRAW, redraw, 0);
        SendMessageA(hwnd, EM_REPLACESEL, 0, (LPARAM)"1\n2\n3\n4\n5\n6\n7\n8");
        lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
        ok(lines == 8, "%d lines instead of 8\n", lines);
        ret = SendMessageA(hwnd, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        ok(ret == 1, "EM_GETSCROLLPOS returned %d instead of 1\n", ret);
        ok(pt.y != 0, "Didn't scroll down after replacing text.\n");
        ret = GetWindowLongA(hwnd, GWL_STYLE);
        ok(ret & WS_VSCROLL, "Scrollbar was not shown yet (style=%x).\n", (UINT)ret);

        SendMessageA(hwnd, WM_SETTEXT, 0, 0);
        lines = SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0);
        ok(lines == 1, "%d lines instead of 1\n", lines);
        ret = SendMessageA(hwnd, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        ok(ret == 1, "EM_GETSCROLLPOS returned %d instead of 1\n", ret);
        ok(pt.y == 0, "y scroll position is %d after clearing text.\n", pt.y);
        ret = GetWindowLongA(hwnd, GWL_STYLE);
        ok(!(ret & WS_VSCROLL), "Scrollbar is still shown (style=%x).\n", (UINT)ret);
    }

    SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
    DestroyWindow(hwnd);

    /* The WS_VSCROLL and WS_HSCROLL styles implicitly set
     * auto vertical/horizontal scrolling options. */
    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP|ES_MULTILINE|WS_VSCROLL|WS_HSCROLL,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    ret = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(ret & ECO_AUTOVSCROLL, "ECO_AUTOVSCROLL isn't set.\n");
    ok(ret & ECO_AUTOHSCROLL, "ECO_AUTOHSCROLL isn't set.\n");
    ret = GetWindowLongA(hwnd, GWL_STYLE);
    ok(!(ret & ES_AUTOVSCROLL), "ES_AUTOVSCROLL is set.\n");
    ok(!(ret & ES_AUTOHSCROLL), "ES_AUTOHSCROLL is set.\n");
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP|ES_MULTILINE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    ret = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(!(ret & ECO_AUTOVSCROLL), "ECO_AUTOVSCROLL is set.\n");
    ok(!(ret & ECO_AUTOHSCROLL), "ECO_AUTOHSCROLL is set.\n");
    ret = GetWindowLongA(hwnd, GWL_STYLE);
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

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());

    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    InflateRect(&expected, -1, 0);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    for (n = -3; n <= 3; n++)
    {
      rc = clientRect;
      InflateRect(&rc, -n, -n);
      SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);

      expected = rc;
      expected.top = max(0, rc.top);
      expected.left = max(0, rc.left);
      expected.bottom = min(clientRect.bottom, rc.bottom);
      expected.right = min(clientRect.right, rc.right);
      SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
      ok(EqualRect(&rc, &expected), "[n=%d] rect %s != %s\n", n, wine_dbgstr_rect(&rc),
         wine_dbgstr_rect(&expected));
    }

    rc = clientRect;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    expected = clientRect;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    /* Adding the selectionbar adds the selectionbar width to the left side. */
    SendMessageA(hwnd, EM_SETOPTIONS, ECOOP_OR, ECO_SELECTIONBAR);
    options = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(options & ECO_SELECTIONBAR, "EM_SETOPTIONS failed to add selectionbar.\n");
    expected.left += 8; /* selection bar width */
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    rc = clientRect;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    expected = clientRect;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    /* Removing the selectionbar subtracts the selectionbar width from the left side,
     * even if the left side is already 0. */
    SendMessageA(hwnd, EM_SETOPTIONS, ECOOP_AND, ~ECO_SELECTIONBAR);
    options = SendMessageA(hwnd, EM_GETOPTIONS, 0, 0);
    ok(!(options & ECO_SELECTIONBAR), "EM_SETOPTIONS failed to remove selectionbar.\n");
    expected.left -= 8; /* selection bar width */
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    /* Set the absolute value of the formatting rectangle. */
    rc = clientRect;
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    expected = clientRect;
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "[n=%d] rect %s != %s\n", n, wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

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
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    /* For some reason it does not limit the values to the client rect with
     * a WPARAM value of 1. */
    rc.top = -15;
    rc.left = -15;
    rc.bottom = clientRect.bottom + 15;
    rc.right = clientRect.right + 15;
    expected = rc;
    SendMessageA(hwnd, EM_SETRECT, 1, (LPARAM)&rc);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    /* Reset to default rect and check how the format rect adjusts to window
     * resize and how it copes with very small windows */
    SendMessageA(hwnd, EM_SETRECT, 0, 0);

    MoveWindow(hwnd, 0, 0, 100, 30, FALSE);
    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    InflateRect(&expected, -1, 0);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    MoveWindow(hwnd, 0, 0, 0, 30, FALSE);
    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    InflateRect(&expected, -1, 0);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    MoveWindow(hwnd, 0, 0, 100, 0, FALSE);
    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    InflateRect(&expected, -1, 0);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    DestroyWindow(hwnd);

    /* The extended window style affects the formatting rectangle. */
    hwnd = CreateWindowExA(WS_EX_CLIENTEDGE, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());

    GetClientRect(hwnd, &clientRect);

    expected = clientRect;
    expected.top += 1;
    InflateRect(&expected, -1, 0);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    rc = clientRect;
    InflateRect(&rc, -5, -5);
    expected = rc;
    expected.top -= 1;
    InflateRect(&expected, 1, 0);
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&rc);
    ok(EqualRect(&rc, &expected), "rect %s != %s\n", wine_dbgstr_rect(&rc),
       wine_dbgstr_rect(&expected));

    DestroyWindow(hwnd);
}

static void test_WM_GETDLGCODE(void)
{
    HWND hwnd;
    UINT res, expected;
    MSG msg;

    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|ES_WANTRETURN|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, 0);
    expected = expected | DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    msg.message = WM_KEYDOWN;
    msg.wParam = VK_RETURN;
    msg.lParam = (MapVirtualKeyA(VK_RETURN, MAPVK_VK_TO_VSC) << 16) | 0x0001;
    msg.pt.x = 0;
    msg.pt.y = 0;
    msg.time = GetTickCount();

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|ES_WANTRETURN|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = expected | DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_WANTRETURN|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    msg.wParam = VK_TAB;
    msg.lParam = (MapVirtualKeyA(VK_TAB, MAPVK_VK_TO_VSC) << 16) | 0x0001;

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hold_key(VK_CONTROL);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    release_key(VK_CONTROL);

    msg.wParam = 'a';
    msg.lParam = (MapVirtualKeyA('a', MAPVK_VK_TO_VSC) << 16) | 0x0001;

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    msg.message = WM_CHAR;

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          ES_MULTILINE|WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL|DLGC_WANTMESSAGE;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    msg.hwnd = hwnd;
    res = SendMessageA(hwnd, WM_GETDLGCODE, VK_RETURN, (LPARAM)&msg);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS|DLGC_HASSETSEL;
    ok(res == expected, "WM_GETDLGCODE returned %x but expected %x\n",
       res, expected);
    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, RICHEDIT_CLASS20A, NULL,
                          WS_POPUP|ES_SAVESEL,
                          0, 0, 200, 60, NULL, NULL, hmoduleRichEdit, NULL);
    ok(hwnd != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());
    res = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
    expected = DLGC_WANTCHARS|DLGC_WANTTAB|DLGC_WANTARROWS;
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
    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 0, "Numerator should be initialized to 0 (got %d).\n", numerator);
    ok(denominator == 0, "Denominator should be initialized to 0 (got %d).\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* test scroll wheel */
    hold_key(VK_CONTROL);
    ret = SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 110, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Test how much the mouse wheel can zoom in and out. */
    ret = SendMessageA(hwnd, EM_SETZOOM, 490, 100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 500, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_SETZOOM, 491, 100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 491, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_SETZOOM, 20, 100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, -WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 10, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_SETZOOM, 19, 100);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, -WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 19, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Test how WM_SCROLLWHEEL treats our custom denominator. */
    ret = SendMessageA(hwnd, EM_SETZOOM, 50, 13);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    hold_key(VK_CONTROL);
    ret = SendMessageA(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA),
                      MAKELPARAM(pt.x, pt.y));
    ok(!ret, "WM_MOUSEWHEEL failed (%d).\n", ret);
    release_key(VK_CONTROL);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 394, "incorrect numerator is %d\n", numerator);
    ok(denominator == 100, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Test bounds checking on EM_SETZOOM */
    ret = SendMessageA(hwnd, EM_SETZOOM, 2, 127);
    ok(ret == TRUE, "EM_SETZOOM rejected valid values (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_SETZOOM, 127, 2);
    ok(ret == TRUE, "EM_SETZOOM rejected valid values (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_SETZOOM, 2, 128);
    ok(ret == FALSE, "EM_SETZOOM accepted invalid values (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 127, "incorrect numerator is %d\n", numerator);
    ok(denominator == 2, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_SETZOOM, 128, 2);
    ok(ret == FALSE, "EM_SETZOOM accepted invalid values (%d).\n", ret);

    /* See if negative numbers are accepted. */
    ret = SendMessageA(hwnd, EM_SETZOOM, -100, -100);
    ok(ret == FALSE, "EM_SETZOOM accepted invalid values (%d).\n", ret);

    /* See if negative numbers are accepted. */
    ret = SendMessageA(hwnd, EM_SETZOOM, 0, 100);
    ok(ret == FALSE, "EM_SETZOOM failed (%d).\n", ret);

    ret = SendMessageA(hwnd, EM_GETZOOM, (WPARAM)&numerator, (LPARAM)&denominator);
    ok(numerator == 127, "incorrect numerator is %d\n", numerator);
    ok(denominator == 2, "incorrect denominator is %d\n", denominator);
    ok(ret == TRUE, "EM_GETZOOM failed (%d).\n", ret);

    /* Reset the zoom value */
    ret = SendMessageA(hwnd, EM_SETZOOM, 0, 0);
    ok(ret == TRUE, "EM_SETZOOM failed (%d).\n", ret);

    DestroyWindow(hwnd);
}

struct dialog_mode_messages
{
    int wm_getdefid, wm_close, wm_nextdlgctl;
};

static struct dialog_mode_messages dm_messages;

#define test_dm_messages(wmclose, wmgetdefid, wmnextdlgctl) \
    ok(dm_messages.wm_close == wmclose, "expected %d WM_CLOSE message, " \
    "got %d\n", wmclose, dm_messages.wm_close); \
    ok(dm_messages.wm_getdefid == wmgetdefid, "expected %d WM_GETDIFID message, " \
    "got %d\n", wmgetdefid, dm_messages.wm_getdefid);\
    ok(dm_messages.wm_nextdlgctl == wmnextdlgctl, "expected %d WM_NEXTDLGCTL message, " \
    "got %d\n", wmnextdlgctl, dm_messages.wm_nextdlgctl)

static LRESULT CALLBACK dialog_mode_wnd_proc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case DM_GETDEFID:
            dm_messages.wm_getdefid++;
            return MAKELONG(ID_RICHEDITTESTDBUTTON, DC_HASDEFID);
        case WM_NEXTDLGCTL:
            dm_messages.wm_nextdlgctl++;
            break;
        case WM_CLOSE:
            dm_messages.wm_close++;
            break;
    }

    return DefWindowProcA(hwnd, iMsg, wParam, lParam);
}

static void test_dialogmode(void)
{
    HWND hwRichEdit, hwParent, hwButton;
    MSG msg= {0};
    int lcount, r;
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = dialog_mode_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "DialogModeParentClass";
    if(!RegisterClassA(&cls)) assert(0);

    hwParent = CreateWindowA("DialogModeParentClass", NULL, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 200, 120, NULL, NULL, GetModuleHandleA(0), NULL);

    /* Test richedit(ES_MULTILINE) */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, ES_MULTILINE, hwParent);

    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    lcount = SendMessageA(hwRichEdit,  EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, 0);
    ok(0x8f == r, "expected 0x8f, got 0x%x\n", r);

    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(3 == lcount, "expected 3, got %d\n", lcount);

    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8f == r, "expected 0x8f, got 0x%x\n", r);
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(3 == lcount, "expected 3, got %d\n", lcount);

    DestroyWindow(hwRichEdit);

    /* Test standalone richedit(ES_MULTILINE) */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, ES_MULTILINE, NULL);

    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8f == r, "expected 0x8f, got 0x%x\n", r);

    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    DestroyWindow(hwRichEdit);

    /* Check  a destination for messages */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, ES_MULTILINE, hwParent);

    SetWindowLongA(hwRichEdit, GWL_STYLE, GetWindowLongA(hwRichEdit, GWL_STYLE)& ~WS_POPUP);
    SetParent( hwRichEdit, NULL);

    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8f == r, "expected 0x8f, got 0x%x\n", r);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 1, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 1);

    DestroyWindow(hwRichEdit);

    /* Check messages from richedit(ES_MULTILINE) */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, ES_MULTILINE, hwParent);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8f == r, "expected 0x8f, got 0x%x\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 1, 0);

    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 1);

    hwButton = CreateWindowA("BUTTON", "OK", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        100, 100, 50, 20, hwParent, (HMENU)ID_RICHEDITTESTDBUTTON, GetModuleHandleA(0), NULL);
    ok(hwButton!=NULL, "CreateWindow failed with error code %d\n", GetLastError());

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 1, 1);

    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    DestroyWindow(hwButton);
    DestroyWindow(hwRichEdit);

    /* Check messages from richedit(ES_MULTILINE|ES_WANTRETURN) */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, ES_MULTILINE|ES_WANTRETURN, hwParent);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(2 == lcount, "expected 2, got %d\n", lcount);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8f == r, "expected 0x8f, got 0x%x\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(3 == lcount, "expected 3, got %d\n", lcount);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 1);

    hwButton = CreateWindowA("BUTTON", "OK", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        100, 100, 50, 20, hwParent, (HMENU)ID_RICHEDITTESTDBUTTON, GetModuleHandleA(0), NULL);
    ok(hwButton!=NULL, "CreateWindow failed with error code %d\n", GetLastError());

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    lcount = SendMessageA(hwRichEdit, EM_GETLINECOUNT, 0, 0);
    ok(4 == lcount, "expected 4, got %d\n", lcount);

    DestroyWindow(hwButton);
    DestroyWindow(hwRichEdit);

    /* Check messages from richedit(0) */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, 0, hwParent);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8b == r, "expected 0x8b, got 0x%x\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 1, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 1);

    hwButton = CreateWindowA("BUTTON", "OK", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        100, 100, 50, 20, hwParent, (HMENU)ID_RICHEDITTESTDBUTTON, GetModuleHandleA(0), NULL);
    ok(hwButton!=NULL, "CreateWindow failed with error code %d\n", GetLastError());

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 1, 1);

    DestroyWindow(hwRichEdit);

    /* Check messages from richedit(ES_WANTRETURN) */

    hwRichEdit = new_window(RICHEDIT_CLASS20A, ES_WANTRETURN, hwParent);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8b == r, "expected 0x8b, got 0x%x\n", r);
    test_dm_messages(0, 0, 0);

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    hwButton = CreateWindowA("BUTTON", "OK", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        100, 100, 50, 20, hwParent, (HMENU)ID_RICHEDITTESTDBUTTON, GetModuleHandleA(0), NULL);
    ok(hwButton!=NULL, "CreateWindow failed with error code %d\n", GetLastError());

    memset(&dm_messages, 0, sizeof(dm_messages));
    r = SendMessageA(hwRichEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(0 == r, "expected 0, got %d\n", r);
    test_dm_messages(0, 0, 0);

    DestroyWindow(hwRichEdit);
    DestroyWindow(hwParent);
}

static void test_EM_FINDWORDBREAK_W(void)
{
    static const struct {
        WCHAR c;
        BOOL isdelimiter;        /* expected result of WB_ISDELIMITER */
    } delimiter_tests[] = {
        {0x0a,   FALSE},         /* newline */
        {0x0b,   FALSE},         /* vertical tab */
        {0x0c,   FALSE},         /* form feed */
        {0x0d,   FALSE},         /* carriage return */
        {0x20,   TRUE},          /* space */
        {0x61,   FALSE},         /* capital letter a */
        {0xa0,   FALSE},         /* no-break space */
        {0x2000, FALSE},         /* en quad */
        {0x3000, FALSE},         /* Ideographic space */
        {0x1100, FALSE},         /* Hangul Choseong Kiyeok (G sound) Ordinary Letter*/
        {0x11ff, FALSE},         /* Hangul Jongseoung Kiyeok-Hieuh (Hard N sound) Ordinary Letter*/
        {0x115f, FALSE},         /* Hangul Choseong Filler (no sound, used with two letter Hangul words) Ordinary Letter */
        {0xac00, FALSE},         /* Hangul character GA*/
        {0xd7af, FALSE},         /* End of Hangul character chart */
        {0xf020, TRUE},          /* MS private for CP_SYMBOL round trip?, see kb897872 */
        {0xff20, FALSE},         /* fullwidth commercial @ */
        {WCH_EMBEDDING, FALSE},  /* object replacement character*/
    };
    int i;
    HWND hwndRichEdit = new_richeditW(NULL);
    ok(IsWindowUnicode(hwndRichEdit), "window should be unicode\n");
    for (i = 0; i < ARRAY_SIZE(delimiter_tests); i++)
    {
        WCHAR wbuf[2];
        int result;

        wbuf[0] = delimiter_tests[i].c;
        wbuf[1] = 0;
        SendMessageW(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)wbuf);
        result = SendMessageW(hwndRichEdit, EM_FINDWORDBREAK, WB_ISDELIMITER,0);
        todo_wine_if (wbuf[0] == 0x20 || wbuf[0] == 0xf020)
            ok(result == delimiter_tests[i].isdelimiter,
                "wanted ISDELIMITER_W(0x%x) %d, got %d\n",
                delimiter_tests[i].c, delimiter_tests[i].isdelimiter, result);
    }
    DestroyWindow(hwndRichEdit);
}

static void test_EM_FINDWORDBREAK_A(void)
{
    static const struct {
        WCHAR c;
        BOOL isdelimiter;        /* expected result of WB_ISDELIMITER */
    } delimiter_tests[] = {
        {0x0a,   FALSE},         /* newline */
        {0x0b,   FALSE},         /* vertical tab */
        {0x0c,   FALSE},         /* form feed */
        {0x0d,   FALSE},         /* carriage return */
        {0x20,   TRUE},          /* space */
        {0x61,   FALSE},         /* capital letter a */
    };
    int i;
    HWND hwndRichEdit = new_richedit(NULL);

    ok(!IsWindowUnicode(hwndRichEdit), "window should not be unicode\n");
    for (i = 0; i < ARRAY_SIZE(delimiter_tests); i++)
    {
        int result;
        char buf[2];
        buf[0] = delimiter_tests[i].c;
        buf[1] = 0;
        SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)buf);
        result = SendMessageA(hwndRichEdit, EM_FINDWORDBREAK, WB_ISDELIMITER, 0);
        todo_wine_if (buf[0] == 0x20)
            ok(result == delimiter_tests[i].isdelimiter,
               "wanted ISDELIMITER_A(0x%x) %d, got %d\n",
               delimiter_tests[i].c, delimiter_tests[i].isdelimiter, result);
    }
    DestroyWindow(hwndRichEdit);
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
      const char *expectedwmtext;
      const char *expectedemtext;
      const char *expectedemtextcrlf;
    } testenteritems[] = {
      { "aaabbb\r\n", 3, "aaa\r\nbbb\r\n", "aaa\rbbb\r", "aaa\r\nbbb\r\n"},
      { "aaabbb\r\n", 6, "aaabbb\r\n\r\n", "aaabbb\r\r", "aaabbb\r\n\r\n"},
      { "aa\rabbb\r\n", 7, "aa\r\nabbb\r\n\r\n", "aa\rabbb\r\r", "aa\r\nabbb\r\n\r\n"},
      { "aa\rabbb\r\n", 3, "aa\r\n\r\nabbb\r\n", "aa\r\rabbb\r", "aa\r\n\r\nabbb\r\n"},
      { "aa\rabbb\r\n", 2, "aa\r\n\r\nabbb\r\n", "aa\r\rabbb\r", "aa\r\n\r\nabbb\r\n"}
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
    ok (result == 1, "[%d] WM_SETTEXT returned %ld instead of 1\n", i, result);

    /* Send Enter */
    SendMessageA(hwndRichEdit, EM_SETSEL, testenteritems[i].cursor, testenteritems[i].cursor);
    simulate_typing_characters(hwndRichEdit, "\r");

    /* 1. Retrieve with WM_GETTEXT */
    buf[0] = 0x00;
    result = SendMessageA(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM)buf);
    expected = testenteritems[i].expectedwmtext;

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
    expected = testenteritems[i].expectedemtext;

    format_test_result(resultbuf, buf);
    format_test_result(expectedbuf, expected);

    result = strcmp(expected, buf);
    ok (result == 0,
        "[%d] EM_GETTEXTEX, GT_DEFAULT unexpected '%s', expected '%s'\n",
        i, resultbuf, expectedbuf);

    /* 3. Retrieve with EM_GETTEXTEX, GT_USECRLF */
    getText.flags = GT_USECRLF;
    getText.codepage = CP_ACP;
    buf[0] = 0x00;
    result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
    expected = testenteritems[i].expectedemtextcrlf;

    format_test_result(resultbuf, buf);
    format_test_result(expectedbuf, expected);

    result = strcmp(expected, buf);
    ok (result == 0,
        "[%d] EM_GETTEXTEX, GT_USECRLF unexpected '%s', expected '%s'\n",
        i, resultbuf, expectedbuf);
  }

  /* Show that WM_CHAR is handled differently from WM_KEYDOWN */
  getText.flags    = GT_DEFAULT;
  getText.codepage = CP_ACP;

  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  ok (result == 1, "[%d] WM_SETTEXT returned %ld instead of 1\n", i, result);
  SendMessageW(hwndRichEdit, WM_CHAR, 'T', 0);
  SendMessageW(hwndRichEdit, WM_KEYDOWN, VK_RETURN, 0);

  result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(result == 2, "Got %d\n", (int)result);
  format_test_result(resultbuf, buf);
  format_test_result(expectedbuf, "T\r");
  result = strcmp(resultbuf, expectedbuf);
  ok (result == 0, "[%d] EM_GETTEXTEX, GT_DEFAULT unexpected '%s', expected '%s'\n", i, resultbuf, expectedbuf);

  result = SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)"");
  ok (result == 1, "[%d] WM_SETTEXT returned %ld instead of 1\n", i, result);
  SendMessageW(hwndRichEdit, WM_CHAR, 'T', 0);
  SendMessageW(hwndRichEdit, WM_CHAR, '\r', 0);

  result = SendMessageA(hwndRichEdit, EM_GETTEXTEX, (WPARAM)&getText, (LPARAM)buf);
  ok(result == 1, "Got %d\n", (int)result);
  format_test_result(resultbuf, buf);
  format_test_result(expectedbuf, "T");
  result = strcmp(resultbuf, expectedbuf);
  ok (result == 0, "[%d] EM_GETTEXTEX, GT_DEFAULT unexpected '%s', expected '%s'\n", i, resultbuf, expectedbuf);

  DestroyWindow(hwndRichEdit);
}

static void test_WM_CREATE(void)
{
    static const WCHAR titleW[] = {'l','i','n','e','1','\n','l','i','n','e','2',0};
    static const char title[] = "line1\nline2";

    HWND rich_edit;
    LRESULT res;
    char buf[64];
    int len;

    rich_edit = CreateWindowA(RICHEDIT_CLASS20A, title, WS_POPUP|WS_VISIBLE,
            0, 0, 200, 80, NULL, NULL, NULL, NULL);
    ok(rich_edit != NULL, "class: %s, error: %d\n", RICHEDIT_CLASS20A, (int) GetLastError());

    len = GetWindowTextA(rich_edit, buf, sizeof(buf));
    ok(len == 5, "GetWindowText returned %d\n", len);
    ok(!strcmp(buf, "line1"), "buf = %s\n", buf);

    res = SendMessageA(rich_edit, EM_GETSEL, 0, 0);
    ok(res == 0, "SendMessage(EM_GETSEL) returned %lx\n", res);

    DestroyWindow(rich_edit);

    rich_edit = CreateWindowW(RICHEDIT_CLASS20W, titleW, WS_POPUP|WS_VISIBLE|ES_MULTILINE,
            0, 0, 200, 80, NULL, NULL, NULL, NULL);
    ok(rich_edit != NULL, "class: %s, error: %d\n", wine_dbgstr_w(RICHEDIT_CLASS20W), (int) GetLastError());

    len = GetWindowTextA(rich_edit, buf, sizeof(buf));
    ok(len == 12, "GetWindowText returned %d\n", len);
    ok(!strcmp(buf, "line1\r\nline2"), "buf = %s\n", buf);

    res = SendMessageA(rich_edit, EM_GETSEL, 0, 0);
    ok(res == 0, "SendMessage(EM_GETSEL) returned %lx\n", res);

    DestroyWindow(rich_edit);
}

/*******************************************************************
 * Test that after deleting all of the text, the first paragraph
 * format reverts to the default.
 */
static void test_reset_default_para_fmt( void )
{
    HWND richedit = new_richeditW( NULL );
    PARAFORMAT2 fmt;
    WORD def_align, new_align;

    memset( &fmt, 0, sizeof(fmt) );
    fmt.cbSize = sizeof(PARAFORMAT2);
    fmt.dwMask = -1;
    SendMessageA( richedit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    def_align = fmt.wAlignment;
    new_align = (def_align == PFA_LEFT) ? PFA_RIGHT : PFA_LEFT;

    simulate_typing_characters( richedit, "123" );

    SendMessageA( richedit, EM_SETSEL, 0, -1 );
    fmt.dwMask = PFM_ALIGNMENT;
    fmt.wAlignment = new_align;
    SendMessageA( richedit, EM_SETPARAFORMAT, 0, (LPARAM)&fmt );

    SendMessageA( richedit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    ok( fmt.wAlignment == new_align, "got %d expect %d\n", fmt.wAlignment, new_align );

    SendMessageA( richedit, EM_SETSEL, 0, -1 );
    SendMessageA( richedit, WM_CUT, 0, 0 );

    SendMessageA( richedit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    ok( fmt.wAlignment == def_align, "got %d expect %d\n", fmt.wAlignment, def_align );

    DestroyWindow( richedit );
}

static void test_EM_SETREADONLY(void)
{
    HWND richedit = new_richeditW(NULL);
    DWORD dwStyle;
    LRESULT res;

    res = SendMessageA(richedit, EM_SETREADONLY, TRUE, 0);
    ok(res == 1, "EM_SETREADONLY\n");
    dwStyle = GetWindowLongA(richedit, GWL_STYLE);
    ok(dwStyle & ES_READONLY, "got wrong value: 0x%x\n", dwStyle);

    res = SendMessageA(richedit, EM_SETREADONLY, FALSE, 0);
    ok(res == 1, "EM_SETREADONLY\n");
    dwStyle = GetWindowLongA(richedit, GWL_STYLE);
    ok(!(dwStyle & ES_READONLY), "got wrong value: 0x%x\n", dwStyle);

    DestroyWindow(richedit);
}

static inline LONG twips2points(LONG value)
{
    return value / 20;
}

#define TEST_EM_SETFONTSIZE(hwnd,size,expected_size,expected_res,expected_undo) \
    _test_font_size(__LINE__,hwnd,size,expected_size,expected_res,expected_undo)
static void _test_font_size(unsigned line, HWND hwnd, LONG size, LONG expected_size,
                            LRESULT expected_res, BOOL expected_undo)
{
    CHARFORMAT2A cf;
    LRESULT res;
    BOOL isundo;

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_SIZE;

    res = SendMessageA(hwnd, EM_SETFONTSIZE, size, 0);
    SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    isundo = SendMessageA(hwnd, EM_CANUNDO, 0, 0);
    ok_(__FILE__,line)(res == expected_res, "EM_SETFONTSIZE unexpected return value: %lx.\n", res);
    ok_(__FILE__,line)(twips2points(cf.yHeight) == expected_size, "got wrong font size: %d, expected: %d\n",
                       twips2points(cf.yHeight), expected_size);
    ok_(__FILE__,line)(isundo == expected_undo, "get wrong undo mark: %d, expected: %d.\n",
                       isundo, expected_undo);
}

static void test_EM_SETFONTSIZE(void)
{
    HWND richedit = new_richedit(NULL);
    CHAR text[] = "wine";
    CHARFORMAT2A tmp_cf;
    LONG default_size;

    tmp_cf.cbSize = sizeof(tmp_cf);
    tmp_cf.dwMask = CFM_SIZE;
    tmp_cf.yHeight = 9 * 20.0;
    SendMessageA(richedit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&tmp_cf);

    SendMessageA(richedit, WM_SETTEXT, 0, (LPARAM)text);

    SendMessageA(richedit, EM_SETMODIFY, FALSE, 0);
    /* without selection */
    TEST_EM_SETFONTSIZE(richedit, 1, 10, TRUE, FALSE); /* 9 + 1 -> 10 */
    SendMessageA(richedit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&tmp_cf);
    default_size = twips2points(tmp_cf.yHeight);
    ok(default_size == 9, "Default font size should not be changed.\n");
    ok(SendMessageA(richedit, EM_SETMODIFY, 0, 0) == FALSE, "Modify flag should not be changed.\n");

    SendMessageA(richedit, EM_SETSEL, 0, 2);

    TEST_EM_SETFONTSIZE(richedit, 0, 9, TRUE, TRUE); /* 9 + 0 -> 9 */

    SendMessageA(richedit, EM_SETMODIFY, FALSE, 0);
    TEST_EM_SETFONTSIZE(richedit, 3, 12, TRUE, TRUE); /* 9 + 3 -> 12 */
    ok(SendMessageA(richedit, EM_SETMODIFY, 0, 0) == FALSE, "Modify flag should not be changed.\n");

    TEST_EM_SETFONTSIZE(richedit, 1, 14, TRUE, TRUE); /* 12 + 1 + 1 -> 14 */
    TEST_EM_SETFONTSIZE(richedit, -1, 12, TRUE, TRUE); /* 14 - 1 - 1 -> 12 */
    TEST_EM_SETFONTSIZE(richedit, 4, 16, TRUE, TRUE); /* 12 + 4 -> 16 */
    TEST_EM_SETFONTSIZE(richedit, 3, 20, TRUE, TRUE); /* 16 + 3 + 1 -> 20 */
    TEST_EM_SETFONTSIZE(richedit, 0, 20, TRUE, TRUE); /* 20 + 0 -> 20 */
    TEST_EM_SETFONTSIZE(richedit, 8, 28, TRUE, TRUE); /* 20 + 8 -> 28 */
    TEST_EM_SETFONTSIZE(richedit, 0, 28, TRUE, TRUE); /* 28 + 0 -> 28 */
    TEST_EM_SETFONTSIZE(richedit, 1, 36, TRUE, TRUE); /* 28 + 1 -> 36 */
    TEST_EM_SETFONTSIZE(richedit, 0, 36, TRUE, TRUE); /* 36 + 0 -> 36 */
    TEST_EM_SETFONTSIZE(richedit, 1, 48, TRUE, TRUE); /* 36 + 1 -> 48 */
    TEST_EM_SETFONTSIZE(richedit, 0, 48, TRUE, TRUE); /* 48 + 0 -> 48 */
    TEST_EM_SETFONTSIZE(richedit, 1, 72, TRUE, TRUE); /* 48 + 1 -> 72 */
    TEST_EM_SETFONTSIZE(richedit, 0, 72, TRUE, TRUE); /* 72 + 0 -> 72 */
    TEST_EM_SETFONTSIZE(richedit, 1, 80, TRUE, TRUE); /* 72 + 1 -> 80 */
    TEST_EM_SETFONTSIZE(richedit, 0, 80, TRUE, TRUE); /* 80 + 0 -> 80 */
    TEST_EM_SETFONTSIZE(richedit, 1, 90, TRUE, TRUE); /* 80 + 1 -> 90 */
    TEST_EM_SETFONTSIZE(richedit, 0, 90, TRUE, TRUE); /* 90 + 0 -> 90 */
    TEST_EM_SETFONTSIZE(richedit, 1, 100, TRUE, TRUE); /* 90 + 1 -> 100 */
    TEST_EM_SETFONTSIZE(richedit, 25, 130, TRUE, TRUE); /* 100 + 25 -> 130 */
    TEST_EM_SETFONTSIZE(richedit, -1, 120, TRUE, TRUE); /* 130 - 1 -> 120 */
    TEST_EM_SETFONTSIZE(richedit, -35, 80, TRUE, TRUE); /* 120 - 35 -> 80 */
    TEST_EM_SETFONTSIZE(richedit, -7, 72, TRUE, TRUE); /* 80 - 7 -> 72 */
    TEST_EM_SETFONTSIZE(richedit, -42, 28, TRUE, TRUE); /* 72 - 42 -> 28 */
    TEST_EM_SETFONTSIZE(richedit, -16, 12, TRUE, TRUE); /* 28 - 16 -> 12 */
    TEST_EM_SETFONTSIZE(richedit, -3, 9, TRUE, TRUE); /* 12 - 3 -> 9 */
    TEST_EM_SETFONTSIZE(richedit, -8, 1, TRUE, TRUE); /* 9 - 8 -> 1 */
    TEST_EM_SETFONTSIZE(richedit, -111, 1, TRUE, TRUE); /* 1 - 111 -> 1 */
    TEST_EM_SETFONTSIZE(richedit, 10086, 1638, TRUE, TRUE); /* 1 + 10086 -> 1638 */

    /* return FALSE when richedit is TM_PLAINTEXT mode */
    SendMessageA(richedit, WM_SETTEXT, 0, (LPARAM)"");
    SendMessageA(richedit, EM_SETTEXTMODE, (WPARAM)TM_PLAINTEXT, 0);
    TEST_EM_SETFONTSIZE(richedit, 0, 9, FALSE, FALSE);

    DestroyWindow(richedit);
}

static void test_alignment_style(void)
{
    HWND richedit = NULL;
    PARAFORMAT2 pf;
    DWORD align_style[] = {ES_LEFT, ES_CENTER, ES_RIGHT, ES_RIGHT | ES_CENTER,
                           ES_LEFT | ES_CENTER, ES_LEFT | ES_RIGHT,
                           ES_LEFT | ES_RIGHT | ES_CENTER};
    DWORD align_mask[] = {PFA_LEFT, PFA_CENTER, PFA_RIGHT, PFA_CENTER, PFA_CENTER,
                          PFA_RIGHT, PFA_CENTER};
    const char * streamtext =
        "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang12298{\\fonttbl{\\f0\\fswiss\\fprq2\\fcharset0 System;}}\r\n"
        "\\viewkind4\\uc1\\pard\\f0\\fs17 TestSomeText\\par\r\n"
        "}\r\n";
    EDITSTREAM es;
    int i;

    for (i = 0; i < ARRAY_SIZE(align_style); i++)
    {
        DWORD dwStyle, new_align;

        richedit = new_windowW(RICHEDIT_CLASS20W, align_style[i], NULL);
        memset(&pf, 0, sizeof(pf));
        pf.cbSize = sizeof(PARAFORMAT2);
        pf.dwMask = -1;

        SendMessageW(richedit, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
        ok(pf.wAlignment == align_mask[i], "(i = %d) got %d expected %d\n",
           i, pf.wAlignment, align_mask[i]);
        dwStyle = GetWindowLongW(richedit, GWL_STYLE);
        ok((i ? (dwStyle & align_style[i]) : (!(dwStyle & 0x0000000f))) ,
           "(i = %d) didn't set right align style: 0x%x\n", i, dwStyle);


        /* Based on test_reset_default_para_fmt() */
        new_align = (align_mask[i] == PFA_LEFT) ? PFA_RIGHT : PFA_LEFT;
        simulate_typing_characters(richedit, "123");

        SendMessageW(richedit, EM_SETSEL, 0, -1);
        pf.dwMask = PFM_ALIGNMENT;
        pf.wAlignment = new_align;
        SendMessageW(richedit, EM_SETPARAFORMAT, 0, (LPARAM)&pf);

        SendMessageW(richedit, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
        ok(pf.wAlignment == new_align, "got %d expect %d\n", pf.wAlignment, new_align);

        SendMessageW(richedit, EM_SETSEL, 0, -1);
        SendMessageW(richedit, WM_CUT, 0, 0);

        SendMessageW(richedit, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
        ok(pf.wAlignment == align_mask[i], "got %d expect %d\n", pf.wAlignment, align_mask[i]);

        DestroyWindow(richedit);
    }

    /* test with EM_STREAMIN */
    richedit = new_windowW(RICHEDIT_CLASS20W, ES_CENTER, NULL);
    simulate_typing_characters(richedit, "abc");
    es.dwCookie = (DWORD_PTR)&streamtext;
    es.dwError = 0;
    es.pfnCallback = test_EM_STREAMIN_esCallback;
    SendMessageW(richedit, EM_STREAMIN, SF_RTF, (LPARAM)&es);
    SendMessageW(richedit, EM_SETSEL, 0, -1);
    memset(&pf, 0, sizeof(pf));
    pf.cbSize = sizeof(PARAFORMAT2);
    pf.dwMask = -1;
    SendMessageW(richedit, EM_GETPARAFORMAT, SCF_SELECTION, (LPARAM)&pf);
    ok(pf.wAlignment == PFA_LEFT, "got %d expected PFA_LEFT\n", pf.wAlignment);
    DestroyWindow(richedit);
}

static void test_WM_GETTEXTLENGTH(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    static const char text1[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee";
    static const char text2[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee\r\n";
    static const char text3[] = "abcdef\x8e\xf0";
    int result;

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text1);
    result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlenA(text1), "WM_GETTEXTLENGTH returned %d, expected %d\n",
       result, lstrlenA(text1));

    SendMessageA(hwndRichEdit, WM_SETTEXT, 0, (LPARAM)text2);
    result = SendMessageA(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlenA(text2), "WM_GETTEXTLENGTH returned %d, expected %d\n",
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

static void test_rtf(void)
{
    const char *specials = "{\\rtf1\\emspace\\enspace\\bullet\\lquote"
        "\\rquote\\ldblquote\\rdblquote\\ltrmark\\rtlmark\\zwj\\zwnj}";
    const WCHAR expect_specials[] = {' ',' ',0x2022,0x2018,0x2019,0x201c,
                                     0x201d,0x200e,0x200f,0x200d,0x200c};
    const char *pard = "{\\rtf1 ABC\\rtlpar\\par DEF\\par HIJ\\pard\\par}";
    const char *highlight = "{\\rtf1{\\colortbl;\\red0\\green0\\blue0;\\red128\\green128\\blue128;\\red192\\green192\\blue192;}\\cf2\\highlight3 foo\\par}";

    HWND edit = new_richeditW( NULL );
    EDITSTREAM es;
    WCHAR buf[80];
    LRESULT result;
    PARAFORMAT2 fmt;
    CHARFORMAT2W cf;

    /* Test rtf specials */
    es.dwCookie = (DWORD_PTR)&specials;
    es.dwError = 0;
    es.pfnCallback = test_EM_STREAMIN_esCallback;
    result = SendMessageA( edit, EM_STREAMIN, SF_RTF, (LPARAM)&es );
    ok( result == 11, "got %ld\n", result );

    result = SendMessageW( edit, WM_GETTEXT, ARRAY_SIZE(buf), (LPARAM)buf );
    ok( result == ARRAY_SIZE(expect_specials), "got %ld\n", result );
    ok( !memcmp( buf, expect_specials, sizeof(expect_specials) ), "got %s\n", wine_dbgstr_w(buf) );

    /* Show that \rtlpar propagates to the second paragraph and is
       reset by \pard in the third. */
    es.dwCookie = (DWORD_PTR)&pard;
    result = SendMessageA( edit, EM_STREAMIN, SF_RTF, (LPARAM)&es );
    ok( result == 11, "got %ld\n", result );

    fmt.cbSize = sizeof(fmt);
    SendMessageW( edit, EM_SETSEL, 1, 1 );
    SendMessageW( edit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    ok( fmt.dwMask & PFM_RTLPARA, "rtl para mask not set\n" );
    ok( fmt.wEffects & PFE_RTLPARA, "rtl para not set\n" );
    SendMessageW( edit, EM_SETSEL, 5, 5 );
    SendMessageW( edit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    ok( fmt.dwMask & PFM_RTLPARA, "rtl para mask not set\n" );
    ok( fmt.wEffects & PFE_RTLPARA, "rtl para not set\n" );
    SendMessageW( edit, EM_SETSEL, 9, 9 );
    SendMessageW( edit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    ok( fmt.dwMask & PFM_RTLPARA, "rtl para mask not set\n" );
    ok( !(fmt.wEffects & PFE_RTLPARA), "rtl para set\n" );

    /* Test \highlight */
    es.dwCookie = (DWORD_PTR)&highlight;
    result = SendMessageA( edit, EM_STREAMIN, SF_RTF, (LPARAM)&es );
    ok( result == 3, "got %ld\n", result );
    SendMessageW( edit, EM_SETSEL, 1, 1 );
    memset( &cf, 0, sizeof(cf) );
    cf.cbSize = sizeof(cf);
    SendMessageW( edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
    ok( (cf.dwEffects & (CFE_AUTOCOLOR | CFE_AUTOBACKCOLOR)) == 0, "got %08x\n", cf.dwEffects );
    ok( cf.crTextColor == RGB(128,128,128), "got %08x\n", cf.crTextColor );
    ok( cf.crBackColor == RGB(192,192,192), "got %08x\n", cf.crBackColor );

    DestroyWindow( edit );
}

static void test_background(void)
{
    HWND hwndRichEdit = new_richedit(NULL);

    /* set the background color to black */
    ValidateRect(hwndRichEdit, NULL);
    SendMessageA(hwndRichEdit, EM_SETBKGNDCOLOR, FALSE, RGB(0, 0, 0));
    ok(GetUpdateRect(hwndRichEdit, NULL, FALSE), "Update rectangle is empty!\n");

    DestroyWindow(hwndRichEdit);
}

static void test_eop_char_fmt(void)
{
    HWND edit = new_richedit( NULL );
    const char *rtf = "{\\rtf1{\\fonttbl{\\f0\\fswiss\\fprq2\\fcharset0 Arial;}{\\f1\\fnil\\fcharset2 Symbol;}}"
        "{\\fs10{\\pard\\fs16\\fi200\\li360\\f0 First\\par"
        "\\f0\\fs25 Second\\par"
        "{\\f0\\fs26 Third}\\par"
        "{\\f0\\fs22 Fourth}\\par}}}";
    EDITSTREAM es;
    CHARFORMAT2W cf;
    int i, num, expect_height;

    es.dwCookie = (DWORD_PTR)&rtf;
    es.dwError = 0;
    es.pfnCallback = test_EM_STREAMIN_esCallback;
    num = SendMessageA( edit, EM_STREAMIN, SF_RTF, (LPARAM)&es );
    ok( num == 25, "got %d\n", num );

    for (i = 0; i <= num; i++)
    {
        SendMessageW( edit, EM_SETSEL, i, i + 1 );
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_SIZE;
        SendMessageW( edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
        ok( cf.dwMask & CFM_SIZE, "%d: got %08x\n", i, cf.dwMask );
        if (i < 6) expect_height = 160;
        else if (i < 13) expect_height = 250;
        else if (i < 18) expect_height = 260;
        else if (i == 18 || i == 25) expect_height = 250;
        else expect_height = 220;
        ok( cf.yHeight == expect_height, "%d: got %d\n", i, cf.yHeight );
    }

    DestroyWindow( edit );
}

static void test_para_numbering(void)
{
    HWND edit = new_richeditW( NULL );
    const char *numbers = "{\\rtf1{\\fonttbl{\\f0\\fswiss\\fprq2\\fcharset0 Arial;}{\\f1\\fnil\\fcharset2 Symbol;}}"
        "\\pard{\\pntext\\f0 3.\\tab}{\\*\\pn\\pnlvlbody\\pnfs32\\pnf0\\pnindent1000\\pnstart2\\pndec{\\pntxta.}}"
        "\\fs20\\fi200\\li360\\f0 First\\par"
        "{\\pntext\\f0 4.\\tab}\\f0 Second\\par"
        "{\\pntext\\f0 6.\\tab}\\f0 Third\\par}";
    const WCHAR expect_numbers_txt[] = {'F','i','r','s','t','\r','S','e','c','o','n','d','\r','T','h','i','r','d',0};
    EDITSTREAM es;
    WCHAR buf[80];
    LRESULT result;
    PARAFORMAT2 fmt, fmt2;
    GETTEXTEX get_text;
    CHARFORMAT2W cf;

    get_text.cb = sizeof(buf);
    get_text.flags = GT_RAWTEXT;
    get_text.codepage = 1200;
    get_text.lpDefaultChar = NULL;
    get_text.lpUsedDefChar = NULL;

    es.dwCookie = (DWORD_PTR)&numbers;
    es.dwError = 0;
    es.pfnCallback = test_EM_STREAMIN_esCallback;
    result = SendMessageA( edit, EM_STREAMIN, SF_RTF, (LPARAM)&es );
    ok( result == lstrlenW( expect_numbers_txt ), "got %ld\n", result );

    result = SendMessageW( edit, EM_GETTEXTEX, (WPARAM)&get_text, (LPARAM)buf );
    ok( result == lstrlenW( expect_numbers_txt ), "got %ld\n", result );
    ok( !lstrcmpW( buf, expect_numbers_txt ), "got %s\n", wine_dbgstr_w(buf) );

    SendMessageW( edit, EM_SETSEL, 1, 1 );
    memset( &fmt, 0, sizeof(fmt) );
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = PFM_ALL2;
    SendMessageW( edit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt );
    ok( fmt.wNumbering == PFN_ARABIC, "got %d\n", fmt.wNumbering );
    ok( fmt.wNumberingStart == 2, "got %d\n", fmt.wNumberingStart );
    ok( fmt.wNumberingStyle == PFNS_PERIOD, "got %04x\n", fmt.wNumberingStyle );
    ok( fmt.wNumberingTab == 1000, "got %d\n", fmt.wNumberingTab );
    ok( fmt.dxStartIndent == 560, "got %d\n", fmt.dxStartIndent );
    ok( fmt.dxOffset == -200, "got %d\n", fmt.dxOffset );

    /* Second para should have identical fmt */
    SendMessageW( edit, EM_SETSEL, 10, 10 );
    memset( &fmt2, 0, sizeof(fmt2) );
    fmt2.cbSize = sizeof(fmt2);
    fmt2.dwMask = PFM_ALL2;
    SendMessageW( edit, EM_GETPARAFORMAT, 0, (LPARAM)&fmt2 );
    ok( !memcmp( &fmt, &fmt2, sizeof(fmt) ), "format mismatch\n" );

    /* Check the eop heights - this determines the label height */
    SendMessageW( edit, EM_SETSEL, 12, 13 );
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_SIZE;
    SendMessageW( edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
    ok( cf.yHeight == 200, "got %d\n", cf.yHeight );

    SendMessageW( edit, EM_SETSEL, 18, 19 );
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_SIZE;
    SendMessageW( edit, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
    ok( cf.yHeight == 200, "got %d\n", cf.yHeight );

    DestroyWindow( edit );
}

static void fill_reobject_struct(REOBJECT *reobj, LONG cp, LPOLEOBJECT poleobj,
                                 LPSTORAGE pstg, LPOLECLIENTSITE polesite, LONG sizel_cx,
                                 LONG sizel_cy, DWORD aspect, DWORD flags, DWORD user)
{
    reobj->cbStruct = sizeof(*reobj);
    reobj->clsid = CLSID_NULL;
    reobj->cp = cp;
    reobj->poleobj = poleobj;
    reobj->pstg = pstg;
    reobj->polesite = polesite;
    reobj->sizel.cx = sizel_cx;
    reobj->sizel.cy = sizel_cy;
    reobj->dvaspect = aspect;
    reobj->dwFlags = flags;
    reobj->dwUser = user;
}

static void test_EM_SELECTIONTYPE(void)
{
    HWND hwnd = new_richedit(NULL);
    IRichEditOle *reole = NULL;
    static const char text1[] = "abcdefg\n";
    int result;
    REOBJECT reo1, reo2;
    IOleClientSite *clientsite;
    HRESULT hr;

    SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text1);
    SendMessageA(hwnd, EM_GETOLEINTERFACE, 0, (LPARAM)&reole);

    SendMessageA(hwnd, EM_SETSEL, 1, 1);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == SEL_EMPTY, "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 1, 2);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == SEL_TEXT, "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 2, 5);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == (SEL_TEXT | SEL_MULTICHAR), "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 0, 1);
    hr = IRichEditOle_GetClientSite(reole, &clientsite);
    ok(hr == S_OK, "IRichEditOle_GetClientSite failed: 0x%08x\n", hr);
    fill_reobject_struct(&reo1, REO_CP_SELECTION, NULL, NULL, clientsite, 10, 10,
                         DVASPECT_CONTENT, 0, 1);
    hr = IRichEditOle_InsertObject(reole, &reo1);
    ok(hr == S_OK, "IRichEditOle_InsertObject failed: 0x%08x\n", hr);
    IOleClientSite_Release(clientsite);

    SendMessageA(hwnd, EM_SETSEL, 0, 1);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == SEL_OBJECT, "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 0, 2);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == (SEL_TEXT | SEL_OBJECT), "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 0, 3);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == (SEL_TEXT | SEL_MULTICHAR | SEL_OBJECT), "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 2, 3);
    hr = IRichEditOle_GetClientSite(reole, &clientsite);
    ok(hr == S_OK, "IRichEditOle_GetClientSite failed: 0x%08x\n", hr);
    fill_reobject_struct(&reo2, REO_CP_SELECTION, NULL, NULL, clientsite, 10, 10,
                         DVASPECT_CONTENT, 0, 2);
    hr = IRichEditOle_InsertObject(reole, &reo2);
    ok(hr == S_OK, "IRichEditOle_InsertObject failed: 0x%08x\n", hr);
    IOleClientSite_Release(clientsite);

    SendMessageA(hwnd, EM_SETSEL, 0, 2);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == (SEL_OBJECT | SEL_TEXT), "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 0, 3);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == (SEL_OBJECT | SEL_MULTIOBJECT | SEL_TEXT), "got wrong selection type: %x.\n", result);

    SendMessageA(hwnd, EM_SETSEL, 0, 4);
    result = SendMessageA(hwnd, EM_SELECTIONTYPE, 0, 0);
    ok(result == (SEL_TEXT| SEL_MULTICHAR | SEL_OBJECT | SEL_MULTIOBJECT), "got wrong selection type: %x.\n", result);

    IRichEditOle_Release(reole);
    DestroyWindow(hwnd);
}

static void test_window_classes(void)
{
    static const struct
    {
        const char *class;
        BOOL success;
    } test[] =
    {
        { "RichEdit", FALSE },
        { "RichEdit20A", TRUE },
        { "RichEdit20W", TRUE },
        { "RichEdit50A", FALSE },
        { "RichEdit50W", FALSE }
    };
    int i;
    HWND hwnd;

    for (i = 0; i < sizeof(test)/sizeof(test[0]); i++)
    {
        SetLastError(0xdeadbeef);
        hwnd = CreateWindowExA(0, test[i].class, NULL, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, NULL);
todo_wine_if(!strcmp(test[i].class, "RichEdit50A") || !strcmp(test[i].class, "RichEdit50W"))
        ok(!hwnd == !test[i].success, "CreateWindow(%s) should %s\n",
           test[i].class, test[i].success ? "succeed" : "fail");
        if (!hwnd)
todo_wine
            ok(GetLastError() == ERROR_CANNOT_FIND_WND_CLASS, "got %d\n", GetLastError());
        else
            DestroyWindow(hwnd);
    }
}

START_TEST( editor )
{
  BOOL ret;
  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED20.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibraryA("riched20.dll");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());
  is_lang_japanese = (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE);

  test_window_classes();
  test_WM_CHAR();
  test_EM_FINDTEXT(FALSE);
  test_EM_FINDTEXT(TRUE);
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
  test_EM_SETSEL();
  test_EM_EXSETSEL();
  test_WM_PASTE();
  test_EM_STREAMIN();
  test_EM_STREAMOUT();
  test_EM_STREAMOUT_FONTTBL();
  test_EM_STREAMOUT_empty_para();
  test_EM_StreamIn_Undo();
  test_EM_FORMATRANGE();
  test_unicode_conversions();
  test_EM_GETTEXTLENGTHEX();
  test_WM_GETTEXTLENGTH();
  test_EM_REPLACESEL(1);
  test_EM_REPLACESEL(0);
  test_WM_NOTIFY();
  test_EN_LINK();
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
  test_dialogmode();
  test_EM_FINDWORDBREAK_W();
  test_EM_FINDWORDBREAK_A();
  test_enter();
  test_WM_CREATE();
  test_reset_default_para_fmt();
  test_EM_SETREADONLY();
  test_EM_SETFONTSIZE();
  test_alignment_style();
  test_rtf();
  test_background();
  test_eop_char_fmt();
  test_para_numbering();
  test_EM_SELECTIONTYPE();

  /* Set the environment variable WINETEST_RICHED20 to keep windows
   * responsive and open for 30 seconds. This is useful for debugging.
   */
  if (getenv( "WINETEST_RICHED20" )) {
    keep_responsive(30);
  }

  OleFlushClipboard();
  ret = FreeLibrary(hmoduleRichEdit);
  ok(ret, "error: %d\n", (int) GetLastError());
}
