#ifndef CIconTask_h
#define CIconTask_h

#include <runtask.h>

typedef void (*PFNICONTASKBALLBACK)(LPVOID pvData, UINT uId, UINT iIconIndex);

class CIconTask : public CRunnableTask
{
public:
#if 0   // Needed if we implement multiple interfaces
    // IUnknown methods
    virtual STDMETHODIMP_(ULONG) AddRef(void) 
        { return CRunnableTask::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void) 
        { return CRunnableTask::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
        { return CRunnableTask::QueryInterface(riid, ppvObj); };
#endif

    // IRunnableTask methods (override)
    virtual STDMETHODIMP RunInitRT(void);

    CIconTask(LPITEMIDLIST pidl, PFNICONTASKBALLBACK pfn, LPVOID pvData, UINT uId);
private:
    virtual ~CIconTask();


    LPITEMIDLIST        _pidl;
    PFNICONTASKBALLBACK _pfn;
    LPVOID              _pvData;
    UINT                _uId;
};

// NOTE: If you pass NULL for psf and pidlFolder, you must pass a full pidl which
// the API takes ownership of. (This is an optimization) lamadio - 7.28.98
HRESULT AddIconTask(IShellTaskScheduler* pts, IShellFolder* psf, LPCITEMIDLIST pidlFolder,
                    LPCITEMIDLIST pidl, PFNICONTASKBALLBACK pfn, LPVOID pvData, UINT uId, 
                    int* piTempIcon);


#endif