/*
 * Wordpad implementation
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2007-2008 by Alexander N. Sørnes <alex@thehandofagony.com>
 * Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include <stdio.h>
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <richedit.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wine/unicode.h>

#include "wordpad.h"

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
static const WCHAR wszAppTitle[] = {'W','o','r','d','p','a','d',0};

static const WCHAR wszMainWndClass[] = {'W','O','R','D','P','A','D','T','O','P',0};

static const WCHAR stringFormat[] = {'%','2','d','\0'};

const WCHAR wszPreviewWndClass[] = {'P','r','t','P','r','e','v','i','e','w',0};
LRESULT CALLBACK preview_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND hMainWnd;
static HWND hEditorWnd;
static HWND hFindWnd;
static HMENU hColorPopupMenu;

static UINT ID_FINDMSGSTRING;

static DWORD wordWrap[2];
static DWORD barState[2];
static WPARAM fileFormat = SF_RTF;

static WCHAR wszFileName[MAX_PATH];
static WCHAR wszFilter[MAX_STRING_LEN*4+6*3+5];
static WCHAR wszDefaultFileName[MAX_STRING_LEN];
static WCHAR wszSaveChanges[MAX_STRING_LEN];
static WCHAR units_cmW[MAX_STRING_LEN];
static WCHAR units_inW[MAX_STRING_LEN];
static WCHAR units_inchW[MAX_STRING_LEN];
static WCHAR units_ptW[MAX_STRING_LEN];

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam );

typedef enum
{
    UNIT_CM,
    UNIT_INCH,
    UNIT_PT
} UNIT;

typedef struct
{
    int endPos;
    BOOL wrapped;
    WCHAR findBuffer[128];
} FINDREPLACE_custom;

/* Load string resources */
static void DoLoadStrings(void)
{
    LPWSTR p = wszFilter;
    static const WCHAR files_rtf[] = {'*','.','r','t','f','\0'};
    static const WCHAR files_txt[] = {'*','.','t','x','t','\0'};
    static const WCHAR files_all[] = {'*','.','*','\0'};

    HINSTANCE hInstance = GetModuleHandleW(0);

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

    p = wszDefaultFileName;
    LoadStringW(hInstance, STRING_DEFAULT_FILENAME, p, MAX_STRING_LEN);

    p = wszSaveChanges;
    LoadStringW(hInstance, STRING_PROMPT_SAVE_CHANGES, p, MAX_STRING_LEN);

    LoadStringW(hInstance, STRING_UNITS_CM, units_cmW, MAX_STRING_LEN);
    LoadStringW(hInstance, STRING_UNITS_IN, units_inW, MAX_STRING_LEN);
    LoadStringW(hInstance, STRING_UNITS_INCH, units_inchW, MAX_STRING_LEN);
    LoadStringW(hInstance, STRING_UNITS_PT, units_ptW, MAX_STRING_LEN);
}

/* Show a message box with resource strings */
static int MessageBoxWithResStringW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    MSGBOXPARAMSW params;

    params.cbSize             = sizeof(params);
    params.hwndOwner          = hWnd;
    params.hInstance          = GetModuleHandleW(0);
    params.lpszText           = lpText;
    params.lpszCaption        = lpCaption;
    params.dwStyle            = uType;
    params.lpszIcon           = NULL;
    params.dwContextHelpId    = 0;
    params.lpfnMsgBoxCallback = NULL;
    params.dwLanguageId       = 0;
    return MessageBoxIndirectW(&params);
}


static void AddButton(HWND hwndToolBar, int nImage, int nCommand)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = nImage;
    button.idCommand = nCommand;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = BTNS_BUTTON;
    button.dwData = 0;
    button.iString = -1;
    SendMessageW(hwndToolBar, TB_ADDBUTTONSW, 1, (LPARAM)&button);
}

static void AddSeparator(HWND hwndToolBar)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = -1;
    button.idCommand = 0;
    button.fsState = 0;
    button.fsStyle = BTNS_SEP;
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

LPWSTR file_basename(LPWSTR path)
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

static BOOL validate_endptr(LPCWSTR endptr, UNIT *punit)
{
    if(punit != NULL)
        *punit = UNIT_CM;
    if(!endptr)
        return FALSE;
    if(!*endptr)
        return TRUE;

    while(*endptr == ' ')
        endptr++;

    if(punit == NULL)
        return *endptr == '\0';

    if(!lstrcmpW(endptr, units_cmW))
    {
        *punit = UNIT_CM;
        endptr += lstrlenW(units_cmW);
    }
    else if (!lstrcmpW(endptr, units_inW))
    {
        *punit = UNIT_INCH;
        endptr += lstrlenW(units_inW);
    }
    else if (!lstrcmpW(endptr, units_inchW))
    {
        *punit = UNIT_INCH;
        endptr += lstrlenW(units_inchW);
    }
    else if (!lstrcmpW(endptr, units_ptW))
    {
        *punit = UNIT_PT;
        endptr += lstrlenW(units_ptW);
    }

    return *endptr == '\0';
}

static BOOL number_from_string(LPCWSTR string, float *num, UNIT *punit)
{
    double ret;
    WCHAR *endptr;

    *num = 0;
    errno = 0;
    ret = wcstod(string, &endptr);

    if (punit != NULL)
        *punit = UNIT_CM;
    if((ret == 0 && errno != 0) || endptr == string || !validate_endptr(endptr, punit))
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

static void on_sizelist_modified(HWND hwndSizeList, LPWSTR wszNewFontSize)
{
    WCHAR sizeBuffer[MAX_STRING_LEN];
    CHARFORMAT2W format;

    ZeroMemory(&format, sizeof(format));
    format.cbSize = sizeof(format);
    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

    wsprintfW(sizeBuffer, stringFormat, format.yHeight / 20);
    if(lstrcmpW(sizeBuffer, wszNewFontSize))
    {
        float size = 0;
        if(number_from_string(wszNewFontSize, &size, NULL)
           && size > 0)
        {
            set_size(size);
        } else
        {
            SetWindowTextW(hwndSizeList, sizeBuffer);
            MessageBoxWithResStringW(hMainWnd, MAKEINTRESOURCEW(STRING_INVALID_NUMBER),
                        wszAppTitle, MB_OK | MB_ICONINFORMATION);
        }
    }
}

static void add_size(HWND hSizeListWnd, unsigned size)
{
    WCHAR buffer[3];
    COMBOBOXEXITEMW cbItem;
    cbItem.mask = CBEIF_TEXT;
    cbItem.iItem = -1;

    wsprintfW(buffer, stringFormat, size);
    cbItem.pszText = buffer;
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
    size_t i;
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
    if (!SendMessageW(hFontListEdit, WM_GETTEXT, MAX_PATH, (LPARAM)fontName)) return;

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

    SendMessageW(hFontListEditWnd, WM_SETTEXT, 0, (LPARAM)wszFaceName);
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

static void on_fontlist_modified(LPWSTR wszNewFaceName)
{
    CHARFORMAT2W format;
    ZeroMemory(&format, sizeof(format));
    format.cbSize = sizeof(format);
    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

    if(lstrcmpW(format.szFaceName, wszNewFaceName))
        set_font(wszNewFaceName);
}

static void add_font(LPCWSTR fontName, DWORD fontType, HWND hListWnd, const NEWTEXTMETRICEXW *ntmc)
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
    cf.Flags = CF_SCREENFONTS | CF_NOSCRIPTSEL | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_NOVERTFONTS;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    SendMessageW(hEditorWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
    lstrcpyW(cf.lpLogFont->lfFaceName, fmt.szFaceName);
    cf.lpLogFont->lfItalic = (fmt.dwEffects & CFE_ITALIC) != 0;
    cf.lpLogFont->lfWeight = (fmt.dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
    cf.lpLogFont->lfUnderline = (fmt.dwEffects & CFE_UNDERLINE) != 0;
    cf.lpLogFont->lfStrikeOut = (fmt.dwEffects & CFE_STRIKEOUT) != 0;
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
        if(cf.lpLogFont->lfUnderline)
            fmt.dwEffects |= CFE_UNDERLINE;
        if(cf.lpLogFont->lfStrikeOut)
            fmt.dwEffects |= CFE_STRIKEOUT;

        fmt.crTextColor = cf.rgbColors;

        SendMessageW(hEditorWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        set_font(cf.lpLogFont->lfFaceName);
    }
}


static int CALLBACK enum_font_proc(const LOGFONTW *lpelfe, const TEXTMETRICW *lpntme,
                            DWORD FontType, LPARAM lParam)
{
    HWND hListWnd = (HWND) lParam;

    if (lpelfe->lfFaceName[0] == '@') return 1;  /* ignore vertical fonts */

    if(SendMessageW(hListWnd, CB_FINDSTRINGEXACT, -1, (LPARAM)lpelfe->lfFaceName) == CB_ERR)
    {

        add_font(lpelfe->lfFaceName, FontType, hListWnd, (const NEWTEXTMETRICEXW*)lpntme);
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

    GetClientRect(hMainWnd, &rect);

    OnSize(hMainWnd, SIZE_RESTORED, MAKELPARAM(rect.right, rect.bottom));
}

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

        rbbinfo.cbSize = REBARBANDINFOW_V6_SIZE;
        rbbinfo.fMask = RBBIM_STYLE;

        SendMessageW(hwndReBar, RB_GETBANDINFOW, index, (LPARAM)&rbbinfo);

        if(!show)
            rbbinfo.fStyle &= ~RBBS_BREAK;
        else
            rbbinfo.fStyle |= RBBS_BREAK;

        SendMessageW(hwndReBar, RB_SETBANDINFOW, index, (LPARAM)&rbbinfo);
    }

    if(bandId == BANDID_TOOLBAR || bandId == BANDID_FORMATBAR || bandId == BANDID_RULER)
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
    set_toolbar_state(BANDID_RULER, is_bar_visible(BANDID_RULER));
    set_statusbar_state(is_bar_visible(BANDID_STATUSBAR));

    update_window();
}

static void preview_exit(HWND hMainWnd)
{
    HMENU hMenu = LoadMenuW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDM_MAINMENU));
    HWND hEditorWnd = GetDlgItem(hMainWnd, IDC_EDITOR);

    set_bar_states();
    ShowWindow(hEditorWnd, TRUE);

    close_preview(hMainWnd);

    SetMenu(hMainWnd, hMenu);
    registry_read_filelist(hMainWnd);

    update_window();
}

static void set_fileformat(WPARAM format)
{
    fileFormat = format;

    set_bar_states();
    set_default_font();
    target_device(hMainWnd, wordWrap[reg_formatindex(fileFormat)]);
}

static void ShowOpenError(DWORD Code)
{
    LPWSTR Message;

    switch(Code)
    {
        case ERROR_ACCESS_DENIED:
            Message = MAKEINTRESOURCEW(STRING_OPEN_ACCESS_DENIED);
            break;

        default:
            Message = MAKEINTRESOURCEW(STRING_OPEN_FAILED);
    }
    MessageBoxW(hMainWnd, Message, wszAppTitle, MB_ICONEXCLAMATION | MB_OK);
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
    {
        ShowOpenError(GetLastError());
        return;
    }

    ReadFile(hFile, fileStart, 5, &readOut, NULL);
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    if(readOut >= 2 && (BYTE)fileStart[0] == 0xff && (BYTE)fileStart[1] == 0xfe)
    {
        format = SF_TEXT | SF_UNICODE;
        SetFilePointer(hFile, 2, NULL, FILE_BEGIN);
    } else if(readOut >= 5)
    {
        static const char header[] = "{\\rtf";
        static const BYTE STG_magic[] = { 0xd0,0xcf,0x11,0xe0 };

        if(!memcmp(header, fileStart, 5))
            format = SF_RTF;
        else if (!memcmp(STG_magic, fileStart, sizeof(STG_magic)))
        {
            CloseHandle(hFile);
            MessageBoxWithResStringW(hMainWnd, MAKEINTRESOURCEW(STRING_OLE_STORAGE_NOT_SUPPORTED),
                    wszAppTitle, MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }

    es.dwCookie = (DWORD_PTR)hFile;
    es.pfnCallback = stream_in;

    clear_formatting();
    set_fileformat(format);
    SendMessageW(hEditorWnd, EM_STREAMIN, format, (LPARAM)&es);

    CloseHandle(hFile);

    SetFocus(hEditorWnd);

    set_caption(szOpenFileName);
    if (szOpenFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, szOpenFileName);

    lstrcpyW(wszFileName, szOpenFileName);
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
    registry_set_filelist(szOpenFileName, hMainWnd);
    update_font_list();
}

static void ShowWriteError(DWORD Code)
{
    LPWSTR Message;

    switch(Code)
    {
        case ERROR_ACCESS_DENIED:
            Message = MAKEINTRESOURCEW(STRING_WRITE_ACCESS_DENIED);
            break;

        default:
            Message = MAKEINTRESOURCEW(STRING_WRITE_FAILED);
    }
    MessageBoxW(hMainWnd, Message, wszAppTitle, MB_ICONEXCLAMATION | MB_OK);
}

static BOOL DoSaveFile(LPCWSTR wszSaveFileName, WPARAM format)
{
    HANDLE hFile;
    EDITSTREAM stream;
    LRESULT ret;

    hFile = CreateFileW(wszSaveFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        ShowWriteError(GetLastError());
        return FALSE;
    }

    if(format == (SF_TEXT | SF_UNICODE))
    {
        static const BYTE unicode[] = {0xff,0xfe};
        DWORD writeOut;
        WriteFile(hFile, &unicode, sizeof(unicode), &writeOut, 0);

        if(writeOut != sizeof(unicode))
        {
            CloseHandle(hFile);
            return FALSE;
        }
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
            return FALSE;
    }

    lstrcpyW(wszFileName, wszSaveFileName);
    set_caption(wszFileName);
    if (wszFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, wszFileName);

    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);
    set_fileformat(format);

    return TRUE;
}

static BOOL DialogSaveFile(void)
{
    OPENFILENAMEW sfn;

    WCHAR wszFile[MAX_PATH] = {'\0'};
    static const WCHAR wszDefExt[] = {'r','t','f','\0'};

    ZeroMemory(&sfn, sizeof(sfn));

    sfn.lStructSize = sizeof(sfn);
    sfn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING;
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
            if(MessageBoxWithResStringW(hMainWnd, MAKEINTRESOURCEW(STRING_SAVE_LOSEFORMATTING),
                           wszAppTitle, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
                continue;
        }
        return DoSaveFile(sfn.lpstrFile, fileformat_flags(sfn.nFilterIndex-1));
    }
    return FALSE;
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
                    return DoSaveFile(wszFileName, fileFormat);
                return DialogSaveFile();

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
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ENABLESIZING;
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

static void dialog_about(void)
{
    HICON icon = LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_WORDPAD), IMAGE_ICON, 48, 48, LR_SHARED);
    ShellAboutW(hMainWnd, wszAppTitle, NULL, icon);
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
                if(wordWrap[ps->lParam] == ID_WORDWRAP_NONE)
                    wrap = IDC_PAGEFMT_WN;
                else if(wordWrap[ps->lParam] == ID_WORDWRAP_WINDOW)
                    wrap = IDC_PAGEFMT_WW;
                else if(wordWrap[ps->lParam] == ID_WORDWRAP_MARGIN)
                    wrap = IDC_PAGEFMT_WM;

                if(wrap != -1)
                    CheckRadioButton(hWnd, IDC_PAGEFMT_WN,
                                     IDC_PAGEFMT_WM, wrap);

                if(barState[ps->lParam] & (1 << BANDID_TOOLBAR))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_TB, TRUE);
                if(barState[ps->lParam] & (1 << BANDID_FORMATBAR))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_FB, TRUE);
                if(barState[ps->lParam] & (1 << BANDID_RULER))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_RU, TRUE);
                if(barState[ps->lParam] & (1 << BANDID_STATUSBAR))
                    CheckDlgButton(hWnd, IDC_PAGEFMT_SB, TRUE);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_PAGEFMT_WN:
                case IDC_PAGEFMT_WW:
                case IDC_PAGEFMT_WM:
                    CheckRadioButton(hWnd, IDC_PAGEFMT_WN, IDC_PAGEFMT_WM,
                                     LOWORD(wParam));
                    break;

                case IDC_PAGEFMT_TB:
                case IDC_PAGEFMT_FB:
                case IDC_PAGEFMT_RU:
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
                    if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_WN))
                        wordWrap[id] = ID_WORDWRAP_NONE;
                    else if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_WW))
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

                    if(IsDlgButtonChecked(hWnd, IDC_PAGEFMT_RU))
                        barState[id] |= (1 << BANDID_RULER);
                    else
                        barState[id] &= ~(1 << BANDID_RULER);

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
    size_t i;
    HINSTANCE hInstance = GetModuleHandleW(0);
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
    target_device(hMainWnd, wordWrap[reg_formatindex(fileFormat)]);
}

static void HandleCommandLine(LPWSTR cmdline)
{
    WCHAR delimiter;
    BOOL opt_print = FALSE;

    /* skip white space */
    while (*cmdline == ' ') cmdline++;

    /* skip executable name */
    delimiter = (*cmdline == '"' ? '"' : ' ');

    if (*cmdline == delimiter) cmdline++;
    while (*cmdline && *cmdline != delimiter) cmdline++;
    if (*cmdline == delimiter) cmdline++;

    while (*cmdline)
    {
        while (isspace(*cmdline)) cmdline++;

        if (*cmdline == '-' || *cmdline == '/')
        {
            if (!cmdline[2] || isspace(cmdline[2]))
            {
                switch (cmdline[1])
                {
                case 'P':
                case 'p':
                    opt_print = TRUE;
                    cmdline += 2;
                    continue;
                }
            }
            /* a filename starting by / */
        }
        break;
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
        MessageBoxWithResStringW(hMainWnd, MAKEINTRESOURCEW(STRING_PRINTING_NOT_IMPLEMENTED), wszAppTitle, MB_OK);
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
        FINDREPLACE_custom *custom_data = (FINDREPLACE_custom*)pFr->lCustData;
        DWORD flags;
        FINDTEXTEXW ft;
        CHARRANGE sel;
        LRESULT ret = -1;
        HMENU hMenu = GetMenu(hMainWnd);
        MENUITEMINFOW mi;

        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_DATA;
        mi.dwItemData = 1;
        SetMenuItemInfoW(hMenu, ID_FIND_NEXT, FALSE, &mi);

        /* Make sure find field is saved. */
        if (pFr->lpstrFindWhat != custom_data->findBuffer)
        {
            lstrcpynW(custom_data->findBuffer, pFr->lpstrFindWhat,
                      sizeof(custom_data->findBuffer) / sizeof(WCHAR));
            pFr->lpstrFindWhat = custom_data->findBuffer;
        }

        SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&sel.cpMin, (LPARAM)&sel.cpMax);
        if(custom_data->endPos == -1) {
            custom_data->endPos = sel.cpMin;
            custom_data->wrapped = FALSE;
        }

        flags = FR_DOWN | (pFr->Flags & (FR_MATCHCASE | FR_WHOLEWORD));
        ft.lpstrText = pFr->lpstrFindWhat;

        /* Only replace the existing selection if it is an exact match. */
        if (sel.cpMin != sel.cpMax &&
            (pFr->Flags & FR_REPLACE || pFr->Flags & FR_REPLACEALL))
        {
            ft.chrg = sel;
            SendMessageW(hEditorWnd, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
            if (ft.chrgText.cpMin == sel.cpMin && ft.chrgText.cpMax == sel.cpMax) {
                SendMessageW(hEditorWnd, EM_REPLACESEL, TRUE, (LPARAM)pFr->lpstrReplaceWith);
                SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&sel.cpMin, (LPARAM)&sel.cpMax);
            }
        }

        /* Search from the start of the selection, but exclude the first character
         * from search if there is a selection. */
        ft.chrg.cpMin = sel.cpMin;
        if (sel.cpMin != sel.cpMax)
            ft.chrg.cpMin++;

        /* Search to the end, then wrap around and search from the start. */
        if (!custom_data->wrapped) {
            ft.chrg.cpMax = -1;
            ret = SendMessageW(hEditorWnd, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
            if (ret == -1) {
                custom_data->wrapped = TRUE;
                ft.chrg.cpMin = 0;
            }
        }

        if (ret == -1) {
            ft.chrg.cpMax = custom_data->endPos + lstrlenW(pFr->lpstrFindWhat) - 1;
            if (ft.chrg.cpMax > ft.chrg.cpMin)
                ret = SendMessageW(hEditorWnd, EM_FINDTEXTEXW, flags, (LPARAM)&ft);
        }

        if (ret == -1) {
            custom_data->endPos = -1;
            EnableWindow(hMainWnd, FALSE);
            MessageBoxWithResStringW(hFindWnd, MAKEINTRESOURCEW(STRING_SEARCH_FINISHED),
                                     wszAppTitle, MB_OK | MB_ICONASTERISK | MB_TASKMODAL);
            EnableWindow(hMainWnd, TRUE);
        } else {
            SendMessageW(hEditorWnd, EM_SETSEL, ft.chrgText.cpMin, ft.chrgText.cpMax);
            SendMessageW(hEditorWnd, EM_SCROLLCARET, 0, 0);

            if (pFr->Flags & FR_REPLACEALL)
                return handle_findmsg(pFr);
        }
    }

    return 0;
}

static void dialog_find(LPFINDREPLACEW fr, BOOL replace)
{
    static WCHAR selBuffer[128];
    static WCHAR replaceBuffer[128];
    static FINDREPLACE_custom custom_data;
    static const WCHAR endl = '\r';
    FINDTEXTW ft;

    /* Allow only one search/replace dialog to open */
    if(hFindWnd != NULL)
    {
        SetActiveWindow(hFindWnd);
        return;
    }

    ZeroMemory(fr, sizeof(FINDREPLACEW));
    fr->lStructSize = sizeof(FINDREPLACEW);
    fr->hwndOwner = hMainWnd;
    fr->Flags = FR_HIDEUPDOWN;
    /* Find field is filled with the selected text if it is non-empty
     * and stays within the same paragraph, otherwise the previous
     * find field is used. */
    SendMessageW(hEditorWnd, EM_GETSEL, (WPARAM)&ft.chrg.cpMin,
                 (LPARAM)&ft.chrg.cpMax);
    ft.lpstrText = &endl;
    if (ft.chrg.cpMin != ft.chrg.cpMax &&
        SendMessageW(hEditorWnd, EM_FINDTEXTW, FR_DOWN, (LPARAM)&ft) == -1)
    {
        /* Use a temporary buffer for the selected text so that the saved
         * find field is only overwritten when a find/replace is clicked. */
        GETTEXTEX gt = {sizeof(selBuffer), GT_SELECTION, 1200, NULL, NULL};
        SendMessageW(hEditorWnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)selBuffer);
        fr->lpstrFindWhat = selBuffer;
    } else {
        fr->lpstrFindWhat = custom_data.findBuffer;
    }
    fr->lpstrReplaceWith = replaceBuffer;
    custom_data.endPos = -1;
    custom_data.wrapped = FALSE;
    fr->lCustData = (LPARAM)&custom_data;
    fr->wFindWhatLen = sizeof(custom_data.findBuffer);
    fr->wReplaceWithLen = sizeof(replaceBuffer);

    if(replace)
        hFindWnd = ReplaceTextW(fr);
    else
        hFindWnd = FindTextW(fr);
}

static int units_to_twips(UNIT unit, float number)
{
    int twips = 0;

    switch(unit)
    {
    case UNIT_CM:
        twips = (int)(number * 1000.0 / (float)CENTMM_PER_INCH * (float)TWIPS_PER_INCH);
        break;

    case UNIT_INCH:
        twips = (int)(number * (float)TWIPS_PER_INCH);
        break;

    case UNIT_PT:
        twips = (int)(number * (0.0138 * (float)TWIPS_PER_INCH));
        break;
    }

    return twips;
}

static void append_current_units(LPWSTR buffer)
{
    static const WCHAR space[] = {' ', 0};
    lstrcatW(buffer, space);
    lstrcatW(buffer, units_cmW);
}

static void number_with_units(LPWSTR buffer, int number)
{
    static const WCHAR fmt[] = {'%','.','2','f',' ','%','s','\0'};
    float converted = (float)number / (float)TWIPS_PER_INCH *(float)CENTMM_PER_INCH / 1000.0;

    sprintfW(buffer, fmt, converted, units_cmW);
}

static BOOL get_comboexlist_selection(HWND hComboEx, LPWSTR wszBuffer, UINT bufferLength)
{
    COMBOBOXEXITEMW cbItem;
    COMBOBOXINFO cbInfo;
    HWND hCombo, hList;
    int idx, result;

    hCombo = (HWND)SendMessageW(hComboEx, CBEM_GETCOMBOCONTROL, 0, 0);
    if (!hCombo)
        return FALSE;
    cbInfo.cbSize = sizeof(COMBOBOXINFO);
    result = SendMessageW(hCombo, CB_GETCOMBOBOXINFO, 0, (LPARAM)&cbInfo);
    if (!result)
        return FALSE;
    hList = cbInfo.hwndList;
    idx = SendMessageW(hList, LB_GETCURSEL, 0, 0);
    if (idx < 0)
        return FALSE;

    ZeroMemory(&cbItem, sizeof(cbItem));
    cbItem.mask = CBEIF_TEXT;
    cbItem.iItem = idx;
    cbItem.pszText = wszBuffer;
    cbItem.cchTextMax = bufferLength-1;
    result = SendMessageW(hComboEx, CBEM_GETITEMW, 0, (LPARAM)&cbItem);

    return result != 0;
}

static INT_PTR CALLBACK datetime_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
                case IDC_DATETIME:
                    if (HIWORD(wParam) != LBN_DBLCLK)
                        break;
                    /* Fall through */

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

static INT_PTR CALLBACK newfile_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                HINSTANCE hInstance = GetModuleHandleW(0);
                WCHAR buffer[MAX_STRING_LEN];
                HWND hListWnd = GetDlgItem(hWnd, IDC_NEWFILE);

                LoadStringW(hInstance, STRING_NEWFILE_RICHTEXT, buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                LoadStringW(hInstance, STRING_NEWFILE_TXT, buffer, MAX_STRING_LEN);
                SendMessageW(hListWnd, LB_ADDSTRING, 0, (LPARAM)&buffer);
                LoadStringW(hInstance, STRING_NEWFILE_TXT_UNICODE, buffer, MAX_STRING_LEN);
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
    static const WORD ALIGNMENT_VALUES[] = {PFA_LEFT, PFA_RIGHT, PFA_CENTER};

    switch(message)
    {
        case WM_INITDIALOG:
            {
                HINSTANCE hInstance = GetModuleHandleW(0);
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
                            PFM_STARTINDENT;
                SendMessageW(hEditorWnd, EM_GETPARAFORMAT, 0, (LPARAM)&pf);

                if(pf.wAlignment == PFA_RIGHT)
                    index ++;
                else if(pf.wAlignment == PFA_CENTER)
                    index += 2;

                SendMessageW(hListWnd, CB_SETCURSEL, index, 0);

                number_with_units(buffer, pf.dxStartIndent + pf.dxOffset);
                SetWindowTextW(hLeftWnd, buffer);
                number_with_units(buffer, pf.dxRightIndent);
                SetWindowTextW(hRightWnd, buffer);
                number_with_units(buffer, -pf.dxOffset);
                SetWindowTextW(hFirstWnd, buffer);
            }
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    {
                        HWND hListWnd = GetDlgItem(hWnd, IDC_PARA_ALIGN);
                        HWND hLeftWnd = GetDlgItem(hWnd, IDC_PARA_LEFT);
                        HWND hRightWnd = GetDlgItem(hWnd, IDC_PARA_RIGHT);
                        HWND hFirstWnd = GetDlgItem(hWnd, IDC_PARA_FIRST);
                        WCHAR buffer[MAX_STRING_LEN];
                        int index;
                        float num;
                        int ret = 0;
                        PARAFORMAT pf;
                        UNIT unit;

                        index = SendMessageW(hListWnd, CB_GETCURSEL, 0, 0);
                        pf.wAlignment = ALIGNMENT_VALUES[index];

                        GetWindowTextW(hLeftWnd, buffer, MAX_STRING_LEN);
                        if(number_from_string(buffer, &num, &unit))
                            ret++;
                        pf.dxOffset = units_to_twips(unit, num);
                        GetWindowTextW(hRightWnd, buffer, MAX_STRING_LEN);
                        if(number_from_string(buffer, &num, &unit))
                            ret++;
                        pf.dxRightIndent = units_to_twips(unit, num);
                        GetWindowTextW(hFirstWnd, buffer, MAX_STRING_LEN);
                        if(number_from_string(buffer, &num, &unit))
                            ret++;
                        pf.dxStartIndent = units_to_twips(unit, num);

                        if(ret != 3)
                        {
                            MessageBoxWithResStringW(hMainWnd, MAKEINTRESOURCEW(STRING_INVALID_NUMBER),
                                        wszAppTitle, MB_OK | MB_ICONASTERISK);
                            return FALSE;
                        } else
                        {
                            if (pf.dxOffset + pf.dxStartIndent < 0
                                && pf.dxStartIndent < 0)
                            {
                                /* The first line is before the left edge, so
                                 * make sure it is at the left edge. */
                                pf.dxOffset = -pf.dxStartIndent;
                            } else if (pf.dxOffset < 0) {
                                /* The second and following lines are before
                                 * the left edge, so set it to be at the left
                                 * edge, and adjust the first line since it
                                 * is relative to it. */
                                pf.dxStartIndent = max(pf.dxStartIndent + pf.dxOffset, 0);
                                pf.dxOffset = 0;
                            }
                            /* Internally the dxStartIndent is the absolute
                             * offset for the first line and dxOffset is
                             * to it value as opposed how it is displayed with
                             * the first line being the relative value.
                             * These two lines make the adjustments. */
                            pf.dxStartIndent = pf.dxStartIndent + pf.dxOffset;
                            pf.dxOffset = pf.dxOffset - pf.dxStartIndent;

                            pf.cbSize = sizeof(pf);
                            pf.dwMask = PFM_ALIGNMENT | PFM_OFFSET | PFM_RIGHTINDENT |
                                        PFM_STARTINDENT;
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
                        UNIT unit;

                        GetWindowTextW(hTabWnd, buffer, MAX_STRING_LEN);
                        append_current_units(buffer);

                        if(SendMessageW(hTabWnd, CB_FINDSTRINGEXACT, -1, (LPARAM)&buffer) == CB_ERR)
                        {
                            float number = 0;
                            int item_count = SendMessageW(hTabWnd, CB_GETCOUNT, 0, 0);

                            if(!number_from_string(buffer, &number, &unit))
                            {
                                MessageBoxWithResStringW(hWnd, MAKEINTRESOURCEW(STRING_INVALID_NUMBER),
                                             wszAppTitle, MB_OK | MB_ICONINFORMATION);
                            } else if (item_count >= MAX_TAB_STOPS) {
                                MessageBoxWithResStringW(hWnd, MAKEINTRESOURCEW(STRING_MAX_TAB_STOPS),
                                             wszAppTitle, MB_OK | MB_ICONINFORMATION);
                            } else {
                                int i;
                                float next_number = -1;
                                int next_number_in_twips = -1;
                                int insert_number = units_to_twips(unit, number);

                                /* linear search for position to insert the string */
                                for(i = 0; i < item_count; i++)
                                {
                                    SendMessageW(hTabWnd, CB_GETLBTEXT, i, (LPARAM)&buffer);
                                    number_from_string(buffer, &next_number, &unit);
                                    next_number_in_twips = units_to_twips(unit, next_number);
                                    if (insert_number <= next_number_in_twips)
                                        break;
                                }
                                if (insert_number != next_number_in_twips)
                                {
                                    number_with_units(buffer, insert_number);
                                    SendMessageW(hTabWnd, CB_INSERTSTRING, i, (LPARAM)&buffer);
                                    SetWindowTextW(hTabWnd, 0);
                                }
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
                        UNIT unit;

                        pf.cbSize = sizeof(pf);
                        pf.dwMask = PFM_TABSTOPS;

                        for(i = 0; SendMessageW(hTabWnd, CB_GETLBTEXT, i,
                                                (LPARAM)&buffer) != CB_ERR &&
                                                        i < MAX_TAB_STOPS; i++)
                        {
                            number_from_string(buffer, &number, &unit);
                            pf.rgxTabs[i] = units_to_twips(unit, number);
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

static LRESULT OnCreate( HWND hWnd )
{
    HWND hToolBarWnd, hFormatBarWnd,  hReBarWnd, hFontListWnd, hSizeListWnd, hRulerWnd;
    HINSTANCE hInstance = GetModuleHandleW(0);
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

    hToolBarWnd = CreateToolbarEx(hReBarWnd, CCS_NOPARENTALIGN|CCS_NOMOVEY|WS_VISIBLE|WS_CHILD|TBSTYLE_TOOLTIPS|BTNS_BUTTON,
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

    rbb.cbSize = REBARBANDINFOW_V6_SIZE;
    rbb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_STYLE | RBBIM_ID;
    rbb.fStyle = RBBS_CHILDEDGE | RBBS_BREAK | RBBS_NOGRIPPER;
    rbb.cx = 0;
    rbb.hwndChild = hToolBarWnd;
    rbb.cxMinChild = 0;
    rbb.cyChild = rbb.cyMinChild = HIWORD(SendMessageW(hToolBarWnd, TB_GETBUTTONSIZE, 0, 0));
    rbb.wID = BANDID_TOOLBAR;

    SendMessageW(hReBarWnd, RB_INSERTBANDW, -1, (LPARAM)&rbb);

    hFontListWnd = CreateWindowExW(0, WC_COMBOBOXEXW, NULL,
                      WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_SORT,
                      0, 0, 200, 150, hReBarWnd, (HMENU)IDC_FONTLIST, hInstance, NULL);

    rbb.hwndChild = hFontListWnd;
    rbb.cx = 200;
    rbb.wID = BANDID_FONTLIST;

    SendMessageW(hReBarWnd, RB_INSERTBANDW, -1, (LPARAM)&rbb);

    hSizeListWnd = CreateWindowExW(0, WC_COMBOBOXEXW, NULL,
                      WS_BORDER | WS_VISIBLE | WS_CHILD | CBS_DROPDOWN,
                      0, 0, 50, 150, hReBarWnd, (HMENU)IDC_SIZELIST, hInstance, NULL);

    rbb.hwndChild = hSizeListWnd;
    rbb.cx = 50;
    rbb.fStyle ^= RBBS_BREAK;
    rbb.wID = BANDID_SIZELIST;

    SendMessageW(hReBarWnd, RB_INSERTBANDW, -1, (LPARAM)&rbb);

    hFormatBarWnd = CreateToolbarEx(hReBarWnd,
         CCS_NOPARENTALIGN | CCS_NOMOVEY | WS_VISIBLE | TBSTYLE_TOOLTIPS | BTNS_BUTTON,
         IDC_FORMATBAR, 8, hInstance, IDB_FORMATBAR, NULL, 0, 16, 16, 16, 16, sizeof(TBBUTTON));

    AddButton(hFormatBarWnd, 0, ID_FORMAT_BOLD);
    AddButton(hFormatBarWnd, 1, ID_FORMAT_ITALIC);
    AddButton(hFormatBarWnd, 2, ID_FORMAT_UNDERLINE);
    AddButton(hFormatBarWnd, 3, ID_FORMAT_COLOR);
    AddSeparator(hFormatBarWnd);
    AddButton(hFormatBarWnd, 4, ID_ALIGN_LEFT);
    AddButton(hFormatBarWnd, 5, ID_ALIGN_CENTER);
    AddButton(hFormatBarWnd, 6, ID_ALIGN_RIGHT);
    AddSeparator(hFormatBarWnd);
    AddButton(hFormatBarWnd, 7, ID_BULLET);

    SendMessageW(hFormatBarWnd, TB_AUTOSIZE, 0, 0);

    rbb.hwndChild = hFormatBarWnd;
    rbb.wID = BANDID_FORMATBAR;

    SendMessageW(hReBarWnd, RB_INSERTBANDW, -1, (LPARAM)&rbb);

    hRulerWnd = CreateWindowExW(0, WC_STATICW, NULL, WS_VISIBLE | WS_CHILD,
                                0, 0, 200, 10, hReBarWnd,  (HMENU)IDC_RULER, hInstance, NULL);


    rbb.hwndChild = hRulerWnd;
    rbb.wID = BANDID_RULER;
    rbb.fStyle |= RBBS_BREAK;

    SendMessageW(hReBarWnd, RB_INSERTBANDW, -1, (LPARAM)&rbb);

    hDLL = LoadLibraryW(wszRichEditDll);
    if(!hDLL)
    {
        MessageBoxWithResStringW(hWnd, MAKEINTRESOURCEW(STRING_LOAD_RICHED_FAILED), wszAppTitle,
                    MB_OK | MB_ICONEXCLAMATION);
        PostQuitMessage(1);
    }

    hEditorWnd = CreateWindowExW(WS_EX_CLIENTEDGE, RICHEDIT_CLASS20W, NULL,
      WS_CHILD|WS_VISIBLE|ES_SELECTIONBAR|ES_MULTILINE|ES_AUTOVSCROLL
      |ES_WANTRETURN|WS_VSCROLL|ES_NOHIDESEL|WS_HSCROLL,
      0, 0, 1000, 100, hWnd, (HMENU)IDC_EDITOR, hInstance, NULL);

    if (!hEditorWnd)
    {
        fprintf(stderr, "Error code %u\n", GetLastError());
        return -1;
    }
    assert(hEditorWnd);

    setup_richedit_olecallback(hEditorWnd);
    SetFocus(hEditorWnd);
    SendMessageW(hEditorWnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);

    set_default_font();

    populate_font_list(hFontListWnd);
    populate_size_list(hSizeListWnd);
    DoLoadStrings();
    SendMessageW(hEditorWnd, EM_SETMODIFY, FALSE, 0);

    ID_FINDMSGSTRING = RegisterWindowMessageW(FINDMSGSTRINGW);

    registry_read_filelist(hWnd);
    registry_read_formatopts_all(barState, wordWrap);
    registry_read_options();
    DragAcceptFiles(hWnd, TRUE);

    return 0;
}

static LRESULT OnUser( HWND hWnd )
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

static LRESULT OnNotify( HWND hWnd, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    NMHDR *pHdr = (NMHDR *)lParam;
    HWND hwndFontList = GetDlgItem(hwndReBar, IDC_FONTLIST);
    HWND hwndSizeList = GetDlgItem(hwndReBar, IDC_SIZELIST);

    if (pHdr->hwndFrom == hwndFontList || pHdr->hwndFrom == hwndSizeList)
    {
        if (pHdr->code == CBEN_ENDEDITW)
        {
            NMCBEENDEDITW *endEdit = (NMCBEENDEDITW *)lParam;
            if(pHdr->hwndFrom == hwndFontList)
            {
                on_fontlist_modified(endEdit->szText);
            } else if (pHdr->hwndFrom == hwndSizeList)
            {
                on_sizelist_modified(hwndSizeList,endEdit->szText);
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
                SendMessageW(hwndEditor, EM_GETLINECOUNT, 0, 0));
        SetWindowTextA(GetDlgItem(hWnd, IDC_STATUSBAR), buf);
        SendMessageW(hWnd, WM_USER, 0, 0);
        return 1;
    }
    return 0;
}

/* Copied from dlls/comdlg32/fontdlg.c */
static const COLORREF textcolors[]=
{
    0x00000000L,0x00000080L,0x00008000L,0x00008080L,
    0x00800000L,0x00800080L,0x00808000L,0x00808080L,
    0x00c0c0c0L,0x000000ffL,0x0000ff00L,0x0000ffffL,
    0x00ff0000L,0x00ff00ffL,0x00ffff00L,0x00FFFFFFL
};

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
            HINSTANCE hInstance = GetModuleHandleW(0);
            int ret = DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_NEWFILE), hWnd, newfile_proc);

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
        dialog_print(hWnd, wszFileName);
        target_device(hMainWnd, wordWrap[reg_formatindex(fileFormat)]);
        break;

    case ID_PRINT_QUICK:
        print_quick(hMainWnd, wszFileName);
        target_device(hMainWnd, wordWrap[reg_formatindex(fileFormat)]);
        break;

    case ID_PREVIEW:
        {
            int index = reg_formatindex(fileFormat);
            DWORD tmp = barState[index];
            barState[index] = 1 << BANDID_STATUSBAR;
            set_bar_states();
            barState[index] = tmp;
            ShowWindow(hEditorWnd, FALSE);

            init_preview(hWnd, wszFileName);

            SetMenu(hWnd, NULL);
            InvalidateRect(0, 0, TRUE);
        }
        break;

    case ID_PRINTSETUP:
        dialog_printsetup(hWnd);
        target_device(hMainWnd, wordWrap[reg_formatindex(fileFormat)]);
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

    case ID_FORMAT_COLOR:
    {
        HWND hReBarWnd = GetDlgItem(hWnd, IDC_REBAR);
        HWND hFormatBarWnd = GetDlgItem(hReBarWnd, IDC_FORMATBAR);
        HMENU hPop;
        RECT itemrc;
        POINT pt;
        int mid;
        int itemidx = SendMessageW(hFormatBarWnd, TB_COMMANDTOINDEX, ID_FORMAT_COLOR, 0);

        SendMessageW(hFormatBarWnd, TB_GETITEMRECT, itemidx, (LPARAM)&itemrc);
        pt.x = itemrc.left;
        pt.y = itemrc.bottom;
        ClientToScreen(hFormatBarWnd, &pt);
        hPop = GetSubMenu(hColorPopupMenu, 0);
        mid = TrackPopupMenu(hPop, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON |
                                   TPM_RETURNCMD | TPM_NONOTIFY,
                             pt.x, pt.y, 0, hWnd, 0);
        if (mid >= ID_COLOR_FIRST && mid <= ID_COLOR_AUTOMATIC)
        {
            CHARFORMAT2W fmt;

            ZeroMemory(&fmt, sizeof(fmt));
            fmt.cbSize = sizeof(fmt);
            SendMessageW(hwndEditor, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);

            fmt.dwMask = CFM_COLOR;

            if (mid < ID_COLOR_AUTOMATIC) {
                fmt.crTextColor = textcolors[mid - ID_COLOR_FIRST];
                fmt.dwEffects &= ~CFE_AUTOCOLOR;
            } else {
                fmt.dwEffects |= CFE_AUTOCOLOR;
            }

            SendMessageW(hwndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        }
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
        MessageBoxW(NULL, data, wszAppTitle, MB_OK);

        HeapFree( GetProcessHeap(), 0, data);
        data = HeapAlloc(GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR));
        tr.chrg.cpMin = 0;
        tr.chrg.cpMax = nLen;
        tr.lpstrText = data;
        SendMessageW(hwndEditor, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
        MessageBoxW(NULL, data, wszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data );

        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_CHARFORMAT:
    case ID_EDIT_DEFCHARFORMAT:
        {
        CHARFORMAT2W cf;

        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = 0;
        SendMessageW(hwndEditor, EM_GETCHARFORMAT,
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

        SendMessageW(hwndEditor, EM_EXGETSEL, 0, (LPARAM)&range);
        data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) * (range.cpMax-range.cpMin+1));
        SendMessageW(hwndEditor, EM_GETSELTEXT, 0, (LPARAM)data);
        sprintf(buf, "Start = %d, End = %d", range.cpMin, range.cpMax);
        MessageBoxA(hWnd, buf, "Editor", MB_OK);
        MessageBoxW(hWnd, data, wszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_READONLY:
        {
        LONG nStyle = GetWindowLongW(hwndEditor, GWL_STYLE);
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

    case ID_TOGGLE_RULER:
        set_toolbar_state(BANDID_RULER, !is_bar_visible(BANDID_RULER));
        update_window();
        break;

    case ID_DATETIME:
        DialogBoxW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDD_DATETIME), hWnd, datetime_proc);
        break;

    case ID_PARAFORMAT:
        DialogBoxW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDD_PARAFORMAT), hWnd, paraformat_proc);
        break;

    case ID_TABSTOPS:
        DialogBoxW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDD_TABSTOPS), hWnd, tabstops_proc);
        break;

    case ID_ABOUT:
        dialog_about();
        break;

    case ID_VIEWPROPERTIES:
        dialog_viewproperties();
        break;

    case IDC_FONTLIST:
        if (HIWORD(wParam) == CBN_SELENDOK)
        {
            WCHAR buffer[LF_FACESIZE];
            HWND hwndFontList = (HWND)lParam;
            get_comboexlist_selection(hwndFontList, buffer, LF_FACESIZE);
            on_fontlist_modified(buffer);
        }
        break;

    case IDC_SIZELIST:
        if (HIWORD(wParam) == CBN_SELENDOK)
        {
            WCHAR buffer[MAX_STRING_LEN+1];
            HWND hwndSizeList = (HWND)lParam;
            get_comboexlist_selection(hwndSizeList, buffer, MAX_STRING_LEN+1);
            on_sizelist_modified(hwndSizeList, buffer);
        }
        break;

    default:
        SendMessageW(hwndEditor, WM_COMMAND, wParam, lParam);
        break;
    }
    return 0;
}

static LRESULT OnInitPopupMenu( HWND hWnd, WPARAM wParam )
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
    EnableMenuItem(hMenu, ID_EDIT_COPY, (selFrom == selTo) ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(hMenu, ID_EDIT_CUT, (selFrom == selTo) ? MF_GRAYED : MF_ENABLED);

    pf.cbSize = sizeof(PARAFORMAT);
    SendMessageW(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    CheckMenuItem(hMenu, ID_EDIT_READONLY,
      (GetWindowLongW(hwndEditor, GWL_STYLE) & ES_READONLY) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_EDIT_MODIFIED,
      SendMessageW(hwndEditor, EM_GETMODIFY, 0, 0) ? MF_CHECKED : MF_UNCHECKED);
    if (pf.dwMask & PFM_ALIGNMENT)
        nAlignment = pf.wAlignment;
    CheckMenuItem(hMenu, ID_ALIGN_LEFT, (nAlignment == PFA_LEFT) ?  MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_CENTER, (nAlignment == PFA_CENTER) ?  MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_RIGHT, (nAlignment == PFA_RIGHT) ?  MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_BULLET, ((pf.wNumbering == PFN_BULLET) ?  MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(hMenu, ID_EDIT_UNDO, SendMessageW(hwndEditor, EM_CANUNDO, 0, 0) ?
            MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, ID_EDIT_REDO, SendMessageW(hwndEditor, EM_CANREDO, 0, 0) ?
            MF_ENABLED : MF_GRAYED);

    CheckMenuItem(hMenu, ID_TOGGLE_TOOLBAR, is_bar_visible(BANDID_TOOLBAR) ?
            MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, ID_TOGGLE_FORMATBAR, is_bar_visible(BANDID_FORMATBAR) ?
            MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, ID_TOGGLE_STATUSBAR, IsWindowVisible(hwndStatus) ?
            MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(hMenu, ID_TOGGLE_RULER, is_bar_visible(BANDID_RULER) ? MF_CHECKED : MF_UNCHECKED);

    gt.flags = GTL_NUMCHARS;
    gt.codepage = 1200;
    textLength = SendMessageW(hEditorWnd, EM_GETTEXTLENGTHEX, (WPARAM)&gt, 0);
    EnableMenuItem(hMenu, ID_FIND, textLength ? MF_ENABLED : MF_GRAYED);

    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_DATA;

    GetMenuItemInfoW(hMenu, ID_FIND_NEXT, FALSE, &mi);

    EnableMenuItem(hMenu, ID_FIND_NEXT, (textLength && mi.dwItemData) ?  MF_ENABLED : MF_GRAYED);

    EnableMenuItem(hMenu, ID_REPLACE, textLength ? MF_ENABLED : MF_GRAYED);

    return 0;
}

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    int nStatusSize = 0;
    RECT rc;
    HWND hwndEditor = preview_isactive() ? GetDlgItem(hWnd, IDC_PREVIEW) : GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    HWND hRulerWnd = GetDlgItem(hwndReBar, IDC_RULER);
    int rebarHeight = 0;

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
        rebarHeight = SendMessageW(hwndReBar, RB_GETBARHEIGHT, 0, 0);

        MoveWindow(hwndReBar, 0, 0, LOWORD(lParam), rebarHeight, TRUE);
    }
    if (hwndEditor)
    {
        GetClientRect(hWnd, &rc);
        MoveWindow(hwndEditor, 0, rebarHeight, rc.right, rc.bottom-nStatusSize-rebarHeight, TRUE);
    }

    redraw_ruler(hRulerWnd);

    return DefWindowProcW(hWnd, WM_SIZE, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == ID_FINDMSGSTRING)
        return handle_findmsg((LPFINDREPLACEW)lParam);

    switch(msg)
    {
    case WM_CREATE:
        return OnCreate( hWnd );

    case WM_USER:
        return OnUser( hWnd );

    case WM_NOTIFY:
        return OnNotify( hWnd, lParam );

    case WM_COMMAND:
        if(preview_isactive())
        {
            return preview_command( hWnd, wParam );
        }

        return OnCommand( hWnd, wParam, lParam );

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_CLOSE:
        if(preview_isactive())
        {
            preview_exit(hWnd);
        } else if(prompt_save_changes())
        {
            registry_set_options(hMainWnd);
            registry_set_formatopts_all(barState, wordWrap);
            PostQuitMessage(0);
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            SetFocus(GetDlgItem(hWnd, IDC_EDITOR));
        return 0;

    case WM_INITMENUPOPUP:
        return OnInitPopupMenu( hWnd, wParam );

    case WM_SIZE:
        return OnSize( hWnd, wParam, lParam );

    case WM_CONTEXTMENU:
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
        if(!preview_isactive())
            return DefWindowProcW(hWnd, msg, wParam, lParam);

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hOldInstance, LPSTR szCmdParagraph, int nCmdShow)
{
    INITCOMMONCONTROLSEX classes = {8, ICC_BAR_CLASSES|ICC_COOL_CLASSES|ICC_USEREX_CLASSES};
    HACCEL hAccel;
    WNDCLASSEXW wc;
    MSG msg;
    RECT rc;
    UINT_PTR hPrevRulerProc;
    HWND hRulerWnd;
    POINTL EditPoint;
    DWORD bMaximized;
    static const WCHAR wszAccelTable[] = {'M','A','I','N','A','C','C','E','L',
                                          'T','A','B','L','E','\0'};

    InitCommonControlsEx(&classes);

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
            SetProcessDefaultLayout(LAYOUT_RTL);
            break;

        default:
            break;
    }

    hAccel = LoadAcceleratorsW(hInstance, wszAccelTable);

    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WORDPAD));
    wc.hIconSm = LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_WORDPAD), IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    wc.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDM_MAINMENU);
    wc.lpszClassName = wszMainWndClass;
    RegisterClassExW(&wc);

    wc.style = 0;
    wc.lpfnWndProc = preview_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hIconSm = NULL;
    wc.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_IBEAM);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = wszPreviewWndClass;
    RegisterClassExW(&wc);

    registry_read_winrect(&rc);
    hMainWnd = CreateWindowExW(0, wszMainWndClass, wszAppTitle, WS_CLIPCHILDREN|WS_OVERLAPPEDWINDOW,
      rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, NULL, NULL, hInstance, NULL);
    registry_read_maximized(&bMaximized);
    if ((nCmdShow == SW_SHOWNORMAL || nCmdShow == SW_SHOWDEFAULT)
	     && bMaximized)
        nCmdShow = SW_SHOWMAXIMIZED;
    ShowWindow(hMainWnd, nCmdShow);

    set_caption(NULL);
    set_bar_states();
    set_fileformat(SF_RTF);
    hColorPopupMenu = LoadMenuW(hInstance, MAKEINTRESOURCEW(IDM_COLOR_POPUP));
    get_default_printer_opts();
    target_device(hMainWnd, wordWrap[reg_formatindex(fileFormat)]);

    hRulerWnd = GetDlgItem(GetDlgItem(hMainWnd, IDC_REBAR), IDC_RULER);
    SendMessageW(GetDlgItem(hMainWnd, IDC_EDITOR), EM_POSFROMCHAR, (WPARAM)&EditPoint, 0);
    hPrevRulerProc = SetWindowLongPtrW(hRulerWnd, GWLP_WNDPROC, (UINT_PTR)ruler_proc);
    SendMessageW(hRulerWnd, WM_USER, (WPARAM)&EditPoint, hPrevRulerProc);

    HandleCommandLine(GetCommandLineW());

    while(GetMessageW(&msg,0,0,0))
    {
        if (IsDialogMessageW(hFindWnd, &msg))
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
