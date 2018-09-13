/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
* This file is part of the Microsoft Private Communication Technology
* reference implementation, version 1.0
*
* The Private Communication Technology reference implementation, version 1.0
* ("PCTRef"), is being provided by Microsoft to encourage the development and
* enhancement of an open standard for secure general-purpose business and
* personal communications on open networks.  Microsoft is distributing PCTRef
* at no charge irrespective of whether you use PCTRef for non-commercial or
* commercial use.
*
* Microsoft expressly disclaims any warranty for PCTRef and all derivatives of
* it.  PCTRef and any related documentation is provided "as is" without
* warranty of any kind, either express or implied, including, without
* limitation, the implied warranties or merchantability, fitness for a
* particular purpose, or noninfringement.  Microsoft shall have no obligation
* to provide maintenance, support, upgrades or new releases to you or to anyone
* receiving from you PCTRef or your modifications.  The entire risk arising out
* of use or performance of PCTRef remains with you.
*
* Please see the file LICENSE.txt,
* or http://pct.microsoft.com/pct/pctlicen.txt
* for more information on licensing.
*
* Please see http://pct.microsoft.com/pct/pct.htm for The Private
* Communication Technology Specification version 1.0 ("PCT Specification")
*
* 1/23/96
*----------------------------------------------------------------------------*/

//
// This file exports four functions: InitializeRNG, ShutdownRNG, GenRandom, and
// GenerateRandomBits, which are used to generate random sequences of bytes.
//

#include <windows.h>
#include <rng.h>
#include <rc4.h>
#include <sha.h>

#define A_SHA_DIGEST_LEN    20
#define RAND_CTXT_LEN       60
#define RC4_REKEY_PARAM     500     // rekey every 500 bytes

#define UNLEN   MAX_PATH

typedef struct _RandContext
{
    DWORD dwBitsFilled;
    BYTE  rgbBitBuffer[RAND_CTXT_LEN];
} RandContext;

#if 0
CRITICAL_SECTION    csRNG;

#define LockRNG()   EnterCriticalSection( &csRNG )
#define UnlockRNG() LeaveCriticalSection( &csRNG )
#else
#define LockRNG()
#define UnlockRNG()
#endif

unsigned char g_rgbStaticBits[A_SHA_DIGEST_LEN];
static DWORD         g_dwRC4BytesUsed = RC4_REKEY_PARAM;     // initially force rekey
static struct RC4_KEYSTRUCT g_rc4key;

static BOOL RandomFillBuffer(BYTE *pbBuffer, DWORD *pdwLength);
static void GatherRandomBits(RandContext *prandContext);
static void AppendRand(RandContext* prandContext, void* pv, DWORD dwSize);

/*****************************************************************************/
VOID STInitializeRNG(VOID)
{
    DWORD dwType;
    DWORD dwSize;

    LONG err;

#if 0

    InitializeCriticalSection( &csRNG );

    LockRNG();

    // grab seed from persistent storage

//    SPQueryPersistentSeed(g_rgbStaticBits, A_SHA_DIGEST_LEN);
#endif

    g_dwRC4BytesUsed = RC4_REKEY_PARAM;

#if 0
    UnlockRNG();
#endif

    return;
}

VOID ShutdownRNG(VOID)
{

#if 0
    DeleteCriticalSection( &csRNG );
#endif

    // put seed into persistent storage

//    SPSetPersistentSeed(g_rgbStaticBits, A_SHA_DIGEST_LEN);

    return;
}

/*****************************************************************************/
int STGenRandom(PVOID Reserved,
              UCHAR *pbBuffer,
              size_t dwLength)
{
    STGenerateRandomBits(pbBuffer, dwLength);
    return TRUE;
}

/************************************************************************/
/* GenerateRandomBits generates a specified number of random bytes and        */
/* places them into the specified buffer.                                */
/************************************************************************/
/*                                                                      */
/* Pseudocode logic flow:                                               */
/*                                                                      */
/* if (bits streamed > threshold)                                       */
/* {                                                                    */
/*  Gather_Bits()                                                       */
/*  SHAMix_Bits(User, Gathered, Static -> Static)                       */
/*  RC4Key(Static -> newRC4Key)                                         */
/*  SHABits(Static -> Static)      // hash after RC4 key generation     */
/* }                                                                    */
/* else                                                                 */
/* {                                                                    */
/*  SHAMix_Bits(User, Static -> Static)                                 */
/* }                                                                    */
/*                                                                      */
/* RC4(newRC4Key -> outbuf)                                             */
/* bits streamed += sizeof(outbuf)                                      */
/*                                                                      */
/************************************************************************/
VOID STGenerateRandomBits(PUCHAR pbBuffer,
                        ULONG  dwLength)
{
    DWORD dwBytesThisPass;
    DWORD dwFilledBytes = 0;

    // break request into chunks that we rekey between

    LockRNG();

    while(dwFilledBytes < dwLength)
    {
        dwBytesThisPass = dwLength - dwFilledBytes;

        RandomFillBuffer(pbBuffer + dwFilledBytes, &dwBytesThisPass);
        dwFilledBytes += dwBytesThisPass;
    }

    UnlockRNG();
}

/*****************************************************************************/
static BOOL RandomFillBuffer(BYTE *pbBuffer, DWORD *pdwLength)
{
    // Variables from loading and storing the registry...
    DWORD   cbDataLen;

    cbDataLen = A_SHA_DIGEST_LEN;

    if(g_dwRC4BytesUsed >= RC4_REKEY_PARAM) {
        // if we need to rekey

        RandContext randContext;
        randContext.dwBitsFilled = 0;

        GatherRandomBits(&randContext);

        // Mix all bits
        {
            A_SHA_CTX SHACtx;
            A_SHAInit(&SHACtx);

            // SHA the static bits
            A_SHAUpdate(&SHACtx, g_rgbStaticBits, A_SHA_DIGEST_LEN);

            // SHA the gathered bits
            A_SHAUpdate(&SHACtx, randContext.rgbBitBuffer, randContext.dwBitsFilled);

            // SHA the user-supplied bits
            A_SHAUpdate(&SHACtx, pbBuffer, *pdwLength);

            // output back out to static bits
            A_SHAFinal(&SHACtx, g_rgbStaticBits);
        }

        // Create RC4 key
        g_dwRC4BytesUsed = 0;
        rc4_key(&g_rc4key, A_SHA_DIGEST_LEN, g_rgbStaticBits);

        // Mix RC4 key bits around
        {
            A_SHA_CTX SHACtx;
            A_SHAInit(&SHACtx);

            // SHA the static bits
            A_SHAUpdate(&SHACtx, g_rgbStaticBits, A_SHA_DIGEST_LEN);

            // output back out to static bits
            A_SHAFinal(&SHACtx, g_rgbStaticBits);
        }

    } else {
        // Use current RC4 key, but capture any user-supplied bits.

        // Mix input bits
        {
            A_SHA_CTX SHACtx;
            A_SHAInit(&SHACtx);

            // SHA the static bits
            A_SHAUpdate(&SHACtx, g_rgbStaticBits, A_SHA_DIGEST_LEN);

            // SHA the user-supplied bits
            A_SHAUpdate(&SHACtx, pbBuffer, *pdwLength);

            // output back out to static bits
            A_SHAFinal(&SHACtx, g_rgbStaticBits);
        }
    }

    // only use RC4_REKEY_PARAM bytes from each RC4 key
    {
        DWORD dwMaxPossibleBytes = RC4_REKEY_PARAM - g_dwRC4BytesUsed;
        if(*pdwLength > dwMaxPossibleBytes) {
                *pdwLength = dwMaxPossibleBytes;
        }
    }

    FillMemory(pbBuffer, *pdwLength, 0);
    rc4(&g_rc4key, *pdwLength, pbBuffer);

    g_dwRC4BytesUsed += *pdwLength;

    return TRUE;
}

/*****************************************************************************/
static void GatherRandomBits(RandContext *prandContext)
{
    DWORD   dwTmp;
    WORD    wTmp;
    BYTE    bTmp;

    // ** indicates US DoD's specific recommendations for password generation

    // proc id
    dwTmp = GetCurrentProcessId();
    AppendRand(prandContext, &dwTmp, sizeof(dwTmp));

    // thread id
    dwTmp = GetCurrentThreadId();
    AppendRand(prandContext, &dwTmp, sizeof(dwTmp));

    // ** ticks since boot (system clock)
    dwTmp = GetTickCount();
    AppendRand(prandContext, &dwTmp, sizeof(dwTmp));

    // cursor position
    {
        POINT                        point;
        GetCursorPos(&point);
        bTmp = LOBYTE(point.x) ^ HIBYTE(point.x);
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
        bTmp = LOBYTE(point.y) ^ HIBYTE(point.y);
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
    }

    // ** system time, in ms, sec, min (date & time)
    {
        SYSTEMTIME                sysTime;
        GetLocalTime(&sysTime);
        AppendRand(prandContext, &sysTime.wMilliseconds, sizeof(sysTime.wMilliseconds));
        bTmp = LOBYTE(sysTime.wSecond) ^ LOBYTE(sysTime.wMinute);
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
    }

    // ** hi-res performance counter (system counters)
    {
        LARGE_INTEGER        liPerfCount;
        if(QueryPerformanceCounter(&liPerfCount)) {
            bTmp = LOBYTE(LOWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
            bTmp = HIBYTE(LOWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
            bTmp = LOBYTE(HIWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
            bTmp = HIBYTE(HIWORD(liPerfCount.LowPart)) ^ LOBYTE(LOWORD(liPerfCount.HighPart));
            AppendRand(prandContext, &bTmp, sizeof(BYTE));
        }
    }

    // memory status
    {
        MEMORYSTATUS        mstMemStat;
        mstMemStat.dwLength = sizeof(MEMORYSTATUS);     // must-do
        GlobalMemoryStatus(&mstMemStat);
        wTmp = HIWORD(mstMemStat.dwAvailPhys);          // low words seem to be always zero
        AppendRand(prandContext, &wTmp, sizeof(WORD));
        wTmp = HIWORD(mstMemStat.dwAvailPageFile);
        AppendRand(prandContext, &wTmp, sizeof(WORD));
        bTmp = LOBYTE(HIWORD(mstMemStat.dwAvailVirtual));
        AppendRand(prandContext, &bTmp, sizeof(BYTE));
    }

    // free disk clusters
    {
        DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters, dwTotalNumberOfClusters;
        if(GetDiskFreeSpace(NULL, &dwSectorsPerCluster, &dwBytesPerSector,     &dwNumberOfFreeClusters, &dwTotalNumberOfClusters)) {
            AppendRand(prandContext, &dwNumberOfFreeClusters, sizeof(dwNumberOfFreeClusters));
            AppendRand(prandContext, &dwTotalNumberOfClusters, sizeof(dwTotalNumberOfClusters));
            AppendRand(prandContext, &dwBytesPerSector, sizeof(dwBytesPerSector));
        }
    }

    // last messages' timestamp
    {
        LONG lTime;
        lTime = GetMessageTime();
        AppendRand(prandContext, &lTime, sizeof(lTime));
    }

    {
        static DWORD dwComputerNameSize;
        static DWORD dwUserNameSize;
        static char lpComputerName [MAX_COMPUTERNAME_LENGTH + 1];
        static char lpUserName [UNLEN + 1];

        //
        // note use of two temp DWORDs - that's to keep the static DWORDs
        // thread safe
        //

        // **SystemID
        if(dwComputerNameSize == 0) {
            DWORD dwTempComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;

            if(GetComputerNameA(lpComputerName, &dwTempComputerNameSize))
                dwComputerNameSize = dwTempComputerNameSize;
        }

        if(dwComputerNameSize != 0) {
            // dwComputerNameSize = len not including null termination
            AppendRand(prandContext, lpComputerName, dwComputerNameSize);
        }

        // **UserID
        if(dwUserNameSize == 0) {
            DWORD dwTempUserNameSize = UNLEN + 1;

            if(GetUserNameA(lpUserName, &dwTempUserNameSize)) {
                // dwUserNameSize = len including null termination
                dwUserNameSize = dwTempUserNameSize - 1;
            }
        }

        if(dwUserNameSize != 0) {
            AppendRand(prandContext, lpUserName, dwUserNameSize);
        }
    }
}

/*****************************************************************************/
static void AppendRand(RandContext* prandContext, void* pv, DWORD dwSize)
{
    DWORD dwBitsLeft = (RAND_CTXT_LEN - prandContext->dwBitsFilled);

    if(dwBitsLeft > 0) {
        if(dwSize > dwBitsLeft) {
            dwSize = dwBitsLeft;
        }

        CopyMemory(prandContext->rgbBitBuffer + prandContext->dwBitsFilled, pv, dwSize);
        prandContext->dwBitsFilled += dwSize;
    }
}
