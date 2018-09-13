//------------------------------------------------------------------------------
// idroptgt.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Author
//     bash
//
// History
//      7-15-97     created     (bash)
//
// Implementation of IDropTarget
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include <ocidl.h>
#include <string.h>

#include "triedit.h"
#include "document.h"
#include "privcid.h"
#include "dispatch.h"
#include "trace.h"
#include "undo.h"

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DragEnter (IDropTarget method)
//
// In design mode, accept drags that originate within Trident. Allow unlocked
// 2D positioned elements to be dragged using a dashed outline as a drag 
// rectangle. If TriEdit's constrained dragging mode has been enabled using
// the Constrain method then the drag will be constrained to points which are
// even multiples of the values in m_ptConstrain. 
//

STDMETHODIMP CTriEditDocument::DragEnter(IDataObject *pDataObject,
                        DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = GetElement(TRUE /* fInDragDrop */);

    m_fLocked = FALSE;
    m_eDirection = CONSTRAIN_NONE;

    if (SUCCEEDED(hr) &&
        m_pihtmlElement &&
        SUCCEEDED(hr=GetTridentWindow()))
    {
        BOOL f2D = FALSE;
        LONG lWidth;
        LONG lHeight;
        IHTMLElement* pihtmlElementParent=NULL;
        HBITMAP hbmp;

        _ASSERTE(m_pihtmlStyle);
        if (IsDesignMode() &&           //Are we in design mode?
            m_pihtmlStyle &&            //abort if don't have style
            IsDragSource() &&           //abort if Trident isn't source of drag
            SUCCEEDED(Is2DElement(m_pihtmlElement, &f2D)) && f2D &&
            SUCCEEDED(IsLocked(m_pihtmlElement, &m_fLocked)) && !m_fLocked &&
            SUCCEEDED(GetScrollPosition()) &&
            SUCCEEDED(GetElementPosition(m_pihtmlElement, &m_rcElement)))
        {
            //first, let's get a pattern brush to use for the move rectangle
            hbmp = LoadBitmap(_Module.GetModuleInstance(), (LPCWSTR)IDR_FEEDBACKRECTBMP);
            _ASSERTE(hbmp);
            m_hbrDragRect = CreatePatternBrush(hbmp);
            _ASSERTE(m_hbrDragRect);
            DeleteObject(hbmp);

            ::SetRect(&m_rcElementParent, 0, 0, 0, 0);
            hr = m_pihtmlElement->get_offsetParent(&pihtmlElementParent);
            if (SUCCEEDED(hr) && pihtmlElementParent)
            {
                GetElementPosition(pihtmlElementParent, &m_rcElementParent);
            }
            SAFERELEASE(pihtmlElementParent);

            lWidth  = m_rcElement.right - m_rcElement.left;
            lHeight = m_rcElement.bottom - m_rcElement.top;

            //this is where we'll initially draw the drag rectangle
            m_rcDragRect = m_rcElementOrig = m_rcElement;

            //convert clicked point to client coordinates
            m_ptClickLast.x = pt.x;
            m_ptClickLast.y = pt.y;
            ScreenToClient(m_hwndTrident, &m_ptClickLast);

            //save point in doc coordinates where clicked.
            m_ptClickOrig = m_ptClickLast;
            m_ptClickOrig.x += m_ptScroll.x;
            m_ptClickOrig.y += m_ptScroll.y;

            if (m_fConstrain)
            {
                m_ptConstrain.x = m_rcElement.left;
                m_ptConstrain.y = m_rcElement.top;
            }

            #define BORDER_WIDTH 7

            if (m_ptClickOrig.x < (m_rcDragRect.left - BORDER_WIDTH))
            {
                m_rcDragRect.left   = m_ptClickOrig.x;
                m_rcDragRect.right  = m_rcDragRect.left + lWidth;
            }
            else if (m_ptClickOrig.x > (m_rcDragRect.right + BORDER_WIDTH))
            {
                m_rcDragRect.right  = m_ptClickOrig.x;
                m_rcDragRect.left   = m_rcDragRect.right - lWidth;
            }

            if (m_ptClickOrig.y < (m_rcDragRect.top  - BORDER_WIDTH))
            {
                m_rcDragRect.top    = m_ptClickOrig.y;
                m_rcDragRect.bottom = m_rcDragRect.top  + lHeight;
            }
            else if (m_ptClickOrig.y > (m_rcDragRect.bottom + BORDER_WIDTH))
            {
                m_rcDragRect.bottom = m_ptClickOrig.y;
                m_rcDragRect.top    = m_rcDragRect.bottom - lHeight;
            }

            m_rcElement = m_rcDragRect;

            //Trace("DragEnter: m_rcElement(%d,%d,%d,%d)", m_rcElement.left, m_rcElement.top, m_rcElement.right, m_rcElement.bottom);
            //Trace("DragEnter: m_rcDragRect(%d,%d,%d,%d)", m_rcDragRect.left, m_rcDragRect.top, m_rcDragRect.right, m_rcDragRect.bottom);
            //Trace("DragEnter: m_ptClickLast(%d,%d)", m_ptClickLast.x, m_ptClickLast.y);
            //Trace("DragEnter: m_ptClickOrig(%d,%d)", m_ptClickOrig.x, m_ptClickOrig.y);

            //now draw the selection rect
            Draw2DDragRect(TRUE);
            *pdwEffect = DROPEFFECT_MOVE;
            hr = S_OK;
        }
        else
        if (!m_fLocked)
        {
            //something is hosed. just bail
            ReleaseElement();
        }
    }

    if (!m_pihtmlElement && NULL != m_pDropTgtTrident)
    {
        hr = m_pDropTgtTrident->DragEnter(pDataObject, grfKeyState, pt, pdwEffect);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DragOver (IDropTarget method)
// 
// Provide feedback during a drag, updating the drag rectangle, and scrolling
// the document as needed.


STDMETHODIMP CTriEditDocument::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_UNEXPECTED;
    POINT ptClient;

    if (m_pihtmlElement &&
        !m_fLocked &&
        SUCCEEDED(GetScrollPosition()))  //we are handling the drag-drop
    {
            ptClient.x = pt.x;
            ptClient.y = pt.y;
            ScreenToClient(m_hwndTrident, &ptClient);

            // scroll if required
            if (S_OK == DragScroll(ptClient))
            {
                *pdwEffect = DROPEFFECT_MOVE | DROPEFFECT_SCROLL;
            }
            else
            {
                if (ptClient.x != m_ptClickLast.x || ptClient.y != m_ptClickLast.y)
                {
                    //update the last click position
                    m_ptClickLast.x = ptClient.x;
                    m_ptClickLast.y = ptClient.y;
    
                    //Trace("DragOver: m_ptClickLast(%d,%d)", m_ptClickLast.x, m_ptClickLast.y);
    
                    //erase the move rectangle
                    Draw2DDragRect(FALSE);
    
                    ConstrainXY(&ptClient);
                    SnapToGrid(&ptClient);

                    //redraw the move rectangle
                    Draw2DDragRect(TRUE);
                }
        *pdwEffect = DROPEFFECT_MOVE;
            }
        hr = S_OK;
        }

    if (!m_pihtmlElement && NULL != m_pDropTgtTrident)
    {
            hr = m_pDropTgtTrident->DragOver(grfKeyState, pt, pdwEffect);
    }   

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DragLeave (IDropTarget method)
// 
// If currently dragging, erase the drag rectangle.
//

STDMETHODIMP CTriEditDocument::DragLeave()
{
    HRESULT hr = E_UNEXPECTED;

    if (m_pihtmlElement && !m_fLocked)
    {
        //erase the move rectangle
        Draw2DDragRect(FALSE);

        if (m_hbrDragRect)
        {
            DeleteObject(m_hbrDragRect);
            m_hbrDragRect = NULL;
        }
        hr = S_OK;
    }
    else if (!m_pihtmlElement && NULL != m_pDropTgtTrident)
    {
        hr = m_pDropTgtTrident->DragLeave();
    }
    ReleaseElement();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::Drop (IDropTarget method)
//
// After a successful drag of an unlocked element, erase the drag rectangle
// and then handle the actual drop by moving or creating an item. Newly
// created items will be 2D positionable.
//

STDMETHODIMP CTriEditDocument::Drop(IDataObject *pDataObject,
                        DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = E_UNEXPECTED;

    if (m_pihtmlElement && !m_fLocked)
    {
        _ASSERTE(m_pihtmlElement);
        _ASSERTE(m_pihtmlStyle);

        //erase the move rectangle
        Draw2DDragRect(FALSE);

        if (m_hbrDragRect)
        {
            DeleteObject(m_hbrDragRect);
            m_hbrDragRect = NULL;
        }

        if (m_pihtmlStyle)
        {
            POINT ptOrig, ptMove;

            m_rcDragRect.left   = m_rcDragRect.left   - m_rcElementParent.left;
            m_rcDragRect.top    = m_rcDragRect.top    - m_rcElementParent.top;
            m_rcDragRect.right  = m_rcDragRect.right  - m_rcElementParent.right;
            m_rcDragRect.bottom = m_rcDragRect.bottom - m_rcElementParent.bottom;

            ptOrig.x = m_rcElementOrig.left;
            ptOrig.y = m_rcElementOrig.top;
            ptMove.x = m_rcDragRect.left;
            ptMove.y = m_rcDragRect.top;
            CUndoDrag* pUndoDrag = new CUndoDrag(m_pihtmlStyle, ptOrig, ptMove);
            if (pUndoDrag)      //constructor sets m_cRef=1
            {
                hr = AddUndoUnit(m_pUnkTrident, pUndoDrag);
                _ASSERTE(SUCCEEDED(hr));
                pUndoDrag->Release();
            }

            m_pihtmlStyle->put_pixelLeft(m_rcDragRect.left);
            m_pihtmlStyle->put_pixelTop(m_rcDragRect.top);
        }

        //cleanup
        hr = S_OK;
    }

    if (!m_pihtmlElement && NULL != m_pDropTgtTrident)
    {
        hr = m_pDropTgtTrident->Drop(pDataObject, grfKeyState, pt, pdwEffect);

        // The following is to workaround a Trident bug where they don't
        // set the focus to their window upon the drop
        if (S_OK == hr)
        {
            CComPtr<IOleInPlaceSite> pInPlaceSite;
            CComPtr<IOleInPlaceFrame> pInPlaceFrame;
            CComPtr<IOleInPlaceUIWindow> pInPlaceWindow;
            RECT posRect, clipRect;
            OLEINPLACEFRAMEINFO frameInfo;
            HWND hwnd, hwndFrame;
            
            if (S_OK == m_pClientSiteHost->QueryInterface(IID_IOleInPlaceSite, (void **)&pInPlaceSite))
            {
                _ASSERTE(NULL != pInPlaceSite.p);
                if (S_OK == pInPlaceSite->GetWindowContext(&pInPlaceFrame, &pInPlaceWindow, &posRect, &clipRect, &frameInfo))
                {
                    if (NULL != pInPlaceWindow.p)
                        pInPlaceWindow->GetWindow(&hwnd);
                    else
                    {
                        _ASSERTE(NULL != pInPlaceFrame.p);
                        pInPlaceFrame->GetWindow(&hwnd);
                    }
                    // We need to walk up the parent chain till we find a frame window to work around a Vegas bug
                    // Note that this is generic enough to do the right thing for all of our clients
                    hwndFrame = hwnd;
                    do
                    {
                        if (GetWindowLong(hwndFrame, GWL_STYLE) & WS_THICKFRAME)
                            break;
                        hwndFrame = GetParent(hwndFrame);
                    } 
                    while (hwndFrame);

                    SetFocus(hwndFrame && IsWindow(hwndFrame) ? hwndFrame : hwnd);
                }
            }
        }

        // Handle 2d drop mode here
        if (S_OK == hr && !IsDragSource())
        {
            BOOL f2DCapable = FALSE;
            BOOL f2D = FALSE;

            GetElement();

            // we do the following if we are in 2DDropMode and the element is 2DCapable 
            // and the element is not already 2D or a DTC
            if (m_f2dDropMode && m_pihtmlElement &&
                SUCCEEDED(Is2DCapable(m_pihtmlElement, &f2DCapable)) && f2DCapable &&
                SUCCEEDED(Is2DElement(m_pihtmlElement, &f2D)) && !f2D &&
                FAILED(IsElementDTC(m_pihtmlElement)))
            {
                HRESULT hr;
                POINT ptClient;
                            
                ptClient.x = pt.x;
                ptClient.y = pt.y;

                if (SUCCEEDED(hr = CalculateNewDropPosition(&ptClient)))
                    hr = Make2DElement(m_pihtmlElement, &ptClient);
                else
                    hr = Make2DElement(m_pihtmlElement);

                _ASSERTE(SUCCEEDED(hr));
            }
    
            if (m_pihtmlElement)
            {
                BOOL f2D = FALSE;
                VARIANT var;
                POINT ptClient;

                ptClient.x = pt.x;
                ptClient.y = pt.y;
                                       
                if (SUCCEEDED(Is2DElement(m_pihtmlElement, &f2D)) && f2D)
                {
                    if (SUCCEEDED(CalculateNewDropPosition(&ptClient)))
                    {
                        IHTMLElement *pihtmlElementParent = NULL;

                        m_pihtmlElement->get_offsetParent(&pihtmlElementParent);

                        if(pihtmlElementParent)
                        {
                            RECT rcParent;

                            if (SUCCEEDED(GetElementPosition(pihtmlElementParent, &rcParent)))
                            {
                                m_pihtmlStyle->put_pixelLeft(ptClient.x - rcParent.left);
                                m_pihtmlStyle->put_pixelTop(ptClient.y - rcParent.top);
                            }
                            SAFERELEASE(pihtmlElementParent);
                        }
                    }

                    VariantInit(&var);
                    var.vt = VT_I4;
                    var.lVal = 0; 
                    m_pihtmlStyle->put_zIndex(var);
                    AssignZIndex(m_pihtmlElement, MADE_ABSOLUTE);
                }
            }
        }
    }

    ReleaseElement();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetElement
//
// Fetch the current Trident element and its style into m_pihtmlElement and 
// m_pihtmlStyle, respectively. If currently in mid-drag-drop (as indicated
// by fInDragDrop) then do not accept an HTML element of type "Text" as
// the currrent element. Returns S_OK or a Trident error.
//

HRESULT CTriEditDocument::GetElement(BOOL fInDragDrop)
{
    IHTMLDocument2* pihtmlDoc2=NULL;
    IHTMLSelectionObject* pihtmlSelObj=NULL;
    IHTMLTxtRange* pihtmlTxtRange=NULL;
    IHTMLControlRange* pihtmlControlRange=NULL;
    IHTMLElement* pihtmlBodyElement=NULL;
    IUnknown* punkBody=NULL;
    IUnknown* punkElement=NULL;
    IDispatch* pidisp=NULL;
    BSTR bstrType=NULL;

    ReleaseElement();           //cleanup just in case...
    _ASSERTE(m_pUnkTrident);

    HRESULT hr = GetDocument(&pihtmlDoc2);

    if (FAILED(hr))
        goto CleanUp;

    hr = pihtmlDoc2->get_selection(&pihtmlSelObj);

    if (FAILED(hr))
        goto CleanUp;

    _ASSERTE(pihtmlSelObj);
    hr = pihtmlSelObj->get_type(&bstrType);
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr) || !bstrType || (fInDragDrop && _wcsicmp(bstrType, L"Text")==0))
        goto CleanUp;

    hr = pihtmlSelObj->createRange(&pidisp);

    if (FAILED(hr) || !pidisp)
        goto CleanUp;

    hr = pidisp->QueryInterface(IID_IHTMLTxtRange, (LPVOID*)&pihtmlTxtRange);

    if (SUCCEEDED(hr))
    {
        _ASSERTE(pihtmlTxtRange);
        hr = pihtmlTxtRange->parentElement(&m_pihtmlElement);
        goto CleanUp;
    }

    hr = pidisp->QueryInterface(IID_IHTMLControlRange, (LPVOID*)&pihtmlControlRange);

    if (SUCCEEDED(hr))
    {
        _ASSERTE(pihtmlControlRange);
        hr = pihtmlControlRange->commonParentElement(&m_pihtmlElement);
    }

CleanUp:
    hr = E_FAIL;

    if (m_pihtmlElement)
    {
        //get the body element
        hr = pihtmlDoc2->get_body(&pihtmlBodyElement);
        _ASSERTE(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            //get their IUnknowns
            hr = pihtmlBodyElement->QueryInterface(IID_IUnknown, (LPVOID*)&punkBody);
            _ASSERTE(SUCCEEDED(hr));
            hr = m_pihtmlElement->QueryInterface(IID_IUnknown, (LPVOID*)&punkElement);
            _ASSERTE(SUCCEEDED(hr));

            //If they're equivalent, the body element is the current element
            //and we don't want it.
            if (punkBody == punkElement)
            {
                hr = E_FAIL;
            }
        }

        // VID98 bug 2647: if type is none, don't bother to cache style.
        // This is to workaround trident crash bug
        if (SUCCEEDED(hr) && _wcsicmp(bstrType, L"None")!=0)
        {
            hr = m_pihtmlElement->get_style(&m_pihtmlStyle);
            _ASSERTE(SUCCEEDED(hr));
            _ASSERTE(m_pihtmlStyle);
        }
        if (FAILED(hr) || !m_pihtmlStyle)
        {
            ReleaseElement();
        }
        hr = S_OK;
    }
    SAFERELEASE(pihtmlDoc2);
    SAFERELEASE(pihtmlSelObj);
    SAFERELEASE(pidisp);
    SAFERELEASE(pihtmlTxtRange);
    SAFERELEASE(pihtmlControlRange);
    SAFERELEASE(pihtmlBodyElement);
    SAFERELEASE(punkBody);
    SAFERELEASE(punkElement);
    SysFreeString(bstrType);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::ReleaseElement
//
// Release any cached reference to the current Trident element and its
// associated style. No return value.
//

void CTriEditDocument::ReleaseElement(void)
{
    SAFERELEASE(m_pihtmlElement);
    SAFERELEASE(m_pihtmlStyle);
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::Draw2DDragRect
//
// After giving the drag-drop handler host a chance to draw the drag rectangle,
// draw the rectangle if the handler choose not to do so. No return value.
//

void CTriEditDocument::Draw2DDragRect(BOOL fDraw)
{
    RECT rect = m_rcDragRect;

    // S_FALSE means that the host has already drawn its own feedback
    if (m_pDragDropHandlerHost && m_pDragDropHandlerHost->DrawDragFeedback(&rect) == S_FALSE)
        return;

    if ((fDraw == m_fDragRectVisible) || (NULL == m_hwndTrident) || (NULL == m_hbrDragRect))
        return;

    HDC hdc = GetDC(m_hwndTrident);
    _ASSERTE(hdc);
    HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, m_hbrDragRect);
    _ASSERTE(hbrOld);

    //BUGS:M3-2723\The Drag Rectangle Must be at Least 8x8 pixels
    LONG lWidth  = max((rect.right - rect.left), 16);
    LONG lHeight = max((rect.bottom - rect.top), 16);

    SetWindowOrgEx(hdc, m_ptScroll.x, m_ptScroll.y, NULL);

    //A Value of 2 is added to the rect's left and top in all the following PatBlt function
    //to work around a rounding off bug caused by trident.

    PatBlt( hdc, rect.left + 2, rect.top + 2,
            lWidth, 1, PATINVERT);

    PatBlt( hdc, rect.left + 2, rect.top + lHeight + 1, //(2 - 1)
            lWidth, 1, PATINVERT);

    PatBlt( hdc, rect.left + 2, rect.top + 3,//(2 + 1)
            1, lHeight - (2 * 1), PATINVERT);

    PatBlt( hdc, rect.left + lWidth + 1 /*(2 - 1)*/, rect.top + 3, //(2 + 1)
            1, lHeight - (2 * 1), PATINVERT);

    m_fDragRectVisible = !m_fDragRectVisible;

    SelectObject(hdc, hbrOld);
    ReleaseDC(m_hwndTrident, hdc);
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetScrollPosition
//
// Get the Trident document's scroll position and store it in m_ptScroll. 
// Return S_OK or a Trident error code.
//

HRESULT CTriEditDocument::GetScrollPosition(void)
{
    IHTMLDocument2* pihtmlDoc2=NULL;
    IHTMLTextContainer* pihtmlTextContainer=NULL;
    IHTMLElement* pihtmlElement=NULL;
    HRESULT hr = E_FAIL;

    _ASSERTE(m_pUnkTrident);
    if (SUCCEEDED(GetDocument(&pihtmlDoc2)))
    {
        if (SUCCEEDED(pihtmlDoc2->get_body(&pihtmlElement)))
        {
            _ASSERTE(pihtmlElement);
            if (pihtmlElement)
            {
                if (SUCCEEDED(pihtmlElement->QueryInterface(IID_IHTMLTextContainer,
                    (LPVOID*)&pihtmlTextContainer)))
                {
                    _ASSERTE(pihtmlTextContainer);
                    if (pihtmlTextContainer)
                    {
                        hr = pihtmlTextContainer->get_scrollLeft(&m_ptScroll.x);
                        _ASSERTE(SUCCEEDED(hr));
                        hr = pihtmlTextContainer->get_scrollTop(&m_ptScroll.y);
                        _ASSERTE(SUCCEEDED(hr));
                        hr = S_OK;
                    }
                }
            }
        }
    }
    SAFERELEASE(pihtmlDoc2);
    SAFERELEASE(pihtmlTextContainer);
    SAFERELEASE(pihtmlElement);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::DragScroll
//
// Scroll the Trident document so as to make the given point visible. If a
// drag rectangle is visible it will be erased before any scrolling occurs;
// the caller is responsible for redrawing the rectangle. Returns S_OK if the
// document was scrolled, S_FALSE if not scrolling was required, or a
// Trident error.
//

#define nScrollInset 5

HRESULT CTriEditDocument::DragScroll(POINT pt)
{
    RECT rectClient, rect;
    long x = 0, y = 0;
    IHTMLDocument2* pihtmlDoc2=NULL;
    IHTMLWindow2* pihtmlWindow2=NULL;

    GetClientRect(m_hwndTrident, &rectClient);
    rect = rectClient;
    InflateRect(&rect, -nScrollInset, -nScrollInset);
    if (PtInRect(&rectClient, pt) && !PtInRect(&rect, pt))
    {
        // determine direction of scroll along both X & Y axis
        if (pt.x < rect.left)
            x = -nScrollInset;
        else if (pt.x >= rect.right)
            x = nScrollInset;
        if (pt.y < rect.top)
            y = -nScrollInset;
        else if (pt.y >= rect.bottom)
            y = nScrollInset;
    }

    if (x == 0 && y == 0) // no scrolling required    
        return S_FALSE;

    _ASSERTE(m_pUnkTrident);
    if (SUCCEEDED(GetDocument(&pihtmlDoc2)))
    {
        _ASSERTE(pihtmlDoc2);
        if (SUCCEEDED(pihtmlDoc2->get_parentWindow(&pihtmlWindow2)))
        {
            _ASSERTE(pihtmlWindow2);

            // erase move rectangle before scrolling
            Draw2DDragRect(FALSE);

            pihtmlWindow2->scrollBy(x,y);
        }
    }

    SAFERELEASE(pihtmlDoc2);
    SAFERELEASE(pihtmlWindow2);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::IsDragSource
//
// Return TRUE if the current OLE drag-drop was originated by Trident, or
// FALSE otherwise.
//


BOOL CTriEditDocument::IsDragSource(void)
{
    BOOL fDragSource = FALSE;
    HRESULT hr;
    VARIANT var;

    if (m_pUnkTrident)
    {
        IOleCommandTarget* pioleCmdTarget;
        if (SUCCEEDED(m_pUnkTrident->QueryInterface(IID_IOleCommandTarget,
                (LPVOID*)&pioleCmdTarget)))
        {
            _ASSERTE(pioleCmdTarget);
            if (pioleCmdTarget)
            {
                VariantInit(&var);
                var.vt = VT_BOOL;
                var.boolVal = FALSE;
                hr = pioleCmdTarget->Exec( &CMDSETID_Forms3,
                              IDM_SHDV_ISDRAGSOURCE,
                              MSOCMDEXECOPT_DONTPROMPTUSER,
                              NULL,
                              &var );
                _ASSERTE(SUCCEEDED(hr));
                fDragSource = (var.boolVal) ? TRUE:FALSE;
                pioleCmdTarget->Release();
            }
        }
    }
    return fDragSource;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::ConstrainXY
//
// If TriEdit's constrained dragging mode is enabled, constrain the 
// rectangle of the current element (m_rcElement) vis-a-vis the given
// point according to the current constraint direction, first computing
// the constraint direction if necessary. Return S_OK.
//

HRESULT CTriEditDocument::ConstrainXY(LPPOINT lppt)  //pt is in client coordinates
{
    POINT ptRel;

    if (m_fConstrain)
    {
        if (CONSTRAIN_NONE == m_eDirection)
        {
            ptRel.x = (lppt->x + m_ptScroll.x) - m_ptClickOrig.x;
            ptRel.y = (lppt->y + m_ptScroll.y) - m_ptClickOrig.y;

            if ((ptRel.x && !ptRel.y) || (abs(ptRel.x) > abs(ptRel.y)))
                m_eDirection = CONSTRAIN_HORIZONTAL;
            else
            if ((!ptRel.y && ptRel.y) || (abs(ptRel.y) > abs(ptRel.x)))
                m_eDirection = CONSTRAIN_VERTICAL;
            else
                m_eDirection = CONSTRAIN_HORIZONTAL;

            if (m_eDirection == CONSTRAIN_VERTICAL)
            {
                LONG lWidth = m_rcElement.right - m_rcElement.left;
                
                m_ptClickOrig.x = m_rcElement.left = m_ptConstrain.x;
                m_rcElement.right = m_rcElement.left + lWidth;
            }
            else
            {
                LONG lHeight = m_rcElement.bottom - m_rcElement.top;

                m_ptClickOrig.y = m_rcElement.top = m_ptConstrain.y;
                m_rcElement.bottom = m_rcElement.top + lHeight;
            }
        }
        switch(m_eDirection)
        {
            case CONSTRAIN_HORIZONTAL:
                lppt->y = (m_ptClickOrig.y - m_ptScroll.y);
                break;

            case CONSTRAIN_VERTICAL:
                lppt->x = (m_ptClickOrig.x - m_ptScroll.x);
                break;
        }
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::SnapToGrid
//
// Snap the appropriate edge of the current HTML element (m_rcElement) to the
// given point, modulo the current TriEdit grid setting. Return S_OK.
//

HRESULT CTriEditDocument::SnapToGrid(LPPOINT lppt)  //pt is in client coordinates
{
    POINT ptRel;
    POINT ptDoc;

    _ASSERTE(lppt);

    //determine relative movement
    ptRel.x = (lppt->x + m_ptScroll.x) - m_ptClickOrig.x;
    ptRel.y = (lppt->y + m_ptScroll.y) - m_ptClickOrig.y;
    ptDoc.x = m_rcElement.left - m_rcElementParent.left + ptRel.x;
    ptDoc.y = m_rcElement.top - m_rcElementParent.top + ptRel.y;

    if (ptRel.x < 0)        //LEFT
    {
        if (ptDoc.x % m_ptAlign.x)
            ptDoc.x -= (ptDoc.x % m_ptAlign.x);
        else
            ptDoc.x -= m_ptAlign.x;
    }
    else
    if (ptRel.x > 0)        //RIGHT
    {
        if (ptDoc.x % m_ptAlign.x)
            ptDoc.x += m_ptAlign.x - (ptDoc.x % m_ptAlign.x);
        else
            ptDoc.x += m_ptAlign.x;
    }

    if (ptRel.y < 0)        //UP
    {
        if (ptDoc.y % m_ptAlign.y)
            ptDoc.y -= (ptDoc.y % m_ptAlign.y);
        else
            ptDoc.y -= m_ptAlign.y;
    }
    else
    if (ptRel.y > 0)        //DOWN
    {
        if (ptDoc.y % m_ptAlign.y)
            ptDoc.y += m_ptAlign.y - (ptDoc.y % m_ptAlign.y);
        else
            ptDoc.y += m_ptAlign.y;
    }

    m_rcDragRect.left   = m_rcElementParent.left + ptDoc.x;
    m_rcDragRect.top    = m_rcElementParent.top + ptDoc.y;
    m_rcDragRect.right  = m_rcDragRect.left + (m_rcElement.right  - m_rcElement.left);
    m_rcDragRect.bottom = m_rcDragRect.top + (m_rcElement.bottom - m_rcElement.top);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::IsDesignMode
//
// Return TRUE if Trident is in design (edit) mode, or FALSE if it is in
// browse mode.
//

BOOL CTriEditDocument::IsDesignMode(void)
{
    HRESULT hr;
    OLECMD olecmd;

    olecmd.cmdID = IDM_EDITMODE;
    hr = m_pCmdTgtTrident->QueryStatus(&CMDSETID_Forms3, 1, &olecmd, NULL);

    return (SUCCEEDED(hr) && (olecmd.cmdf & OLECMDF_LATCHED));
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetElementPosition
//
// Return (under prc) the position of the given HTML element in document
// coordinates. Return S_OK or a Trident error code as the return value.
//

HRESULT CTriEditDocument::GetElementPosition(IHTMLElement* pihtmlElement, LPRECT prc)
{
    IHTMLElement* pelem = NULL;
    IHTMLElement* pelemNext = NULL;
    POINT ptExtent;
    HRESULT hr;

    _ASSERTE(pihtmlElement && prc);
    if(!pihtmlElement || !prc)
        return E_POINTER;

    if(FAILED(pihtmlElement->get_offsetLeft(&prc->left)))
        return(E_FAIL);
    if(FAILED(pihtmlElement->get_offsetTop(&prc->top)))
        return(E_FAIL);

    hr = pihtmlElement->get_offsetParent(&pelemNext);

    while (SUCCEEDED(hr) && pelemNext)
    {
        POINT pt;

        if(FAILED(hr = pelemNext->get_offsetLeft(&pt.x)))
            goto QuickExit;
        if(FAILED(hr = pelemNext->get_offsetTop(&pt.y)))
            goto QuickExit;
        prc->left += pt.x;
        prc->top += pt.y;
        pelem = pelemNext;
        pelemNext = NULL;
        hr = pelem->get_offsetParent(&pelemNext);
        SAFERELEASE(pelem);
    }

    if (FAILED(hr = pihtmlElement->get_offsetWidth(&ptExtent.x)))
        goto QuickExit;
    if (FAILED(hr = pihtmlElement->get_offsetHeight(&ptExtent.y)))
        goto QuickExit;

    prc->right  = prc->left + ptExtent.x;
    prc->bottom = prc->top  + ptExtent.y;

QuickExit:
    _ASSERTE(SUCCEEDED(hr));
    SAFERELEASE(pelem);
    SAFERELEASE(pelemNext);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::GetTridentWindow
//
// Fetch the IOleWindow interface of the Trident instance in to m_hwndTrident.
// Return S_OK or the Trident error code.
//

STDMETHODIMP CTriEditDocument::GetTridentWindow()
{
    LPOLEWINDOW piolewinTrident;
    HRESULT hr = E_FAIL;

    if( m_pOleObjTrident &&
        SUCCEEDED(hr = m_pOleObjTrident->QueryInterface(IID_IOleWindow, (LPVOID*)&piolewinTrident)))
    {
        m_hwndTrident = NULL;
        hr = piolewinTrident->GetWindow(&m_hwndTrident);
        _ASSERTE(m_hwndTrident != NULL);
        piolewinTrident->Release();
    }

    _ASSERTE(SUCCEEDED(hr));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CTriEditDocument::CalculateNewDropPosition
//
// Adjust the given point to adjust for the fact that the Trident document may
// be scrolled. Return S_OK or a Trident error code.

HRESULT CTriEditDocument::CalculateNewDropPosition(POINT *pt)
{
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(hr = GetTridentWindow()) && 
        ScreenToClient(m_hwndTrident, pt) &&
        SUCCEEDED(hr = GetScrollPosition()))
    {
        pt->x += m_ptScroll.x;
        pt->y += m_ptScroll.y;
    }

    return hr;
}
