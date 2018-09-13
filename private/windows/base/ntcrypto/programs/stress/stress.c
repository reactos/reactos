#undef UNICODE					// ## Not Yet
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>

#define NTAG_MULTIPHASE		0x80000000
#define DES_TEST                0x00000008

#define UTILITY_BUF_SIZE	1024
#define UTILITY_BUF_SIZE_2	1050
#define EXPORT_BUFFER_LEN	32768

#define MAXKEYLEN  128

HCRYPTPROV	hMe;
CHAR		pszMyName[64];
OFSTRUCT        ImageInfoBuf;
HFILE           hFile;
BYTE		buf[UTILITY_BUF_SIZE];
BYTE		buf2[UTILITY_BUF_SIZE];
DWORD		BLen, BLen2, i;
HCRYPTKEY	hKey = 0;
HCRYPTKEY	hKey2 = 0;
HCRYPTKEY	hMyPubKey;
HCRYPTKEY	hClientKey;
WORD            wRandomSize;
DWORD           dRandom;
BYTE            *pTmp;
DWORD           count;
BYTE            *bRanbuf;
BYTE            *bcryptbuf;
DWORD           i;
BYTE            pData[8];
BYTE            pHashData1[50];
DWORD           BHashLen1;
BYTE            pHashData2[50];
DWORD           BHashLen2;
BYTE            pHashSignData1[MAXKEYLEN];
DWORD           BHashSignLen1;
BYTE            pHashSignData2[MAXKEYLEN];
DWORD           BHashSignLen2;
HCRYPTHASH	hHash = 0;

BOOL Logon(int cArg);

BOOL TEncrypt(HCRYPTKEY hTKey, HCRYPTHASH hTHash, BOOL FinalFlag,
              DWORD dwFlags, BYTE *Tbuf, DWORD *pBLen, DWORD pdwBufLen,
              int cArg, CHAR *szAlgid, CHAR *szmode);
BOOL TDecrypt(HCRYPTKEY hTKey, HCRYPTHASH hTHash, BOOL FinalFlag,
              DWORD dwFlags, BYTE *Tbuf, DWORD *pBLen, int cArg,
              CHAR *szAlgid, CHAR *szmode);

BOOL TestRC2(int cArg, CHAR *bbuf, DWORD bsize);

BOOL TestRC4(int cArg, CHAR *bbuf, DWORD bsize);

#ifdef TEST_VERSION
BOOL TestDES(int cArg, CHAR *bbuf, DWORD bsize);
#endif

BOOL TestHash(int cArg, CHAR *bbuf, DWORD bsize);

BOOL TestExchange(int cArg, CHAR *bbuf, DWORD bsize);

BOOL Hash(int cArg, CHAR *bbuf, DWORD bsize, BYTE *pHashOut,
          DWORD *pHashLenOut,  BYTE *pSigData, DWORD *pdwSigLen, DWORD Algid);

int __cdecl main(int cArg, char *rgszArg[])
{

    // Make sure keys don't exist to start
    strcpy(pszMyName, "stress");
    CryptAcquireContext(&hMe, pszMyName, MS_DEF_PROV, PROV_RSA_FULL,
                        CRYPT_DELETEKEYSET);

    while (TRUE)
    {

        // Logon to provider
        if (Logon(cArg))
	    exit(0);

        pTmp = (BYTE *) &wRandomSize;

	while (1)
	{
            if (RCRYPT_FAILED(CryptGenRandom(hMe, 2, pTmp)))
            {
                if (cArg > 1)
	            printf("GenRandom failed = %x\n", GetLastError());
                else
                    printf("FAIL\n");
                return(TRUE);
	    }
	    if (wRandomSize != 0)
               break;
        }

	dRandom = (DWORD) (wRandomSize + (wRandomSize % 8));

	if ((bRanbuf = VirtualAlloc(0, dRandom, MEM_COMMIT |
                                      MEM_RESERVE, PAGE_READWRITE)) == 0)
        {
            if (cArg > 1)
	    {
	        printf("malloc failed = %x\n", GetLastError());
            }
	    else
                printf("malloc FAIL\n");
            return(TRUE);
        }

	if (RCRYPT_FAILED(CryptGenRandom(hMe, dRandom, bRanbuf)))
        {
            if (cArg > 1)
	        printf("GenRandom failed = %x\n", GetLastError());
	    else
                printf("FAIL\n");
            return(TRUE);
        }

	if ((bcryptbuf = VirtualAlloc(0, dRandom + 8, MEM_COMMIT |
                                      MEM_RESERVE, PAGE_READWRITE)) == 0)
	{
            if (cArg > 1)
	    {
	        printf("malloc failed = %x\n", GetLastError());
            }
	    else
                printf("malloc FAIL\n");
            return(TRUE);
        }

	if (cArg > 1)
        {
            printf("bytes generated			 %x\n", wRandomSize);
        }

        memcpy(bcryptbuf, bRanbuf, dRandom);

	if (TestRC2(cArg, bcryptbuf, dRandom))
            exit(0);

	if (TestRC4(cArg, bcryptbuf, dRandom))
            exit(0);

#ifdef TEST_VERSION
	if (TestDES(cArg, bcryptbuf, dRandom))
            exit(0);
#endif

	if (TestHash(cArg, bcryptbuf, dRandom))
            exit(0);

	if (TestExchange(cArg, bcryptbuf, dRandom))
		exit(0);

        if (VirtualFree(bRanbuf, 0, MEM_RELEASE) != TRUE)
        {
            if (cArg > 1)
	        printf("VirtulaFree failed: %x\n", GetLastError());
        }

	if (VirtualFree(bcryptbuf, 0, MEM_RELEASE) != TRUE)
        {
            if (cArg > 1)
	        printf("VirtulaFree failed: %x\n", GetLastError());
        }

        if (cArg > 1)
            printf("CryptReleaseContext		");

        if (RCRYPT_FAILED(CryptReleaseContext(hMe, 0)))
	{
	    printf("FAIL Error = %x\n", GetLastError());
            return(TRUE);
	}
	else
            if (cArg > 1)
                printf("SUCCEED\n");

        if (cArg > 1)
            printf("CryptAcquireContext Delete	");

        strcpy(pszMyName, "stress");
        if (RCRYPT_FAILED(CryptAcquireContext(&hMe, pszMyName,
                          MS_DEF_PROV, PROV_RSA_FULL, CRYPT_DELETEKEYSET)))
        {
	    if (cArg > 1)
                printf("FAIL Error = %x\n", GetLastError());
            return(FALSE);
	}
	else
	{
	    if (cArg > 1)
            {
               printf("SUCCEED\n");
               printf("\n");
            }
        }

    }

    exit(0);

}


BOOL TestExchange(int cArg, CHAR *bbuf, DWORD bsize)
{
	BYTE		ExpBuf[EXPORT_BUFFER_LEN];
	BYTE		SigBuf[UTILITY_BUF_SIZE];
	DWORD		ExpBufLen, SigBufLen;
	HCRYPTKEY	hKey2;
	HCRYPTHASH	hHash;
	PUBLICKEYSTRUC	*pPubKey;
	RSAPUBKEY	*pRSAKey;
	
//
// Generate a RC4 key
//
	
    if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RC4, CRYPT_EXPORTABLE, &hKey)))
    {
	if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptGenKey FAIL\n");
	return(TRUE);
    }

//
// Look for our own exchange key
//

    if (RCRYPT_FAILED(CryptGetUserKey(hMe, AT_KEYEXCHANGE, &hKey2)))
    {
	    if (cArg > 1)
		    printf("cguk failed = %x", GetLastError());
	    else
		    printf("CryptGetUserKey FAIL\n");
	    return (TRUE);
    }

//
// Export it in PUBLICKEYBLOB form
//
    if (cArg > 1)
	    printf("CryptExportKey (PKB)		");

    ExpBufLen = EXPORT_BUFFER_LEN;
    if (RCRYPT_FAILED(CryptExportKey(hKey2, 0, PUBLICKEYBLOB, 0,
				     ExpBuf, &ExpBufLen)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptExportKey FAIL\n");
	    return (TRUE);
    }

    
    if (cArg > 1)
	    printf("SUCCEED\n");

    CryptDestroyKey(hKey2);

    pPubKey = (PUBLICKEYSTRUC *)ExpBuf;
    pRSAKey = (RSAPUBKEY *)(ExpBuf + sizeof(PUBLICKEYSTRUC));

    if (pPubKey->aiKeyAlg != CALG_RSA_KEYX)
    {
	    printf("Pub key fails check\n");
	    return(TRUE);
    }
    
    if (pRSAKey->pubexp != 0x10001)
    {
	    printf("RSA key fails check\n");
	    return(TRUE);
    }
    
//
// Import it in PUBLICKEYBLOB form
//
    
    if (cArg > 1)
	    printf("CryptImportKey (PKB)		");

    if (RCRYPT_FAILED(CryptImportKey(hMe, ExpBuf, ExpBufLen, 0, 0, &hKey2)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptImportKey FAIL\n");
	    return (TRUE);
    }

    if (cArg > 1)
	    printf("SUCCEED\n");

//
// Encrypt and Decrypt
//
    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "RC4", ""))
    {
        return(TRUE);
    }

//
// Export the key in SIMPLEBLOB form
//
    
    if (cArg > 1)
	    printf("CryptExportKey			");

    ExpBufLen = EXPORT_BUFFER_LEN;
    
    if (RCRYPT_FAILED(CryptExportKey(hKey, hKey2, SIMPLEBLOB, 0,
				     ExpBuf, &ExpBufLen)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptExportKey FAIL\n");
	    return (TRUE);
    }    

    if (cArg > 1)
	    printf("SUCCEED\n");

//
// Nuke the old key
//
    
    if (RCRYPT_FAILED(CryptDestroyKey(hKey)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptDestroyKey(hKey2)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

//
// Sign the blob with the key exchange key
//

    if (cArg > 1)
	    printf("CryptCreateHash			");
    
    if (RCRYPT_FAILED(CryptCreateHash(hMe, CALG_SHA, 0, 0, &hHash)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptCreateHash FAIL\n");
	    return (TRUE);
    }

    if (cArg > 1)
	    printf("SUCCEED\n");

    if (cArg > 1)
	    printf("CryptHashData			");
    
    if (RCRYPT_FAILED(CryptHashData(hHash, ExpBuf, ExpBufLen, 0)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptHashData FAIL\n");
	    return (TRUE);
    }    

    if (cArg > 1)
	    printf("SUCCEED\n");

    if (cArg > 1)
	    printf("CryptSignHash (KEYX)		");

    SigBufLen = UTILITY_BUF_SIZE;
    if (RCRYPT_FAILED(CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0,
				    SigBuf, &SigBufLen)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptSignHash FAIL\n");
	    return (TRUE);
    }

    if (cArg > 1)
	    printf("SUCCEED\n");


    if (RCRYPT_FAILED(CryptDestroyHash(hHash)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyHash FAIL\n");
        return(TRUE);
    }

    if (cArg > 1)
	    printf("CryptCreateHash			");
    
    if (RCRYPT_FAILED(CryptCreateHash(hMe, CALG_SHA, 0, 0, &hHash)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptCreateHash FAIL\n");
	    return (TRUE);
    }

    if (cArg > 1)
	    printf("SUCCEED\n");

    if (cArg > 1)
	    printf("CryptHashData			");
    
    if (RCRYPT_FAILED(CryptHashData(hHash, ExpBuf, ExpBufLen, 0)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptHashData FAIL\n");
	    return (TRUE);
    }    

    if (cArg > 1)
	    printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptGetUserKey(hMe, AT_KEYEXCHANGE, &hKey2)))
    {
	    if (cArg > 1)
		    printf("cguk failed = %x", GetLastError());
	    else
		    printf("CryptGetUserKey FAIL\n");
	    return (TRUE);
    }

    if (cArg > 1)
	    printf("CryptVerifySignature (KEYX)	");
    
    if (RCRYPT_FAILED(CryptVerifySignature(hHash, SigBuf, SigBufLen, hKey2,
					   0, 0)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptVerifySignature FAIL\n");
	    return (TRUE);
    }    

    if (cArg > 1)
	    printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptDestroyKey(hKey2)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptDestroyHash(hHash)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptGetUserKey(hMe, AT_KEYEXCHANGE, &hKey2)))
    {
	    if (cArg > 1)
		    printf("cguk failed = %x", GetLastError());
	    else
		    printf("CryptGetUserKey FAIL\n");
	    return (TRUE);
    }

    if (cArg > 1)
	    printf("CryptImportKey			");
    
    if (RCRYPT_FAILED(CryptImportKey(hMe, ExpBuf, ExpBufLen, 0, 0,
				     &hKey)))
    {
	    if (cArg > 1)
		    printf("failed = %x", GetLastError());
	    else
		    printf("CryptImportKey FAIL\n");
	    return (TRUE);
    }    

    if (cArg > 1)
	    printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptDestroyKey(hKey2)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "RC4", ""))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptDestroyKey(hKey)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    return(FALSE);

}

BOOL TestRC2(int cArg, CHAR *bbuf, DWORD bsize)
{

//
// Generate a RC2 key
//
    if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RC2, 0, &hKey)))
    {
	if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptGenKey FAIL\n");
	return(TRUE);
    }

//
// Encrypt and Decrypt using CBC default mode
//
    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "RC2", "CBC"))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "RC2", "CBC"))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

//
// Change mode to ECB
//
    *pData = CRYPT_MODE_ECB;

    if (RCRYPT_FAILED(CryptSetKeyParam(hKey, KP_MODE, pData, 0)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptSetKeyParam FAIL\n");
        return(TRUE);
    }

    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "RC2", "ECB"))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "RC2", "ECB"))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

//
// Change mode to CFB
//
    *pData = CRYPT_MODE_CFB;

    if (RCRYPT_FAILED(CryptSetKeyParam(hKey, KP_MODE, pData, 0)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptSetKeyParam FAIL\n");
        return(TRUE);
    }

    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "RC2", "CFB"))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "RC2", "CFB"))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptDestroyKey(hKey)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    return(FALSE);

}


BOOL TestRC4(int cArg, CHAR *bbuf, DWORD bsize)
{

//
// Generate a RC4 key
//
	
    if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RC4, 0, &hKey)))
    {
	if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptGenKey FAIL\n");
	return(TRUE);
    }

//
// Encrypt and Decrypt
//
    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "RC4", ""))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "RC4", ""))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptDestroyKey(hKey)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    return(FALSE);

}

#ifdef TEST_VERSION
BOOL TestDES(int cArg, CHAR *bbuf, DWORD bsize)
{

//
// Generate a DES key
//
    if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_DES, 0, &hKey)))
    {
	if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptGenKey FAIL\n");
	return(TRUE);
    }

//
// Encrypt and Decrypt using CBC default mode
//
    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "DES", "CBC"))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "DES", "CBC"))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

//
// Change mode to ECB
//
    *pData = CRYPT_MODE_ECB;

    if (RCRYPT_FAILED(CryptSetKeyParam(hKey, KP_MODE, pData, 0)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptSetKeyParam FAIL\n");
        return(TRUE);
    }

    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "DES", "ECB"))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "DES", "ECB"))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

//
// Change mode to CFB
//
    *pData = CRYPT_MODE_CFB;

    if (RCRYPT_FAILED(CryptSetKeyParam(hKey, KP_MODE, pData, 0)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else
	    printf("CryptSetKeyParam FAIL\n");
        return(TRUE);
    }

    BLen = bsize;
    BLen2 = bsize + 8;
    if (TEncrypt(hKey, 0, TRUE, 0, bbuf, &BLen, BLen2, cArg, "DES", "CFB"))
    {
        return(TRUE);
    }

    if (TDecrypt(hKey, 0, TRUE, 0, bbuf, &BLen, cArg, "DES", "CFB"))
    {
        return(TRUE);
    }

    if (cArg > 1)
        printf("Compare data			");
    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("FAIL\n");
	return(TRUE);
    }
    if (cArg > 1)
	printf("SUCCEED\n");

    if (RCRYPT_FAILED(CryptDestroyKey(hKey)))
    {
        if (cArg > 1)
	    printf("failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    return(FALSE);

}
#endif


BOOL TestHash(int cArg, CHAR *bbuf, DWORD bsize)
{

    if (cArg > 1)
        printf("Hash, compare with MD4 & MD5	");

    BHashLen1 = 50;
    BHashSignLen1 = MAXKEYLEN;
    if (Hash(cArg, bbuf, bsize, pHashData1, &BHashLen1, pHashSignData1,
             &BHashSignLen1, CALG_MD4))
    {
        return(TRUE);
    }

    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("Data corrupted check 1 in Hash test\n");
	return(TRUE);
    }

    BHashLen2 = 50;
    BHashSignLen2 = MAXKEYLEN;
    if (Hash(cArg, bbuf, bsize, pHashData2, &BHashLen2, pHashSignData2,
             &BHashSignLen2, CALG_MD4))
    {
        return(TRUE);
    }

    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("Data corrupted check 2 in Hash test\n");
	return(TRUE);
    }

    if (BHashLen1 != BHashLen2)
    {
	printf("Hash lengths don't match MD4 FAIL\n");
	return(TRUE);
    }

    if (memcmp(pHashData1, pHashData2, BHashLen1) != 0)
    {
	printf("Hash data doesn't compare MD4 FAIL\n");
	return(TRUE);
    }

    if (BHashSignLen1 != BHashSignLen2)
    {
	printf("Hash signatures lengths don't match MD4 FAIL\n");
	return(TRUE);
    }

    if (memcmp(pHashSignData1, pHashSignData2, BHashLen1) != 0)
    {
	printf("Hash signature data doesn't compare MD4 FAIL\n");
	return(TRUE);
    }

    BHashLen1 = 50;
    BHashSignLen1 = MAXKEYLEN;
    if (Hash(cArg, bbuf, bsize, pHashData1, &BHashLen1, pHashSignData1,
             &BHashSignLen1, CALG_MD5))
    {
        return(TRUE);
    }

    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("Data corrupted check 3 in Hash test\n");
	return(TRUE);
    }

    BHashLen2 = 50;
    BHashSignLen2 = MAXKEYLEN;
    if (Hash(cArg, bbuf, bsize, pHashData2, &BHashLen2, pHashSignData2,
             &BHashSignLen2, CALG_MD5))
    {
        return(TRUE);
    }

    if (memcmp(bbuf, bRanbuf, bsize) != 0)
    {
	printf("Data corrupted check 4 in Hash test\n");
	return(TRUE);
    }

    if (BHashLen1 != BHashLen2)
    {
	printf("Hash lengths don't match MD5 FAIL\n");
	return(TRUE);
    }

    if (memcmp(pHashData1, pHashData2, BHashLen1) != 0)
    {
	printf("Hash data doesn't compare MD5 FAIL\n");
	return(TRUE);
    }

    if (BHashSignLen1 != BHashSignLen2)
    {
	printf("Hash signatures lengths don't match MD5 FAIL\n");
	return(TRUE);
    }

    if (memcmp(pHashSignData1, pHashSignData2, BHashSignLen1) != 0)
    {
	printf("Hash signature data doesn't compare MD5 FAIL\n");
	return(TRUE);
    }

    if (cArg > 1)
	printf("SUCCEED\n");

    return(FALSE);
    
}


BOOL Hash(int cArg, CHAR *bbuf, DWORD bsize, BYTE *pHashOut,
          DWORD *pHashLenOut, BYTE *pSigData, DWORD *pdwSigLen, DWORD Algid)
{

    if (RCRYPT_FAILED(CryptCreateHash(hMe, Algid, 0, 0, &hHash)))
    {
        if (cArg > 1)
	    printf("CryptCreateHash failed = %x\n", GetLastError());
	else 
	    printf("CryptCreateHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptHashData(hHash, bbuf, bsize, 0)))
    {
        if (cArg > 1)
	    printf("CryptUpDataHash failed = %x\n", GetLastError());
	else 
	    printf("CryptHashData FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptSignHash(hHash, AT_SIGNATURE, 0, 0, pSigData,
	              pdwSigLen)))
    {
        if (cArg > 1)
	    printf("CryptSignHash failed = %x\n", GetLastError());
	else 
	    printf("CryptSignHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptDestroyHash(hHash)))
    {
        if (cArg > 1)
	    printf("CryptDestroyHash failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptCreateHash(hMe, Algid, 0, 0, &hHash)))
    {
        if (cArg > 1)
	    printf("CryptCreateHash failed = %x\n", GetLastError());
	else 
	    printf("CryptCreateHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptHashData(hHash, bbuf, bsize, 0)))
    {
        if (cArg > 1)
	    printf("CryptUpDataHash failed = %x\n", GetLastError());
	else 
	    printf("CryptHashData FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptGetUserKey(hMe, AT_SIGNATURE, &hKey2)))
    {
	    if (cArg > 1)
		    printf("cguk failed = %x", GetLastError());
	    else
		    printf("CryptGetUserKey FAIL\n");
	    return (TRUE);
    }

    if (RCRYPT_FAILED(CryptVerifySignature(hHash, pSigData, *pdwSigLen,
                                           hKey2, 0, 0)))
    {
        if (cArg > 1)
	    printf("CryptVerifySignature failed = %x\n", GetLastError());
	else 
	    printf("CryptVerifySignature FAIL\n");
        return(TRUE);
    }



    if (RCRYPT_FAILED(CryptDestroyKey(hKey2)))
    {
        if (cArg > 1)
	    printf("CryptDestroyKey failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyKey FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptDestroyHash(hHash)))
    {
        if (cArg > 1)
	    printf("CryptDestroyHash failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptCreateHash(hMe, Algid, 0, 0, &hHash)))
    {
        if (cArg > 1)
	    printf("CryptCreateHash failed = %x\n", GetLastError());
	else 
	    printf("CryptCreateHash FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptHashData(hHash, bbuf, bsize, 0)))
    {
        if (cArg > 1)
	    printf("CryptUpDataHash failed = %x\n", GetLastError());
	else 
	    printf("CryptHashData FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptGetHashParam(hHash, HP_HASHVAL, pHashOut,
	              pHashLenOut, 0)))
    {
        if (cArg > 1)
	    printf("CryptGetHashParam failed = %x\n", GetLastError());
	else 
	    printf("CryptGetHashParam FAIL\n");
        return(TRUE);
    }

    if (RCRYPT_FAILED(CryptDestroyHash(hHash)))
    {
        if (cArg > 1)
	    printf("CryptDestroyHash failed = %x\n", GetLastError());
	else 
	    printf("CryptDestroyHash FAIL\n");
        return(TRUE);
    }

    return(FALSE);

}


BOOL Logon(int cArg)
{

	strcpy(pszMyName, "stress");

        if (cArg > 1)
            printf("CryptAcquireContext		");

        if (RCRYPT_FAILED(CryptAcquireContext(&hMe, pszMyName,
                          MS_DEF_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET)))
        {
	    if (cArg > 1)
                printf("FAIL Error = %x\n", GetLastError());
            return(TRUE);
	}
	else
	{
	if (cArg > 1)
           printf("SUCCEED\n");
        }

        if (cArg > 1)
            printf("CryptGenKeys			");

        if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RSA_SIGN,
                                          CRYPT_EXPORTABLE, &hClientKey)))
        {
	    if (cArg > 1)
                printf("FAIL Error = %x\n", GetLastError());
	    return(TRUE);
	}

        CryptDestroyKey(hClientKey);

        if (RCRYPT_FAILED(CryptGenKey(hMe, CALG_RSA_KEYX,
                                          CRYPT_EXPORTABLE, &hClientKey)))
        {
	    if (cArg > 1)
                printf("FAIL Error = %x\n", GetLastError());
	    return(TRUE);
	}
	if (cArg > 1)
            printf("SUCCEED\n");

        CryptDestroyKey(hClientKey);

	return(FALSE);

}



BOOL TEncrypt(HCRYPTKEY hTKey, HCRYPTHASH hTHash, BOOL FinalFlag,
              DWORD dwFlags, BYTE *Tbuf, DWORD *pBLen, DWORD pdwBufLen,
              int cArg, CHAR *szAlgid, CHAR *szmode)
{
        if (cArg > 1)
            printf("CryptEncrypt %s-%s		", szAlgid, szmode);
	if (RCRYPT_FAILED(CryptEncrypt(hTKey, hTHash, FinalFlag, dwFlags,
                                       Tbuf, pBLen, pdwBufLen)))
	{
            if (cArg > 1)
	        printf("failed = %x\n", GetLastError());
	    else 
	        printf("CryptEncrypt FAIL\n");
            return(TRUE);
	}
        if (cArg > 1)
	    printf("SUCCEED\n");

        return(FALSE);

}


BOOL TDecrypt(HCRYPTKEY hTKey, HCRYPTHASH hTHash, BOOL FinalFlag,
              DWORD dwFlags, BYTE *Tbuf, DWORD *pBLen, int cArg,
              CHAR *szAlgid, CHAR *szmode)
{

        if (cArg > 1)
            printf("CryptDecrypt %s-%s		", szAlgid, szmode);
	if (RCRYPT_FAILED(CryptDecrypt(hTKey, hTHash, FinalFlag, dwFlags,
                                       Tbuf, pBLen)))
	{
            if (cArg > 1)
	        printf("failed = %x\n", GetLastError());
	    else 
	        printf("CryptDecrypt FAIL\n");
            return(TRUE);
	}
        if (cArg > 1)
	    printf("SUCCEED\n");

        return(FALSE);

}


