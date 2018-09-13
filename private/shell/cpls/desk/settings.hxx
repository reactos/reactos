
/*****************************************************************\
*
* CDeviceSettings class
*
*  This class is in charge of all the settings specific to one display
*  device. Including Screen Size, Color Depth, Font size.  
*
\*****************************************************************/

// NOTE: (dli) We probably should let this class be the only guy that does the
// real change settings work. 
class CDEVMODELST;
typedef CDEVMODELST *PCDEVMODELST;

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#define MM_REDRAWPREVIEW (WM_USER + 1)
class CDeviceSettings : public IDataObject
{
protected:

    //
    // The Display Device we are currently working with.
    //

    HWND _hwndMain; // This is the hwnd for the main settings page
    DISPLAY_DEVICE _DisplayDevice;
    LPTSTR _pszDisplay;
    PCDEVMODELST _pcdmlModes;

    int _iOrgResolution;  // The current system settings
    int _iOrgColor;
    int _iOrgFrequency;

    int _iResolution;     // The current CPL settings. 
    int _iColor;
    int _iFrequency;

    POINT _ptPos;     // Position of this display
    RECT  _rcOrigPos; // Original Position
    UINT  _iRate;     // This is UINT for a good reason
    int   _cResolutions;

    //
    // If the current device is attached to the desktop
    //
    BOOL _fAttached ;    // Is it current attached in the CPL(may not be attached in the system)
    BOOL _fOrgAttached;  // Is it attached in the system?
    BOOL _fBadDriver ;
    BOOL _fUsingDefault ;

    //
    // When a new driver is installed, we don't want to save the parameters
    // to the registry.
    //
    BOOL _fNewDriver ;
    BOOL _fPrimary ;

    // I am currently active in the display cpl
    // When the UpdateSettingsUI stuff goes away, this should also go. 
    BOOL _fActive ;
    
    // Ref count for IDataObject
    UINT _cRef;
    
    // Private functions
    void SetCurColor(int iClr);
    void SetCurFrequency(int iFreq);
    void SetCurResolution(int iRes);

    void PopulateColorList();
    void PopulateScreenSizeList();
    void PopulateFrequencyList();

    void _UpdateSettingsUI();

    BOOL _IsDataFormatSupported(CLIPFORMAT cfFormat);
    LONG SendCtlMsg(int idCtl, UINT msg, WPARAM wParam, LPARAM lParam ) {
        return SendDlgItemMessage(_hwndMain, idCtl, msg, wParam, lParam);
    }
    void _IndexesFromDevMode(PDEVMODE pdm, int * piRes, int * piClr, int * piFreq);
    STDMETHODIMP TransferDisplayName(FORMATETC * pfmtetc, STGMEDIUM * pstgmed);
    
public:
    CDeviceSettings(HWND hwnd = NULL);
    ~CDeviceSettings();

    BOOL InitSettings(PDISPLAY_DEVICE pdd);
    BOOL RefreshSettings(BOOL bComplete);

    int SaveSettings(DWORD dwSet);
    int RestoreSettings();
    BOOL ConfirmChangeSettings();
    
    void SetCurPosition(LPPOINT ppt);
    void GetCurPosition(LPRECT prcPos);
    void GetOrigPosition(LPRECT prcPos);

    void SetPrimary(BOOL fPrimary) { _fPrimary = fPrimary; };
    void SetAttached(BOOL fAttached);
    void SetActive(BOOL fActive);
    
    BOOL IsSmallFontNecessary();
    BOOL IsColorChanged();
    BOOL IsFrequencyChanged();
    BOOL IsResolutionChanged();
    BOOL IsAttached() { return _fAttached; };
    BOOL IsPrimary() { return _fPrimary; };
    
    void ChangeColor(int iClr);
    int  GetColorBits();
    void ChangeResolution(int iRes);
    int  GetCurResolution() { return _iResolution; };
    PMONPARAM GetMonitorParams();
    void UpdateRefreshRate(PMONPARAM pMonParams);

    BOOL GetMonitorName(LPTSTR pszName, LPTSTR pszDevice);

    void SetMode(int w, int h, int bpp);

    // IUnknown methods
    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDataObject methods

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


extern void InitClipboardFormats();
