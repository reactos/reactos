/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CISFBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "precomp.h"
#include <mshtmcid.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

WINE_DEFAULT_DEBUG_CHANNEL(qcklnch);

//Misc Methods:
//Subclassing Button

LRESULT CALLBACK MyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        MessageBox(0, L"Button Clicked!!", L"Testing", MB_OK | MB_ICONINFORMATION);
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//*****************************************************************************************
//CISFBand

CISFBand::CISFBand() : m_hWnd(NULL), m_BandID(0) {}

CISFBand::~CISFBand() {}

//ToolbarTest

HWND CISFBand::CreateSimpleToolbar(HWND hWndParent, HINSTANCE hInst)
{
    //This ought to be global.. (currently testing)
    //HIMAGELIST g_hImageList = NULL;

    // Declare and initialize local constants.    
    const int ImageListID = 0;
    const int bitmapSize = 16;

    const DWORD buttonStyles = BTNS_AUTOSIZE;

    // Create the toolbar.
    HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | TBSTYLE_WRAPABLE | TBSTYLE_LIST | CCS_NORESIZE, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        hWndParent, NULL, hInst, NULL);
    if (hWndToolbar == NULL)
        return NULL;

    // Create the image list.
    /* g_hImageList = ImageList_Create(bitmapSize, bitmapSize,   // Dimensions of individual bitmaps.
    ILC_COLOR16 | ILC_MASK,   // Ensures transparent background.
    numButtons, 0);

    // Set the image list.
    SendMessage(hWndToolbar, TB_SETIMAGELIST,
    (WPARAM)ImageListID,
    (LPARAM)g_hImageList);

    // Load the button images.
    SendMessage(hWndToolbar, TB_LOADIMAGES,
    (WPARAM)IDB_STD_SMALL_COLOR,
    (LPARAM)HINST_COMMCTRL);*/

    // Initialize button info.
    // IDM_NEW, IDM_OPEN, and IDM_SAVE are application-defined command constants.    
    /*{
    { MAKELONG(STD_FILENEW,  ImageListID), IDM_NEW,  TBSTATE_ENABLED, buttonStyles,{ 0 }, 0, (INT_PTR)L"New" },
    { MAKELONG(STD_FILEOPEN, ImageListID), IDM_OPEN, TBSTATE_ENABLED, buttonStyles,{ 0 }, 0, (INT_PTR)L"Open" },
    { MAKELONG(STD_FILESAVE, ImageListID), IDM_SAVE, 0,               buttonStyles,{ 0 }, 0, (INT_PTR)L"Save" }
    };*/
    // Add buttons.
    /*SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)nb, (LPARAM)&tbButtons);*/

    //Enumerate objects
    CComPtr<IEnumIDList> pedl;
    HRESULT hr = m_pISF->EnumObjects(0, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pedl);
    LPITEMIDLIST pidl = NULL;
    STRRET stret;
    //LPWSTR pstr = NULL;

    if (SUCCEEDED(hr))
    {
        for (int i=0; pedl->Next(MAX_PATH, &pidl, 0) != S_FALSE; i++)
        {
            WCHAR sz[MAX_PATH];
            m_pISF->GetDisplayNameOf(pidl, SHGDN_NORMAL, &stret);
            //StrRetToStr(&stret, pidl, &pstr);
            StrRetToBuf(&stret, pidl, sz, sizeof(sz));
            //MessageBox(0, sz, L"Namespace Object Found!", MB_OK | MB_ICONINFORMATION);

            TBBUTTON tb = { MAKELONG(I_IMAGENONE,  ImageListID), i, TBSTATE_ENABLED, buttonStyles,{ 0 }, 0, (INT_PTR)sz };
            SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
            SendMessage(hWndToolbar, TB_INSERTBUTTONW, 0, (LPARAM)&tb);            
        }        
    }

    // Resize the toolbar, and then show it.
    SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
    ShowWindow(hWndToolbar, TRUE);

    CoTaskMemFree((void*)pidl);
    //CoTaskMemFree((void*)pstr);
    return hWndToolbar;
}

/*****************************************************************************/

//IObjectWithSite
    HRESULT STDMETHODCALLTYPE CISFBand::SetSite(IUnknown *pUnkSite)
    {
        MessageBox(0, L"CISFBand::SetSite called!", L"Testing", MB_OK | MB_ICONINFORMATION);

        HRESULT hRet;
        HWND hwndParent;

        TRACE("CISFBand::SetSite(0x%p)\n", pUnkSite);

        hRet = IUnknown_GetWindow(pUnkSite, &hwndParent);
        if (FAILED(hRet))
        {
            TRACE("Querying site window failed: 0x%x\n", hRet);
            return hRet;
        }
        m_Site = pUnkSite; 

        //m_hWnd = CreateWindowEx(0, L"BUTTON", L">>", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, 50, 50, hwndParent, 0, m_hInstance, 0);
        //SetWindowSubclass(hwndParent, MyWndProc, 0, 0); //when button is clicked, parent receives WM_COMMAND, and thus subclassed to show a test message box
        
        m_hWnd = CreateSimpleToolbar(hwndParent, m_hInstance);
        SetWindowSubclass(hwndParent, MyWndProc, 0, 0);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite)
    {
        //MessageBox(0, L"GetSite called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        TRACE("CISFBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

        HRESULT hr;
        if (m_Site != NULL)
        {
            hr = m_Site->QueryInterface(riid, ppvSite);
            if (FAILED(hr)) return hr;
        }       

        *ppvSite = NULL;
        return E_FAIL;
    }

/*****************************************************************************/
//IDeskBand
    HRESULT STDMETHODCALLTYPE CISFBand::GetWindow(OUT HWND *phwnd)
    {
        //MessageBox(0, L"GetWindow called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        if (!m_hWnd)
            return E_FAIL;
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;       

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        /* FIXME: Implement */

        //MessageBox(0, L"ContextSensitiveHelp called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ShowDW(
        IN BOOL bShow)
    {
        //MessageBox(0, L"ShowDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        if (m_hWnd)
        {
            ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
        }
        
        return S_OK;       
    }

    HRESULT STDMETHODCALLTYPE CISFBand::CloseDW(
        IN DWORD dwReserved)
    {
        //MessageBox(0, L"CloseDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        if (m_hWnd)
        {
            ShowWindow(m_hWnd, SW_HIDE);
            DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) 
    {
        /* No need to implement this method */

        //MessageBox(0, L"ResizeBorderDW called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetBandInfo(  //Need a check
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi)
    {
        //MessageBox(0, L"GetBandInfo called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        TRACE("CTaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWnd);
                
        if (m_hWnd && pdbi)
        {
            m_BandID = dwBandID;
            
            RECT actualRect;
            POINTL actualSize;
            POINTL idealSize;
            POINTL maxSize;
            POINTL itemSize;            

            ::GetWindowRect(m_hWnd, &actualRect);
            actualSize.x = actualRect.right - actualRect.left;
            actualSize.y = actualRect.bottom - actualRect.top;

            // Obtain the ideal size, to be used as min and max 
            SendMessageW(m_hWnd, TB_AUTOSIZE, 0, 0);
            SendMessageW(m_hWnd, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&maxSize));                

            idealSize = maxSize;
            SendMessageW(m_hWnd, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&idealSize));           

            // Obtain the button size, to be used as the integral size 
            DWORD size = SendMessageW(m_hWnd, TB_GETBUTTONSIZE, 0, 0);
            itemSize.x = GET_X_LPARAM(size);
            itemSize.y = GET_Y_LPARAM(size);

            if (pdbi->dwMask & DBIM_MINSIZE)
            {
                pdbi->ptMinSize = idealSize;
            }
            if (pdbi->dwMask & DBIM_MAXSIZE)
            {
                pdbi->ptMaxSize = maxSize;
            }
            if (pdbi->dwMask & DBIM_INTEGRAL)
            {
                pdbi->ptIntegral = itemSize;
            }
            if (pdbi->dwMask & DBIM_ACTUAL)
            {
                pdbi->ptActual = actualSize;
            }
            if (pdbi->dwMask & DBIM_TITLE)
                wcscpy(pdbi->wszTitle, L"Quick Launch");                
            if (pdbi->dwMask & DBIM_MODEFLAGS)
            {
                pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;
            }
            if (pdbi->dwMask & DBIM_BKCOLOR)
                pdbi->crBkgnd = 0;            
        }
        return S_OK;
    }    

 /*****************************************************************************/
 //IDeskBar   
    HRESULT STDMETHODCALLTYPE CISFBand::SetClient(
        IN IUnknown *punkClient)
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);

        //MessageBox(0, L"SetClient called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetClient(
        OUT IUnknown **ppunkClient)
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);

        //MessageBox(0, L"GetClient called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::OnPosRectChangeDB(
        IN RECT *prc)
    {
        TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);      

        //MessageBox(0, L"OnPosRectChangeDB called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return S_OK;
    }

/*****************************************************************************/
//IPersistStream
    HRESULT STDMETHODCALLTYPE CISFBand::GetClassID(
        OUT CLSID *pClassID)
    {
        //MessageBox(0, L"GetClassID called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        TRACE("CISFBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the quick launch band */
         *pClassID = CLSID_QuickLaunchBand;        

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::IsDirty()
    {
        /* The object hasn't changed since the last save! */

        //MessageBox(0, L"IsDirty called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
       
        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Load(
        IN IStream *pStm)
    {
        TRACE("CISFBand::Load called\n");
        /* Nothing to do */

        //MessageBox(0, L"Load called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Save(
        IN IStream *pStm,
        IN BOOL fClearDirty)
    {
        /* Nothing to do */

        //MessageBox(0, L"Save called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize)
    {
        TRACE("CISFBand::GetSizeMax called\n");        
        //pcbSize->QuadPart = 0;

        //MessageBox(0, L"GetSizeMax called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        return S_OK;
    }
    

/*****************************************************************************/
//IWinEventHandler
    HRESULT STDMETHODCALLTYPE CISFBand::ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plrResult)
    {
        TRACE("CISFBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
        
        //MessageBox(0, L"ProcessMessage called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ContainsWindow(
        IN HWND hWnd)
    {
        //MessageBox(0, L"ContainsWindow called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);

        if (hWnd == m_hWnd ||
            IsChild(m_hWnd, hWnd))
        {
            TRACE("CISFBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
            return S_OK;
        }

        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {        
        //MessageBox(0, L"OnWinEvent called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
         UNIMPLEMENTED;
                 
         return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::IsWindowOwner(HWND hWnd)
    {
        //MessageBox(0, L"IsWindowOwner called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        return (hWnd == m_hWnd) ? S_OK : S_FALSE;        
    }
    
/*****************************************************************************/
// *** IOleCommandTarget methods ***
    HRESULT STDMETHODCALLTYPE CISFBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {
        //MessageBox(0, L"QueryStatus called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        UNIMPLEMENTED;

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {
        if (IsEqualIID(*pguidCmdGroup, IID_IBandSite))
        {
            return S_OK;
        }

        if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
        {
            return S_OK;
        }

        //MessageBox(0, L"Exec called!", L"Test Caption", MB_OK | MB_ICONINFORMATION);
        UNIMPLEMENTED;

        return E_NOTIMPL;
    }

//IShellFolderBand
    HRESULT STDMETHODCALLTYPE CISFBand::GetBandInfoSFB(PBANDINFOSFB pbi)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::InitializeSFB(IShellFolder *psf, PCIDLIST_ABSOLUTE pidl)
    {
        if (pidl != NULL)
        {
            psf->BindToObject(pidl, 0, IID_IShellFolder, (void**)&m_pISF);
            m_pidl = pidl;
        }
        else
        {
            m_pISF = psf;
            m_pidl = pidl;
        }
       
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::SetBandInfoSFB( PBANDINFOSFB pbi)
    {
        return E_NOTIMPL;
    }

//C Constructor
    extern "C"
    HRESULT WINAPI CISFBand_CreateInstance(REFIID riid, void** ppv)
    {
        return ShellObjectCreator<CISFBand>(riid, ppv);
    }

