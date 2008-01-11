/*
 * Wordpad implementation
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2007 by Alexander N. Sørnes <alex@thehandofagony.com>
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

#define WIN32_LEAN_AND_MEAN
//#define _WIN32_IE 0x0400

#define MAX_STRING_LEN 255

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <math.h>
#include <errno.h>

#include "resource.h"

#ifdef NONAMELESSUNION
# define U(x)  (x).u
# define U2(x) (x).u2
# define U3(x) (x).u3
#else
# define U(x)  (x)
# define U2(x) (x)
# define U3(x) (x)
#endif

/* use LoadString */
static const WCHAR xszAppTitle[] = {'W','i','n','e',' ','W','o','r','d','p','a','d',0};
static const WCHAR xszMainMenu[] = {'M','A','I','N','M','E','N','U',0};

static const WCHAR wszRichEditClass[] = {'R','I','C','H','E','D','I','T','2','0','W',0};
static const WCHAR wszMainWndClass[] = {'W','O','R','D','P','A','D','T','O','P',0};
static const WCHAR wszAppTitle[] = {'W','i','n','e',' ','W','o','r','d','p','a','d',0};

static const WCHAR key_recentfiles[] = {'R','e','c','e','n','t',' ','f','i','l','e',
                                        ' ','l','i','s','t',0};
static const WCHAR key_options[] = {'O','p','t','i','o','n','s',0};
static const WCHAR key_rtf[] = {'R','T','F',0};
static const WCHAR key_text[] = {'T','e','x','t',0};

static const WCHAR var_file[] = {'F','i','l','e','%','d',0};
static const WCHAR var_framerect[] = {'F','r','a','m','e','R','e','c','t',0};
static const WCHAR var_barstate0[] = {'B','a','r','S','t','a','t','e','0',0};
static const WCHAR var_pagemargin[] = {'P','a','g','e','M','a','r','g','i','n',0};

static const WCHAR stringFormat[] = {'%','2','d','\0'};

static HWND hMainWnd;
static HWND hEditorWnd;
static HWND hFindWnd;
static HMENU hPopupMenu;

static UINT ID_FINDMSGSTRING;

static WCHAR wszFilter[MAX_STRING_LEN*4+6*3+5];
static WCHAR wszDefaultFileName[MAX_STRING_LEN];
static WCHAR wszSaveChanges[MAX_STRING_LEN];
static WCHAR wszPrintFilter[MAX_STRING_LEN*2+6+4+1];
static WCHAR units_cmW[MAX_STRING_LEN];

static char units_cmA[MAX_STRING_LEN];

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam );

/* Load string resources */
static void DoLoadStrings(void)
{
    LPWSTR p = wszFilter;
    static const WCHAR files_rtf[] = {'*','.','r','t','f','\0'};
    static const WCHAR files_txt[] = {'*','.','t','x','t','\0'};
    static const WCHAR files_all[] = {'*','.','*','\0'};
    static const WCHAR files_prn[] = {'*','.','P','R','N',0};
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);

    LoadStringW(hInstance, STRING_RICHTEXT_FILES_RTF, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_rtf);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_TEXT_FILES_TXT, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_txt);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_TEXT_FILES_UNICODE_TXT, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_txt);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_all);
    p += lstrlenW(p) + 1;
    *p = '\0';

    p = wszPrintFilter;
    LoadStringW(hInstance, STRING_PRINTER_FILES_PRN, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_prn);
    p += lstrlenW(p) + 1;
    LoadStringW(hInstance, STRING_ALL_FILES, p, MAX_STRING_LEN);
    p += lstrlenW(p) + 1;
    lstrcpyW(p, files_all);
    p += lstrlenW(p) + 1;
    *p = 0;

    p = wszDefaultFileName;
    LoadStringW(hInstance, STRING_DEFAULT_FILENAME, p, MAX_STRING_LEN);

    p = wszSaveChanges;
    LoadStringW(hInstance, STRING_PROMPT_SAVE_CHANGES, p, MAX_STRING_LEN);

    LoadStringA(hInstance, STRING_UNITS_CM, units_cmA, MAX_STRING_LEN);
    LoadStringW(hInstance, STRING_UNITS_CM, units_cmW, MAX_STRING_LEN);
}

static void AddButton(HWND hwndToolBar, int nImage, int nCommand)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = nImage;
    button.idCommand = nCommand;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = TBSTYLE_BUTTON;
    button.dwData = 0;
    button.iString = -1;
    SendMessageW(hwndToolBar, TB_ADDBUTTONSW, 1, (LPARAM)&button);
}

static void AddTextButton(HWND hWnd, int string, int command, int id)
{
    REBARBANDINFOW rb;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);
    static const WCHAR button[] = {'B','U','T','T','O','N',0};
    WCHAR text[MAX_STRING_LEN];
    HWND hButton;
    RECT rc;

    LoadStringW(hInstance, string, text, MAX_STRING_LEN);
    hButton = CreateWindowW(button, text,
                            WS_VISIBLE | WS_CHILD, 5, 5, 100, 15,
                            hMainWnd, (HMENU)command, hInstance, NULL);

    rb.cbSize = sizeof(rb);
    rb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_ID;
    rb.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
    rb.hwndChild = hButton;
    rb.cyChild = rb.cyMinChild = 22;
    rb.cx = rb.cxMinChild = 90;
    rb.cxIdeal = 100;
    rb.wID = id;

    rc.bottom = 22;
    rc.right = 90;

    SendMessageW(hWnd, RB_INSERTBAND, -1, (LPARAM)&rb);
    SetWindowPos(hButton, 0, 0, 0, 90, 22, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

static void AddSeparator(HWND hwndToolBar)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = -1;
    button.idCommand = 0;
    button.fsState = 0;
    button.fsStyle = TBSTYLE_SEP;
    button.dwData = 0;
    button.iString = -1;
    SendMessageW(hwndToolBar, TB_ADDBUTTONSW, 1, (LPARAM)&button);
}

static DWORD CALLBACK stream_in(DWORD_PTR cookie, LPBYTE buffer, LONG cb, LONG *pcb)
{
    HANDLE hFile = (HANDLE)cookie;
    DWORD read;

    if(!ReadFile(hFile, buffer, cb, &read, 0))
        return 1;

    *pcb = read;

    return 0;
}

static DWORD CALLBACK stream_out(DWORD_PTR cookie, LPBYTE buffer, LONG cb, LONG *pcb)
{
    DWORD written;
    int ret;
    HANDLE hFile = (HANDLE)cookie;

    ret = WriteFile(hFile, buffer, cb, &written, 0);

    if(!ret || (cb != written))
        return 1;

    *pcb = cb;

    return 0;
}


static LPWSTR file_basename(LPWSTR path)
{
    LPWSTR pos = path + lstrlenW(path);

    while(pos > path)
    {
        if(*pos == '\\' || *pos == '/')
        {
            pos++;
            break;
        }
        pos--;
    }
    return pos;
}

static WCHAR wszFileName[MAX_PATH];
static WPARAM fileFormat = SF_RTF;

static void set_caption(LPCWSTR wszNewFileName)
{
    static const WCHAR wszSeparator[] = {' ','-',' '};
    WCHAR *wszCaption;
    SIZE_T length = 0;

    if(!wszNewFileName)
        wszNewFileName = wszDefaultFileName;
    else
        wszNewFileName = file_basename((LPWSTR)wszNewFileName);

    wszCaption = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                lstrlenW(wszNewFileName)*sizeof(WCHAR)+sizeof(wszSeparator)+sizeof(wszAppTitle));

    if(!wszCaption)
        return;

    memcpy(wszCaption, wszNewFileName, lstrlenW(wszNewFileName)*sizeof(WCHAR));
    length += lstrlenW(wszNewFileName);
    memcpy(wszCaption + length, wszSeparator, sizeof(wszSeparator));
    length += sizeof(wszSeparator) / sizeof(WCHAR);
    memcpy(wszCaption + length, wszAppTitle, sizeof(wszAppTitle));

    SetWindowTextW(hMainWnd, wszCaption);

    HeapFree(GetProcessHeap(), 0, wszCaption);
}

static LRESULT registry_get_handle(HKEY *hKey, LPDWORD action, LPCWSTR subKey)
{
    LONG ret;
    static const WCHAR wszProgramKey[] = {'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'A','p','p','l','e','t','s','\\',
        'W','o','r','d','p','a','d',0};
    LPWSTR key = (LPWSTR)wszProgramKey;

    if(subKey)
    {
        WCHAR backslash[] = {'\\',0};
        key = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                        (lstrlenW(wszProgramKey)+lstrlenW(subKey)+lstrlenW(backslash)+1)
                        *sizeof(WCHAR));

        if(!key)
            return 1;

        lstrcpyW(key, wszProgramKey);
        lstrcatW(key, backslash);
        lstrcatW(key, subKey);
    }

    if(action)
    {
        ret = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE, NULL, hKey, action);
    } else
    {
        ret = RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ | KEY_WRITE, hKey);
    }

    if(subKey)
        HeapFree(GetProcessHeap(), 0, key);

    return ret;
}

static RECT margins;

static void registry_set_options(void)
{
    HKEY hKey;
    DWORD action;

    if(registry_get_handle(&hKey, &action, (LPWSTR)key_options) == ERROR_SUCCESS)
    {
        RECT rc;

        GetWindowRect(hMainWnd, &rc);

        RegSetValueExW(hKey, var_framerect, 0, REG_BINARY, (LPBYTE)&rc, sizeof(RECT));

        RegSetValueExW(hKey, var_pagemargin, 0, REG_BINARY, (LPBYTE)&margins, sizeof(RECT));
    }
}

static RECT registry_read_winrect(void)
{
    HKEY hKey;
    RECT rc;
    DWORD size = sizeof(RECT);

    ZeroMemory(&rc, sizeof(RECT));
    if(registry_get_handle(&hKey, 0, (LPWSTR)key_options) != ERROR_SUCCESS ||
       RegQueryValueExW(hKey, var_framerect, 0, NULL, (LPBYTE)&rc, &size) !=
       ERROR_SUCCESS || size != sizeof(RECT))
    {
        rc.top = 0;
        rc.left = 0;
        rc.bottom = 300;
        rc.right = 600;
    }

    RegCloseKey(hKey);
    return rc;
}

static void truncate_path(LPWSTR file, LPWSTR out, LPWSTR pos1, LPWSTR pos2)
{
    static const WCHAR dots[] = {'.','.','.',0};

    *++pos1 = 0;

    lstrcatW(out, file);
    lstrcatW(out, dots);
    lstrcatW(out, pos2);
}

static void format_filelist_filename(LPWSTR file, LPWSTR out)
{
    LPWSTR pos_basename;
    LPWSTR truncpos1, truncpos2;
    WCHAR myDocs[MAX_STRING_LEN];

    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, (LPWSTR)&myDocs);
    pos_basename = file_basename(file);
    truncpos1 = NULL;
    truncpos2 = NULL;

    *(pos_basename-1) = 0;
    if(!lstrcmpiW(file, myDocs) || (lstrlenW(pos_basename) > FILELIST_ENTRY_LENGTH))
    {
        truncpos1 = pos_basename;
        *(pos_basename-1) = '\\';
    } else
    {
        LPWSTR pos;
        BOOL morespace = FALSE;

        *(pos_basename-1) = '\\';

        for(pos = file; pos < pos_basename; pos++)
        {
            if(*pos == '\\' || *pos == '/')
            {
                if(truncpos1)
                {
                    if((pos - file + lstrlenW(pos_basename)) > FILELIST_ENTRY_LENGTH)
                        break;

                    truncpos1 = pos;
                    morespace = TRUE;
                    break;
                }

                if((pos - file + lstrlenW(pos_basename)) > FILELIST_ENTRY_LENGTH)
                    break;

                truncpos1 = pos;
            }
        }

        if(morespace)
        {
            for(pos = pos_basename; pos >= truncpos1; pos--)
            {
                if(*pos == '\\' || *pos == '/')
                {
                    if((truncpos1 - file + lstrlenW(pos_basename) + pos_basename - pos) > FILELIST_ENTRY_LENGTH)
                        break;

                    truncpos2 = pos;
                }
            }
        }
    }

    if(truncpos1 == pos_basename)
        lstrcatW(out, pos_basename);
    else if(truncpos1 == truncpos2 || !truncpos2)
        lstrcatW(out, file);
    else
        truncate_path(file, out, truncpos1, truncpos2 ? truncpos2 : (pos_basename-1));
}

static void registry_read_filelist(HWND hMainWnd)
{
    HKEY hFileKey;

    if(registry_get_handle(&hFileKey, 0, key_recentfiles) == ERROR_SUCCESS)
    {
        WCHAR itemText[MAX_PATH+3], buffer[MAX_PATH];
        /* The menu item name is not the same as the file name, so we need to store
           the file name here */
        static WCHAR file1[MAX_PATH], file2[MAX_PATH], file3[MAX_PATH], file4[MAX_PATH];
        WCHAR numFormat[] = {'&','%','d',' ',0};
        LPWSTR pFile[] = {file1, file2, file3, file4};
        DWORD pathSize = MAX_PATH*sizeof(WCHAR);
        int i;
        WCHAR key[6];
        MENUITEMINFOW mi;
        HMENU hMenu = GetMenu(hMainWnd);

        mi.cbSize = sizeof(MENUITEMINFOW);
        mi.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;
        mi.fType = MFT_STRING;
        mi.dwTypeData = itemText;
        mi.wID = ID_FILE_RECENT1;

        RemoveMenu(hMenu, ID_FILE_RECENT_SEPARATOR, MF_BYCOMMAND);
        for(i = 0; i < FILELIST_ENTRIES; i++)
        {
            wsprintfW(key, var_file, i+1);
            RemoveMenu(hMenu, ID_FILE_RECENT1+i, MF_BYCOMMAND);
            if(RegQueryValueExW(hFileKey, (LPWSTR)key, 0, NULL, (LPBYTE)pFile[i], &pathSize)
               != ERROR_SUCCESS)
                break;

            mi.dwItemData = (DWORD)pFile[i];
            wsprintfW(itemText, numFormat, i+1);

            lstrcpyW(buffer, pFile[i]);

            format_filelist_filename(buffer, itemText);

            InsertMenuItemW(hMenu, ID_FILE_EXIT, FALSE, &mi);
            mi.wID++;
            pathSize = MAX_PATH*sizeof(WCHAR);
        }
        mi.fType = MFT_SEPARATOR;
        mi.fMask = MIIM_FTYPE | MIIM_ID;
        InsertMenuItemW(hMenu, ID_FILE_EXIT, FALSE, &mi);

        RegCloseKey(hFileKey);
    }
}

static void registry_set_filelist(LPCWSTR newFile)
{
    HKEY hKey;
    DWORD action;

    if(registry_get_handle(&hKey, &action, key_recentfiles) == ERROR_SUCCESS)
    {
        LPCWSTR pFiles[FILELIST_ENTRIES];
        int i;
        HMENU hMenu = GetMenu(hMainWnd);
        MENUITEMINFOW mi;
        WCHAR buffer[6];

        mi.cbSize = sizeof(MENUITEMINFOW);
        mi.fMask = MIIM_DATA;

        for(i = 0; i < FILELIST_ENTRIES; i++)
            pFiles[i] = NULL;

        for(i = 0; GetMenuItemInfoW(hMenu, ID_FILE_RECENT1+i, FALSE, &mi); i++)
            pFiles[i] = (LPWSTR)mi.dwItemData;

        if(lstrcmpiW(newFile, pFiles[0]))
        {
            for(i = 0; pFiles[i] && i < FILELIST_ENTRIES; i++)
            {
                if(!lstrcmpiW(pFiles[i], newFile))
                {
                    int j;
                    for(j = 0; pFiles[j] && j < i; j++)
                    {
                        pFiles[i-j] = pFiles[i-j-1];
                    }
                    pFiles[0] = NULL;
                    break;
                }
            }

            if(!pFiles[0])
            {
                pFiles[0] = newFile;
            } else
            {
                for(i = 0; pFiles[i] && i < FILELIST_ENTRIES-1; i++)
                    pFiles[FILELIST_ENTRIES-1-i] = pFiles[FILELIST_ENTRIES-2-i];

                pFiles[0] = newFile;
            }

            for(i = 0; pFiles[i] && i < FILELIST_ENTRIES; i++)
            {
                wsprintfW(buffer, var_file, i+1);
                RegSetValueExW(hKey, (LPWSTR)&buffer, 0, REG_SZ, (LPBYTE)pFiles[i],
                               (lstrlenW(pFiles[i])+1)*sizeof(WCHAR));
            }
        }
    }
    RegCloseKey(hKey);
    registry_read_filelist(hMainWnd);
}

static BOOL validate_endptr(LPCSTR endptr, BOOL units)
{
    if(!endptr || !*endptr)
        return TRUE;

    while(*endptr == ' ')
        endptr++;

    if(!units)
        return *endptr != '\0';

    /* FIXME: Allow other units and convert between them */
    if(!lstrcmpA(endptr, units_cmA))
        endptr += 2;

    return *endptr != '\0';
}

static BOOL number_from_string(LPCWSTR string, float *num, BOOL units)
{
    double ret;
    char buffer[MAX_STRING_LEN];
    char *endptr = buffer;

    WideCharToMultiByte(CP_ACP, 0, string, -1, buffer, MAX_STRING_LEN, NULL, NULL);
    *num = 0;
    errno = 0;
    ret = strtod(buffer, &endptr);

    if((ret == 0 && errno != 0) || endptr == buffer || validate_endptr(endptr, units))
    {
        return FALSE;
    } else
    {
        *num = (float)ret;
        return TRUE;
    }
}

static void set_size(float size)
{
    CHARFORMAT2W fmt;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_SIZE;
    fmt.yHeight = (int)(size * 20.0);
    SendMessageW(hEditorWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
}

static void add_size(HWND hSizeListWnd, unsigned size)
{
    WCHAR buffer[3];
    COMBOBOXEXITEMW cbItem;
    cbItem.mask = CBEIF_TEXT;
    cbItem.iItem = -1;

    wsprintfW(buffer, stringFormat, size);
    cbItem.pszText = (LPWSTR)buffer;
    SendMessageW(hSizeListWnd, CBEM_INSERTITEMW, 0, (LPARAM)&cbItem);
}

static void populate_size_list(HWND hSizeListWnd)
{
    HWND hReBarWnd = GetDlgItem(hMainWnd, IDC_REBAR);
    HWND hFontListWnd = GetDlgItem(hReBarWnd, IDC_FONTLIST);
    COMBOBOXEXITEMW cbFontItem;
    CHARFORMAT2W fmt;
    HWND hListEditWnd = (HWND)SendMessageW(hSizeListWnd, CBEM_GETEDITCONTROL, 0, 0);
    HDC hdc = GetDC(hMainWnd);
    static const unsigned choices[] = {8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
    WCHAR buffer[3];
    int i;
    DWORD fontStyle;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);

    cbFontItem.mask = CBEIF_LPARAM;
    cbFontItem.iItem = SendMessageW(hFontListWnd, CB_FINDSTRINGEXACT, -1, (LPARAM)fmt.szFaceName);
    SendMessageW(hFontListWnd, CBEM_GETITEMW, 0, (LPARAM)&cbFontItem);

    fontStyle = (DWORD)LOWORD(cbFontItem.lParam);

    SendMessageW(hSizeListWnd, CB_RESETCONTENT, 0, 0);

    if((fontStyle & RASTER_FONTTYPE) && cbFontItem.iItem)
    {
        add_size(hSizeListWnd, (BYTE)MulDiv(HIWORD(cbFontItem.lParam), 72,
                               GetDeviceCaps(hdc, LOGPIXELSY)));
    } else
    {
        for(i = 0; i < sizeof(choices)/sizeof(choices[0]); i++)
            add_size(hSizeListWnd, choices[i]);
    }

    wsprintfW(buffer, stringFormat, fmt.yHeight / 20);
    SendMessageW(hListEditWnd, WM_SETTEXT, 0, (LPARAM)buffer);
}

static void update_size_list(void)
{
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    HWND hwndSizeList = GetDlgItem(hReBar, IDC_SIZELIST);
    HWND hwndSizeListEdit = (HWND)SendMessageW(hwndSizeList, CBEM_GETEDITCONTROL, 0, 0);
    WCHAR fontSize[MAX_STRING_LEN], sizeBuffer[MAX_STRING_LEN];
    CHARFORMAT2W fmt;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);

    SendMessageW(hwndSizeListEdit, WM_GETTEXT, MAX_PATH, (LPARAM)fontSize);
    wsprintfW(sizeBuffer, stringFormat, fmt.yHeight / 20);

    if(lstrcmpW(fontSize, sizeBuffer))
        SendMessageW(hwndSizeListEdit, WM_SETTEXT, 0, (LPARAM)sizeBuffer);
}

static void update_font_list(void)
{
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    HWND hFontList = GetDlgItem(hReBar, IDC_FONTLIST);
    HWND hFontListEdit = (HWND)SendMessageW(hFontList, CBEM_GETEDITCONTROL, 0, 0);
    WCHAR fontName[MAX_STRING_LEN];
    CHARFORMAT2W fmt;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
    SendMessageW(hFontListEdit, WM_GETTEXT, MAX_PATH, (LPARAM)fontName);

    if(lstrcmpW(fontName, fmt.szFaceName))
    {
        SendMessageW(hFontListEdit, WM_SETTEXT, 0, (LPARAM)fmt.szFaceName);
        populate_size_list(GetDlgItem(hReBar, IDC_SIZELIST));
    } else
    {
        update_size_list();
    }
}

static void clear_formatting(void)
{
    PARAFORMAT2 pf;

    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_ALIGNMENT;
    pf.wAlignment = PFA_LEFT;
    SendMessageW(hEditorWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}

static int reg_formatindex(WPARAM format)
{
    return (format & SF_TEXT) ? 1 : 0;
}

static int fileformat_number(WPARAM format)
{
    int number = 0;

    if(format == SF_TEXT)
    {
        number = 1;
    } else if (format == (SF_TEXT | SF_UNICODE))
    {
        number = 2;
    }
    return number;
}

static WPARAM fileformat_flags(int format)
{
    WPARAM flags[] = { SF_RTF , SF_TEXT , SF_TEXT | SF_UNICODE };

    return flags[format];
}

static void set_font(LPCWSTR wszFaceName)
{
    HWND hReBarWnd = GetDlgItem(hMainWnd, IDC_REBAR);
    HWND hSizeListWnd = GetDlgItem(hReBarWnd, IDC_SIZELIST);
    HWND hFontListWnd = GetDlgItem(hReBarWnd, IDC_FONTLIST);
    HWND hFontListEditWnd = (HWND)SendMessageW(hFontListWnd, CBEM_GETEDITCONTROL, 0, 0);
    CHARFORMAT2W fmt;

    ZeroMemory(&fmt, sizeof(fmt));

    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_FACE;

    lstrcpyW(fmt.szFaceName, wszFaceName);

    SendMessageW(hEditorWnd, EM_SETCHARFORMAT,  SCF_SELECTION, (LPARAM)&fmt);

    populate_size_list(hSizeListWnd);

    SendMessageW(hFontListEditWnd, WM_SETTEXT, 0, (LPARAM)(LPWSTR)wszFaceName);
}

static void set_default_font(void)
{
    static const WCHAR richTextFont[] = {'T','i','m','e','s',' ','N','e','w',' ',
                                         'R','o','m','a','n',0};
    static const WCHAR plainTextFont[] = {'C','o','u','r','i','e','r',' ','N','e','w',0};
    CHARFORMAT2W fmt;
    LPCWSTR font;

    ZeroMemory(&fmt, sizeof(fmt));

    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
    fmt.dwEffects = 0;

    if(fileFormat & SF_RTF)
        font = richTextFont;
    else
        font = plainTextFont;

    lstrcpyW(fmt.szFaceName, font);

    SendMessageW(hEditorWnd, EM_SETCHARFORMAT,  SCF_DEFAULT, (LPARAM)&fmt);
}

static void add_font(LPCWSTR fontName, DWORD fontType, HWND hListWnd, NEWTEXTMETRICEXW *ntmc)
{
    COMBOBOXEXITEMW cbItem;
    WCHAR buffer[MAX_PATH];
    int fontHeight = 0;

    cbItem.mask = CBEIF_TEXT;
    cbItem.pszText = buffer;
    cbItem.cchTextMax = MAX_STRING_LEN;
    cbItem.iItem = 0;

    while(SendMessageW(hListWnd, CBEM_GETITEMW, 0, (LPARAM)&cbItem))
    {
        if(lstrcmpiW(cbItem.pszText, fontName) <= 0)
            cbItem.iItem++;
        else
            break;
    }
    cbItem.pszText = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(fontName) + 1)*sizeof(WCHAR) );
    lstrcpyW( cbItem.pszText, fontName );

    cbItem.mask |= CBEIF_LPARAM;
    if(fontType & RASTER_FONTTYPE)
        fontHeight = ntmc->ntmTm.tmHeight - ntmc->ntmTm.tmInternalLeading;

    cbItem.lParam = MAKELONG(fontType,fontHeight);
    SendMessageW(hListWnd, CBEM_INSERTITEMW, 0, (LPARAM)&cbItem);
	HeapFree( GetProcessHeap(), 0, cbItem.pszText );
}

static void dialog_choose_font(void)
{
    CHOOSEFONTW cf;
    LOGFONTW lf;
    CHARFORMAT2W fmt;
    HDC hDC = GetDC(hMainWnd);

    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hMainWnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_NOSCRIPTSEL | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
    lstrcpyW(cf.lpLogFont->lfFaceName, fmt.szFaceName);
    cf.lpLogFont->lfItalic = (fmt.dwEffects & CFE_ITALIC) ? TRUE : FALSE;
    cf.lpLogFont->lfWeight = (fmt.dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
    cf.lpLogFont->lfUnderline = (fmt.dwEffects & CFE_UNDERLINE) ? TRUE : FALSE;
    cf.lpLogFont->lfStrikeOut = (fmt.dwEffects & CFE_STRIKEOUT) ? TRUE : FALSE;
    cf.lpLogFont->lfHeight = -MulDiv(fmt.yHeight / 20, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    cf.rgbColors = fmt.crTextColor;

    if(ChooseFontW(&cf))
    {
        ZeroMemory(&fmt, sizeof(fmt));
        fmt.cbSize = sizeof(fmt);
        fmt.dwMask = CFM_BOLD | CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR;
        fmt.yHeight = cf.iPointSize * 2;

        if(cf.nFontType & BOLD_FONTTYPE)
            fmt.dwEffects |= CFE_BOLD;
        if(cf.nFontType & ITALIC_FONTTYPE)
            fmt.dwEffects |= CFE_ITALIC;
        if(cf.lpLogFont->lfUnderline == TRUE)
            fmt.dwEffects |= CFE_UNDERLINE;
        if(cf.lpLogFont->lfStrikeOut == TRUE)
            fmt.dwEffects |= CFE_STRIKEOUT;

        fmt.crTextColor = cf.rgbColors;

        SendMessageW(hEditorWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        set_font(cf.lpLogFont->lfFaceName);
    }
}


int CALLBACK enum_font_proc(const LOGFONTW *lpelfe, const TEXTMETRICW *lpntme,
                            DWORD FontType, LPARAM lParam)
{
    HWND hListWnd = (HWND) lParam;

    if(SendMessageW(hListWnd, CB_FINDSTRINGEXACT, -1, (LPARAM)lpelfe->lfFaceName) == CB_ERR)
    {

        add_font((LPWSTR)lpelfe->lfFaceName, FontType, hListWnd, (NEWTEXTMETRICEXW*)lpntme);
    }

    return 1;
}

static void populate_font_list(HWND hListWnd)
{
    HDC hdc = GetDC(hMainWnd);
    LOGFONTW fontinfo;
    HWND hListEditWnd = (HWND)SendMessageW(hListWnd, CBEM_GETEDITCONTROL, 0, 0);
    CHARFORMAT2W fmt;

    fontinfo.lfCharSet = DEFAULT_CHARSET;
    *fontinfo.lfFaceName = '\0';
    fontinfo.lfPitchAndFamily = 0;

    EnumFontFamiliesExW(hdc, &fontinfo, enum_font_proc,
                        (LPARAM)hListWnd, 0);

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&fmt);
    SendMessageW(hListEditWnd, WM_SETTEXT, 0, (LPARAM)fmt.szFaceName);
}

static void update_window(void)
{
    RECT rect;

    GetWindowRect(hMainWnd, &rect);

    (void) OnSize(hMainWnd, SIZE_RESTORED, MAKELONG(rect.bottom, rect.right));
}

static DWORD barState[2];
static DWORD wordWrap[2];

static BOOL is_bar_visible(int bandId)
{
    return barState[reg_formatindex(fileFormat)] & (1 << bandId);
}

static void store_bar_state(int bandId, BOOL show)
{
    int formatIndex = reg_formatindex(fileFormat);

    if(show)
        barState[formatIndex] |= (1 << bandId);
    else
        barState[formatIndex] &= ~(1 << bandId);
}

static void set_toolbar_state(int bandId, BOOL show)
{
    HWND hwndReBar = GetDlgItem(hMainWnd, IDC_REBAR);

    SendMessageW(hwndReBar, RB_SHOWBAND, SendMessageW(hwndReBar, RB_IDTOINDEX, bandId, 0), show);

    if(bandId == BANDID_TOOLBAR)
    {
        REBARBANDINFOW rbbinfo;
        int index = SendMessageW(hwndReBar, RB_IDTOINDEX, BANDID_FONTLIST, 0);

        rbbinfo.cbSize = sizeof(rbbinfo);
        rbbinfo.fMask = RBBIM_STYLE;

        SendMessageW(hwndReBar, RB_GETBANDINFO, index, (LPARAM)&rbbinfo);

        if(!show)
            rbbinfo.fStyle &= ~RBBS_BREAK;
        else
            rbbinfo.fStyle |= RBBS_BREAK;

        SendMessageW(hwndReBar, RB_SETBANDINFO, index, (LPARAM)&rbbinfo);
    }

    if(bandId == BANDID_TOOLBAR || bandId == BANDID_FORMATBAR)
        store_bar_state(bandId, show);
}

static void set_statusbar_state(BOOL show)
{
    HWND hStatusWnd = GetDlgItem(hMainWnd, IDC_STATUSBAR);

    ShowWindow(hStatusWnd, show ? SW_SHOW : SW_HIDE);
    store_bar_state(BANDID_STATUSBAR, show);
}

static void set_bar_states(void)
{
    set_toolbar_state(BANDID_TOOLBAR, is_bar_visible(BANDID_TOOLBAR));
    set_toolbar_state(BANDID_FONTLIST, is_bar_visible(BANDID_FORMATBAR));
    set_toolbar_state(BANDID_SIZELIST, is_bar_visible(BANDID_FORMATBAR));
    set_toolbar_state(BANDID_FORMATBAR, is_bar_visible(BANDID_FORMATBAR));
    set_statusbar_state(is_bar_visible(BANDID_STATUSBAR));

    update_window();
}

static HGLOBAL devMode;
static HGLOBAL devNames;

static HDC make_dc(void)
{
    if(devNames && devMode)
    {
        LPDEVNAMES dn = GlobalLock(devNames);
        LPDEVMODEW dm = GlobalLock(devMode);
        HDC ret;

        ret = CreateDCW((LPWSTR)dn + dn->wDriverOffset,
                        (LPWSTR)dn + dn->wDeviceOffset, 0, dm);

        GlobalUnlock(dn);
        GlobalUnlock(dm);

        return ret;
    } else
    {
        return 0;
    }
}

static LONG twips_to_pixels(int twips, int dpi)
{
    float ret = ((float)twips / ((float)567 * 2.54)) * (float)dpi;
    return (LONG)ret;
}

static LONG devunits_to_twips(int units, int dpi)
{
    float ret = ((float)units / (float)dpi) * (float)567 * 2.54;
    return (LONG)ret;
}

static LONG centmm_to_twips(int mm)
{
    return MulDiv(mm, 567, 1000);
}

static LONG twips_to_centmm(int twips)
{
    return MulDiv(twips, 1000, 567);
}

static RECT get_print_rect(HDC hdc)
{
    RECT rc;
    int width, height;

    if(hdc)
    {
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        width = devunits_to_twips(GetDeviceCaps(hdc, PHYSICALWIDTH), dpiX);
        height = devunits_to_twips(GetDeviceCaps(hdc, PHYSICALHEIGHT), dpiY);
    } else
    {
        width = centmm_to_twips(18500);
        height = centmm_to_twips(27000);
    }

    rc.left = margins.left;
    rc.right = width - margins.right;
    rc.top = margins.top;
    rc.bottom = height - margins.bottom;

    return rc;
}

static void target_device(void)
{
    HDC hdc = make_dc();
    int width = 0;
    int index = reg_formatindex(fileFormat);

    if(wordWrap[index] == ID_WORDWRAP_MARGIN)
    {
        RECT rc = get_print_rect(hdc);
        width = rc.right;
    }

    if(!hdc)
    {
        HDC hMaindc = GetDC(hMainWnd);
        hdc = CreateCompatibleDC(hMaindc);
        ReleaseDC(hMainWnd, hMaindc);
    }

    SendMessageW(hEditorWnd, EM_SETTARGETDEVICE, (WPARAM)hdc, width);

    DeleteDC(hdc);
}

static void set_fileformat(WPARAM format)
{
    HICON hIcon;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);
    fileFormat = format;

    if(format & SF_TEXT)
        hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_TXT));
    else
        hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_RTF));

    SendMessageW(hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    set_bar_states();
    set_default_font();
    target_device();
}

static void DoOpenFile(LPCWSTR szOpenFileName)
{
    HANDLE hFile;
    EDITSTREAM es;
    char fileStart[5];
    DWORD readOut;
    WPARAM format = SF_TEXT;

    hFile = CreateFileW(szOpenFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    ReadFile(hFile, fileStart, 5, &readOut, NULL);
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    if(readOut >= 2 && (BYTE)fileStart[0] == 0xff && (BYTE)fileStart[1] == 0xfe)
    {
        format = SF_TEXT | SF_UNICODE;
        SetFilePointer(hFile, 2, NULL, FILE_BEGIN);
    } else if(readOut >= 5)
    {
        static const char header[] = "{\\rtf";
        if(!memcmp(header, fileStart, 5))
            format = SF_RTF;
    }

    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = stream_in;

    clear_formatting();
    set_fileformat(format);
    SendMessageW(hEditorWnd, EM_STREAMIN, format, (LPARAM)&es);

    CloseHandle(hFile);

    SetFocus(hEditorWnd);

    set_caption(szOpenFileName);

    lstrcpyW(wszFileName, szOpenFileName);
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
    registry_set_filelist(szOpenFileName);
    update_font_list();
}

static void DoSaveFile(LPCWSTR wszSaveFileName, WPARAM format)
{
    HANDLE hFile;
    EDITSTREAM stream;
    LRESULT ret;

    hFile = CreateFileW(wszSaveFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
        return;

    if(format == (SF_TEXT | SF_UNICODE))
    {
        static const BYTE unicode[] = {0xff,0xfe};
        DWORD writeOut;
        WriteFile(hFile, &unicode, sizeof(unicode), &writeOut, 0);

        if(writeOut != sizeof(unicode))
            return;
    }

    stream.dwCookie = (DWORD_PTR)hFile;
    stream.pfnCallback = stream_out;

    ret = SendMessageW(hEditorWnd, EM_STREAMOUT, format, (LPARAM)&stream);

    CloseHandle(hFile);

    SetFocus(hEditorWnd);

    if(!ret)
    {
        GETTEXTLENGTHEX gt;
        gt.flags = GTL_DEFAULT;
        gt.codepage = 1200;

        if(SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0))
            return;
    }

    lstrcpyW(wszFileName, wszSaveFileName);
    set_caption(wszFileName);
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
    set_fileformat(format);
}

static void DialogSaveFile(void)
{
    OPENFILENAMEW sfn;

    WCHAR wszFile[MAX_PATH] = {'\0'};
    static const WCHAR wszDefExt[] = {'r','t','f','\0'};

    ZeroMemory(&sfn, sizeof(sfn));

    sfn.lStructSize = sizeof(sfn);
    sfn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    sfn.hwndOwner = hMainWnd;
    sfn.lpstrFilter = wszFilter;
    sfn.lpstrFile = wszFile;
    sfn.nMaxFile = MAX_PATH;
    sfn.lpstrDefExt = wszDefExt;
    sfn.nFilterIndex = fileformat_number(fileFormat)+1;

    while(GetSaveFileNameW(&sfn))
    {
        if(fileformat_flags(sfn.nFilterIndex-1) != SF_RTF)
        {
            if(MessageBoxW(hMainWnd, MAKEINTRESOURCEW(STRING_SAVE_LOSEFORMATTING),
                           wszAppTitle, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
            {
                continue;
            } else
            {
                DoSaveFile(sfn.lpstrFile, fileformat_flags(sfn.nFilterIndex-1));
                break;
            }
        } else
        {
            DoSaveFile(sfn.lpstrFile, fileformat_flags(sfn.nFilterIndex-1));
            break;
        }
    }
}

static BOOL prompt_save_changes(void)
{
    if(!wszFileName[0])
    {
        GETTEXTLENGTHEX gt;
        gt.flags = GTL_NUMCHARS;
        gt.codepage = 1200;
        if(!SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0))
            return TRUE;
    }

    if(!SendMessageW(hEditorWnd, EM_GETMODIFY, 0, 0))
    {
        return TRUE;
    } else
    {
        LPWSTR displayFileName;
        WCHAR *text;
        int ret;

        if(!wszFileName[0])
            displayFileName = wszDefaultFileName;
        else
            displayFileName = file_basename(wszFileName);

        text = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                         (lstrlenW(displayFileName)+lstrlenW(wszSaveChanges))*sizeof(WCHAR));

        if(!text)
            return FALSE;

        wsprintfW(text, wszSaveChanges, displayFileName);

        ret = MessageBoxW(hMainWnd, text, wszAppTitle, MB_YESNOCANCEL | MB_ICONEXCLAMATION);

        HeapFree(GetProcessHeap(), 0, text);

        switch(ret)
        {
            case IDNO:
                return TRUE;

            case IDYES:
                if(wszFileName[0])
                    DoSaveFile(wszFileName, fileFormat);
                else
                    DialogSaveFile();
                return TRUE;

            default:
                return FALSE;
        }
    }
}

static void DialogOpenFile(void)
{
    OPENFILENAMEW ofn;

    WCHAR wszFile[MAX_PATH] = {'\0'};
    static const WCHAR wszDefExt[] = {'r','t','f','\0'};

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = wszFilter;
    ofn.lpstrFile = wszFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = wszDefExt;
    ofn.nFilterIndex = fileformat_number(fileFormat)+1;

    if(GetOpenFileNameW(&ofn))
    {
        if(prompt_save_changes())
            DoOpenFile(ofn.lpstrFile);
    }
}

static LPWSTR dialog_print_to_file(void)
{
    OPENFILENAMEW ofn;
    static WCHAR file[MAX_PATH] = {'O','U','T','P','U','T','.','P','R','N',0};
    static const WCHAR defExt[] = {'P','R','N',0};

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = (LPWSTR)wszPrintFilter;
    ofn.lpstrFile = (LPWSTR)file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = (LPWSTR)defExt;

    if(GetSaveFileNameW(&ofn))
        return (LPWSTR)file;
    else
        return FALSE;
}

static int get_num_pages(FORMATRANGE fr)
{
    int page = 0;

    do
    {
        page++;
        fr.chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE,
                                     (LPARAM)&fr);
    }
    while(fr.chrg.cpMin && fr.chrg.cpMin < fr.chrg.cpMax);

    return page;
}

static void char_from_pagenum(FORMATRANGE *fr, int page)
{
    int i;

    for(i = 1; i <= page; i++)
    {
        if(i == page)
            break;

        fr->chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)fr);
    }
}

static void print(LPPRINTDLGW pd)
{
    FORMATRANGE fr;
    DOCINFOW di;
    int printedPages = 0;

    fr.hdc = pd->hDC;
    fr.hdcTarget = pd->hDC;

    fr.rc = get_print_rect(fr.hdc);
    fr.rcPage.left = 0;
    fr.rcPage.right = fr.rc.right + margins.right;
    fr.rcPage.top = 0;
    fr.rcPage.bottom = fr.rc.bottom + margins.bottom;

    ZeroMemory(&di, sizeof(di));
    di.cbSize = sizeof(di);
    di.lpszDocName = (LPWSTR)wszFileName;

    if(pd->Flags & PD_PRINTTOFILE)
    {
        di.lpszOutput = dialog_print_to_file();
        if(!di.lpszOutput)
            return;
    }

    if(pd->Flags & PD_SELECTION)
    {
        SendMessageW(hEditorWnd, EM_EXGETSEL, 0, (LPARAM)&fr.chrg);
    } else
    {
        GETTEXTLENGTHEX gt;
        gt.flags = GTL_DEFAULT;
        gt.codepage = 1200;
        fr.chrg.cpMin = 0;
        fr.chrg.cpMax = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

        if(pd->Flags & PD_PAGENUMS)
            char_from_pagenum(&fr, pd->nToPage);
    }

    StartDocW(fr.hdc, &di);
    do
    {
        if(StartPage(fr.hdc) <= 0)
            break;

        fr.chrg.cpMin = SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)&fr);

        if(EndPage(fr.hdc) <= 0)
            break;

        printedPages++;
        if((pd->Flags & PD_PAGENUMS) && (printedPages > (pd->nToPage - pd->nFromPage)))
            break;
    }
    while(fr.chrg.cpMin && fr.chrg.cpMin < fr.chrg.cpMax);

    EndDoc(fr.hdc);
    SendMessageW(hEditorWnd, EM_FORMATRANGE, FALSE, 0);
    target_device();
}

static void dialog_printsetup(void)
{
    PAGESETUPDLGW ps;

    ZeroMemory(&ps, sizeof(ps));
    ps.lStructSize = sizeof(ps);
    ps.hwndOwner = hMainWnd;
    ps.Flags = PSD_INHUNDREDTHSOFMILLIMETERS | PSD_MARGINS;
    ps.rtMargin.left = twips_to_centmm(margins.left);
    ps.rtMargin.right = twips_to_centmm(margins.right);
    ps.rtMargin.top = twips_to_centmm(margins.top);
    ps.rtMargin.bottom = twips_to_centmm(margins.bottom);
    ps.hDevMode = devMode;
    ps.hDevNames = devNames;

    if(PageSetupDlgW(&ps))
    {
        margins.left = centmm_to_twips(ps.rtMargin.left);
        margins.right = centmm_to_twips(ps.rtMargin.right);
        margins.top = centmm_to_twips(ps.rtMargin.top);
        margins.bottom = centmm_to_twips(ps.rtMargin.bottom);
        devMode = ps.hDevMode;
        devNames = ps.hDevNames;
        target_device();
    }
}

static void get_default_printer_opts(void)
{
    PRINTDLGW pd;
    ZeroMemory(&pd, sizeof(pd));

    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.Flags = PD_RETURNDC | PD_RETURNDEFAULT;
    pd.hwndOwner = hMainWnd;
    pd.hDevMode = devMode;

    PrintDlgW(&pd);

    devMode = pd.hDevMode;
    devNames = pd.hDevNames;
}

static void print_quick(void)
{
    PRINTDLGW pd;

    ZeroMemory(&pd, sizeof(pd));
    pd.hDC = make_dc();

    print(&pd);
}

static void dialog_print(void)
{
    PRINTDLGW pd;
    int from = 0;
    int to = 0;

    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hMainWnd;
    pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE;
    pd.nMinPage = 1;
    pd.nMaxPage = -1;
    pd.hDevMode = devMode;
    pd.hDevNames = devNames;

    SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    if(from == to)
        pd.Flags |= PD_NOSELECTION;

    if(PrintDlgW(&pd))
    {
        devMode = pd.hDevMode;
        devNames = pd.hDevNames;
        print(&pd);
    }
}

typedef struct _previewinfo
{
    int page;
    int pages;
    HDC hdc;
    HDC hdcSized;
    RECT window;
} previewinfo, *ppreviewinfo;

static previewinfo preview;

static void preview_bar_show(BOOL show)
{
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    int i;

    if(show)
    {
        REBARBANDINFOW rb;

        AddTextButton(hReBar, STRING_PREVIEW_PRINT, ID_PRINT, BANDID_PREVIEW_BTN1);
        AddTextButton(hReBar, STRING_PREVIEW_NEXTPAGE, ID_PREVIEW_NEXTPAGE, BANDID_PREVIEW_BTN2);
        AddTextButton(hReBar, STRING_PREVIEW_PREVPAGE, ID_PREVIEW_PREVPAGE, BANDID_PREVIEW_BTN3);
        AddTextButton(hReBar, STRING_PREVIEW_CLOSE, ID_FILE_EXIT, BANDID_PREVIEW_BTN4);

        rb.cbSize = sizeof(rb);
        rb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_ID;
        rb.fStyle = RBBS_NOGRIPPER | RBBS_VARIABLEHEIGHT;
        rb.cyChild = rb.cyMinChild = 22;
        rb.cx = rb.cxMinChild = 90;
        rb.cxIdeal = 100;
        rb.wID = BANDID_PREVIEW_BUFFER;

        SendMessageW(hReBar, RB_INSERTBAND, -1, (LPARAM)&rb);
    } else
    {
        for(i = 0; i <= PREVIEW_BUTTONS; i++)
            SendMessageW(hReBar, RB_DELETEBAND, SendMessageW(hReBar, RB_IDTOINDEX, BANDID_PREVIEW_BTN1+i, 0), 0);
    }
}

static void preview_exit(void)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);
    HMENU hMenu = LoadMenuW(hInstance, xszMainMenu);

    set_bar_states();
    preview.window.right = 0;
    preview.window.bottom = 0;
    preview.page = 0;
    preview.pages = 0;
    ShowWindow(hEditorWnd, TRUE);

    preview_bar_show(FALSE);

    SetMenu(hMainWnd, hMenu);
    registry_read_filelist(hMainWnd);

    update_window();
}

static LRESULT print_preview(void)
{
    FORMATRANGE fr;
    GETTEXTLENGTHEX gt;
    HDC hdc;
    RECT window, background;
    HBITMAP hBitmapCapture, hBitmapScaled;
    int bmWidth, bmHeight, bmNewWidth, bmNewHeight;
    float ratioWidth, ratioHeight, ratio;
    int xOffset, yOffset;
    int barheight;
    HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
    PAINTSTRUCT ps;

    hdc = BeginPaint(hMainWnd, &ps);
    GetClientRect(hMainWnd, &window);

    fr.hdcTarget = make_dc();
    fr.rc = get_print_rect(fr.hdcTarget);
    fr.rcPage.left = 0;
    fr.rcPage.top = 0;
    fr.rcPage.bottom = fr.rc.bottom + margins.bottom;
    fr.rcPage.right = fr.rc.right + margins.right;

    bmWidth = twips_to_pixels(fr.rcPage.right, GetDeviceCaps(hdc, LOGPIXELSX));
    bmHeight = twips_to_pixels(fr.rcPage.bottom, GetDeviceCaps(hdc, LOGPIXELSY));

    hBitmapCapture = CreateCompatibleBitmap(hdc, bmWidth, bmHeight);

    if(!preview.hdc)
    {
        RECT paper;

        preview.hdc = CreateCompatibleDC(hdc);
        fr.hdc = preview.hdc;
        gt.flags = GTL_DEFAULT;
        gt.codepage = 1200;
        fr.chrg.cpMin = 0;
        fr.chrg.cpMax = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

        paper.left = 0;
        paper.right = bmWidth;
        paper.top = 0;
        paper.bottom = bmHeight;

        if(!preview.pages)
            preview.pages = get_num_pages(fr);

        SelectObject(preview.hdc, hBitmapCapture);

        char_from_pagenum(&fr, preview.page);

        FillRect(preview.hdc, &paper, GetStockObject(WHITE_BRUSH));
        SendMessageW(hEditorWnd, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
        SendMessageW(hEditorWnd, EM_FORMATRANGE, FALSE, 0);

        EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_PREVPAGE), preview.page > 1);
        EnableWindow(GetDlgItem(hReBar, ID_PREVIEW_NEXTPAGE), preview.page < preview.pages);
    }

    barheight = SendMessageW(hReBar, RB_GETBARHEIGHT, 0, 0);
    ratioWidth = ((float)window.right - 20.0) / (float)bmHeight;
    ratioHeight = ((float)window.bottom - 20.0 - (float)barheight) / (float)bmHeight;

    if(ratioWidth > ratioHeight)
        ratio = ratioHeight;
    else
        ratio = ratioWidth;

    bmNewWidth = (int)((float)bmWidth * ratio);
    bmNewHeight = (int)((float)bmHeight * ratio);
    hBitmapScaled = CreateCompatibleBitmap(hdc, bmNewWidth, bmNewHeight);

    xOffset = ((window.right - bmNewWidth) / 2);
    yOffset = ((window.bottom - bmNewHeight + barheight) / 2);

    if(window.right != preview.window.right || window.bottom != preview.window.bottom)
    {
        DeleteDC(preview.hdcSized),
        preview.hdcSized = CreateCompatibleDC(hdc);
        SelectObject(preview.hdcSized, hBitmapScaled);

        StretchBlt(preview.hdcSized, 0, 0, bmNewWidth, bmNewHeight, preview.hdc, 0, 0, bmWidth, bmHeight, SRCCOPY);
    }

    window.top = barheight;
    FillRect(hdc, &window, GetStockObject(GRAY_BRUSH));

    SelectObject(hdc, hBitmapScaled);

    background.left = xOffset - 2;
    background.right = xOffset + bmNewWidth + 2;
    background.top = yOffset - 2;
    background.bottom = yOffset + bmNewHeight + 2;

    FillRect(hdc, &background, GetStockObject(BLACK_BRUSH));

    BitBlt(hdc, xOffset, yOffset, bmNewWidth, bmNewHeight, preview.hdcSized, 0, 0, SRCCOPY);

    DeleteDC(fr.hdcTarget);
    preview.window = window;

    EndPaint(hMainWnd, &ps);

    return 0;
}

static LRESULT preview_command(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
        case ID_FILE_EXIT:
            PostMessageW(hMainWnd, WM_CLOSE, 0, 0);
            break;

        case ID_PREVIEW_NEXTPAGE:
        case ID_PREVIEW_PREVPAGE:
            {
                HWND hReBar = GetDlgItem(hMainWnd, IDC_REBAR);
                RECT rc;

                if(LOWORD(wParam) == ID_PREVIEW_NEXTPAGE)
                    preview.page++;
                else
                    preview.page--;

                preview.hdc = 0;
                preview.window.right = 0;

                GetClientRect(hMainWnd, &rc);
                rc.top += SendMessageW(hReBar, RB_GETBARHEIGHT, 0, 0);
                InvalidateRect(hMainWnd, &rc, TRUE);
            }
            break;

        case ID_PRINT:
            dialog_print();
            preview_exit();
            break;
    }

    return 0;
}


static void dialog_about(void)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);
    HICON icon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WORDPAD));
    ShellAboutW(hMainWnd, wszAppTitle, 0, icon);
}

static INT_PTR CALLBACK formatopts_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                LPPROPSHEETPAGEW ps = (LPPROPSHEETPAGEW)lParam;
                int wrap = -1;
                char id[4];
                HWND hIdWnd = GetDlgItem(hWnd, IDC_PAGEFMT_ID);

                sprintf(id, "%d\n", (int)ps->lParam);
                SetWindowTextA(hIdWnd, id);
                if(wordWrap[ps->lParam] == ID_WORDWRAP_WINDOW)
                    wrap = IDC_PAGEFMT_WW;
                else if(wordWrap[ps->lParam] == ID_WORDWRAP_MARGIN)
                    wrap = IDC_PAGEFMT_WM;

                if(wrap != -1)
                    CheckRadioButton(hWnd, IDC_PAGEFMT_WW,
                                     IDC_PAGEFMT_WM, wrap);

                if(barState[ps->lParam] & (1 << BANDID_TOOLBAR))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_TB, TRUE);
                if(barState[ps->lParam] & (1 << BANDID_FORMATBAR))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_FB, TRUE);
                if(barState[ps->lParam] & (BANDID_STATUSBAR))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_SB, TRUE);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_PAGEFMT_WW:
                case IDC_PAGEFMT_WM:
                    CheckRadioButton(hWnd, IDC_PAGEFMT_WW, IDC_PAGEFMT_WM,
                                     LOWORD(wParam));
                    break;

                case IDC_PAGEFMT_TB:
                case IDC_PAGEFMT_FB:
                case IDC_PAGEFMT_SB:
                    CheckDlgButton(hWnd, LOWORD(wParam),
                                   !IsDlgButtonChecked(hWnd, LOWORD(wParam)));
                    break;
            }
            break;
        case WM_NOTIFY:
            {
                LPNMHDR header = (LPNMHDR)lParam;
                if(header->code == PSN_APPLY)
                {
                    HWND hIdWnd = GetDlgItem(hWnd, IDC_PAGEFMT_ID);
                    char sid[4];
                    int id;

                    GetWindowTextA(hIdWnd, sid, 4);
                    id = atoi(sid);
                    if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_WW))
                        wordWrap[id] = ID_WORDWRAP_WINDOW;
                    else if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_WM))
                        wordWrap[id] = ID_WORDWRAP_MARGIN;

                    if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_TB))
                        barState[id] |= (1 << BANDID_TOOLBAR);
                    else
                        barState[id] &= ~(1 << BANDID_TOOLBAR);

                    if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_FB))
                        barState[id] |= (1 << BANDID_FORMATBAR);
                    else
                        barState[id] &= ~(1 << BANDID_FORMATBAR);

                    if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_SB))
                        barState[id] |= (1 << BANDID_STATUSBAR);
                    else
                        barState[id] &= ~(1 << BANDID_STATUSBAR);
                }
            }
            break;
    }
    return FALSE;
}

static void dialog_viewproperties(void)
{
    PROPSHEETPAGEW psp[2];
    PROPSHEETHEADERW psh;
    int i;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);
    LPCPROPSHEETPAGEW ppsp = (LPCPROPSHEETPAGEW)&psp;

    psp[0].dwSize = sizeof(PROPSHEETPAGEW);
    psp[0].dwFlags = PSP_USETITLE;
    U(psp[0]).pszTemplate = MAKEINTRESOURCEW(IDD_FORMATOPTS);
    psp[0].pfnDlgProc = formatopts_proc;
    psp[0].hInstance = hInstance;
    psp[0].lParam = reg_formatindex(SF_TEXT);
    psp[0].pfnCallback = NULL;
    psp[0].pszTitle = MAKEINTRESOURCEW(STRING_VIEWPROPS_TEXT);
    for(i = 1; i < sizeof(psp)/sizeof(psp[0]); i++)
    {
        psp[i].dwSize = psp[0].dwSize;
        psp[i].dwFlags = psp[0].dwFlags;
        U(psp[i]).pszTemplate = U(psp[0]).pszTemplate;
        psp[i].pfnDlgProc = psp[0].pfnDlgProc;
        psp[i].hInstance = psp[0].hInstance;
        psp[i].lParam = reg_formatindex(SF_RTF);
        psp[i].pfnCallback = psp[0].pfnCallback;
        psp[i].pszTitle = MAKEINTRESOURCEW(STRING_VIEWPROPS_RICHTEXT);
    }

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hMainWnd;
    psh.hInstance = hInstance;
    psh.pszCaption = MAKEINTRESOURCEW(STRING_VIEWPROPS_TITLE);
    psh.nPages = sizeof(psp)/sizeof(psp[0]);
    U3(psh).ppsp = ppsp;
    U(psh).pszIcon = MAKEINTRESOURCEW(IDI_WORDPAD);

    if(fileFormat & SF_RTF)
        U2(psh).nStartPage = 1;
    else
        U2(psh).nStartPage = 0;
    PropertySheetW(&psh);
    set_bar_states();
    target_device();
}

static void HandleCommandLine(LPWSTR cmdline)
{
    WCHAR delimiter;
    int opt_print = 0;

    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    if (*cmdline == delimiter) cmdline++;
    while (*cmdline && *cmdline != delimiter) cmdline++;
    if (*cmdline == delimiter) cmdline++;

    while (*cmdline == ' ' || *cmdline == '-' || *cmdline == '/')
    {
        WCHAR option;

        if (*cmdline++ == ' ') continue;

        option = *cmdline;
        if (option) cmdline++;
        while (*cmdline == ' ') cmdline++;

        switch (option)
        {
            case 'p':
            case 'P':
                opt_print = 1;
                break;
        }
    }

    if (*cmdline)
    {
        /* file name is passed on the command line */
        if (cmdline[0] == '"')
        {
            cmdline++;
            cmdline[lstrlenW(cmdline) - 1] = 0;
        }
        DoOpenFile(cmdline);
        InvalidateRect(hMainWnd, NULL, FALSE);
    }

    if (opt_print)
        MessageBox(hMainWnd, "Printing not implemented", "WordPad", MB_OK);
}

static LRESULT handle_findmsg(LPFINDREPLACEW pFr)
{
    if(pFr->Flags & FR_DIALOGTERM)
    {
        hFindWnd = 0;
        pFr->Flags = FR_FINDNEXT;
        return 0;
    }

    if(pFr->Flags & FR_FINDNEXT || pFr->Flags & FR_REPLACE || pFr->Flags & FR_REPLACEALL)
    {
        DWORD flags = FR_DOWN;
        FINDTEXTW ft;
        static CHARRANGE cr;
        LRESULT end, ret;
        GETTEXTLENGTHEX gt;
        LRESULT length;
        int startPos;
        HMENU hMenu = GetMenu(hMainWnd);
        MENUITEMINFOW mi;

        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_DATA;
        mi.dwItemData = 1;
        SetMenuItemInfoW(hMenu, ID_FIND_NEXT, FALSE, &mi);

        gt.flags = GTL_NUMCHARS;
        gt.codepage = 1200;

        length = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);

        if(pFr->lCustData == -1)
        {
            SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&end);
            cr.cpMin = startPos;
            pFr->lCustData = startPos;
            cr.cpMax = length;
            if(cr.cpMin == length)
                cr.cpMin = 0;
        } else
        {
            startPos = pFr->lCustData;
        }

        if(cr.cpMax > length)
        {
            startPos = 0;
            cr.cpMin = 0;
            cr.cpMax = length;
        }

        ft.chrg = cr;
        ft.lpstrText = pFr->lpstrFindWhat;

        if(pFr->Flags & FR_MATCHCASE)
            flags |= FR_MATCHCASE;
        if(pFr->Flags & FR_WHOLEWORD)
            flags |= FR_WHOLEWORD;

        ret = SendMessageW(hEditorWnd, EM_FINDTEXTW, (WPARAM)flags, (LPARAM)&ft);

        if(ret == -1)
        {
            if(cr.cpMax == length && cr.cpMax != startPos)
            {
                ft.chrg.cpMin = cr.cpMin = 0;
                ft.chrg.cpMax = cr.cpMax = startPos;

                ret = SendMessageW(hEditorWnd, EM_FINDTEXTW, (WPARAM)flags, (LPARAM)&ft);
            }
        }

        if(ret == -1)
        {
            pFr->lCustData = -1;
            MessageBoxW(hMainWnd, MAKEINTRESOURCEW(STRING_SEARCH_FINISHED), wszAppTitle,
                        MB_OK | MB_ICONASTERISK);
        } else
        {
            end = ret + lstrlenW(pFr->lpstrFindWhat);
            cr.cpMin = end;
            SendMessageW(hEditorWnd, EM_SETSEL, (WPARAM)ret, (LPARAM)end);
            SendMessageW(hEditorWnd, EM_SCROLLCARET, 0, 0);

            if(pFr->Flags & FR_REPLACE || pFr->Flags & FR_REPLACEALL)
                SendMessageW(hEditorWnd, EM_REPLACESEL, TRUE, (LPARAM)pFr->lpstrReplaceWith);

            if(pFr->Flags & FR_REPLACEALL)
                handle_findmsg(pFr);
        }
    }

    return 0;
}

static void dialog_find(LPFINDREPLACEW fr, BOOL replace)
{
    static WCHAR findBuffer[MAX_STRING_LEN];

    ZeroMemory(fr, sizeof(FINDREPLACEW));
    fr->lStructSize = sizeof(FINDREPLACEW);
    fr->hwndOwner = hMainWnd;
    fr->Flags = FR_HIDEUPDOWN;
    fr->lpstrFindWhat = findBuffer;
    fr->lCustData = -1;
    fr->wFindWhatLen = MAX_STRING_LEN*sizeof(WCHAR);

    if(replace)
        hFindWnd = ReplaceTextW(fr);
    else
        hFindWnd = FindTextW(fr);
}

static void registry_read_options(void)
{
    HKEY hKey;
    DWORD size = sizeof(RECT);

    if(registry_get_handle(&hKey, 0, key_options) != ERROR_SUCCESS ||
       RegQueryValueExW(hKey, var_pagemargin, 0, NULL, (LPBYTE)&margins,
                        &size) != ERROR_SUCCESS || size != sizeof(RECT))
    {
        margins.top = 1417;
        margins.bottom = 1417;
        margins.left = 1757;
        margins.right = 1757;
    }

    RegCloseKey(hKey);
}

static void registry_read_formatopts(int index, LPCWSTR key)
{
    HKEY hKey;
    DWORD action = 0;
    BOOL fetched = FALSE;
    barState[index] = 0;
    wordWrap[index] = 0;

    if(registry_get_handle(&hKey, &action, key) != ERROR_SUCCESS)
        return;

    if(action == REG_OPENED_EXISTING_KEY)
    {
        DWORD size = sizeof(DWORD);

        if(RegQueryValueExW(hKey, var_barstate0, 0, NULL, (LPBYTE)&barState[index],
                         &size) == ERROR_SUCCESS)
            fetched = TRUE;
    }

    if(!fetched)
        barState[index] = (1 << BANDID_TOOLBAR) | (1 << BANDID_FORMATBAR) | (1 << BANDID_RULER) | (1 << BANDID_STATUSBAR);

    if(index == reg_formatindex(SF_RTF))
        wordWrap[index] = ID_WORDWRAP_WINDOW;
    else if(index == reg_formatindex(SF_TEXT))
        wordWrap[index] = ID_WORDWRAP_WINDOW; /* FIXME: should be ID_WORDWRAP_NONE once we support it */

    RegCloseKey(hKey);
}

static void registry_read_formatopts_all(void)
{
    registry_read_formatopts(reg_formatindex(SF_RTF), key_rtf);
    registry_read_formatopts(reg_formatindex(SF_TEXT), key_text);
}

static void registry_set_formatopts(int index, LPCWSTR key)
{
    HKEY hKey;
    DWORD action = 0;

    if(registry_get_handle(&hKey, &action, key) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, var_barstate0, 0, REG_DWORD, (LPBYTE)&barState[index],
                       sizeof(DWORD));

        RegCloseKey(hKey);
    }
}

static void registry_set_formatopts_all(void)
{
    registry_set_formatopts(reg_formatindex(SF_RTF), key_rtf);
    registry_set_formatopts(reg_formatindex(SF_TEXT), key_text);
}

static int current_units_to_twips(float number)
{
    int twips = (int)(number * 567);
    return twips;
}

static void append_current_units(LPWSTR buffer)
{
    static const WCHAR space[] = {' '};
    lstrcatW(buffer, space);
    lstrcatW(buffer, units_cmW);
}

static void number_with_units(LPWSTR buffer, int number)
{
    float converted = (float)number / 567;
    char string[MAX_STRING_LEN];

    sprintf(string, "%.2f ", converted);
    lstrcatA(string, units_cmA);
    MultiByteToWideChar(CP_ACP, 0, string, -1, buffer, MAX_STRING_LEN);
}

BOOL CALLBACK datetime_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                WCHAR buffer[MAX_STRING_LEN];
                SYSTEMTIME st;
                HWND hListWnd = GetDlgItem(hWnd, IDC_DATETIME);
                GetLocalTime(&st);

                GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, 0, (LPWSTR)&buffer,
                               MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, 0, (LPWSTR)&buffer,
                               MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, 0, (LPWSTR)&buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);

                SendMessageW(hListWnd, LB_SETSEL, TRUE, 0);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    {
                        LRESULT index;
                        HWND hListWnd = GetDlgItem(hWnd, IDC_DATETIME);

                        index = SendMessageW(hListWnd, LB_GETCURSEL, 0, 0);

                        if(index != LB_ERR)
                        {
                            WCHAR buffer[MAX_STRING_LEN];
                            SendMessageW(hListWnd, LB_GETTEXT, index, (LPARAM)&buffer);
                            SendMessageW(hEditorWnd, EM_REPLACESEL, TRUE, (LPARAM)&buffer);
                        }
                    }
                    /* Fall through */

                case IDCANCEL:
                    EndDialog(hWnd, wParam);
                    return TRUE;
            }
    }
    return FALSE;
}

BOOL CALLBACK newfile_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd, GWLP_HINSTANCE);
                WCHAR buffer[MAX_STRING_LEN];
                HWND hListWnd = GetDlgItem(hWnd, IDC_NEWFILE);

                LoadStringW(hInstance, STRING_NEWFILE_RICHTEXT, (LPWSTR)buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                LoadStringW(hInstance, STRING_NEWFILE_TXT, (LPWSTR)buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                LoadStringW(hInstance, STRING_NEWFILE_TXT_UNICODE, (LPWSTR)buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);

                SendMessageW(hListWnd, LB_SETSEL, TRUE, 0);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    {
                        LRESULT index;
                        HWND hListWnd = GetDlgItem(hWnd, IDC_NEWFILE);
                        index = SendMessageW(hListWnd, LB_GETCURSEL, 0, 0);

                        if(index != LB_ERR)
                            EndDialog(hWnd, MAKELONG(fileformat_flags(index),0));
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hWnd, MAKELONG(ID_NEWFILE_ABORT,0));
                    return TRUE;
            }
    }
    return FALSE;
}

static INT_PTR CALLBACK paraformat_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hMainWnd,
                                                                  GWLP_HINSTANCE);
                WCHAR buffer[MAX_STRING_LEN];
                HWND hListWnd = GetDlgItem(hWnd, IDC_PARA_ALIGN);
                HWND hLeftWnd = GetDlgItem(hWnd, IDC_PARA_LEFT);
                HWND hRightWnd = GetDlgItem(hWnd, IDC_PARA_RIGHT);
                HWND hFirstWnd = GetDlgItem(hWnd, IDC_PARA_FIRST);
                PARAFORMAT2 pf;
                int index = 0;

                LoadStringW(hInstance, STRING_ALIGN_LEFT, buffer,
                            MAX_STRING_LEN);
                SendMessageW(hListWnd, CB_ADDSTRING, 0, (LPARAM)buffer);
                LoadStringW(hInstance, STRING_ALIGN_RIGHT, buffer,
                            MAX_STRING_LEN);
                SendMessageW(hListWnd, CB_ADDSTRING, 0, (LPARAM)buffer);
                LoadStringW(hInstance, STRING_ALIGN_CENTER, buffer,
                            MAX_STRING_LEN);
                SendMessageW(hListWnd, CB_ADDSTRING, 0, (LPARAM)buffer);

                pf.cbSize = sizeof(pf);
                pf.dwMask = PFM_ALIGNMENT | PFM_OFFSET | PFM_RIGHTINDENT |
                            PFM_OFFSETINDENT;
                SendMessageW(hEditorWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);

                if(pf.wAlignment == PFA_RIGHT)
                    index ++;
                else if(pf.wAlignment == PFA_CENTER)
                    index += 2;

                SendMessageW(hListWnd, CB_SETCURSEL, index, 0);

                number_with_units(buffer, pf.dxOffset);
                SetWindowTextW(hLeftWnd, buffer);
                number_with_units(buffer, pf.dxRightIndent);
                SetWindowTextW(hRightWnd, buffer);
                number_with_units(buffer, pf.dxStartIndent - pf.dxOffset);
                SetWindowTextW(hFirstWnd, buffer);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    {
                        HWND hLeftWnd = GetDlgItem(hWnd, IDC_PARA_LEFT);
                        HWND hRightWnd = GetDlgItem(hWnd, IDC_PARA_RIGHT);
                        HWND hFirstWnd = GetDlgItem(hWnd, IDC_PARA_FIRST);
                        WCHAR buffer[MAX_STRING_LEN];
                        float num;
                        int ret = 0;
                        PARAFORMAT pf;

                        GetWindowTextW(hLeftWnd, buffer, MAX_STRING_LEN);
                        if(number_from_string(buffer, &num, TRUE))
                            ret++;
                        pf.dxOffset = current_units_to_twips(num);
                        GetWindowTextW(hRightWnd, buffer, MAX_STRING_LEN);
                        if(number_from_string(buffer, &num, TRUE))
                            ret++;
                        pf.dxRightIndent = current_units_to_twips(num);
                        GetWindowTextW(hFirstWnd, buffer, MAX_STRING_LEN);
                        if(number_from_string(buffer, &num, TRUE))
                            ret++;
                        pf.dxStartIndent = current_units_to_twips(num);

                        if(ret != 3)
                        {
                            MessageBoxW(hMainWnd, MAKEINTRESOURCEW(STRING_INVALID_NUMBER),
                                        wszAppTitle, MB_OK | MB_ICONASTERISK);
                            return FALSE;
                        } else
                        {
                            pf.dxStartIndent = pf.dxStartIndent + pf.dxOffset;
                            pf.cbSize = sizeof(pf);
                            pf.dwMask = PFM_OFFSET | PFM_OFFSETINDENT | PFM_RIGHTINDENT;
                            SendMessageW(hEditorWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
                        }
                    }
                    /* Fall through */

                case IDCANCEL:
                    EndDialog(hWnd, wParam);
                    return TRUE;
            }
    }
    return FALSE;
}

static INT_PTR CALLBACK tabstops_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                HWND hTabWnd = GetDlgItem(hWnd, IDC_TABSTOPS);
                PARAFORMAT pf;
                WCHAR buffer[MAX_STRING_LEN];
                int i;

                pf.cbSize = sizeof(pf);
                pf.dwMask = PFM_TABSTOPS;
                SendMessageW(hEditorWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
                SendMessageW(hTabWnd, CB_LIMITTEXT, MAX_STRING_LEN-1, 0);

                for(i = 0; i < pf.cTabCount; i++)
                {
                    number_with_units(buffer, pf.rgxTabs[i]);
                    SendMessageW(hTabWnd, CB_ADDSTRING, 0, (LPARAM)&buffer);
                }
                SetFocus(hTabWnd);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_TABSTOPS:
                    {
                        HWND hTabWnd = (HWND)lParam;
                        HWND hAddWnd = GetDlgItem(hWnd, ID_TAB_ADD);
                        HWND hDelWnd = GetDlgItem(hWnd, ID_TAB_DEL);
                        HWND hEmptyWnd = GetDlgItem(hWnd, ID_TAB_EMPTY);

                        if(GetWindowTextLengthW(hTabWnd))
                            EnableWindow(hAddWnd, TRUE);
                        else
                            EnableWindow(hAddWnd, FALSE);

                        if(SendMessageW(hTabWnd, CB_GETCOUNT, 0, 0))
                        {
                            EnableWindow(hEmptyWnd, TRUE);

                            if(SendMessageW(hTabWnd, CB_GETCURSEL, 0, 0) == CB_ERR)
                                EnableWindow(hDelWnd, FALSE);
                            else
                                EnableWindow(hDelWnd, TRUE);
                        } else
                        {
                            EnableWindow(hEmptyWnd, FALSE);
                        }
                    }
                    break;

                case ID_TAB_ADD:
                    {
                        HWND hTabWnd = GetDlgItem(hWnd, IDC_TABSTOPS);
                        WCHAR buffer[MAX_STRING_LEN];

                        GetWindowTextW(hTabWnd, buffer, MAX_STRING_LEN);
                        append_current_units(buffer);

                        if(SendMessageW(hTabWnd, CB_FINDSTRINGEXACT, -1, (LPARAM)&buffer) == CB_ERR)
                        {
                            float number = 0;

                            if(!number_from_string(buffer, &number, TRUE))
                            {
                                MessageBoxW(hWnd, MAKEINTRESOURCEW(STRING_INVALID_NUMBER),
                                             wszAppTitle, MB_OK | MB_ICONINFORMATION);
                            } else
                            {
                                SendMessageW(hTabWnd, CB_ADDSTRING, 0, (LPARAM)&buffer);
                                SetWindowTextW(hTabWnd, 0);
                            }
                        }
                        SetFocus(hTabWnd);
                    }
                    break;

                case ID_TAB_DEL:
                    {
                        HWND hTabWnd = GetDlgItem(hWnd, IDC_TABSTOPS);
                        LRESULT ret;
                        ret = SendMessageW(hTabWnd, CB_GETCURSEL, 0, 0);
                        if(ret != CB_ERR)
                            SendMessageW(hTabWnd, CB_DELETESTRING, ret, 0);
                    }
                    break;

                case ID_TAB_EMPTY:
                    {
                        HWND hTabWnd = GetDlgItem(hWnd, IDC_TABSTOPS);
                        SendMessageW(hTabWnd, CB_RESETCONTENT, 0, 0);
                        SetFocus(hTabWnd);
                    }
                    break;

                case IDOK:
                    {
                        HWND hTabWnd = GetDlgItem(hWnd, IDC_TABSTOPS);
                        int i;
                        WCHAR buffer[MAX_STRING_LEN];
                        PARAFORMAT pf;
                        float number;

                        pf.cbSize = sizeof(pf);
                        pf.dwMask = PFM_TABSTOPS;

                        for(i = 0; SendMessageW(hTabWnd, CB_GETLBTEXT, i,
                                                (LPARAM)&buffer) != CB_ERR &&
                                                        i < MAX_TAB_STOPS; i++)
                        {
                            number_from_string(buffer, &number, TRUE);
                            pf.rgxTabs[i] = current_units_to_twips(number);
                        }
                        pf.cTabCount = i;
                        SendMessageW(hEditorWnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
                    }
                    /* Fall through */
                case IDCANCEL:
                    EndDialog(hWnd, wParam);
                    return TRUE;
            }
    }
    return FALSE;
}

static int context_menu(LPARAM lParam)
{
    int x = (int)(short)LOWORD(lParam);
    int y = (int)(short)HIWORD(lParam);
    HMENU hPop = GetSubMenu(hPopupMenu, 0);

    if(x == -1)
    {
        int from = 0, to = 0;
        POINTL pt;
        SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
        SendMessageW(hEditorWnd, EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)to);
        ClientToScreen(hEditorWnd, (POINT*)&pt);
        x = pt.x;
        y = pt.y;
    }

    TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                   x, y, 0, hMainWnd, 0);

    return 0;
}

static LRESULT OnCreate( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hToolBarWnd, hFormatBarWnd,  hReBarWnd, hFontListWnd, hSizeListWnd;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    HANDLE hDLL;
    TBADDBITMAP ab;
    int nStdBitmaps = 0;
    REBARINFO rbi;
    REBARBANDINFOW rbb;
    static const WCHAR wszRichEditDll[] = {'R','I','C','H','E','D','2','0','.','D','L','L','\0'};
    static const WCHAR wszRichEditText[] = {'R','i','c','h','E','d','i','t',' ','t','e','x','t','\0'};

    CreateStatusWindowW(CCS_NODIVIDER|WS_CHILD|WS_VISIBLE, wszRichEditText, hWnd, IDC_STATUSBAR);

    hReBarWnd = CreateWindowExW(WS_EX_TOOLWINDOW, REBARCLASSNAMEW, NULL,
      CCS_NODIVIDER|WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_TOP,
      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hWnd, (HMENU)IDC_REBAR, hInstance, NULL);

    rbi.cbSize = sizeof(rbi);
    rbi.fMask = 0;
    rbi.himl = NULL;
    if(!SendMessageW(hReBarWnd, RB_SETBARINFO, 0, (LPARAM)&rbi))
        return -1;

    hToolBarWnd = CreateToolbarEx(hReBarWnd, CCS_NOPARENTALIGN|CCS_NOMOVEY|WS_VISIBLE|WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_BUTTON,
      IDC_TOOLBAR,
      1, hInstance, IDB_TOOLBAR,
      NULL, 0,
      24, 24, 16, 16, sizeof(TBBUTTON));

    ab.hInst = HINST_COMMCTRL;
    ab.nID = IDB_STD_SMALL_COLOR;
    nStdBitmaps = SendMessageW(hToolBarWnd, TB_ADDBITMAP, 0, (LPARAM)&ab);

    AddButton(hToolBarWnd, nStdBitmaps+STD_FILENEW, ID_FILE_NEW);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILEOPEN, ID_FILE_OPEN);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILESAVE, ID_FILE_SAVE);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_PRINT, ID_PRINT_QUICK);
    AddButton(hToolBarWnd, nStdBitmaps+STD_PRINTPRE, ID_PREVIEW);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FIND, ID_FIND);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_CUT, ID_EDIT_CUT);
    AddButton(hToolBarWnd, nStdBitmaps+STD_COPY, ID_EDIT_COPY);
    AddButton(hToolBarWnd, nStdBitmaps+STD_PASTE, ID_EDIT_PASTE);
    AddButton(hToolBarWnd, nStdBitmaps+STD_UNDO, ID_EDIT_UNDO);
    AddButton(hToolBarWnd, nStdBitmaps+STD_REDOW, ID_EDIT_REDO);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, 0, ID_DATETIME);

    SendMessageW(hToolBarWnd, TB_AUTOSIZE, 0, 0);

    rbb.cbSize = sizeof(rbb);
    rbb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_STYLE | RBBIM_ID;
    rbb.fStyle = RBBS_CHILDEDGE | RBBS_BREAK | RBBS_NOGRIPPER;
    rbb.cx = 0;
    rbb.hwndChild = hToolBarWnd;
    rbb.cxMinChild = 0;
    rbb.cyChild = rbb.cyMinChild = HIWORD(SendMessageW(hToolBarWnd, TB_GETBUTTONSIZE, 0, 0));
    rbb.wID = BANDID_TOOLBAR;

    SendMessageW(hReBarWnd, RB_INSERTBAND, -1, (LPARAM)&rbb);

    hFontListWnd = CreateWindowExW(0, WC_COMBOBOXEXW, NULL,
                      WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_SORT,
                      0, 0, 200, 150, hReBarWnd, (HMENU)IDC_FONTLIST, hInstance, NULL);

    rbb.hwndChild = hFontListWnd;
    rbb.cx = 200;
    rbb.wID = BANDID_FONTLIST;

    SendMessageW(hReBarWnd, RB_INSERTBAND, -1, (LPARAM)&rbb);

    hSizeListWnd = CreateWindowExW(0, WC_COMBOBOXEXW, NULL,
                      WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN,
                      0, 0, 50, 150, hReBarWnd, (HMENU)IDC_SIZELIST, hInstance, NULL);

    rbb.hwndChild = hSizeListWnd;
    rbb.cx = 50;
    rbb.fStyle ^= RBBS_BREAK;
    rbb.wID = BANDID_SIZELIST;

    SendMessageW(hReBarWnd, RB_INSERTBAND, -1, (LPARAM)&rbb);

    hFormatBarWnd = CreateToolbarEx(hReBarWnd,
         CCS_NOPARENTALIGN | CCS_NOMOVEY | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_BUTTON,
         IDC_FORMATBAR, 7, hInstance, IDB_FORMATBAR, NULL, 0, 16, 16, 16, 16, sizeof(TBBUTTON));

    AddButton(hFormatBarWnd, 0, ID_FORMAT_BOLD);
    AddButton(hFormatBarWnd, 1, ID_FORMAT_ITALIC);
    AddButton(hFormatBarWnd, 2, ID_FORMAT_UNDERLINE);
    AddSeparator(hFormatBarWnd);
    AddButton(hFormatBarWnd, 3, ID_ALIGN_LEFT);
    AddButton(hFormatBarWnd, 4, ID_ALIGN_CENTER);
    AddButton(hFormatBarWnd, 5, ID_ALIGN_RIGHT);
    AddSeparator(hFormatBarWnd);
    AddButton(hFormatBarWnd, 6, ID_BULLET);

    SendMessageW(hFormatBarWnd, TB_AUTOSIZE, 0, 0);

    rbb.hwndChild = hFormatBarWnd;
    rbb.wID = BANDID_FORMATBAR;

    SendMessageW(hReBarWnd, RB_INSERTBAND, -1, (LPARAM)&rbb);

    hDLL = LoadLibraryW(wszRichEditDll);
    if(!hDLL)
    {
        MessageBoxW(hWnd, MAKEINTRESOURCEW(STRING_LOAD_RICHED_FAILED), wszAppTitle,
                    MB_OK | MB_ICONEXCLAMATION);
        PostQuitMessage(1);
    }

    hEditorWnd = CreateWindowExW(WS_EX_CLIENTEDGE, wszRichEditClass, NULL,
      WS_CHILD|WS_VISIBLE|ECO_SELECTIONBAR|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,
      0, 0, 1000, 100, hWnd, (HMENU)IDC_EDITOR, hInstance, NULL);

    if (!hEditorWnd)
    {
        fprintf(stderr, "Error code %u\n", GetLastError());
        return -1;
    }
    assert(hEditorWnd);

    SetFocus(hEditorWnd);
    SendMessageW(hEditorWnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);

    set_default_font();

    populate_font_list(hFontListWnd);
    populate_size_list(hSizeListWnd);
    DoLoadStrings();
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);

    ID_FINDMSGSTRING = RegisterWindowMessageW(FINDMSGSTRINGW);

    registry_read_filelist(hWnd);
    registry_read_formatopts_all();
    registry_read_options();
    DragAcceptFiles(hWnd, TRUE);

    return 0;
}

static LRESULT OnUser( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    HWND hwndToolBar = GetDlgItem(hwndReBar, IDC_TOOLBAR);
    HWND hwndFormatBar = GetDlgItem(hwndReBar, IDC_FORMATBAR);
    int from, to;
    CHARFORMAT2W fmt;
    PARAFORMAT2 pf;
    GETTEXTLENGTHEX gt;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);

    gt.flags = GTL_NUMCHARS;
    gt.codepage = 1200;

    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_FIND,
                 SendMessageW(hwndEditor, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0) ? 1 : 0);

    SendMessageW(hwndEditor, EM_GETCHARFORMAT, TRUE, (LPARAM)&fmt);

    SendMessageW(hwndEditor, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_UNDO,
      SendMessageW(hwndEditor, EM_CANUNDO, 0, 0));
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_REDO,
      SendMessageW(hwndEditor, EM_CANREDO, 0, 0));
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_CUT, from == to ? 0 : 1);
    SendMessageW(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_COPY, from == to ? 0 : 1);

    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_FORMAT_BOLD, (fmt.dwMask & CFM_BOLD) &&
            (fmt.dwEffects & CFE_BOLD));
    SendMessageW(hwndFormatBar, TB_INDETERMINATE, ID_FORMAT_BOLD, !(fmt.dwMask & CFM_BOLD));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_FORMAT_ITALIC, (fmt.dwMask & CFM_ITALIC) &&
            (fmt.dwEffects & CFE_ITALIC));
    SendMessageW(hwndFormatBar, TB_INDETERMINATE, ID_FORMAT_ITALIC, !(fmt.dwMask & CFM_ITALIC));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_FORMAT_UNDERLINE, (fmt.dwMask & CFM_UNDERLINE) &&
            (fmt.dwEffects & CFE_UNDERLINE));
    SendMessageW(hwndFormatBar, TB_INDETERMINATE, ID_FORMAT_UNDERLINE, !(fmt.dwMask & CFM_UNDERLINE));

    SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_ALIGN_LEFT, (pf.wAlignment == PFA_LEFT));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_ALIGN_CENTER, (pf.wAlignment == PFA_CENTER));
    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_ALIGN_RIGHT, (pf.wAlignment == PFA_RIGHT));

    SendMessageW(hwndFormatBar, TB_CHECKBUTTON, ID_BULLET, (pf.wNumbering & PFN_BULLET));
    return 0;
}

static LRESULT OnNotify( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    NMHDR *pHdr = (NMHDR *)lParam;
    HWND hwndFontList = GetDlgItem(hwndReBar, IDC_FONTLIST);
    HWND hwndSizeList = GetDlgItem(hwndReBar, IDC_SIZELIST);
    WCHAR sizeBuffer[MAX_PATH];

    if (pHdr->hwndFrom == hwndFontList || pHdr->hwndFrom == hwndSizeList)
    {
        if (pHdr->code == CBEN_ENDEDITW)
        {
            CHARFORMAT2W format;
            NMCBEENDEDIT *endEdit = (NMCBEENDEDIT *)lParam;

            ZeroMemory(&format, sizeof(format));
            format.cbSize = sizeof(format);
            SendMessageW(hwndEditor, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

            if(pHdr->hwndFrom == hwndFontList)
            {
                if(lstrcmpW(format.szFaceName, (LPWSTR)endEdit->szText))
                    set_font((LPCWSTR) endEdit->szText);
            } else if (pHdr->hwndFrom == hwndSizeList)
            {
                wsprintfW(sizeBuffer, stringFormat, format.yHeight / 20);
                if(lstrcmpW(sizeBuffer, (LPWSTR)endEdit->szText))
                {
                    float size = 0;
                    if(number_from_string((LPWSTR)endEdit->szText, &size, FALSE))
                    {
                        set_size(size);
                    } else
                    {
                        SetWindowTextW(hwndSizeList, sizeBuffer);
                        MessageBoxW(hMainWnd, MAKEINTRESOURCEW(STRING_INVALID_NUMBER), wszAppTitle, MB_OK | MB_ICONINFORMATION);
                    }
                }
            }
        }
        return 0;
    }

    if (pHdr->hwndFrom != hwndEditor)
        return 0;

    if (pHdr->code == EN_SELCHANGE)
    {
        SELCHANGE *pSC = (SELCHANGE *)lParam;
        char buf[128];

        update_font_list();

        sprintf( buf,"selection = %d..%d, line count=%ld",
                 pSC->chrg.cpMin, pSC->chrg.cpMax,
        SendMessage(hwndEditor, EM_GETLINECOUNT, 0, 0));
        SetWindowText(GetDlgItem(hWnd, IDC_STATUSBAR), buf);
        SendMessage(hWnd, WM_USER, 0, 0);
        return 1;
    }
    return 0;
}

static LRESULT OnCommand( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    static FINDREPLACEW findreplace;

    if ((HWND)lParam == hwndEditor)
        return 0;

    switch(LOWORD(wParam))
    {

    case ID_FILE_EXIT:
        PostMessageW(hWnd, WM_CLOSE, 0, 0);
        break;

    case ID_FILE_NEW:
        {
            HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            int ret = DialogBox(hInstance, MAKEINTRESOURCE(IDD_NEWFILE), hWnd,
                                (DLGPROC)newfile_proc);

            if(ret != ID_NEWFILE_ABORT)
            {
                if(prompt_save_changes())
                {
                    SETTEXTEX st;

                    set_caption(NULL);
                    wszFileName[0] = '\0';

                    clear_formatting();

                    st.flags = ST_DEFAULT;
                    st.codepage = 1200;
                    SendMessageW(hEditorWnd, EM_SETTEXTEX, (WPARAM)&st, 0);

                    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
                    set_fileformat(ret);
                    update_font_list();
                }
            }
        }
        break;

    case ID_FILE_OPEN:
        DialogOpenFile();
        break;

    case ID_FILE_SAVE:
        if(wszFileName[0])
        {
            DoSaveFile(wszFileName, fileFormat);
            break;
        }
        /* Fall through */

    case ID_FILE_SAVEAS:
        DialogSaveFile();
        break;

    case ID_FILE_RECENT1:
    case ID_FILE_RECENT2:
    case ID_FILE_RECENT3:
    case ID_FILE_RECENT4:
        {
            HMENU hMenu = GetMenu(hWnd);
            MENUITEMINFOW mi;

            mi.cbSize = sizeof(MENUITEMINFOW);
            mi.fMask = MIIM_DATA;
            if(GetMenuItemInfoW(hMenu, LOWORD(wParam), FALSE, &mi))
                DoOpenFile((LPWSTR)mi.dwItemData);
        }
        break;

    case ID_FIND:
        dialog_find(&findreplace, FALSE);
        break;

    case ID_FIND_NEXT:
        handle_findmsg(&findreplace);
        break;

    case ID_REPLACE:
        dialog_find(&findreplace, TRUE);
        break;

    case ID_FONTSETTINGS:
        dialog_choose_font();
        break;

    case ID_PRINT:
        dialog_print();
        break;

    case ID_PRINT_QUICK:
        print_quick();
        break;

    case ID_PREVIEW:
        {
            int index = reg_formatindex(fileFormat);
            DWORD tmp = barState[index];
            barState[index] = 0;
            set_bar_states();
            barState[index] = tmp;
            ShowWindow(hEditorWnd, FALSE);
            preview_bar_show(TRUE);

            preview.page = 1;
            preview.hdc = 0;
            SetMenu(hWnd, NULL);
            InvalidateRect(0, 0, TRUE);
        }
        break;

    case ID_PRINTSETUP:
        dialog_printsetup();
        break;

    case ID_FORMAT_BOLD:
    case ID_FORMAT_ITALIC:
    case ID_FORMAT_UNDERLINE:
        {
        CHARFORMAT2W fmt;
        int effects = CFE_BOLD;

        ZeroMemory(&fmt, sizeof(fmt));
        fmt.cbSize = sizeof(fmt);
        SendMessageW(hwndEditor, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);

        fmt.dwMask = CFM_BOLD;

        if (LOWORD(wParam) == ID_FORMAT_ITALIC)
        {
            effects = CFE_ITALIC;
            fmt.dwMask = CFM_ITALIC;
        } else if (LOWORD(wParam) == ID_FORMAT_UNDERLINE)
        {
            effects = CFE_UNDERLINE;
            fmt.dwMask = CFM_UNDERLINE;
        }

        fmt.dwEffects ^= effects;

        SendMessageW(hwndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        break;
        }

    case ID_EDIT_CUT:
        PostMessageW(hwndEditor, WM_CUT, 0, 0);
        break;

    case ID_EDIT_COPY:
        PostMessageW(hwndEditor, WM_COPY, 0, 0);
        break;

    case ID_EDIT_PASTE:
        PostMessageW(hwndEditor, WM_PASTE, 0, 0);
        break;

    case ID_EDIT_CLEAR:
        PostMessageW(hwndEditor, WM_CLEAR, 0, 0);
        break;

    case ID_EDIT_SELECTALL:
        {
        CHARRANGE range = {0, -1};
        SendMessageW(hwndEditor, EM_EXSETSEL, 0, (LPARAM)&range);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_GETTEXT:
        {
        int nLen = GetWindowTextLengthW(hwndEditor);
        LPWSTR data = HeapAlloc( GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR) );
        TEXTRANGEW tr;

        GetWindowTextW(hwndEditor, data, nLen+1);
        MessageBoxW(NULL, data, xszAppTitle, MB_OK);

        HeapFree( GetProcessHeap(), 0, data);
        data = HeapAlloc(GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR));
        tr.chrg.cpMin = 0;
        tr.chrg.cpMax = nLen;
        tr.lpstrText = data;
        SendMessage (hwndEditor, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
        MessageBoxW(NULL, data, xszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data );

        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_CHARFORMAT:
    case ID_EDIT_DEFCHARFORMAT:
        {
        CHARFORMAT2W cf;
        LRESULT i;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = 0;
        i = SendMessageW(hwndEditor, EM_GETCHARFORMAT,
                        LOWORD(wParam) == ID_EDIT_CHARFORMAT, (LPARAM)&cf);
        return 0;
        }

    case ID_EDIT_PARAFORMAT:
        {
        PARAFORMAT2 pf;
        ZeroMemory(&pf, sizeof(pf));
        pf.cbSize = sizeof(pf);
        SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
        return 0;
        }

    case ID_EDIT_SELECTIONINFO:
        {
        CHARRANGE range = {0, -1};
        char buf[128];
        WCHAR *data = NULL;

        SendMessage(hwndEditor, EM_EXGETSEL, 0, (LPARAM)&range);
        data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) * (range.cpMax-range.cpMin+1));
        SendMessage(hwndEditor, EM_GETSELTEXT, 0, (LPARAM)data);
        sprintf(buf, "Start = %d, End = %d", range.cpMin, range.cpMax);
        MessageBoxA(hWnd, buf, "Editor", MB_OK);
        MessageBoxW(hWnd, data, xszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_READONLY:
        {
        long nStyle = GetWindowLong(hwndEditor, GWL_STYLE);
        if (nStyle & ES_READONLY)
            SendMessageW(hwndEditor, EM_SETREADONLY, 0, 0);
        else
            SendMessageW(hwndEditor, EM_SETREADONLY, 1, 0);
        return 0;
        }

    case ID_EDIT_MODIFIED:
        if (SendMessageW(hwndEditor, EM_GETMODIFY, 0, 0))
            SendMessageW(hwndEditor, EM_SETMODIFY, 0, 0);
        else
            SendMessageW(hwndEditor, EM_SETMODIFY, 1, 0);
        return 0;

    case ID_EDIT_UNDO:
        SendMessageW(hwndEditor, EM_UNDO, 0, 0);
        return 0;

    case ID_EDIT_REDO:
        SendMessageW(hwndEditor, EM_REDO, 0, 0);
        return 0;

    case ID_BULLET:
        {
            PARAFORMAT pf;

            pf.cbSize = sizeof(pf);
            pf.dwMask = PFM_NUMBERING;
            SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);

            pf.dwMask |=  PFM_OFFSET;

            if(pf.wNumbering == PFN_BULLET)
            {
                pf.wNumbering = 0;
                pf.dxOffset = 0;
            } else
            {
                pf.wNumbering = PFN_BULLET;
                pf.dxOffset = 720;
            }

            SendMessageW(hwndEditor, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
        }
        break;

    case ID_ALIGN_LEFT:
    case ID_ALIGN_CENTER:
    case ID_ALIGN_RIGHT:
        {
        PARAFORMAT2 pf;

        pf.cbSize = sizeof(pf);
        pf.dwMask = PFM_ALIGNMENT;
        switch(LOWORD(wParam)) {
        case ID_ALIGN_LEFT: pf.wAlignment = PFA_LEFT; break;
        case ID_ALIGN_CENTER: pf.wAlignment = PFA_CENTER; break;
        case ID_ALIGN_RIGHT: pf.wAlignment = PFA_RIGHT; break;
        }
        SendMessageW(hwndEditor, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
        break;
        }

    case ID_BACK_1:
        SendMessageW(hwndEditor, EM_SETBKGNDCOLOR, 1, 0);
        break;

    case ID_BACK_2:
        SendMessageW(hwndEditor, EM_SETBKGNDCOLOR, 0, RGB(255,255,192));
        break;

    case ID_TOGGLE_TOOLBAR:
        set_toolbar_state(BANDID_TOOLBAR, !is_bar_visible(BANDID_TOOLBAR));
        update_window();
        break;

    case ID_TOGGLE_FORMATBAR:
        set_toolbar_state(BANDID_FONTLIST, !is_bar_visible(BANDID_FORMATBAR));
        set_toolbar_state(BANDID_SIZELIST, !is_bar_visible(BANDID_FORMATBAR));
        set_toolbar_state(BANDID_FORMATBAR, !is_bar_visible(BANDID_FORMATBAR));
        update_window();
        break;

    case ID_TOGGLE_STATUSBAR:
        set_statusbar_state(!is_bar_visible(BANDID_STATUSBAR));
        update_window();
        break;

    case ID_DATETIME:
        {
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_DATETIME), hWnd, (DLGPROC)datetime_proc);
        break;
        }

    case ID_PARAFORMAT:
        {
            HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_PARAFORMAT), hWnd,
                       paraformat_proc);
        }
        break;

    case ID_TABSTOPS:
        {
            HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
            DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_TABSTOPS), hWnd, tabstops_proc);
        }
        break;

    case ID_ABOUT:
        dialog_about();
        break;

    case ID_VIEWPROPERTIES:
        dialog_viewproperties();
        break;

    default:
        SendMessageW(hwndEditor, WM_COMMAND, wParam, lParam);
        break;
    }
    return 0;
}

static LRESULT OnInitPopupMenu( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    HMENU hMenu = (HMENU)wParam;
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndStatus = GetDlgItem(hWnd, IDC_STATUSBAR);
    PARAFORMAT pf;
    int nAlignment = -1;
    int selFrom, selTo;
    GETTEXTLENGTHEX gt;
    LRESULT textLength;
    MENUITEMINFOW mi;

    SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&selFrom, (LPARAM)&selTo);
    EnableMenuItem(hMenu, ID_EDIT_COPY, MF_BYCOMMAND|(selFrom == selTo) ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(hMenu, ID_EDIT_CUT, MF_BYCOMMAND|(selFrom == selTo) ? MF_GRAYED : MF_ENABLED);

    pf.cbSize = sizeof(PARAFORMAT);
    SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    CheckMenuItem(hMenu, ID_EDIT_READONLY,
      MF_BYCOMMAND|(GetWindowLong(hwndEditor, GWL_STYLE)&ES_READONLY ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_EDIT_MODIFIED,
      MF_BYCOMMAND|(SendMessage(hwndEditor, EM_GETMODIFY, 0, 0) ? MF_CHECKED : MF_UNCHECKED));
    if (pf.dwMask & PFM_ALIGNMENT)
        nAlignment = pf.wAlignment;
    CheckMenuItem(hMenu, ID_ALIGN_LEFT, MF_BYCOMMAND|(nAlignment == PFA_LEFT) ?
            MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_CENTER, MF_BYCOMMAND|(nAlignment == PFA_CENTER) ?
            MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_RIGHT, MF_BYCOMMAND|(nAlignment == PFA_RIGHT) ?
            MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_BULLET, MF_BYCOMMAND | ((pf.wNumbering == PFN_BULLET) ?
            MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(hMenu, ID_EDIT_UNDO, MF_BYCOMMAND|(SendMessageW(hwndEditor, EM_CANUNDO, 0, 0)) ?
            MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, ID_EDIT_REDO, MF_BYCOMMAND|(SendMessageW(hwndEditor, EM_CANREDO, 0, 0)) ?
            MF_ENABLED : MF_GRAYED);

    CheckMenuItem(hMenu, ID_TOGGLE_TOOLBAR, MF_BYCOMMAND|(is_bar_visible(BANDID_TOOLBAR)) ?
            MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, ID_TOGGLE_FORMATBAR, MF_BYCOMMAND|(is_bar_visible(BANDID_FORMATBAR)) ?
            MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, ID_TOGGLE_STATUSBAR, MF_BYCOMMAND|IsWindowVisible(hwndStatus) ?
            MF_CHECKED : MF_UNCHECKED);

    gt.flags = GTL_NUMCHARS;
    gt.codepage = 1200;
    textLength = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);
    EnableMenuItem(hMenu, ID_FIND, MF_BYCOMMAND|(textLength ? MF_ENABLED : MF_GRAYED));

    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_DATA;

    GetMenuItemInfoW(hMenu, ID_FIND_NEXT, FALSE, &mi);

    EnableMenuItem(hMenu, ID_FIND_NEXT, MF_BYCOMMAND|((textLength && mi.dwItemData) ?
                   MF_ENABLED : MF_GRAYED));

    EnableMenuItem(hMenu, ID_REPLACE, MF_BYCOMMAND|(textLength ? MF_ENABLED : MF_GRAYED));

    return 0;
}

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    int nStatusSize = 0;
    RECT rc;
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    int rebarHeight = 0;
    int rebarRows = 2;

    if (hwndStatusBar)
    {
        SendMessageW(hwndStatusBar, WM_SIZE, 0, 0);
        if (IsWindowVisible(hwndStatusBar))
        {
            GetClientRect(hwndStatusBar, &rc);
            nStatusSize = rc.bottom - rc.top;
        } else
        {
            nStatusSize = 0;
        }
    }
    if (hwndReBar)
    {
        if(!is_bar_visible(BANDID_TOOLBAR))
            rebarRows--;

        if(!is_bar_visible(BANDID_FORMATBAR))
            rebarRows--;

        rebarHeight = rebarRows ? SendMessageW(hwndReBar, RB_GETBARHEIGHT, 0, 0) : 0;

        MoveWindow(hwndReBar, 0, 0, LOWORD(lParam), rebarHeight, TRUE);
    }
    if (hwndEditor)
    {
        GetClientRect(hWnd, &rc);
        MoveWindow(hwndEditor, 0, rebarHeight, rc.right, rc.bottom-nStatusSize-rebarHeight, TRUE);
    }

    return DefWindowProcW(hWnd, WM_SIZE, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == ID_FINDMSGSTRING)
        return handle_findmsg((LPFINDREPLACEW)lParam);

    switch(msg)
    {
    case WM_CREATE:
        return OnCreate( hWnd, wParam, lParam );

    case WM_USER:
        return OnUser( hWnd, wParam, lParam );

    case WM_NOTIFY:
        return OnNotify( hWnd, wParam, lParam );

    case WM_COMMAND:
        if(preview.page)
            return preview_command( hWnd, wParam, lParam );
        else
            return OnCommand( hWnd, wParam, lParam );

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CLOSE:
        if(preview.page)
        {
            preview_exit();
        } else if(prompt_save_changes())
        {
            registry_set_options();
            registry_set_formatopts_all();
            PostQuitMessage(0);
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            SetFocus(GetDlgItem(hWnd, IDC_EDITOR));
        return 0;

    case WM_INITMENUPOPUP:
        return OnInitPopupMenu( hWnd, wParam, lParam );

    case WM_SIZE:
        return OnSize( hWnd, wParam, lParam );

    case WM_CONTEXTMENU:
        if((HWND)wParam == hEditorWnd)
            return context_menu(lParam);
        else
            return DefWindowProcW(hWnd, msg, wParam, lParam);

    case WM_DROPFILES:
        {
            WCHAR file[MAX_PATH];
            DragQueryFileW((HDROP)wParam, 0, file, MAX_PATH);
            DragFinish((HDROP)wParam);

            if(prompt_save_changes())
                DoOpenFile(file);
        }
        break;
    case WM_PAINT:
        if(preview.page)
            return print_preview();
        else
            return DefWindowProcW(hWnd, msg, wParam, lParam);

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hOldInstance, LPWSTR szCmdParagraph, int res)
{
    INITCOMMONCONTROLSEX classes = {8, ICC_BAR_CLASSES|ICC_COOL_CLASSES|ICC_USEREX_CLASSES};
    HACCEL hAccel;
    WNDCLASSW wc;
    MSG msg;
    RECT rc;
    static const WCHAR wszAccelTable[] = {'M','A','I','N','A','C','C','E','L',
                                          'T','A','B','L','E','\0'};

    InitCommonControlsEx(&classes);

    hAccel = LoadAcceleratorsW(hInstance, wszAccelTable);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WORDPAD));
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = xszMainMenu;
    wc.lpszClassName = wszMainWndClass;
    RegisterClassW(&wc);

    rc = registry_read_winrect();
    hMainWnd = CreateWindowExW(0, wszMainWndClass, wszAppTitle, WS_CLIPCHILDREN|WS_OVERLAPPEDWINDOW,
      rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, NULL, NULL, hInstance, NULL);
    ShowWindow(hMainWnd, SW_SHOWDEFAULT);

    set_caption(NULL);
    set_bar_states();
    set_fileformat(SF_RTF);
    hPopupMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDM_POPUP));
    get_default_printer_opts();
    target_device();

    HandleCommandLine(GetCommandLineW());

    while(GetMessageW(&msg,0,0,0))
    {
        if (IsDialogMessage(hFindWnd, &msg))
            continue;

        if (TranslateAcceleratorW(hMainWnd, hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!PeekMessageW(&msg, 0, 0, 0, PM_NOREMOVE))
            SendMessageW(hMainWnd, WM_USER, 0, 0);
    }

    return 0;
}
