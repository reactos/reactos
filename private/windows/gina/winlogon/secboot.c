//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       secboot.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include "precomp.h"
#pragma hdrstop
#include <wxlpc.h>
#include <md5.h>
#include "rng.h"

#if DBG
#define HIDDEN
#else
#define HIDDEN static
#endif


UCHAR   KeyDataBuffer[ 16 ];
ULONG   KeyDataPresent = 0;
HICON   KeyDataPwIcon = NULL ;
HICON   KeyDataDiskIcon = NULL ;
NTSTATUS KeyDataStatus = STATUS_SUCCESS ;
HANDLE  KeyDataThread ;
HKEY    LsaKey ;

#define SB_OK       IDOK
#define SB_REBOOT   IDCANCEL

typedef struct _HASH {
    UCHAR Digest[16];
} HASH, *PHASH ;

HIDDEN
UCHAR KeyShuffle[ 16 ] = { 8, 10, 3, 7, 2, 1, 9, 15, 0, 5, 13, 4, 11, 6, 12, 14 };

HIDDEN
CHAR HexKey[ 17 ] = "0123456789abcdef" ;

#define ToHex( f ) (HexKey[f & 0xF])

HIDDEN
BOOL
ObfuscateKey(
    PHASH   Hash
    )
{
    HKEY Key ;
    HKEY Key2 ;
    int Result ;
    HASH H ;
    CHAR Classes[ 9 ];
    int i ;
    HASH R ;
    PCHAR Class ;
    DWORD Disp ;
    DWORD FailCount = 0;


    for (Result = 0 ; Result < 16 ; Result++ )
    {
        H.Digest[Result] = Hash->Digest[ KeyShuffle[ Result ] ];
    }

    (void) RegDeleteKey( LsaKey, TEXT("Data") );
    (void) RegDeleteKey( LsaKey, TEXT("Skew1") );
    (void) RegDeleteKey( LsaKey, TEXT("GBG") );
    (void) RegDeleteKey( LsaKey, TEXT("JD") );

    Classes[8] = '\0';

    STGenerateRandomBits( R.Digest, 16 );

    Class = Classes ;

    for ( i = 0 ; i < 4 ; i++ )
    {
        *Class++ = ToHex( (H.Digest[ i ] >> 4) );
        *Class++ = ToHex( H.Digest[ i ] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "JD",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        RegSetValueEx( Key, TEXT("Lookup"), 0,
                        REG_BINARY, R.Digest, 6 );

        RegCloseKey( Key );
    }

    else
    {
        return FALSE ;
    }

    Class = Classes ;

    for ( i = 0 ; i < 4 ; i++ )
    {
        STGenerateRandomBits( R.Digest, 16 );

        *Class++ = ToHex( (H.Digest[ i+4 ] >> 4 ) );
        *Class++ = ToHex( H.Digest[ i+4 ] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "Skew1",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        RegSetValueEx( Key, TEXT("SkewMatrix"), 0,
                        REG_BINARY, R.Digest, 16 );

        RegCloseKey( Key );
    }
    else
    {
        FailCount++;
    }

    STGenerateRandomBits( R.Digest, 16 );

    for ( i = 0, Class = Classes ; i < 4 ; i++ )
    {
        *Class++ = ToHex( (H.Digest[ i+8 ] >> 4 ));
        *Class++ = ToHex( H.Digest[i+8] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "GBG",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        RegSetValueEx( Key, TEXT("GrafBlumGroup"), 0,
                        REG_BINARY, R.Digest, 9 );

        RegCloseKey( Key );
    }
    else
    {
        FailCount++;
    }

    STGenerateRandomBits( H.Digest, 8 );

    Class = Classes ;

    STGenerateRandomBits( R.Digest, 16 );

    for ( i = 0 ; i < 4 ; i++ )
    {
        *Class++ = ToHex( (H.Digest[ i+12 ] >> 4 ) );
        *Class++ = ToHex( H.Digest[ i+12 ] );
    }

    Result = RegCreateKeyExA( LsaKey,
                              "Data",
                              0,
                              Classes,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key,
                              &Disp );

    if ( Result == 0 )
    {
        STGenerateRandomBits( H.Digest, 16 );

        RegSetValueEx( Key, TEXT("Pattern"), 0,
                        REG_BINARY, R.Digest, 64 );

        RegCloseKey( Key );
    }
    else
    {
        FailCount++;
    }

    return TRUE ;

}

#define FromHex( c )    ( ( ( c >= '0' ) && ( c <= '9') ) ? c - '0' :      \
                          ( ( c >= 'a' ) && ( c <= 'f') ) ? c - 'a' + 10:      \
                          ( ( c >= 'A' ) && ( c <= 'F' ) ) ? c - 'A' + 10: -1 )
HIDDEN
BOOL
DeobfuscateKey(
    PHASH Hash
    )
{
    HASH ProtoHash ;
    int Result ;
    CHAR Class[ 9 ];
    HKEY Key ;
    DWORD Size ;
    DWORD i ;
    PUCHAR j ;
    int t;
    int t2 ;

    Result = RegOpenKeyEx( LsaKey, TEXT("JD"), 0,
                           KEY_READ, &Key );

    j = ProtoHash.Digest ;

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    return FALSE ;
                }
            }

        }

    }

    Result = RegOpenKeyEx( LsaKey, TEXT("Skew1"), 0,
                            KEY_READ, &Key );

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    return FALSE ;
                }
            }

        }

    }

    Result = RegOpenKeyEx( LsaKey, TEXT("GBG"), 0,
                            KEY_READ, &Key );

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    return FALSE ;
                }
            }

        }

    }

    Result = RegOpenKeyEx( LsaKey, TEXT("Data"), 0,
                            KEY_READ, &Key );

    if ( Result == 0 )
    {
        Size = 9 ;

        Result = RegQueryInfoKeyA( Key,
                                   Class,
                                   &Size,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL,
                                   NULL, NULL, NULL );

        RegCloseKey( Key );

        if ( Result == 0 )
        {
            for ( i = 0 ; i < 8 ; i += 2 )
            {
                t = FromHex( Class[ i ] );
                t2 = FromHex( Class[ i+1 ] );
                if ( (t >= 0 ) && ( t2 >= 0 ) )
                {
                    *j++ = (t << 4) + t2 ;
                }
                else
                {
                    return FALSE ;
                }
            }

        }

    }

    for ( i = 0 ; i < 16 ; i++ )
    {
        Hash->Digest[ KeyShuffle[ i ] ] = ProtoHash.Digest[ i ] ;
    }

    return TRUE ;

}

VOID
SbMessageBox(
    HWND hWnd,
    int Text,
    int Caption,
    UINT Flags
    )
{
    WCHAR String1[ MAX_PATH ];
    WCHAR String2[ MAX_PATH ];

    LoadString( GetModuleHandle(NULL), Caption, String1, MAX_PATH );
    LoadString( GetModuleHandle(NULL), Text, String2, MAX_PATH );

    MessageBox( hWnd, String2, String1, Flags );
}

NTSTATUS
SbLoadKeyFromDisk(
    VOID
    )
{
    HANDLE  hFile ;
    ULONG Actual ;
    ULONG ErrorMode ;

    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );


    hFile = CreateFileA( "A:\\startkey.key",
                         GENERIC_READ,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );



    if ( hFile == INVALID_HANDLE_VALUE )
    {
        SetErrorMode( ErrorMode );

        return STATUS_OBJECT_NAME_NOT_FOUND ;
    }

    if (!ReadFile( hFile, KeyDataBuffer, 16, &Actual, NULL ) ||
        (Actual != 16 ))
    {
        SetErrorMode( ErrorMode );

        CloseHandle( hFile );

        return STATUS_FILE_CORRUPT_ERROR ;

    }

    KeyDataPresent = 1 ;

    SetErrorMode( ErrorMode );

    CloseHandle( hFile );

    return STATUS_SUCCESS ;
}

INT_PTR
CALLBACK
SbPromptDlg(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    WCHAR PW[ 128 ];
    MD5_CTX Md5;
    int PWLen ;

    switch ( Message )
    {
        case WM_INITDIALOG:
            if ( KeyDataPwIcon == NULL )
            {
                KeyDataPwIcon = LoadImage( GetModuleHandle(NULL),
                                           MAKEINTRESOURCE( IDD_SB_ICON_PW ),
                                           IMAGE_ICON,
                                           64, 72,
                                           LR_DEFAULTCOLOR );

            }

            SendMessage( GetDlgItem( hDlg, IDD_SB_PW_ICON ),
                         STM_SETICON,
                         (WPARAM) KeyDataPwIcon,
                         0 );

            CentreWindow( hDlg );

            return TRUE ;

        case WM_COMMAND:
            switch ( LOWORD( wParam ) )
            {
                case IDCANCEL:
                    EndDialog( hDlg, SB_REBOOT );
                    return TRUE ;

                case IDOK:

                    // Get text

                    PWLen = GetDlgItemText( hDlg, IDD_SB_PASSWORD, PW, 128 );

                    // Convert length to bytes

//                    PWLen++;
                    PWLen *= sizeof(WCHAR);

                    // hash it

                    MD5Init( &Md5 );
                    MD5Update( &Md5, (PUCHAR) PW, PWLen );
  //                  MD5Update( &Md5, (PUCHAR) PW, PWLen + 1 );
                    MD5Final( &Md5 );

                    // save it

                    RtlCopyMemory( KeyDataBuffer, Md5.digest, 16 );
                    KeyDataPresent = 1;

                    // clean up:

                    EndDialog( hDlg, SB_OK );
                    FillMemory( PW, PWLen, 0xFF );
                    ZeroMemory( PW, PWLen );
                    FillMemory( &Md5, sizeof( Md5 ), 0xFF );
                    ZeroMemory( &Md5, sizeof( Md5 ) );

                    return TRUE ;
                default:
                    break;

            }
        case WM_CLOSE:
            break;

    }

    return FALSE ;
}


INT_PTR
CALLBACK
SbDiskDlg(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    NTSTATUS Status ;

    switch ( Message )
    {
        case WM_INITDIALOG:
            if ( KeyDataDiskIcon == NULL )
            {
                KeyDataDiskIcon = LoadImage( GetModuleHandle(NULL),
                                           MAKEINTRESOURCE( IDD_SB_ICON_DISK ),
                                           IMAGE_ICON,
                                           64, 72,
                                           LR_DEFAULTCOLOR );

            }

            SendMessage( GetDlgItem( hDlg, IDD_SB_DISK_ICON ),
                         STM_SETICON,
                         (WPARAM) KeyDataDiskIcon,
                         0 );

            CentreWindow( hDlg );

            return TRUE ;

        case WM_COMMAND:
            switch ( LOWORD( wParam ) )
            {
                case IDCANCEL:
                    EndDialog( hDlg, SB_REBOOT );
                    return TRUE ;

                case IDOK:
                    Status = SbLoadKeyFromDisk();
                    if ( !NT_SUCCESS( Status ) )
                    {
                        SbMessageBox( hDlg,
                                      IDS_KEYFILE_NOT_FOUND,
                                      IDS_KEYFILE_NOT_FOUND_CAP,
                                      MB_ICONSTOP | MB_OK);
                    }
                    else
                    {
                        EndDialog( hDlg, SB_OK );
                    }
                    return TRUE ;
                default:
                    break;

            }
        case WM_CLOSE:
            break;

    }

    return FALSE ;
}

BOOL
SbGetKeyData(
    WX_AUTH_TYPE ExpectedAuth
    )
{
    int result = SB_OK ;

    if ( ExpectedAuth == WxPrompt )
    {
        result = (int)DialogBoxParam( GetModuleHandle(NULL),
                                      MAKEINTRESOURCE( IDD_SECURE_BOOT ),
                                      NULL,
                                      SbPromptDlg,
                                      0 );
    }
    else if ( ExpectedAuth == WxDisk )
    {
        result = (int)DialogBoxParam( GetModuleHandle( NULL ),
                                      MAKEINTRESOURCE( IDD_SECURE_BOOT_DISK ),
                                      NULL,
                                      SbDiskDlg,
                                      0 );

    }
    else if ( ExpectedAuth == WxStored )
    {
        result = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                            0,
                            KEY_READ,
                            &LsaKey );

        if ( result == 0 )
        {
            if ( DeobfuscateKey( (PHASH) KeyDataBuffer ) )
            {
                KeyDataPresent = 1;
            }
            else
            {
                result = SB_REBOOT ;
            }
        }
        else
        {
            result = SB_REBOOT ;
        }

    }


    if ( result == SB_REBOOT )
    {
        EnablePrivilege( SE_SHUTDOWN_PRIVILEGE, TRUE );

        NtShutdownSystem( ShutdownReboot );
    }

    return TRUE ;
}



NTSTATUS
WxGetKeyData(
    IN HANDLE Handle,
    IN WX_AUTH_TYPE ExpectedAuthSource,
    IN ULONG BufferSize,
    OUT PUCHAR Buffer,
    OUT PULONG BufferData
    )
{
    if ( !KeyDataPresent )
    {
        SbGetKeyData( ExpectedAuthSource );
    }

    if ( BufferSize > 16 )
    {
        BufferSize = 16 ;
    }

    RtlCopyMemory( Buffer, KeyDataBuffer, BufferSize );

    FillMemory( KeyDataBuffer, 16, 0xFF );
    ZeroMemory( KeyDataBuffer, 16 );
    KeyDataPresent = 0;

    *BufferData = BufferSize ;

    return STATUS_SUCCESS ;

}

NTSTATUS
WxReportResults(
    IN HANDLE Handle,
    IN NTSTATUS Status
    )
{
    KeyDataStatus = Status ;
    return STATUS_SUCCESS ;

}

VOID
WxClientDisconnect(
    IN HANDLE Handle
    )
{
    if ( KeyDataStatus != STATUS_SUCCESS )
    {
        EnablePrivilege( SE_SHUTDOWN_PRIVILEGE, TRUE );

        NtShutdownSystem( ShutdownReboot );

    }
}


NTSTATUS
WxCallDialog(
    IN HANDLE Handle,
    IN ULONG Flags,
    IN PUNICODE_STRING DllName,
    IN LPCWSTR ResourceId,
    IN PSTRING FunctionName,
    IN OUT PVOID Data,
    IN ULONG DataLength,
    OUT int * Result
    )
{
    return STATUS_NOT_IMPLEMENTED ;
}


DWORD
SbKeyData(
    PVOID Ignored
    )
{
    NTSTATUS Status ;

    Status = WxServerThread( NULL );

    return (DWORD) Status ;

}

NTSTATUS
SbBootPrompt(
    VOID
    )
{
    HKEY Key ;
    int err ;
    DWORD Type ;
    DWORD Length ;
    DWORD Value ;
    WX_AUTH_TYPE AuthType ;
    DWORD tid ;

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                        0,
                        KEY_READ,
                        &Key );

    if ( err == 0 )
    {
        Length = sizeof( Value );

        err = RegQueryValueEx( Key,
                                TEXT("SecureBoot"),
                                NULL,
                                &Type,
                                (PUCHAR) &Value,
                                &Length );
        if ( err == 0 )
        {
            AuthType = (WX_AUTH_TYPE) Value ;

            SbGetKeyData( AuthType );

            KeyDataThread = CreateThread( NULL, 0, SbKeyData, NULL, 0, &tid );
        }

        RegCloseKey( Key );
    }

    return STATUS_SUCCESS ;
}

VOID
SbSyncWithKeyThread(
    VOID
    )
{
    if ( KeyDataThread )
    {
        WaitForSingleObject( KeyDataThread, INFINITE );

        CloseHandle( KeyDataThread );
    }
}
