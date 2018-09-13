#ifndef _FSMENU_H
#define _FSMENU_H
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------

#include <objbase.h>

//
// Define API decoration for direct importing of DLL references.
//
#ifndef FSMENUAPI
#if !defined(_FSMENU_)
#define FSSTDAPI          EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#define FSSTDAPI_(type)   EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#else
#define FSSTDAPI          STDAPI
#define FSSTDAPI_(type)   STDAPI_(type)
#endif
#endif // FSMENUAPI

#ifdef __cplusplus
extern "C" {
#endif


#define FMF_NONE            0x00000000
#define FMF_NOEMPTYITEM     0x00000001
#define FMF_INCLUDEFOLDERS  0x00000002
#define FMF_NOPROGRAMS      0x00000004
#define FMF_LARGEICONS      0x00000008
#define FMF_NOBREAK         0x00000010
#define FMF_NOABORT         0x00000020
#define FMF_DELAY_INVALID   0x00000040
#define FMF_RESTRICTHEIGHT  0x00000080   // Restrict height of menu to cyMax
#define FMF_TOOLTIPS        0x00000100   // Call the callback for tooltips
#define FMF_MOREITEMS       0x00000200   // Add a "more items" item
#define FMF_CANORDER        0x00000400   // Order of menu is determined by user
#define FMF_INHERITMASK     0x00000784   // Flags inherited in submenus ;Internal  

#define FMF_DIRTY           0x80000000   // Don't include this in the mask ;Internal

#define FMAI_SEPARATOR      0x00000001

typedef struct tagFMCBDATA
{
    HMENU           hmenu;
    int             iPos;
    LPCITEMIDLIST   pidlFolder;
    LPCITEMIDLIST   pidl;
    IShellFolder *  psf;
    LPVOID          pvHeader;
    UINT            idCmd;
} FMCBDATA;


// Message values for callback
typedef enum
{
    FMM_ADD         = 0,
    FMM_DELETEALL   = 1,
    FMM_REMOVE      = 2,
    FMM_GETTOOLTIP  = 3,
    FMM_GETMORESTRING = 4,
    FMM_GETSTREAM   = 5
} FMM;


typedef struct tagFMMORESTRING
{
    UINT    uID;                    // command ID for the "More" item
    TCHAR   szMoreString[128];      // display string for menu item
} FMMORESTRING, * PFMMORESTRING;


typedef HRESULT (CALLBACK *PFNFMCALLBACK)(FMM fmm, FMCBDATA * pdata, LPARAM lParam);

typedef struct tagFMGETSTREAM
{
    IStream * pstm;
} FMGETSTREAM, * PFMGETSTREAM;


// Structure for a filemenu
typedef struct tagFMDATA
{
    DWORD           cbSize;
    DWORD           dwMask;         // FMD_ flags

    OUT int         cItems;         // Returned

} FMDATA, * PFMDATA;

// Mask values for FMDATA
#define FMD_DEFAULT     0x00000000


FSSTDAPI            FileMenu_InitMenuPopupEx(HMENU hmenu, PFMDATA pfmdata);
FSSTDAPI_(BOOL)     FileMenu_InitMenuPopup(HMENU hmenu);

// Structure for a filemenu item
typedef struct tagFMITEM
{
    DWORD           cbSize;
    DWORD           dwMask;         // FMI_ mask flags
    DWORD           dwType;         // FMIT_ flags
    UINT            uID;            // Command ID of item
    UINT            uItem;          // Position of item in menu
    int             iImage;         // Image 
    LPVOID          pvData;         // Data
    HMENU           hmenuSub;
    UINT            cyItem;
    LPARAM          lParam;         // Application data
} FMITEM, * PFMITEM;

// Mask values for FMITEM
#define FMI_TYPE        0x00000001      // dwType field
#define FMI_ID          0x00000002      // uID field
#define FMI_ITEM        0x00000004      // uItem field
#define FMI_IMAGE       0x00000008      // iImage field
#define FMI_DATA        0x00000010      // pvData field
#define FMI_HMENU       0x00000020      // hmenuSub field
#define FMI_METRICS     0x00000040      // cyItem field
#define FMI_LPARAM      0x00000080      // lParam field

// Type flags for FMITEM.  Refers to, among other things,
// the type of pvData.
#define FMIT_STRING     0x00000001      // string
#define FMIT_SEPARATOR  0x00000002      // separator
#define FMIT_UNICODE    0x80000000      // any string values are unicode

FSSTDAPI            FileMenu_InsertItemEx(HMENU hmenu, UINT iPos, FMITEM const * pfmitem);
FSSTDAPI_(BOOL)     FileMenu_InsertItem(HMENU hmenu, LPTSTR psz, UINT id, int iImage, HMENU hmenuSub, UINT cyItem, UINT iPos);

FSSTDAPI            FileMenu_GetItemInfo(HMENU hmenu, UINT uItem, BOOL bByPos, FMITEM * pfmitem);
FSSTDAPI            FileMenu_GetLastSelectedItem(HMENU hmenu, HMENU * phmenu, UINT * puItem);

FSSTDAPI_(LRESULT)  FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT *lpdi);
FSSTDAPI_(LRESULT)  FileMenu_MeasureItem(HWND hwnd, MEASUREITEMSTRUCT *lpmi);
FSSTDAPI_(UINT)     FileMenu_DeleteAllItems(HMENU hmenu);
FSSTDAPI_(LRESULT)  FileMenu_HandleMenuChar(HMENU hmenu, TCHAR ch);
FSSTDAPI_(BOOL)     FileMenu_GetLastSelectedItemPidls(HMENU hmenu, LPITEMIDLIST *ppidlFolder, LPITEMIDLIST *ppidlItem);
FSSTDAPI_(HMENU)    FileMenu_FindSubMenuByPidl(HMENU hmenu, LPITEMIDLIST pidl);
FSSTDAPI_(void)     FileMenu_Invalidate(HMENU hmenu);
FSSTDAPI_(HMENU)    FileMenu_Create(COLORREF clr, int cxBmpGap, HBITMAP hbmp, int cySel, DWORD fmf);
FSSTDAPI_(BOOL)     FileMenu_AppendItem(HMENU hmenu, LPTSTR psz, UINT id, int iImage, HMENU hmenuSub, UINT cyItem);
FSSTDAPI_(BOOL)     FileMenu_TrackPopupMenuEx(HMENU hmenu, UINT Flags, int x, int y, HWND hwndOwner, LPTPMPARAMS lpTpm);
FSSTDAPI_(BOOL)     FileMenu_DeleteItemByCmd(HMENU hmenu, UINT id);
FSSTDAPI_(void)     FileMenu_Destroy(HMENU hmenu);
FSSTDAPI_(BOOL)     FileMenu_EnableItemByCmd(HMENU hmenu, UINT id, BOOL fEnable);
FSSTDAPI_(BOOL)     FileMenu_DeleteSeparator(HMENU hmenu);
FSSTDAPI_(BOOL)     FileMenu_DeleteMenuItemByFirstID(HMENU hmenu, UINT id);
FSSTDAPI_(DWORD)    FileMenu_GetItemExtent(HMENU hmenu, UINT iItem);
FSSTDAPI_(BOOL)     FileMenu_DeleteItemByIndex(HMENU hmenu, UINT iItem);
FSSTDAPI_(void)     FileMenu_AbortInitMenu(void);
FSSTDAPI_(BOOL)     FileMenu_IsFileMenu(HMENU hmenu);
FSSTDAPI_(BOOL)     FileMenu_CreateFromMenu(HMENU hmenu, COLORREF clr, int cxBmpGap, HBITMAP hbmp, int cySel, DWORD fmf);
FSSTDAPI_(BOOL)     FileMenu_InsertSeparator(HMENU hmenu, UINT iPos);
FSSTDAPI_(BOOL)     FileMenu_GetPidl(HMENU hmenu, UINT iPos, LPITEMIDLIST *ppidl);
FSSTDAPI_(UINT)     FileMenu_AppendFilesForPidl(HMENU hmenu, LPITEMIDLIST pidl, BOOL bInsertSeparator);
FSSTDAPI_(UINT)     FileMenu_AddFilesForPidl(HMENU hmenu, UINT iPos, UINT idNewItems,
                                                    LPITEMIDLIST pidl, DWORD fmf, UINT fMenuFilter, PFNFMCALLBACK pfncb);


// Structure for composing a filemenu
typedef struct tagFMCOMPOSEA
{
    DWORD           cbSize;
    DWORD           dwMask;         // FMC_ flags
    UINT            id;
    DWORD           dwFlags;        // FMF_ flags
    DWORD           dwFSFilter;     // SHCONTF_ flags
    LPITEMIDLIST    pidlFolder;     // Folder to enumerate
    LPSTR           pszFolder;      // Folder to enumerate
    PFNFMCALLBACK   pfnCallback;    // Callback
    UINT            cyMax;          // Max allowable height in pixels
    UINT            cxMax;          // Max allowable width in pixels
    UINT            cySpacing;      // Spacing between menu items in pixels
    LPSTR           pszFilterTypes; // Multi-string list of extensions (e.g., "ext\0doc\0")

    OUT int         cItems;         // Returned
} FMCOMPOSEA;

typedef struct tagFMCOMPOSEW
{
    DWORD           cbSize;
    DWORD           dwMask;         // FMC_ flags
    UINT            id;
    DWORD           dwFlags;        // FMF_ flags
    DWORD           dwFSFilter;     // SHCONTF_ flags
    LPITEMIDLIST    pidlFolder;     // Folder to enumerate
    LPWSTR          pszFolder;      // Folder to enumerate
    PFNFMCALLBACK   pfnCallback;    // Callback
    UINT            cyMax;          // Max allowable height in pixels
    UINT            cxMax;          // Max allowable width in pixels
    UINT            cySpacing;      // Spacing between menu items in pixels
    LPWSTR          pszFilterTypes; // Multi-string list of extensions (e.g., "ext\0doc\0")

    OUT int         cItems;         // Returned
} FMCOMPOSEW;

#ifdef UNICODE
#define FMCOMPOSE   FMCOMPOSEW
#else
#define FMCOMPOSE   FMCOMPOSEA
#endif

// Mask values for FMCOMPOSE
#define FMC_FLAGS       0x00000001
#define FMC_FILTER      0x00000002
#define FMC_PIDL        0x00000004      // Mutually exclusive with FMC_STRING
#define FMC_STRING      0x00000008
#define FMC_CALLBACK    0x00000010
#define FMC_CYMAX       0x00000020
#define FMC_CXMAX       0x00000040
#define FMC_CYSPACING   0x00000080
#define FMC_FILTERTYPES 0x00000100

FSSTDAPI 
FileMenu_ComposeA(
    IN HMENU        hmenu,
    IN UINT         nMethod,
    IN FMCOMPOSEA * pfmc);
FSSTDAPI 
FileMenu_ComposeW(
    IN HMENU        hmenu,
    IN UINT         nMethod,
    IN FMCOMPOSEW * pfmc);

#ifdef UNICODE
#define FileMenu_Compose    FileMenu_ComposeW
#else
#define FileMenu_Compose    FileMenu_ComposeA
#endif

// Method ordinals for FileMenu_Compose
#define FMCM_INSERT     0
#define FMCM_APPEND     1
#define FMCM_REPLACE    2


typedef struct tagFMTOOLTIP
    {
    LPWSTR  pszTip;             // retured tip text, free with LocalFree()
    DWORD   dwMask;
    RECT    rcMargin;           // Margin to add around text
    DWORD   dwMaxWidth;         // Maximum tip width 
    DWORD   dwTabstop;
    UINT    uDrawFlags;
    } FMTOOLTIP;

// Mask flags for FMTOOLTIP 
#define FMTT_MARGIN     0x00000001      // Use the rcMargin field 
#define FMTT_MAXWIDTH   0x00000002      // Use the dwMaxWidth field 
#define FMTT_DRAWFLAGS  0x00000004      // Use the uDrawFlags field 
#define FMTT_TABSTOP    0x00000008      // Use the dwTabstop field 

FSSTDAPI_(BOOL)     FileMenu_ProcessCommand(HWND hwnd, HMENU hmenuBar, UINT idMenu, HMENU hmenu, UINT idCmd);
FSSTDAPI_(BOOL)     FileMenu_HandleMenuSelect(HMENU hmenu, WPARAM wparam, LPARAM lparam);
FSSTDAPI_(BOOL)     FileMenu_HandleNotify(HMENU hmenu, LPCITEMIDLIST * ppidl, LONG lEvent);
FSSTDAPI_(BOOL)     FileMenu_IsUnexpanded(HMENU hmenu);
FSSTDAPI_(void)     FileMenu_DelayedInvalidate(HMENU hmenu);
FSSTDAPI_(BOOL)     FileMenu_IsDelayedInvalid(HMENU hmenu);

FSSTDAPI            FileMenu_SaveOrder(HMENU hmenu);
FSSTDAPI_(void)     FileMenu_EditMode(BOOL bEdit);

#ifdef __cplusplus
}
#endif

#endif //_FSMENU_H
