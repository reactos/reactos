//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       shrpage.hxx
//
//  Contents:   "Sharing" shell property page extension
//
//  History:    6-Apr-95        BruceFo     Created
//
//--------------------------------------------------------------------------

#ifndef __SHRPAGE_HXX__
#define __SHRPAGE_HXX__

class CShareInfo;

class CSharingPropertyPage
{
    DECLARE_SIG;

public:

    //
    // Page callback function for delete
    //

    static
    UINT CALLBACK
    PageCallback(
        IN HWND hwnd,
        IN UINT uMsg,
        IN LPPROPSHEETPAGE ppsp
        );

    //
    // Main page dialog procedure: static
    //

    static
    INT_PTR CALLBACK
    DlgProcPage(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
        );

    static
    LRESULT CALLBACK
    SizeWndProc(
        IN HWND hwnd,
        IN UINT wMsg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // constructor, destructor, 2nd phase constructor
    //

    CSharingPropertyPage(
        IN PWSTR pszPath,
        IN BOOL bDialog     // called as a dialog, not a property page?
        );

    ~CSharingPropertyPage();

    HRESULT
    InitInstance(
        VOID
        );


    // Does the operating system support caching on this share?
    // This method initializes _bIsCachingSupported, if it is not already true
    bool
    IsCachingSupported(
        VOID
        );

private:

    //
    // Main page dialog procedure: non-static
    //

    BOOL
    _PageProc(
        IN HWND hWnd,
        IN UINT msg,
        IN WPARAM wParam,
        IN LPARAM lParam
        );

    //
    // Window messages and notifications
    //

    BOOL
    _OnInitDialog(
        IN HWND hwnd,
        IN HWND hwndFocus,
        IN LPARAM lInitParam
        );

    BOOL
    _OnCommand(
        IN HWND hwnd,
        IN WORD wNotifyCode,
        IN WORD wID,
        IN HWND hwndCtl
        );

    BOOL
    _OnPermissions(
        IN HWND hwnd
        );

    BOOL
    _OnRemove(
        IN HWND hwnd
        );

    BOOL
    _OnNewShare(
        IN HWND hwnd
        );

    BOOL
    _OnCaching(
        IN HWND hwnd
        );

    BOOL
    _OnNotify(
        IN HWND hwnd,
        IN int idCtrl,
        IN LPNMHDR phdr
        );

    BOOL
    _OnPropertySheetNotify(
        IN HWND hwnd,
        IN LPNMHDR phdr
        );

    //
    // Other helper methods
    //

    VOID
    _InitializeControls(
        IN HWND hwnd
        );

    VOID
    _SetControlsToDefaults(
        IN HWND hwnd
        );

    HRESULT
    _ConstructShareList(
        VOID
        );

    VOID
    _SetFieldsFromCurrent(
        IN HWND hwnd
        );

    VOID
    _CacheMaxUses(
        IN HWND hwnd
        );

    VOID
    _HideControls(
        IN HWND hwnd,
        IN int cShares
        );

    VOID
    _EnableControls(
        IN HWND hwnd,
        IN BOOL bEnable
        );

    BOOL
    _ReadControls(
        IN HWND hwnd
        );

    VOID
    _SetControlsFromData(
        IN HWND hwnd,
        IN PWSTR pszPreferredSelection
        );

    BOOL
    _ValidatePage(
        IN HWND hwnd
        );

    BOOL
    _DoApply(
        IN HWND hwnd
        );

    BOOL
    _DoCancel(
        IN HWND hwnd
        );

    VOID
    _MarkItemDirty(
        VOID
        );

    VOID
    _MarkPageDirty(
        VOID
        );

    HWND
    _GetFrameWindow(
        VOID
        )
    {
        return GetParent(_hwndPage);
    }

#if DBG == 1
    VOID
    Dump(
        IN PWSTR pszCaption
        );
#endif // DBG == 1

    //
    // Private class variables
    //

    PWSTR               _pszPath;
    HWND                _hwndPage;          // HWND to the property page
    BOOL                _fInitializingPage;

    BOOL                _bNewShare;

    BOOL                _bDirty;            // Dirty flag: anything changed?
    BOOL                _bItemDirty;        // Dirty flag: this item changed
    BOOL                _bShareNameChanged;
    BOOL                _bCommentChanged;
    BOOL                _bUserLimitChanged;
    BOOL                _bSecDescChanged;

    CShareInfo*         _pInfoList; // doubly-linked circular w/dummy head node
    CShareInfo*         _pReplaceList;  // list of shares to delete: share names replaced with new shares.
    CShareInfo*         _pCurInfo;
    ULONG               _cShares;   // count of shares, not incl. removed shares

    WORD                _wMaxUsers;

    WNDPROC _pfnAllowProc;

    BOOL    _bDialog;       // called as a dialog, not a property page?

    bool _bIsCachingSupported; // Does the operating system support caching on this share?
};

#endif  // __SHRPAGE_HXX__
