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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "winerror.h"
#include "comctl32.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskdialog);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static const UINT DIALOG_MIN_WIDTH = 240;
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

struct taskdialog_button_desc
{
    int id;
    const WCHAR *text;
    unsigned int width;
    unsigned int line;
    HINSTANCE hinst;
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
    struct taskdialog_button_desc *default_button;
};

struct taskdialog_info
{
    HWND hwnd;
    PFTASKDIALOGCALLBACK callback;
    LONG_PTR callback_data;
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

/* used to calculate size for the controls */
static void taskdialog_get_text_extent(const struct taskdialog_template_desc *desc, const WCHAR *text,
        BOOL user_resource, SIZE *sz)
{
    RECT rect = { 0, 0, desc->dialog_width - DIALOG_SPACING * 2, 0}; /* padding left and right of the control */
    const WCHAR *textW = NULL;
    static const WCHAR nulW;
    unsigned int length;
    HFONT oldfont;
    HDC hdc;

    if (IS_INTRESOURCE(text))
    {
        if (!(length = LoadStringW(user_resource ? desc->taskconfig->hInstance : COMCTL32_hModule,
                (UINT_PTR)text, (WCHAR *)&textW, 0)))
        {
            WARN("Failed to load text\n");
            textW = &nulW;
            length = 0;
        }
    }
    else
    {
        textW = text;
        length = strlenW(textW);
    }

    hdc = GetDC(0);
    oldfont = SelectObject(hdc, desc->font);

    dialogunits_to_pixels(desc, &rect.right, NULL);
    DrawTextW(hdc, textW, length, &rect, DT_LEFT | DT_EXPANDTABS | DT_CALCRECT | DT_WORDBREAK);
    pixels_to_dialogunits(desc, &rect.right, &rect.bottom);

    SelectObject(hdc, oldfont);
    ReleaseDC(0, hdc);

    sz->cx = rect.right - rect.left;
    sz->cy = rect.bottom - rect.top;
}

static unsigned int taskdialog_add_control(struct taskdialog_template_desc *desc, WORD id, const WCHAR *class,
        HINSTANCE hInstance, const WCHAR *text, DWORD style, short x, short y, short cx, short cy)
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

    template->style = WS_VISIBLE | style;
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
    unsigned int size;
    SIZE sz;

    if (!str)
        return 0;

    taskdialog_get_text_extent(desc, str, TRUE, &sz);

    desc->dialog_height += DIALOG_SPACING;
    size = taskdialog_add_control(desc, id, WC_STATICW, desc->taskconfig->hInstance, str, 0, DIALOG_SPACING,
            desc->dialog_height, sz.cx, sz.cy);
    desc->dialog_height += sz.cy + DIALOG_SPACING;
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

static void taskdialog_init_button(struct taskdialog_button_desc *button, struct taskdialog_template_desc *desc,
        int id, const WCHAR *text, BOOL custom_button)
{
    SIZE sz;

    taskdialog_get_text_extent(desc, text, custom_button, &sz);

    button->id = id;
    button->text = text;
    button->width = max(DIALOG_BUTTON_WIDTH, sz.cx + DIALOG_SPACING * 2);
    button->line = 0;
    button->hinst = custom_button ? desc->taskconfig->hInstance : COMCTL32_hModule;

    if (id == desc->taskconfig->nDefaultButton)
        desc->default_button = button;
}

static void taskdialog_init_common_buttons(struct taskdialog_template_desc *desc, struct taskdialog_button_desc *buttons,
    unsigned int *button_count)
{
    DWORD flags = desc->taskconfig->dwCommonButtons;

#define TASKDIALOG_INIT_COMMON_BUTTON(id) \
    do { \
        taskdialog_init_button(&buttons[(*button_count)++], desc, ID##id, MAKEINTRESOURCEW(IDS_BUTTON_##id), FALSE); \
    } while(0)

    if (flags & TDCBF_OK_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(OK);
    if (flags & TDCBF_YES_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(YES);
    if (flags & TDCBF_NO_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(NO);
    if (flags & TDCBF_RETRY_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(RETRY);
    if (flags & TDCBF_CANCEL_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(CANCEL);
    if (flags & TDCBF_CLOSE_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(CLOSE);

#undef TASKDIALOG_INIT_COMMON_BUTTON
}

static unsigned int taskdialog_add_buttons(struct taskdialog_template_desc *desc)
{
    unsigned int count = 0, buttons_size, i, line_count, size = 0;
    unsigned int location_x, *line_widths, alignment = ~0u;
    const TASKDIALOGCONFIG *taskconfig = desc->taskconfig;
    struct taskdialog_button_desc *buttons;

    /* Allocate enough memory for the custom and the default buttons. Maximum 6 default buttons possible. */
    buttons_size = 6;
    if (taskconfig->cButtons && taskconfig->pButtons)
        buttons_size += taskconfig->cButtons;

    if (!(buttons = Alloc(buttons_size * sizeof(*buttons))))
        return 0;

    /* Custom buttons */
    if (taskconfig->cButtons && taskconfig->pButtons)
        for (i = 0; i < taskconfig->cButtons; i++)
            taskdialog_init_button(&buttons[count++], desc, taskconfig->pButtons[i].nButtonID,
                    taskconfig->pButtons[i].pszButtonText, TRUE);

    /* Common buttons */
    taskdialog_init_common_buttons(desc, buttons, &count);

    /* There must be at least one button */
    if (count == 0)
        taskdialog_init_button(&buttons[count++], desc, IDOK, MAKEINTRESOURCEW(IDS_BUTTON_OK), FALSE);

    if (!desc->default_button)
        desc->default_button = &buttons[0];

    /* For easy handling just allocate as many lines as buttons, the worst case. */
    line_widths = Alloc(count * sizeof(*line_widths));

    /* Separate buttons into lines */
    location_x = DIALOG_SPACING;
    for (i = 0, line_count = 0; i < count; i++)
    {
        if (location_x + buttons[i].width + DIALOG_SPACING > desc->dialog_width)
        {
            location_x = DIALOG_SPACING;
            line_count++;
        }

        buttons[i].line = line_count;

        location_x += buttons[i].width + DIALOG_SPACING;
        line_widths[line_count] += buttons[i].width + DIALOG_SPACING;
    }
    line_count++;

    /* Try to balance lines so they are about the same size */
    for (i = 1; i < line_count - 1; i++)
    {
        int diff_now = abs(line_widths[i] - line_widths[i - 1]);
        unsigned int j, last_button = 0;
        int diff_changed;

        for (j = 0; j < count; j++)
            if (buttons[j].line == i - 1)
                last_button = j;

        /* Difference in length of both lines if we wrapped the last button from the last line into this one */
        diff_changed = abs(2 * buttons[last_button].width + line_widths[i] - line_widths[i - 1]);

        if (diff_changed < diff_now)
        {
            buttons[last_button].line = i;
            line_widths[i] += buttons[last_button].width;
            line_widths[i - 1] -= buttons[last_button].width;
        }
    }

    /* Calculate left alignment so all lines are as far right as possible. */
    for (i = 0; i < line_count; i++)
    {
        int new_alignment = desc->dialog_width - line_widths[i];
        if (new_alignment < alignment)
            alignment = new_alignment;
    }

    /* Now that we got them all positioned, create all buttons */
    location_x = alignment;
    for (i = 0; i < count; i++)
    {
        DWORD style = &buttons[i] == desc->default_button ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;

        if (i > 0 && buttons[i].line != buttons[i - 1].line) /* New line */
        {
            location_x = alignment;
            desc->dialog_height += DIALOG_BUTTON_HEIGHT + DIALOG_SPACING;
        }

        size += taskdialog_add_control(desc, buttons[i].id, WC_BUTTONW, buttons[i].hinst, buttons[i].text, style,
                location_x, desc->dialog_height, buttons[i].width, DIALOG_BUTTON_HEIGHT);

        location_x += buttons[i].width + DIALOG_SPACING;
    }

    /* Add height for last row and spacing */
    desc->dialog_height += DIALOG_BUTTON_HEIGHT + DIALOG_SPACING;

    Free(line_widths);
    Free(buttons);

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

static WCHAR *taskdialog_get_exe_name(const TASKDIALOGCONFIG *taskconfig, WCHAR *name, DWORD length)
{
    DWORD len = GetModuleFileNameW(NULL, name, length);
    if (len && len < length)
    {
        WCHAR *p;
        if ((p = strrchrW(name, '/'))) name = p + 1;
        if ((p = strrchrW(name, '\\'))) name = p + 1;
        return name;
    }
    else
        return NULL;
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
    WCHAR pathW[MAX_PATH];
    RECT ref_rect;
    char *ptr;
    HDC hdc;

    /* Window title */
    if (!taskconfig->pszWindowTitle)
        titleW = taskdialog_get_exe_name(taskconfig, pathW, ARRAY_SIZE(pathW));
    else if (IS_INTRESOURCE(taskconfig->pszWindowTitle))
    {
        if (!LoadStringW(taskconfig->hInstance, LOWORD(taskconfig->pszWindowTitle), (WCHAR *)&titleW, 0))
            titleW = taskdialog_get_exe_name(taskconfig, pathW, ARRAY_SIZE(pathW));
    }
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
    desc.default_button = NULL;

    size += taskdialog_add_main_instruction(&desc);
    size += taskdialog_add_content(&desc);
    size += taskdialog_add_buttons(&desc);

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

static HRESULT taskdialog_notify(struct taskdialog_info *dialog_info, UINT notification, WPARAM wparam, LPARAM lparam)
{
    return dialog_info->callback ? dialog_info->callback(dialog_info->hwnd, notification, wparam, lparam,
            dialog_info->callback_data) : S_OK;
}

static void taskdialog_on_button_click(struct taskdialog_info *dialog_info, WORD command_id)
{
    if (taskdialog_notify(dialog_info, TDN_BUTTON_CLICKED, command_id, 0) == S_OK)
        EndDialog(dialog_info->hwnd, command_id);
}

static INT_PTR CALLBACK taskdialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR taskdialog_info_propnameW[] = {'T','a','s','k','D','i','a','l','o','g','I','n','f','o',0};
    struct taskdialog_info *dialog_info;

    TRACE("hwnd=%p msg=0x%04x wparam=%lx lparam=%lx\n", hwnd, msg, wParam, lParam);

    if (msg != WM_INITDIALOG)
        dialog_info = GetPropW(hwnd, taskdialog_info_propnameW);

    switch (msg)
    {
        case TDM_CLICK_BUTTON:
            taskdialog_on_button_click(dialog_info, LOWORD(wParam));
            break;
        case WM_INITDIALOG:
            dialog_info = (struct taskdialog_info *)lParam;
            dialog_info->hwnd = hwnd;
            SetPropW(hwnd, taskdialog_info_propnameW, dialog_info);

            taskdialog_notify(dialog_info, TDN_DIALOG_CONSTRUCTED, 0, 0);
            break;
        case WM_SHOWWINDOW:
            taskdialog_notify(dialog_info, TDN_CREATED, 0, 0);
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                taskdialog_on_button_click(dialog_info, LOWORD(wParam));
                return TRUE;
            }
            break;
        case WM_DESTROY:
            taskdialog_notify(dialog_info, TDN_DESTROYED, 0, 0);
            RemovePropW(hwnd, taskdialog_info_propnameW);
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
    struct taskdialog_info dialog_info;
    DLGTEMPLATE *template;
    INT ret;

    TRACE("%p, %p, %p, %p\n", taskconfig, button, radio_button, verification_flag_checked);

    if (!taskconfig || taskconfig->cbSize != sizeof(TASKDIALOGCONFIG))
        return E_INVALIDARG;

    dialog_info.callback = taskconfig->pfCallback;
    dialog_info.callback_data = taskconfig->lpCallbackData;

    template = create_taskdialog_template(taskconfig);
    ret = (short)DialogBoxIndirectParamW(taskconfig->hInstance, template, taskconfig->hwndParent,
            taskdialog_proc, (LPARAM)&dialog_info);
    Free(template);

    if (button) *button = ret;
    if (radio_button) *radio_button = taskconfig->nDefaultButton;
    if (verification_flag_checked) *verification_flag_checked = TRUE;

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
    taskconfig.u.pszMainIcon = icon;
    taskconfig.pszMainInstruction = main_instruction;
    taskconfig.pszContent = content;
    return TaskDialogIndirect(&taskconfig, button, NULL, NULL);
}
