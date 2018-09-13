//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// bindstcb.h 
//
//   Bind status callback object.
//
//   History:
//
//       3/31/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Check for previous includes of this file.
//

#ifndef _BINDSTCB_H_

#define _BINDSTCB_H_

//
// Class definition for the bind status callback class.
//

class CBindStatusCallback : public IBindStatusCallback
{
//
// Methods
//

public:

    // Constructor
    CBindStatusCallback(IXMLDocument* pIXMLDocument, LPCWSTR pszURLW);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IBindStatusCallback methods.
    STDMETHODIMP GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding* pIBinding);
    STDMETHODIMP GetPriority(LONG *pnPriority);

    STDMETHODIMP OnProgress(ULONG ulProgress,
                            ULONG ulProgressMax,
                            ULONG ulStatusCode,
                            LPCWSTR szStatusText);

    STDMETHODIMP OnDataAvailable(DWORD grfBSCF,
                                 DWORD dwSize,
                                 FORMATETC* pfmtect,
                                 STGMEDIUM* pstgmed);

    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown* pIUnknown);
    STDMETHODIMP OnLowResource(DWORD dwReserved);
    STDMETHODIMP OnStopBinding(HRESULT hrStatus, LPCWSTR szStatusText);

    // Helper functions.

    HRESULT Init(IBindStatusCallback* pPrevIBindStatusCallback);

private:

    // Destructor.
    ~CBindStatusCallback(void);

//
// Member variables.
//

private:

    ULONG                   m_cRef;
    IXMLDocument*           m_pIXMLDocument;
    LPTSTR                  m_pszURL;
    IBindStatusCallback*    m_pPrevIBindStatusCallback;
};



#define DOWNLOAD_PROGRESS  0x9001
#define DOWNLOAD_COMPLETE  0x9002

class CBindStatusCallback2 : public IBindStatusCallback
{
//
// Methods
//

public:

    // Constructor
    CBindStatusCallback2(HWND hwnd);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IBindStatusCallback methods.
    STDMETHODIMP GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding* pIBinding);
    STDMETHODIMP GetPriority(LONG *pnPriority);

    STDMETHODIMP OnProgress(ULONG ulProgress,
                            ULONG ulProgressMax,
                            ULONG ulStatusCode,
                            LPCWSTR szStatusText);

    STDMETHODIMP OnDataAvailable(DWORD grfBSCF,
                                 DWORD dwSize,
                                 FORMATETC* pfmtect,
                                 STGMEDIUM* pstgmed);

    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown* pIUnknown);
    STDMETHODIMP OnLowResource(DWORD dwReserved);
    STDMETHODIMP OnStopBinding(HRESULT hrStatus, LPCWSTR szStatusText);

private:

    // Destructor.
    ~CBindStatusCallback2(void);

//
// Member variables.
//

private:

    ULONG           m_cRef;
    HWND            m_hwnd;
};


#endif _BINDSTCB_H_