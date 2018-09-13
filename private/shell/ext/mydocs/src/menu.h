#ifndef __menu_h
#define __menu_h


/*-----------------------------------------------------------------------------
/ CMyDocsContextMenu
/----------------------------------------------------------------------------*/

class CMyDocsContextMenu : public IContextMenu, IContextMenu2, CUnknown
{
    private:
        IShellFolder  *  m_psf;         // real shell folder for this item
        IContextMenu  *  m_pcm;         // real context menu for this item
        IContextMenu2 *  m_pcm2;        // real context menu 2 for this item
        UINT             m_cidl;
        LPITEMIDLIST  *  m_aidl;

    public:
        CMyDocsContextMenu(IShellFolder * psf, UINT cidl, LPCITEMIDLIST * aidl );
        ~CMyDocsContextMenu();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IContextMenu
        STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
        STDMETHODIMP GetCommandString(UINT idCmd, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);

        // IContextMenu2
        STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam );
};


#endif
