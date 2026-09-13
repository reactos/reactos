/*
 * Unit test suite for comdlg32 API functions: find/replace dialogs
 *
 * Copyright 2010 by Dylan Smith
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
 */

#include "windows.h"
#include "commdlg.h"
#include "cderr.h"
#include "wine/test.h"

static UINT ID_FINDMSGSTRING;

static LRESULT handle_findmsg(FINDREPLACEA *fr)
{
    return 0;
}

static LRESULT CALLBACK OwnerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == ID_FINDMSGSTRING) {
        return handle_findmsg((FINDREPLACEA*)lParam);
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void test_param_check(void)
{
    char findbuffer[64];
    char replacebuffer[64];
    FINDREPLACEA fr, *pFr;
    WNDCLASSA wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = OwnerWndProc;
    wc.lpszClassName = "test_param_check";
    RegisterClassA(&wc);

#define CHECK_FIND_OR_REPLACE(FUNC, FAIL, ERR_CODE) \
    do { \
       HWND hwnd = FUNC(pFr); \
       BOOL is_ok = !!hwnd == !FAIL; \
       ok(is_ok, "%s should%s fail\n", #FUNC, FAIL ? "" : "n't"); \
       if (FAIL && is_ok) { \
          DWORD ext_err = CommDlgExtendedError(); \
          ok(ext_err == ERR_CODE, "expected err %x got %lx\n", \
             ERR_CODE, ext_err); \
       } else { \
          DestroyWindow(hwnd); \
       } \
    } while (0)

#define CHECK_FIND_FAIL(ERR_CODE) \
    CHECK_FIND_OR_REPLACE(FindTextA, TRUE, ERR_CODE)

#define CHECK_FIND_SUCCEED() \
    CHECK_FIND_OR_REPLACE(FindTextA, FALSE, 0)

#define CHECK_REPLACE_FAIL(ERR_CODE) \
    CHECK_FIND_OR_REPLACE(ReplaceTextA, TRUE, ERR_CODE)

#define CHECK_REPLACE_SUCCEED() \
    CHECK_FIND_OR_REPLACE(ReplaceTextA, FALSE, 0)

#define CHECK_FINDREPLACE_FAIL(ERR_CODE) \
    do { \
       CHECK_FIND_FAIL(ERR_CODE); \
       CHECK_REPLACE_FAIL(ERR_CODE); \
    } while (0)

    pFr = NULL;
    CHECK_FINDREPLACE_FAIL(CDERR_INITIALIZATION);
    pFr = &fr;

    ZeroMemory(&fr, sizeof(fr));
    /* invalid lStructSize (0) */
    CHECK_FINDREPLACE_FAIL(CDERR_STRUCTSIZE);
    fr.lStructSize = sizeof(fr);

    /* invalid hwndOwner (NULL) */
    CHECK_FINDREPLACE_FAIL(CDERR_DIALOGFAILURE);
    fr.hwndOwner = CreateWindowA(wc.lpszClassName, NULL, WS_VISIBLE, 0, 0, 200, 100,
                                 NULL, NULL, GetModuleHandleA(NULL), NULL);

    /* invalid wFindWhatLen (0) */
    CHECK_FINDREPLACE_FAIL(FRERR_BUFFERLENGTHZERO);
    fr.wFindWhatLen = sizeof(findbuffer);

    /* invalid lpstrFindWhat (NULL) */
    CHECK_FINDREPLACE_FAIL(FRERR_BUFFERLENGTHZERO);
    fr.lpstrFindWhat = findbuffer;
    strcpy(findbuffer, "abc");

    /* invalid lpstrReplaceWith (NULL) for ReplaceText */
    CHECK_FIND_SUCCEED();
    CHECK_REPLACE_FAIL(FRERR_BUFFERLENGTHZERO);
    fr.lpstrReplaceWith = replacebuffer;
    strcpy(replacebuffer, "def");

    /* wReplaceWithLen may be 0, even for ReplaceText */
    CHECK_FIND_SUCCEED();
    CHECK_REPLACE_SUCCEED();
    fr.wReplaceWithLen = sizeof(replacebuffer);

    /* invalid lpfnHook (NULL) when Flags has FR_ENABLEHOOK */
    fr.Flags = FR_ENABLEHOOK;
    CHECK_FINDREPLACE_FAIL(CDERR_NOHOOK);

    /* invalid hInstance (NULL)
     * when Flags has FR_ENABLETEMPLATE or FR_ENABLETEMPLATEHANDLE */
    fr.Flags = FR_ENABLETEMPLATE;
    CHECK_FINDREPLACE_FAIL(CDERR_FINDRESFAILURE);
    fr.Flags = FR_ENABLETEMPLATEHANDLE;
    CHECK_FINDREPLACE_FAIL(CDERR_NOHINSTANCE);
    fr.hInstance = GetModuleHandleA(NULL);

    /* invalid lpTemplateName (NULL) when Flags has FR_ENABLETEMPLATE */
    fr.Flags = FR_ENABLETEMPLATE;
    CHECK_FINDREPLACE_FAIL(CDERR_FINDRESFAILURE);
    fr.Flags = 0;

    CHECK_FIND_SUCCEED();
    CHECK_REPLACE_SUCCEED();

    DestroyWindow(fr.hwndOwner);
}

START_TEST(finddlg)
{
    ID_FINDMSGSTRING = RegisterWindowMessageA(FINDMSGSTRINGA);

    test_param_check();
}
