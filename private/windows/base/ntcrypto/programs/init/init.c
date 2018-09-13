#undef UNICODE					// ## Not Yet
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>

CHAR			pszMyName[64];
HCRYPTKEY		hClientKey;
HCRYPTPROV		hMe;
HCRYPTKEY		hKey2;

BOOL Logon(int cArg);

int __cdecl main(int cArg, char *rgszArg[])
{

        // Logon to provider
	if (!Logon(cArg))
	    goto exit;

exit:
    return(0);

}

BOOL Logon(int cArg)
{
	HCRYPTKEY	hTestKey;

	pszMyName[0] = 0;

	if (RCRYPT_FAILED(CryptAcquireContext(&hMe, pszMyName, MS_DEF_PROV,
                          PROV_RSA_FULL, 0)))
	{
            if (cArg > 1)
		printf("\nUser doesn't exists, try to create it	");

	    if (RCRYPT_FAILED(CryptAcquireContext(&hMe, pszMyName,
                              MS_DEF_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET)))
            {
	        if (cArg > 1)
                    printf("FAIL Error = %x\n", GetLastError());
                return(FALSE);
	    }
	    else
	    {
	    if (cArg > 1)
                printf("SUCCEED\n");
            }
        }
	else
        {
            if (cArg > 1)
		printf("CryptAcquireContext for user: %s\n", pszMyName);
        }

	
	if (RCRYPT_FAILED(CryptGetUserKey(hMe, AT_SIGNATURE, &hTestKey)))
	{

            if (cArg > 1)
                printf("Create signature key for %s:	", pszMyName);
    
	    if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RSA_SIGN,
                                          CRYPT_EXPORTABLE, &hClientKey)))
	    {
	        if (cArg > 1)
                    printf("FAIL Error = %x\n", GetLastError());
	        return(FALSE);
	    }
	    if (cArg > 1)
                printf("SUCCEED\n");
        }
	else
		CryptDestroyKey(hTestKey);
		
	if (RCRYPT_FAILED(CryptGetUserKey(hMe, AT_KEYEXCHANGE, &hTestKey)))
	{
            if (cArg > 1 )
                printf("Create key exchange for %s:		", pszMyName);

	    if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RSA_KEYX,
                                          CRYPT_EXPORTABLE, &hKey2)))
	    {
	        if (cArg > 1)
                    printf("FAIL Error = %x\n", GetLastError());
	        return(FALSE);
	    }
            if (cArg > 1)
                printf("SUCCEED\n");
        }
	else
		CryptDestroyKey(hTestKey);

        if (cArg > 1)
            printf("Init completed\n");

	return(TRUE);
       
}

