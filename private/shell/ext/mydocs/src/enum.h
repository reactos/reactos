#ifndef __enum_h
#define __enum_h


/*-----------------------------------------------------------------------------
/ CDsEnum
/----------------------------------------------------------------------------*/

#define MAX_DSSHELL_ENUM    64

class CMyDocsEnum : public IEnumIDList, CUnknown
{
    private:
        HWND            m_hwndOwner;                // used for MsgBox etc

        ULONG           m_cFetched;                 // number of items previously fetched

        LPITEMIDLIST    m_pidlRoot;                 // pidl to root of real folder we're enumerating...

        IShellFolder *  m_psf;                      // real shell folder for this folder
        IEnumIDList  *  m_peidl;                    // real enumerator for this folder
        BOOL            m_bRoot;                    // Is this the root MyDocs folder
        HDPA            m_hidl;                     // array of special pidls

    private:
        VOID _GetSpecialItems( HKEY hkey );

    public:
        CMyDocsEnum(IShellFolder * psf, HWND hwndOwner, DWORD grfFlags, LPITEMIDLIST pidlRoot, BOOL bRoot );
        ~CMyDocsEnum();
        VOID DoFirstTimeInitialization( );
        LPITEMIDLIST FindSpecialItem( LPTSTR pName );

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IEnumIDList
        STDMETHODIMP Next(ULONG celt, LPITEMIDLIST* rgelt, ULONG* pceltFetched);
        STDMETHODIMP Skip(ULONG celt);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(LPENUMIDLIST* ppenum);

};


#endif
