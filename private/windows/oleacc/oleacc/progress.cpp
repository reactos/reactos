// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PROGRESS.CPP
//
//  Wrapper for COMCTL32's progress bar
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "progress.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOTOOLBAR
#define NOHOTKEY
#define NOHEADER
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE
#include <commctrl.h>



// --------------------------------------------------------------------------
//
//  CreateProgressBarClient()
//
//  EXTERNAL for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateProgressBarClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvProgress)
{
    CProgressBar*   pprogress;
    HRESULT         hr;

    InitPv(ppvProgress);

    pprogress = new CProgressBar(hwnd, idChildCur);
    if (!pprogress)
        return(E_OUTOFMEMORY);

    hr = pprogress->QueryInterface(riid, ppvProgress);
    if (!SUCCEEDED(hr))
        delete pprogress;

    return(hr);
}




// --------------------------------------------------------------------------
//
//  CProgressBar::CProgressBar()
//
// --------------------------------------------------------------------------
CProgressBar::CProgressBar(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = TRUE;
}



// --------------------------------------------------------------------------
//
//  CProgressBar::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CProgressBar::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_PROGRESSBAR;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CProgressBar::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CProgressBar::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    LONG    lCur;
    LONG    lMin;
    LONG    lMax;
    TCHAR   szPercentage[16];
    TCHAR   szFormat[8];

    InitPv(pszValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // The value of the progress bar is the current percent complete.  This is
    // cur pos - low pos / high pos - low pos.
    //

    //
    // We can now get the range and the position without changing them,
    // Thank you CheeChew!
    //
    lCur = SendMessageINT(m_hwnd, PBM_GETPOS, 0, 0);
    lMin = SendMessageINT(m_hwnd, PBM_GETRANGE, TRUE, 0);
    lMax = SendMessageINT(m_hwnd, PBM_GETRANGE, FALSE, 0);

    //
    // Don't want to divide by zero.
    //
    if (lMin == lMax)
        lCur = 100;
    else
    {
        //
        // Convert to a percentage.
        //
        lCur = max(lCur, lMin);
        lCur = min(lMax, lCur);
        
        lCur = (100 * (lCur - lMin)) / (lMax - lMin);
    }

    // Make a string
    LoadString(hinstResDll, STR_PERCENTAGE_FORMAT, szFormat, ARRAYSIZE(szFormat));
    wsprintf(szPercentage, szFormat, lCur);

    *pszValue = TCharSysAllocString(szPercentage);
    if (! *pszValue)
        return(E_OUTOFMEMORY);
    else
        return(S_OK);
}
