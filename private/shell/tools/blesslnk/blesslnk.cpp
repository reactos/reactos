// blesslnk.cpp : Program to bless a LNK file with darwin descriptor
// or logo3 app name/id

#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <shlobj.h>
#include <shlguidp.h>
#include <shlwapi.h>
#include "cdfsubs.hpp"
#include "resource.h"

#define FAIL_ARGS       1
#define FAIL_OLE        2
#define FAIL_LOAD       3
#define FAIL_ENTRY      4
#define FAIL_REG        5
#define FAIL_SHELL      6

#define DARWIN_ID       0x1
#define LOGO3_ID        0x2

// The following strings are used to support the shell link set path feature that
// allows us to bless links for Darwin or Logo3 publising support in IE4

#define DARWINGUID_TAG TEXT("::{9db1186e-40df-11d1-aa8c-00c04fb67863}:")
#define DARWIN_TAG_LEN (ARRAYSIZE(DARWINGUID_TAG)-1)

#define LOGO3GUID_TAG  TEXT("::{9db1186f-40df-11d1-aa8c-00c04fb67863}:")
#define LOGO3_TAG_LEN  (ARRAYSIZE(LOGO3GUID_TAG)-1)


// checking for shell32 4.72.2106.0 or greater

#define IE401_SHELL_MAJOR               0x0004
#define IE401_SHELL_MINOR               0x48
#define IE401_SHELL_BUILD               0x083a

BOOL IE401ShellAvailable()
{
        BOOL bCanBlessLink = FALSE;
        DLLGETVERSIONPROC lpfnVersionProc = NULL;
        DLLVERSIONINFO dlinfo;

        HMODULE hMod = LoadLibrary("shell32.dll");

        if (hMod) {

                if ( (lpfnVersionProc = (DLLGETVERSIONPROC)GetProcAddress(hMod,"DllGetVersion")) ) 
                {

                        dlinfo.cbSize = sizeof(DLLVERSIONINFO);

                        if ( (lpfnVersionProc(&dlinfo) == S_OK) &&

                            (dlinfo.dwMajorVersion > IE401_SHELL_MAJOR) ||
                                ((dlinfo.dwMajorVersion == IE401_SHELL_MAJOR) &&
                                 (dlinfo.dwMinorVersion >= IE401_SHELL_MINOR)
                                ))

                        {
                                bCanBlessLink = TRUE;
                                if (dlinfo.dwMajorVersion == IE401_SHELL_MAJOR &&
                                    dlinfo.dwMinorVersion == IE401_SHELL_MINOR &&
                                    dlinfo.dwBuildNumber < IE401_SHELL_BUILD)
                                {
                                        bCanBlessLink = FALSE;
                                }
                                        
                        }
                }
        }


        if (hMod)
                FreeLibrary(hMod);

        return bCanBlessLink;
}

HRESULT SetLnkBlessing( IShellLink *pishl, DWORD dwSig, LPSTR szBlessing )
{
    HRESULT hr = S_OK;
    char szPath[MAX_PATH*4];
        char szTarget[MAX_PATH];

        if (dwSig == DARWIN_ID) {
                lstrcpy(szPath, DARWINGUID_TAG);
        } else if (dwSig == LOGO3_ID) {
                lstrcpy(szPath, LOGO3GUID_TAG);
        }else {
                hr = E_INVALIDARG;
        }

        if (SUCCEEDED(hr)) {

                lstrcat(szPath, szBlessing);

                lstrcat(szPath, "::");

                // get real target

                hr = pishl->GetPath(szTarget, MAX_PATH, NULL,0);

                // copy real target name

                if (SUCCEEDED(hr)) {
                        lstrcat(szPath, szTarget);

                        hr = pishl->SetPath( szPath );
                }
        }


    return hr;
}

int __cdecl main(int argc, char * argv[])
{
    int             iReturn = 0;
    HRESULT         hr = S_OK;
    LPSTR           pszLnkName = NULL;
    LPSTR           pszLogo3ID = NULL;
    LPSTR           pszDarwinID = NULL;
    LPSTR           pszCDFURL = NULL;
    LPSTR           pszTok;
    IPersistFile    *pipfLnk = NULL;


    // Parse command line arguments.
    int iTok;
    for (iTok = 1; iTok < argc; iTok++)
    {                                
            pszTok = argv[iTok];
            
            if ((pszTok[0] == '-') || (pszTok[0] == '/'))
            {
                    switch (pszTok[1])
                    {
                    case 'c':
                    case 'C':
                            pszCDFURL = argv[iTok+1];
                            iTok++;
                            break;

                    case 'l':
                    case 'L':
                            pszLogo3ID = argv[iTok+1];
                            iTok++;
                            break;
                                                            
                    case 'd':
                    case 'D':
                            pszDarwinID = argv[iTok+1];
                            iTok++;
                            break;
                            
                    case '?':
                            fprintf(stderr, "\nUsage: blesslnk [/l Logo3-ID] [/d Darwin-ID] lnkname\n/l - bless for Logo3 Application Channel notifcation.\n/d - bless for Darwin\n" );
                            break;

                    default:
                            fprintf(stderr, "err - unrecognized flag: %s\n", pszTok);
                            return FAIL_ARGS;
                    }
            }
            else
            {
                    if (pszLnkName == NULL)
                    {
                            pszLnkName = pszTok;
                            break;
                    }
                    else
                    {
                            fprintf(stderr, "err - extra argument: %s\n", pszTok);
                            return FAIL_ARGS;
                    }
            }
    }

    if (pszLnkName == NULL)
    {
            fprintf(stderr, "err - no lnk file specified\n" );
            return FAIL_ARGS;
    }

    if (!IE401ShellAvailable()) 
    {
            fprintf(stderr, "err - Need to have IE401 shell enabled for this feature.\n" );
            return FAIL_SHELL;
    }



    // Initialize OLE.                              
    if (FAILED(CoInitialize(NULL)))
    {
            return FAIL_OLE;
    }

    if ( SUCCEEDED(hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFile, (LPVOID*)&pipfLnk)) )
    {
        WCHAR szwLnkName[MAX_PATH];

        MultiByteToWideChar( CP_ACP, 0, pszLnkName, -1, szwLnkName, MAX_PATH );

        if ( SUCCEEDED(hr = pipfLnk->Load(szwLnkName, STGM_READ)) )
        {

            IShellLink *pishl = NULL;

            if ( SUCCEEDED(hr = pipfLnk->QueryInterface(IID_IShellLink, (LPVOID*)&pishl)) )
                        {
                    if ( pszLogo3ID )
                    {
                        if ( FAILED(hr = SetLnkBlessing( pishl, LOGO3_ID, pszLogo3ID )) )
                            fprintf( stderr, "err - failed to bless %s with Logo3-ID %s\n", pszLnkName, pszLogo3ID );
                    }

                    if ( pszDarwinID )
                    {
                        if ( FAILED(hr = SetLnkBlessing(pishl, DARWIN_ID, pszDarwinID)) )
                            fprintf( stderr, "err - failed to bless %s with Darwin-ID %s\n", pszLnkName, pszDarwinID );
                                        }

                        }

                        if ( SUCCEEDED(hr) )
                                hr = pipfLnk->Save( NULL, FALSE );

                        if ( FAILED(hr) )
                                fprintf( stderr, "err - failed with error %lx\n", hr );

        }

        pipfLnk->Release();
    }

    if (pszCDFURL && SUCCEEDED(hr)) {

        // BUGBUG: pszLogo3ID is passed instead of a friendly name. In
        // the future, we can add another switch to blesslnk to accomodate
        // this. Also, we must subscribe with UI (non-silent mode) because
        // subscribing will not work properly otherwise.

        SubscribeChannel(NULL, pszLogo3ID, pszCDFURL, FALSE);
    }
        

    CoUninitialize();
            
    return iReturn;
}                       
