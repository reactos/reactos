#undef UNICODE					// ## Not Yet
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <windows.h>
#include <winperf.h>
#include "..\..\inc\rsa.h"
#include "..\..\inc\md5.h"
#include "..\..\inc\rc4.h"

#define MAX_BUF_LEN	0x6000

BYTE RandState[20];

void memnuke(volatile BYTE *pData, DWORD dwLen);

void EncryptKey(BYTE *pFileName, BYTE val)
{
    RC4_KEYSTRUCT key;
    BYTE          RealKey[RC4_KEYSIZE] = {0xa2, 0x17, 0x9c, 0x98, 0xca};
    FILE          *OutFile;
    HFILE         hFile;
    OFSTRUCT      ImageInfoBuf;
    DWORD         NumBytes;
    DWORD         lpdwFileSizeHigh;
    LPVOID        lpvAddress;
    DWORD         NumBytesRead;
    DWORD         index;

    for (index = 0; index < RC4_KEYSIZE; index++)
    {
        RealKey[index] = RealKey[index] ^ val;
    }

    if ((hFile = OpenFile(pFileName, &ImageInfoBuf, OF_READ)) ==
	 HFILE_ERROR)
    {
        fprintf(stderr, "Can't open file %s\n", pFileName);
        ExitProcess(1);
    }

    if ((NumBytes = GetFileSize((HANDLE) hFile, &lpdwFileSizeHigh)) ==
	0xffffffff)
    {
        fprintf(stderr, "Can't GetFileSize for file %s\n", pFileName);
	_lclose(hFile);
        ExitProcess(1);
    }

    if ((lpvAddress = VirtualAlloc(NULL, NumBytes, MEM_RESERVE | MEM_COMMIT,
	PAGE_READWRITE)) == NULL)
    {
        fprintf(stderr, "Can't Read file %s\n", pFileName);
	_lclose(hFile);
        ExitProcess(1);
    }

    if (!ReadFile((HANDLE) hFile, lpvAddress, NumBytes, &NumBytesRead, 0))
    {
        fprintf(stderr, "Can't Read file %s\n", pFileName);
	_lclose(hFile);
	VirtualFree(lpvAddress, 0, MEM_RELEASE);
        ExitProcess(1);
    }

    _lclose(hFile);

    rc4_key(&key, RC4_KEYSIZE, RealKey);

    rc4(&key, NumBytes, lpvAddress);

    if ((OutFile = fopen(pFileName, "wb+")) == NULL)
    {
        fprintf(stderr, "Can't open file: %s \n", pFileName);
        ExitProcess(1);
    }

    if (fwrite(lpvAddress, NumBytes, 1, OutFile) == 0)
    {
        fprintf(stderr, "Can't write file: %s\n, FileName");
        ExitProcess(1);
    }

    VirtualFree(lpvAddress, 0, MEM_RELEASE);
    fclose(OutFile);

}


int __cdecl main(int cArg, char *rgszArg[])
{
	BYTE	Signature[0x48];
	FILE	*SigFile;
	
	if (cArg < 3)
	{
		printf("Usage: crypt <value [0-3]> <filename>\n");
		ExitProcess(1);
	}

	EncryptKey((CHAR *) rgszArg[2], (BYTE) atoi(rgszArg[1]));

	ExitProcess(0);

	return(0);
}

BOOL				// Keep as BOOL for the future
GenRandom (ULONG huid, BYTE *pbBuffer, size_t dwLength)
	{
	BYTE 	Randoms[20];
	long	i;
	int		index=0;
	SYSTEMTIME	lpSysTime;
	LARGE_INTEGER liPerfCount;
	MEMORYSTATUS	lpmstMemStat;
	POINT			Point;
	RC4_KEYSTRUCT	RC4Struct;
	BYTE			*pbTmp;

	DWORD	Type;
	LONG	lReturnCode;
	char	pReturnedData[100];			//good enough guesstimate (?)
	DWORD	dwMemPerfDataLen = 100;
	PERF_DATA_BLOCK	*pPDBlock;


	//mouse
	if (GetCursorPos(&Point))
		{
		Randoms[index++] =  LOBYTE(Point.x) ^ HIBYTE(Point.x);
		Randoms[index++] =  LOBYTE(Point.y) ^ HIBYTE(Point.y);
		}

	//local time: seconds and milliseconds
	GetLocalTime(&lpSysTime);
		Randoms[index++] = LOBYTE(lpSysTime.wMilliseconds);
		Randoms[index++] = HIBYTE(lpSysTime.wMilliseconds);
		Randoms[index++] = LOBYTE(lpSysTime.wSecond) ^ LOBYTE(lpSysTime.wMinute);

	//performance counter (millisecond counters of machine time)
	if (QueryPerformanceCounter(&liPerfCount))
		{
//		printf("Performance Counters\n");
		Randoms[index++] = (LOBYTE(LOWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart)));
		Randoms[index++] = (HIBYTE(LOWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart)));
		Randoms[index++] = (LOBYTE(HIWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart)));
		Randoms[index++] = (HIBYTE(HIWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart)));
		}

	//memory status report: available resources
	GlobalMemoryStatus(&lpmstMemStat);
		//only use hiwords, since lowwords always zero
		Randoms[index++] = LOBYTE(HIWORD(lpmstMemStat.dwAvailPhys));
		Randoms[index++] = HIBYTE(HIWORD(lpmstMemStat.dwAvailPhys));

		Randoms[index++] = LOBYTE(HIWORD(lpmstMemStat.dwAvailPageFile));
		Randoms[index++] = HIBYTE(HIWORD(lpmstMemStat.dwAvailPageFile));

		Randoms[index++] = LOBYTE(HIWORD(lpmstMemStat.dwAvailVirtual));
		//high byte doesn't change much

	//query high-res 100ns timer
	lReturnCode = RegQueryValueEx(HKEY_PERFORMANCE_DATA,
		TEXT("2"),
		NULL,
		&Type,
		(BYTE *) pReturnedData,
		&dwMemPerfDataLen);

	pPDBlock = (PERF_DATA_BLOCK *)pReturnedData;

	if (lReturnCode == ERROR_SUCCESS)
		{
		Randoms[index++] = LOBYTE(LOWORD(pPDBlock->PerfTime100nSec.LowPart)) ^ LOBYTE(LOWORD(pPDBlock->PerfTime100nSec.HighPart));
		Randoms[index++] = HIBYTE(LOWORD(pPDBlock->PerfTime100nSec.LowPart)) ^ LOBYTE(LOWORD(pPDBlock->PerfTime100nSec.HighPart));
		Randoms[index++] = LOBYTE(HIWORD(pPDBlock->PerfTime100nSec.LowPart)) ^ LOBYTE(LOWORD(pPDBlock->PerfTime100nSec.HighPart));
		Randoms[index++] = HIBYTE(HIWORD(pPDBlock->PerfTime100nSec.LowPart)) ^ LOBYTE(LOWORD(pPDBlock->PerfTime100nSec.HighPart));
		}

	//now we have %index% - 1 bytes of data! XOR with previous random data and
	// use this as the seed for RC4 stream, flip around and XOR
	for (i=0; i < 20; i++) {
		RandState[i] ^= Randoms[19-i];
	}
	rc4_key (&RC4Struct, 20, RandState);
	
	//now call RC4 and generate bits
	rc4 (&RC4Struct, dwLength, pbBuffer);

	// scrub rc4 struct & random work area
	pbTmp = (unsigned char *)&RC4Struct;
	memnuke(pbTmp, 258);
	memnuke(Randoms, 20);

	return TRUE;
	}

void memnuke(volatile BYTE *pData, DWORD dwLen)
{
	DWORD	i;

	for(i=0;i<dwLen;i++)
	{
	 	pData[i] = 0x00;
		pData[i] = 0xff;
		pData[i] = 0x00;
	}

	return;
}
