/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    uob.c

Abstract:

    Object Manager User Mode Test Program

Author:

    Steve Wood (stevewo) 03-Aug-1989

Environment:

    User Mode

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <string.h>

STRING  DirTypeName;
STRING  LinkTypeName;

VOID
TestParent( VOID );

VOID
TestChild( VOID );

VOID
DumpObjectDirs(
    IN PCH DirName,
    IN ULONG Level
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING Name;
    HANDLE Handle;
    ULONG Context, Length;
    NTSTATUS Status;
    BOOLEAN RestartScan;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    CHAR DirInfoBuffer[ 256 ];
    CHAR SubDirName[ 128 ];
    STRING LinkName;
    STRING LinkTarget;
    HANDLE LinkHandle;

    RtlInitString( &Name, DirName );
    InitializeObjectAttributes( &ObjectAttributes,
                                &Name,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    NtCreateDirectoryObject( &Handle,
                             DIRECTORY_ALL_ACCESS,
                             &ObjectAttributes
                           );

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)&DirInfoBuffer;
    RestartScan = TRUE;
    while (TRUE) {
        Status = NtQueryDirectoryObject( Handle,
                                         (PVOID)DirInfo,
                                         sizeof( DirInfoBuffer ),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &Length
                                       );
        if (!NT_SUCCESS( Status )) {
            break;
            }

        DbgPrint( "%s%s%Z - %Z",
                 DirName,
                 Level ? "\\" : "",
                 &DirInfo->Name,
                 &DirInfo->TypeName
                 );
        if (RtlEqualString( &DirInfo->TypeName, &DirTypeName, TRUE )) {
            DbgPrint( "\n" );
            strcpy( SubDirName, DirName );
            if (Level) {
                strcat( SubDirName, "\\" );
                }
            strcat( SubDirName, DirInfo->Name.Buffer );
            DumpObjectDirs( SubDirName, Level+1 );
            }
        else
        if (RtlEqualString( &DirInfo->TypeName, &LinkTypeName, TRUE )) {
            strcpy( SubDirName, DirName );
            if (Level) {
                strcat( SubDirName, "\\" );
                }
            strcat( SubDirName, DirInfo->Name.Buffer );
            RtlInitString( &LinkName, SubDirName );
            InitializeObjectAttributes( &ObjectAttributes,
                                        &LinkName,
                                        0,
                                        NULL,
                                        NULL
                                      );
            Status = NtOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_ALL_ACCESS,
                                               &ObjectAttributes
                                             );
            if (!NT_SUCCESS( Status )) {
                DbgPrint( " - unable to open symbolic link (%X)\n", Status  );
                }
            else {
                LinkTarget.MaximumLength = sizeof( SubDirName );
                LinkTarget.Length = 0;
                LinkTarget.Buffer = SubDirName;
                Status = NtQuerySymbolicLinkObject( LinkHandle,
                                                    &LinkTarget
                                                  );
                if (!NT_SUCCESS( Status )) {
                    DbgPrint( " - unable to query symbolic link target (%X)\n", Status );
                    }
                else {
                    DbgPrint( " => %Z\n", &LinkTarget );
                    }

                NtClose( LinkHandle );
                }
            }
        else {
            DbgPrint( "\n" );
            }

        RestartScan = FALSE;
        }

    NtClose( Handle );
}

char ParameterBuffer[ 4096 ];

main(
    int argc,
    char **argv,
    char **envp,
    int DebugFlag
    )
{
    NTSTATUS Status;
    STRING ImageName;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;

    if (argc == 1) {
        TestParent();

        Parameters[ RTL_USER_PROC_PARAMS_IMAGEFILE ] = argv[ 0 ];

        Parameters[ RTL_USER_PROC_PARAMS_CMDLINE ] = " CHILD";

        Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG ] =
             DebugFlag ? "1" : "0";

        Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG+1 ] = NULL;

        Arguments[ 0 ] = argv[ 0 ];
        Arguments[ 1 ] = "CHILD";
        Arguments[ 2 ] = NULL;

        ProcessParameters = (PRTL_USER_PROCESS_PARAMETERS)ParameterBuffer;
        ProcessParameters->Length = 0;
        ProcessParameters->MaximumLength = sizeof( ParameterBuffer );

        Status = RtlVectorsToProcessParameters( Arguments,
                                                envp,
                                                Parameters,
                                                ProcessParameters
                                              );
        if (!NT_SUCCESS( Status )) {
            DbgPrint( "RtlVectorToProcessParameters failed - Status = %X\n",
                      Status
                    );
            }
        else {
            RtlInitString( &ImageName, "\\C:\\TMP\\UOB.EXE" );
            Status = RtlCreateUserProcess( &ImageName,
                                           NULL,
                                           NULL,
                                           NULL,
                                           TRUE,
                                           NULL,
                                           NULL,
                                           ProcessParameters,
                                           &ProcessInformation,
                                           NULL
                                         );
            if (!NT_SUCCESS( Status )) {
                DbgPrint( "RtlCreateUserProcess( %Z ) failed - Status = %X\n",
                          &ImageName, Status
                        );
                }
            else {
                Status = NtResumeThread( ProcessInformation.Thread, NULL );
                Status = NtWaitForSingleObject( ProcessInformation.Process,
                                                FALSE,
                                                (PLARGE_INTEGER)NULL
                                              );
                if (!NT_SUCCESS( Status )) {
                    DbgPrint( "NtWaitForSingleObject failed - Status = %X\n",
                              Status
                            );
                    }
                }
            }
        }
    else {
        TestChild();
        }

    NtTerminateProcess( NtCurrentProcess(), Status );
}


VOID
TestParent( VOID )
{
    NTSTATUS Status;
    STRING DirectoryName;
    STRING LinkName;
    STRING LinkTarget;
    STRING SectionName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle, LinkHandle, SectionHandle;
    ULONG ReturnedLength;
    CHAR ObjectInfoBuffer[ 512 ];
    OBJECT_BASIC_INFORMATION ObjectBasicInfo;
    POBJECT_NAME_INFORMATION ObjectNameInfo;
    POBJECT_TYPE_INFORMATION ObjectTypeInfo;
    LARGE_INTEGER SectionSize;

    Status = STATUS_SUCCESS;

    DbgPrint( "Entering Object Manager User Mode Test Program\n" );

    RtlInitString( &SectionName, "\\A:\\OSO001.MSG" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &SectionName,
                                OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    SectionSize.LowPart = 0x1000;
    SectiinSize.HighPart = 0;
    Status = NtCreateSection( &SectionHandle,
                              GENERIC_READ,
                              &ObjectAttributes,
                              &SectionSize,
                              PAGE_READONLY,
                              SEC_RESERVE,
                              NULL
                            );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z section object (%X) [OK]\n", &SectionName, Status );
        }

    RtlInitString( &DirectoryName, "\\Drives" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                (PSECURITY_DESCRIPTOR)1

                              );
    ObjectAttributes.Length = 0;
    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      -1,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    RtlInitString( &DirectoryName, "\\Drives" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                (PSECURITY_DESCRIPTOR)1

                              );
    ObjectAttributes.Length = 0;
    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                -1,
                                NULL,
                                (PSECURITY_DESCRIPTOR)1

                              );
    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                (PSECURITY_DESCRIPTOR)1

                              );
    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                NULL

                              );
    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z directory object (%X)\n",
                 &DirectoryName, Status );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    Status = NtClose( DirectoryHandle );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to close %Z directory object handle - %lx (%X)\n",
                 &DirectoryName,
                 DirectoryHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X)\n",
                 &DirectoryName, Status );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    Status = NtQueryObject( DirectoryHandle,
                            ObjectBasicInformation,
                            &ObjectBasicInfo,
                            sizeof( ObjectBasicInfo ),
                            &ReturnedLength
                          );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "NtQueryObject( %lx, ObjectBasicInfo ) failed - Status == %X\n",
                 DirectoryHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }
    DbgPrint( "NtQueryObject( %lx, ObjectBasicInfo ) returned %lx bytes\n",
             DirectoryHandle,
             ReturnedLength
             );
    DbgPrint( "    Attributes = %lx\n",          ObjectBasicInfo.Attributes );
    DbgPrint( "    GrantedAccess = %lx\n",       ObjectBasicInfo.GrantedAccess );
    DbgPrint( "    HandleCount = %lx\n",         ObjectBasicInfo.HandleCount );
    DbgPrint( "    PointerCount = %lx\n",        ObjectBasicInfo.PointerCount );
    DbgPrint( "    PagedPoolCharge = %lx\n",     ObjectBasicInfo.PagedPoolCharge );
    DbgPrint( "    NonPagedPoolCharge = %lx\n",  ObjectBasicInfo.NonPagedPoolCharge );
    DbgPrint( "    NameInfoSize = %lx\n",        ObjectBasicInfo.NameInfoSize );
    DbgPrint( "    TypeInfoSize = %lx\n",        ObjectBasicInfo.TypeInfoSize );
    DbgPrint( "    SecurityDescriptorSize = %lx\n", ObjectBasicInfo.SecurityDescriptorSize );

    ObjectNameInfo = (POBJECT_NAME_INFORMATION)ObjectInfoBuffer;
    Status = NtQueryObject( DirectoryHandle,
                            ObjectNameInformation,
                            ObjectNameInfo,
                            sizeof( ObjectInfoBuffer ),
                            &ReturnedLength
                          );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "NtQueryObject( %lx, ObjectNameInfo ) failed - Status == %X\n",
                 DirectoryHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }
    DbgPrint( "NtQueryObject( %lx, ObjectNameInfo ) returned %lx bytes\n",
             DirectoryHandle,
             ReturnedLength
             );
    DbgPrint( "    Name = (%ld,%ld) '%Z'\n",
             ObjectNameInfo->Name.MaximumLength,
             ObjectNameInfo->Name.Length,
             &ObjectNameInfo->Name
           );


    ObjectTypeInfo = (POBJECT_TYPE_INFORMATION)ObjectInfoBuffer;
    Status = NtQueryObject( DirectoryHandle,
                            ObjectTypeInformation,
                            ObjectTypeInfo,
                            sizeof( ObjectInfoBuffer ),
                            &ReturnedLength
                          );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "NtQueryObject( %lx, ObjectTypeInfo ) failed - Status == %X\n",
                 DirectoryHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }
    DbgPrint( "NtQueryObject( %lx, ObjectTypeInfo ) returned %lx bytes\n",
             DirectoryHandle,
             ReturnedLength
             );
    DbgPrint( "    TypeName = (%ld,%ld) '%Z'\n",
             ObjectTypeInfo->TypeName.MaximumLength,
             ObjectTypeInfo->TypeName.Length,
             &ObjectTypeInfo->TypeName
           );

    RtlInitString( &LinkName, "TestSymbolicLink" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    ObjectAttributes.RootDirectory = DirectoryHandle;
    RtlInitString( &LinkTarget, "\\Device\\FileSystem" );
    Status = NtCreateSymbolicLinkObject( &LinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &LinkTarget
                                       );

    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z => %Z symbolic link object (%X)\n",
                 &LinkName, &LinkTarget, Status );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    Status = NtClose( DirectoryHandle );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to close %Z directory object handle - %lx (%X)\n",
                 &DirectoryName,
                 DirectoryHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    RtlInitString( &DirTypeName, "Directory" );
    RtlInitString( &LinkTypeName, "SymbolicLink" );
    DumpObjectDirs( "\\", 0 );

    RtlInitString( &LinkName, "TestSymbolicLink" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &LinkName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    ObjectAttributes.RootDirectory = LinkHandle;
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X) [OK]\n", &DirectoryName, Status );
        }

    Status = NtClose( LinkHandle );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to close %Z symbolic link handle - %lx (%X)\n",
                 &LinkName,
                 LinkHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X)\n", &DirectoryName, Status );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    Status = NtMakeTemporaryObject( DirectoryHandle );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "NtMakeTemporaryObject( %lx ) failed - Status == %X\n",
                 DirectoryHandle,
                 Status
               );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    Status = NtClose( DirectoryHandle );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to close %Z directory object handle - %lx (%X)\n",
                 &DirectoryName,
                 DirectoryHandle,
                 Status
                 );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X) [OK]\n", &DirectoryName, Status );
        }

    RtlInitString( &DirectoryName, "\\ExclusiveDir" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_EXCLUSIVE,
                                NULL,
                                NULL

                              );
    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes
                                    );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to create %Z directory object (%X)\n",
                 &DirectoryName, Status );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_EXCLUSIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X)\n",
                 &DirectoryName, Status );
        NtTerminateProcess( NtCurrentProcess(), Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    DbgPrint( "Exiting Object Manager User Mode Test Program with Status = %X\n", Status );
}


VOID
TestChild( VOID )
{
    NTSTATUS Status;
    STRING DirectoryName;
    HANDLE DirectoryHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;

    Status = STATUS_SUCCESS;

    DbgPrint( "Entering Object Manager User Mode Child Test Program\n" );

    RtlInitString( &DirectoryName, "\\ExclusiveDir" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE | OBJ_EXCLUSIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenDirectoryObject( &DirectoryHandle,
                                    DIRECTORY_ALL_ACCESS,
                                    &ObjectAttributes
                                  );
    if (!NT_SUCCESS( Status )) {
        DbgPrint( "Unable to open %Z directory object (%X) [OK]\n",
                 &DirectoryName, Status );
        }

    DbgPrint( "Exiting Object Manager User Mode Child Test Program with Status = %X\n", Status );
}
