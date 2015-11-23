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

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

static const WCHAR sysW[] = {'S','y','s','t','e','m',0};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

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
  ITextSelection *txtSel = NULL, *txtSel2;
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
  EXPECT_REF(reOle, 2);

  res = SendMessageA(w, EM_GETOLEINTERFACE, 0, (LPARAM)&reOle1);
  ok(res == 1, "SendMessage\n");
  ok(reOle1 == reOle, "Should not return a new IRichEditOle interface\n");
  EXPECT_REF(reOle, 3);

  hres = IRichEditOle_QueryInterface(reOle, &IID_ITextDocument,
                                 (void **) &txtDoc);
  ok(hres == S_OK, "IRichEditOle_QueryInterface\n");
  ok(txtDoc != NULL, "IRichEditOle_QueryInterface\n");

  hres = ITextDocument_GetSelection(txtDoc, NULL);
  ok(hres == E_INVALIDARG, "ITextDocument_GetSelection: 0x%x\n", hres);

  EXPECT_REF(txtDoc, 4);

  hres = ITextDocument_GetSelection(txtDoc, &txtSel);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  EXPECT_REF(txtDoc, 4);
  EXPECT_REF(txtSel, 2);

  hres = ITextDocument_GetSelection(txtDoc, &txtSel2);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(txtSel2 == txtSel, "got %p, %p\n", txtSel, txtSel2);

  EXPECT_REF(txtDoc, 4);
  EXPECT_REF(txtSel, 3);

  ITextSelection_Release(txtSel2);

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

  punk = NULL;
  hres = IRichEditOle_QueryInterface(reOle, &IID_IOleClientSite, (void **) &punk);
  ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface\n");

  punk = NULL;
  hres = IRichEditOle_QueryInterface(reOle, &IID_IOleWindow, (void **) &punk);
  ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface\n");

  punk = NULL;
  hres = IRichEditOle_QueryInterface(reOle, &IID_IOleInPlaceSite, (void **) &punk);
  ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface\n");

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
  hres = ITextDocument_Open(txtDoc, &testfile, tomText, CP_ACP);
todo_wine {
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(is_existing_file(filename) == TRUE, "a file should be created default\n");
}
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  /* test of share mode */
  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomShareDenyRead, CP_ACP);
todo_wine
  ok(hres == S_OK, "got 0x%08x\n", hres);
  SetLastError(0xdeadbeef);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomShareDenyWrite, CP_ACP);
todo_wine
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Open(txtDoc, &testfile, tomShareDenyWrite|tomShareDenyRead, CP_ACP);
todo_wine
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Open(txtDoc, &testfile, tomReadOnly, CP_ACP);
todo_wine
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Open(txtDoc, &testfile, tomReadOnly, CP_UTF8);
todo_wine
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Open(txtDoc, &testfile, tomReadOnly, 1200);
todo_wine
  ok(hres == S_OK, "got 0x%08x\n", hres);
  result = SendMessageW(w, WM_GETTEXT, 1024, (LPARAM)bufUnicode);
  todo_wine ok(result == 12, "ITextDocument_Open: Test UTF-16 returned %d, expected 12\n", result);
  result = lstrcmpW(bufUnicode, &chUTF16[1]);
  todo_wine ok(result == 0, "ITextDocument_Open: Test UTF-16 set wrong text: Result: %s\n", wine_dbgstr_w(bufUnicode));
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  VariantClear(&testfile);
}

static void test_GetText(void)
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
  static const WCHAR bufW6[] = {'T','e','s','t','S','o','m','e','T',0};
  BOOL is64bit = sizeof(void *) > sizeof(int);
  ITextRange *range;

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  /* ITextSelection */
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

  /* ITextRange */
  hres = ITextDocument_Range(txtDoc, 0, 4, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 4, 0, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 1, 1, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));
  if (!is64bit)
  {
    hres = ITextRange_GetText(range, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08x\n", hres);
  }
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 8, 12, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!lstrcmpW(bstr, bufW3), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 8, 13, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!lstrcmpW(bstr, bufW2), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 12, 13, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!lstrcmpW(bstr, bufW5), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 0, -1, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, -1, 9, &range);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(!lstrcmpW(bstr, bufW6), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  /* detached selection/range */
  if (is64bit) {
    bstr = (void*)0xdeadbeef;
    hres = ITextSelection_GetText(txtSel, &bstr);
    ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);
todo_wine
    ok(bstr == NULL, "got %p\n", bstr);

    bstr = (void*)0xdeadbeef;
    hres = ITextRange_GetText(range, &bstr);
    ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);
todo_wine
    ok(bstr == NULL, "got %p\n", bstr);
  }
  else {
    hres = ITextSelection_GetText(txtSel, NULL);
    ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

    hres = ITextRange_GetText(range, NULL);
    ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);
  }

  ITextRange_Release(range);
  ITextSelection_Release(txtSel);
}

static void test_ITextDocument_Range(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge, *range2;
  HRESULT hres;
  LONG value;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = ITextDocument_Range(txtDoc, 0, 0, &txtRge);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%x.\n", hres);
  EXPECT_REF(txtRge, 1);

  hres = ITextDocument_Range(txtDoc, 0, 0, &range2);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%x.\n", hres);
  ok(range2 != txtRge, "A new pointer should be returned\n");
  ITextRange_Release(range2);

  hres = ITextDocument_Range(txtDoc, 0, 0, NULL);
  ok(hres == E_INVALIDARG, "ITextDocument_Range should fail 0x%x.\n", hres);

  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  hres = ITextDocument_Range(txtDoc, 8, 30, &range2);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%x.\n", hres);
  hres = ITextRange_GetStart(range2, &value);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(value == 8, "got %d\n", value);

  hres = ITextRange_GetEnd(range2, &value);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(value == 13, "got %d\n", value);
  ITextRange_Release(range2);

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
  LONG pch;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";

  first = 0, lim = 4;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 0, lim = 0;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 12, lim = 12;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 13, lim = 13;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  first = 12, lim = 12;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_GetChar(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetChar\n");

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextRange_GetChar(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextRange_Release(txtRge);
}

static void test_ITextSelection_GetChar(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  LONG pch;
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

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextSelection_GetChar(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextSelection_Release(txtSel);
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
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
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
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 12, "got wrong start value: %d\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 12, "got wrong end value: %d\n", end);

  /* SetStart */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  /* same value */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  hres = ITextRange_SetStart(txtRge, 1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  /* negative resets to 0, return value is S_FALSE when
     position wasn't changed */
  hres = ITextRange_SetStart(txtRge, -1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextRange_SetStart(txtRge, -1);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  start = -1;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 0, "got %d\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextRange_SetStart(txtRge, 1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextRange_SetEnd(txtRge, 3);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextRange_SetStart(txtRge, 10);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 10, "got %d\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 10, "got %d\n", end);

  /* more that total text length */
  hres = ITextRange_SetStart(txtRge, 50);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 12, "got %d\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 12, "got %d\n", end);

  /* SetEnd */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  /* same value */
  hres = ITextRange_SetEnd(txtRge, 5);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextRange_SetEnd(txtRge, 5);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  /* negative resets to 0 */
  hres = ITextRange_SetEnd(txtRge, -1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  end = -1;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 0, "got %d\n", end);

  start = -1;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 0, "got %d\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextRange_SetStart(txtRge, 3);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextRange_SetEnd(txtRge, 1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 1, "got %d\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 1, "got %d\n", end);

  /* more than total count */
  hres = ITextRange_SetEnd(txtRge, 50);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 1, "got %d\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 13, "got %d\n", end);

  /* zero */
  hres = ITextRange_SetEnd(txtRge, 0);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 0, "got %d\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 0, "got %d\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  /* detached range */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_SetEnd(txtRge, 3);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_GetStart(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_GetEnd(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextRange_Release(txtRge);
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

  /* SetStart/SetEnd */
  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  /* same value */
  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  hres = ITextSelection_SetStart(txtSel, 1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  /* negative resets to 0, return value is S_FALSE when
     position wasn't changed */
  hres = ITextSelection_SetStart(txtSel, -1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextSelection_SetStart(txtSel, -1);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  start = -1;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 0, "got %d\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextSelection_SetStart(txtSel, 1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextSelection_SetEnd(txtSel, 3);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextSelection_SetStart(txtSel, 10);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 10, "got %d\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 10, "got %d\n", end);

  /* more that total text length */
  hres = ITextSelection_SetStart(txtSel, 50);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 12, "got %d\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 12, "got %d\n", end);

  /* SetEnd */
  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  /* same value */
  hres = ITextSelection_SetEnd(txtSel, 5);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextSelection_SetEnd(txtSel, 5);
  ok(hres == S_FALSE, "got 0x%08x\n", hres);

  /* negative resets to 0 */
  hres = ITextSelection_SetEnd(txtSel, -1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  end = -1;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 0, "got %d\n", end);

  start = -1;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 0, "got %d\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextSelection_SetStart(txtSel, 3);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  hres = ITextSelection_SetEnd(txtSel, 1);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 1, "got %d\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 1, "got %d\n", end);

  /* more than total count */
  hres = ITextSelection_SetEnd(txtSel, 50);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 1, "got %d\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 13, "got %d\n", end);

  /* zero */
  hres = ITextSelection_SetEnd(txtSel, 0);
  ok(hres == S_OK, "got 0x%08x\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 0, "got %d\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 0, "got %d\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  /* detached selection */
  hres = ITextSelection_GetStart(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextSelection_GetEnd(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextSelection_Release(txtSel);
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
  hres = ITextRange_GetStart(txtRgeDup, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == first, "got wrong value: %d\n", start);
  hres = ITextRange_GetEnd(txtRgeDup, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == lim, "got wrong value: %d\n", end);

  ITextRange_Release(txtRgeDup);

  hres = ITextRange_GetDuplicate(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetDuplicate\n");

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextRange_GetDuplicate(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_GetDuplicate(txtRge, &txtRgeDup);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextRange_Release(txtRge);
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
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, tomTrue);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 4, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 4, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 4, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 4, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, tomFalse);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 8, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 8, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, tomEnd);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 8, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 8, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  /* tomStart is the default */
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, 256);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 4, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 4, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = 6, lim = 6;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, tomEnd);
  ok(hres == S_FALSE, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 6, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 6, "got wrong end value: %d\n", end);
  ITextRange_Release(txtRge);

  first = 8, lim = 8;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == S_FALSE, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(start == 8, "got wrong start value: %d\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(end == 8, "got wrong end value: %d\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextRange_Collapse(txtRge, tomUndefined);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextRange_Release(txtRge);
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

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextSelection_Collapse(txtSel, tomStart);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  hres = ITextSelection_Collapse(txtSel, tomUndefined);
  ok(hres == CO_E_RELEASED, "got 0x%08x\n", hres);

  ITextSelection_Release(txtSel);
}

static void test_GetClientSite(void)
{
  HWND w;
  IRichEditOle *reOle = NULL, *reOle1 = NULL;
  ITextDocument *txtDoc = NULL;
  IOleClientSite *clientSite = NULL, *clientSite1 = NULL, *clientSite2 = NULL;
  IOleWindow *oleWin = NULL, *oleWin1 = NULL;
  IOleInPlaceSite *olePlace = NULL, *olePlace1 = NULL;
  HRESULT hres;
  LONG refcount1, refcount2;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = IRichEditOle_GetClientSite(reOle, &clientSite);
  ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08x\n", hres);
  EXPECT_REF(clientSite, 1);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IRichEditOle, (void **)&reOle1);
  ok(hres == E_NOINTERFACE, "IOleClientSite_QueryInterface: %x\n", hres);

  hres = IRichEditOle_GetClientSite(reOle, &clientSite1);
  ok(hres == S_OK, "got 0x%08x\n", hres);
  ok(clientSite != clientSite1, "got %p, %p\n", clientSite, clientSite1);
  IOleClientSite_Release(clientSite1);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleClientSite, (void **)&clientSite1);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  ok(clientSite == clientSite1, "Should not return a new pointer.\n");
  EXPECT_REF(clientSite, 2);

  /* IOleWindow interface */
  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleWindow, (void **)&oleWin);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  refcount1 = get_refcount((IUnknown *)clientSite);
  refcount2 = get_refcount((IUnknown *)oleWin);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleWindow, (void **)&oleWin1);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  ok(oleWin == oleWin1, "Should not return a new pointer.\n");
  refcount1 = get_refcount((IUnknown *)clientSite);
  refcount2 = get_refcount((IUnknown *)oleWin);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  hres = IOleWindow_QueryInterface(oleWin, &IID_IOleClientSite, (void **)&clientSite2);
  ok(hres == S_OK, "IOleWindow_QueryInterface: 0x%08x\n", hres);
  ok(clientSite2 == clientSite1, "got wrong pointer\n");

  /* IOleInPlaceSite interface */
  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleInPlaceSite, (void **)&olePlace);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  refcount1 = get_refcount((IUnknown *)olePlace);
  refcount2 = get_refcount((IUnknown *)clientSite);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleInPlaceSite, (void **)&olePlace1);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  ok(olePlace == olePlace1, "Should not return a new pointer.\n");
  IOleInPlaceSite_Release(olePlace1);

  hres = IOleWindow_QueryInterface(oleWin, &IID_IOleInPlaceSite, (void **)&olePlace1);
  ok(hres == S_OK, "IOleWindow_QueryInterface: 0x%08x\n", hres);
  refcount1 = get_refcount((IUnknown *)olePlace1);
  refcount2 = get_refcount((IUnknown *)oleWin);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  IOleInPlaceSite_Release(olePlace1);
  IOleInPlaceSite_Release(olePlace);
  IOleWindow_Release(oleWin1);
  IOleWindow_Release(oleWin);
  IOleClientSite_Release(clientSite2);
  IOleClientSite_Release(clientSite1);
  IOleClientSite_Release(clientSite);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_IOleWindow_GetWindow(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  IOleClientSite *clientSite = NULL;
  IOleWindow *oleWin = NULL;
  HRESULT hres;
  HWND hwnd;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = IRichEditOle_GetClientSite(reOle, &clientSite);
  ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08x\n", hres);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleWindow, (void **)&oleWin);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  hres = IOleWindow_GetWindow(oleWin, &hwnd);
  ok(hres == S_OK, "IOleClientSite_GetWindow: 0x%08x\n", hres);
  ok(w == hwnd, "got wrong pointer\n");

  hres = IOleWindow_GetWindow(oleWin, NULL);
  ok(hres == E_INVALIDARG, "IOleClientSite_GetWindow: 0x%08x\n", hres);

  IOleWindow_Release(oleWin);
  IOleClientSite_Release(clientSite);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_IOleInPlaceSite_GetWindow(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  IOleClientSite *clientSite = NULL;
  IOleInPlaceSite *olePlace = NULL;
  HRESULT hres;
  HWND hwnd;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = IRichEditOle_GetClientSite(reOle, &clientSite);
  ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08x\n", hres);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleInPlaceSite, (void **)&olePlace);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08x\n", hres);
  hres = IOleInPlaceSite_GetWindow(olePlace, &hwnd);
  ok(hres == S_OK, "IOleInPlaceSite_GetWindow: 0x%08x\n", hres);
  ok(w == hwnd, "got wrong pointer.\n");

  hres = IOleInPlaceSite_GetWindow(olePlace, NULL);
  ok(hres == E_INVALIDARG, "IOleInPlaceSite_GetWindow: 0x%08x\n", hres);

  IOleInPlaceSite_Release(olePlace);
  IOleClientSite_Release(clientSite);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_GetFont(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range = NULL;
  ITextSelection *selection;
  ITextFont *font, *font2;
  CHARFORMAT2A cf;
  LONG value;
  float size;
  HRESULT hr;
  HWND hwnd;
  BOOL ret;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_GetSelection(doc, &selection);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  hr = ITextSelection_GetFont(selection, &font);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  hr = ITextSelection_GetFont(selection, &font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(font != font2, "got %p, %p\n", font, font2);
  ITextFont_Release(font2);
  ITextFont_Release(font);
  ITextSelection_Release(selection);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 1);

  hr = ITextRange_GetFont(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 2);
  EXPECT_REF(font, 1);

  hr = ITextRange_GetFont(range, &font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(font != font2, "got %p, %p\n", font, font2);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 3);
  EXPECT_REF(font, 1);
  EXPECT_REF(font2, 1);

  ITextFont_Release(font2);

  /* set different font style within a range */
  hr = ITextFont_GetItalic(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetSize(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  size = 0.0;
  hr = ITextFont_GetSize(font, &size);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(size > 0.0, "size %.2f\n", size);

  value = 0;
  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine
  ok(value == GetSystemDefaultLCID(), "got lcid %x, user lcid %x\n", value,
      GetSystemDefaultLCID());

  /* range is non-italic */
  value = tomTrue;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  cf.cbSize = sizeof(CHARFORMAT2A);
  cf.dwMask = CFM_ITALIC|CFM_SIZE;
  cf.dwEffects = CFE_ITALIC;
  cf.yHeight = 24.0;

  SendMessageA(hwnd, EM_SETSEL, 2, 3);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  /* now range is partially italicized */
  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  size = 0.0;
  hr = ITextFont_GetSize(font, &size);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(size == tomUndefined, "size %.2f\n", size);

  ITextFont_Release(font);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetFont(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextRange_GetFont(range, &font2);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextRange_Release(range);
}

static void test_GetPara(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range = NULL;
  ITextPara *para, *para2;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 1);

  hr = ITextRange_GetPara(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextRange_GetPara(range, &para);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 2);
  EXPECT_REF(para, 1);

  hr = ITextRange_GetPara(range, &para2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(para != para2, "got %p, %p\n", para, para2);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 3);
  EXPECT_REF(para, 1);
  EXPECT_REF(para2, 1);

  ITextPara_Release(para);
  ITextPara_Release(para2);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(selection, 2);

  hr = ITextSelection_GetPara(selection, &para);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(selection, 3);
  EXPECT_REF(para, 1);

  hr = ITextSelection_GetPara(selection, &para2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(para != para2, "got %p, %p\n", para, para2);

  ITextPara_Release(para);
  ITextPara_Release(para2);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetPara(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextRange_GetPara(range, &para);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_GetPara(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_GetPara(selection, &para);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_dispatch(void)
{
  static const WCHAR testnameW[] = {'G','e','t','T','e','x','t',0};
  static const WCHAR testname2W[] = {'T','e','x','t',0};
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range = NULL;
  WCHAR *nameW;
  DISPID dispid;
  HRESULT hr;
  UINT count;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, NULL);

  range = NULL;
  hr = ITextDocument_Range(doc, 0, 0, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(range != NULL, "got %p\n", range);

  dispid = 123;
  nameW = (WCHAR*)testnameW;
  hr = ITextRange_GetIDsOfNames(range, &IID_NULL, &nameW, 1, LOCALE_USER_DEFAULT, &dispid);
  ok(hr == DISP_E_UNKNOWNNAME, "got 0x%08x\n", hr);
  ok(dispid == DISPID_UNKNOWN, "got %d\n", dispid);

  dispid = 123;
  nameW = (WCHAR*)testname2W;
  hr = ITextRange_GetIDsOfNames(range, &IID_NULL, &nameW, 1, LOCALE_USER_DEFAULT, &dispid);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(dispid == DISPID_VALUE, "got %d\n", dispid);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  /* try dispatch methods on detached range */
  hr = ITextRange_GetTypeInfoCount(range, &count);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  dispid = 123;
  nameW = (WCHAR*)testname2W;
  hr = ITextRange_GetIDsOfNames(range, &IID_NULL, &nameW, 1, LOCALE_USER_DEFAULT, &dispid);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(dispid == DISPID_VALUE, "got %d\n", dispid);

  ITextRange_Release(range);
}

static void test_detached_font_getters(ITextFont *font, BOOL duplicate)
{
  HRESULT hr, hrexp = duplicate ? S_OK : CO_E_RELEASED;
  LONG value;
  float size;
  BSTR str;

  hr = ITextFont_GetBold(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetBold(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetForeColor(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetForeColor(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetLanguageID(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetName(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetName(font, &str);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetSize(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetSize(font, &size);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetStrikeThrough(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetStrikeThrough(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetSubscript(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetSubscript(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetSuperscript(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetSuperscript(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);

  hr = ITextFont_GetUnderline(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetUnderline(font, &value);
  ok(hr == hrexp, "got 0x%08x\n", hr);
}

static void test_textfont_global_defaults(ITextFont *font)
{
  float valuef;
  LONG value;
  HRESULT hr;
  BSTR str;

  value = tomUndefined;
  hr = ITextFont_GetAllCaps(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetAnimation(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetBackColor(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomAutoColor, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse || value == tomTrue, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetEmboss(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetForeColor(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomAutoColor, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetHidden(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetEngrave(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  valuef = 1.0;
  hr = ITextFont_GetKerning(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == GetSystemDefaultLCID(), "got %d\n", value);

  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(!lstrcmpW(sysW, str), "%s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  value = tomUndefined;
  hr = ITextFont_GetOutline(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  valuef = 1.0;
  hr = ITextFont_GetPosition(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetProtected(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetShadow(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  valuef = 0.0;
  hr = ITextFont_GetSize(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef >= 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetSmallCaps(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  valuef = 1.0;
  hr = ITextFont_GetSpacing(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetStrikeThrough(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetSubscript(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetSuperscript(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetUnderline(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomUndefined;
  hr = ITextFont_GetWeight(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == FW_NORMAL || value == FW_BOLD, "got %d\n", value);
}

static void test_textfont_undefined(ITextFont *font)
{
  float valuef;
  LONG value;
  HRESULT hr;

  value = tomFalse;
  hr = ITextFont_GetAllCaps(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetAnimation(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetBackColor(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetEmboss(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetForeColor(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetHidden(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetEngrave(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  valuef = 0.0;
  hr = ITextFont_GetKerning(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetOutline(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  valuef = 0.0;
  hr = ITextFont_GetPosition(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetProtected(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetShadow(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  valuef = 0.0;
  hr = ITextFont_GetSize(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetSmallCaps(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  valuef = 0.0;
  hr = ITextFont_GetSpacing(font, &valuef);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetStrikeThrough(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetSubscript(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetSuperscript(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetUnderline(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);

  value = tomFalse;
  hr = ITextFont_GetWeight(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUndefined, "got %d\n", value);
}

static inline FLOAT twips_to_points(LONG value)
{
  return value * 72.0 / 1440;
}

static void test_ITextFont(void)
{
  static const WCHAR arialW[] = {'A','r','i','a','l',0};
  static const CHAR test_text1[] = "TestSomeText";
  ITextFont *font, *font2, *font3;
  FLOAT size, position, kerning;
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range = NULL;
  CHARFORMAT2A cf;
  LONG value;
  HRESULT hr;
  HWND hwnd;
  BOOL ret;
  BSTR str;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 10, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_Reset(font, tomUseTwips);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_Reset(font, tomUsePoints);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetName(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  /* default font size unit is point */
  size = 0.0;
  hr = ITextFont_GetSize(font, &size);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  /* set to some non-zero values */
  hr = ITextFont_SetPosition(font, 20.0);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_SetKerning(font, 10.0);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  position = 0.0;
  hr = ITextFont_GetPosition(font, &position);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  kerning = 0.0;
  hr = ITextFont_GetKerning(font, &kerning);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  memset(&cf, 0, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_SIZE|CFM_OFFSET|CFM_KERNING;

  /* CHARFORMAT members are in twips */
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok(size == twips_to_points(cf.yHeight), "got yHeight %d, size %.2f\n", cf.yHeight, size);
  ok(position == twips_to_points(cf.yOffset), "got yOffset %d, position %.2f\n", cf.yOffset, position);
  ok(kerning == twips_to_points(cf.wKerning), "got wKerning %d, kerning %.2f\n", cf.wKerning, kerning);

  hr = ITextFont_Reset(font, tomUseTwips);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_Reset(font, tomUsePoints);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_GetDuplicate(font, &font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_Reset(font2, tomUseTwips);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextFont_Reset(font2, tomUsePoints);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  ITextFont_Release(font2);

  /* default font name */
  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  /* change font name for an inner subrange */
  memset(&cf, 0, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_FACE;
  strcpy(cf.szFaceName, "Arial");

  SendMessageA(hwnd, EM_SETSEL, 3, 4);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  /* still original name */
  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  SendMessageA(hwnd, EM_SETSEL, 1, 2);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  /* name is returned for first position within a range */
  SendMessageA(hwnd, EM_SETSEL, 0, 1);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(!lstrcmpW(str, arialW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  /* GetDuplicate() */
  hr = ITextFont_GetDuplicate(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  EXPECT_REF(range, 2);
  font2 = NULL;
  hr = ITextFont_GetDuplicate(font, &font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  EXPECT_REF(range, 2);

  /* set whole range to italic */
  cf.cbSize = sizeof(CHARFORMAT2A);
  cf.dwMask = CFM_ITALIC;
  cf.dwEffects = CFE_ITALIC;

  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  /* duplicate retains original value */
  value = tomTrue;
  hr = ITextFont_GetItalic(font2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* get a duplicate from a cloned font */
  hr = ITextFont_GetDuplicate(font2, &font3);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ITextFont_Release(font3);

  ITextRange_Release(range);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextFont_GetDuplicate(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  test_detached_font_getters(font, FALSE);
  test_detached_font_getters(font2, TRUE);

  /* get a duplicate of detached font */
  hr = ITextFont_GetDuplicate(font2, &font3);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ITextFont_Release(font3);

  /* reset detached font to undefined */
  value = tomUndefined;
  hr = ITextFont_GetBold(font2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value != tomUndefined, "got %d\n", value);

  /* reset to undefined for detached font */
  hr = ITextFont_Reset(font2, tomUndefined);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_undefined(font2);

  /* font is detached, default means global TOM defaults */
  hr = ITextFont_Reset(font2, tomDefault);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_GetDuplicate(font2, &font3);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_Reset(font2, tomApplyNow);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_Reset(font2, tomApplyLater);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_Reset(font2, tomTrackParms);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_SetItalic(font2, tomUndefined);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextFont_Reset(font2, tomCacheParms);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  test_textfont_global_defaults(font2);

  ITextFont_Release(font3);
  ITextFont_Release(font2);

  font2 = (void*)0xdeadbeef;
  hr = ITextFont_GetDuplicate(font, &font2);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(font2 == NULL, "got %p\n", font2);

  hr = ITextFont_Reset(font, tomDefault);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextFont_Release(font);

  /* Reset() */
  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 10, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = tomUndefined;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value != tomUndefined, "got %d\n", value);

  /* reset to undefined for attached font */
  hr = ITextFont_Reset(font, tomUndefined);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  value = tomUndefined;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value != tomUndefined, "got %d\n", value);

  /* tomCacheParms/tomTrackParms */
  hr = ITextFont_Reset(font, tomCacheParms);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  memset(&cf, 0, sizeof(cf));
  cf.cbSize = sizeof(CHARFORMAT2A);
  cf.dwMask = CFM_ITALIC;

  cf.dwEffects = CFE_ITALIC;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  /* still cached value */
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextFont_Reset(font, tomTrackParms);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  /* switch back to cache - value retained */
  hr = ITextFont_Reset(font, tomCacheParms);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  /* tomApplyLater */
  hr = ITextFont_Reset(font, tomApplyLater);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_SetItalic(font, tomFalse);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  cf.dwEffects = 0;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok((cf.dwEffects & CFE_ITALIC) == CFE_ITALIC, "got 0x%08x\n", cf.dwEffects);

  hr = ITextFont_Reset(font, tomApplyNow);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  cf.dwEffects = 0;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok((cf.dwEffects & CFE_ITALIC) == 0, "got 0x%08x\n", cf.dwEffects);

  hr = ITextFont_SetItalic(font, tomUndefined);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextFont_SetItalic(font, tomAutoColor);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  cf.dwEffects = 0;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok((cf.dwEffects & CFE_ITALIC) == 0, "got 0x%08x\n", cf.dwEffects);

  ITextRange_Release(range);
  ITextFont_Release(font);
  release_interfaces(&hwnd, &reOle, &doc, NULL);
}

static void test_Delete(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range, *range2;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextDocument_Range(doc, 1, 2, &range2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 4, "got %d\n", value);

  /* unit type doesn't matter is count is 0 */
  value = 0;
  hr = ITextRange_Delete(range2, tomSentence, 0, &value);
todo_wine {
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 1, "got %d\n", value);
}
  value = 1;
  hr = ITextRange_Delete(range2, tomCharacter, 0, &value);
todo_wine {
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);
}
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine
  ok(value == 3, "got %d\n", value);

  hr = ITextRange_GetStart(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 1, "got %d\n", value);

  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
todo_wine
  ok(value == 1, "got %d\n", value);

  ITextRange_Release(range);
  ITextRange_Release(range2);
  release_interfaces(&hwnd, &reOle, &doc, NULL);
}

static void test_SetText(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  static const WCHAR textW[] = {'a','b','c','d','e','f','g','h','i',0};
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range, *range2;
  LONG value;
  HRESULT hr;
  HWND hwnd;
  BSTR str;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextDocument_Range(doc, 0, 4, &range2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = 1;
  hr = ITextRange_GetStart(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 4, "got %d\n", value);

  hr = ITextRange_SetText(range, NULL);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = 1;
  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);

  str = SysAllocString(textW);
  hr = ITextRange_SetText(range, str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  SysFreeString(str);

  value = 1;
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 9, "got %d\n", value);

  value = 1;
  hr = ITextRange_GetStart(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);

  str = SysAllocStringLen(NULL, 0);
  hr = ITextRange_SetText(range, str);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  value = 1;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);
  SysFreeString(str);

  ITextRange_Release(range2);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_SetText(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  str = SysAllocStringLen(NULL, 0);
  hr = ITextRange_SetText(range, str);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  SysFreeString(str);

  ITextRange_Release(range);
}

static void test_InRange(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  ITextRange *range, *range2, *range3;
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextDocument_Range(doc, 0, 4, &range2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  /* matches selection */
  hr = ITextDocument_Range(doc, 1, 2, &range3);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_InRange(range, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextRange_InRange(range, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextRange_InRange(range, range2, NULL);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = tomFalse;
  hr = ITextRange_InRange(range, range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  /* selection */
  hr = ITextSelection_InRange(selection, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextSelection_InRange(selection, range2, NULL);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, range2, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, range3, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* seems to work on ITextSelection ranges only */
  value = tomFalse;
  hr = ITextSelection_InRange(selection, (ITextRange*)selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_InRange(range, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextRange_InRange(range, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextRange_InRange(range, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextRange_InRange(range, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* selection */
  hr = ITextSelection_InRange(selection, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextSelection_InRange(selection, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  ITextRange_Release(range);
  ITextRange_Release(range2);
  ITextRange_Release(range3);
  ITextSelection_Release(selection);
}

static void test_ITextRange_IsEqual(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  ITextRange *range, *range2, *range3;
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextDocument_Range(doc, 0, 4, &range2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  /* matches selection */
  hr = ITextDocument_Range(doc, 1, 2, &range3);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_IsEqual(range, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextRange_IsEqual(range, range2, NULL);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = tomFalse;
  hr = ITextRange_IsEqual(range, range2, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, range3, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* selection */
  hr = ITextSelection_IsEqual(selection, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextSelection_IsEqual(selection, range2, NULL);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, range2, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, range3, &value);
  ok(hr == S_FALSE, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* seems to work on ITextSelection ranges only */
  value = tomFalse;
  hr = ITextSelection_IsEqual(selection, (ITextRange*)selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_IsEqual(range, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextRange_IsEqual(range, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* selection */
  hr = ITextSelection_IsEqual(selection, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  hr = ITextSelection_IsEqual(selection, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  ITextRange_Release(range);
  ITextRange_Release(range2);
  ITextRange_Release(range3);
  ITextSelection_Release(selection);
}

static void test_Select(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_Select(range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = 1;
  hr = ITextSelection_GetStart(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);

  hr = ITextRange_Select(range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextSelection_Select(selection);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_Select(range);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_Select(selection);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextRange_Release(range);
  ITextSelection_Release(selection);
}

static void test_GetStoryType(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_GetStoryType(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  value = tomTextFrameStory;
  hr = ITextRange_GetStoryType(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUnknownStory, "got %d\n", value);

  hr = ITextSelection_GetStoryType(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  value = tomTextFrameStory;
  hr = ITextSelection_GetStoryType(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomUnknownStory, "got %d\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetStoryType(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = 123;
  hr = ITextRange_GetStoryType(range, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == 123, "got %d\n", value);

  hr = ITextSelection_GetStoryType(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = 123;
  hr = ITextSelection_GetStoryType(selection, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == 123, "got %d\n", value);

  ITextRange_Release(range);
  ITextSelection_Release(selection);
}

static void test_SetFont(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range, *range2;
  ITextFont *font, *font2;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextDocument_Range(doc, 5, 2, &range2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  EXPECT_REF(range, 1);
  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  EXPECT_REF(range, 2);

  EXPECT_REF(range2, 1);
  hr = ITextRange_GetFont(range2, &font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  EXPECT_REF(range2, 2);

  hr = ITextRange_SetFont(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  /* setting same font, no-op */
  EXPECT_REF(range, 2);
  hr = ITextRange_SetFont(range, font);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  EXPECT_REF(range, 2);

  EXPECT_REF(range2, 2);
  EXPECT_REF(range, 2);
  hr = ITextRange_SetFont(range, font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  EXPECT_REF(range2, 2);
  EXPECT_REF(range, 2);

  /* originally range 0-4 is non-italic */
  value = tomTrue;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomFalse, "got %d\n", value);

  /* set range 5-2 to italic, then set this font to range 0-4 */
  hr = ITextFont_SetItalic(font2, tomTrue);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_SetFont(range, font2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == tomTrue, "got %d\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_SetFont(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextRange_SetFont(range, font);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_SetFont(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = ITextSelection_SetFont(selection, font);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextFont_Release(font);
  ITextFont_Release(font2);
  ITextRange_Release(range);
  ITextRange_Release(range2);
  ITextSelection_Release(selection);
}

static void test_InsertObject(void)
{
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  IOleClientSite *clientsite;
  REOBJECT reo;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reole, &doc, NULL);

  hr = IRichEditOle_InsertObject(reole, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  hr = IRichEditOle_GetClientSite(reole, &clientsite);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  reo.cbStruct = sizeof(reo);
  reo.cp = 0;
  memset(&reo.clsid, 0, sizeof(reo.clsid));
  reo.poleobj = NULL;
  reo.pstg = NULL;
  reo.polesite = clientsite;
  reo.sizel.cx = 10;
  reo.sizel.cy = 10;
  reo.dvaspect = DVASPECT_CONTENT;
  reo.dwFlags = 0;
  reo.dwUser = 0;

  hr = IRichEditOle_InsertObject(reole, &reo);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  IOleClientSite_Release(clientsite);
  release_interfaces(&hwnd, &reole, &doc, NULL);
}

static void test_GetStoryLength(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_GetStoryLength(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  value = 0;
  hr = ITextRange_GetStoryLength(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 13, "got %d\n", value);

  hr = ITextSelection_GetStoryLength(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  value = 0;
  hr = ITextSelection_GetStoryLength(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 13, "got %d\n", value);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");

  value = 0;
  hr = ITextRange_GetStoryLength(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 1, "got %d\n", value);

  value = 0;
  hr = ITextSelection_GetStoryLength(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 1, "got %d\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetStoryLength(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = 100;
  hr = ITextRange_GetStoryLength(range, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == 100, "got %d\n", value);

  hr = ITextSelection_GetStoryLength(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  value = 100;
  hr = ITextSelection_GetStoryLength(selection, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);
  ok(value == 100, "got %d\n", value);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_ITextSelection_GetDuplicate(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection, *sel2;
  ITextRange *range, *range2;
  ITextFont *font;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextSelection_GetDuplicate(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

  EXPECT_REF(selection, 2);

  hr = ITextSelection_GetDuplicate(selection, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextSelection_GetDuplicate(selection, &range2);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(range != range2, "got %p, %p\n", range, range2);

  EXPECT_REF(selection, 2);
  EXPECT_REF(range, 1);
  EXPECT_REF(range2, 1);

  ITextRange_Release(range2);

  value = 0;
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 1, "got %d\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 2, "got %d\n", value);

  SendMessageA(hwnd, EM_SETSEL, 2, 3);

  value = 0;
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 1, "got %d\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 2, "got %d\n", value);

  hr = ITextRange_QueryInterface(range, &IID_ITextSelection, (void**)&sel2);
  ok(hr == E_NOINTERFACE, "got 0x%08x\n", hr);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextSelection_GetDuplicate(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_GetDuplicate(selection, &range);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_Expand(void)
{
  static const char test_text1[] = "TestSomeText";
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reole, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextRange_Expand(range, tomStory, NULL);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 13, "got %d\n", value);

  hr = ITextSelection_Expand(selection, tomStory, NULL);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  hr = ITextSelection_GetStart(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);
  hr = ITextSelection_GetEnd(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 13, "got %d\n", value);

  hr = ITextRange_SetStart(range, 1);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  hr = ITextRange_SetEnd(range, 2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  hr = ITextSelection_SetStart(selection, 1);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  hr = ITextSelection_SetEnd(selection, 2);
  ok(hr == S_OK, "got 0x%08x\n", hr);

  value = 0;
  hr = ITextRange_Expand(range, tomStory, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 12, "got %d\n", value);
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 13, "got %d\n", value);

  value = 0;
  hr = ITextSelection_Expand(selection, tomStory, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 12, "got %d\n", value);
  hr = ITextSelection_GetStart(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 0, "got %d\n", value);
  hr = ITextSelection_GetEnd(selection, &value);
  ok(hr == S_OK, "got 0x%08x\n", hr);
  ok(value == 13, "got %d\n", value);

  release_interfaces(&hwnd, &reole, &doc, NULL);

  hr = ITextRange_Expand(range, tomStory, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextRange_Expand(range, tomStory, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_Expand(selection, tomStory, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  hr = ITextSelection_Expand(selection, tomStory, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08x\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_ITextRange_SetStart(void)
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
  hres = ITextRange_SetStart(txtRge, first);
  ok(hres == S_FALSE, "ITextRange_SetStart\n");

#define TEST_TXTRGE_SETSTART(cp, expected_start, expected_end)  \
  hres = ITextRange_SetStart(txtRge, cp);                       \
  ok(hres == S_OK, "ITextRange_SetStart\n");                    \
  ITextRange_GetStart(txtRge, &start);                          \
  ITextRange_GetEnd(txtRge, &end);                              \
  ok(start == expected_start, "got wrong start value: %d\n", start);  \
  ok(end == expected_end, "got wrong end value: %d\n", end);

  TEST_TXTRGE_SETSTART(2, 2, 8)
  TEST_TXTRGE_SETSTART(-1, 0, 8)
  TEST_TXTRGE_SETSTART(13, 12, 12)

  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextRange_SetEnd(void)
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
  hres = ITextRange_SetEnd(txtRge, lim);
  ok(hres == S_FALSE, "ITextRange_SetEnd\n");

#define TEST_TXTRGE_SETEND(cp, expected_start, expected_end)    \
  hres = ITextRange_SetEnd(txtRge, cp);                         \
  ok(hres == S_OK, "ITextRange_SetEnd\n");                      \
  ITextRange_GetStart(txtRge, &start);                          \
  ITextRange_GetEnd(txtRge, &end);                              \
  ok(start == expected_start, "got wrong start value: %d\n", start);  \
  ok(end == expected_end, "got wrong end value: %d\n", end);

  TEST_TXTRGE_SETEND(6, 4, 6)
  TEST_TXTRGE_SETEND(14, 4, 13)
  TEST_TXTRGE_SETEND(-1, 0, 0)

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextSelection_SetStart(void)
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
  hres = ITextSelection_SetStart(txtSel, first);
  ok(hres == S_FALSE, "ITextSelection_SetStart\n");

#define TEST_TXTSEL_SETSTART(cp, expected_start, expected_end)        \
  hres = ITextSelection_SetStart(txtSel, cp);                         \
  ok(hres == S_OK, "ITextSelection_SetStart\n");                      \
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);           \
  ok(start == expected_start, "got wrong start value: %d\n", start);  \
  ok(end == expected_end, "got wrong end value: %d\n", end);

  TEST_TXTSEL_SETSTART(2, 2, 8)
  TEST_TXTSEL_SETSTART(-1, 0, 8)
  TEST_TXTSEL_SETSTART(13, 12, 12)

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
}

static void test_ITextSelection_SetEnd(void)
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
  hres = ITextSelection_SetEnd(txtSel, lim);
  ok(hres == S_FALSE, "ITextSelection_SetEnd\n");

#define TEST_TXTSEL_SETEND(cp, expected_start, expected_end)          \
  hres = ITextSelection_SetEnd(txtSel, cp);                           \
  ok(hres == S_OK, "ITextSelection_SetEnd\n");                        \
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);           \
  ok(start == expected_start, "got wrong start value: %d\n", start);  \
  ok(end == expected_end, "got wrong end value: %d\n", end);

  TEST_TXTSEL_SETEND(6, 4, 6)
  TEST_TXTSEL_SETEND(14, 4, 13)
  TEST_TXTSEL_SETEND(-1, 0, 0)

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
}

static void test_ITextRange_GetFont(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  ITextFont *txtFont = NULL, *txtFont1 = NULL;
  HRESULT hres;
  int first, lim;
  int refcount;
  static const CHAR test_text1[] = "TestSomeText";
  LONG value;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4, lim = 4;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);

  hres = ITextRange_GetFont(txtRge, &txtFont);
  ok(hres == S_OK, "ITextRange_GetFont\n");
  refcount = get_refcount((IUnknown *)txtFont);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 2, "got wrong ref count: %d\n", refcount);

  hres = ITextRange_GetFont(txtRge, &txtFont1);
  ok(hres == S_OK, "ITextRange_GetFont\n");
  ok(txtFont1 != txtFont, "A new pointer should be return\n");
  refcount = get_refcount((IUnknown *)txtFont1);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  ITextFont_Release(txtFont1);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 2, "got wrong ref count: %d\n", refcount);

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextFont_GetOutline(txtFont, &value);
  ok(hres == CO_E_RELEASED, "ITextFont after ITextDocument destroyed\n");

  ITextFont_Release(txtFont);
}

static void test_ITextSelection_GetFont(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  ITextFont *txtFont = NULL, *txtFont1 = NULL;
  HRESULT hres;
  int first, lim;
  int refcount;
  static const CHAR test_text1[] = "TestSomeText";
  LONG value;

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4, lim = 4;
  SendMessageA(w, EM_SETSEL, first, lim);
  refcount = get_refcount((IUnknown *)txtSel);
  ok(refcount == 2, "got wrong ref count: %d\n", refcount);

  hres = ITextSelection_GetFont(txtSel, &txtFont);
  ok(hres == S_OK, "ITextSelection_GetFont\n");
  refcount = get_refcount((IUnknown *)txtFont);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  refcount = get_refcount((IUnknown *)txtSel);
  ok(refcount == 3, "got wrong ref count: %d\n", refcount);

  hres = ITextSelection_GetFont(txtSel, &txtFont1);
  ok(hres == S_OK, "ITextSelection_GetFont\n");
  ok(txtFont1 != txtFont, "A new pointer should be return\n");
  refcount = get_refcount((IUnknown *)txtFont1);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  ITextFont_Release(txtFont1);
  refcount = get_refcount((IUnknown *)txtSel);
  ok(refcount == 3, "got wrong ref count: %d\n", refcount);

  release_interfaces(&w, &reOle, &txtDoc, &txtSel);

  hres = ITextFont_GetOutline(txtFont, &value);
  ok(hres == CO_E_RELEASED, "ITextFont after ITextDocument destroyed\n");

  ITextFont_Release(txtFont);
}

static void test_ITextRange_GetPara(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  ITextPara *txtPara = NULL, *txtPara1 = NULL;
  HRESULT hres;
  int first, lim;
  int refcount;
  static const CHAR test_text1[] = "TestSomeText";
  LONG value;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4, lim = 4;
  ITextDocument_Range(txtDoc, first, lim, &txtRge);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);

  hres = ITextRange_GetPara(txtRge, &txtPara);
  ok(hres == S_OK, "ITextRange_GetPara\n");
  refcount = get_refcount((IUnknown *)txtPara);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 2, "got wrong ref count: %d\n", refcount);

  hres = ITextRange_GetPara(txtRge, &txtPara1);
  ok(hres == S_OK, "ITextRange_GetPara\n");
  ok(txtPara1 != txtPara, "A new pointer should be return\n");
  refcount = get_refcount((IUnknown *)txtPara1);
  ok(refcount == 1, "got wrong ref count: %d\n", refcount);
  ITextPara_Release(txtPara1);
  refcount = get_refcount((IUnknown *)txtRge);
  ok(refcount == 2, "got wrong ref count: %d\n", refcount);

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextPara_GetStyle(txtPara, &value);
  ok(hres == CO_E_RELEASED, "ITextPara after ITextDocument destroyed\n");

  ITextPara_Release(txtPara);
}

static void test_ITextRange_GetText(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  BSTR bstr = NULL;
  static const CHAR test_text1[] = "TestSomeText";
  static const WCHAR bufW1[] = {'T', 'e', 's', 't', 0};
  static const WCHAR bufW2[] = {'T', 'e', 'x', 't', '\r', 0};
  static const WCHAR bufW3[] = {'T', 'e', 'x', 't', 0};
  static const WCHAR bufW4[] = {'T', 'e', 's', 't', 'S', 'o', 'm',
                                'e', 'T', 'e', 'x', 't', '\r', 0};
  static const WCHAR bufW5[] = {'\r', 0};


#define TEST_TXTRGE_GETTEXT(first, lim, expected_string)                \
  create_interfaces(&w, &reOle, &txtDoc, NULL);                         \
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);                   \
  ITextDocument_Range(txtDoc, first, lim, &txtRge);                     \
  hres = ITextRange_GetText(txtRge, &bstr);                             \
  ok(hres == S_OK, "ITextRange_GetText\n");                             \
  ok(!lstrcmpW(bstr, expected_string), "got wrong text: %s\n", wine_dbgstr_w(bstr)); \
  SysFreeString(bstr);                                                  \
  ITextRange_Release(txtRge);                                           \
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  TEST_TXTRGE_GETTEXT(0, 4, bufW1)
  TEST_TXTRGE_GETTEXT(4, 0, bufW1)
  TEST_TXTRGE_GETTEXT(8, 12, bufW3)
  TEST_TXTRGE_GETTEXT(8, 13, bufW2)
  TEST_TXTRGE_GETTEXT(12, 13, bufW5)
  TEST_TXTRGE_GETTEXT(0, 13, bufW4)
  TEST_TXTRGE_GETTEXT(1, 1, NULL)
}

static void test_ITextRange_SetRange(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  int start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, 0, 0, &txtRge);

#define TEST_TXTRGE_SETRANGE(first, lim, expected_start, expected_end, expected_return) \
  hres = ITextRange_SetRange(txtRge, first, lim);                       \
  ok(hres == expected_return, "ITextRange_SetRange\n");                 \
  ITextRange_GetStart(txtRge, &start);                                  \
  ITextRange_GetEnd(txtRge, &end);                                      \
  ok(start == expected_start, "got wrong start value: %d\n", start);    \
  ok(end == expected_end, "got wrong end value: %d\n", end);

  TEST_TXTRGE_SETRANGE(2, 4, 2, 4, S_OK)
  TEST_TXTRGE_SETRANGE(2, 4, 2, 4, S_FALSE)
  TEST_TXTRGE_SETRANGE(4, 2, 2, 4, S_FALSE)
  TEST_TXTRGE_SETRANGE(14, 14, 12, 12, S_OK)
  TEST_TXTRGE_SETRANGE(15, 15, 12, 12, S_FALSE)
  TEST_TXTRGE_SETRANGE(14, 1, 1, 13, S_OK)
  TEST_TXTRGE_SETRANGE(-1, 4, 0, 4, S_OK)

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextRange_IsEqual2(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge1 = NULL, *txtRge2 = NULL;
  HRESULT hres;
  static const CHAR test_text1[] = "TestSomeText";
  LONG res;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, 2, 4, &txtRge1);
  ITextDocument_Range(txtDoc, 2, 4, &txtRge2);

#define TEST_TXTRGE_ISEQUAL(expected_hres, expected_res)                \
  hres = ITextRange_IsEqual(txtRge1, txtRge2, &res);                    \
  ok(hres == expected_hres, "ITextRange_IsEqual\n");                    \
  ok(res == expected_res, "got wrong return value: %d\n", res);

  TEST_TXTRGE_ISEQUAL(S_OK, tomTrue)
  ITextRange_SetRange(txtRge2, 1, 2);
  TEST_TXTRGE_ISEQUAL(S_FALSE, tomFalse)

  ITextRange_SetRange(txtRge1, 1, 1);
  ITextRange_SetRange(txtRge2, 2, 2);
  TEST_TXTRGE_ISEQUAL(S_FALSE, tomFalse)

  ITextRange_SetRange(txtRge2, 1, 1);
  TEST_TXTRGE_ISEQUAL(S_OK, tomTrue)

  hres = ITextRange_IsEqual(txtRge1, txtRge1, &res);
  ok(hres == S_OK, "ITextRange_IsEqual\n");
  ok(res == tomTrue, "got wrong return value: %d\n", res);

  hres = ITextRange_IsEqual(txtRge1, txtRge2, NULL);
  ok(hres == S_OK, "ITextRange_IsEqual\n");

  hres = ITextRange_IsEqual(txtRge1, NULL, NULL);
  ok(hres == S_FALSE, "ITextRange_IsEqual\n");

  ITextRange_Release(txtRge1);
  ITextRange_Release(txtRge2);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextRange_GetStoryLength(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  LONG count;
  static const CHAR test_text1[] = "TestSomeText";
  int len = strlen(test_text1) + 1;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, 0, 0, &txtRge);

  hres = ITextRange_GetStoryLength(txtRge, &count);
  ok(hres == S_OK, "ITextRange_GetStoryLength\n");
  ok(count == len, "got wrong length: %d\n", count);

  ITextRange_SetRange(txtRge, 1, 2);
  hres = ITextRange_GetStoryLength(txtRge, &count);
  ok(hres == S_OK, "ITextRange_GetStoryLength\n");
  ok(count == len, "got wrong length: %d\n", count);

  hres = ITextRange_GetStoryLength(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetStoryLength\n");

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_ITextSelection_GetStoryLength(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  LONG count;
  static const CHAR test_text1[] = "TestSomeText";
  int len = strlen(test_text1) + 1;

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  hres = ITextSelection_GetStoryLength(txtSel, &count);
  ok(hres == S_OK, "ITextSelection_GetStoryLength\n");
  ok(count == len, "got wrong length: %d\n", count);

  SendMessageA(w, EM_SETSEL, 1, 2);
  hres = ITextSelection_GetStoryLength(txtSel, &count);
  ok(hres == S_OK, "ITextSelection_GetStoryLength\n");
  ok(count == len, "got wrong length: %d\n", count);

  hres = ITextSelection_GetStoryLength(txtSel, NULL);
  ok(hres == E_INVALIDARG, "ITextSelection_GetStoryLength\n");

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
  test_GetText();
  test_ITextSelection_GetChar();
  test_ITextSelection_GetStart_GetEnd();
  test_ITextSelection_SetStart();
  test_ITextSelection_SetEnd();
  test_ITextSelection_Collapse();
  test_ITextSelection_GetFont();
  test_ITextSelection_GetStoryLength();
  test_ITextDocument_Range();
  test_ITextRange_GetChar();
  test_ITextRange_GetStart_GetEnd();
  test_ITextRange_GetDuplicate();
  test_ITextRange_SetStart();
  test_ITextRange_SetEnd();
  test_ITextRange_Collapse();
  test_ITextRange_GetFont();
  test_ITextRange_GetPara();
  test_ITextRange_GetText();
  test_ITextRange_SetRange();
  test_ITextRange_IsEqual2();
  test_ITextRange_GetStoryLength();
  test_GetClientSite();
  test_IOleWindow_GetWindow();
  test_IOleInPlaceSite_GetWindow();
  test_GetFont();
  test_GetPara();
  test_dispatch();
  test_ITextFont();
  test_Delete();
  test_SetText();
  test_InRange();
  test_ITextRange_IsEqual();
  test_Select();
  test_GetStoryType();
  test_SetFont();
  test_InsertObject();
  test_GetStoryLength();
  test_ITextSelection_GetDuplicate();
  test_Expand();
}
