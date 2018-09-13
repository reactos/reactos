///////////////////////////////////////////////////////////////////////////////
/*  File: fsobject.cpp

    Description: Contains member function definitions for class FSObject and
        it's derived subclasses.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "dskquota.h"
#include "fsobject.h"
#include "pathstr.h"

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSObject::~FSObject

    Description: Destructor.  Frees object's name buffer.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
FSObject::~FSObject(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSObject::~FSObject")));
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSObject::AddRef

    Description: Increments object reference count.
        Note this is not a member of IUnknown; but it works the same.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ULONG
FSObject::AddRef(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("FSObject::AddRef")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
             this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSObject::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.  Note this is not a member of IUnknown; but it
        works the same.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ULONG
FSObject::Release(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("FSObject::Release")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
             this, m_cRef, m_cRef - 1));

    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FSObject::ObjectSupportsQuotas

    Description: Determine a file system object's type (and locality) from
        it's name string.

    Arguments:
        pszFSObjName - Volume root name. (i.e. "C:\", "\\scratch\scratch").

    Returns:
        S_OK                     - Success.  Supports quotas.
        ERROR_NOT_SUPPORTED (hr) - File system doesn't support quotas.
        Other win32 error        - Couldn't get volume information.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
    08/16/96    Added pSupportsQuotas.                               BrianAu
    12/05/96    Disabled check for $DeadMeat volume label.           BrianAu
                Leave the code in place for a while.  I'll remove
                it later when we're sure it's not needed.
    07/03/97    Changed name from ObjectTypeFromName.                BrianAu
                Changed logic to indicate reason for not supporting
                quotas.
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FSObject::ObjectSupportsQuotas(
    LPCTSTR pszFSObjName
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSObject::ObjectSupportsQuotas")));
    DBGPRINT((DM_CONTROL, DL_MID, TEXT("\tobject = \"%s\""), pszFSObjName ? pszFSObjName : TEXT("<null>")));

    HRESULT hr = E_FAIL;
    DWORD dwFileSysFlags = 0;
    TCHAR szFileSysName[MAX_PATH];

    DBGASSERT((NULL != pszFSObjName));

    if (GetVolumeInformation(
                pszFSObjName,
                NULL, 0,
                NULL, 0,
                &dwFileSysFlags,
                szFileSysName,
                ARRAYSIZE(szFileSysName)))
    {
        //
        // Does the file system support quotas?
        //
        if (0 != (dwFileSysFlags & FILE_VOLUME_QUOTAS))
        {
            //
            // Yes, it does.
            //
            hr = S_OK;
            DBGPRINT((DM_CONTROL, DL_LOW, TEXT("Vol \"%s\" supports quotas"), pszFSObjName));
        }
        else
        {
            //
            // Doesn't support quotas.
            //
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            DBGPRINT((DM_CONTROL, DL_HIGH, TEXT("File system \"%s\" on \"%s\" doesn't support quotas."),
                     szFileSysName, pszFSObjName));
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        DBGERROR((TEXT("Error %d calling GetVolumeInformation for \"%s\""), dwErr, pszFSObjName));
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSObject::Create

    Description: 2 overloaded functions.
        Static functions for creating a File System object of the
        proper type.  Clients call Create with an object name string or
        a reference to an existing FSObject instance.

    Arguments:
        pszFSObjName - Address of volume root string.

        ppNewObject - Address of FSObject pointer to accept the address of the
            new file system object.

        ObjToClone - Reference to file system object to be cloned.

    Returns:
        NOERROR                   - Success.
        E_OUTOFMEMORY             - Insufficient memory.
        ERROR_ACCESS_DENIED (hr)  - Insufficient access to open device.
        ERROR_FILE_NOT_FOUND (hr) - Disk device not found.
        ERROR_INVALID_NAME (hr)   - Object name is invalid.
        ERROR_NOT_SUPPORTED (hr)  - Volume doesn't support quotas.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/23/96    Initial creation.                                    BrianAu
    09/05/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FSObject::Create(
    LPCTSTR pszFSObjName,
    DWORD dwAccess,
    FSObject **ppNewObject
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSObject::Create")));
    DBGPRINT((DM_CONTROL, DL_MID, TEXT("\tVol = \"%s\""), pszFSObjName ? pszFSObjName : TEXT("<null>")));
    HRESULT hr = NOERROR;

    DBGASSERT((NULL != pszFSObjName));
    DBGASSERT((NULL != ppNewObject));

    *ppNewObject = NULL;

    FSObject *pNewObject = NULL;

    try
    {
        hr = FSObject::ObjectSupportsQuotas(pszFSObjName);
        if (SUCCEEDED(hr))
        {
            pNewObject = new FSLocalVolume(pszFSObjName);

            //
            // Do any subclass-specific initialization.
            // i.e.:  Volume opens the volume device.
            //
            hr = pNewObject->Initialize(dwAccess);

            if (SUCCEEDED(hr))
            {
                //
                // Return ptr to caller.
                //
                DBGPRINT((DM_CONTROL, DL_MID, TEXT("FSObject created")));
                pNewObject->AddRef();
                *ppNewObject = pNewObject;
            }
            else
            {
                DBGPRINT((DM_CONTROL, DL_MID, TEXT("FSObject create FAILED with error 0x%08X"), hr));
                delete pNewObject;
                pNewObject = NULL;
            }
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        delete pNewObject;  // Will also free name if necessary.
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        delete pNewObject;
        hr = E_UNEXPECTED;
    }

    return hr;
}

//
// Version to clone an existing FSObject.
//
HRESULT
FSObject::Create(
    const FSObject& ObjectToClone,
    FSObject **ppNewObject
    )
{
    return FSObject::Create(ObjectToClone.m_strFSObjName,
                            ObjectToClone.m_dwAccessRights,
                            ppNewObject);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSObject::GetName

    Description: Retrieves the file system object's name string.

    Arguments:
        pszBuffer - Address of buffer to accept name string.

        cchBuffer - Size of destination buffer in characters.

    Returns:
        NOERROR                   - Success.
        ERROR_INSUFFICIENT_BUFFER - Destination buffer is too small for name.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT FSObject::GetName(LPTSTR pszBuffer, ULONG cchBuffer) const
{
    HRESULT hr = NOERROR;

    DBGASSERT((NULL != pszBuffer));
    if ((ULONG)m_strFSObjName.Length() < cchBuffer)
        lstrcpyn(pszBuffer, m_strFSObjName, cchBuffer);
    else
    {
        *pszBuffer = TEXT('\0');
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FSVolume::~FSVolume

    Description: Destructor. Closes volume handle.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
FSVolume::~FSVolume(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::~FSVolume")));
    if (NULL != m_hVolume)
        CloseHandle(m_hVolume);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSVolume::Initialize

    Description: Initializes a volume object by opening the NTFS volume.

    Arguments:
        dwAccess - Desired access.  GENERIC_READ, GENERIC_WRITE.

    Returns:
        NOERROR              - Success.
        ERROR_ACCESS_DENIED (hr) - Insufficient access to open device.
        ERROR_FILE_NOT_FOUND (hr) - Disk device not found.
        ERROR_INVALID_NAME (hr) - Invalid path string.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
    08/11/96    Added access right handling.                         BrianAu
    08/16/96    Added device name formatting.                        BrianAu
    07/03/97    Changed so caller passes in desired access.          BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT FSVolume::Initialize(
    DWORD dwAccess
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::Initialize")));
    DBGPRINT((DM_CONTROL, DL_MID, TEXT("\tdwAccess = 0x%08X"), dwAccess));

    HRESULT hr = NOERROR;

    //
    // Close the device if it's open.
    //
    if (NULL != m_hVolume)
        CloseHandle(m_hVolume);

    //
    // Create a path to the actual quota file on the volume.
    // This string is appended to the existing "volume name" we already
    // have.
    //
    CPath strQuotaFile(m_strFSObjName);
    strQuotaFile.AddBackslash();
    strQuotaFile += CString("$Extend\\$Quota:$Q:$INDEX_ALLOCATION");

    m_hVolume = CreateFile(strQuotaFile,
                           dwAccess,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);

    if (INVALID_HANDLE_VALUE == m_hVolume)
    {
        //
        // Couldn't open device because...
        // 1. I/O error
        // 2. File (device) not found.
        // 3. Access denied.
        //
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        DBGERROR((TEXT("Error %d opening quota file \"%s\""), dwErr, strQuotaFile.Cstr()));
        m_hVolume = NULL;
    }
    else
    {
        //
        // Save access granted to caller.  Will be used to validate
        // operation requests later.
        //
        DBGPRINT((DM_CONTROL, DL_MID, TEXT("Quota file \"%s\" open with access 0x%08X"), strQuotaFile.Cstr(), dwAccess));
        m_dwAccessRights = dwAccess;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: FSVolume::QueryObjectQuotaInformation

    Description: Retrieves quota information for the volume.  This includes
        default quota threshold, default quota limit and system control flags.

    Arguments:
        poi - Address of object information buffer.  This type contains
            a subset of the information in FILE_FS_CONTROL_INFORMATION
            (defined in ntioapi.h).

    Returns:
        NOERROR                  - Success.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        Other                    - NTFS subsystem failure result.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FSVolume::QueryObjectQuotaInformation(
    PDISKQUOTA_FSOBJECT_INFORMATION poi
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::QueryObjectQuotaInformation")));
    HRESULT hr = E_FAIL;

    if (!GrantedAccess(GENERIC_READ))
    {
        DBGPRINT((DM_CONTROL, DL_MID, TEXT("Access denied reading quota info")));
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        NTSTATUS status = STATUS_SUCCESS;
        IO_STATUS_BLOCK iosb;
        FILE_FS_CONTROL_INFORMATION ControlInfo;

        status = NtQueryVolumeInformationFile(
                    m_hVolume,
                    &iosb,
                    &ControlInfo,
                    sizeof(ControlInfo),
                    FileFsControlInformation);

        if (STATUS_SUCCESS == status)
        {
            //
            // Update caller's buffer with quota control data.
            //
            poi->DefaultQuotaThreshold  = ControlInfo.DefaultQuotaThreshold.QuadPart;
            poi->DefaultQuotaLimit      = ControlInfo.DefaultQuotaLimit.QuadPart;
            poi->FileSystemControlFlags = ControlInfo.FileSystemControlFlags;
            hr = NOERROR;
        }
        else
        {
            DBGERROR((TEXT("NtQueryVolumeInformationFile failed with NTSTATUS 0x%08X"), status));
            hr = HResultFromNtStatus(status);
        }
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FSVolume::SetObjectQuotaInformation

    Description: Writes new quota information to the volume.  This includes
        default quota threshold, default quota limit and system control flags.

    Arguments:
        poi - Address of object information buffer.  This type contains
            a subset of the information in FILE_FS_CONTROL_INFORMATION
            (defined in ntioapi.h).

        dwChangeMask - Mask specifying which elements in *poi to write to disk.
            Can be any combination of:
                FSObject::ChangeState
                FSObject::ChangeLogFlags
                FSObject::ChangeThreshold
                FSObject::ChangeLimit

    Returns:
        NOERROR                  - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
        Other                    - NTFS subsystem failure result.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FSVolume::SetObjectQuotaInformation(
    PDISKQUOTA_FSOBJECT_INFORMATION poi,
    DWORD dwChangeMask
    ) const
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::SetObjectQuotaInformation")));
    HRESULT hr = E_FAIL;

    if (!GrantedAccess(GENERIC_WRITE))
    {
        DBGPRINT((DM_CONTROL, DL_MID, TEXT("Access denied setting quota info")));
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        NTSTATUS status = STATUS_SUCCESS;
        IO_STATUS_BLOCK iosb;
        FILE_FS_CONTROL_INFORMATION ControlInfo;

        //
        // First read current info from disk.
        // Then replace whatever we're changing.
        //
        status = NtQueryVolumeInformationFile(
                    m_hVolume,
                    &iosb,
                    &ControlInfo,
                    sizeof(ControlInfo),
                    FileFsControlInformation);

        if (STATUS_SUCCESS == status)
        {
            //
            // Only alter those values specified in dwChangeMask.
            //
            if (FSObject::ChangeState & dwChangeMask)
            {
                ControlInfo.FileSystemControlFlags &= ~DISKQUOTA_STATE_MASK;
                ControlInfo.FileSystemControlFlags |= (poi->FileSystemControlFlags & DISKQUOTA_STATE_MASK);
            }
            if (FSObject::ChangeLogFlags & dwChangeMask)
            {
                ControlInfo.FileSystemControlFlags &= ~DISKQUOTA_LOGFLAG_MASK;
                ControlInfo.FileSystemControlFlags |= (poi->FileSystemControlFlags & DISKQUOTA_LOGFLAG_MASK);
            }
            if (FSObject::ChangeThreshold & dwChangeMask)
            {
                ControlInfo.DefaultQuotaThreshold.QuadPart = poi->DefaultQuotaThreshold;
            }
            if (FSObject::ChangeLimit & dwChangeMask)
            {
                ControlInfo.DefaultQuotaLimit.QuadPart = poi->DefaultQuotaLimit;
            }

            status = NtSetVolumeInformationFile(
                        m_hVolume,
                        &iosb,
                        &ControlInfo,
                        sizeof(ControlInfo),
                        FileFsControlInformation);

            if (STATUS_SUCCESS == status)
            {
                hr = NOERROR;
            }
            else
            {
                DBGERROR((TEXT("NtSetVolumeInformationFile failed with NTSTATUS = 0x%08X"), status));
                hr = HResultFromNtStatus(status);
            }
        }
        else
        {
            DBGERROR((TEXT("NtQueryVolumeInformationFile failed with NTSTATUS = 0x%08X"), status));
            hr = HResultFromNtStatus(status);
        }
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FSVolume::QueryUserQuotaInformation

    Description: Retrieves user quota information for the volume.  This includes
        quota threshold and quota limit.  This function works like an enumerator.
        Repeated calls will return multiple user records.

    Arguments:
        pBuffer - Address of buffer to receive quota information.

        cbBuffer - Number of bytes in buffer.

        bReturnSingleEntry - TRUE  = Return only one record from quota file.
                             FALSE = Return as many whole entries as possible
                                     in buffer.

        pSidList [optional] - Address of SID list identifying users to obtain
            information for.  Specify NULL to include all users.

        cbSidList [optional] - Number of bytes in sid list.  Ignored if pSidList
            is NULL.

        pStartSid [optional] - Address of SID identifying which user is to start
            the enumeration.  Specify NULL to start with current user in
            enumeration.

        bRestartScan - TRUE = restart scan from first user in the SID list or
            the entire file if pSidList is NULL.
            FALSE = Continue enumeration from current user record.

    Returns:
        NOERROR                  - Success.
        ERROR_NO_MORE_ITEMS      - Read last entry in quota file.
        ERROR_ACCESS_DENIED (hr) - No READ access to quota device.
        Other                    - Quota subsystem error.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FSVolume::QueryUserQuotaInformation(
    PVOID pBuffer,
    ULONG cbBuffer,
    BOOL bReturnSingleEntry,
    PVOID pSidList,
    ULONG cbSidList,
    PSID  pStartSid,
    BOOL  bRestartScan
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::QueryUserQuotaInformation")));
    HRESULT hr = E_FAIL;

    DBGASSERT((NULL != pBuffer));

    if (!GrantedAccess(GENERIC_READ))
    {
        DBGPRINT((DM_CONTROL, DL_MID, TEXT("Access denied querying user quota info")));
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        NTSTATUS status = STATUS_SUCCESS;
        IO_STATUS_BLOCK iosb;

        status = NtQueryQuotaInformationFile(
                    m_hVolume,
                    &iosb,
                    pBuffer,
                    cbBuffer,
                    (BOOLEAN)bReturnSingleEntry,
                    pSidList,
                    cbSidList,
                    pStartSid,
                    (BOOLEAN)bRestartScan);

        switch(status)
        {
            case STATUS_SUCCESS:
                hr = NOERROR;
                break;

            default:
                DBGERROR((TEXT("NtQueryQuotaInformationFile failed with NTSTATUS 0x%08X"), status));
                //
                // Fall through...
                //
            case STATUS_NO_MORE_ENTRIES:
                hr = HResultFromNtStatus(status);
                break;
        }
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: FSVolume::SetUserQuotaInformation

    Description: Writes new user quota information to the volume.  This includes
        quota threshold, and quota limit.

    Arguments:
        pBuffer - Address of buffer containing quota information.

        cbBuffer - Number of bytes of data in buffer.

    Returns:
        NOERROR                  - Success.
        ERROR_ACCESS_DENIED (hr) - No WRITE access to quota device.
                                   Or tried to set limit on Administrator.
        Other                    - Quota subsystem error.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/24/96    Initial creation.                                    BrianAu
    08/11/96    Added access control.                                BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
FSVolume::SetUserQuotaInformation(
    PVOID pBuffer,
    ULONG cbBuffer
    ) const
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("FSVolume::SetUserQuotaInformation")));

    HRESULT hr = NOERROR;

    DBGASSERT((NULL != pBuffer));

    if (!GrantedAccess(GENERIC_WRITE))
    {
        DBGPRINT((DM_CONTROL, DL_MID, TEXT("Access denied setting user quota info")));
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        NTSTATUS status = STATUS_SUCCESS;
        IO_STATUS_BLOCK iosb;

        status = NtSetQuotaInformationFile(
                    m_hVolume,
                    &iosb,
                    pBuffer,
                    cbBuffer);

        if (STATUS_SUCCESS == status)
        {
            hr = NOERROR;
        }
        else
        {
            DBGERROR((TEXT("NtSetQuotaInformationFile failed with NTSTATUS 0x%08X"), status));
            hr = HResultFromNtStatus(status);
        }
    }
    return hr;
}


//
// Convert an NTSTATUS value to an HRESULT.
// This is a simple attempt at converting the most common NTSTATUS values that
// might be returned from NtQueryxxxxx and NTSetxxxxxx functions.  If I've missed
// some obvious ones, go ahead and add them.
//
HRESULT
FSObject::HResultFromNtStatus(
    NTSTATUS status
    )
{
    HRESULT hr = E_FAIL;  // Default if none matched.

    static const struct
    {
        NTSTATUS status;
        HRESULT hr;
    } rgXref[] = {

       { STATUS_SUCCESS,                NOERROR                                        },
       { STATUS_INVALID_PARAMETER,      E_INVALIDARG                                   },
       { STATUS_NO_MORE_ENTRIES,        HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)        },
       { STATUS_ACCESS_DENIED,          HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)        },
       { STATUS_BUFFER_TOO_SMALL,       HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)  },
       { STATUS_BUFFER_OVERFLOW,        HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)      },
       { STATUS_INVALID_HANDLE,         HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)       },
       { STATUS_INVALID_DEVICE_REQUEST, HRESULT_FROM_WIN32(ERROR_BAD_DEVICE)           },
       { STATUS_FILE_INVALID,           HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE) }};

    for (int i = 0; i < ARRAYSIZE(rgXref); i++)
    {
        if (rgXref[i].status == status)
        {
            hr = rgXref[i].hr;
            break;
        }
    }
    return hr;
}
