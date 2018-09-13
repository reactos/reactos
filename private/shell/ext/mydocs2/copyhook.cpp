#include "precomp.hxx"
#pragma hdrstop

#include "util.h"
#include "dll.h"
#include "resource.h"

class CMyDocsCopyHook : public ICopyHook
{
public:

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvect);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ICopyHook
    STDMETHOD_(UINT,CopyCallback)(HWND hwnd, UINT wFunc, UINT wFlags,
                                  LPCTSTR pszSrcFile, DWORD dwSrcAttribs,
                                  LPCTSTR pszDestFile, DWORD dwDestAttribs);
private:
    CMyDocsCopyHook();
    ~CMyDocsCopyHook();

    LONG _cRef;

    friend HRESULT CMyDocsCopyHook_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);    // for ctor
};

STDMETHODIMP CMyDocsCopyHook::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CMyDocsCopyHook, ICopyHook),    // IID_ICopyHook
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CMyDocsCopyHook::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) CMyDocsCopyHook::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

CMyDocsCopyHook::CMyDocsCopyHook() : _cRef(1)
{
    DllAddRef();
}

CMyDocsCopyHook::~CMyDocsCopyHook()
{
    DllRelease();
}

// ICopyHook methods
UINT CMyDocsCopyHook::CopyCallback(HWND hwnd, UINT wFunc, UINT wFlags,
                                   LPCTSTR pszSrcFile,  DWORD dwSrcAttribs,
                                   LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
    UINT uRes = IDYES;

    if ((wFunc == FO_COPY) || (wFunc == FO_MOVE))
    {
        TCHAR szPersonal[MAX_PATH];

        if (S_OK == SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPersonal) &&
            lstrcmpi(pszSrcFile, szPersonal) == 0)
        {
            // the source is the personal directory, now check if the
            // destination is on the desktop...
            DWORD dwRes = IsPathGoodMyDocsPath(hwnd, pszDestFile);

            if (dwRes == PATH_IS_NONEXISTENT)
            {
                lstrcpyn(szPersonal, pszDestFile, ARRAYSIZE(szPersonal));
                if (PathRemoveFileSpec(szPersonal))
                {
                    dwRes = IsPathGoodMyDocsPath(hwnd, szPersonal);
                }
            }

            if (dwRes == PATH_IS_DESKTOP)
            {
                // keep the user from moving the personal folder to the desktop
                TCHAR szVerb[ 32 ];
                LPTSTR pName = PathFindFileName(pszSrcFile);

                LoadString(g_hInstance, (wFunc == FO_COPY) ? IDS_COPY : IDS_MOVE, szVerb, ARRAYSIZE(szVerb));

                uRes = IDNO;

                GetMyDocumentsDisplayName(szPersonal, ARRAYSIZE(szPersonal));

                if (IsMyDocsHidden())
                {
                    if ( IDYES == ShellMessageBox(g_hInstance, hwnd,
                                                  (LPTSTR)IDS_NODRAG_DESKTOP_HIDDEN, szPersonal,
                                                  MB_YESNO | MB_ICONQUESTION,
                                                  szPersonal, szVerb))
                    {
                        RestoreMyDocsFolder();
                    }
                }
                else
                {
                    ShellMessageBox(g_hInstance, hwnd,
                                    (LPTSTR)IDS_NODRAG_DESKTOP_NOT_HIDDEN, szPersonal,
                                    MB_OK | MB_ICONSTOP, szPersonal, szVerb);
                }
            }
        }
    }
    return uRes;
}

HRESULT CMyDocsCopyHook_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    CMyDocsCopyHook * pMDCH = new CMyDocsCopyHook();
    if (pMDCH)
    {
        *ppunk = SAFECAST(pMDCH, ICopyHook*);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}
