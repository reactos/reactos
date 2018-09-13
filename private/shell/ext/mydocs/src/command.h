#ifndef __command_h
#define __command_h


/*-----------------------------------------------------------------------------
/ CMyDocsCommand
/----------------------------------------------------------------------------*/

class CMyDocsCommand : public IOleCommandTarget, CUnknown
{
    private:

    public:
        CMyDocsCommand();
        ~CMyDocsCommand();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IOleCommandTarget
        STDMETHOD(QueryStatus)(const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[],OLECMDTEXT *pCmdText);
        STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
};


#endif
