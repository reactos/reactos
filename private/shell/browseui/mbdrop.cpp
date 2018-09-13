//
//  Routines for implementing drop target capability to menubands.
//

#include "priv.h"
#include "mbdrop.h"
#include "iface.h"      // for MBIF_

#define SUPERCLASS 


//=================================================================
// Implementation of CMenuBandDropTarget
//=================================================================

// Constructor
CMenuBandDropTarget::CMenuBandDropTarget(HWND hwnd, int idTarget, DWORD dwFlags) : 
    _cRef(1), _hwndParent(hwnd), _idTarget(idTarget), _dwFlagsMBIF(dwFlags)
{
}

STDMETHODIMP_(ULONG) CMenuBandDropTarget::AddRef()
{
    _cRef++;
    return _cRef;
}


STDMETHODIMP_(ULONG) CMenuBandDropTarget::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0) 
        return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP CMenuBandDropTarget::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CMenuBandDropTarget, IDropTarget),
        { 0 },
    };

    hres = QISearch(this, (LPCQITAB)qit, riid, ppvObj);

    return hres;
}


/*----------------------------------------------------------
Purpose: IDropTarget::DragEnter method

*/
STDMETHODIMP CMenuBandDropTarget::DragEnter(IDataObject * pdtobj, DWORD grfKeyState, 
                                  POINTL pt, DWORD * pdwEffect)
{
    // If this item cascades out, then we want to pop the submenu open 
    // after a timer.  We don't allow a drop on the cascadable item
    // itself.  (We could, but then we'd have to default to a location
    // inside the submenu, and I'm lazy right now.)

    if (*pdwEffect & (DROPEFFECT_MOVE | DROPEFFECT_COPY)) 
    {
        if (_dwFlagsMBIF & SMIF_SUBMENU)
        {
            // _idTimer = SetTimer(NULL, 0, 2000, 
        }

        *pdwEffect &= (DROPEFFECT_MOVE | DROPEFFECT_COPY);
    }
    else
        *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}    


/*----------------------------------------------------------
Purpose: IDropTarget::DragOver method

*/
STDMETHODIMP CMenuBandDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    *pdwEffect &= (DROPEFFECT_MOVE | DROPEFFECT_COPY);
    return S_OK;
}    


/*----------------------------------------------------------
Purpose: IDropTarget::DragLeave method

*/
STDMETHODIMP CMenuBandDropTarget::DragLeave(void)
{
    // Kill timer, release object
    return S_OK;
}


/*----------------------------------------------------------
Purpose: IDropTarget::Drop method

*/
STDMETHODIMP CMenuBandDropTarget::Drop(IDataObject * pdtobj, DWORD grfKeyState, POINTL pt, 
                                       DWORD * pdwEffect)
{
    if (*pdwEffect & (DROPEFFECT_MOVE | DROPEFFECT_COPY)) 
    {
        if (_dwFlagsMBIF & SMIF_SUBMENU)
        {
            // We don't allow drops on submenu items.  Must go into
            // cascaded menu.
            *pdwEffect = DROPEFFECT_NONE;
        }
    }
    return S_OK;
}    


