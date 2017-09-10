/*
 * Task dialog control
 *
 * Copyright 2017 Fabian Maurer
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

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskdialog);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static const UINT DIALOG_MIN_WIDTH = 180;
static const UINT DIALOG_SPACING = 5;
static const UINT DIALOG_BUTTON_WIDTH = 50;
static const UINT DIALOG_BUTTON_HEIGHT = 14;

static const UINT ID_MAIN_INSTRUCTION = 0xf000;
static const UINT ID_CONTENT          = 0xf001;

struct taskdialog_control
{
    struct list entry;
    DLGITEMTEMPLATE *template;
    unsigned int template_size;
};

struct taskdialog_template_desc
{
    const TASKDIALOGCONFIG *taskconfig;
    unsigned int dialog_height;
    unsigned int dialog_width;
    struct list controls;
    WORD control_count;
    LONG x_baseunit;
    LONG y_baseunit;
    HFONT font;
};

static void pixels_to_dialogunits(const struct taskdialog_template_desc *desc, LONG *width, LONG *height)
{
    if (width)
        *width = MulDiv(*width, 4, desc->x_baseunit);
    if (height)
        *height = MulDiv(*height, 8, desc->y_baseunit);
}

static void dialogunits_to_pixels(const struct taskdialog_template_desc *desc, LONG *width, LONG *height)
{
    if (width)
        *width = MulDiv(*width, desc->x_baseunit, 4);
    if (height)
        *height = MulDiv(*height, desc->y_baseunit, 8);
}

static void template_write_data(char **ptr, const void *src, unsigned int size)
{
    memcpy(*ptr, src, size);
    *ptr += size;
}

static unsigned int taskdialog_add_control(struct taskdialog_template_desc *desc, WORD id, const WCHAR *class,
        HINSTANCE hInstance, const WCHAR *text, short x, short y, short cx, short cy)
{
    struct taskdialog_control *control = Alloc(sizeof(*control));
    unsigned int size, class_size, text_size;
    DLGITEMTEMPLATE *template;
    static const WCHAR nulW;
    const WCHAR *textW;
    char *ptr;

    class_size = (strlenW(class) + 1) * sizeof(WCHAR);

    if (IS_INTRESOURCE(text))
        text_size = LoadStringW(hInstance, (UINT_PTR)text, (WCHAR *)&textW, 0) * sizeof(WCHAR);
    else
    {
        textW = text;
        text_size = strlenW(textW) * sizeof(WCHAR);
    }

    size = sizeof(DLGITEMTEMPLATE);
    size += class_size;
    size += text_size + sizeof(WCHAR);
    size += sizeof(WORD); /* creation data */

    control->template = template = Alloc(size);
    control->template_size = size;

    template->style = WS_VISIBLE;
    template->dwExtendedStyle = 0;
    template->x = x;
    template->y = y;
    template->cx = cx;
    template->cy = cy;
    template->id = id;
    ptr = (char *)(template + 1);
    template_write_data(&ptr, class, class_size);
    template_write_data(&ptr, textW, text_size);
    template_write_data(&ptr, &nulW, sizeof(nulW));

    list_add_tail(&desc->controls, &control->entry);
    desc->control_count++;
    return ALIGNED_LENGTH(size, 3);
}

static unsigned int taskdialog_add_static_label(struct taskdialog_template_desc *desc, WORD id, const WCHAR *str)
{
    RECT rect = { 0, 0, desc->dialog_width - DIALOG_SPACING * 2, 0}; /* padding left and right of the control */
    const WCHAR *textW = NULL;
    unsigned int size, length;
    HFONT oldfont;
    HDC hdc;

    if (!str)
        return 0;

    if (IS_INTRESOURCE(str))
    {
        if (!(length = LoadStringW(desc->taskconfig->hInstance, (UINT_PTR)str, (WCHAR *)&textW, 0)))
        {
            WARN("Failed to load static text %s, id %#x\n", debugstr_w(str), id);
            return 0;
        }
    }
    else
    {
        textW = str;
        length = strlenW(textW);
    }

    hdc = GetDC(0);
    oldfont = SelectObject(hdc, desc->font);

    dialogunits_to_pixels(desc, &rect.right, NULL);
    DrawTextW(hdc, textW, length, &rect, DT_LEFT | DT_EXPANDTABS | DT_CALCRECT | DT_WORDBREAK);
    pixels_to_dialogunits(desc, &rect.right, &rect.bottom);

    SelectObject(hdc, oldfont);
    ReleaseDC(0, hdc);

    desc->dialog_height += DIALOG_SPACING;
    size = taskdialog_add_control(desc, id, WC_STATICW, desc->taskconfig->hInstance, str, DIALOG_SPACING,
            desc->dialog_height, rect.right, rect.bottom);
    desc->dialog_height += rect.bottom;
    return size;
}

static unsigned int taskdialog_add_main_instruction(struct taskdialog_template_desc *desc)
{
    return taskdialog_add_static_label(desc, ID_MAIN_INSTRUCTION, desc->taskconfig->pszMainInstruction);
}

static unsigned int taskdialog_add_content(struct taskdialog_template_desc *desc)
{
    return taskdialog_add_static_label(desc, ID_CONTENT, desc->taskconfig->pszContent);
}

static unsigned int taskdialog_add_common_buttons(struct taskdialog_template_desc *desc)
{
    short button_x = desc->dialog_width - DIALOG_BUTTON_WIDTH - DIALOG_SPACING;
    DWORD flags = desc->taskconfig->dwCommonButtons;
    unsigned int size = 0;

#define TASKDIALOG_ADD_COMMON_BUTTON(id) \
    do { \
        size += taskdialog_add_control(desc, ID##id, WC_BUTTONW, COMCTL32_hModule, MAKEINTRESOURCEW(IDS_BUTTON_##id), \
            button_x, desc->dialog_height + DIALOG_SPACING, DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT); \
        button_x -= DIALOG_BUTTON_WIDTH + DIALOG_SPACING; \
    } while(0)
    if (flags & TDCBF_CLOSE_BUTTON)
        TASKDIALOG_ADD_COMMON_BUTTON(CLOSE);
    if (flags & TDCBF_CANCEL_BUTTON)
        TASKDIALOG_ADD_COMMON_BUTTON(CANCEL);
    if (flags & TDCBF_RETRY_BUTTON)
        TASKDIALOG_ADD_COMMON_BUTTON(RETRY);
    if (flags & TDCBF_NO_BUTTON)
        TASKDIALOG_ADD_COMMON_BUTTON(NO);
    if (flags & TDCBF_YES_BUTTON)
        TASKDIALOG_ADD_COMMON_BUTTON(YES);
    if (flags & TDCBF_OK_BUTTON)
        TASKDIALOG_ADD_COMMON_BUTTON(OK);
    /* Always add OK button */
    if (list_empty(&desc->controls))
        TASKDIALOG_ADD_COMMON_BUTTON(OK);
#undef TASKDIALOG_ADD_COMMON_BUTTON

    /* make room for common buttons row */
    desc->dialog_height +=  DIALOG_BUTTON_HEIGHT + 2 * DIALOG_SPACING;
    return size;
}

static void taskdialog_clear_controls(struct list *controls)
{
    struct taskdialog_control *control, *control2;

    LIST_FOR_EACH_ENTRY_SAFE(control, control2, controls, struct taskdialog_control, entry)
    {
        list_remove(&control->entry);
        Free(control->template);
        Free(control);
    }
}

static unsigned int taskdialog_get_reference_rect(const struct taskdialog_template_desc *desc, RECT *ret)
{
    HMONITOR monitor = MonitorFromWindow(desc->taskconfig->hwndParent ? desc->taskconfig->hwndParent : GetActiveWindow(),
            MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info;

    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);

    if (desc->taskconfig->dwFlags & TDF_POSITION_RELATIVE_TO_WINDOW && desc->taskconfig->hwndParent)
        GetWindowRect(desc->taskconfig->hwndParent, ret);
    else
        *ret = info.rcWork;

    pixels_to_dialogunits(desc, &ret->left, &ret->top);
    pixels_to_dialogunits(desc, &ret->right, &ret->bottom);

    pixels_to_dialogunits(desc, &info.rcWork.left, &info.rcWork.top);
    pixels_to_dialogunits(desc, &info.rcWork.right, &info.rcWork.bottom);
    return info.rcWork.right - info.rcWork.left;
}

static DLGTEMPLATE *create_taskdialog_template(const TASKDIALOGCONFIG *taskconfig)
{
    struct taskdialog_control *control, *control2;
    unsigned int size, title_size, screen_width;
    struct taskdialog_template_desc desc;
    static const WORD fontsize = 0x7fff;
    static const WCHAR emptyW[] = { 0 };
    const WCHAR *titleW = NULL;
    DLGTEMPLATE *template;
    NONCLIENTMETRICSW ncm;
    RECT ref_rect;
    char *ptr;
    HDC hdc;

    /* Window title */
    if (!taskconfig->pszWindowTitle)
        FIXME("use executable name for window title\n");
    else if (IS_INTRESOURCE(taskconfig->pszWindowTitle))
        FIXME("load window title from resources\n");
    else
        titleW = taskconfig->pszWindowTitle;
    if (!titleW)
        titleW = emptyW;
    title_size = (strlenW(titleW) + 1) * sizeof(WCHAR);

    size = sizeof(DLGTEMPLATE) + 2 * sizeof(WORD);
    size += title_size;
    size += 2; /* font size */

    list_init(&desc.controls);
    desc.taskconfig = taskconfig;
    desc.control_count = 0;

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    desc.font = CreateFontIndirectW(&ncm.lfMessageFont);

    hdc = GetDC(0);
    SelectObject(hdc, desc.font);
    desc.x_baseunit = GdiGetCharDimensions(hdc, NULL, &desc.y_baseunit);
    ReleaseDC(0, hdc);

    screen_width = taskdialog_get_reference_rect(&desc, &ref_rect);

    desc.dialog_height = 0;
    desc.dialog_width = max(taskconfig->cxWidth, DIALOG_MIN_WIDTH);
    desc.dialog_width = min(desc.dialog_width, screen_width);

    size += taskdialog_add_main_instruction(&desc);
    size += taskdialog_add_content(&desc);
    size += taskdialog_add_common_buttons(&desc);

    template = Alloc(size);
    if (!template)
    {
        taskdialog_clear_controls(&desc.controls);
        DeleteObject(desc.font);
        return NULL;
    }

    template->style = DS_MODALFRAME | DS_SETFONT | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
    template->cdit = desc.control_count;
    template->x = (ref_rect.left + ref_rect.right + desc.dialog_width) / 2;
    template->y = (ref_rect.top + ref_rect.bottom + desc.dialog_height) / 2;
    template->cx = desc.dialog_width;
    template->cy = desc.dialog_height;

    ptr = (char *)(template + 1);
    ptr += 2; /* menu */
    ptr += 2; /* class */
    template_write_data(&ptr, titleW, title_size);
    template_write_data(&ptr, &fontsize, sizeof(fontsize));

    /* write control entries */
    LIST_FOR_EACH_ENTRY_SAFE(control, control2, &desc.controls, struct taskdialog_control, entry)
    {
        ALIGN_POINTER(ptr, 3);

        template_write_data(&ptr, control->template, control->template_size);

        /* list item won't be needed later */
        list_remove(&control->entry);
        Free(control->template);
        Free(control);
    }

    DeleteObject(desc.font);
    return template;
}

static INT_PTR CALLBACK taskdialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p msg=0x%04x wparam=%lx lparam=%lx\n", hwnd, msg, wParam, lParam);

    switch (msg)
    {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                WORD command_id = LOWORD(wParam);
                EndDialog(hwnd, command_id);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

/***********************************************************************
 * TaskDialogIndirect [COMCTL32.@]
 */
HRESULT WINAPI TaskDialogIndirect(const TASKDIALOGCONFIG *taskconfig, int *button,
                                  int *radio_button, BOOL *verification_flag_checked)
{
    DLGTEMPLATE *template;
    INT ret;

    TRACE("%p, %p, %p, %p\n", taskconfig, button, radio_button, verification_flag_checked);

    template = create_taskdialog_template(taskconfig);
    ret = DialogBoxIndirectParamW(taskconfig->hInstance, template, taskconfig->hwndParent, taskdialog_proc, 0);
    Free(template);

    if (button) *button = ret;
    if (radio_button) *radio_button = taskconfig->nDefaultButton;
    if (verification_flag_checked) *verification_flag_checked = TRUE;

    return S_OK;
}
