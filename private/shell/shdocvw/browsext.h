#ifndef _BROWSEXT_H
#define _BROWSEXT_H

#include "tbext.h"

#define FCIDM_TOOLS_EXT_PLACEHOLDER         0x9000
#define FCIDM_TOOLS_EXT_MOD_MARKER          0x9001
#define FCIDM_HELP_EXT_PLACEHOLDER          0x9002
#define FCIDM_HELP_EXT_MOD_MARKER           0x9003

//
// This class is used to store/retrieve images by name (guid) from shared image lists
//
class CImageList
{
public:
    CImageList(HIMAGELIST himl = NULL);
    ~CImageList();

    CImageList& operator=(HIMAGELIST himl);
    operator HIMAGELIST() { return _himl; }
    int GetImageIndex(REFGUID rguid);
    int AddIcon(HICON hicon, REFGUID rguid);
    BOOL HasImages() { return (_himl != NULL); }
    void FreeImages();

protected:
    static int _DPADestroyCallback(LPVOID p, LPVOID d);

    // Associate guids with indices into the image list
    struct ImageAssoc
    {
        GUID    guid;
        int     iImage;
    };
    HIMAGELIST  _himl;
    HDPA        _hdpa;      // Array of ImageAssoc
};


//
// Internal interface fo managing buttons added to the internet toolbar and menu items added to the
// tools menu.  This interface will likely go away afer IE5B2 when we move this functionality to
// a browser helper object.
//
EXTERN_C const IID IID_IToolbarExt;

DECLARE_INTERFACE_(IToolbarExt, IUnknown)
{
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) PURE;
    virtual STDMETHODIMP_(ULONG) AddRef(void) PURE;
    virtual STDMETHODIMP_(ULONG) Release(void) PURE;

    // *** IToolbarExt methods ***
    virtual STDMETHODIMP GetButtons(TBBUTTON* ptbArr, int nNumButtons, BOOL fInit) PURE;
    virtual STDMETHODIMP GetNumButtons(UINT* pButtons) PURE;
    virtual STDMETHODIMP InitButtons(IExplorerToolbar* pxtb, UINT* puStringIndex, const GUID* pguidCommandGroup) PURE;
    virtual STDMETHODIMP OnCustomizableMenuPopup(HMENU hMenuParent, HMENU hMenu) PURE;
    virtual STDMETHODIMP OnMenuSelect(UINT nCmdID) PURE;
};

class CBrowserExtension : public IToolbarExt
                        , public IObjectWithSite
                        , public IOleCommandTarget
{
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* pUnkSite);
    virtual STDMETHODIMP GetSite(REFIID riid, void ** ppvSite);

    // *** IToolbarExt methods ***
    virtual STDMETHODIMP GetButtons(TBBUTTON* ptbArr, int nNumButtons, BOOL fInit);
    virtual STDMETHODIMP GetNumButtons(UINT* pButtons);
    virtual STDMETHODIMP InitButtons(IExplorerToolbar* pxtb, UINT* puStringIndex, const GUID* pguidCommandGroup);
    virtual STDMETHODIMP OnCustomizableMenuPopup(HMENU hMenuParent, HMENU hMenu);
    virtual STDMETHODIMP OnMenuSelect(UINT nCmdID);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);

protected:
    // Instance creator
    friend HRESULT CBrowserExtension_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

    CBrowserExtension();
    ~CBrowserExtension();

    HRESULT Update();

    struct ExtensionItem
    {
        CLSID               guid;       // id of the extension
        IBrowserExtension*  pIBE;
        BITBOOL             fButton:1;  // if has a button
        BITBOOL             fVisible:1; // if defaults to visible on the toolbar
        int                 iStringID;  // Keeps track of the location in the string resource for the button
        int                 iImageID;   // The ID of the icon in the image lists
        int                 idCmd;      // toolbar CmdId
        UINT                idmMenu;    // idm of the menu this extension belongs to
    };

    void            _AddItem(HKEY hkeyExtensions,  LPCWSTR pszButtonGuid, REFGUID rguid);
    ExtensionItem*  _FindItem(REFGUID rguid);
    void            _FreeItems();
    HRESULT         _Exec(int nItem, int nCmdID);
    UINT            _GetImageLists(CImageList** ppimlDef, CImageList** ppimlHot, BOOL fSmall);
    void            _ReleaseImageLists(UINT uiIndex);
    HRESULT         _AddCustomImagesToImageList(CImageList& rimlNormal, CImageList& rimlHot, BOOL fSmallIcons);
    HRESULT         _AddCustomStringsToBuffer(IExplorerToolbar * pxtb, const GUID* pguidCommandGroup);

    int             _GetCmdIdFromClsid(LPCWSTR pszGuid);
    int             _GetIdpaFromCmdId(int nCmdId);

    typedef struct tagBROWSEXT_MENU_INFO
    {
        UINT    idmMenu;        // idm for this menu
        UINT    idmPlaceholder;
        UINT    idmModMarker;   // separator with this idm is present if customizations have been made
        int     iInsert;        // insertion point for custom items
    } BROWSEXT_MENU_INFO;

    HRESULT         _GetCustomMenuInfo(HMENU hMenuParent, HMENU hMenu, BROWSEXT_MENU_INFO * pMI);

    LONG                _cRef;                  // reference count
    HDPA                _hdpa;                  // array of ExtensionItem*
    int                 _nExtButtons;           // Number of Buttons
    int                 _nExtToolsMenuItems;    // Number of Tools Menu Items
    IShellBrowser*      _pISB;                  // Passed into the IObjectWithSite::GetSite
    UINT                _uStringIndex;          // index of first string added to toolbar

    // Used for CUT/COPY/PASTE imagelist
    struct CImageCache
    {
        UINT        uiResDef;               // resource id for grey-scale bitmap
        UINT        uiResHot;               // resource id for color bitmap
        CImageList  imlDef;                 // grey scale imagelist
        CImageList  imlHot;                 // color imagelist
        int         cUsage;                 // number of instances using this item
    };
    static CImageCache      _rgImages[3];   // cached image lists:
                                            //   16 color 16x16
                                            //   16 color 20x20
                                            //   256 color 20x20
    UINT                    _uiImageIndex;  // Currently used index into _rgImages (-1 is none)


#ifdef DEBUG
    BOOL _fStringInit;      // These are used to insure that AddExtButtonsTBArray is only called after
    BOOL _fImageInit;       // AddCustomImag... and AddCustomStrin... have been called.
#endif
};

EXTERN_C const CLSID CLSID_PrivBrowsExtCommands;
typedef enum {
    PBEC_GETSTRINGINDEX     =   1,
} PBEC_COMMANDS;

#endif // _BROWSEXT_H
