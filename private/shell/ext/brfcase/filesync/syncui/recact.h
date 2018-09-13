//
// recact.h: Declares data, defines and struct types for RecAct
//                                module.
//
//

#ifndef __RECACT_H__
#define __RECACT_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  DEFINES

// RecAct message ranges
//
#define RAM_FIRST       (WM_USER+1)
#define RAM_LAST        (WM_USER+20)
#define RN_FIRST        (0U-700U)
#define RN_LAST         (0U-799U)

// Window class name
//
#define WC_RECACT       TEXT("RecAction")



// BOOL RecAct_Enable(HWND hwnd, BOOL fEnable);
//
#define RecAct_Enable(hwnd, fEnable) \
        EnableWindow((hwnd), (fEnable))

// int RecAct_GetItemCount(HWND hwnd);
//
#define RAM_GETITEMCOUNT                (RAM_FIRST + 0)
#define RecAct_GetItemCount(hwnd) \
                (int)SendMessage(hwnd, RAM_GETITEMCOUNT, 0, 0L)

// Side item structure
//
#define SI_UNCHANGED    0
#define SI_CHANGED      1
#define SI_NEW          2
#define SI_NOEXIST      3
#define SI_UNAVAILABLE  4
#define SI_DELETED      5

typedef struct tagSIDE_ITEM
    {
    LPTSTR pszDir;
    UINT uState;        // One of SI_* flags
    FILESTAMP fs;
    UINT ichRealPath;   // index to beginning of real path
    } SIDEITEM,  * LPSIDEITEM;

// RecAction Item structure
//
#define RAIF_ACTION      0x0001     // Mask codes
#define RAIF_NAME        0x0002
#define RAIF_STYLE       0x0004
#define RAIF_INSIDE      0x0008
#define RAIF_OUTSIDE     0x0010
#define RAIF_LPARAM      0x0020
#define RAIF_HTWIN       0x0040

#define RAIF_ALL         0x007f


typedef struct tagRA_ITEM
    {
    UINT mask;          // One of RAIF_
    int iItem;
    UINT uStyle;        // One of RAIS_
    UINT uAction;       // One of RAIA_

    LPTSTR pszName;

    SIDEITEM siInside;
    SIDEITEM siOutside;

    LPARAM lParam;
    HTWIN htwin;

    } RA_ITEM,  * LPRA_ITEM;


// RecAct item styles
//
#define RAIS_CANMERGE   0x0001
#define RAIS_FOLDER     0x0002

// RecAct actions
//
#define RAIA_TOOUT      0       // Don't change these values without changing
#define RAIA_TOIN       1       //  the order of the bitmaps in s_rgidAction
#define RAIA_SKIP       2
#define RAIA_CONFLICT   3
#define RAIA_MERGE      4
#define RAIA_SOMETHING  5       // These two require RAIS_FOLDER
#define RAIA_NOTHING    6
#define RAIA_ORPHAN     7
#define RAIA_DELETEOUT  8
#define RAIA_DELETEIN   9
#define RAIA_DONTDELETE 10

// Insert item at specified index.  Item is inserted at end if
// i is greater than or equal to the number of items in the twinview.
// Returns the index of the inserted item, or -1 on error.
//
// int RecAct_InsertItem(HWND hwnd, const LPRA_ITEM pitem);
//
#define RAM_INSERTITEM              (RAM_FIRST + 1)
#define RecAct_InsertItem(hwnd, pitem) \
                (int)SendMessage((hwnd), RAM_INSERTITEM, 0, (LPARAM)(const LPRA_ITEM)(pitem))

// Delete an item at the specified index.
//
// int RecAct_DeleteItem(HWND hwnd, int i);
//
#define RAM_DELETEITEM                  (RAM_FIRST + 2)
#define RecAct_DeleteItem(hwnd, i) \
                (int)SendMessage((hwnd), RAM_DELETEITEM, (WPARAM)(int)(i), 0L)

// Deletes all items in the control
//
// BOOL RecAct_DeleteAllItems(HWND hwnd);
//
#define RAM_DELETEALLITEMS              (RAM_FIRST + 3)
#define RecAct_DeleteAllItems(hwnd) \
                (BOOL)SendMessage((hwnd), RAM_DELETEALLITEMS, 0, 0L)

// BOOL RecAct_GetItem(HWND hwnd, LPRA_ITEM pitem);
//
#define RAM_GETITEM                             (RAM_FIRST + 4)
#define RecAct_GetItem(hwnd, pitem) \
                (BOOL)SendMessage((hwnd), RAM_GETITEM, 0, (LPARAM)(LPRA_ITEM)(pitem))

// BOOL RecAct_SetItem(HWND hwnd, const LPRA_ITEM pitem);
//
#define RAM_SETITEM                             (RAM_FIRST + 5)
#define RecAct_SetItem(hwnd, pitem) \
                (BOOL)SendMessage((hwnd), RAM_SETITEM, 0, (LPARAM)(const LPRA_ITEM)(pitem))

// Get the current selection by index.  -1 if nothing is selected.
//
// int RecAct_GetCurSel(HWND hwnd);
//
#define RAM_GETCURSEL                   (RAM_FIRST + 6)
#define RecAct_GetCurSel(hwnd) \
                (int)SendMessage((hwnd), RAM_GETCURSEL, (WPARAM)0, 0L)

// Set the current selection by index.  -1 to deselect.
//
// int RecAct_SetCurSel(HWND hwnd, int i);
//
#define RAM_SETCURSEL                   (RAM_FIRST + 7)
#define RecAct_SetCurSel(hwnd, i) \
                (int)SendMessage((hwnd), RAM_SETCURSEL, (WPARAM)(i), 0L)

// RecAct_FindItem flags
//
#define RAFI_NAME       0x0001
#define RAFI_LPARAM     0x0002
#define RAFI_ACTION     0x0004

typedef struct tagRA_FINDITEM
    {
    UINT    flags;      // One of RAFI_* flags
    UINT    uAction;    // One of RAIA_* flags
    LPCTSTR  psz;
    LPARAM  lParam;
    
    } RA_FINDITEM;

// Find an item according to RA_FINDITEM struct.  iStart = -1 to
//  start at beginning.
//
// int RecAct_FindItem(HWND hwnd, int iStart, const RA_FINDITEM FAR* prafi);
#define RAM_FINDITEM                            (RAM_FIRST + 8)
#define RecAct_FindItem(hwnd, iStart, prafi) \
                (int)SendMessage((hwnd), RAM_FINDITEM, (WPARAM)(int)(iStart), (LPARAM)(const RA_FINDINFO *)(prafi))

// Refresh the control.
//
// void RecAct_Refresh(HWND hwnd);
#define RAM_REFRESH                             (RAM_FIRST + 9)
#define RecAct_Refresh(hwnd) \
                SendMessage((hwnd), RAM_REFRESH, 0, 0L)


// Notification codes
//
#define RN_SELCHANGED   (RN_FIRST-0)
#define RN_ITEMCHANGED  (RN_FIRST-1)

typedef struct tagNM_RECACT
    {
    NMHDR   hdr;
    int     iItem;
    UINT    mask;           // One of RAIF_* 
    UINT    uAction;        // One of RAIA_*
    UINT    uActionOld;     // One of RAIA_*
    LPARAM  lParam;
    
    } NM_RECACT;

// Window styles
#define RAS_SINGLEITEM  0x0001L


/////////////////////////////////////////////////////  EXPORTED DATA


/////////////////////////////////////////////////////  PUBLIC PROTOTYPES

BOOL PUBLIC RecAct_Init (HINSTANCE hinst);
void PUBLIC RecAct_Term(HINSTANCE hinst);

HRESULT PUBLIC RAI_Create(LPRA_ITEM * ppitem, LPCTSTR pszBrfPath, LPCTSTR pszPath, PRECLIST prl, PFOLDERTWINLIST pftl);
HRESULT PUBLIC RAI_CreateFromRecItem(LPRA_ITEM * ppitem, LPCTSTR pszBrfPath, PRECITEM pri);
HRESULT PUBLIC RAI_Free(LPRA_ITEM pitem);

#endif // __RECACT_H__

