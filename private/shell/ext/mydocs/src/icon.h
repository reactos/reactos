#ifndef __icon_h
#define __icon_h


/*-----------------------------------------------------------------------------
/ CMyDocsExtractIcon
/----------------------------------------------------------------------------*/

class CMyDocsExtractIcon : public IExtractIcon, CUnknown
{
    private:
        IShellFolder *    m_psf;    // real shell folder
        IExtractIcon *    m_pei;
        LPITEMIDLIST      m_pidl;
        folder_type       m_type;
#ifdef UNICODE
        BOOL              m_fAnsiExtractIcon;
#endif // UNICODE

    public:
        CMyDocsExtractIcon(IShellFolder * psf, HWND hwndOwner, LPCITEMIDLIST pidl, folder_type type);
        ~CMyDocsExtractIcon();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IExtractIcon
        STDMETHOD(GetIconLocation)(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int* pIndex, UINT* pwFlags);
        STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex, HICON* pLargeIcon, HICON* pSmallIcon, UINT nIconSize);
};


#endif
