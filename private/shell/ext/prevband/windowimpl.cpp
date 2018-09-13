//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft, 1997
//
//  File:       WindowImpl.cpp
//
//  Contents:   CPrevBand::CWindowImpl methods
//
//  History:    7-24-97  Davepl  Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "PreviewBand.h"

#define DELETE_BRUSH(x) { if (x) DeleteObject(x); x = NULL; }

void CPreviewBand::ReleaseGraphics()
{
    switch(m_stateBmp)
    {
        case UNAVAILABLE:
        case PENDING:
            break;
        default:
            DeleteObject(m_hPreviewBmp);
            m_hPreviewBmp = NULL;
    }

    if (m_brButtonShadow)
    {
        DeleteObject(m_brButtonShadow); 
        m_brButtonShadow = NULL;
    }

    if (m_brButton)
    {
        DeleteObject(m_brButton); 
        m_brButton = NULL;
    }

    if (m_brWindow)       
    {
        DeleteObject(m_brWindow); 
        m_brWindow = NULL;
    }

    if (m_penButtonText)
    {
        DeleteObject(m_penButtonText);
        m_penButtonText = NULL;
    }
}

BOOL CPreviewBand::GenerateGraphics()
{
    m_brButtonShadow = CreateSolidBrush( GetSysColor(COLOR_BTNSHADOW) );
    m_brWindow       = CreateSolidBrush( GetSysColor(COLOR_WINDOW)    );
    m_brButton       = CreateSolidBrush( GetSysColor(COLOR_BTNFACE)   );

    m_penButtonText  = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNTEXT) );

    return (m_brButtonShadow &&
            m_brWindow       &&
            m_brButton       &&
            m_penButtonText);
}

// CPreviewBand::OnCreate
//
// Creates the child controls, etc

LRESULT CPreviewBand::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    LRESULT lResult = DefWindowProc(uMsg, wParam, lParam);    
    RECT    rcInit  = { 0, 0, 0, 0 };

    SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // Create the owner-draw button that will serve as our preview window

    if (-1 != lResult)
    {
        if (NULL == (m_hwndPrevCtl = CreateWindow(_T("Button"),                         // class
                                                  NULL,                                 // title
                                                  WS_VISIBLE | WS_CHILD |BS_PUSHBUTTON |// style
                                                  BS_OWNERDRAW | WS_DISABLED,
                                                  0,0,0,0,                              // size
                                                  m_hWnd,                               // parent
                                                  NULL,                                 // menu
                                                  _Module.m_hInstResource,              // module
                                                  NULL)))
        {
            lResult = -1;            
        }
    }

    if (-1 != lResult)
    {
        if (FALSE == GenerateGraphics())
            lResult = -1;
    }

    bHandled = TRUE;
    return lResult;
}

//
// CPreviewBand::OnDestroy
//

LRESULT CPreviewBand::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    ReleaseGraphics();
    return 0;
}

//
// CPreviewBand::OnEraseBkgnd
//

LRESULT CPreviewBand::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = TRUE;
    return TRUE;
}

//
// CPreviewBand::OnSize
//

LRESULT CPreviewBand::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    BOOL fwSizeType = wParam;           // resizing flag 
    int  nWidth     = LOWORD(lParam);   // width of client area 
    int  nHeight    = HIWORD(lParam);   // height of client area 
    
    int  cxPreview   = nWidth  - GetSystemMetrics(SM_CXBORDER) * 4;
    int  cyPreview   = nHeight - GetSystemMetrics(SM_CYBORDER) * 4;
    int  cPreview    = min(cxPreview, cyPreview);
    
    CWindow(m_hwndPrevCtl).MoveWindow((nWidth / 2)  - (cPreview / 2),
                                      (nHeight / 2) - (cPreview / 2),
                                      cPreview,
                                      cPreview,
                                      FALSE);

    UpdatePreview();
    
    bHandled = TRUE;
    return TRUE;
}

// CPreviewBand::OnDrawItem
//
// Handles drawing of the preview control

LRESULT CPreviewBand::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    UINT             idCtl = (UINT) wParam;             // control identifier 
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam; // item-drawing information         

    if (m_hwndPrevCtl == lpdis->hwndItem)
    {
        CWindow ctlPreview(m_hwndPrevCtl);

        RECT rcCtl;
        ctlPreview.GetClientRect(&rcCtl);
        
        FillRect(lpdis->hDC, &rcCtl, m_brWindow);

        switch(m_stateBmp)
        {
            case UNAVAILABLE:
            {
                SetBkMode(lpdis->hDC, TRANSPARENT);
                rcCtl.top = rcCtl.bottom / 2;
                DrawTextEx(lpdis->hDC, 
                            m_uSelected ? m_szNoPreviewAvail : m_szNoItemSelected,
                            -1,
                            &rcCtl,
                            DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS,
                            NULL);
                break;
            }
            
            case PENDING:
            {
                SetBkMode(lpdis->hDC, TRANSPARENT);
                rcCtl.top = rcCtl.bottom / 2;
                DrawTextEx(lpdis->hDC, 
                            m_szPreviewPending,
                            -1,
                            &rcCtl,
                            DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS,
                            NULL);
                break;
            }

            default:
            {
                HDC hdcTemp = CreateCompatibleDC(lpdis->hDC);
                HBITMAP hbmpOld = (HBITMAP) SelectObject(hdcTemp, m_hPreviewBmp);
                BitBlt(lpdis->hDC, 0, 0, rcCtl.right, rcCtl.bottom, hdcTemp, 0, 0, SRCCOPY);
                SelectObject(hdcTemp, hbmpOld);
                break;
            }
        }

        bHandled = TRUE;
    }
    return TRUE;
}


//
// CPreviewBand::OnPaint
//

LRESULT CPreviewBand::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    if (GetUpdateRect(NULL))
    {
        PAINTSTRUCT ps;
        HDC hdcPaint = BeginPaint(&ps);
        if (hdcPaint)
        {
            RECT rcWindow;
            GetClientRect(&rcWindow);

            FillRect(hdcPaint, &rcWindow, m_brButton);

            RECT rcPreview;
            CWindow m_wndPreview(m_hwndPrevCtl);
            m_wndPreview.GetWindowRect(&rcPreview);
            CWindow(GetDesktopWindow()).MapWindowPoints(m_hWnd, &rcPreview);

            rcPreview.left   -= GetSystemMetrics(SM_CXBORDER);
            rcPreview.right  += GetSystemMetrics(SM_CXBORDER);
            rcPreview.top    -= GetSystemMetrics(SM_CYBORDER);
            rcPreview.bottom += GetSystemMetrics(SM_CYBORDER);

            DrawEdge(hdcPaint, &rcPreview, EDGE_SUNKEN, BF_TOPLEFT);

            DrawEdge(hdcPaint, &rcPreview, EDGE_RAISED, BF_BOTTOMRIGHT);
            rcPreview.right  += 1;
            rcPreview.bottom += 1;
            DrawEdge(hdcPaint, &rcPreview, EDGE_RAISED, BF_BOTTOMRIGHT);

            EndPaint(&ps);
        }
    }

    bHandled = TRUE;
    return 0;    
}

//
// CPrevBand::OnSetFocus
//

LRESULT CPreviewBand::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    m_bFocus = TRUE;

    // Inform the input object site that the focus has changed
    if(m_ptrSite)
        m_ptrSite->OnFocusChangeIS((IDockingWindow*)this, TRUE);

    return 0;
}

//
// CPrevBand::OnKillFocus
//

LRESULT CPreviewBand::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    m_bFocus = FALSE;

    // Inform the input object site that the focus has changed
    if(m_ptrSite)
       m_ptrSite->OnFocusChangeIS((IDockingWindow*)this, FALSE);

    return 0;
}

// CPreviewBand::WorkerThreadBootstrap
//
// Thread statup function, serves only to get the right context
// for the this pointer (its a static, calls non-static member)

typedef struct
{
    CPreviewBand  * pThis;
    IExtractImage * pExtractImage;
} PREV_THREADPARAMS;

ULONG CPreviewBand::WorkerThreadBootstrap(void * pvParam)
{
    PREV_THREADPARAMS * pParams = (PREV_THREADPARAMS *) pvParam;
    pParams->pThis->AsyncExtractThread(pParams->pExtractImage);
    delete pParams;
    
    return 0;
}

void CPreviewBand::AsyncExtractThread(IExtractImage * pExtractImage)
{
    if (IsWindow(m_hwndPrevCtl))
    {
        RECT rcPreview;
        CWindow(m_hwndPrevCtl).GetClientRect(&rcPreview);
        DWORD dwFlags = 0;
        DWORD dwPriority = IEIT_PRIORITY_NORMAL;
        SIZE  sPreview  = { rcPreview.right, rcPreview.bottom };

        WCHAR szPath[MAX_PATH];
        HRESULT hr = pExtractImage->GetLocation(szPath, MAX_PATH, &dwPriority, &sPreview, 16, &dwFlags);
        if (SUCCEEDED(hr))
        {
            HBITMAP hbmpTemp = NULL;

            m_stateBmp = PENDING;

            pExtractImage->Extract(&hbmpTemp);
        
            // We only update the preview if we're the most currently-spawned
            // worker thread
            
            if (GetCurrentThreadId() == m_dwThreadid)
            {
                m_hPreviewBmp = hbmpTemp;
                CWindow(m_hwndPrevCtl).InvalidateRect(NULL);
            }
        }
    }
    pExtractImage->Release();
    _tih.Release();
}

// CPreviewBand::UpdatePreview
//
// Spins off a thread that will update the preview
    
HRESULT CPreviewBand::UpdatePreview()
{
    HRESULT hr = S_OK;

    // Allocate a params block (we can't just let it use out m_ptrExtractImage
    // because we might null or replace that pointer before its done with it,
    // so the worker thread needs its own copy)

    PREV_THREADPARAMS * pParams = new PREV_THREADPARAMS;
    if (NULL == pParams)
        return E_OUTOFMEMORY;

    pParams->pThis = this;
    pParams->pExtractImage = m_ptrExtractImage;

    CWindow(m_hwndPrevCtl).InvalidateRect(NULL);

    if (m_hPreviewBmp)
    {
        DeleteObject(m_hPreviewBmp);
        m_hPreviewBmp = NULL;
    }

    if (m_uSelected && m_ptrExtractImage)
    {
        // Addref ourselves and the image extractor on behalf of the worker thread

        _tih.AddRef();
        pParams->pExtractImage->AddRef();

        HANDLE hThread = CreateThread(NULL, 0, WorkerThreadBootstrap, (void *) pParams, 0, &m_dwThreadid);
        if (NULL == hThread)
        {
            // If we couldn't start the thread, release the references and free the param
            // block, since the thread won't be doing it...

            _tih.Release();
            pParams->pExtractImage->Release();
            delete pParams;
        }
        else
            CloseHandle(hThread);

    }
    return hr;
}
