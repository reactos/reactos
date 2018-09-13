#ifndef _INC_DSKQUOTA_USER_H
#define _INC_DSKQUOTA_USER_H
///////////////////////////////////////////////////////////////////////////////
/*  File: user.h

    Description: Contains declarations for class DiskQuotaUser.  The
        DiskQuotaUser object represents a user's quota information on a
        particular volume.  Per-user quota information is managed through
        the IDiskQuotaUser interface.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/20/96    Added m_dwID member to DiskQuotaUser.                BrianAu
    09/05/96    Added domain name string and cache.                  BrianAu
    08/25/97    Added OLE automation support.                        BrianAu
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
#ifndef _INC_DSKQUOTA_FSOBJECT_H
#   include "fsobject.h"
#endif
#ifndef _INC_DSKQUOTA_DISPATCH_H
#   include "dispatch.h"   // MIDL-generated header (automation).
#endif
#ifndef _INC_DSKQUOTA_OADISP_H
#   include "oadisp.h"     // OleAutoDispatch class (automation).
#endif


class DiskQuotaUser : public IDiskQuotaUser {
    private:
        LONGLONG      m_llQuotaUsed;
        LONGLONG      m_llQuotaThreshold;
        LONGLONG      m_llQuotaLimit;
        LONG          m_cRef;                 // Ref counter.
        ULONG         m_ulUniqueId;           // Unique object ID.
        PSID          m_pSid;                 // Ptr to user's SID structure.
        LPTSTR        m_pszLogonName;         // "brianau@microsoft.com"
        LPTSTR        m_pszDisplayName;       // "Brian Aust"
        FSObject     *m_pFSObject;            // Ptr to file sys object.
        BOOL          m_bNeedCacheUpdate;     // T = Cached data is invalid.
        INT           m_iContainerName;       // Index into acct container name cache.
        DWORD         m_dwAccountStatus;      // Status of user account.

        static HANDLE m_hMutex;               // For serializing access to users.
        static DWORD  m_dwMutexWaitTimeout;   // How long to wait for mutex.
        static LONG   m_cUsersAlive;          // Count of users currently alive.
        static ULONG  m_ulNextUniqueId;       // Unique ID generator.
        static CArray<CString> m_ContainerNameCache; // Cache container names as they
                                                  // are found.  Don't need to dup
                                                  // names in each user object.
        VOID Destroy(VOID);
        VOID DestroyContainerNameCache(VOID);

        BOOL Lock(VOID);
        VOID ReleaseLock(VOID);

        //
        // Prevent copy construction.
        //
        DiskQuotaUser(const DiskQuotaUser& user);
        void operator = (const DiskQuotaUser& user);

        HRESULT
        GetLargeIntegerQuotaItem(
            PLONGLONG pllItem,
            PLONGLONG pllValueOut);

        HRESULT
        SetLargeIntegerQuotaItem(
            PLONGLONG pllItem,
            LONGLONG llValue,
            BOOL bWriteThrough = TRUE);

        HRESULT
        RefreshCachedInfo(
            VOID);
            
        HRESULT
        WriteCachedInfo(
            VOID);

        HRESULT
        GetCachedContainerName(
            INT iCacheIndex,
            LPTSTR pszContainer,
            UINT cchContainer);

        HRESULT
        CacheContainerName(
            LPCTSTR pszContainer,
            INT *pCacheIndex);
        

    public:
        DiskQuotaUser(FSObject *pFSObject);
        ~DiskQuotaUser(VOID);

        HRESULT 
        Initialize(
            PFILE_QUOTA_INFORMATION pfqi = NULL);

        VOID
        SetAccountStatus(
            DWORD dwStatus);

        STDMETHODIMP
        SetName(
            LPCWSTR pszContainer,
            LPCWSTR pszLogonName,
            LPCWSTR pszDisplayName);

        //
        // IUnknown interface.
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
        // IDiskQuotaUser methods.
        //
        STDMETHODIMP
        GetID(
            ULONG *pulID);

        STDMETHODIMP
        GetName(
            LPWSTR pszContainerBuffer,
            DWORD cchContainerBuffer,
            LPWSTR pszLogonNameBuffer,
            DWORD cchLogonNameBuffer,
            LPWSTR pszDisplayNameBuffer,
            DWORD cchDisplayNameBuffer);

        STDMETHODIMP 
        GetSidLength(
            LPDWORD pcbSid);

        STDMETHODIMP 
        GetSid(
            PBYTE pSid, 
            DWORD cbSidBuf);

        STDMETHODIMP 
        GetQuotaThreshold(
            PLONGLONG pllThreshold)
            {
                return GetLargeIntegerQuotaItem(&m_llQuotaThreshold, 
                                                pllThreshold);
            }

        STDMETHODIMP 
        GetQuotaThresholdText(
            LPWSTR pszText,
            DWORD cchText);

        STDMETHODIMP 
        GetQuotaLimit(
            PLONGLONG pllLimit)
            {
                return GetLargeIntegerQuotaItem(&m_llQuotaLimit, 
                                                pllLimit);
            }

        STDMETHODIMP 
        GetQuotaLimitText(
            LPWSTR pszText,
            DWORD cchText);

        STDMETHODIMP 
        GetQuotaUsed(
            PLONGLONG pllUsed)
            {
                return GetLargeIntegerQuotaItem(&m_llQuotaUsed, 
                                                pllUsed);
            }

        STDMETHODIMP 
        GetQuotaUsedText(
            LPWSTR pszText,
            DWORD cchText);

        STDMETHODIMP
        GetQuotaInformation(
            LPVOID pbInfo,
            DWORD cbInfo);

        STDMETHODIMP 
        SetQuotaThreshold(
            LONGLONG llThreshold,
            BOOL bWriteThrough = TRUE);

        STDMETHODIMP 
        SetQuotaLimit(
            LONGLONG llLimit,
            BOOL bWriteThrough = TRUE);

        STDMETHODIMP
        Invalidate(
            VOID) { m_bNeedCacheUpdate = TRUE;
                    return NO_ERROR; }

        STDMETHODIMP
        GetAccountStatus(
            LPDWORD pdwAccountStatus);

};


//
// Proxy class to handle all automation interface duties.
// It implements IDispatch and DIDiskQuotaUser passing any actions
// for real disk quota activity onto a referenced DiskQuotaUser object.
// Instances are created in DiskQuotaUser::QueryInterface in response
// to requests for IDispatch and DIDiskQuotaUser.
//
class DiskQuotaUserDisp : public DIDiskQuotaUser 
{
    public:

        explicit DiskQuotaUserDisp(PDISKQUOTA_USER pUser);
        ~DiskQuotaUserDisp(VOID);

        //
        // IUnknown interface.
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
        // IDispatch methods.
        //
        STDMETHODIMP
        GetIDsOfNames(
            REFIID riid,  
            OLECHAR ** rgszNames,  
            UINT cNames,  
            LCID lcid,  
            DISPID *rgDispId);

        STDMETHODIMP
        GetTypeInfo(
            UINT iTInfo,  
            LCID lcid,  
            ITypeInfo **ppTInfo);

        STDMETHODIMP
        GetTypeInfoCount(
            UINT *pctinfo);

        STDMETHODIMP
        Invoke(
            DISPID dispIdMember,  
            REFIID riid,  
            LCID lcid,  
            WORD wFlags,  
            DISPPARAMS *pDispParams,  
            VARIANT *pVarResult,  
            EXCEPINFO *pExcepInfo,  
            UINT *puArgErr);

        STDMETHODIMP
        get_ID(
            long *pID);

        STDMETHODIMP
        get_AccountContainerName(
            BSTR *pContainerName);

        STDMETHODIMP 
        get_DisplayName(
            BSTR *pDisplayName);

        STDMETHODIMP 
        get_LogonName(
            BSTR *pLogonName);

        STDMETHODIMP 
        get_QuotaThreshold(
            double *pThreshold);

        STDMETHODIMP 
        put_QuotaThreshold(
            double Threshold);

        STDMETHODIMP 
        get_QuotaThresholdText(
            BSTR *pThresholdText);

        STDMETHODIMP 
        get_QuotaLimit(
            double *pLimit);

        STDMETHODIMP 
        put_QuotaLimit(
            double Limit);

        STDMETHODIMP 
        get_QuotaLimitText(
            BSTR *pLimitText);

        STDMETHODIMP 
        get_QuotaUsed(
            double *pUsed);

        STDMETHODIMP 
        get_AccountStatus(
            AccountStatusConstants *pStatus);

        STDMETHODIMP 
        get_QuotaUsedText(
            BSTR *pUsedText);

        //
        // Methods.
        //
        STDMETHODIMP 
        Invalidate(void);

    private:
        LONG            m_cRef;
        PDISKQUOTA_USER m_pUser;     // For delegation
        OleAutoDispatch m_Dispatch;  // Automation dispatch object.

        //
        // Prevent copy.
        //
        DiskQuotaUserDisp(const DiskQuotaUserDisp& rhs);
        DiskQuotaUserDisp& operator = (const DiskQuotaUserDisp& rhs);
};


#endif // _INC_DISKQUOTA_USER_H
