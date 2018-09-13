#include <cdlpch.h>
#pragma hdrstop

#include <winnls.h>
#include <shlobj.h>
 
CMutexSem g_mxsCDLGetLongPathNameGlobals;

// stolen from shellp.h
#define ILCreateFromPathORD                     157
#define ILFreeORD                               155

// stolen from shsemip.h
typedef LPITEMIDLIST  (WINAPI *ILCreateFromPathPtr)(LPCWSTR pszPath);
typedef void          (WINAPI *ILFreePtr)(LPITEMIDLIST pidl);

// We'll use this if we're running on Memphis or NT5
#ifdef UNICODE
#define STR_GETLONGPATHNAMEW "GetLongPathNameW"
typedef DWORD (WINAPI *GetLongPathNameWPtr)( LPCWSTR lpszShortPath,
                                             LPWSTR  lpszLongPath,
                                             DWORD    cchBuffer
                                           );
#else
#define STR_GETLONGPATHNAMEA "GetLongPathNameA"
typedef DWORD (WINAPI *GetLongPathNameAPtr)( LPCSTR lpszShortPath,
                                             LPSTR  lpszLongPath,
                                             DWORD    cchBuffer
                                           );
#endif


STATIC ILCreateFromPathPtr     s_pfnILCreate;
STATIC ILFreePtr               s_pfnILFree;
STATIC GetLongPathNameAPtr     s_pfnGetLongPathNameA;

#define cKnownDirs  5
STATIC struct KnownDirsMap {
    BOOL   m_bInited;
    LPTSTR m_aszCaches[cKnownDirs];
    struct _tagKDMap {
        TCHAR szShort[MAX_PATH];
        int cchShort;
        TCHAR szCanonical[MAX_PATH];
        int cchCanonical;
    } m_aKDMap[cKnownDirs];

    int IndexKnownDirs( LPTSTR szName )
    {
        int i;

        for ( i = 0; i < cKnownDirs; i++ ) {
            // we only want to compare out through the cache folder itself 
            BOOL fMatch = (m_aKDMap[i].cchShort != 0 &&
                           CompareString( LOCALE_SYSTEM_DEFAULT,
                                          NORM_IGNORECASE,
                                          m_aKDMap[i].szShort,
                                          m_aKDMap[i].cchShort,
                                          szName,
                                          m_aKDMap[i].cchShort ) == 2)
                                                 ||
                          (m_aKDMap[i].cchCanonical != 0 &&
                           CompareString( LOCALE_SYSTEM_DEFAULT,
                                          NORM_IGNORECASE,
                                          m_aKDMap[i].szCanonical,
                                          m_aKDMap[i].cchCanonical,
                                          szName,
                                          m_aKDMap[i].cchCanonical ) == 2);
            if ( fMatch )
                break;
        }

        if ( i >= cKnownDirs )
            i = -1; // signal a miss
        return i;
    };

} s_kdMap = {
    FALSE,
    { 
    "\\Occache\\",
    "\\OC Cache\\",
    "\\Downloaded ActiveX Controls\\",
    "\\Downloaded Components\\",
    "\\Downloaded Program Files\\"
    },
    {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 }
    }
};

STATIC BOOL IsCanonicalName( LPCTSTR szName )
{
    // simple test - if there's a ~ in it, it has a contraction in it
    // and is therefore non-canonical
    for ( ; *szName != '\0' && *szName != '~'; szName++ );
    
    return *szName != '~';
}

STATIC DWORD s_CDLGetLongPathName( LPTSTR szLong, LPCTSTR szShort, DWORD cchBuffer)
{
    HRESULT hr = E_FAIL;
    HMODULE hmodS32;
    HMODULE hmodK32;
    DWORD   cchLong = 0;

    // Don't jump through all these hoops if there aren't any contractions in it.
    if ( IsCanonicalName( szShort ) ) {
        lstrcpyn( szLong, szShort, cchBuffer );
        return lstrlen( szLong );
    }

    hmodS32 = LoadLibrary( "SHELL32.DLL" );
    hmodK32 = LoadLibrary( "KERNEL32.DLL" );

    // Set up our globals with short and long versions of the base cache path 
    if ( hmodS32 && hmodK32 ) {
#ifdef UNICODE
        s_pfnGetLongPathNameW = (GetLongPathNameWPtr)GetProcAddress(hmodK32, (LPCSTR)STR_GETLONGPATHNAMEW );
#else
        s_pfnGetLongPathNameA = (GetLongPathNameAPtr)GetProcAddress(hmodK32, (LPCSTR)STR_GETLONGPATHNAMEA );
#endif
        s_pfnILCreate = (ILCreateFromPathPtr)GetProcAddress(hmodS32, (LPCSTR)ILCreateFromPathORD );
        s_pfnILFree = (ILFreePtr)GetProcAddress(hmodS32, (LPCSTR)ILFreeORD );

        if ( s_pfnILCreate != NULL  && s_pfnILFree != NULL ) {
            // We need to initialize our static know directories table.
            // To be safe, we'll grab a mutex while we do the dirty deed.
            {   // constrain the scope of the mutex
                CLock lck(g_mxsCDLGetLongPathNameGlobals);

                if ( !s_kdMap.m_bInited ) {
                     TCHAR szWinDir[MAX_PATH];
                    int i;

                    GetWindowsDirectory( szWinDir, MAX_PATH );
                    for ( i = 0; i < cKnownDirs; i++ ) {
                        lstrcpy( s_kdMap.m_aKDMap[i].szCanonical, szWinDir );
                        lstrcat( s_kdMap.m_aKDMap[i].szCanonical, s_kdMap.m_aszCaches[i] );
                        s_kdMap.m_aKDMap[i].cchCanonical = lstrlen( s_kdMap.m_aKDMap[i].szCanonical );
                        GetShortPathName( s_kdMap.m_aKDMap[i].szCanonical,
                                          s_kdMap.m_aKDMap[i].szShort, MAX_PATH );
                        s_kdMap.m_aKDMap[i].cchShort = lstrlen( s_kdMap.m_aKDMap[i].szShort );
                    }
                    s_kdMap.m_bInited = TRUE;
                }
            }

            TCHAR *pch;
            TCHAR *szT = new CHAR[MAX_PATH];

            if ( szShort != NULL && szT != NULL && s_kdMap.m_bInited ) {
                LPITEMIDLIST pidl = NULL;

                // Okay, kids, now this gets fun.
                // If we're on Memphis or NT5, we can simply call GetLongPathName
                // to get the canonical format.
                // If not, we've a bit more work ahead of us.
                // If the path is not down into one of the dowload cache folders,
                // we can use shell32 functions to create the long path. This involves
                // generating a pidl for the file, then converting the pidl back into a path.
                // If the path goes down into one of the cache directories, we're in
                // a bit of trouble, because OC cache does not implement
                // IShellFolder::ParseDisplayName, so we cannot generate a proper pidl.
                // In that case, we use prefab paths to these known directories
                // and tack on the long name of the file.
#ifdef UNICODE
                // NOTE: the prototypes in Winbase.h are deceiving - look at the argument types,
                // NOT at the argument names, the output parameter is second!
                if ( s_pfnGetLongPathNameW ) {
                    hr = (s_pfnGetLongPathNameW( szShort, szLong, MAX_PATH ) != 0)? S_OK : E_FAIL;
#else
                if ( s_pfnGetLongPathNameA ) {
                    hr = (s_pfnGetLongPathNameA( szShort, szLong, MAX_PATH ) != 0)? S_OK : E_FAIL;
#endif
                } else {
                    LPTSTR szFileName;
                    if ( GetFullPathName( szShort, MAX_PATH, szT, &szFileName ) ) {
                        int iKnownDir = s_kdMap.IndexKnownDirs( szT );
                        if ( iKnownDir >= 0 ) {
                            WIN32_FIND_DATA wfd;
                            int cchBase = lstrlen(szT) - lstrlen(szFileName);
                    
                            // okay, it's in one of our caches
                            if ( !IsCanonicalName( szFileName ) ) {
                                HANDLE hfind = FindFirstFile( szShort, &wfd );
                                if ( hfind != INVALID_HANDLE_VALUE ) {
                                    szFileName = wfd.cFileName;
                                } else
                                    szFileName = NULL;
                            }

                            if ( szFileName != NULL  ) {
                                lstrcpy( szLong, s_kdMap.m_aKDMap[iKnownDir].szCanonical );
                                // chop off szT right before the file name.
                                // if this is longer than the known dir name, we have a
                                // conflict.* subdirectory and must add this before the file
                                if ( cchBase != s_kdMap.m_aKDMap[iKnownDir].cchShort &&
                                     cchBase != s_kdMap.m_aKDMap[iKnownDir].cchCanonical ) {
                                    LPTSTR szConflict;
                                    CHAR chT = szT[cchBase];
                                    szT[cchBase] = '\0';
                                    // search back from before the file name.
                                    for ( szConflict = &szT[cchBase - 2];
                                          *szConflict != '\\'; szConflict-- );
                                    szConflict++; // we already have a '\'
                                    lstrcat( szLong, szConflict );
                                    szT[cchBase] = chT;
                                }
                                lstrcat( szLong, szFileName );
                                hr = S_OK;
                            }

                        } else {
        #ifndef UNICODE
                            WCHAR *szwT = new WCHAR[MAX_PATH];
                            if ( szwT && 
                                 MultiByteToWideChar( CP_ACP, 0, szShort, -1, szwT, MAX_PATH ) )
                                pidl = s_pfnILCreate( szwT );
                            delete szwT;
        #else
                            pidl = s_pfnILCreate( szShort );
        #endif
                            if ( pidl != NULL ) {
                                // Now we get the shell to turn the item id list back into 
                                // a path rich in long file names, which is our canonical form
                                if ( SHGetPathFromIDList( pidl, szT ) ) {
                                    if ( szLong != NULL ) {
#ifdef UNICODE
                                        LPWSTR szLongW;
                                        hr = Ansi2Unicode( szT, &szLongW );
                                        if ( SUCCEEDED(hr) ) {
                                            StrCpyNW( szLong, szLongW, cchBuffer );
                                            delete szLongW;
                                        }
#else
                                        //BUFFER OVERRUN lstrcpyA( szLong, szT );
                                        StrCpyN( szLong, szT, cchBuffer );
                                        hr = S_OK;
#endif
                                    }
                                }
                                s_pfnILFree( pidl );
                            } // if we got the pidl
                        } // else we're getting shell32 to do the dirty-work
                    } // if we got the full path/file name
                } // else we can't use GetLongPathName

                delete szT;
            } // if we can get out temp string
        } 
        FreeLibrary( hmodS32 );
        FreeLibrary( hmodK32 );
    }

    return ((hr==S_OK)? lstrlen(szLong) : 0);
}

DWORD WINAPI CDLGetLongPathNameA( LPSTR szLong, LPCSTR szShort, DWORD cchBuffer)
{
#ifndef UNICODE
    return s_CDLGetLongPathName( szLong, szShort, cchBuffer );
#else
    HRESULT hr;
    LPWSTR szShortW;
    TCHAR  szLongT[MAX_PATH];

    hr = Ansi2Unicode( szShort, &szShortW );
    if ( SUCCEEDED(hr) ) {
        hr = s_CDLGetLongPathName( szLongT, szShortW, MAX_PATH );
        if ( SUCCEEDED(hr) ) {
            LPSTR szLongA;
            hr = Unicode2Ansi( szLongT, &szLongA );
            if ( SUCCEEDED(hr) ) {
                lstrcpynA( szLong, szLongA, cchBuffer );
                delete szLongA;
            }
        }
        delete szShortW;
    }
    return hr;
#endif
}

DWORD WINAPI CDLGetLongPathNameW( LPWSTR szLong, LPCWSTR szShort, DWORD cchBuffer)
{
#ifdef UNICODE
    return s_CDLGetLongPathName( szLong, szShort, cchBuffer );
#else
    HRESULT hr;
    LPSTR szShortA;
    TCHAR  szLongT[MAX_PATH];

    hr = Unicode2Ansi( szShort, &szShortA );
    if ( SUCCEEDED(hr) ) {
        hr = s_CDLGetLongPathName( szLongT, szShortA, cchBuffer );
        if ( SUCCEEDED(hr) ) {
            LPWSTR szLongW;
            hr = Ansi2Unicode( szLongT, &szLongW );
            if ( SUCCEEDED(hr) ) {
                StrCpyNW( szLong, szLongW, cchBuffer );
                delete szLongW;
            }
        }
        delete szShortA;
    }
    return hr;
#endif
}

