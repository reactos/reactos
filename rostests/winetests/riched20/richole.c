/*
 * Tests for IRichEditOle and friends.
 *
 * Copyright 2008 Google (Dan Hipschman)
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

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <initguid.h>
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <wine/test.h>

static HMODULE hmoduleRichEdit;

static HWND new_window(LPCSTR lpClassName, DWORD dwStyle, HWND parent)
{
  HWND hwnd = CreateWindowA(lpClassName, NULL,
                            dwStyle | WS_POPUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE,
                            0, 0, 200, 60, parent, NULL, hmoduleRichEdit, NULL);
  return hwnd;
}

static HWND new_richedit(HWND parent)
{
  return new_window(RICHEDIT_CLASS20A, ES_MULTILINE, parent);
}

static BOOL touch_file(LPCWSTR filename)
{
  HANDLE file;

  file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL,
		     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if(file == INVALID_HANDLE_VALUE)
    return FALSE;
  CloseHandle(file);
  return TRUE;
}

static BOOL is_existing_file(LPCWSTR filename)
{
  HANDLE file;

  file = CreateFileW(filename, GENERIC_READ, 0, NULL,
		     OPEN_EXISTING, 0, NULL);
  if(file == INVALID_HANDLE_VALUE)
    return FALSE;
  CloseHandle(file);
  return TRUE;
}

static void create_interfaces(HWND *w, IRichEditOle **reOle, ITextDocument **txtDoc,
                              ITextSelection **txtSel)
{
  *w = new_richedit(NULL);
  SendMessageA(*w, EM_GETOLEINTERFACE, 0, (LPARAM)reOle);
  IRichEditOle_QueryInterface(*reOle, &IID_ITextDocument,
                                 (void **) txtDoc);
  ITextDocument_GetSelection(*txtDoc, txtSel);
}

static void release_interfaces(HWND *w, IRichEditOle **reOle, ITextDocument **txtDoc,
                               ITextSelection **txtSel)
{
  if(txtSel)
    ITextSelection_Release(*txtSel);
  ITextDocument_Release(*txtDoc);
  IRichEditOle_Release(*reOle);
  DestroyWindow(*w);
}

static ULONG get_refcount(IUnknown *iface)
{
  IUnknown_AddRef(iface);
  return IUnknown_Release(iface);
}

static void test_Interfaces(void)
{
  IRichEditOle *reOle = NULL, *reOle1 = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  IUnknown *punk;
  HRESULT hres;
  LRESULT res;
  HWND w;
  ULONG refcount;

  w = new_richedit(NULL);
  if (!w) {
    skip("Couldn't create window\n");
    return;
  }

  res = SendMessageA(w, EM_GETOLEINTERFACE, 0, (LPARAM)&reOle);
  ok(res, "SendMessage\n");
  ok(reOle != NULL, "EM_GETOLEINTERFACE\n");
  refcount = get_refcount((IUnknown *)reOle);
  ok(refcount == 2, "got wrong ref count: %d\n", refcount);

  res = SendMessageA(w, EM_GETOLEINTERFACE, 0, (LPARAM)&reOle1);
  ok(res == 1, "SendMessage\n");
  ok(reOle1 == reOle, "Should not return a new IRichEditOle interface\n");
  refcount = get_refcount((IUnknown *)reOle);
  ok(refcount == 3, "got wrong ref count: %d\n", refcount);

  hres = IRichEditOle_QueryInterface(reOle, &IID_ITextDocument,
                                 (void **) &txtDoc);
  ok(hres == S_OK, "IRichEditOle_QueryInterface\n");
  ok(txtDoc != NULL, "IRichEditOle_QueryInterface\n");

  hres = ITextDocument_GetSelection(txtDoc, NULL);
  ok(hres == E_INVALIDARG, "ITextDocument_GetSelection: 0x%x\n", hres);

  ITextDocument_GetSelection(txtDoc, &txtSel);

  punk = NULL;
  hres = ITextSelection_QueryInterface(txtSel, &IID_ITextSelection, (void **) &punk);
  ok(hres == S_OK, "ITextSelection_QueryInterface\n");
  ok(punk != NULL, "ITextSelection_QueryInterface\n");
  IUnknown_Release(punk);

  punk = NULL;
  hres = ITextSelection_QueryInterface(txtSel, &IID_ITextRange, (void **) &punk);
  ok(hres == S_OK, "ITextSelection_QueryInterface\n");
  ok(punk != NULL, "ITextSelection_QueryInterface\n");
  IUnknown_Release(punk);

  punk = NULL;
  hres = ITextSelection_QueryInterface(txtSel, &IID_IDispatch, (void **) &punk);
  ok(hres == S_OK, "ITextSelection_QueryInterface\n");
  ok(punk != NULL, "ITextSelection_QueryInterface\n");
  IUnknown_Release(punk);

  ITextDocument_Release(txtDoc);
  IRichEditOle_Release(reOle);
  refcount = IRichEditOle_Release(reOle);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  DestroyWindow(w);

  /* Methods should return CO_E_RELEASED if the backing document has
     been released.  One test should suffice.  */
  hres = ITextSelection_CanEdit(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "ITextSelection after ITextDocument destroyed\n");

  ITextSelection_Release(txtSel);
}

static void test_ITextDocument_Open(void)
{
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  HWND w;
  HANDLE hFile;
  VARIANT testfile;
  WCHAR filename[] = {'t', 'e', 's', 't','.','t','x','t', 0};
  int result;
  DWORD dw;
  static const CHAR chACP[] = "TestSomeText";
  static const CHAR chUTF8[] = "\xef\xbb\xbfTextWithUTF8BOM";
  static const WCHAR chUTF16[] = {0xfeff, 'T', 'e', 's', 't', 'S', 'o', 'm',
                                  'e', 'T', 'e', 'x', 't', 0};

#define MAX_BUF_LEN 1024
  CHAR bufACP[MAX_BUF_LEN];
  WCHAR bufUnicode[MAX_BUF_LEN];

  static const int tomConstantsSingle[] =
    {
      tomReadOnly, tomShareDenyRead, tomShareDenyWrite,
      tomCreateAlways, tomOpenExisting, tomOpenAlways,
      tomTruncateExisting, tomRTF, tomText
    };

  static const int tomConstantsMulti[] =
    {
      tomReadOnly|tomShareDenyRead|tomPasteFile, tomReadOnly|tomPasteFile,
      tomReadOnly|tomShareDenyWrite|tomPasteFile,
      tomReadOnly|tomShareDenyRead|tomShareDenyWrite|tomPasteFile, tomShareDenyWrite|tomPasteFile,
      tomShareDenyRead|tomShareDenyWrite|tomPasteFile, tomShareDenyRead|tomPasteFile,
      tomShareDenyRead|tomShareDenyWrite, tomReadOnly|tomShareDenyRead|tomShareDenyWrite,
      tomReadOnly|tomShareDenyWrite, tomReadOnly|tomShareDenyRead
    };

  int tomNumSingle =  sizeof(tomConstantsSingle)/sizeof(tomConstantsSingle[0]);
  int tomNumMulti = sizeof(tomConstantsMulti)/sizeof(tomConstantsMulti[0]);
  int i;

  V_VT(&testfile) = VT_BSTR;
  V_BSTR(&testfile) = SysAllocString(filename);

  for(i=0; i < tomNumSingle; i++)
    {
      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsSingle[i], CP_ACP);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_ACP hres:0x%x\n",
         tomConstantsSingle[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);

      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsSingle[i], CP_UTF8);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_UTF8 hres:0x%x\n",
         tomConstantsSingle[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);
    }

  for(i=0; i < tomNumMulti; i++)
    {
      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsMulti[i], CP_ACP);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_ACP hres:0x%x\n",
         tomConstantsMulti[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);

      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsMulti[i], CP_UTF8);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_UTF8 hres:0x%x\n",
         tomConstantsMulti[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);
    }

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateAlways, CP_ACP);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_ACP\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateAlways, CP_UTF8);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_UTF8\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenAlways, CP_ACP);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_ACP\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenAlways, CP_UTF8);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_UTF8\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_ACP);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_ACP\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_UTF8);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_UTF8\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  touch_file(filename);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_ACP);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "ITextDocument_Open should fail Codepage:CP_ACP\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  touch_file(filename);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_UTF8);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "ITextDocument_Open should fail Codepage:CP_UTF8\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenExisting, CP_ACP);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "ITextDocument_Open should fail Codepage:CP_ACP\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenExisting, CP_UTF8);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "ITextDocument_Open should fail Codepage:CP_UTF8\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);
  ITextDocument_Open(txtDoc, &testfile, tomText, CP_ACP);
  todo_wine ok(is_existing_file(filename) == TRUE, "a file should be created default\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  /* test of share mode */
  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  ITextDocument_Open(txtDoc, &testfile, tomShareDenyRead, CP_ACP);
  SetLastError(0xdeadbeef);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  ITextDocument_Open(txtDoc, &testfile, tomShareDenyWrite, CP_ACP);
  SetLastError(0xdeadbeef);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SetLastError(0xdeadbeef);
  ITextDocument_Open(txtDoc, &testfile, tomShareDenyWrite|tomShareDenyRead, CP_ACP);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  /* tests to check the content */
  hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  WriteFile(hFile, chACP, sizeof(chACP)-sizeof(CHAR), &dw, NULL);
  CloseHandle(hFile);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  ITextDocument_Open(txtDoc, &testfile, tomReadOnly, CP_ACP);
  result = SendMessageA(w, WM_GETTEXT, 1024, (LPARAM)bufACP);
  todo_wine ok(result == 12, "ITextDocument_Open: Test ASCII returned %d, expected 12\n", result);
  result = strcmp(bufACP, chACP);
  todo_wine ok(result == 0, "ITextDocument_Open: Test ASCII set wrong text: Result: %s\n", bufACP);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  WriteFile(hFile, chUTF8, sizeof(chUTF8)-sizeof(CHAR), &dw, NULL);
  CloseHandle(hFile);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  ITextDocument_Open(txtDoc, &testfile, tomReadOnly, CP_UTF8);
  result = SendMessageA(w, WM_GETTEXT, 1024, (LPARAM)bufACP);
  todo_wine ok(result == 15, "ITextDocument_Open: Test UTF-8 returned %d, expected 15\n", result);
  result = strcmp(bufACP, &chUTF8[3]);
  todo_wine ok(result == 0, "ITextDocument_Open: Test UTF-8 set wrong text: Result: %s\n", bufACP);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  WriteFile(hFile, chUTF16, sizeof(chUTF16)-sizeof(WCHAR), &dw, NULL);
  CloseHandle(hFile);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  ITextDocument_Open(txtDoc, &testfile, tomReadOnly, 1200);
  result = SendMessageW(w, WM_GETTEXT, 1024, (LPARAM)bufUnicode);
  todo_wine ok(result == 12, "ITextDocument_Open: Test UTF-16 returned %d, expected 12\n", result);
  result = lstrcmpW(bufUnicode, &chUTF16[1]);
  todo_wine ok(result == 0, "ITextDocument_Open: Test UTF-16 set wrong text: Result: %s\n", wine_dbgstr_w(bufUnicode));
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  VariantClear(&testfile);
}

static void test_ITextSelection_GetText(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  BSTR bstr = NULL;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";
  static const WCHAR bufW1[] = {'T', 'e', 's', 't', 0};
  static const WCHAR bufW2[] = {'T', 'e', 'x', 't', '\r', 0};
  static const WCHAR bufW3[] = {'T', 'e', 'x', 't', 0};
  static const WCHAR bufW4[] = {'T', 'e', 's', 't', 'S', 'o', 'm',
                                'e', 'T', 'e', 'x', 't', '\r', 0};
  static const WCHAR bufW5[] = {'\r', 0};
  BOOL is64bit = sizeof(void *) > sizeof(int);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 0, lim = 4;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 4, lim = 0;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 1, lim = 1;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));

  if (!is64bit)
    {
      hres = ITextSelection_GetText(txtSel, NULL);
      ok(hres == E_INVALIDARG, "ITextSelection_GetText\n");
    }

  first = 8, lim = 12;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW3), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 8, lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW2), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 12, lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW5), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 0, lim = -1;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW4), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = -1, lim = 9;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
}

static void test_ITextDocument_Range(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  ITextRange *pointer = NULL;
  HRESULT hres;
  ULONG refcount;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = ITextDocument_Range(txtDoc, 0, 0, &txtRge);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%x.\n", hres);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 1, "get wrong refcount: returned %d expected 1\n", refcount);

  pointer = txtRge;
  hres = ITextDocument_Range(txtDoc, 0, 0, &txtRge);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%x.\n", hres);
  ok(pointer != txtRge, "A new pointer should be returned\n");
  ITextRange_Release(pointer);

  hres = ITextDocument_Range(txtDoc, 0, 0, NULL);
  ok(hres == E_INVALIDARG, "ITextDocument_Range should fail 0x%x.\n", hres);

  release_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = ITextRange_CanEdit(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "ITextRange after ITextDocument destroyed\n");
  ITextRange_Release(txtRge);
}

static void test_ITextRange_GetChar(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  LONG pch = 0xdeadbeef;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";

  first = 0, lim = 4;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 0, lim = 0;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 12, lim = 12;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 13, lim = 13;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  first = 12, lim = 12;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_GetChar(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetChar\n");
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextSelection_GetChar(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  LONG pch = 0xdeadbeef;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 0, lim = 4;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", pch);

  first = 0, lim = 0;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", pch);

  first = 12, lim = 12;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", pch);

  first = 13, lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", pch);

  hres = ITextSelection_GetChar(txtSel, NULL);
  ok(hres == E_INVALIDARG, "ITextSelection_GetChar\n");

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
}

static void test_ITextRange_GetStart_GetEnd(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  int first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 1, lim = 6;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 1, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 6, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = 6, lim = 1;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 1, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 6, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = -1, lim = 13;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 0, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 13, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = 13, lim = 13;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 12, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 12, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextSelection_GetStart_GetEnd(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  int first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 2, lim = 5;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 2, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 5, "got wrong end value: %d\n", end);

  first = 5, lim = 2;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 2, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 5, "got wrong end value: %d\n", end);

  first = 0, lim = -1;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 0, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 13, "got wrong end value: %d\n", end);

  first = 13, lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 12, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 12, "got wrong end value: %d\n", end);

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
}

static void test_ITextRange_GetDuplicate(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  ITextRange *txtRgeDup = NULL;
  HRESULT hres;
  LONG first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  first = 0, lim = 4;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%x.\n", hres);

  hres = ITextRange_GetDuplicate(txtRge, &txtRgeDup);
  ok(hres == S_OK, "ITextRange_GetDuplicate\n");
  ok(txtRgeDup != txtRge, "A new pointer should be returned\n");
  ITextRange_GetStart(txtRgeDup, &start);
  ok(start == first, "got wrong value: %d\n", start);
  ITextRange_GetEnd(txtRgeDup, &end);
  ok(end == lim, "got wrong value: %d\n", end);

  ITextRange_Release(txtRgeDup);

  hres = ITextRange_GetDuplicate(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetDuplicate\n");

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextRange_Collapse(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  LONG first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4, lim = 8;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, tomTrue);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 4, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 4, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 4, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 4, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, tomFalse);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 8, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 8, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, tomEnd);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 8, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 8, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  /* tomStart is the default */
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, 256);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 4, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 4, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = 6, lim = 6;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, tomEnd);
  ok(hres == S_FALSE, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 6, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 6, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = 8, lim = 8;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == S_FALSE, "ITextRange_Collapse\n");
  ITextRange_GetStart(txtRge, &start);
  ok(start == 8, "got wrong start value: %d\n", start);
  ITextRange_GetEnd(txtRge, &end);
  ok(end == 8, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextSelection_Collapse(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  LONG first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4, lim = 8;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomTrue);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 4, "got wrong start value: %d\n", start);
  ok(end == 4, "got wrong end value: %d\n", end);

  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomStart);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 4, "got wrong start value: %d\n", start);
  ok(end == 4, "got wrong end value: %d\n", end);

  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomFalse);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 8, "got wrong start value: %d\n", start);
  ok(end == 8, "got wrong end value: %d\n", end);

  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomEnd);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 8, "got wrong start value: %d\n", start);
  ok(end == 8, "got wrong end value: %d\n", end);

  /* tomStart is the default */
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, 256);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 4, "got wrong start value: %d\n", start);
  ok(end == 4, "got wrong end value: %d\n", end);

  first = 6, lim = 6;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomEnd);
  ok(hres == S_FALSE, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 6, "got wrong start value: %d\n", start);
  ok(end == 6, "got wrong end value: %d\n", end);

  first = 8, lim = 8;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomStart);
  ok(hres == S_FALSE, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 8, "got wrong start value: %d\n", start);
  ok(end == 8, "got wrong end value: %d\n", end);

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
}

START_TEST(richole)
{
  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED20.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibraryA("riched20.dll");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  test_Interfaces();
  test_ITextDocument_Open();
  test_ITextSelection_GetText();
  test_ITextSelection_GetChar();
  test_ITextSelection_GetStart_GetEnd();
  test_ITextSelection_Collapse();
  test_ITextDocument_Range();
  test_ITextRange_GetChar();
  test_ITextRange_GetStart_GetEnd();
  test_ITextRange_GetDuplicate();
  test_ITextRange_Collapse();
}
