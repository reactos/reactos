//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// chanmenu.h 
//
//   Conext menu interface for items.
//
//   History:
//
//       3/26/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _CHANMENU_H_

#define _CHANMENU_H_

//
// Class definition for the item context menu class.
//

class CChannelMenu : public IContextMenu,
                     public IShellExtInit
{
//
// Methods
//

public:

    // Constructor
    CChannelMenu(void);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IContextMenu methods.
    STDMETHODIMP QueryContextMenu(HMENU hmenu,
                                  UINT indexMenu,
                                  UINT idCmdFirst,
                                  UINT idCmdLast,
                                  UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(UINT_PTR idCommand,
                                  UINT uFLags,
                                  UINT *pwReserved,
                                  LPSTR pszName,
                                  UINT cchMax);

    // ISHelExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl, LPDATAOBJECT pdobj, HKEY hkey);

private:

    // Destructor.
    ~CChannelMenu(void);

    // Helper functions.
    void RemoveMenuItems(HMENU hmenu);
    void Refresh(HWND hwnd);
    void ViewSource(HWND hwnd);
    HRESULT Subscribe(HWND hwnd);


//
// Member variables.
//

private:

    ULONG             m_cRef;
    ISubscriptionMgr* m_pSubscriptionMgr;
    BSTR              m_bstrURL;
    BSTR              m_bstrName;
    TCHAR             m_szPath[MAX_PATH];
    TASK_TRIGGER      m_tt;
    SUBSCRIPTIONINFO  m_si;
};


#endif // _CHANMENU_H_
