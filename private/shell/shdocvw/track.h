
#ifndef TRACK_INC_
#define TRACK_INC_

#include "urltrack.h"

typedef struct _LRecord
{
    struct _LRecord    *pNext;
    LPTSTR              pthisUrl;         // URL name of this document
    DWORD               Context;         // browsing from
    BOOL                fuseCache;
    FILETIME            ftIn;
}LRecord;

class   CUrlTrackingStg : public IUrlTrackingStg
{
public:
     CUrlTrackingStg ();
    ~CUrlTrackingStg (void);

    // IUnknown methods
    virtual STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IUrlTrackingStg methods
    STDMETHODIMP     OnLoad(LPCTSTR lpUrl, BRMODE ContextMode, BOOL fUseCache);
    STDMETHODIMP     OnUnload(LPCTSTR lpUrl);

protected:
    LPINTERNET_CACHE_ENTRY_INFO          QueryCacheEntry (LPCTSTR lpUrl);

    HANDLE           OpenLogFile (LPCTSTR lpFileName);
    HRESULT          UpdateLogFile(LRecord* pNode, SYSTEMTIME* pst);

    LRecord*         AddNode();
    void             DeleteAllNode();
    void             DeleteFirstNode();
    void             DeleteCurrentNode(LRecord *pThis);
    LRecord*         FindCurrentNode(LPCTSTR lpUrl);

    void             cleanup();
    void             ReadTrackingPrefix();
    BOOL             ConvertToPrefixedURL(LPCTSTR lpszUrl, LPTSTR *lplpPrefixedUrl);

    HRESULT          WininetWorkAround(LPCTSTR lpszUrl, LPCTSTR lpOldFile, LPTSTR lpFile);
    void             DetermineAppModule();

private:
    DWORD   _cRef;
            
    HANDLE           _hFile;                // handle to log file
    LRecord         *_pRecords;             // link list of tracked items
    LPTSTR           _lpPfx;    

    BOOL             _fModule:1;
    BOOL             _fScreenSaver:1;
};


#endif // TRACK_INC_

