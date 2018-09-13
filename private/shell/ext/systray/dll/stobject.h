#include "stclsid.h"

class CSysTray: public IOleCommandTarget
{
public:
    // IUnknown Implementation
    HRESULT __stdcall QueryInterface(REFIID iid, void** ppvObject);
    ULONG __stdcall AddRef(void);
    ULONG __stdcall Release(void);

    // IOleCommandTarget Implementation
    HRESULT __stdcall QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT* pCmdText);
    HRESULT __stdcall Exec(const GUID* pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG* pvaIn, VARIANTARG* pvaOut);

    CSysTray(BOOL fRunTrayOnConstruct);
    ~CSysTray();

private:
    // Data
    long m_cRef;

private:
    // Functions
    HRESULT CreateSysTrayThread();
    static DWORD WINAPI SysTrayThreadProc(void* lpv);
    HRESULT DestroySysTrayWindow();
};
