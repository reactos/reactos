#ifndef _INC_DSKQUOTA_FSOBJECT_H
#define _INC_DSKQUOTA_FSOBJECT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: fsobject.h

    Description: Contains declarations for file system objects used in the
        quota management library.  Abstractions are provided for NTFS
        volumes, directories and local/remote versions of both.  The idea
        is to hide any peculiarities of these variations behind a common
        FSObject interface.

        The holder of a pointer to an FSObject can call the member functions
        IsLocal() and Type() to determine the exact type and locality of 
        the object.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////


class FSObject 
{
    private:
        LONG   m_cRef;              // Instance ref counter.

        //
        // Prevent copy construction.
        //
        FSObject(const FSObject& obj);
        void operator = (const FSObject& obj);

    protected:
        CPath m_strFSObjName;
        DWORD  m_dwAccessRights;    // Access rights granted to client.
                                    // 0                           = None.
                                    // GENERIC_READ                = Read
                                    // GENRIC_READ | GENERIC_WRITE = Read/Write.

        static HRESULT HResultFromNtStatus(NTSTATUS status);

    public:
        //
        // Types of FS Objects.
        //
        enum { TypeUnknown, Volume, Directory };

        //
        // Flags used to indicate what data is to be updated in calls to 
        // SetObjectQuotaInformation() and SetUserQuotaInformation().
        //
        enum {
                ChangeState     = 0x01,
                ChangeLogFlags  = 0x02,
                ChangeThreshold = 0x04,
                ChangeLimit     = 0x08
             };

        FSObject(LPCTSTR pszObjName)
            : m_cRef(0),
              m_dwAccessRights(0),
              m_strFSObjName(pszObjName)
              { DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSObject::FSObject"))); }

        virtual ~FSObject(void);

        ULONG AddRef(VOID);
        ULONG Release(VOID);

        //
        // Pure virtual interface for opening volume/directory.
        //
        virtual HRESULT Initialize(DWORD dwAccess) = 0;

        static HRESULT
        Create(
            LPCTSTR pszFSObjName,
            DWORD dwAccess,
            FSObject **ppNewObject);

        static HRESULT
        Create(
            const FSObject& obj,
            FSObject **ppNewObject);

        static HRESULT 
        ObjectSupportsQuotas(
            LPCTSTR pszFSObjName);

        HRESULT GetName(LPTSTR pszBuffer, ULONG cchBuffer) const;

        virtual HRESULT QueryUserQuotaInformation(
                            PVOID pBuffer,
                            ULONG cBufferLength,
                            BOOL bReturnSingleEntry,
                            PVOID pSidList,
                            ULONG cSidListLength,
                            PSID  pStartSid,
                            BOOL  bRestartScan
                            ) = 0;
    
        virtual HRESULT SetUserQuotaInformation(
                            PVOID pBuffer,
                            ULONG cBufferLength
                            ) const = 0;

        virtual HRESULT QueryObjectQuotaInformation(
                            PDISKQUOTA_FSOBJECT_INFORMATION poi
                            ) = 0;

        virtual HRESULT SetObjectQuotaInformation(
                            PDISKQUOTA_FSOBJECT_INFORMATION poi,
                            DWORD dwChangeMask
                            ) const = 0;

        virtual BOOL IsLocal(VOID) const = 0;
        virtual UINT Type(VOID) const = 0;

        DWORD GetAccessRights(VOID) const
            { return m_dwAccessRights; }

        BOOL GrantedAccess(DWORD dwAccess) const
            { return (m_dwAccessRights & dwAccess) == dwAccess; }
};


class FSVolume : public FSObject
{
    private:
        //
        // Prevent copying.
        //
        FSVolume(const FSVolume&);
        void operator = (const FSVolume&);

    protected:
        HANDLE m_hVolume;

    public:
        FSVolume(LPCTSTR pszVolName)
            : FSObject(pszVolName),
              m_hVolume(NULL) 
              { DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::FSVolume"))); }

        virtual ~FSVolume(void);

        HRESULT Initialize(DWORD dwAccess);

        UINT Type(VOID) const
            { return FSObject::Volume; }


        virtual HRESULT QueryObjectQuotaInformation(
                            PDISKQUOTA_FSOBJECT_INFORMATION poi
                            );


        virtual HRESULT SetObjectQuotaInformation(
                            PDISKQUOTA_FSOBJECT_INFORMATION poi,
                            DWORD dwChangeMask
                            ) const;

        virtual HRESULT QueryUserQuotaInformation(
                            PVOID pBuffer,
                            ULONG cBufferLength,
                            BOOL bReturnSingleEntry,
                            PVOID pSidList,
                            ULONG cSidListLength,
                            PSID  pStartSid,
                            BOOL  bRestartScan
                            );
    
        virtual HRESULT SetUserQuotaInformation(
                            PVOID pBuffer,
                            ULONG cBufferLength
                            ) const;
};

class FSLocalVolume : public FSVolume
{
    private:
        //
        // Prevent copying.
        //
        FSLocalVolume(const FSLocalVolume&);
        void operator = (const FSLocalVolume&);

    public:
        FSLocalVolume(LPCTSTR pszVolName) 
            : FSVolume(pszVolName) { }

        BOOL IsLocal(VOID) const
            { return TRUE; }

};


//
// These next classes were originally designed when I thought we might
// need a hierarchy of file system object "types".  As it turns out,
// we really only need FSVolume and FSLocalVolume.  I'll leave these 
// in case the problem changes again sometime in the future.  For now, 
// these are excluded from compilation.  [brianau - 2/17/98]
//
#if 0
/*

class FSRemoteVolume : public FSVolume
{
    private:
        //
        // Prevent copying.
        //
        FSRemoteVolume(const FSRemoteVolume&);
        void operator = (const FSRemoteVolume&);

    public:
        FSRemoteVolume(VOID)
            : FSVolume() { }

        BOOL IsLocal(VOID) const
            { return FALSE; }

};


class FSDirectory : public FSObject
{
    private:
        //
        // Prevent copying.
        //
        FSDirectory(const FSDirectory&);
        void operator = (const FSDirectory&);

    protected:
        HANDLE m_hDirectory;

    public:
        FSDirectory(VOID)
            : FSObject(),
              m_hDirectory(NULL) { }

        HRESULT Initialize(DWORD dwAccess)
            { return E_NOTIMPL; }

        UINT Type(VOID) const
            { return FSObject::Directory; }


        virtual HRESULT QueryObjectQuotaInformation(
                            PDISKQUOTA_FSOBJECT_INFORMATION poi
                            ) { return E_NOTIMPL; }


        virtual HRESULT SetObjectQuotaInformation(
                            PDISKQUOTA_FSOBJECT_INFORMATION poi,
                            DWORD dwChangeMask
                            ) const { return E_NOTIMPL; }

        virtual HRESULT QueryUserQuotaInformation(
                            PVOID pUserInfoBuffer,
                            ULONG uBufferLength,
                            BOOL bReturnSingleEntry,
                            PVOID pSidList,
                            ULONG uSidListLength,
                            PSID pStartSid,
                            BOOL bRestartScan
                            ) { return E_NOTIMPL; }

        virtual HRESULT SetUserQuotaInformation(
                            PVOID pUserInfoBuffer,
                            ULONG uBufferLength
                            ) const { return E_NOTIMPL; }

};


class FSLocalDirectory : public FSDirectory
{
    private:
        //
        // Prevent copying.
        //
        FSLocalDirectory(const FSLocalDirectory&);
        void operator = (const FSLocalDirectory&);

    public:
        FSLocalDirectory(VOID)
            : FSDirectory() { }

        BOOL IsLocal(VOID) const
            { return TRUE; }

};


class FSRemoteDirectory : public FSDirectory
{
    private:
        //
        // Prevent copying.
        //
        FSRemoteDirectory(const FSRemoteDirectory&);
        void operator = (const FSRemoteDirectory&);

    public:
        FSRemoteDirectory(VOID)
            : FSDirectory() { }

        BOOL IsLocal(VOID) const
            { return FALSE; }
};
*/
#endif // #if 0

#endif  // DISKQUOTA_FSOBJECT_H


