/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Apitest.c

Abstract:

    This module contains the function test for the Win32 Registry API.

Author:

    David J. Gilman (davegi) 28-Dec-1991

Environment:

    Windows, Crt - User Mode

Notes:

    This test can be compiled for Unicode by defining the compiler symbol
    UNICODE.

    Since this is a test program it relies on assertions for error checking
    rather than a more robust mechanism.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "crtools.h"



#define HKEY_ROOT                       HKEY_CURRENT_USER
#define SAVE_RESTORE_FILE               TEXT( "srkey.reg" )
#define KEY_PATH                        \
        TEXT( "TestUser1\\TestUser1_1\\TestUser1_2" )


#define PREDEFINED_HANDLE               HKEY_USERS
#define PREDEFINED_HANDLE_STRING        \
        TEXT( "HKEY_USERS\\.Default\\TestUser1" )


#define KEY_NAME_1                      TEXT( "TestUser1" )
#define KEY_NAME_1_TITLE_INDEX          ( 0 )
#define KEY_NAME_1_CLASS                TEXT( "Test User Class" )
#define KEY_NAME_1_CLASS_LENGTH         LENGTH( KEY_NAME_1_CLASS )

#define KEY_NAME_1_1                    TEXT( "TestUser1_1" )
#define KEY_NAME_1_1_LENGTH             LENGTH( KEY_NAME_1_1 )
#define KEY_NAME_1_1_TITLE_INDEX        ( 0 )
#define KEY_NAME_1_1_CLASS              TEXT( "Test User Class" )
#define KEY_NAME_1_1_CLASS_LENGTH       LENGTH( KEY_NAME_1_1_CLASS )

#define KEY_NAME_1_2                    TEXT( "TestUser1_2" )
#define KEY_NAME_1_2_LENGTH             LENGTH( KEY_NAME_1_2 )
#define KEY_NAME_1_2_TITLE_INDEX        (0 )
#define KEY_NAME_1_2_CLASS              TEXT( "Test User Class" )
#define KEY_NAME_1_2_CLASS_LENGTH       LENGTH( KEY_NAME_1_2_CLASS )

#define VALUE_NAME_1                    TEXT( "One" )
#define VALUE_NAME_1_LENGTH             LENGTH( VALUE_NAME_1 )
#define VALUE_NAME_1_TITLE_INDEX        0
#define VALUE_DATA_1                    "Number One"
#define VALUE_DATA_1_LENGTH             11
#define VALUE_DATA_1_TYPE               REG_SZ

#define VALUE_NAME_2                    TEXT( "Second" )
#define VALUE_NAME_2_LENGTH             LENGTH( VALUE_NAME_2 )
#define VALUE_NAME_2_TITLE_INDEX        ( 0 )
#define VALUE_DATA_2                    ( 2 )
#define VALUE_DATA_2_LENGTH             ( sizeof( VALUE_DATA_2 ))
#define VALUE_DATA_2_TYPE               REG_DWORD

#define MAX_DATA_LENGTH                 ( 32 )

//
// Root handle for apitest's nodes.
//

HKEY    RootHandle;

//
// Error and informational messages.
//

PSTR    UsageMessage =

    "Usage: apitest [-?] [-q] [\\machinename]\n";

PSTR    HelpMessage =

    "\n  where:\n"                                                          \
      "    -?           - display this message.\n"                          \
      "    -q           - quiet - suppresses all output\n"                  \
      "    machinename  - remote machine.\n";

PSTR    InvalidSwitchMessage =

    "Invalid switch - %s\n";

PSTR    InvalidMachineNameMessage =

    "Invalid machine name - %s\n";

//
// Event handle used for synchronization.
//

HANDLE  _EventHandle;
HANDLE  _EventHandle1;
HANDLE  _EventHandle2;

BOOL    Quiet;



VOID
DeleteTree(
    IN HKEY KeyHandle
    )

{
    LONG        Error;
    DWORD       Index;
    HKEY        ChildHandle;


    TSTR        KeyName[ MAX_PATH ];
    DWORD       KeyNameLength;
    TSTR        ClassName[ MAX_PATH ];
    DWORD       ClassNameLength;
    DWORD       TitleIndex;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;

    ClassNameLength = MAX_PATH;

    Error = RegQueryInfoKey(
                KeyHandle,
                ClassName,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );
    REG_API_SUCCESS( RegQueryInfoKey );

    for( Index = 0; Index < NumberOfSubKeys; Index++ ) {

        KeyNameLength = MAX_PATH;

        Error = RegEnumKey(
                    KeyHandle,
                    0,
                    // Index,
                    KeyName,
                    KeyNameLength
                    );
        REG_API_SUCCESS( RegEnumKey );

        Error = RegOpenKey(
                    KeyHandle,
                    KeyName,
                    &ChildHandle
                    );

        REG_API_SUCCESS( RegOpenKey );

        DeleteTree( ChildHandle );

        Error = RegCloseKey(
                    ChildHandle
                    );
        REG_API_SUCCESS( RegCloseKey );

        Error = RegDeleteKey(
                    KeyHandle,
                    KeyName
                    );
        REG_API_SUCCESS( RegDeleteKey );
    }
}
VOID
DeleteTestTree(
    )

{
    LONG    Error;
    HKEY    KeyHandle;

    Error = RegOpenKey(
                RootHandle,
                KEY_NAME_1,
                &KeyHandle
                );

    if( Error == ERROR_SUCCESS ) {

        DeleteTree( KeyHandle );

        Error = RegCloseKey(
                    KeyHandle
                    );
        REG_API_SUCCESS( RegCloseKey );

        Error = RegDeleteKey(
                    RootHandle,
                    KEY_NAME_1
                    );
        REG_API_SUCCESS( RegDeleteKey );
    }
}

DWORD
NotifyThread(
    LPVOID  Parameters
    )

{
    LONG        Error;
    BOOL        ErrorFlag;
    HANDLE      EventHandle;

    UNREFERENCED_PARAMETER( Parameters );

    //
    // Create the notification event.
    //

    EventHandle = CreateEvent(
                    NULL,
                    FALSE,
                    FALSE,
                    NULL
                    );
    ASSERT( EventHandle != NULL );

    //
    // Set-up an asynchronous notify.
    //

    Error = RegNotifyChangeKeyValue(
                RootHandle,
                FALSE,
                REG_LEGAL_CHANGE_FILTER,
                EventHandle,
                TRUE
                );
    REG_API_SUCCESS( RegNotifyChangeKeyValue );

    //
    // Release the main thread.
    //

    ErrorFlag = SetEvent( _EventHandle );
    ASSERT( ErrorFlag == TRUE );

    //
    // Wait for a notification.
    //

    Error = (LONG)WaitForSingleObject( EventHandle, (DWORD)-1 );
    ASSERT( Error == 0 );

    if( ! Quiet ) {
        printf( "First notification triggered\n" );
    }

    CloseHandle( EventHandle );
#if 0
    //
    //  BUGBUG ramonsa - there is a race condition here. Until I fix
    //  this I'll use an asynchronous notification instead of
    //  a synchronous one.
    //
    ErrorFlag = SetEvent( _EventHandle1 );
    ASSERT( ErrorFlag == TRUE );

    //
    // Wait for a notification.
    //
    Error = RegNotifyChangeKeyValue(
                RootHandle,
                TRUE,
                REG_NOTIFY_CHANGE_SECURITY,
                NULL,
                FALSE
                );
    REG_API_SUCCESS( RegNotifyChangeKeyValue );

    if( ! Quiet ) {
        printf( "Second notification triggered\n" );
    }
    ErrorFlag = SetEvent( _EventHandle2 );
    ASSERT( ErrorFlag == TRUE );
#else

    EventHandle = CreateEvent(
                    NULL,
                    FALSE,
                    FALSE,
                    NULL
                    );
    ASSERT( EventHandle != NULL );

    //
    // Set-up an asynchronous notify.
    //

    Error = RegNotifyChangeKeyValue(
                RootHandle,
                FALSE,
                REG_LEGAL_CHANGE_FILTER,
                EventHandle,
                TRUE
                );
    REG_API_SUCCESS( RegNotifyChangeKeyValue );

    //
    // Release the main thread.
    //

    ErrorFlag = SetEvent( _EventHandle1 );
    ASSERT( ErrorFlag == TRUE );

    //
    // Wait for a notification.
    //

    Error = (LONG)WaitForSingleObject( EventHandle, (DWORD)-1 );
    ASSERT( Error == 0 );

    if( ! Quiet ) {
        printf( "Second notification triggered\n" );
    }

    CloseHandle( EventHandle );
    ErrorFlag = SetEvent( _EventHandle2 );
    ASSERT( ErrorFlag == TRUE );




#endif



    return ( DWORD ) TRUE;
}

VOID
main(
    INT     argc,
    PCHAR   argv[ ]
    )

{
    LONG                    Error;
    BOOL                    ErrorFlag;
    DWORD                   Index;

    PTSTR                   MachineName;

    PKEY                    Key;
    TSTR                    NameString[ MAX_PATH ];

    HANDLE                  NotifyThreadHandle;
    DWORD                   ThreadID;

    HKEY                    PredefinedHandle;
    HKEY                    Handle1;
    HKEY                    Handle1_1;
    HKEY                    Handle1_2;

    PSECURITY_DESCRIPTOR    SecurityDescriptor;
    SECURITY_ATTRIBUTES     SecurityAttributes;

    DWORD                   Disposition;
    TSTR                    KeyName[ MAX_PATH ];
    DWORD                   KeyNameLength;
    TSTR                    ClassName[ MAX_PATH ];
    DWORD                   ClassNameLength;
    DWORD                   NumberOfSubKeys;
    DWORD                   MaxSubKeyLength;
    DWORD                   MaxClassLength;
    DWORD                   NumberOfValues;
    DWORD                   MaxValueNameLength;
    DWORD                   MaxValueDataLength;
    DWORD                   SecurityDescriptorLength;
    FILETIME                LastWriteTime;


    TSTR                    ValueName[ MAX_PATH ];
    DWORD                   ValueNameLength;

    BYTE                    Data[ MAX_DATA_LENGTH ];
    DWORD                   DataLength;

    BYTE                    Data_1[ ]   = VALUE_DATA_1;
    DWORD                   Data_2      = VALUE_DATA_2;

    DWORD                   TitleIndex;
    DWORD                   Type;


    UNREFERENCED_PARAMETER( argc );

    //
    // By default, be verbose and operate on the local machine.
    //

    Quiet       = FALSE;
    MachineName = NULL;

    //
    // Initialize options based on the command line.
    //

    while( *++argv ) {

        //
        // If the command line argument is a switch character...
        //

        if( isswitch(( *argv )[ 0 ] )) {

            switch( tolower(( *argv )[ 1 ] )) {

            //
            // Display the detailed help message and quit.
            //

            case '?':

                DisplayMessage( FALSE, UsageMessage );
                DisplayMessage( TRUE, HelpMessage );
                break;

            //
            // Quiet - no output.
            //

            case 'q':

                Quiet = TRUE;
                break;

            //
            // Display invalid switch message and quit.
            //

            default:

                DisplayMessage( FALSE, InvalidSwitchMessage, *argv );
                DisplayMessage( TRUE, UsageMessage );
            }
        } else {

            MachineName = *argv;
        }
    }

    //
    // If a machine name was passed on the command line, connect to
    // the Registry on that machine else use the local Registry.
    // In either case construct a string representation of the
    // test's main key (i.e. \\machine\HKEY_USERS\.Default\TestUser1 or
    // HKEY_USERS\.Default\TestUser1.
    //

    if( MachineName ) {

        Error = RegConnectRegistry(
                    MachineName,
                    PREDEFINED_HANDLE,
                    &PredefinedHandle
                    );

        REG_API_SUCCESS( RegConnectRegistry );

        strcpy( NameString, MachineName );
        strcat( NameString, "\\\\" );
        strcat( NameString, PREDEFINED_HANDLE_STRING );

    } else {

        PredefinedHandle = PREDEFINED_HANDLE;
        strcpy( NameString, PREDEFINED_HANDLE_STRING );
    }

    //
    // Open ".Default" key as the root for the remainder of the test.
    //

    Error = RegOpenKeyEx(
                PredefinedHandle,
                ".Default",
                REG_OPTION_RESERVED,
                MAXIMUM_ALLOWED,
                &RootHandle
                );
    REG_API_SUCCESS( RegOpenKeyEx );

    //
    // Predefined handle is no longer needed.
    //

    Error = RegCloseKey(
                PredefinedHandle
                );
    REG_API_SUCCESS( RegCloseKey );

    //
    // Delete the save / restore file (in case it exists from a previous
    // run of the test) as RegSaveKey requires a new file.
    //

    DeleteFile( SAVE_RESTORE_FILE );

    //
    // Remove any leftover keys from previous runs of this test.
    //

    DeleteTestTree( );

    //
    // Use the Win 3.1 API (which calls the Win32 API) to create a path.
    //

    Error = RegCreateKey(
                RootHandle,
                KEY_PATH,
                &Handle1
                );
    REG_API_SUCCESS( RegCreateKey );

    //
    // Close the key so the delete (DeleteTestTree) will work.
    //

    Error = RegCloseKey(
                Handle1
                );
    REG_API_SUCCESS( RegCloseKey );

    //
    // Remove the path.
    //

    DeleteTestTree( );

    //
    // Create the synchronization event.
    //

    _EventHandle = CreateEvent(
                    NULL,
                    FALSE,
                    FALSE,
                    NULL
                    );
    ASSERT( _EventHandle != NULL );

    _EventHandle1 = CreateEvent(
                    NULL,
                    FALSE,
                    FALSE,
                    NULL
                    );
    ASSERT( _EventHandle1 != NULL );

    _EventHandle2 = CreateEvent(
                    NULL,
                    FALSE,
                    FALSE,
                    NULL
                    );
    ASSERT( _EventHandle2 != NULL );

    //
    // Create the notify thread.
    //

    NotifyThreadHandle = CreateThread(
                            NULL,
                            0,
                            NotifyThread,
                            NULL,
                            0,
                            &ThreadID
                            );
    ASSERT( NotifyThreadHandle != NULL );

    //
    // Wait for the notify thread to create its event.
    //

    Error = (LONG)WaitForSingleObject( _EventHandle, (DWORD)-1 );
    ASSERT( Error == 0 );

    //
    // Use Win 3.1 compatible APIs to create/close, open/close and delete
    // the key TestUser1.
    //

    Error = RegCreateKey(
                RootHandle,
                KEY_NAME_1,
                &Handle1
                );
    REG_API_SUCCESS( RegCreateKey );

    Error = RegCloseKey(
                Handle1
                );
    REG_API_SUCCESS( RegCloseKey );

    //
    // Wait for the notify thread to create its event.
    //

    Error = (LONG)WaitForSingleObject( _EventHandle1, (DWORD)-1 );
    ASSERT( Error == 0 );

    Error = RegOpenKey(
                RootHandle,
                KEY_NAME_1,
                &Handle1
                );
    REG_API_SUCCESS( RegOpenKey );

    Error = RegCloseKey(
                Handle1
                );
    REG_API_SUCCESS( RegCloseKey );

    Error = RegDeleteKey(
                RootHandle,
                KEY_NAME_1
                );
    REG_API_SUCCESS( RegDeleteKey );

    //
    // Use Win32 APIs to create/close, open/close and create (open) the
    // key TestUser1.
    //

    //
    // Allocate and initialize the SecurityDescriptor.
    //

    SecurityDescriptor = malloc( sizeof( SECURITY_DESCRIPTOR ));
    ASSERT( SecurityDescriptor != NULL );
    ErrorFlag = InitializeSecurityDescriptor(
                    SecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );
    ASSERT( ErrorFlag == TRUE );

    SecurityAttributes.nLength              = sizeof( SECURITY_ATTRIBUTES );
    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttributes.bInheritHandle       = FALSE;

    Error = RegCreateKeyEx(
                RootHandle,
                KEY_NAME_1,
                0,
                KEY_NAME_1_CLASS,
                REG_OPTION_RESERVED,
                KEY_ALL_ACCESS,
                &SecurityAttributes,
                &Handle1,
                &Disposition
                );
    REG_API_SUCCESS( RegCreateKeyEx );

    ASSERT( Disposition == REG_CREATED_NEW_KEY );

    Error = RegCloseKey(
                Handle1
                );
    REG_API_SUCCESS( RegCloseKey );


    //
    // Wait for the notify thread to create its event.
    //

    Error = RegOpenKeyEx(
                RootHandle,
                KEY_NAME_1,
                REG_OPTION_RESERVED,
                KEY_ALL_ACCESS,
                &Handle1
                );
    REG_API_SUCCESS( RegOpenKeyEx );

    Error = RegCloseKey(
                Handle1
                );
    REG_API_SUCCESS( RegCloseKey );

    Error = RegCreateKeyEx(
                RootHandle,
                KEY_NAME_1,
                0,
                KEY_NAME_1_CLASS,
                REG_OPTION_RESERVED,
                KEY_ALL_ACCESS,
                NULL,
                &Handle1,
                &Disposition
                );
    REG_API_SUCCESS( RegCreateKeyEx );

    ASSERT( Disposition == REG_OPENED_EXISTING_KEY );

    //
    // Get and set the key's SECURITY_DESCRIPTOR. Setting will trigger
    // a notification.
    //

    SecurityDescriptorLength = 0;

    //
    // Get the SECURITY_DESCRIPTOR's length.
    //

    Error = RegGetKeySecurity(
                Handle1,
                OWNER_SECURITY_INFORMATION
                | GROUP_SECURITY_INFORMATION
                | DACL_SECURITY_INFORMATION,
                SecurityDescriptor,
                &SecurityDescriptorLength
                );
    ASSERT( Error == ERROR_INSUFFICIENT_BUFFER );

    SecurityDescriptor = realloc(
                            SecurityDescriptor,
                            SecurityDescriptorLength
                            );
    ASSERT( SecurityDescriptor != NULL );
    ErrorFlag = InitializeSecurityDescriptor(
                    SecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );
    ASSERT( ErrorFlag == TRUE );

    Error = RegSetKeySecurity(
                Handle1,
                OWNER_SECURITY_INFORMATION
                | GROUP_SECURITY_INFORMATION
                | DACL_SECURITY_INFORMATION,
                SecurityDescriptor
                );
    REG_API_SUCCESS( RegSetKeySecurity );

    Error = (LONG)WaitForSingleObject( _EventHandle2, (DWORD)-1 );
    ASSERT( Error == 0 );

    //
    // Reinitialize after the realloc.
    //

    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;

    //
    // Create two sub-keys.
    //

    Error = RegCreateKeyEx(
                Handle1,
                KEY_NAME_1_1,
                0,
                KEY_NAME_1_1_CLASS,
                REG_OPTION_RESERVED,
                KEY_ALL_ACCESS,
                &SecurityAttributes,
                &Handle1_1,
                &Disposition
                );
    REG_API_SUCCESS( RegCreateKeyEx );

    ASSERT( Disposition == REG_CREATED_NEW_KEY );

    Error = RegCreateKeyEx(
                Handle1,
                KEY_NAME_1_2,
                0,
                KEY_NAME_1_2_CLASS,
                0,
                KEY_ALL_ACCESS,
                &SecurityAttributes,
                &Handle1_2,
                &Disposition
                );
    REG_API_SUCCESS( RegCreateKeyEx );

    ASSERT( Disposition == REG_CREATED_NEW_KEY );

    //
    // Enumerate the two sub-keys using the Win 3.1 and the the Win32
    // enumeration APIs.
    //

    KeyNameLength = MAX_PATH;

    Error = RegEnumKey(
                Handle1,
                0,
                KeyName,
                KeyNameLength
                );
    REG_API_SUCCESS( RegEnumKey );

    ASSERT( Compare( KeyName, KEY_NAME_1_1, KEY_NAME_1_1_LENGTH ));

    KeyNameLength   = MAX_PATH;
    ClassNameLength = MAX_PATH;

    Error = RegEnumKeyEx(
                Handle1,
                1,
                KeyName,
                &KeyNameLength,
                NULL,
                ClassName,
                &ClassNameLength,
                &LastWriteTime
                );
    REG_API_SUCCESS( RegEnumKeyEx );

    ASSERT( Compare( KeyName, KEY_NAME_1_2, KEY_NAME_1_2_LENGTH ));
    ASSERT( KeyNameLength == KEY_NAME_1_2_LENGTH );
    //ASSERT( TitleIndex == KEY_NAME_1_2_TITLE_INDEX );
    ASSERT( Compare( ClassName, KEY_NAME_1_2_CLASS, KEY_NAME_1_2_CLASS_LENGTH ));
    ASSERT( ClassNameLength == KEY_NAME_1_2_CLASS_LENGTH );

    //
    // If the Quiet command line option wasn't set, display the TestUser1 key.
    //

    if( ! Quiet ) {
        Key = ParseKey( NameString );
        REG_API_SUCCESS( Key != NULL );
        DisplayKeys( Key, TRUE, TRUE, TRUE );
        FreeKey( Key );
    }

    //
    // Close the two sub-keys.
    //

    Error = RegCloseKey(
                Handle1_1
                );
    REG_API_SUCCESS( RegCloseKey );

    Error = RegCloseKey(
                Handle1_2
                );
    REG_API_SUCCESS( RegCloseKey );

    Error = RegFlushKey(
                Handle1
                );

    REG_API_SUCCESS( RegFlushKey );

    //
    // Save the TestUser1 tree to a file.
    //
#if 0
    Error = RegSaveKey(
                Handle1,
                SAVE_RESTORE_FILE,
                SecurityDescriptor
                );
    REG_API_SUCCESS( RegSaveKey );

    RegCloseKey( Handle1 );

    //
    // Delete the TestUser1 tree.
    //

    DeleteTestTree( );


    //
    //  Load TestUser1 from the file
    //
    Error = RegLoadKey(
                RootHandle,
                KEY_NAME_1,
                SAVE_RESTORE_FILE
                );
    REG_API_SUCCESS( RegLoadKey );

    //
    //  Unload TestUser1
    //
    Error = RegUnLoadKey(
                RootHandle,
                KEY_NAME_1
                );
    REG_API_SUCCESS( RegUnLoadKey );


    //
    // Restore the TestUser1 tree from a file.
    //

    Error = RegCreateKey(
                RootHandle,
                KEY_NAME_1,
                &Handle1
                );
    REG_API_SUCCESS( RegCreateKey );

    Error = RegRestoreKey(
                Handle1,
                SAVE_RESTORE_FILE,
                0
                );
    REG_API_SUCCESS( RegRestoreKey );
#endif

    //
    // Delete the two sub-keys.
    //

    Error = RegDeleteKey(
                Handle1,
                KEY_NAME_1_1
                );
    REG_API_SUCCESS( RegDeleteKey );

    Error = RegDeleteKey(
                Handle1,
                KEY_NAME_1_2
                );
    REG_API_SUCCESS( RegDeleteKey );

    //
    // Set a value in the TestUser1 key using the Win 3.1 compatible API.
    //

    Error = RegSetValue(
                RootHandle,
                KEY_NAME_1,
                VALUE_DATA_1_TYPE,
                Data_1,
                VALUE_DATA_1_LENGTH
                );
    REG_API_SUCCESS( RegSetValue );

    //
    // Set a value in the TestUser1 key using the Win32 API.
    //
    Error = RegSetValueEx(
                Handle1,
                VALUE_NAME_2,
                0,
                VALUE_DATA_2_TYPE,
                ( PVOID ) &Data_2,
                VALUE_DATA_2_LENGTH
                );
    REG_API_SUCCESS( RegSetValueEx );

    //
    // Commit the Key to the Registry.
    //

    Error = RegFlushKey(
                Handle1
                );
    REG_API_SUCCESS( RegFlushKey );

    //
    // If the Quiet command line option wasn't set, display the TestUser1 key.
    //

    if( ! Quiet ) {
        Key = ParseKey( NameString );
        REG_API_SUCCESS( Key != NULL );
        DisplayKeys( Key, TRUE, TRUE, TRUE );
        FreeKey( Key );
    }

    //
    // Query a value in the TestUser1 key using the Win 3.1 compatible API.
    //

    DataLength = MAX_DATA_LENGTH;

    Error = RegQueryValue(
                RootHandle,
                KEY_NAME_1,
                Data,
                &DataLength
                );
    REG_API_SUCCESS( RegQueryValue );

    ASSERT( Compare( Data, &Data_1, VALUE_DATA_1_LENGTH ));
    ASSERT( DataLength == VALUE_DATA_1_LENGTH );

    //
    // Query a value in the TestUser1 key using the Win32 API.
    //

    DataLength = MAX_DATA_LENGTH;

    Error = RegQueryValueEx(
                Handle1,
                VALUE_NAME_2,
                NULL,
                &Type,
                Data,
                &DataLength
                );
    REG_API_SUCCESS( RegQueryValueEx );

    //ASSERT( TitleIndex == VALUE_NAME_2_TITLE_INDEX );
    ASSERT( Type == VALUE_DATA_2_TYPE );
    ASSERT(( DWORD ) Data[ 0 ] == Data_2 );
    ASSERT( DataLength == VALUE_DATA_2_LENGTH );

    //
    // Query information about the key.
    //

    ClassNameLength = MAX_PATH;

    Error = RegQueryInfoKey(
                Handle1,
                ClassName,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime
                );
    REG_API_SUCCESS( RegQueryInfoKey );

    ASSERT( Compare( ClassName, KEY_NAME_1_CLASS, KEY_NAME_1_CLASS_LENGTH ));
    ASSERT( ClassNameLength == KEY_NAME_1_CLASS_LENGTH );
    //ASSERT( TitleIndex == KEY_NAME_1_TITLE_INDEX );
    ASSERT( NumberOfSubKeys == 0 );

    ASSERT( MaxSubKeyLength == 0 );
    ASSERT( MaxClassLength == 0 );


    ASSERT( NumberOfValues == 2 );

    ASSERT( MaxValueNameLength == VALUE_NAME_2_LENGTH * sizeof(WCHAR) );
    ASSERT( MaxValueDataLength == VALUE_DATA_1_LENGTH * sizeof(WCHAR) );

    //
    // Enumerate the values.
    //

    for( Index = 0; Index < 2; Index++ ) {

        ValueNameLength = MAX_PATH;
        DataLength      = MAX_DATA_LENGTH;

        Error = RegEnumValue(
                    Handle1,
                    Index,
                    ValueName,
                    &ValueNameLength,
                    NULL,
                    &Type,
                    Data,
                    &DataLength
                    );
        REG_API_SUCCESS( RegEnumValue );

        //
        // Check specifics depending on the value being queried.
        //

        switch( Index ) {

        case 0:

            //
            // No name - win 3.1 compatible value.
            //

            ASSERT( ValueNameLength == 0 );
            //ASSERT( TitleIndex == VALUE_NAME_1_TITLE_INDEX );
            ASSERT( Type == VALUE_DATA_1_TYPE );
            ASSERT( Compare( Data, Data_1, VALUE_DATA_1_LENGTH ));
            ASSERT( DataLength == VALUE_DATA_1_LENGTH );
            break;

        case 1:

            ASSERT( Compare( ValueName, VALUE_NAME_2, VALUE_NAME_2_LENGTH ));
            ASSERT( ValueNameLength == VALUE_NAME_2_LENGTH );
            //ASSERT( TitleIndex == VALUE_NAME_2_TITLE_INDEX );
            ASSERT( Type == VALUE_DATA_2_TYPE );
            ASSERT(( DWORD ) Data[ 0 ] == Data_2 );
            ASSERT( DataLength == VALUE_DATA_2_LENGTH );
            break;

        default:

            ASSERT_MESSAGE( FALSE, "Valid value enumeration index - " );
        }
    }

    //
    // All done! Get rid of the key and close it.
    //

    Error = RegDeleteKey(
                RootHandle,
                KEY_NAME_1
                );
    REG_API_SUCCESS( RegDeleteKey );

    Error = RegCloseKey(
                Handle1
                );
    REG_API_SUCCESS( RegCloseKey );
}
