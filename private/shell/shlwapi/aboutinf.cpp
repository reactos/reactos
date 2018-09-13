#include <string.h>
#include <ieverp.h>
#include "priv.h"
#include "ids.h"

#define SECURITY_WIN32
#include <schnlsp.h> //for UNISP_NAME_A
#include <sspi.h> //for SCHANNEL.dll api -- to obtain encryption key size
#include <issperr.h> //error codes

#include <mluisupp.h>
#include <wininet.h>    // INTERNET_MAX_URL_LENGTH

#define MAX_REG_VALUE   256

// Some Static strings that we use to read from the registry

#ifdef UNIX
#define VERSION         "IEUNIX Version"
#define UNIX_IE_PRODUCT_ID TEXT("86139-999-2001594-12504")
#ifndef ux10
#define UNIX_IE_PRODUCT_FILE TEXT("%MWDEV%/ie/setup/sunos5/.iepid")
#else
#define UNIX_IE_PRODUCT_FILE TEXT("%MWDEV%/ie/setup/ux10/.iepid")
#endif
#endif

typedef PSecurityFunctionTable (APIENTRY *INITSECURITYINTERFACE_FN_A) (VOID);

// Returns the maximum cipher strength
DWORD GetCipherStrength()
{
    DWORD                           dwKeySize = 0;
    HINSTANCE                       hSecurity;
    INITSECURITYINTERFACE_FN_A      pfnInitSecurityInterfaceA;
    PSecurityFunctionTable          pSecFuncTable;

    //
    // Can't go directly to schannel on NT5.  (Note that g_bRunningOnNT5OrHigher
    // may not be initialized when fUseSChannel is initialized!)
    //
    static BOOL fUseSChannel = TRUE;
    if (fUseSChannel && !g_bRunningOnNT5OrHigher)
    {
        //
        // This is better for performance. Rather than call through
        // SSPI, we go right to the DLL doing the work.
        //
        hSecurity = LoadLibrary("schannel");
    }
    else
    {
        //
        // Use SSPI
        //
        if (g_bRunningOnNT)
        {
            hSecurity = LoadLibrary("security");
        }
        else
        {
            hSecurity = LoadLibrary("secur32");
        }
    }

    if (hSecurity == NULL)
    {
        return 0;
    }

    //
    // Get the SSPI dispatch table
    //
    pfnInitSecurityInterfaceA =
        (INITSECURITYINTERFACE_FN_A)GetProcAddress(hSecurity, "InitSecurityInterfaceA");

    if (pfnInitSecurityInterfaceA == NULL)
    {
        goto exit;
    }

    pSecFuncTable = (PSecurityFunctionTable)((*pfnInitSecurityInterfaceA)());
    if (pSecFuncTable == NULL)
    {
        goto exit;
    }

    if (pSecFuncTable->AcquireCredentialsHandleA && pSecFuncTable->QueryCredentialsAttributesA)
    {
        TimeStamp  tsExpiry;
        CredHandle chCred;
        SecPkgCred_CipherStrengths cs;

        if (SEC_E_OK == (*pSecFuncTable->AcquireCredentialsHandleA)(NULL,  
                          UNISP_NAME_A, // Package
                          SECPKG_CRED_OUTBOUND,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &chCred,      // Handle
                          &tsExpiry ))
        {
            if (SEC_E_OK == (*pSecFuncTable->QueryCredentialsAttributesA)(&chCred, SECPKG_ATTR_CIPHER_STRENGTHS, &cs))
            {
                dwKeySize = cs.dwMaximumCipherStrength;
            }

            // Free the handle if we can
            if (pSecFuncTable->FreeCredentialsHandle)
            {
                (*pSecFuncTable->FreeCredentialsHandle)(&chCred);
            }
        }
    }

exit:
    FreeLibrary(hSecurity);

    if (dwKeySize == 0 && fUseSChannel)
    {
        // Failed, so retry using SSPI
        fUseSChannel = FALSE;
        dwKeySize = GetCipherStrength();
    }
    return dwKeySize;
}


BOOL SHAboutInfoA(LPSTR lpszInfo, DWORD cchSize)
{
    HKEY        hkey;
    char        szVersion[30];
    char        szUserName[MAX_REG_VALUE];
    char        szCompanyName[MAX_REG_VALUE];
    char        szKeySize[11];
    char        szProductId[MAX_REG_VALUE];
    char        szUpdateUrl[INTERNET_MAX_URL_LENGTH];
    char        szIEAKStr[MAX_REG_VALUE];
    LPSTR       lpszAboutKey;
    DWORD       dwKeySize = 0;
    DWORD       cb;
    DWORD       dwType;
	BOOL        fIEOrShell = TRUE;

    lpszInfo[0]    = '\0';
    szKeySize[0]   = '\0';

#ifndef UNIX
	// Are we in the explorer or IE process?
	fIEOrShell = GetModuleHandle("EXPLORER.EXE") || GetModuleHandle("IEXPLORE.EXE");
	
    if (g_bRunningOnNT)
        lpszAboutKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    else
#endif
        lpszAboutKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszAboutKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        // get the encription key size
        dwKeySize = GetCipherStrength();
        wsprintf(szKeySize, "~%d", dwKeySize);

        // get the custom IEAK update url 
        // (always get from Windows\CurrentVersion because IEAK policy file must be platform
        // independent

        cb = sizeof(szUpdateUrl);
        if(SHGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 
            "IEAKUpdateUrl", &dwType, (LPBYTE)szUpdateUrl, &cb) != ERROR_SUCCESS)
            szUpdateUrl[0] = '\0';

#ifndef UNIX
        // get the Version number (version string is in the following format 5.00.xxxx.x)
        szVersion[0] = '\0';
        cb = ARRAYSIZE(szVersion);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SOFTWARE\\Microsoft\\Internet Explorer"),
                                        TEXT("Version"), &dwType, (LPVOID)szVersion, &cb))
        {
            DWORD dwLen;

            // added by pritobla on 9/1/98
            // CustomizedVersion contains a 2-letter code that identifies what mode was used
            // (CORP, ICP, ISP, etc.) in building this version IE using the IEAK.
            dwLen = lstrlen(szVersion);
            cb = ARRAYSIZE(szVersion) - dwLen;
            SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Internet Explorer"),
                                           TEXT("CustomizedVersion"), &dwType, (LPVOID)&szVersion[dwLen], &cb);
        }
#else
        // Get the version details from ieverp.h
        sprintf(szVersion, "%s.%04d.%04d",VER_PRODUCTVERSION_STRING,
                                      VER_PRODUCTBUILD,
                                      VER_PRODUCTBUILD_QFE);
#endif // UNIX

		if (!fIEOrShell)
		{
			// Not in the explorer or iexplore process so we are doing some side by side stuff so
			// reflect this in the version string. Maybe we should get the version out of MSHTML
			// but not sure since this still doesn't reflect IE4 or IE5 properly anyway.
			MLLoadString(IDS_SIDEBYSIDE, szVersion, ARRAYSIZE(szVersion));
		}
		
        // get the custom IEAK branded help string
        cb = sizeof(szIEAKStr);
        if(RegQueryValueExA(hkey, "IEAKHelpString", 0, &dwType, (LPBYTE)szIEAKStr, &cb) != ERROR_SUCCESS)
            szIEAKStr[0] = '\0';

        // get the User name.
        cb = sizeof(szUserName);
        if(RegQueryValueExA(hkey, "RegisteredOwner", 0, &dwType, (LPBYTE)szUserName, &cb) != ERROR_SUCCESS)
            szUserName[0] = '\0';

        // get the Organization name.
        cb = sizeof(szCompanyName);
        if(RegQueryValueExA(hkey, "RegisteredOrganization", 0, &dwType, (LPBYTE)szCompanyName, &cb) != ERROR_SUCCESS)
            szCompanyName[0] = '\0';

#ifndef UNIX
        cb = sizeof(szProductId);
        if (SHGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Internet Explorer\\Registration", "ProductId", &dwType, (LPVOID)szProductId, &cb) != ERROR_SUCCESS)
        {
            szProductId[0] = '\0';
        }
#else
        HANDLE hPidFile;
        char szPidFileName[MAX_PATH];
        DWORD dwRead;
        SHExpandEnvironmentStrings(UNIX_IE_PRODUCT_FILE, szPidFileName, MAX_PATH);
        if ((hPidFile = CreateFileA(szPidFileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) 
            == INVALID_HANDLE_VALUE)
        {
            sprintf(szProductId, "%s", UNIX_IE_PRODUCT_ID);
        }
        else
        {
            ReadFile(hPidFile, (LPVOID)szProductId, 23, &dwRead, NULL);
            szProductId[dwRead] = 0;
        }

        CloseHandle(hPidFile);
#endif
        lstrcatn(lpszInfo, szVersion, cchSize);
        lstrcatn(lpszInfo, "~", cchSize);
        lstrcatn(lpszInfo, szUserName, cchSize);
        lstrcatn(lpszInfo, "~", cchSize);
        lstrcatn(lpszInfo, szCompanyName, cchSize);
        lstrcatn(lpszInfo, szKeySize, cchSize);
        lstrcatn(lpszInfo, "~", cchSize);
        lstrcatn(lpszInfo, szProductId, cchSize);
        lstrcatn(lpszInfo, "~", cchSize);
        lstrcatn(lpszInfo, szUpdateUrl, cchSize);
        lstrcatn(lpszInfo, "~", cchSize);
        lstrcatn(lpszInfo, szIEAKStr, cchSize);

        RegCloseKey(hkey);
    }
    else
        return FALSE;

    return TRUE;
}

BOOL SHAboutInfoW(LPWSTR lpszInfo, DWORD cchSize)
{
    LPSTR   lpszTmp;
    BOOL    bRet = FALSE;

    lpszInfo[0] = L'\0';

    if(NULL != (lpszTmp = (LPSTR)LocalAlloc(LPTR, cchSize)))
        if(SHAboutInfoA(lpszTmp, cchSize))
            if(MultiByteToWideChar(CP_ACP, 0, lpszTmp, -1, lpszInfo, cchSize) != 0)
                bRet = TRUE;

    if(lpszTmp)
        LocalFree(lpszTmp);

    return bRet;
}
