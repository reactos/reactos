#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "windows.h"

#define MSG_ERROR_VALUE_INCORRECT_SIZE        "\tERROR: Value entry data has incorrect size \n\t\tValueName = %ls \n\t\tNameSize  = %d \n\t\tValueType = %s \n\t\tValueSize = %d\n"
#define MSG_ERROR_VALUE_NOT_NUL_TERMINATED    "\tERROR: Value entry data is not NUL terminated \n\t\tValueName = %ls \n\t\tNameSize  = %d \n\t\tValueType = %s\n"
#define MSG_ERROR_VALUE_UNKNOWN_DATA          "\tERROR: Value entry contains unknown data \n\t\tValueName = %ls \n\t\tNameSize  = %d \n\t\tValueType = %#x \n\t\tValueSize = %d\n"
#define MSG_ERROR_REG_ENUM_VALUE              "\tERROR: RegEnumValue() failed, iValue = %d, Status = %d \n"
#define MSG_ERROR_REG_OPEN_KEY_EX             "\tERROR: RegOpenKeyEx() failed, Status = %d \n"
#define MSG_ERROR_REG_QUERY_INFO_KEY          "\tERROR: RegQueryInfoKey() failed, Status = %d \n"
#define MSG_ERROR_REG_ENUM_KEY_EX             "ERROR: RegEnumKeyEx() failed, \n\t Status = %d \n\t, SubKey = %d"
#define MSG_ERROR_REG_CONNECT_REGISTRY        "ERROR: Unable to connect to %s, Status = %d \n"
#define MSG_COMPLETE_KEY_NAME                 "%ls\\%ls \n"

VOID
ExamineValueEntries( IN HKEY    Key,
                     IN LPCWSTR CompleteKeyName,
                     IN DWORD   cchMaxValueName,
                     IN DWORD   cbMaxValueData,
                     IN DWORD   cValues,
                     IN LPCWSTR PredefinedKeyName )


{
    LONG    Status;
    DWORD   iValue;
    LPWSTR  lpszValue;
    DWORD   cchValue;
    DWORD   dwType;
    PBYTE   lpbData;
    DWORD   cbData;
    BOOLEAN KeyNameAlreadyPrinted;

    //
    //  Allocate the buffers for the value name and value data
    //

    lpszValue = ( LPWSTR )malloc( (cchMaxValueName + 1)*sizeof( WCHAR ) );
    lpbData = ( LPBYTE )malloc( cbMaxValueData );
    if( ( lpszValue == NULL ) ||
        ( lpbData == NULL ) ) {
        printf( "ERROR: Unable to allocate memory, cchMaxValueName = %d, cbMaxValuedata = %d \n",
                 cchMaxValueName, cbMaxValueData );
        if( lpszValue != NULL ) {
            free( lpszValue );
        }
        if( lpbData != NULL ) {
            free( lpbData );
        }
        return;
    }

    //
    //  Examine all value entries
    //

    KeyNameAlreadyPrinted = FALSE;
    for( iValue = 0; iValue < cValues; iValue++ ) {
        cchValue = cchMaxValueName + 1;
        cbData = cbMaxValueData;
        Status = RegEnumValueW( Key,
                                iValue,
                                lpszValue,
                                &cchValue,
                                0,
                                &dwType,
                                lpbData,
                                &cbData );

        if( Status != ERROR_SUCCESS ) {
            if( !KeyNameAlreadyPrinted ) {
                KeyNameAlreadyPrinted = TRUE;
                printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
            }
            printf( MSG_ERROR_REG_ENUM_VALUE, iValue, Status );
            continue;
        }

//
//      For debugging only
//
//        printf( "\tValueName = %ls \n", lpszValue );
//

        switch( dwType ) {

            case REG_BINARY:

                if( cbData == 0 ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_BINARY", cbData );
                }
                break;

            case REG_DWORD:

                if( cbData != sizeof( DWORD ) ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_DWORD", cbData );
                }
                break;

            case REG_DWORD_BIG_ENDIAN:

                if( cbData != sizeof( DWORD ) ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_DWORD_BIG_ENDIAN", cbData );
                }
                break;

            case REG_EXPAND_SZ:

                if( ( cbData != 0 )  && ( ( cbData % sizeof( WCHAR ) ) == 0 )) {
                    if( *( ( PWCHAR )( lpbData + cbData - sizeof( WCHAR ) ) ) != ( WCHAR )'\0' ) {
                        if( !KeyNameAlreadyPrinted ) {
                            KeyNameAlreadyPrinted = TRUE;
                            printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                        }
                        printf( MSG_ERROR_VALUE_NOT_NUL_TERMINATED,
                                lpszValue, cchValue, "REG_EXPAND_SZ" );
                    }
                } else {
                        if( !KeyNameAlreadyPrinted ) {
                            KeyNameAlreadyPrinted = TRUE;
                            printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                        }
                        printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                                lpszValue, cchValue, "REG_EXPAND_SZ", cbData );
                }
                break;

            case REG_LINK:

                if( cbData == 0 ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_LINK", cbData );
                }
                break;

            case REG_MULTI_SZ:

                if( ( cbData != 0 )  && ( ( cbData % sizeof( WCHAR ) ) == 0 )) {
                    if( *( ( PWCHAR )( lpbData + cbData - sizeof( WCHAR ) ) ) != ( WCHAR )'\0' ) {
                        if( !KeyNameAlreadyPrinted ) {
                            KeyNameAlreadyPrinted = TRUE;
                            printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                        }
                        printf( MSG_ERROR_VALUE_NOT_NUL_TERMINATED,
                                lpszValue, cchValue, "REG_MULTI_SZ" );
                    }
                } else {
                        if( !KeyNameAlreadyPrinted ) {
                            KeyNameAlreadyPrinted = TRUE;
                            printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                        }
                        printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                                lpszValue, cchValue, "REG_MULTI_SZ", cbData );
                }
                break;

            case REG_NONE:

                if( cbData != 0 ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_NONE", cbData );
                }
                break;

            case REG_RESOURCE_LIST:

                if( cbData == 0 ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_RESOURCE_LIST", cbData );
                }
                break;


            case REG_SZ:

                if( ( cbData != 0 ) && ( ( cbData % sizeof( WCHAR ) ) == 0 ) ) {
                    if( *( ( PWCHAR )( lpbData + cbData - sizeof( WCHAR ) ) ) != ( WCHAR )'\0' ) {
                        if( !KeyNameAlreadyPrinted ) {
                            KeyNameAlreadyPrinted = TRUE;
                            printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                        }
                        printf( MSG_ERROR_VALUE_NOT_NUL_TERMINATED,
                                lpszValue, cchValue, "REG_SZ" );
                    }
                } else {
                        if( !KeyNameAlreadyPrinted ) {
                            KeyNameAlreadyPrinted = TRUE;
                            printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                        }
                        printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                                lpszValue, cchValue, "REG_SZ", cbData );
                }
                break;

            case REG_FULL_RESOURCE_DESCRIPTOR:

                if( cbData == 0 ) {
                    if( !KeyNameAlreadyPrinted ) {
                        KeyNameAlreadyPrinted = TRUE;
                        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                    }
                    printf( MSG_ERROR_VALUE_INCORRECT_SIZE,
                            lpszValue, cchValue, "REG_FULL_RESOURCE_DESCRIPTOR", cbData );
                }
                break;

            default:

                if( !KeyNameAlreadyPrinted ) {
                    KeyNameAlreadyPrinted = TRUE;
                    printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                }
                printf( MSG_ERROR_VALUE_UNKNOWN_DATA,
                        lpszValue, cchValue, dwType, cbData );
                break;

        }
    }

    //
    //  Free buffers for value name and value data
    //
    free( lpszValue );
    free( lpbData );
}





VOID
ExamineKey(
    IN  HKEY    PredefinedKey,
    IN  LPCWSTR ParentName,
    IN  LPCWSTR KeyName,
    IN  LPCWSTR PredefinedKeyName
    )

{
    LPWSTR      CompleteKeyName;

    HKEY        Key;

    LONG        Status;

    WCHAR       szClass[ MAX_PATH + 1 ];
    DWORD       cchClass;
    DWORD       cSubKeys;
    DWORD       cchMaxSubKey;
    DWORD       cchMaxClass;
    DWORD       cValues;
    DWORD       cchMaxValueName;
    DWORD       cbMaxValueData;
    DWORD       cbSecurityDescriptor;
    FILETIME    ftLastWriteTime;

    WCHAR       szSubKeyName[ MAX_PATH + 1 ];
    DWORD       cchSubKeyNameLength;

    DWORD       iSubKey;
    BOOLEAN     KeyNameAlreadyPrinted;


    //
    //  Build the complete key name
    //

    if( wcslen( ParentName ) == 0 ) {
        CompleteKeyName = wcsdup( KeyName );
        if( CompleteKeyName == NULL ) {
            printf( "ERROR: wcsdup( KeyName ) failed \n" );
            return;
        }
    } else {
        CompleteKeyName = wcsdup( ParentName );
        if( CompleteKeyName == NULL ) {
            printf( "ERROR: wcsdup( ParentName ) failed \n" );
            return;
        }
        if( wcslen( KeyName ) != 0 ) {
            CompleteKeyName = realloc( CompleteKeyName,
                                       ( wcslen( CompleteKeyName ) +
                                         wcslen( L"\\" ) +
                                         wcslen( KeyName ) + 1 )*sizeof( WCHAR ) );
            wcscat( CompleteKeyName, L"\\" );
            wcscat( CompleteKeyName, KeyName );
        }
    }

//
//  For debugging only
//
//    printf( "%ls\\%ls \n", PredefinedKeyName, CompleteKeyName );
//

    //
    //  Open the key
    //

    Status = RegOpenKeyExW( PredefinedKey,
                           CompleteKeyName,
                           0,
                           MAXIMUM_ALLOWED,
                           &Key );


    if( Status != ERROR_SUCCESS ) {
        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
        printf( MSG_ERROR_REG_OPEN_KEY_EX, Status );
        free( CompleteKeyName );
        return;
    }

    //
    //  Determine the number of value entries, the maximum length of a value
    //  entry name, the maximum data size, and the number of subkeys
    //

    cchClass = sizeof( szClass ) / sizeof( WCHAR );
    Status = RegQueryInfoKeyW( Key,
                               szClass,
                               &cchClass,
                               0,
                               &cSubKeys,
                               &cchMaxSubKey,
                               &cchMaxClass,
                               &cValues,
                               &cchMaxValueName,
                               &cbMaxValueData,
                               &cbSecurityDescriptor,
                               &ftLastWriteTime );

    if( Status != ERROR_SUCCESS ) {
        printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
        printf( MSG_ERROR_REG_QUERY_INFO_KEY, Status );
        free( CompleteKeyName );
        RegCloseKey( Key );
        return;
    }


    if( cValues != 0 ) {

        //
        // Examine the value entries
        //

        ExamineValueEntries( Key,
                            CompleteKeyName,
                            cchMaxValueName,
                            cbMaxValueData,
                            cValues,
                            PredefinedKeyName );

    }

    //
    //  Traverse each subkey
    //
    if( cSubKeys != 0 ) {
        KeyNameAlreadyPrinted = FALSE;
        for( iSubKey = 0; iSubKey < cSubKeys; iSubKey++ ) {
            cchSubKeyNameLength = sizeof( szSubKeyName )/sizeof( WCHAR );
            cchClass = sizeof( szClass ) / sizeof( WCHAR );
            Status = RegEnumKeyExW( Key,
                                    iSubKey,
                                    szSubKeyName,
                                    &cchSubKeyNameLength,
                                    0,
                                    NULL,
                                    NULL,
                                    &ftLastWriteTime );

            if( Status != ERROR_SUCCESS ) {
                if( !KeyNameAlreadyPrinted ) {
                    KeyNameAlreadyPrinted = TRUE;
                    printf( MSG_COMPLETE_KEY_NAME, PredefinedKeyName, CompleteKeyName );
                }
                printf( MSG_ERROR_REG_ENUM_KEY_EX, Status, iSubKey );
                continue;
            }
            ExamineKey( PredefinedKey,
                        CompleteKeyName,
                        szSubKeyName,
                        PredefinedKeyName );
        }
    }
    RegCloseKey( Key );

    free( CompleteKeyName );
}




main( int argc, char* argv[] )
{
    DWORD   i;
    HKEY    RemoteUsers;
    HKEY    RemoteLocalMachine;
    LONG    Status;

    if( argc <= 1 ) {
        printf( "\n******* Examining HKEY_LOCAL_MACHINE on local machine\n\n" );
        ExamineKey( HKEY_LOCAL_MACHINE,
                    L"",
                    L"",
                    L"HKEY_LOCAL_MACHINE" );

        printf( "\n******* Examining HKEY_USERS on local machine\n\n" );
        ExamineKey( HKEY_USERS,
                    L"",
                    L"",
                    L"HKEY_USERS" );

        printf( "\n******* Examining HKEY_CLASSES_ROOT on local machine\n\n" );
        ExamineKey( HKEY_CLASSES_ROOT,
                    L"",
                    L"",
                    L"HKEY_CLASSES_ROOT" );

        printf( "\n******* Examining HKEY_CURRENT_USER on local machine\n\n" );
        ExamineKey( HKEY_CURRENT_USER,
                    L"",
                    L"",
                    L"HKEY_CURRENT_USER" );
    } else {
        for( i = 1; i < argc; i++ ) {
            //
            // printf( "Machine name = %s \n", argv[ i ] );
            //

            Status = RegConnectRegistry( argv[ i ],
                                         HKEY_LOCAL_MACHINE,
                                         &RemoteLocalMachine );

            if( Status != ERROR_SUCCESS ) {
                printf( MSG_ERROR_REG_CONNECT_REGISTRY, argv[i], Status );
                continue;
            }

            Status = RegConnectRegistry( argv[ i ],
                                         HKEY_USERS,
                                         &RemoteUsers );

            if( Status != ERROR_SUCCESS ) {
                RegCloseKey( RemoteLocalMachine );
                printf( MSG_ERROR_REG_CONNECT_REGISTRY, argv[i], Status );
                continue;
            }

            printf( "\n******* Examining HKEY_LOCAL_MACHINE on %s \n\n", argv[i] );
            ExamineKey( RemoteLocalMachine,
                        L"",
                        L"",
                        L"HKEY_LOCAL_MACHINE" );

            printf( "\n******* Examining HKEY_USERS on %s \n\n", argv[i] );
            ExamineKey( RemoteUsers,
                        L"",
                        L"",
                        L"HKEY_USERS" );

            RegCloseKey( RemoteLocalMachine );
            RegCloseKey( RemoteUsers );
        }
    }
}
