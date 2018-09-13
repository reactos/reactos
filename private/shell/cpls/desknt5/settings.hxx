/**************************************************************************\
* Module Name: settings.hxx
*
* CDeviceSettings class
*
*  This class is in charge of all the settings specific to one display
*  device. Including Screen Size, Color Depth, Font size.  
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/


#ifndef SETTINGS_HXX
#define SETTINGS_HXX

#if DEBUG
void *  __cdecl operator new(size_t Size, LPCTSTR File, int Line);
#define DBG_NEW new(TEXT(__FILE__), __LINE__)
#endif
void *  __cdecl operator new(size_t nSize);
void  __cdecl operator delete(void *pv);

#define MAKEXYRES(p,xval,yval)  ((p)->x = xval, (p)->y = yval)

#define _CURXRES  ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmPelsWidth : -1)
#define _CURYRES  ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmPelsHeight : -1)
#define _ORGXRES  ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmPelsWidth : -1)
#define _ORGYRES  ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmPelsHeight : -1)

#define _CURCOLOR ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmBitsPerPel : -1)
#define _ORGCOLOR ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmBitsPerPel : -1)

#define _CURFREQ  ((_pCurDevmode != NULL) ? (int)_pCurDevmode->dmDisplayFrequency : -1)
#define _ORGFREQ  ((_pOrgDevmode != NULL) ? (int)_pOrgDevmode->dmDisplayFrequency : -1)

#define MODE_INVALID    0x00000001
#define MODE_RAW        0x00000002

typedef struct _MODEARRAY {

    DWORD     dwFlags;
    LPDEVMODE lpdm;

} MODEARRAY, *PMODEARRAY;

class CDeviceSettings : public IDataObject
{
protected:
    //
    // The Display Device we are currently working with.
    //

    LPDISPLAY_DEVICE _pDisplayDevice;

    ULONG       _cpdm;
    PMODEARRAY  _apdm;

    //
    // The current system settings
    //

    POINT       _ptOrgPos;
    LPDEVMODE   _pOrgDevmode;
    BOOL        _fOrgAttached;

    //
    // The current CPL settings.
    //

    POINT       _ptCurPos;
    LPDEVMODE   _pCurDevmode;
    BOOL        _fCurAttached;

    //
    // If the current device is attached to the desktop
    //
    BOOL        _fUsingDefault;
    BOOL        _fPrimary;
    BOOL        _fBadData;

    //
    // Pruning 
    //

    BOOL        _bCanBePruned;       // true if raw mode list != pruned mode list
    BOOL        _bIsPruningReadOnly; // true if can be pruned and pruning mode can be written
    BOOL        _bIsPruningOn;       // true if can be pruned and pruning mode is on
    HKEY        _hPruningRegKey;
    
    //
    // Ref count for IDataObject
    //

    UINT        _cRef;

    //
    // Private functions
    //

    void _Dump_CDeviceSettings(BOOL bAll);
    void _Dump_CDevmodeList(VOID);
    void _Dump_CDevmode(LPDEVMODE pdm);
    int  _InsertSortedDwords(int val1, int val2, int cval, int **ppval);
    BOOL _AddDevMode(LPDEVMODE lpdm);
    void _BestMatch(LPPOINT Res, int Color);
    BOOL _ExactMatch(LPDEVMODE lpdm);
    BOOL _PerfectMatch(LPDEVMODE lpdm);
    void _SetCurrentValues(LPDEVMODE lpdm);
    int  _GetCurrentModeFrequencyList(int Color, LPPOINT Res, PLONGLONG *ppFrequency);
    BOOL _MarkMode(LPDEVMODE lpdm);
    BOOL _IsCurDevmodeRaw();
    BOOL _IsModeVisible(int i);
    static BOOL _IsModeVisible(CDeviceSettings* pSettings, int i);

    //
    // OLE support for extensibility.
    //
    void _InitClipboardFormats();
    void _FilterModes();

    static LPDEVMODEW _lpfnEnumAllModes(LPVOID pContext, DWORD iMode);
    static BOOL       _lpfnSetSelectedMode(LPVOID pContext, LPDEVMODEW lpdm);
    static LPDEVMODEW _lpfnGetSelectedMode(LPVOID pContext);
    static VOID       _lpfnSetPruningMode(LPVOID pContext, BOOL bIsPruningOn);
    static VOID       _lpfnGetPruningMode(LPVOID pContext, 
                                          BOOL* pbCanBePruned,
                                          BOOL* pbIsPruningReadOnly,
                                          BOOL* pbIsPruningOn);

public:

    CDeviceSettings();
    ~CDeviceSettings();

    //
    // General Settings support
    //

    BOOL InitSettings(LPDISPLAY_DEVICE pDisplay);
    int  SaveSettings(DWORD dwSet);
    int  RestoreSettings();
    BOOL ConfirmChangeSettings();
    BOOL bIsModeChanged()               {return _pCurDevmode != _pOrgDevmode;}

    //
    // Device Settings
    //

    void SetPrimary(BOOL fPrimary)      { _fPrimary     = fPrimary; };
    void SetAttached(BOOL fAttached)    { _fCurAttached = fAttached; };
    BOOL IsPrimary()                    { return _fPrimary; };
    BOOL IsAttached()                   { return _fCurAttached; };
    BOOL IsOrgAttached()                { return _fOrgAttached; };
    BOOL IsSmallFontNecessary();

    LPDEVMODE GetCurrentDevMode(void); 
    //
    // Color information
    //

    int  GetColorList(LPPOINT Res, PLONGLONG *ppColor);
    void SetCurColor(int Color)         { _BestMatch(NULL, Color); }
    int  GetCurColor()                  { return _CURCOLOR;}
    BOOL IsColorChanged()
    {
        return (_ORGCOLOR == -1) ? FALSE : (_CURCOLOR != _ORGCOLOR);
    }

    //
    // Resolution information
    //

    int  GetResolutionList(int Color, PPOINT *ppRes);
    void SetCurResolution(LPPOINT ppt)  { _BestMatch(ppt, -1); }
    void GetCurResolution(LPPOINT ppt)  
    { 
        ppt->x = _CURXRES;
        ppt->y = _CURYRES; 
    }
    BOOL IsResolutionChanged()
    {
        if (_ORGXRES == -1)
            return FALSE;
        else
            return ((_CURXRES != _ORGXRES) && (_CURYRES != _ORGYRES));
    }

    int  GetFrequencyList(int Color, LPPOINT Res, PLONGLONG *ppFreq);
    void SetCurFrequency(int Frequency);
    int  GetCurFrequency()              { return _CURFREQ; }
    BOOL IsFrequencyChanged()
    {
        return (_ORGFREQ == -1) ? FALSE : (_CURFREQ != _ORGFREQ);
    }

    //
    // Position information
    //

    void SetCurPosition(LPPOINT ppt) {_ptCurPos = *ppt;}
    void SetOrgPosition(LPPOINT ppt) {_ptOrgPos = *ppt;}
    void GetCurPosition(PRECT prc)
    {
        prc->left   = _ptCurPos.x;
        prc->top    = _ptCurPos.y;
        prc->right  = _ptCurPos.x + _CURXRES;
        prc->bottom = _ptCurPos.y + _CURYRES;
    }
    void GetOrgPosition(PRECT prc)
    {
        prc->left   = _ptOrgPos.x;
        prc->top    = _ptOrgPos.y;
        prc->right  = _ptOrgPos.x + _ORGXRES;
        prc->bottom = _ptOrgPos.y + _ORGYRES;
    }

    //
    // Monitor information
    //

    BOOL GetMonitorName(LPTSTR pszName);
    BOOL GetMonitorDevice(LPTSTR pszDevice);
    HRESULT GetDevInstID(LPTSTR lpszDeviceKey, STGMEDIUM *pstgmed);

    //
    // Pruning mode
    //
    
    void SetPruningMode(BOOL bIsPruningOn);
    void GetPruningMode(BOOL* pbCanBePruned, 
                        BOOL* pbIsPruningReadOnly,
                        BOOL* pbIsPruningOn);

    //
    // IUnknown methods
    //

    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IDataObject methods
    //

    STDMETHODIMP GetData(FORMATETC *pfmtetcIn, STGMEDIUM *pstgmed);
    STDMETHODIMP GetDataHere(FORMATETC *pfmtetc, STGMEDIUM *pstgpmed);
    STDMETHODIMP QueryGetData(FORMATETC *pfmtetc);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pfmtetcIn, FORMATETC *pfmtetcOut);
    STDMETHODIMP SetData(FORMATETC *pfmtetc, STGMEDIUM *pstgmed, BOOL bRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppienumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pfmtetc, DWORD dwAdviseFlags, IAdviseSink * piadvsink, PDWORD pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppienumStatData);

};

#endif // SETTINGS_HXX
