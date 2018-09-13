//+--------------------------------------------------------------------------- 
// 
// Microsoft Windows 
// Copyright (C) Microsoft Corporation, 1992-1992 
// 
// File:        ofsmisc.c
//
// Contents:    Miscellaneous OFS interfaces
//
//---------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>


//+---------------------------------------------------------------------------
// Function:    SynchronousNtFsControlFile
//
// Synopsis:    Call NtFsControlFile and wait if handle is asynchronous
//
// Arguments:   [h]             -- handle to file/directory/volume
//              [pisb]          -- pointer to IO_STATUS_BLOCK
//              [FsControlCode] -- FsControl code
//              [pvIn]          -- optional input buffer
//              [cbIn]          -- input buffer size
//              [pvOut]         -- optional output buffer
//              [cbOut]         -- output buffer size
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS
SynchronousNtFsControlFile(
    IN HANDLE h,
    OUT IO_STATUS_BLOCK *pisb,
    IN ULONG FsControlCode,
    IN VOID *pvIn OPTIONAL,
    IN ULONG cbIn,
    OUT VOID *pvOut OPTIONAL,
    IN ULONG cbOut)
{
    NTSTATUS Status;

    Status = NtFsControlFile(
                    h,
                    NULL,
                    NULL,
                    NULL,
                    pisb,
                    FsControlCode,
                    pvIn,
                    cbIn,
                    pvOut,
                    cbOut);

    if (Status == STATUS_PENDING)
    {
        Status = NtWaitForSingleObject(h, TRUE, NULL);
    }
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    UsnFsctl, private
//
// Synopsis:    Return a USN according to the passed FsControlCode
//
// Arguments:   [hf]            -- handle to any file/directory in volume
//              [pusn]          -- pointer to USN to be returned
//              [FsControlCode] -- FSCTL_OFS_USN_GENERATE or FSCTL_OFS_USN_GET_CLOSE.
//
// Returns:     Status code
//---------------------------------------------------------------------------

#if defined(_CAIRO_) || DBG

#include "iofs.h"

NTSTATUS
UsnFsctl(
    IN HANDLE hf,
    OUT USN *pusn,
    IN ULONG FsControlCode)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    Status = SynchronousNtFsControlFile(
                hf,
                &isb,
                FsControlCode,
                NULL,                           // input buffer
                0,                              // input buffer length
                pusn,                           // output buffer
                sizeof(*pusn));                 // output buffer length
    ASSERT(!NT_SUCCESS(Status) || isb.Information == sizeof(*pusn));
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    OFSGetCloseUsn, public
//
// Synopsis:    Determine the USN associated with the passed handle.
//
// Arguments:   [hf]            -- handle to any file/directory in volume
//              [pusn]          -- pointer to USN to be returned
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
OFSGetCloseUsn(
    IN HANDLE hf,
    OUT USN *pusn)
{
    return(UsnFsctl(hf, pusn, FSCTL_OFS_USN_GET_CLOSE));
}


//+---------------------------------------------------------------------------
// Function:    RtlGenerateUsn, public
//
// Synopsis:    Generate and return the next USN associated with a volume.
//
// Arguments:   [hf]            -- handle to any file/directory in volume
//              [pusn]          -- pointer to USN to be returned
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlGenerateUsn(
    IN HANDLE hf,
    OUT USN *pusn)
{
    return(UsnFsctl(hf, pusn, FSCTL_OFS_USN_GENERATE));
}


//+---------------------------------------------------------------------------
// Function:    OFSGetVersion, public
//
// Synopsis:    Determine if the passed handle resides on an OFS volume.
//              If pversion != NULL, return the format version number.
//
// Arguments:   [hf]            -- handle to any file/directory in volume
//              [pversion]      -- pointer to version (may be NULL)
//
// Returns:     Status code
//
//---------------------------------------------------------------------------

#define QuadAlign(n) (((n) + (sizeof(LONGLONG) - 1)) & ~(sizeof(LONGLONG) - 1))

#define CSTRUCT(n, type, cchname)                                       \
        (((n) * QuadAlign(sizeof(type) + (cchname) * sizeof(WCHAR)) +   \
          sizeof(type) - 1) /                                           \
         sizeof(type))

NTSTATUS NTSYSAPI NTAPI
OFSGetVersion(HANDLE hf, ULONG *pversion)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    FILE_FS_ATTRIBUTE_INFORMATION *pfai;
    LARGE_INTEGER faibuf[CSTRUCT(1, FILE_FS_ATTRIBUTE_INFORMATION, 32)];

    pfai = (FILE_FS_ATTRIBUTE_INFORMATION *) faibuf;

    Status = NtQueryVolumeInformationFile(
                 hf,
                 &isb,
                 pfai,
                 sizeof(faibuf),
                 FileFsAttributeInformation);
    if (NT_SUCCESS(Status))
    {
        if (pfai->FileSystemNameLength != 3 * sizeof(WCHAR) ||
            pfai->FileSystemName[0] != L'O' ||
            pfai->FileSystemName[1] != L'F' ||
            pfai->FileSystemName[2] != L'S')
        {
            Status = STATUS_UNRECOGNIZED_VOLUME;
        }
        else if (pversion != NULL)
        {

            Status = SynchronousNtFsControlFile(
                hf,
                &isb,
                FSCTL_OFS_VERSION,
                NULL,                           // input buffer
                0,                              // input buffer length
                pversion,                       // output buffer
                sizeof(*pversion));             // output buffer length

            ASSERT(!NT_SUCCESS(Status) || isb.Information == sizeof(*pversion));
        }
    }
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlNameToOleId, public
//
// Synopsis:    Translate OLENAMEs to ids
//
// Arguments:   [hf]            -- handle to *volume*
//              [cbNames]       -- OLENAMES size
//              [pon]           -- pointer to OLENAMES
//              [pOleId]        -- pointer to output array of OleIds
//
// Returns:     Status code
//
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlNameToOleId(
    IN HANDLE hf,                               // must be volume handle
    IN ULONG cbNames,
    IN OLENAMES const *pon,
    OUT ULONG *pOleId)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    Status = SynchronousNtFsControlFile(
                hf,
                &isb,
                FSCTL_OFS_TRANSLATE_OLENAMES,
                (VOID *) pon,                   // input buffer
                cbNames,                        // input buffer length
                pOleId,                         // output buffer
                pon->cNames * sizeof(*pOleId)); // output buffer length
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlOleIdToName, public
//
// Synopsis:    Translate OLEIDs to names
//
// Arguments:   [hf]            -- handle to *volume*
//              [cOleId]        -- count of OleIds in array
//              [pOleId]        -- pointer to array of OleIds
//              [pcbNameBuf]    -- pointer to OLENAMES buffer size (updated)
//              [pon]           -- pointer to OLENAMES buffer
//
// Returns:     Status code
//
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlOleIdToName(
    IN HANDLE hf,                               // must be volume handle
    IN ULONG cOleId,
    IN ULONG const *pOleId,
    IN OUT ULONG *pcbNameBuf,
    OUT OLENAMES *pon)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    Status = SynchronousNtFsControlFile(
                hf,
                &isb,
                FSCTL_OFS_TRANSLATE_OLEIDS,
                (VOID *) pOleId,          // input buffer
                cOleId * sizeof(*pOleId), // input buffer length
                pon,                      // output buffer
                *pcbNameBuf);             // output buffer length
    *pcbNameBuf = isb.Information;
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlQueryQuota, public
//
// Synopsis:    Query Quota information for specific users
//
// Arguments:   [hf]    -- handle to *volume*
//              [pcb]   -- length of buffer on input and output
//              [pfqi]  -- pointer quota information to be filled in
//
// Returns:     Status code
//
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlQueryQuota(
    IN HANDLE hf,                               // must be volume handle
    IN OUT ULONG *pcb,
    IN OUT FILE_QUOTA_INFORMATION *pfqi)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    Status = SynchronousNtFsControlFile(
                hf,
                &isb,
                FSCTL_OFS_QUERY_QUOTA,
                pfqi,                   // input buffer
                *pcb,                   // input buffer length
                pfqi,                   // output buffer
                *pcb);                  // output buffer length

    *pcb = isb.Information;
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlQueryClassId, public
//
// Synopsis:    Fetch the ClassId for an open handle
//
// Arguments:   [hf]            -- handle to file
//              [pclsid]        -- pointer to ClassId buffer
//
// Returns:     Status code
//---------------------------------------------------------------------------

FILE_OBJECTID_INFORMATION foiiZero;

NTSTATUS NTSYSAPI NTAPI
RtlQueryClassId(
    IN HANDLE hf,
    OUT GUID *pclsid)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;
    FILE_OLE_INFORMATION foi;

    ASSERT(sizeof(GUID) == sizeof(foiiZero.ObjectId.Lineage));
    Status = NtQueryInformationFile(
                 hf,
                 &isb,
                 &foi,
                 sizeof(foi),
                 FileOleInformation);
    if (NT_SUCCESS(Status))
    {
        if (RtlCompareMemory(
                &foiiZero.ObjectId.Lineage,
                &foi.OleClassIdInformation.ClassId,
                sizeof(foiiZero.ObjectId.Lineage)) ==
            sizeof(foiiZero.ObjectId.Lineage))
        {
            Status = STATUS_NOT_FOUND;
        }
        else
        {
            *pclsid = foi.OleClassIdInformation.ClassId;
        }
    }
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlSetClassId, public
//
// Synopsis:    Set the ClassId for an open handle
//
// Arguments:   [hf]            -- handle to file
//              [pclsid]        -- pointer to ClassId -- NULL means delete
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlSetClassId(
    IN HANDLE hf,
    OPTIONAL IN GUID const *pclsid)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    ASSERT(sizeof(foiiZero.ObjectId.Lineage) == sizeof(*pclsid));
    Status = NtSetInformationFile(
                 hf,
                 &isb,
                 pclsid == NULL? &foiiZero.ObjectId.Lineage : (VOID *) pclsid,
                 sizeof(foiiZero.ObjectId.Lineage),
                 FileOleClassIdInformation);
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlQueryObjectId, public
//
// Synopsis:    Fetch the ObjectId for an open handle
//
// Arguments:   [hf]            -- handle to file
//              [poid]          -- pointer to ObjectId buffer
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlQueryObjectId(
    IN HANDLE hf,
    OUT OBJECTID *poid)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;
    FILE_OLE_INFORMATION foi;

    Status = NtQueryInformationFile(
                 hf,
                 &isb,
                 &foi,
                 sizeof(foi),
                 FileOleInformation);
    if (NT_SUCCESS(Status))
    {
        if (RtlCompareMemory(
                &foiiZero.ObjectId,
                &foi.ObjectIdInformation.ObjectId,
                sizeof(foiiZero.ObjectId)) == sizeof(foiiZero.ObjectId))
        {
            Status = STATUS_NOT_FOUND;
        }
        else
        {
            *poid = foi.ObjectIdInformation.ObjectId;
        }
    }
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlSetObjectId, public
//
// Synopsis:    Set the ObjectId for an open handle
//
// Arguments:   [hf]            -- handle to file
//              [poid]          -- pointer to ObjectId -- NULL means delete
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlSetObjectId(
    IN HANDLE hf,
    OPTIONAL IN OBJECTID const *poid)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    ASSERT(sizeof(foiiZero) == sizeof(*poid));
    Status = NtSetInformationFile(
                 hf,
                 &isb,
                 poid == NULL? &foiiZero : (VOID *) poid,
                 sizeof(foiiZero),
                 FileObjectIdInformation);
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlSetTunnelMode, public
//
// Synopsis:    Set the ObjectId tunnel mode for an OFS volume.
//
// Arguments:   [hVolume]       -- handle to volume
//              [ulFlags]       -- Bits to set
//              [ulMask]        -- Mask of bits to change
//              [pulOld]        -- pointer to store Old flags
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS
RtlSetTunnelMode(
    IN HANDLE hVolume,
    IN ULONG ulFlags,
    IN ULONG ulMask,
    OUT ULONG *pulOld)
{
    TUNNELMODE tm;
    TUNNELMODEOUT tmo;
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    tm.ulFlags = ulFlags;
    tm.ulMask = ulMask;

    Status = SynchronousNtFsControlFile(
                    hVolume,                    // Handle
                    &isb,                       // I/O Status block
                    FSCTL_OFS_TUNNEL_MODE,      // Control code
                    &tm,                        // Input buffer
                    sizeof(tm),                 // Input length
                    &tmo,                       // Output buffer
                    sizeof(tmo));               // Output length

    *pulOld = tmo.ulFlags;
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlSearchVolume, public
//
// Synopsis:    Search an OFS volume for files with passed ObjectId
//
// Arguments:   [hAncestor]     -- handle to file
//              [poid]          -- pointer to ObjectId
//              [cLineage]      -- Lineage count/flags
//              [fContinue]     -- TRUE if should continue
//              [usBufLen]      -- output buffer length
//              [pResults]      -- output buffer
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlSearchVolume(
    IN HANDLE hAncestor,
    IN OBJECTID const *poid,
    IN USHORT cLineage,
    IN BOOLEAN fContinue,
    IN ULONG usBufLen,
    OUT FINDOBJECTOUT *pfoo)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK isb;

    ((FINDOBJECT *) pfoo)->oid = *poid;
    ((FINDOBJECT *) pfoo)->cLineage = cLineage;
    ((FINDOBJECT *) pfoo)->ulFlags = fContinue? FO_CONTINUE_ENUM : 0;

    Status = SynchronousNtFsControlFile(
                    hAncestor,                  // Handle
                    &isb,                       // I/O Status block
                    FSCTL_OFS_FINDOBJECT,       // Control code
                    pfoo,                       // Input buffer
                    sizeof(FINDOBJECT),         // Input length
                    pfoo,                       // Output buffer
                    usBufLen);                  // Output length
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    RtlGenerateRelatedObjectId, public
//
// Synopsis:    Generate an object id which is related to the one passed in.
//
// Arguments:   [poidOld]       -- pointer to original ObjectId
//              [poidNew]       -- pointer to buffer for new ObjectId
//
// Returns:     Status code
//---------------------------------------------------------------------------

VOID NTSYSAPI NTAPI
RtlGenerateRelatedObjectId(
    IN OBJECTID const *poidOld,
    OUT OBJECTID *poidNew)
{       
    poidNew->Lineage = poidOld->Lineage;
    do
    {
        poidNew->Uniquifier = rand();
    } while (poidNew->Uniquifier == poidOld->Uniquifier);
}


//+---------------------------------------------------------------------------
// Function:    RtlSetReplicationState, public
//
// Synopsis:    Generate an object id which is related to the one passed in.
//
// Arguments:   [hf]            -- handle
//
// Returns:     Status code
//---------------------------------------------------------------------------

NTSTATUS NTSYSAPI NTAPI
RtlSetReplicationState(
    IN HANDLE hf)
{       
    IO_STATUS_BLOCK isb;

    return(SynchronousNtFsControlFile(
                    hf,                         // Handle
                    &isb,                       // I/O Status block
                    FSCTL_SET_REPLICATION_STATE,// Control code
                    NULL,                       // Input buffer
                    0,                          // Input length
                    NULL,                       // Output buffer
                    0));                        // Output length
}

#endif // _CAIRO_

//+---------------------------------------------------------------------------
// Function:    _purecall, private
//
// Synopsis:    C++ stub
//
// Arguments:   NONE
//
// Returns:     NONE
//
//---------------------------------------------------------------------------

VOID __cdecl
_purecall(VOID)
{
    ASSERTMSG("_purecall() was called", FALSE);
    RtlRaiseStatus(STATUS_NOT_IMPLEMENTED);
}
