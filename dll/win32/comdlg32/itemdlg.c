/*
 * Common Item Dialog
 *
 * Copyright 2010,2011 David Hedberg
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

#ifndef __REACTOS__ /* Win 7 */

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winreg.h"
#include "shlwapi.h"

#include "commdlg.h"
#include "cdlg.h"
#include "filedlgbrowser.h"

#include "wine/debug.h"
#include "wine/list.h"

#define IDC_NAV_TOOLBAR      200
#define IDC_NAVBACK          201
#define IDC_NAVFORWARD       202

#include <initguid.h>
/* This seems to be another version of IID_IFileDialogCustomize. If
 * there is any difference I have yet to find it. */
DEFINE_GUID(IID_IFileDialogCustomizeAlt, 0x8016B7B3, 0x3D49, 0x4504, 0xA0,0xAA, 0x2A,0x37,0x49,0x4E,0x60,0x6F);

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

static const WCHAR notifysink_childW[] = {'n','f','s','_','c','h','i','l','d',0};
static const WCHAR floatnotifysinkW[] = {'F','l','o','a','t','N','o','t','i','f','y','S','i','n','k',0};
static const WCHAR radiobuttonlistW[] = {'R','a','d','i','o','B','u','t','t','o','n','L','i','s','t',0};

enum ITEMDLG_TYPE {
    ITEMDLG_TYPE_OPEN,
    ITEMDLG_TYPE_SAVE
};

enum ITEMDLG_CCTRL_TYPE {
    IDLG_CCTRL_MENU,
    IDLG_CCTRL_PUSHBUTTON,
    IDLG_CCTRL_COMBOBOX,
    IDLG_CCTRL_RADIOBUTTONLIST,
    IDLG_CCTRL_CHECKBUTTON,
    IDLG_CCTRL_EDITBOX,
    IDLG_CCTRL_SEPARATOR,
    IDLG_CCTRL_TEXT,
    IDLG_CCTRL_OPENDROPDOWN,
    IDLG_CCTRL_VISUALGROUP
};

typedef struct cctrl_item {
    DWORD id, parent_id;
    LPWSTR label;
    CDCONTROLSTATEF cdcstate;
    HWND hwnd;
    struct list entry;
} cctrl_item;

typedef struct {
    HWND hwnd, wrapper_hwnd;
    UINT id, dlgid;
    enum ITEMDLG_CCTRL_TYPE type;
    CDCONTROLSTATEF cdcstate;
    struct list entry;

    struct list sub_cctrls;
    struct list sub_cctrls_entry;
    struct list sub_items;
} customctrl;

typedef struct {
    struct list entry;
    IFileDialogEvents *pfde;
    DWORD cookie;
} events_client;

typedef struct FileDialogImpl {
    IFileDialog2 IFileDialog2_iface;
    union {
        IFileOpenDialog IFileOpenDialog_iface;
        IFileSaveDialog IFileSaveDialog_iface;
    } u;
    enum ITEMDLG_TYPE dlg_type;
    IExplorerBrowserEvents IExplorerBrowserEvents_iface;
    IServiceProvider       IServiceProvider_iface;
    ICommDlgBrowser3       ICommDlgBrowser3_iface;
    IOleWindow             IOleWindow_iface;
    IFileDialogCustomize   IFileDialogCustomize_iface;
    LONG ref;

    FILEOPENDIALOGOPTIONS options;
    COMDLG_FILTERSPEC *filterspecs;
    UINT filterspec_count;
    UINT filetypeindex;

    struct list events_clients;
    DWORD events_next_cookie;

    IShellItemArray *psia_selection;
    IShellItemArray *psia_results;
    IShellItem *psi_defaultfolder;
    IShellItem *psi_setfolder;
    IShellItem *psi_folder;

    HWND dlg_hwnd;
    IExplorerBrowser *peb;
    DWORD ebevents_cookie;

    LPWSTR set_filename;
    LPWSTR default_ext;
    LPWSTR custom_title;
    LPWSTR custom_okbutton;
    LPWSTR custom_cancelbutton;
    LPWSTR custom_filenamelabel;

    UINT cctrl_width, cctrl_def_height, cctrls_cols;
    UINT cctrl_indent;
    HWND cctrls_hwnd;
    struct list cctrls;
    UINT_PTR cctrl_next_dlgid;
    customctrl *cctrl_active_vg;

    HMENU hmenu_opendropdown;
    customctrl cctrl_opendropdown;
    HFONT hfont_opendropdown;
    BOOL opendropdown_has_selection;
    DWORD opendropdown_selection;

    GUID client_guid;
} FileDialogImpl;

/**************************************************************************
 * Event wrappers.
 */
static HRESULT events_OnFileOk(FileDialogImpl *This)
{
    events_client *cursor;
    HRESULT hr = S_OK;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        hr = IFileDialogEvents_OnFileOk(cursor->pfde, (IFileDialog*)&This->IFileDialog2_iface);
        if(FAILED(hr) && hr != E_NOTIMPL)
            break;
    }

    if(hr == E_NOTIMPL)
        hr = S_OK;

    return hr;
}

static HRESULT events_OnFolderChanging(FileDialogImpl *This, IShellItem *folder)
{
    events_client *cursor;
    HRESULT hr = S_OK;
    TRACE("%p (%p)\n", This, folder);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        hr = IFileDialogEvents_OnFolderChanging(cursor->pfde, (IFileDialog*)&This->IFileDialog2_iface, folder);
        if(FAILED(hr) && hr != E_NOTIMPL)
            break;
    }

    if(hr == E_NOTIMPL)
        hr = S_OK;

    return hr;
}

static void events_OnFolderChange(FileDialogImpl *This)
{
    events_client *cursor;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        IFileDialogEvents_OnFolderChange(cursor->pfde, (IFileDialog*)&This->IFileDialog2_iface);
    }
}

static void events_OnSelectionChange(FileDialogImpl *This)
{
    events_client *cursor;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        IFileDialogEvents_OnSelectionChange(cursor->pfde, (IFileDialog*)&This->IFileDialog2_iface);
    }
}

static void events_OnTypeChange(FileDialogImpl *This)
{
    events_client *cursor;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        IFileDialogEvents_OnTypeChange(cursor->pfde, (IFileDialog*)&This->IFileDialog2_iface);
    }
}

static HRESULT events_OnOverwrite(FileDialogImpl *This, IShellItem *shellitem)
{
    events_client *cursor;
    HRESULT hr = S_OK;
    FDE_OVERWRITE_RESPONSE response = FDEOR_DEFAULT;
    TRACE("%p %p\n", This, shellitem);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        TRACE("Notifying %p\n", cursor);
        hr = IFileDialogEvents_OnOverwrite(cursor->pfde, (IFileDialog*)&This->IFileDialog2_iface, shellitem, &response);
        TRACE("<-- hr=%x response=%u\n", hr, response);
        if(FAILED(hr) && hr != E_NOTIMPL)
            break;
    }

    if(hr == E_NOTIMPL)
        hr = S_OK;

    if(SUCCEEDED(hr))
    {
        if (response == FDEOR_DEFAULT)
        {
            WCHAR buf[100];
            int answer;

            LoadStringW(COMDLG32_hInstance, IDS_OVERWRITEFILE, buf, 100);
            answer = MessageBoxW(This->dlg_hwnd, buf, This->custom_title,
                       MB_YESNO | MB_ICONEXCLAMATION);
            if (answer == IDNO || answer == IDCANCEL)
            {
                hr = E_FAIL;
            }
        }
        else if (response == FDEOR_REFUSE)
            hr = E_FAIL;
    }

    return hr;
}

static inline HRESULT get_cctrl_event(IFileDialogEvents *pfde, IFileDialogControlEvents **pfdce)
{
    return IFileDialogEvents_QueryInterface(pfde, &IID_IFileDialogControlEvents, (void**)pfdce);
}

static HRESULT cctrl_event_OnButtonClicked(FileDialogImpl *This, DWORD ctl_id)
{
    events_client *cursor;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        IFileDialogControlEvents *pfdce;
        if(SUCCEEDED(get_cctrl_event(cursor->pfde, &pfdce)))
        {
            TRACE("Notifying %p\n", cursor);
            IFileDialogControlEvents_OnButtonClicked(pfdce, &This->IFileDialogCustomize_iface, ctl_id);
            IFileDialogControlEvents_Release(pfdce);
        }
    }

    return S_OK;
}

static HRESULT cctrl_event_OnItemSelected(FileDialogImpl *This, DWORD ctl_id, DWORD item_id)
{
    events_client *cursor;
    TRACE("%p %i %i\n", This, ctl_id, item_id);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        IFileDialogControlEvents *pfdce;
        if(SUCCEEDED(get_cctrl_event(cursor->pfde, &pfdce)))
        {
            TRACE("Notifying %p\n", cursor);
            IFileDialogControlEvents_OnItemSelected(pfdce, &This->IFileDialogCustomize_iface, ctl_id, item_id);
            IFileDialogControlEvents_Release(pfdce);
        }
    }

    return S_OK;
}

static HRESULT cctrl_event_OnCheckButtonToggled(FileDialogImpl *This, DWORD ctl_id, BOOL checked)
{
    events_client *cursor;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        IFileDialogControlEvents *pfdce;
        if(SUCCEEDED(get_cctrl_event(cursor->pfde, &pfdce)))
        {
            TRACE("Notifying %p\n", cursor);
            IFileDialogControlEvents_OnCheckButtonToggled(pfdce, &This->IFileDialogCustomize_iface, ctl_id, checked);
            IFileDialogControlEvents_Release(pfdce);
        }
    }

    return S_OK;
}

static HRESULT cctrl_event_OnControlActivating(FileDialogImpl *This,
                                                  DWORD ctl_id)
{
    events_client *cursor;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(cursor, &This->events_clients, events_client, entry)
    {
        IFileDialogControlEvents *pfdce;
        if(SUCCEEDED(get_cctrl_event(cursor->pfde, &pfdce)))
        {
            TRACE("Notifying %p\n", cursor);
            IFileDialogControlEvents_OnControlActivating(pfdce, &This->IFileDialogCustomize_iface, ctl_id);
            IFileDialogControlEvents_Release(pfdce);
        }
    }

    return S_OK;
}

/**************************************************************************
 * Helper functions.
 */
static UINT get_file_name(FileDialogImpl *This, LPWSTR *str)
{
    HWND hwnd_edit = GetDlgItem(This->dlg_hwnd, IDC_FILENAME);
    UINT len;

    if(!hwnd_edit)
    {
        if(This->set_filename)
        {
            len = lstrlenW(This->set_filename);
            *str = CoTaskMemAlloc(sizeof(WCHAR)*(len+1));
            lstrcpyW(*str, This->set_filename);
            return len;
        }
        return FALSE;
    }

    len = SendMessageW(hwnd_edit, WM_GETTEXTLENGTH, 0, 0);
    *str = CoTaskMemAlloc(sizeof(WCHAR)*(len+1));
    if(!*str)
        return FALSE;

    SendMessageW(hwnd_edit, WM_GETTEXT, len+1, (LPARAM)*str);
    return len;
}

static BOOL set_file_name(FileDialogImpl *This, LPCWSTR str)
{
    HWND hwnd_edit = GetDlgItem(This->dlg_hwnd, IDC_FILENAME);

    if(This->set_filename)
        LocalFree(This->set_filename);

    This->set_filename = StrDupW(str);

    return SendMessageW(hwnd_edit, WM_SETTEXT, 0, (LPARAM)str);
}

static void fill_filename_from_selection(FileDialogImpl *This)
{
    IShellItem *psi;
    LPWSTR *names;
    HRESULT hr;
    UINT item_count, valid_count;
    UINT len_total, i;

    if(!This->psia_selection)
        return;

    hr = IShellItemArray_GetCount(This->psia_selection, &item_count);
    if(FAILED(hr) || !item_count)
        return;

    names = HeapAlloc(GetProcessHeap(), 0, item_count*sizeof(LPWSTR));

    /* Get names of the selected items */
    valid_count = 0; len_total = 0;
    for(i = 0; i < item_count; i++)
    {
        hr = IShellItemArray_GetItemAt(This->psia_selection, i, &psi);
        if(SUCCEEDED(hr))
        {
            UINT attr;

            hr = IShellItem_GetAttributes(psi, SFGAO_FOLDER, &attr);
            if(SUCCEEDED(hr) &&
               (( (This->options & FOS_PICKFOLDERS) && !(attr & SFGAO_FOLDER)) ||
                (!(This->options & FOS_PICKFOLDERS) &&  (attr & SFGAO_FOLDER))))
                continue;

            hr = IShellItem_GetDisplayName(psi, SIGDN_PARENTRELATIVEPARSING, &names[valid_count]);
            if(SUCCEEDED(hr))
            {
                len_total += lstrlenW(names[valid_count]) + 3;
                valid_count++;
            }
            IShellItem_Release(psi);
        }
    }

    if(valid_count == 1)
    {
        set_file_name(This, names[0]);
        CoTaskMemFree(names[0]);
    }
    else if(valid_count > 1)
    {
        LPWSTR string = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len_total);
        LPWSTR cur_point = string;

        for(i = 0; i < valid_count; i++)
        {
            LPWSTR file = names[i];
            *cur_point++ = '\"';
            lstrcpyW(cur_point, file);
            cur_point += lstrlenW(file);
            *cur_point++ = '\"';
            *cur_point++ = ' ';
            CoTaskMemFree(file);
        }
        *(cur_point-1) = '\0';

        set_file_name(This, string);
        HeapFree(GetProcessHeap(), 0, string);
    }

    HeapFree(GetProcessHeap(), 0, names);
    return;
}

static LPWSTR get_first_ext_from_spec(LPWSTR buf, LPCWSTR spec)
{
    WCHAR *endpos, *ext;

    lstrcpyW(buf, spec);
    if( (endpos = StrChrW(buf, ';')) )
        *endpos = '\0';

    ext = PathFindExtensionW(buf);
    if(StrChrW(ext, '*'))
        return NULL;

    return ext;
}

static BOOL shell_item_exists(IShellItem* shellitem)
{
    LPWSTR filename;
    HRESULT hr;
    BOOL result;

    hr = IShellItem_GetDisplayName(shellitem, SIGDN_FILESYSPATH, &filename);
    if (SUCCEEDED(hr))
    {
        /* FIXME: Implement SFGAO_VALIDATE in Wine and use it instead. */
        result = (GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES);
        CoTaskMemFree(filename);
    }
    else
    {
        SFGAOF attributes;
        result = SUCCEEDED(IShellItem_GetAttributes(shellitem, SFGAO_VALIDATE, &attributes));
    }

    return result;
}

static HRESULT on_default_action(FileDialogImpl *This)
{
    IShellFolder *psf_parent, *psf_desktop;
    LPITEMIDLIST *pidla;
    LPITEMIDLIST current_folder;
    LPWSTR fn_iter, files = NULL, tmp_files;
    UINT file_count = 0, len, i;
    int open_action;
    HRESULT hr, ret = E_FAIL;

    len = get_file_name(This, &tmp_files);
    if(len)
    {
        UINT size_used;
        file_count = COMDLG32_SplitFileNames(tmp_files, len, &files, &size_used);
        CoTaskMemFree(tmp_files);
    }
    if(!file_count) return E_FAIL;

    hr = SHGetIDListFromObject((IUnknown*)This->psi_folder, &current_folder);
    if(FAILED(hr))
    {
        ERR("Failed to get pidl for current directory.\n");
        HeapFree(GetProcessHeap(), 0, files);
        return hr;
    }

    TRACE("Acting on %d file(s).\n", file_count);

    pidla = HeapAlloc(GetProcessHeap(), 0, sizeof(LPITEMIDLIST) * file_count);
    open_action = ONOPEN_OPEN;
    fn_iter = files;

    for(i = 0; i < file_count && open_action == ONOPEN_OPEN; i++)
    {
        WCHAR canon_filename[MAX_PATH];
        psf_parent = NULL;

        COMDLG32_GetCanonicalPath(current_folder, fn_iter, canon_filename);

        if( (This->options & FOS_NOVALIDATE) &&
            !(This->options & FOS_FILEMUSTEXIST) )
            open_action = ONOPEN_OPEN;
        else
            open_action = ONOPEN_BROWSE;

        open_action = FILEDLG95_ValidatePathAction(canon_filename, &psf_parent, This->dlg_hwnd,
                                                   This->options & ~FOS_FILEMUSTEXIST,
                                                   (This->dlg_type == ITEMDLG_TYPE_SAVE),
                                                   open_action);

        /* Add the proper extension */
        if(open_action == ONOPEN_OPEN)
        {
            static const WCHAR dotW[] = {'.',0};

            if(This->dlg_type == ITEMDLG_TYPE_SAVE)
            {
                WCHAR extbuf[MAX_PATH], *newext = NULL;

                if(This->filterspec_count)
                {
                    newext = get_first_ext_from_spec(extbuf, This->filterspecs[This->filetypeindex].pszSpec);
                }
                else if(This->default_ext)
                {
                    lstrcpyW(extbuf, dotW);
                    lstrcatW(extbuf, This->default_ext);
                    newext = extbuf;
                }

                if(newext)
                {
                    WCHAR *ext = PathFindExtensionW(canon_filename);
                    if(lstrcmpW(ext, newext))
                        lstrcatW(canon_filename, newext);
                }
            }
            else
            {
                if( !(This->options & FOS_NOVALIDATE) && (This->options & FOS_FILEMUSTEXIST) &&
                    !PathFileExistsW(canon_filename))
                {
                    if(This->default_ext)
                    {
                        lstrcatW(canon_filename, dotW);
                        lstrcatW(canon_filename, This->default_ext);

                        if(!PathFileExistsW(canon_filename))
                        {
                            FILEDLG95_OnOpenMessage(This->dlg_hwnd, 0, IDS_FILENOTEXISTING);
                            open_action = ONOPEN_BROWSE;
                        }
                    }
                    else
                    {
                        FILEDLG95_OnOpenMessage(This->dlg_hwnd, 0, IDS_FILENOTEXISTING);
                        open_action = ONOPEN_BROWSE;
                    }
                }
            }
        }

        pidla[i] = COMDLG32_SHSimpleIDListFromPathAW(canon_filename);

        if(psf_parent && !(open_action == ONOPEN_BROWSE))
            IShellFolder_Release(psf_parent);

        fn_iter += (WCHAR)lstrlenW(fn_iter) + 1;
    }

    HeapFree(GetProcessHeap(), 0, files);
    ILFree(current_folder);

    if((This->options & FOS_PICKFOLDERS) && open_action == ONOPEN_BROWSE)
        open_action = ONOPEN_OPEN; /* FIXME: Multiple folders? */

    switch(open_action)
    {
    case ONOPEN_SEARCH:
        FIXME("Filtering not implemented.\n");
        break;

    case ONOPEN_BROWSE:
        hr = IExplorerBrowser_BrowseToObject(This->peb, (IUnknown*)psf_parent, SBSP_DEFBROWSER);
        if(FAILED(hr))
            ERR("Failed to browse to directory: %08x\n", hr);

        IShellFolder_Release(psf_parent);
        break;

    case ONOPEN_OPEN:
        hr = SHGetDesktopFolder(&psf_desktop);
        if(SUCCEEDED(hr))
        {
            if(This->psia_results)
            {
                IShellItemArray_Release(This->psia_results);
                This->psia_results = NULL;
            }

            hr = SHCreateShellItemArray(NULL, psf_desktop, file_count, (PCUITEMID_CHILD_ARRAY)pidla,
                                        &This->psia_results);

            IShellFolder_Release(psf_desktop);

            if(FAILED(hr))
                break;

            if(This->options & FOS_PICKFOLDERS)
            {
                SFGAOF attributes;
                hr = IShellItemArray_GetAttributes(This->psia_results, SIATTRIBFLAGS_AND, SFGAO_FOLDER, &attributes);
                if(hr != S_OK)
                {
                    WCHAR buf[64];
                    LoadStringW(COMDLG32_hInstance, IDS_INVALID_FOLDERNAME, buf, sizeof(buf)/sizeof(WCHAR));

                    MessageBoxW(This->dlg_hwnd, buf, This->custom_title, MB_OK | MB_ICONEXCLAMATION);

                    IShellItemArray_Release(This->psia_results);
                    This->psia_results = NULL;
                    break;
                }
            }

            if((This->options & FOS_OVERWRITEPROMPT) && This->dlg_type == ITEMDLG_TYPE_SAVE)
            {
                IShellItem *shellitem;

                for (i=0; SUCCEEDED(hr) && i<file_count; i++)
                {
                    hr = IShellItemArray_GetItemAt(This->psia_results, i, &shellitem);
                    if (SUCCEEDED(hr))
                    {
                        if (shell_item_exists(shellitem))
                            hr = events_OnOverwrite(This, shellitem);

                        IShellItem_Release(shellitem);
                    }
                }

                if (FAILED(hr))
                    break;
            }

            if(events_OnFileOk(This) == S_OK)
                ret = S_OK;
        }
        break;

    default:
        ERR("Failed.\n");
        break;
    }

    /* Clean up */
    for(i = 0; i < file_count; i++)
        ILFree(pidla[i]);
    HeapFree(GetProcessHeap(), 0, pidla);

    /* Success closes the dialog */
    return ret;
}

static void show_opendropdown(FileDialogImpl *This)
{
    HWND open_hwnd;
    RECT open_rc;
    MSG msg;

    open_hwnd = GetDlgItem(This->dlg_hwnd, IDOK);

    GetWindowRect(open_hwnd, &open_rc);

    if (TrackPopupMenu(This->hmenu_opendropdown, 0, open_rc.left, open_rc.bottom, 0, This->dlg_hwnd, NULL) &&
        PeekMessageW(&msg, This->dlg_hwnd, WM_MENUCOMMAND, WM_MENUCOMMAND, PM_REMOVE))
    {
        MENUITEMINFOW mii;

        This->opendropdown_has_selection = TRUE;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID;
        GetMenuItemInfoW((HMENU)msg.lParam, msg.wParam, TRUE, &mii);
        This->opendropdown_selection = mii.wID;

        if(SUCCEEDED(on_default_action(This)))
            EndDialog(This->dlg_hwnd, S_OK);
        else
            This->opendropdown_has_selection = FALSE;
    }
}

/**************************************************************************
 * Control item functions.
 */

static void item_free(cctrl_item *item)
{
    DestroyWindow(item->hwnd);
    HeapFree(GetProcessHeap(), 0, item->label);
    HeapFree(GetProcessHeap(), 0, item);
}

static cctrl_item* get_item(customctrl* parent, DWORD itemid, CDCONTROLSTATEF visible_flags, DWORD* position)
{
    DWORD dummy;
    cctrl_item* item;

    if (!position) position = &dummy;

    *position = 0;

    LIST_FOR_EACH_ENTRY(item, &parent->sub_items, cctrl_item, entry)
    {
        if (item->id == itemid)
            return item;

        if ((item->cdcstate & visible_flags) == visible_flags)
            (*position)++;
    }

    return NULL;
}

static cctrl_item* get_first_item(customctrl* parent)
{
    cctrl_item* item;

    LIST_FOR_EACH_ENTRY(item, &parent->sub_items, cctrl_item, entry)
    {
        if ((item->cdcstate & (CDCS_VISIBLE|CDCS_ENABLED)) == (CDCS_VISIBLE|CDCS_ENABLED))
            return item;
    }

    return NULL;
}

static HRESULT add_item(customctrl* parent, DWORD itemid, LPCWSTR label, cctrl_item** result)
{
    cctrl_item* item;
    LPWSTR label_copy;

    if (get_item(parent, itemid, 0, NULL))
        return E_INVALIDARG;

    item = HeapAlloc(GetProcessHeap(), 0, sizeof(*item));
    label_copy = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(label)+1)*sizeof(WCHAR));

    if (!item || !label_copy)
    {
        HeapFree(GetProcessHeap(), 0, item);
        HeapFree(GetProcessHeap(), 0, label_copy);
        return E_OUTOFMEMORY;
    }

    item->id = itemid;
    item->parent_id = parent->id;
    lstrcpyW(label_copy, label);
    item->label = label_copy;
    item->cdcstate = CDCS_VISIBLE|CDCS_ENABLED;
    item->hwnd = NULL;
    list_add_tail(&parent->sub_items, &item->entry);

    *result = item;

    return S_OK;
}

/**************************************************************************
 * Control functions.
 */
static inline customctrl *get_cctrl_from_dlgid(FileDialogImpl *This, DWORD dlgid)
{
    customctrl *ctrl, *sub_ctrl;

    LIST_FOR_EACH_ENTRY(ctrl, &This->cctrls, customctrl, entry)
    {
        if(ctrl->dlgid == dlgid)
            return ctrl;

        LIST_FOR_EACH_ENTRY(sub_ctrl, &ctrl->sub_cctrls, customctrl, sub_cctrls_entry)
            if(sub_ctrl->dlgid == dlgid)
                return sub_ctrl;
    }

    ERR("Failed to find control with dialog id %d\n", dlgid);
    return NULL;
}

static inline customctrl *get_cctrl(FileDialogImpl *This, DWORD ctlid)
{
    customctrl *ctrl, *sub_ctrl;

    LIST_FOR_EACH_ENTRY(ctrl, &This->cctrls, customctrl, entry)
    {
        if(ctrl->id == ctlid)
            return ctrl;

        LIST_FOR_EACH_ENTRY(sub_ctrl, &ctrl->sub_cctrls, customctrl, sub_cctrls_entry)
            if(sub_ctrl->id == ctlid)
                return sub_ctrl;
    }

    if (This->hmenu_opendropdown && This->cctrl_opendropdown.id == ctlid)
        return &This->cctrl_opendropdown;

    TRACE("No existing control with control id %d\n", ctlid);
    return NULL;
}

static void ctrl_resize(HWND hctrl, UINT min_width, UINT max_width, BOOL multiline)
{
    LPWSTR text;
    UINT len, final_width;
    UINT lines, final_height;
    SIZE size;
    RECT rc;
    HDC hdc;
    WCHAR *c;

    TRACE("\n");

    len = SendMessageW(hctrl, WM_GETTEXTLENGTH, 0, 0);
    text = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(len+1));
    if(!text) return;
    SendMessageW(hctrl, WM_GETTEXT, len+1, (LPARAM)text);

    hdc = GetDC(hctrl);
    GetTextExtentPoint32W(hdc, text, lstrlenW(text), &size);
    ReleaseDC(hctrl, hdc);

    if(len && multiline)
    {
        /* FIXME: line-wrap */
        for(lines = 1, c = text; *c != '\0'; c++)
            if(*c == '\n') lines++;

        final_height = size.cy*lines + 2*4;
    }
    else
    {
        GetWindowRect(hctrl, &rc);
        final_height = rc.bottom - rc.top;
    }

    final_width = min(max(size.cx, min_width) + 4, max_width);
    SetWindowPos(hctrl, NULL, 0, 0, final_width, final_height,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

    HeapFree(GetProcessHeap(), 0, text);
}

static UINT ctrl_get_height(customctrl *ctrl) {
    RECT rc;
    GetWindowRect(ctrl->wrapper_hwnd, &rc);
    return rc.bottom - rc.top;
}

static void ctrl_free(customctrl *ctrl)
{
    customctrl *sub_cur1, *sub_cur2;
    cctrl_item *item_cur1, *item_cur2;

    TRACE("Freeing control %p\n", ctrl);
    if(ctrl->type == IDLG_CCTRL_MENU)
    {
        TBBUTTON tbb;
        SendMessageW(ctrl->hwnd, TB_GETBUTTON, 0, (LPARAM)&tbb);
        DestroyMenu((HMENU)tbb.dwData);
    }

    LIST_FOR_EACH_ENTRY_SAFE(sub_cur1, sub_cur2, &ctrl->sub_cctrls, customctrl, sub_cctrls_entry)
    {
        list_remove(&sub_cur1->sub_cctrls_entry);
        ctrl_free(sub_cur1);
    }

    LIST_FOR_EACH_ENTRY_SAFE(item_cur1, item_cur2, &ctrl->sub_items, cctrl_item, entry)
    {
        list_remove(&item_cur1->entry);
        item_free(item_cur1);
    }

    DestroyWindow(ctrl->hwnd);
    HeapFree(GetProcessHeap(), 0, ctrl);
}

static void customctrl_resize(FileDialogImpl *This, customctrl *ctrl)
{
    RECT rc;
    UINT total_height;
    UINT max_width;
    customctrl *sub_ctrl;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_PUSHBUTTON:
    case IDLG_CCTRL_COMBOBOX:
    case IDLG_CCTRL_CHECKBUTTON:
    case IDLG_CCTRL_TEXT:
        ctrl_resize(ctrl->hwnd, 160, 160, TRUE);
        GetWindowRect(ctrl->hwnd, &rc);
        SetWindowPos(ctrl->wrapper_hwnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                     SWP_NOZORDER|SWP_NOMOVE);
        break;
    case IDLG_CCTRL_VISUALGROUP:
        total_height = 0;
        ctrl_resize(ctrl->hwnd, 0, This->cctrl_indent, TRUE);

        LIST_FOR_EACH_ENTRY(sub_ctrl, &ctrl->sub_cctrls, customctrl, sub_cctrls_entry)
        {
            customctrl_resize(This, sub_ctrl);
            SetWindowPos(sub_ctrl->wrapper_hwnd, NULL, This->cctrl_indent, total_height, 0, 0,
                         SWP_NOZORDER|SWP_NOSIZE);

            total_height += ctrl_get_height(sub_ctrl);
        }

        /* The label should be right adjusted */
        {
            UINT width, height;

            GetWindowRect(ctrl->hwnd, &rc);
            width = rc.right - rc.left;
            height = rc.bottom - rc.top;

            SetWindowPos(ctrl->hwnd, NULL, This->cctrl_indent - width, 0, width, height, SWP_NOZORDER);
        }

        /* Resize the wrapper window to fit all the sub controls */
        SetWindowPos(ctrl->wrapper_hwnd, NULL, 0, 0, This->cctrl_width + This->cctrl_indent, total_height,
                     SWP_NOZORDER|SWP_NOMOVE);
        break;
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        total_height = 0;
        max_width = 0;

        LIST_FOR_EACH_ENTRY(item, &ctrl->sub_items, cctrl_item, entry)
        {
            ctrl_resize(item->hwnd, 160, 160, TRUE);
            SetWindowPos(item->hwnd, NULL, 0, total_height, 0, 0,
                         SWP_NOZORDER|SWP_NOSIZE);

            GetWindowRect(item->hwnd, &rc);

            total_height += rc.bottom - rc.top;
            max_width = max(rc.right - rc.left, max_width);
        }

        SetWindowPos(ctrl->hwnd, NULL, 0, 0, max_width, total_height,
                     SWP_NOZORDER|SWP_NOMOVE);

        SetWindowPos(ctrl->wrapper_hwnd, NULL, 0, 0, max_width, total_height,
                     SWP_NOZORDER|SWP_NOMOVE);

        break;
    }
    case IDLG_CCTRL_EDITBOX:
    case IDLG_CCTRL_SEPARATOR:
    case IDLG_CCTRL_MENU:
    case IDLG_CCTRL_OPENDROPDOWN:
        /* Nothing */
        break;
    }
}

static LRESULT notifysink_on_create(HWND hwnd, CREATESTRUCTW *crs)
{
    FileDialogImpl *This = crs->lpCreateParams;
    TRACE("%p\n", This);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)This);
    return TRUE;
}

static LRESULT notifysink_on_bn_clicked(FileDialogImpl *This, HWND hwnd, WPARAM wparam)
{
    customctrl *ctrl = get_cctrl_from_dlgid(This, LOWORD(wparam));

    TRACE("%p, %lx\n", This, wparam);

    if(ctrl)
    {
        if(ctrl->type == IDLG_CCTRL_CHECKBUTTON)
        {
            BOOL checked = (SendMessageW(ctrl->hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
            cctrl_event_OnCheckButtonToggled(This, ctrl->id, checked);
        }
        else
            cctrl_event_OnButtonClicked(This, ctrl->id);
    }

    return TRUE;
}

static LRESULT notifysink_on_cbn_selchange(FileDialogImpl *This, HWND hwnd, WPARAM wparam)
{
    customctrl *ctrl = get_cctrl_from_dlgid(This, LOWORD(wparam));
    TRACE("%p, %p (%lx)\n", This, ctrl, wparam);

    if(ctrl)
    {
        UINT index = SendMessageW(ctrl->hwnd, CB_GETCURSEL, 0, 0);
        UINT selid = SendMessageW(ctrl->hwnd, CB_GETITEMDATA, index, 0);

        cctrl_event_OnItemSelected(This, ctrl->id, selid);
    }
    return TRUE;
}

static LRESULT notifysink_on_tvn_dropdown(FileDialogImpl *This, LPARAM lparam)
{
    NMTOOLBARW *nmtb = (NMTOOLBARW*)lparam;
    customctrl *ctrl = get_cctrl_from_dlgid(This, GetDlgCtrlID(nmtb->hdr.hwndFrom));
    POINT pt = { 0, nmtb->rcButton.bottom };
    TBBUTTON tbb;
    UINT idcmd;

    TRACE("%p, %p (%lx)\n", This, ctrl, lparam);

    if(ctrl)
    {
        cctrl_event_OnControlActivating(This,ctrl->id);

        SendMessageW(ctrl->hwnd, TB_GETBUTTON, 0, (LPARAM)&tbb);
        ClientToScreen(ctrl->hwnd, &pt);
        idcmd = TrackPopupMenu((HMENU)tbb.dwData, TPM_RETURNCMD, pt.x, pt.y, 0, This->dlg_hwnd, NULL);
        if(idcmd)
            cctrl_event_OnItemSelected(This, ctrl->id, idcmd);
    }

    return TBDDRET_DEFAULT;
}

static LRESULT notifysink_on_wm_command(FileDialogImpl *This, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(HIWORD(wparam))
    {
    case BN_CLICKED:          return notifysink_on_bn_clicked(This, hwnd, wparam);
    case CBN_SELCHANGE:       return notifysink_on_cbn_selchange(This, hwnd, wparam);
    }

    return FALSE;
}

static LRESULT notifysink_on_wm_notify(FileDialogImpl *This, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    NMHDR *nmhdr = (NMHDR*)lparam;

    switch(nmhdr->code)
    {
    case TBN_DROPDOWN:        return notifysink_on_tvn_dropdown(This, lparam);
    }

    return FALSE;
}

static LRESULT CALLBACK notifysink_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    FileDialogImpl *This = (FileDialogImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    customctrl *ctrl;
    HWND hwnd_child;
    RECT rc;

    switch(message)
    {
    case WM_NCCREATE:         return notifysink_on_create(hwnd, (CREATESTRUCTW*)lparam);
    case WM_COMMAND:          return notifysink_on_wm_command(This, hwnd, wparam, lparam);
    case WM_NOTIFY:           return notifysink_on_wm_notify(This, hwnd, wparam, lparam);
    case WM_SIZE:
        hwnd_child = GetPropW(hwnd, notifysink_childW);
        ctrl = (customctrl*)GetWindowLongPtrW(hwnd_child, GWLP_USERDATA);
        if(ctrl && ctrl->type != IDLG_CCTRL_VISUALGROUP)
        {
            GetClientRect(hwnd, &rc);
            SetWindowPos(hwnd_child, NULL, 0, 0, rc.right, rc.bottom, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
        }
        return TRUE;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

static HRESULT cctrl_create_new(FileDialogImpl *This, DWORD id,
                                LPCWSTR text, LPCWSTR wndclass, DWORD ctrl_wsflags,
                                DWORD ctrl_exflags, UINT height, customctrl **ppctrl)
{
    HWND ns_hwnd, control_hwnd, parent_hwnd;
    DWORD wsflags = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
    customctrl *ctrl;

    if(get_cctrl(This, id))
        return E_UNEXPECTED; /* Duplicate id */

    if(This->cctrl_active_vg)
        parent_hwnd = This->cctrl_active_vg->wrapper_hwnd;
    else
        parent_hwnd = This->cctrls_hwnd;

    ns_hwnd = CreateWindowExW(0, floatnotifysinkW, NULL, wsflags,
                              0, 0, This->cctrl_width, height, parent_hwnd,
                              (HMENU)This->cctrl_next_dlgid, COMDLG32_hInstance, This);
    control_hwnd = CreateWindowExW(ctrl_exflags, wndclass, text, wsflags | ctrl_wsflags,
                                   0, 0, This->cctrl_width, height, ns_hwnd,
                                   (HMENU)This->cctrl_next_dlgid, COMDLG32_hInstance, 0);

    if(!ns_hwnd || !control_hwnd)
    {
        ERR("Failed to create wrapper (%p) or control (%p)\n", ns_hwnd, control_hwnd);
        DestroyWindow(ns_hwnd);
        DestroyWindow(control_hwnd);

        return E_FAIL;
    }

    SetPropW(ns_hwnd, notifysink_childW, control_hwnd);

    ctrl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(customctrl));
    if(!ctrl)
        return E_OUTOFMEMORY;

    ctrl->hwnd = control_hwnd;
    ctrl->wrapper_hwnd = ns_hwnd;
    ctrl->id = id;
    ctrl->dlgid = This->cctrl_next_dlgid;
    ctrl->cdcstate = CDCS_ENABLED | CDCS_VISIBLE;
    list_init(&ctrl->sub_cctrls);
    list_init(&ctrl->sub_items);

    if(This->cctrl_active_vg)
        list_add_tail(&This->cctrl_active_vg->sub_cctrls, &ctrl->sub_cctrls_entry);
    else
        list_add_tail(&This->cctrls, &ctrl->entry);

    SetWindowLongPtrW(ctrl->hwnd, GWLP_USERDATA, (LPARAM)ctrl);

    if(ppctrl) *ppctrl = ctrl;

    This->cctrl_next_dlgid++;
    return S_OK;
}

/**************************************************************************
 * Container functions.
 */
static UINT ctrl_container_resize(FileDialogImpl *This, UINT container_width)
{
    UINT container_height;
    UINT column_width;
    UINT nr_of_cols;
    UINT max_control_height, total_height = 0;
    UINT cur_col_pos, cur_row_pos;
    customctrl *ctrl;
    BOOL fits_height;
    static const UINT cspacing = 90;    /* Columns are spaced with 90px */
    static const UINT rspacing = 4;     /* Rows are spaced with 4 px. */

    /* Given the new width of the container, this function determines the
     * needed height of the container and places the controls according to
     * the new layout. Returns the new height.
     */

    TRACE("%p\n", This);

    column_width = This->cctrl_width + cspacing;
    nr_of_cols = (container_width - This->cctrl_indent + cspacing) / column_width;

    /* We don't need to do anything unless the number of visible columns has changed. */
    if(nr_of_cols == This->cctrls_cols)
    {
        RECT rc;
        GetWindowRect(This->cctrls_hwnd, &rc);
        return rc.bottom - rc.top;
    }

    This->cctrls_cols = nr_of_cols;

    /* Get the size of the tallest control, and the total size of
     * all the controls to figure out the number of slots we need.
     */
    max_control_height = 0;
    LIST_FOR_EACH_ENTRY(ctrl, &This->cctrls, customctrl, entry)
    {
        if(ctrl->cdcstate & CDCS_VISIBLE)
        {
            UINT control_height = ctrl_get_height(ctrl);
            max_control_height = max(max_control_height, control_height);

            total_height +=  control_height + rspacing;
        }
    }

    if(!total_height)
        return 0;

    container_height = max(total_height / nr_of_cols, max_control_height + rspacing);
    TRACE("Guess: container_height: %d\n",container_height);

    /* Incrementally increase container_height until all the controls
     * fit.
     */
    do {
        UINT columns_needed = 1;
        cur_row_pos = 0;

        fits_height = TRUE;
        LIST_FOR_EACH_ENTRY(ctrl, &This->cctrls, customctrl, entry)
        {
            if(ctrl->cdcstate & CDCS_VISIBLE)
            {
                UINT control_height = ctrl_get_height(ctrl);

                if(cur_row_pos + control_height > container_height)
                {
                    if(++columns_needed > nr_of_cols)
                    {
                        container_height += 1;
                        fits_height = FALSE;
                        break;
                    }
                    cur_row_pos = 0;
                }

                cur_row_pos += control_height + rspacing;
            }
        }
    } while(!fits_height);

    TRACE("Final container height: %d\n", container_height);

    /* Move the controls to their final destination
     */
    cur_col_pos = 0, cur_row_pos = 0;
    LIST_FOR_EACH_ENTRY(ctrl, &This->cctrls, customctrl, entry)
    {
        if(ctrl->cdcstate & CDCS_VISIBLE)
        {
            RECT rc;
            UINT control_height, control_indent;
            GetWindowRect(ctrl->wrapper_hwnd, &rc);
            control_height = rc.bottom - rc.top;

            if(cur_row_pos + control_height > container_height)
            {
                cur_row_pos = 0;
                cur_col_pos += This->cctrl_width + cspacing;
            }


            if(ctrl->type == IDLG_CCTRL_VISUALGROUP)
                control_indent = 0;
            else
                control_indent = This->cctrl_indent;

            SetWindowPos(ctrl->wrapper_hwnd, NULL, cur_col_pos + control_indent, cur_row_pos, 0, 0,
                         SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);

            cur_row_pos += control_height + rspacing;
        }
    }

    /* Sanity check */
    if(cur_row_pos + This->cctrl_width > container_width)
        ERR("-- Failed to place controls properly.\n");

    return container_height;
}

static void ctrl_container_reparent(FileDialogImpl *This, HWND parent)
{
    LONG wndstyle;

    if(parent)
    {
        customctrl *ctrl, *sub_ctrl;
        HFONT font;

        wndstyle = GetWindowLongW(This->cctrls_hwnd, GWL_STYLE);
        wndstyle &= ~(WS_POPUP);
        wndstyle |= WS_CHILD;
        SetWindowLongW(This->cctrls_hwnd, GWL_STYLE, wndstyle);

        SetParent(This->cctrls_hwnd, parent);
        ShowWindow(This->cctrls_hwnd, TRUE);

        /* Set the fonts to match the dialog font. */
        font = (HFONT)SendMessageW(parent, WM_GETFONT, 0, 0);
        if(!font)
            ERR("Failed to get font handle from dialog.\n");

        LIST_FOR_EACH_ENTRY(ctrl, &This->cctrls, customctrl, entry)
        {
            if(font) SendMessageW(ctrl->hwnd, WM_SETFONT, (WPARAM)font, TRUE);

            /* If this is a VisualGroup */
            LIST_FOR_EACH_ENTRY(sub_ctrl, &ctrl->sub_cctrls, customctrl, sub_cctrls_entry)
            {
                if(font) SendMessageW(sub_ctrl->hwnd, WM_SETFONT, (WPARAM)font, TRUE);
            }

            if (ctrl->type == IDLG_CCTRL_RADIOBUTTONLIST)
            {
                cctrl_item* item;
                LIST_FOR_EACH_ENTRY(item, &ctrl->sub_items, cctrl_item, entry)
                {
                    if (font) SendMessageW(item->hwnd, WM_SETFONT, (WPARAM)font, TRUE);
                }
            }

            customctrl_resize(This, ctrl);
        }
    }
    else
    {
        ShowWindow(This->cctrls_hwnd, FALSE);

        wndstyle = GetWindowLongW(This->cctrls_hwnd, GWL_STYLE);
        wndstyle &= ~(WS_CHILD);
        wndstyle |= WS_POPUP;
        SetWindowLongW(This->cctrls_hwnd, GWL_STYLE, wndstyle);

        SetParent(This->cctrls_hwnd, NULL);
    }
}

static LRESULT ctrl_container_on_create(HWND hwnd, CREATESTRUCTW *crs)
{
    FileDialogImpl *This = crs->lpCreateParams;
    TRACE("%p\n", This);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)This);
    return TRUE;
}

static LRESULT ctrl_container_on_wm_destroy(FileDialogImpl *This)
{
    customctrl *cur1, *cur2;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY_SAFE(cur1, cur2, &This->cctrls, customctrl, entry)
    {
        list_remove(&cur1->entry);
        ctrl_free(cur1);
    }

    return TRUE;
}

static LRESULT CALLBACK ctrl_container_wndproc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    FileDialogImpl *This = (FileDialogImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch(umessage)
    {
    case WM_NCCREATE:         return ctrl_container_on_create(hwnd, (CREATESTRUCTW*)lparam);
    case WM_DESTROY:          return ctrl_container_on_wm_destroy(This);
    default:                  return DefWindowProcW(hwnd, umessage, wparam, lparam);
    }

    return FALSE;
}

static void radiobuttonlist_set_selected_item(FileDialogImpl *This, customctrl *ctrl, cctrl_item *item)
{
    cctrl_item *cursor;

    LIST_FOR_EACH_ENTRY(cursor, &ctrl->sub_items, cctrl_item, entry)
    {
        SendMessageW(cursor->hwnd, BM_SETCHECK, (cursor == item) ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static LRESULT radiobuttonlist_on_bn_clicked(FileDialogImpl *This, HWND hwnd, HWND child)
{
    DWORD ctrl_id = (DWORD)GetWindowLongPtrW(hwnd, GWLP_ID);
    customctrl *ctrl;
    cctrl_item *item;
    BOOL found_item=FALSE;

    ctrl = get_cctrl_from_dlgid(This, ctrl_id);

    if (!ctrl)
    {
        ERR("Can't find this control\n");
        return 0;
    }

    LIST_FOR_EACH_ENTRY(item, &ctrl->sub_items, cctrl_item, entry)
    {
        if (item->hwnd == child)
        {
            found_item = TRUE;
            break;
        }
    }

    if (!found_item)
    {
        ERR("Can't find control item\n");
        return 0;
    }

    radiobuttonlist_set_selected_item(This, ctrl, item);

    cctrl_event_OnItemSelected(This, ctrl->id, item->id);

    return 0;
}

static LRESULT radiobuttonlist_on_wm_command(FileDialogImpl *This, HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    switch(HIWORD(wparam))
    {
    case BN_CLICKED:          return radiobuttonlist_on_bn_clicked(This, hwnd, (HWND)lparam);
    }

    return FALSE;
}

static LRESULT CALLBACK radiobuttonlist_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    FileDialogImpl *This = (FileDialogImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch(message)
    {
    case WM_COMMAND:        return radiobuttonlist_on_wm_command(This, hwnd, wparam, lparam);
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

static HRESULT init_custom_controls(FileDialogImpl *This)
{
    WNDCLASSW wc;
    static const WCHAR ctrl_container_classname[] =
        {'i','d','l','g','_','c','o','n','t','a','i','n','e','r','_','p','a','n','e',0};

    InitCommonControlsEx(NULL);

    This->cctrl_width = 160;      /* Controls have a fixed width */
    This->cctrl_indent = 100;
    This->cctrl_def_height = 23;
    This->cctrls_cols = 0;

    This->cctrl_next_dlgid = 0x2000;
    list_init(&This->cctrls);
    This->cctrl_active_vg = NULL;

    if( !GetClassInfoW(COMDLG32_hInstance, ctrl_container_classname, &wc) )
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = ctrl_container_wndproc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = COMDLG32_hInstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = ctrl_container_classname;

        if(!RegisterClassW(&wc)) return E_FAIL;
    }

    This->cctrls_hwnd = CreateWindowExW(0, ctrl_container_classname, NULL,
                                        WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                        0, 0, 0, 0, NULL, 0,
                                        COMDLG32_hInstance, This);
    if(!This->cctrls_hwnd)
        return E_FAIL;

    SetWindowLongW(This->cctrls_hwnd, GWL_STYLE, WS_TABSTOP);

    /* Register class for  */
    if( !GetClassInfoW(COMDLG32_hInstance, floatnotifysinkW, &wc) ||
        wc.hInstance != COMDLG32_hInstance)
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = notifysink_proc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = COMDLG32_hInstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = floatnotifysinkW;

        if (!RegisterClassW(&wc))
            ERR("Failed to register FloatNotifySink window class.\n");
    }

    if( !GetClassInfoW(COMDLG32_hInstance, radiobuttonlistW, &wc) ||
        wc.hInstance != COMDLG32_hInstance)
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = radiobuttonlist_proc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = COMDLG32_hInstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = radiobuttonlistW;

        if (!RegisterClassW(&wc))
            ERR("Failed to register RadioButtonList window class.\n");
    }

    return S_OK;
}

/**************************************************************************
 * Window related functions.
 */
static BOOL update_open_dropdown(FileDialogImpl *This)
{
    /* Show or hide the open dropdown button as appropriate */
    BOOL show=FALSE, showing;
    HWND open_hwnd, dropdown_hwnd;

    if (This->hmenu_opendropdown)
    {
        INT num_visible_items=0;
        cctrl_item* item;

        LIST_FOR_EACH_ENTRY(item, &This->cctrl_opendropdown.sub_items, cctrl_item, entry)
        {
            if (item->cdcstate & CDCS_VISIBLE)
            {
                num_visible_items++;
                if (num_visible_items >= 2)
                {
                    show = TRUE;
                    break;
                }
            }
        }
    }

    open_hwnd = GetDlgItem(This->dlg_hwnd, IDOK);
    dropdown_hwnd = GetDlgItem(This->dlg_hwnd, psh1);

    showing = (GetWindowLongPtrW(dropdown_hwnd, GWL_STYLE) & WS_VISIBLE) != 0;

    if (showing != show)
    {
        RECT open_rc, dropdown_rc;

        GetWindowRect(open_hwnd, &open_rc);
        GetWindowRect(dropdown_hwnd, &dropdown_rc);

        if (show)
        {
            ShowWindow(dropdown_hwnd, SW_SHOW);

            SetWindowPos(open_hwnd, NULL, 0, 0,
                (open_rc.right - open_rc.left) - (dropdown_rc.right - dropdown_rc.left),
                open_rc.bottom - open_rc.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        }
        else
        {
            ShowWindow(dropdown_hwnd, SW_HIDE);

            SetWindowPos(open_hwnd, NULL, 0, 0,
                (open_rc.right - open_rc.left) + (dropdown_rc.right - dropdown_rc.left),
                open_rc.bottom - open_rc.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        }
    }

    return show;
}

static void update_layout(FileDialogImpl *This)
{
    HDWP hdwp;
    HWND hwnd;
    RECT dialog_rc;
    RECT cancel_rc, dropdown_rc, open_rc;
    RECT filetype_rc, filename_rc, filenamelabel_rc;
    RECT toolbar_rc, ebrowser_rc, customctrls_rc;
    static const UINT vspacing = 4, hspacing = 4;
    static const UINT min_width = 320, min_height = 200;
    BOOL show_dropdown;

    if (!GetClientRect(This->dlg_hwnd, &dialog_rc))
    {
        TRACE("Invalid dialog window, not updating layout\n");
        return;
    }

    if(dialog_rc.right < min_width || dialog_rc.bottom < min_height)
    {
        TRACE("Dialog size (%d, %d) too small, not updating layout\n", dialog_rc.right, dialog_rc.bottom);
        return;
    }

    /****
     * Calculate the size of the dialog and all the parts.
     */

    /* Cancel button */
    hwnd = GetDlgItem(This->dlg_hwnd, IDCANCEL);
    if(hwnd)
    {
        int cancel_width, cancel_height;
        GetWindowRect(hwnd, &cancel_rc);
        cancel_width = cancel_rc.right - cancel_rc.left;
        cancel_height = cancel_rc.bottom - cancel_rc.top;

        cancel_rc.left = dialog_rc.right - cancel_width - hspacing;
        cancel_rc.top = dialog_rc.bottom - cancel_height - vspacing;
        cancel_rc.right = cancel_rc.left + cancel_width;
        cancel_rc.bottom = cancel_rc.top + cancel_height;
    }

    /* Open/Save dropdown */
    show_dropdown = update_open_dropdown(This);

    if(show_dropdown)
    {
        int dropdown_width, dropdown_height;
        hwnd = GetDlgItem(This->dlg_hwnd, psh1);

        GetWindowRect(hwnd, &dropdown_rc);
        dropdown_width = dropdown_rc.right - dropdown_rc.left;
        dropdown_height = dropdown_rc.bottom - dropdown_rc.top;

        dropdown_rc.left = cancel_rc.left - dropdown_width - hspacing;
        dropdown_rc.top = cancel_rc.top;
        dropdown_rc.right = dropdown_rc.left + dropdown_width;
        dropdown_rc.bottom = dropdown_rc.top + dropdown_height;
    }
    else
    {
        dropdown_rc.left = dropdown_rc.right = cancel_rc.left - hspacing;
        dropdown_rc.top = cancel_rc.top;
        dropdown_rc.bottom = cancel_rc.bottom;
    }

    /* Open/Save button */
    hwnd = GetDlgItem(This->dlg_hwnd, IDOK);
    if(hwnd)
    {
        int open_width, open_height;
        GetWindowRect(hwnd, &open_rc);
        open_width = open_rc.right - open_rc.left;
        open_height = open_rc.bottom - open_rc.top;

        open_rc.left = dropdown_rc.left - open_width;
        open_rc.top = dropdown_rc.top;
        open_rc.right = open_rc.left + open_width;
        open_rc.bottom = open_rc.top + open_height;
    }

    /* The filetype combobox. */
    hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILETYPE);
    if(hwnd)
    {
        int filetype_width, filetype_height;
        GetWindowRect(hwnd, &filetype_rc);

        filetype_width = filetype_rc.right - filetype_rc.left;
        filetype_height = filetype_rc.bottom - filetype_rc.top;

        filetype_rc.right = cancel_rc.right;

        filetype_rc.left = filetype_rc.right - filetype_width;
        filetype_rc.top = cancel_rc.top - filetype_height - vspacing;
        filetype_rc.bottom = filetype_rc.top + filetype_height;

        if(!This->filterspec_count)
            filetype_rc.left = filetype_rc.right;
    }

    /* Filename label. */
    hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAMESTATIC);
    if(hwnd)
    {
        int filetypelabel_width, filetypelabel_height;
        GetWindowRect(hwnd, &filenamelabel_rc);

        filetypelabel_width = filenamelabel_rc.right - filenamelabel_rc.left;
        filetypelabel_height = filenamelabel_rc.bottom - filenamelabel_rc.top;

        filenamelabel_rc.left = 160; /* FIXME */
        filenamelabel_rc.top = filetype_rc.top;
        filenamelabel_rc.right = filenamelabel_rc.left + filetypelabel_width;
        filenamelabel_rc.bottom = filenamelabel_rc.top + filetypelabel_height;
    }

    /* Filename edit box. */
    hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAME);
    if(hwnd)
    {
        int filename_width, filename_height;
        GetWindowRect(hwnd, &filename_rc);

        filename_width = filetype_rc.left - filenamelabel_rc.right - hspacing*2;
        filename_height = filename_rc.bottom - filename_rc.top;

        filename_rc.left = filenamelabel_rc.right + hspacing;
        filename_rc.top = filetype_rc.top;
        filename_rc.right = filename_rc.left + filename_width;
        filename_rc.bottom = filename_rc.top + filename_height;
    }

    hwnd = GetDlgItem(This->dlg_hwnd, IDC_NAV_TOOLBAR);
    if(hwnd)
    {
        GetWindowRect(hwnd, &toolbar_rc);
        MapWindowPoints(NULL, This->dlg_hwnd, (POINT*)&toolbar_rc, 2);
    }

    /* The custom controls */
    customctrls_rc.left = dialog_rc.left + hspacing;
    customctrls_rc.right = dialog_rc.right - hspacing;
    customctrls_rc.bottom = filename_rc.top - vspacing;
    customctrls_rc.top = customctrls_rc.bottom -
        ctrl_container_resize(This, customctrls_rc.right - customctrls_rc.left);

    /* The ExplorerBrowser control. */
    ebrowser_rc.left = dialog_rc.left + hspacing;
    ebrowser_rc.top = toolbar_rc.bottom + vspacing;
    ebrowser_rc.right = dialog_rc.right - hspacing;
    ebrowser_rc.bottom = customctrls_rc.top - vspacing;

    /****
     * Move everything to the right place.
     */

    /* FIXME: The Save Dialog uses a slightly different layout. */
    hdwp = BeginDeferWindowPos(7);

    if(hdwp && This->peb)
        IExplorerBrowser_SetRect(This->peb, &hdwp, ebrowser_rc);

    if(hdwp && This->cctrls_hwnd)
        DeferWindowPos(hdwp, This->cctrls_hwnd, NULL,
                       customctrls_rc.left, customctrls_rc.top,
                       customctrls_rc.right - customctrls_rc.left, customctrls_rc.bottom - customctrls_rc.top,
                       SWP_NOZORDER | SWP_NOACTIVATE);

    /* The default controls */
    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILETYPE)) )
        DeferWindowPos(hdwp, hwnd, NULL, filetype_rc.left, filetype_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAME)) )
        DeferWindowPos(hdwp, hwnd, NULL, filename_rc.left, filename_rc.top,
                       filename_rc.right - filename_rc.left, filename_rc.bottom - filename_rc.top,
                       SWP_NOZORDER | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDC_FILENAMESTATIC)) )
        DeferWindowPos(hdwp, hwnd, NULL, filenamelabel_rc.left, filenamelabel_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDOK)) )
        DeferWindowPos(hdwp, hwnd, NULL, open_rc.left, open_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && This->hmenu_opendropdown && (hwnd = GetDlgItem(This->dlg_hwnd, psh1)))
        DeferWindowPos(hdwp, hwnd, NULL, dropdown_rc.left, dropdown_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp && (hwnd = GetDlgItem(This->dlg_hwnd, IDCANCEL)) )
        DeferWindowPos(hdwp, hwnd, NULL, cancel_rc.left, cancel_rc.top, 0, 0,
                       SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

    if(hdwp)
        EndDeferWindowPos(hdwp);
    else
        ERR("Failed to position dialog controls.\n");

    return;
}

static HRESULT init_explorerbrowser(FileDialogImpl *This)
{
    IShellItem *psi_folder;
    IObjectWithSite *client;
    FOLDERSETTINGS fos;
    RECT rc = {0};
    HRESULT hr;

    /* Create ExplorerBrowser instance */
    OleInitialize(NULL);

    hr = CoCreateInstance(&CLSID_ExplorerBrowser, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IExplorerBrowser, (void**)&This->peb);
    if(FAILED(hr))
    {
        ERR("Failed to instantiate ExplorerBrowser control.\n");
        return hr;
    }

    IExplorerBrowser_SetOptions(This->peb, EBO_SHOWFRAMES | EBO_NOBORDER);

    hr = IExplorerBrowser_Initialize(This->peb, This->dlg_hwnd, &rc, NULL);
    if(FAILED(hr))
    {
        ERR("Failed to initialize the ExplorerBrowser control.\n");
        IExplorerBrowser_Release(This->peb);
        This->peb = NULL;
        return hr;
    }
    hr = IExplorerBrowser_Advise(This->peb, &This->IExplorerBrowserEvents_iface, &This->ebevents_cookie);
    if(FAILED(hr))
        ERR("Advise (ExplorerBrowser) failed.\n");

    /* Get previous options? */
    fos.ViewMode = fos.fFlags = 0;
    if(!(This->options & FOS_ALLOWMULTISELECT))
        fos.fFlags |= FWF_SINGLESEL;

    IExplorerBrowser_SetFolderSettings(This->peb, &fos);

    hr = IExplorerBrowser_QueryInterface(This->peb, &IID_IObjectWithSite, (void**)&client);
    if (hr == S_OK)
    {
        hr = IObjectWithSite_SetSite(client, (IUnknown*)&This->IFileDialog2_iface);
        IObjectWithSite_Release(client);
        if(FAILED(hr))
            ERR("SetSite failed, 0x%08x\n", hr);
    }

    /* Browse somewhere */
    psi_folder = This->psi_setfolder ? This->psi_setfolder : This->psi_defaultfolder;
    IExplorerBrowser_BrowseToObject(This->peb, (IUnknown*)psi_folder, SBSP_DEFBROWSER);

    return S_OK;
}

static void init_toolbar(FileDialogImpl *This, HWND hwnd)
{
    HWND htoolbar;
    TBADDBITMAP tbab;
    TBBUTTON button[2];

    htoolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL, TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE,
                               0, 0, 0, 0,
                               hwnd, (HMENU)IDC_NAV_TOOLBAR, NULL, NULL);

    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = IDB_HIST_LARGE_COLOR;
    SendMessageW(htoolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);

    button[0].iBitmap = HIST_BACK;
    button[0].idCommand = IDC_NAVBACK;
    button[0].fsState = TBSTATE_ENABLED;
    button[0].fsStyle = BTNS_BUTTON;
    button[0].dwData = 0;
    button[0].iString = 0;

    button[1].iBitmap = HIST_FORWARD;
    button[1].idCommand = IDC_NAVFORWARD;
    button[1].fsState = TBSTATE_ENABLED;
    button[1].fsStyle = BTNS_BUTTON;
    button[1].dwData = 0;
    button[1].iString = 0;

    SendMessageW(htoolbar, TB_ADDBUTTONSW, 2, (LPARAM)button);
    SendMessageW(htoolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(24,24));
    SendMessageW(htoolbar, TB_AUTOSIZE, 0, 0);
}

static void update_control_text(FileDialogImpl *This)
{
    HWND hitem;
    LPCWSTR custom_okbutton;
    cctrl_item* item;

    if(This->custom_title)
        SetWindowTextW(This->dlg_hwnd, This->custom_title);

    if(This->hmenu_opendropdown && (item = get_first_item(&This->cctrl_opendropdown)))
        custom_okbutton = item->label;
    else
        custom_okbutton = This->custom_okbutton;

    if(custom_okbutton &&
       (hitem = GetDlgItem(This->dlg_hwnd, IDOK)))
    {
        SetWindowTextW(hitem, custom_okbutton);
        ctrl_resize(hitem, 50, 250, FALSE);
    }

    if(This->custom_cancelbutton &&
       (hitem = GetDlgItem(This->dlg_hwnd, IDCANCEL)))
    {
        SetWindowTextW(hitem, This->custom_cancelbutton);
        ctrl_resize(hitem, 50, 250, FALSE);
    }

    if(This->custom_filenamelabel &&
       (hitem = GetDlgItem(This->dlg_hwnd, IDC_FILENAMESTATIC)))
    {
        SetWindowTextW(hitem, This->custom_filenamelabel);
        ctrl_resize(hitem, 50, 250, FALSE);
    }
}

static LRESULT CALLBACK dropdown_subclass_proc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    static const WCHAR prop_this[] = {'i','t','e','m','d','l','g','_','T','h','i','s',0};
    static const WCHAR prop_oldwndproc[] = {'i','t','e','m','d','l','g','_','o','l','d','w','n','d','p','r','o','c',0};

    if (umessage == WM_LBUTTONDOWN)
    {
        FileDialogImpl *This = (FileDialogImpl*)GetPropW(hwnd, prop_this);

        SendMessageW(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        show_opendropdown(This);
        SendMessageW(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);

        return 0;
    }

    return CallWindowProcW((WNDPROC)GetPropW(hwnd, prop_oldwndproc), hwnd, umessage, wparam, lparam);
}

static LRESULT on_wm_initdialog(HWND hwnd, LPARAM lParam)
{
    FileDialogImpl *This = (FileDialogImpl*)lParam;
    HWND hitem;

    TRACE("(%p, %p)\n", This, hwnd);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)This);
    This->dlg_hwnd = hwnd;

    hitem = GetDlgItem(This->dlg_hwnd, pshHelp);
    if(hitem) ShowWindow(hitem, SW_HIDE);

    hitem = GetDlgItem(This->dlg_hwnd, IDC_FILETYPESTATIC);
    if(hitem) ShowWindow(hitem, SW_HIDE);

    /* Fill filetypes combobox, or hide it. */
    hitem = GetDlgItem(This->dlg_hwnd, IDC_FILETYPE);
    if(This->filterspec_count)
    {
        HDC hdc;
        HFONT font;
        SIZE size;
        UINT i, maxwidth = 0;

        hdc = GetDC(hitem);
        font = (HFONT)SendMessageW(hitem, WM_GETFONT, 0, 0);
        SelectObject(hdc, font);

        for(i = 0; i < This->filterspec_count; i++)
        {
            SendMessageW(hitem, CB_ADDSTRING, 0, (LPARAM)This->filterspecs[i].pszName);

            if(GetTextExtentPoint32W(hdc, This->filterspecs[i].pszName, lstrlenW(This->filterspecs[i].pszName), &size))
                maxwidth = max(maxwidth, size.cx);
        }
        ReleaseDC(hitem, hdc);

        if(maxwidth > 0)
        {
            maxwidth += GetSystemMetrics(SM_CXVSCROLL) + 4;
            SendMessageW(hitem, CB_SETDROPPEDWIDTH, (WPARAM)maxwidth, 0);
        }
        else
            ERR("Failed to calculate width of filetype dropdown\n");

        SendMessageW(hitem, CB_SETCURSEL, This->filetypeindex, 0);
    }
    else
        ShowWindow(hitem, SW_HIDE);

    if(This->set_filename &&
       (hitem = GetDlgItem(This->dlg_hwnd, IDC_FILENAME)) )
        SendMessageW(hitem, WM_SETTEXT, 0, (LPARAM)This->set_filename);

    if(This->hmenu_opendropdown)
    {
        HWND dropdown_hwnd;
        LOGFONTW lfw, lfw_marlett;
        HFONT dialog_font;
        static const WCHAR marlett[] = {'M','a','r','l','e','t','t',0};
        static const WCHAR prop_this[] = {'i','t','e','m','d','l','g','_','T','h','i','s',0};
        static const WCHAR prop_oldwndproc[] = {'i','t','e','m','d','l','g','_','o','l','d','w','n','d','p','r','o','c',0};

        dropdown_hwnd = GetDlgItem(This->dlg_hwnd, psh1);

        /* Change dropdown button font to Marlett */
        dialog_font = (HFONT)SendMessageW(dropdown_hwnd, WM_GETFONT, 0, 0);

        GetObjectW(dialog_font, sizeof(lfw), &lfw);

        memset(&lfw_marlett, 0, sizeof(lfw_marlett));
        lstrcpyW(lfw_marlett.lfFaceName, marlett);
        lfw_marlett.lfHeight = lfw.lfHeight;
        lfw_marlett.lfCharSet = SYMBOL_CHARSET;

        This->hfont_opendropdown = CreateFontIndirectW(&lfw_marlett);

        SendMessageW(dropdown_hwnd, WM_SETFONT, (LPARAM)This->hfont_opendropdown, 0);

        /* Subclass button so we can handle LBUTTONDOWN */
        SetPropW(dropdown_hwnd, prop_this, (HANDLE)This);
        SetPropW(dropdown_hwnd, prop_oldwndproc,
            (HANDLE)SetWindowLongPtrW(dropdown_hwnd, GWLP_WNDPROC, (LONG_PTR)dropdown_subclass_proc));
    }

    ctrl_container_reparent(This, This->dlg_hwnd);
    init_explorerbrowser(This);
    init_toolbar(This, hwnd);
    update_control_text(This);
    update_layout(This);

    if(This->filterspec_count)
        events_OnTypeChange(This);

    if ((hitem = GetDlgItem(This->dlg_hwnd, IDC_FILENAME)))
        SetFocus(hitem);

    return FALSE;
}

static LRESULT on_wm_size(FileDialogImpl *This)
{
    update_layout(This);
    return FALSE;
}

static LRESULT on_wm_getminmaxinfo(FileDialogImpl *This, LPARAM lparam)
{
    MINMAXINFO *mmi = (MINMAXINFO*)lparam;
    TRACE("%p (%p)\n", This, mmi);

    /* FIXME */
    mmi->ptMinTrackSize.x = 640;
    mmi->ptMinTrackSize.y = 480;

    return FALSE;
}

static LRESULT on_wm_destroy(FileDialogImpl *This)
{
    TRACE("%p\n", This);

    if(This->peb)
    {
        IExplorerBrowser_Destroy(This->peb);
        IExplorerBrowser_Release(This->peb);
        This->peb = NULL;
    }

    ctrl_container_reparent(This, NULL);
    This->dlg_hwnd = NULL;

    DeleteObject(This->hfont_opendropdown);
    This->hfont_opendropdown = NULL;

    return TRUE;
}

static LRESULT on_idok(FileDialogImpl *This)
{
    TRACE("%p\n", This);

    if(SUCCEEDED(on_default_action(This)))
        EndDialog(This->dlg_hwnd, S_OK);

    return FALSE;
}

static LRESULT on_idcancel(FileDialogImpl *This)
{
    TRACE("%p\n", This);

    EndDialog(This->dlg_hwnd, HRESULT_FROM_WIN32(ERROR_CANCELLED));

    return FALSE;
}

static LRESULT on_command_opendropdown(FileDialogImpl *This, WPARAM wparam, LPARAM lparam)
{
    if(HIWORD(wparam) == BN_CLICKED)
    {
        HWND hwnd = (HWND)lparam;
        SendMessageW(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        show_opendropdown(This);
        SendMessageW(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
    }

    return FALSE;
}

static LRESULT on_browse_back(FileDialogImpl *This)
{
    TRACE("%p\n", This);
    IExplorerBrowser_BrowseToIDList(This->peb, NULL, SBSP_NAVIGATEBACK);
    return FALSE;
}

static LRESULT on_browse_forward(FileDialogImpl *This)
{
    TRACE("%p\n", This);
    IExplorerBrowser_BrowseToIDList(This->peb, NULL, SBSP_NAVIGATEFORWARD);
    return FALSE;
}

static LRESULT on_command_filetype(FileDialogImpl *This, WPARAM wparam, LPARAM lparam)
{
    if(HIWORD(wparam) == CBN_SELCHANGE)
    {
        IShellView *psv;
        HRESULT hr;
        LPWSTR filename;
        UINT prev_index = This->filetypeindex;

        This->filetypeindex = SendMessageW((HWND)lparam, CB_GETCURSEL, 0, 0);
        TRACE("File type selection changed to %d.\n", This->filetypeindex);

        if(prev_index == This->filetypeindex)
            return FALSE;

        hr = IExplorerBrowser_GetCurrentView(This->peb, &IID_IShellView, (void**)&psv);
        if(SUCCEEDED(hr))
        {
            IShellView_Refresh(psv);
            IShellView_Release(psv);
        }

        if(This->dlg_type == ITEMDLG_TYPE_SAVE && get_file_name(This, &filename))
        {
            WCHAR buf[MAX_PATH], extbuf[MAX_PATH], *ext;

            ext = get_first_ext_from_spec(extbuf, This->filterspecs[This->filetypeindex].pszSpec);
            if(ext)
            {
                lstrcpyW(buf, filename);

                if(PathMatchSpecW(buf, This->filterspecs[prev_index].pszSpec))
                    PathRemoveExtensionW(buf);

                lstrcatW(buf, ext);
                set_file_name(This, buf);
            }
            CoTaskMemFree(filename);
        }

        /* The documentation claims that OnTypeChange is called only
         * when the dialog is opened, but this is obviously not the
         * case. */
        events_OnTypeChange(This);
    }

    return FALSE;
}

static LRESULT on_wm_command(FileDialogImpl *This, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
    {
    case IDOK:                return on_idok(This);
    case IDCANCEL:            return on_idcancel(This);
    case psh1:                return on_command_opendropdown(This, wparam, lparam);
    case IDC_NAVBACK:         return on_browse_back(This);
    case IDC_NAVFORWARD:      return on_browse_forward(This);
    case IDC_FILETYPE:        return on_command_filetype(This, wparam, lparam);
    default:                  TRACE("Unknown command.\n");
    }
    return FALSE;
}

static LRESULT CALLBACK itemdlg_dlgproc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    FileDialogImpl *This = (FileDialogImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch(umessage)
    {
    case WM_INITDIALOG:       return on_wm_initdialog(hwnd, lparam);
    case WM_COMMAND:          return on_wm_command(This, wparam, lparam);
    case WM_SIZE:             return on_wm_size(This);
    case WM_GETMINMAXINFO:    return on_wm_getminmaxinfo(This, lparam);
    case WM_DESTROY:          return on_wm_destroy(This);
    }

    return FALSE;
}

static HRESULT create_dialog(FileDialogImpl *This, HWND parent)
{
    INT_PTR res;

    SetLastError(0);
    res = DialogBoxParamW(COMDLG32_hInstance,
                          MAKEINTRESOURCEW(NEWFILEOPENV3ORD),
                          parent, itemdlg_dlgproc, (LPARAM)This);
    This->dlg_hwnd = NULL;
    if(res == -1)
    {
        ERR("Failed to show dialog (LastError: %d)\n", GetLastError());
        return E_FAIL;
    }

    TRACE("Returning 0x%08x\n", (HRESULT)res);
    return (HRESULT)res;
}

/**************************************************************************
 * IFileDialog implementation
 */
static inline FileDialogImpl *impl_from_IFileDialog2(IFileDialog2 *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, IFileDialog2_iface);
}

static HRESULT WINAPI IFileDialog2_fnQueryInterface(IFileDialog2 *iface,
                                                    REFIID riid,
                                                    void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IFileDialog) ||
       IsEqualGUID(riid, &IID_IFileDialog2))
    {
        *ppvObject = iface;
    }
    else if(IsEqualGUID(riid, &IID_IFileOpenDialog) && This->dlg_type == ITEMDLG_TYPE_OPEN)
    {
        *ppvObject = &This->u.IFileOpenDialog_iface;
    }
    else if(IsEqualGUID(riid, &IID_IFileSaveDialog) && This->dlg_type == ITEMDLG_TYPE_SAVE)
    {
        *ppvObject = &This->u.IFileSaveDialog_iface;
    }
    else if(IsEqualGUID(riid, &IID_IExplorerBrowserEvents))
    {
        *ppvObject = &This->IExplorerBrowserEvents_iface;
    }
    else if(IsEqualGUID(riid, &IID_IServiceProvider))
    {
        *ppvObject = &This->IServiceProvider_iface;
    }
    else if(IsEqualGUID(&IID_ICommDlgBrowser3, riid) ||
            IsEqualGUID(&IID_ICommDlgBrowser2, riid) ||
            IsEqualGUID(&IID_ICommDlgBrowser, riid))
    {
        *ppvObject = &This->ICommDlgBrowser3_iface;
    }
    else if(IsEqualGUID(&IID_IOleWindow, riid))
    {
        *ppvObject = &This->IOleWindow_iface;
    }
    else if(IsEqualGUID(riid, &IID_IFileDialogCustomize) ||
            IsEqualGUID(riid, &IID_IFileDialogCustomizeAlt))
    {
        *ppvObject = &This->IFileDialogCustomize_iface;
    }
    else
        FIXME("Unknown interface requested: %s.\n", debugstr_guid(riid));

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IFileDialog2_fnAddRef(IFileDialog2 *iface)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    return ref;
}

static ULONG WINAPI IFileDialog2_fnRelease(IFileDialog2 *iface)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    if(!ref)
    {
        UINT i;
        for(i = 0; i < This->filterspec_count; i++)
        {
            LocalFree((void*)This->filterspecs[i].pszName);
            LocalFree((void*)This->filterspecs[i].pszSpec);
        }
        HeapFree(GetProcessHeap(), 0, This->filterspecs);

        DestroyWindow(This->cctrls_hwnd);

        if(This->psi_defaultfolder) IShellItem_Release(This->psi_defaultfolder);
        if(This->psi_setfolder)     IShellItem_Release(This->psi_setfolder);
        if(This->psi_folder)        IShellItem_Release(This->psi_folder);
        if(This->psia_selection)    IShellItemArray_Release(This->psia_selection);
        if(This->psia_results)      IShellItemArray_Release(This->psia_results);

        LocalFree(This->set_filename);
        LocalFree(This->default_ext);
        LocalFree(This->custom_title);
        LocalFree(This->custom_okbutton);
        LocalFree(This->custom_cancelbutton);
        LocalFree(This->custom_filenamelabel);

        DestroyMenu(This->hmenu_opendropdown);
        DeleteObject(This->hfont_opendropdown);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IFileDialog2_fnShow(IFileDialog2 *iface, HWND hwndOwner)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", iface, hwndOwner);

    This->opendropdown_has_selection = FALSE;

    return create_dialog(This, hwndOwner);
}

static HRESULT WINAPI IFileDialog2_fnSetFileTypes(IFileDialog2 *iface, UINT cFileTypes,
                                                  const COMDLG_FILTERSPEC *rgFilterSpec)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    UINT i;
    TRACE("%p (%d, %p)\n", This, cFileTypes, rgFilterSpec);

    if(This->filterspecs)
        return E_UNEXPECTED;

    if(!rgFilterSpec)
        return E_INVALIDARG;

    if(!cFileTypes)
        return S_OK;

    This->filterspecs = HeapAlloc(GetProcessHeap(), 0, sizeof(COMDLG_FILTERSPEC)*cFileTypes);
    for(i = 0; i < cFileTypes; i++)
    {
        This->filterspecs[i].pszName = StrDupW(rgFilterSpec[i].pszName);
        This->filterspecs[i].pszSpec = StrDupW(rgFilterSpec[i].pszSpec);
    }
    This->filterspec_count = cFileTypes;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetFileTypeIndex(IFileDialog2 *iface, UINT iFileType)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%d)\n", This, iFileType);

    if(!This->filterspecs)
        return E_FAIL;

    iFileType = max(iFileType, 1);
    iFileType = min(iFileType, This->filterspec_count);
    This->filetypeindex = iFileType-1;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetFileTypeIndex(IFileDialog2 *iface, UINT *piFileType)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, piFileType);

    if(!piFileType)
        return E_INVALIDARG;

    if(This->filterspec_count == 0)
        *piFileType = 0;
    else
        *piFileType = This->filetypeindex + 1;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnAdvise(IFileDialog2 *iface, IFileDialogEvents *pfde, DWORD *pdwCookie)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    events_client *client;
    TRACE("%p (%p, %p)\n", This, pfde, pdwCookie);

    if(!pfde || !pdwCookie)
        return E_INVALIDARG;

    client = HeapAlloc(GetProcessHeap(), 0, sizeof(events_client));
    client->pfde = pfde;
    client->cookie = ++This->events_next_cookie;

    IFileDialogEvents_AddRef(pfde);
    *pdwCookie = client->cookie;

    list_add_tail(&This->events_clients, &client->entry);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnUnadvise(IFileDialog2 *iface, DWORD dwCookie)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    events_client *client, *found = NULL;
    TRACE("%p (%d)\n", This, dwCookie);

    LIST_FOR_EACH_ENTRY(client, &This->events_clients, events_client, entry)
    {
        if(client->cookie == dwCookie)
        {
            found = client;
            break;
        }
    }

    if(found)
    {
        list_remove(&found->entry);
        IFileDialogEvents_Release(found->pfde);
        HeapFree(GetProcessHeap(), 0, found);
        return S_OK;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI IFileDialog2_fnSetOptions(IFileDialog2 *iface, FILEOPENDIALOGOPTIONS fos)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (0x%x)\n", This, fos);

    if( !(This->options & FOS_PICKFOLDERS) && (fos & FOS_PICKFOLDERS) )
    {
        WCHAR buf[30];
        LoadStringW(COMDLG32_hInstance, IDS_SELECT_FOLDER, buf, sizeof(buf)/sizeof(WCHAR));
        IFileDialog2_SetTitle(iface, buf);
    }

    This->options = fos;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetOptions(IFileDialog2 *iface, FILEOPENDIALOGOPTIONS *pfos)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, pfos);

    if(!pfos)
        return E_INVALIDARG;

    *pfos = This->options;

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetDefaultFolder(IFileDialog2 *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, psi);
    if(This->psi_defaultfolder)
        IShellItem_Release(This->psi_defaultfolder);

    This->psi_defaultfolder = psi;

    if(This->psi_defaultfolder)
        IShellItem_AddRef(This->psi_defaultfolder);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetFolder(IFileDialog2 *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, psi);
    if(This->psi_setfolder)
        IShellItem_Release(This->psi_setfolder);

    This->psi_setfolder = psi;

    if(This->psi_setfolder)
        IShellItem_AddRef(This->psi_setfolder);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetFolder(IFileDialog2 *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", This, ppsi);
    if(!ppsi)
        return E_INVALIDARG;

    /* FIXME:
       If the dialog is shown, return the current(ly selected) folder. */

    *ppsi = NULL;
    if(This->psi_folder)
        *ppsi = This->psi_folder;
    else if(This->psi_setfolder)
        *ppsi = This->psi_setfolder;
    else if(This->psi_defaultfolder)
        *ppsi = This->psi_defaultfolder;

    if(*ppsi)
    {
        IShellItem_AddRef(*ppsi);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialog2_fnGetCurrentSelection(IFileDialog2 *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    HRESULT hr;
    TRACE("%p (%p)\n", This, ppsi);

    if(!ppsi)
        return E_INVALIDARG;

    if(This->psia_selection)
    {
        /* FIXME: Check filename edit box */
        hr = IShellItemArray_GetItemAt(This->psia_selection, 0, ppsi);
        return hr;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialog2_fnSetFileName(IFileDialog2 *iface, LPCWSTR pszName)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", iface, debugstr_w(pszName));

    set_file_name(This, pszName);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetFileName(IFileDialog2 *iface, LPWSTR *pszName)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%p)\n", iface, pszName);

    if(!pszName)
        return E_INVALIDARG;

    *pszName = NULL;
    if(get_file_name(This, pszName))
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI IFileDialog2_fnSetTitle(IFileDialog2 *iface, LPCWSTR pszTitle)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", This, debugstr_w(pszTitle));

    LocalFree(This->custom_title);
    This->custom_title = StrDupW(pszTitle);
    update_control_text(This);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetOkButtonLabel(IFileDialog2 *iface, LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", This, debugstr_w(pszText));

    LocalFree(This->custom_okbutton);
    This->custom_okbutton = StrDupW(pszText);
    update_control_text(This);
    update_layout(This);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetFileNameLabel(IFileDialog2 *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", This, debugstr_w(pszLabel));

    LocalFree(This->custom_filenamelabel);
    This->custom_filenamelabel = StrDupW(pszLabel);
    update_control_text(This);
    update_layout(This);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnGetResult(IFileDialog2 *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    HRESULT hr;
    TRACE("%p (%p)\n", This, ppsi);

    if(!ppsi)
        return E_INVALIDARG;

    if(This->psia_results)
    {
        UINT item_count;
        hr = IShellItemArray_GetCount(This->psia_results, &item_count);
        if(SUCCEEDED(hr))
        {
            if(item_count != 1)
                return E_FAIL;

            /* Adds a reference. */
            hr = IShellItemArray_GetItemAt(This->psia_results, 0, ppsi);
        }

        return hr;
    }

    return E_UNEXPECTED;
}

static HRESULT WINAPI IFileDialog2_fnAddPlace(IFileDialog2 *iface, IShellItem *psi, FDAP fdap)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p, %d)\n", This, psi, fdap);
    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetDefaultExtension(IFileDialog2 *iface, LPCWSTR pszDefaultExtension)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", This, debugstr_w(pszDefaultExtension));

    LocalFree(This->default_ext);
    This->default_ext = StrDupW(pszDefaultExtension);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnClose(IFileDialog2 *iface, HRESULT hr)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (0x%08x)\n", This, hr);

    if(This->dlg_hwnd)
        EndDialog(This->dlg_hwnd, hr);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetClientGuid(IFileDialog2 *iface, REFGUID guid)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", This, debugstr_guid(guid));
    This->client_guid = *guid;
    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnClearClientData(IFileDialog2 *iface)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetFilter(IFileDialog2 *iface, IShellItemFilter *pFilter)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, pFilter);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialog2_fnSetCancelButtonLabel(IFileDialog2 *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    TRACE("%p (%s)\n", This, debugstr_w(pszLabel));

    LocalFree(This->custom_cancelbutton);
    This->custom_cancelbutton = StrDupW(pszLabel);
    update_control_text(This);
    update_layout(This);

    return S_OK;
}

static HRESULT WINAPI IFileDialog2_fnSetNavigationRoot(IFileDialog2 *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileDialog2(iface);
    FIXME("stub - %p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static const IFileDialog2Vtbl vt_IFileDialog2 = {
    IFileDialog2_fnQueryInterface,
    IFileDialog2_fnAddRef,
    IFileDialog2_fnRelease,
    IFileDialog2_fnShow,
    IFileDialog2_fnSetFileTypes,
    IFileDialog2_fnSetFileTypeIndex,
    IFileDialog2_fnGetFileTypeIndex,
    IFileDialog2_fnAdvise,
    IFileDialog2_fnUnadvise,
    IFileDialog2_fnSetOptions,
    IFileDialog2_fnGetOptions,
    IFileDialog2_fnSetDefaultFolder,
    IFileDialog2_fnSetFolder,
    IFileDialog2_fnGetFolder,
    IFileDialog2_fnGetCurrentSelection,
    IFileDialog2_fnSetFileName,
    IFileDialog2_fnGetFileName,
    IFileDialog2_fnSetTitle,
    IFileDialog2_fnSetOkButtonLabel,
    IFileDialog2_fnSetFileNameLabel,
    IFileDialog2_fnGetResult,
    IFileDialog2_fnAddPlace,
    IFileDialog2_fnSetDefaultExtension,
    IFileDialog2_fnClose,
    IFileDialog2_fnSetClientGuid,
    IFileDialog2_fnClearClientData,
    IFileDialog2_fnSetFilter,
    IFileDialog2_fnSetCancelButtonLabel,
    IFileDialog2_fnSetNavigationRoot
};

/**************************************************************************
 * IFileOpenDialog
 */
static inline FileDialogImpl *impl_from_IFileOpenDialog(IFileOpenDialog *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, u.IFileOpenDialog_iface);
}

static HRESULT WINAPI IFileOpenDialog_fnQueryInterface(IFileOpenDialog *iface,
                                                       REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IFileOpenDialog_fnAddRef(IFileOpenDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IFileOpenDialog_fnRelease(IFileOpenDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileOpenDialog_fnShow(IFileOpenDialog *iface, HWND hwndOwner)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Show(&This->IFileDialog2_iface, hwndOwner);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileTypes(IFileOpenDialog *iface, UINT cFileTypes,
                                                     const COMDLG_FILTERSPEC *rgFilterSpec)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileTypes(&This->IFileDialog2_iface, cFileTypes, rgFilterSpec);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileTypeIndex(IFileOpenDialog *iface, UINT iFileType)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileTypeIndex(&This->IFileDialog2_iface, iFileType);
}

static HRESULT WINAPI IFileOpenDialog_fnGetFileTypeIndex(IFileOpenDialog *iface, UINT *piFileType)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetFileTypeIndex(&This->IFileDialog2_iface, piFileType);
}

static HRESULT WINAPI IFileOpenDialog_fnAdvise(IFileOpenDialog *iface, IFileDialogEvents *pfde,
                                               DWORD *pdwCookie)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Advise(&This->IFileDialog2_iface, pfde, pdwCookie);
}

static HRESULT WINAPI IFileOpenDialog_fnUnadvise(IFileOpenDialog *iface, DWORD dwCookie)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Unadvise(&This->IFileDialog2_iface, dwCookie);
}

static HRESULT WINAPI IFileOpenDialog_fnSetOptions(IFileOpenDialog *iface, FILEOPENDIALOGOPTIONS fos)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetOptions(&This->IFileDialog2_iface, fos);
}

static HRESULT WINAPI IFileOpenDialog_fnGetOptions(IFileOpenDialog *iface, FILEOPENDIALOGOPTIONS *pfos)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetOptions(&This->IFileDialog2_iface, pfos);
}

static HRESULT WINAPI IFileOpenDialog_fnSetDefaultFolder(IFileOpenDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetDefaultFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFolder(IFileOpenDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileOpenDialog_fnGetFolder(IFileOpenDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetFolder(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileOpenDialog_fnGetCurrentSelection(IFileOpenDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetCurrentSelection(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileName(IFileOpenDialog *iface, LPCWSTR pszName)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileOpenDialog_fnGetFileName(IFileOpenDialog *iface, LPWSTR *pszName)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileOpenDialog_fnSetTitle(IFileOpenDialog *iface, LPCWSTR pszTitle)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetTitle(&This->IFileDialog2_iface, pszTitle);
}

static HRESULT WINAPI IFileOpenDialog_fnSetOkButtonLabel(IFileOpenDialog *iface, LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetOkButtonLabel(&This->IFileDialog2_iface, pszText);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFileNameLabel(IFileOpenDialog *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFileNameLabel(&This->IFileDialog2_iface, pszLabel);
}

static HRESULT WINAPI IFileOpenDialog_fnGetResult(IFileOpenDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_GetResult(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileOpenDialog_fnAddPlace(IFileOpenDialog *iface, IShellItem *psi, FDAP fdap)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_AddPlace(&This->IFileDialog2_iface, psi, fdap);
}

static HRESULT WINAPI IFileOpenDialog_fnSetDefaultExtension(IFileOpenDialog *iface,
                                                            LPCWSTR pszDefaultExtension)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetDefaultExtension(&This->IFileDialog2_iface, pszDefaultExtension);
}

static HRESULT WINAPI IFileOpenDialog_fnClose(IFileOpenDialog *iface, HRESULT hr)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_Close(&This->IFileDialog2_iface, hr);
}

static HRESULT WINAPI IFileOpenDialog_fnSetClientGuid(IFileOpenDialog *iface, REFGUID guid)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetClientGuid(&This->IFileDialog2_iface, guid);
}

static HRESULT WINAPI IFileOpenDialog_fnClearClientData(IFileOpenDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_ClearClientData(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileOpenDialog_fnSetFilter(IFileOpenDialog *iface, IShellItemFilter *pFilter)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    return IFileDialog2_SetFilter(&This->IFileDialog2_iface, pFilter);
}

static HRESULT WINAPI IFileOpenDialog_fnGetResults(IFileOpenDialog *iface, IShellItemArray **ppenum)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    TRACE("%p (%p)\n", This, ppenum);

    *ppenum = This->psia_results;

    if(*ppenum)
    {
        IShellItemArray_AddRef(*ppenum);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileOpenDialog_fnGetSelectedItems(IFileOpenDialog *iface, IShellItemArray **ppsai)
{
    FileDialogImpl *This = impl_from_IFileOpenDialog(iface);
    TRACE("%p (%p)\n", This, ppsai);

    if(This->psia_selection)
    {
        *ppsai = This->psia_selection;
        IShellItemArray_AddRef(*ppsai);
        return S_OK;
    }

    return E_FAIL;
}

static const IFileOpenDialogVtbl vt_IFileOpenDialog = {
    IFileOpenDialog_fnQueryInterface,
    IFileOpenDialog_fnAddRef,
    IFileOpenDialog_fnRelease,
    IFileOpenDialog_fnShow,
    IFileOpenDialog_fnSetFileTypes,
    IFileOpenDialog_fnSetFileTypeIndex,
    IFileOpenDialog_fnGetFileTypeIndex,
    IFileOpenDialog_fnAdvise,
    IFileOpenDialog_fnUnadvise,
    IFileOpenDialog_fnSetOptions,
    IFileOpenDialog_fnGetOptions,
    IFileOpenDialog_fnSetDefaultFolder,
    IFileOpenDialog_fnSetFolder,
    IFileOpenDialog_fnGetFolder,
    IFileOpenDialog_fnGetCurrentSelection,
    IFileOpenDialog_fnSetFileName,
    IFileOpenDialog_fnGetFileName,
    IFileOpenDialog_fnSetTitle,
    IFileOpenDialog_fnSetOkButtonLabel,
    IFileOpenDialog_fnSetFileNameLabel,
    IFileOpenDialog_fnGetResult,
    IFileOpenDialog_fnAddPlace,
    IFileOpenDialog_fnSetDefaultExtension,
    IFileOpenDialog_fnClose,
    IFileOpenDialog_fnSetClientGuid,
    IFileOpenDialog_fnClearClientData,
    IFileOpenDialog_fnSetFilter,
    IFileOpenDialog_fnGetResults,
    IFileOpenDialog_fnGetSelectedItems
};

/**************************************************************************
 * IFileSaveDialog
 */
static inline FileDialogImpl *impl_from_IFileSaveDialog(IFileSaveDialog *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, u.IFileSaveDialog_iface);
}

static HRESULT WINAPI IFileSaveDialog_fnQueryInterface(IFileSaveDialog *iface,
                                                       REFIID riid,
                                                       void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IFileSaveDialog_fnAddRef(IFileSaveDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IFileSaveDialog_fnRelease(IFileSaveDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileSaveDialog_fnShow(IFileSaveDialog *iface, HWND hwndOwner)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Show(&This->IFileDialog2_iface, hwndOwner);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileTypes(IFileSaveDialog *iface, UINT cFileTypes,
                                                     const COMDLG_FILTERSPEC *rgFilterSpec)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileTypes(&This->IFileDialog2_iface, cFileTypes, rgFilterSpec);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileTypeIndex(IFileSaveDialog *iface, UINT iFileType)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileTypeIndex(&This->IFileDialog2_iface, iFileType);
}

static HRESULT WINAPI IFileSaveDialog_fnGetFileTypeIndex(IFileSaveDialog *iface, UINT *piFileType)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetFileTypeIndex(&This->IFileDialog2_iface, piFileType);
}

static HRESULT WINAPI IFileSaveDialog_fnAdvise(IFileSaveDialog *iface, IFileDialogEvents *pfde,
                                               DWORD *pdwCookie)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Advise(&This->IFileDialog2_iface, pfde, pdwCookie);
}

static HRESULT WINAPI IFileSaveDialog_fnUnadvise(IFileSaveDialog *iface, DWORD dwCookie)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Unadvise(&This->IFileDialog2_iface, dwCookie);
}

static HRESULT WINAPI IFileSaveDialog_fnSetOptions(IFileSaveDialog *iface, FILEOPENDIALOGOPTIONS fos)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetOptions(&This->IFileDialog2_iface, fos);
}

static HRESULT WINAPI IFileSaveDialog_fnGetOptions(IFileSaveDialog *iface, FILEOPENDIALOGOPTIONS *pfos)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetOptions(&This->IFileDialog2_iface, pfos);
}

static HRESULT WINAPI IFileSaveDialog_fnSetDefaultFolder(IFileSaveDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetDefaultFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFolder(IFileSaveDialog *iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFolder(&This->IFileDialog2_iface, psi);
}

static HRESULT WINAPI IFileSaveDialog_fnGetFolder(IFileSaveDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetFolder(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileSaveDialog_fnGetCurrentSelection(IFileSaveDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetCurrentSelection(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileName(IFileSaveDialog *iface, LPCWSTR pszName)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileSaveDialog_fnGetFileName(IFileSaveDialog *iface, LPWSTR *pszName)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetFileName(&This->IFileDialog2_iface, pszName);
}

static HRESULT WINAPI IFileSaveDialog_fnSetTitle(IFileSaveDialog *iface, LPCWSTR pszTitle)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetTitle(&This->IFileDialog2_iface, pszTitle);
}

static HRESULT WINAPI IFileSaveDialog_fnSetOkButtonLabel(IFileSaveDialog *iface, LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetOkButtonLabel(&This->IFileDialog2_iface, pszText);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFileNameLabel(IFileSaveDialog *iface, LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFileNameLabel(&This->IFileDialog2_iface, pszLabel);
}

static HRESULT WINAPI IFileSaveDialog_fnGetResult(IFileSaveDialog *iface, IShellItem **ppsi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_GetResult(&This->IFileDialog2_iface, ppsi);
}

static HRESULT WINAPI IFileSaveDialog_fnAddPlace(IFileSaveDialog *iface, IShellItem *psi, FDAP fdap)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_AddPlace(&This->IFileDialog2_iface, psi, fdap);
}

static HRESULT WINAPI IFileSaveDialog_fnSetDefaultExtension(IFileSaveDialog *iface,
                                                            LPCWSTR pszDefaultExtension)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetDefaultExtension(&This->IFileDialog2_iface, pszDefaultExtension);
}

static HRESULT WINAPI IFileSaveDialog_fnClose(IFileSaveDialog *iface, HRESULT hr)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_Close(&This->IFileDialog2_iface, hr);
}

static HRESULT WINAPI IFileSaveDialog_fnSetClientGuid(IFileSaveDialog *iface, REFGUID guid)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetClientGuid(&This->IFileDialog2_iface, guid);
}

static HRESULT WINAPI IFileSaveDialog_fnClearClientData(IFileSaveDialog *iface)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_ClearClientData(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileSaveDialog_fnSetFilter(IFileSaveDialog *iface, IShellItemFilter *pFilter)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    return IFileDialog2_SetFilter(&This->IFileDialog2_iface, pFilter);
}

static HRESULT WINAPI IFileSaveDialog_fnSetSaveAsItem(IFileSaveDialog* iface, IShellItem *psi)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p)\n", This, psi);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnSetProperties(IFileSaveDialog* iface, IPropertyStore *pStore)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p)\n", This, pStore);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnSetCollectedProperties(IFileSaveDialog* iface,
                                                               IPropertyDescriptionList *pList,
                                                               BOOL fAppendDefault)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p, %d)\n", This, pList, fAppendDefault);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnGetProperties(IFileSaveDialog* iface, IPropertyStore **ppStore)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("stub - %p (%p)\n", This, ppStore);
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileSaveDialog_fnApplyProperties(IFileSaveDialog* iface,
                                                        IShellItem *psi,
                                                        IPropertyStore *pStore,
                                                        HWND hwnd,
                                                        IFileOperationProgressSink *pSink)
{
    FileDialogImpl *This = impl_from_IFileSaveDialog(iface);
    FIXME("%p (%p, %p, %p, %p)\n", This, psi, pStore, hwnd, pSink);
    return E_NOTIMPL;
}

static const IFileSaveDialogVtbl vt_IFileSaveDialog = {
    IFileSaveDialog_fnQueryInterface,
    IFileSaveDialog_fnAddRef,
    IFileSaveDialog_fnRelease,
    IFileSaveDialog_fnShow,
    IFileSaveDialog_fnSetFileTypes,
    IFileSaveDialog_fnSetFileTypeIndex,
    IFileSaveDialog_fnGetFileTypeIndex,
    IFileSaveDialog_fnAdvise,
    IFileSaveDialog_fnUnadvise,
    IFileSaveDialog_fnSetOptions,
    IFileSaveDialog_fnGetOptions,
    IFileSaveDialog_fnSetDefaultFolder,
    IFileSaveDialog_fnSetFolder,
    IFileSaveDialog_fnGetFolder,
    IFileSaveDialog_fnGetCurrentSelection,
    IFileSaveDialog_fnSetFileName,
    IFileSaveDialog_fnGetFileName,
    IFileSaveDialog_fnSetTitle,
    IFileSaveDialog_fnSetOkButtonLabel,
    IFileSaveDialog_fnSetFileNameLabel,
    IFileSaveDialog_fnGetResult,
    IFileSaveDialog_fnAddPlace,
    IFileSaveDialog_fnSetDefaultExtension,
    IFileSaveDialog_fnClose,
    IFileSaveDialog_fnSetClientGuid,
    IFileSaveDialog_fnClearClientData,
    IFileSaveDialog_fnSetFilter,
    IFileSaveDialog_fnSetSaveAsItem,
    IFileSaveDialog_fnSetProperties,
    IFileSaveDialog_fnSetCollectedProperties,
    IFileSaveDialog_fnGetProperties,
    IFileSaveDialog_fnApplyProperties
};

/**************************************************************************
 * IExplorerBrowserEvents implementation
 */
static inline FileDialogImpl *impl_from_IExplorerBrowserEvents(IExplorerBrowserEvents *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, IExplorerBrowserEvents_iface);
}

static HRESULT WINAPI IExplorerBrowserEvents_fnQueryInterface(IExplorerBrowserEvents *iface,
                                                              REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    TRACE("%p (%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IExplorerBrowserEvents_fnAddRef(IExplorerBrowserEvents *iface)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    TRACE("%p\n", This);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IExplorerBrowserEvents_fnRelease(IExplorerBrowserEvents *iface)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    TRACE("%p\n", This);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationPending(IExplorerBrowserEvents *iface,
                                                                   PCIDLIST_ABSOLUTE pidlFolder)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    IShellItem *psi;
    HRESULT hr;
    TRACE("%p (%p)\n", This, pidlFolder);

    hr = SHCreateItemFromIDList(pidlFolder, &IID_IShellItem, (void**)&psi);
    if(SUCCEEDED(hr))
    {
        hr = events_OnFolderChanging(This, psi);
        IShellItem_Release(psi);

        /* The ExplorerBrowser treats S_FALSE as S_OK, we don't. */
        if(hr == S_FALSE)
            hr = E_FAIL;

        return hr;
    }
    else
        ERR("Failed to convert pidl (%p) to a shellitem.\n", pidlFolder);

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnViewCreated(IExplorerBrowserEvents *iface,
                                                             IShellView *psv)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    TRACE("%p (%p)\n", This, psv);
    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationComplete(IExplorerBrowserEvents *iface,
                                                                    PCIDLIST_ABSOLUTE pidlFolder)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    HRESULT hr;
    TRACE("%p (%p)\n", This, pidlFolder);

    if(This->psi_folder)
        IShellItem_Release(This->psi_folder);

    hr = SHCreateItemFromIDList(pidlFolder, &IID_IShellItem, (void**)&This->psi_folder);
    if(FAILED(hr))
    {
        ERR("Failed to get the current folder.\n");
        This->psi_folder = NULL;
    }

    events_OnFolderChange(This);

    return S_OK;
}

static HRESULT WINAPI IExplorerBrowserEvents_fnOnNavigationFailed(IExplorerBrowserEvents *iface,
                                                                  PCIDLIST_ABSOLUTE pidlFolder)
{
    FileDialogImpl *This = impl_from_IExplorerBrowserEvents(iface);
    TRACE("%p (%p)\n", This, pidlFolder);
    return S_OK;
}

static const IExplorerBrowserEventsVtbl vt_IExplorerBrowserEvents = {
    IExplorerBrowserEvents_fnQueryInterface,
    IExplorerBrowserEvents_fnAddRef,
    IExplorerBrowserEvents_fnRelease,
    IExplorerBrowserEvents_fnOnNavigationPending,
    IExplorerBrowserEvents_fnOnViewCreated,
    IExplorerBrowserEvents_fnOnNavigationComplete,
    IExplorerBrowserEvents_fnOnNavigationFailed
};

/**************************************************************************
 * IServiceProvider implementation
 */
static inline FileDialogImpl *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, IServiceProvider_iface);
}

static HRESULT WINAPI IServiceProvider_fnQueryInterface(IServiceProvider *iface,
                                                        REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_IServiceProvider(iface);
    TRACE("%p\n", This);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IServiceProvider_fnAddRef(IServiceProvider *iface)
{
    FileDialogImpl *This = impl_from_IServiceProvider(iface);
    TRACE("%p\n", This);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IServiceProvider_fnRelease(IServiceProvider *iface)
{
    FileDialogImpl *This = impl_from_IServiceProvider(iface);
    TRACE("%p\n", This);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IServiceProvider_fnQueryService(IServiceProvider *iface,
                                                      REFGUID guidService,
                                                      REFIID riid, void **ppv)
{
    FileDialogImpl *This = impl_from_IServiceProvider(iface);
    HRESULT hr = E_NOTIMPL;
    TRACE("%p (%s, %s, %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    *ppv = NULL;
    if(IsEqualGUID(guidService, &SID_STopLevelBrowser) && This->peb)
        hr = IExplorerBrowser_QueryInterface(This->peb, riid, ppv);
    else if(IsEqualGUID(guidService, &SID_SExplorerBrowserFrame))
        hr = IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppv);
    else
        FIXME("Interface %s requested from unknown service %s\n",
              debugstr_guid(riid), debugstr_guid(guidService));

    return hr;
}

static const IServiceProviderVtbl vt_IServiceProvider = {
    IServiceProvider_fnQueryInterface,
    IServiceProvider_fnAddRef,
    IServiceProvider_fnRelease,
    IServiceProvider_fnQueryService
};

/**************************************************************************
 * ICommDlgBrowser3 implementation
 */
static inline FileDialogImpl *impl_from_ICommDlgBrowser3(ICommDlgBrowser3 *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, ICommDlgBrowser3_iface);
}

static HRESULT WINAPI ICommDlgBrowser3_fnQueryInterface(ICommDlgBrowser3 *iface,
                                                        REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI ICommDlgBrowser3_fnAddRef(ICommDlgBrowser3 *iface)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI ICommDlgBrowser3_fnRelease(ICommDlgBrowser3 *iface)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    TRACE("%p\n", This);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnDefaultCommand(ICommDlgBrowser3 *iface,
                                                          IShellView *shv)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    HRESULT hr;
    TRACE("%p (%p)\n", This, shv);

    hr = on_default_action(This);

    if(SUCCEEDED(hr))
        EndDialog(This->dlg_hwnd, S_OK);

    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnStateChange(ICommDlgBrowser3 *iface,
                                                       IShellView *shv, ULONG uChange )
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    IDataObject *new_selection;
    HRESULT hr;
    TRACE("%p (%p, %x)\n", This, shv, uChange);

    switch(uChange)
    {
    case CDBOSC_SELCHANGE:
        if(This->psia_selection)
        {
            IShellItemArray_Release(This->psia_selection);
            This->psia_selection = NULL;
        }

        hr = IShellView_GetItemObject(shv, SVGIO_SELECTION, &IID_IDataObject, (void**)&new_selection);
        if(SUCCEEDED(hr))
        {
            hr = SHCreateShellItemArrayFromDataObject(new_selection, &IID_IShellItemArray,
                                                      (void**)&This->psia_selection);
            if(SUCCEEDED(hr))
            {
                fill_filename_from_selection(This);
                events_OnSelectionChange(This);
            }

            IDataObject_Release(new_selection);
        }
        break;
    default:
        TRACE("Unhandled state change\n");
    }
    return S_OK;
}

static HRESULT WINAPI ICommDlgBrowser3_fnIncludeObject(ICommDlgBrowser3 *iface,
                                                       IShellView *shv, LPCITEMIDLIST pidl)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    IShellItem *psi;
    LPWSTR filename;
    LPITEMIDLIST parent_pidl;
    HRESULT hr;
    ULONG attr;
    TRACE("%p (%p, %p)\n", This, shv, pidl);

    if(!This->filterspec_count && !(This->options & FOS_PICKFOLDERS))
        return S_OK;

    hr = SHGetIDListFromObject((IUnknown*)shv, &parent_pidl);
    if(SUCCEEDED(hr))
    {
        LPITEMIDLIST full_pidl = ILCombine(parent_pidl, pidl);
        hr = SHCreateItemFromIDList(full_pidl, &IID_IShellItem, (void**)&psi);
        ILFree(parent_pidl);
        ILFree(full_pidl);
    }
    if(FAILED(hr))
    {
        ERR("Failed to get shellitem (%08x).\n", hr);
        return S_OK;
    }

    hr = IShellItem_GetAttributes(psi, SFGAO_FOLDER|SFGAO_LINK, &attr);
    if(FAILED(hr) || (attr & (SFGAO_FOLDER | SFGAO_LINK)))
    {
        IShellItem_Release(psi);
        return S_OK;
    }

    if((This->options & FOS_PICKFOLDERS) && !(attr & (SFGAO_FOLDER | SFGAO_LINK)))
    {
        IShellItem_Release(psi);
        return S_FALSE;
    }

    hr = S_OK;
    if(SUCCEEDED(IShellItem_GetDisplayName(psi, SIGDN_PARENTRELATIVEPARSING, &filename)))
    {
        if(!PathMatchSpecW(filename, This->filterspecs[This->filetypeindex].pszSpec))
            hr = S_FALSE;
        CoTaskMemFree(filename);
    }

    IShellItem_Release(psi);
    return hr;
}

static HRESULT WINAPI ICommDlgBrowser3_fnNotify(ICommDlgBrowser3 *iface,
                                                IShellView *ppshv, DWORD dwNotifyType)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("Stub: %p (%p, 0x%x)\n", This, ppshv, dwNotifyType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetDefaultMenuText(ICommDlgBrowser3 *iface,
                                                            IShellView *pshv,
                                                            LPWSTR pszText, int cchMax)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("Stub: %p (%p, %p, %d)\n", This, pshv, pszText, cchMax);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetViewFlags(ICommDlgBrowser3 *iface, DWORD *pdwFlags)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("Stub: %p (%p)\n", This, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnColumnClicked(ICommDlgBrowser3 *iface,
                                                         IShellView *pshv, int iColumn)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("Stub: %p (%p, %d)\n", This, pshv, iColumn);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnGetCurrentFilter(ICommDlgBrowser3 *iface,
                                                          LPWSTR pszFileSpec, int cchFileSpec)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("Stub: %p (%p, %d)\n", This, pszFileSpec, cchFileSpec);
    return E_NOTIMPL;
}

static HRESULT WINAPI ICommDlgBrowser3_fnOnPreviewCreated(ICommDlgBrowser3 *iface,
                                                          IShellView *pshv)
{
    FileDialogImpl *This = impl_from_ICommDlgBrowser3(iface);
    FIXME("Stub: %p (%p)\n", This, pshv);
    return E_NOTIMPL;
}

static const ICommDlgBrowser3Vtbl vt_ICommDlgBrowser3 = {
    ICommDlgBrowser3_fnQueryInterface,
    ICommDlgBrowser3_fnAddRef,
    ICommDlgBrowser3_fnRelease,
    ICommDlgBrowser3_fnOnDefaultCommand,
    ICommDlgBrowser3_fnOnStateChange,
    ICommDlgBrowser3_fnIncludeObject,
    ICommDlgBrowser3_fnNotify,
    ICommDlgBrowser3_fnGetDefaultMenuText,
    ICommDlgBrowser3_fnGetViewFlags,
    ICommDlgBrowser3_fnOnColumnClicked,
    ICommDlgBrowser3_fnGetCurrentFilter,
    ICommDlgBrowser3_fnOnPreviewCreated
};

/**************************************************************************
 * IOleWindow implementation
 */
static inline FileDialogImpl *impl_from_IOleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, IOleWindow_iface);
}

static HRESULT WINAPI IOleWindow_fnQueryInterface(IOleWindow *iface, REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_IOleWindow(iface);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IOleWindow_fnAddRef(IOleWindow *iface)
{
    FileDialogImpl *This = impl_from_IOleWindow(iface);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IOleWindow_fnRelease(IOleWindow *iface)
{
    FileDialogImpl *This = impl_from_IOleWindow(iface);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IOleWindow_fnContextSensitiveHelp(IOleWindow *iface, BOOL fEnterMOde)
{
    FileDialogImpl *This = impl_from_IOleWindow(iface);
    FIXME("Stub: %p (%d)\n", This, fEnterMOde);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleWindow_fnGetWindow(IOleWindow *iface, HWND *phwnd)
{
    FileDialogImpl *This = impl_from_IOleWindow(iface);
    TRACE("%p (%p)\n", This, phwnd);
    *phwnd = This->dlg_hwnd;
    return S_OK;
}

static const IOleWindowVtbl vt_IOleWindow = {
    IOleWindow_fnQueryInterface,
    IOleWindow_fnAddRef,
    IOleWindow_fnRelease,
    IOleWindow_fnGetWindow,
    IOleWindow_fnContextSensitiveHelp
};

/**************************************************************************
 * IFileDialogCustomize implementation
 */
static inline FileDialogImpl *impl_from_IFileDialogCustomize(IFileDialogCustomize *iface)
{
    return CONTAINING_RECORD(iface, FileDialogImpl, IFileDialogCustomize_iface);
}

static HRESULT WINAPI IFileDialogCustomize_fnQueryInterface(IFileDialogCustomize *iface,
                                                            REFIID riid, void **ppvObject)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    return IFileDialog2_QueryInterface(&This->IFileDialog2_iface, riid, ppvObject);
}

static ULONG WINAPI IFileDialogCustomize_fnAddRef(IFileDialogCustomize *iface)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    return IFileDialog2_AddRef(&This->IFileDialog2_iface);
}

static ULONG WINAPI IFileDialogCustomize_fnRelease(IFileDialogCustomize *iface)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    return IFileDialog2_Release(&This->IFileDialog2_iface);
}

static HRESULT WINAPI IFileDialogCustomize_fnEnableOpenDropDown(IFileDialogCustomize *iface,
                                                                DWORD dwIDCtl)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    MENUINFO mi;
    TRACE("%p (%d)\n", This, dwIDCtl);

    if (This->hmenu_opendropdown || get_cctrl(This, dwIDCtl))
        return E_UNEXPECTED;

    This->hmenu_opendropdown = CreatePopupMenu();

    if (!This->hmenu_opendropdown)
        return E_OUTOFMEMORY;

    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE;
    mi.dwStyle = MNS_NOTIFYBYPOS;
    SetMenuInfo(This->hmenu_opendropdown, &mi);

    This->cctrl_opendropdown.hwnd = NULL;
    This->cctrl_opendropdown.wrapper_hwnd = NULL;
    This->cctrl_opendropdown.id = dwIDCtl;
    This->cctrl_opendropdown.dlgid = 0;
    This->cctrl_opendropdown.type = IDLG_CCTRL_OPENDROPDOWN;
    This->cctrl_opendropdown.cdcstate = CDCS_ENABLED | CDCS_VISIBLE;
    list_init(&This->cctrl_opendropdown.sub_cctrls);
    list_init(&This->cctrl_opendropdown.sub_items);

    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddMenu(IFileDialogCustomize *iface,
                                                     DWORD dwIDCtl,
                                                     LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    TBBUTTON tbb;
    HRESULT hr;
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pszLabel);

    hr = cctrl_create_new(This, dwIDCtl, NULL, TOOLBARCLASSNAMEW,
                          TBSTYLE_FLAT | CCS_NODIVIDER, 0,
                          This->cctrl_def_height, &ctrl);
    if(SUCCEEDED(hr))
    {
        SendMessageW(ctrl->hwnd, TB_BUTTONSTRUCTSIZE, sizeof(tbb), 0);
        ctrl->type = IDLG_CCTRL_MENU;

        /* Add the actual button with a popup menu. */
        tbb.iBitmap = I_IMAGENONE;
        tbb.dwData = (DWORD_PTR)CreatePopupMenu();
        tbb.iString = (DWORD_PTR)pszLabel;
        tbb.fsState = TBSTATE_ENABLED;
        tbb.fsStyle = BTNS_WHOLEDROPDOWN;
        tbb.idCommand = 1;

        SendMessageW(ctrl->hwnd, TB_ADDBUTTONSW, 1, (LPARAM)&tbb);
    }

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddPushButton(IFileDialogCustomize *iface,
                                                           DWORD dwIDCtl,
                                                           LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pszLabel);

    hr = cctrl_create_new(This, dwIDCtl, pszLabel, WC_BUTTONW, BS_MULTILINE, 0,
                          This->cctrl_def_height, &ctrl);
    if(SUCCEEDED(hr))
        ctrl->type = IDLG_CCTRL_PUSHBUTTON;

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddComboBox(IFileDialogCustomize *iface,
                                                         DWORD dwIDCtl)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d)\n", This, dwIDCtl);

    hr =  cctrl_create_new(This, dwIDCtl, NULL, WC_COMBOBOXW, CBS_DROPDOWNLIST, 0,
                           This->cctrl_def_height, &ctrl);
    if(SUCCEEDED(hr))
        ctrl->type = IDLG_CCTRL_COMBOBOX;

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddRadioButtonList(IFileDialogCustomize *iface,
                                                                DWORD dwIDCtl)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d)\n", This, dwIDCtl);

    hr =  cctrl_create_new(This, dwIDCtl, NULL, radiobuttonlistW, 0, 0, 0, &ctrl);
    if(SUCCEEDED(hr))
    {
        ctrl->type = IDLG_CCTRL_RADIOBUTTONLIST;
        SetWindowLongPtrW(ctrl->hwnd, GWLP_USERDATA, (LPARAM)This);
    }

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddCheckButton(IFileDialogCustomize *iface,
                                                            DWORD dwIDCtl,
                                                            LPCWSTR pszLabel,
                                                            BOOL bChecked)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d, %p, %d)\n", This, dwIDCtl, pszLabel, bChecked);

    hr = cctrl_create_new(This, dwIDCtl, pszLabel, WC_BUTTONW, BS_AUTOCHECKBOX|BS_MULTILINE, 0,
                          This->cctrl_def_height, &ctrl);
    if(SUCCEEDED(hr))
    {
        ctrl->type = IDLG_CCTRL_CHECKBUTTON;
        SendMessageW(ctrl->hwnd, BM_SETCHECK, bChecked ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddEditBox(IFileDialogCustomize *iface,
                                                        DWORD dwIDCtl,
                                                        LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pszText);

    hr = cctrl_create_new(This, dwIDCtl, pszText, WC_EDITW, ES_AUTOHSCROLL, WS_EX_CLIENTEDGE,
                          This->cctrl_def_height, &ctrl);
    if(SUCCEEDED(hr))
        ctrl->type = IDLG_CCTRL_EDITBOX;

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddSeparator(IFileDialogCustomize *iface,
                                                          DWORD dwIDCtl)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d)\n", This, dwIDCtl);

    hr = cctrl_create_new(This, dwIDCtl, NULL, WC_STATICW, SS_ETCHEDHORZ, 0,
                          GetSystemMetrics(SM_CYEDGE), &ctrl);
    if(SUCCEEDED(hr))
        ctrl->type = IDLG_CCTRL_SEPARATOR;

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddText(IFileDialogCustomize *iface,
                                                     DWORD dwIDCtl,
                                                     LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl;
    HRESULT hr;
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pszText);

    hr = cctrl_create_new(This, dwIDCtl, pszText, WC_STATICW, 0, 0,
                          This->cctrl_def_height, &ctrl);
    if(SUCCEEDED(hr))
        ctrl->type = IDLG_CCTRL_TEXT;

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetControlLabel(IFileDialogCustomize *iface,
                                                             DWORD dwIDCtl,
                                                             LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pszLabel);

    if(!ctrl) return E_INVALIDARG;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_MENU:
    case IDLG_CCTRL_PUSHBUTTON:
    case IDLG_CCTRL_CHECKBUTTON:
    case IDLG_CCTRL_TEXT:
    case IDLG_CCTRL_VISUALGROUP:
        SendMessageW(ctrl->hwnd, WM_SETTEXT, 0, (LPARAM)pszLabel);
        break;
    case IDLG_CCTRL_OPENDROPDOWN:
        return E_NOTIMPL;
    default:
        break;
    }

    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnGetControlState(IFileDialogCustomize *iface,
                                                             DWORD dwIDCtl,
                                                             CDCONTROLSTATEF *pdwState)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pdwState);

    if(!ctrl || ctrl->type == IDLG_CCTRL_OPENDROPDOWN) return E_NOTIMPL;

    *pdwState = ctrl->cdcstate;
    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetControlState(IFileDialogCustomize *iface,
                                                             DWORD dwIDCtl,
                                                             CDCONTROLSTATEF dwState)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This,dwIDCtl);
    TRACE("%p (%d, %x)\n", This, dwIDCtl, dwState);

    if(ctrl && ctrl->hwnd)
    {
        LONG wndstyle = GetWindowLongW(ctrl->hwnd, GWL_STYLE);

        if(dwState & CDCS_ENABLED)
            wndstyle &= ~(WS_DISABLED);
        else
            wndstyle |= WS_DISABLED;

        if(dwState & CDCS_VISIBLE)
            wndstyle |= WS_VISIBLE;
        else
            wndstyle &= ~(WS_VISIBLE);

        SetWindowLongW(ctrl->hwnd, GWL_STYLE, wndstyle);

        /* We save the state separately since at least one application
         * relies on being able to hide a control. */
        ctrl->cdcstate = dwState;
    }

    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnGetEditBoxText(IFileDialogCustomize *iface,
                                                            DWORD dwIDCtl,
                                                            WCHAR **ppszText)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    WCHAR len, *text;
    TRACE("%p (%d, %p)\n", This, dwIDCtl, ppszText);

    if(!ctrl || !ctrl->hwnd || !(len = SendMessageW(ctrl->hwnd, WM_GETTEXTLENGTH, 0, 0)))
        return E_FAIL;

    text = CoTaskMemAlloc(sizeof(WCHAR)*(len+1));
    if(!text) return E_FAIL;

    SendMessageW(ctrl->hwnd, WM_GETTEXT, len+1, (LPARAM)text);
    *ppszText = text;
    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetEditBoxText(IFileDialogCustomize *iface,
                                                            DWORD dwIDCtl,
                                                            LPCWSTR pszText)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %s)\n", This, dwIDCtl, debugstr_w(pszText));

    if(!ctrl || ctrl->type != IDLG_CCTRL_EDITBOX)
        return E_FAIL;

    SendMessageW(ctrl->hwnd, WM_SETTEXT, 0, (LPARAM)pszText);
    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnGetCheckButtonState(IFileDialogCustomize *iface,
                                                                 DWORD dwIDCtl,
                                                                 BOOL *pbChecked)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pbChecked);

    if(ctrl && ctrl->hwnd)
        *pbChecked = (SendMessageW(ctrl->hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);

    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetCheckButtonState(IFileDialogCustomize *iface,
                                                                 DWORD dwIDCtl,
                                                                 BOOL bChecked)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %d)\n", This, dwIDCtl, bChecked);

    if(ctrl && ctrl->hwnd)
        SendMessageW(ctrl->hwnd, BM_SETCHECK, bChecked ? BST_CHECKED:BST_UNCHECKED, 0);

    return S_OK;
}

static UINT get_combobox_index_from_id(HWND cb_hwnd, DWORD dwIDItem)
{
    UINT count = SendMessageW(cb_hwnd, CB_GETCOUNT, 0, 0);
    UINT i;
    if(!count || (count == CB_ERR))
        return -1;

    for(i = 0; i < count; i++)
        if(SendMessageW(cb_hwnd, CB_GETITEMDATA, i, 0) == dwIDItem)
            return i;

    TRACE("Item with id %d not found in combobox %p (item count: %d)\n", dwIDItem, cb_hwnd, count);
    return -1;
}

static HRESULT WINAPI IFileDialogCustomize_fnAddControlItem(IFileDialogCustomize *iface,
                                                            DWORD dwIDCtl,
                                                            DWORD dwIDItem,
                                                            LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    HRESULT hr;
    TRACE("%p (%d, %d, %s)\n", This, dwIDCtl, dwIDItem, debugstr_w(pszLabel));

    if(!ctrl) return E_FAIL;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_COMBOBOX:
    {
        UINT index;
        cctrl_item* item;

        hr = add_item(ctrl, dwIDItem, pszLabel, &item);

        if (FAILED(hr)) return hr;

        index = SendMessageW(ctrl->hwnd, CB_ADDSTRING, 0, (LPARAM)pszLabel);
        SendMessageW(ctrl->hwnd, CB_SETITEMDATA, index, dwIDItem);

        return S_OK;
    }
    case IDLG_CCTRL_MENU:
    case IDLG_CCTRL_OPENDROPDOWN:
    {
        cctrl_item* item;
        HMENU hmenu;

        hr = add_item(ctrl, dwIDItem, pszLabel, &item);

        if (FAILED(hr)) return hr;

        if (ctrl->type == IDLG_CCTRL_MENU)
        {
            TBBUTTON tbb;
            SendMessageW(ctrl->hwnd, TB_GETBUTTON, 0, (LPARAM)&tbb);
            hmenu = (HMENU)tbb.dwData;
        }
        else /* ctrl->type == IDLG_CCTRL_OPENDROPDOWN */
            hmenu = This->hmenu_opendropdown;

        AppendMenuW(hmenu, MF_STRING, dwIDItem, pszLabel);
        return S_OK;
    }
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        hr = add_item(ctrl, dwIDItem, pszLabel, &item);

        if (SUCCEEDED(hr))
        {
            item->hwnd = CreateWindowExW(0, WC_BUTTONW, pszLabel,
                WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|BS_RADIOBUTTON|BS_MULTILINE,
                0, 0, 0, 0, ctrl->hwnd, ULongToHandle(dwIDItem), COMDLG32_hInstance, 0);

            if (!item->hwnd)
            {
                ERR("Failed to create radio button\n");
                list_remove(&item->entry);
                item_free(item);
                return E_FAIL;
            }
        }

        return hr;
    }
    default:
        break;
    }

    return E_NOINTERFACE; /* win7 */
}

static HRESULT WINAPI IFileDialogCustomize_fnRemoveControlItem(IFileDialogCustomize *iface,
                                                               DWORD dwIDCtl,
                                                               DWORD dwIDItem)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %d)\n", This, dwIDCtl, dwIDItem);

    if(!ctrl) return E_FAIL;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_COMBOBOX:
    {
        cctrl_item* item;
        DWORD position;

        item = get_item(ctrl, dwIDItem, CDCS_VISIBLE|CDCS_ENABLED, &position);

        if ((item->cdcstate & (CDCS_VISIBLE|CDCS_ENABLED)) == (CDCS_VISIBLE|CDCS_ENABLED))
        {
            if(SendMessageW(ctrl->hwnd, CB_DELETESTRING, position, 0) == CB_ERR)
                return E_FAIL;
        }

        list_remove(&item->entry);
        item_free(item);

        return S_OK;
    }
    case IDLG_CCTRL_MENU:
    case IDLG_CCTRL_OPENDROPDOWN:
    {
        HMENU hmenu;
        cctrl_item* item;

        item = get_item(ctrl, dwIDItem, 0, NULL);

        if (!item)
            return E_UNEXPECTED;

        if (item->cdcstate & CDCS_VISIBLE)
        {
            if (ctrl->type == IDLG_CCTRL_MENU)
            {
                TBBUTTON tbb;
                SendMessageW(ctrl->hwnd, TB_GETBUTTON, 0, (LPARAM)&tbb);
                hmenu = (HMENU)tbb.dwData;
            }
            else /* ctrl->type == IDLG_CCTRL_OPENDROPDOWN */
                hmenu = This->hmenu_opendropdown;

            if(!hmenu || !DeleteMenu(hmenu, dwIDItem, MF_BYCOMMAND))
                return E_UNEXPECTED;
        }

        list_remove(&item->entry);
        item_free(item);

        return S_OK;
    }
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        item = get_item(ctrl, dwIDItem, 0, NULL);

        if (!item)
            return E_UNEXPECTED;

        list_remove(&item->entry);
        item_free(item);

        return S_OK;
    }
    default:
        break;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialogCustomize_fnRemoveAllControlItems(IFileDialogCustomize *iface,
                                                                   DWORD dwIDCtl)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    TRACE("%p (%d)\n", This, dwIDCtl);

    /* Not implemented by native */
    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialogCustomize_fnGetControlItemState(IFileDialogCustomize *iface,
                                                                 DWORD dwIDCtl,
                                                                 DWORD dwIDItem,
                                                                 CDCONTROLSTATEF *pdwState)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %d, %p)\n", This, dwIDCtl, dwIDItem, pdwState);

    if(!ctrl) return E_FAIL;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_COMBOBOX:
    case IDLG_CCTRL_MENU:
    case IDLG_CCTRL_OPENDROPDOWN:
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        item = get_item(ctrl, dwIDItem, 0, NULL);

        if (!item)
            return E_UNEXPECTED;

        *pdwState = item->cdcstate;

        return S_OK;
    }
    default:
        break;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetControlItemState(IFileDialogCustomize *iface,
                                                                 DWORD dwIDCtl,
                                                                 DWORD dwIDItem,
                                                                 CDCONTROLSTATEF dwState)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %d, %x)\n", This, dwIDCtl, dwIDItem, dwState);

    if(!ctrl) return E_FAIL;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_COMBOBOX:
    {
        cctrl_item* item;
        BOOL visible, was_visible;
        DWORD position;

        item = get_item(ctrl, dwIDItem, CDCS_VISIBLE|CDCS_ENABLED, &position);

        if (!item)
            return E_UNEXPECTED;

        visible = ((dwState & (CDCS_VISIBLE|CDCS_ENABLED)) == (CDCS_VISIBLE|CDCS_ENABLED));
        was_visible = ((item->cdcstate & (CDCS_VISIBLE|CDCS_ENABLED)) == (CDCS_VISIBLE|CDCS_ENABLED));

        if (visible && !was_visible)
        {
            SendMessageW(ctrl->hwnd, CB_INSERTSTRING, position, (LPARAM)item->label);
            SendMessageW(ctrl->hwnd, CB_SETITEMDATA, position, dwIDItem);
        }
        else if (!visible && was_visible)
        {
            SendMessageW(ctrl->hwnd, CB_DELETESTRING, position, 0);
        }

        item->cdcstate = dwState;

        return S_OK;
    }
    case IDLG_CCTRL_MENU:
    case IDLG_CCTRL_OPENDROPDOWN:
    {
        HMENU hmenu;
        cctrl_item* item;
        CDCONTROLSTATEF prev_state;
        DWORD position;

        item = get_item(ctrl, dwIDItem, CDCS_VISIBLE, &position);

        if (!item)
            return E_UNEXPECTED;

        prev_state = item->cdcstate;

        if (ctrl->type == IDLG_CCTRL_MENU)
        {
            TBBUTTON tbb;
            SendMessageW(ctrl->hwnd, TB_GETBUTTON, 0, (LPARAM)&tbb);
            hmenu = (HMENU)tbb.dwData;
        }
        else /* ctrl->type == IDLG_CCTRL_OPENDROPDOWN */
            hmenu = This->hmenu_opendropdown;

        if (dwState & CDCS_VISIBLE)
        {
            if (prev_state & CDCS_VISIBLE)
            {
                /* change state */
                EnableMenuItem(hmenu, dwIDItem,
                    MF_BYCOMMAND|((dwState & CDCS_ENABLED) ? MFS_ENABLED : MFS_DISABLED));
            }
            else
            {
                /* show item */
                MENUITEMINFOW mii;

                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_ID|MIIM_STATE|MIIM_STRING;
                mii.fState = (dwState & CDCS_ENABLED) ? MFS_ENABLED : MFS_DISABLED;
                mii.wID = dwIDItem;
                mii.dwTypeData = item->label;

                InsertMenuItemW(hmenu, position, TRUE, &mii);
            }
        }
        else if (prev_state & CDCS_VISIBLE)
        {
            /* hide item */
            DeleteMenu(hmenu, dwIDItem, MF_BYCOMMAND);
        }

        item->cdcstate = dwState;

        if (ctrl->type == IDLG_CCTRL_OPENDROPDOWN)
        {
            update_control_text(This);
            update_layout(This);
        }

        return S_OK;
    }
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        item = get_item(ctrl, dwIDItem, CDCS_VISIBLE, NULL);

        if (!item)
            return E_UNEXPECTED;

        /* Oddly, native allows setting this but doesn't seem to do anything with it. */
        item->cdcstate = dwState;

        return S_OK;
    }
    default:
        break;
    }

    return E_FAIL;
}

static HRESULT WINAPI IFileDialogCustomize_fnGetSelectedControlItem(IFileDialogCustomize *iface,
                                                                    DWORD dwIDCtl,
                                                                    DWORD *pdwIDItem)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %p)\n", This, dwIDCtl, pdwIDItem);

    if(!ctrl) return E_FAIL;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_COMBOBOX:
    {
        UINT index = SendMessageW(ctrl->hwnd, CB_GETCURSEL, 0, 0);
        if(index == CB_ERR)
            return E_FAIL;

        *pdwIDItem = SendMessageW(ctrl->hwnd, CB_GETITEMDATA, index, 0);
        return S_OK;
    }
    case IDLG_CCTRL_OPENDROPDOWN:
        if (This->opendropdown_has_selection)
        {
            *pdwIDItem = This->opendropdown_selection;
            return S_OK;
        }
        else
        {
            /* Return first enabled item. */
            cctrl_item* item = get_first_item(ctrl);

            if (item)
            {
                *pdwIDItem = item->id;
                return S_OK;
            }

            WARN("no enabled items in open dropdown\n");
            return E_FAIL;
        }
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        LIST_FOR_EACH_ENTRY(item, &ctrl->sub_items, cctrl_item, entry)
        {
            if (SendMessageW(item->hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
                *pdwIDItem = item->id;
                return S_OK;
            }
        }

        WARN("no checked items in radio button list\n");
        return E_FAIL;
    }
    default:
        FIXME("Unsupported control type %d\n", ctrl->type);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetSelectedControlItem(IFileDialogCustomize *iface,
                                                                    DWORD dwIDCtl,
                                                                    DWORD dwIDItem)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *ctrl = get_cctrl(This, dwIDCtl);
    TRACE("%p (%d, %d)\n", This, dwIDCtl, dwIDItem);

    if(!ctrl) return E_INVALIDARG;

    switch(ctrl->type)
    {
    case IDLG_CCTRL_COMBOBOX:
    {
        UINT index = get_combobox_index_from_id(ctrl->hwnd, dwIDItem);

        if(index == -1)
            return E_INVALIDARG;

        if(SendMessageW(ctrl->hwnd, CB_SETCURSEL, index, 0) == CB_ERR)
            return E_FAIL;

        return S_OK;
    }
    case IDLG_CCTRL_RADIOBUTTONLIST:
    {
        cctrl_item* item;

        item = get_item(ctrl, dwIDItem, 0, NULL);

        if (item)
        {
            radiobuttonlist_set_selected_item(This, ctrl, item);
            return S_OK;
        }

        return E_INVALIDARG;
    }
    default:
        FIXME("Unsupported control type %d\n", ctrl->type);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI IFileDialogCustomize_fnStartVisualGroup(IFileDialogCustomize *iface,
                                                              DWORD dwIDCtl,
                                                              LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    customctrl *vg;
    HRESULT hr;
    TRACE("%p (%d, %s)\n", This, dwIDCtl, debugstr_w(pszLabel));

    if(This->cctrl_active_vg)
        return E_UNEXPECTED;

    hr = cctrl_create_new(This, dwIDCtl, pszLabel, WC_STATICW, 0, 0,
                          This->cctrl_def_height, &vg);
    if(SUCCEEDED(hr))
    {
        vg->type = IDLG_CCTRL_VISUALGROUP;
        This->cctrl_active_vg = vg;
    }

    return hr;
}

static HRESULT WINAPI IFileDialogCustomize_fnEndVisualGroup(IFileDialogCustomize *iface)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    TRACE("%p\n", This);

    This->cctrl_active_vg = NULL;

    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnMakeProminent(IFileDialogCustomize *iface,
                                                           DWORD dwIDCtl)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    FIXME("stub - %p (%d)\n", This, dwIDCtl);
    return S_OK;
}

static HRESULT WINAPI IFileDialogCustomize_fnSetControlItemText(IFileDialogCustomize *iface,
                                                                DWORD dwIDCtl,
                                                                DWORD dwIDItem,
                                                                LPCWSTR pszLabel)
{
    FileDialogImpl *This = impl_from_IFileDialogCustomize(iface);
    FIXME("stub - %p (%d, %d, %s)\n", This, dwIDCtl, dwIDItem, debugstr_w(pszLabel));
    return E_NOTIMPL;
}

static const IFileDialogCustomizeVtbl vt_IFileDialogCustomize = {
    IFileDialogCustomize_fnQueryInterface,
    IFileDialogCustomize_fnAddRef,
    IFileDialogCustomize_fnRelease,
    IFileDialogCustomize_fnEnableOpenDropDown,
    IFileDialogCustomize_fnAddMenu,
    IFileDialogCustomize_fnAddPushButton,
    IFileDialogCustomize_fnAddComboBox,
    IFileDialogCustomize_fnAddRadioButtonList,
    IFileDialogCustomize_fnAddCheckButton,
    IFileDialogCustomize_fnAddEditBox,
    IFileDialogCustomize_fnAddSeparator,
    IFileDialogCustomize_fnAddText,
    IFileDialogCustomize_fnSetControlLabel,
    IFileDialogCustomize_fnGetControlState,
    IFileDialogCustomize_fnSetControlState,
    IFileDialogCustomize_fnGetEditBoxText,
    IFileDialogCustomize_fnSetEditBoxText,
    IFileDialogCustomize_fnGetCheckButtonState,
    IFileDialogCustomize_fnSetCheckButtonState,
    IFileDialogCustomize_fnAddControlItem,
    IFileDialogCustomize_fnRemoveControlItem,
    IFileDialogCustomize_fnRemoveAllControlItems,
    IFileDialogCustomize_fnGetControlItemState,
    IFileDialogCustomize_fnSetControlItemState,
    IFileDialogCustomize_fnGetSelectedControlItem,
    IFileDialogCustomize_fnSetSelectedControlItem,
    IFileDialogCustomize_fnStartVisualGroup,
    IFileDialogCustomize_fnEndVisualGroup,
    IFileDialogCustomize_fnMakeProminent,
    IFileDialogCustomize_fnSetControlItemText
};

static HRESULT FileDialog_constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv, enum ITEMDLG_TYPE type)
{
    FileDialogImpl *fdimpl;
    HRESULT hr;
    IShellFolder *psf;
    TRACE("%p, %s, %p\n", pUnkOuter, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    fdimpl = HeapAlloc(GetProcessHeap(), 0, sizeof(FileDialogImpl));
    if(!fdimpl)
        return E_OUTOFMEMORY;

    fdimpl->ref = 1;
    fdimpl->IFileDialog2_iface.lpVtbl = &vt_IFileDialog2;
    fdimpl->IExplorerBrowserEvents_iface.lpVtbl = &vt_IExplorerBrowserEvents;
    fdimpl->IServiceProvider_iface.lpVtbl = &vt_IServiceProvider;
    fdimpl->ICommDlgBrowser3_iface.lpVtbl = &vt_ICommDlgBrowser3;
    fdimpl->IOleWindow_iface.lpVtbl = &vt_IOleWindow;
    fdimpl->IFileDialogCustomize_iface.lpVtbl = &vt_IFileDialogCustomize;

    if(type == ITEMDLG_TYPE_OPEN)
    {
        fdimpl->dlg_type = ITEMDLG_TYPE_OPEN;
        fdimpl->u.IFileOpenDialog_iface.lpVtbl = &vt_IFileOpenDialog;
        fdimpl->options = FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR;
        fdimpl->custom_title = fdimpl->custom_okbutton = NULL;
    }
    else
    {
        WCHAR buf[16];
        fdimpl->dlg_type = ITEMDLG_TYPE_SAVE;
        fdimpl->u.IFileSaveDialog_iface.lpVtbl = &vt_IFileSaveDialog;
        fdimpl->options = FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;

        LoadStringW(COMDLG32_hInstance, IDS_SAVE, buf, sizeof(buf)/sizeof(WCHAR));
        fdimpl->custom_title = StrDupW(buf);
        fdimpl->custom_okbutton = StrDupW(buf);
    }

    fdimpl->filterspecs = NULL;
    fdimpl->filterspec_count = 0;
    fdimpl->filetypeindex = 0;

    fdimpl->psia_selection = fdimpl->psia_results = NULL;
    fdimpl->psi_setfolder = fdimpl->psi_folder = NULL;

    list_init(&fdimpl->events_clients);
    fdimpl->events_next_cookie = 0;

    fdimpl->dlg_hwnd = NULL;
    fdimpl->peb = NULL;

    fdimpl->set_filename = NULL;
    fdimpl->default_ext = NULL;
    fdimpl->custom_cancelbutton = fdimpl->custom_filenamelabel = NULL;

    fdimpl->client_guid = GUID_NULL;

    fdimpl->hmenu_opendropdown = NULL;
    fdimpl->hfont_opendropdown = NULL;

    /* FIXME: The default folder setting should be restored for the
     * application if it was previously set. */
    SHGetDesktopFolder(&psf);
    SHGetItemFromObject((IUnknown*)psf, &IID_IShellItem, (void**)&fdimpl->psi_defaultfolder);
    IShellFolder_Release(psf);

    hr = init_custom_controls(fdimpl);
    if(FAILED(hr))
    {
        ERR("Failed to initialize custom controls (0x%08x).\n", hr);
        IFileDialog2_Release(&fdimpl->IFileDialog2_iface);
        return E_FAIL;
    }

    hr = IFileDialog2_QueryInterface(&fdimpl->IFileDialog2_iface, riid, ppv);
    IFileDialog2_Release(&fdimpl->IFileDialog2_iface);
    return hr;
}

HRESULT FileOpenDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    return FileDialog_constructor(pUnkOuter, riid, ppv, ITEMDLG_TYPE_OPEN);
}

HRESULT FileSaveDialog_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    return FileDialog_constructor(pUnkOuter, riid, ppv, ITEMDLG_TYPE_SAVE);
}

#endif /* Win 7 */
