#ifndef _INC_DSKQUOTA_CONTROL_H
#define _INC_DSKQUOTA_CONTROL_H
///////////////////////////////////////////////////////////////////////////////
/*  File: control.h

    Description: Contains declaration for class DiskQuotaControl.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_DSKQUOTA_H
#   include "dskquota.h"
#endif
#ifndef _INC_DSKQUOTA_FSOBJECT_H
#   include "fsobject.h"   // File System object types.
#endif
#ifndef _INC_DSKQUOTA_SIDNAME_H
#   include "sidname.h"    // Sid Name Resolver.
#endif
#ifndef _INC_DSKQUOTA_NTDS_H
#   include "ntds.h"       // For DS versions of LookupAccountSid/Name
#endif
#ifndef _INC_DSKQUOTA_DISPATCH_H
#   include "dispatch.h"   // MIDL-generated header.
#endif
#ifndef _INC_DSKQUOTA_OADISP_H
#   include "oadisp.h"     // OleAutoDispatch class.
#endif


class DiskQuotaControl : public IDiskQuotaControl 
{
    private:
        LONG               m_cRef;                     // Object ref count.
        BOOL               m_bInitialized;             // Controller initialized?
        LONGLONG           m_llDefaultQuotaThreshold;  // "New User" default threshold.
        LONGLONG           m_llDefaultQuotaLimit;      // "New User" default limit.
        FSObject          *m_pFSObject;                // Volume or directory.
        DWORD              m_dwFlags;                  // State of quota system.
        PSID_NAME_RESOLVER m_pSidNameResolver;         // For getting SID account names.
        CMutex             m_mutex;                    // Ensures safe shutdown.

        //
        // Support for IConnectionPointContainer.
        // 1. Static array of supported interface IDs.
        // 2. Array of connection pt interface pointers.
        //    Dynamically grows as clients connect for events.
        //
        static const IID * const m_rgpIConnPtsSupported[];
        PCONNECTIONPOINT  *m_rgConnPts;                // Array of conn pt object ptrs.
        UINT               m_cConnPts;                 // Count of conn pts supported.

        //
        // Create connection point objects for the supported connection
        // point types.
        //
        HRESULT
        InitConnectionPoints(
            VOID);

        //
        // Read quota information from disk to member variables.
        //
        HRESULT
        QueryQuotaInformation(
            VOID);

        //
        // Write quota information from member variables to disk.
        //
        HRESULT
        SetQuotaInformation(
            DWORD dwChangeFlags);

        HRESULT
        GetDefaultQuotaItem(
            PLONGLONG pllItem,
            PLONGLONG pllValueOut);

        //
        // Prevent copy construction.
        //
        DiskQuotaControl(const DiskQuotaControl& control);
        void operator = (const DiskQuotaControl& control);

    public:
        NTDS m_NTDS; 

        //
        // If you add a new connection point type, add a corresponding enumeration
        // member that identifies the location of the conn pt IID in 
        // m_rgpIConnPtsSupported[].
        //
        enum { ConnPt_iQuotaEvents     = 0,
               ConnPt_iQuotaEventsDisp = 1, };

        DiskQuotaControl(VOID);
        ~DiskQuotaControl(VOID);

        HRESULT NotifyUserNameChanged(PDISKQUOTA_USER pUser);

        FSObject *GetFSObjectPtr(VOID)
            { return m_pFSObject; }

        //
        // IUnknown methods.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IConnectionPointContainer methods.
        //
        STDMETHODIMP
        FindConnectionPoint(
            REFIID,
            PCONNECTIONPOINT *pCP);

        STDMETHODIMP
        EnumConnectionPoints(
            PENUMCONNECTIONPOINTS *pEnumCP);

        //
        // IDiskQuotaControl methods.
        //
        STDMETHODIMP
        Initialize(
            LPCWSTR pszFSObjectName,
            BOOL bReadWrite);

        STDMETHODIMP
        GetQuotaState(
            LPDWORD pdwState);   

        STDMETHODIMP
        SetQuotaState(
            DWORD dwState);     

        STDMETHODIMP
        SetQuotaLogFlags(
            DWORD dwFlags);    

        STDMETHODIMP
        GetQuotaLogFlags(
            LPDWORD pdwFlags);    

        STDMETHODIMP 
        SetDefaultQuotaThreshold(
            LONGLONG llThreshold);

        STDMETHODIMP 
        SetDefaultQuotaLimit(
            LONGLONG llLimit);

        STDMETHODIMP 
        GetDefaultQuotaThreshold(
            PLONGLONG pllThreshold);

        STDMETHODIMP
        GetDefaultQuotaThresholdText(
            LPWSTR pszText,
            DWORD cchText);

        STDMETHODIMP 
        GetDefaultQuotaLimit(
            PLONGLONG pllLimit);

        STDMETHODIMP
        GetDefaultQuotaLimitText(
            LPWSTR pszText,
            DWORD cchText);

        STDMETHODIMP 
        AddUserSid(
            PSID pSid, 
            DWORD fNameResolution,
            PDISKQUOTA_USER *ppUser);

        STDMETHODIMP 
        AddUserName(
            LPCWSTR pszLogonName,
            DWORD fNameResolution,
            PDISKQUOTA_USER *ppUser);

        STDMETHODIMP 
        DeleteUser(
            PDISKQUOTA_USER pUser);

        STDMETHODIMP 
        FindUserSid(
            PSID pSid, 
            DWORD fNameResolution,
            PDISKQUOTA_USER *ppUser);

        STDMETHODIMP 
        FindUserName(
            LPCWSTR pszLogonName, 
            PDISKQUOTA_USER *ppUser);

        STDMETHODIMP 
        CreateEnumUsers(
            PSID *rgpSids, 
            DWORD cpSids,
            DWORD fNameResolution,
            PENUM_DISKQUOTA_USERS *ppEnum);

        STDMETHODIMP
        CreateUserBatch(
            PDISKQUOTA_USER_BATCH *ppUserBatch);

        STDMETHODIMP
        InvalidateSidNameCache(
            VOID);

        STDMETHODIMP
        GiveUserNameResolutionPriority(
            PDISKQUOTA_USER pUser);

        STDMETHODIMP
        ShutdownNameResolution(
            VOID);
};



class DiskQuotaControlDisp : public DIDiskQuotaControl 
{
    public:
        DiskQuotaControlDisp(PDISKQUOTA_CONTROL pQC);

        ~DiskQuotaControlDisp(VOID);

        //
        // IUnknown methods.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID riid, 
            LPVOID *ppvOut);

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

        //
        // Automation Properties.
        //
        STDMETHODIMP put_QuotaState(QuotaStateConstants State);
        STDMETHODIMP get_QuotaState(QuotaStateConstants *pState);

        STDMETHODIMP get_QuotaFileIncomplete(VARIANT_BOOL *pbIncomplete);

        STDMETHODIMP get_QuotaFileRebuilding(VARIANT_BOOL *pbRebuilding);

        STDMETHODIMP put_LogQuotaThreshold(VARIANT_BOOL bLogThreshold);
        STDMETHODIMP get_LogQuotaThreshold(VARIANT_BOOL *pbLogThreshold);

        STDMETHODIMP put_LogQuotaLimit(VARIANT_BOOL bLogLimit);
        STDMETHODIMP get_LogQuotaLimit(VARIANT_BOOL *pbLogLimit);

        STDMETHODIMP put_DefaultQuotaThreshold(double Threshold);
        STDMETHODIMP get_DefaultQuotaThreshold(double *pThreshold);
        STDMETHODIMP get_DefaultQuotaThresholdText(BSTR *pThresholdText);

        STDMETHODIMP put_DefaultQuotaLimit(double Limit);
        STDMETHODIMP get_DefaultQuotaLimit(double *pLimit);
        STDMETHODIMP get_DefaultQuotaLimitText(BSTR *pLimitText);

        STDMETHODIMP put_UserNameResolution(UserNameResolutionConstants ResolutionType);
        STDMETHODIMP get_UserNameResolution(UserNameResolutionConstants *pResolutionType);

        //
        // Automation Methods.
        //
        STDMETHODIMP Initialize(
            BSTR path, 
            VARIANT_BOOL bReadOnly);

        STDMETHODIMP AddUser(
            BSTR LogonName,
            DIDiskQuotaUser **ppUser);

        STDMETHODIMP DeleteUser(
            DIDiskQuotaUser *pUser);

        STDMETHODIMP FindUser(
            BSTR LogonName,
            DIDiskQuotaUser **ppUser);

        STDMETHODIMP TranslateLogonNameToSID(
            BSTR LogonName,
            BSTR *psid);

        STDMETHODIMP
            _NewEnum(
                IDispatch **ppEnum);

        STDMETHODIMP InvalidateSidNameCache(void);

        STDMETHODIMP GiveUserNameResolutionPriority(
            DIDiskQuotaUser *pUser);

        STDMETHODIMP ShutdownNameResolution(void);

    private:
        LONG                  m_cRef;                     
        PDISKQUOTA_CONTROL    m_pQC;                      // For delegation
        OleAutoDispatch       m_Dispatch;
        DWORD                 m_fOleAutoNameResolution;
        PENUM_DISKQUOTA_USERS m_pUserEnum;

        //
        // Prevent copy.
        //
        DiskQuotaControlDisp(const DiskQuotaControlDisp& rhs);
        DiskQuotaControlDisp& operator = (const DiskQuotaControlDisp& rhs);
};


#endif  // __DISK_QUOTA_CONTROL_H
