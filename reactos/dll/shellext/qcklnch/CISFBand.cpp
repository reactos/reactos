/*
 * PROJECT:     ReactOS shell extensions
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/qcklnch/CISFBand.cpp
 * PURPOSE:     Quick Launch Toolbar (Taskbar Shell Extension)
 * PROGRAMMERS: Shriraj Sawant a.k.a SR13 <sr.official@hotmail.com>
 */

#include "precomp.h"
#include <mshtmcid.h>
#include <commoncontrols.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

WINE_DEFAULT_DEBUG_CHANNEL(qcklnch);

//*****************************************************************************************
// *** CISFBand *** 

CISFBand::CISFBand() : m_hWndTb(NULL), m_BandID(0), m_pidl(NULL) {}

CISFBand::~CISFBand() {}

// *** CWindowImpl ***
//Subclassing 

LRESULT CISFBand::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TBBUTTON tb;
    POINT pt;
    DWORD pos = GetMessagePos();    
    pt.x = GET_X_LPARAM(pos);
    pt.y = GET_Y_LPARAM(pos);
    ScreenToClient(&pt);

    int index = SendMessage(m_hWndTb, TB_HITTEST, 0, (LPARAM)&pt);
    bool chk = SendMessage(m_hWndTb, TB_GETBUTTON, abs(index), (LPARAM)&tb);    
    if(chk) SHInvokeDefaultCommand(m_hWndTb, m_pISF, (LPITEMIDLIST)tb.dwData);    

    return 0;
}

//ToolbarTest
HWND CISFBand::CreateSimpleToolbar(HWND hWndParent, HINSTANCE hInst)
{
    // Declare and initialize local constants.     
    const DWORD buttonStyles = BTNS_AUTOSIZE;

    // Create the toolbar.
    HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NORESIZE, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        hWndParent, NULL, hInst, NULL);
    if (hWndToolbar == NULL)
        return NULL; 

    // Set the image list.
    HIMAGELIST* piml;
    HRESULT hr1 = SHGetImageList(SHIL_SMALL, IID_IImageList, (void**)&piml);    
    SendMessage(hWndToolbar, TB_SETIMAGELIST, 0, (LPARAM)piml);    

    //Enumerate objects
    CComPtr<IEnumIDList> pedl;
    HRESULT hr2 = m_pISF->EnumObjects(0, SHCONTF_FOLDERS, &pedl);
    LPITEMIDLIST pidl = NULL;
    STRRET stret;      

    if (SUCCEEDED(hr1) && SUCCEEDED(hr2))
    {
        ULONG count = 0;
        for (int i=0; pedl->Next(MAX_PATH, &pidl, 0) != S_FALSE; i++, count++)
        {
            WCHAR sz[MAX_PATH];
            int index = SHMapPIDLToSystemImageListIndex(m_pISF, pidl, NULL);            
            m_pISF->GetDisplayNameOf(pidl, SHGDN_NORMAL, &stret);            
            StrRetToBuf(&stret, pidl, sz, sizeof(sz));            

            TBBUTTON tb = { MAKELONG(index, 0), i, TBSTATE_ENABLED, buttonStyles,{ 0 }, (DWORD_PTR)pidl, (INT_PTR)sz };            
            SendMessage(hWndToolbar, TB_INSERTBUTTONW, 0, (LPARAM)&tb);            
        }        
    }
    else return NULL;

    // Resize the toolbar, and then show it.
    SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
    ::ShowWindow(hWndToolbar, TRUE);

    CoTaskMemFree((void*)pidl);    
    return hWndToolbar;
}

/*****************************************************************************/

// *** IObjectWithSite *** 
    HRESULT STDMETHODCALLTYPE CISFBand::SetSite(IUnknown *pUnkSite)
    {
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
        
        m_hWndTb = CreateSimpleToolbar(hwndParent, m_hInstance);        
        SubclassWindow(m_hWndTb);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite)
    {        
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
// *** IDeskBand *** 
    HRESULT STDMETHODCALLTYPE CISFBand::GetWindow(OUT HWND *phwnd)
    {
        if (!m_hWndTb)
            return E_FAIL;
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWndTb;       

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        /* FIXME: Implement */        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ShowDW(
        IN BOOL bShow)
    {        
        if (m_hWndTb)
        {
            ::ShowWindow(m_hWndTb, bShow ? SW_SHOW : SW_HIDE);
        }
        
        return S_OK;       
    }

    HRESULT STDMETHODCALLTYPE CISFBand::CloseDW(
        IN DWORD dwReserved)
    {        
        if (m_hWndTb)
        {
            ::ShowWindow(m_hWndTb, SW_HIDE);
            ::DestroyWindow(m_hWndTb);
            m_hWndTb = NULL;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) 
    {
        /* No need to implement this method */

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetBandInfo(  //Need a check
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi)
    {        
        TRACE("CTaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, m_hWndTb);
                
        if (m_hWndTb && pdbi)
        {
            m_BandID = dwBandID;
            
            RECT actualRect;
            POINTL actualSize;
            POINTL idealSize;
            POINTL maxSize;
            POINTL itemSize;            

            ::GetWindowRect(m_hWndTb, &actualRect);
            actualSize.x = actualRect.right - actualRect.left;
            actualSize.y = actualRect.bottom - actualRect.top;

            // Obtain the ideal size, to be used as min and max 
            SendMessageW(m_hWndTb, TB_AUTOSIZE, 0, 0);
            SendMessageW(m_hWndTb, TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&maxSize));                

            idealSize = maxSize;
            SendMessageW(m_hWndTb, TB_GETIDEALSIZE, FALSE, reinterpret_cast<LPARAM>(&idealSize));           

            // Obtain the button size, to be used as the integral size 
            DWORD size = SendMessageW(m_hWndTb, TB_GETBUTTONSIZE, 0, 0);
            itemSize.x = GET_X_LPARAM(size);
            itemSize.y = GET_Y_LPARAM(size);

            if (pdbi->dwMask & DBIM_MINSIZE)
            {
                pdbi->ptMinSize.x = -1;
                pdbi->ptMinSize.y = idealSize.y;
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
                pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT | DBIMF_USECHEVRON | DBIMF_NOMARGINS | DBIMF_BKCOLOR;
            }
            if (pdbi->dwMask & DBIM_BKCOLOR)
                pdbi->dwMask &= ~DBIM_BKCOLOR;
        
        }
        return S_OK;
    }    

 /*****************************************************************************/
 // *** IDeskBar ***    
    HRESULT STDMETHODCALLTYPE CISFBand::SetClient(
        IN IUnknown *punkClient)
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetClient(
        OUT IUnknown **ppunkClient)
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::OnPosRectChangeDB(
        IN RECT *prc)
    {
        TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);      

        return S_OK;
    }

/*****************************************************************************/
// *** IPersistStream *** 
    HRESULT STDMETHODCALLTYPE CISFBand::GetClassID(
        OUT CLSID *pClassID)
    {        
        TRACE("CISFBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the quick launch band */
         *pClassID = CLSID_QuickLaunchBand;        

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::IsDirty()
    {
        /* The object hasn't changed since the last save! */

        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Load(
        IN IStream *pStm)
    {
        TRACE("CISFBand::Load called\n");
        /* Nothing to do */

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::Save(
        IN IStream *pStm,
        IN BOOL fClearDirty)
    {
        /* Nothing to do */

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize)
    {
        TRACE("CISFBand::GetSizeMax called\n");         

        return S_OK;
    }    

/*****************************************************************************/
// *** IWinEventHandler *** 
    HRESULT STDMETHODCALLTYPE CISFBand::ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plrResult)
    {
        TRACE("CISFBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
        
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::ContainsWindow(
        IN HWND hWnd)
    {        
        if (hWnd == m_hWndTb ||
            ::IsChild(m_hWndTb, hWnd))
        {
            TRACE("CISFBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
            return S_OK;
        }

        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {          
         UNIMPLEMENTED;
                 
         return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::IsWindowOwner(HWND hWnd)
    {        
        return (hWnd == m_hWndTb) ? S_OK : S_FALSE;        
    }  

/*****************************************************************************/
// *** IOleCommandTarget methods ***
    HRESULT STDMETHODCALLTYPE CISFBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {        
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
        
        UNIMPLEMENTED;

        return E_NOTIMPL;
    }

/*****************************************************************************/
// *** IShellFolderBand ***
    HRESULT STDMETHODCALLTYPE CISFBand::GetBandInfoSFB(PBANDINFOSFB pbi)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::InitializeSFB(IShellFolder *psf, PCIDLIST_ABSOLUTE pidl)
    {
        LPITEMIDLIST pidlRoot;
        SHGetSpecialFolderLocation(0, CSIDL_DESKTOP, &pidlRoot);

        if (pidl == NULL || !psf->CompareIDs(0x80000000L, pidl, pidlRoot))
        {
            m_pISF = psf;
            m_pidl = pidl;
        }
        else 
        {
            psf->BindToObject(pidl, 0, IID_IShellFolder, (void**)&m_pISF);
            m_pidl = pidl;
        }
               
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CISFBand::SetBandInfoSFB( PBANDINFOSFB pbi)
    {
        return E_NOTIMPL;
    }

/*****************************************************************************/
//C Constructor
    extern "C"
    HRESULT WINAPI CISFBand_CreateInstance(REFIID riid, void** ppv)
    {
        return ShellObjectCreator<CISFBand>(riid, ppv);
    }

