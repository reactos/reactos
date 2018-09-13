/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    reg.hxx

Abstract:

    contains class definition for registry access.

Author:

    Madan Appiah (madana)  19-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#define DEFAULT_KEY_ACCESS  ( KEY_QUERY_VALUE | \
                               KEY_SET_VALUE | \
                               KEY_CREATE_SUB_KEY | \
                               KEY_ENUMERATE_SUB_KEYS )

#define BASIC_ACCESS  ( KEY_QUERY_VALUE | \
                               KEY_ENUMERATE_SUB_KEYS )

#define DEFAULT_CLASS       TEXT("DefaultClass")
#define DEFAULT_CLASS_SIZE  sizeof(DEFAULT_CLASS)

#define MAX_KEY_SIZE        64 + 1

#define SERVICES_KEY        \
    TEXT("System\\CurrentControlSet\\Services\\")

#define FAIL_IF_KEY_NOT_EXISTS      0
#define CREATE_KEY_IF_NOT_EXISTS    1

typedef struct _KEY_QUERY_INFO {
    TCHAR Class[DEFAULT_CLASS_SIZE];
    DWORD ClassSize;
    DWORD NumSubKeys;
    DWORD MaxSubKeyLen;
    DWORD MaxClassLen;
    DWORD NumValues;
    DWORD MaxValueNameLen;
    DWORD MaxValueLen;
    DWORD SecurityDescriptorLen;
    FILETIME LastWriteTime;
} KEY_QUERY_INFO, *LPKEY_QUERY_INFO;

/*++

Class Description:

    Defines a REGISTRY class that manipulates the registry keys.

Public Member functions:

    Create : is a overloaded function that creates a subkey.

    GetValue : is a overloaded function that retrieves REG_DWORD,
        REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ AND REG_BINARY data values.

    SetValue : is a overloaded function that sets REG_DWORD,
        REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ AND REG_BINARY data values.

    GetNumSubKeys : returns number of subkeys under this key object.
    DeleteKey : deletes a subkey node.

    FindFirstKey : returns the first subkey of this key.
    FindNextKey : returns the next subkey of this key.
--*/

class REGISTRY_OBJ {

private:

    HKEY _RegHandle;
    DWORD _Status;
    DWORD _Index;
    DWORD _ValIndex;
    DWORD _dwAccess;

public:


    REGISTRY_OBJ( HKEY Handle = NULL, DWORD Error = ERROR_SUCCESS );
    REGISTRY_OBJ( HKEY ParentHandle, LPTSTR KeyName, DWORD dwFlags = FAIL_IF_KEY_NOT_EXISTS )
    {
        _dwAccess = DEFAULT_KEY_ACCESS;
        _RegHandle = NULL;
        WorkWith(ParentHandle, KeyName, dwFlags);
    }

    REGISTRY_OBJ( REGISTRY_OBJ *ParentObj, LPTSTR KeyName, DWORD dwFlags = FAIL_IF_KEY_NOT_EXISTS)
    {
        _dwAccess = DEFAULT_KEY_ACCESS;
        _RegHandle = NULL;
        WorkWith(ParentObj, KeyName, dwFlags);
    }

    ~REGISTRY_OBJ( VOID ) {
        if( _RegHandle != NULL ) {
            REGCLOSEKEY( _RegHandle );
        }
        return;
    };

    DWORD WorkWith( HKEY ParentHandle, LPTSTR KeyName, DWORD dwFlags = FAIL_IF_KEY_NOT_EXISTS, DWORD dwAccess = DEFAULT_KEY_ACCESS);
    DWORD WorkWith( REGISTRY_OBJ *ParentObj, LPTSTR KeyName, DWORD dwFlags = FAIL_IF_KEY_NOT_EXISTS );

    DWORD GetStatus( VOID ) {
        return( _Status );
    };

    DWORD GetAccessFlags( VOID ) {
        return( _dwAccess );
    };

    DWORD Create( LPTSTR ChildName, HKEY *pChildHandle = NULL );
    DWORD Create( LPTSTR ChildName, REGISTRY_OBJ **ChildObj );
    DWORD Create( LPTSTR ChildName, REGISTRY_OBJ **ChildObj, DWORD *KeyDisposition );

    DWORD GetValue( LPTSTR ValueName, DWORD *Data );
    DWORD GetValue( LPTSTR ValueName, LPTSTR *Data, DWORD *NumStrings );
    DWORD GetValue( LPTSTR ValueName, LPBYTE *Data, DWORD *DataLen );
    DWORD GetValue( LPTSTR ValueName, LPBYTE Data, DWORD *DataLen );

    DWORD SetValue( LPTSTR ValueName, LPDWORD Data );
    DWORD SetValue( LPTSTR ValueName, LPTSTR Data, DWORD StringType );
    DWORD SetValue( LPSTR ValueName, LPSTR Data, DWORD DataLen, DWORD StringType );
    DWORD SetValue( LPTSTR ValueName, LPBYTE Data, DWORD DataLen );

    DWORD GetKeyInfo( LPKEY_QUERY_INFO QueryInfo ) {

        DWORD Error;
        QueryInfo->ClassSize = DEFAULT_CLASS_SIZE;

        Error = RegQueryInfoKey(
                    _RegHandle,
                    QueryInfo->Class,
                    &QueryInfo->ClassSize,
                    NULL,
                    &QueryInfo->NumSubKeys,
                    &QueryInfo->MaxSubKeyLen,
                    &QueryInfo->MaxClassLen,
                    &QueryInfo->NumValues,
                    &QueryInfo->MaxValueNameLen,
                    &QueryInfo->MaxValueLen,
                    &QueryInfo->SecurityDescriptorLen,
                    &QueryInfo->LastWriteTime
                    );

        return( Error );
    }

    DWORD GetNumSubKeys(DWORD *NumSubKeys ) {

        DWORD Error;
        KEY_QUERY_INFO QueryInfo;

        Error = GetKeyInfo( &QueryInfo );

        if( Error == ERROR_SUCCESS ) {
            *NumSubKeys = QueryInfo.NumSubKeys;
        }

        return( Error );
    }

    DWORD DeleteKey( LPTSTR ChildKeyName );
    DWORD DeleteValue( LPTSTR ValueName );

    DWORD FindNextKey( LPTSTR Key, DWORD KeySize );
    DWORD FindFirstKey( LPTSTR Key, DWORD KeySize ) {
        _Index = 0;
        return( FindNextKey(Key, KeySize) );
    };

    DWORD FindNextValue( LPSTR ValueName, DWORD ValueSize,
                         LPBYTE Data,  DWORD *DataLen );
    DWORD FindFirstValue( LPSTR ValueName, DWORD ValueSize, LPBYTE Data,
                          DWORD *DataLen ) {
        _ValIndex = 0;
        return( FindNextValue(ValueName, ValueSize, Data, DataLen ) );
    };

    DWORD GetValueSizeAndType(
        LPTSTR ValueName,
        LPDWORD ValueSize,
        LPDWORD ValueType ) 
    {
       return RegQueryValueEx( 
            _RegHandle, ValueName, 0, ValueType, NULL, ValueSize); 
    }

};

