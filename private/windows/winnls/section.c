/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    section.c

Abstract:

    This file contains functions that deal with creating, opening, or
    mapping a section for data table files for the NLS API.

    External Routines found in this file:
      CreateNlsObjectDirectory
      CreateRegKey
      OpenRegKey
      QueryRegValue
      SetRegValue
      CreateSectionFromReg
      CreateSectionOneValue
      CreateSectionTemp
      OpenSection
      MapSection
      UnMapSection
      GetNlsSectionName
      GetCodePageDLLPathName

Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Forward Declarations.
//

ULONG
OpenDataFile(
    HANDLE *phFile,
    LPWSTR pFile);

ULONG
GetNTFileName(
    LPWSTR pFile,
    PUNICODE_STRING pFileName);

ULONG
CreateSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID *ppWorldSid,
    ACCESS_MASK AccessMask);

ULONG
AppendAccessAllowedACE(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    ACCESS_MASK AccessMask);





//-------------------------------------------------------------------------//
//                           INTERNAL MACROS                               //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NLS_REG_BUFFER_ALLOC
//
//  Allocates the buffer used by the registry enumeration and query calls
//  and sets the pKeyValueFull variable to point at the newly created buffer.
//
//  NOTE: This macro may return if an error is encountered.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_REG_BUFFER_ALLOC( pKeyValueFull,                               \
                              BufSize,                                     \
                              pBuffer,                                     \
                              CritSect )                                   \
{                                                                          \
    if ((pBuffer = (PVOID)NLS_ALLOC_MEM(BufSize)) == NULL)                 \
    {                                                                      \
        KdPrint(("NLSAPI: Could NOT Allocate Memory.\n"));                 \
        if (CritSect)                                                      \
        {                                                                  \
            RtlLeaveCriticalSection(&gcsTblPtrs);                          \
        }                                                                  \
        return ((ULONG)STATUS_NO_MEMORY);                                  \
    }                                                                      \
                                                                           \
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pBuffer;                  \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_REG_BUFFER_FREE
//
//  Frees the buffer used by the registry enumeration and query calls.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_REG_BUFFER_FREE(pBuffer)        (NLS_FREE_MEM(pBuffer))





//-------------------------------------------------------------------------//
//                           EXTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  CreateNlsObjectDirectory
//
//  This routine creates the object directory for the NLS memory mapped
//  sections.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG CreateNlsObjectDirectory()
{
    ULONG pSecurityDescriptor[MAX_PATH_LEN]; // security descriptor buffer
    PSID pWorldSid;                          // ptr to world SID
    UNICODE_STRING ObDirName;                // directory name
    OBJECT_ATTRIBUTES ObjA;                  // object attributes structure
    HANDLE hDirHandle;                       // directory handle
    ULONG rc = 0L;                           // return code


    //
    //  Create the security descriptor with READ access to the world.
    //
    if (rc = CreateSecurityDescriptor( pSecurityDescriptor,
                                       &pWorldSid,
                                       DIRECTORY_QUERY | DIRECTORY_TRAVERSE ))
    {
        NLS_FREE_MEM(pWorldSid);
        return (rc);
    }

    //
    //  Add Admin Access for Query.
    //
    if (rc = AppendAccessAllowedACE( pSecurityDescriptor,
                                     DIRECTORY_QUERY |
                                     DIRECTORY_TRAVERSE |
                                     DIRECTORY_CREATE_OBJECT ))
    {
        NLS_FREE_MEM(pWorldSid);
        return (rc);
    }

    //
    //  Create the object directory.
    //
    RtlInitUnicodeString(&ObDirName, NLS_OBJECT_DIRECTORY_NAME);
    InitializeObjectAttributes( &ObjA,
                                &ObDirName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                NULL,
                                pSecurityDescriptor );

    rc = NtCreateDirectoryObject( &hDirHandle,
                                  DIRECTORY_TRAVERSE | DIRECTORY_CREATE_OBJECT,
                                  &ObjA );

    //
    //  Free the memory used for the SID and close the directory handle.
    //
    NLS_FREE_MEM(pWorldSid);
    NtClose(hDirHandle);

    //
    //  Check for error from NtCreateDirectoryObject.
    //
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Create Object Directory %wZ - %lx.\n",
                 &ObDirName, rc));
        return (rc);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateRegKey
//
//  This routine creates a key in the registry.
//
//  12-17-97    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG CreateRegKey(
    PHANDLE phKeyHandle,
    LPWSTR pBaseName,
    LPWSTR pKey,
    ULONG fAccess)
{
    WCHAR pwszKeyName[MAX_PATH_LEN];   // ptr to the full key name
    HANDLE UserKeyHandle;              // HKEY_CURRENT_USER equivalent
    OBJECT_ATTRIBUTES ObjA;            // object attributes structure
    UNICODE_STRING ObKeyName;          // key name
    ULONG rc = 0L;                     // return code


    //
    //  Get the full key name.
    //
    if (pBaseName == NULL)
    {
        //
        //  Get current user's key handle.
        //
        rc = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &UserKeyHandle);
        if (!NT_SUCCESS(rc))
        {
            KdPrint(("NLSAPI: Could NOT Open HKEY_CURRENT_USER - %lx.\n", rc));
            return (rc);
        }
        pwszKeyName[0] = UNICODE_NULL;
    }
    else
    {
        //
        //  Base name exists, so not current user.
        //
        UserKeyHandle = NULL;
        NlsStrCpyW(pwszKeyName, pBaseName);
    }
    NlsStrCatW(pwszKeyName, pKey);

    //
    //  Create the registry key.
    //
    RtlInitUnicodeString(&ObKeyName, pwszKeyName);
    InitializeObjectAttributes( &ObjA,
                                &ObKeyName,
                                OBJ_CASE_INSENSITIVE,
                                UserKeyHandle,
                                NULL );
    rc = NtCreateKey( phKeyHandle,
                      fAccess,
                      &ObjA,
                      0,
                      NULL,
                      REG_OPTION_NON_VOLATILE,
                      NULL );

    //
    //  Close the current user handle, if necessary.
    //
    if (UserKeyHandle != NULL)
    {
        NtClose(UserKeyHandle);
    }

    //
    //  Check for error from NtCreateKey.
    //
    if (!NT_SUCCESS(rc))
    {
        *phKeyHandle = NULL;
    }

    //
    //  Return the status from NtCreateKey.
    //
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenRegKey
//
//  This routine opens a key in the registry.
//
//  08-02-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG OpenRegKey(
    PHANDLE phKeyHandle,
    LPWSTR pBaseName,
    LPWSTR pKey,
    ULONG fAccess)
{
    WCHAR pwszKeyName[MAX_PATH_LEN];   // ptr to the full key name
    HANDLE UserKeyHandle;              // HKEY_CURRENT_USER equivalent
    OBJECT_ATTRIBUTES ObjA;            // object attributes structure
    UNICODE_STRING ObKeyName;          // key name
    ULONG rc = 0L;                     // return code


    //
    //  Get the full key name.
    //
    if (pBaseName == NULL)
    {
        //
        //  Get current user's key handle.
        //
        rc = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &UserKeyHandle);
        if (!NT_SUCCESS(rc))
        {
            KdPrint(("NLSAPI: Could NOT Open HKEY_CURRENT_USER - %lx.\n", rc));
            return (rc);
        }
        pwszKeyName[0] = UNICODE_NULL;
    }
    else
    {
        //
        //  Base name exists, so not current user.
        //
        UserKeyHandle = NULL;
        NlsStrCpyW(pwszKeyName, pBaseName);
    }
    if (pKey)
    {
        NlsStrCatW(pwszKeyName, pKey);
    }

    //
    //  Open the registry key.
    //
    RtlInitUnicodeString(&ObKeyName, pwszKeyName);
    InitializeObjectAttributes( &ObjA,
                                &ObKeyName,
                                OBJ_CASE_INSENSITIVE,
                                UserKeyHandle,
                                NULL );
    rc = NtOpenKey( phKeyHandle,
                    fAccess,
                    &ObjA );

    //
    //  Close the current user handle, if necessary.
    //
    if (UserKeyHandle != NULL)
    {
        NtClose(UserKeyHandle);
    }

    //
    //  Check for error from NtOpenKey.
    //
    if (!NT_SUCCESS(rc))
    {
        *phKeyHandle = NULL;
    }

    //
    //  Return the status from NtOpenKey.
    //
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  QueryRegValue
//
//  This routine queries the given value from the registry.
//
//  NOTE: If pIfAlloc is NULL, then no buffer will be allocated.
//        If this routine is successful, the CALLER must free the
//        ppKeyValueFull information buffer if *pIfAlloc is TRUE.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG QueryRegValue(
    HANDLE hKeyHandle,
    LPWSTR pValue,
    PKEY_VALUE_FULL_INFORMATION *ppKeyValueFull,
    ULONG Length,
    LPBOOL pIfAlloc)
{
    UNICODE_STRING ObValueName;        // value name
    PVOID pBuffer;                     // ptr to buffer for enum
    ULONG ResultLength;                // # bytes written
    ULONG rc = 0L;                     // return code


    //
    //  Set contents of pIfAlloc to FALSE to show that we did NOT do a
    //  memory allocation (yet).
    //
    if (pIfAlloc)
    {
        *pIfAlloc = FALSE;
    }

    //
    //  Query the value from the registry.
    //
    RtlInitUnicodeString(&ObValueName, pValue);

    RtlZeroMemory(*ppKeyValueFull, Length);
    rc = NtQueryValueKey( hKeyHandle,
                          &ObValueName,
                          KeyValueFullInformation,
                          *ppKeyValueFull,
                          Length,
                          &ResultLength );

    //
    //  Check the error code.  If the buffer is too small, allocate
    //  a new one and try the query again.
    //
    if ((rc == STATUS_BUFFER_OVERFLOW) && (pIfAlloc))
    {
        //
        //  Buffer is too small, so allocate a new one.
        //
        NLS_REG_BUFFER_ALLOC(*ppKeyValueFull, ResultLength, pBuffer, FALSE);
        RtlZeroMemory(*ppKeyValueFull, ResultLength);
        rc = NtQueryValueKey( hKeyHandle,
                              &ObValueName,
                              KeyValueFullInformation,
                              *ppKeyValueFull,
                              ResultLength,
                              &ResultLength );

        //
        //  Set contents of pIfAlloc to TRUE to show that we DID do
        //  a memory allocation.
        //
        *pIfAlloc = TRUE;
    }

    //
    //  If there is an error at this point, then the query failed.
    //
    if (rc != NO_ERROR)
    {
        if ((pIfAlloc) && (*pIfAlloc))
        {
            NLS_REG_BUFFER_FREE(pBuffer);
        }
        return (rc);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetRegValue
//
//  This routine sets the given value in the registry.
//
//  12-17-97    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG SetRegValue(
    HANDLE hKeyHandle,
    LPCWSTR pValue,
    LPCWSTR pData,
    ULONG DataLength)
{
    UNICODE_STRING ObValueName;        // value name


    //
    //  Set the value in the registry.
    //
    RtlInitUnicodeString(&ObValueName, pValue);

    return (NtSetValueKey( hKeyHandle,
                           &ObValueName,
                           0,
                           REG_SZ,
                           (PVOID)pData,
                           DataLength ));
}

////////////////////////////////////////////////////////////////////////////
//
//  CreateSectionTemp
//
//  This routine creates a temporary memory mapped section for the given file
//  name and returns the handle to the section.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG CreateSectionTemp(
    HANDLE *phSec,
    LPWSTR pwszFileName)
{
    HANDLE hFile = (HANDLE)0;          // file handle
    OBJECT_ATTRIBUTES ObjA;            // object attributes structure
    ULONG rc = 0L;                     // return code


    //
    //  Make sure we're in the critical section when entering this call.
    //
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    //
    //  Open the data file.
    //
    if (rc = OpenDataFile( &hFile,
                           pwszFileName ))
    {
        return (rc);
    }

    //
    //  Create the section.
    //
    InitializeObjectAttributes( &ObjA,
                                NULL,
                                0,
                                NULL,
                                NULL );

    rc = NtCreateSection( phSec,
                          SECTION_MAP_READ,
                          &ObjA,
                          NULL,
                          PAGE_READONLY,
                          SEC_COMMIT,
                          hFile );

    //
    //  Close the file.
    //
    NtClose(hFile);

    //
    //  Check for error from NtCreateSection.
    //
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Create Temp Section for %ws - %lx.\n",
                 pwszFileName, rc));
    }

    //
    //  Return success.
    //
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  OpenSection
//
//  This routine opens the named memory mapped section for the given section
//  name and returns the handle to the section.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG OpenSection(
    HANDLE *phSec,
    PUNICODE_STRING pObSectionName,
    PVOID *ppBaseAddr,
    ULONG AccessMask,
    BOOL bCloseHandle)
{
    OBJECT_ATTRIBUTES ObjA;            // object attributes structure
    ULONG rc = 0L;                     // return code


    //
    //  Make sure we're in the critical section when entering this call.
    //
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    //
    //  Open the Section.
    //
    InitializeObjectAttributes( &ObjA,
                                pObSectionName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    rc = NtOpenSection( phSec,
                        AccessMask,
                        &ObjA );

    //
    //  Check for error from NtOpenSection.
    //
    if (!NT_SUCCESS(rc))
    {
        return (rc);
    }

    //
    //  Map a View of the Section.
    //
    if (rc = MapSection( *phSec,
                         ppBaseAddr,
                         PAGE_READONLY,
                         FALSE ))
    {
        NtClose(*phSec);
        return (rc);
    }

    //
    //  Close the handle to the section.  Once the section has been mapped,
    //  the pointer to the base address will remain valid until the section
    //  is unmapped.  It is not necessary to leave the handle to the section
    //  around.
    //
    if (bCloseHandle)
    {
        NtClose(*phSec);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  MapSection
//
//  This routine maps a view of the section to the current process and adds
//  the appropriate information to the hash table.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG MapSection(
    HANDLE hSec,
    PVOID *ppBaseAddr,
    ULONG PageProtection,
    BOOL bCloseHandle)
{
    SIZE_T ViewSize;                   // view size of mapped section
    ULONG rc = 0L;                     // return code


    //
    //  Make sure we're in the critical section when entering this call.
    //
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    //
    //  Map a View of the Section.
    //
    *ppBaseAddr = (PVOID)NULL;
    ViewSize = 0L;

    rc = NtMapViewOfSection( hSec,
                             NtCurrentProcess(),
                             ppBaseAddr,
                             0L,
                             0L,
                             NULL,
                             &ViewSize,
                             ViewUnmap,
                             0L,
                             PageProtection );

    //
    //  Close the handle to the section.  Once the section has been mapped,
    //  the pointer to the base address will remain valid until the section
    //  is unmapped.  It is not necessary to leave the handle to the section
    //  around.
    //
    if (bCloseHandle)
    {
        NtClose(hSec);
    }

    //
    //  Check for error from NtMapViewOfSection.
    //
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Map View of Section - %lx.\n", rc));
        return (rc);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  UnMapSection
//
//  This routine unmaps a view of the given section to the current process.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG UnMapSection(
    PVOID pBaseAddr)
{
    ULONG rc = 0L;                     // return code


    //
    //  Make sure we're in the critical section when entering this call.
    //
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    //
    //  UnMap a View of the Section.
    //
    rc = NtUnmapViewOfSection( NtCurrentProcess(),
                               pBaseAddr );

    //
    //  Check for error from NtUnmapViewOfSection.
    //
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Unmap View of Section - %lx.\n", rc));
        return (rc);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetNlsSectionName
//
//  This routine returns a section name by concatenating the given
//  section prefix and the given integer value converted to a string.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG GetNlsSectionName(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pwszPrefix,
    LPWSTR pwszSecName)
{
    //
    //  Create section name string.
    //
    NlsStrCpyW(pwszSecName, pwszPrefix);
    return ( NlsConvertIntegerToString( Value,
                                        Base,
                                        Padding,
                                        pwszSecName + NlsStrLenW(pwszSecName),
                                        MAX_PATH_LEN ) );
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCodePageDLLPathName
//
//  This routine returns the full path name for the DLL file found in
//  the CodePage section of the registry for the given code page value.
//
//  10-23-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG GetCodePageDLLPathName(
    UINT CodePage,
    LPWSTR pDllName,
    USHORT cchLen)
{
    WCHAR pTmpBuf[MAX_SMALL_BUF_LEN];            // temp buffer
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query info
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];        // ptr to static buffer
    BOOL IfAlloc = FALSE;                        // if buffer was allocated
    ULONG rc = 0L;                               // return code


    //
    //  Open the CodePage registry key.
    //
    OPEN_CODEPAGE_KEY(ERROR_BADDB);

    //
    //  Convert the code page value to a unicode string.
    //
    if (rc = NlsConvertIntegerToString( CodePage,
                                        10,
                                        0,
                                        pTmpBuf,
                                        MAX_SMALL_BUF_LEN ))
    {
        return (rc);
    }

    //
    //  Query the registry for the code page value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    if (rc = QueryRegValue( hCodePageKey,
                            pTmpBuf,
                            &pKeyValueFull,
                            MAX_KEY_VALUE_FULLINFO,
                            &IfAlloc ))
    {
        return (rc);
    }

    //
    //  Make sure there is data with this value.
    //
    if (pKeyValueFull->DataLength > 2)
    {
        //
        //  Get the full path name for the DLL file.
        //
        GetSystemDirectoryW(pDllName, cchLen / 2);
        NlsStrCatW(pDllName, L"\\");
        NlsStrCatW(pDllName, GET_VALUE_DATA_PTR(pKeyValueFull));
    }
    else
    {
        rc = ERROR_INVALID_PARAMETER;
    }

    //
    //  Free the buffer used for the query.
    //
    if (IfAlloc)
    {
        NLS_FREE_MEM(pKeyValueFull);
    }

    //
    //  Return.
    //
    return (rc);
}




//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  OpenDataFile
//
//  This routine opens the data file for the specified file name and
//  returns the handle to the file.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG OpenDataFile(
    HANDLE *phFile,
    LPWSTR pFile)
{
    UNICODE_STRING ObFileName;         // file name
    OBJECT_ATTRIBUTES ObjA;            // object attributes structure
    IO_STATUS_BLOCK iosb;              // IO status block
    ULONG rc = 0L;                     // return code


    //
    //  Get the NT file name.
    //
    if (rc = GetNTFileName( pFile,
                            &ObFileName ))
    {
        return (rc);
    }

    //
    //  Open the file.
    //
    InitializeObjectAttributes( &ObjA,
                                &ObFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    rc = NtOpenFile( phFile,
                     FILE_READ_DATA | SYNCHRONIZE,
                     &ObjA,
                     &iosb,
                     FILE_SHARE_READ,
                     FILE_SYNCHRONOUS_IO_NONALERT );


    //
    //  Check for error from NtOpenFile.
    //
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Open File %wZ - %lx.\n", &ObFileName, rc));
        RtlFreeHeap(RtlProcessHeap(), 0, ObFileName.Buffer);
        return (rc);
    }
    if (!NT_SUCCESS(iosb.Status))
    {
        KdPrint(("NLSAPI: Could NOT Open File %wZ - Status = %lx.\n",
                 &ObFileName, iosb.Status));
        RtlFreeHeap(RtlProcessHeap(), 0, ObFileName.Buffer);
        return ((ULONG)iosb.Status);
    }

    //
    //  Return success.
    //
    RtlFreeHeap(RtlProcessHeap(), 0, ObFileName.Buffer);
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetNTFileName
//
//  This routine returns the full path name for the data file found in
//  the given registry information buffer.
//
//  NOTE: The pFileName parameter will contain a newly allocated buffer
//        that must be freed by the caller (pFileName->buffer).
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG GetNTFileName(
    LPWSTR pFile,
    PUNICODE_STRING pFileName)
{
    WCHAR pwszFilePath[MAX_PATH_LEN];  // ptr to file path string
    UNICODE_STRING ObFileName;         // file name
    ULONG rc = 0L;                     // return code


    //
    //  Get the full path name for the file.
    //
    GetSystemDirectoryW(pwszFilePath, MAX_PATH_LEN);
    NlsStrCatW(pwszFilePath, L"\\");
    NlsStrCatW(pwszFilePath, pFile);

    //
    //  Make the file name an NT path name.
    //
    RtlInitUnicodeString(&ObFileName, pwszFilePath);
    if (!RtlDosPathNameToNtPathName_U( ObFileName.Buffer,
                                       pFileName,
                                       NULL,
                                       NULL ))
    {
        KdPrint(("NLSAPI: Could NOT convert %wZ to NT path name - %lx.\n",
                 &ObFileName, rc));
        return (ERROR_FILE_NOT_FOUND);
    }

    //
    //  Return success.
    //
    return (NO_ERROR);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateSecurityDescriptor
//
//  This routine creates the security descriptor needed to create the
//  memory mapped section for a data file and returns the world SID.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG CreateSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    PSID *ppWorldSid,
    ACCESS_MASK AccessMask)
{
    ULONG rc = 0L;                     // return code
    PACL pAclBuffer;                   // ptr to ACL buffer
    ULONG SidLength;                   // length of SID - 1 sub authority
    PSID pWSid;                        // ptr to world SID
    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    PSID RestrictedSid;


    //
    //  Create World SID.
    //
    SidLength = RtlLengthRequiredSid(1);

    if ((pWSid = (PSID)NLS_ALLOC_MEM(SidLength)) == NULL)
    {
        *ppWorldSid = NULL;
        KdPrint(("NLSAPI: Could NOT Allocate SID Buffer.\n"));
        return (ERROR_OUTOFMEMORY);
    }
    *ppWorldSid = pWSid;

    RtlInitializeSid( pWSid,
                      &SidAuth,
                      1 );

    *(RtlSubAuthoritySid(pWSid, 0)) = SECURITY_WORLD_RID;

    rc = RtlAllocateAndInitializeSid( &NtAuth,
                                      1,
                                      SECURITY_RESTRICTED_CODE_RID,
                                      0, 0, 0, 0, 0, 0, 0,
                                      &RestrictedSid );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Allocate SID Buffer.\n"));
        return (rc);
    }

    //
    //  Initialize Security Descriptor.
    //
    rc = RtlCreateSecurityDescriptor( pSecurityDescriptor,
                                      SECURITY_DESCRIPTOR_REVISION );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Create Security Descriptor - %lx.\n", rc));
        goto CSD_Exit;
    }

    //
    //  Initialize ACL.
    //
    pAclBuffer = (PACL)((PBYTE)pSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
    rc = RtlCreateAcl( (PACL)pAclBuffer,
                       MAX_PATH_LEN * sizeof(ULONG),
                       ACL_REVISION2 );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Create ACL - %lx.\n", rc));
        goto CSD_Exit;
    }

    //
    //  Add an ACE to the ACL that allows World GENERIC_READ to the
    //  section object.
    //
    rc = RtlAddAccessAllowedAce( (PACL)pAclBuffer,
                                 ACL_REVISION2,
                                 AccessMask,
                                 pWSid );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Add Access Allowed ACE - %lx.\n", rc));
        goto CSD_Exit;
    }

    //
    //  Add an ACE to the ACL that allows Restricted GENERIC_READ to the
    //  section object.
    //
    rc = RtlAddAccessAllowedAce( (PACL)pAclBuffer,
                                 ACL_REVISION2,
                                 AccessMask,
                                 RestrictedSid );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Add Access Allowed ACE - %lx.\n", rc));
        goto CSD_Exit;
    }

    //
    //  Assign the DACL to the security descriptor.
    //
    rc = RtlSetDaclSecurityDescriptor( (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                                       (BOOLEAN)TRUE,
                                       (PACL)pAclBuffer,
                                       (BOOLEAN)FALSE );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Set DACL Security Descriptor - %lx.\n", rc));
        goto CSD_Exit;
    }

    //
    //  Set success.
    //
    rc = NO_ERROR;

CSD_Exit:
    //
    //  Free the Sid.
    //
    RtlFreeHeap(RtlProcessHeap(), 0, RestrictedSid);

    //
    //  Return the result.
    //
    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  AppendAccessAllowedACE
//
//  This routine adds an ACE to the ACL for administrators.
//
//  03-08-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG AppendAccessAllowedACE(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    ACCESS_MASK AccessMask)
{
    ULONG rc = 0L;                     // return code
    PACL pDaclBuffer;                  // ptr to DACL buffer
    ULONG SidLength;                   // length of SID - 2 sub authorities
    PSID pWSid;                        // ptr to world SID
    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_NT_AUTHORITY;
    BOOLEAN DaclPresent;
    BOOLEAN DaclDefaulted;


    //
    //  Create World SID.
    //
    SidLength = RtlLengthRequiredSid(1);

    if ((pWSid = (PSID)NLS_ALLOC_MEM(SidLength)) == NULL)
    {
        KdPrint(("NLSAPI: Could NOT Allocate SID Buffer.\n"));
        return (ERROR_OUTOFMEMORY);
    }

    RtlInitializeSid( pWSid,
                      &SidAuth,
                      1 );

    *(RtlSubAuthoritySid(pWSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;

    //
    //  Get DACL.
    //
    rc = RtlGetDaclSecurityDescriptor( pSecurityDescriptor,
                                       &DaclPresent,
                                       &pDaclBuffer,
                                       &DaclDefaulted );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Get DACL Security Descriptor - %lx.\n", rc));
        return (rc);
    }

    //
    //  Add an ACE to the ACL that allows Admin query access to the
    //  section object.
    //
    rc = RtlAddAccessAllowedAce( (PACL)pDaclBuffer,
                                 ACL_REVISION2,
                                 AccessMask,
                                 pWSid );
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Add Access Allowed ACE - %lx.\n", rc));
        return (rc);
    }

    //
    //  Free SID.
    //
    NLS_FREE_MEM(pWSid);

    //
    //  Return success.
    //
    return (NO_ERROR);
}
