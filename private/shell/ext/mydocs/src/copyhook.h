#ifndef __copyhook_h
#define __copyhook_h


/*-----------------------------------------------------------------------------
/ CMyDocsCopyHook
/----------------------------------------------------------------------------*/

class CMyDocsCopyHook : public ICopyHook, CUnknown
{
    private:

    public:
        CMyDocsCopyHook();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // ICopyHook
        STDMETHOD_(UINT,CopyCallback)( HWND hwnd, UINT wFunc, UINT wFlags,
                                       LPCTSTR pszSrcFile, DWORD dwSrcAttribs,
                                       LPCTSTR pszDestFile, DWORD dwDestAttribs
                                      );
};


#endif
