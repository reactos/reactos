//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// cdfview.h 
//
//   The class definition for the cdf view class.  This class implements the
//   IShelFolder interface.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _CDFVIEW_H_

#define _CDFVIEW_H_

//
// Function prototypes.
//

HRESULT QueryInternetShortcut(LPCTSTR pszURL, REFIID riid, void** ppvOut);

HRESULT QueryInternetShortcut(PCDFITEMIDLIST pcdfidl, REFIID riid,
                              void** ppvOut);


//
// Class definition for the cdf view class.
//

class CCdfView : public IShellFolder,
                 public CPersist
{
//
// Methods
//

public:

    // Constructors
    CCdfView(void);
    CCdfView(PCDFITEMIDLIST pcdfidl,
             LPCITEMIDLIST pidlParentPath,
             IXMLElementCollection* pIXMLElementCollection);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IShellFolder
    STDMETHODIMP ParseDisplayName(HWND hwndOwner,
                                  LPBC pbcReserved,
                                  LPOLESTR lpszDisplayName,
                                  ULONG* pchEaten,
                                  LPITEMIDLIST* ppidl,
                                  ULONG* pdwAttributes);

    STDMETHODIMP EnumObjects(HWND hwndOwner,
                             DWORD grfFlags,
                             LPENUMIDLIST* ppenumIDList);

    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl,
                              LPBC pbcReserved,
                              REFIID riid,
                              LPVOID* ppvOut);

    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl,
                               LPBC pbcReserved,
                               REFIID riid,
                               LPVOID* ppvObj);

    STDMETHODIMP CompareIDs(LPARAM lParam,
                            LPCITEMIDLIST pidl1,
                            LPCITEMIDLIST pidl2);

    STDMETHODIMP CreateViewObject(HWND hwndOwner,
                                  REFIID riid,
                                  LPVOID* ppvOut);

    STDMETHODIMP GetAttributesOf(UINT cidl,
                                 LPCITEMIDLIST* apidl,
                                 ULONG* pfAttributesOut);

    STDMETHODIMP GetUIObjectOf(HWND hwndOwner,
                               UINT cidl,
                               LPCITEMIDLIST* apidl,
                               REFIID riid,
                               UINT* prgfInOut,
                               LPVOID * ppvOut);

    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl,
                                  DWORD uFlags,
                                  LPSTRRET lpName);

    STDMETHODIMP SetNameOf(HWND hwndOwner,
                           LPCITEMIDLIST pidl,
                           LPCOLESTR lpszName,
                           DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

private:

    // Destructor
    ~CCdfView(void);

    // Parsing helper functions.
    HRESULT  ParseCdfFolder(HWND hwndOwner, DWORD dwParseFlags);

    // Folder helper functions.

//
// Member variables.
//

private:

    ULONG                   m_cRef;
    PCDFITEMIDLIST          m_pcdfidl;     // This folder's pidl
    LPITEMIDLIST            m_pidlPath;    // Path to this folder.
    IXMLElementCollection*  m_pIXMLElementCollection;
    BOOL                    m_fIsRootFolder; // Is this the root folder.
};


#endif _CDFVIEW_H_
