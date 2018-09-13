#ifndef _INC_DSKQUOTA_POLICY_H
#define _INC_DSKQUOTA_POLICY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: policy.h

    Description: Header for policy.cpp.
        See policy.cpp for functional description.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/14/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _GPEDIT_H_
#   include <gpedit.h>
#endif
#ifndef _INC_DSKQUOTA_EVENTLOG_H
#   include "eventlog.h"
#endif
#ifndef _INC_DSKQUOTA_REGISTRY_H
#   include "registry.h"
#endif

//
// Structure used for storing and transferring disk quota policy.
//
struct DISKQUOTAPOLICYINFO
{
    LONGLONG llDefaultQuotaThreshold;  // Default user quota threshold (bytes).
    LONGLONG llDefaultQuotaLimit;      // Default user quota limit (bytes).
    DWORD    dwQuotaState;             // NTFS quota state flags.
    DWORD    dwQuotaLogFlags;          // NTFS quota logging flags.
    bool     bRemovableMedia;          // Apply policy to removable media?
};

typedef DISKQUOTAPOLICYINFO *LPDISKQUOTAPOLICYINFO;
typedef const DISKQUOTAPOLICYINFO *LPCDISKQUOTAPOLICYINFO;


#undef INTERFACE
#define INTERFACE IDiskQuotaPolicy
DECLARE_INTERFACE_(IDiskQuotaPolicy, IUnknown)
{
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    // IDiskQuotaPolicy methods
    //
    STDMETHOD(Initialize)(THIS_ LPGPEINFORMATION pGPEInfo, HKEY hkeyRoot) PURE;
#ifdef POLICY_MMC_SNAPIN
    STDMETHOD(Save)(THIS_ LPCDISKQUOTAPOLICYINFO pInfo) PURE;
#endif
    STDMETHOD(Load)(THIS_ LPDISKQUOTAPOLICYINFO pInfo) PURE;
    STDMETHOD(Apply)(THIS_ LPCDISKQUOTAPOLICYINFO pInfo) PURE;
};
typedef IDiskQuotaPolicy *LPDISKQUOTAPOLICY;


//
// Class for saving/loading/applying disk quota policy information.
// Used by both the MMC policy snapin (server) and the GPE extension (client).
//
class CDiskQuotaPolicy : public IDiskQuotaPolicy
{
    public:
        explicit CDiskQuotaPolicy(LPGPEINFORMATION pGPEInfo = NULL, HKEY hkeyRoot = NULL, bool bVerboseEventLog = false, BOOL *pbAbort = NULL);
        ~CDiskQuotaPolicy(void);

        //
        // IUnknown interface.
        //
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //
        // IDiskQuotaPolicy interface.
        //
        STDMETHODIMP Initialize(LPGPEINFORMATION pGPEInfo, HKEY hkeyRoot);
#ifdef POLICY_MMC_SNAPIN
        STDMETHODIMP Save(const DISKQUOTAPOLICYINFO *pPolicyInfo);
#endif
        STDMETHODIMP Load(DISKQUOTAPOLICYINFO *pPolicyInfo);
        STDMETHODIMP Apply(const DISKQUOTAPOLICYINFO *pPolicyInfo);

    private:
        LONG             m_cRef;
        LPGPEINFORMATION m_pGPEInfo;
        HKEY             m_hkeyRoot;
        BOOL            *m_pbAbort;            // Monitor to detect abort.
        bool             m_bRootKeyOpened;     // Need to close root key?
        bool             m_bVerboseEventLog;   // Verbose log output?

        static const TCHAR m_szRegKeyPolicy[];
#ifdef POLICY_MMC_SNAPIN
        static const TCHAR m_szRegValPolicy[];
#endif
        static HRESULT GetDriveNames(CArray<CString> *prgstrDrives, bool bRemovableMedia);
        static HRESULT OkToApplyPolicy(LPCTSTR pszDrive, bool RemovableMedia);
        static void InitPolicyInfo(LPDISKQUOTAPOLICYINFO pInfo);
        static void LoadPolicyInfo(const RegKey& key, LPDISKQUOTAPOLICYINFO pInfo);
};


#endif // _INC_DSKQUOTA_POLICY_H


