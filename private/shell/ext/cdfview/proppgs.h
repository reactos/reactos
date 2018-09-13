//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// proppgs.h 
//
//   IShellPropSheetExt menu interface for items.
//
//   History:
//
//       3/26/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _PROPPGS_H_

#define _PROPPGS_H_

//
// Class definition for the item context menu class.
//

class CPropertyPages : public IShellPropSheetExt,
                       public IShellExtInit
{
//
// Methods
//

public:

    // Constructor
    CPropertyPages(void);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IShellPropSheetExt
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    STDMETHODIMP ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage,
                             LPARAM lParam);
    
    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl, LPDATAOBJECT pdobj, HKEY hkey);


private:

    // Destructor.
    ~CPropertyPages(void);

    // Helper functions.

    BOOL OnInitDialog(HWND hdlg);
    BOOL OnCommand(HWND hdlg, WORD wNotifyCode, WORD wID, HWND hwndCtl);
    BOOL OnNotify(HWND hdlg, WPARAM idCtrl, LPNMHDR pnmh);
    void OnDestroy(HWND hdlg);
    void ShowOfflineSummary(HWND hdlg, BOOL bShow);
    void AddRemoveSubsPages(HWND hdlg, BOOL bAdd);
    HRESULT InitializeSubsMgr2();

    static INT_PTR PropSheetDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static UINT PropSheetCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

    inline static CPropertyPages *GetThis(HWND hdlg)
    {
        CPropertyPages *pThis = (CPropertyPages*) GetWindowLongPtr(hdlg, DWLP_USER);

        ASSERT(NULL != pThis);

        return pThis;
    }


//
// Member variables.
//

private:

    ULONG               m_cRef;
    ISubscriptionMgr2*  m_pSubscriptionMgr2;
    IDataObject*        m_pInitDataObject;
    TCHAR               m_szPath[MAX_PATH];
    TCHAR               m_szURL[INTERNET_MAX_URL_LENGTH];
    WORD                m_wHotkey;
    BOOL                m_bStartSubscribed;
};


#endif // _PROPPGS_H_
