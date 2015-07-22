/*
 * Unit test suite for comdlg32 API functions: file dialogs
 *
 * Copyright 2007 Google (Lei Zhang)
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

//#include <windows.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <wine/test.h>

#include <wingdi.h>
#include <objbase.h>
#include <cderr.h>
#include <dlgs.h>
#include <commdlg.h>

#include <shlguid.h>
#define COBJMACROS
#include <shobjidl.h>

/* ##### */

static BOOL resizesupported = TRUE;

static void toolbarcheck( HWND hDlg)
{
    /* test toolbar properties */
    /* bug #10532 */
    int maxtextrows;
    HWND ctrl;
    DWORD ret;
    char classname[20];

    for( ctrl = GetWindow( hDlg, GW_CHILD);
            ctrl ; ctrl = GetWindow( ctrl, GW_HWNDNEXT)) {
        GetClassNameA( ctrl, classname, 10);
        classname[7] = '\0';
        if( !strcmp( classname, "Toolbar")) break;
    }
    ok( ctrl != NULL, "could not get the toolbar control\n");
    ret = SendMessageA( ctrl, TB_ADDSTRINGA, 0, (LPARAM)"winetestwinetest\0\0");
    ok( ret == 0, "addstring returned %d (expected 0)\n", ret);
    maxtextrows = SendMessageA( ctrl, TB_GETTEXTROWS, 0, 0);
    ok( maxtextrows == 0 || broken(maxtextrows == 1),  /* Win2k and below */
        "Get(Max)TextRows returned %d (expected 0)\n", maxtextrows);
}


static UINT_PTR CALLBACK OFNHookProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR nmh;

    if( msg == WM_NOTIFY)
    {
        nmh = (LPNMHDR) lParam;
        if( nmh->code == CDN_INITDONE)
        {
            PostMessageA( GetParent(hDlg), WM_COMMAND, IDCANCEL, FALSE);
        } else if (nmh->code == CDN_FOLDERCHANGE )
        {
            char buf[1024];
            int ret;

            memset(buf, 0x66, sizeof(buf));
            ret = SendMessageA( GetParent(hDlg), CDM_GETFOLDERIDLIST, 5, (LPARAM)buf);
            ok(ret > 0, "CMD_GETFOLDERIDLIST not implemented\n");
            if (ret > 5)
                ok(buf[0] == 0x66 && buf[1] == 0x66, "CMD_GETFOLDERIDLIST: The buffer was touched on failure\n");
            toolbarcheck( GetParent(hDlg));
        }
    }

    return 0;
}

/* bug 6829 */
static void test_DialogCancel(void)
{
    OPENFILENAMEA ofn;
    BOOL result;
    char szFileName[MAX_PATH] = "";
    char szInitialDir[MAX_PATH];

    GetWindowsDirectoryA(szInitialDir, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = "txt";
    ofn.lpfnHook = OFNHookProc;
    ofn.lpstrInitialDir = szInitialDir;

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(),
       "expected CDERR_INITIALIZATION, got %d\n", CommDlgExtendedError());

    result = GetOpenFileNameA(&ofn);
    ok(FALSE == result, "expected FALSE, got %d\n", result);
    ok(0 == CommDlgExtendedError(), "expected 0, got %d\n",
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(),
       "expected CDERR_INITIALIZATION, got %d\n", CommDlgExtendedError());

    result = GetSaveFileNameA(&ofn);
    ok(FALSE == result, "expected FALSE, got %d\n", result);
    ok(0 == CommDlgExtendedError(), "expected 0, got %d\n",
       CommDlgExtendedError());

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(),
       "expected CDERR_INITIALIZATION, got %d\n", CommDlgExtendedError());

    /* Before passing the ofn to Unicode functions, remove the ANSI strings */
    ofn.lpstrFilter = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = NULL;

    PrintDlgA(NULL);
    ok(CDERR_INITIALIZATION == CommDlgExtendedError(),
       "expected CDERR_INITIALIZATION, got %d\n", CommDlgExtendedError());

    SetLastError(0xdeadbeef);
    result = GetOpenFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        win_skip("GetOpenFileNameW is not implemented\n");
    else
    {
        ok(FALSE == result, "expected FALSE, got %d\n", result);
        ok(0 == CommDlgExtendedError(), "expected 0, got %d\n", CommDlgExtendedError());
    }

    SetLastError(0xdeadbeef);
    result = GetSaveFileNameW((LPOPENFILENAMEW) &ofn);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        win_skip("GetSaveFileNameW is not implemented\n");
    else
    {
        ok(FALSE == result, "expected FALSE, got %d\n", result);
        ok(0 == CommDlgExtendedError(), "expected 0, got %d\n", CommDlgExtendedError());
    }
}

static UINT_PTR CALLBACK create_view_window2_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NOTIFY)
    {
        if (((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE)
        {
            IShellBrowser *shell_browser = (IShellBrowser *)SendMessageA(GetParent(dlg), WM_USER + 7 /* WM_GETISHELLBROWSER */, 0, 0);
            IShellView *shell_view = NULL;
            IShellView2 *shell_view2 = NULL;
            SV2CVW2_PARAMS view_params;
            FOLDERSETTINGS folder_settings;
            HRESULT hr;
            RECT rect = {0, 0, 0, 0};

            hr = IShellBrowser_QueryActiveShellView(shell_browser, &shell_view);
            ok(SUCCEEDED(hr), "QueryActiveShellView returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView_QueryInterface(shell_view, &IID_IShellView2, (void **)&shell_view2);
            if (hr == E_NOINTERFACE)
            {
                win_skip("IShellView2 not supported\n");
                goto cleanup;
            }
            ok(SUCCEEDED(hr), "QueryInterface returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView2_DestroyViewWindow(shell_view2);
            ok(SUCCEEDED(hr), "DestroyViewWindow returned %#x\n", hr);

            folder_settings.ViewMode = FVM_LIST;
            folder_settings.fFlags = 0;

            view_params.cbSize = sizeof(view_params);
            view_params.psvPrev = NULL;
            view_params.pfs = &folder_settings;
            view_params.psbOwner = shell_browser;
            view_params.prcView = &rect;
            view_params.pvid = NULL;
            view_params.hwndView = NULL;

            hr = IShellView2_CreateViewWindow2(shell_view2, &view_params);
            if (hr == E_FAIL)
            {
                win_skip("CreateViewWindow2 is broken on Vista/W2K8\n");
                goto cleanup;
            }
            ok(SUCCEEDED(hr), "CreateViewWindow2 returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView2_GetCurrentInfo(shell_view2, &folder_settings);
            ok(SUCCEEDED(hr), "GetCurrentInfo returned %#x\n", hr);
            ok(folder_settings.ViewMode == FVM_LIST,
               "view mode is %d, expected FVM_LIST\n",
               folder_settings.ViewMode);

            hr = IShellView2_DestroyViewWindow(shell_view2);
            ok(SUCCEEDED(hr), "DestroyViewWindow returned %#x\n", hr);

            /* XP and W2K3 need this. On W2K the call to DestroyWindow() fails and has
             * no side effects. NT4 doesn't get here. (FIXME: Vista doesn't get here yet).
             */
            DestroyWindow(view_params.hwndView);

            view_params.pvid = &VID_Details;
            hr = IShellView2_CreateViewWindow2(shell_view2, &view_params);
            ok(SUCCEEDED(hr), "CreateViewWindow2 returned %#x\n", hr);
            if (FAILED(hr)) goto cleanup;

            hr = IShellView2_GetCurrentInfo(shell_view2, &folder_settings);
            ok(SUCCEEDED(hr), "GetCurrentInfo returned %#x\n", hr);
            ok(folder_settings.ViewMode == FVM_DETAILS || broken(folder_settings.ViewMode == FVM_LIST), /* nt4 */
               "view mode is %d, expected FVM_DETAILS\n",
               folder_settings.ViewMode);

cleanup:
            if (shell_view2) IShellView2_Release(shell_view2);
            if (shell_view) IShellView_Release(shell_view);
            PostMessageA(GetParent(dlg), WM_COMMAND, IDCANCEL, 0);
        }
    }
    return 0;
}

static UINT_PTR WINAPI template_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
    {
        HWND p,cb;
        INT sel;
        p = GetParent(dlg);
        ok(p!=NULL, "Failed to get parent of template\n");
        cb = GetDlgItem(p,0x470);
        ok(cb!=NULL, "Failed to get filter combobox\n");
        sel = SendMessageA(cb, CB_GETCURSEL, 0, 0);
        ok (sel != -1, "Failed to get selection from filter listbox\n");
    }
    if (msg == WM_NOTIFY)
    {
        if (((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE)
            PostMessageA(GetParent(dlg), WM_COMMAND, IDCANCEL, 0);
    }
    return 0;
}

static void test_create_view_window2(void)
{
    OPENFILENAMEA ofn = {0};
    char filename[1024] = {0};
    DWORD ret;

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = create_view_window2_hook;
    ofn.Flags = OFN_ENABLEHOOK | OFN_EXPLORER;
    ret = GetOpenFileNameA(&ofn);
    ok(!ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
}

static void test_create_view_template(void)
{
    OPENFILENAMEA ofn = {0};
    char filename[1024] = {0};
    DWORD ret;

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = template_hook;
    ofn.Flags = OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATE;
    ofn.hInstance = GetModuleHandleA(NULL);
    ofn.lpTemplateName = "template1";
    ofn.lpstrFilter="text\0*.txt\0All\0*\0\0";
    ret = GetOpenFileNameA(&ofn);
    ok(!ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
}

/* test cases for resizing of the file dialog */
static const struct {
    DWORD flags;
    int resize_folderchange;/* change in CDN_FOLDERCHANGE handler */
    int resize_timer1;      /* change in first WM_TIMER handler */
    int resize_check;       /* expected change (in second  WM_TIMER handler) */
    BOOL todo;              /* mark that test todo_wine */
    BOOL testcontrols;      /* test resizing and moving of the controls */
} resize_testcases[] = {
    { 0                , 10, 10, 20,FALSE,FALSE},   /* 0 */
    { 0                ,-10,-10,-20,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0,  0,  0,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0,-10,  0,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0, 10, 10,FALSE, TRUE},
    { OFN_ENABLESIZING ,-10,  0, 10,FALSE,FALSE},   /* 5 */
    { OFN_ENABLESIZING , 10,  0, 10,FALSE,FALSE},
    { OFN_ENABLESIZING ,  0, 10, 20,FALSE,FALSE},
    /* mark the end */
    { 0xffffffff }
};

static UINT_PTR WINAPI resize_template_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static RECT initrc, rc;
    static int index, count;
    static BOOL gotSWP_bottom, gotShowWindow;
    HWND parent = GetParent( dlg);
    int resize;
#define MAXNRCTRLS 30
    static RECT ctrlrcs[MAXNRCTRLS];
    static int ctrlids[MAXNRCTRLS];
    static HWND ctrls[MAXNRCTRLS];
    static int nrctrls;

    switch( msg)
    {
        case WM_INITDIALOG:
        {
            DWORD style;

            index = ((OPENFILENAMEA*)lParam)->lCustData;
            count = 0;
            gotSWP_bottom = gotShowWindow = FALSE;
            /* test style */
            style = GetWindowLongA( parent, GWL_STYLE);
            if( resize_testcases[index].flags & OFN_ENABLESIZING)
                if( !(style & WS_SIZEBOX)) {
                    win_skip( "OFN_ENABLESIZING flag not supported.\n");
                    resizesupported = FALSE;
                    PostMessageA( parent, WM_COMMAND, IDCANCEL, 0);
                } else
                    ok( style & WS_SIZEBOX,
                            "testid %d: dialog should have a WS_SIZEBOX style.\n", index);
            else
                ok( !(style & WS_SIZEBOX),
                        "testid %d: dialog should not have a WS_SIZEBOX style.\n", index);
            break;
        }
        case WM_NOTIFY:
        {
            if(( (LPNMHDR)lParam)->code == CDN_FOLDERCHANGE){
                GetWindowRect( parent, &initrc);
                if( (resize  = resize_testcases[index].resize_folderchange)){
                    MoveWindow( parent, initrc.left,initrc.top, initrc.right - initrc.left + resize,
                            initrc.bottom - initrc.top + resize, TRUE);
                }
                SetTimer( dlg, 0, 100, 0);
            }
            break;
        }
        case WM_TIMER:
        {
            if( count == 0){
                /* store the control rectangles */
                if( resize_testcases[index].testcontrols) {
                    HWND ctrl;
                    int i;
                    for( i = 0, ctrl = GetWindow( parent, GW_CHILD);
                            i < MAXNRCTRLS && ctrl;
                            i++, ctrl = GetWindow( ctrl, GW_HWNDNEXT)) {
                        ctrlids[i] = GetDlgCtrlID( ctrl);
                        GetWindowRect( ctrl, &ctrlrcs[i]);
                        MapWindowPoints( NULL, parent, (LPPOINT) &ctrlrcs[i], 2);
                        ctrls[i] = ctrl;
                    }
                    nrctrls = i;
                }
                if( (resize  = resize_testcases[index].resize_timer1)){
                    GetWindowRect( parent, &rc);
                    MoveWindow( parent, rc.left,rc.top, rc.right - rc.left + resize,
                            rc.bottom - rc.top + resize, TRUE);
                }
            } else if( count == 1){
                resize  = resize_testcases[index].resize_check;
                GetWindowRect( parent, &rc);
                if( resize_testcases[index].todo){
                    todo_wine {
                        ok( resize == rc.right - rc.left - initrc.right + initrc.left,
                            "testid %d size-x change %d expected %d\n", index,
                            rc.right - rc.left - initrc.right + initrc.left, resize);
                        ok( resize == rc.bottom - rc.top - initrc.bottom + initrc.top,
                            "testid %d size-y change %d expected %d\n", index,
                            rc.bottom - rc.top - initrc.bottom + initrc.top, resize);
                    }
                }else{
                    ok( resize == rc.right - rc.left - initrc.right + initrc.left,
                        "testid %d size-x change %d expected %d\n", index,
                        rc.right - rc.left - initrc.right + initrc.left, resize);
                    ok( resize == rc.bottom - rc.top - initrc.bottom + initrc.top,
                        "testid %d size-y change %d expected %d\n", index,
                        rc.bottom - rc.top - initrc.bottom + initrc.top, resize);
                }
                if( resize_testcases[index].testcontrols) {
                    int i;
                    RECT rc;
                    for( i = 0; i < nrctrls; i++) {
                        GetWindowRect( ctrls[i], &rc);
                        MapWindowPoints( NULL, parent, (LPPOINT) &rc, 2);
                        switch( ctrlids[i]){

/* test if RECT R1, moved and sized result in R2 */
#define TESTRECTS( R1, R2, Mx, My, Sx, Sy) \
         ((R1).left + (Mx) ==(R2).left \
        &&(R1).top + (My) ==(R2).top \
        &&(R1).right + (Mx) + (Sx) == (R2).right \
        &&(R1).bottom + (My) + (Sy) ==(R2).bottom)

                            /* sized horizontal and moved vertical */
                            case cmb1:
                            case edt1:
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 10, 10, 0),
                                    "control id %03x should have sized horizontally and moved vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* sized horizontal and vertical */
                            case lst2:
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 0, 10, 10),
                                    "control id %03x should have sized horizontally and vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* moved horizontal and vertical */
                            case IDCANCEL:
                            case pshHelp:
                                ok( TESTRECTS( ctrlrcs[i], rc, 10, 10, 0, 0),
                                    "control id %03x should have moved horizontally and vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* moved vertically */
                            case chx1:
                            case stc2:
                            case stc3:
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 10, 0, 0),
                                    "control id %03x should have moved vertically, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* resized horizontal */
                            case cmb2: /* aka IDC_LOOKIN */
                                ok( TESTRECTS( ctrlrcs[i], rc, 0, 0, 10, 0)||
                                        TESTRECTS( ctrlrcs[i], rc, 0, 0, 0, 0), /* Vista and higher */
                                    "control id %03x should have resized horizontally, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* non moving non sizing controls */
                            case stc4:
                                ok( TESTRECTS( rc, ctrlrcs[i], 0, 0, 0, 0),
                                    "control id %03x was moved/resized, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* todo_wine: non moving non sizing controls */
                            case lst1:
todo_wine
                                ok( TESTRECTS( rc, ctrlrcs[i], 0, 0, 0, 0),
                                    "control id %03x was moved/resized, before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
                                break;
                            /* don't test: id is not unique */
                            case IDOK:
                            case stc1:
                            case 0:
                            case  -1:
                                break;
                            default:
                                trace("untested control id %03x before %d,%d-%d,%d after  %d,%d-%d,%d\n",
                                    ctrlids[i], ctrlrcs[i].left, ctrlrcs[i].top,
                                    ctrlrcs[i].right, ctrlrcs[i].bottom,
                                    rc.left, rc.top, rc.right, rc.bottom);
#undef TESTRECTS
#undef MAXNRCTRLS
                        }
                    }
                }
                KillTimer( dlg, 0);
                PostMessageA( parent, WM_COMMAND, IDCANCEL, 0);
            }
            count++;
        }
        break;
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *pwp = (WINDOWPOS *)lParam;
            if(  !index && pwp->hwndInsertAfter == HWND_BOTTOM){
                gotSWP_bottom = TRUE;
                ok(!gotShowWindow, "The WM_WINDOWPOSCHANGING message came after a WM_SHOWWINDOW message\n");
            }
        }
        break;
        case WM_SHOWWINDOW:
        {
            if(  !index){
                gotShowWindow = TRUE;
                ok(gotSWP_bottom, "No WM_WINDOWPOSCHANGING message came before a WM_SHOWWINDOW message\n");
            }
        }
        break;
    }
    return 0;
}

static void test_resize(void)
{
    OPENFILENAMEA ofn = { OPENFILENAME_SIZE_VERSION_400A };
    char filename[1024] = {0};
    DWORD ret;
    int i;

    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = resize_template_hook;
    ofn.hInstance = GetModuleHandleA(NULL);
    ofn.lpTemplateName = "template_sz";
    for( i = 0; resize_testcases[i].flags != 0xffffffff; i++) {
        ofn.lCustData = i;
        ofn.Flags = resize_testcases[i].flags |
            OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATE | OFN_SHOWHELP ;
        ret = GetOpenFileNameA(&ofn);
        ok(!ret, "GetOpenFileName returned %#x\n", ret);
        ret = CommDlgExtendedError();
        ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    }
}

/* test cases for control message IDOK */
/* Show case for bug #19079 */
typedef struct {
    int  retval;        /* return code of the message handler */
    BOOL setmsgresult;  /* set the result in the DWLP_MSGRESULT */
    BOOL usemsgokstr;   /* use the FILEOKSTRING message instead of WM_NOTIFY:CDN_FILEOK */
    BOOL do_subclass;   /* subclass the dialog hook procedure */
    BOOL expclose;      /* is the dialog expected to close ? */
    BOOL actclose;      /* has the dialog actually closed ? */
} ok_wndproc_testcase;

static ok_wndproc_testcase ok_testcases[] = {
    { 0,        FALSE,  FALSE,  FALSE,  TRUE},
    { 0,         TRUE,  FALSE,  FALSE,  TRUE},
    { 0,        FALSE,  FALSE,   TRUE,  TRUE},
    { 0,         TRUE,  FALSE,   TRUE,  TRUE},
    { 1,        FALSE,  FALSE,  FALSE,  TRUE},
    { 1,         TRUE,  FALSE,  FALSE, FALSE},
    { 1,        FALSE,  FALSE,   TRUE, FALSE},
    { 1,         TRUE,  FALSE,   TRUE, FALSE},
    /* FILEOKSTRING tests */
    { 1,         TRUE,   TRUE,  FALSE, FALSE},
    { 1,        FALSE,   TRUE,   TRUE, FALSE},
    /* mark the end */
    { -1 }
};

/* test_ok_wndproc can be used as hook procedure or a subclass
 * window proc for the file dialog */
static UINT_PTR WINAPI test_ok_wndproc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND parent = GetParent( dlg);
    static ok_wndproc_testcase *testcase = NULL;
    static UINT msgFILEOKSTRING;
    if (msg == WM_INITDIALOG)
    {
        testcase = (ok_wndproc_testcase*)((OPENFILENAMEA*)lParam)->lCustData;
        testcase->actclose = TRUE;
        msgFILEOKSTRING = RegisterWindowMessageA( FILEOKSTRINGA);
    }
    if( msg == WM_NOTIFY) {
        if(((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE) {
            SetTimer( dlg, 0, 100, 0);
            PostMessageA( parent, WM_COMMAND, IDOK, 0);
            return FALSE;
        } else if(((LPNMHDR)lParam)->code == CDN_FILEOK) {
            if( testcase->usemsgokstr)
                return FALSE;
            if( testcase->setmsgresult)
                SetWindowLongPtrA( dlg, DWLP_MSGRESULT, testcase->retval);
            return testcase->retval;
        }
    }
    if( msg == msgFILEOKSTRING) {
        if( !testcase->usemsgokstr)
            return FALSE;
        if( testcase->setmsgresult)
            SetWindowLongPtrA( dlg, DWLP_MSGRESULT, testcase->retval);
        return testcase->retval;
    }
    if( msg == WM_TIMER) {
        /* the dialog did not close automatically */
        testcase->actclose = FALSE;
        KillTimer( dlg, 0);
        PostMessageA( parent, WM_COMMAND, IDCANCEL, 0);
        return FALSE;
    }
    if( testcase && testcase->do_subclass)
        return DefWindowProcA( dlg, msg, wParam, lParam);
    return FALSE;
}

static UINT_PTR WINAPI ok_template_hook(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_SETFONT)
        SetWindowLongPtrA( dlg, GWLP_WNDPROC, (LONG_PTR) test_ok_wndproc);
    return FALSE;
}

static void test_ok(void)
{
    OPENFILENAMEA ofn = { OPENFILENAME_SIZE_VERSION_400A };
    char filename[1024] = {0};
    char tmpfilename[ MAX_PATH];
    char curdir[MAX_PATH];
    int i;
    DWORD ret;
    BOOL cdret;

    cdret = GetCurrentDirectoryA(sizeof(curdir), curdir);
    ok(cdret, "Failed to get current dir err %d\n", GetLastError());
    if (!GetTempFileNameA(".", "txt", 0, tmpfilename)) {
        skip("Failed to create a temporary file name\n");
        return;
    }
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.hInstance = GetModuleHandleA(NULL);
    ofn.lpTemplateName = "template1";
    ofn.Flags =  OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATE ;
    for( i = 0; ok_testcases[i].retval != -1; i++) {
        strcpy( filename, tmpfilename);
        ofn.lCustData = (LPARAM)(ok_testcases + i);
        ofn.lpfnHook = ok_testcases[i].do_subclass ? ok_template_hook : test_ok_wndproc;
        ret = GetOpenFileNameA(&ofn);
        ok( ok_testcases[i].expclose == ok_testcases[i].actclose,
                "testid %d: Open File dialog should %shave closed.\n", i,
                ok_testcases[i].expclose ? "" : "NOT ");
        ok(ret == ok_testcases[i].expclose, "testid %d: GetOpenFileName returned %#x\n", i, ret);
        ret = CommDlgExtendedError();
        ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
        cdret = SetCurrentDirectoryA(curdir);
        ok(cdret, "Failed to restore current dir err %d\n", GetLastError());
    }
    ret =  DeleteFileA( tmpfilename);
    ok( ret, "Failed to delete temporary file %s err %d\n", tmpfilename, GetLastError());
}

/* test arranging with a custom template */
typedef struct {
    int x, y;  /* left, top coordinates */
    int cx, cy; /* width and height */
} posz;
static struct {
   int nrcontrols;    /* 0: no controls, 1: just the stc32 control 2: with button */
   posz poszDlg;
   posz poszStc32;
   posz poszBtn;
   DWORD ofnflags;
} arrange_tests[] = {
    /* do not change the first two cases: used to get the uncustomized sizes */
    { 0, {0},{0},{0},0 },
    { 0, {0},{0},{0}, OFN_SHOWHELP},
    /* two tests with just a subdialog, no controls */
    { 0, {0, 0, 316, 76},{0},{0},0 },
    { 0, {0, 0, 100, 76},{0},{0}, OFN_SHOWHELP},
    /* now with a control with id stc32 */
    { 1, {0, 0, 316, 76} ,{0, 0, 204, 76,},{0},0 }, /* bug #17748*/
    { 1, {0, 0, 316, 76} ,{0, 0, 204, 76,},{0}, OFN_SHOWHELP}, /* bug #17748*/
    /* tests with size of the stc32 control higher or wider then the standard dialog */
    { 1, {0, 0, 316, 170} ,{0, 0, 204, 170,},{0},0 },
    { 1, {0, 0, 316, 165} ,{0, 0, 411, 165,},{0}, OFN_SHOWHELP },
    /* move the stc32 control around */
    { 1, {0, 0, 300, 100} ,{73, 17, 50, 50,},{0},0 },
    /* add control */
    { 2, {0, 0, 280, 100} ,{0, 0, 50, 50,},{300,20,30,30},0 },
    /* enable resizing should make the dialog bigger */
    { 0, {0},{0},{0}, OFN_SHOWHELP|OFN_ENABLESIZING},
    /* mark the end */
    { -1 }
};

static UINT_PTR WINAPI template_hook_arrange(HWND dlgChild, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int index, fixhelp;
    static posz posz0[2];
    static RECT clrcParent, clrcChild, rcStc32;
    static HWND hwndStc32;
    HWND dlgParent;

    dlgParent = GetParent( dlgChild);
    if (msg == WM_INITDIALOG) {
        index = ((OPENFILENAMEA*)lParam)->lCustData;
        /* get the positions before rearrangement */
        GetClientRect( dlgParent, &clrcParent);
        GetClientRect( dlgChild, &clrcChild);
        hwndStc32 = GetDlgItem( dlgChild, stc32);
        if( hwndStc32)  GetWindowRect( hwndStc32, &rcStc32);
    }
    if (msg == WM_NOTIFY && ((LPNMHDR)lParam)->code == CDN_FOLDERCHANGE) {
        RECT wrcParent;

        GetWindowRect( dlgParent, &wrcParent);
        /* the fist two "tests" just save the dialogs position, with and without
         * help button */
        if( index == 0) {
            posz0[0].x = wrcParent.left;
            posz0[0].y = wrcParent.top;
            posz0[0].cx = wrcParent.right - wrcParent.left;
            posz0[0].cy = wrcParent.bottom - wrcParent.top;
        } else if( index == 1) {
            posz0[1].x = wrcParent.left;
            posz0[1].y = wrcParent.top;
            posz0[1].cx = wrcParent.right - wrcParent.left;
            posz0[1].cy = wrcParent.bottom - wrcParent.top;
            fixhelp = posz0[1].cy - posz0[0].cy;
        } else {
            /* the real tests */
            int withhelp;
            int expectx, expecty;
            DWORD style;

            withhelp = (arrange_tests[index].ofnflags & OFN_SHOWHELP) != 0;
            GetWindowRect( dlgParent, &wrcParent);
            if( !hwndStc32) {
                /* case with no custom subitem with stc32:
                 * default to all custom controls below the standard */
                expecty = posz0[withhelp].cy + clrcChild.bottom;
                expectx =  posz0[withhelp].cx;
            } else {
                /* special case: there is a control with id stc32 */
                /* expected height */
                expecty = posz0[withhelp].cy;
                if( rcStc32.bottom - rcStc32.top + (withhelp ? 0 : fixhelp) > clrcParent.bottom) {
                    expecty +=  clrcChild.bottom -  clrcParent.bottom;
                    if( !withhelp) expecty += fixhelp;
                }
                else
                    expecty +=  clrcChild.bottom - ( rcStc32.bottom - rcStc32.top) ;
                /* expected width */
                expectx = posz0[withhelp].cx;
                if( rcStc32.right - rcStc32.left > clrcParent.right) {
                    expectx +=  clrcChild.right -  clrcParent.right;
                }
                else
                    expectx +=  clrcChild.right - ( rcStc32.right - rcStc32.left) ;
            }
            style = GetWindowLongA( dlgParent, GWL_STYLE);
            if( !(style & WS_SIZEBOX)) {
                /* without the OFN_ENABLESIZING flag */
                ok( wrcParent.bottom - wrcParent.top == expecty,
                        "Wrong height of dialog %d, expected %d\n",
                        wrcParent.bottom - wrcParent.top, expecty);
                ok( wrcParent.right - wrcParent.left == expectx,
                        "Wrong width of dialog %d, expected %d\n",
                        wrcParent.right - wrcParent.left, expectx);
            } else todo_wine {
                /* with the OFN_ENABLESIZING flag */
                ok( wrcParent.bottom - wrcParent.top > expecty,
                        "Wrong height of dialog %d, expected more than %d\n",
                        wrcParent.bottom - wrcParent.top, expecty);
                ok( wrcParent.right - wrcParent.left > expectx,
                        "Wrong width of dialog %d, expected more than %d\n",
                        wrcParent.right - wrcParent.left, expectx);
            }

        }
        PostMessageA( dlgParent, WM_COMMAND, IDCANCEL, 0);
    }
    return 0;
}

static void test_arrange(void)
{
    OPENFILENAMEA ofn = {0};
    char filename[1024] = {0};
    DWORD ret;
    HRSRC hRes;
    HANDLE hDlgTmpl;
    LPBYTE pv;
    DLGTEMPLATE *template;
    DLGITEMTEMPLATE *itemtemplateStc32, *itemtemplateBtn;
    int i;

    /* load subdialog template into memory */
    hRes = FindResourceA( GetModuleHandleA(NULL), "template_stc32", (LPSTR)RT_DIALOG);
    hDlgTmpl = LoadResource( GetModuleHandleA(NULL), hRes );
    /* get pointers to the structures for the dialog and the controls */
    pv = LockResource( hDlgTmpl );
    template = (DLGTEMPLATE*)pv;
    if( template->x != 11111) {
        win_skip("could not find the dialog template\n");
        return;
    }
    /* skip dialog template, menu, class and title */
    pv +=  sizeof(DLGTEMPLATE);
    pv += 3 * sizeof(WORD);
    /* skip font info */
    while( *(WORD*)pv)
        pv += sizeof(WORD);
    pv += sizeof(WORD);
    /* align on 32 bit boundaries */
    pv = (LPBYTE)(((UINT_PTR)pv + 3 ) & ~3);
    itemtemplateStc32 = (DLGITEMTEMPLATE*)pv;
    if( itemtemplateStc32->x != 22222) {
        win_skip("could not find the first item template\n");
        return;
    }
    /* skip itemtemplate, class, title and creation data */
    pv += sizeof(DLGITEMTEMPLATE);
    pv +=  4 * sizeof(WORD);
    /* align on 32 bit boundaries */
    pv = (LPBYTE)(((UINT_PTR)pv + 3 ) & ~3);
    itemtemplateBtn = (DLGITEMTEMPLATE*)pv;
    if( itemtemplateBtn->x != 12345) {
        win_skip("could not find the second item template\n");
        return;
    }

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = template_hook_arrange;
    ofn.hInstance = hDlgTmpl;
    ofn.lpstrFilter="text\0*.txt\0All\0*\0\0";
    for( i = 0; arrange_tests[i].nrcontrols != -1; i++) {
        ofn.lCustData = i;
        ofn.Flags = OFN_ENABLEHOOK | OFN_EXPLORER| OFN_ENABLETEMPLATEHANDLE | OFN_HIDEREADONLY |
            arrange_tests[i].ofnflags;
        template->cdit = arrange_tests[i].nrcontrols;
        template->x = arrange_tests[i].poszDlg.x;
        template->y = arrange_tests[i].poszDlg.y;
        template->cx = arrange_tests[i].poszDlg.cx;
        template->cy = arrange_tests[i].poszDlg.cy;
        itemtemplateStc32->x = arrange_tests[i].poszStc32.x;
        itemtemplateStc32->y = arrange_tests[i].poszStc32.y;
        itemtemplateStc32->cx = arrange_tests[i].poszStc32.cx;
        itemtemplateStc32->cy = arrange_tests[i].poszStc32.cy;
        itemtemplateBtn->x = arrange_tests[i].poszBtn.x;
        itemtemplateBtn->y = arrange_tests[i].poszBtn.y;
        itemtemplateBtn->cx = arrange_tests[i].poszBtn.cx;
        itemtemplateBtn->cy = arrange_tests[i].poszBtn.cy;
        ret = GetOpenFileNameA(&ofn);
        ok(!ret, "GetOpenFileNameA returned %#x\n", ret);
        ret = CommDlgExtendedError();
        ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    }
}

static CHAR SYSDIR[MAX_PATH];

static UINT_PTR CALLBACK path_hook_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR nmh;

    if( msg == WM_NOTIFY)
    {
        nmh = (LPNMHDR) lParam;
        if( nmh->code == CDN_INITDONE)
        {
            PostMessageA( GetParent(hDlg), WM_COMMAND, IDCANCEL, FALSE);
        }
        else if ( nmh->code == CDN_FOLDERCHANGE)
        {
            char buf[1024];
            int ret;

            memset(buf, 0x66, sizeof(buf));
            ret = SendMessageA( GetParent(hDlg), CDM_GETFOLDERPATH, sizeof(buf), (LPARAM)buf);
            ok(!lstrcmpiA(SYSDIR, buf), "Expected '%s', got '%s'\n", SYSDIR, buf);
            ok(lstrlenA(SYSDIR) + 1 == ret, "Expected %d, got %d\n", lstrlenA(SYSDIR) + 1, ret);
        }
    }

    return 0;
}

static void test_getfolderpath(void)
{
    OPENFILENAMEA ofn;
    BOOL result;
    char szFileName[MAX_PATH] = "";
    char szInitialDir[MAX_PATH];

    /* We need to pick a different directory as the other tests because of new
     * Windows 7 behavior.
     */
    GetSystemDirectoryA(szInitialDir, MAX_PATH);
    lstrcpyA(SYSDIR, szInitialDir);

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    ofn.lpstrDefExt = "txt";
    ofn.lpfnHook = path_hook_proc;
    ofn.lpstrInitialDir = szInitialDir;

    result = GetOpenFileNameA(&ofn);
    ok(FALSE == result, "expected FALSE, got %d\n", result);
    ok(0 == CommDlgExtendedError(), "expected 0, got %d\n",
       CommDlgExtendedError());

    result = GetSaveFileNameA(&ofn);
    ok(FALSE == result, "expected FALSE, got %d\n", result);
    ok(0 == CommDlgExtendedError(), "expected 0, got %d\n",
       CommDlgExtendedError());
}

static void test_resizable2(void)
{
    OPENFILENAMEA ofn = {0};
    char filename[1024] = "pls press Enter if sizable, Esc otherwise";
    DWORD ret;

    /* interactive because there is no hook function */
    if( !winetest_interactive) {
        skip( "some interactive resizable dialog tests (set WINETEST_INTERACTIVE=1)\n");
        return;
    }
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = 1024;
    ofn.lpfnHook = NULL;
    ofn.hInstance = GetModuleHandleA(NULL);
    ofn.lpTemplateName = "template1";
    ofn.Flags = OFN_EXPLORER;
#define ISSIZABLE TRUE
    ret = GetOpenFileNameA(&ofn);
    ok( ret == ISSIZABLE, "File Dialog should have been sizable\n");
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLETEMPLATE;
    ret = GetOpenFileNameA(&ofn);
    ok( ret != ISSIZABLE, "File Dialog should NOT have been sizable\n");
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLETEMPLATEHANDLE;
    ofn.hInstance = LoadResource( GetModuleHandleA(NULL), FindResourceA( GetModuleHandleA(NULL), "template1", (LPSTR)RT_DIALOG));
    ofn.lpTemplateName = NULL;
    ret = GetOpenFileNameA(&ofn);
    ok( ret != ISSIZABLE, "File Dialog should NOT have been sizable\n");
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLEHOOK;
    ret = GetOpenFileNameA(&ofn);
    ok( ret != ISSIZABLE, "File Dialog should NOT have been sizable\n");
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);
#undef ISSIZABLE
}

static void test_mru(void)
{
    ok_wndproc_testcase testcase = {0};
    OPENFILENAMEA ofn = { OPENFILENAME_SIZE_VERSION_400A };
    const char *test_dir_name = "C:\\mru_test";
    const char *test_file_name = "test.txt";
    const char *test_full_path = "C:\\mru_test\\test.txt";
    char filename_buf[MAX_PATH];
    DWORD ret;

    ofn.lpstrFile = filename_buf;
    ofn.nMaxFile = sizeof(filename_buf);
    ofn.lpTemplateName = "template1";
    ofn.hInstance = GetModuleHandleA(NULL);
    ofn.Flags =  OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_NOCHANGEDIR;
    ofn.lCustData = (LPARAM)&testcase;
    ofn.lpfnHook = test_ok_wndproc;

    SetLastError(0xdeadbeef);
    ret = CreateDirectoryA(test_dir_name, NULL);
    ok(ret == TRUE, "CreateDirectoryA should have succeeded: %d\n", GetLastError());

    /* "teach" comdlg32 about this directory */
    strcpy(filename_buf, test_full_path);
    SetLastError(0xdeadbeef);
    ret = GetOpenFileNameA(&ofn);
    ok(ret, "GetOpenFileNameA should have succeeded: %d\n", GetLastError());
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %x\n", ret);
    ok(testcase.actclose, "Open File dialog should have closed.\n");
    ok(!strcmp(ofn.lpstrFile, test_full_path), "Expected to get %s, got %s\n", test_full_path, ofn.lpstrFile);

    /* get a filename without a full path. it should return the file in
     * test_dir_name, not in the CWD */
    strcpy(filename_buf, test_file_name);
    SetLastError(0xdeadbeef);
    ret = GetOpenFileNameA(&ofn);
    ok(ret, "GetOpenFileNameA should have succeeded: %d\n", GetLastError());
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %x\n", ret);
    ok(testcase.actclose, "Open File dialog should have closed.\n");
    if(strcmp(ofn.lpstrFile, test_full_path) != 0)
        win_skip("Platform doesn't save MRU data\n");

    SetLastError(0xdeadbeef);
    ret = RemoveDirectoryA(test_dir_name);
    ok(ret == TRUE, "RemoveDirectoryA should have succeeded: %d\n", GetLastError());
}

static UINT_PTR WINAPI test_extension_wndproc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND parent = GetParent( dlg);
    if( msg == WM_NOTIFY) {
        SetTimer( dlg, 0, 1000, 0);
        PostMessageA( parent, WM_COMMAND, IDOK, 0);
    }
    if( msg == WM_TIMER) {
        /* the dialog did not close automatically */
        KillTimer( dlg, 0);
        PostMessageA( parent, WM_COMMAND, IDCANCEL, 0);
    }
    return FALSE;
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

static void test_extension_helper(OPENFILENAMEA* ofn, const char *filter,
                                  const char *expected_filename)
{
    char *filename_ptr;
    DWORD ret;
    BOOL boolret;

    strcpy(ofn->lpstrFile, "deadbeef");
    ofn->lpstrFilter = filter;

    boolret = GetSaveFileNameA(ofn);
    ok(boolret, "%s: expected TRUE\n", filter);

    ret = CommDlgExtendedError();
    ok(!ret, "%s: CommDlgExtendedError returned %#x\n", filter, ret);

    filename_ptr = ofn->lpstrFile + ofn->nFileOffset;
    ok(strcmp(filename_ptr, expected_filename) == 0,
        "%s: Filename is %s, expected %s\n", filter, filename_ptr, expected_filename);
}

static void test_extension(void)
{
    OPENFILENAMEA ofn = { OPENFILENAME_SIZE_VERSION_400A };
    char filename[1024] = {0};
    char curdir[MAX_PATH];
    unsigned int i;
    BOOL boolret;

    const char *defext_concrete_filters[] = {
        "TestFilter (*.abc)\0*.abc\0",
        "TestFilter (*.abc;)\0*.abc;\0",
        "TestFilter (*.abc;*.def)\0*.abc;*.def\0",
    };

    const char *defext_wildcard_filters[] = {
        "TestFilter (*.pt*)\0*.pt*\0",
        "TestFilter (*.pt*;*.abc)\0*.pt*;*.abc\0",
        "TestFilter (*.ab?)\0*.ab?\0",
        "TestFilter (*.*)\0*.*\0",
        "TestFilter (*sav)\0*sav\0",
        NULL    /* is a test, not an endmark! */
    };

    boolret = GetCurrentDirectoryA(sizeof(curdir), curdir);
    ok(boolret, "Failed to get current dir err %d\n", GetLastError());

    ofn.hwndOwner = NULL;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_ENABLEHOOK;
    ofn.lpstrInitialDir = curdir;
    ofn.lpfnHook = test_extension_wndproc;
    ofn.nFileExtension = 0;

    ofn.lpstrDefExt = NULL;

    /* Without lpstrDefExt, append no extension */
    test_extension_helper(&ofn, "TestFilter (*.abc) lpstrDefExt=NULL\0*.abc\0", "deadbeef");
    test_extension_helper(&ofn, "TestFilter (*.ab?) lpstrDefExt=NULL\0*.ab?\0", "deadbeef");

    ofn.lpstrDefExt = "";

    /* If lpstrDefExt="" and the filter has a concrete extension, append it */
    test_extension_helper(&ofn, "TestFilter (*.abc) lpstrDefExt=\"\"\0*.abc\0", "deadbeef.abc");

    /* If lpstrDefExt="" and the filter has a wildcard extension, do nothing */
    test_extension_helper(&ofn, "TestFilter (*.ab?) lpstrDefExt=\"\"\0*.ab?\0", "deadbeef");

    ofn.lpstrDefExt = "xyz";

    /* Append concrete extensions from filters */
    for (i = 0; i < ARRAY_SIZE(defext_concrete_filters); i++) {
        test_extension_helper(&ofn, defext_concrete_filters[i], "deadbeef.abc");
    }

    /* Append nothing from this filter */
    test_extension_helper(&ofn, "TestFilter (*.)\0*.\0", "deadbeef");

    /* Ignore wildcard extensions in filters */
    for (i = 0; i < ARRAY_SIZE(defext_wildcard_filters); i++) {
        test_extension_helper(&ofn, defext_wildcard_filters[i], "deadbeef.xyz");
    }

    /* Append valid extensions consisting of multiple parts */
    test_extension_helper(&ofn, "TestFilter (*.abc.def)\0*.abc.def\0", "deadbeef.abc.def");
    test_extension_helper(&ofn, "TestFilter (.abc.def)\0.abc.def\0", "deadbeef.abc.def");
    test_extension_helper(&ofn, "TestFilter (*.*.def)\0*.*.def\0", "deadbeef.xyz");
}

#undef ARRAY_SIZE


static BOOL WINAPI test_null_enum(HWND hwnd, LPARAM lParam)
{
    /* Find the textbox and send a filename so IDOK will work.
       If the file textbox is empty IDOK will be ignored */
    CHAR className[20];
    if(GetClassNameA(hwnd, className, sizeof(className)) > 0 && !strcmp("Edit",className))
    {
        SetWindowTextA(hwnd, "testfile");
        return FALSE; /* break window enumeration */
    }
    return TRUE;
}

static UINT_PTR WINAPI test_null_wndproc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND parent = GetParent( dlg);
    if( msg == WM_NOTIFY) {
        SetTimer( dlg, 0, 100, 0);
        SetTimer( dlg, 1, 1000, 0);
        EnumChildWindows( parent, test_null_enum, 0);
    }
    if( msg == WM_TIMER) {
        if(!wParam)
            PostMessageA( parent, WM_COMMAND, IDOK, 0);
        else {
            /* the dialog did not close automatically */
            KillTimer( dlg, 0);
            PostMessageA( parent, WM_COMMAND, IDCANCEL, 0);
        }
    }
    return FALSE;
}

static void test_null_filename(void)
{
    OPENFILENAMEA ofnA = {0};
    OPENFILENAMEW ofnW = {0};
    WCHAR filterW[] = {'t','e','x','t','\0','*','.','t','x','t','\0',
                       'A','l','l','\0','*','\0','\0'};
    DWORD ret;

    ofnA.lStructSize = OPENFILENAME_SIZE_VERSION_400A;
    ofnA.lpstrFile = NULL;
    ofnA.nMaxFile = 0;
    ofnA.nFileOffset = 0xdead;
    ofnA.nFileExtension = 0xbeef;
    ofnA.lpfnHook = test_null_wndproc;
    ofnA.Flags = OFN_ENABLEHOOK | OFN_EXPLORER;
    ofnA.hInstance = GetModuleHandleA(NULL);
    ofnA.lpstrFilter = "text\0*.txt\0All\0*\0\0";
    ofnA.lpstrDefExt = NULL;
    ret = GetOpenFileNameA(&ofnA);
    todo_wine ok(ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    todo_wine ok(!ret, "CommDlgExtendedError returned %#x, should be 0\n", ret);

    todo_wine ok(ofnA.nFileOffset != 0xdead, "ofnA.nFileOffset is 0xdead\n");
    todo_wine ok(ofnA.nFileExtension != 0xbeef, "ofnA.nFileExtension is 0xbeef\n");

    ofnA.lpstrFile = NULL;
    ofnA.nMaxFile = 1024; /* bogus input - lpstrFile = NULL but fake 1024 bytes available */
    ofnA.nFileOffset = 0xdead;
    ofnA.nFileExtension = 0xbeef;
    ret = GetOpenFileNameA(&ofnA);
    ok(ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);

    ok(ofnA.nFileOffset != 0xdead, "ofnA.nFileOffset is 0xdead\n");
    ok(ofnA.nFileExtension == 0, "ofnA.nFileExtension is 0x%x, should be 0\n", ofnA.nFileExtension);

    /* unicode tests */
    ofnW.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofnW.lpstrFile = NULL;
    ofnW.nMaxFile = 0;
    ofnW.nFileOffset = 0xdead;
    ofnW.nFileExtension = 0xbeef;
    ofnW.lpfnHook = test_null_wndproc;
    ofnW.Flags = OFN_ENABLEHOOK | OFN_EXPLORER;
    ofnW.hInstance = GetModuleHandleW(NULL);
    ofnW.lpstrFilter = filterW;
    ofnW.lpstrDefExt = NULL;
    ret = GetOpenFileNameW(&ofnW);
    todo_wine ok(ret, "GetOpenFileNameW returned %#x\n", ret);
    ret = CommDlgExtendedError();
    todo_wine ok(!ret, "CommDlgExtendedError returned %#x\n", ret);

    todo_wine ok(ofnW.nFileOffset != 0xdead, "ofnW.nFileOffset is 0xdead\n");
    todo_wine ok(ofnW.nFileExtension != 0xbeef, "ofnW.nFileExtension is 0xbeef\n");

    ofnW.lpstrFile = NULL;
    ofnW.nMaxFile = 1024; /* bogus input - lpstrFile = NULL but fake 1024 bytes available */
    ofnW.nFileOffset = 0xdead;
    ofnW.nFileExtension = 0xbeef;
    ret = GetOpenFileNameW(&ofnW);
    ok(ret, "GetOpenFileNameA returned %#x\n", ret);
    ret = CommDlgExtendedError();
    ok(!ret, "CommDlgExtendedError returned %#x\n", ret);

    ok(ofnW.nFileOffset != 0xdead, "ofnW.nFileOffset is 0xdead\n");
    ok(ofnW.nFileExtension == 0, "ofnW.nFileExtension is 0x%x, should be 0\n", ofnW.nFileExtension);
}

START_TEST(filedlg)
{
    test_DialogCancel();
    test_create_view_window2();
    test_create_view_template();
    test_arrange();
    test_resize();
    test_ok();
    test_getfolderpath();
    test_mru();
    if( resizesupported) test_resizable2();
    test_extension();
    test_null_filename();
}
