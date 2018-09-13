//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// itemmenu.h 
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

#ifndef _ITEMMENU_H_

#define _ITEMMENU_H_

//
// Function prototypes.
//

//
// REVIEW: Prototype for DoFileDownloadEx
//         This function is exported from shdocvw, but doesn't have a prototype
//         defined in any .h file.
// 

HRESULT WINAPI DoFileDownloadEx(LPCWSTR pwszURL, BOOL fSaveAs);

//
// Use a flag to conditionally compile code that uses the default context menu
// handler implemented in shell32.dll.
//

#define USE_DEFAULT_MENU_HANDLER        0

#if USE_DEFAULT_MENU_HANDLER

//
//
//

HRESULT CALLBACK MenuCallBack(IShellFolder* pIShellFolder,
                              HWND hwndOwner,
                              LPDATAOBJECT pdtobj,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

#else // USE_DEFAULT_MENU_HANDLER

//
// Defines
//

#define     MAX_PROP_PAGES  5

//
// Function prototypes
//

BOOL CALLBACK AddPages_Callback(HPROPSHEETPAGE hpage, LPARAM ppsh);

//
// Class definition for the item context menu class.
//

class CContextMenu : public IContextMenu2
{
//
// Methods
//

public:

    // Constructor
    CContextMenu(PCDFITEMIDLIST* apcdfidl,
                 LPITEMIDLIST pidlPath, UINT nCount);

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

    // IContextMenu2 methods.
    STDMETHODIMP HandleMenuMsg(UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam);

private:

    // Destructor.
    ~CContextMenu(void);

    // Helper functions.
    HRESULT DoOpen(HWND hwnd, int nShow);
    HRESULT DoOpenFolder(HWND hwnd, int nShow);
    HRESULT DoOpenStory(HWND hwnd, int nShow);
    HRESULT DoProperties(HWND hwnd);

    HRESULT QueryInternetShortcut(PCDFITEMIDLIST pcdfidl, REFIID riid,
                                  void** ppvOut);

//
// Member variables.
//

private:

    ULONG           m_cRef;
    UINT            m_nCount;
    PCDFITEMIDLIST* m_apcdfidl;
    LPITEMIDLIST    m_pidlPath;
};

#endif // USE_DEFAULT_MENU_HANDLER


#endif // _ITEMMENU_H_