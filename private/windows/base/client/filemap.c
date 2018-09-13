/*++

Copyright (c) 1990,1991 Microsoft Corporation

Module Name:

    filemap.c

Abstract:

    This module implements Win32 mapped file APIs

Author:

    Mark Lucovsky (markl) 15-Feb-1991

Revision History:

--*/

#include "basedll.h"
HANDLE
APIENTRY
CreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to CreateFileMappingW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    LPCWSTR NameBuffer;

    NameBuffer = NULL;
    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        NameBuffer = (LPCWSTR)Unicode->Buffer;
        }

    return CreateFileMappingW(
                hFile,
                lpFileMappingAttributes,
                flProtect,
                dwMaximumSizeHigh,
                dwMaximumSizeLow,
                NameBuffer
                );
}

HANDLE
APIENTRY
CreateFileMappingW(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
/*++

Routine Description:

    A file mapping object can be created using CreateFileMapping

    Creating a file mapping object creates the potential for mapping a
    view of the file into an address space.  File mapping objects may be
    shared either through process creation or handle duplication.
    Having a handle to a file mapping object allows for mapping of the
    file.  It does not mean that the file is actually mapped.

    A file mapping object has a maximum size.  This is used to size the
    file.  A file may not grow beyond the size specified in the mapping
    object.  While not required, it is recommended that when opening a
    file that you intend to map, the file should be opened for exclusive
    access.  Win32 does not require that a mapped file and a file
    accessed via the IO primitives (ReadFile/WriteFile) are coherent.

    In addition to the STANDARD_RIGHTS_REQUIRED access flags, the
    following object type specific access flags are valid for file
    mapping objects:

      - FILE_MAP_WRITE - Write map access to the file mapping object is
            desired.  This allows a writable view of the file to be
            mapped.  Note that if flProtect does not include
            PAGE_READWRITE, this access type does not allow writing the
            mapped file.

      - FILE_MAP_READ - Read map access to the file mapping object is
            desired.  This allows a readablee view of the file to be
            mapped.

      - FILE_MAP_ALL_ACCESS - This set of access flags specifies all of
            the possible access flags for a file mapping object.

Arguments:

    hFile - Supplies an open handle to a file that a mapping object is
        to be created for.  The file must be opened with an access mode
        that is compatible with the specified pretection flags. A value
        of INVALID_HANDLE_VALUE specifies that the mapping object is
        backed by the system paging file.  If this is the case, a size
        must be specified.

    lpFileMappingAttributes - An optional parameter that may be used to
        specify the attributes of the new file mapping object.  If the
        parameter is not specified, then the file mapping object is
        created without a security descriptor, and the resulting handle
        is not inherited on process creation:

    flProtect - The protection desired for mapping object when the file
        is mapped.

        flProtect Values

        PAGE_READONLY - Read access to the committed region of pages is
            allowed.  An attempt to write or execute the committed
            region results in an access violation.  The specified hFile
            must have been created with GENERIC_READ access.

        PAGE_READWRITE - Read and write access to the committed region
            of pages is allowed.  The specified hFile must have been
            created with GENERIC_READ and GENERIC_WRITE access.

        PAGE_WRITECOPY - Read and copy on write access to the committed
            region of pages is allowed.  The specified hFile must have been
            created with GENERIC_READ access.

    dwMaximumSizeHigh - Supplies the high order 32-bits of the maximum
        size of the file mapping object.

    dwMaximumSizeLow - Supplies the low order 32-bits of the maximum
        size of the file mapping object.  A value of zero along with a
        value of zero in dwMaximumSizeHigh indicates that the size of
        the file mapping object is equal to the current size of the file
        specified by hFile.

    lpName - Supplies the name ofthe file mapping object.

Return Value:

    NON-NULL - Returns a handle to the new file mapping object.  The
        handle has full access to the new file mapping object and may be
        used in any API that requires a handle to a file mapping object.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    HANDLE Section;
    NTSTATUS Status;
    LARGE_INTEGER SectionSizeData;
    PLARGE_INTEGER SectionSize;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    ACCESS_MASK DesiredAccess;
    UNICODE_STRING ObjectName;
    ULONG AllocationAttributes;
    PWCHAR pstrNewObjName = NULL;

    DesiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
    AllocationAttributes = flProtect & (SEC_FILE | SEC_IMAGE | SEC_RESERVE | SEC_COMMIT | SEC_NOCACHE);
    flProtect ^= AllocationAttributes;
    if (AllocationAttributes == 0) {
        AllocationAttributes = SEC_COMMIT;
        }

    if ( flProtect == PAGE_READWRITE ) {
        DesiredAccess |= (SECTION_MAP_READ | SECTION_MAP_WRITE);
        }
    else
    if ( flProtect != PAGE_READONLY && flProtect != PAGE_WRITECOPY ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
        }

    if ( ARGUMENT_PRESENT(lpName) ) {
        if (gpTermsrvFormatObjectName && 
            (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {
    
            RtlInitUnicodeString(&ObjectName,pstrNewObjName);
    
        } else {
    
            RtlInitUnicodeString(&ObjectName,lpName);
        }

        pObja = BaseFormatObjectAttributes(&Obja,lpFileMappingAttributes,&ObjectName);
        }
    else {
        pObja = BaseFormatObjectAttributes(&Obja,lpFileMappingAttributes,NULL);
        }

    if ( dwMaximumSizeLow || dwMaximumSizeHigh ) {
        SectionSize = &SectionSizeData;
        SectionSize->LowPart = dwMaximumSizeLow;
        SectionSize->HighPart = dwMaximumSizeHigh;
        }
    else {
        SectionSize = NULL;
        }

    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = NULL;
        if ( !SectionSize ) {
            SetLastError(ERROR_INVALID_PARAMETER);
            if (pstrNewObjName) {
                RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
            }
            return NULL;
            }
        }

    Status = NtCreateSection(
                &Section,
                DesiredAccess,
                pObja,
                SectionSize,
                flProtect,
                AllocationAttributes,
                hFile
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return Section = NULL;
        }
    else {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        }
    return Section;
}

HANDLE
APIENTRY
OpenFileMappingA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )

/*++

Routine Description:

    ANSI thunk to OpenFileMappingW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    if ( ARGUMENT_PRESENT(lpName) ) {
        Unicode = &NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString,lpName);
        Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            if ( Status == STATUS_BUFFER_OVERFLOW ) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                }
            else {
                BaseSetLastNTError(Status);
                }
            return NULL;
            }
        }
    else {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    return OpenFileMappingW(
                dwDesiredAccess,
                bInheritHandle,
                (LPCWSTR)Unicode->Buffer
                );
}

HANDLE
APIENTRY
OpenFileMappingW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING ObjectName;
    NTSTATUS Status;
    HANDLE Object;
    PWCHAR pstrNewObjName = NULL;

    if ( !lpName ) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return NULL;
        }

    if (gpTermsrvFormatObjectName && 
        (pstrNewObjName = gpTermsrvFormatObjectName(lpName))) {

        RtlInitUnicodeString(&ObjectName,pstrNewObjName);

    } else {

        RtlInitUnicodeString(&ObjectName,lpName);
    }

    InitializeObjectAttributes(
        &Obja,
        &ObjectName,
        (bInheritHandle ? OBJ_INHERIT : 0),
        BaseGetNamedObjectDirectory(),
        NULL
        );

    if ( dwDesiredAccess == FILE_MAP_COPY ) {
        dwDesiredAccess = FILE_MAP_READ;
        }

    Status = NtOpenSection(
                &Object,
                dwDesiredAccess,
                &Obja
                );

    if (pstrNewObjName) {
        RtlFreeHeap(RtlProcessHeap(), 0, pstrNewObjName);
    }
    
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    return Object;
}


LPVOID
APIENTRY
MapViewOfFile(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    SIZE_T dwNumberOfBytesToMap
    )

/*++

Routine Description:

    A view of a file may be mapped into the address space of the calling
    process using MapViewOfFile.

    Mapping a file object makes the specified portion of the file
    visible in the address space of the calling process.  The return
    address is a pointer to memory that when addressed causes data
    within the file to be accessed.

    Mapping a view of a file has some simple coherency rules:

      - Multiple views on a file are coherent if they are derived from
        the same file mapping object.  If a process opens a file,
        creates a mapping object, duplicates the object to another
        process...  If both processes map a view of the file, they will
        both see a coherent view of the file's data...  they will
        effectively be viewing shared memory backed by the file.

      - If multiple mapping objects exist for the same file, then views
        derived from the different mapping objects are not garunteed to
        be coherent.

      - A mapped view on a file is not garunteed to be coherent with a
        file being accessed via ReadFile or WriteFile.

Arguments:

    hFileMappingObject - Supplies an open handle to a file mapping object
        that is to be mapped into the callers address space.

    dwDesiredAccess - Specifies the access that is requested to the file
        mapping object. This determines the page protection of the pages
        mapped by the file.

        dwDesiredAccess Values:

        FILE_MAP_WRITE - Read/write access is desired.  The mapping
            object must have been created with PAGE_READWRITE
            protection.  The hFileMappingObject must have been created
            with FILE_MAP_WRITE access. A read/write view of the file will
            be mapped.

        FILE_MAP_READ - Read access is desired.  The mapping object must
            have been created with PAGE_READWRITE or PAGE_READ
            protection.  The hFileMappingObject must have been created
            with FILE_MAP_READ access.  A read only view of the file
            will be mapped.

    dwFileOffsetHigh - Supplies the high order 32-bits of the file
        offset where mapping is to begin.

    dwFileOffsetLow - Supplies the low order 32-bits of the file offset
        where mapping is to begin. The combination of the high and low
        offsets must specify a 64Kb aligned offset within the file. It
        is an error if this is not the case.

    dwNumberOfBytesToMap - Supplies the number of bytes of the file to map.
        A value of zero specifies that the entire file is to be mapped.

Return Value:

    NON-NULL - Returns the address of where the file is mapped.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return MapViewOfFileEx(
            hFileMappingObject,
            dwDesiredAccess,
            dwFileOffsetHigh,
            dwFileOffsetLow,
            dwNumberOfBytesToMap,
            NULL
            );
}

LPVOID
APIENTRY
MapViewOfFileEx(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    SIZE_T dwNumberOfBytesToMap,
    LPVOID lpBaseAddress
    )

/*++

Routine Description:

    A view of a file may be mapped into the address space of the calling
    process using MapViewOfFileEx.

    Mapping a file object makes the specified portion of the file
    visible in the address space of the calling process.  The return
    address is a pointer to memory that when addressed causes data
    within the file to be accessed. This API allows the caller to
    supply the system with a suggested mapping address. The system
    will round this address down to the nearest 64k boundry and attempt
    to map the file at thet address. If there is not enough address space
    at that address, this call will fail.

    Mapping a view of a file has some simple coherency rules:

      - Multiple views on a file are coherent if they are derived from
        the same file mapping object.  If a process opens a file,
        creates a mapping object, duplicates the object to another
        process...  If both processes map a view of the file, they will
        both see a coherent view of the file's data...  they will
        effectively be viewing shared memory backed by the file.

      - If multiple mapping objects exist for the same file, then views
        derived from the different mapping objects are not garunteed to
        be coherent.

      - A mapped view on a file is not garunteed to be coherent with a
        file being accessed via ReadFile or WriteFile.

Arguments:

    hFileMappingObject - Supplies an open handle to a file mapping object
        that is to be mapped into the callers address space.

    dwDesiredAccess - Specifies the access that is requested to the file
        mapping object. This determines the page protection of the pages
        mapped by the file.

        dwDesiredAccess Values:

        FILE_MAP_WRITE - Read/write access is desired.  The mapping
            object must have been created with PAGE_READWRITE
            protection.  The hFileMappingObject must have been created
            with FILE_MAP_WRITE access. A read/write view of the file will
            be mapped.

        FILE_MAP_READ - Read access is desired.  The mapping object must
            have been created with PAGE_READWRITE or PAGE_READ
            protection.  The hFileMappingObject must have been created
            with FILE_MAP_READ access.  A read only view of the file
            will be mapped.

    dwFileOffsetHigh - Supplies the high order 32-bits of the file
        offset where mapping is to begin.

    dwFileOffsetLow - Supplies the low order 32-bits of the file offset
        where mapping is to begin. The combination of the high and low
        offsets must specify a 64Kb aligned offset within the file. It
        is an error if this is not the case.

    dwNumberOfBytesToMap - Supplies the number of bytes of the file to map.
        A value of zero specifies that the entire file is to be mapped.

    lpBaseAddress - Supplies the base address of where in the processes
        address space the mapping is to begin at.  The address is
        rounded down to the nearest 64k boundry by the system.  A value
        of NULL for this parameter operates exactly the same as
        MapViewOfFile...  The system picks the mapping base address
        without any hint from the caller.

Return Value:

    NON-NULL - Returns the address of where the file is mapped.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize;
    PVOID ViewBase;
    ULONG Protect;

    SectionOffset.LowPart = dwFileOffsetLow;
    SectionOffset.HighPart = dwFileOffsetHigh;
    ViewSize = dwNumberOfBytesToMap;
    ViewBase = lpBaseAddress;

    if ( dwDesiredAccess == FILE_MAP_COPY ) {
        Protect = PAGE_WRITECOPY;
        }
    else
    if ( dwDesiredAccess & FILE_MAP_WRITE ) {
        Protect = PAGE_READWRITE;
        }
    else if ( dwDesiredAccess & FILE_MAP_READ ) {
        Protect = PAGE_READONLY;
        }
    else {
        Protect = PAGE_NOACCESS;
        }

    Status = NtMapViewOfSection(
                hFileMappingObject,
                NtCurrentProcess(),
                &ViewBase,
                0L,
                0L,
                &SectionOffset,
                &ViewSize,
                ViewShare,
                0L,
                Protect
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return NULL;
        }
    return ViewBase;
}


BOOL
APIENTRY
FlushViewOfFile(
    LPCVOID lpBaseAddress,
    SIZE_T dwNumberOfBytesToFlush
    )

/*++

Routine Description:

    A byte range within a mapped view of a file can be flushed to disk
    using FlushViewOfFile.

    A byte range within a mapped view of a file can be flushed to disk
    using FlushViewOfFile.

    Flushing a range of a mapped view causes any dirty pages within that
    range to be written to disk.  This operation automatically happens
    whenever a view is unmapped (either explicitly or as a result of
    process termination).


Arguments:

    lpBaseAddress - Supplies the base address of a set of bytes that are
        to be flushed to the on disk representation of the mapped file.

    dwNumberOfBytesToFlush - Supplies the number of bytes to flush.

Return Value:

    TRUE - The operation was successful.  All dirty pages within the
        specified range are stored in the on-disk representation of the
        mapped file.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    IO_STATUS_BLOCK IoStatus;

    BaseAddress = (PVOID)lpBaseAddress;
    RegionSize = dwNumberOfBytesToFlush;

    Status = NtFlushVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &RegionSize,
                &IoStatus
                );
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_NOT_MAPPED_DATA ) {
            return TRUE;
            }
        BaseSetLastNTError(Status);
        return FALSE;
        }

    return TRUE;
}

BOOL
APIENTRY
UnmapViewOfFile(
    LPCVOID lpBaseAddress
    )

/*++

Routine Description:

    A previously mapped view of a file may be unmapped from the callers
    address space using UnmapViewOfFile.

Arguments:

    lpBaseAddress - Supplies the base address of a previously mapped
        view of a file that is to be unmapped.  This value must be
        identical to the value returned by a previous call to
        MapViewOfFile.

Return Value:

    TRUE - The operation was successful.  All dirty pages within the
        specified range are stored in the on-disk representation of the
        mapped file.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;

    Status = NtUnmapViewOfSection(NtCurrentProcess(),(PVOID)lpBaseAddress);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    return TRUE;
}
