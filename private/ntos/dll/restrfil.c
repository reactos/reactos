/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    restrfil.c

Abstract:

    This Module implements file system redirection for the ActiveX sandbox.

Author:

    Michael Warning (MikeW)     1-Mar-1998

Environment:

    User mode callable only

Revision History:

--*/


#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>

//
// We need this stuff to get the site directory.  This info should be moved
// into the job object eventually.
//

#define GetSiteSidFromToken xxxGetSiteSidFromToken
#define GetMangledSiteSid   xxxGetMangledSiteSid
#define IsTokenRestricted   xxxIsTokenRestricted
#include <winbase.h>
#undef GetSiteSidFromToken
#undef GetMangledSiteSid  
#undef IsTokenRestricted


void Base32Encode(LPVOID pvData, UINT cbData, LPWSTR pchData);
HRESULT GetMangledSiteSid(PSID pSid, ULONG cchMangledSite, LPWSTR *ppwszMangledSite);
PSID GetSiteSidFromToken(IN HANDLE TokenHandle);
BOOL IsTokenRestricted(IN HANDLE TokenHandle);



//
// Internal prototypes
//

BOOL     IsInterestingPath(
                    OBJECT_ATTRIBUTES *NormalFile, 
                    OBJECT_ATTRIBUTES *RestrictedFile);
NTSTATUS CopyRestrictedFile(
                    OBJECT_ATTRIBUTES *SourceAttributes, 
                    OBJECT_ATTRIBUTES *DestinationAttributes);
NTSTATUS CreateDirectories(OBJECT_ATTRIBUTES *Attributes);
BOOL     FileExists(OBJECT_ATTRIBUTES *Attributes);
NTSTATUS CopyStream(
                HANDLE SourceFile, 
                HANDLE DestinationFile, 
                FILE_STREAM_INFORMATION *StreamInfo,
                BYTE  *Buffer,
                ULONG  BufferSize);
NTSTATUS InitializeRestrictedStuff();

//
// UnRestricted versions of the api's
//

NTSTATUS NtUnRestrictedCreateFile(
                        OUT PHANDLE             FileHandle,
                        IN  ACCESS_MASK         DesiredAccess,
                        IN  POBJECT_ATTRIBUTES  ObjectAttributes,
                        OUT PIO_STATUS_BLOCK    IoStatusBlock,
                        IN  PLARGE_INTEGER      AllocationSize OPTIONAL,
                        IN  ULONG               FileAttributes,
                        IN  ULONG               ShareAccess,
                        IN  ULONG               CreateDisposition,
                        IN  ULONG               CreateOptions,
                        IN  PVOID               EaBuffer OPTIONAL,
                        IN  ULONG               EaLength);

NTSTATUS NtUnRestrictedOpenFile(
                        OUT PHANDLE             FileHandle,
                        IN  ACCESS_MASK         DesiredAccess,
                        IN  POBJECT_ATTRIBUTES  ObjectAttributes,
                        OUT PIO_STATUS_BLOCK    IoStatusBlock,
                        IN  ULONG               ShareAccess,
                        IN  ULONG               OpenOptions);
NTSTATUS NtUnRestrictedDeleteFile(
                        IN  POBJECT_ATTRIBUTES  ObjectAttributes);
NTSTATUS NtUnRestrictedQueryAttributesFile(
                        IN  POBJECT_ATTRIBUTES      ObjectAttributes,
                        OUT PFILE_BASIC_INFORMATION FileInformation);
NTSTATUS NtUnRestrictedSetInformationFile(
                        IN  HANDLE FileHandle,
                        OUT PIO_STATUS_BLOCK IoStatusBlock,
                        IN  PVOID FileInformation,
                        IN  ULONG Length,
                        IN  FILE_INFORMATION_CLASS FileInformationClass);

//
// Miscellaneous globals
//

enum            
{
    eUnRestricted = 0,
    eRestricted,
    eUnknownRestricted
}
Restricted = eUnknownRestricted;

UNICODE_STRING  SystemPath1 = {0, 0, 0};
UNICODE_STRING  SystemPath2 = {0, 0, 0};
UNICODE_STRING  SystemPath3 = {0, 0, 0};
UNICODE_STRING  SiteDirectory = {0, 0, 0};



void 
CheckRestricted()
/*++

Routine Description:

    Determine if this is a restricted process and thus should have filesystem
    redirection active.

Arguments:

    None

Return Value:

    void

--*/
{
    NTSTATUS            Status;
    HANDLE              hToken;
    UNICODE_STRING      SandboxKeyName;
    HANDLE              SandboxKey;
    OBJECT_ATTRIBUTES   Attributes;
    BYTE                Buffer[256];
    ULONG               Length;

    KEY_VALUE_PARTIAL_INFORMATION *ValueInfo = (KEY_VALUE_PARTIAL_INFORMATION *) Buffer;

    //
    // Assume unrestricted unless we have reason to believe otherwise
    //

    Restricted = eUnRestricted;

    //
    // HACKHACK: Temporarily allow redirection to be forced off by setting
    //           HKLM\Software\Sandbox!FileSystemRedir = (REG_DWORD) 0
    //

    RtlInitUnicodeString(&SandboxKeyName, L"\\Registry\\Machine\\Software\\Sandbox");

    InitializeObjectAttributes(
            &Attributes,
            &SandboxKeyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    Status = NtOpenKey(&SandboxKey, KEY_READ, &Attributes);

    if (!NT_SUCCESS(Status) && STATUS_OBJECT_NAME_NOT_FOUND != Status)
        return;

    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&SandboxKeyName, L"FileSystemRedir");

        Status = NtQueryValueKey(
                        SandboxKey,
                        &SandboxKeyName,
                        KeyValuePartialInformation,
                        ValueInfo,
                        sizeof(Buffer),
                        &Length);

        NtClose(SandboxKey);

        if (!NT_SUCCESS(Status) && STATUS_OBJECT_NAME_NOT_FOUND != Status)
            return;

        if (NT_SUCCESS(Status) && 0 == * (DWORD *) (&ValueInfo->Data))
            return;
    }

    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_READ, &hToken);

    if (NT_SUCCESS(Status))
    {
        if (IsTokenRestricted(hToken))
        {
            Status = InitializeRestrictedStuff();

            if (NT_SUCCESS(Status))
                Restricted = eRestricted;
        }

        NtClose(hToken);
    }
}
 

   
__inline
BOOL 
IsRestricted()
/*++

Routine Description:

    Determine if this is a restricted process and thus should have filesystem
    redirection active.

Arguments:

    None

Return Value:

    TRUE if this is a restricted process, otherwise FALSE

--*/
{
    if (eUnRestricted == Restricted)
        return FALSE;
    
    if (eUnknownRestricted == Restricted)
        CheckRestricted();

    return (eRestricted == Restricted);
}



NTSTATUS
NtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )
/*++

Routine Description:

    Entry point for the restricted version of NtCreateFile

Arguments:

    FileHandle - A pointer to a variable to receive the handle to the open file.

    DesiredAccess - Supplies the types of access that the caller would like to
        the file.

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Specifies the address of the caller's I/O status block.

    AllocationSize - Initial size that should be allocated to the file.  This
        parameter only has an affect if the file is created.  Further, if
        not specified, then it is taken to mean zero.

    FileAttributes - Specifies the attributes that should be set on the file,
        if it is created.

    ShareAccess - Supplies the types of share access that the caller would like
        to the file.

    CreateDisposition - Supplies the method for handling the create/open.

    CreateOptions - Caller options for how to perform the create/open.

    EaBuffer - Optionally specifies a set of EAs to be applied to the file if
        it is created.

    EaLength - Supplies the length of the EaBuffer.

Return Value:

    The function value is the final status of the create/open operation.

--*/
{
#define CALL_CREATE(object)                 \
            NtUnRestrictedCreateFile(       \
                    FileHandle,             \
                    DesiredAccess,          \
                    object,                 \
                    IoStatusBlock,          \
                    AllocationSize,         \
                    FileAttributes,         \
                    ShareAccess,            \
                    CreateDisposition,      \
                    CreateOptions,          \
                    EaBuffer,               \
                    EaLength)                       
    /*\
        ((st =                                       \
       ) ? st                                        \
    :  ((Restricted == eRestricted) ? DbgPrint("%s %wZ\n", (object==NormalFile)?"UnMapped":"  Mapped",object->ObjectName) : 0), st)
*/
                    
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES  *OriginalAttributes;
    OBJECT_ATTRIBUTES  *NormalFile;
    OBJECT_ATTRIBUTES   RestrictedFileAttributes;
    OBJECT_ATTRIBUTES  *RestrictedFile = &RestrictedFileAttributes;
    UNICODE_STRING      RestrictedObjectName;

    FILE_BASIC_INFORMATION  BasicInfo;

//NTSTATUS st;

    if (!IsRestricted())
        return CALL_CREATE(ObjectAttributes);

    //
    // Check to see if this file is mappable.  If not don't try
    //

    NormalFile = ObjectAttributes;

    CopyMemory(RestrictedFile, NormalFile, sizeof(OBJECT_ATTRIBUTES));
    RestrictedFile->ObjectName = &RestrictedObjectName;

    if (!IsInterestingPath(NormalFile, RestrictedFile))
        return CALL_CREATE(NormalFile);

    //
    // The file/directory is mappable
    //

    //
    // If this is a directory, try to access the normal one first, then the 
    // shadowed one
    //
    if (CreateOptions & FILE_DIRECTORY_FILE)
    {
        Status = CALL_CREATE(NormalFile);

        if (!NT_SUCCESS(Status))
        {
            Status = CALL_CREATE(RestrictedFile);
        }
    }

    //
    // If this is a file which which already has a shadowed version, try the
    // operation only on that version
    //
    else if (FileExists(RestrictedFile))
    {
        Status = CALL_CREATE(RestrictedFile);
    }

    //
    // This is a file operation and no shadowed version of the file exists,
    // try the operation in the unshadowed area
    //
    else
    {
        Status = CALL_CREATE(NormalFile);

        if (STATUS_ACCESS_DENIED == Status)
        {
            Status = CreateDirectories(RestrictedFile);
            
            if (NT_SUCCESS(Status))
            {
                Status = CopyRestrictedFile(NormalFile, RestrictedFile);

                if (NT_SUCCESS(Status) 
                    || STATUS_OBJECT_NAME_NOT_FOUND == Status)
                {
                    Status = CALL_CREATE(RestrictedFile);
                }
            }
        }
    }


/*
if (!NT_SUCCESS(Status))
  DbgPrint("  Failed %wZ\n", NormalFile->ObjectName);
*/
    RtlFreeHeap(RtlProcessHeap(), 0, RestrictedObjectName.Buffer);

    return Status;
    
#undef CALL_CREATE
}
NTSTATUS
ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )
{
    return NtCreateFile(
                    FileHandle,
                    DesiredAccess,
                    ObjectAttributes,
                    IoStatusBlock,
                    AllocationSize,
                    FileAttributes,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    EaBuffer,
                    EaLength);
}



NTSTATUS
NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
/*++

Routine Description:

    Entry point for the restricted version of NtOpenFile

Arguments:

    FileHandle - A pointer to a variable to receive the handle to the open file.

    DesiredAccess - Supplies the types of access that the caller would like to
        the file.

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    IoStatusBlock - Specifies the address of the caller's I/O status block.

    ShareAccess - Supplies the types of share access that the caller would like
        to the file.

    OpenOptions - Caller options for how to perform the open.

Return Value:

    The function value is the final completion status of the open/create
    operation.

--*/
{
#define CALL_OPEN(object)                 \
            NtUnRestrictedOpenFile(       \
                    FileHandle,             \
                    DesiredAccess,          \
                    object,                 \
                    IoStatusBlock,          \
                    ShareAccess,            \
                    OpenOptions)                   
    /*\
        ((st =                                       \
       ) ? st                                        \
    :  ((Restricted == eRestricted) ? DbgPrint("%s %wZ\n", (object==NormalFile)?"UnMapped":"  Mapped",object->ObjectName) : 0), st)
*/
                    
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES  *OriginalAttributes;
    OBJECT_ATTRIBUTES  *NormalFile;
    OBJECT_ATTRIBUTES   RestrictedFileAttributes;
    OBJECT_ATTRIBUTES  *RestrictedFile = &RestrictedFileAttributes;
    UNICODE_STRING      RestrictedObjectName;

    FILE_BASIC_INFORMATION  BasicInfo;

//NTSTATUS st;

    if (!IsRestricted())
        return CALL_OPEN(ObjectAttributes);

    //
    // Check to see if this file is mappable.  If not don't try
    //

    NormalFile = ObjectAttributes;

    CopyMemory(RestrictedFile, NormalFile, sizeof(OBJECT_ATTRIBUTES));
    RestrictedFile->ObjectName = &RestrictedObjectName;

    if (!IsInterestingPath(NormalFile, RestrictedFile))
        return CALL_OPEN(NormalFile);

    //
    // The file/directory is mappable
    //

    if (OpenOptions & FILE_DIRECTORY_FILE)
    {
        //
        // This is a directory, attempt to access the given one, if that fails
        // try to access the shadowed version
        //

        Status = CALL_OPEN(NormalFile);

        if (!NT_SUCCESS(Status))
        {
            Status = CALL_OPEN(RestrictedFile);
        }
    }
    else
    {
        //
        // This is a file (not a directory).  Attempt to open the shadowed
        // version.  If that fails, attempt to open the real version.  If that
        // fails with access denied, then try to copy the file to the shadow
        // area and try to open it there again.
        //

        Status = CALL_OPEN(RestrictedFile);

        if (!NT_SUCCESS(Status))
        {           
            Status = CALL_OPEN(NormalFile);

            if (!NT_SUCCESS(Status))
            {
                Status = CopyRestrictedFile(NormalFile, RestrictedFile);

                if (NT_SUCCESS(Status))
                {
                    Status = CALL_OPEN(RestrictedFile);
                }
            }
        }
    }
/*
if (!NT_SUCCESS(Status))
  DbgPrint("  Failed %wZ\n", NormalFile->ObjectName);
*/
    RtlFreeHeap(RtlProcessHeap(), 0, RestrictedObjectName.Buffer);

    return Status;
    
#undef CALL_OPEN
}
NTSTATUS
ZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
{
    return NtOpenFile(
                FileHandle,
                DesiredAccess,
                ObjectAttributes,
                IoStatusBlock,
                ShareAccess,
                OpenOptions);
}



NTSTATUS NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
/*++

Routine Description:

    Attempt to delete the shadowed version of the given file.  If that fails
    attempt to delete the real version of the file

Arguments:

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES  *NormalFile;
    OBJECT_ATTRIBUTES   RestrictedFileAttributes;
    OBJECT_ATTRIBUTES  *RestrictedFile = &RestrictedFileAttributes;
    UNICODE_STRING      RestrictedObjectName;

    if (!IsRestricted())
        return NtUnRestrictedDeleteFile(ObjectAttributes);

    //
    // Check to see if this file is mappable.  Map it if it is
    //

    NormalFile = ObjectAttributes;

    CopyMemory(RestrictedFile, NormalFile, sizeof(OBJECT_ATTRIBUTES));
    RestrictedFile->ObjectName = &RestrictedObjectName;

    if (IsInterestingPath(NormalFile, RestrictedFile))
    {
        Status = NtUnRestrictedDeleteFile(RestrictedFile);
        
        RtlFreeHeap(RtlProcessHeap(), 0, RestrictedObjectName.Buffer);
            
        if (NT_SUCCESS(Status) || STATUS_ACCESS_DENIED == Status)
            return Status;
    }

    //
    // The file is not mappable or deleting the mapped version failed.
    //

    return NtUnRestrictedDeleteFile(NormalFile);
}
NTSTATUS ZwDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    return NtDeleteFile(ObjectAttributes);
}



NTSTATUS
NtQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
    )
/*++

Routine Description:

    Attempt to query the shadowed version of the given file.  If that fails
    attempt to query the real version of the file

Arguments:

    ObjectAttributes - Supplies the attributes to be used for file object (name,
        SECURITY_DESCRIPTOR, etc.)

    FileInformation - Supplies an output buffer to receive the returned file
        attributes information.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES  *NormalFile;
    OBJECT_ATTRIBUTES   RestrictedFileAttributes;
    OBJECT_ATTRIBUTES  *RestrictedFile = &RestrictedFileAttributes;
    UNICODE_STRING      RestrictedObjectName;

    if (!IsRestricted())
        return NtUnRestrictedQueryAttributesFile(
                        ObjectAttributes, 
                        FileInformation);

    //
    // Check to see if this file is mappable.  Map it if it is
    //

    NormalFile = ObjectAttributes;

    CopyMemory(RestrictedFile, NormalFile, sizeof(OBJECT_ATTRIBUTES));
    RestrictedFile->ObjectName = &RestrictedObjectName;

    if (IsInterestingPath(NormalFile, RestrictedFile))
    {
        Status = NtUnRestrictedQueryAttributesFile(
                        RestrictedFile, 
                        FileInformation);

        RtlFreeHeap(RtlProcessHeap(), 0, RestrictedObjectName.Buffer);
            
        if (NT_SUCCESS(Status) || STATUS_ACCESS_DENIED == Status)
            return Status;
    }

    //
    // The file is not mappable or accessing the mapped version failed.
    //

    return NtUnRestrictedQueryAttributesFile(
                    NormalFile, 
                    FileInformation);
}
NTSTATUS
ZwQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
    )
{
    return NtQueryAttributesFile(ObjectAttributes, FileInformation);
}



NTSTATUS
NtSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
/*++

Routine Description:

    Hook NtSetInformationFile to catch renames/moves of files by restricted 
    processes.

Arguments:

    FileHandle - Supplies a handle to the file whose information should be
        changed.

    IoStatusBlock - Address of the caller's I/O status block.

    FileInformation - Supplies a buffer containing the information which should
        be changed on the file.

    Length - Supplies the length, in bytes, of the FileInformation buffer.

    FileInformationClass - Specifies the type of information which should be
        changed about the file.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   NormalFileAttributes;
    OBJECT_ATTRIBUTES  *NormalFile = &NormalFileAttributes;
    UNICODE_STRING      NormalFileName;
    OBJECT_ATTRIBUTES   RestrictedFileAttributes;
    OBJECT_ATTRIBUTES  *RestrictedFile = &RestrictedFileAttributes;
    UNICODE_STRING      RestrictedObjectName;

    FILE_RENAME_INFORMATION *RenameInfo;

    //
    // Try the original operation first
    //

    Status = NtUnRestrictedSetInformationFile(
                        FileHandle, 
                        IoStatusBlock, 
                        FileInformation, 
                        Length, 
                        FileInformationClass);

    if (NT_SUCCESS(Status) 
        || !IsRestricted() 
        || FileRenameInformation != FileInformationClass)
    {
        return Status;
    }

    //
    // Check to see if this file is mappable.  Map it if it is
    //

    RenameInfo = (FILE_RENAME_INFORMATION *) FileInformation;

    NormalFileName.MaximumLength = (USHORT) RenameInfo->FileNameLength;
    NormalFileName.Length        = (USHORT) RenameInfo->FileNameLength;
    NormalFileName.Buffer        = RenameInfo->FileName;

    InitializeObjectAttributes(
            NormalFile,
            &NormalFileName,
            OBJ_CASE_INSENSITIVE,
            RenameInfo->RootDirectory,
            NULL);

    CopyMemory(RestrictedFile, NormalFile, sizeof(OBJECT_ATTRIBUTES));
    RestrictedFile->ObjectName = &RestrictedObjectName;

    if (IsInterestingPath(NormalFile, RestrictedFile))
    {
        FILE_RENAME_INFORMATION *NewRenameInfo;
        NTSTATUS                 Status2;

        NewRenameInfo = RtlAllocateHeap(
                            RtlProcessHeap(), 
                            0, 
                            sizeof(*RenameInfo) + RestrictedObjectName.Length);

        if (NULL != RenameInfo)
        {
            NewRenameInfo->ReplaceIfExists = RenameInfo->ReplaceIfExists;
            NewRenameInfo->RootDirectory   = NULL;
            NewRenameInfo->FileNameLength  = RestrictedObjectName.Length;
            CopyMemory(
                    NewRenameInfo->FileName, 
                    RestrictedObjectName.Buffer, 
                    RestrictedObjectName.Length);

            Status2 = NtUnRestrictedSetInformationFile(
                                FileHandle, 
                                IoStatusBlock, 
                                NewRenameInfo, 
                                sizeof(*RenameInfo) + RestrictedObjectName.Length, 
                                FileInformationClass);

            Status = NT_SUCCESS(Status2) ? Status2 : Status;

            RtlFreeHeap(RtlProcessHeap(), 0, NewRenameInfo);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, RestrictedObjectName.Buffer);
    }

    return Status;
}
NTSTATUS
ZwSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return NtSetInformationFile(
                    FileHandle, 
                    IoStatusBlock, 
                    FileInformation, 
                    Length, 
                    FileInformationClass);
}



NTSTATUS InitializeRestrictedStuff()
/*++

Routine Description:

    Determine if this process needs to have file mapping turned on and if so
    figure out the paths to the system drive and shadow area

Arguments:

    None

Return Value:

    STATUS_SUCCESS if file mapping support was initialized.

--*/
{
    OBJECT_ATTRIBUTES   Attributes;
    UNICODE_STRING      Path;
    UNICODE_STRING      ProfileDirectory;
    HKEY                ProfileKey;
    HKEY                UserProfileKey;
    WCHAR               Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+128/sizeof(WCHAR)];
    ULONG               BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION)+128;
    IO_STATUS_BLOCK     IoStatus;
    NTSTATUS            Status;
    HANDLE              SystemRoot;
    UNICODE_STRING     *TempString;

    HANDLE              hToken;
    PSID                SiteSid;
    WCHAR               MangledSiteBuffer[MAX_MANGLED_SITE];
    LPWSTR              MangledSite = MangledSiteBuffer;

    static BOOL         Initialized = FALSE;

    KEY_VALUE_PARTIAL_INFORMATION  *KeyInfo;

    if (Initialized)
        return STATUS_SUCCESS;

    //
    // Figure out the path to \Device\<systemvolume>.  We need to open a
    // handle to SystemRoot and then query the name of that handle.
    //

    Path.MaximumLength = sizeof(Buffer);
    Path.Length        = 0;
    Path.Buffer        = Buffer;
    RtlAppendUnicodeToString(&Path, L"\\??\\");
    RtlAppendUnicodeToString(&Path, USER_SHARED_DATA->NtSystemRoot);

    InitializeObjectAttributes(
            &Attributes,
            &Path,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    Status = NtUnRestrictedOpenFile(
                        &SystemRoot, 
                        GENERIC_READ | SYNCHRONIZE, 
                        &Attributes, 
                        &IoStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtQueryObject(
                    SystemRoot, 
                    ObjectNameInformation,
                    Buffer,
                    sizeof(Buffer),
                    NULL);

    NtClose(SystemRoot);

    if (!NT_SUCCESS(Status))
        return Status;

    // The path is the name of the SystemRoot handle minus the length of 
    // NtSystemRoot minus the drive letter
    
    TempString = & ((OBJECT_NAME_INFORMATION *) Buffer)->Name;
    TempString->Length -= sizeof(WCHAR) * 
                                    (wcslen(USER_SHARED_DATA->NtSystemRoot) 
                                                - sizeof("d"));

    TempString->Buffer[TempString->Length / sizeof(WCHAR)] = L'\0';
         
    Status = RtlCreateUnicodeString(
                        &SystemPath1, 
                        TempString->Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

//DbgPrint("SystemPath1 = %wZ\n", &SystemPath1);

    // Figure out the path to \??\<systemdrive>

    Path.Length        = 0;
    Path.Buffer        = Buffer;
    RtlAppendUnicodeToString(&Path, L"\\??\\");
    RtlAppendUnicodeToString(&Path, USER_SHARED_DATA->NtSystemRoot);
    Path.Buffer[sizeof("\\??\\d")] = L'\0';

    Status = RtlCreateUnicodeString(&SystemPath2, Path.Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

//DbgPrint("SystemPath2 = %wZ\n", &SystemPath2);

    // Figure out the path to \DosDevices\<systemdrive>

    Path.Length        = 0;
    Path.Buffer        = Buffer;
    RtlAppendUnicodeToString(&Path, L"\\DosDevices\\");
    RtlAppendUnicodeToString(&Path, USER_SHARED_DATA->NtSystemRoot);
    Path.Buffer[sizeof("\\DosDevices\\d")] = L'\0';

    Status = RtlCreateUnicodeString(&SystemPath3, Path.Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

//DbgPrint("SystemPath3 = %wZ\n", &SystemPath3);

    //
    // Figure out where the site directory is
    //
    // This code is temporary.  Eventually the shadow directory will be stored
    // in the job object
    //

    // First open the list of profile locations

    RtlInitUnicodeString(&Path, L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList");

    InitializeObjectAttributes(
            &Attributes,
            &Path,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    Status = NtOpenKey(&ProfileKey, KEY_READ, &Attributes);

    if (!NT_SUCCESS(Status))
        return Status;

    // Next get a stringized version of the user's SID

    Status = RtlFormatCurrentUserKeyPath(&Path);

    if (!NT_SUCCESS(Status))
    {
        NtClose(ProfileKey);
        return Status;
    }

    Path.Buffer += sizeof("\\Registry\\User");
    Path.Length -= sizeof(L"\\Registry\\User");

    // And open the profile for the current user

    Attributes.RootDirectory = ProfileKey;

    Status = NtOpenKey(&UserProfileKey, KEY_READ, &Attributes);

    NtClose(ProfileKey);
    Path.Buffer -= sizeof("\\Registry\\User");
    Path.Length += sizeof(L"\\Registry\\User");
    RtlFreeUnicodeString(&Path);

    if (!NT_SUCCESS(Status))
        return Status;

    // Allocate a buffer and get the info

    RtlInitUnicodeString(&Path, L"ProfileImagePath");

    do
    {
        ULONG ResultLength;

        KeyInfo = RtlAllocateHeap(RtlProcessHeap(), 0, BufferSize);

        if (NULL == KeyInfo)
        {
            NtClose(UserProfileKey);
            Status = STATUS_NO_MEMORY;
            break;
        }

        Status = NtQueryValueKey(
                        UserProfileKey,
                        &Path,
                        KeyValuePartialInformation,
                        KeyInfo,
                        BufferSize,
                        &ResultLength);

        if (!NT_SUCCESS(Status))
        {
            BufferSize *= 2;
            RtlFreeHeap(RtlProcessHeap(), 0, KeyInfo);
        }
    }
    while (   STATUS_BUFFER_OVERFLOW == Status 
           || STATUS_BUFFER_TOO_SMALL == Status);

    NtClose(UserProfileKey);

    if (!NT_SUCCESS(Status))
        return Status;

    // Create the path to the site directory

    Path.Length         = 0;
    Path.MaximumLength  = sizeof(Buffer);
    Path.Buffer         = Buffer;

    ProfileDirectory.Length        = (USHORT) KeyInfo->DataLength;
    ProfileDirectory.MaximumLength = (USHORT) KeyInfo->DataLength;
    ProfileDirectory.Buffer        = (WCHAR *) KeyInfo->Data;

    Status = RtlExpandEnvironmentStrings_U(NULL, &ProfileDirectory, &Path, NULL);

    if (!NT_SUCCESS(Status))
        return Status;

    // RtlExpandEnvironmentStrings_U includes the null terminator in the length

    Path.Length        -= sizeof(L'\0');

    // Remove the drive letter from the path

    Path.Length        -= 2 * sizeof(WCHAR);
    MoveMemory(Path.Buffer, Path.Buffer + 2, Path.Length);

    // 
    // Figure out the site directory name
    //

    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_READ, &hToken);

    if (!NT_SUCCESS(Status))
        return Status;

    SiteSid = GetSiteSidFromToken(hToken);

    NtClose(hToken);

    if (NULL == SiteSid)
        return STATUS_UNSUCCESSFUL;

    Status = GetMangledSiteSid(SiteSid, MAX_MANGLED_SITE, &MangledSite);

    RtlFreeSid(SiteSid);

    if (!NT_SUCCESS(Status))
        return Status;

    RtlAppendUnicodeToString(&Path, L"\\");
    RtlAppendUnicodeToString(&Path, MangledSite);

    Path.Buffer[Path.Length / sizeof(WCHAR)] = L'\0';

    Status = RtlCreateUnicodeString(&SiteDirectory, Path.Buffer);

//DbgPrint("Site Directory = %wZ\n", &SiteDirectory);

    Initialized = NT_SUCCESS(Status);

    return Status;
}



BOOL IsInterestingPath(
                IN     POBJECT_ATTRIBUTES NormalFile, 
                IN OUT POBJECT_ATTRIBUTES RestrictedFile)
/*++

Routine Description:

    Determine if this path pointed to by NormalFile can be mapped and if so
    point RestrictedFile at the mapped version

Arguments:

    NormalFile - The file to be tested
    RestrictedFile - The resultant restricted file

Return Value:

    STATUS_SUCCESS if RestrictedFile was setup

--*/
{
    BYTE                    Buffer[1024];
    OBJECT_NAME_INFORMATION *NameInfo = (OBJECT_NAME_INFORMATION *) Buffer;
    NTSTATUS                Status;
    BOOLEAN                 CaseInSensitive;
    UNICODE_STRING         *SystemPath = NULL;
    UNICODE_STRING         *RestrictedName;
    
    //
    // Expand out the root directory of a relative path, if applicable
    //

    if (NULL != NormalFile->RootDirectory)
    {
        WCHAR  LastChar;

        // Get the name corresponding to the root handle

        Status = NtQueryObject(
                        NormalFile->RootDirectory,
                        ObjectNameInformation,
                        NameInfo,
                        sizeof(Buffer),
                        NULL);

        if (!NT_SUCCESS(Status))
            return FALSE;

        // Make sure there's a backslash on the end

        LastChar = NameInfo->Name.Buffer[
                                (NameInfo->Name.Length-1)*sizeof(WCHAR)];

        if (L'\\' != LastChar)
        {
            Status = RtlAppendUnicodeToString(&NameInfo->Name, L"\\");

            if (!NT_SUCCESS(Status))
                return FALSE;
        }
    }
    else
    {
        NameInfo->Name.Length = 0;
        NameInfo->Name.Buffer = (WCHAR *) (((BYTE *) NameInfo) 
                                                        + sizeof(*NameInfo));
    }

    //
    // Concatenate the file/directory to it's root to get a full path
    //

    NameInfo->Name.MaximumLength = sizeof(Buffer) 
                                        - sizeof(*NameInfo) 
                                        - NameInfo->Name.Length;

    Status = RtlAppendUnicodeStringToString(
                            &NameInfo->Name, 
                            NormalFile->ObjectName);

    if (!NT_SUCCESS(Status))
        return FALSE;
    
    //
    // Check to see if the path should be mapped
    //

    CaseInSensitive = (BOOLEAN) 
                        ((OBJ_CASE_INSENSITIVE & NormalFile->Attributes) != 0);

    if (RtlPrefixUnicodeString(&SystemPath1, &NameInfo->Name, CaseInSensitive))
        SystemPath = &SystemPath1;
    else
    if (RtlPrefixUnicodeString(&SystemPath2, &NameInfo->Name, CaseInSensitive))
        SystemPath = &SystemPath2;
    else
    if (RtlPrefixUnicodeString(&SystemPath3, &NameInfo->Name, CaseInSensitive))
        SystemPath = &SystemPath3;

    if (NULL == SystemPath)
        return FALSE;

    //
    // This path is on the system drive, reject it if it's pointing at the
    // mapped area already
    //

    NameInfo->Name.Length -= SystemPath->Length;
    NameInfo->Name.Buffer += SystemPath->Length / sizeof(WCHAR);

    if (RtlPrefixUnicodeString(&SiteDirectory,&NameInfo->Name,CaseInSensitive))
        return FALSE;

    NameInfo->Name.Length += SystemPath->Length;
    NameInfo->Name.Buffer -= SystemPath->Length / sizeof(WCHAR);

    //
    // This path should be mapped
    //

    RestrictedName = RestrictedFile->ObjectName;
    RestrictedName->MaximumLength = SystemPath1.Length
                                    + SiteDirectory.Length
                                    + NameInfo->Name.Length;
    RestrictedName->Buffer = RtlAllocateHeap(
                                    RtlProcessHeap(), 
                                    0, 
                                    RestrictedName->MaximumLength);

    if (NULL == RestrictedName->Buffer)
        return FALSE;

    NameInfo->Name.Buffer[NameInfo->Name.Length] = L'\0';

    RestrictedFile->RootDirectory = NULL;
    RestrictedName->Length = 0;

    RtlCopyUnicodeString(RestrictedName, &SystemPath1);
    RtlAppendUnicodeStringToString(RestrictedName, &SiteDirectory);
    RtlAppendUnicodeToString(
                RestrictedName, 
                NameInfo->Name.Buffer + SystemPath->Length / sizeof(WCHAR));

    return TRUE;
}



NTSTATUS CreateDirectories(OBJECT_ATTRIBUTES *Attributes)
/*++

Routine Description:

    Given the path pointed to by Attributes, make sure all the directories
    above the last element in the path exist.

Parameters:

    Attributes - The path

Return Value:

    STATUS_SUCCESS if all goes well

--*/
{
    NTSTATUS            Status;
    UNICODE_STRING      SubDirectory;
    OBJECT_ATTRIBUTES   DirectoryAttributes;
    HANDLE              NewDirectory = NULL;
    WCHAR              *NextDirectory;
    WCHAR              *EndOfString;
    IO_STATUS_BLOCK     IoStatus;

    ASSERT(NULL == Attributes->RootDirectory);

    InitializeObjectAttributes(
            &DirectoryAttributes,
            &SubDirectory,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    // 
    // Keep track of the end of the path and trim off any trailing '\'s
    //

    ASSERT(Attributes->ObjectName->Length >= 2);

    EndOfString         = Attributes->ObjectName->Buffer 
                            + Attributes->ObjectName->Length / sizeof(WCHAR);

    if (L'\\' == EndOfString[-1])
        --EndOfString;

    // 
    // Keep of a private version of the path so we can muck with it
    //

    SubDirectory.Buffer = Attributes->ObjectName->Buffer;
    NextDirectory       = SubDirectory.Buffer;
    NextDirectory      += SiteDirectory.Length / sizeof(WCHAR);

    //
    // For each element in the path, try to create a directory using the full
    // path up to that element.
    //
    // Notes: CreateFile for "\??" returns STATUS_OBJECT_TYPE_MISMATCH
    //        CreateFile for "\??\D:" returns STATUS_INVALID_PARAMETER
    //

    do
    {
        // Find the end of the next element

        do
        {
            ++NextDirectory;
        }
        while (NextDirectory < EndOfString && L'\\' != *NextDirectory);

        if ( !(NextDirectory < EndOfString) )
            break;

        // Adjust SubDirectory to include it

        SubDirectory.Length        = (NextDirectory - SubDirectory.Buffer);
        SubDirectory.Length       *= sizeof(WCHAR);
        SubDirectory.MaximumLength = SubDirectory.Length;

        // Create it

        Status = NtUnRestrictedCreateFile(
                        &NewDirectory,
                        GENERIC_READ | SYNCHRONIZE,
                        &DirectoryAttributes,
                        &IoStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0);

        if (NT_SUCCESS(Status))
            NtClose(NewDirectory);

        NewDirectory = NULL;
    }
    while ((NT_SUCCESS(Status) 
                    || STATUS_OBJECT_TYPE_MISMATCH == Status
                    || STATUS_INVALID_PARAMETER    == Status)
           && NextDirectory < EndOfString);

    return Status;
}



BOOL FileExists(OBJECT_ATTRIBUTES *Attributes)

/*++

Routine Description:

    Determine if the given file/directory exists

Arguments:

    Attributes      - The file/directory to check

Return Value:

    TRUE if it exists 

--*/
{
    NTSTATUS                Status;
    FILE_BASIC_INFORMATION  FileInfo;

    Status = NtQueryAttributesFile(Attributes, &FileInfo);

    return (STATUS_SUCCESS == Status);
}



NTSTATUS CopyStream(
                HANDLE SourceFile, 
                HANDLE DestinationFile, 
                FILE_STREAM_INFORMATION *StreamInfo,
                BYTE  *Buffer,
                ULONG  BufferSize)
/*++

Routine Description:

    Copy a stream from one file to another

Arguments:

    SourceFile      - The source file
    DestinationFile - The destination file
    StreamInfo      - Information about the stream to copy
    Buffer          - The buffer to use for copying
    BufferSize      - The size of Buffer

Return Value:

    STATUS_SUCCESS if all goes well

--*/
{
    NTSTATUS            Status;
    UNICODE_STRING      StreamName;
    OBJECT_ATTRIBUTES   SourceAttributes;
    OBJECT_ATTRIBUTES   DestinationAttributes;
    HANDLE              SourceStream;
    HANDLE              DestinationStream;
    IO_STATUS_BLOCK     SourceIoStatus;
    IO_STATUS_BLOCK     DestinationIoStatus;

    StreamName.MaximumLength = (USHORT) StreamInfo->StreamNameLength;
    StreamName.Length        = (USHORT) StreamInfo->StreamNameLength;
    StreamName.Buffer        = StreamInfo->StreamName;

    //
    // Data stream names are of the form ":<name>:$DATA" so if the second
    // char in the name is a colon then this is the default stream and we
    // don't need to open/create it
    //

    if (L':' == StreamInfo->StreamName[1])
    {
        SourceStream = SourceFile;
        DestinationStream = DestinationFile;
    }
    else
    {
        InitializeObjectAttributes(
                &SourceAttributes,
                &StreamName,
                OBJ_CASE_INSENSITIVE,
                SourceFile,
                NULL);            
        InitializeObjectAttributes(
                &DestinationAttributes,
                &StreamName,
                OBJ_CASE_INSENSITIVE,
                DestinationFile,
                NULL);

        Status = NtUnRestrictedOpenFile(
                        &SourceStream,
                        GENERIC_READ | SYNCHRONIZE,
                        &SourceAttributes,
                        &SourceIoStatus,
                        FILE_SHARE_READ,
                        FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT);

        if (!NT_SUCCESS(Status))
            return Status;

        Status = NtUnRestrictedCreateFile(
                        &DestinationStream,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &DestinationAttributes,
                        &DestinationIoStatus,
                        &StreamInfo->StreamAllocationSize,
                        FILE_ATTRIBUTE_NORMAL,
                        0,
                        FILE_CREATE,
                        FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0);

        if (!NT_SUCCESS(Status))
        {
            NtClose(SourceStream);
            return Status;
        }
    }

    //
    // Copy the stream
    //

    do
    {
        Status = NtReadFile(
                        SourceStream,
                        NULL,
                        NULL,
                        NULL,
                        &SourceIoStatus,
                        Buffer,
                        BufferSize,
                        0,
                        NULL);

        if (STATUS_END_OF_FILE == Status)
        {
            Status = STATUS_SUCCESS;
            break;
        }
        else if (NT_SUCCESS(Status))
        {
            Status = NtWriteFile(
                            DestinationStream,
                            NULL,
                            NULL,
                            NULL,
                            &DestinationIoStatus,
                            Buffer,
                            SourceIoStatus.Information,
                            0,
                            NULL);
        }
    }
    while (NT_SUCCESS(Status) && SourceIoStatus.Information == BufferSize);

    if (SourceStream != SourceFile)
    {
        NtClose(SourceStream);
        NtClose(DestinationStream);
    }

    return Status;
}



NTSTATUS CopyRestrictedFile(
                    OBJECT_ATTRIBUTES *SourceAttributes, 
                    OBJECT_ATTRIBUTES *DestinationAttributes)
/*++

Routine Description:

    Copy a file.  Substreams in the file are copied and a best effort is made
    to preserve the file attributes - although the copy does not fail if
    the attributes can't be preserved.  Acl's and extended attributes such
    as object id's are not copied.

    If the any of the parent directories of the destination don't exist they
    are created.

Arguments:

    SourceAttributes        - The source file
    DestinationAttributues  - The destination file

Return Value:

    STATUS_SUCCESS if all goes well

    The copy fails if the destination already exists or the source is read-only

--*/
{
    NTSTATUS        Status;
    HANDLE          SourceFile = NULL;
    HANDLE          DestinationFile = NULL;
    IO_STATUS_BLOCK SourceIoStatus;
    IO_STATUS_BLOCK DestinationIoStatus;
    ULONG           StreamInfoSize = 4096;
    BYTE           *Buffer = NULL;
    ULONG           BufferSize = 8192;

    FILE_BASIC_INFORMATION      BasicInfo;
    FILE_STANDARD_INFORMATION   StandardInfo;
    FILE_STREAM_INFORMATION    *StreamInfo = NULL;
    FILE_STREAM_INFORMATION    *Stream;

    //
    // Check the attributes on the source.  If it's set to 
    // read-only don't try to copy it
    //

    Status = NtUnRestrictedQueryAttributesFile(SourceAttributes, &BasicInfo);

    if (!NT_SUCCESS(Status))
        return Status;
    else if (BasicInfo.FileAttributes & FILE_ATTRIBUTE_READONLY)
        return STATUS_ACCESS_DENIED;


    //
    // Try to open the source file
    //

    Status = NtUnRestrictedOpenFile(
                    &SourceFile,
                    GENERIC_READ | SYNCHRONIZE,
                    SourceAttributes,
                    &SourceIoStatus,
                    FILE_SHARE_READ,
                    FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status))
        return Status;

    //
    // Get the size of the source.  If this fails don't worry about it.
    //

    Status = NtQueryInformationFile(
                    SourceFile,
                    &SourceIoStatus,
                    &StandardInfo,
                    sizeof(StandardInfo),
                    FileStandardInformation);

    if (!NT_SUCCESS(Status))
        StandardInfo.AllocationSize.QuadPart = 0;

    // 
    // Get the file stream information.  There's no way to tell how big a 
    // buffer we'll need to just keep doubling it.
    //

    do
    {
        StreamInfo = RtlAllocateHeap(RtlProcessHeap(), 0, StreamInfoSize);    

        if (NULL == StreamInfo)
        {
            Status = STATUS_NO_MEMORY;
            goto ErrorOut;
        }

        Status = NtQueryInformationFile(
                        SourceFile,
                        &SourceIoStatus,
                        StreamInfo,
                        StreamInfoSize,
                        FileStreamInformation);

        StreamInfoSize *= 2;
    }
    while (STATUS_BUFFER_OVERFLOW == Status 
               || STATUS_BUFFER_TOO_SMALL == Status);

    //
    // Allocate a buffer to do the copying
    //

    do
    {
        Buffer = RtlAllocateHeap(RtlProcessHeap(), 0, BufferSize);

        if (NULL == Buffer)
            BufferSize /= 2;
    }
    while (NULL == Buffer && BufferSize > 15);

    if (NULL == Buffer)
        goto ErrorOut;
            
    //
    // Try to create the destination file
    //

    Status = NtUnRestrictedCreateFile(
                    &DestinationFile,
                    GENERIC_WRITE | SYNCHRONIZE,
                    DestinationAttributes,
                    &DestinationIoStatus,
                    &StandardInfo.AllocationSize,
                    BasicInfo.FileAttributes,
                    0,
                    FILE_CREATE,
                    FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    //
    // If creating the file failed because the path doesn't exist,
    // create the path and try again
    //

    if (STATUS_OBJECT_PATH_NOT_FOUND == Status)
    {
        Status = CreateDirectories(DestinationAttributes);

        if (NT_SUCCESS(Status))
        {
            Status = NtUnRestrictedCreateFile(
                            &DestinationFile,
                            GENERIC_WRITE | SYNCHRONIZE,
                            DestinationAttributes,
                            &DestinationIoStatus,
                            &StandardInfo.AllocationSize,
                            BasicInfo.FileAttributes,
                            0,
                            FILE_CREATE,
                            FILE_SEQUENTIAL_ONLY |FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0);
        }
    }

    if (!NT_SUCCESS(Status))
        goto ErrorOut;

    //
    // Set the file times.  It's ok to fail
    //

    NtSetInformationFile(
            DestinationFile,
            &DestinationIoStatus,
            &BasicInfo,
            sizeof(BasicInfo),
            FileBasicInformation);

    //
    // Copy each stream in the file
    //

    Stream = StreamInfo;

    do
    {
        Status = CopyStream(
                        SourceFile, 
                        DestinationFile, 
                        Stream, 
                        Buffer, 
                        BufferSize);

        if (0 == Stream->NextEntryOffset)
            break;

        Stream = (FILE_STREAM_INFORMATION *) 
                        (((BYTE *) Stream) + Stream->NextEntryOffset);
    }
    while (NT_SUCCESS(Status));

ErrorOut:

    if (NULL != Buffer)
        RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
    if (NULL != StreamInfo)
        RtlFreeHeap(RtlProcessHeap(), 0, StreamInfo);
    if (NULL != SourceFile)
        NtClose(SourceFile);

    if (NULL != DestinationFile)
    {
        NtClose(DestinationFile);

        if (!NT_SUCCESS(Status))
        {
            NTSTATUS DeleteStatus;
            DeleteStatus = NtUnRestrictedDeleteFile(DestinationAttributes);
            ASSERT(NT_SUCCESS(DeleteStatus));
        }
    }

    return Status;
}








//
// The stuff below is copied from advapi.  It can be deleted once the path to
// the site directory gets put in the job object and the job object is made
// accessible
//




PSID
GetSiteSidFromToken(
                    IN HANDLE TokenHandle
                    )
{
    PTOKEN_GROUPS RestrictedSids = NULL;
    ULONG ReturnLength;
    NTSTATUS Status;
    PSID psSiteSid = NULL;


    Status = NtQueryInformationToken(
        TokenHandle,
        TokenRestrictedSids,
        NULL,
        0,
        &ReturnLength
        );
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        //BaseSetLastNTError(Status);
        return NULL;
    }

    RestrictedSids = (PTOKEN_GROUPS) RtlAllocateHeap(RtlProcessHeap(), 0, ReturnLength);
    if (RestrictedSids == NULL)
    {
//        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    Status = NtQueryInformationToken(
        TokenHandle,
        TokenRestrictedSids,
        RestrictedSids,
        ReturnLength,
        &ReturnLength
        );
    if (NT_SUCCESS(Status))
    {
        UINT i;
        SID_IDENTIFIER_AUTHORITY InternetSiteAuthority = SECURITY_INTERNETSITE_AUTHORITY;

        for (i = 0; i < RestrictedSids->GroupCount; i++) {

            if (RtlCompareMemory((PVOID) &((SID *) RestrictedSids->Groups[i].Sid)->IdentifierAuthority,
                (PVOID) &InternetSiteAuthority,
                sizeof(SID_IDENTIFIER_AUTHORITY)) == sizeof(SID_IDENTIFIER_AUTHORITY))
            {
                psSiteSid = RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthSid((RestrictedSids->Groups[i]).Sid));
                if (psSiteSid == NULL) {
//                    SetLastError(ERROR_OUTOFMEMORY);
                }
                else {
                    RtlCopySid(RtlLengthSid((RestrictedSids->Groups[i]).Sid), psSiteSid, (RestrictedSids->Groups[i]).Sid);
                }

                break;
            }

        }
    }
    else
    {
        //BaseSetLastNTError(Status);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, RestrictedSids);
    return psSiteSid;
}

HRESULT
GetMangledSiteSid(PSID pSid, ULONG cchMangledSite, LPWSTR *ppwszMangledSite)
{
    if (cchMangledSite < MAX_MANGLED_SITE)
    {
/*        *ppwszMangledSite = (WCHAR *) LocalAlloc(
                                            0,
                                            MAX_MANGLED_SITE * sizeof(WCHAR));*/
//        if (NULL == *ppwszMangledSite)
            return E_OUTOFMEMORY;
    }

    // The value of MAX_MANGLED_SITE assumes 4 dwords
    ASSERT(4 == *RtlSubAuthorityCountSid(pSid));

    Base32Encode(
            RtlSubAuthoritySid(pSid, 0),
            *RtlSubAuthorityCountSid(pSid) * sizeof(DWORD),
            *ppwszMangledSite);

    // The output string should always be MAX_MANGLED_SITE - 1 chars long
    ASSERT(MAX_MANGLED_SITE - 1 == wcslen(*ppwszMangledSite));

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   Base32Encode
//
//  Synopsis:   Convert the given data to base32
//
//  Notes:      Adapted from Mim64Encode in the mshtml project.
//
//              For 128 bit input (4 DWORDs) the output string will be
//              27 chars long (including the null terminator)
//
//-----------------------------------------------------------------------------

void Base32Encode(LPVOID pvData, UINT cbData, LPWSTR pchData)
{
    static const WCHAR alphabet[32] =
        { L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h',
          L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p',
          L'q', L'r', L's', L't', L'u', L'v', L'w', L'x',
          L'y', L'z', L'0', L'1', L'2', L'3', L'4', L'5' };

    int   shift = 0;    // The # of unprocessed bits in accum
    ULONG accum = 0;    // The unprocessed bits
    ULONG value;
    BYTE *pData = (BYTE *) pvData;

    // For each byte...

    while (cbData)
    {
        // Move the byte into the low bits of the accumulator

        accum = (accum << 8) | *pData++;
        shift += 8;
        --cbData;

        // Lop off the high 5 or 10 bits and write them out

        while ( shift >= 5 )
        {
            shift -= 5;
            value = (accum >> shift) & 0x1Fl;

            *pchData++ = alphabet[value];
        }
    }

    // If there are any remaining bits, push out one more char padded with 0's

    if (shift)
    {
        value = (accum << (5 - shift)) & 0x1Fl;

        *pchData++ = alphabet[value];
    }

    *pchData = L'\0';
}


BOOL
IsTokenRestricted(
    IN HANDLE TokenHandle
    )
{
    PTOKEN_GROUPS RestrictedSids = NULL;
    ULONG ReturnLength;
    NTSTATUS Status;
    BOOL Result = FALSE;


    Status = NtQueryInformationToken(
                TokenHandle,
                TokenRestrictedSids,
                NULL,
                0,
                &ReturnLength
                );
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
//        BaseSetLastNTError(Status);
        return(FALSE);
    }

    RestrictedSids = (PTOKEN_GROUPS) RtlAllocateHeap(RtlProcessHeap(), 0, ReturnLength);
    if (RestrictedSids == NULL)
    {
//        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                TokenHandle,
                TokenRestrictedSids,
                RestrictedSids,
                ReturnLength,
                &ReturnLength
                );
    if (NT_SUCCESS(Status))
    {
        if (RestrictedSids->GroupCount != 0)
        {
            Result = TRUE;
        }
    }
    else
    {
//        BaseSetLastNTError(Status);
    }
    RtlFreeHeap(RtlProcessHeap(), 0, RestrictedSids);
    return(Result);
}
