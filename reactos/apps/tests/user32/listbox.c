/* Unit test suite for list boxes.
 *
 * Copyright 2003 Ferenc Wagner
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

#include <assert.h>
#include <windows.h>

#include "wine/test.h"

#ifdef VISIBLE
#define WAIT Sleep (1000)
#define REDRAW RedrawWindow (handle, NULL, 0, RDW_UPDATENOW)
#else
#define WAIT
#define REDRAW
#endif

HWND
create_listbox (DWORD add_style)
{
  HWND handle=CreateWindow ("LISTBOX", "TestList",
                            (LBS_STANDARD & ~LBS_SORT) | add_style,
                            0, 0, 100, 100,
                            NULL, NULL, NULL, 0);

  assert (handle);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) "First added");
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) "Second added");
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) "Third added");

#ifdef VISIBLE
  ShowWindow (handle, SW_SHOW);
#endif
  REDRAW;

  return handle;
}

struct listbox_prop {
  DWORD add_style;
};

struct listbox_stat {
  int selected, anchor, caret, selcount;
};

struct listbox_test {
  struct listbox_prop prop;
  struct listbox_stat  init,  init_todo;
  struct listbox_stat click, click_todo;
  struct listbox_stat  step,  step_todo;
};

void
listbox_query (HWND handle, struct listbox_stat *results)
{
  results->selected = SendMessage (handle, LB_GETCURSEL, 0, 0);
  results->anchor   = SendMessage (handle, LB_GETANCHORINDEX, 0, 0);
  results->caret    = SendMessage (handle, LB_GETCARETINDEX, 0, 0);
  results->selcount = SendMessage (handle, LB_GETSELCOUNT, 0, 0);
}

void
buttonpress (HWND handle, WORD x, WORD y)
{
  LPARAM lp=x+(y<<16);

  WAIT;
  SendMessage (handle, WM_LBUTTONDOWN, (WPARAM) MK_LBUTTON, lp);
  SendMessage (handle, WM_LBUTTONUP  , (WPARAM) 0         , lp);
  REDRAW;
}

void
keypress (HWND handle, WPARAM keycode, BYTE scancode, BOOL extended)
{
  LPARAM lp=1+(scancode<<16)+(extended?KEYEVENTF_EXTENDEDKEY:0);

  WAIT;
  SendMessage (handle, WM_KEYDOWN, keycode, lp);
  SendMessage (handle, WM_KEYUP  , keycode, lp | 0xc000000);
  REDRAW;
}

#define listbox_field_ok(t, s, f, got) \
  ok (t.s.f==got.f, "style %#x, step " #s ", field " #f \
      ": expected %d, got %d\n", (unsigned int)t.prop.add_style, \
      t.s.f, got.f)

#define listbox_todo_field_ok(t, s, f, got) \
  if (t.s##_todo.f) todo_wine { listbox_field_ok(t, s, f, got); } \
  else listbox_field_ok(t, s, f, got)

#define listbox_ok(t, s, got) \
  listbox_todo_field_ok(t, s, selected, got); \
  listbox_todo_field_ok(t, s, anchor, got); \
  listbox_todo_field_ok(t, s, caret, got); \
  listbox_todo_field_ok(t, s, selcount, got)

void
check (const struct listbox_test test)
{
  struct listbox_stat answer;
  HWND hLB=create_listbox (test.prop.add_style);
  RECT second_item;

  listbox_query (hLB, &answer);
  listbox_ok (test, init, answer);

  SendMessage (hLB, LB_GETITEMRECT, (WPARAM) 1, (LPARAM) &second_item);
  buttonpress(hLB, (WORD)second_item.left, (WORD)second_item.top);

  listbox_query (hLB, &answer);
  listbox_ok (test, click, answer);

  keypress (hLB, VK_DOWN, 0x50, TRUE);

  listbox_query (hLB, &answer);
  listbox_ok (test, step, answer);

  WAIT;
  DestroyWindow (hLB);
}

START_TEST(listbox)
{
  const struct listbox_test SS =
/*   {add_style} */
    {{0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,1,0,0},
     {     2,      2,      2, LB_ERR}, {0,1,0,0}};
/* {selected, anchor,  caret, selcount}{TODO fields} */
  const struct listbox_test SS_NS =
    {{LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {1,1,0,0},
     {     2,      2,      2, LB_ERR}, {1,1,1,0}};
  const struct listbox_test MS =
    {{LBS_MULTIPLESEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,1,0,0},
     {     2,      1,      2,      1}, {0,1,0,1}};
  const struct listbox_test MS_NS =
    {{LBS_MULTIPLESEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {1,0,0,1},
     {     1,      1,      1, LB_ERR}, {0,1,0,1},
     {     2,      2,      2, LB_ERR}, {0,1,0,1}};

  trace (" Testing single selection...\n");
  check (SS);
  trace (" ... with NOSEL\n");
  check (SS_NS);
  trace (" Testing multiple selection...\n");
  check (MS);
  trace (" ... with NOSEL\n");
  check (MS_NS);
}
