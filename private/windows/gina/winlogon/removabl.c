/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    removabl.c

Abstract:

    Removable Media Services Module - This module contains services
    related to the protection of removable media (e.g., floppies
    and CD Roms) related to logon and logoff.


Author:

    Jim Kelly (JimK) Oct-19-1994

Environment:

    Part of Winlogon.

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop





//////////////////////////////////  Section  ////////////////////////////////
//                                                                         //
// Constants used in this module only.                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#define  RMVP_DRIVE_CD_ROM                           (1)
#define  RMVP_DRIVE_FLOPPY                           (2)
#define  RMVP_DRIVE_DASD                             (3)

//
// Size of the buffer used to enumerate object directory
// contents
//

#define RMVP_OBJDIR_ENUM_BUFFER_SIZE              (1024)






//////////////////////////////////  Section  ////////////////////////////////
//                                                                         //
// Variables Global to this module.                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////



PSID
    RmvpLocalSystemSid,
    RmvpAdministratorsSid,
    RmvpPowerUsersSid ;

UNICODE_STRING
    RmvpNtfs,
    RmvpFat,
    RmvpCdfs ;

//
// flags indicating whether or not we are configured to allocate
// floppies and/or CDRoms.
//

BOOLEAN
    RmvpAllocateFloppies = FALSE,
    RmvpAllocateCDRoms = FALSE;

ULONG
    RmvpAllocateDisks = 0 ;

#define RMVP_ADMIN_ONLY_DASD    0   // Admins only can eject
#define RMVP_ADMIN_POWER_DASD   1   // Admins and power users
#define RMVP_ADMIN_USER_DASD    2   // Admins and the interactive user






//////////////////////////////////  Section  ////////////////////////////////
//                                                                         //
// Prototypes of modules private to this module                            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

VOID
RmvpAllocateDevice(
    IN     HANDLE                       DeviceHandle,
    IN     PSECURITY_DESCRIPTOR         SecurityDescriptor,
    IN     PUNICODE_STRING              FsName
    );

BOOLEAN
RmvpOpenDevice(
    IN  HANDLE                          Root,
    IN  POBJECT_DIRECTORY_INFORMATION   Object,
    IN  ULONG                           DeviceType,
    OUT PHANDLE                         Device
    );





//////////////////////////////////  Section  ////////////////////////////////
//                                                                         //
// Services Exported By This Module                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


BOOL
RmvInitializeRemovableMediaSrvcs(
    VOID
    )
/*++
Routine Description:

    Initialize RemovableMediaServices Module



Arguments:

    None.

Return Value:

    None.


--*/
{
    NTSTATUS
        Status;

    SID_IDENTIFIER_AUTHORITY
        NtAuthority = SECURITY_NT_AUTHORITY;


    //
    // See if we need to protect floppy and/or CDRom drives.
    //


    //
    // Initialize needed well-known sids
    //

    Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                          1,
                                          SECURITY_LOCAL_SYSTEM_RID,
                                          0, 0, 0, 0, 0, 0, 0,
                                          &RmvpLocalSystemSid
                                          );

    if ( !NT_SUCCESS(Status) )
    {
        return FALSE ;
    }

    Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                          2,
                                          SECURITY_BUILTIN_DOMAIN_RID,
                                          DOMAIN_ALIAS_RID_ADMINS,
                                          0, 0, 0, 0, 0, 0,
                                          &RmvpAdministratorsSid
                                          );

    if ( !NT_SUCCESS(Status) )
    {
        return FALSE ;
    }

    Status = RtlAllocateAndInitializeSid( &NtAuthority,
                                          2,
                                          SECURITY_BUILTIN_DOMAIN_RID,
                                          DOMAIN_ALIAS_RID_POWER_USERS,
                                          0, 0, 0, 0, 0, 0,
                                          &RmvpPowerUsersSid );

    if ( !NT_SUCCESS(Status) )
    {
        return FALSE ;
    }

    RtlInitUnicodeString( &RmvpNtfs, L"\\ntfs" );
    RtlInitUnicodeString( &RmvpFat, L"\\fat" );
    RtlInitUnicodeString( &RmvpCdfs, L"\\cdfs" );

    return TRUE ;
}

#if 0

VOID
RmvAllocateRemovableMedia(
    IN  PSID            AllocatorId
    )
/*++
Routine Description:

    This routine must be called during the logon process.
    Its purpose is to:

          See if floppy and / or CD Rom allocation is enabled.
          If so, then allocate the devices by:

            Restricting TRAVERSE access to the devices, and

            Forcing all handles to volumes open on those devices
            to be made invalid.

    Note that the work of finding and openning these devices must
    be done at logon time rather than system initialization time
    because of NT's ability to dynamically add devices while running.



Arguments:

    AllocatorId - Pointer to the SID to which the devices are
        to be allocated.

Return Value:

    None.


--*/
{
    NTSTATUS
        Status,
        IgnoreStatus;

    ACCESS_MASK
        Access;

    MYACE
        Ace[3];

    ACEINDEX
        AceCount = 0;

    PSECURITY_DESCRIPTOR
        SecurityDescriptor;

    ULONG
        DriveType,
        ReturnedLength,
        Context = 0;

    CHAR
        Buffer[RMVP_OBJDIR_ENUM_BUFFER_SIZE];

    HANDLE
        DeviceHandle,
        FatHandle  = NULL,
        CdfsHandle = NULL,
        DirectoryHandle;


    UNICODE_STRING
        DirectoryName,
        CdfsName,
        FatfsName;

    OBJECT_ATTRIBUTES
        Attributes;

    BOOLEAN
        Opened;

    POBJECT_DIRECTORY_INFORMATION
        DirInfo;


    RtlInitUnicodeString( &CdfsName,     L"\\cdfs" );
    RtlInitUnicodeString( &FatfsName,    L"\\fat" );




    //
    // If there is nothing to allocate, then return.
    //

    if (!RmvpAllocateFloppies  &&  !RmvpAllocateCDRoms) {
        return;
    }



    ///////////////////////////////////////////////////////////////////////
    //
    // Technical Note:
    //
    //      If I understand the way things work correctly, then there is
    //      a race condition that exists that we have to work around.
    //
    //      The problem is that until a volume is mounted in a file-system
    //      that file system might not be loaded (and a corresponding device
    //      object may not exist).  Therefore, we must not try to open a
    //      filesystem until we have opened a device object.  For example,
    //      don't open FAT until we have opened \device\floppy0 AND PROTECTED
    //      IT AGAINST FUTURE TRAVERSE ACCESS.
    //
    //      So, if you are looking at this routine and wondering why I don't
    //      open the FAT and CDFS file systems right away, it is in case they
    //      aren't loaded yet, but someone manages to squeek in a volume mount
    //      after I try to open the filesystems.
    //
    //      What I believe will work is to open any floppy or cdrom devices
    //      I find, then change who has traverse access to them, then open
    //      the corresponding filesystem object IF IT HASN'T ALREADY BEEN
    //      OPENED.  Once a filesystem object is opened, there is no need
    //      to close it and re-open it.
    //
    ///////////////////////////////////////////////////////////////////////




    //
    // Set up the protection to be applied to the removable device objects
    //
    //     System: FILE_READ_ATTRIBUTES | READ_CONTROL | WRITE_DAC | SYNCHRONIZE
    //     AllocationId: RWE
    //
    // We apply the restrictive System access to the object so that network shares
    // don't gain access (by nature of using a handle to the volume root
    // that LM server keeps).
    //

    Access = FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE;
    SetMyAce( &(Ace[AceCount]), AllocatorId,   Access, 0 );
    AceCount++;
    //Access &= ~FILE_TRAVERSE;
    Access = (FILE_READ_ATTRIBUTES | READ_CONTROL | WRITE_DAC | SYNCHRONIZE);
    SetMyAce( &(Ace[AceCount]), RmvpLocalSystemSid, Access,   0 );
    AceCount++;

#ifdef TRMV
    {
        //
        // This is needed for our test program (an interactive program
        // run in user mode) so that we don't set protection on the device
        // preventing us from changing the protection.
        //
        // THIS CODE IS NOT INCLUDED WHEN THIS MODULE IS PART OF WINLOGON.
        //

        ACCESS_MASK
            TestorAccess = (FILE_READ_ATTRIBUTES | READ_CONTROL | WRITE_DAC | SYNCHRONIZE);


        SetMyAce( &(Ace[AceCount]), RmvpAdministratorsSid, TestorAccess,   0 );
        AceCount++;
    }
#endif // TEST REMOVABLE


    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor != NULL) {


        //
        // We find floppies and or CD Roms by enumerating the objects in
        // the "\Device" object container and selecting the ones that
        // start with "floppy" followed by a number or "cdrom" followed
        // by a number.  Even after finding such an object, we want to be
        // sure it is a Device object and then open it and query it to
        // make sure it is really a floppy or cdrom.  The reason I say this
        // is because you will find names like "floppyControllerEvent" in
        // the \device container.
        //

        RtlInitUnicodeString( &DirectoryName, L"\\Device" );
        InitializeObjectAttributes( &Attributes,
                                    &DirectoryName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );


        Status = NtOpenDirectoryObject( &DirectoryHandle,
                                        STANDARD_RIGHTS_READ    |
                                            DIRECTORY_QUERY     |
                                            DIRECTORY_TRAVERSE,
                                        &Attributes );
#if DBG
        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "Failed to open \\Device object container.  Status: %#x\n", Status ));
        }
#endif //DBG
        ASSERT(NT_SUCCESS(Status));


        //
        //  Loop through the directory querying device objects
        //

        Status = NtQueryDirectoryObject( DirectoryHandle,
                                         Buffer,
                                         sizeof(Buffer),
                                         TRUE, // one entry at a time for now
                                         TRUE,
                                         &Context,
                                         &ReturnedLength );

#if DBG
        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_ENTRIES) {
                DebugLog((DEB_ERROR, "failed to query directory object, status: 0x%lx\n", Status));
            }
        }
#endif //DBG

        while ( NT_SUCCESS(Status) ) {

            //
            //  Point to the first record in the buffer, we are guaranteed to have
            //  one otherwise Status would have been NO_MORE_ENTRIES
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;
            while (DirInfo->Name.Length != 0) {

                //
                // For every record in the buffer compare it's name with the ones we're
                // looking for
                //

                if (RmvpAllocateFloppies) {
                    Opened = RmvpOpenDevice( DirectoryHandle,
                                             DirInfo,
                                             RMVP_DRIVE_FLOPPY,
                                             &DeviceHandle
                                             );
                    if (Opened) {
                        RmvpAllocateDevice( DeviceHandle, &DirInfo->Name, SecurityDescriptor, &FatfsName, &FatHandle );
                        NtClose( DeviceHandle );
                    }
                }

                if (RmvpAllocateCDRoms) {
                    Opened = RmvpOpenDevice( DirectoryHandle,
                                             DirInfo,
                                             RMVP_DRIVE_CD_ROM,
                                             &DeviceHandle
                                             );
                    if (Opened) {
                        RmvpAllocateDevice( DeviceHandle, &DirInfo->Name, SecurityDescriptor, &CdfsName, &CdfsHandle );
                        NtClose( DeviceHandle );
                    }
                }

                //
                //  Advance DirInfo to the next entry
                //

                DirInfo ++;

            } //end_while (more names in buffer)


            //
            // Get the next block of entries
            //

            Status = NtQueryDirectoryObject( DirectoryHandle,
                                             Buffer,
                                             sizeof(Buffer),
                                             // LATER FALSE,
                                             TRUE, // one entry at a time for now
                                             FALSE,
                                             &Context,
                                             &ReturnedLength );

        } //end_while (enumeration succeeded)

        //
        // We no longer need handles to FAT or CDFS (if they were opened).
        //

        if (FatHandle != NULL) {
            IgnoreStatus = NtClose( FatHandle );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }
        if (CdfsHandle != NULL) {
            IgnoreStatus = NtClose( CdfsHandle );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }






        //
        // Free up the security descriptor
        //

        DeleteSecurityDescriptor(SecurityDescriptor);

    } else {
        DebugLog((DEB_ERROR, "failed to create removable media security descriptor"));
    }


    return;
}
#endif



//////////////////////////////////  Section  ////////////////////////////////
//                                                                         //
// Services Private To This Module                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

VOID
RmvpAllocateDevice(
    IN     HANDLE                       DeviceHandle,
    IN     PSECURITY_DESCRIPTOR         SecurityDescriptor,
    IN     PUNICODE_STRING              FsName
    )

/*++

Routine Description:

    This routine attempts to allocate the device opened by DeviceHandle.

    Allocation entails changing the protection on the device (the new
    protection is passed in, but is expected to restrict TRAVERSE access),
    and notifying the filesystem to invalidate all handles to the
    volumes on the device.



Arguments:

    DeviceHandle - handle to the device being allocated.  This must be opened
        for WRITE_DAC access.

    DeviceName - The name of the device being allocated.

    SecurityDescriptor - New protection to assign to the device.  Only the
        Dacl of this security descriptor is used.

    FsName - The name of the file system that owns responsibility for the
        device.  This is expected to be either "\\fat" or
        "\\cdfs".

    FsHandle - Pointer to a handle to the filesystem.  If the handle
        value is NULL, then it is assumed that the filesystem has not yet
        been opened.  In that case, this routine will attempt to open the
        filesystem after assigning protection to the device.


Return Value:

    TRUE - indicates the device was successfully opened.

    FALSE - indicates the device was NOT successfully opened.


--*/
{

    NTSTATUS
        Status;

    OBJECT_ATTRIBUTES
        Attributes;

    IO_STATUS_BLOCK
        IoStatusBlock;

    BOOLEAN
        WasEnabled;

    HANDLE 
        FsHandle = NULL ;

    //
    // Change the protection on the device to restrict traverse access.
    //

    Status = NtSetSecurityObject( DeviceHandle, DACL_SECURITY_INFORMATION, SecurityDescriptor);
    ASSERT(NT_SUCCESS(Status));


    //
    // If the File system isn't yet open, then do so now.
    //


    //
    // Turn on the TCB privilege for our open to the filesystem to work.
    //

    Status = RtlAdjustPrivilege( SE_TCB_PRIVILEGE,
                                 TRUE,              // Enable
                                 FALSE,             // Client
                                 &WasEnabled
                                 );
    ASSERT(NT_SUCCESS(Status));

    if (NT_SUCCESS(Status)) {
        InitializeObjectAttributes( &Attributes,
                                    FsName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        Status = NtOpenFile( &FsHandle,
                             (SYNCHRONIZE | FILE_READ_ATTRIBUTES),
                             &Attributes,
                             &IoStatusBlock,
                             (FILE_SHARE_READ | FILE_SHARE_WRITE),
                             FILE_SYNCHRONOUS_IO_NONALERT
                           );

        if (!NT_SUCCESS(Status)) {

            //
            // If no volumes have ever been mounted in the file system,
            // then the file system may not be loaded.  So, this error
            // can not be considered serious.
            //

            FsHandle = NULL;    // In case garbage was put in our out parameter

            if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
                DebugLog((DEB_ERROR, "Failed to open <%wZ>.  Status: %#x\n", FsName, Status));
            }
        }


        //
        // Now restore the privilege (if necessary)
        //

        if (!WasEnabled) {
            Status = RtlAdjustPrivilege( SE_TCB_PRIVILEGE,
                                         FALSE,             // Disable
                                         FALSE,             // Client
                                         &WasEnabled
                                         );
            ASSERT(NT_SUCCESS(Status));
        }
    }
    

    if (FsHandle != NULL) {

        //
        // Notify the file system that it needs to invalidate the
        // passed in volume.
        //

        Status = NtFsControlFile( FsHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  FSCTL_INVALIDATE_VOLUMES,
                                  &DeviceHandle,       //DavidGoe is going to define a real structure to pass here
                                  sizeof(HANDLE),
                                  NULL,
                                  0 );
#if DBG
        if (!NT_SUCCESS(Status)) {
            DebugLog((DEB_ERROR, "Failed to remove handles from:\n"
                      "                   file system: <%wZ>\n"
                      "                        Status: 0x%lx\n\n",
                      FsName, Status));
            ASSERT(NT_SUCCESS(Status));
        }

#endif //DBG

        NtClose( FsHandle );

    }

    return;
}

#if 0

BOOLEAN
RmvpOpenDevice(
    IN  HANDLE                          Root,
    IN  POBJECT_DIRECTORY_INFORMATION   Object,
    IN  ULONG                           DeviceType,
    OUT PHANDLE                         Device
    )

/*++

Routine Description:

    This routine is called to open a handle to a device whose name starts
    with the string specified in the Prefix parameter.


Arguments:

    Root - handle to an object directory to be used as the root
        when openning the device object.  That is, the name specified
        in the Object parameter is relative to the object this handle
        refers to.

    Object - Contains the name of the object and the name of the type
        of the object.  If the name of the type is not "device" and the
        name of the object does not start with the string passed in
        the Prefix parameter, then the open is not attempted.

    DeviceType - specifies the type of device the object is expected to
        be.  Valid values are:

                    RMVP_DRIVE_CD_ROM
                    RMVP_DRIVE_FLOPPY
                    RMVP_DRIVE_DASD

    Device - Receives the handle to the device object (upon success).


Return Value:

    TRUE - indicates the device was successfully opened.

    FALSE - indicates the device was NOT successfully opened.


--*/
{
    NTSTATUS
        IgnoreStatus,
        Status;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    IO_STATUS_BLOCK
        IoStatusBlock;

    UNICODE_STRING
        Prefix,
        DeviceName;

    HANDLE
        FileHandle;

    FILE_FS_DEVICE_INFORMATION
        DeviceInfo;



    //RmvpPrint("Attempting to open Object<%wZ>, Type<%wZ>\n", &Object->Name, &Object->TypeName);


    RtlInitUnicodeString( &DeviceName, L"Device" );

    //
    // If the type is not "Device" then we aren't interested in this object
    //

    if (!RtlEqualUnicodeString(&Object->TypeName, &DeviceName, TRUE)) {
        return(FALSE);
    }


    //
    // If the object name doesn't have the right prefix, then we aren't
    // interested in this object
    //

    if (DeviceType == RMVP_DRIVE_CD_ROM) {

        RtlInitUnicodeString( &Prefix,  L"cdrom" );
        if (!RtlPrefixUnicodeString( &Prefix, &Object->Name, TRUE)) {
            return(FALSE);
        }
    } else {

        ASSERT( DeviceType == RMVP_DRIVE_FLOPPY);
        RtlInitUnicodeString( &Prefix, L"floppy" );
        if (!RtlPrefixUnicodeString( &Prefix, &Object->Name, TRUE)) {
            return(FALSE);
        }
    }



    //
    // Looks promising.  Open the Device object
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                &Object->Name,
                                OBJ_CASE_INSENSITIVE,
                                Root,
                                NULL
                              );

    Status = NtOpenFile( &FileHandle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES  |
                                      WRITE_DAC             |
                                      READ_CONTROL          |
                                      SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT
                       );
#if DBG
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Can't open for allocate: Object<%wZ> Type<%wZ>\n"
                  "          Status: 0x%lx\n", &Object->Name, &Object->TypeName, Status));
    }
#endif //DBG

    if (NT_SUCCESS( Status )) {

        Status = NtQueryVolumeInformationFile( FileHandle,
                                               &IoStatusBlock,
                                               &DeviceInfo,
                                               sizeof( DeviceInfo ),
                                               FileFsDeviceInformation
                                             );

        if (NT_SUCCESS( Status )) {

            //
            // See if its the right kind of beast and has the right name prefix
            //

            if (DeviceType == RMVP_DRIVE_CD_ROM) {
                if (DeviceInfo.DeviceType == FILE_DEVICE_CD_ROM) {
                    (*Device) = FileHandle;
                    DebugLog((DEB_TRACE, "Winlogon: Allocating CD Rom:\n"
                              "                        object: <%wZ>\n"
                              "                          type: <%wZ>\n",
                              &Object->Name, &Object->TypeName ));
                    return(TRUE);
                }

            } else if (DeviceType == RMVP_DRIVE_FLOPPY) {
                ASSERT( DeviceType == RMVP_DRIVE_FLOPPY);
                if ((DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE)) {
                    (*Device) = FileHandle;
                    DebugLog((DEB_TRACE, "Winlogon: Allocating floppy:\n"
                              "                        object: <%wZ>\n"
                              "                          type: <%wZ>\n",
                              &Object->Name, &Object->TypeName ));
                    return(TRUE);
                }
            } else {
                if ( DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA)
                {
                }
            }
        }

        IgnoreStatus = NtClose(FileHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return(FALSE);

}

#endif 

NTSTATUS 
RmvpOpenNtDeviceFromWin32Path(
    PWSTR Path,
    HANDLE * Device,
    PBOOLEAN IsFloppy,
    PUNICODE_STRING FileSystem
    )
{
    HANDLE hDevice ;
    HANDLE LinkHandle ;
    WCHAR ExtendedPath[ 16 ];
    UCHAR Buffer[ 32 + sizeof( FILE_FS_ATTRIBUTE_INFORMATION )];
    WCHAR Link[ MAX_PATH ];
    OBJECT_ATTRIBUTES ObjA ;
    UNICODE_STRING Name;
    NTSTATUS Status ;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo ;
    PFILE_FS_ATTRIBUTE_INFORMATION FsAttr ;
    PWSTR FsName ;

    if ( IsFloppy )
    {
        *IsFloppy = FALSE ;
    }

    ExtendedPath[0] = L'\\';
    ExtendedPath[1] = L'?';
    ExtendedPath[2] = L'?';
    ExtendedPath[3] = L'\\';
    ExtendedPath[4] = L'\0';

    wcsncat( ExtendedPath, Path, 16 );

    RtlInitUnicodeString( &Name, ExtendedPath );

    InitializeObjectAttributes( &ObjA, 
                                &Name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenSymbolicLinkObject(
                    &LinkHandle,
                    MAXIMUM_ALLOWED,
                    &ObjA );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    Name.Buffer = Link ;
    Name.Length = 0 ;
    Name.MaximumLength = MAX_PATH * sizeof( WCHAR );

    Status = NtQuerySymbolicLinkObject(
                LinkHandle,
                &Name,
                NULL );

    NtClose( LinkHandle );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    InitializeObjectAttributes( &ObjA,
                                &Name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenFile( &hDevice,
                             (ACCESS_MASK)FILE_READ_ATTRIBUTES  |
                                          WRITE_DAC             |
                                          READ_CONTROL          |
                                          SYNCHRONIZE,
                             &ObjA,
                             &IoStatusBlock,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_SYNCHRONOUS_IO_NONALERT
                           );

    if ( NT_SUCCESS( Status ) )
    {
        //
        // if this worked, make sure it is something we want:
        //

        Status = NtQueryVolumeInformationFile(
                        hDevice,
                        &IoStatusBlock,
                        &DeviceInfo,
                        sizeof( DeviceInfo ),
                        FileFsDeviceInformation );

        if ( NT_SUCCESS( Status ) )
        {
            if ( ( DeviceInfo.DeviceType == FILE_DEVICE_CD_ROM ) ||
                 ( DeviceInfo.DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM ) ||
                 ( DeviceInfo.DeviceType == FILE_DEVICE_DISK ) ||
                 ( DeviceInfo.DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ) )
            {
                //
                // One of the interesting types:
                //

                if ( ( DeviceInfo.DeviceType == FILE_DEVICE_DISK ) ||
                     ( DeviceInfo.DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ) )
                {
                    //
                    // make sure it's removable
                    //

                    if ( (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) == 0 )
                    {
                        Status = STATUS_INVALID_PARAMETER ;

                    }
                    else 
                    {
                        if ( ( DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE ) &&
                             ( IsFloppy != NULL ) )
                        {
                            *IsFloppy = TRUE ;
                        }

                    }
                }

                if (!NT_SUCCESS( Status ) )
                {
                    FsAttr = (PFILE_FS_ATTRIBUTE_INFORMATION) Buffer ;

                    Status = NtQueryVolumeInformationFile(
                                    hDevice,
                                    &IoStatusBlock,
                                    FsAttr,
                                    sizeof( Buffer ),
                                    FileFsAttributeInformation );

                    if ( NT_SUCCESS( Status ) )
                    {
                        if (FsAttr->FileSystemNameLength + 2 < FileSystem->MaximumLength )
                        {
                            FsName = FileSystem->Buffer ;
                            *FsName++ = L'\\';
                            RtlCopyMemory(
                                FsName,
                                FsAttr->FileSystemName,
                                FsAttr->FileSystemNameLength );
                            FileSystem->Length = (USHORT) FsAttr->FileSystemNameLength + 2;

                        }
                        else 
                        {
                            Status = STATUS_BUFFER_OVERFLOW ;
                        }
                    }

                }
            }
            else 
            {
                Status = STATUS_INVALID_PARAMETER ;
            }

            if ( NT_SUCCESS( Status ) )
            {
                *Device = hDevice ;
            }
            else 
            {
                NtClose( hDevice );
            }
        }
    }

    return Status ;

}


VOID
RmvpAllocateRemovableMedia(
    IN PSID AllocatorId,
    IN HANDLE DeviceHandle,
    IN PUNICODE_STRING FsName,
    IN BOOL AllowPowerUser,
    IN BOOL AllowAdmins,
    IN BOOL AllowAllocator
    )
/*++
Routine Description:

    This routine must be called during the logon process.
    Its purpose is to:

          See if floppy and / or CD Rom allocation is enabled.
          If so, then allocate the devices by:

            Restricting TRAVERSE access to the devices, and

            Forcing all handles to volumes open on those devices
            to be made invalid.

    Note that the work of finding and openning these devices must
    be done at logon time rather than system initialization time
    because of NT's ability to dynamically add devices while running.



Arguments:

    AllocatorId - Pointer to the SID to which the devices are
        to be allocated.

Return Value:

    None.


--*/
{
    NTSTATUS
        Status ;

    ACCESS_MASK
        Access;

    MYACE
        Ace[5];

    ACEINDEX
        AceCount = 0;

    PSECURITY_DESCRIPTOR
        SecurityDescriptor;

    ULONG
        DriveType,
        ReturnedLength,
        Context = 0;





    ///////////////////////////////////////////////////////////////////////
    //
    // Technical Note:
    //
    //      If I understand the way things work correctly, then there is
    //      a race condition that exists that we have to work around.
    //
    //      The problem is that until a volume is mounted in a file-system
    //      that file system might not be loaded (and a corresponding device
    //      object may not exist).  Therefore, we must not try to open a
    //      filesystem until we have opened a device object.  For example,
    //      don't open FAT until we have opened \device\floppy0 AND PROTECTED
    //      IT AGAINST FUTURE TRAVERSE ACCESS.
    //
    //      So, if you are looking at this routine and wondering why I don't
    //      open the FAT and CDFS file systems right away, it is in case they
    //      aren't loaded yet, but someone manages to squeek in a volume mount
    //      after I try to open the filesystems.
    //
    //      What I believe will work is to open any floppy or cdrom devices
    //      I find, then change who has traverse access to them, then open
    //      the corresponding filesystem object IF IT HASN'T ALREADY BEEN
    //      OPENED.  Once a filesystem object is opened, there is no need
    //      to close it and re-open it.
    //
    ///////////////////////////////////////////////////////////////////////




    //
    // Set up the protection to be applied to the removable device objects
    //
    //     System: FILE_READ_ATTRIBUTES | READ_CONTROL | WRITE_DAC | SYNCHRONIZE
    //     AllocationId: RWE
    //
    // We apply the restrictive System access to the object so that network shares
    // don't gain access (by nature of using a handle to the volume root
    // that LM server keeps).
    //

    if ( AllowAllocator )
    {
        Access = FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE;
        SetMyAce( &(Ace[AceCount]), AllocatorId,   Access, 0 );
        AceCount++;
    }
    //Access &= ~FILE_TRAVERSE;

    Access = (FILE_READ_ATTRIBUTES | READ_CONTROL | WRITE_DAC | SYNCHRONIZE);
    SetMyAce( &(Ace[AceCount]), RmvpLocalSystemSid, Access,   0 );
    AceCount++;

    if ( AllowAdmins )
    {
        Access = (FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE);
        SetMyAce( &Ace[AceCount], RmvpAdministratorsSid, Access, 0 );
        AceCount++ ;
    }

    if ( AllowPowerUser )
    {
        Access = (FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE);
        SetMyAce( &Ace[AceCount], RmvpPowerUsersSid, Access, 0 );
        AceCount++ ;

    }

#ifdef TRMV
    {
        //
        // This is needed for our test program (an interactive program
        // run in user mode) so that we don't set protection on the device
        // preventing us from changing the protection.
        //
        // THIS CODE IS NOT INCLUDED WHEN THIS MODULE IS PART OF WINLOGON.
        //

        ACCESS_MASK
            TestorAccess = (FILE_READ_ATTRIBUTES | READ_CONTROL | WRITE_DAC | SYNCHRONIZE);


        SetMyAce( &(Ace[AceCount]), RmvpAdministratorsSid, TestorAccess,   0 );
        AceCount++;
    }
#endif // TEST REMOVABLE


    // Check we didn't goof
    ASSERT((sizeof(Ace) / sizeof(MYACE)) >= AceCount);

    //
    // Create the security descriptor
    //

    SecurityDescriptor = CreateSecurityDescriptor(Ace, AceCount);
    if (SecurityDescriptor != NULL) {

        RmvpAllocateDevice(
                DeviceHandle,
                SecurityDescriptor,
                FsName );


        //
        // Free up the security descriptor
        //

        DeleteSecurityDescriptor(SecurityDescriptor);

    } else {
        DebugLog((DEB_ERROR, "failed to create removable media security descriptor"));
    }


    return;
}

NTSTATUS
RmvpDoCdRom(
    PSID Allocator,
    PWSTR Path
    )
{
    HANDLE hCdRom ;
    NTSTATUS Status ;
    WCHAR Buffer[ 32 ];
    UNICODE_STRING FsName ;

    if ( !RmvpAllocateCDRoms )
    {
        return STATUS_SUCCESS ;
    }

    FsName.Buffer = Buffer;
    FsName.Length = 0 ;
    FsName.MaximumLength = 32 * sizeof(WCHAR) ;
    Buffer[0] = L'\0';

    Status = RmvpOpenNtDeviceFromWin32Path(
                Path,
                &hCdRom,
                NULL,
                &FsName );

    
    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    DebugLog(( DEB_TRACE, "Adjusting ACL for device %ws (%ws)\n", Path, Buffer ));

    RmvpAllocateRemovableMedia(
        Allocator,
        hCdRom,
        &FsName,
        FALSE,
        FALSE,
        TRUE );

    NtClose( hCdRom );

    return STATUS_SUCCESS ;


}

NTSTATUS
RmvpDoRemovable(
    PSID Allocator,
    PWSTR Path
    )
{
    HANDLE hDisk ;
    NTSTATUS Status ;
    BOOLEAN IsFloppy ;
    WCHAR Buffer[ 32 ];
    UNICODE_STRING FsName ;

    FsName.Buffer = Buffer;
    FsName.Length = 0 ;
    FsName.MaximumLength = 32 * sizeof(WCHAR) ;
    FsName.Buffer[ 0 ] = L'\0';


    Status = RmvpOpenNtDeviceFromWin32Path(
                Path,
                &hDisk,
                &IsFloppy,
                &FsName );


    
    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    if ( IsFloppy )
    {
        if ( !RmvpAllocateFloppies )
        {
            NtClose( hDisk );
            return STATUS_SUCCESS ;
        }
    }
    else 
    {
        if ( RmvpAllocateDisks == 0 )
        {
            NtClose( hDisk );
            return STATUS_SUCCESS ;
        }
    }

    //
    // ok, we're supposed to do it:
    //


    RmvpAllocateRemovableMedia(
            Allocator,
            hDisk,
            &FsName,
            ( IsFloppy ? FALSE : (RmvpAllocateDisks >= RMVP_ADMIN_POWER_DASD)),
            ( IsFloppy ? FALSE : TRUE ),
            ( IsFloppy ? TRUE : (RmvpAllocateDisks >= RMVP_ADMIN_USER_DASD ) )
            );

    NtClose( hDisk );

    return STATUS_SUCCESS ;


}

VOID
RmvAllocateRemovableMedia(
    IN  PSID            AllocatorId
    )
{
    WCHAR Path[ 6 ];
    int DriveType ;


    //
    // don't allocate media if we aren't the console winlogon
    //

    if ( !g_Console )
    {
        return;
    }

    RmvpAllocateFloppies = GetProfileInt( TEXT("Winlogon"), ALLOCATE_FLOPPIES, 0) == 1 ;

    RmvpAllocateCDRoms = GetProfileInt( TEXT("Winlogon"), ALLOCATE_CDROMS, 0) == 1 ;

    RmvpAllocateDisks = GetProfileInt( TEXT("Winlogon"), ALLOCATE_DASD, 0);

    Path[0] = L'A';
    Path[1] = L':';
    Path[2] = L'\0';
    Path[3] = L'\0';

    for ( Path[0] = L'A' ; Path[0] < L'Z' ; Path[0]++ )
    {
        DriveType = GetDriveType( Path );

        switch ( DriveType )
        {
            default:
            case DRIVE_UNKNOWN:
            case DRIVE_NO_ROOT_DIR:
            case DRIVE_FIXED:
            case DRIVE_RAMDISK:
                break;

            case DRIVE_CDROM:
                RmvpDoCdRom( AllocatorId, Path );
                break;

            case DRIVE_REMOVABLE:
                RmvpDoRemovable( AllocatorId, Path );
                break;

        }
    }

}
