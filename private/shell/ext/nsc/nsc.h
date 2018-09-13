// Name Space Control definitions

#define NAME_SPACE_CLASS    "NameSpaceControl"	// window class name

// Window Styles

#define NSS_TREE		0x0000	    // tree control
#define NSS_COMBOBOX		0x0001	    // combo box

#define NSS_SHOWNONFOLDERS	0x0002	    // include non folder things (files)
#define NSS_SHOWHIDDEN		0x0004
#define NSS_ONLYFSDIRS		0x0008	    // For finding a folder to start document searching
#define NSS_DONTGOBELOWDOMAIN	0x0010	    // For starting the Find Computer
#define NSS_RETURNFSANCESTORS	0x0020
#define NSS_DROPTARGET		0x0040	    // register as a drop target
#define NSS_BROWSEFORCOMPUTER	0x4000	    // Browsing for Computers.
#define NSS_BROWSEFORPRINTER	0x8000	    // Browsing for Printers


// structures

typedef DWORD HNAMESPACEITEM;	// handle to a name space item

typedef enum {
    NSIF_HITEM		= 0x0001,
    NSIF_FOLDER		= 0x0002,
    NSIF_PARENTFOLDER	= 0x0004,
    NSIF_IDLIST		= 0x0008,
    NSIF_FULLIDLIST	= 0x0010,
    NSIF_ATTRIBUTES	= 0x0020
} NSI_FLAGS;

typedef struct {
    NSI_FLAGS	    flags;
    HNAMESPACEITEM  hitem;
    IShellFolder    *psf;
    LPCITEMIDLIST   pidl;
    DWORD	    dwAttributes;
} NSC_ITEMINFO;

typedef enum {
    NSSR_ENUMBELOWROOT  = 0x0001,
    NSSR_CREATEPIDL     = 0x0002,
} NSSR_FLAGS;

typedef struct {
    NSSR_FLAGS	    flags;
    IShellFolder    *psf;           // NULL -> desktop shell folder
    LPCITEMIDLIST   pidlRoot;       // PIDL, NULL for desktop, or CSIDL for shell special folder
    int             iExpandDepth;   // how many levels to expand the tree
    LPCITEMIDLIST   pidlExpandTo;   // NULL, or PIDL to expand to
} NSC_SETROOT;

// Window Messages

#define NSM_SETROOT	(WM_USER + 1)

#define NameSpace_SetRoot(hwnd, psr) \
    (BOOL)SendMessage(hwnd, NSM_SETROOT, (WPARAM)0, (LPARAM)psr)

#define NSM_GETIDLIST	(WM_USER + 2)

#define NameSpace_GetIDList(hwnd, hitem) \
    (LPITEMIDLIST)SendMessage(hwnd, NSM_GETPIDL, 0, (WPARAM)hitem)

#define NameSpace_GetFullIDList(hwnd, hitem) \
    (LPITEMIDLIST)SendMessage(hwnd, NSM_GETPIDL, 1, (WPARAM)hitem)

#define NSM_GETITEMINFO	(WM_USER + 3)
#define NameSpace_GetItemInfo(hwnd, hitem, pinfo) \
    (BOOL)SendMessage(hwnd, NSM_GETITEMINFO, (WPARAM)hitem, (LPARAM)pinfo)

#define NSM_FINDITEM	(WM_USER + 4)
#define NameSpace_FindItem(hwnd, pidl, pinfo) \
    (HNAMESPACEITEM)SendMessage(hwnd, NSM_FINDITEM, (WPARAM)pidl, (LPARAM)pinfo)

#define NSM_DOVERB      (WM_USER + 5)
#define NameSpace_DoVerb(hwnd, hitem, pszVerb) \
    (HNAMESPACEITEM)SendMessage(hwnd, NSM_DOVERB, (WPARAM)hitem, (LPARAM)pszVerb)

// WM_NOTIFY codes
#define NSN_FIRST       (0U - 800)

#define NSN_SELCHANGE	(NSN_FIRST - 1)
#define NSN_BEGINDRAG   (NSN_FIRST - 2)
#define NSN_ENDDRAG	(NSN_FIRST - 3)
#define NSN_FILTERITEM	(NSN_FIRST - 4)
#define NSN_PREDOVERB   (NSN_FIRST - 5)
#define NSN_AFTERDOVERB (NSN_FIRST - 6)


// structure in lParam for NSN_FILTERITEM
typedef struct {
    NMHDR	    hdr;
    NSC_ITEMINFO    item;
} NS_NOTIFY;

// private stuff --------------------------------------

// API

BOOL NameSpace_RegisterClass(HINSTANCE hinst);


typedef struct
{
    HWND hwnd;			// window handle of this control
    HWND hwndParent;		// parent window to notify
    HWND hwndTree;		// tree or combo box
    DWORD style;
    UINT flags;			// NSCF_ state bits
    UINT id;			// our control ID
    BOOL fCacheIsDesktop : 1;	// state flags
    BOOL fAutoExpanding : 1;    // tree is auto-expanding

    // HWND hwndNextViewer;	// BUGBUG: implement this
    // HTREEITEM htiCut;
    IContextMenu *pcm;		// context menu currently being displayed

    IShellFolder *psfRoot;

    LPITEMIDLIST pidlRoot;

    HTREEITEM htiCache;		// tree item associated with Current shell folder
    IShellFolder *psfCache;	// cache of the last IShellFolder I needed...

    HTREEITEM htiDragging;      // item being dragged

    ULONG nChangeNotifyID;      // SHChangeNotify registration ID
} NSC;


LPITEMIDLIST _CacheParentShellFolder(NSC *pns, HTREEITEM hti, LPITEMIDLIST pidl);

// nscdrop.c

void CTreeDropTarget_Register(NSC *pns);
void CTreeDropTarget_Revoke(NSC *pns);

