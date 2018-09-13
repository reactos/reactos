#define UNICODE
#define _INC_OLE
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <stdio.h>
#include <string.h>

void
FatalError(
    LPSTR Message,
    ULONG_PTR MessageParameter1,
    ULONG_PTR MessageParameter2
    )
{
    if (Message != NULL) {
        fprintf( stderr, "FIXLINK: " );
        fprintf( stderr, Message, MessageParameter1, MessageParameter2 );
        fprintf( stderr, "\n" );
        }

    exit( 1 );
}

void
Usage(
    LPSTR Message,
    ULONG MessageParameter
    )
{
    fprintf( stderr, "usage: FIXLINKS [-v] [-q] [-s SystemRoot | -S searchString replaceString]\n" );
    fprintf( stderr, "                [-r Directory | fileNames]\n" );
    fprintf( stderr, "where: -v specifies verbose output\n" );
    fprintf( stderr, "       -q specifies query only, no actual updating of link files\n" );
    fprintf( stderr, "       -s specifies the value to look for and replace with %%SystemRoot%%\n" );
    fprintf( stderr, "       -S specifies the value to look for and replace with replaceString\n" );
    fprintf( stderr, "       -r Directory specifies the root of a directory tree to\n" );
    fprintf( stderr, "          search for .LNK files to update.\n" );
    fprintf( stderr, "       fileNames specify one or more .LNK files to be updated\n" );

    //
    // No return from FatalError
    //

    if (Message != NULL) {
        fprintf( stderr, "\n" );
        }
    FatalError( Message, MessageParameter, 0 );
}

PWSTR
GetErrorMessage(
    DWORD MessageId
    )
{
    PWSTR Message, s;

    Message = NULL;
    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   NULL,
                   MessageId,
                   0,
                   (PWSTR)&Message,
                   128,
                   (va_list *)NULL
                 );
    if (Message == NULL) {
        Message = (PWSTR)LocalAlloc( 0, 128 );
        swprintf( Message, L"Unable to get message for %08x", MessageId );
        }
    else {
        s = wcsrchr( Message, L'\r' );
        if (s == NULL) {
            s = wcsrchr( Message, L'\n' );
            }

        if (s != NULL) {
            *s = UNICODE_NULL;
            }
        }

    return Message;
}


PWSTR
GetArgAsUnicode(
    LPSTR s
    )
{
    ULONG n;
    PWSTR ps;

    n = strlen( s );
    ps = HeapAlloc( GetProcessHeap(),
                    0,
                    (n + 1) * sizeof( WCHAR )
                  );
    if (ps == NULL) {
        FatalError( "Out of memory", 0, 0 );
        }

    if (MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             s,
                             n,
                             ps,
                             n
                           ) != (LONG)n
       ) {
        FatalError( "Unable to convert parameter '%s' to Unicode (%u)",
                    (ULONG_PTR)s,
                    GetLastError() );
        }

    ps[ n ] = UNICODE_NULL;
    return ps;
}


WCHAR SearchString[ MAX_PATH ];
ULONG cchSearchString;
WCHAR ReplaceString[ MAX_PATH ];
ULONG cchReplaceString;
WCHAR RootOfSearch[ MAX_PATH ];
BOOL VerboseFlag;
BOOL QueryFlag;

HRESULT
ProcessLinkFile(
    PWSTR FileName
    );

void
ProcessLinkFilesInDirectoryTree(
    PWSTR Directory
    );

__cdecl
main(
    int argc,
    char *argv[]
    )
{
    char *s;
    BOOL FileArgumentSeen = FALSE;
    PWSTR FileName;
    HRESULT Error;

    GetEnvironmentVariable( L"SystemRoot", SearchString, MAX_PATH );
    cchSearchString = wcslen( SearchString );
    if (cchSearchString == 0) {
        FatalError( "Unable to get value of SYSTEMROOT environment variable", 0, 0 );
        }
    wcscpy( ReplaceString, L"%SystemRoot%" );
    cchReplaceString = wcslen( ReplaceString );
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'r':
                        if (--argc) {
                            GetFullPathName( GetArgAsUnicode( *++argv ),
                                             MAX_PATH,
                                             RootOfSearch,
                                             &FileName
                                           );
                            }
                        else {
                            Usage( "Missing parameter to -r switch", 0 );
                            }
                        break;

                    case 's':
                        if (--argc) {
                            wcscpy( SearchString, GetArgAsUnicode( *++argv ) );
                            }
                        else {
                            Usage( "Missing parameter to -%c switch", (ULONG)*s);
                            }

                        cchSearchString = wcslen( SearchString );
                        if (cchSearchString == 0) {
                            FatalError( "May not specify an empty search string", 0, 0 );
                            }

                        if (*s == 'S') {
                            if (--argc) {
                                wcscpy( ReplaceString, GetArgAsUnicode( *++argv ) );
                                }
                            else {
                                Usage( "Missing parameter to -S switch", 0 );
                                }

                            cchReplaceString = wcslen( ReplaceString );
                            if (cchSearchString == 0) {
                                FatalError( "May not specify an empty replacement string", 0, 0 );
                                }
                            }
                        break;

                    case 'q':
                        QueryFlag = TRUE;
                        break;

                    case 'v':
                        VerboseFlag = TRUE;
                        break;

                    default:
                        Usage( "Invalid switch -%c'", (ULONG)*s );
                    }
                }
            }
        else {

            if (wcslen( RootOfSearch )) {
                FatalError( "May not specify file names with -r option", 0, 0 );
                }

            FileArgumentSeen = TRUE;
            FileName = GetArgAsUnicode( s );
            if (FileName == NULL) {
                Error = (HRESULT)GetLastError();
                }
            else {
                Error = ProcessLinkFile( FileName );
                }

            if (Error != NO_ERROR) {
                FatalError( "Failed to load from file '%s' (%ws)",
                            (ULONG_PTR)s,
                            (ULONG_PTR)GetErrorMessage( Error )
                            );
                }
            }
        }

    if (!FileArgumentSeen) {
        if (wcslen( RootOfSearch )) {
            ProcessLinkFilesInDirectoryTree( RootOfSearch );
            }
        else {
            Usage( "No textFile specified", 0 );
            }
        }

    return 0;
}

HRESULT
ProcessLinkFile(
    PWSTR FileName
    )
{
    HRESULT rc;
    IShellLink *psl;
    IPersistFile *ppf;
    WCHAR szPath[ MAX_PATH ];
    WCHAR szNewPath[ MAX_PATH ];
    PWSTR s;
    BOOL FileNameShown;
    BOOL FileUpdated;

    CoInitialize( NULL );

    rc = CoCreateInstance( &CLSID_ShellLink,
                           NULL,
                           CLSCTX_INPROC,
                           &IID_IShellLink,
                           &psl
                         );
    if (!SUCCEEDED( rc )) {
        FatalError( "Unable to create ShellLink instance (%ws)",
                    (ULONG_PTR)GetErrorMessage( rc ),
                    0
                    );
        }

    rc = (psl->lpVtbl->QueryInterface)( psl, &IID_IPersistFile, &ppf );
    if (!SUCCEEDED( rc )) {
        FatalError( "Unable to get ShellLink PersistFile interface (%ws)",
                    (ULONG_PTR)GetErrorMessage( rc ),
                    0
                    );
        ppf->lpVtbl->Release( ppf );
        }

    rc = (ppf->lpVtbl->Load)( ppf, FileName, STGM_READWRITE );
    if (!SUCCEEDED( rc )) {
        printf( "%ws: unable to get load link file (%ws)\n", FileName, GetErrorMessage( rc ) );
        ppf->lpVtbl->Release( ppf );
        psl->lpVtbl->Release( psl );
        return S_OK;
        }

    FileUpdated = FALSE;
    FileNameShown = FALSE;
    if (VerboseFlag) {
        printf( "%ws:\n", FileName );
        FileNameShown = TRUE;
        }

    rc = (psl->lpVtbl->GetPath)( psl, szPath, MAX_PATH, NULL, 0 );
    if (SUCCEEDED( rc )) {
        if (!_wcsnicmp( szPath, SearchString, cchSearchString )) {
            if (!FileNameShown) {
                printf( "%ws:\n", FileName );
                FileNameShown = TRUE;
                }
            printf( "    Path: %ws", szPath );
            wcscpy( szNewPath, ReplaceString );
            wcscat( szNewPath, &szPath[ cchSearchString ] );
            if (QueryFlag) {
                printf( " - would be changed to %ws\n", szNewPath );
                }
            else {
                rc = (psl->lpVtbl->SetPath)( psl, szNewPath );
                if (SUCCEEDED( rc )) {
                    printf( " - changed to %ws\n", szNewPath );
                    FileUpdated = TRUE;
                    }
                else {
                    printf( " - unable to modify (%ws)\n", GetErrorMessage( rc ) );
                    }
                }
            }
        else {
            if (VerboseFlag) {
                printf( "    Path: %ws\n", szPath );
                }
            }
        }
    else {
        ppf->lpVtbl->Release( ppf );
        psl->lpVtbl->Release( psl );
        printf( "    Unable to get ShellLink Path (%ws)\n", GetErrorMessage( rc ) );
        }

    rc = (psl->lpVtbl->GetWorkingDirectory)( psl, szPath, MAX_PATH );
    if (SUCCEEDED( rc )) {
        if (!_wcsnicmp( szPath, SearchString, cchSearchString )) {
            if (!FileNameShown) {
                printf( "%ws:\n", FileName );
                FileNameShown = TRUE;
                }
            printf( "    Working Directory: %ws", szPath );
            wcscpy( szNewPath, ReplaceString );
            wcscat( szNewPath, &szPath[ cchSearchString ] );
            if (QueryFlag) {
                printf( " - would be changed to %ws\n", szNewPath );
                }
            else {
                rc = (psl->lpVtbl->SetWorkingDirectory)( psl, szNewPath );
                if (SUCCEEDED( rc )) {
                    printf( " - changed to %ws\n", szNewPath );
                    FileUpdated = TRUE;
                    }
                else {
                    printf( " - unable to modify (%ws)\n", GetErrorMessage( rc ) );
                    }
                }
            }
        else {
            if (VerboseFlag) {
                printf( "    Working Directory: %ws\n", szPath );
                }
            }
        }
    else {
        ppf->lpVtbl->Release( ppf );
        psl->lpVtbl->Release( psl );
        printf( "    Unable to get ShellLink Working Directory (%ws)\n", GetErrorMessage( rc ) );
        }

    if (FileUpdated) {
        rc = (ppf->lpVtbl->Save)( ppf, FileName, TRUE );
        if (SUCCEEDED( rc )) {
            rc = (ppf->lpVtbl->SaveCompleted)( ppf, FileName );
            if (SUCCEEDED( rc )) {
                printf( "    Link file updated.\n" );
                }
            else {
                printf( "    **** unable to save changes to link file (%ws)\n", GetErrorMessage( rc ) );
                }
            }
        }

    ppf->lpVtbl->Release( ppf );
    psl->lpVtbl->Release( psl );
    return S_OK;
}

void
ProcessLinkFilesInDirectoryTree(
    PWSTR Directory
    )
{
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    WCHAR Path[ MAX_PATH ];
    PWSTR FileName;
    ULONG n;

    wcscpy( Path, Directory );
    FileName = &Path[ wcslen( Path ) ];
    *FileName++ = L'\\';
    wcscpy( FileName, L"*" );

    FindHandle = FindFirstFile( Path, &FindData );
    if (FindHandle == INVALID_HANDLE_VALUE) {
        return;
        }

    if (VerboseFlag) {
        printf( "%ws\n", Directory );
        }

    SetCurrentDirectory( Directory );
    do {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp( FindData.cFileName, L"." ) && wcscmp( FindData.cFileName, L".." )) {
                wcscpy( FileName, FindData.cFileName );
                ProcessLinkFilesInDirectoryTree( Path );
                }
            }
        else {
            n = wcslen( FindData.cFileName );
            while (n-- && FindData.cFileName[ n ] != L'.') {
                }

            if (!_wcsicmp( &FindData.cFileName[ n ], L".lnk" )) {
                wcscpy( FileName, FindData.cFileName );
                ProcessLinkFile( Path );
                }
            }
        }
    while (FindNextFile( FindHandle, &FindData ));

    FindClose( FindHandle );
    return;
}
