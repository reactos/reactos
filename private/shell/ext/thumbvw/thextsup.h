/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * Declarations of general support routines for view extensions
 */

#ifndef _VWEXTSUP_H
#define _VWEXTSUP_H

// BUGBUG:: place holder, need to move header for dispatch to public place...
#define DISPID_SELECTIONCHANGED     200

// defines for doing wait cursors easily.
UINT GetPidlLength( LPCITEMIDLIST pidl );


#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#ifdef SetWaitCursor
#undef SetWaitCursor
#endif
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursorA(NULL, (LPCSTR) IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

// structure used for sorting a listview..
struct ListViewCompareStruct
{
    LPARAM m_iCompareFlag;
    int m_iAscend;
    LPSHELLFOLDER m_pFolder;
};

// the callback function for using the above structure to sort a listview...
int CALLBACK ListViewCompare( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

// given a listview HWND, it will return a list of PIDLs that are stored in 
// the LPARAMs of the currently selected items.
HRESULT GetSelectionPidlList( HWND hwnd,
                              int cItems,
                              LPCITEMIDLIST * apidl,
                              int iItemHit );

// given a pidl and an IShellFolder to use for the comparison, return the
// ListView item number
int FindInView( HWND hWnd, LPSHELLFOLDER pFolder, LPCITEMIDLIST pidl );

// empty the list view of items, freeing the Pidls associated with the LPARAM
void Clear( HWND hWnd );

void AddMenuSeparatorWithID( HMENU hMenu, int iIndex, UINT iID );
void AddMenuSeparator( HMENU hMenu, int iIndex );
UINT MergeMenus( HMENU hOriginal, HMENU hNew, UINT idStart, UINT idPos );
HMENU LoadPopupMenu( HINSTANCE hDllInst, int iResID );

UINT GetCurColorRes( void );

LPCITEMIDLIST * DuplicateIDArray( LPCITEMIDLIST * apidl, UINT cidl );
LPCITEMIDLIST FindLastPidl(LPCITEMIDLIST pidl);

HRESULT SHCLSIDFromStringA( LPCSTR szCLSID, CLSID * pCLSID );
HRESULT SHStringFromCLSIDA( LPSTR szCLSID, DWORD cSize, REFCLSID rclsid );

#ifdef UNICODE
#define SHCLSIDFromString           CLSIDFromString
#define SHStringFromCLSID(a,b,c)    StringFromGUID2(c, a, b)
#else
#define SHCLSIDFromString           SHCLSIDFromStringA
#define SHStringFromCLSID(a,b,c)    SHStringFromCLSIDA(a,b,c)
#endif


// a handy macro for getting form one part of a pidl to the next.
#define NextPIDL( pidl ) ((LPCITEMIDLIST) (((LPBYTE) pidl) + pidl->mkid.cb ))

// take a simple pidl such as those returned by SHChangeNotify and call ParseDisplayName on
// it to get a real one...
HRESULT SimpleIDLISTToRealIDLIST( IShellFolder * psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST * ppidlReal );
HRESULT Invoke_OnConnectionPointerContainer(IUnknown * punk, REFIID riidCP, DISPID dispidMember
                                            , REFIID riid, LCID lcid, WORD wFlags
                                            , DISPPARAMS * pdispparams, VARIANT * pvarResult
                                            , EXCEPINFO * pexcepinfo, UINT * puArgErr);

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
void _DragMove(HWND hwndTarget, const POINTL ptStart);


#endif
