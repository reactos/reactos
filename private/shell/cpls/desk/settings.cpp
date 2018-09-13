/**************************************************************************\
* Module Name: settings.cpp
*
* Contains Implementation of the CDeviceSettings class who is in charge of
* the settings of a single display. This is the data base class who does the
* real change display settings work
*
* Copyright (c) Microsoft Corp.  1992-1996 All Rights Reserved
*
* NOTES:
*
* History: Created by dli on July 17, 1997
*
\**************************************************************************/


#include "precomp.h"
#include "setcdcl.hxx"
#include "device.hxx"
#include "settings.hxx"

#define ISPRIMARY(pDevice)       (pDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)

TCHAR gpszError[] = TEXT("Unknown Error");
static const TCHAR sc_szDisplayDevice[] = TEXT("Display Device"); // "\DisplayX"
static const TCHAR sc_szDisplayName[] = TEXT("Display Name");     // " ATI Mach64 Turbo 3 " 
static const TCHAR sc_szMonitorDevice[] = TEXT("Monitor Device"); // " \DisplayX\MonitorX "
static const TCHAR sc_szMonitorName[] = TEXT("Monitor Name");     // " NEC Multi-sync II "

UINT g_cfDisplayDevice = 0;
UINT g_cfDisplayName = 0;
UINT g_cfMonitorDevice = 0;
UINT g_cfMonitorName = 0;
#ifdef WINNT
extern ULONG gUnattenedBitsPerPel;
extern ULONG gUnattenedXResolution;
extern ULONG gUnattenedYResolution;
extern ULONG gUnattenedVRefresh;
#endif

void InitClipboardFormats()
{
    if (g_cfDisplayDevice == 0)
        g_cfDisplayDevice = RegisterClipboardFormat(sc_szDisplayDevice);
    if (g_cfDisplayName == 0)
        g_cfDisplayName = RegisterClipboardFormat(sc_szDisplayName);
    if (g_cfMonitorDevice == 0)
        g_cfMonitorDevice = RegisterClipboardFormat(sc_szMonitorDevice);
    if (g_cfMonitorName == 0)
        g_cfMonitorName = RegisterClipboardFormat(sc_szMonitorName);
}

HRESULT CDeviceSettings::QueryInterface(REFIID riid, LPVOID * ppvObj)
{ 
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);
    
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_IDataObject))
        *ppvObj = SAFECAST(this, IDataObject*);
    else
        return E_NOINTERFACE;  // Otherwise, don't delegate to HTMLObj!!
    
    AddRef();
    return S_OK;
}


ULONG CDeviceSettings::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CDeviceSettings::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


BOOL CDeviceSettings::GetMonitorName(LPTSTR pszName, LPTSTR pszDevice)
{
    DISPLAY_DEVICE ddTmp;

    ZeroMemory(&ddTmp, sizeof(ddTmp));
    ddTmp.cb = sizeof(DISPLAY_DEVICE);

    if (EnumDisplayDevices(_DisplayDevice.DeviceName, 0, &ddTmp, 0))
    {
        if (pszName)
            lstrcpy(pszName, (LPTSTR)ddTmp.DeviceString);

        if (pszDevice)
            lstrcpy(pszDevice, (LPTSTR)ddTmp.DeviceName);

        return TRUE;
    }

    return FALSE;
}

STDMETHODIMP CDeviceSettings::TransferDisplayName(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hres = DV_E_TYMED;
    if (pfmtetc->tymed & TYMED_HGLOBAL)
    {
        int cch;
        LPTSTR pszOut = NULL;
        TCHAR szMonitorName[130];
        TCHAR szMonitorDevice[40];
        if ((pfmtetc->cfFormat == g_cfMonitorDevice) || (pfmtetc->cfFormat == g_cfMonitorName))
        {
            GetMonitorName(szMonitorName, szMonitorDevice);
            if (pfmtetc->cfFormat == g_cfMonitorName)
                pszOut = szMonitorName;
            else
                pszOut = szMonitorDevice;
        }
        else if (pfmtetc->cfFormat == g_cfDisplayDevice)
        {
            pszOut = (LPTSTR)_DisplayDevice.DeviceName;
        }
        else
        {
            ASSERT(pfmtetc->cfFormat == g_cfDisplayName);
            pszOut = (LPTSTR)_DisplayDevice.DeviceString;
        }
        cch = lstrlen(pszOut) + 1;
        LPWSTR pwszDevice = (LPWSTR)GlobalAlloc(GPTR, cch * SIZEOF(WCHAR));
        if (pwszDevice)
        {
            int cchConverted = 0;
            // We always return UNICODE string
#ifdef UNICODE
            lstrcpy(pwszDevice, pszOut);
#else       
            cchConverted = MultiByteToWideChar(CP_ACP, 0, pszOut , -1, pwszDevice, cch);
            ASSERT(cchConverted == cch);
#endif
            pstgmed->tymed = TYMED_HGLOBAL;
            pstgmed->hGlobal = pwszDevice;
            hres = S_OK;
        }
        else
            hres = E_OUTOFMEMORY;
    }

    return hres;
}

BOOL CDeviceSettings::_IsDataFormatSupported(CLIPFORMAT cfFormat)
{
    if ((cfFormat == g_cfDisplayDevice)
        || (cfFormat == g_cfDisplayName)
        || (cfFormat == g_cfMonitorDevice)
        || (cfFormat == g_cfMonitorName))
        return TRUE;    
    return FALSE;
}

STDMETHODIMP CDeviceSettings::GetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed)
{
    HRESULT hr;

    ASSERT(this);
    ASSERT(pfmtetc);
    ASSERT(pstgmed);

    // Ignore pfmtetc.ptd.  All supported data formats are device-independent.

    ZeroMemory(pstgmed, SIZEOF(*pstgmed));

    if (pfmtetc->dwAspect == DVASPECT_CONTENT)
    {
        if (_IsDataFormatSupported(pfmtetc->cfFormat))
            hr = (pfmtetc->lindex == -1) ? TransferDisplayName(pfmtetc, pstgmed) : DV_E_LINDEX;
        else
            hr = DV_E_FORMATETC;
    }
    else
        hr = DV_E_DVASPECT;

    return(hr);
}


STDMETHODIMP CDeviceSettings::GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed)
{
    ZeroMemory(pfmtetc, SIZEOF(pfmtetc));
    return E_NOTIMPL;
}


STDMETHODIMP CDeviceSettings::QueryGetData(FORMATETC *pfmtetc)
{
    HRESULT hr;

    ASSERT(pfmtetc);
    // Ignore pfmtetc.ptd.  All supported data formats are device-independent.

    if (pfmtetc->dwAspect == DVASPECT_CONTENT)
    {
        if (IsFlagSet(pfmtetc->tymed, TYMED_HGLOBAL))
        {
            if (_IsDataFormatSupported(pfmtetc->cfFormat))
                hr = (pfmtetc->lindex == -1) ? S_OK : DV_E_LINDEX;
            else
                hr = DV_E_FORMATETC;
        }
        else
            hr = DV_E_TYMED;
    }
    else
        hr = DV_E_DVASPECT;

    return(hr);
}

STDMETHODIMP CDeviceSettings::GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut)
{
    HRESULT hr;
    ASSERT(pfmtetcIn);
    ASSERT(pfmtetcOut);

    hr = QueryGetData(pfmtetcIn);

    if (hr == S_OK)
    {
        *pfmtetcOut = *pfmtetcIn;

        if (pfmtetcIn->ptd == NULL)
            hr = DATA_S_SAMEFORMATETC;
        else
        {
            pfmtetcIn->ptd = NULL;
            ASSERT(hr == S_OK);
        }
    }
    else
        ZeroMemory(pfmtetcOut, SIZEOF(*pfmtetcOut));
    return(hr);
}


STDMETHODIMP CDeviceSettings::SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDeviceSettings::EnumFormatEtc(DWORD dwDirFlags, IEnumFORMATETC ** ppiefe)
{
    HRESULT hr;

    ASSERT(ppiefe);
    *ppiefe = NULL;
    if (dwDirFlags == DATADIR_GET)
    {
        FORMATETC rgfmtetc[] =
        {
            { g_cfDisplayDevice,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { g_cfDisplayName,   NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { g_cfMonitorDevice,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            { g_cfMonitorName,   NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };

        hr = SHCreateStdEnumFmtEtc(ARRAYSIZE(rgfmtetc), rgfmtetc, ppiefe);
    }
    else
        hr = E_NOTIMPL;

    return(hr);
}

STDMETHODIMP CDeviceSettings::DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, DWORD * pdwConnection)
{
    ASSERT(pfmtetc);
    ASSERT(pdwConnection);

    *pdwConnection = 0;
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CDeviceSettings::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}



STDMETHODIMP CDeviceSettings::EnumDAdvise(IEnumSTATDATA ** ppiesd)
{
    ASSERT(ppiesd);
    *ppiesd = NULL;
    return OLE_E_ADVISENOTSUPPORTED;
}

// If any attached device is at 640x480, we want to force small font
BOOL CDeviceSettings::IsSmallFontNecessary()
{
    if (_fOrgAttached || _fAttached)
    {
        int cx, cy;
        _pcdmlModes->ResXYFromIndex(_iResolution, &cx, &cy);

        //
        // Force Small fonts at 640x480
        //
        if (cx < 800 || cy < 600)
            return TRUE;
    }
    return FALSE;
}

// This function turns a device on only in the CPL
void CDeviceSettings::SetAttached(BOOL fAttached)
{
    if (fAttached != _fAttached)
    {
        // This device is just turned on, set the Org values because Org values
        // means values we first got. 
        if (fAttached == TRUE)
        {
            _iOrgResolution = _iResolution;
            _iOrgColor      = _iColor;
            _iOrgFrequency  = _iFrequency;
            this->GetCurPosition(&_rcOrigPos);
        }
        else
        {
            _iOrgResolution = -1;
            _iOrgColor      = -1;
            _iOrgFrequency  = -1;
            SetRectEmpty(&_rcOrigPos);
        }
    }
    _fAttached = fAttached;
}

// Switch this device to be the active one.
void CDeviceSettings::SetActive(BOOL fActive)
{
    _fActive = fActive;
    _UpdateSettingsUI();
}

void CDeviceSettings::SetCurPosition(LPPOINT ppt)
{
    if (ppt)
        _ptPos = *ppt;
}

void CDeviceSettings::GetCurPosition(LPRECT prcPos)
{
    if (prcPos)
    {
        int cx,cy;
        prcPos->left = _ptPos.x;
        prcPos->top  = _ptPos.y;
        _pcdmlModes->ResXYFromIndex(_iResolution, &cx, &cy);
        prcPos->right = prcPos->left + cx;
        prcPos->bottom = prcPos->top + cy;
    }
}

void CDeviceSettings::GetOrigPosition(LPRECT prcPos)
{
    if (prcPos)
        *prcPos = _rcOrigPos;
}

void CDeviceSettings::_UpdateSettingsUI()
{
    // Only update the ui if I am active
    if (_fActive)
    {
        // Set the correct locations for the color and resolution
        PopulateColorList();
        PopulateScreenSizeList();
#ifdef REF_ON_MAIN_PAGE
        PopulateFrequencyList();
#endif
    }
}

void CDeviceSettings::ChangeColor(int iClr)
{
    if (iClr == _iColor)
        return;
    
    int iFreq, iRes;
    //
    // Realize the new monitor mode
    //

    iFreq = _iFrequency;
    iRes = _iResolution;

    _pcdmlModes->FindClosestMode(&iRes, iClr, &iFreq);

    this->SetCurResolution(iRes);
    this->SetCurFrequency(iFreq);
    this->SetCurColor(iClr);
}


// Get the current color depths in the CPL
int CDeviceSettings::GetColorBits()
{
#define MAX_COLOR_DEPTH 64
    return _fAttached ? _pcdmlModes->ColorFromIndex(_iColor) : MAX_COLOR_DEPTH;
#undef MAX_COLOR_DEPTH
}

void CDeviceSettings::ChangeResolution(int iRes)
{
    if (iRes == _iResolution)
        return;
    
    int iFreq, iClr;
    //
    // Realize the new monitor mode
    //
    
    iFreq = _iFrequency;
    iClr = _iColor;
    
    _pcdmlModes->FindClosestMode(iRes, &iClr, &iFreq);
    
    this->SetCurFrequency(iFreq);
    this->SetCurColor(iClr);
    this->SetCurResolution(iRes);
}

PMONPARAM CDeviceSettings::GetMonitorParams()
{
    //
    // Build a list of valid refesh rates for this Res and Color depth
    //
    PMONPARAM pMonParams = (PMONPARAM)LocalAlloc( LPTR, _pcdmlModes->GetFreqCount() * sizeof(MONPARAM) );

    if (pMonParams)
    {
        int i;
        _iRate = 0;
        for( i = 0; i < _pcdmlModes->GetFreqCount(); i++ ) {

            if ( i == _iFrequency )
                pMonParams->iCurRate = _iRate;

            if (_pcdmlModes->LookUp(_iResolution, _iColor, i)) {
                pMonParams->aRates[_iRate++] = _pcdmlModes->FreqFromIndex(i);

            }
        }
        pMonParams->cRates = _iRate;

        // Remember current selection
        _iRate = pMonParams->iCurRate;
    }

    return pMonParams;
}

void CDeviceSettings::UpdateRefreshRate(PMONPARAM pMonParams)
{
    int iFreq, iClr, iRes;
    //
    // Check for new refresh rate and if it changed, realize the new monitor mode
    //
    if (_iRate != pMonParams->iCurRate ) {


        // Convert subdialog index into this dialog index
        iFreq = _pcdmlModes->IndexFromFreq( pMonParams->aRates[pMonParams->iCurRate] );

        iClr = _iColor;
        iRes = _iResolution;

        // BUGBUG - this should never change iRes or iClr since we
        // paired down the list before we sent it off.  But for now
        // for safety, we'll leave in the call.
        //
        _pcdmlModes->FindClosestMode(&iRes, &iClr, iFreq);

        SetCurResolution(iRes);
        SetCurColor(iClr);
        // END OF BUGBUG (code that should be able to be removed safely)

        // Set the new frequency
        SetCurFrequency(iFreq);
    }

    //BUGBUG: (dli) this is kind of ugly. 
    LocalFree(pMonParams);
}


// Constructor for CDeviceSettings
//
//  (gets called when ever a CDeviceSettings object is created)
//

CDeviceSettings::CDeviceSettings(HWND hwnd) : _cResolutions(0), _pszDisplay(NULL), _pcdmlModes(NULL),
        _hwndMain(hwnd), _cRef(1)
{
    ASSERT(_fAttached == FALSE);
    ASSERT(_fOrgAttached == FALSE);
    ASSERT(_fBadDriver == FALSE);
    ASSERT(_fUsingDefault == FALSE);
    ASSERT(_fNewDriver == FALSE);
    ASSERT(_fPrimary == FALSE);
    ASSERT(_fActive == FALSE);
}

BOOL CDeviceSettings::InitSettings(PDISPLAY_DEVICE pdd)
{
    BOOL bRet = FALSE;
    ASSERT(pdd);
    if (pdd)
    {
        _DisplayDevice = *pdd;

        //
        // Save the name of this device for later use.
        //

        if (_DisplayDevice.DeviceName[0])
        {
            _pszDisplay = CloneString((LPTSTR)&(_DisplayDevice.DeviceName[0]));
        }

        //
        // new matrix storage
        //
        _pcdmlModes = new CDEVMODELST;

        if (_pcdmlModes)
        {
            
            bRet = this->RefreshSettings(TRUE);

            if (bRet)
            {
                _fPrimary = ISPRIMARY(_DisplayDevice);
                WarnMsg(_pszDisplay, TEXT(" Created and Initializd successfully"));
            }
            else
            {
                WarnMsg(_pszDisplay, TEXT("Refresh Settings Failed!!!"));
                _fBadDriver = TRUE;
            }
        }
    }

    return bRet;
}

//
// Destructor
//
CDeviceSettings::~CDeviceSettings() {
    WarnMsg(TEXT("Destructing"), _pszDisplay);
    if (_pszDisplay != NULL)
        LocalFree(_pszDisplay);

    if (_pcdmlModes != NULL) {

        delete _pcdmlModes;
        _pcdmlModes = NULL;

    }
}


void CDeviceSettings::PopulateScreenSizeList()
{
    //
    // Tell the controls what their valid values are.
    //
    int cResolutions = _pcdmlModes->GetResCount();
    this->SendCtlMsg(IDC_SCREENSIZE, TBM_SETRANGE, TRUE, MAKELONG(0, cResolutions - 1));
    this->SetCurResolution(_iResolution);
////EnableWindow(GetDlgItem(_hwndMain, IDC_SCREENSIZE), _fOrgAttached);
}

void CDeviceSettings::PopulateColorList()
{
    int i;
    this->SendCtlMsg(IDC_COLORBOX, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < _pcdmlModes->GetClrCount(); i++) {

        int cBits;
        LPTSTR lpszColor;

        //
        // convert bit count to number of colors and make it a string
        //

        cBits = _pcdmlModes->ColorFromIndex(i);

	switch (cBits)
    {
        case 32: lpszColor = FmtSprint(ID_DSP_TXT_TRUECOLOR32); break;
        case 24: lpszColor = FmtSprint(ID_DSP_TXT_TRUECOLOR24); break;
        case 16: lpszColor = FmtSprint(ID_DSP_TXT_16BIT_COLOR); break;
        case 15: lpszColor = FmtSprint(ID_DSP_TXT_15BIT_COLOR); break;
        default:
            lpszColor = FmtSprint(ID_DSP_TXT_COLOR, (1 << cBits));
    }

    this->SendCtlMsg(IDC_COLORBOX, CB_INSERTSTRING, i, (LPARAM)lpszColor);

    LocalFree(lpszColor);
    }

    this->SetCurColor(_iColor);

////EnableWindow(GetDlgItem(_hwndMain, IDC_COLORBOX), _fOrgAttached);
}


void CDeviceSettings::PopulateFrequencyList()
{
    int i;
    this->SendCtlMsg(ID_DSP_FREQ, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < _pcdmlModes->GetFreqCount(); i++) {

        LPTSTR lpszFreq;
        int cHz = _pcdmlModes->FreqFromIndex(i);

        //
        // convert bit count to number of colors and make it a string.
        //

        if ((cHz == 0) ||
            (cHz == 1) ) {

            lpszFreq = FmtSprint(ID_DSP_TXT_DEFFREQ);

        } else if (cHz < 50) {

            lpszFreq = FmtSprint(ID_DSP_TXT_INTERLACED, cHz);


        } else {

            lpszFreq = FmtSprint(ID_DSP_TXT_FREQ, cHz);
        }

        this->SendCtlMsg(ID_DSP_FREQ, CB_INSERTSTRING, i, (LPARAM)lpszFreq);

        LocalFree(lpszFreq);
    }

    this->SetCurFrequency(_iFrequency);
}


void CDeviceSettings::_IndexesFromDevMode(PDEVMODE pdm, int * piRes, int * piClr, int * piFreq)
{
    *piRes = _pcdmlModes->IndexFromResXY(pdm->dmPelsWidth, pdm->dmPelsHeight);
    *piClr = _pcdmlModes->IndexFromColor(pdm->dmBitsPerPel);
    *piFreq = _pcdmlModes->IndexFromFreq(pdm->dmDisplayFrequency);
}

void CDeviceSettings::SetMode(int w, int h, int bpp)
{
    int iRes  = _pcdmlModes->IndexFromResXY(w,h);
    int iClr  = _pcdmlModes->IndexFromColor(bpp);
    SetCurResolution(iRes);
    SetCurColor(iClr);
}

//
// RefreshSettings -- Reenumerate the settings, and build the mode list when bComplete == TRUE
//

BOOL CDeviceSettings::RefreshSettings(BOOL bComplete)
{
    DEVMODE defaultdm = {0};
    PCDEVMODE pcdev = NULL;
    int iResOk, iClrOk, iFreqOk;
    int iRes, iClr, iFreq;
    BOOL bCurrent = FALSE;
    BOOL bRegistry = FALSE;
    BOOL bModeAdded = FALSE;
    
    ZeroMemory(&defaultdm, sizeof(DEVMODE));
    defaultdm.dmSize = sizeof(DEVMODE);
    bCurrent = EnumDisplaySettingsEx(_pszDisplay, ENUM_CURRENT_SETTINGS, &defaultdm, 0);

    //
    // in VGA fallback or SafeMode EnumDisplaySettings will fail
    // deal with this case
    //
    if (!bCurrent && _pszDisplay == NULL)
    {
        ZeroMemory(&defaultdm, sizeof(DEVMODE));
        defaultdm.dmSize = sizeof(DEVMODE);

        HDC hdc = GetDC(NULL);
        defaultdm.dmPelsWidth  = GetDeviceCaps(hdc, HORZRES);
        defaultdm.dmPelsHeight = GetDeviceCaps(hdc, VERTRES);
        defaultdm.dmBitsPerPel = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
        defaultdm.dmFields = DM_POSITION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        ReleaseDC(NULL, hdc);
        bCurrent = TRUE;
    }

    //
    // get the mode from the registry if this device does not have a current mode
    //
    if (!bCurrent)
    {
        ZeroMemory(&defaultdm, sizeof(DEVMODE));
        defaultdm.dmSize = sizeof(DEVMODE);
        bRegistry = EnumDisplaySettingsEx(_pszDisplay, ENUM_REGISTRY_SETTINGS, &defaultdm, 0);
    }
    
    //
    // Don't rebuild the mode list if we are just responding to the 
    // display changes message
    //

    if (bComplete)
    {
        //
        // Clear cached values for selected modes.
        // These are reset at the end of the routine before exiting.
        //

        _iColor = -1;
        _iFrequency = -1;
        _iResolution = -1;

        //
        // Clear initial mode value.
        // Setting all these values to -1 means the user has to save on OK.
        //

        _iOrgResolution = -1;
        _iOrgColor = -1;
        _iOrgFrequency = -1;

        // Clear out old values
        //

        if (_pcdmlModes != NULL) {

            delete _pcdmlModes;
            _pcdmlModes = NULL;

        }

        //
        // new matrix storage
        //
        _pcdmlModes = new CDEVMODELST;

        //
        // Try to build the list of mode.
        //
        if (_pcdmlModes->BuildList(_pszDisplay, _hwndMain) == FALSE)
        {
            //
            // At least add the current settings.
            //
            // we always want at least the current mode in the list
            // the user is running in this mode so it must be valid.
            //
            // this can happen when the user changes the monitor type
            // the current display mode can be come invalid (over
            // the maximal scan rate) but the mode is still running
            // and we dont want to consider it invalid.
            //
            if (bCurrent)
            {
                DEVMODE *pdm = (DEVMODE*)malloc(defaultdm.dmSize);
                CopyMemory(pdm, &defaultdm, defaultdm.dmSize);
                _pcdmlModes->AddDevMode(pdm);
                _pcdmlModes->ReconstructModeMatrix();
            }
            else
            {
                return FALSE;
            }
        }
    }
    
    //
    // Initialize the resolution, color and frequency settings. 
    //

    iResOk  = 0;
    iClrOk  = 0;
    iFreqOk = 0;

    // Is this device attached to the desktop ?
    _fAttached = BOOLIFY(defaultdm.dmFields & DM_POSITION);
    if (_fAttached && ((defaultdm.dmPelsWidth == 0) || (defaultdm.dmPelsHeight == 0)))
        _fAttached = FALSE;
    _fOrgAttached = _fAttached;

    // Set the initial position
    _ptPos.x = defaultdm.dmPosition.x;
    _ptPos.y = defaultdm.dmPosition.y;

    _IndexesFromDevMode(&defaultdm, &iRes, &iClr, &iFreq);

    // If we have a current mode, which is not in the mode list, add it
    //
    // we always want the current mode in the list
    // the user is running in this mode so it must be valid.
    //
    // this can happen when the user changes the monitor type
    // the current display mode can be come invalid (over
    // the maximal scan rate) but the mode is still running
    // and we dont want to consider it invalid.
    //
    if (bCurrent && (iRes == -1 || iClr == -1))
    {
        WarnMsg(_pszDisplay, TEXT("!!!!! -- Default mode does not exist in the mode list"));

        DEVMODE *pdm = (DEVMODE*)malloc(defaultdm.dmSize);
        CopyMemory(pdm, &defaultdm, defaultdm.dmSize);
        _pcdmlModes->AddDevMode(pdm);
        _pcdmlModes->ReconstructModeMatrix();
        bModeAdded = TRUE;
        _IndexesFromDevMode(&defaultdm, &iRes, &iClr, &iFreq);
        ASSERT(iRes != -1);
        ASSERT(iClr != -1);
    }

    if (pcdev = _pcdmlModes->LookUp(iRes, iClr, iFreq))
    {
        iResOk  = iRes;
        iClrOk  = iClr;
        iFreqOk = iFreq;

        //
        // For the active display, set the original values to the current
        // values. These might be invalid values, such as -1, in the
        // case of setup or a bad_mode in the registry ...
        //

        if (bComplete && _fOrgAttached)
        {
            pcdev->vTestMode(TRUE);

            _iOrgResolution = iRes;
            _iOrgColor      = iClr;
            _iOrgFrequency  = iFreq;
            _rcOrigPos.left   = defaultdm.dmPosition.x;
            _rcOrigPos.top    = defaultdm.dmPosition.y;
            _rcOrigPos.right  = defaultdm.dmPosition.x + defaultdm.dmPelsWidth;
            _rcOrigPos.bottom = defaultdm.dmPosition.y + defaultdm.dmPelsHeight;
        }
    }

#ifdef WINNT
    if (pcdev == NULL)
    {
        //
        // If the current mode is invalid, and there is no mode in the
        // registry that we can read, then we are in some sort of setup mode.
        // Try to use the setup values that are passed in by setup.
        //

        //
        // Try to find the mode that matches the predefined parameters
        // set during setup.
        // If they are not valid, we will find the closest match.
        //
        // Try to find a 60 Hz Mode if no frequency is specified.
        // Try to find a 256 color mode if no color depth is defined
        //

        if (gUnattenedVRefresh == 0) {
            gUnattenedVRefresh = 60;
        }

        if (gUnattenedBitsPerPel == 0) {
            gUnattenedBitsPerPel = 8;
        }

        // DbgPrint("VRefresh = %d  BitsPerPel = %d  XResolution = %d  YResolution = %d \n",
        //          gUnattenedVRefresh, gUnattenedBitsPerPel,
        //          gUnattenedXResolution, gUnattenedYResolution);

        iFreqOk = _pcdmlModes->IndexFromFreq(gUnattenedVRefresh);
        iClrOk  = _pcdmlModes->IndexFromColor(gUnattenedBitsPerPel);
        iResOk  = _pcdmlModes->IndexFromResXY(gUnattenedXResolution,
                                           gUnattenedYResolution);

        if (iFreqOk == -1) {
            iFreqOk = 0;
        }
        if (iResOk == -1) {
            iResOk = 0;
        }
        if (iClrOk == -1) {
            iClrOk = 0;
        }
    }
#endif
    
    //
    // Call FindClosest mode to make sure we actually have a valid mode
    // show up in the display applet.
    //

    _pcdmlModes->FindClosestMode(iResOk, &iClrOk, &iFreqOk);

    if (bComplete || bModeAdded)
        // repopulate the settings lists
        _UpdateSettingsUI();
    
    this->SetCurResolution(iResOk);
    this->SetCurColor(iClrOk);
    this->SetCurFrequency(iFreqOk);
    return TRUE;

}

//
// SaveSettings
//
//  Writes the new display parameters to the proper place in the
//  registry.
//
int CDeviceSettings::SaveSettings(DWORD dwSet)
{
    //
    // Save all of the new values out to the registry
    // Resolution color bits and frequency
    //
    int iResult;
    PCDEVMODE pcdev;
    PDEVMODE  pdevmode;
    pcdev = _pcdmlModes->LookUp(_iResolution,
                               _iColor,
                               _iFrequency);
    ASSERT(pcdev != NULL);
    pdevmode = pcdev->GetData();

    //
    // We always have to set DM_POSITION when calling the API.
    // In order to remove a device from the desktop, what actually needs
    // to be done is provide an emptry rectangle.
    //
    pdevmode->dmFields |= DM_POSITION;
    if (!_fAttached)
    {
        pdevmode->dmPelsWidth = 0;
        pdevmode->dmPelsHeight = 0;
    }
    else
    {
        int cx, cy;

        // Set the correct coordinates
        pdevmode->dmPosition.x = _ptPos.x;
        pdevmode->dmPosition.y = _ptPos.y;
        
        _pcdmlModes->ResXYFromIndex(_iResolution, &cx, &cy);
        pdevmode->dmPelsWidth = cx;
        pdevmode->dmPelsHeight = cy;
    }

#ifndef WINNT
    //
    // on Win9x work around a bug, dont try to set the position (DM_POSITION)
    // on the primary if we are running a old driver (VGA.DRV)
    // it is allways going to fail, if we try.
    // if we are setting the same mode, dont change the position
    //
    HDC hdc = GetDC(NULL);
    int w   = GetDeviceCaps(hdc, HORZRES);
    int h   = GetDeviceCaps(hdc, VERTRES);
    int bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    int c1  = GetDeviceCaps(hdc, CAPS1);
    ReleaseDC(NULL, hdc);

    if (_fPrimary && !(c1 & C1_REINIT_ABLE) &&
        (int)pdevmode->dmPelsWidth  == w &&
        (int)pdevmode->dmPelsHeight == h &&
        (int)pdevmode->dmBitsPerPel == bpp)
    {
        pdevmode->dmFields &= ~(DM_POSITION|DM_DISPLAYFREQUENCY|DM_DISPLAYFLAGS);
    }
#endif

    TraceMsg(TF_GENERAL, "SaveSettings:: Display: %s   Attached: %d   at %d %d  size %d %d",
             _pszDisplay, _fAttached, _ptPos.x, _ptPos.y, pdevmode->dmPelsWidth, pdevmode->dmPelsHeight);
    //
    // DLI: These calls have NORESET flag set so that it only goes to
    // change the registry settings, it does not refresh the display

    iResult = ChangeDisplaySettingsEx(_pszDisplay,
                                pdevmode,
                                NULL,
                                dwSet | ( _fPrimary ? CDS_SET_PRIMARY : 0),
                                NULL);

    if (iResult < 0)
    {
        FmtMessageBox(_hwndMain,
                      MB_ICONEXCLAMATION,
                      FALSE,
                      ID_DSP_TXT_CHANGE_SETTINGS,
                      ID_DSP_TXT_CHANGESETTINGS_FAILED);

        WarnMsg(TEXT("SaveSettings:: ChangeDisplaySettingsEx not successful on "), _pszDisplay);
    }

    if ((dwSet & CDS_UPDATEREGISTRY) && (iResult == DISP_CHANGE_SUCCESSFUL))
    {
        if (_fOrgAttached != _fAttached)
        {
            // This is for real, we should not have the test flag there. 
            ASSERT(!(dwSet & CDS_TEST));
            if (_fOrgAttached == FALSE)
            {
                ASSERT(_fAttached == TRUE);
                RefreshSettings(TRUE);
            }
            _fOrgAttached = _fAttached;
        }
    }
    return iResult;
}

BOOL CDeviceSettings::ConfirmChangeSettings()
{
    // Succeeded, so, reset the original settings 
    _iOrgColor = _iColor;
    _iOrgResolution = _iResolution;
    _iOrgFrequency = _iFrequency;
    this->GetCurPosition(&_rcOrigPos);
    return TRUE;
}

int CDeviceSettings::RestoreSettings()
{

    //
    // Test failed, so retore the old settings, only restore the color and resolution
    // information, don't restore the monitor position and its attached status
    // This function is currently only called when restoring resolution
    //
    int iResult;
    PCDEVMODE pcdev;
    PDEVMODE  pdevmode;

    // If this display was originally turned off, don't bother
    if ((_iOrgResolution == -1) || (_iOrgColor == -1) || (_iOrgFrequency == -1))
        return FALSE;

    // If this has been no change, don't bother, this is possible because other displays
    // could have changed
    if (!IsColorChanged() && !IsFrequencyChanged() && !IsResolutionChanged())
        return DISP_CHANGE_SUCCESSFUL;
    
    pcdev = _pcdmlModes->LookUp(_iOrgResolution,
                               _iOrgColor,
                               _iOrgFrequency);

    ASSERT(pcdev != NULL);
    pdevmode = pcdev->GetData();
    
    //
    // We always have to set DM_POSITION when calling the API.
    // In order to remove a device from the desktop, what actually needs
    // to be done is provide an emptry rectangle.
    //
    pdevmode->dmFields |= DM_POSITION;
    if (!_fAttached)
    {
        pdevmode->dmPelsWidth = 0;
        pdevmode->dmPelsHeight = 0;
    }
    else
    {
        int cx, cy;
        _pcdmlModes->ResXYFromIndex(_iOrgResolution, &cx, &cy);
        pdevmode->dmPelsWidth = cx;
        pdevmode->dmPelsHeight = cy;

        // Set the correct coordinates
        pdevmode->dmPosition.x = _ptPos.x;
        pdevmode->dmPosition.y = _ptPos.y;
    }

    TraceMsg(TF_GENERAL, "SaveSettings:: Display: %s   Attached: %d   at %d %d  size %d %d",
             _pszDisplay, _fAttached, _ptPos.x, _ptPos.y, pdevmode->dmPelsWidth, pdevmode->dmPelsHeight);
    //
    // DLI: These calls have NORESET flag set so that it only goes to
    // change the registry settings, it does not refresh the display

    iResult = ChangeDisplaySettingsEx(_pszDisplay,
                                      pdevmode,
                                      NULL,
                                      CDS_UPDATEREGISTRY | CDS_NORESET | ( _fPrimary ? CDS_SET_PRIMARY : 0),
                                      NULL);
    if (iResult  < 0 )
    {
        FmtMessageBox(_hwndMain,
                      MB_ICONEXCLAMATION,
                      FALSE,
                      ID_DSP_TXT_CHANGE_SETTINGS,
                      ID_DSP_TXT_CHANGESETTINGS_FAILED);

        WarnMsg(TEXT("RestoreSettings:: ChangeDisplaySettingsEx not successful on "), _pszDisplay);
        return FALSE;
    }
    else
    {
        // Succeeded, so, restore the original settings and update the UI
        _iColor      = _iOrgColor;
        _iResolution = _iOrgResolution;
        _iFrequency   = _iOrgFrequency;
        _UpdateSettingsUI();
    }

        
    return iResult;
}

//
// SetCurResolution method
//
//  Sets the string in under the resolution slider, sets the thumb to the
//  correct pos. and remembers the new resolution index.
//
//
void CDeviceSettings::SetCurResolution(int iRes )
{
    BOOL bRepaint;
    int cx, cy;
    LPTSTR lpszXbyY;
    
    _pcdmlModes->ResXYFromIndex(iRes, &cx, &cy);

    lpszXbyY = FmtSprint(ID_DSP_TXT_XBYY, cx, cy);
    if (_fActive)
    {
        this->SendCtlMsg(IDC_RESXY, WM_SETTEXT, 0, (LPARAM)lpszXbyY);
        this->SendCtlMsg(IDC_SCREENSIZE, TBM_SETPOS, TRUE, iRes);
    }
    LocalFree(lpszXbyY);

    bRepaint = (iRes != _iResolution);
        
    _iResolution = iRes;

    if (bRepaint)
        SendMessage(_hwndMain, MM_REDRAWPREVIEW, 0, 0);
}

//
// SetCurFrequency method
//
//  Updates the combo list, and remembers the new frequency index
//
void CDeviceSettings::SetCurFrequency(int iFreq )
{

#ifdef REF_ON_MAIN_PAGE
    if (_fActive)
        this->SendCtlMsg(ID_DSP_FREQ, CB_SETCURSEL, iFreq, 0);
#endif

    _iFrequency = iFreq;
}


//
// SetCurColor method
//
//  Updates the combo list, repaints the correct color bar, and
//  remembers the new color index
//
void CDeviceSettings::SetCurColor(int iClr)
{
    int cBits;
    HBITMAP hbm, hbmOld;
    int iBitmap = IDB_COLOR4DITHER;

    if (_fActive)
    {
        this->SendCtlMsg(IDC_COLORBOX, CB_SETCURSEL, iClr, 0);
    
        cBits = _pcdmlModes->ColorFromIndex(iClr);
        ASSERT(cBits > 0);
        if (cBits == 1)
            iBitmap = IDB_COLOR1;
        else if (cBits <= 4)
            iBitmap = IDB_COLOR4;
        else if (_pcdmlModes->ColorFromIndex(_iOrgColor) >= 16)
        {
            if (cBits <= 8)
                iBitmap = IDB_COLOR8;
            else if (cBits <= 16)
                iBitmap = IDB_COLOR16;
            else
                iBitmap = IDB_COLOR24;
        }

        hbm = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(iBitmap), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (_fActive && hbm)
        {
            hbmOld = (HBITMAP)this->SendCtlMsg(IDC_COLORSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
            if (hbmOld)
                DeleteObject(hbmOld);
        }
    }
    
    _iColor = iClr;
}

BOOL CDeviceSettings::IsColorChanged() {
    if (_iOrgColor == -1)
        return FALSE;
    return (_iColor != _iOrgColor);
}

BOOL CDeviceSettings::IsFrequencyChanged()
{
    if (_iOrgFrequency == -1)
        return FALSE;
    return (_iFrequency != _iOrgFrequency);
}

BOOL CDeviceSettings::IsResolutionChanged()
{
    if (_iOrgResolution == -1)
        return FALSE;
    return (_iResolution != _iOrgResolution);
}


/***************************************************************************\
*
*     FUNCTION: FmtMessageBox(HWND hwnd, int dwTitleID, UINT fuStyle,
*                   BOOL fSound, DWORD dwTextID, ...);
*
*     PURPOSE:  Formats messages with FormatMessage and then displays them
*               in a message box
*
*     PARAMETERS:
*               hwnd        - parent window for message box
*               fuStyle     - MessageBox style
*               fSound      - if TRUE, MessageBeep will be called with fuStyle
*               dwTitleID   - Message ID for optional title, "Error" will
*                             be displayed if dwTitleID == -1
*               dwTextID    - Message ID for the message box text
*               ...         - optional args to be embedded in dwTextID
*                             see FormatMessage for more details
* History:
* 22-Apr-1993 JonPa         Created it.
\***************************************************************************/
int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    BOOL fSound,
    DWORD dwTitleID,
    DWORD dwTextID, ... )
{

    LPTSTR pszMsg;
    LPTSTR pszTitle;
    int idRet;

    va_list marker;

    va_start(marker, dwTextID);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       hInstance,
                       dwTextID,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {

        pszMsg = gpszError;

    }

    va_end(marker);

    GetLastError();

    pszTitle = NULL;

    if (dwTitleID != -1) {

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_HMODULE |
                          FORMAT_MESSAGE_MAX_WIDTH_MASK |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      hInstance,
                      dwTitleID,
                      0,
                      (LPTSTR)&pszTitle,
                      1,
                      NULL);
                      //(va_list *)&pszTitleStr);

    }

    //
    // Turn on the beep if requested
    //

    if (fSound) {
        MessageBeep(fuStyle & (MB_ICONASTERISK | MB_ICONEXCLAMATION |
                MB_ICONHAND | MB_ICONQUESTION | MB_OK));
    }

    idRet = MessageBox(hwnd, pszMsg, pszTitle, fuStyle);

    if (pszTitle != NULL)
        LocalFree(pszTitle);

    if (pszMsg != gpszError)
        LocalFree(pszMsg);

    return idRet;
}

/***************************************************************************\
*
*     FUNCTION: FmtSprint(DWORD id, ...);
*
*     PURPOSE:  sprintf but it gets the pattern string from the message rc,
*               and allocates the buffer for the end result.
*
* History:
* 03-May-1993 JonPa         Created it.
\***************************************************************************/
LPTSTR FmtSprint(DWORD id, ... ) {
    LPTSTR pszMsg;
    va_list marker;

    va_start(marker, id);

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_HMODULE |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       hInstance,
                       id,
                       0,
                       (LPTSTR)&pszMsg,
                       1,
                       &marker)) {


        GetLastError();
        pszMsg = TEXT("...");
    }
    va_end(marker);

    return pszMsg;
}

/****************************************************************************\
*
* LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan )
*
*   If pszScan starts with pszTarget, then the function returns the first
* char of pszScan that follows the pszTarget; other wise it returns pszScan.
*
* eg: SubStrEnd("abc", "abcdefg" ) ==> "defg"
*     SubStrEnd("abc", "abZQRT" ) ==> "abZQRT"
*
* History:
*   09-Dec-1993 JonPa   Wrote it.
\****************************************************************************/
LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan ) {
    int i;

    for (i = 0; pszScan[i] != TEXT('\0') && pszTarget[i] != TEXT('\0') &&
            CharUpper(CHARTOPSZ(pszScan[i])) ==
            CharUpper(CHARTOPSZ(pszTarget[i])); i++);

    if (pszTarget[i] == TEXT('\0')) {

        // we found the substring
        return pszScan + i;
    }

    return pszScan;
}

/****************************************************************************\
*
* CloneString
*
* Makes a copy of a string.  By copy, I mean it actually allocs memeory
* and copies the chars across.
*
* NOTE: the caller must LocalFree the string when they are done with it.
*
* Returns a pointer to a LocalAlloced buffer that has a copy of the
* string in it.  If an error occurs, it retuns NULl.
*
* 16-Dec-1993 JonPa     Wrote it.
\****************************************************************************/
LPTSTR CloneString(LPTSTR psz ) {
    int cb;
    LPTSTR psz2;

    if (psz == NULL)
        return NULL;

    cb = (lstrlen(psz) + 1) * SIZEOF(TCHAR);

    psz2 = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb);
    if (psz2 != NULL)
        CopyMemory(psz2, psz, cb);

    return psz2;
}


/****************************************************************************\
*
* DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
*
* Thread that gets started when the use hits the Apply Now button.
* This thread creates a new desktop with the new video mode, switches to it
* and then displays a dialog box asking if the display looks OK.  If the
* user does not respond within the time limit, then 'NO' is assumed to be
* the answer.
*
\****************************************************************************/
DWORD WINAPI ApplyNowThd(LPVOID lpThreadParameter)
{

    PNEW_DESKTOP_PARAM lpDesktopParam = (PNEW_DESKTOP_PARAM) lpThreadParameter;
    HDESK hdsk = NULL;
    HDESK hdskDefault = NULL;
    BOOL bTest = FALSE;
    HDC hdc;

    //
    // HACK:
    // We need to make a USER call before calling the desktop stuff so we can
    // sure our threads internal data structure are associated with the default
    // desktop.
    // Otherwise USER has problems closing the desktop with our thread on it.
    //

    GetSystemMetrics(SM_CXSCREEN);

    //
    // Create the desktop
    //

    hdskDefault = GetThreadDesktop(GetCurrentThreadId());

    if (hdskDefault != NULL) {

        hdsk = CreateDesktop(TEXT("Display.Cpl Desktop"),
                             lpDesktopParam->pwszDevice,
                             lpDesktopParam->lpdevmode,
                             0,
                             MAXIMUM_ALLOWED,
                             NULL);

        if (hdsk != NULL) {

            //
            // use the desktop for this thread
            //

            if (SetThreadDesktop(hdsk)) {

                hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

                if (hdc) {

                    DrawBmp(hdc);

                    DeleteDC(hdc);

                    bTest = TRUE;
                }

                //
                // Sleep for some seconds so you have time to look at the screen.
                //

                Sleep(7000);

            }
        }


        //
        // Reset the thread to the right desktop
        //

        SetThreadDesktop(hdskDefault);

        SwitchDesktop(hdskDefault);

        //
        // Can only close the desktop after we have switched to the new one.
        //

        if (hdsk != NULL)
            CloseDesktop(hdsk);

    }

    ExitThread((DWORD) bTest);

    return 0;
}

/***************************************************************************\
* CDialogProc
*
*
* History:
* 23-Sep-1993 JonPa     Created it
\***************************************************************************/

BOOL CALLBACK CDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCDIALOG pdlg;
#ifdef DBG_PRINT
    static int i=0;
    int j;

    for(j =0; j < i; j++)
        DPRINTF((szdbuf, TEXT(" ")));
    i++;
    DPRINTF((szdbuf,TEXT("> CDialogProc: hw=%08x msg=%04X wp=%08x lp=%08x\n"), (DWORD)hwnd, msg, wParam, lParam));
#endif

    if (msg == WM_INITDIALOG) {
        /*
         * Set up Object/Window relationship
         */
        SetWindowLong(hwnd, DWL_USER, lParam);
        pdlg = (PCDIALOG) lParam;
        pdlg->SetHWnd(hwnd);
        pdlg->SetHWndParent(GetParent(hwnd));
    }

    pdlg = (PCDIALOG)GetWindowLong(hwnd, DWL_USER);



    if (pdlg != NULL) {

        /*
         * Dispatch to the Dialog Object's WndProc method
         */
#ifdef DBG_PRINT
        LRESULT lr = pdlg->WndProc(msg, wParam, lParam);

        i--;
        for(j =0; j < i; j++)
            DPRINTF((szdbuf, TEXT(" ")));

        DPRINTF((szdbuf,TEXT("< CDialogProc: hw=%08x msg=%04X wp=%08x lp=%08x ret=%08x\n"), (DWORD)hwnd, msg, wParam, lParam, lr));
        return lr;
#else
        return pdlg->WndProc(msg, wParam, lParam);
#endif

    }



#ifdef DBG_PRINT
    i--;
    for(j =0; j < i; j++)
        DPRINTF((szdbuf, TEXT(" ")));
    DPRINTF((szdbuf,TEXT("<.CDialogProc: hw=%08x msg=%04X wp=%08x lp=%08x ret=FALSE\n"), (DWORD)hwnd, msg, wParam, lParam ));
#endif

    return FALSE;
}
