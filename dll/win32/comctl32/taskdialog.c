/*
 * Task dialog control
 *
 * Copyright 2017 Fabian Maurer
 * Copyright 2018 Zhiyi Zhang
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "winerror.h"
#include "comctl32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskdialog);

static const UINT DIALOG_MIN_WIDTH = 240;
static const UINT DIALOG_SPACING = 5;
static const UINT DIALOG_BUTTON_WIDTH = 50;
static const UINT DIALOG_BUTTON_HEIGHT = 14;
static const UINT DIALOG_EXPANDO_ICON_WIDTH = 10;
static const UINT DIALOG_EXPANDO_ICON_HEIGHT = 10;
static const UINT DIALOG_TIMER_MS = 200;

static const UINT ID_TIMER = 1;

struct taskdialog_info
{
    HWND hwnd;
    const TASKDIALOGCONFIG *taskconfig;
    DWORD last_timer_tick;
    HFONT font;
    HFONT main_instruction_font;
    /* Control handles */
    HWND main_icon;
    HWND main_instruction;
    HWND content;
    HWND progress_bar;
    HWND *radio_buttons;
    INT radio_button_count;
    HWND *command_links;
    INT command_link_count;
    HWND expanded_info;
    HWND expando_button;
    HWND verification_box;
    HWND footer_icon;
    HWND footer_text;
    HWND *buttons;
    INT button_count;
    HWND default_button;
    /* Dialog metrics */
    struct
    {
        LONG x_baseunit;
        LONG y_baseunit;
        LONG h_spacing;
        LONG v_spacing;
    } m;
    INT selected_radio_id;
    BOOL verification_checked;
    BOOL expanded;
    BOOL has_cancel;
    WCHAR *expanded_text;
    WCHAR *collapsed_text;
};

struct button_layout_info
{
    LONG width;
    LONG line;
};

static HRESULT taskdialog_notify(struct taskdialog_info *dialog_info, UINT notification, WPARAM wparam, LPARAM lparam);
static void taskdialog_on_button_click(struct taskdialog_info *dialog_info, HWND hwnd, WORD id);
static void taskdialog_layout(struct taskdialog_info *dialog_info);

static void taskdialog_du_to_px(struct taskdialog_info *dialog_info, LONG *width, LONG *height)
{
    if (width) *width = MulDiv(*width, dialog_info->m.x_baseunit, 4);
    if (height) *height = MulDiv(*height, dialog_info->m.y_baseunit, 8);
}

static void template_write_data(char **ptr, const void *src, unsigned int size)
{
    memcpy(*ptr, src, size);
    *ptr += size;
}

static unsigned int taskdialog_get_reference_rect(const TASKDIALOGCONFIG *taskconfig, RECT *ret)
{
    HMONITOR monitor = MonitorFromWindow(taskconfig->hwndParent ? taskconfig->hwndParent : GetActiveWindow(),
                                         MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info;

    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);

    if ((taskconfig->dwFlags & TDF_POSITION_RELATIVE_TO_WINDOW) && taskconfig->hwndParent)
        GetWindowRect(taskconfig->hwndParent, ret);
    else
        *ret = info.rcWork;

    return info.rcWork.right - info.rcWork.left;
}

static WCHAR *taskdialog_get_exe_name(WCHAR *name, DWORD length)
{
    DWORD len = GetModuleFileNameW(NULL, name, length);
    if (len && len < length)
    {
        WCHAR *p;
        if ((p = wcsrchr(name, '/'))) name = p + 1;
        if ((p = wcsrchr(name, '\\'))) name = p + 1;
        return name;
    }
    else
        return NULL;
}

static DLGTEMPLATE *create_taskdialog_template(const TASKDIALOGCONFIG *taskconfig)
{
    unsigned int size, title_size;
    static const WORD fontsize = 0x7fff;
    const WCHAR *titleW = NULL;
    DLGTEMPLATE *template;
    WCHAR pathW[MAX_PATH];
    char *ptr;

    /* Window title */
    if (!taskconfig->pszWindowTitle)
        titleW = taskdialog_get_exe_name(pathW, ARRAY_SIZE(pathW));
    else if (IS_INTRESOURCE(taskconfig->pszWindowTitle))
    {
        if (!LoadStringW(taskconfig->hInstance, LOWORD(taskconfig->pszWindowTitle), (WCHAR *)&titleW, 0))
            titleW = taskdialog_get_exe_name(pathW, ARRAY_SIZE(pathW));
    }
    else
        titleW = taskconfig->pszWindowTitle;
    if (!titleW)
        titleW = L"";
    title_size = (lstrlenW(titleW) + 1) * sizeof(WCHAR);

    size = sizeof(DLGTEMPLATE) + 2 * sizeof(WORD);
    size += title_size;
    size += 2; /* font size */

    template = Alloc(size);
    if (!template) return NULL;

    template->style = DS_MODALFRAME | DS_SETFONT | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
    if (taskconfig->dwFlags & TDF_CAN_BE_MINIMIZED) template->style |= WS_MINIMIZEBOX;
    if (!(taskconfig->dwFlags & TDF_NO_SET_FOREGROUND)) template->style |= DS_SETFOREGROUND;
    if (taskconfig->dwFlags & TDF_RTL_LAYOUT) template->dwExtendedStyle = WS_EX_LAYOUTRTL | WS_EX_RIGHT | WS_EX_RTLREADING;

    ptr = (char *)(template + 1);
    ptr += 2; /* menu */
    ptr += 2; /* class */
    template_write_data(&ptr, titleW, title_size);
    template_write_data(&ptr, &fontsize, sizeof(fontsize));

    return template;
}

static HWND taskdialog_find_button(HWND *buttons, INT count, INT id)
{
    INT button_id;
    INT i;

    for (i = 0; i < count; i++)
    {
        button_id = GetWindowLongW(buttons[i], GWLP_ID);
        if (button_id == id) return buttons[i];
    }

    return NULL;
}

static void taskdialog_enable_button(const struct taskdialog_info *dialog_info, INT id, BOOL enable)
{
    HWND hwnd = taskdialog_find_button(dialog_info->command_links, dialog_info->command_link_count, id);
    if (!hwnd) hwnd = taskdialog_find_button(dialog_info->buttons, dialog_info->button_count, id);
    if (hwnd) EnableWindow(hwnd, enable);
}

static void taskdialog_click_button(struct taskdialog_info *dialog_info, INT id)
{
    if (taskdialog_notify(dialog_info, TDN_BUTTON_CLICKED, id, 0) == S_OK) EndDialog(dialog_info->hwnd, id);
}

static void taskdialog_button_set_shield(const struct taskdialog_info *dialog_info, INT id, BOOL elevate)
{
    HWND hwnd = taskdialog_find_button(dialog_info->command_links, dialog_info->command_link_count, id);
    if (!hwnd) hwnd = taskdialog_find_button(dialog_info->buttons, dialog_info->button_count, id);
    if (hwnd) SendMessageW(hwnd, BCM_SETSHIELD, 0, elevate);
}

static void taskdialog_enable_radio_button(const struct taskdialog_info *dialog_info, INT id, BOOL enable)
{
    HWND hwnd = taskdialog_find_button(dialog_info->radio_buttons, dialog_info->radio_button_count, id);
    if (hwnd) EnableWindow(hwnd, enable);
}

static void taskdialog_click_radio_button(const struct taskdialog_info *dialog_info, INT id)
{
    HWND hwnd = taskdialog_find_button(dialog_info->radio_buttons, dialog_info->radio_button_count, id);
    if (hwnd) SendMessageW(hwnd, BM_CLICK, 0, 0);
}

static HRESULT taskdialog_notify(struct taskdialog_info *dialog_info, UINT notification, WPARAM wparam, LPARAM lparam)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    return taskconfig->pfCallback
               ? taskconfig->pfCallback(dialog_info->hwnd, notification, wparam, lparam, taskconfig->lpCallbackData)
               : S_OK;
}

static void taskdialog_move_controls_vertically(HWND parent, HWND *controls, INT count, INT offset)
{
    RECT rect;
    POINT pt;
    INT i;

    for (i = 0; i < count; i++)
    {
        if (!controls[i]) continue;

        GetWindowRect(controls[i], &rect);
        pt.x = rect.left;
        pt.y = rect.top;
        MapWindowPoints(HWND_DESKTOP, parent, &pt, 1);
        SetWindowPos(controls[i], 0, pt.x, pt.y + offset, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

static void taskdialog_toggle_expando_control(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    const WCHAR *text;
    RECT info_rect, rect;
    INT height, offset;

    dialog_info->expanded = !dialog_info->expanded;
    text = dialog_info->expanded ? dialog_info->expanded_text : dialog_info->collapsed_text;
    SendMessageW(dialog_info->expando_button, WM_SETTEXT, 0, (LPARAM)text);
    ShowWindow(dialog_info->expanded_info, dialog_info->expanded ? SW_SHOWDEFAULT : SW_HIDE);

    GetWindowRect(dialog_info->expanded_info, &info_rect);
    /* If expanded information starts up not expanded, call taskdialog_layout()
     * to to set size for expanded information control at least once */
    if (IsRectEmpty(&info_rect))
    {
        taskdialog_layout(dialog_info);
        return;
    }
    height = info_rect.bottom - info_rect.top + dialog_info->m.v_spacing;
    offset = dialog_info->expanded ? height : -height;

    /* Update vertical layout, move all controls after expanded information */
    /* Move dialog */
    GetWindowRect(dialog_info->hwnd, &rect);
    SetWindowPos(dialog_info->hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top + offset,
                 SWP_NOMOVE | SWP_NOZORDER);
    /* Move controls */
    if (!(taskconfig->dwFlags & TDF_EXPAND_FOOTER_AREA))
    {
        taskdialog_move_controls_vertically(dialog_info->hwnd, &dialog_info->progress_bar, 1, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, &dialog_info->expando_button, 1, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, &dialog_info->verification_box, 1, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, &dialog_info->footer_icon, 1, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, &dialog_info->footer_text, 1, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, dialog_info->buttons, dialog_info->button_count, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, dialog_info->radio_buttons,
                                            dialog_info->radio_button_count, offset);
        taskdialog_move_controls_vertically(dialog_info->hwnd, dialog_info->command_links,
                                            dialog_info->command_link_count, offset);
    }
}

static void taskdialog_on_button_click(struct taskdialog_info *dialog_info, HWND hwnd, WORD id)
{
    INT command_id;
    HWND button, radio_button;

    /* Prefer the id from hwnd because the id from WM_COMMAND is truncated to WORD */
    command_id = hwnd ? GetWindowLongW(hwnd, GWLP_ID) : id;

    if (hwnd && hwnd == dialog_info->expando_button)
    {
        taskdialog_toggle_expando_control(dialog_info);
        taskdialog_notify(dialog_info, TDN_EXPANDO_BUTTON_CLICKED, dialog_info->expanded, 0);
        return;
    }

    if (hwnd && hwnd == dialog_info->verification_box)
    {
        dialog_info->verification_checked = !dialog_info->verification_checked;
        taskdialog_notify(dialog_info, TDN_VERIFICATION_CLICKED, dialog_info->verification_checked, 0);
        return;
    }

    radio_button = taskdialog_find_button(dialog_info->radio_buttons, dialog_info->radio_button_count, command_id);
    if (radio_button)
    {
        dialog_info->selected_radio_id = command_id;
        taskdialog_notify(dialog_info, TDN_RADIO_BUTTON_CLICKED, command_id, 0);
        return;
    }

    button = taskdialog_find_button(dialog_info->command_links, dialog_info->command_link_count, command_id);
    if (!button) button = taskdialog_find_button(dialog_info->buttons, dialog_info->button_count, command_id);
    if (!button && command_id == IDOK)
    {
        button = dialog_info->command_link_count > 0 ? dialog_info->command_links[0] : dialog_info->buttons[0];
        command_id = GetWindowLongW(button, GWLP_ID);
    }

    if (button && taskdialog_notify(dialog_info, TDN_BUTTON_CLICKED, command_id, 0) == S_OK)
        EndDialog(dialog_info->hwnd, command_id);
}

static WCHAR *taskdialog_gettext(struct taskdialog_info *dialog_info, BOOL user_resource, const WCHAR *text)
{
    const WCHAR *textW = NULL;
    INT length;
    WCHAR *ret;

    if (IS_INTRESOURCE(text))
    {
        if (!(length = LoadStringW(user_resource ? dialog_info->taskconfig->hInstance : COMCTL32_hModule,
                                   (UINT_PTR)text, (WCHAR *)&textW, 0)))
            return NULL;
    }
    else
    {
        textW = text;
        length = lstrlenW(textW);
    }

    ret = Alloc((length + 1) * sizeof(WCHAR));
    if (ret) memcpy(ret, textW, length * sizeof(WCHAR));

    return ret;
}

static BOOL taskdialog_hyperlink_enabled(struct taskdialog_info *dialog_info)
{
    return dialog_info->taskconfig->dwFlags & TDF_ENABLE_HYPERLINKS;
}

static BOOL taskdialog_use_command_link(struct taskdialog_info *dialog_info)
{
    return dialog_info->taskconfig->dwFlags & (TDF_USE_COMMAND_LINKS | TDF_USE_COMMAND_LINKS_NO_ICON);
}

static void taskdialog_get_label_size(struct taskdialog_info *dialog_info, HWND hwnd, LONG max_width, SIZE *size,
                                      BOOL syslink)
{
    DWORD style = DT_EXPANDTABS | DT_CALCRECT | DT_WORDBREAK;
    HFONT hfont, old_hfont;
    HDC hdc;
    RECT rect = {0};
    WCHAR *text;
    INT text_length;

    if (syslink)
    {
        SendMessageW(hwnd, LM_GETIDEALSIZE, max_width, (LPARAM)size);
        return;
    }

    if (dialog_info->taskconfig->dwFlags & TDF_RTL_LAYOUT)
        style |= DT_RIGHT | DT_RTLREADING;
    else
        style |= DT_LEFT;

    hfont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
    text_length = GetWindowTextLengthW(hwnd);
    text = Alloc((text_length + 1) * sizeof(WCHAR));
    if (!text)
    {
        size->cx = 0;
        size->cy = 0;
        return;
    }
    GetWindowTextW(hwnd, text, text_length + 1);
    hdc = GetDC(hwnd);
    old_hfont = SelectObject(hdc, hfont);
    rect.right = max_width;
    size->cy = DrawTextW(hdc, text, text_length, &rect, style);
    size->cx = min(max_width, rect.right - rect.left);
    if (old_hfont) SelectObject(hdc, old_hfont);
    ReleaseDC(hwnd, hdc);
    Free(text);
}

static void taskdialog_get_button_size(HWND hwnd, LONG max_width, SIZE *size)
{
    size->cx = max_width;
    size->cy = 0;
    SendMessageW(hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)size);
}

static void taskdialog_get_expando_size(struct taskdialog_info *dialog_info, HWND hwnd, SIZE *size)
{
    DWORD style = DT_EXPANDTABS | DT_CALCRECT | DT_WORDBREAK;
    HFONT hfont, old_hfont;
    HDC hdc;
    RECT rect = {0};
    LONG icon_width, icon_height;
    INT text_offset;
    LONG max_width, max_text_height;

    hdc = GetDC(hwnd);
    hfont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
    old_hfont = SelectObject(hdc, hfont);

    icon_width = DIALOG_EXPANDO_ICON_WIDTH;
    icon_height = DIALOG_EXPANDO_ICON_HEIGHT;
    taskdialog_du_to_px(dialog_info, &icon_width, &icon_height);

    GetCharWidthW(hdc, '0', '0', &text_offset);
    text_offset /= 2;

    if (dialog_info->taskconfig->dwFlags & TDF_RTL_LAYOUT)
        style |= DT_RIGHT | DT_RTLREADING;
    else
        style |= DT_LEFT;

    max_width = DIALOG_MIN_WIDTH / 2;
    taskdialog_du_to_px(dialog_info, &max_width, NULL);

    rect.right = max_width - icon_width - text_offset;
    max_text_height = DrawTextW(hdc, dialog_info->expanded_text, -1, &rect, style);
    size->cy = max(max_text_height, icon_height);
    size->cx = rect.right - rect.left;

    rect.right = max_width - icon_width - text_offset;
    max_text_height = DrawTextW(hdc, dialog_info->collapsed_text, -1, &rect, style);
    size->cy = max(size->cy, max_text_height);
    size->cx = max(size->cx, rect.right - rect.left);
    size->cx = min(size->cx, max_width);

    if (old_hfont) SelectObject(hdc, old_hfont);
    ReleaseDC(hwnd, hdc);
}

static ULONG_PTR taskdialog_get_standard_icon(LPCWSTR icon)
{
    if (icon == TD_WARNING_ICON)
        return IDI_WARNING;
    else if (icon == TD_ERROR_ICON)
        return IDI_ERROR;
    else if (icon == TD_INFORMATION_ICON)
        return IDI_INFORMATION;
    else if (icon == TD_SHIELD_ICON)
        return IDI_SHIELD;
    else
        return (ULONG_PTR)icon;
}

static void taskdialog_set_icon(struct taskdialog_info *dialog_info, INT element, HICON icon)
{
    DWORD flags = dialog_info->taskconfig->dwFlags;
    INT cx = 0, cy = 0;
    HICON hicon;

    if (!icon) return;

    if (((flags & TDF_USE_HICON_MAIN) && element == TDIE_ICON_MAIN)
        || ((flags & TDF_USE_HICON_FOOTER) && element == TDIE_ICON_FOOTER))
        hicon = icon;
    else
    {
        if (element == TDIE_ICON_FOOTER)
        {
            cx = GetSystemMetrics(SM_CXSMICON);
            cy = GetSystemMetrics(SM_CYSMICON);
        }
        hicon = LoadImageW(dialog_info->taskconfig->hInstance, (LPCWSTR)icon, IMAGE_ICON, cx, cy, LR_SHARED | LR_DEFAULTSIZE);
        if (!hicon)
            hicon = LoadImageW(NULL, (LPCWSTR)taskdialog_get_standard_icon((LPCWSTR)icon), IMAGE_ICON, cx, cy,
                               LR_SHARED | LR_DEFAULTSIZE);
    }

    if (!hicon) return;

    if (element == TDIE_ICON_MAIN)
    {
        SendMessageW(dialog_info->hwnd, WM_SETICON, (WPARAM)ICON_BIG, (LPARAM)hicon);
        SendMessageW(dialog_info->main_icon, STM_SETICON, (WPARAM)hicon, 0);
    }
    else if (element == TDIE_ICON_FOOTER)
        SendMessageW(dialog_info->footer_icon, STM_SETICON, (WPARAM)hicon, 0);
}

static void taskdialog_set_element_text(struct taskdialog_info *dialog_info, TASKDIALOG_ELEMENTS element,
                                        const WCHAR *text)
{
    HWND hwnd = NULL;
    WCHAR *textW;

    if (element == TDE_CONTENT)
        hwnd = dialog_info->content;
    else if (element == TDE_EXPANDED_INFORMATION)
        hwnd = dialog_info->expanded_info;
    else if (element == TDE_FOOTER)
        hwnd = dialog_info->footer_text;
    else if (element == TDE_MAIN_INSTRUCTION)
        hwnd = dialog_info->main_instruction;

    if (!hwnd) return;

    textW = taskdialog_gettext(dialog_info, TRUE, text);
    SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)textW);
    Free(textW);
}

static void taskdialog_check_default_radio_buttons(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    HWND default_button;

    if (!dialog_info->radio_button_count) return;

    default_button = taskdialog_find_button(dialog_info->radio_buttons, dialog_info->radio_button_count,
                                            taskconfig->nDefaultRadioButton);

    if (!default_button && !(taskconfig->dwFlags & TDF_NO_DEFAULT_RADIO_BUTTON))
        default_button = dialog_info->radio_buttons[0];

    if (default_button)
    {
        SendMessageW(default_button, BM_SETCHECK, BST_CHECKED, 0);
        taskdialog_on_button_click(dialog_info, default_button, 0);
    }
}

static void taskdialog_add_main_icon(struct taskdialog_info *dialog_info)
{
    if (!dialog_info->taskconfig->hMainIcon) return;

    dialog_info->main_icon =
        CreateWindowW(WC_STATICW, NULL, WS_CHILD | WS_VISIBLE | SS_ICON, 0, 0, 0, 0, dialog_info->hwnd, NULL, 0, NULL);
    taskdialog_set_icon(dialog_info, TDIE_ICON_MAIN, dialog_info->taskconfig->hMainIcon);
}

static HWND taskdialog_create_label(struct taskdialog_info *dialog_info, const WCHAR *text, HFONT font, BOOL syslink)
{
    WCHAR *textW;
    HWND hwnd;
    const WCHAR *class;
    DWORD style = WS_CHILD | WS_VISIBLE;

    if (!text) return NULL;

    class = syslink ? WC_LINK : WC_STATICW;
    if (syslink) style |= WS_TABSTOP;
    textW = taskdialog_gettext(dialog_info, TRUE, text);
    hwnd = CreateWindowW(class, textW, style, 0, 0, 0, 0, dialog_info->hwnd, NULL, 0, NULL);
    Free(textW);

    SendMessageW(hwnd, WM_SETFONT, (WPARAM)font, 0);
    return hwnd;
}

static void taskdialog_add_main_instruction(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    NONCLIENTMETRICSW ncm;

    if (!taskconfig->pszMainInstruction) return;

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    /* 1.25 times the height */
    ncm.lfMessageFont.lfHeight = ncm.lfMessageFont.lfHeight * 5 / 4;
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    dialog_info->main_instruction_font = CreateFontIndirectW(&ncm.lfMessageFont);

    dialog_info->main_instruction =
        taskdialog_create_label(dialog_info, taskconfig->pszMainInstruction, dialog_info->main_instruction_font, FALSE);
}

static void taskdialog_add_content(struct taskdialog_info *dialog_info)
{
    dialog_info->content = taskdialog_create_label(dialog_info, dialog_info->taskconfig->pszContent, dialog_info->font,
                                                   taskdialog_hyperlink_enabled(dialog_info));
}

static void taskdialog_add_progress_bar(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    DWORD style = PBS_SMOOTH | PBS_SMOOTHREVERSE | WS_CHILD | WS_VISIBLE;

    if (!(taskconfig->dwFlags & (TDF_SHOW_PROGRESS_BAR | TDF_SHOW_MARQUEE_PROGRESS_BAR))) return;
    if (taskconfig->dwFlags & TDF_SHOW_MARQUEE_PROGRESS_BAR) style |= PBS_MARQUEE;
    dialog_info->progress_bar =
        CreateWindowW(PROGRESS_CLASSW, NULL, style, 0, 0, 0, 0, dialog_info->hwnd, NULL, 0, NULL);
}

static void taskdialog_add_radio_buttons(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    static const DWORD style = BS_AUTORADIOBUTTON | BS_MULTILINE | BS_TOP | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
    WCHAR *textW;
    INT i;

    if (!taskconfig->cRadioButtons || !taskconfig->pRadioButtons) return;

    dialog_info->radio_buttons = Alloc(taskconfig->cRadioButtons * sizeof(*dialog_info->radio_buttons));
    if (!dialog_info->radio_buttons) return;

    dialog_info->radio_button_count = taskconfig->cRadioButtons;
    for (i = 0; i < dialog_info->radio_button_count; i++)
    {
        textW = taskdialog_gettext(dialog_info, TRUE, taskconfig->pRadioButtons[i].pszButtonText);
        dialog_info->radio_buttons[i] =
            CreateWindowW(WC_BUTTONW, textW, i == 0 ? style | WS_GROUP : style, 0, 0, 0, 0, dialog_info->hwnd,
                          LongToHandle(taskconfig->pRadioButtons[i].nButtonID), 0, NULL);
        SendMessageW(dialog_info->radio_buttons[i], WM_SETFONT, (WPARAM)dialog_info->font, 0);
        Free(textW);
    }
}

static void taskdialog_add_command_links(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    DWORD default_style = BS_MULTILINE | BS_LEFT | BS_TOP | WS_CHILD | WS_VISIBLE | WS_TABSTOP, style;
    BOOL is_default;
    WCHAR *textW;
    INT i;

    if (!taskconfig->cButtons || !taskconfig->pButtons || !taskdialog_use_command_link(dialog_info)) return;

    dialog_info->command_links = Alloc(taskconfig->cButtons * sizeof(*dialog_info->command_links));
    if (!dialog_info->command_links) return;

    dialog_info->command_link_count = taskconfig->cButtons;
    for (i = 0; i < dialog_info->command_link_count; i++)
    {
        is_default = taskconfig->pButtons[i].nButtonID == taskconfig->nDefaultButton;
        style = is_default ? default_style | BS_DEFCOMMANDLINK : default_style | BS_COMMANDLINK;
        textW = taskdialog_gettext(dialog_info, TRUE, taskconfig->pButtons[i].pszButtonText);
        dialog_info->command_links[i] = CreateWindowW(WC_BUTTONW, textW, style, 0, 0, 0, 0, dialog_info->hwnd,
                                                      LongToHandle(taskconfig->pButtons[i].nButtonID), 0, NULL);
        SendMessageW(dialog_info->command_links[i], WM_SETFONT, (WPARAM)dialog_info->font, 0);
        Free(textW);

        if (is_default && !dialog_info->default_button) dialog_info->default_button = dialog_info->command_links[i];
    }
}

static void taskdialog_add_expanded_info(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;

    if (!taskconfig->pszExpandedInformation) return;

    dialog_info->expanded = taskconfig->dwFlags & TDF_EXPANDED_BY_DEFAULT;
    dialog_info->expanded_info = taskdialog_create_label(dialog_info, taskconfig->pszExpandedInformation,
                                                         dialog_info->font, taskdialog_hyperlink_enabled(dialog_info));
    ShowWindow(dialog_info->expanded_info, dialog_info->expanded ? SW_SHOWDEFAULT : SW_HIDE);
}

static void taskdialog_add_expando_button(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    const WCHAR *textW;

    if (!taskconfig->pszExpandedInformation) return;

    if (!taskconfig->pszCollapsedControlText && !taskconfig->pszExpandedControlText)
    {
        dialog_info->expanded_text = taskdialog_gettext(dialog_info, FALSE, MAKEINTRESOURCEW(IDS_TD_EXPANDED));
        dialog_info->collapsed_text = taskdialog_gettext(dialog_info, FALSE, MAKEINTRESOURCEW(IDS_TD_COLLAPSED));
    }
    else
    {
        textW = taskconfig->pszExpandedControlText ? taskconfig->pszExpandedControlText
                                                   : taskconfig->pszCollapsedControlText;
        dialog_info->expanded_text = taskdialog_gettext(dialog_info, TRUE, textW);
        textW = taskconfig->pszCollapsedControlText ? taskconfig->pszCollapsedControlText
                                                    : taskconfig->pszExpandedControlText;
        dialog_info->collapsed_text = taskdialog_gettext(dialog_info, TRUE, textW);
    }

    textW = dialog_info->expanded ? dialog_info->expanded_text : dialog_info->collapsed_text;

    dialog_info->expando_button = CreateWindowW(WC_BUTTONW, textW, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0,
                                                0, 0, 0, dialog_info->hwnd, 0, 0, 0);
    SendMessageW(dialog_info->expando_button, WM_SETFONT, (WPARAM)dialog_info->font, 0);
}

static void taskdialog_add_verification_box(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    static const DWORD style = BS_AUTOCHECKBOX | BS_MULTILINE | BS_LEFT | BS_TOP | WS_CHILD | WS_VISIBLE | WS_TABSTOP;
    WCHAR *textW;

    /* TDF_VERIFICATION_FLAG_CHECKED works even if pszVerificationText is not set */
    if (taskconfig->dwFlags & TDF_VERIFICATION_FLAG_CHECKED) dialog_info->verification_checked = TRUE;

    if (!taskconfig->pszVerificationText) return;

    textW = taskdialog_gettext(dialog_info, TRUE, taskconfig->pszVerificationText);
    dialog_info->verification_box = CreateWindowW(WC_BUTTONW, textW, style, 0, 0, 0, 0, dialog_info->hwnd, 0, 0, 0);
    SendMessageW(dialog_info->verification_box, WM_SETFONT, (WPARAM)dialog_info->font, 0);
    Free(textW);

    if (taskconfig->dwFlags & TDF_VERIFICATION_FLAG_CHECKED)
        SendMessageW(dialog_info->verification_box, BM_SETCHECK, BST_CHECKED, 0);
}

static void taskdialog_add_button(struct taskdialog_info *dialog_info, HWND *button, INT_PTR id, const WCHAR *text,
                                  BOOL custom_button)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    WCHAR *textW;

    textW = taskdialog_gettext(dialog_info, custom_button, text);
    *button = CreateWindowW(WC_BUTTONW, textW, WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, dialog_info->hwnd,
                            (HMENU)id, 0, NULL);
    Free(textW);
    SendMessageW(*button, WM_SETFONT, (WPARAM)dialog_info->font, 0);

    if (id == taskconfig->nDefaultButton && !dialog_info->default_button) dialog_info->default_button = *button;
}

static void taskdialog_add_buttons(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    BOOL use_command_links = taskdialog_use_command_link(dialog_info);
    DWORD flags = taskconfig->dwCommonButtons;
    INT count, max_count;

    /* Allocate enough memory for the custom and the default buttons. Maximum 6 default buttons possible. */
    max_count = 6;
    if (!use_command_links && taskconfig->cButtons && taskconfig->pButtons) max_count += taskconfig->cButtons;

    dialog_info->buttons = Alloc(max_count * sizeof(*dialog_info->buttons));
    if (!dialog_info->buttons) return;

    for (count = 0; !use_command_links && count < taskconfig->cButtons; count++)
        taskdialog_add_button(dialog_info, &dialog_info->buttons[count], taskconfig->pButtons[count].nButtonID,
                              taskconfig->pButtons[count].pszButtonText, TRUE);

#define TASKDIALOG_INIT_COMMON_BUTTON(id)                                                                             \
    do                                                                                                                \
    {                                                                                                                 \
        taskdialog_add_button(dialog_info, &dialog_info->buttons[count++], ID##id, MAKEINTRESOURCEW(IDS_BUTTON_##id), \
                              FALSE);                                                                                 \
    } while (0)

    if (flags & TDCBF_OK_BUTTON) TASKDIALOG_INIT_COMMON_BUTTON(OK);
    if (flags & TDCBF_YES_BUTTON) TASKDIALOG_INIT_COMMON_BUTTON(YES);
    if (flags & TDCBF_NO_BUTTON) TASKDIALOG_INIT_COMMON_BUTTON(NO);
    if (flags & TDCBF_RETRY_BUTTON) TASKDIALOG_INIT_COMMON_BUTTON(RETRY);
    if (flags & TDCBF_CANCEL_BUTTON) TASKDIALOG_INIT_COMMON_BUTTON(CANCEL);
    if (flags & TDCBF_CLOSE_BUTTON) TASKDIALOG_INIT_COMMON_BUTTON(CLOSE);

    if (!count && !dialog_info->command_link_count) TASKDIALOG_INIT_COMMON_BUTTON(OK);
#undef TASKDIALOG_INIT_COMMON_BUTTON

    dialog_info->button_count = count;
}

static void taskdialog_add_footer_icon(struct taskdialog_info *dialog_info)
{
    if (!dialog_info->taskconfig->hFooterIcon) return;

    dialog_info->footer_icon =
        CreateWindowW(WC_STATICW, NULL, WS_CHILD | WS_VISIBLE | SS_ICON, 0, 0, 0, 0, dialog_info->hwnd, NULL, 0, 0);
    taskdialog_set_icon(dialog_info, TDIE_ICON_FOOTER, dialog_info->taskconfig->hFooterIcon);
}

static void taskdialog_add_footer_text(struct taskdialog_info *dialog_info)
{
    dialog_info->footer_text = taskdialog_create_label(dialog_info, dialog_info->taskconfig->pszFooter,
                                                       dialog_info->font, taskdialog_hyperlink_enabled(dialog_info));
}

static LONG taskdialog_get_dialog_width(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    BOOL syslink = taskdialog_hyperlink_enabled(dialog_info);
    LONG max_width, main_icon_width, screen_width;
    RECT rect;
    SIZE size;

    screen_width = taskdialog_get_reference_rect(taskconfig, &rect);
    if ((taskconfig->dwFlags & TDF_SIZE_TO_CONTENT) && !taskconfig->cxWidth)
    {
        max_width = DIALOG_MIN_WIDTH;
        taskdialog_du_to_px(dialog_info, &max_width, NULL);
        main_icon_width = dialog_info->m.h_spacing;
        if (dialog_info->main_icon) main_icon_width += GetSystemMetrics(SM_CXICON);
        if (dialog_info->content)
        {
            taskdialog_get_label_size(dialog_info, dialog_info->content, 0, &size, syslink);
            max_width = max(max_width, size.cx + main_icon_width + dialog_info->m.h_spacing * 2);
        }
    }
    else
    {
        max_width = max(taskconfig->cxWidth, DIALOG_MIN_WIDTH);
        taskdialog_du_to_px(dialog_info, &max_width, NULL);
    }
    max_width = min(max_width, screen_width);
    return max_width;
}

static void taskdialog_label_layout(struct taskdialog_info *dialog_info, HWND hwnd, INT start_x, LONG dialog_width,
                                    LONG *dialog_height, BOOL syslink)
{
    LONG x, y, max_width;
    SIZE size;

    if (!hwnd) return;

    x = start_x + dialog_info->m.h_spacing;
    y = *dialog_height + dialog_info->m.v_spacing;
    max_width = dialog_width - x - dialog_info->m.h_spacing;
    taskdialog_get_label_size(dialog_info, hwnd, max_width, &size, syslink);
    SetWindowPos(hwnd, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
    *dialog_height = y + size.cy;
}

static void taskdialog_layout(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    BOOL syslink = taskdialog_hyperlink_enabled(dialog_info);
    static BOOL first_time = TRUE;
    RECT ref_rect;
    LONG dialog_width, dialog_height = 0;
    LONG h_spacing, v_spacing;
    LONG main_icon_right, main_icon_bottom;
    LONG expando_right, expando_bottom;
    struct button_layout_info *button_layout_infos;
    LONG button_min_width, button_height;
    LONG *line_widths, line_count, align;
    LONG footer_icon_right, footer_icon_bottom;
    LONG x, y;
    SIZE size;
    INT i;

    taskdialog_get_reference_rect(dialog_info->taskconfig, &ref_rect);
    dialog_width = taskdialog_get_dialog_width(dialog_info);

    h_spacing = dialog_info->m.h_spacing;
    v_spacing = dialog_info->m.v_spacing;

    /* Main icon */
    main_icon_right = 0;
    main_icon_bottom = 0;
    if (dialog_info->main_icon)
    {
        x = h_spacing;
        y = dialog_height + v_spacing;
        size.cx = GetSystemMetrics(SM_CXICON);
        size.cy = GetSystemMetrics(SM_CYICON);
        SetWindowPos(dialog_info->main_icon, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        main_icon_right = x + size.cx;
        main_icon_bottom = y + size.cy;
    }

    /* Main instruction */
    taskdialog_label_layout(dialog_info, dialog_info->main_instruction, main_icon_right, dialog_width, &dialog_height,
                            FALSE);

    /* Content */
    taskdialog_label_layout(dialog_info, dialog_info->content, main_icon_right, dialog_width, &dialog_height, syslink);

    /* Expanded information */
    if (!(taskconfig->dwFlags & TDF_EXPAND_FOOTER_AREA) && dialog_info->expanded)
        taskdialog_label_layout(dialog_info, dialog_info->expanded_info, main_icon_right, dialog_width, &dialog_height,
                                syslink);

    /* Progress bar */
    if (dialog_info->progress_bar)
    {
        x = main_icon_right + h_spacing;
        y = dialog_height + v_spacing;
        size.cx = dialog_width - x - h_spacing;
        size.cy = GetSystemMetrics(SM_CYVSCROLL);
        SetWindowPos(dialog_info->progress_bar, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        dialog_height = y + size.cy;
    }

    /* Radio buttons */
    for (i = 0; i < dialog_info->radio_button_count; i++)
    {
        x = main_icon_right + h_spacing;
        y = dialog_height + v_spacing;
        taskdialog_get_button_size(dialog_info->radio_buttons[i], dialog_width - x - h_spacing, &size);
        size.cx = dialog_width - x - h_spacing;
        SetWindowPos(dialog_info->radio_buttons[i], 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        dialog_height = y + size.cy;
    }

    /* Command links */
    for (i = 0; i < dialog_info->command_link_count; i++)
    {
        x = main_icon_right + h_spacing;
        y = dialog_height;
        /* Only add spacing for the first command links. There is no vertical spacing between command links */
        if (!i)
            y += v_spacing;
        taskdialog_get_button_size(dialog_info->command_links[i], dialog_width - x - h_spacing, &size);
        size.cx = dialog_width - x - h_spacing;
        /* Add spacing */
        size.cy += 4;
        SetWindowPos(dialog_info->command_links[i], 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        dialog_height = y + size.cy;
    }

    dialog_height = max(dialog_height, main_icon_bottom);

    expando_right = 0;
    expando_bottom = dialog_height;
    /* Expando control */
    if (dialog_info->expando_button)
    {
        x = h_spacing;
        y = dialog_height + v_spacing;
        taskdialog_get_expando_size(dialog_info, dialog_info->expando_button, &size);
        SetWindowPos(dialog_info->expando_button, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        expando_right = x + size.cx;
        expando_bottom = y + size.cy;
    }

    /* Verification box */
    if (dialog_info->verification_box)
    {
        x = h_spacing;
        y = expando_bottom + v_spacing;
        size.cx = DIALOG_MIN_WIDTH / 2;
        taskdialog_du_to_px(dialog_info, &size.cx, NULL);
        taskdialog_get_button_size(dialog_info->verification_box, size.cx, &size);
        SetWindowPos(dialog_info->verification_box, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        expando_right = max(expando_right, x + size.cx);
        expando_bottom = y + size.cy;
    }

    /* Common and custom buttons */
    button_layout_infos = Alloc(dialog_info->button_count * sizeof(*button_layout_infos));
    line_widths = Alloc(dialog_info->button_count * sizeof(*line_widths));

    button_min_width = DIALOG_BUTTON_WIDTH;
    button_height = DIALOG_BUTTON_HEIGHT;
    taskdialog_du_to_px(dialog_info, &button_min_width, &button_height);
    for (i = 0; i < dialog_info->button_count; i++)
    {
        taskdialog_get_button_size(dialog_info->buttons[i], dialog_width - expando_right - h_spacing * 2, &size);
        button_layout_infos[i].width = max(size.cx, button_min_width);
    }

    /* Separate buttons into lines */
    x = expando_right + h_spacing;
    for (i = 0, line_count = 0; i < dialog_info->button_count; i++)
    {
        button_layout_infos[i].line = line_count;
        x += button_layout_infos[i].width + h_spacing;
        line_widths[line_count] += button_layout_infos[i].width + h_spacing;

        if ((i + 1 < dialog_info->button_count) && (x + button_layout_infos[i + 1].width + h_spacing >= dialog_width))
        {
            x = expando_right + h_spacing;
            line_count++;
        }
    }
    line_count++;

    /* Try to balance lines so they are about the same size */
    for (i = 1; i < line_count - 1; i++)
    {
        int diff_now = abs(line_widths[i] - line_widths[i - 1]);
        unsigned int j, last_button = 0;
        int diff_changed;

        for (j = 0; j < dialog_info->button_count; j++)
            if (button_layout_infos[j].line == i - 1) last_button = j;

        /* Difference in length of both lines if we wrapped the last button from the last line into this one */
        diff_changed = abs(2 * button_layout_infos[last_button].width + line_widths[i] - line_widths[i - 1]);

        if (diff_changed < diff_now)
        {
            button_layout_infos[last_button].line = i;
            line_widths[i] += button_layout_infos[last_button].width;
            line_widths[i - 1] -= button_layout_infos[last_button].width;
        }
    }

    /* Calculate left alignment so all lines are as far right as possible. */
    align = dialog_width - h_spacing;
    for (i = 0; i < line_count; i++)
    {
        int new_alignment = dialog_width - line_widths[i];
        if (new_alignment < align) align = new_alignment;
    }

    /* Now that we got them all positioned, move all buttons */
    x = align;
    size.cy = button_height;
    for (i = 0; i < dialog_info->button_count; i++)
    {
        /* New line */
        if (i > 0 && button_layout_infos[i].line != button_layout_infos[i - 1].line)
        {
            x = align;
            dialog_height += size.cy + v_spacing;
        }

        y = dialog_height + v_spacing;
        size.cx = button_layout_infos[i].width;
        SetWindowPos(dialog_info->buttons[i], 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        x += button_layout_infos[i].width + h_spacing;
    }

    /* Add height for last row button and spacing */
    dialog_height += size.cy + v_spacing;
    dialog_height = max(dialog_height, expando_bottom);

    Free(button_layout_infos);
    Free(line_widths);

    /* Footer icon */
    footer_icon_right = 0;
    footer_icon_bottom = dialog_height;
    if (dialog_info->footer_icon)
    {
        x = h_spacing;
        y = dialog_height + v_spacing;
        size.cx = GetSystemMetrics(SM_CXSMICON);
        size.cy = GetSystemMetrics(SM_CYSMICON);
        SetWindowPos(dialog_info->footer_icon, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        footer_icon_right = x + size.cx;
        footer_icon_bottom = y + size.cy;
    }

    /* Footer text */
    taskdialog_label_layout(dialog_info, dialog_info->footer_text, footer_icon_right, dialog_width, &dialog_height,
                            syslink);
    dialog_height = max(dialog_height, footer_icon_bottom);

    /* Expanded information */
    if ((taskconfig->dwFlags & TDF_EXPAND_FOOTER_AREA) && dialog_info->expanded)
        taskdialog_label_layout(dialog_info, dialog_info->expanded_info, 0, dialog_width, &dialog_height, syslink);

    /* Add height for spacing, title height and frame height */
    dialog_height += v_spacing;
    dialog_height += GetSystemMetrics(SM_CYCAPTION);
    dialog_height += GetSystemMetrics(SM_CXDLGFRAME);

    if (first_time)
    {
        x = (ref_rect.left + ref_rect.right - dialog_width) / 2;
        y = (ref_rect.top + ref_rect.bottom - dialog_height) / 2;
        SetWindowPos(dialog_info->hwnd, 0, x, y, dialog_width, dialog_height, SWP_NOZORDER);
        first_time = FALSE;
    }
    else
        SetWindowPos(dialog_info->hwnd, 0, 0, 0, dialog_width, dialog_height, SWP_NOMOVE | SWP_NOZORDER);
}

static void taskdialog_draw_expando_control(struct taskdialog_info *dialog_info, LPDRAWITEMSTRUCT dis)
{
    HWND hwnd;
    HDC hdc;
    RECT rect = {0};
    WCHAR *text;
    LONG icon_width, icon_height;
    INT text_offset;
    UINT style = DFCS_FLAT;
    BOOL draw_focus;

    hdc = dis->hDC;
    hwnd = dis->hwndItem;

    SendMessageW(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);

    icon_width = DIALOG_EXPANDO_ICON_WIDTH;
    icon_height = DIALOG_EXPANDO_ICON_HEIGHT;
    taskdialog_du_to_px(dialog_info, &icon_width, &icon_height);
    rect.right = icon_width;
    rect.bottom = icon_height;
    style |= dialog_info->expanded ? DFCS_SCROLLUP : DFCS_SCROLLDOWN;
    DrawFrameControl(hdc, &rect, DFC_SCROLL, style);

    GetCharWidthW(hdc, '0', '0', &text_offset);
    text_offset /= 2;

    rect = dis->rcItem;
    rect.left += icon_width + text_offset;
    text = dialog_info->expanded ? dialog_info->expanded_text : dialog_info->collapsed_text;
    DrawTextW(hdc, text, -1, &rect, DT_WORDBREAK | DT_END_ELLIPSIS | DT_EXPANDTABS);

    draw_focus = (dis->itemState & ODS_FOCUS) && !(dis->itemState & ODS_NOFOCUSRECT);
    if(draw_focus) DrawFocusRect(hdc, &rect);
}

static void taskdialog_init(struct taskdialog_info *dialog_info, HWND hwnd)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    NONCLIENTMETRICSW ncm;
    HDC hdc;
    INT id;

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    memset(dialog_info, 0, sizeof(*dialog_info));
    dialog_info->taskconfig = taskconfig;
    dialog_info->hwnd = hwnd;
    dialog_info->font = CreateFontIndirectW(&ncm.lfMessageFont);

    hdc = GetDC(dialog_info->hwnd);
    SelectObject(hdc, dialog_info->font);
    dialog_info->m.x_baseunit = GdiGetCharDimensions(hdc, NULL, &dialog_info->m.y_baseunit);
    ReleaseDC(dialog_info->hwnd, hdc);

    dialog_info->m.h_spacing = DIALOG_SPACING;
    dialog_info->m.v_spacing = DIALOG_SPACING;
    taskdialog_du_to_px(dialog_info, &dialog_info->m.h_spacing, &dialog_info->m.v_spacing);

    if (taskconfig->dwFlags & TDF_CALLBACK_TIMER)
    {
        SetTimer(hwnd, ID_TIMER, DIALOG_TIMER_MS, NULL);
        dialog_info->last_timer_tick = GetTickCount();
    }

    taskdialog_add_main_icon(dialog_info);
    taskdialog_add_main_instruction(dialog_info);
    taskdialog_add_content(dialog_info);
    taskdialog_add_expanded_info(dialog_info);
    taskdialog_add_progress_bar(dialog_info);
    taskdialog_add_radio_buttons(dialog_info);
    taskdialog_add_command_links(dialog_info);
    taskdialog_add_expando_button(dialog_info);
    taskdialog_add_verification_box(dialog_info);
    taskdialog_add_buttons(dialog_info);
    taskdialog_add_footer_icon(dialog_info);
    taskdialog_add_footer_text(dialog_info);

    /* Set default button */
    if (!dialog_info->default_button && dialog_info->command_links)
        dialog_info->default_button = dialog_info->command_links[0];
    if (!dialog_info->default_button) dialog_info->default_button = dialog_info->buttons[0];
    SendMessageW(dialog_info->hwnd, WM_NEXTDLGCTL, (WPARAM)dialog_info->default_button, TRUE);
    id = GetWindowLongW(dialog_info->default_button, GWLP_ID);
    SendMessageW(dialog_info->hwnd, DM_SETDEFID, id, 0);

    dialog_info->has_cancel =
        (taskconfig->dwFlags & TDF_ALLOW_DIALOG_CANCELLATION)
        || taskdialog_find_button(dialog_info->command_links, dialog_info->command_link_count, IDCANCEL)
        || taskdialog_find_button(dialog_info->buttons, dialog_info->button_count, IDCANCEL);

    if (!dialog_info->has_cancel) DeleteMenu(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_BYCOMMAND);

    taskdialog_layout(dialog_info);
}

static BOOL CALLBACK takdialog_destroy_control(HWND hwnd, LPARAM lParam)
{
    DestroyWindow(hwnd);
    return TRUE;
}

static void taskdialog_destroy(struct taskdialog_info *dialog_info)
{
    EnumChildWindows(dialog_info->hwnd, takdialog_destroy_control, 0);

    if (dialog_info->taskconfig->dwFlags & TDF_CALLBACK_TIMER) KillTimer(dialog_info->hwnd, ID_TIMER);
    if (dialog_info->font) DeleteObject(dialog_info->font);
    if (dialog_info->main_instruction_font) DeleteObject(dialog_info->main_instruction_font);
    Free(dialog_info->buttons);
    Free(dialog_info->radio_buttons);
    Free(dialog_info->command_links);
    Free(dialog_info->expanded_text);
    Free(dialog_info->collapsed_text);
}

static INT_PTR CALLBACK taskdialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR taskdialog_info_propnameW[] = L"TaskDialogInfo";
    struct taskdialog_info *dialog_info;
    LRESULT result;

    TRACE("hwnd %p, msg 0x%04x, wparam %Ix, lparam %Ix\n", hwnd, msg, wParam, lParam);

    if (msg != WM_INITDIALOG)
        dialog_info = GetPropW(hwnd, taskdialog_info_propnameW);

    switch (msg)
    {
        case TDM_NAVIGATE_PAGE:
            dialog_info->taskconfig = (const TASKDIALOGCONFIG *)lParam;
            taskdialog_destroy(dialog_info);
            taskdialog_init(dialog_info, hwnd);
            taskdialog_notify(dialog_info, TDN_DIALOG_CONSTRUCTED, 0, 0);
            /* Default radio button click notification is sent before TDN_NAVIGATED */
            taskdialog_check_default_radio_buttons(dialog_info);
            taskdialog_notify(dialog_info, TDN_NAVIGATED, 0, 0);
            break;
        case TDM_CLICK_BUTTON:
            taskdialog_click_button(dialog_info, wParam);
            break;
        case TDM_ENABLE_BUTTON:
            taskdialog_enable_button(dialog_info, wParam, lParam);
            break;
        case TDM_SET_MARQUEE_PROGRESS_BAR:
        {
            BOOL marquee = wParam;
            LONG style;
            if(!dialog_info->progress_bar) break;
            style = GetWindowLongW(dialog_info->progress_bar, GWL_STYLE);
            style = marquee ? style | PBS_MARQUEE : style & (~PBS_MARQUEE);
            SetWindowLongW(dialog_info->progress_bar, GWL_STYLE, style);
            break;
        }
        case TDM_SET_PROGRESS_BAR_STATE:
            result = SendMessageW(dialog_info->progress_bar, PBM_SETSTATE, wParam, 0);
            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, result);
            break;
        case TDM_SET_PROGRESS_BAR_RANGE:
            result = SendMessageW(dialog_info->progress_bar, PBM_SETRANGE, 0, lParam);
            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, result);
            break;
        case TDM_SET_PROGRESS_BAR_POS:
            result = 0;
            if (dialog_info->progress_bar)
            {
                LONG style = GetWindowLongW(dialog_info->progress_bar, GWL_STYLE);
                if (!(style & PBS_MARQUEE)) result = SendMessageW(dialog_info->progress_bar, PBM_SETPOS, wParam, 0);
            }
            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, result);
            break;
        case TDM_SET_PROGRESS_BAR_MARQUEE:
            SendMessageW(dialog_info->progress_bar, PBM_SETMARQUEE, wParam, lParam);
            break;
        case TDM_SET_ELEMENT_TEXT:
            taskdialog_set_element_text(dialog_info, wParam, (const WCHAR *)lParam);
            taskdialog_layout(dialog_info);
            break;
        case TDM_UPDATE_ELEMENT_TEXT:
            taskdialog_set_element_text(dialog_info, wParam, (const WCHAR *)lParam);
            break;
        case TDM_CLICK_RADIO_BUTTON:
            taskdialog_click_radio_button(dialog_info, wParam);
            break;
        case TDM_ENABLE_RADIO_BUTTON:
            taskdialog_enable_radio_button(dialog_info, wParam, lParam);
            break;
        case TDM_CLICK_VERIFICATION:
        {
            BOOL checked = (BOOL)wParam;
            BOOL focused = (BOOL)lParam;
            dialog_info->verification_checked = checked;
            if (dialog_info->verification_box)
            {
                SendMessageW(dialog_info->verification_box, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
                taskdialog_notify(dialog_info, TDN_VERIFICATION_CLICKED, checked, 0);
                if (focused) SetFocus(dialog_info->verification_box);
            }
            break;
        }
        case TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE:
            taskdialog_button_set_shield(dialog_info, wParam, lParam);
            break;
        case TDM_UPDATE_ICON:
            taskdialog_set_icon(dialog_info, wParam, (HICON)lParam);
            break;
        case WM_INITDIALOG:
            dialog_info = (struct taskdialog_info *)lParam;

            taskdialog_init(dialog_info, hwnd);

            SetPropW(hwnd, taskdialog_info_propnameW, dialog_info);
            taskdialog_notify(dialog_info, TDN_DIALOG_CONSTRUCTED, 0, 0);
            taskdialog_notify(dialog_info, TDN_CREATED, 0, 0);
            /* Default radio button click notification sent after TDN_CREATED */
            taskdialog_check_default_radio_buttons(dialog_info);
            return FALSE;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                taskdialog_on_button_click(dialog_info, (HWND)lParam, LOWORD(wParam));
                break;
            }
            return FALSE;
        case WM_HELP:
            taskdialog_notify(dialog_info, TDN_HELP, 0, 0);
            break;
        case WM_TIMER:
            if (ID_TIMER == wParam)
            {
                DWORD elapsed = GetTickCount() - dialog_info->last_timer_tick;
                if (taskdialog_notify(dialog_info, TDN_TIMER, elapsed, 0) == S_FALSE)
                    dialog_info->last_timer_tick = GetTickCount();
            }
            break;
        case WM_NOTIFY:
        {
            PNMLINK pnmLink = (PNMLINK)lParam;
            HWND hwndFrom = pnmLink->hdr.hwndFrom;
            if ((taskdialog_hyperlink_enabled(dialog_info))
                && (hwndFrom == dialog_info->content || hwndFrom == dialog_info->expanded_info
                    || hwndFrom == dialog_info->footer_text)
                && (pnmLink->hdr.code == NM_CLICK || pnmLink->hdr.code == NM_RETURN))
            {
                taskdialog_notify(dialog_info, TDN_HYPERLINK_CLICKED, 0, (LPARAM)pnmLink->item.szUrl);
                break;
            }
            return FALSE;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            if (dis->hwndItem == dialog_info->expando_button)
            {
                taskdialog_draw_expando_control(dialog_info, dis);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                break;
            }
            return FALSE;
        }
        case WM_DESTROY:
            taskdialog_notify(dialog_info, TDN_DESTROYED, 0, 0);
            RemovePropW(hwnd, taskdialog_info_propnameW);
            taskdialog_destroy(dialog_info);
            break;
        case WM_CLOSE:
            if (dialog_info->has_cancel)
            {
                if(taskdialog_notify(dialog_info, TDN_BUTTON_CLICKED, IDCANCEL, 0) == S_OK)
                    EndDialog(hwnd, IDCANCEL);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 0);
                break;
            }
            return FALSE;
        default:
            return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 * TaskDialogIndirect [COMCTL32.@]
 */
HRESULT WINAPI TaskDialogIndirect(const TASKDIALOGCONFIG *taskconfig, int *button,
                                  int *radio_button, BOOL *verification_flag_checked)
{
    struct taskdialog_info dialog_info;
    DLGTEMPLATE *template;
    INT ret;

    TRACE("%p, %p, %p, %p\n", taskconfig, button, radio_button, verification_flag_checked);

    if (!taskconfig || taskconfig->cbSize != sizeof(TASKDIALOGCONFIG))
        return E_INVALIDARG;

    dialog_info.taskconfig = taskconfig;

    template = create_taskdialog_template(taskconfig);
    ret = (short)DialogBoxIndirectParamW(taskconfig->hInstance, template, taskconfig->hwndParent,
            taskdialog_proc, (LPARAM)&dialog_info);
    Free(template);

    if (button) *button = ret;
    if (radio_button) *radio_button = dialog_info.selected_radio_id;
    if (verification_flag_checked) *verification_flag_checked = dialog_info.verification_checked;

    return S_OK;
}

/***********************************************************************
 * TaskDialog [COMCTL32.@]
 */
HRESULT WINAPI TaskDialog(HWND owner, HINSTANCE hinst, const WCHAR *title, const WCHAR *main_instruction,
    const WCHAR *content, TASKDIALOG_COMMON_BUTTON_FLAGS common_buttons, const WCHAR *icon, int *button)
{
    TASKDIALOGCONFIG taskconfig;

    TRACE("%p, %p, %s, %s, %s, %#x, %s, %p\n", owner, hinst, debugstr_w(title), debugstr_w(main_instruction),
        debugstr_w(content), common_buttons, debugstr_w(icon), button);

    memset(&taskconfig, 0, sizeof(taskconfig));
    taskconfig.cbSize = sizeof(taskconfig);
    taskconfig.hwndParent = owner;
    taskconfig.hInstance = hinst;
    taskconfig.dwCommonButtons = common_buttons;
    taskconfig.pszWindowTitle = title;
    taskconfig.pszMainIcon = icon;
    taskconfig.pszMainInstruction = main_instruction;
    taskconfig.pszContent = content;
    return TaskDialogIndirect(&taskconfig, button, NULL, NULL);
}
