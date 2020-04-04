/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for IShellFolderViewCB
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "shelltest.h"
#include <atlsimpcoll.h>
#include <stdio.h>
#include <shellutils.h>
//#include <msgtrace.h>

static DWORD g_WinVersion;
#define WINVER_VISTA   0x0600

#ifndef SFVM_SELECTIONCHANGED
#define SFVM_SELECTIONCHANGED          8 /* undocumented */
#define SFVM_DRAWMENUITEM              9 /* undocumented */
#define SFVM_MEASUREMENUITEM          10 /* undocumented */
#define SFVM_EXITMENULOOP             11 /* undocumented */
#define SFVM_VIEWRELEASE              12 /* undocumented */
#define SFVM_GETNAMELENGTH            13 /* undocumented */
#define SFVM_WINDOWCLOSING            16 /* undocumented */
#define SFVM_LISTREFRESHED            17 /* undocumented */
#define SFVM_WINDOWFOCUSED            18 /* undocumented */
#define SFVM_REGISTERCOPYHOOK         20 /* undocumented */
#define SFVM_COPYHOOKCALLBACK         21 /* undocumented */
#define SFVM_UNMERGEFROMMENU          28 /* undocumented */
#define SFVM_ADDINGOBJECT             29 /* undocumented */
#define SFVM_REMOVINGOBJECT           30 /* undocumented */
#define SFVM_GETCOMMANDDIR            33 /* undocumented */
#define SFVM_GETCOLUMNSTREAM          34 /* undocumented */
#define SFVM_CANSELECTALL             35 /* undocumented */
#define SFVM_ISSTRICTREFRESH          37 /* undocumented */
#define SFVM_ISCHILDOBJECT            38 /* undocumented */
#define SFVM_GETEXTVIEWS              40 /* undocumented */
#define SFVM_GET_CUSTOMVIEWINFO       77 /* undocumented */
#define SFVM_ENUMERATEDITEMS          79 /* undocumented */
#define SFVM_GET_VIEW_DATA            80 /* undocumented */
#define SFVM_GET_WEBVIEW_LAYOUT       82 /* undocumented */
#define SFVM_GET_WEBVIEW_CONTENT      83 /* undocumented */
#define SFVM_GET_WEBVIEW_TASKS        84 /* undocumented */
#define SFVM_GET_WEBVIEW_THEME        86 /* undocumented */
#define SFVM_GETDEFERREDVIEWSETTINGS  92 /* undocumented */
#endif

#define DUM_MSG_GetWindow               400
#define DUM_MSG_ContextSensitiveHelp    401
#define DUM_MSG_InsertMenusSB           402
#define DUM_MSG_SetMenuSB               403
#define DUM_MSG_RemoveMenusSB           404
#define DUM_MSG_SetStatusTextSB         405
#define DUM_MSG_EnableModelessSB        406
#define DUM_MSG_TranslateAcceleratorSB  407
#define DUM_MSG_BrowseObject            408
#define DUM_MSG_GetViewStateStream      409
#define DUM_MSG_GetControlWindow        410
#define DUM_MSG_SendControlMsg          411
#define DUM_MSG_QueryActiveShellView    412
#define DUM_MSG_OnViewWindowActive      413
#define DUM_MSG_SetToolbarItems         414


const char* msg2str(UINT uMsg)
{
    static char buf[2][50];
    static int index = 0;
    index ^= 1;

    switch (uMsg)
    {
    case SFVM_MERGEMENU: return "SFVM_MERGEMENU";
    case SFVM_INVOKECOMMAND: return "SFVM_INVOKECOMMAND";
    case SFVM_GETHELPTEXT: return "SFVM_GETHELPTEXT";
    case SFVM_GETTOOLTIPTEXT: return "SFVM_GETTOOLTIPTEXT";
    case SFVM_GETBUTTONINFO: return "SFVM_GETBUTTONINFO";
    case SFVM_GETBUTTONS: return "SFVM_GETBUTTONS";
    case SFVM_INITMENUPOPUP: return "SFVM_INITMENUPOPUP";
    case SFVM_SELECTIONCHANGED: return "SFVM_SELECTIONCHANGED";
    case SFVM_DRAWMENUITEM: return "SFVM_DRAWMENUITEM";
    case SFVM_MEASUREMENUITEM: return "SFVM_MEASUREMENUITEM";
    case SFVM_EXITMENULOOP: return "SFVM_EXITMENULOOP";
    case SFVM_VIEWRELEASE: return "SFVM_VIEWRELEASE";
    case SFVM_GETNAMELENGTH: return "SFVM_GETNAMELENGTH";
    case SFVM_FSNOTIFY: return "SFVM_FSNOTIFY";
    case SFVM_WINDOWCREATED: return "SFVM_WINDOWCREATED";
    case SFVM_WINDOWCLOSING: return "SFVM_WINDOWCLOSING";
    case SFVM_LISTREFRESHED: return "SFVM_LISTREFRESHED";
    case SFVM_WINDOWFOCUSED: return "SFVM_WINDOWFOCUSED";
    case SFVM_REGISTERCOPYHOOK: return "SFVM_REGISTERCOPYHOOK";
    case SFVM_COPYHOOKCALLBACK: return "SFVM_COPYHOOKCALLBACK";
    case SFVM_GETDETAILSOF: return "SFVM_GETDETAILSOF";
    case SFVM_COLUMNCLICK: return "SFVM_COLUMNCLICK";
    case SFVM_QUERYFSNOTIFY: return "SFVM_QUERYFSNOTIFY";
    case SFVM_DEFITEMCOUNT: return "SFVM_DEFITEMCOUNT";
    case SFVM_DEFVIEWMODE: return "SFVM_DEFVIEWMODE";
    case SFVM_UNMERGEFROMMENU: return "SFVM_UNMERGEFROMMENU";
    case SFVM_ADDINGOBJECT: return "SFVM_ADDINGOBJECT";
    case SFVM_REMOVINGOBJECT: return "SFVM_REMOVINGOBJECT";
    case SFVM_UPDATESTATUSBAR: return "SFVM_UPDATESTATUSBAR";
    case SFVM_BACKGROUNDENUM: return "SFVM_BACKGROUNDENUM";
    case SFVM_GETCOMMANDDIR: return "SFVM_GETCOMMANDDIR";
    case SFVM_GETCOLUMNSTREAM: return "SFVM_GETCOLUMNSTREAM";
    case SFVM_CANSELECTALL: return "SFVM_CANSELECTALL";
    case SFVM_DIDDRAGDROP: return "SFVM_DIDDRAGDROP";
    case SFVM_ISSTRICTREFRESH: return "SFVM_ISSTRICTREFRESH";
    case SFVM_ISCHILDOBJECT: return "SFVM_ISCHILDOBJECT";
    case SFVM_SETISFV: return "SFVM_SETISFV";
    case SFVM_GETEXTVIEWS: return "SFVM_GETEXTVIEWS";
    case SFVM_THISIDLIST: return "SFVM_THISIDLIST";
    case SFVM_ADDPROPERTYPAGES: return "SFVM_ADDPROPERTYPAGES";
    case SFVM_BACKGROUNDENUMDONE: return "SFVM_BACKGROUNDENUMDONE";
    case SFVM_GETNOTIFY: return "SFVM_GETNOTIFY";
    case SFVM_GETSORTDEFAULTS: return "SFVM_GETSORTDEFAULTS";
    case SFVM_SIZE: return "SFVM_SIZE";
    case SFVM_GETZONE: return "SFVM_GETZONE";
    case SFVM_GETPANE: return "SFVM_GETPANE";
    case SFVM_GETHELPTOPIC: return "SFVM_GETHELPTOPIC";
    case SFVM_GETANIMATION: return "SFVM_GETANIMATION";
    case SFVM_GET_CUSTOMVIEWINFO: return "SFVM_GET_CUSTOMVIEWINFO";
    case SFVM_ENUMERATEDITEMS: return "SFVM_ENUMERATEDITEMS";
    case SFVM_GET_VIEW_DATA: return "SFVM_GET_VIEW_DATA";
    case SFVM_GET_WEBVIEW_LAYOUT: return "SFVM_GET_WEBVIEW_LAYOUT";
    case SFVM_GET_WEBVIEW_CONTENT: return "SFVM_GET_WEBVIEW_CONTENT";
    case SFVM_GET_WEBVIEW_TASKS: return "SFVM_GET_WEBVIEW_TASKS";
    case SFVM_GET_WEBVIEW_THEME: return "SFVM_GET_WEBVIEW_THEME";
    case SFVM_GETDEFERREDVIEWSETTINGS: return "SFVM_GET_WEBVIEW_THEME";

    case DUM_MSG_GetWindow: return "|GetWindow|";
    case DUM_MSG_ContextSensitiveHelp: return "|ContextSensitiveHelp|";
    case DUM_MSG_InsertMenusSB: return "|InsertMenusSB|";
    case DUM_MSG_SetMenuSB: return "|SetMenuSB|";
    case DUM_MSG_RemoveMenusSB: return "|RemoveMenusSB|";
    case DUM_MSG_SetStatusTextSB: return "|SetStatusTextSB|";
    case DUM_MSG_EnableModelessSB: return "|EnableModelessSB|";
    case DUM_MSG_TranslateAcceleratorSB: return "|TranslateAcceleratorSB|";
    case DUM_MSG_BrowseObject: return "|BrowseObject|";
    case DUM_MSG_GetViewStateStream: return "|GetViewStateStream|";
    case DUM_MSG_GetControlWindow: return "|GetControlWindow|";
    case DUM_MSG_SendControlMsg: return "|SendControlMsg|";
    case DUM_MSG_QueryActiveShellView: return "|QueryActiveShellView|";
    case DUM_MSG_OnViewWindowActive: return "|OnViewWindowActive|";
    case DUM_MSG_SetToolbarItems: return "|SetToolbarItems|";
    default:
        sprintf(buf[index], "[%u]", uMsg);
        return buf[index];
    }
}


#define PTR_VALUE   0xf7f7f7f7

struct message
{
    message(UINT msg, WPARAM wp, LPARAM lp) : uMsg(msg), wParam(wp), lParam(lp) { ; }

    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
};

CSimpleArray<message> g_Received;

void clear_list()
{
    g_Received.RemoveAll();
}

void add_msg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    g_Received.Add(message(uMsg, wParam, lParam));
}

void print_list_(const CSimpleArray<message>& input, const char* file, int line)
{
    trace_(file, line)("Got list:\n");
    for (int n = 0; n < input.GetSize(); ++n)
    {
        const message& msg = input[n];
        trace_(file, line)("msg: %d(%s), wParam:0x%x, lParam:0x%lx\n", msg.uMsg, msg2str(msg.uMsg), msg.wParam, msg.lParam);
    }
    trace_(file, line)("End of list.\n");
}

void compare_list_(const CSimpleArray<message>& input, const message* compare, const char* file, int line)
{
    int input_item = 0;
    int compare_item = 0;
    LONG old_failures = winetest_get_failures();
    while (compare[compare_item].uMsg && input_item < input.GetSize())
    {
        const message& inp = input[input_item];
        const message& cmp = compare[compare_item];
        if (cmp.uMsg == inp.uMsg)
        {
            if (cmp.lParam != (LPARAM)PTR_VALUE)
            {
                ok_(file, line)(cmp.lParam == inp.lParam, "Expected lParam to be 0x%lx, was 0x%lx for %i(%s)\n",
                                cmp.lParam, inp.lParam, compare_item, msg2str(cmp.uMsg));
            }
            else
            {
                ok_(file, line)(inp.lParam != 0, "Expected lParam to be a pointer, was 0 for %i(%s)\n",
                                compare_item, msg2str(cmp.uMsg));
            }
            if (cmp.wParam != PTR_VALUE)
            {
                ok_(file, line)(cmp.wParam == inp.wParam, "Expected wParam to be 0x%x, was 0x%x for %i(%s)\n",
                                cmp.wParam, inp.wParam, compare_item, msg2str(cmp.uMsg));
            }
            else
            {
                ok_(file, line)(inp.wParam != 0, "Expected wParam to be a pointer, was 0 for %i(%s)\n",
                                compare_item, msg2str(cmp.uMsg));
            }
            compare_item++;
        }
        else
        {
            /* We skip unknown items for now */
        }

        input_item++;
    }

    while (compare[compare_item].uMsg)
    {
        ok_(file, line)(0, "Message %i(%s) not found\n", compare_item, msg2str(compare[compare_item].uMsg));
        compare_item++;
    }
    if (old_failures != winetest_get_failures())
    {
        print_list_(input, file, line);
    }
}

#define compare_list(cmp)   compare_list_(g_Received, cmp, __FILE__, __LINE__)
#define print_list()        print_list_(g_Received, __FILE__, __LINE__)


LONG g_AddRef = 0;
LONG g_Release = 0;

class CFolderViewCB :
    public IShellFolderViewCB
{
public:
    CFolderViewCB(void) :
        m_RefCount(1)
    {
    }
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void  **ppvObject)
    {
        if (riid == IID_IShellFolderViewCB)
        {
            *ppvObject = static_cast<IShellFolderViewCB*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        InterlockedIncrement(&g_AddRef);
        return InterlockedIncrement(&m_RefCount);
    }
    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        InterlockedIncrement(&g_Release);
        return InterlockedDecrement(&m_RefCount);
    }
    virtual HRESULT STDMETHODCALLTYPE MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        //DbgPrint("MessageSFVCB(uMsg:%s, wParam:%u, lParam:%u\n", msg2str(uMsg), wParam, lParam);
        add_msg(uMsg, wParam, lParam);
        return E_NOTIMPL;
    }
private:
    LONG m_RefCount;
};


static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


class CDummyShellBrowser : public IShellBrowser
{
public:
    HWND m_hwnd;
    CDummyShellBrowser()
    {
        static const TCHAR* class_name = TEXT("DUMMY_TEST_CLASS");
        WNDCLASSEX wx = {};
        wx.cbSize = sizeof(WNDCLASSEX);
        wx.lpfnWndProc = WndProc;
        wx.hInstance = GetModuleHandle(NULL);
        wx.lpszClassName = class_name;
        wx.style = CS_DBLCLKS;
        RegisterClassEx(&wx);
        m_hwnd = CreateWindowEx(0, class_name, TEXT("dummy_name"), WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
        ShowWindow(m_hwnd, SW_SHOW);
    }

    // *** IUnknown methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void  **ppvObject)
    {
        if (riid == IID_IShellBrowser || riid == IID_IUnknown)
        {
            *ppvObject = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return 2;
    }
    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        return 1;
    }

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        *phwnd = m_hwnd;
        add_msg(DUM_MSG_GetWindow, NULL, NULL);
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_ContextSensitiveHelp, NULL, NULL);
        return E_NOTIMPL;
    }

    // *** IShellBrowser methods ***
    virtual HRESULT STDMETHODCALLTYPE InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_InsertMenusSB, NULL, NULL);
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_SetMenuSB, NULL, NULL);
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE RemoveMenusSB(HMENU hmenuShared)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_RemoveMenusSB, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE SetStatusTextSB(LPCWSTR pszStatusText)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_SetStatusTextSB, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE EnableModelessSB(BOOL fEnable)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_EnableModelessSB, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorSB(MSG *pmsg, WORD wID)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_TranslateAcceleratorSB, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE BrowseObject(PCUIDLIST_RELATIVE pidl,UINT wFlags)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_BrowseObject, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetViewStateStream(DWORD grfMode,IStream **ppStrm)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_GetViewStateStream, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetControlWindow(UINT id,HWND *phwnd)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_GetControlWindow, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE SendControlMsg(UINT id,UINT uMsg,WPARAM wParam,LPARAM lParam,LRESULT *pret)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_SendControlMsg, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(IShellView **ppshv)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_QueryActiveShellView, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnViewWindowActive(IShellView *pshv)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_OnViewWindowActive, NULL, NULL);
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE SetToolbarItems(LPTBBUTTONSB lpButtons,UINT nButtons,UINT uFlags)
    {
        //DbgPrint("%s()\n", __FUNCTION__);
        add_msg(DUM_MSG_SetToolbarItems, NULL, NULL);
        return E_NOTIMPL;
    }
};


START_TEST(IShellFolderViewCB)
{
    RTL_OSVERSIONINFOEXW rtlinfo = { sizeof(rtlinfo) };
    void (__stdcall* pRtlGetVersion)(RTL_OSVERSIONINFOEXW*);
    pRtlGetVersion = (void (__stdcall*)(RTL_OSVERSIONINFOEXW*))GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");
    pRtlGetVersion(&rtlinfo);
    g_WinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;

    CoInitialize(NULL);
    CFolderViewCB cb;

    CComPtr<IShellFolder> desktop;

    HRESULT hr = SHGetDesktopFolder(&desktop);
    ok_hex(hr, S_OK);
    if (!desktop)
        return;

    SFV_CREATE sfvc = { sizeof(sfvc), desktop };
    sfvc.psfvcb = &cb;
    IShellView* psv = NULL;

    g_AddRef = 0;
    g_Release = 0;

    hr = SHCreateShellFolderView(&sfvc, &psv);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_int(g_AddRef, 1);
    ok_int(g_Release, 0);

    clear_list();

    HWND wnd;
    RECT rc = { 0 };
    FOLDERSETTINGS fs = { FVM_DETAILS, 0 };
    CDummyShellBrowser dum;
    hr = psv->CreateViewWindow(NULL, &fs, &dum, &rc, &wnd);


    static message init_list[] =
    {
        /* ... */
        message(DUM_MSG_GetWindow, 0, 0),
        /* Some unknown messages here, and multiple SFVM_GET_WEBVIEW_THEME + SFVM_GETSORTDEFAULTS + SFVM_GETCOLUMNSTREAM */
        message(SFVM_SIZE, 0, 0),
        /* ... */
        message(DUM_MSG_InsertMenusSB, 0, 0),
        message(SFVM_MERGEMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),
        /* ... */
        message(SFVM_WINDOWCREATED, PTR_VALUE, 0),
        /* ... */

        message(SFVM_GETBUTTONINFO, 0, PTR_VALUE),
        message(SFVM_GETBUTTONS, PTR_VALUE, PTR_VALUE),
        message(DUM_MSG_SetToolbarItems, 0, 0),

        message(0, 0, 0)
    };


    static message init_list_vista[] =
    {
        /* Some unknown messages here */
        message(DUM_MSG_GetWindow, 0, 0),
        /* Some unknown messages here, and multiple SFVM_GET_WEBVIEW_THEME + SFVM_GETSORTDEFAULTS + SFVM_GETCOLUMNSTREAM */
        message(SFVM_SIZE, 0, 0),
        message(SFVM_WINDOWCREATED, PTR_VALUE, 0),
        /* Some unknown messages here */
        message(DUM_MSG_InsertMenusSB, 0, 0),
        message(SFVM_MERGEMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),

        message(0, 0, 0)
    };

    if (g_WinVersion < WINVER_VISTA)
    {
        compare_list(init_list);
    }
    else
    {
        compare_list(init_list_vista);
    }
    clear_list();

    hr = psv->Refresh();
    //ok_hex(hr, S_FALSE);

    static message refresh_list[] =
    {
        message(SFVM_LISTREFRESHED, 1, 0),

        message(SFVM_UNMERGEFROMMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),
        message(DUM_MSG_RemoveMenusSB, 0, 0),
        message(DUM_MSG_InsertMenusSB, 0, 0),
        message(SFVM_MERGEMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),

        message(0, 0, 0)
    };

    static message refresh_list_vista[] =
    {
        message(SFVM_LISTREFRESHED, 1, 0),

        message(SFVM_UNMERGEFROMMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),
        message(DUM_MSG_RemoveMenusSB, 0, 0),
        message(DUM_MSG_InsertMenusSB, 0, 0),
        message(SFVM_MERGEMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),
        /* Some messages here, like SFVM_GET_WEBVIEW_THEME, SFVM_GETSORTDEFAULTS, SFVM_GETCOLUMNSTREAM */
        message(SFVM_UNMERGEFROMMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),
        message(DUM_MSG_RemoveMenusSB, 0, 0),
        message(DUM_MSG_InsertMenusSB, 0, 0),
        message(SFVM_MERGEMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),

        message(0, 0, 0)
    };

    if (g_WinVersion < WINVER_VISTA)
    {
        compare_list(refresh_list);
    }
    else
    {
        compare_list(refresh_list_vista);
    }
    clear_list();

    hr = psv->DestroyViewWindow();

    static message destroy_list[] =
    {
        message(SFVM_UNMERGEFROMMENU, 0, PTR_VALUE),
        message(DUM_MSG_SetMenuSB, 0, 0),
        message(DUM_MSG_RemoveMenusSB, 0, 0),
        message(SFVM_WINDOWCLOSING, PTR_VALUE, 0),

        message(0, 0, 0)
    };

    compare_list(destroy_list);
    clear_list();

    CComPtr<IShellFolderView> folderView;
    hr = psv->QueryInterface(IID_PPV_ARG(IShellFolderView, &folderView));
    ok_hex(hr, S_OK);
    if (SUCCEEDED(hr))
    {
        IShellFolderViewCB* oldPtr;

        hr = folderView->SetCallback(NULL, &oldPtr);
        ok_int(g_AddRef, 1);
        ok_int(g_Release, 0);

        /* Last pointer is not optional! */
        IShellFolderViewCB* oldPtr2;
        hr = folderView->SetCallback(oldPtr, &oldPtr2);
        ok_int(g_AddRef, 2);
        ok_int(g_Release, 0);
    }

    ULONG refCount = psv->Release();
    ok(refCount == 1, "refCount = %lu\n", refCount);

    static message release_list[] =
    {
        message(SFVM_VIEWRELEASE, 0, 0),

        message(0, 0, 0)
    };

    /* Investigate why this fails */
    if (refCount == 0)
    {
        compare_list(release_list);
    }
}
