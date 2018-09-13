#undef UNICODE					// ## Not Yet
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <winperf.h>
#include "rsa.h"
#include "md5.h"
#include "rc4.h"

#define MAX_BUF_LEN	0x6000

extern LPBSAFE_PUB_KEY	PUB;
extern LPBSAFE_PRV_KEY	PRV;

BOOL initkey(void);

BYTE RandState[20];

void memnuke(volatile BYTE *pData, DWORD dwLen);

BOOL GenerateKeys(int KeyLen)
{
	BYTE		*kPrivate;
	BYTE		*kPublic;
	DWORD		dwPrivSize;
	DWORD		dwPubSize;
	DWORD		bits;
	DWORD		i;
	DWORD		j;

	printf("Generating a %d bit keypair\n", KeyLen);

	bits = KeyLen;
	BSafeComputeKeySizes(&dwPubSize, &dwPrivSize, &bits);

	if ((kPrivate = (BYTE *)malloc(dwPrivSize)) == NULL)
	{
		printf("Cannot allocate private key\n");
		exit(0);
	}

	if ((kPublic = (BYTE *)malloc(dwPubSize)) == NULL)
	{
		printf("Cannot allocate public key\n");
		exit(0);
	}
	
	if (!BSafeMakeKeyPair((LPBSAFE_PUB_KEY)kPublic,
			      (LPBSAFE_PRV_KEY)kPrivate,
			      KeyLen))
	{
		printf("Error generating keypair.\n");
		exit(0);
	}

	printf("Public:\n");
        j = 0;
	for(i=0;i<dwPubSize;i++)
	{
	    printf("0x%2.2x, ", kPublic[i]);

	    if (j == 7)
	    {
		printf("\n");
		j = 0;
	    }
	    else
		j++;
	}

	printf("\n");

	printf("Private:\n");
        j = 0;
	for(i=0;i<dwPrivSize;i++)
	{
            printf("0x%2.2x, ", kPrivate[i]);
	    if (j == 7)
	    {
		printf("\n");
		j = 0;
	    }
	    else
		j++;
	}

	free(kPublic);
	free(kPrivate);
	
	printf("\n");

	return(1);
		
}

BOOL VerifyImage(char *PathName, BYTE *SigData)
{
	FILE	*MyFile;
	BYTE	Buffer[MAX_BUF_LEN];
	BYTE	SigHash[0x48];
	int	BufLen;
	MD5_CTX	HashState;

	if (!initkey())
		return FALSE;

	// Open the file..
	if ((MyFile = fopen(PathName,"rb"))==NULL)
		return FALSE;

	MD5Init(&HashState);
	
	while(!feof(MyFile))
	{
		BufLen = fread(Buffer, sizeof(BYTE), MAX_BUF_LEN, MyFile);
		MD5Update(&HashState, Buffer, BufLen);
	}

	fclose(MyFile);

	// Finish the hash
	MD5Final(&HashState);

	// Decrypt the signature data
	BSafeEncPublic(PUB, SigData, SigHash);

	if (memcmp(HashState.digest, SigHash, 16))
		return FALSE;

	return TRUE;
}

BOOL MakeSig(char *PathName, BYTE *SigData)
{
	FILE	*MyFile;
	BYTE	Buffer[MAX_BUF_LEN];
	BYTE	SigHash[0x48];
	int	BufLen;
	MD5_CTX	HashState;

	if (!initkey())
		return FALSE;

	// Open the file..
	if ((MyFile = fopen(PathName,"rb"))==NULL)
		return FALSE;

	MD5Init(&HashState);
	
	while(!feof(MyFile))
	{
		BufLen = fread(Buffer, sizeof(BYTE), MAX_BUF_LEN, MyFile);
		MD5Update(&HashState, Buffer, BufLen);
	}

	fclose(MyFile);

	// Finish the hash
	MD5Final(&HashState);

        memset(SigHash, 0xff, 0x40);

	SigHash[0x40-1] = 0;
        SigHash[0x40-2] = 1;
	SigHash[16] = 0;

	memcpy(SigHash, HashState.digest, 16);

	// Encrypt the signature data
	BSafeDecPrivate(PRV, SigHash, SigData);

	return TRUE;
}

BOOL PrintHash(char *PathName)
{
	FILE	*MyFile;
	BYTE	Buffer[MAX_BUF_LEN];
	int	BufLen;
	MD5_CTX	HashState;
	DWORD   i;

	// Open the file..
	if ((MyFile = fopen(PathName,"rb"))==NULL)
		return FALSE;

	MD5Init(&HashState);
	
	while(!feof(MyFile))
	{
		BufLen = fread(Buffer, sizeof(BYTE), MAX_BUF_LEN, MyFile);
		MD5Update(&HashState, Buffer, BufLen);
	}

	fclose(MyFile);

	// Finish the hash
	MD5Final(&HashState);

        printf("Hash for file: ");
        for (i = 0; i < 16; i++)
	{
	    printf("%x ", HashState.digest[i]);
        }
        printf("\n");

	return TRUE;
}

int __cdecl main(int cArg, char *rgszArg[])
{
	BYTE	Signature[0x48];
	FILE	*SigFile;
	
	if (cArg < 3)
	{
		printf("Usage: signfile [s/v] <filename> <signature file>\n");
		printf("	s - sign file\n");
		printf("	v - verify file\n");

		ExitProcess(1);
	}

	if (toupper(*rgszArg[1]) == 'H')
	{
		if (!PrintHash(rgszArg[2]))
			ExitProcess(1);
        }

	if (toupper(*rgszArg[1]) == 'G')
	{
		GenerateKeys(atoi(rgszArg[2]));

		ExitProcess(0);
	}
	
	if (toupper(*rgszArg[1]) == 'V')
	{
		if ((SigFile = fopen(rgszArg[3], "rb")) == NULL)
		{
			fprintf(stderr, "Can't open signature file %s\n", rgszArg[3]);
			ExitProcess(1);
		}

		if (fread(Signature, 0x48, 1, SigFile) == 0)
		{
			fprintf(stderr, "Invalid signature file %s\n", rgszArg[3]);
			ExitProcess(1);
		}

		if (VerifyImage(rgszArg[2], Signature))
		{
			printf("Signature is valid.\n");
			ExitProcess(0);
		}
		else
		{
			printf("Signature is not valid.\n");
			ExitProcess(0);
		}
	}

	if (toupper(*rgszArg[1]) == 'S')
	{
		if ((SigFile = fopen(rgszArg[3], "wb+")) == NULL)
		{
			fprintf(stderr, "Can't open signature file %s\n", rgszArg[3]);
			ExitProcess(1);
		}

		if (!MakeSig(rgszArg[2], Signature))
			ExitProcess(1);

		if (fwrite(Signature, 0x48, 1, SigFile) == 0)
		{
			fprintf(stderr, "Can't write to signature file %s\n", rgszArg[3]);
			ExitProcess(1);
		}

		printf("Signature file generated.\n");

		fclose(SigFile);
	}

	ExitProcess(0);

	return(0);

}
