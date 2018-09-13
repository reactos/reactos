#include "priv.h"
#include "dspsprt.h"
#include "basesb.h"
#include "cnctnpt.h"
#include "stdenum.h"
#include "winlist.h"

class CSDEnumWindows;
#define DM_SWINDOWS 0
#define WM_INVOKE_ON_RIGHT_THREAD   (WM_USER)

class SWData
{
private:
    int m_cRef;

public:
    // Pidl variable is changed on the fly requiring reads/writes to be
    // protected by critical sections. Pid is also changed after creation but
    // only by _EnsurePid. So as long code calls _EnsurePid before reading Pid
    // no critical sections are required to read.
    
    LPITEMIDLIST pidl;
    IDispatch *pid;     // The IDispatch for the item
    long      lCookie;  // The cookie to make sure that the person releasing is the one that added it
    HWND      hwnd;     // The top hwnd, so we can
    DWORD     dwThreadId; // when it is in the pending box...
    BOOL      fActive:1;
    int       swClass;
    
    SWData()
    {
        ASSERT(pid == NULL);
        ASSERT(hwnd == NULL);
        ASSERT(pidl == NULL);
        
        m_cRef = 1;
    }

    ~SWData()
    {
        if (pid)
            pid->Release();
        ILFree(pidl); // null is OK
    }
    
    ULONG AddRef()
    {
        ASSERTCRITICAL;
        
        return m_cRef++;
    }

    ULONG Release()
    {
        ASSERTCRITICAL;
        
        m_cRef--;
        if ( !m_cRef )
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }
};


class CSDWindows : public IShellWindows
                 , public IConnectionPointContainer
                 , protected CImpIDispatch
{
    friend CSDEnumWindows;

    public:
        LONG            m_cRef; //Public for debug checks
        UINT            m_cProcessAttach;


    protected:
        HDPA            m_hdpa;             // DPA to hold information about each window
        HDPA            m_hdpaPending;      // DPA to hold information about pending windows.
        int             m_cRealWindows;     // count of real windows, not callbacks
        int             m_cTickCount;       // used to generate cookies
        HWND            m_hwndHack;
        DWORD           m_dwThreadID;
        SWData* _FindItem(long lCookie);
        SWData* _FindAndRemovePendingItem(HWND hwnd, long lCookie);
        void _EnsurePid(SWData *pswd);
        void _DoInvokeCookie(DISPID dispid, long lCookie, BOOL fCheckThread);
        HRESULT _Item(VARIANT index, IDispatch **ppid, BOOL fRemoveDeadwood);
        static LRESULT CALLBACK s_ThreadNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#ifdef DEBUG
        void _DBDumpList(void);
#endif

        // Embed our Connection Point object - implmentation in cnctnpt.cpp
        CConnectionPoint m_cpWindowsEvents;

    public:
        CSDWindows(void);
        ~CSDWindows(void);

        BOOL         Init(void);

        //IUnknown methods
        STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch members
        virtual STDMETHODIMP GetTypeInfoCount(UINT FAR* pctinfo)
            { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
        virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
            { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
        virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgdispid)
            { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
        virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr)
            { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }


        //IShellWindows methods
        STDMETHODIMP get_WindowPath (BSTR *pbs);
        STDMETHODIMP get_Count(long *plCount);
        STDMETHODIMP Item(VARIANT, IDispatch **ppid);
        STDMETHODIMP _NewEnum(IUnknown **ppunk);


        // *** IConnectionPointContainer ***
        STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum);
        STDMETHODIMP FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT * ppCP);


        // The members to allow new items to be added and removed
        STDMETHODIMP Register(IDispatch *pid, long HWND, int swClass, long *plCookie);
        STDMETHODIMP RegisterPending(long lThreadId, VARIANT* pvarloc, VARIANT* pvarlocRoot, int swClass, long *plCookie);
        STDMETHODIMP Revoke(long lCookie);

        STDMETHODIMP OnNavigate(long lCookie, VARIANT* pvarLoc);
        STDMETHODIMP OnActivated(long lCookie, VARIANT_BOOL fActive);
        STDMETHODIMP FindWindow(VARIANT* varLoc, VARIANT* varlocRoot, int swClass, long * phwnd, int swfwOptions, IDispatch** ppdispAuto);
        STDMETHODIMP OnCreated(long lCookie, IUnknown *punk);
        STDMETHODIMP ProcessAttachDetach(VARIANT_BOOL fAttach);

};

typedef CSDWindows *PCSDWindows;

#ifdef DEBUG // used by DBGetClassSymbolic
extern "C" const int SIZEOF_CSDWindows = SIZEOF(CSDWindows);
#endif

//Enumerator of whatever is held in the collection
class CSDEnumWindows : public IEnumVARIANT
{
    protected:
        LONG        m_cRef;
        CSDWindows  *m_psdw;
        int         m_iCur;

        ~CSDEnumWindows();

    public:
        CSDEnumWindows(CSDWindows *psdw);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IEnumFORMATETC members
        STDMETHODIMP Next(ULONG, VARIANT *, ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset(void);
        STDMETHODIMP Clone(IEnumVARIANT **);
};

STDAPI CSDWindows_CreateInstance(IShellWindows **ppunk)
{
    HRESULT hres = E_OUTOFMEMORY;   // assume failure...
    *ppunk = NULL;

    CSDWindows* psdf = new CSDWindows();
    if (psdf)
    {
        if (psdf->Init())
            hres = psdf->QueryInterface(IID_IShellWindows, (void **)ppunk);
        psdf->Release();
    }
    return hres;
}

CSDWindows::CSDWindows(void) : CImpIDispatch(&IID_IShellWindows)
{
    DllAddRef();
    m_cRef = 1;
    ASSERT(m_hdpa == NULL);
    ASSERT(m_hdpaPending == NULL);
    ASSERT(m_cProcessAttach == 0);

    m_cpWindowsEvents.SetOwner((IUnknown*)SAFECAST(this, IShellWindows*), &DIID_DShellWindowsEvents);
}

int DPA_SWindowsFree(LPVOID p, LPVOID d)
{
    ((SWData*)p)->Release();
    return 1;
}

CSDWindows::~CSDWindows(void)
{
    TraceMsg(DM_SWINDOWS, "sd TR - CSDWindows::~CSDWindows called");
    if (m_hdpa)
    {
        // We need to release the data associated with all of the items in the list
        // as well as release our usage of the interfaces...
        HDPA hdpa = m_hdpa;
        m_hdpa = NULL;

        DPA_DestroyCallback(hdpa, DPA_SWindowsFree, 0);
    }
    if (m_hdpaPending)
    {
        // We need to release the data associated with all of the items in the list
        // as well as release our usage of the interfaces...
        HDPA hdpa = m_hdpaPending;
        m_hdpaPending = NULL;

        DPA_DestroyCallback(hdpa, DPA_SWindowsFree, 0);
    }
    if (m_hwndHack)
        DestroyWindow(m_hwndHack);

    DllRelease();
}


/*
 * CSDWindows::Init
 *
 * Purpose:
 *  Performs any intiailization of a CSDWindows that's prone to failure
 *  that we also use internally before exposing the object outside.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful,
 *                  FALSE otherwise.
 */

BOOL CSDWindows::Init(void)
{
    TraceMsg(DM_SWINDOWS, "sd TR - CSDWindows::Init called");

    m_hdpa = ::DPA_Create(0);
    m_hdpaPending = ::DPA_Create(0);
    m_dwThreadID = GetCurrentThreadId();
    m_hwndHack = SHCreateWorkerWindow(s_ThreadNotifyWndProc, NULL, 0, 0, (HMENU)0, this);

    if (!m_hdpa || !m_hdpaPending || !m_hwndHack)
        return FALSE;

    return TRUE;
}


STDMETHODIMP CSDWindows::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDWindows, IConnectionPointContainer), 
        QITABENT(CSDWindows, IShellWindows),
        QITABENTMULTI(CSDWindows, IDispatch, IShellWindows),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSDWindows::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSDWindows::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


//The IShellWindows implementation

/*
 * CSDWindows::Count
 * Property, read-only
 *
 * The number of figures in this collection
 */

STDMETHODIMP CSDWindows::get_Count(long *plCount)
{
    TraceMsg(DM_SWINDOWS, "sd TR - CSDWindows::Get_Count called");
#ifdef DEBUG
    if (*plCount == -1)
        _DBDumpList();
#endif

    ENTERCRITICAL;
    *plCount = m_cRealWindows;
    LEAVECRITICAL;
    
    return NOERROR;
}

#ifdef DEBUG
void CSDWindows::_DBDumpList(void)
{
    ENTERCRITICAL;
    for (int i = DPA_GetPtrCount(m_hdpa) - 1; i >= 0; i--)
    {
        TCHAR szClass[32];
        SWData* pswd = (SWData*)DPA_FastGetPtr(m_hdpa, i);

        szClass[0] = 0;
        if (IsWindow(pswd->hwnd))
            GetClassName(pswd->hwnd, szClass, ARRAYSIZE(szClass));

        TraceMsg(DM_TRACE, "csdw.dbdl: i=%d hwnd=%x (class=%s) cookie=%d tid=%d IDisp=%x pidl=%x fActive=%u swClass=%d", i,
            pswd->hwnd, szClass, pswd->lCookie, pswd->dwThreadId,
            pswd->pid, pswd->pidl, pswd->fActive, pswd->swClass);
    }
    LEAVECRITICAL;
    return;
}
#endif

/*
 * _EnsurePid
 *
 * Little helper function to ensure that the pid is around and registered.
 * For delay registered guys, this involves calling back to the registered
 * window handle via a private message to tell it to give us a marshalled
 * IDispatch.
 *
 * Callers of _EnusrePid must have pswd addref'ed to ensure it will stay
 * alive.
 */

#define WAIT_TIME 20000

void CSDWindows::_EnsurePid(SWData *pswd)
{
    ENTERCRITICAL;
    IDispatch *pid = pswd->pid;
    LEAVECRITICAL;
    
    if (!pid) 
    {
        ASSERT(pswd->hwnd);

#ifndef NO_MARSHALLING
        // we can not pass a stream between two processes, so we ask 
        // the other process to create a shared memory block with our
        // information in it such that we can then create a stream on it...

        // IDispatch from.  They will CoMarshalInterface their IDispatch
        // into the stream and return TRUE if successful.  We then
        // reset the stream pointer to the head and unmarshal the IDispatch
        // and store it in our list.
        DWORD       dwProcId = GetCurrentProcessId();
        DWORD_PTR   dwResult;

        //
        // Use SendMessageTimeoutA since SendMessageTimeoutW doesn't work on w95.
        if (SendMessageTimeoutA(pswd->hwnd, WMC_MARSHALIDISPATCHSLOW, 0, 
                (LPARAM)dwProcId, SMTO_ABORTIFHUNG, WAIT_TIME, &dwResult) && dwResult)
        {
            // There should be an easier way to get this but for now...
            DWORD cb;
            LPBYTE pv = (LPBYTE)SHLockShared((HANDLE)dwResult, dwProcId);
            
            // Don't know for sure a good way to get the size so assume that first DWORD
            // is size of rest of the area
            if (pv && ((cb = *((DWORD*)pv)) > 0))
            {
                IStream *pIStream;
                if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pIStream))) 
                {
                    const LARGE_INTEGER li = {0, 0};
    
                    pIStream->Write(pv + sizeof(DWORD), cb, NULL);
                    pIStream->Seek(li, STREAM_SEEK_SET, NULL);
                    HRESULT hres = CoUnmarshalInterface(pIStream, IID_IDispatch, (void **)&pid);
                    ASSERT(SUCCEEDED(hres));
                    pIStream->Release();
                }
            }
            SHUnlockShared(pv);
            SHFreeShared((HANDLE)dwResult, dwProcId);
        }
#else
        // UNIX IE has no marshalling capability YET 
        SendMessage(pswd->hwnd, WMC_MARSHALIDISPATCHSLOW, 0, (LPARAM)&(pid));
        // Since we don't use CoMarshal... stuff here we need to increment the
        // reference count.
        pid->AddRef();
#endif

        BOOL bRelease;
        
        ENTERCRITICAL;
        
        if( NULL == pswd->pid )
        {
            pswd->pid = pid;
            bRelease = FALSE;
        }
        else
        {
            // Another thread beat us to it. We don't need the pid
            bRelease = TRUE;
        }
        
        LEAVECRITICAL;

        // Don't want to do this inside the critical section
        if( bRelease )
            pid->Release();
    }
}

struct TMWStruct
{
    SWData * pswd;
    HDPA hdpaWindowList;
    int swClass;
};
typedef struct TMWStruct TMW;
typedef TMW * LPTMW;

BOOL CALLBACK CSDEnumWindowsProc(HWND hwnd, LPARAM lParam /*TYPE: LPTMW*/)
{
    LPTMW lptwm = (LPTMW) lParam;
    int i;
    SWData *pswd;
    BOOL fFound = FALSE;

    // We walk a global hdpa window list, so we better be in a critical section.
    ASSERTCRITICAL;
    
    ASSERT(lptwm && lptwm->hdpaWindowList);
    lptwm->pswd = NULL;
    
    for (i = DPA_GetPtrCount(lptwm->hdpaWindowList) - 1;(i >= 0) && (!fFound); i--)
    {
        pswd = (SWData*)DPA_FastGetPtr(lptwm->hdpaWindowList, i);
        if (pswd->hwnd == hwnd && (lptwm->swClass == -1 || lptwm->swClass == pswd->swClass))
        {
            lptwm->pswd = pswd;
            pswd->AddRef();
            fFound = TRUE;
            break;
        }
    }
    return !fFound;
}

void CSDGetTopMostWindow(TMW* ptmw)
{
    EnumWindows(CSDEnumWindowsProc, (LPARAM)ptmw);
}


/*
 * CSDWindows::_Item
 * Method
 *
 * Just like Item, except caller can specify if error is returned vs window deleted when
 * window is in enumeration list but can't get idispatch.   This permits ::Next
 * operator to skip bad windows, but still return valid ones.
 */

HRESULT CSDWindows::_Item(VARIANT index, IDispatch **ppid, BOOL fRemoveDeadwood)
{
    TMW tmw;
    tmw.swClass = -1;
    tmw.pswd = NULL;
    tmw.hdpaWindowList = m_hdpa;
    *ppid = NULL;

    TraceMsg(DM_SWINDOWS, "sd TR - CSDWindows::Get_Item called");

    // This is sortof gross, but if we are passed a pointer to another variant, simply
    // update our copy here...
    if (index.vt == (VT_BYREF|VT_VARIANT) && index.pvarVal)
        index = *index.pvarVal;


    ASSERT(!(fRemoveDeadwood && index.vt != VT_I2 && index.vt != VT_I4));

Retry:

    switch (index.vt)
    {
    case VT_UI4:
        tmw.swClass = index.ulVal;
        // fall through
        
    case VT_ERROR:
        {
            HWND hwnd = GetActiveWindow();

            if (!hwnd)
                hwnd = GetForegroundWindow();
            if (hwnd)
            {
                ENTERCRITICAL;

                if (!CSDEnumWindowsProc(hwnd, (LPARAM)&tmw)) {
                    ASSERT(tmw.pswd);
                }
                LEAVECRITICAL;
            }
            if (!tmw.pswd)
            {
                ENTERCRITICAL;
                CSDGetTopMostWindow(&tmw);
                LEAVECRITICAL;
            }
        }
        break;

    case VT_I2:
        index.lVal = (long)index.iVal;
        // And fall through...

    case VT_I4:
        if ((index.lVal >= 0))
        {
            ENTERCRITICAL;
            if (index.lVal < DPA_GetPtrCount(m_hdpa))
            {
                tmw.pswd = (SWData*)DPA_GetPtr(m_hdpa, index.lVal);
                tmw.pswd->AddRef();
            }
            LEAVECRITICAL;
        }

        break;
#if 0
    case VT_BSTR:
        break;
#endif

    default:
        return E_INVALIDARG;
    }

    if (tmw.pswd) 
    {
        _EnsurePid(tmw.pswd);
        
        *ppid = tmw.pswd->pid;
        if (tmw.pswd->hwnd && !IsWindow(tmw.pswd->hwnd))
        {
            *ppid = NULL;
        }
        
        if (*ppid)
        {
            (*ppid)->AddRef();
            ENTERCRITICAL;
            tmw.pswd->Release();
            LEAVECRITICAL;
            tmw.pswd = NULL;
            return NOERROR;
        }
        else if (fRemoveDeadwood)
        {
            // In case the window was blown away in a fault we should try to recover...
            // We can only do this if caller is expecting to have item deleted out from
            // under it (see CSDEnumWindows::Next, below)
            Revoke(tmw.pswd->lCookie);
            tmw.swClass = -1;
            ENTERCRITICAL;
            tmw.pswd->Release();
            LEAVECRITICAL;
            tmw.pswd = NULL;
            goto Retry;
        }
        else
        {
            ENTERCRITICAL;
            tmw.pswd->Release();
            LEAVECRITICAL;
            tmw.pswd = NULL;
        }
    }

    return S_FALSE;   // Not a strong error, but a null pointer type of error
}

/*
 * CSDWindows::Item
 * Method
 *
 * This is essentially an array lookup operator for the collection.
 * Collection.Item by itself the same as the collection itself.
 * Otherwise you can refer to the item by index or by path, which
 * shows up in the VARIANT parameter.  We have to check the type
 * of the variant to see if it's VT_I4 (an index) or VT_BSTR (a
 * path) and do the right thing.
 */

STDMETHODIMP CSDWindows::Item(VARIANT index, IDispatch **ppid)
{
    return _Item(index, ppid, FALSE);
}



/*
 * CSDWindows::_NewEnum
 * Method
 *
 * Creates and returns an enumerator of the current list of
 * figures in this collection.
 */

STDMETHODIMP CSDWindows::_NewEnum(IUnknown **ppunk)
{
    *ppunk = new CSDEnumWindows(this);
    return *ppunk ? NOERROR : E_OUTOFMEMORY;
}


// *** IConnectionPointContainer ***

STDMETHODIMP CSDWindows::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT *ppCP)
{
    TraceMsg(DM_SWINDOWS, "ief CSDWindows::%s",TEXT("FindConnectionPoint"));

    if (NULL == ppCP)
        return E_POINTER;

    if (IsEqualIID(iid, DIID_DShellWindowsEvents) ||
        IsEqualIID(iid, IID_IDispatch))
    {
        *ppCP = m_cpWindowsEvents.CastToIConnectionPoint();
    }
    else
    {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }

    (*ppCP)->AddRef();
    return S_OK;
}



STDMETHODIMP CSDWindows::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, m_cpWindowsEvents.CastToIConnectionPoint());
}

void CSDWindows::_DoInvokeCookie(DISPID dispid, long lCookie, BOOL fCheckThread)
{

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.  Note: if we add more notification types we may want to
    // have this function call the equivelent code as is in iedisp code for DoInvokeParam.
    //
    if (m_cpWindowsEvents.IsEmpty())
        return;

    if (fCheckThread && (m_dwThreadID != GetCurrentThreadId()))
    {
        PostMessage(m_hwndHack, WM_INVOKE_ON_RIGHT_THREAD, (WPARAM)dispid, (LPARAM)lCookie);
        return;
    }


    VARIANTARG VarArgList[1] = {0};

    DISPPARAMS dispparams;

    // fill out DISPPARAMS structure
    dispparams.rgvarg = VarArgList;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;

    //
    VarArgList[0].vt = VT_I4;
    VarArgList[0].lVal = lCookie;

    IConnectionPoint_SimpleInvoke(&m_cpWindowsEvents, dispid, &dispparams);
}


STDMETHODIMP CSDWindows::Register(IDispatch *pid, long hwnd, int swClass, long *plCookie)
{
    // Lets allocate a structure for this new window
    if (!plCookie || (hwnd == NULL && swClass != SWC_CALLBACK) || (swClass == SWC_CALLBACK && pid == NULL))
        return E_POINTER;

    BOOL fAllocatedNewItem = FALSE;

    // If the pid isn't specified now (delay register), we'll call back later to
    // get it if we need it.
    if (pid)
        pid->AddRef();

    // We need to be carefull as to not leave a window of opertunity between removing the item from
    // the pending list till it is on the main list or some other thread could open a different window
    // up... Also guard m_hdpa, m_cTickCount, and m_cRealWindows
    // To avoid deadlocks, do not add any callouts to the code below!
    ENTERCRITICAL; 

    SWData *pswd = _FindAndRemovePendingItem((HWND)hwnd, 0);

    // First see if we have
    if (!pswd)
    {
        pswd = new SWData();
        if (!pswd)
        {
            LEAVECRITICAL;
            
            if (pid)
                pid->Release();
            return E_OUTOFMEMORY;
        }

        fAllocatedNewItem = TRUE;
    }

    pswd->pid = pid;
    pswd->swClass = swClass;
    pswd->hwnd = (HWND)hwnd;

    //  Guarantee a non-zero cookie, since 0 is used as a NULL value in
    //  various places (eg shbrowse.cpp)
    if (fAllocatedNewItem)
    {
        m_cTickCount++;
        if (m_cTickCount == 0) 
            m_cTickCount++;
        pswd->lCookie = m_cTickCount;
    }

    if (plCookie)
        *plCookie = pswd->lCookie;

    // Give our refcount to the DPA
    DPA_InsertPtr(m_hdpa, m_cRealWindows, pswd);

    if (hwnd) 
        m_cRealWindows++;

    LEAVECRITICAL;
    
    // We should now notify anyone waiting that there is a window registered...
    _DoInvokeCookie(DISPID_WINDOWREGISTERED, pswd->lCookie, TRUE);

    return NOERROR;
}

STDMETHODIMP CSDWindows::RegisterPending(long lThreadId, VARIANT* pvarloc, VARIANT* pvarlocRoot, int swClass, long *plCookie)
{
    if (plCookie)
        *plCookie = 0;

    SWData *pswd = new SWData();
    if (pswd)
    {
        // pswd is not in any DPA at this point so it is safe to change
        // variables outside of critical section
        
        pswd->swClass = swClass;
        pswd->dwThreadId = (DWORD)lThreadId;
        pswd->pidl = VariantToIDList(pvarloc);
        if (pswd->pidl)
        {
            ASSERT(!pvarlocRoot || pvarlocRoot->vt == VT_EMPTY);

            ENTERCRITICAL; // guards m_hdpa access and m_cTickCount

            //  Guarantee a non-zero cookie, since 0 is used as a NULL value in
            //  various places (eg shbrowse.cpp)
            m_cTickCount++;
            if (m_cTickCount == 0) 
                m_cTickCount++;
            pswd->lCookie = m_cTickCount;
            if (plCookie)
                *plCookie = m_cTickCount;

            // Give our refcount to the DPA
            DPA_AppendPtr(m_hdpaPending, pswd);

            LEAVECRITICAL;

            return NOERROR;     // success
        }

        // Not using this SWData so Release it
        ENTERCRITICAL;
        pswd->Release();
        LEAVECRITICAL;
    }
    return E_OUTOFMEMORY;
}


SWData* CSDWindows::_FindItem(long lCookie)
{
    SWData * pResult = NULL;

    ENTERCRITICAL;
    
    for (int i = DPA_GetPtrCount(m_hdpa) - 1;i >= 0; i--)
    {
        SWData* pswd = (SWData*)DPA_FastGetPtr(m_hdpa, i);
        if (pswd->lCookie == lCookie)
            pResult = pswd;
    }

    if( NULL != pResult )
        pResult->AddRef();
    
    LEAVECRITICAL;

    return pResult;
}

SWData* CSDWindows::_FindAndRemovePendingItem(HWND hwnd, long lCookie)
{
    SWData* pswdRet = NULL; // assume error
    DWORD dwThreadId = hwnd ? GetWindowThreadProcessId(hwnd, NULL) : 0;
    //
    ENTERCRITICAL;
    
    for (int i = DPA_GetPtrCount(m_hdpaPending) - 1;i >= 0; i--)
    {
        SWData* pswd = (SWData*)DPA_FastGetPtr(m_hdpaPending, i);
        if ((pswd->dwThreadId == dwThreadId)  || (pswd->lCookie == lCookie))
        {
            pswdRet = pswd;
            DPA_DeletePtr(m_hdpaPending, i);
            break;
        }
    }
    
    // Since we are both removing the SWData from the pending array (Release)
    // and returning it (AddRef) we can just leave its refcount alone. The
    // caller should release it when they are done with it.
    
    LEAVECRITICAL;
    
    return pswdRet;
}

STDMETHODIMP CSDWindows::Revoke(long lCookie)
{
    SWData *pswd = NULL;    // init to avoid bogus C4701 warning
    int i;
    HRESULT hres = S_FALSE;

    ENTERCRITICAL; // guards m_hdpa and m_cRealWindows
    
    for (i = DPA_GetPtrCount(m_hdpa) - 1;i >= 0; i--)
    {
        pswd = (SWData*)DPA_FastGetPtr(m_hdpa, i);
        if (pswd->lCookie == lCookie)
            break;
    }
    // Remove it from the list while in semaphore...
    if (i >= 0)
    {
        // Since we are deleting the SWData from the array we should not
        // addref it. We are taking the refcount from the array.
        DPA_DeletePtr(m_hdpa, i);
        if (pswd->hwnd) 
            m_cRealWindows--;
    }
    
    LEAVECRITICAL;

    if ((i >= 0) || (pswd = _FindAndRemovePendingItem(NULL, lCookie)))
    {
        // We should now notify anyone waiting that there is a window registered...
        _DoInvokeCookie(DISPID_WINDOWREVOKED, pswd->lCookie, TRUE);

        ENTERCRITICAL;
        pswd->Release();
        LEAVECRITICAL;
        
        hres = NOERROR;
    }

    return hres;

}

STDMETHODIMP CSDWindows::OnNavigate(long lCookie, VARIANT* pvarLoc)
{
    SWData* pswd = _FindItem(lCookie);
    if (pswd)
    {
        ENTERCRITICAL;
        
        ILFree(pswd->pidl);
        pswd->pidl = VariantToIDList(pvarLoc);
        HRESULT hr = pswd->pidl ? S_OK : E_OUTOFMEMORY;
        pswd->Release();
        
        LEAVECRITICAL;
        
        return hr;
    }
    return E_INVALIDARG;
}

STDMETHODIMP CSDWindows::OnActivated(long lCookie, VARIANT_BOOL fActive)
{
    SWData* pswd = _FindItem(lCookie);
    if (pswd) 
    {
        ENTERCRITICAL;
        
        pswd->fActive = (BOOL)fActive;
        pswd->Release();
        
        LEAVECRITICAL;
        
        return S_OK;
    }
    return E_INVALIDARG;
}


STDMETHODIMP CSDWindows::OnCreated(long lCookie, IUnknown *punk)
{
    SWData* pswd = _FindItem(lCookie);
    LPTARGETNOTIFY ptgn;
    HRESULT hr = E_FAIL;

    if (pswd )
    {
        _EnsurePid(pswd);
        if (pswd->pid && SUCCEEDED(pswd->pid->QueryInterface(IID_ITargetNotify, (void **) &ptgn)))
        {
            hr = ptgn->OnCreate(punk, lCookie);
            ptgn->Release();
        }
        
        ENTERCRITICAL;
        pswd->Release();
        LEAVECRITICAL;
    }
    return hr;
}

STDMETHODIMP CSDWindows::FindWindow(VARIANT* pvarLoc, VARIANT* pvarLocRoot, int swClass, long *phwnd, int swfwOptions, IDispatch** ppdispOut)
{
    HRESULT hres = S_FALSE;

    LPCITEMIDLIST pidl = VariantToConstIDList(pvarLoc);
    ASSERT(!pvarLocRoot || pvarLocRoot->vt == VT_EMPTY);

    long lCookie = 0;

    if (!pidl && pvarLoc && (swfwOptions & SWFO_COOKIEPASSED))
    {
        if (pvarLoc->vt == VT_I4)
            lCookie = pvarLoc->lVal;
        else if (pvarLoc->vt == VT_I2)
            lCookie = (LONG)pvarLoc->iVal;
    }

    // The caller may not set the SWFO_NEEDDISPATCH flag, but NULL it out anyway.
    // They are either lazy or at runtime decide if they want the IDispatch
    // (Like WinList_FindFolderWindow).
    if (ppdispOut)
        *ppdispOut = NULL;
    if (swfwOptions & SWFO_NEEDDISPATCH)
    {
        if (!ppdispOut)
            return E_POINTER;
    }
    if (phwnd)
        *phwnd = NULL;

Restart:
    if (pidl || lCookie)
    {
        int i;
        SWData* pswd = NULL;
        LPITEMIDLIST pidlCur = NULL;

        // If no PIDL we will assume an Empty idl...
        if (!pidl)
            pidl = &s_idlNULL;

        if (swfwOptions & SWFO_INCLUDEPENDING)
        {
            for(i=0; TRUE; i++ )
            {
                // NULL is OK
                ILFree( pidlCur );
                pidlCur = NULL;
                
                ENTERCRITICAL;
                
                if( NULL != pswd )
                {
                    pswd->Release();
                    pswd = NULL;
                }

                if( i >= DPA_GetPtrCount(m_hdpaPending) )
                {
                    LEAVECRITICAL;
                    break;
                }

                pswd = (SWData*)DPA_FastGetPtr(m_hdpaPending, i);
                pswd->AddRef();

                // PIDL can change outside of critsect so we must clone it to
                // read it. It may be out of take after this clone but that is
                // OK, at least it won't be partially written
                pidlCur = ILClone(pswd->pidl);
                
                LEAVECRITICAL;

                if (!pswd || (pswd->swClass != swClass))
                    continue;   // try next one...

                if (!(lCookie  && lCookie == pswd->lCookie))
                {
                    if (!ILIsEqual(pidlCur, pidl))
                    {
                        continue;
                    }
                }

                // Found the one, return E_PENDING to say that the open is currently pending
                if (phwnd)
                {
                    *phwnd = pswd->lCookie;   // Something for them to use...
                }


                {
                ENTERCRITICAL;
                pswd->Release();
                pswd = NULL;
                LEAVECRITICAL;
                }
                
                ILFree( pidlCur );
                pidlCur = NULL;
                
                return E_PENDING;
            }
        }

        for(i=0; TRUE; i++ )
        {
        
            // NULL is OK
            ILFree( pidlCur );
            pidlCur = NULL;
            
            ENTERCRITICAL;
            
            if( NULL != pswd )
            {
                pswd->Release();
                pswd = NULL;
            }
            
            if( i >= DPA_GetPtrCount(m_hdpa) )
            {
                LEAVECRITICAL;
                break;
            }

            pswd = (SWData*)DPA_FastGetPtr(m_hdpa, i);
            pswd->AddRef();

            // PIDL can change outside of critsect so we must clone it to
            // read it. It may be out of take after this clone but that is
            // OK, at least it won't be partially written
            pidlCur = ILClone(pswd->pidl);
            
            LEAVECRITICAL;


            if (!pswd || (pswd->swClass != swClass))
                continue;   // try next one...

            if (!(lCookie  && lCookie == pswd->lCookie))
            {
                if (pidl && (!pidlCur || !ILIsEqual(pidlCur, pidl)))
                    continue;
            }

            if (swfwOptions & SWFO_NEEDDISPATCH)
                _EnsurePid(pswd);

            if (phwnd)
            {
                if (pswd->hwnd && !IsWindow(pswd->hwnd))
                {
                    // Incase the window was blown away in a fault we should try to recover...
                    ASSERT(IsWindow(pswd->hwnd));
                    Revoke(pswd->lCookie);
                    
                    ENTERCRITICAL;
                    pswd->Release();
                    pswd = NULL;
                    LEAVECRITICAL;

                    ILFree( pidlCur );
                    pidlCur = NULL;
                    
                    goto Restart;       // gotos SUCK!!!!
                }
                *phwnd = PtrToLong(pswd->hwnd); // windows handles 32b
            }

            if (swfwOptions & SWFO_NEEDDISPATCH)
            {
                if (pswd->pid)
                {
                    *ppdispOut = pswd->pid;
                    pswd->pid->AddRef();
                    hres = S_OK;
                }
            }
            else
            {
                hres = S_OK;
            }

            {
            ENTERCRITICAL;
            pswd->Release();
            pswd = NULL;
            LEAVECRITICAL;
            }
            
            ILFree( pidlCur );
            pidlCur = NULL;
            
            break;
        }
    }

    return hres;
}

HRESULT CSDWindows::ProcessAttachDetach(VARIANT_BOOL fAttach)
{
    if (fAttach)
        m_cProcessAttach++;
    else
    {
        m_cProcessAttach--;
    
        if (m_cProcessAttach == 0)
        {
            // We can now blow away the object in the shell context...
            if (g_dwWinListCFRegister) 
            {
#ifdef DEBUG
                long cwindow;
                get_Count(&cwindow);
                //ASSERT(cwindow==0);
                if (cwindow != 0)
                    TraceMsg(DM_ERROR, "csdw.pad: cwindow=%d (!=0)", cwindow);
#endif
                CoRevokeClassObject(g_dwWinListCFRegister);
                g_dwWinListCFRegister = 0;
            }
        }
    }
    return NOERROR;
}

CSDEnumWindows::CSDEnumWindows(CSDWindows *psdw)
{
    DllAddRef();
    m_cRef = 1;
    m_psdw = psdw;
    m_psdw->AddRef();
    m_iCur=0;
}

CSDEnumWindows::~CSDEnumWindows(void)
{
    DllRelease();
    m_psdw->Release();
}

STDMETHODIMP CSDEnumWindows::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSDEnumWindows, IEnumVARIANT),    // IID_IEnumVARIANT
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) CSDEnumWindows::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSDEnumWindows::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP CSDEnumWindows::Next(ULONG cVar, VARIANT *pVar, ULONG *pulVar)
{
    ULONG       cReturn=0L;
    HRESULT     hr;

    TraceMsg(DM_SWINDOWS, "sd TR - CSDEnumWindows::Next called");
    if (!pulVar)
    {
        if (cVar != 1)
            return E_POINTER;
    }
    else
        *pulVar = 0;

    VARIANT index;
    index.vt = VT_I4;

    while (cVar > 0)
    {
        IDispatch *pid;

        index.lVal = m_iCur++;
        
        hr = m_psdw->_Item(index, &pid, TRUE);            
        if (NOERROR != hr)
            break;

        pVar->pdispVal = pid;
        pVar->vt = VT_DISPATCH;
        pVar++;
        cReturn++;
        cVar--;
    }

    if (NULL != pulVar)
        *pulVar = cReturn;

    return cReturn? NOERROR : S_FALSE;
}


STDMETHODIMP CSDEnumWindows::Skip(ULONG cSkip)
{
    long cItems;
    m_psdw->get_Count(&cItems);

    if ((int)(m_iCur+cSkip) >= cItems)
        return S_FALSE;

    m_iCur+=cSkip;
    return NOERROR;
}


STDMETHODIMP CSDEnumWindows::Reset(void)
{
    m_iCur=0;
    return NOERROR;
}


STDMETHODIMP CSDEnumWindows::Clone(LPENUMVARIANT *ppEnum)
{
    CSDEnumWindows *pNew = new CSDEnumWindows(m_psdw);
    if (pNew)
    {
        *ppEnum = SAFECAST(pNew, IEnumVARIANT *);
        return NOERROR;
    }

    *ppEnum = NULL;
    return E_OUTOFMEMORY;
}

LRESULT CALLBACK CSDWindows::s_ThreadNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSDWindows* pThis = (CSDWindows*)GetWindowLong(hwnd, 0);
    LRESULT lRes = 0L;
    
    if (uMsg < WM_USER)
        return(::DefWindowProc(hwnd, uMsg, wParam, lParam));
    else    
    {
        switch (uMsg) {
        case WM_INVOKE_ON_RIGHT_THREAD:
            pThis->_DoInvokeCookie((DISPID)wParam, (LONG)lParam, FALSE);
            break;
        }
    }
    return lRes;
}    
