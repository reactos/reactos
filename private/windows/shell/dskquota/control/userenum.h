#ifndef _INC_DSKQUOTA_USERENUM_H
#define _INC_DSKQUOTA_USERENUM_H
///////////////////////////////////////////////////////////////////////////////
/*  File: userenum.h

    Description: Contains declaration for class DiskQuotaUserEnum.

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
#   include "fsobject.h"
#endif
#ifndef _INC_DSKQUOTA_SIDNAME_H
#   include "sidname.h"
#endif
#ifndef _INC_DSKQUOTA_DISPATCH_H
#   include "dispatch.h"   // MIDL-generated header.
#endif
#ifndef _INC_DSKQUOTA_OADISP_H
#   include "oadisp.h"     // OleAutoDispatch class.
#endif


class DiskQuotaUserEnum : public IEnumDiskQuotaUsers {

    private:
        LONG     m_cRef;            // Object Ref counter.
        LPBYTE   m_pbBuffer;        // For reading disk info.
        LPBYTE   m_pbCurrent;       // Pointer to "current" rec in cache.
        DWORD    m_cbBuffer;        // Size of buffer in bytes.
        PSIDLIST m_pSidList;        // Optional SidList filter.
        DWORD    m_cbSidList;       // Sid list length in bytes.
        BOOL     m_bEOF;            // End of quota info file reached?
        BOOL     m_bSingleUser;     // Single-user enumeration?
        BOOL     m_bInitialized;    // Initialize() already called?
        BOOL     m_bRestartScan;    // Restart NTFS quota file scan?
        DWORD    m_fNameResolution; // None, sync, async
        FSObject *m_pFSObject;      // Pointer to file system object.
        PDISKQUOTA_CONTROL m_pQuotaController; // Ptr to quota controller.
        PSID_NAME_RESOLVER m_pSidNameResolver; // For getting SID account names.

        HRESULT 
        QueryQuotaInformation(
            BOOL bReturnSingleEntry = FALSE,
            PVOID pSidList = NULL,
            ULONG cbSidList = 0,
            PSID pStartSid = NULL,
            BOOL bRestartScan = FALSE);

        HRESULT 
        CreateUserObject(
            PFILE_QUOTA_INFORMATION pfqi, 
            PDISKQUOTA_USER *ppOut);

        HRESULT 
        GetNextUser(
            PDISKQUOTA_USER *ppOut);

        HRESULT
        InitializeSidList(
            PSIDLIST pSidList,
            DWORD cbSidList);

        HRESULT
        InitializeSidList(
            PSID *rgpSids,
            DWORD cpSids);

        //
        // Prevent copy construction.
        //
        DiskQuotaUserEnum(const DiskQuotaUserEnum& UserEnum);
        void operator = (const DiskQuotaUserEnum& UserEnum);

    public:
        DiskQuotaUserEnum(
            PDISKQUOTA_CONTROL pQuotaController,
            PSID_NAME_RESOLVER pSidNameResolver,
            FSObject *pFSObject);

        ~DiskQuotaUserEnum(VOID);

        HRESULT 
        Initialize(
            DWORD fNameResolution, 
            DWORD cbBuffer = 2048, 
            PSID *rgpSids = NULL,
            DWORD cpSids = 0);

        HRESULT 
        Initialize(
            const DiskQuotaUserEnum& UserEnum);

        STDMETHODIMP
        SetNameResolution(
            DWORD fNameResolution);

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
        // IEnumDiskQuotaUsers methods.
        //
        STDMETHODIMP 
        Next(
            DWORD, 
            PDISKQUOTA_USER *, 
            LPDWORD);

        STDMETHODIMP 
        Skip(
            DWORD);

        STDMETHODIMP 
        Reset(
            VOID);

        STDMETHODIMP 
        Clone(
            PENUM_DISKQUOTA_USERS *);
};

//
// Enumerator for VB's "for each" construct.
//
class DiskQuotaUserCollection : public IEnumVARIANT
{
    public:
        DiskQuotaUserCollection(PDISKQUOTA_CONTROL pController,
                                DWORD fNameResolution);

        ~DiskQuotaUserCollection(VOID);

        HRESULT Initialize(VOID);

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
        // IEnumVARIANT Methods.
        //
        STDMETHODIMP
        Next(
            DWORD cUsers,
            VARIANT *rgvar,
            DWORD *pcUsersFetched);

        STDMETHODIMP
        Skip(
            DWORD cUsers);

        STDMETHODIMP
        Reset(
            void);

        STDMETHODIMP
        Clone(
            IEnumVARIANT **ppEnum);

    private:
        LONG                  m_cRef;
        PDISKQUOTA_CONTROL    m_pController;
        PENUM_DISKQUOTA_USERS m_pEnum;
        DWORD                 m_fNameResolution;
};


#endif // _INC_DSKQUOTA_USERENUM_H

