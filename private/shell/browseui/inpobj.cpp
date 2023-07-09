#include "priv.h"
#include "inpobj.h"

#define DM_FOCUS        0   // focus stuff
#define DM_FOCUS2       0   // like DM_FOCUS, but verbose

//***   IInputObject {

HRESULT UnkHasFocusIO(IUnknown *punkThis)
{
    HRESULT hres = E_FAIL;

    if (punkThis != NULL) {
        IInputObject *pio;

        hres = punkThis->QueryInterface(IID_IInputObject, (LPVOID*)&pio);
        if (SUCCEEDED(hres)) {
            hres = pio->HasFocusIO();
            pio->Release();
        }
    }

    if (hres != S_OK)
        TraceMsg(DM_FOCUS2, "hfio hres=%x (!=S_OK)", hres);
    return hres;
}

HRESULT UnkTranslateAcceleratorIO(IUnknown *punkThis, MSG* pmsg)
{
    HRESULT hres = E_FAIL;

    if (punkThis != NULL) {
        IInputObject *pio;

        hres = punkThis->QueryInterface(IID_IInputObject, (LPVOID*)&pio);
        if (SUCCEEDED(hres)) {
            hres = pio->TranslateAcceleratorIO(pmsg);
            pio->Release();
        }
    }

    return hres;
}

HRESULT UnkUIActivateIO(IUnknown *punkThis, BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hres = E_FAIL;

    if (punkThis != NULL) {
        IInputObject *pio;

        hres = punkThis->QueryInterface(IID_IInputObject, (LPVOID*)&pio);
        if (SUCCEEDED(hres)) {
            hres = pio->UIActivateIO(fActivate, lpMsg);
            pio->Release();
        }
    }

    if (FAILED(hres))
        TraceMsg(DM_FOCUS2, "uiaio(fActivate=%d) hres=%x (FAILED)", fActivate, hres);

    return hres;
}

// }

//***   IInputObjectSite {

HRESULT UnkOnFocusChangeIS(IUnknown *punkThis, IUnknown *punkSrc, BOOL fSetFocus)
{
    HRESULT hres = E_FAIL;

    if (punkThis != NULL) {
        IInputObjectSite *pis;

        hres = punkThis->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pis);
        if (SUCCEEDED(hres)) {
            hres = pis->OnFocusChangeIS(punkSrc, fSetFocus);
            pis->Release();
        }
    }

    if (FAILED(hres))
        TraceMsg(DM_FOCUS, "ofcis(punk=%x fSetFocus=%d) hres=%x (FAILED)", punkSrc, fSetFocus, hres);

    return hres;
}

// }

//***   composites {

//***   _MayUIActTAB -- attempt TAB-activation of IOleWindow/IInputObject
// ENTRY/EXIT
//  powEtc      IOleWindow/IInputObject pair.
//  lpMsg       msg causing activation (may be NULL) (typically TAB)
//  fShowing    currently showing?
//  phwnd       [OUT] hwnd for object
//  hr          [RET] UIActivateIO result, plus E_FAIL
// DESCRIPTION
//  when TABing we only want to activate certain guys, viz. those who are
// currently showing, visible, and willing to accept activation.
HRESULT _MayUIActTAB(IOleWindow *powEtc, LPMSG lpMsg, BOOL fShowing, HWND *phwnd)
{
    HRESULT hr;
    HWND hwnd = 0;

    hr = E_FAIL;
    if (powEtc && fShowing)
    {
        hr = powEtc->GetWindow(&hwnd);
        if (IsWindowVisible(hwnd))
            hr = UnkUIActivateIO(powEtc, TRUE, lpMsg);
    }

    if (phwnd)
        *phwnd = hwnd;

    return hr;
}

// }
