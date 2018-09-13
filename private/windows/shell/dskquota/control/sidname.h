#ifndef _INC_DSKQUOTA_SIDNAME_H
#define _INC_DSKQUOTA_SIDNAME_H
///////////////////////////////////////////////////////////////////////////////
/*  File: sidname.h

    Description: Declarations for SID/Name resolver.  See sidname.cpp for
        details of operation.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/12/96    Initial creation.                                    BrianAu
    03/18/98    Replaced "domain", "name" and "full name" with       BrianAu
                "container", "logon name" and "display name" to
                better match the actual contents.  This was in 
                reponse to making the quota UI DS-aware.  The 
                "logon name" is now a unique key as it contains
                both account name and domain-like information.
                i.e. "REDMOND\brianau" or "brianau@microsoft.com".
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_SIDCACHE_H
#   include "sidcache.h"
#endif
#ifndef _INC_DSKQUOTA_USER_H
#   include "user.h"
#endif

//
// This is a PRIVATE interface.  That's why it's here and not in dskquota.h
//
#undef  INTERFACE
#define INTERFACE ISidNameResolver

DECLARE_INTERFACE_(ISidNameResolver, IUnknown)
{
    //
    // ISidNameResolver methods.
    //
    STDMETHOD(Initialize)(THIS) PURE;
    STDMETHOD(FindUserName)(THIS_ PDISKQUOTA_USER) PURE;
    STDMETHOD(FindUserNameAsync)(THIS_ PDISKQUOTA_USER) PURE;
    STDMETHOD(Shutdown)(THIS_ BOOL) PURE;
    STDMETHOD(PromoteUserToQueueHead)(THIS_ PDISKQUOTA_USER) PURE;
};

typedef ISidNameResolver SID_NAME_RESOLVER, *PSID_NAME_RESOLVER;



class DiskQuotaControl;  // Fwd reference.

class SidNameResolver : public ISidNameResolver
{
    private:
        LONG                      m_cRef;
        DiskQuotaControl&         m_rQuotaController;
        HANDLE                    m_hsemQueueNotEmpty;
        HANDLE                    m_hMutex;
        HANDLE                    m_hResolverThread;
        HANDLE                    m_heventResolverThreadReady;
        DWORD                     m_dwResolverThreadId;
        BOOL                      m_bCacheCreationFailed;
        CQueueAsArray<PDISKQUOTA_USER> m_UserQueue;

        //
        // Prevent copying.
        //
        SidNameResolver(const SidNameResolver& );
        operator = (const SidNameResolver& );

        HRESULT
        ResolveSidToName(PDISKQUOTA_USER pUser);

        static DWORD ThreadProc(DWORD dwParam);

        HRESULT
        CreateResolverThread(
            PHANDLE phThread,
            LPDWORD pdwThreadId);

        HRESULT
        ThreadOnQueueNotEmpty(VOID);

        HRESULT
        ClearInputQueue(VOID);

        HRESULT
        FindCachedUserName(
            PDISKQUOTA_USER);

        HRESULT
        GetUserSid(PDISKQUOTA_USER pUser, PSID *ppSid);

        HRESULT
        RemoveUserFromResolverQueue(PDISKQUOTA_USER *ppUser);

        HRESULT
        AddUserToResolverQueue(PDISKQUOTA_USER pUser);

        void Lock(void)
            { WaitForSingleObject(m_hMutex, INFINITE); }

        void ReleaseLock(void)
            { ReleaseMutex(m_hMutex); }

    public:
        SidNameResolver(DiskQuotaControl& rQuotaController);
        ~SidNameResolver(void);

        //
        // IUnknown methods.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID, 
            LPVOID *);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // ISidNameResolver methods.
        //
        STDMETHODIMP
        Initialize(
            VOID);

        STDMETHODIMP
        FindUserName(
            PDISKQUOTA_USER);

        STDMETHODIMP
        FindUserNameAsync(
            PDISKQUOTA_USER);

        STDMETHODIMP
        Shutdown(
            BOOL bWait);

        STDMETHODIMP
        PromoteUserToQueueHead(
            PDISKQUOTA_USER);
};


#endif // _INC_DSKQUOTA_SIDNAME_H

