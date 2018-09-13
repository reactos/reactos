#ifndef __viewcb_h
#define __viewcb_h


/*-----------------------------------------------------------------------------
/ CMyDocsViewCB
/----------------------------------------------------------------------------*/

class CMyDocsViewCB : public IShellFolderViewCB, CUnknown
{
    private:
        IShellFolderView        * m_psfv;
        IShellFolderViewCB      * m_psfvcb;
        LPITEMIDLIST            m_pidlReal;

    public:
        CMyDocsViewCB(IShellFolderView * psfv, LPITEMIDLIST pidl);
        ~CMyDocsViewCB();
        STDMETHOD(SetRealCB)(IShellFolderViewCB * psfvcb);

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IShellFolderViewCB
        STDMETHOD(MessageSFVCB)(UINT uMsg, WPARAM wParam, LPARAM lParam);

};


#endif
