#undef UNICODE					// ## Not Yet
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>

#define MS_RSA_TYPE     "RSA Full (Signature and Key Exchange)"

typedef struct _PROVENTRY
{
    LPTSTR szProvider;
    DWORD  dwType;
    LPTSTR szType;
    LPTSTR szImagePath;
    LPTSTR szSigPath;
} PROVENTRY, *PPROVENTRY;

PROVENTRY provList[] =
{
    {
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\Microsoft Base Cryptographic Provider v1.0",
        01,
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type 001",
        "rsabase.dll",
        "rsabase.sig"
    },

    {
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\Microsoft Default Provider",
        99,
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type 099",
        "defprov.dll",
        "sign"
    },
    
    {
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\CSP Provider",
        20,
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type 020",
        "csp.dll",
        "cspsign"
    },

    {
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\Microsoft Enhanced Cryptographic Provider v1.0",
        01,
        "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider Types\\Type 001",
        "rsaenh.dll",
        "rsaenh.sig"
    },

};




DWORD     	dwIgn;
HKEY      	hKey;
DWORD           err;
DWORD           dwValue;
HANDLE          hFileSig;
DWORD     	NumBytesRead;
DWORD           lpdwFileSizeHigh;
LPVOID          lpvAddress;    
DWORD           NumBytes;

int __cdecl main(int cArg, char *rgszArg[])
{

    int iLoopCount;
    for (iLoopCount=0; iLoopCount<( sizeof(provList)/sizeof(PROVENTRY)); iLoopCount++)
    {
        //
        // Just open signature file.  This file was created by sign.exe.
        //
        if ((hFileSig = CreateFile(provList[iLoopCount].szSigPath,
                                   GENERIC_READ, 0, NULL,
			           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
			           0)) != INVALID_HANDLE_VALUE)
        {
            if ((NumBytes = GetFileSize((HANDLE) hFileSig, &lpdwFileSizeHigh)) ==
                                        0xffffffff)
            {
                printf("Install failed: Getting size of file %s: %x\n",
                        provList[iLoopCount].szSigPath, GetLastError());
                CloseHandle(hFileSig);
                return(FALSE);
            }

            if ((lpvAddress = VirtualAlloc(NULL, NumBytes, MEM_RESERVE |
		                                           MEM_COMMIT,
                                           PAGE_READWRITE)) == NULL)
            {
                CloseHandle(hFileSig);
                printf("Install failed: Alloc to read %s: %x\n",
                        provList[iLoopCount].szSigPath, GetLastError());
                return(FALSE);
            }

            if (!ReadFile((HANDLE) hFileSig, lpvAddress, NumBytes,
		          &NumBytesRead, 0))
            {

                CloseHandle(hFileSig);
                printf("Install failed: Reading %s: %x\n",
                        provList[iLoopCount].szSigPath, GetLastError());
                VirtualFree(lpvAddress, 0, MEM_RELEASE);
                return(FALSE);
            }

            CloseHandle(hFileSig);

            if (NumBytesRead != NumBytes)
            {
                printf("Install failed: Bytes read doesn't match file size\n");
                return(FALSE);
            }

	    //
	    // Create or open in local machine for provider: 
	    //
            if ((err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                      (const char *) provList[iLoopCount].szProvider,
                                      0L, "", REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS, NULL, &hKey,
                                      &dwIgn)) != ERROR_SUCCESS)
            {
                printf("Install failed: RegCreateKeyEx\n");
            }

	    //
	    // Set Image path to dll
	    //
            if ((err = RegSetValueEx(hKey, "Image Path", 0L, REG_SZ, provList[iLoopCount].szImagePath,
	                             strlen(provList[iLoopCount].szImagePath)+1)) != ERROR_SUCCESS)
            {
                printf("Install failed: Setting Image Path value\n");
                return(FALSE);
            }

	    //
	    // Set Type 
	    //
            dwValue = provList[iLoopCount].dwType;
            if ((err = RegSetValueEx(hKey, "Type", 0L, REG_DWORD,
                                     (LPTSTR) &dwValue,
                                     sizeof(DWORD))) != ERROR_SUCCESS)
            {
                printf("Install failed: Setting Type value: %x\n", err);
                return(FALSE);
            }

	    //
	    // Place signature
	    //
            if ((err = RegSetValueEx(hKey, "Signature", 0L, REG_BINARY, 
                                     (LPTSTR) lpvAddress,
                                     NumBytes)) != ERROR_SUCCESS)
            {
                printf("Install failed: Setting Signature value for %s: %x\n", provList[iLoopCount].szSigPath, err);
                return(FALSE);
            }

            RegCloseKey(hKey);
            VirtualFree(lpvAddress, 0, MEM_RELEASE);

	    //
	    // Create or open in local machine for provider type: Type
	    //
            if ((err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                      (const char *) provList[iLoopCount].szType,
                                      0L, "", REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS, NULL, &hKey,
                                      &dwIgn)) != ERROR_SUCCESS)
            {
                printf("Install failed: Registry entry existed: %x\n", err);
            }

            // always set MS_DEF_PROV to be the default provider
            if ((err = RegSetValueEx(hKey, "Name", 0L, REG_SZ, MS_DEF_PROV,
                                     strlen(MS_DEF_PROV)+1)) != ERROR_SUCCESS)
            {
                printf("Install failed: Setting Default type: %x\n", err);
                return(FALSE);
            }

            if ((err = RegSetValueEx(hKey, "TypeName", 0L, REG_SZ, MS_RSA_TYPE,
                                     strlen(MS_RSA_TYPE)+1)) != ERROR_SUCCESS)
            {
                printf("Install failed: Setting type name: %x\n", err);
                return(FALSE);
            }

	    printf("Installed: %s\n", provList[iLoopCount].szImagePath);

        }

    }   // loop through all provs we know about

}
