/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Crtools.h

Abstract:

    This module is the master header file for the Configuration Registry
    Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <windows.h>
#include <winreg.h>


//
// Additional type to declare string arrays.
//

//
// Assertion/debug macros/functions.
//


#ifdef UNICODE

#define NUL_SIZE    ( 2 )
typedef WCHAR       TSTR;

#else

#define NUL_SIZE    ( 1 )
typedef char        TSTR;

#endif // UNICODE

#if DBG

VOID
CrAssert(
    IN PSTR FailedAssertion,
    IN PSTR FileName,
    IN DWORD LineNumber,
    IN PSTR Message OPTIONAL
    );

#define ASSERT( exp )                                                   \
    if( !( exp ))                                                       \
        CrAssert( #exp, __FILE__, __LINE__, NULL )

#define ASSERT_MESSAGE( exp, msg )                                      \
    if( !( exp ))                                                       \
        CrAssert( #exp, __FILE__, __LINE__, msg )

#define ASSERT_IS_KEY( Key )                                            \
    ASSERT( ARGUMENT_PRESENT( Key ));                                   \
    ASSERT( Key->Signature == KEY_SIGNATURE );

#else

#define ASSERT( exp )

#define ASSERT_MESSAGE( msg, exp )

#define ASSERT_IS_KEY( Key )

#endif // DBG


//
// Macro to check for a switch character.
//

#define isswitch( s )                                                   \
    ((( s ) == '-' ) || (( s ) == '/' ))
//
// Macro to check if an argument is present (i.e. non-NULL).
//

#define ARGUMENT_PRESENT( arg )                                         \
    ((( PVOID ) arg ) != (( PVOID ) NULL ))

//
//  Compare two blocks of memory for equality.
//
//  BOOL
//  Compare(
//      IN PVOID Block1,
//      IN PVOID Block2,
//      IN DWORD NumberOfBytes
//      );
//

#define Compare( s1, s2, c )                                            \
    ( memcmp(( PVOID )( s1 ), ( PVOID )( s2 ), ( size_t )( c )) == 0 )

//
// Compute the length (in bytes) of a Unicode string w/o the trailing NUL.
//

#define LENGTH( str )   ( sizeof( str ) -  NUL_SIZE )

//
// Check the success of a Win32 Registry API.
//

#define REG_API_SUCCESS( api )                                          \
    ASSERT_MESSAGE( Error == ERROR_SUCCESS, #api )

//
//
// A KEY structure is used to hold information about a Registry Key.
//

typedef struct _KEY
    KEY,
    *PKEY;

struct _KEY {
    PKEY        Parent;
    HKEY        KeyHandle;
    PSTR        SubKeyName;
    PSTR        SubKeyFullName;
    PSTR        ClassName;
    DWORD       ClassLength;
    DWORD       TitleIndex;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyNameLength;
    DWORD       MaxSubKeyClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;

#if DBG

    DWORD       Signature;

#endif

};

#define FILE_TIME_STRING_LENGTH     ( 25 * sizeof( TCHAR ))
#define KEY_SIGNATURE               ( 0xABBABAAB )

#define HKEY_CLASSES_ROOT_STRING    "HKEY_CLASSES_ROOT"
#define HKEY_CURRENT_USER_STRING    "HKEY_CURRENT_USER"
#define HKEY_LOCAL_MACHINE_STRING   "HKEY_LOCAL_MACHINE"
#define HKEY_USERS_STRING           "HKEY_USERS"

extern KEY  KeyClassesRoot;
extern KEY  KeyCurrentUser;
extern KEY  KeyLocalMachine;
extern KEY  KeyUsers;


PKEY
AllocateKey(
    IN PSTR MachineName,
    IN PKEY Parent,
    IN PSTR SubKeyName
    );

VOID
DisplayData(
    IN PBYTE ValueData,
    IN DWORD ValueDataLength
    );

VOID
DisplayKey(
    IN PKEY Key,
    IN BOOL Values,
    IN BOOL Data
    );

VOID
DisplayKeys(
    IN PKEY Key,
    IN BOOL Values,
    IN BOOL Data,
    IN BOOL Recurse
    );

VOID
DisplayKeyInformation(
    IN PKEY Key
    );

VOID
DisplayMessage(
    IN BOOL Terminate,
    IN PSTR Format,
    IN ...
    );

VOID
DisplayValues(
    IN PKEY Key,
    IN BOOL Data
    );

PSTR
FormatFileTime(
    IN PFILETIME FileTime OPTIONAL,
    IN PSTR Buffer OPTIONAL
    );

VOID
FreeKey(
    IN PKEY Key
    );

PKEY
ParseKey(
    IN PSTR SubKeyName
    );
