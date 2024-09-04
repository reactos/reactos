/* Unit test suite for property sheet control.
 *
 * Copyright 2006 Huw Davies
 * Copyright 2009 Jan de Mooij
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

#include <windows.h>
#include <commctrl.h>
#include "msg.h"

#include "resources.h"
#include "wine/test.h"

#ifdef __REACTOS__
#undef WC_DIALOG
#define WC_DIALOG       (MAKEINTATOM(0x8002))
#endif

static HWND parenthwnd;
static HWND sheethwnd;

static BOOL rtl;
static LONG active_page = -1;

#define IDC_APPLY_BUTTON 12321

static HPROPSHEETPAGE (WINAPI *pCreatePropertySheetPageA)(const PROPSHEETPAGEA *desc);
static HPROPSHEETPAGE (WINAPI *pCreatePropertySheetPageW)(const PROPSHEETPAGEW *desc);
static BOOL (WINAPI *pDestroyPropertySheetPage)(HPROPSHEETPAGE proppage);
static INT_PTR (WINAPI *pPropertySheetA)(const PROPSHEETHEADERA *header);

static void detect_locale(void)
{
    DWORD reading_layout;
    rtl = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
            (void *)&reading_layout, sizeof(reading_layout)) && reading_layout == 1;
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
}


static int CALLBACK sheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    switch(msg)
    {
    case PSCB_PRECREATE:
      {
        HMODULE module = GetModuleHandleA("comctl32.dll");
        DWORD size, buffer_size;
        HRSRC hrsrc;

        hrsrc = FindResourceA(module, MAKEINTRESOURCEA(1006 /* IDD_PROPSHEET */),
                (LPSTR)RT_DIALOG);
        size = SizeofResource(module, hrsrc);
        ok(size != 0, "Failed to get size of propsheet dialog resource\n");
        buffer_size = HeapSize(GetProcessHeap(), 0, (void *)lparam);
        ok(buffer_size == 2 * size, "Unexpected template buffer size %u, resource size %u\n",
                buffer_size, size);
        break;
      }
    case PSCB_INITIALIZED:
      {
        char caption[256];
        GetWindowTextA(hwnd, caption, sizeof(caption));
        ok(!strcmp(caption,"test caption"), "caption: %s\n", caption);
        sheethwnd = hwnd;
        return 0;
      }
    }
    return 0;
}

static INT_PTR CALLBACK page_dlg_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                      LPARAM lparam)
{
    switch(msg)
    {
    case WM_INITDIALOG:
      {
        HWND sheet = GetParent(hwnd);
        char caption[256];
        GetWindowTextA(sheet, caption, sizeof(caption));
        ok(!strcmp(caption,"test caption"), "caption: %s\n", caption);
        return TRUE;
      }

    case WM_NOTIFY:
      {
        NMHDR *nmhdr = (NMHDR *)lparam;
        switch(nmhdr->code)
        {
        case PSN_APPLY:
            return TRUE;
        default:
            return FALSE;
        }
      }
    case WM_NCDESTROY:
        ok(!SendMessageA(sheethwnd, PSM_INDEXTOHWND, 400, 0),"Should always be 0\n");
        return TRUE;

    default:
        return FALSE;
    }
}

static void test_title(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg;
    DWORD style;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got invalid handle value %p\n", hdlg);

    style = GetWindowLongA(hdlg, GWL_STYLE);
    ok(style == (WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CAPTION|WS_SYSMENU|
                 DS_CONTEXTHELP|DS_MODALFRAME|DS_SETFONT|DS_3DLOOK),
       "got unexpected style: %x\n", style);

    DestroyWindow(hdlg);
}

static void test_nopage(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg, hpage;
    MSG msg;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got invalid handle value %p\n", hdlg);

    ShowWindow(hdlg,SW_NORMAL);
    SendMessageA(hdlg, PSM_REMOVEPAGE, 0, 0);
    hpage = /* PropSheet_GetCurrentPageHwnd(hdlg); */
        (HWND)SendMessageA(hdlg, PSM_GETCURRENTPAGEHWND, 0, 0);
    active_page = /* PropSheet_HwndToIndex(hdlg, hpage)); */
        (int)SendMessageA(hdlg, PSM_HWNDTOINDEX, (WPARAM)hpage, 0);
    ok(hpage == NULL, "expected no current page, got %p, index=%d\n", hpage, active_page);
    flush_events();
    RedrawWindow(hdlg,NULL,NULL,RDW_UPDATENOW|RDW_ERASENOW);

    /* Check that the property sheet was fully redrawn */
    ok(!PeekMessageA(&msg, 0, WM_PAINT, WM_PAINT, PM_NOREMOVE),
       "expected no pending WM_PAINT messages\n");
    DestroyWindow(hdlg);
}

static int CALLBACK disableowner_callback(HWND hwnd, UINT msg, LPARAM lparam)
{
    switch(msg)
    {
    case PSCB_INITIALIZED:
      {
        ok(IsWindowEnabled(parenthwnd) == 0, "parent window should be disabled\n");
        PostQuitMessage(0);
        return FALSE;
      }
    }
    return FALSE;
}

static void register_parent_wnd_class(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "parent class";
    RegisterClassA(&cls);
}

static void test_disableowner(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    INT_PTR p;

    register_parent_wnd_class();
    parenthwnd = CreateWindowA("parent class", "", WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), 0);

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = NULL;
    psp.lParam = 0;

    hpsp[0] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = parenthwnd;
    U3(psh).phpage = hpsp;
    psh.pfnCallback = disableowner_callback;

    p = pPropertySheetA(&psh);
    ok(p == 0, "Expected 0, got %ld\n", p);
    ok(IsWindowEnabled(parenthwnd) != 0, "parent window should be enabled\n");
    DestroyWindow(parenthwnd);
}

static INT_PTR CALLBACK nav_page_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg){
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lparam;
            switch(hdr->code){
            case PSN_SETACTIVE:
                active_page = /* PropSheet_HwndToIndex(hdr->hwndFrom, hwnd); */
                    (int)SendMessageA(hdr->hwndFrom, PSM_HWNDTOINDEX, (WPARAM)hwnd, 0);
                return TRUE;
            case PSN_KILLACTIVE:
                /* prevent navigation away from the fourth page */
                if(active_page == 3){
                    SetWindowLongPtrA(hwnd, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
            }
            break;
        }
    }
    return FALSE;
}

static WNDPROC old_nav_dialog_proc;

static LRESULT CALLBACK new_nav_dialog_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case DM_SETDEFID:
        ok( IsWindowEnabled( GetDlgItem(hwnd, wp) ), "button is not enabled\n" );
        break;
    }
    return CallWindowProcW( old_nav_dialog_proc, hwnd, msg, wp, lp );
}

static LRESULT CALLBACK hook_proc( int code, WPARAM wp, LPARAM lp )
{
    static BOOL done;
    if (code == HCBT_CREATEWND)
    {
        CBT_CREATEWNDW *c = (CBT_CREATEWNDW *)lp;

        /* The first dialog created will be the parent dialog */
        if (!done && c->lpcs->lpszClass == (LPWSTR)WC_DIALOG)
        {
            old_nav_dialog_proc = (WNDPROC)SetWindowLongPtrW( (HWND)wp, GWLP_WNDPROC, (LONG_PTR)new_nav_dialog_proc );
            done = TRUE;
        }
    }

    return CallNextHookEx( NULL, code, wp, lp );
}

static void test_wiznavigation(void)
{
    HPROPSHEETPAGE hpsp[4];
    PROPSHEETPAGEA psp[4];
    PROPSHEETHEADERA psh;
    HWND hdlg, control;
    LONG_PTR controlID;
    DWORD style;
    LRESULT defidres;
    BOOL hwndtoindex_supported = TRUE;
    const INT nextID = 12324;
    const INT backID = 12323;
    HHOOK hook;

    /* set up a hook proc in order to subclass the main dialog early on */
    hook = SetWindowsHookExW( WH_CBT, hook_proc, NULL, GetCurrentThreadId() );

    /* create the property sheet pages */
    memset(psp, 0, sizeof(PROPSHEETPAGEA) * 4);

    psp[0].dwSize = sizeof(PROPSHEETPAGEA);
    psp[0].hInstance = GetModuleHandleA(NULL);
    U(psp[0]).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_INTRO);
    psp[0].pfnDlgProc = nav_page_proc;
    hpsp[0] = pCreatePropertySheetPageA(&psp[0]);

    psp[1].dwSize = sizeof(PROPSHEETPAGEA);
    psp[1].hInstance = GetModuleHandleA(NULL);
    U(psp[1]).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_EDIT);
    psp[1].pfnDlgProc = nav_page_proc;
    hpsp[1] = pCreatePropertySheetPageA(&psp[1]);

    psp[2].dwSize = sizeof(PROPSHEETPAGEA);
    psp[2].hInstance = GetModuleHandleA(NULL);
    U(psp[2]).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_RADIO);
    psp[2].pfnDlgProc = nav_page_proc;
    hpsp[2] = pCreatePropertySheetPageA(&psp[2]);

    psp[3].dwSize = sizeof(PROPSHEETPAGEA);
    psp[3].hInstance = GetModuleHandleA(NULL);
    U(psp[3]).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_EXIT);
    psp[3].pfnDlgProc = nav_page_proc;
    hpsp[3] = pCreatePropertySheetPageA(&psp[3]);

    /* set up the property sheet dialog */
    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_MODELESS | PSH_WIZARD;
    psh.pszCaption = "A Wizard";
    psh.nPages = 4;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got invalid handle %p\n", hdlg);

    ok(active_page == 0, "Active page should be 0. Is: %d\n", active_page);

    style = GetWindowLongA(hdlg, GWL_STYLE) & ~(DS_CONTEXTHELP|WS_SYSMENU);
    ok(style == (WS_POPUP|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CAPTION|
                 DS_MODALFRAME|DS_SETFONT|DS_3DLOOK),
       "got unexpected style: %x\n", style);

    control = GetFocus();
    controlID = GetWindowLongPtrA(control, GWLP_ID);
    ok(controlID == nextID, "Focus should have been set to the Next button. Expected: %d, Found: %ld\n", nextID, controlID);

    /* simulate pressing the Next button */
    SendMessageA(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (!active_page) hwndtoindex_supported = FALSE;
    if (hwndtoindex_supported)
        ok(active_page == 1, "Active page should be 1 after pressing Next. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtrA(control, GWLP_ID);
    ok(controlID == IDC_PS_EDIT1, "Focus should be set to the first item on the second page. Expected: %d, Found: %ld\n", IDC_PS_EDIT1, controlID);

    defidres = SendMessageA(hdlg, DM_GETDEFID, 0, 0);
    ok(defidres == MAKELRESULT(nextID, DC_HASDEFID), "Expected default button ID to be %d, is %d\n", nextID, LOWORD(defidres));

    /* set the focus to the second edit box on this page */
    SetFocus(GetNextDlgTabItem(hdlg, control, FALSE));

    /* press next again */
    SendMessageA(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (hwndtoindex_supported)
        ok(active_page == 2, "Active page should be 2 after pressing Next. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtrA(control, GWLP_ID);
    ok(controlID == IDC_PS_RADIO1, "Focus should have been set to item on third page. Expected: %d, Found %ld\n", IDC_PS_RADIO1, controlID);

    /* back button */
    SendMessageA(hdlg, PSM_PRESSBUTTON, PSBTN_BACK, 0);
    if (hwndtoindex_supported)
        ok(active_page == 1, "Active page should be 1 after pressing Back. Is: %d\n", active_page);

    control = GetFocus();
    controlID = GetWindowLongPtrA(control, GWLP_ID);
    ok(controlID == IDC_PS_EDIT1, "Focus should have been set to the first item on second page. Expected: %d, Found %ld\n", IDC_PS_EDIT1, controlID);

    defidres = SendMessageA(hdlg, DM_GETDEFID, 0, 0);
    ok(defidres == MAKELRESULT(backID, DC_HASDEFID), "Expected default button ID to be %d, is %d\n", backID, LOWORD(defidres));

    /* press next twice */
    SendMessageA(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (hwndtoindex_supported)
        ok(active_page == 2, "Active page should be 2 after pressing Next. Is: %d\n", active_page);
    SendMessageA(hdlg, PSM_PRESSBUTTON, PSBTN_NEXT, 0);
    if (hwndtoindex_supported)
        ok(active_page == 3, "Active page should be 3 after pressing Next. Is: %d\n", active_page);
    else
        active_page = 3;

    control = GetFocus();
    controlID = GetWindowLongPtrA(control, GWLP_ID);
    ok(controlID == nextID, "Focus should have been set to the Next button. Expected: %d, Found: %ld\n", nextID, controlID);

    /* try to navigate away, but shouldn't be able to */
    SendMessageA(hdlg, PSM_PRESSBUTTON, PSBTN_BACK, 0);
    ok(active_page == 3, "Active page should still be 3 after pressing Back. Is: %d\n", active_page);

    defidres = SendMessageA(hdlg, DM_GETDEFID, 0, 0);
    ok(defidres == MAKELRESULT(nextID, DC_HASDEFID), "Expected default button ID to be %d, is %d\n", nextID, LOWORD(defidres));

    DestroyWindow(hdlg);
    UnhookWindowsHookEx( hook );
}

static void test_buttons(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg;
    HWND button;
    RECT rc;
    int prevRight, top;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = "prop_page1";
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc;
    psp.lParam = 0;

    hpsp[0] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_MODELESS | PSH_USECALLBACK;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback;

    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got null handle\n");

    /* OK button */
    button = GetDlgItem(hdlg, IDOK);
    GetWindowRect(button, &rc);
    prevRight = rc.right;
    top = rc.top;

    /* Cancel button */
    button = GetDlgItem(hdlg, IDCANCEL);
    GetWindowRect(button, &rc);
    ok(rc.top == top, "Cancel button should have same top as OK button\n");
    if (rtl)
        ok(rc.left < prevRight, "Cancel button should be to the left of OK button\n");
    else
        ok(rc.left > prevRight, "Cancel button should be to the right of OK button\n");
    prevRight = rc.right;

    button = GetDlgItem(hdlg, IDC_APPLY_BUTTON);
    GetWindowRect(button, &rc);
    ok(rc.top == top, "Apply button should have same top as OK button\n");
    if (rtl)
        ok(rc.left < prevRight, "Apply button should be to the left of Cancel button\n");
    else
        ok(rc.left > prevRight, "Apply button should be to the right of Cancel button\n");
    prevRight = rc.right;

    button = GetDlgItem(hdlg, IDHELP);
    GetWindowRect(button, &rc);
    ok(rc.top == top, "Help button should have same top as OK button\n");
    if (rtl)
        ok(rc.left < prevRight, "Help button should be to the left of Apply button\n");
    else
        ok(rc.left > prevRight, "Help button should be to the right of Apply button\n");

    DestroyWindow(hdlg);
}

static BOOL add_button_has_been_pressed;

static INT_PTR CALLBACK
page_with_custom_default_button_dlg_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch(LOWORD(wparam))
        {
        case IDC_PS_PUSHBUTTON1:
            switch(HIWORD(wparam))
            {
            case BN_CLICKED:
                add_button_has_been_pressed = TRUE;
                return TRUE;
            }
            break;
        }
        break;
    }
    return FALSE;
}

static void test_custom_default_button(void)
{
    HWND hdlg, page;
    PROPSHEETPAGEA psp[1];
    PROPSHEETHEADERA psh;
    MSG msg;
    LRESULT result;

    psp[0].dwSize = sizeof (PROPSHEETPAGEA);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = GetModuleHandleA(NULL);
    U(psp[0]).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_WITH_CUSTOM_DEFAULT_BUTTON);
    U2(psp[0]).pszIcon = NULL;
    psp[0].pfnDlgProc = page_with_custom_default_button_dlg_proc;
    psp[0].pszTitle = "Page1";
    psp[0].lParam = 0;

    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS;
    psh.hwndParent = GetDesktopWindow();
    psh.hInstance = GetModuleHandleA(NULL);
    U(psh).pszIcon = NULL;
    psh.pszCaption =  "PropertySheet1";
    psh.nPages = 1;
    U3(psh).ppsp = psp;
    U2(psh).nStartPage = 0;

    /* The goal of the test is to make sure that the Add button is pressed
     * when the ENTER key is pressed and a different control, a combobox,
     * has the keyboard focus. */
    add_button_has_been_pressed = FALSE;

    /* Create the modeless property sheet. */
    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "Cannot create the property sheet\n");

    /* Set the Add button as the default button. */
    SendMessageA(hdlg, DM_SETDEFID, (WPARAM)IDC_PS_PUSHBUTTON1, 0);

    /* Make sure the default button is the Add button. */
    result = SendMessageA(hdlg, DM_GETDEFID, 0, 0);
    ok(DC_HASDEFID == HIWORD(result), "The property sheet does not have a default button\n");
    ok(IDC_PS_PUSHBUTTON1 == LOWORD(result), "The default button is not the Add button\n");

    /* At this point, the combobox should have keyboard focus, so we press ENTER.
     * Pull the lever, Kronk! */
    page = (HWND)SendMessageW(hdlg, PSM_GETCURRENTPAGEHWND, 0, 0);
    PostMessageW(GetDlgItem(page, IDC_PS_COMBO1), WM_KEYDOWN, VK_RETURN, 0);

    /* Process all the messages in the queue for this thread. */
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        /* (!PropSheet_IsDialogMessage(hdlg, &msg)) */
        if (!((BOOL)SendMessageA(hdlg, PSM_ISDIALOGMESSAGE, 0, (LPARAM)&msg)))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    ok(add_button_has_been_pressed, "The Add button has not been pressed!\n");

    DestroyWindow(hdlg);
}

#define RECEIVER_SHEET_CALLBACK 0
#define RECEIVER_SHEET_WINPROC  1
#define RECEIVER_PAGE           2

#define NUM_MSG_SEQUENCES   1
#define PROPSHEET_SEQ_INDEX 0

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];
static WNDPROC oldWndProc;

static const struct message property_sheet_seq[] = {
    { PSCB_PRECREATE, sent|id, 0, 0, RECEIVER_SHEET_CALLBACK },
    { PSCB_INITIALIZED, sent|id, 0, 0, RECEIVER_SHEET_CALLBACK },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_NCCALCSIZE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_WINDOWPOSCHANGED, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_MOVE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_SIZE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_GETTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_NCCALCSIZE, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    { DM_REPOSITION, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_ACTIVATEAPP, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_NCACTIVATE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_ACTIVATE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_IME_SETCONTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_IME_NOTIFY, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_SETFOCUS, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_KILLFOCUS, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_IME_SETCONTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_PARENTNOTIFY, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_INITDIALOG, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, RECEIVER_PAGE },
    /*{ WM_NCCALCSIZE, sent|id, 0, 0, RECEIVER_PAGE },*/
    { WM_CHILDACTIVATE, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_WINDOWPOSCHANGED, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_MOVE, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_SIZE, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_NOTIFY, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_STYLECHANGING, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_STYLECHANGED, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_GETTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_SETTEXT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_SHOWWINDOW, sent|id, 0, 0, RECEIVER_PAGE },
    /*{ 0x00000401, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { 0x00000400, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_CHANGEUISTATE, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_UPDATEUISTATE, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_UPDATEUISTATE, sent|id|optional, 0, 0, RECEIVER_PAGE },
    { WM_SHOWWINDOW, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_WINDOWPOSCHANGING, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_NCPAINT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_ERASEBKGND, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_CTLCOLORDLG, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_WINDOWPOSCHANGED, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_PAINT, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_PAINT, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_NCPAINT, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_ERASEBKGND, sent|id, 0, 0, RECEIVER_PAGE },*/
    { WM_CTLCOLORDLG, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_CTLCOLORSTATIC, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_CTLCOLORSTATIC, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_CTLCOLORBTN, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_CTLCOLORBTN, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_CTLCOLORBTN, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { WM_COMMAND, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_NOTIFY, sent|id|optional, 0, 0, RECEIVER_PAGE },
    { WM_NOTIFY, sent|id|optional, 0, 0, RECEIVER_PAGE },
    { WM_WINDOWPOSCHANGING, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_WINDOWPOSCHANGED, sent|id|optional, 0, 0, RECEIVER_SHEET_WINPROC },
    /*{ WM_NCACTIVATE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_GETICON, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    /*{ WM_ACTIVATE, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_ACTIVATE, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_ACTIVATEAPP, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_ACTIVATEAPP, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_DESTROY, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },
    { WM_DESTROY, sent|id, 0, 0, RECEIVER_PAGE },*/
    /*{ WM_NCDESTROY, sent|id, 0, 0, RECEIVER_PAGE },
    { WM_NCDESTROY, sent|id, 0, 0, RECEIVER_SHEET_WINPROC },*/
    { 0 }
};

static void save_message(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, INT receiver)
{
    struct message msg = { 0 };

    if (message < WM_USER &&
        message != WM_GETICON &&
        message != WM_GETTEXT &&
        message != WM_IME_SETCONTEXT &&
        message != WM_IME_NOTIFY &&
        message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_SETCURSOR &&
        (message < WM_NCCREATE || message > WM_NCMBUTTONDBLCLK) &&
        (message < WM_MOUSEFIRST || message > WM_MOUSEHWHEEL) &&
        message != 0x90)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam|id;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.id = receiver;
        add_message(sequences, PROPSHEET_SEQ_INDEX, &msg);
    }
}

static LRESULT CALLBACK sheet_callback_messages_proc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    save_message(hwnd, msg, wParam, lParam, RECEIVER_SHEET_WINPROC);

    return CallWindowProcA(oldWndProc, hwnd, msg, wParam, lParam);
}

static int CALLBACK sheet_callback_messages(HWND hwnd, UINT msg, LPARAM lParam)
{
    save_message(hwnd, msg, 0, lParam, RECEIVER_SHEET_CALLBACK);

    switch (msg)
    {
    case PSCB_INITIALIZED:
        oldWndProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)&sheet_callback_messages_proc);
        return TRUE;
    }

    return TRUE;
}

static INT_PTR CALLBACK page_dlg_proc_messages(HWND hwnd, UINT msg, WPARAM wParam,
                                               LPARAM lParam)
{
    save_message(hwnd, msg, wParam, lParam, RECEIVER_PAGE);

    return FALSE;
}

static void test_messages(void)
{
    HPROPSHEETPAGE hpsp[1];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_MESSAGE_TEST);
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc_messages;
    psp.lParam = 0;

    hpsp[0] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_WIZARD | PSH_USECALLBACK
                  | PSH_MODELESS | PSH_USEICONID;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;
    psh.pfnCallback = sheet_callback_messages;

    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got invalid handle %p\n", hdlg);

    ShowWindow(hdlg,SW_NORMAL);

    ok_sequence(sequences, PROPSHEET_SEQ_INDEX, property_sheet_seq, "property sheet with custom window proc", TRUE);

    DestroyWindow(hdlg);
}

static void test_PSM_ADDPAGE(void)
{
    HPROPSHEETPAGE hpsp[5];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg, tab;
    BOOL ret;
    DWORD r;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_MESSAGE_TEST);
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc_messages;
    psp.lParam = 0;

    /* multiple pages with the same data */
    hpsp[0] = pCreatePropertySheetPageA(&psp);
    hpsp[1] = pCreatePropertySheetPageA(&psp);
    hpsp[2] = pCreatePropertySheetPageA(&psp);

    U(psp).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_ERROR);
    hpsp[3] = pCreatePropertySheetPageA(&psp);

    psp.dwFlags = PSP_PREMATURE;
    hpsp[4] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_MODELESS;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;

    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got invalid handle %p\n", hdlg);

    /* add pages one by one */
    ret = SendMessageA(hdlg, PSM_ADDPAGE, 0, (LPARAM)hpsp[1]);
    ok(ret == TRUE, "got %d\n", ret);

    /* try with null and invalid value */
    ret = SendMessageA(hdlg, PSM_ADDPAGE, 0, 0);
    ok(ret == FALSE, "got %d\n", ret);

if (0)
{
    /* crashes on native */
    ret = SendMessageA(hdlg, PSM_ADDPAGE, 0, (LPARAM)INVALID_HANDLE_VALUE);
}
    /* check item count */
    tab = (HWND)SendMessageA(hdlg, PSM_GETTABCONTROL, 0, 0);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 2, "got %d\n", r);

    ret = SendMessageA(hdlg, PSM_ADDPAGE, 0, (LPARAM)hpsp[2]);
    ok(ret == TRUE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 3, "got %d\n", r);

    /* add property sheet page that can't be created */
    ret = SendMessageA(hdlg, PSM_ADDPAGE, 0, (LPARAM)hpsp[3]);
    ok(ret == TRUE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 4, "got %d\n", r);

    /* select page that can't be created */
    ret = SendMessageA(hdlg, PSM_SETCURSEL, 3, 1);
    ok(ret == TRUE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 3, "got %d\n", r);

    /* test PSP_PREMATURE flag with incorrect property sheet page */
    ret = SendMessageA(hdlg, PSM_ADDPAGE, 0, (LPARAM)hpsp[4]);
    ok(ret == FALSE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 3, "got %d\n", r);

    pDestroyPropertySheetPage(hpsp[4]);
    DestroyWindow(hdlg);
}

static void test_PSM_INSERTPAGE(void)
{
    HPROPSHEETPAGE hpsp[5];
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HWND hdlg, tab;
    BOOL ret;
    DWORD r;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_MESSAGE_TEST);
    U2(psp).pszIcon = NULL;
    psp.pfnDlgProc = page_dlg_proc_messages;
    psp.lParam = 0;

    /* multiple pages with the same data */
    hpsp[0] = pCreatePropertySheetPageA(&psp);
    hpsp[1] = pCreatePropertySheetPageA(&psp);
    hpsp[2] = pCreatePropertySheetPageA(&psp);

    U(psp).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_ERROR);
    hpsp[3] = pCreatePropertySheetPageA(&psp);

    psp.dwFlags = PSP_PREMATURE;
    hpsp[4] = pCreatePropertySheetPageA(&psp);

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.dwFlags = PSH_MODELESS;
    psh.pszCaption = "test caption";
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = hpsp;

    hdlg = (HWND)pPropertySheetA(&psh);
    ok(hdlg != INVALID_HANDLE_VALUE, "got invalid handle %p\n", hdlg);

    /* add pages one by one */
    ret = SendMessageA(hdlg, PSM_INSERTPAGE, 5, (LPARAM)hpsp[1]);
    ok(ret == TRUE, "got %d\n", ret);

    /* try with invalid values */
    ret = SendMessageA(hdlg, PSM_INSERTPAGE, 0, 0);
    ok(ret == FALSE, "got %d\n", ret);

if (0)
{
    /* crashes on native */
    ret = SendMessageA(hdlg, PSM_INSERTPAGE, 0, (LPARAM)INVALID_HANDLE_VALUE);
}

    ret = SendMessageA(hdlg, PSM_INSERTPAGE, (WPARAM)INVALID_HANDLE_VALUE, (LPARAM)hpsp[2]);
    ok(ret == FALSE, "got %d\n", ret);

    /* check item count */
    tab = (HWND)SendMessageA(hdlg, PSM_GETTABCONTROL, 0, 0);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 2, "got %d\n", r);

    ret = SendMessageA(hdlg, PSM_INSERTPAGE, (WPARAM)hpsp[1], (LPARAM)hpsp[2]);
    ok(ret == TRUE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 3, "got %d\n", r);

    /* add property sheet page that can't be created */
    ret = SendMessageA(hdlg, PSM_INSERTPAGE, 1, (LPARAM)hpsp[3]);
    ok(ret == TRUE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 4, "got %d\n", r);

    /* select page that can't be created */
    ret = SendMessageA(hdlg, PSM_SETCURSEL, 1, 0);
    ok(ret == TRUE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 3, "got %d\n", r);

    /* test PSP_PREMATURE flag with incorrect property sheet page */
    ret = SendMessageA(hdlg, PSM_INSERTPAGE, 0, (LPARAM)hpsp[4]);
    ok(ret == FALSE, "got %d\n", ret);

    r = SendMessageA(tab, TCM_GETITEMCOUNT, 0, 0);
    ok(r == 3, "got %d\n", r);

    pDestroyPropertySheetPage(hpsp[4]);
    DestroyWindow(hdlg);
}

struct custom_proppage
{
    union
    {
        PROPSHEETPAGEA pageA;
        PROPSHEETPAGEW pageW;
    } u;
    unsigned int addref_called;
    unsigned int release_called;
};

static UINT CALLBACK proppage_callback_a(HWND hwnd, UINT msg, PROPSHEETPAGEA *psp)
{
    struct custom_proppage *cpage = (struct custom_proppage *)psp->lParam;
    PROPSHEETPAGEA *psp_orig = &cpage->u.pageA;

    ok(hwnd == NULL, "Expected NULL hwnd, got %p\n", hwnd);

    ok(psp->lParam && psp->lParam != (LPARAM)psp, "Expected newly allocated page description, got %lx, %p\n",
            psp->lParam, psp);
    ok(psp_orig->pszTitle == psp->pszTitle, "Expected same page title pointer\n");
    ok(!lstrcmpA(psp_orig->pszTitle, psp->pszTitle), "Expected same page title string\n");

    switch (msg)
    {
    case PSPCB_ADDREF:
        ok(psp->dwSize > PROPSHEETPAGEA_V1_SIZE, "Expected ADDREF for V2+ only, got size %u\n", psp->dwSize);
        cpage->addref_called++;
        break;
    case PSPCB_RELEASE:
        ok(psp->dwSize >= PROPSHEETPAGEA_V1_SIZE, "Unexpected RELEASE, got size %u\n", psp->dwSize);
        cpage->release_called++;
        break;
    default:
        ok(0, "Unexpected message %u\n", msg);
    }

    return 1;
}

static UINT CALLBACK proppage_callback_w(HWND hwnd, UINT msg, PROPSHEETPAGEW *psp)
{
    struct custom_proppage *cpage = (struct custom_proppage *)psp->lParam;
    PROPSHEETPAGEW *psp_orig = &cpage->u.pageW;

    ok(hwnd == NULL, "Expected NULL hwnd, got %p\n", hwnd);
    ok(psp->lParam && psp->lParam != (LPARAM)psp, "Expected newly allocated page description, got %lx, %p\n",
            psp->lParam, psp);
    ok(psp_orig->pszTitle == psp->pszTitle, "Expected same page title pointer\n");
    ok(!lstrcmpW(psp_orig->pszTitle, psp->pszTitle), "Expected same page title string\n");

    switch (msg)
    {
    case PSPCB_ADDREF:
        ok(psp->dwSize > PROPSHEETPAGEW_V1_SIZE, "Expected ADDREF for V2+ only, got size %u\n", psp->dwSize);
        cpage->addref_called++;
        break;
    case PSPCB_RELEASE:
        ok(psp->dwSize >= PROPSHEETPAGEW_V1_SIZE, "Unexpected RELEASE, got size %u\n", psp->dwSize);
        cpage->release_called++;
        break;
    default:
        ok(0, "Unexpected message %u\n", msg);
    }

    return 1;
}

static void test_CreatePropertySheetPage(void)
{
    static const WCHAR titleW[] = {'T','i','t','l','e',0};
    struct custom_proppage page;
    HPROPSHEETPAGE hpsp;
    BOOL ret;

    memset(&page.u.pageA, 0, sizeof(page.u.pageA));
    page.u.pageA.dwFlags = PSP_USECALLBACK;
    page.u.pageA.pfnDlgProc = page_dlg_proc_messages;
    page.u.pageA.pfnCallback = proppage_callback_a;
    page.u.pageA.lParam = (LPARAM)&page;
    page.u.pageA.pszTitle = "Title";

    /* Only minimal size validation is performed */
    for (page.u.pageA.dwSize = PROPSHEETPAGEA_V1_SIZE - 1; page.u.pageA.dwSize <= PROPSHEETPAGEA_V4_SIZE + 1; page.u.pageA.dwSize++)
    {
        page.addref_called = 0;
        hpsp = pCreatePropertySheetPageA(&page.u.pageA);

        if (page.u.pageA.dwSize < PROPSHEETPAGEA_V1_SIZE)
            ok(hpsp == NULL, "Expected failure, size %u\n", page.u.pageA.dwSize);
        else
        {
            ok(hpsp != NULL, "Failed to create a page, size %u\n", page.u.pageA.dwSize);
            ok(page.addref_called == (page.u.pageA.dwSize > PROPSHEETPAGEA_V1_SIZE) ? 1 : 0, "Expected ADDREF callback message\n");
        }

        if (hpsp)
        {
            page.release_called = 0;
            ret = pDestroyPropertySheetPage(hpsp);
            ok(ret, "Failed to destroy a page\n");
            ok(page.release_called == 1, "Expected RELEASE callback message\n");
        }
    }

    memset(&page.u.pageW, 0, sizeof(page.u.pageW));
    page.u.pageW.dwFlags = PSP_USECALLBACK;
    page.u.pageW.pfnDlgProc = page_dlg_proc_messages;
    page.u.pageW.pfnCallback = proppage_callback_w;
    page.u.pageW.lParam = (LPARAM)&page;
    page.u.pageW.pszTitle = titleW;

    for (page.u.pageW.dwSize = PROPSHEETPAGEW_V1_SIZE - 1; page.u.pageW.dwSize <= PROPSHEETPAGEW_V4_SIZE + 1; page.u.pageW.dwSize++)
    {
        page.addref_called = 0;
        hpsp = pCreatePropertySheetPageW(&page.u.pageW);

        if (page.u.pageW.dwSize < PROPSHEETPAGEW_V1_SIZE)
            ok(hpsp == NULL, "Expected failure, size %u\n", page.u.pageW.dwSize);
        else
        {
            ok(hpsp != NULL, "Failed to create a page, size %u\n", page.u.pageW.dwSize);
            ok(page.addref_called == (page.u.pageW.dwSize > PROPSHEETPAGEW_V1_SIZE) ? 1 : 0, "Expected ADDREF callback message\n");
        }

        if (hpsp)
        {
            page.release_called = 0;
            ret = pDestroyPropertySheetPage(hpsp);
            ok(ret, "Failed to destroy a page\n");
            ok(page.release_called == 1, "Expected RELEASE callback message\n");
        }
    }
}

static void test_bad_control_class(void)
{
    PROPSHEETPAGEA psp;
    PROPSHEETHEADERA psh;
    HPROPSHEETPAGE hpsp;
    INT_PTR ret;

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.hInstance = GetModuleHandleA(NULL);
    U(psp).pszTemplate = (LPCSTR)MAKEINTRESOURCE(IDD_PROP_PAGE_BAD_CONTROL);
    psp.pfnDlgProc = page_dlg_proc;

    hpsp = pCreatePropertySheetPageA(&psp);
    ok(hpsp != 0, "CreatePropertySheetPage failed\n");

    memset(&psh, 0, sizeof(psh));
    psh.dwSize = PROPSHEETHEADERA_V1_SIZE;
    psh.nPages = 1;
    psh.hwndParent = GetDesktopWindow();
    U3(psh).phpage = &hpsp;

    ret = pPropertySheetA(&psh);
    ok(ret == 0, "got %ld\n", ret);

    /* Need to recreate hpsp otherwise the test fails under Windows */
    hpsp = pCreatePropertySheetPageA(&psp);
    ok(hpsp != 0, "CreatePropertySheetPage failed\n");
    U3(psh).phpage = &hpsp;

    psh.dwFlags = PSH_MODELESS;
    ret = pPropertySheetA(&psh);
    ok(ret != 0, "got %ld\n", ret);

    ok(IsWindow((HWND)ret), "bad window handle %#lx\n", ret);
    DestroyWindow((HWND)ret);
}

static void init_functions(void)
{
    HMODULE hComCtl32 = LoadLibraryA("comctl32.dll");

#define X(f) p##f = (void*)GetProcAddress(hComCtl32, #f);
    X(CreatePropertySheetPageA);
    X(CreatePropertySheetPageW);
    X(DestroyPropertySheetPage);
    X(PropertySheetA);
#undef X
}

START_TEST(propsheet)
{
    detect_locale();
    if (rtl)
    {
        /* use locale-specific RTL resources when on an RTL locale */
        /* without this, propsheets on RTL locales use English LTR resources */
        trace("RTL locale detected\n");
        SetProcessDefaultLayout(LAYOUT_RTL);
    }

    init_functions();

    test_bad_control_class();
    test_title();
    test_nopage();
    test_disableowner();
    test_wiznavigation();
    test_buttons();
    test_custom_default_button();
    test_messages();
    test_PSM_ADDPAGE();
    test_PSM_INSERTPAGE();
    test_CreatePropertySheetPage();
}
