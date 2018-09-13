#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <wchar.h>
#include "windows.h"
#include "winreg.h"

#define TEST_STRING     "Test String"
#define TEST_STRING_W   L"Test String"



BOOLEAN
AdjustPrivilege(
    PSTR    SecurityNameString
    )
{
    HANDLE              TokenHandle;
    LUID_AND_ATTRIBUTES LuidAndAttributes;
//    PSTR                SecurityNameString;

    TOKEN_PRIVILEGES    TokenPrivileges;
    TOKEN_PRIVILEGES    PreviousTokenPrivileges;
    DWORD               ReturnLength;

    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &TokenHandle ) ) {
        printf( "OpenProcessToken failed \n" );
        return( FALSE );
    }

//    SecurityNameString = SE_RESTORE_NAME; // SE_SECURITY_NAME;

    if( !LookupPrivilegeValue( NULL,
                               SecurityNameString,
                               &( LuidAndAttributes.Luid ) ) ) {
        printf( "LookupPrivilegeValue failed, Error = %#x \n", GetLastError() );
        return( FALSE );
    }

    LuidAndAttributes.Attributes = SE_PRIVILEGE_ENABLED;
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0] = LuidAndAttributes;

    if( !AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                &TokenPrivileges,
                                0,
                                NULL,
                                NULL ) ) {
        printf( "AdjustTokenPrivileges failed, Error = %#x \n", GetLastError() );
        return( FALSE );
    }


    if( GetLastError() != NO_ERROR ) {
        return( FALSE );
    }

    return( TRUE );
}



#define ENVIRONMENT_NAME    L"Environment"
#define TESTKEY_NAME        L"TestKey"
#define TESTKEY_FULL_NAME   L"Environment\\TestKey"
#define KEY1_NAME           L"Key1"
#define KEY1_FULL_NAME      L"Environment\\TestKey\\Key1"
#define KEY2_NAME           L"Key2"
#define KEY2_FULL_NAME      L"Environment\\TestKey\\Key2"
#define VALUE_NAME          L"123"
#define VALUE_DATA          L"This is a string"



INT __cdecl
main()
{
    DWORD       Status;

    HKEY        TestKeyHandle;
    HKEY        Key1Handle;
    HKEY        Key2Handle;
    HKEY        EnvironmentHandle;

    WCHAR       ValueData[] = VALUE_DATA;

    WCHAR       BufferForKeyName[100];
    WCHAR       BufferForKeyClass[100];
    WCHAR       BufferForValueEntryName[100];
    BYTE        BufferForValueEntryData[100];


    DWORD       DataType;
    DWORD       DataSize;
    DWORD       NameSize;
    DWORD       ClassSize;


    DWORD       cSubKeys;
    DWORD       cbMaxSubkey;
    DWORD       cbMaxClass;
    DWORD       cValues;
    DWORD       vbMaxValueName;
    DWORD       cbMaxValueData;
    DWORD       cbSecurityDescriptor;
    FILETIME    ftLastWriteTime;


    BYTE        BufferForSecurityDescriptor[2048];

    HANDLE      NotificationEvent;

    PWSTR       File1 = L"d:\\File1";
    PWSTR       File2 = L"d:\\File2";
    DWORD       Disposition;



/*
    Key = NULL;
    Status = RegOpenKeyExW( HKEY_CURRENT_USER,
                            L"",
                            0,
                            MAXIMUM_ALLOWED,
                            &Key );
*/








//    AdjustPrivilege( SE_BACKUP_NAME );
//    AdjustPrivilege( SE_RESTORE_NAME );


    NotificationEvent = CreateEvent( NULL,
                                     FALSE,
                                     FALSE,
                                     NULL );

    if( NotificationEvent == NULL ) {
        printf( "CreateEvent failed, ErrorCode = %d \n", GetLastError() );
    }



    Status = RegOpenKeyExW( HKEY_CURRENT_USER,
                            TESTKEY_FULL_NAME,
                            0,
                            MAXIMUM_ALLOWED,
                            &TestKeyHandle );

    if( Status != 0 ) {
        printf( "RegOpenKeyExW failed, Status = %d \n", Status );
    } else {
        printf( "RegOpenKeyExW succeeded \n" );
    }


    Status = RegOpenKeyW( HKEY_CURRENT_USER,
                          ENVIRONMENT_NAME,
                          &EnvironmentHandle );

    if( Status != 0 ) {
        printf( "RegOpenKeyW failed, Status = %d \n", Status );
    } else {
        printf( "RegOpenKeyW succeeded \n" );
    }






    Status = RegCreateKeyExW( TestKeyHandle,
                              KEY1_NAME,
                              NULL,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              MAXIMUM_ALLOWED,
                              NULL,
                              &Key1Handle,
                              &Disposition );

    if( Status != 0 ) {
        printf( "RegCreateKeyExW failed, Status = %d \n", Status );
    } else {
        printf( "RegCreateKeyExW succeeded \n" );
    }



    Status = RegCreateKeyW( TestKeyHandle,
                            KEY2_NAME,
                            &Key2Handle );


    if( Status != 0 ) {
        printf( "RegCreateKeyW failed, Status = %d \n", Status );
    } else {
        printf( "RegCreateKeyW succeeded \n" );
    }


    Status = RegSetValueExW( Key1Handle,
                             VALUE_NAME,
                             NULL,
                             REG_SZ,
                             ValueData,
                             sizeof( ValueData ) );

    if( Status != 0 ) {
        printf( "RegSetValueExW failed, Status = %d \n", Status );
    } else {
        printf( "RegSetValueExW succeeded \n" );
    }


    Status = RegSetValueW( Key1Handle,
                             NULL,
                             REG_SZ,
                             ValueData,
                             sizeof( ValueData ) );

    if( Status != 0 ) {
        printf( "RegSetValueW failed, Status = %d \n", Status );
    } else {
        printf( "RegSetValueW succeeded \n" );
    }



    Status = RegFlushKey( Key1Handle );
    if( Status != 0 ) {
        printf( "RegFlushKey failed, Status = %d \n", Status );
    } else {
        printf( "RegFlushKey succeeded \n" );
    }




    DataSize = sizeof( BufferForValueEntryData );
    memset( BufferForValueEntryData, '\0', DataSize );
    Status = RegQueryValueExW( Key1Handle,
                               VALUE_NAME,
                               NULL,
                               &DataType,
                               BufferForValueEntryData,
                               &DataSize );

    if( Status != 0 ) {
        printf( "RegQueryValueExW failed, Status = %d \n", Status );
    } else {
        printf( "RegQueryValueExW succeeded \n" );
    }



    DataSize = sizeof( BufferForValueEntryData );
    memset( BufferForValueEntryData, '\0', DataSize );
    Status = RegQueryValueW( Key1Handle,
                             NULL,
                             BufferForValueEntryData,
                             &DataSize );


    if( Status != 0 ) {
        printf( "RegQueryValueW failed, Status = %d \n", Status );
    } else {
        printf( "RegQueryValueW succeeded \n" );
    }


    DataSize = sizeof( BufferForValueEntryData );
    memset( BufferForValueEntryData, 'X', DataSize );
    NameSize = sizeof( BufferForValueEntryName );
    memset( BufferForValueEntryName, 'X', NameSize );

    Status = RegEnumValueW( Key1Handle,
                            0,
                            BufferForValueEntryName,
                            &NameSize,
                            NULL,
                            &DataType,
                            BufferForValueEntryData,
                            &DataSize );


    if( Status != 0 ) {
        printf( "RegEnumValueW failed, Status = %d \n", Status );
    } else {
        printf( "RegEnumValueW succeeded \n" );
    }


    NameSize = sizeof( BufferForKeyName );
    ClassSize = sizeof( BufferForKeyClass );
    Status = RegEnumKeyExW( TestKeyHandle,
                            0,
                            BufferForKeyName,
                            &NameSize,
                            NULL,
                            BufferForKeyClass,
                            &ClassSize,
                            &ftLastWriteTime );

    if( Status != 0 ) {
        printf( "RegEnumKeyExW failed, Status = %d \n", Status );
    } else {
        printf( "RegEnumKeyExW succeeded \n" );
    }


    NameSize = sizeof( BufferForKeyName );
    Status = RegEnumKeyW( TestKeyHandle,
                          0,
                          BufferForKeyName,
                          &NameSize );

    if( Status != 0 ) {
	printf( "RegEnumKeyW failed, Status = %d \n", Status );
    } else {
	printf( "RegEnumKeyW succeeded \n" );
    }


    ClassSize = sizeof( BufferForKeyClass );
    Status = RegQueryInfoKeyW( Key1Handle,
                               BufferForKeyClass,
                               &ClassSize,
                               NULL,
                               &cSubKeys,
                               &cbMaxSubkey,
                               &cbMaxClass,
                               &cValues,
                               &vbMaxValueName,
                               &cbMaxValueData,
                               &cbSecurityDescriptor,
                               &ftLastWriteTime );



    if( Status != 0 ) {
        printf( "RegQueryInfoKeyW failed, Status = %d \n", Status );
    } else {
        printf( "RegQueryInfoKeyW succeeded \n" );
    }


    Status = RegGetKeySecurity( Key1Handle,
                                DACL_SECURITY_INFORMATION,
                                ( PSECURITY_DESCRIPTOR )BufferForSecurityDescriptor,
                                &cbSecurityDescriptor );

    if( Status != 0 ) {
        printf( "RegGetKeySecurity failed, Status = %d \n", Status );
    } else {
        printf( "RegGetKeySecurity succeeded \n" );
    }



    Status = RegSetKeySecurity( Key1Handle,
                                DACL_SECURITY_INFORMATION,
                                ( PSECURITY_DESCRIPTOR )BufferForSecurityDescriptor );

    if( Status != 0 ) {
        printf( "RegSetKeySecurity failed, Status = %d \n", Status );
    } else {
        printf( "RegSetKeySecurity succeeded \n" );
    }

/*
    Status = RegSaveKeyW( Key1Handle,
                          File1,
                          NULL );


    if( Status != 0 ) {
        printf( "RegSaveKeyW failed, Status = %d \n", Status );
    } else {
        printf( "RegSaveKeyW succeeded \n" );
    }



    Status = RegRestoreKeyW( Key2Handle,
                             File1,
                             0 );


    if( Status != 0 ) {
        printf( "RegRestoreKeyW failed, Status = %d \n", Status );
    } else {
        printf( "RegRestoreKeyW succeeded \n" );
    }
*/

    Status = RegDeleteValueW( Key1Handle,
                              VALUE_NAME );


    if( Status != 0 ) {
        printf( "RegDeleteValueW failed, Status = %d \n", Status );
    } else {
        printf( "RegDeleteValueW succeeded \n" );
    }


    Status = RegCloseKey( Key1Handle );

    if( Status != 0 ) {
        printf( "RegCloseKey failed, Status = %d \n", Status );
    } else {
        printf( "RegCloseKey succeeded \n" );
    }

    Status = RegDeleteKeyW( TestKeyHandle,
                            KEY1_NAME );

    if( Status != 0 ) {
        printf( "RegCloseKey failed, Status = %d \n", Status );
    } else {
        printf( "RegCloseKey succeeded \n" );
    }


    Status = RegNotifyChangeKeyValue( HKEY_CURRENT_USER,
                                      TRUE,
                                      REG_NOTIFY_CHANGE_NAME |
                                      REG_NOTIFY_CHANGE_ATTRIBUTES |
                                      REG_NOTIFY_CHANGE_LAST_SET |
                                      REG_NOTIFY_CHANGE_SECURITY,
                                      NotificationEvent,
                                      TRUE );

    if( Status != 0 ) {
        printf( "RegNotifyChangeKeyValue failed, Status = %d \n", Status );
    } else {
        printf( "RegNotifyChangeKeyValue succeeded \n" );
    }


    //
    //  Cleanup
    //

    CloseHandle( NotificationEvent );
//    DeleteFileW( File1 );
    RegCloseKey( Key2Handle );
    RegDeleteKeyW( TestKeyHandle,
                   KEY2_NAME );


}
