/*
 * PROJECT:     ReactOS utilities
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        apps/utils/getfirefox/getfirefox.c
 * PURPOSE:     Main program
 * COPYRIGHT:   Copyright 2001 John R. Sheets (for CodeWeavers)
 *              Copyright 2004 Mike McCormack (for CodeWeavers)
 *              Copyright 2005 Ge van Geldorp (gvg@reactos.org)
 */
/*
 * Based on Wine dlls/shdocvw/shdocvw_main.c
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


#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define DOWNLOAD_URL L"http://links.reactos.org/getfirefox"

typedef struct _IBindStatusCallbackImpl
  {
    const IBindStatusCallbackVtbl *vtbl;
    LONG ref;
    HWND hDialog;
    BOOL *pbCancelled;
  } IBindStatusCallbackImpl;

static HRESULT WINAPI
dlQueryInterface(IBindStatusCallback* This, REFIID riid, void** ppvObject)
{
  if (NULL == ppvObject)
    {
      return E_POINTER;
    }
    
  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_IBindStatusCallback))
    {
      IBindStatusCallback_AddRef( This );
      *ppvObject = This;
      return S_OK;
    }

  return E_NOINTERFACE;
}

static ULONG WINAPI
dlAddRef(IBindStatusCallback* iface)
{
  IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;
    
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI
dlRelease(IBindStatusCallback* iface)
{
  IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;
  DWORD ref = InterlockedDecrement(&This->ref);
    
  if( !ref )
    {
      DestroyWindow( This->hDialog );
      HeapFree(GetProcessHeap(), 0, This);
    }
    
  return ref;
}

static HRESULT WINAPI
dlOnStartBinding(IBindStatusCallback* iface, DWORD dwReserved, IBinding* pib)
{
  DPRINT1("OnStartBinding not implemented\n");

  return S_OK;
}

static HRESULT WINAPI
dlGetPriority(IBindStatusCallback* iface, LONG* pnPriority)
{
  DPRINT1("GetPriority not implemented\n");

  return S_OK;
}

static HRESULT WINAPI
dlOnLowResource( IBindStatusCallback* iface, DWORD reserved)
{
  DPRINT1("OnLowResource not implemented\n");

  return S_OK;
}

static HRESULT WINAPI
dlOnProgress(IBindStatusCallback* iface, ULONG ulProgress,
             ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
  IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;
  HWND Item;
  LONG r;
  WCHAR OldText[100];

  Item = GetDlgItem(This->hDialog, IDC_PROGRESS);
  if (NULL != Item && 0 != ulProgressMax)
    {
      SendMessageW(Item, PBM_SETPOS, (ulProgress * 100) / ulProgressMax, 0);
    }

  Item = GetDlgItem(This->hDialog, IDC_STATUS);
  if (NULL != Item)
    {
      SendMessageW(Item, WM_GETTEXT, sizeof(OldText) / sizeof(OldText[0]),
                   (LPARAM) OldText);
      if (sizeof(OldText) / sizeof(OldText[0]) - 1 <= wcslen(OldText) ||
          0 != wcscmp(OldText, szStatusText))
        {
          SendMessageW(Item, WM_SETTEXT, 0, (LPARAM) szStatusText);
        }
    }

  SetLastError(0);
  r = GetWindowLongPtrW(This->hDialog, GWLP_USERDATA);
  if (0 != r || 0 != GetLastError())
    {
      *This->pbCancelled = TRUE;
      DPRINT("Cancelled\n");
      return E_ABORT;
    }

  return S_OK;
}

static HRESULT WINAPI
dlOnStopBinding(IBindStatusCallback* iface, HRESULT hresult, LPCWSTR szError)
{
  DPRINT1("OnStopBinding not implemented\n");

  return S_OK;
}

static HRESULT WINAPI
dlGetBindInfo(IBindStatusCallback* iface, DWORD* grfBINDF, BINDINFO* pbindinfo)
{
  DPRINT1("GetBindInfo not implemented\n");

  return S_OK;
}

static HRESULT WINAPI
dlOnDataAvailable(IBindStatusCallback* iface, DWORD grfBSCF,
                  DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
  DPRINT1("OnDataAvailable implemented\n");

  return S_OK;
}

static HRESULT WINAPI
dlOnObjectAvailable(IBindStatusCallback* iface, REFIID riid, IUnknown* punk)
{
  DPRINT1("OnObjectAvailable implemented\n");

  return S_OK;
}

static const IBindStatusCallbackVtbl dlVtbl =
{
    dlQueryInterface,
    dlAddRef,
    dlRelease,
    dlOnStartBinding,
    dlGetPriority,
    dlOnLowResource,
    dlOnProgress,
    dlOnStopBinding,
    dlGetBindInfo,
    dlOnDataAvailable,
    dlOnObjectAvailable
};

static IBindStatusCallback*
CreateDl(HWND Dlg, BOOL *pbCancelled)
{
  IBindStatusCallbackImpl *This;

  This = HeapAlloc(GetProcessHeap(), 0, sizeof(IBindStatusCallbackImpl));
  This->vtbl = &dlVtbl;
  This->ref = 1;
  This->hDialog = Dlg;
  This->pbCancelled = pbCancelled;

  return (IBindStatusCallback*) This;
}

static BOOL
GetShortcutName(LPWSTR ShortcutName)
{
  if (! SHGetSpecialFolderPathW(0, ShortcutName, CSIDL_PROGRAMS, FALSE))
    {
      return FALSE;
    }
  if (NULL == PathAddBackslashW(ShortcutName))
    {
      return FALSE;
    }
  if (0 == LoadStringW(GetModuleHandle(NULL), IDS_START_MENU_NAME,
                       ShortcutName + wcslen(ShortcutName),
                       MAX_PATH - wcslen(ShortcutName)))
    {
      return FALSE;
    }
  if (MAX_PATH - 5 < wcslen(ShortcutName))
    {
      return FALSE;
    }
  wcscat(ShortcutName, L".lnk");

  return TRUE;
}

static DWORD WINAPI
ThreadFunc(LPVOID Context)
{
  static const WCHAR szUrl[] = DOWNLOAD_URL;
  IBindStatusCallback *dl;
  WCHAR path[MAX_PATH], ShortcutName[MAX_PATH];
  LPWSTR p;
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  HWND Dlg = (HWND) Context;
  DWORD r;
  BOOL bCancelled = FALSE;
  BOOL bTempfile = FALSE;

  /* built the path for the download */
  p = wcsrchr(szUrl, L'/');
  if (NULL == p)
    {
      goto end;
    }
  if (! GetTempPathW(MAX_PATH, path))
    {
      goto end;
    }
  wcscat(path, p + 1);

  /* download it */
  bTempfile = TRUE;
  dl = CreateDl(Context, &bCancelled);
  r = URLDownloadToFileW(NULL, szUrl, path, 0, dl);
  if (NULL != dl)
    {
      IBindStatusCallback_Release(dl);
    }
  if (S_OK != r || bCancelled )
    {
      goto end;
    }
  ShowWindow(Dlg, SW_HIDE);

  /* run it */
  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  r = CreateProcessW(path, NULL, NULL, NULL, 0, 0, NULL, NULL, &si, &pi);
  if (0 == r)
    {
      goto end;
    }
  CloseHandle(pi.hThread);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);

  if (BST_CHECKED == SendMessageW(GetDlgItem(Dlg, IDC_REMOVE), BM_GETCHECK,
                                  0, 0) &&
      GetShortcutName(ShortcutName))
    {
      DeleteFileW(ShortcutName);
    }

end:
  if (bTempfile)
    {
      DeleteFileW(path);
    }
  EndDialog(Dlg, 0);
  return 0;
}

static INT_PTR CALLBACK
dlProc(HWND Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HANDLE Thread;
  DWORD ThreadId;
  HWND Item;
  HICON Icon;
  WCHAR ShortcutName[MAX_PATH];

  switch (Msg)
    {
    case WM_INITDIALOG:
      Icon = LoadIconW((HINSTANCE) GetWindowLongPtr(Dlg, GWLP_HINSTANCE),
                       MAKEINTRESOURCEW(IDI_ICON_MAIN));
      if (NULL != Icon)
        {
          SendMessageW(Dlg, WM_SETICON, ICON_BIG, (LPARAM) Icon);
          SendMessageW(Dlg, WM_SETICON, ICON_SMALL, (LPARAM) Icon);
        }
      SetWindowLongPtrW(Dlg, GWLP_USERDATA, 0);
      Item = GetDlgItem(Dlg, IDC_PROGRESS);
      if (NULL != Item)
        {
          SendMessageW(Item, PBM_SETRANGE, 0, MAKELPARAM(0,100));
          SendMessageW(Item, PBM_SETPOS, 0, 0);
        }
      Item = GetDlgItem(Dlg, IDC_REMOVE);
      if (NULL != Item)
        {
          if (GetShortcutName(ShortcutName) &&
              INVALID_FILE_ATTRIBUTES != GetFileAttributesW(ShortcutName))
            {
              SendMessageW(Item, BM_SETCHECK, BST_CHECKED, 0);
            }
          else
            {
              SendMessageW(Item, BM_SETCHECK, BST_UNCHECKED, 0);
              ShowWindow(Item, SW_HIDE);
            }
        }
      Thread = CreateThread(NULL, 0, ThreadFunc, Dlg, 0, &ThreadId);
      if (NULL == Thread)
        {
          return FALSE;
        }
      CloseHandle(Thread);
      return TRUE;

    case WM_COMMAND:
      if (wParam == IDCANCEL)
        {
          SetWindowLongPtrW(Dlg, GWLP_USERDATA, 1);
          PostMessage(Dlg, WM_CLOSE, 0, 0);
        }
      return FALSE;

    case WM_CLOSE:
        EndDialog(Dlg, 0);
        return TRUE;

	default:
      return FALSE;
    }
}


/***********************************************************************
 *              Main program
 */
int
main(int argc, char *argv[])
{
  InitCommonControls();

  DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_GETFIREFOX), 0,
             dlProc);

  return 0;
}
