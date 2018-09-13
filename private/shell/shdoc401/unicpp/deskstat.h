#ifndef _DESKSTAT_H_
#define _DESKSTAT_H_

// BUGBUG: This is NOT a IE4COMPONENTA struct, it is a IE4COMPONENTT struct.
// Note: This is the old COMPONENTA structure used in IE4. It is kept here for compatibility.
typedef struct _tagIE4COMPONENTA
{
    DWORD   dwSize;
    DWORD   dwID;
    int     iComponentType;
    BOOL    fChecked;
    BOOL    fDirty;
    BOOL    fNoScroll;
    COMPPOS cpPos;
    TCHAR   szFriendlyName[MAX_PATH];
    TCHAR   szSource[INTERNET_MAX_URL_LENGTH];
    TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];
} IE4COMPONENTA;
typedef IE4COMPONENTA *LPIE4COMPONENTA;
typedef const IE4COMPONENTA *LPCIE4COMPONENTA;

// BUGBUG: This is NOT a COMPONENTA struct, it is a COMPONENTT struct.

// Note: This is the new NT5 COMPONENT structure. The old component structure is kept at the 
// begining of this struct and the new fields are added at the end. The dwSize field is used to 
// distinguish between the old and new structures.
//
typedef struct _tagCOMPONENTA
{
    DWORD   dwSize;
    DWORD   dwID;
    int     iComponentType;
    BOOL    fChecked;
    BOOL    fDirty;
    BOOL    fNoScroll;
    COMPPOS cpPos;
    TCHAR   szFriendlyName[MAX_PATH];
    TCHAR   szSource[INTERNET_MAX_URL_LENGTH];
    TCHAR   szSubscribedURL[INTERNET_MAX_URL_LENGTH];
    // Add the new fields below this point. Everything above must exactly match the 
    // old IE4COMPONENTA structure for compatibility.
    DWORD           dwCurItemState;
    COMPSTATEINFO   csiOriginal;
    COMPSTATEINFO   csiRestored;
} COMPONENTA;
typedef COMPONENTA *LPCOMPONENTA;
typedef const COMPONENTA *LPCCOMPONENTA;

typedef struct _tagTAGENTRY
{
    LPCSTR pszTag;
    BOOL fSkipPast;
} TAGENTRY;

#define c_wszNULL   (L"")

class CActiveDesktop : public IActiveDesktop, IActiveDesktopP, IADesktopP2
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IActiveDesktop ***
    virtual STDMETHODIMP ApplyChanges(DWORD dwFlags);
    virtual STDMETHODIMP GetWallpaper(LPWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwReserved);
    virtual STDMETHODIMP SetWallpaper(LPCWSTR pwszWallpaper, DWORD dwReserved);
    virtual STDMETHODIMP GetWallpaperOptions(LPWALLPAPEROPT pwpo, DWORD dwReserved);
    virtual STDMETHODIMP SetWallpaperOptions(LPCWALLPAPEROPT pwpo, DWORD dwReserved);
    virtual STDMETHODIMP GetPattern(LPWSTR pwszPattern, UINT cchPattern, DWORD dwReserved);
    virtual STDMETHODIMP SetPattern(LPCWSTR pszPattern, DWORD dwReserved);
    virtual STDMETHODIMP GetDesktopItemOptions(LPCOMPONENTSOPT pco, DWORD dwReserved);
    virtual STDMETHODIMP SetDesktopItemOptions(LPCCOMPONENTSOPT pco, DWORD dwReserved);
    virtual STDMETHODIMP AddDesktopItem(LPCCOMPONENT pcomp, DWORD dwReserved);
    virtual STDMETHODIMP AddDesktopItemWithUI(HWND hwnd, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual STDMETHODIMP ModifyDesktopItem(LPCCOMPONENT pcomp, DWORD dwFlags);
    virtual STDMETHODIMP RemoveDesktopItem(LPCCOMPONENT pcomp, DWORD dwReserved);
    virtual STDMETHODIMP GetDesktopItemCount(LPINT lpiCount, DWORD dwReserved);
    virtual STDMETHODIMP GetDesktopItem(int nComponent, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual STDMETHODIMP GetDesktopItemByID(DWORD dwID, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual STDMETHODIMP GenerateDesktopItemHtml(LPCWSTR pwszFileName, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual STDMETHODIMP AddUrl(HWND hwnd, LPCWSTR pszSource, LPCOMPONENT pcomp, DWORD dwFlags);
    virtual STDMETHODIMP GetDesktopItemBySource(LPCWSTR pszSource, LPCOMPONENT pcomp, DWORD dwReserved);
    // *** IActiveDesktopP ***
    virtual STDMETHODIMP SetSafeMode(DWORD dwFlags);
    virtual STDMETHODIMP EnsureUpdateHTML(void);
    virtual STDMETHODIMP SetScheme(LPCWSTR pwszSchemeName, DWORD dwFlags);
    virtual STDMETHODIMP GetScheme(LPWSTR pwszSchemeName, LPDWORD lpdwcchBuffer, DWORD dwFlags);
    // *** IADesktopP2 ***
    virtual STDMETHODIMP ReReadWallpaper(void);
    virtual STDMETHODIMP GetADObjectFlags(LPDWORD lpdwFlags, DWORD dwMask);
    virtual STDMETHODIMP UpdateAllDesktopSubscriptions();
    virtual STDMETHODIMP MakeDynamicChanges(IOleObject *pOleObj);
 
    //Some Ansi versions of the methods for Internal Use
    BOOL AddComponentPrivate(COMPONENTA *pcomp, DWORD dwID);
    BOOL UpdateComponentPrivate(int iIndex, COMPONENTA *pcomp);
    BOOL RemoveComponentPrivate(int iIndex, COMPONENTA *pcomp);
    BOOL GetComponentPrivate(int nComponent, COMPONENTA *pcomp);

    CActiveDesktop();

protected:
    DWORD           _cRef;
    DWORD           _dwNextID;
    BOOL            _fDirty;
    BOOL            _fWallpaperDirty;
    BOOL            _fWallpaperChangedDuringInit;
    BOOL            _fPatternDirty;
    BOOL            _fSingleItem;
    BOOL            _fInitialized;
    BOOL            _fNeedBodyEnd;
    HDSA            _hdsaComponent;
    TCHAR           _szSelectedWallpaper[MAX_PATH];
    TCHAR           _szBackupWallpaper[MAX_PATH];
    FILETIME        _ftWallpaperFileTime;
    TCHAR           _szSelectedPattern[MAX_PATH];
    LPTSTR          _pszScheme;
    WALLPAPEROPT    _wpo;
    COMPONENTSOPT   _co;
    HANDLE          _hFileHtml;
    HFILE           _hfileHtmlBackground;
    BOOL            _fNoDeskMovr;
    BOOL            _fBackgroundHtml;

    ~CActiveDesktop();

    int  _FindComponentIndexByID(DWORD dwID);
    int  _FindComponentBySource(LPTSTR lpszSource, COMPONENTA *pComp);

    void _ReadComponent(HKEY hkey, LPCTSTR pszComp);
    void _SortAndRationalize(void);
    void _ReadComponents(BOOL fActiveDesktop);
    void _ReadWallpaper(BOOL fActiveDesktop);
    void _ReadPattern(void);
    void _Initialize(void);

    void _SaveComponent(HKEY hkey, int iIndex, COMPONENTA *pcomp);
    void _SaveComponents(void);
    void _SaveWallpaper(void);
    void _SavePattern(DWORD dwFlags);
    void _SaveSettings(DWORD dwFlags);

    void _GenerateHtmlHeader(void);
    void _GenerateHtmlPicture(COMPONENTA *pcomp);
    void _GenerateHtmlDoc(COMPONENTA *pcomp);
    void _GenerateHtmlSite(COMPONENTA *pcomp);
    void _GenerateHtmlControl(COMPONENTA *pcomp);
    void _GenerateHtmlComponent(COMPONENTA *pcomp);
    void _GenerateHtmlFooter(void);
    void _GenerateHtml(void);

    void _WriteHtmlFromString(LPCTSTR psz);
    void _WriteHtmlFromId(UINT uid);
    void _WriteHtmlFromIdF(UINT uid, ...);
    void _WriteHtmlFromFile(LPCTSTR pszContents);
    void _WriteHtmlFromHfile(HFILE hfile, int iOffsetStart, int iOffsetEnd);
    void _WriteResizeable(COMPONENTA *pcomp);

    int _ScanTagEntries(HFILE hfile, int iOffsetStart, TAGENTRY *pte, int cte);
    int _ScanForTag(HFILE hfile, int iOffsetStart, LPCSTR pszTagA);

    HRESULT _CopyComponent(COMPONENTA *pCompDest, COMPONENTA *pCompSrc, DWORD dwFlags);

private:
    HRESULT _AddDTIWithUIPrivateA(HWND hwnd, LPCCOMPONENT pComp, DWORD dwFlags);
};

extern IActiveDesktop *g_pActiveDesk;
int GetIntFromSubkey(HKEY hKey, LPCTSTR lpszValueName, int iDefault);
int GetIntFromReg(HKEY hKey, LPCTSTR lpszSubkey, LPCTSTR lpszNameValue, int iDefault);
BOOL GetStringFromReg(HKEY hkey, LPCTSTR lpszSubkey, LPCTSTR lpszValueName, LPCTSTR lpszDefault, LPTSTR lpszValue, DWORD cchSizeofValueBuff);
STDAPI CActiveDesktop_InternalCreateInstance(LPUNKNOWN * ppunk, REFIID riid);
void GetPerUserFileName(LPTSTR pszOutputFileName, DWORD dwSize, LPTSTR pszPartialFileName);
STDAPI CDeskHtmlProp_RegUnReg(BOOL bReg);

//Function to convert components in either direction.
void ConvertCompStruct(COMPONENTA *pCompDest, COMPONENTA *pCompSrc, BOOL fPubToPriv);
void SetStateInfo(COMPSTATEINFO *pCompStateInfo, COMPPOS *pCompPos, DWORD dwItemState);

#define MultiCompToWideComp(MultiComp, WideComp)  ConvertCompStruct((COMPONENTA *)WideComp, MultiComp, FALSE)
#define WideCompToMultiComp(WideComp, MultiComp)  ConvertCompStruct(MultiComp, (COMPONENTA *)WideComp, TRUE)

#define COMPONENT_TOP_WINDOWLESS (COMPONENT_TOP / 2)
#define IsWindowLessComponent(pcomp) (((pcomp)->iComponentType == COMP_TYPE_PICTURE) || ((pcomp)->iComponentType == COMP_TYPE_HTMLDOC))

#define COMPONENT_DEFAULT_WIDTH   ((DWORD)-1)
#define COMPONENT_DEFAULT_HEIGHT  ((DWORD)-1)

#define DESKMOVR_FILENAME       TEXT("\\Web\\DeskMovr.htt")
#define DESKTOPHTML_FILENAME    TEXT("\\Microsoft\\Internet Explorer\\Desktop.htt")
#define PATTERN_FILENAME        TEXT("\\Microsoft\\Internet Explorer\\Pattern.bmp")

#define SAVE_PATTERN_NAME       0x00000001
#define GENERATE_PATTERN_FILE   0x00000002

#endif // _DESKSTAT_H_
