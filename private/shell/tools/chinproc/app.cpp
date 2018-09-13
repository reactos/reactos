
#include "priv.h"       
#include <stdlib.h>
#include <stdio.h>

HINSTANCE g_hinst;

#define APP_VERSION         "Version 0.2"

#define PFF_SYSTEM      0x00000001


void PrintSyntax(void)
{
    fprintf(stderr, "chinproc.exe  " APP_VERSION "\n\n");
    fprintf(stderr, "Changes the InProcServer32 entry for any CLSID using\n");
    fprintf(stderr, "<dll> from %%SystemRoot%%\\system32 to a local path,\n");
    fprintf(stderr, "or vice versa.\n");
    fprintf(stderr, "Syntax:  chinproc [-s] {<path>|<dll>} \n\n");
    fprintf(stderr, "          -s   Change to %%SystemRoot%%\\system32\\foo.dll\n");
    fprintf(stderr, "               (NT only)\n\n");
    fprintf(stderr, "          Default action is to set the InProcServer32 to <path>\n");
}    


/*----------------------------------------------------------
Purpose: Worker function to do the work

Returns: 
Cond:    --
*/
int
DoWork(int cArgs, char * rgszArgs[])
{
    LPSTR psz;
    LPSTR pszDll = NULL;
    DWORD dwFlags = 0;
    int i;
    int nRet = 0;

    // (The first arg is actually the exe.  Skip that.)

    for (i = 1; i < cArgs; i++)
    {
        psz = rgszArgs[i];

        // Check for options
        if ('/' == *psz || '-' == *psz)
        {
            psz++;
            switch (*psz)
            {
            case '?':
                // Help
                PrintSyntax();
                return 0;

            case 's':
                dwFlags |= PFF_SYSTEM;
                *psz++;

                // Is this Win95?
                if (0x80000000 & GetVersion())
                {
                    // Yes; can't allow -s
                    fprintf(stderr, "Cannot use -s on Win95 machines.\n");
                    return -1;
                }
                break;

            default:
                // unknown
                fprintf(stderr, "Invalid option -%c\n", *psz);
                return -1;
            }
        }
        else if (!pszDll)
            pszDll = rgszArgs[i];
        else
        {
            fprintf(stderr, "Ignoring invalid parameter - %s\n", rgszArgs[i]);
        }
    }

    if (!pszDll)
    {
        PrintSyntax();
        return -2;
    }

    // Enumerate the HKCR\CLSID for any CLSIDs that use pszDll.
    DWORD dwRet;
    HKEY hkeyCLSID;

    dwRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0, KEY_SET_VALUE | KEY_READ, &hkeyCLSID);
    if (NO_ERROR == dwRet)
    {
        DWORD dwIndex = 0;
        TCHAR szSubkey[MAX_PATH];

        while (ERROR_SUCCESS == RegEnumKey(hkeyCLSID, dwIndex++, szSubkey, SIZECHARS(szSubkey)))
        {
            TCHAR szPath[MAX_PATH];
            DWORD cbPath = sizeof(szPath);
            LPTSTR pszFile = PathFindFileName(pszDll);

            // Does this entry match pszDll?
            PathAppend(szSubkey, TEXT("InProcServer32"));
            if (NO_ERROR == SHGetValue(hkeyCLSID, szSubkey, NULL, NULL, szPath, &cbPath) &&
                0 == lstrcmpi(PathFindFileName(szPath), pszFile))
            {
                // Yes; change it according to dwFlags
                fprintf(stdout, ".");

                if (dwFlags & PFF_SYSTEM)
                {
                    // Prepend %SystemRoot%\system32 on it
                    lstrcpy(szPath, TEXT("%SystemRoot%\\system32"));
                    PathAppend(szPath, pszDll);
                    SHSetValue(hkeyCLSID, szSubkey, NULL, REG_EXPAND_SZ, szPath, sizeof(szPath));
                }
                else
                {
                    SHSetValue(hkeyCLSID, szSubkey, NULL, REG_SZ, pszDll, CbFromCch(lstrlen(pszDll) + 1));
                }
            }
        }

        fprintf(stdout, "\nFinished!\n");
        RegCloseKey(hkeyCLSID);
    }
    else
        fprintf(stderr, "Failed to open HKCR\\CLSID\n");

    return nRet;
}


int __cdecl main(int argc, char * argv[])
{
    return DoWork(argc, argv);
}    

