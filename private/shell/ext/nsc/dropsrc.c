#include <windows.h>

#include "dropsrc.h"
#include "common.h"

//-----------------------------------------------------------------

typedef struct {
    IDropSource dsrc;
    UINT cRef;
    DWORD grfInitialKeyState;
} CDropSource;

IDropSourceVtbl c_CDropSourceVtbl;	// forward decl

HRESULT CDropSource_CreateInstance(IDropSource **ppdsrc)
{
    CDropSource *this = (CDropSource *)LocalAlloc(LPTR, sizeof(CDropSource));
    if (this)
    {
        this->dsrc.lpVtbl = &c_CDropSourceVtbl;
        this->cRef = 1;
        *ppdsrc = &this->dsrc;

        return S_OK;
    }
    else
    {
	*ppdsrc = NULL;
	return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CDropSource_QueryInterface(IDropSource *pdsrc, REFIID riid, void **ppvObj)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    if (IsEqualIID(riid, &IID_IDropSource) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = this;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDropSource_AddRef(IDropSource *pdsrc)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    this->cRef++;
    return this->cRef;
}

STDMETHODIMP_(ULONG) CDropSource_Release(IDropSource *pdsrc)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    this->cRef--;
    if (this->cRef > 0)
	return this->cRef;

    LocalFree((HLOCAL)this);

    return 0;
}

STDMETHODIMP CDropSource_QueryContinueDrag(IDropSource *pdsrc, BOOL fEscapePressed, DWORD grfKeyState)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;

    // initialize ourself with the drag begin button
    if (this->grfInitialKeyState == 0)
        this->grfInitialKeyState = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

    Assert(this->grfInitialKeyState);

    if (!(grfKeyState & this->grfInitialKeyState))
        return DRAGDROP_S_DROP;	
    else
        return S_OK;
}

STDMETHODIMP CDropSource_GiveFeedback(IDropSource *pdsrc, DWORD dwEffect)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

IDropSourceVtbl c_CDropSourceVtbl = {
    CDropSource_QueryInterface, CDropSource_AddRef, CDropSource_Release,
    CDropSource_QueryContinueDrag,
    CDropSource_GiveFeedback
};


