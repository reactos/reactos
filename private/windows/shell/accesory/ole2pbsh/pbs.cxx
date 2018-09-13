//+---------------------------------------------------------------------
//
//   File:       pbs.cxx
//
//   Contents:   Paintbrush OLE2 Server Class code
//
//   Classes:
//               PBCtrl
//               PBInPlace
//               PBDV
//
//               CXBag
//
//------------------------------------------------------------------------

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>      // common dialog boxes

#include <ole2.h>
#include <o2base.hxx>     // the base classes and utilities

#include "pbs.hxx"

    extern PBFactory *gpPBFactory;

    LPPBCTRL gpCtrlThis = NULL;
    DWORD gdwRegROT = 0L;
    LPMONIKER gpFileMoniker = NULL;
    LPXBAG gpXBagOnClipboard = NULL;
    extern BOOL gfInPlace;
    //
    // the following is a flag from the old PBrush C code
    // indicating whether or not the image is dirty:
    extern "C" BOOL gfDirty;


CLIPFORMAT gacfAccepted[] =
{
    CF_BITMAP,
    CF_DIB,
    CF_METAFILEPICT,
    CF_UNICODETEXT,
    CF_TEXT
};

//
// Glue functions for the C code
//===============================
//

void LockPBObject(void)
{
    DOUT(L"\\\\PBrush: LockPBObject\r\n");
    if(gpCtrlThis != NULL)
    {
        gpCtrlThis->Lock();
    }
    else
    {
        DOUT(L"\\\\PBrush: LockPBObject (gpCtrlThis == NULL)!!!\r\n");
    }
}

void UnLockPBObject(void)
{
    DOUT(L"\\\\PBrush: UnLockPBObject\r\n");
    if(gpCtrlThis != NULL)
    {
        gpCtrlThis->UnLock();
    }
    else
    {
        DOUT(L"\\\\PBrush: UnLockPBObject (gpCtrlThis == NULL)!!!\r\n");
    }
}

void
FlushOleClipboard(void)
{
    if(gfXBagOnClipboard)
    {
        if(OleIsCurrentClipboard(gpXBagOnClipboard) == NOERROR)
        {
            DOUT(L"\\\\PBrush: OleFlushClipboard()\r\n");
            OleFlushClipboard();
            gpXBagOnClipboard->Detach();
            gpXBagOnClipboard = NULL;
        }
        gfXBagOnClipboard = FALSE;
    }
}

LPWSTR
GetClientObjName(void)
{
    static OLECHAR achNoName[] = L"";

    if(gpCtrlThis != NULL && gpCtrlThis->_lpstrCntrObj != NULL)
        return gpCtrlThis->_lpstrCntrObj;
    return achNoName;
}

BOOL
OleClipboardContainsAcceptableFormats(CLIPFORMAT FAR* lpcf)
{
    BOOL fContainsAcceptableFormats = FALSE;
    LPDATAOBJECT pDataObj = NULL;
    if(OleGetClipboard(&pDataObj) == NOERROR)
    {
        fContainsAcceptableFormats = ObjectOffersAcceptableFormats(pDataObj, lpcf);
        pDataObj->Release();
    }
    return fContainsAcceptableFormats;
}

BOOL
GetTypedHGlobalFromOleClipboard(CLIPFORMAT cf, HGLOBAL FAR* lphGlobal)
{
    BOOL fSucess = FALSE;
    *lphGlobal = NULL;
    LPDATAOBJECT pDataObj = NULL;
    if(OleGetClipboard(&pDataObj) == NOERROR)
    {
        if(GetTypedHGlobalFromObject(pDataObj, &cf, lphGlobal) == NOERROR)
            fSucess = TRUE;
        pDataObj->Release();
    }

    return fSucess;
}

void
TransferToClipboard(void)
{
    DOUT(L"TransferToClipboard\r\n");

    //
    // BUGBUG we should just Assert(gpCtrlThis != NULL)
    //
    if(gpCtrlThis != NULL)
    {
        LPXBAG pXBag;
        HRESULT hr = CXBag::Create(&pXBag, gpCtrlThis, NULL);
        if(hr == NOERROR)
        {
            if(gfXBagOnClipboard)
                OleSetClipboard(NULL);
            hr = OleSetClipboard(pXBag);
            gfXBagOnClipboard = (hr == NOERROR) ? TRUE : FALSE;
            if(hr == NOERROR)
            {
                gpXBagOnClipboard = pXBag;
            }
            else
            {
                DOUT(L"TransferToClipboard FAILED!\r\n");
            }
        }
    }
    else
    {
        DOUT(L"TransferToClipboard called with a NULL gpCtrlThis!\r\n");
    }
}

void
RegisterAsDropTarget(HWND hwnd)
{
    if(gpCtrlThis != NULL && gpCtrlThis->_pInPlace != NULL)
    {
        LPPBINPLACE lpPbIp = (LPPBINPLACE)gpCtrlThis->_pInPlace;
        lpPbIp->RegisterAsDropTarget(hwnd);
    }
}

void
RevokeOurDropTarget(void)
{
    if(gpCtrlThis != NULL && gpCtrlThis->_pInPlace != NULL)
    {
        LPPBINPLACE lpPbIp = (LPPBINPLACE)gpCtrlThis->_pInPlace;
        lpPbIp->RevokeOurDropTarget();
    }
}

void
SetNativeExtents( int cx, int cy )
{
    if(gpCtrlThis != NULL && gpCtrlThis->_pDV != NULL)
    {
        LPPBDV lpPBDV = (LPPBDV)gpCtrlThis->_pDV;
        lpPBDV->SetNativeExtents(cx, cy);
    }
}

void
GetInPlaceInfo(LPOLEINPLACEFRAME *ppFrame, OLEINPLACEFRAMEINFO **ppInfo)
{
    if(gfStandalone || gpCtrlThis == NULL || gpCtrlThis->_pInPlace == NULL )
    {
        *ppFrame = NULL;
        *ppInfo = NULL;
    }
    else
    {
        LPPBINPLACE lpPbIp = (LPPBINPLACE)gpCtrlThis->_pInPlace;
        lpPbIp->GetInPlaceInfo(ppFrame, ppInfo);
    }
}

int
CalcMenuPos(int iMenu)
{
    if(gfStandalone || gpCtrlThis == NULL || gpCtrlThis->_pInPlace == NULL)
        return iMenu;
    LPPBINPLACE lpPbIp = (LPPBINPLACE)gpCtrlThis->_pInPlace;
    return lpPbIp->CalcMenuPos(iMenu);

}

HWND
GetInPlaceFrameWindow()
{
    if(gfStandalone || gpCtrlThis == NULL)
        return NULL;
    return gpCtrlThis->GetFrameWindow();
}

void
AdviseDataChange(void)
{
    if(!gfStandalone && gpCtrlThis && !gfInPlace)
    {
        DOUT(L"PBrush: AdviseDataChange sending OnDataChange...\n\r");
        gpCtrlThis->_pDV->OnDataChange();
    }
    else if(gfInPlace)
        gfDirty = TRUE;   //make sure we see this as dirty data...
}

void
AdviseRename(LPTSTR lpname)
{
    //
    //BUGBUG: do we need to anything with the filemoniker in rename cases?
    //
    if(!gfStandalone && gpCtrlThis)
    {
        if (gpCtrlThis->_pOleAdviseHolder != NULL)
        {
            LPMONIKER pmk = NULL;
            if(NOERROR == CreateFileMoniker(lpname, &pmk))
            {
                gpCtrlThis->_pOleAdviseHolder->SendOnRename(pmk);
                pmk->Release();
            }
        }
    }
}

void
DoOleSave(void)
{
    if(!gfStandalone && gpCtrlThis && gpCtrlThis->_pClientSite)
    {
        gpCtrlThis->_pClientSite->SaveObject();
    }
}

void
DoOleClose(BOOL fSave)
{
    if(gpCtrlThis)
    {
        DOUT(L"PBrush: DoOleClose\r\n");
        if(gfStandalone)
        {
            RevokeAsRunning(&gdwRegROT);
            if(gpFileMoniker != NULL)
                gpFileMoniker->Release();
            gfClosing = TRUE;
            gpCtrlThis->UnLock();
            gpCtrlThis->Release();
        }
        else
            gpCtrlThis->Close(fSave ? OLECLOSE_SAVEIFDIRTY: OLECLOSE_NOSAVE);
    }
}

//
// Data Transfer Utilities
//========================
//

BOOL
ObjectOffersAcceptableFormats(LPDATAOBJECT pDataObj, CLIPFORMAT FAR* lpcf)
{
    *lpcf = 0;
    FORMATETC formatetc;
    formatetc.ptd = NULL;
    formatetc.lindex = (DWORD)-1;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.tymed = TYMED_HGLOBAL;
    for(int i = 0; i < sizeof(gacfAccepted) / sizeof(CLIPFORMAT); i++)
    {
        formatetc.cfFormat = gacfAccepted[i];
        switch(formatetc.cfFormat)
        {
        case CF_BITMAP:
            formatetc.tymed = TYMED_GDI;
            break;
        case CF_ENHMETAFILE:
        case CF_METAFILEPICT:
            formatetc.tymed = TYMED_MFPICT;
            break;
        }
        if(pDataObj->QueryGetData(&formatetc) == NOERROR)
        {
            *lpcf = formatetc.cfFormat;
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
GetTypedHGlobalFromObject(LPDATAOBJECT pDataObj, CLIPFORMAT FAR* lpcf, HGLOBAL FAR* lphGlobal)
{
    *lphGlobal = NULL;
    FORMATETC formatetc;
    formatetc.ptd = NULL;
    formatetc.lindex = (DWORD)-1;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.tymed = TYMED_HGLOBAL;
    formatetc.cfFormat = *lpcf;
    STGMEDIUM medium = { TYMED_HGLOBAL, NULL, NULL };
    switch(formatetc.cfFormat)
    {
    case CF_PALETTE:
    case CF_BITMAP:
        medium.tymed = formatetc.tymed = TYMED_GDI;
        break;
    case CF_ENHMETAFILE:
    case CF_METAFILEPICT:
        medium.tymed = formatetc.tymed = TYMED_MFPICT;
        break;
    }
    HRESULT hr = pDataObj->GetData(&formatetc, &medium);
    if(hr == NOERROR)
        *lphGlobal = medium.hGlobal;
    return hr;
}

//
// Data Transfer Object
//========================
//


//+---------------------------------------------------------------
//
//  Member:     CXBag::Create, static
//
//  Synopsis:   Create a new, initialized transfer object
//
//---------------------------------------------------------------
HRESULT
CXBag::Create(LPXBAG *ppXBag, LPPBCTRL pHost, LPPOINT pptSelected)
{
    Assert(ppXBag != NULL && pHost != NULL);
    LPXBAG pXBag = new CXBag(pHost);
    if((*ppXBag = pXBag) == NULL)
        return E_OUTOFMEMORY;

    GetPickRect(&pXBag->_rcSelection);
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::CXBag
//
//  Synopsis:   constructor
//
//---------------------------------------------------------------
CXBag::CXBag(LPPBCTRL pHost)
{
    _pHost = pHost;
    _ulRefs = 1;
    _pStgBag = NULL;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::~CXBag
//
//  Synopsis:   destructor
//
//---------------------------------------------------------------
CXBag::~CXBag()
{
    if(_pStgBag != NULL)
        _pStgBag->Release();
}


IMPLEMENT_STANDARD_IUNKNOWN(CXBag)

//+---------------------------------------------------------------
//
//  Member:     CXBag::QueryInterface, public
//
//  Synopsis:   Expose our IFaces
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
#ifdef VERBOSE
#if DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"CXBag::QueryInterface (%lx)\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif //DBG
#endif //VERBOSE

    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IDataObject))
    {
        *ppv = (LPVOID)(LPDATAOBJECT)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::DAdvise
//
//  Notes:      Not suported by transfer objects (us)
//
//---------------------------------------------------------------
HRESULT
CXBag::DAdvise( FORMATETC FAR* pFormatetc,
                DWORD advf,
                LPADVISESINK pAdvSink,
                DWORD FAR* pdwConnection)
{
    DOUT(L"CXBag::DAdvise OLE_E_ADVISENOTSUPPORTED\r\n");
    return OLE_E_ADVISENOTSUPPORTED;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::DUnadvise
//
//  Notes:      Not suported by transfer objects (us)
//
//---------------------------------------------------------------
HRESULT
CXBag::DUnadvise( DWORD dwConnection)
{
    DOUT(L"CXBag::DUnadvise OLE_E_ADVISENOTSUPPORTED\r\n");
    return OLE_E_ADVISENOTSUPPORTED;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::EnumDAdvise
//
//  Notes:      Not suported by transfer objects (us)
//
//---------------------------------------------------------------
HRESULT
CXBag::EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise)
{
    DOUT(L"CXBag::EnumDAdvise OLE_E_ADVISENOTSUPPORTED\r\n");
    return OLE_E_ADVISENOTSUPPORTED;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::GetCanonicalFormatEtc
//
//  Notes:      Not suported by transfer objects (us)
//
//---------------------------------------------------------------
HRESULT
CXBag::GetCanonicalFormatEtc( LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    DOUT(L"CXBag::GetCanonicalFormatEtc E_NOTIMPL\r\n");
    pformatetcOut->ptd = NULL;
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::SetData
//
//  Notes:      Not suported by transfer objects (us)
//
//---------------------------------------------------------------
HRESULT
CXBag::SetData(LPFORMATETC pformatetc,
                            STGMEDIUM FAR * pmedium,
                            BOOL fRelease)
{
    DOUT(L"CXBag::SetData E_NOTIMPL\r\n");
    return E_NOTIMPL;
}

//
// List of formats offered by our data transfer object via EnumFormatEtc
// NOTE: OleClipFormat is a global array of stock formats defined in
//       o2base\dvutils.cxx and initialized via RegisterOleClipFormats()
//       in our class factory.
//
static FORMATETC g_aGetFmtEtcs[] =
{
    { CF_BITMAP, NULL, DVASPECT_ALL, -1L, TYMED_GDI },
    { CF_PALETTE, NULL, DVASPECT_ALL, -1L, TYMED_GDI },
    { CF_METAFILEPICT, NULL, DVASPECT_ALL, -1L, TYMED_MFPICT },
    { (CLIPFORMAT)(OCF_EMBEDSOURCE + MAX_CF_VAL), NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE },
    { (CLIPFORMAT)(OCF_OBJECTDESCRIPTOR + MAX_CF_VAL), NULL, DVASPECT_ALL, -1L, TYMED_HGLOBAL },
    { (CLIPFORMAT)(OCF_LINKSOURCE + MAX_CF_VAL), NULL, DVASPECT_ALL, -1L, TYMED_ISTREAM | TYMED_HGLOBAL },
    { (CLIPFORMAT)(OCF_LINKSRCDESCRIPTOR + MAX_CF_VAL), NULL, DVASPECT_ALL, -1L, TYMED_HGLOBAL }
};

//+---------------------------------------------------------------
//
//  Member:     CXBag::EnumFormatEtc
//
//     OLE:     IDataObject
//
//  Synopsis:   Answer an enumerator over supported formats
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    DOUT(L"CXBag::EnumFormatEtc\r\n");
    HRESULT hr = E_NOTIMPL;
    *ppenumFormatEtc = NULL;
    int cFormats = sizeof(g_aGetFmtEtcs) / sizeof(g_aGetFmtEtcs[0]);

    if (dwDirection == DATADIR_GET)
    {
        DOUT(L"CXBag::EnumFormatEtc returning our enumerator\r\n");
        //
        // Let our static-array enumerator do the work...
        //
        hr = CreateFORMATETCEnum(g_aGetFmtEtcs, gfWholeHog ? cFormats : cFormats - 2, ppenumFormatEtc);
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::GetDataHere
//
//     OLE:     IDataObject
//
//  Synopsis:   Deliver requested format in specified medium
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    SetTransferFlag(TRUE);
    DOUT(L"CXBag::GetDataHere\r\n");
    HRESULT hr = DV_E_FORMATETC;
    if ( (pformatetc->cfFormat == OleClipFormat[OCF_EMBEDSOURCE]) &&
         (pformatetc->dwAspect == DVASPECT_CONTENT) &&
         (pformatetc->tymed == TYMED_ISTORAGE) )
    {
        DOUT(L"CXBag::GetDataHere OCF_EMBEDSOURCE on TYMED_ISTORAGE\r\n");
        hr = BagItInStorage(pmedium, TRUE);
    }
    else if(pformatetc->tymed & TYMED_ISTREAM)
    {
        DOUT(L"CXBag::GetDataHere on TYMED_ISTREAM\r\n");
        if ( pformatetc->cfFormat == OleClipFormat[OCF_LINKSOURCE] )
        {
            DOUT(L"CXBag::GetData OCF_LINKSOURCE on STREAM\r\n");
            if(_pHost != NULL)
            {
                hr = _pHost->_pDV->GetLINKSOURCE( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetc,
                                                    pmedium,
                                                    TRUE /* fHere */ );
            }
        }
    }
    else if((pformatetc->tymed & TYMED_HGLOBAL) && (_pHost != NULL))
    {
        DOUT(L"CXBag::GetDataHere on TYMED_HGLOBAL\r\n");
        if ( pformatetc->cfFormat == OleClipFormat[OCF_OBJECTDESCRIPTOR] )
        {
            DOUT(L"CXBag::GetDataHere OCF_OBJECTDESCRIPTOR\r\n");
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetc,
                                                    pmedium,
                                                    TRUE /* fHere */ );
        }
#ifdef LATER
        else if (gfWholeHog && pformatetc->cfFormat == OleClipFormat[OCF_LINKSOURCE])
        {
            DOUT(L"CXBag::GetDataHere OCF_LINKSOURCE\r\n");
            hr = _pHost->_pDV->GetLINKSOURCE( (LPSRVRDV)(_pHost->_pDV),
                                                pformatetc,
                                                pmedium,
                                                TRUE /* fHere */ );
        }
        else if ( pformatetc->cfFormat == OleClipFormat[OCF_LINKSRCDESCRIPTOR] )
        {
            DOUT(L"CXBag::GetDataHere OCF_LINKSRCDESCRIPTOR\r\n");
            hr = _pHost->_pDV->GetLINKSOURCE( (LPSRVRDV)(_pHost->_pDV),
                                                pformatetc,
                                                pmedium,
                                                TRUE /* fHere */ );
        }
        else if ( pformatetc->cfFormat == CF_PALETTE )
        {
            DOUT(L"CXBag::GetDataHere CF_PALETTE\r\n");
            if((pmedium->hGlobal = GetTransferPalette()) != NULL)
            {
                hr = NOERROR;
            }
        }
        else if ( pformatetc->cfFormat == CF_BITMAP )
        {
            DOUT(L"CXBag::GetDataHere CF_BITMAP\r\n");
            if((pmedium->hGlobal = GetTransferBitmap()) != NULL)
            {
                hr = NOERROR;
            }
        }
#endif //LATER
    }
    else if(pformatetc->tymed & TYMED_MFPICT)
    {
        DOUT(L"CXBag::GetDataHere on TYMED_MFPICT\r\n");
        if ( pformatetc->cfFormat == CF_METAFILEPICT )
        {
            DOUT(L"CXBag::GetDataHere CF_METAFILEPICT (on MFPICT)\r\n");
            if(_pHost != NULL)
            {
                hr = _pHost->_pDV->GetMETAFILEPICT( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetc,
                                                    pmedium,
                                                    TRUE /* fHere */ );
            }
        }
    }
    SetTransferFlag(FALSE);
    return hr;
}

#define nPelsPerLogInch(hDc, fHoriz) GetDeviceCaps(hDC, fHoriz ? LOGPIXELSX : LOGPIXELSY)
#define nHMPerInch 2540
#define nMMPerMeterTimes10 10000
#define nMMPerInchTimes10  254

static inline int
PelsToLogHimetric( HDC hDC, BOOL bHoriz, int cPels )
{
    return MulDiv(cPels, nHMPerInch, nPelsPerLogInch(hDc, bHoriz) );
}

static inline void
RectConvertToHiMetric( HDC hDC, LPRECT pRect )
{
    if(pRect->left != 0)
        pRect->left = PelsToLogHimetric(hDC, 1 , pRect->left);
    if(pRect->top != 0)
        pRect->top = PelsToLogHimetric(hDC, 0, pRect->top);
    if(pRect->right != 0)
        pRect->right = PelsToLogHimetric(hDC, 1, pRect->right);
    if(pRect->bottom != 0)
        pRect->bottom = PelsToLogHimetric(hDC, 0, pRect->bottom);
}

HANDLE
MetafilePictFromBmp( HBITMAP hbm, HPALETTE hpal )
{
    BITMAP bitmap;
    LPMETAFILEPICT pMF;
    HDC hDC = NULL;
    HANDLE hMem = NULL;
    HDC hMemDC = NULL;
    HANDLE hMF = NULL;
    RECT rc = { 0,0,0,0 };
    HPALETTE hpalOld, hpalDefault;

    if(hbm == NULL)
    {
        return(NULL);
    }

    GetObject(hbm, sizeof(BITMAP), (LPSTR)&bitmap);
    rc.right = bitmap.bmWidth;
    rc.bottom = bitmap.bmHeight;
    if((hDC = (HDC)CreateMetaFile(NULL)) != NULL)
    {
        if((hMemDC = CreateCompatibleDC(NULL)) != NULL)
        {
            SetMapMode(hDC, MM_ANISOTROPIC);
            SetWindowOrgEx(hDC, 0, 0, NULL);
            SetWindowExtEx(hDC, rc.right, rc.bottom, NULL);

            if (hpal) {
                hpalOld = SelectPalette(hMemDC, hpal, FALSE);
                RealizePalette(hMemDC);

                SelectPalette(hDC, hpal, TRUE);
                RealizePalette(hDC);
            }

            SelectObject(hMemDC, hbm);
            StretchBlt(hDC, 0, 0, rc.right, rc.bottom,
                    hMemDC, 0, 0, rc.right, rc.bottom, SRCCOPY);

            if (hpalDefault = (HPALETTE)GetStockObject(DEFAULT_PALETTE))
            {
                SelectPalette(hDC, hpalDefault, TRUE);
                //RealizePalette(hDC);
            }

            hMF = CloseMetaFile(hDC);
            RectConvertToHiMetric(hMemDC, &rc);

            if (hpalOld)
            {
                SelectPalette(hMemDC, hpalOld, FALSE);
                RealizePalette(hMemDC);
            }

            DeleteDC(hMemDC);

            if((hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE,
                    sizeof(METAFILEPICT))) == NULL)
            {
                DeleteMetaFile((HMETAFILE)hMF);
                return(NULL);
            }

            pMF = (LPMETAFILEPICT)GlobalLock(hMem);
            pMF->hMF = (HMETAFILE)hMF;
            pMF->mm = MM_ANISOTROPIC;
            pMF->xExt = rc.right;
            pMF->yExt = rc.bottom;

            GlobalUnlock(hMem);
            return hMem;
        }
    }
    return hMF;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::GetData
//
//     OLE:     IDataObject
//
//  Synopsis:   Deliver data in requested format
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    DOUT(L"CXBag::GetData\r\n");

    SetTransferFlag(TRUE);
    pmedium->pUnkForRelease = NULL;    // transfer ownership to caller
    pmedium->hGlobal = NULL;
    HRESULT hr = DV_E_FORMATETC;
    if ( (pformatetcIn->cfFormat == OleClipFormat[OCF_EMBEDSOURCE]) &&
         (pformatetcIn->dwAspect == DVASPECT_CONTENT) &&
         (pformatetcIn->tymed == TYMED_ISTORAGE) )
    {
        hr = BagItInStorage(pmedium,FALSE);
    }
    else if(gfWholeHog && pformatetcIn->tymed & TYMED_ISTREAM)
    {
        if ( pformatetcIn->cfFormat == OleClipFormat[OCF_LINKSOURCE] )
        {
            DOUT(L"CXBag::GetDataHere OCF_LINKSOURCE on STREAM\r\n");
            if(_pHost != NULL)
            {
                hr = _pHost->_pDV->GetLINKSOURCE( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetcIn,
                                                    pmedium,
                                                    FALSE /* fHere */ );
            }
        }
    }
    else if((pformatetcIn->tymed & TYMED_HGLOBAL) && (_pHost != NULL))
    {
        pmedium->tymed = TYMED_HGLOBAL;
        if ( pformatetcIn->cfFormat == OleClipFormat[OCF_OBJECTDESCRIPTOR] )
        {
            DOUT(L"CXBag::GetData OCF_OBJECTDESCRIPTOR on HGLOBAL\r\n");
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetcIn,
                                                    pmedium,
                                                    FALSE /* fHere */ );
        }
#ifdef FOOLISH
        else if (gfWholeHog && pformatetcIn->cfFormat == OleClipFormat[OCF_LINKSOURCE] )
        {
            DOUT(L"CXBag::GetData OCF_LINKSOURCE on HGLOBAL\r\n");
            hr = _pHost->_pDV->GetLINKSOURCE( (LPSRVRDV)(_pHost->_pDV),
                                                pformatetcIn,
                                                pmedium,
                                                FALSE /* fHere */ );
            GetHGlobalFromStream(pmedium->pstm, &pmedium->hGlobal);
            pmedium->pstm = NULL;
        }
#endif //FOOLISH
        else if (gfWholeHog && pformatetcIn->cfFormat == OleClipFormat[OCF_LINKSRCDESCRIPTOR] )
        {
            DOUT(L"CXBag::GetData OCF_LINKSRCDESCRIPTOR on HGLOBAL\r\n");
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetcIn,
                                                    pmedium,
                                                    FALSE /* fHere */ );
        }
    }
    else if((pformatetcIn->tymed & TYMED_GDI) && (_pHost != NULL))
    {
        pmedium->tymed = TYMED_GDI;
        if ( pformatetcIn->cfFormat == CF_PALETTE )
        {
            DOUT(L"CXBag::GetData CF_PALETTE\r\n");
            if((pmedium->hGlobal = GetTransferPalette()) != NULL)
                hr = NOERROR;
        }
        else if ( pformatetcIn->cfFormat == CF_BITMAP )
        {
            DOUT(L"CXBag::GetData CF_BITMAP\r\n");
            if((pmedium->hGlobal = GetTransferBitmap()) != NULL)
                hr = NOERROR;
        }
    }
    else if(pformatetcIn->tymed & TYMED_MFPICT)
    {
        pmedium->tymed = TYMED_MFPICT;
        if ( pformatetcIn->cfFormat == CF_METAFILEPICT )
        {
            DOUT(L"CXBag::GetData CF_METAFILEPICT (on MFPICT)\r\n");
            if(_pHost != NULL)
            {
                if((pmedium->hGlobal = MetafilePictFromBmp(ghBitmapSnapshot, ghPaletteSnapshot)) != NULL)
                    hr = NOERROR;
            }
        }
    }
    SetTransferFlag(FALSE);
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::QueryGetData
//
//     OLE:     IDataObject
//
//  Synopsis:   Answer whether a request for this format might suceed
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::QueryGetData(LPFORMATETC pformatetc)
{
    DOUT(L"CXBag::QueryGetData\r\n");

    HRESULT hr = DV_E_FORMATETC;

    //
    // Pick out the formats we are willing to offer on IStorage
    //
    if ( (pformatetc->cfFormat == OleClipFormat[OCF_EMBEDSOURCE]) &&
         (pformatetc->dwAspect == DVASPECT_CONTENT) &&
         (pformatetc->tymed == TYMED_ISTORAGE) )
    {
        DOUT(L"CXBag::QueryGetData [IStorage]\r\n");
        hr = NOERROR;
    }
    //
    // Pick out the formats we are willing to offer on TYMED_ISTREAM
    //
    else if (gfWholeHog && (pformatetc->tymed == TYMED_ISTREAM) &&
                (pformatetc->cfFormat == OleClipFormat[OCF_LINKSOURCE]) )
    {
        DOUT(L"CXBag::QueryGetData [ISTREAM]\r\n");
        hr = NOERROR;
    }
    //
    // Pick out the formats we are willing to offer on TYMED_MFPICT
    //
    else if ( (pformatetc->tymed == TYMED_MFPICT) &&
                (pformatetc->cfFormat == CF_METAFILEPICT) )
    {
        DOUT(L"CXBag::QueryGetData [TYMED_MFPICT]\r\n");
        hr = NOERROR;
    }
    //
    // Pick out the formats we are willing to offer on TYMED_GDI
    //
    else if( (pformatetc->tymed & TYMED_GDI) &&
                (pformatetc->cfFormat == CF_PALETTE) ||
                (pformatetc->cfFormat == CF_BITMAP)
           )
    {
        DOUT(L"CXBag::QueryGetData [GDI]\r\n");
        hr = NOERROR;
    }
    //
    // Pick out the formats we are willing to offer on TYMED_HGLOBAL
    //
    else if (pformatetc->tymed == TYMED_HGLOBAL)
    {
        if( (pformatetc->cfFormat == OleClipFormat[OCF_OBJECTDESCRIPTOR]) ||
#ifdef FOOLISH
                (gfWholeHog && pformatetc->cfFormat == OleClipFormat[OCF_LINKSOURCE]) ||
#endif //FOOLISH
                (gfWholeHog && pformatetc->cfFormat == OleClipFormat[OCF_LINKSRCDESCRIPTOR])
          )
        {
            DOUT(L"CXBag::QueryGetData [HGLOBAL]\r\n");
            hr = NOERROR;
        }
    }

    return hr;
}



//+---------------------------------------------------------------
//
//  Member:     CXBag::SnapShotAndDetach
//
//  Synopsis:   Save a snapshot then detach from Host
//
//---------------------------------------------------------------
HRESULT
CXBag::SnapShotAndDetach(void)
{
    DOUT(L"CXBag::SnapShotAndDetach\r\n");

    STGMEDIUM medium;
    medium.pUnkForRelease = NULL;    // transfer ownership to caller
    medium.hGlobal = NULL;
    medium.tymed = TYMED_ISTORAGE;
    HRESULT hr = BagItInStorage(&medium, FALSE);
    Detach();
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::BagItInStorage, private
//
//  Synopsis:   Save or copy a snapshot into specified stg
//
//---------------------------------------------------------------
HRESULT
CXBag::BagItInStorage(LPSTGMEDIUM pmedium, BOOL fStgProvided)
{
    DOUT(L"CXBag::BagItInStorage\r\n");
    SIZEL sizel;

    if(_pHost != NULL)
    {
        SetTransferFlag(TRUE);
        //
        // Temporarily set our extent to the size of the pick rect...
        //
        _pHost->GetExtent(DVASPECT_CONTENT, &sizel);
        RECT rcPick;
        GetPickRect(&rcPick);
        SIZEL sizelPick;
        sizelPick.cx = HimetricFromHPix(rcPick.right - rcPick.left);
        sizelPick.cy = HimetricFromVPix(rcPick.bottom - rcPick.top);
        _pHost->SetExtent(DVASPECT_CONTENT, &sizelPick);
    }

    HRESULT hr = NOERROR;
    LPPERSISTSTORAGE pPStg = NULL;

    LPSTORAGE pStg = fStgProvided ? pmedium->pstg: NULL;
    if(!fStgProvided)
    {
        //
        // allocate a temp docfile that will delete on last release
        //
        hr = StgCreateDocfile(
            NULL,
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
            0,
            &pStg);
        pmedium->tymed = TYMED_ISTORAGE;
        pmedium->pstg = pStg;
    }

    if(pStg == NULL)
        goto LExit;

    if(_pStgBag == NULL)
    {
        if(_pHost == NULL)
        {
            hr = E_UNEXPECTED;  // We got prematurely detached!
            goto LExit;
        }
        _pHost->QueryInterface(IID_IPersistStorage, (LPVOID FAR*)&pPStg);
        Assert(pPStg != NULL);
        hr = OleSave(pPStg, pStg, FALSE /* fSameAsLoad */);
        pPStg->SaveCompleted(NULL);
    }
    else
    {
        _pStgBag->CopyTo(NULL, NULL, NULL, pStg);
    }

LExit:
    if(pPStg != NULL)
        pPStg->Release();

    //
    // Restore our real extent...
    //
    if(_pHost != NULL)
    {
        _pHost->SetExtent(DVASPECT_CONTENT, &sizel);
        SetTransferFlag(FALSE);
    }
    return hr;
}


//
// PBrush Server Object:
//=========================
//

PBHeader::PBHeader()
{
    //
    // These are just bogus defaults, call SetNativeExtents to
    // establish real values!
    //
    _sizel.cx =  3 * 1000L;
    _sizel.cy =  2 * 1000L;
    _dwNative = 0;
}

HRESULT
PBHeader::Read(LPSTREAM pStrm)
{
    return pStrm->Read(this, sizeof(PBHeader), NULL);
}

HRESULT
PBHeader::Write(LPSTREAM pStrm)
{
     return pStrm->Write(this, sizeof(PBHeader), NULL);
}


//+---------------------------------------------------------------
//
//  Member:     PBCtrl::ClassInit, public
//
//  Synopsis:   Initializes the PBCtrl class
//
//  Arguments:  [pClass] -- our class descriptor
//
//  Returns:    TRUE iff the class could be initialized successfully
//
//  Notes:      This method initializes the verb tables in the
//              class descriptor.
//
//---------------------------------------------------------------

    OLECHAR PBCtrl::gachEditVerb[MAX_VERBNAME_LEN] = L"Edit";
    OLECHAR PBCtrl::gachOpenVerb[MAX_VERBNAME_LEN] = L"Open";

BOOL
PBCtrl::ClassInit(LPCLASSDESCRIPTOR pClass)
{
    // These are our verb tables.  They are used by the base class
    // in implementing methods of the IOleObject interface.
    // NOTE: the verb table must be in ascending, consecutive order
    static OLEVERB OleVerbs[] =
    {
    //  { lVerb, lpszVerbName, fuFlags, grfAttribs },
        { OLEIVERB_PRIMARY, gachEditVerb, MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU },
        { 1, gachOpenVerb, MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU },
        { OLEIVERB_INPLACEACTIVATE, L"", 0, 0 },
        { OLEIVERB_UIACTIVATE, L"", 0, 0 },
        { OLEIVERB_HIDE, L"", 0, 0 },
        { OLEIVERB_OPEN, L"", 0, 0 },
        { OLEIVERB_SHOW, L"", 0, 0 },
    };
    LoadString(pClass->_hinst, IDSEdit, gachEditVerb, MAX_VERBNAME_LEN);
    LoadString(pClass->_hinst, IDSFileOpen, gachOpenVerb, MAX_VERBNAME_LEN);
    pClass->_pVerbTable = OleVerbs;
    pClass->_cVerbTable = sizeof(OleVerbs) / sizeof(OLEVERB);
    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::Create, public
//
//  Synopsis:   Creates and initializes an PBCtrl object
//
//  Arguments:  [pUnkOuter] -- A controlling unknown.  NULL if we are not
//                             being created as part of an aggregation
//              [pClass]    -- The OlePad class descriptor
//              [ppUnkCtrl] -- Where we return our controlling unknown
//              [ppObj]     -- Pointer to the PBCtrl object created
//
//  Returns:    Success if the object could be successfully created and
//              initialized.
//
//---------------------------------------------------------------

HRESULT
PBCtrl::Create( LPUNKNOWN pUnkOuter,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPPBCTRL FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    if(gpCtrlThis)
    {
        DOUT(L"PBCtrl::Create non-NULL gpCtrlThis!");
        return E_FAIL;
    }

    // create an object
    HRESULT hr = E_OUTOFMEMORY;
    LPPBCTRL pObj = new PBCtrl(pUnkOuter);
    if (pObj != NULL)
    {
        // initialize it
        if (OK(hr = pObj->Init(pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = gpCtrlThis = pObj;
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::PBCtrl, protected
//
//  Synopsis:   Constructor for the PBCtrl class
//
//  Arguments:  [pUnkOuter] -- the controlling unknown or NULL if we are
//                              not being created as part of an aggregate
//
//  Notes:      This is the first part of a two-stage construction process.
//              The second part is in the Init method.  Use the static
//              Create method to properly instantiate an PBCtrl object
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // `this' argument to base-member init. list.
PBCtrl::PBCtrl(LPUNKNOWN pUnkOuter):
        _PrivUnk(this)  // the controlling unknown holds a pointer to the object
{
    static SrvrCtrl::LPFNDOVERB VerbFuncs[] =
    {
        &SrvrCtrl::DoShow,            // Edit, OLEIVERB_PRIMARY
        &SrvrCtrl::DoOpen,            // Open
        &SrvrCtrl::DoInPlaceActivate, // OLEIVERB_INPLACEACTIVATE
        &SrvrCtrl::DoUIActivate,      // OLEIVERB_UIACTIVATE
        &SrvrCtrl::DoHide,            // OLEIVERB_HIDE
        &SrvrCtrl::DoOpen,            // OLEIVERB_OPEN
        &SrvrCtrl::DoShow,            // OLEIVERB_SHOW
    };

    _pUnkOuter = (pUnkOuter != NULL) ? pUnkOuter : (LPUNKNOWN)&_PrivUnk;

    _pDVCtrlUnk = NULL;
    _pIPCtrlUnk = NULL;

    _pVerbFuncs = VerbFuncs;
    EnableIPB(FALSE);
    _cLock = 0;
}
#pragma warning(default:4355)

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::Init, protected
//
//  Synopsis:   Initializes an PBCtrl object
//
//  Arguments:  [pClass] -- the class descriptor
//
//  Returns:    SUCCESS iff the class could be initialized
//
//  Notes:      This is the second part of a two-stage construction
//              process.  Use the static Create method to properly
//              instantiate an PBCtrl object.
//
//---------------------------------------------------------------

HRESULT
PBCtrl::Init(LPCLASSDESCRIPTOR pClass)
{
    HRESULT hr = NOERROR;

    if (OK(hr = SrvrCtrl::Init(pClass)))
    {
        LPPBDV pPBDV;
        LPUNKNOWN pDVCtrlUnk;
        if (OK(hr = PBDV::Create(this, pClass, &pDVCtrlUnk, &pPBDV)))
        {
            LPPBINPLACE pPBInPlace;
            LPUNKNOWN pIPCtrlUnk;
            if (OK(hr = PBInPlace::Create(this,
                                        pClass,
                                        &pIPCtrlUnk,
                                        &pPBInPlace)))
            {
                _pDVCtrlUnk = pDVCtrlUnk;
                _pDV = pPBDV;
                _pIPCtrlUnk = pIPCtrlUnk;
                _pInPlace = pPBInPlace;

                pDVCtrlUnk->AddRef();
                Lock();
            }
            pDVCtrlUnk->Release();  // on failure this will free DV subobject
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::~PBCtrl
//
//  Synopsis:   Destructor for the PBCtrl class
//
//---------------------------------------------------------------

PBCtrl::~PBCtrl(void)
{
    if(_state > OS_PASSIVE)
    {
        // make sure we are back to the loaded state
        Assert(OK(TransitionTo(OS_LOADED)));
    }
    gpCtrlThis = NULL;
    //
    // If we are not being shut down by the user (via FILE:EXIT etc.,)
    // then we must cause app shutdown here...
    //
    if(!gfUserClose && !gfTerminating)
        TerminateServer();

    //
    // release the controlling unknowns of our subobjects, which should
    // free them
    //
    if (_pDVCtrlUnk != NULL)
        Assert(_pDVCtrlUnk->Release() == 0);

    if (_pIPCtrlUnk != NULL)
        Assert(_pIPCtrlUnk->Release() == 0);
}


// the standard IUnknown methods all delegate to the controlling unknown.
IMPLEMENT_DELEGATING_IUNKNOWN(PBCtrl)

IMPLEMENT_PRIVATE_IUNKNOWN(PBCtrl)

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::PrivateUnknown::QueryInterface
//
//  Synopsis:   QueryInterface on our controlling unknown
//
//---------------------------------------------------------------

STDMETHODIMP
PBCtrl::PrivateUnknown::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
#ifdef DUMP_QI
#if DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer,
            L"PBCtrl::PrivateUnknown::QueryInterface (%lx)\r\n",
            riid.Data1);
    DOUT(achBuffer);
#endif
#endif // DUMP_QI

    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (void FAR *)this;
    }
    else if (IsEqualIID(riid,IID_IOleObject))
    {
        *ppv = (void FAR *) (LPOLEOBJECT)_pPBCtrl;
    }
    else    // try each of our delegate subobjects until one succeeds
    {
        HRESULT hr;
        if (!OK(hr = _pPBCtrl->_pDVCtrlUnk->QueryInterface(riid, ppv)))
            hr = _pPBCtrl->_pIPCtrlUnk->QueryInterface(riid, ppv);
        return hr;
    }

    //
    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    //
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     PBCtrl::GetMoniker
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      Our overide creates a file moniker for the Standalone case
//
//---------------------------------------------------------------

STDMETHODIMP
PBCtrl::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    DOUT(L"PBCtrl::GetMoniker\r\n");

    HRESULT hr = E_INVALIDARG;

    if (ppmk == NULL)
    {
        DOUT(L"PBCtrl::GetMoniker E_INVALIDARG\r\n");
        return hr;
    }
    *ppmk = NULL;   // set out parameters to NULL

    if(gfStandalone)
    {
        if(gpFileMoniker != NULL)
        {
            *ppmk = gpFileMoniker;
            gpFileMoniker->AddRef();
            hr = NOERROR;
        }
        else if((hr = CreateFileMoniker(gachLinkFilename, &gpFileMoniker)) == NOERROR)
        {
            gpFileMoniker->AddRef(); //because we are keeping this pointer
            *ppmk = gpFileMoniker;
            RegisterAsRunning((LPUNKNOWN)this, *ppmk, &gdwRegROT);
        }
        else
        {
            DOUT(L"PBCtrl::GetMoniker CreateFileMoniker FAILED!\r\n");
        }
    }
    else
    {
        return SrvrCtrl::GetMoniker(dwAssign, dwWhichMoniker, ppmk);
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::IsUpToDate
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      Return S_FALSE if we are dirty (not up-to-date)
//
//---------------------------------------------------------------

STDMETHODIMP
PBCtrl::IsUpToDate(void)
{
    DOUT(L"PBCtrl::IsUpToDate\r\n");

    return gfDirty ? S_FALSE : S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::GetHostNames, public
//
//  Synopsis:   Returns any host names set by the container
//
//  Arguments:  [plpstrCntrApp] -- location for string indicating the top-level
//                                  container application.
//              [plpstrCntrObj] -- location for string indicating the top-level
//                                  document.
//
//  Notes:      The method makes available the strings originally set by the
//              container using IOleObject::SetHostNames.  These strings are
//              used in the title bar of an open-edited object.  It is useful
//              to containers that need to forward this information to their
//              embeddings.  The returned strings are allocated using the
//              standard task allocator and should be freed appropriately by
//              the caller.
//
//---------------------------------------------------------------

void
PBCtrl::GetHostNames(LPTSTR FAR* plpstrCntrApp, LPTSTR FAR* plpstrCntrObj)
{
    if (_lpstrCntrApp != NULL)
    {
        TaskAllocString(_lpstrCntrApp, plpstrCntrApp);
    }
    else
    {
        *plpstrCntrApp = NULL;
    }

    if (_lpstrCntrObj != NULL)
    {
        TaskAllocString(_lpstrCntrObj, plpstrCntrObj);
    }
    else
    {
        *plpstrCntrObj = NULL;
    }
}

void
PBCtrl::Lock(void)
{
    ++_cLock;
#if DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer, L"\\\\PBCtrl::Lock (%d)\r\n", _cLock);
    DOUT(achBuffer);
#endif
    CoLockObjectExternal((LPUNKNOWN)&_PrivUnk, TRUE, TRUE);
}

void
PBCtrl::UnLock(void)
{
    --_cLock;
#if DBG
    OLECHAR achBuffer[256];
    wsprintf(achBuffer, L"\\\\PBCtrl::UnLock (%d)\r\n", _cLock);
    DOUT(achBuffer);
#endif
    Assert(_cLock >= 0);
    CoLockObjectExternal((LPUNKNOWN)&_PrivUnk, FALSE, TRUE);
}


HRESULT
PBCtrl::PassiveToLoaded()
{
    DOUT(L"=== PBCtrl::PassiveToLoaded\r\n");

    gfInvisible = FALSE;
    return SrvrCtrl::PassiveToLoaded();
}

HRESULT
PBCtrl::LoadedToPassive()
{
    DOUT(L"=== PBCtrl::LoadedToPassive\r\n");

    //
    // In the LocalServer (our) case, this transition will lead
    // to our being fully released and shut down. We get here only
    // when the container calls our (IOleObject) Close method.
    // We should already have saved our data by now...
    //

    gfInvisible = TRUE;
    HRESULT hr = SrvrCtrl::LoadedToPassive();
    UnLock();
    return hr;
}

HRESULT
PBCtrl::LoadedToRunning()
{
    LPPBINPLACE lpPbIp = (LPPBINPLACE)gpCtrlThis->_pInPlace;
    lpPbIp->SetWindowHandle(gpahwndApp[iFrame]);
    return SrvrCtrl::LoadedToRunning();
}

HRESULT
PBCtrl::RunningToLoaded()
{
    DOUT(L"=== PBCtrl::RunningToLoaded\r\n");

    FlushOleClipboard();
    return SrvrCtrl::RunningToLoaded();
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::RunningToOpened
//
//  Synopsis:   Effects the running to open state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
PBCtrl::RunningToOpened()
{
    DOUT(L"PBCtrl::RunningToOpened\r\n");

    HWND hwnd = gpahwndApp[iFrame];
    ShowWindow(hwnd, SW_SHOW | SW_SHOWNORMAL);
    if(IsIconic(hwnd))
        SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0L);
    SetForegroundWindow(hwnd);
    BringWindowToTop(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    DrawMenuBar(hwnd);

    OleNoteObjectVisible((LPUNKNOWN)&_PrivUnk, TRUE);
#ifdef SUPORT_DROP_TARGETTING
    RegisterAsDropTarget(gpahwndApp[iPaint]);
#endif

    //
    // notify our container so it can hatch-shade our object indicating
    // it is open-edited
    //
    if (_pClientSite != NULL)
        _pClientSite->OnShowWindow(TRUE);

    SetFocus(hwnd);
    //
    // BUGBUG: get the pick menu status updated corectly...
    //SendMessage(gpahwndApp[iTool], WM_SELECTTOOL, 1, 0L);
    //PostMessage(gpahwndApp[iPaint], WM_OUTLINE, 0, 0L);
    //
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::OpenedToRunning
//
//  Synopsis:   Effects the open to running state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
PBCtrl::OpenedToRunning()
{
    DOUT(L"PBCtrl::OpenedToRunning\r\n");

#ifdef SUPORT_DROP_TARGETTING
    RevokeOurDropTarget();
#endif
    HWND hwnd = gpahwndApp[iFrame];
    ShowWindow(hwnd, SW_HIDE);
    HRESULT hr = SrvrCtrl::OpenedToRunning();
    OleNoteObjectVisible((LPUNKNOWN)&_PrivUnk, FALSE);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::RunningToInPlace
//
//  Synopsis:   Effects the running to inplace-active state transition
//
//  Returns:    SUCCESS if the object results in the in-place state
//
//---------------------------------------------------------------

HRESULT
PBCtrl::RunningToInPlace(void)
{
    DOUT(L"PBCtrl::RunningToInPlace\r\n");

    HRESULT hr = SrvrCtrl::RunningToInPlace();
    return hr;
}

HRESULT
PBCtrl::InPlaceToUIActive(void)
{
    DOUT(L"PBCtrl::InPlaceToUIActive\r\n");

    HRESULT hr = SrvrCtrl::InPlaceToUIActive();
    InvalidateRect(_pInPlace->WindowHandle(), NULL, FALSE);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::UIActiveToInPlace
//
//  Synopsis:   Effects the U.I. active to inplace-active state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
PBCtrl::UIActiveToInPlace(void)
{
    DOUT(L"PBCtrl::UIActiveToInPlace\r\n");

    HRESULT hr = SrvrCtrl::UIActiveToInPlace();
    InvalidateRect(_pInPlace->WindowHandle(), NULL, FALSE);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBCtrl::InPlaceToRunning
//
//  Synopsis:   Effects the inplace-active to running state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
PBCtrl::InPlaceToRunning(void)
{
#ifdef DBG
    OLECHAR achTemp[256];
    wsprintf(achTemp,L"PBCtrl::InPlaceToRunning [%d]\r\n",(int)gfDirty);
    DOUT(achTemp);
#endif
    if(gfDirty)
    {
        _pDV->OnDataChange();
    }

    HRESULT hr = SrvrCtrl::InPlaceToRunning();
    LPPBINPLACE lpPbIp = (LPPBINPLACE)gpCtrlThis->_pInPlace;
    lpPbIp->SetWindowHandle(gpahwndApp[iFrame]);
    return hr;
}

    TCHAR szContentsStrm[] = L"contents";
    TCHAR szEmbeddingsStrm[] = L"embeddings";

//---------------------------------------------------------------
//
//  The Format Tables
//
//---------------------------------------------------------------

//
// GetData format information
// note: the LINKSRCDESCRIPTOR and OBJECTDESCRIPTOR are identical structures
//       so we use the OBJECTDESCRIPTOR get/set fns for both.
//

    static FORMATETC PBGetFormatEtc[] =
    {// { cfFormat, ptd, dwAspect, lindex, tymed },
        { CF_METAFILEPICT, NULL, DVASPECT_ALL, -1L, TYMED_MFPICT },
        { (CLIPFORMAT)(OCF_EMBEDDEDOBJECT + MAX_CF_VAL), NULL, DVASPECT_ALL, -1L, TYMED_ISTORAGE },
        { (CLIPFORMAT)(OCF_OBJECTDESCRIPTOR + MAX_CF_VAL), NULL, DVASPECT_ALL, -1L, TYMED_HGLOBAL }
    };

    static SrvrDV::LPFNGETDATA PBGetFormatFuncs[] =
    {
        &PBDV::GetMETAFILEPICT,
        &PBDV::GetEMBEDDEDOBJECT,
        &PBDV::GetOBJECTDESCRIPTOR
    };

//+---------------------------------------------------------------
//
//  Member:     PBDV::ClassInit (static)
//
//  Synopsis:   Initializes the PBDV class
//
//  Arguments:  [pClass] -- our class descriptor
//
//  Returns:    TRUE iff the class could be initialized successfully
//
//  Notes:      This method initializes the format tables in the
//              class descriptor.
//
//---------------------------------------------------------------

BOOL
PBDV::ClassInit(LPCLASSDESCRIPTOR pClass)
{
    //
    // fill in the class descriptor structure with the format tables
    //
    pClass->_pSetFmtTable = NULL;
    pClass->_cSetFmtTable = 0;

    //
    // walk our format tables and complete the cfFormat field from
    // the array of standard OLE clipboard formats.
    //
    LPFORMATETC pfe = PBGetFormatEtc;
    int c = sizeof(PBGetFormatEtc)/sizeof(FORMATETC);
    pClass->_cGetFmtTable = c;
    pClass->_pGetFmtTable = PBGetFormatEtc;

    //
    // The following loop is a tricky (read HACK-ish) way to get our indexes
    // into the OleClipFormat resolved after we have had a chance to
    // initialize the OleClipFormat array (via registering the string
    // names of the OLE formats via RegisterClipboardFormat()...
    //
    for (int i = 0; i < c; i++, pfe++)
    {
        int j = pfe->cfFormat;
        if(j > MAX_CF_VAL)
            pfe->cfFormat = OleClipFormat[j - MAX_CF_VAL];
    }

    pfe = g_aGetFmtEtcs;
    c = sizeof(g_aGetFmtEtcs)/sizeof(FORMATETC);
    for (i = 0; i < c; i++, pfe++)
    {
        int j = pfe->cfFormat;
        if(j > MAX_CF_VAL)
            pfe->cfFormat = OleClipFormat[j - MAX_CF_VAL];
    }

    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     PBDV::Create, public
//
//  Synopsis:   Creates and initializes an PBDV object
//
//  Arguments:  [pCtrl]     -- our control subobject
//              [pClass]    -- The class descriptor
//              [ppUnkCtrl] -- Where we return our controlling unknown
//              [ppObj]     -- where the created object is returned
//
//  Returns:    Success if the object could be successfully created and
//              initialized.
//
//---------------------------------------------------------------

HRESULT
PBDV::Create(LPPBCTRL pCtrl,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPPBDV FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    // create an object
    HRESULT hr;
    LPPBDV pObj;
    pObj = new PBDV((LPUNKNOWN)(LPOLEOBJECT)pCtrl);
    if (pObj == NULL)
    {
        hr =  E_OUTOFMEMORY;
    }
    else
    {
        // initialize it
        if (OK(hr = pObj->Init(pCtrl, pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = pObj;
        }
        else
        {
            pObj->_PrivUnk.Release();
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBDV::Create, public
//
//  Synopsis:   Creates and initializes an transfer PBDV object
//              from an existing PBDV object
//
//  Arguments:  [pPBDV]     -- object to duplicate in transfer object
//              [pClass]    -- The class descriptor
//              [ppUnkCtrl] -- Where we return our controlling unknown
//              [ppObj]     -- where the created object is returned
//
//  Returns:    Success if the object could be successfully created and
//              initialized.
//
//---------------------------------------------------------------

HRESULT
PBDV::Create(LPPBDV pPBDV,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPPBDV FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    // create an object
    HRESULT hr;
    LPPBDV pObj;
    pObj = new PBDV(NULL);
    if (pObj == NULL)
    {
        hr =  E_OUTOFMEMORY;
    }
    else
    {
        // initialize it
        if (OK(hr = pObj->Init(pPBDV, pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = pObj;
        }
        else
        {
            pObj->Release();
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBDV::PBDV, protected
//
//  Synopsis:   Constructor for the PBDV class
//
//  Arguments:  [pUnkOuter] -- the controlling unknown.  This is either
//                             a PBCtrl subobject or NULL we are being
//                             created as a transfer object.
//
//  Notes:      This is the first part of a two-stage construction process.
//              The second part is in the Init method.  Use the static
//              Create method to properly instantiate an PBDV object
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // `this' argument to base-member init. list.
PBDV::PBDV(LPUNKNOWN pUnkOuter):
    _PrivUnk(this)  // the controlling unknown holds a pointer to the object
{
    _pUnkOuter = (pUnkOuter != NULL) ? pUnkOuter : (LPUNKNOWN)&_PrivUnk;

    _pGetFuncs = PBGetFormatFuncs;
    _pSetFuncs = NULL;

    _sizel = _header._sizel;
}
#pragma warning(default:4355)

//+---------------------------------------------------------------
//
//  Member:     PBDV::Init, protected
//
//  Synopsis:   Initializes an PBDV object
//
//  Arguments:  [pCtrl]  -- our controlling PBCtrl subobject
//              [pClass] -- the class descriptor
//
//  Returns:    SUCCESS iff the class could be initialized
//
//  Notes:      This is the second part of a two-stage construction
//              process.  Use the static Create method to properly
//              instantiate an PBDV object.
//
//---------------------------------------------------------------

HRESULT
PBDV::Init(LPPBCTRL pCtrl, LPCLASSDESCRIPTOR pClass)
{
    HRESULT hr = SrvrDV::Init(pClass, pCtrl);
    //
    //do our own xtra init here...
    //
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBDV::Init, protected
//
//  Synopsis:   Initializes a transfer PBDV object from another PBDV object
//
//  Arguments:  [pDV]    -- the PBDV subobject to duplicate for transfer purposes
//              [pClass] -- the class descriptor
//
//  Returns:    SUCCESS iff the class could be initialized
//
//  Notes:      This is the second part of a two-stage construction
//              process.  Use the static Create method to properly
//              instantiate an PBDV object.
//
//---------------------------------------------------------------

HRESULT
PBDV::Init(LPPBDV pDV, LPCLASSDESCRIPTOR pClass)
{
    // The base class will duplicate it's member variables and do
    // a "Save-Copy-As" operation into a new IStorage pointed to by _pStg.
    HRESULT hr;
    if (OK(hr = SrvrDV::Init(pClass, pDV)))
    {
        // OK, now initialize our derived members:
        LPSTREAM pStrm;
        hr = _pStg->OpenStream(szContentsStrm, NULL, STGM_SALL,0, &pStrm);
        if (OK(hr))
        {
            hr = _header.Read(pStrm);
            if (OK(hr))
            {
                _sizel = _header._sizel;
            }
            pStrm->Release();
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     PBDV::~PBDV
//
//  Synopsis:   Destructor for the PBDV class
//
//---------------------------------------------------------------

PBDV::~PBDV(void)
{
}

// the standard IUnknown methods all delegate to a controlling unknown.
IMPLEMENT_DELEGATING_IUNKNOWN(PBDV)

IMPLEMENT_PRIVATE_IUNKNOWN(PBDV)

//+---------------------------------------------------------------
//
//  Member:     PBDV::PrivateUnknown::QueryInterface
//
//  Synopsis:   QueryInterface on our controlling unknown
//
//---------------------------------------------------------------

STDMETHODIMP
PBDV::PrivateUnknown::QueryInterface (REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IDataObject))
    {
        *ppv = (LPVOID)(LPDATAOBJECT)_pPBDV;
    }
    else if (IsEqualIID(riid,IID_IViewObject))
    {
        *ppv = (LPVOID)(LPVIEWOBJECT)_pPBDV;
    }
    else if (IsEqualIID(riid,IID_IPersist))
    {
        //
        // The standard handler wants this
        //
        *ppv = (LPVOID)(LPPERSIST)(LPPERSISTSTORAGE)_pPBDV;
    }
    else if (IsEqualIID(riid,IID_IPersistStorage))
    {
        *ppv = (LPVOID)(LPPERSISTSTORAGE)_pPBDV;
    }
    else if (IsEqualIID(riid,IID_IPersistFile))
    {
        *ppv = (LPVOID)(LPPERSISTFILE)_pPBDV;
    }
    else
    {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    //
    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    //
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     PBDV::RenderContent, public
//
//  Synopsis:   Draws our contents into a display context
//
//  Arguments:  The arguments are the same as to the IViewObject::Draw method
//
//  Returns:    SUCCESS if the content was rendered
//
//  Notes:      This virtual method of the base class is called to implement
//              IViewObject::Draw for the DVASPECT_CONTENT aspect.
//
//----------------------------------------------------------------

HRESULT
PBDV::RenderContent(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue) (DWORD),
        DWORD dwContinue)
{
    DOUT(L">>>PBrush: RenderContent\r\n");

    //
    // Call the PBrush C code to get a rendering of this picture...
    //
    RenderPicture(hdcDraw, lprectl);

    return NOERROR;
}


HRESULT
PBDV::GetClipboardCopy(LPSRVRDV FAR* ppDV)
{
    *ppDV = NULL;
    return E_FAIL;
}

//+---------------------------------------------------------------
//
//  Member:     LoadFromStorage
//
//  Synopsis:   Loads our data from a storage
//
//  Arguments:  [pStg] -- storage to load from
//
//  Returns:    SUCCESS if our native data was loaded
//
//  Notes:      This is a virtual method in our base class that is called
//              on an IPersistStorage::Load and an IPersistFile::Load when
//              the file is a docfile.  We override this method to
//              do our server-specific loading.
//
//----------------------------------------------------------------

HRESULT
PBDV::LoadFromStorage(LPSTORAGE pStg)
{
    DOUT(L"..PBDV::LoadFromStorage\n\r");

    LPSTREAM pStrm;
    HRESULT hr = pStg->OpenStream(szContentsStrm, NULL, STGM_SALL,0, &pStrm);
    if (OK(hr))
    {
        // Read the whole contents stream into memory and use that as a stream
        // to do our bits and pieces reads.
        pStrm = ConvertToMemoryStream(pStrm);

        hr = _header.Read(pStrm);
        if (OK(hr))
        {
            _sizel = _header._sizel;
            if(_header._dwNative > 0)
            {
                //
                //TODO: this code needs to be optimized.
                //
                LPBYTE lpbData = NULL;
                HANDLE hData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT,
                                            _header._dwNative);
                if(hData != NULL)
                {
                    if ((lpbData = (LPBYTE)GlobalLock(hData)) != NULL)
                    {
                        hr = pStrm->Read(lpbData, _header._dwNative, NULL);
                        PutNativeData(lpbData, _pCtrl->_pInPlace->WindowHandle());
                        GlobalUnlock(hData);
                        GlobalFree(hData);
                    }
                }
            }
            else
            {
                DOUT(L"!!PBDV::LoadFromStorage (no native data)\n\r");
            }
        }
        else
        {
            DOUT(L"!!PBDV::LoadFromStorage header read failed\n\r");
        }
        pStrm->Release();
    }
    else
    {
        DOUT(L"!!PBDV::LoadFromStorage Failure opening contents stream\n\r");
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SaveToStorage
//
//  Synopsis:   Saves our data to a storage
//
//  Arguments:  [pStg]        -- storage to save to
//              [fSameAsLoad] -- flag indicating whether this is the same
//                               storage that we originally loaded from
//
//  Returns:    SUCCESS if our native data was saved
//
//  Notes:      This is a virtual method in our base class that is called
//              on an IPersistStorage::Save and IPersistFile::Save when the
//              file is a docfile.  We override this method to
//              do our server-specific saving.
//
//----------------------------------------------------------------

HRESULT
PBDV::SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    DOUT(L"..PBDV::SaveToStorage\n\r");

    //
    // open or create the native data stream and write our native data...
    //
    LPSTREAM pStrm;
    HRESULT hr = pStg->CreateStream(szContentsStrm,
            STGM_SALL | STGM_CREATE,
            0L,
            0L,
            &pStrm);

    if (OK(hr))
    {
        //
        // update the members that may have changed...
        //
        _header._sizel = _sizel;
        HANDLE hNative = GetNativeData();
        _header._dwNative = GlobalSize(hNative);
        hr = _header.Write(pStrm);

        if (OK(hr))
        {
            if(_header._dwNative > 0)
            {
                LPBYTE pNative = (LPBYTE)GlobalLock(hNative);
                if(pNative != NULL)
                {
                    hr = pStrm->Write(pNative, _header._dwNative, NULL);
                    GlobalUnlock(hNative);
                }
            }
            else
            {
                DOUT(L"!!PBDV::SaveToStorage (no native data)\n\r");
            }
        }
        else
        {
            DOUT(L"!!PBDV::SaveToStorage Failure writing header\n\r");
        }
        GlobalFree(hNative);
        pStrm->Release();
    }
    else
    {
        DOUT(L"!!PBDV::SaveToStorage Failure creating contents stream\n\r");
    }
    return hr;
}
