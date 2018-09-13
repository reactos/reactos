#ifndef CIconTask_h
#define CIconTask_h

#include <runtask.h>

typedef void (*PFNNSCICONTASKBALLBACK)(LPVOID pvData, UINT_PTR uId, int iIcon, int iOpenIcon, DWORD dwFlags, UINT uSynchId);

class CNscIconTask : public CRunnableTask
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

    CNscIconTask(LPITEMIDLIST pidl, PFNNSCICONTASKBALLBACK pfn, LPVOID pvData, UINT_PTR uId, UINT uSynchId);
    
private:
    virtual ~CNscIconTask();

    LPITEMIDLIST           _pidl;
    PFNNSCICONTASKBALLBACK _pfn;
    LPVOID                 _pvData;
    UINT_PTR               _uId;
    UINT                   _uSynchId;
};

HRESULT AddNscIconTask(IShellTaskScheduler* pts, LPCITEMIDLIST pidl, 
                    PFNNSCICONTASKBALLBACK pfn, LPVOID pvData, UINT_PTR uId, 
                    UINT uSynchId);


#endif
