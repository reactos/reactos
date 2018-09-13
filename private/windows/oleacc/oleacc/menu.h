// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  MENU.H
//
//  Menu bar Active Accessibility implementation
//
//	There are four classes here. 
//	CMenu is the class that knows how to deal with menu bar objects. These
//	have children that are CMenuItem objects, or just children (rare - 
//	this is when you have a command right on the menu bar).
//	CMenuItem is something that when you click on it opens a popup.
//	It has 1 child that is a CMenuPopupFrame.
//	CMenuPopupFrame is the HWND that pops up when you click on a CMenuItem. It
//	has 1 child, a CMenuPopup.
//  CMenuPopup objects represent the client area of a CMenuPopupFrame HWND.
//  It has children that are menu items (little m, little i), separators, and 
//	CMenuItems (when you have cascading menus).
//
//	This results in a heirarchy that looks like this:
//	menu bar
//		file menu item
//		edit menu item
//          edit menu popup frame (when droppped down)
//			    edit menu popup 
//				    cut menu item
//				    copy menu item
//      view menu item
//          view menu popup (invisible)
//              this menu item
//              that menu item
//      etc.
//
// -------------------------------------------------------------------------=

extern HRESULT CreateMenuBar(HWND hwnd, BOOL fSysMenu, LONG idCur, REFIID riid, void**ppvMenu);
extern HRESULT CreateMenuItem(IAccessible*, HWND, HMENU, HMENU, long, long, BOOL, REFIID, void**);
extern BOOL    MyGetMenuString( IAccessible * pTheObj, HWND hwnd, HMENU hMenu, long id, BOOL fShell, LPTSTR lpszBuf, UINT cchMax );

//
// This is the CMenuClass. It inherits from CAccessible directly since it is a child
// object of a CWindow object.
//
class CMenu : public CAccessible 
{
    public:
        // IAccessible
        STDMETHODIMP        get_accChild(VARIANT, IDispatch**);

        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accDescription(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        STDMETHODIMP        get_accFocus(VARIANT*);
        STDMETHODIMP        get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP        accSelect(long, VARIANT);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);
        STDMETHODIMP        accDoDefaultAction(VARIANT);

        // IEnumVARIANT
        STDMETHODIMP        Clone(IEnumVARIANT ** ppenum);

        /*CTOR*/            CMenu(HWND, BOOL, long);
        void                SetupChildren(void);
        HMENU               GetMenu(void) {return m_hMenu;}

    protected:
        BOOL                m_fSysMenu;		// TRUE if this is a system menu
        HMENU               m_hMenu;		// the menu handle
};

//
// This is the CMenuItem class. It inherits from CAccessible because it is
// a child of the CMenu object or the CMenuPopup object.
//
class CMenuItem : public CAccessible
{
    public:
        // IAccessible
        STDMETHODIMP        get_accParent(IDispatch** ppdispParent);
        STDMETHODIMP        get_accChild(VARIANT, IDispatch**);

        STDMETHODIMP        get_accName(VARIANT varChild, BSTR* pszName);
        STDMETHODIMP        get_accRole(VARIANT varChild, VARIANT* pvarRole);
        STDMETHODIMP        get_accState(VARIANT varChild, VARIANT* pvarState);
        STDMETHODIMP        get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut);
        STDMETHODIMP        get_accFocus(VARIANT* pvarFocus);
        STDMETHODIMP        get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction);

        STDMETHODIMP        accSelect(long flagsSel, VARIANT varChild);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long x, long y, VARIANT* pvarHit);
        STDMETHODIMP        accDoDefaultAction(VARIANT varChild);

        // IEnumVARIANT
        STDMETHODIMP        Clone(IEnumVARIANT** ppenum);

        /*CTOR*/            CMenuItem(IAccessible*, HWND, HMENU, HMENU, long, long, BOOL);
        /*DTOR*/            ~CMenuItem();
        void                SetupChildren(void);
        HMENU               GetMenu(void) {return m_hMenu;}
		
    protected:
        IAccessible*    m_paccParent;   // Parent menu object
		HMENU			m_hMenu;		// menu we are in
		HMENU			m_hSubMenu;		// hMenu of the popup!
        BOOL            m_fInAPopup;    // TRUE - this item is in a popup. False - in a menu bar
        long            m_ItemID;       // Item we are. This will be like 1..n
};

//
// This is the CMenuPopupFrame class. It inherits from the CWindow class
// because it is a thing that has an HWND. We have to override some of the
// methods because it is not a normal window at all.
// It will create a CMenuPopup inside itself as its only child.
//
class CMenuPopupFrame : public CWindow
{
    public:
        // IAccessible
        STDMETHODIMP    get_accParent(IDispatch ** ppdispParent);
        STDMETHODIMP    get_accChild (VARIANT, IDispatch**);
        STDMETHODIMP    get_accName(VARIANT varChild, BSTR* pszName);
        STDMETHODIMP    accHitTest(long x, long y, VARIANT* pvarHit);
        STDMETHODIMP    get_accFocus(VARIANT* pvarFocus);
        STDMETHODIMP    accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP    accNavigate(long dwNavDir, VARIANT varStart, VARIANT* pvarEnd);

        // IEnumVARIANT
        STDMETHODIMP    Clone(IEnumVARIANT **ppenum);
        STDMETHODIMP    Next(ULONG celt, VARIANT* rgvar, ULONG* pceltFetched);

        /*CTOR*/        CMenuPopupFrame(HWND hwnd,long idChildCur);
        /*DTOR*/        ~CMenuPopupFrame(void);
        void            SetupChildren(void);

    protected:
        long            m_ItemID;       // What is the ID for the parent item that created us?
        HMENU           m_hMenu;        // what hmenu are we?
        HWND            m_hwndParent;   // what hwnd do we descend from?
        BOOL            m_fSonOfPopup;  // are we descended from a popup?
        BOOL            m_fSysMenu;     // are we descended from a sys menu?
};

//
// This is the CMenuPopup class. It inherits from the CClient class because
// it represents the client area of the popup window (HWND type window).
//
class CMenuPopup :  public CClient
{
    public:
        // IAccessible
        STDMETHODIMP        get_accParent(IDispatch** ppdispParent);
        STDMETHODIMP        get_accChild(VARIANT, IDispatch**);

        STDMETHODIMP        get_accName(VARIANT, BSTR*);
        STDMETHODIMP        get_accDescription(VARIANT, BSTR*);
        STDMETHODIMP        get_accRole(VARIANT, VARIANT*);
        STDMETHODIMP        get_accState(VARIANT, VARIANT*);
        STDMETHODIMP        get_accKeyboardShortcut(VARIANT, BSTR*);
        STDMETHODIMP        get_accFocus(VARIANT*);
        STDMETHODIMP        get_accDefaultAction(VARIANT, BSTR*);

        STDMETHODIMP        accSelect(long, VARIANT);
        STDMETHODIMP        accLocation(long*, long*, long*, long*, VARIANT);
        STDMETHODIMP        accNavigate(long, VARIANT, VARIANT*);
        STDMETHODIMP        accHitTest(long, long, VARIANT*);
        STDMETHODIMP        accDoDefaultAction(VARIANT);

        // IEnumVARIANT
        STDMETHODIMP        Clone(IEnumVARIANT** ppenum);

        /*CTOR*/            CMenuPopup(HWND, long);
        /*DTOR*/            ~CMenuPopup(void);
        void                SetupChildren(void);
        void                SetParentInfo(IAccessible* paccParent,HMENU hMenu,long ItemID);
        HMENU               GetMenu(void) {return m_hMenu;}

		
    protected:
        long                m_ItemID;       // what is the id our parent gave us?
        HMENU               m_hMenu;        // what hmenu are we?
        HWND                m_hwndParent;   // what hwnd do we descend from?
        BOOL                m_fSonOfPopup;  // are we descended from a popup?
        BOOL                m_fSysMenu;     // are we descended from a sys menu?
        IAccessible*        m_paccParent;   // only set if we are invisible, so we know our parent
};

//
// Special system HBITMAP values
//
#define MENUHBM_SYSTEM      1
#define MENUHBM_RESTORE     2
#define MENUHBM_MINIMIZE    3
#define MENUHBM_BULLET      4
#define MENUHBM_CLOSE       5
#define MENUHBM_CLOSE_D     6
#define MENUHBM_MINIMIZE_D  7


// --------------------------------------------------------------------------
//
//  SHELL MENU ITEMS (ownerdraw hack-o-rama parsing)
//
//  The Start menu and other context menus that are ownerdraw in the shell
//  have been inaccessible up til now.
//
//  We are going to hack it and fix it.  If we come across an OWNERDRAW menu
//  item in a popup in the shell process, we will get the item data from
//  it and grovel it.  If there is a string pointer, great.  If not, we will
//  grovel the ITEMIDLIST to pull the string out of there.
//
//  NOTE:  Be careful, massive validation and maybe even try-except would 
//  not be a bad idea.
//
//  ALSO:  This needs to work on Win '95 and Nashville.
//
// --------------------------------------------------------------------------

typedef enum
{
    FMI_NULL            = 0x0000,
    FMI_MARKER          = 0x0001,
    FMI_FOLDER          = 0x0002,
    FMI_EXPAND          = 0x0004,
    FMI_EMPTY           = 0x0008,
    FMI_SEPARATOR       = 0x0010,
    FMI_DISABLED        = 0x0020,     // Enablingly Challenged ???
    FMI_ON_MENU         = 0x0040,
    FMI_IGNORE_PIDL     = 0x0080,
    FMI_FILESYSTEM      = 0x0100,
    FMI_MARGIN          = 0x0200,
    FMI_MAXTIPWIDTH     = 0x0400,
    FMI_TABSTOP         = 0x0800,
    FMI_DRAWFLAGS       = 0x1000,
} FILEMENUITEMFLAGS;


//
// In reality, this is a variable structure, with szFriendlyName null-termed
// followed by sz8.3Name null-termed.
//
typedef struct tagITEMIDLIST
{
    SHORT   cbTotal;
    BYTE    aID[12];
    TCHAR   szFriendlyName[1];
} ITEMIDLIST, *LPITEMIDLIST;


// sizeof(cbTotal) is 2 + sizeof(aID) is 12 
#define OFFSET_SZFRIENDLYNAME   14


//
// One of these per file menu.
//
typedef struct
{
    void *psf;                      // Shell Folder.
    HMENU hmenu;                    // Menu.
    LPITEMIDLIST pidlFolder;        // Pidl for the folder.
    DWORD hdpaFMI;                  // List of items (see below).
    UINT idItems;                   // Command.
    UINT fmf;                       // Header flags.
    UINT fFSFilter;                 // file system enum filter
    HBITMAP hbmp;                   // Background bitmap.
    UINT cxBmp;                     // Width of bitmap.
    UINT cyBmp;                     // Height of bitmap.
    UINT cxBmpGap;                  // Gap for bitmap.
    UINT yBmp;                      // Cached Y coord.
    COLORREF clrBkg;                // Background color.
    UINT cySel;                     // Prefered height of selection.
    DWORD pfncb;                    // Callback function.
} FILEMENUHEADER, *PFILEMENUHEADER;

//
// One of these for each file menu item.
//
typedef struct
{
    PFILEMENUHEADER pFMH;           // The header.
    int iImage;                     // Image index to use.
    FILEMENUITEMFLAGS Flags;        // Misc flags above.
    LPITEMIDLIST pidl;              // IDlist for item.
    LPTSTR psz;                     // Text when not using pidls.
} FILEMENUITEM, *PFILEMENUITEM;

