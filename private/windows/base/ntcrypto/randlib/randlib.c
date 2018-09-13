/*++

Copyright (c) 1993, 1998  Microsoft Corporation

Module Name:

    randlib.c

Abstract:

    This module implements the core cryptographic random number generator
    for use by system components.

    The #define KMODE_RNG affects whether the file is built in a way
    suitable for kernel mode usage.  if KMODE_RNG is not defined, the file
    is built in a way suitable for user mode usage.

Author:

    Scott Field (sfield)    27-Nov-96
    Jeff Spelman (jeffspel) 14-Oct-96

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>

#ifdef KMODE_RNG
#include <ntos.h>
#ifdef USE_HW_RNG
#ifdef _M_IX86
#include <io.h>
#include "deftypes.h"   //ISD typedefs and constants
#include "ioctldef.h"   //ISD ioctl definitions
#endif  // _M_IX86
#endif  // USE_HW_RNG
#endif  // KMODE_RNG

#include <windows.h>
#include <winioctl.h>
#include <lmcons.h>

#include <rc4.h>
#include <sha.h>
#include <md4.h>

#include <ntddksec.h>   // IOCTL_
#include <randlib.h>

#include "vlhash.h"
#include "circhash.h"
#include "cpu.h"
#include "seed.h"


#ifdef KMODE_RNG
#include <ntos.h>
#ifdef USE_HW_RNG
#ifdef _M_IX86
static DWORD g_dwHWDriver = 0;
static PFILE_OBJECT   g_pFileObject = NULL;
static PDEVICE_OBJECT g_pDeviceObject = NULL;
#endif  // _M_IX86
#endif  // USE_HW_RNG
#endif  // KMODE_RNG


#include "umkm.h"

//
// note: RAND_CTXT_LEN dictates the maximum input quantity for re-seed entropy
// is.  We make this fairly large, so that we can take all the entropy generated
// during the GatherRandomBits().  Since the lifetime of the RandContext structure
// is very short, and it lives on the stack, this larger than necessary size
// is ok.  The last few items processed during GatherRandomBits() are of
// variable size, up to a maximum of of UNLEN for the username.
//

#define RAND_CTXT_LEN           (256)
#define RC4_REKEY_PARAM_NT      (16384) // rekey less often on NT

#ifndef KMODE_RNG
#define RC4_REKEY_PARAM_DEFAULT (512)   // rekey every 512 bytes by default
#else
#define RC4_REKEY_PARAM_DEFAULT RC4_REKEY_PARAM_NT
#endif


static unsigned int     g_dwRC4RekeyParam = RC4_REKEY_PARAM_DEFAULT;

static CircularHash     g_CircularHashCtx;
static BYTE             g_VeryLargeHash[A_SHA_DIGEST_LEN*4];

static void *           g_RC4SafeCtx;

#ifndef KMODE_RNG

typedef NTSYSAPI NTSTATUS (NTAPI *NTQUERYSYSTEMINFORMATION) (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

typedef NTSYSAPI NTSTATUS (NTAPI *NTOPENFILE) (
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

typedef NTSYSAPI VOID (NTAPI *RTLINITUNICODESTRING) (
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

typedef BOOL (WINAPI *GETCURSORPOS)(
    LPPOINT lpPoint
    );

typedef LONG (WINAPI *GETMESSAGETIME)(
    VOID
    );

NTQUERYSYSTEMINFORMATION    ___NtQuerySystemInformationRNG = NULL;
NTOPENFILE                  ___NtOpenFileRNG = NULL;
RTLINITUNICODESTRING        ___RtlInitUnicodeStringRNG = NULL;

GETCURSORPOS                ___GetCursorPosRNG = NULL;
GETMESSAGETIME              ___GetMessageTimeRNG = NULL;

HANDLE g_hKsecDD = NULL;


#define _NtQuerySystemInformation   ___NtQuerySystemInformationRNG
#define _NtOpenFile                 ___NtOpenFileRNG
#define _RtlInitUnicodeString       ___RtlInitUnicodeStringRNG
#define _GetCursorPos               ___GetCursorPosRNG
#define _GetMessageTime             ___GetMessageTimeRNG

#else

#define _NtQuerySystemInformation ZwQuerySystemInformation

#endif // !KMODE_RNG

/// TODO: cache hKeySeed later.
///extern HKEY g_hKeySeed;



//
// private function prototypes.
//


BOOL
GenRandom (
    IN      PVOID           hUID,
        OUT BYTE            *pbBuffer,
    IN      size_t          dwLength
    );


BOOL
RandomFillBuffer(
        OUT BYTE            *pbBuffer,
    IN      DWORD           *pdwLength
    );

BOOL
GatherRandomKey(
    IN      BYTE            *pbUserSeed,
    IN      DWORD           cbUserSeed,
    IN  OUT BYTE            *pbRandomKey,
    IN  OUT DWORD           *pcbRandomKey
    );

BOOL
GatherRandomKeyFastUserMode(
    IN      BYTE            *pbUserSeed,
    IN      DWORD           cbUserSeed,
    IN  OUT BYTE            *pbRandomKey,
    IN  OUT DWORD           *pcbRandomKey
    );


BOOL
IsRNGWinNT(
    VOID
    );


#ifdef _M_IX86
unsigned int
QueryForHWRandomBits(
    IN      DWORD *pdwRandom,
    IN  OUT DWORD cdwRandom
    );
#endif //_M_IX86


#ifdef KMODE_RNG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NewGenRandom)
#pragma alloc_text(PAGE, NewGenRandomEx)
#pragma alloc_text(PAGE, GenRandom)
#pragma alloc_text(PAGE, RandomFillBuffer)
#pragma alloc_text(PAGE, InitializeRNG)
#pragma alloc_text(PAGE, ShutdownRNG)
#ifdef _M_IX86
#pragma alloc_text(PAGE, QueryForHWRandomBits)
#endif //_M_IX86
#pragma alloc_text(PAGE, GatherRandomKey)

#endif  // ALLOC_PRAGMA
#endif  // KMODE_RNG



/************************************************************************/
/* NewGenRandom generates a specified number of random bytes and places */
/* them into the specified buffer.                                      */
/************************************************************************/
/*                                                                      */
/* Pseudocode logic flow:                                               */
/*                                                                      */
/* if (bits streamed >= threshold)                                      */
/* {                                                                    */
/*  Gather_Bits()                                                       */
/*  SHAMix_Bits(User, Gathered, Static -> Static)                       */
/*  RC4Key(Static -> newRC4Key)                                         */
/*  SaveToRegistry(Static)                                              */
/* }                                                                    */
/* else                                                                 */
/* {                                                                    */
/*  Mix_Bits(User, Static -> Static)                                    */
/* }                                                                    */
/*                                                                      */
/* RC4(newRC4Key -> outbuf)                                             */
/* bits streamed += sizeof(outbuf)                                      */
/*                                                                      */
/************************************************************************/


unsigned int
RSA32API
NewGenRandomEx(
    IN      RNG_CONTEXT *pRNGContext,
    IN  OUT unsigned char *pbRandBuffer,
    IN      unsigned long cbRandBuffer
    )
{
    unsigned char **ppbRandSeed;
    unsigned long *pcbRandSeed;
    unsigned int fRet;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    fRet = TRUE;

    if( pRNGContext->cbSize != sizeof( RNG_CONTEXT ) )
        return FALSE;

    if( pRNGContext->pbRandSeed && pRNGContext->cbRandSeed ) {

        ppbRandSeed = &pRNGContext->pbRandSeed;
        pcbRandSeed = &pRNGContext->cbRandSeed;

    } else {

        ppbRandSeed = NULL;
        pcbRandSeed = NULL;
    }

    InitRand( ppbRandSeed, pcbRandSeed );
    InitializeRNG( NULL );

    if( pRNGContext->Flags & RNG_FLAG_REKEY_ONLY ) {

        //
        // caller wants REKEY only.
        //

        fRet = GatherRandomKey( NULL, 0, pbRandBuffer, &cbRandBuffer );

    } else {

        //
        // standard RNG request.
        //

        fRet = GenRandom(0, pbRandBuffer, cbRandBuffer);
    }

    if( ppbRandSeed && pcbRandSeed ) {
        DeInitRand( *ppbRandSeed, *pcbRandSeed);
    }

    return fRet;
}

unsigned int
RSA32API
NewGenRandom (
    IN  OUT unsigned char **ppbRandSeed,
    IN      unsigned long *pcbRandSeed,
    IN  OUT unsigned char *pbBuffer,
    IN      unsigned long dwLength
    )
{
    RNG_CONTEXT RNGContext;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG


    ZeroMemory( &RNGContext, sizeof(RNGContext) );
    RNGContext.cbSize = sizeof(RNGContext);

    if( ppbRandSeed && pcbRandSeed ) {
        BOOL fRet;

        RNGContext.pbRandSeed = *ppbRandSeed;
        RNGContext.cbRandSeed = *pcbRandSeed;

        fRet = NewGenRandomEx( &RNGContext, pbBuffer, dwLength );
        *pcbRandSeed = RNGContext.cbRandSeed;

        return fRet;
    }

    return NewGenRandomEx( &RNGContext, pbBuffer, dwLength );
}

unsigned int
RSA32API
InitRand(
    IN  OUT unsigned char **ppbRandSeed,
    IN      unsigned long *pcbRandSeed
    )
{

    static BOOL fInitialized = FALSE;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    if( !fInitialized ) {

        InitCircularHash(
                    &g_CircularHashCtx,
                    7,
                    CH_ALG_MD4,
                    0   // CH_MODE_FEEDBACK
                    );

        //
        // get prior seed.
        //

        ReadSeed( g_VeryLargeHash, sizeof( g_VeryLargeHash ) );

        fInitialized = TRUE;
    }

    if( ppbRandSeed != NULL && pcbRandSeed != NULL && *pcbRandSeed != 0 )
        UpdateCircularHash( &g_CircularHashCtx, *ppbRandSeed, *pcbRandSeed );

    return TRUE;
}

unsigned int
RSA32API
DeInitRand(
    IN  OUT unsigned char *pbRandSeed,
    IN      unsigned long cbRandSeed
    )
{
    PBYTE       pbCircularHash;
    DWORD       cbCircularHash;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    if( pbRandSeed == NULL || cbRandSeed == 0 )
        return TRUE;

    if(GetCircularHashValue( &g_CircularHashCtx, &pbCircularHash, &cbCircularHash )) {

        unsigned long cbToCopy;

        if( cbRandSeed > cbCircularHash ) {
            cbToCopy = cbCircularHash;
        } else {
            cbToCopy = cbRandSeed;
        }

        memcpy(pbRandSeed, pbCircularHash, cbToCopy);
    }

    return TRUE;
}

unsigned int
RSA32API
InitializeRNG(
    VOID *pvReserved
    )
{
    void *pvCtx;
    void *pvOldCtx;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    if( g_RC4SafeCtx ) {
        return TRUE;
    }

    if(!rc4_safe_startup( &pvCtx )) {
        return FALSE;
    }

    pvOldCtx = INTERLOCKEDCOMPAREEXCHANGEPOINTER( &g_RC4SafeCtx, pvCtx, NULL );

    if( pvOldCtx ) {

        //
        // race condition occured during init.
        //

        rc4_safe_shutdown( pvCtx );
    }

    return TRUE;
}

void
RSA32API
ShutdownRNG(
    VOID *pvReserved
    )
{
    void *pvCtx;
    HKEY hKey;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    pvCtx = InterlockedExchangePointer( &g_RC4SafeCtx, NULL );

    if( pvCtx ) {
        rc4_safe_shutdown( pvCtx );
    }

#ifndef KMODE_RNG
{
    HANDLE hFile;
    hFile = InterlockedExchangePointer( &g_hKsecDD, NULL );

    if( hFile ) {
        CloseHandle( hFile );
    }
}
#endif

#if 0
    // TODO later: finish logic for caching registry key.
    hKey = InterlockedExchangePointer( &g_hKeySeed, NULL );

    if( hKey ) {
        REGCLOSEKEY( hKey );
    }
#endif

}

BOOL
GenRandom (
    IN      PVOID hUID,
        OUT BYTE *pbBuffer,
    IN      size_t dwLength
    )
{
    DWORD           dwBytesThisPass;
    DWORD           dwFilledBytes;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG

    dwFilledBytes = 0;

    // break request into chunks that we rekey between
    while(dwFilledBytes < dwLength)
    {
        dwBytesThisPass = dwLength - dwFilledBytes;

        if(!RandomFillBuffer(
                pbBuffer + dwFilledBytes,
                &dwBytesThisPass
                )) {

            return FALSE;
        }

        dwFilledBytes += dwBytesThisPass;
    }

    return TRUE;
}


BOOL
RandomFillBuffer(
        OUT BYTE *pbBuffer,
    IN      DWORD *pdwLength
    )
{
    unsigned int RC4BytesUsed;
    unsigned int KeyId;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG


    //
    // update circular hash with user supplied bits.
    //

    if(!UpdateCircularHash( &g_CircularHashCtx, pbBuffer, *pdwLength ))
        return FALSE;


    //
    // select key.
    //

    rc4_safe_select( g_RC4SafeCtx, &KeyId, &RC4BytesUsed );


    //
    // check if re-key required.
    //

    if ( RC4BytesUsed >= g_dwRC4RekeyParam )
    {
        PBYTE       pbCircularHash;
        DWORD       cbCircularHash;
        BYTE        pbRandomKey[ 256 ];
        DWORD       cbRandomKey = sizeof(pbRandomKey);

        RC4BytesUsed = g_dwRC4RekeyParam;

        if(!GetCircularHashValue(
                &g_CircularHashCtx,
                &pbCircularHash,
                &cbCircularHash
                )) {

            return FALSE;
        }

        if(!GatherRandomKey( pbCircularHash, cbCircularHash, pbRandomKey, &cbRandomKey ))
            return FALSE;

        //
        // Create RC4 key
        //

        rc4_safe_key(
                g_RC4SafeCtx,
                KeyId,
                cbRandomKey,
                pbRandomKey
                );

        ZeroMemory( pbRandomKey, sizeof(pbRandomKey) );
    }


    //
    // only use RC4_REKEY_PARAM bytes from each RC4 key
    //

    {
        DWORD dwMaxPossibleBytes = g_dwRC4RekeyParam - RC4BytesUsed;

        if (*pdwLength > dwMaxPossibleBytes)
            *pdwLength = dwMaxPossibleBytes;
    }

    rc4_safe( g_RC4SafeCtx, KeyId, *pdwLength, pbBuffer );

    return TRUE;
}


#ifdef KMODE_RNG
#ifdef USE_HW_RNG
#ifdef _M_IX86
#define NUM_HW_DWORDS_TO_GATHER     4
#define INTEL_DRIVER_NAME           L"\\Device\\ISECDRV"

unsigned int
QueryForHWRandomBits(
    IN      DWORD *pdwRandom,
    IN  OUT DWORD cdwRandom
    )
{
    UNICODE_STRING ObjectName;
    IO_STATUS_BLOCK StatusBlock;
    KEVENT Event;
    PIRP pIrp = NULL;
  	ISD_Capability ISD_Cap;                //in/out for GetCapability
	ISD_RandomNumber ISD_Random;           //in/out for GetRandomNumber
    PDEVICE_OBJECT pDeviceObject = NULL;
    DWORD i = 0;
    unsigned int Status = ERROR_SUCCESS;

    PAGED_CODE();

    if (1 == g_dwHWDriver)
    {
        Status = STATUS_ACCESS_DENIED;
        goto Ret;
    }

    ZeroMemory( &ObjectName, sizeof(ObjectName) );
    ZeroMemory( &StatusBlock, sizeof(StatusBlock) );
    ZeroMemory(&ISD_Cap, sizeof(ISD_Cap));


    if (NULL == g_pDeviceObject)
    {
        ObjectName.Length = sizeof(INTEL_DRIVER_NAME) - sizeof(WCHAR);
        ObjectName.MaximumLength = sizeof(INTEL_DRIVER_NAME);
        ObjectName.Buffer = INTEL_DRIVER_NAME;
        Status = IoGetDeviceObjectPointer(&ObjectName,
                                          FILE_ALL_ACCESS,
                                          &g_pFileObject,
                                          &pDeviceObject);

        if ( !NT_SUCCESS(Status) )
        {
            g_dwHWDriver = 1;
            goto Ret;
        }

        if (NULL == g_pDeviceObject)
        {
            InterlockedExchangePointer(&g_pDeviceObject, pDeviceObject);
        }
    }

    //
    // If this fails then it is because there is no such device
    // which signals completion.
    //


    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    ISD_Cap.uiIndex = ISD_RNG_ENABLED;  //Set input member
    pIrp = IoBuildDeviceIoControlRequest(
        IOCTL_ISD_GetCapability,
        g_pDeviceObject,
        &ISD_Cap,
        sizeof(ISD_Cap),
        &ISD_Cap,
        sizeof(ISD_Cap),
        FALSE,
        &Event,
        &StatusBlock); 

    if (pIrp == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Ret;
    }

    Status = IoCallDriver(g_pDeviceObject, pIrp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = StatusBlock.Status;
    }

    if (ISD_Cap.iStatus != ISD_EOK) {  
        Status = STATUS_NOT_IMPLEMENTED;
        goto Ret;
    }

    // now get the random bits
    for (i = 0; i < cdwRandom; i++) {
        ZeroMemory(&ISD_Random, sizeof(ISD_Random));
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        pIrp = IoBuildDeviceIoControlRequest(
            IOCTL_ISD_GetRandomNumber,
            g_pDeviceObject,
            &ISD_Random,
            sizeof(ISD_Random),
            &ISD_Random,
            sizeof(ISD_Random),
            FALSE,
            &Event,
            &StatusBlock); 

        if (pIrp == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Ret;
        }

        Status = IoCallDriver(g_pDeviceObject, pIrp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = StatusBlock.Status;
        }    

        if (ISD_Random.iStatus != ISD_EOK) {
            Status = STATUS_NOT_IMPLEMENTED;
            goto Ret;
        }

        pdwRandom[i] = pdwRandom[i] ^ ISD_Random.uiRandomNum;
    }
Ret:
    return Status;
}
#endif // _M_IX86
#endif // USE_HW_RNG
#endif // KMODE_RNG

BOOL
GatherRandomKey(
    IN      BYTE            *pbUserSeed,
    IN      DWORD           cbUserSeed,
    IN  OUT BYTE            *pbRandomKey,
    IN  OUT DWORD           *pcbRandomKey
    )
{

    LPBYTE  pbWorkingBuffer = NULL;
    DWORD   cbWorkingBuffer;
    DWORD   cbBufferRemaining;
    BYTE    *pbCurrentBuffer;
    DWORD   *pdwTmp;
    BOOL    fRet;

#ifdef KMODE_RNG
    PAGED_CODE();
#endif  // KMODE_RNG




    //
    // in NT Usermode, try to re-seed by calling the Kernelmode RNG.
    //

#ifndef KMODE_RNG
    if( GatherRandomKeyFastUserMode( pbUserSeed, cbUserSeed, pbRandomKey, pcbRandomKey ))
        return TRUE;
#endif



//
// verify current working buffer has space for candidate data.
//

#define VERIFY_BUFFER( size ) {                                         \
    if( cbBufferRemaining < size )                                      \
        goto finished;                                                  \
    }

//
// update working buffer and increment to next QWORD aligned boundary.
//

#define UPDATE_BUFFER( size ) {                                         \
    DWORD dwSizeRounded;                                                \
    dwSizeRounded = (size + sizeof(ULONG64)) & ~(sizeof(ULONG64)-1);    \
    if(dwSizeRounded > cbBufferRemaining)                               \
        goto finished;                                                  \
    pbCurrentBuffer += dwSizeRounded;                                   \
    cbBufferRemaining -= dwSizeRounded;                                 \
    }


    cbWorkingBuffer = 3584;
    pbWorkingBuffer = (PBYTE)ALLOC( cbWorkingBuffer );
    if( pbWorkingBuffer == NULL ) {
        return FALSE;
    }

    cbBufferRemaining = cbWorkingBuffer;
    pbCurrentBuffer = pbWorkingBuffer;

    //
    // pickup user supplied bits.
    //

    VERIFY_BUFFER( cbUserSeed );
    CopyMemory( pbCurrentBuffer, pbUserSeed, cbUserSeed );
    UPDATE_BUFFER( cbUserSeed );

    //
    // ** indicates US DoD's specific recommendations for password generation
    //


    //
    // process id
    //

    pdwTmp = (PDWORD)pbCurrentBuffer;

#ifndef KMODE_RNG
    *pdwTmp = GetCurrentProcessId();
#else
///    *pdwTmp = PsGetCurrentProcessId();
#endif

    UPDATE_BUFFER( sizeof(DWORD) );

    //
    // thread id
    //

    pdwTmp = (PDWORD)pbCurrentBuffer;

#ifndef KMODE_RNG
    *pdwTmp = GetCurrentThreadId();
#else
///    *pdwTmp = PsGetCurrentThreadId();
#endif

    UPDATE_BUFFER( sizeof(DWORD) );


    //
    // ** ticks since boot (system clock)
    //

    pdwTmp = (PDWORD)pbCurrentBuffer;

#ifndef KMODE_RNG
    *pdwTmp = GetTickCount();
#else
///    *pdwTmp = NtGetTickCount();
#endif  // !KMODE_RNG

    UPDATE_BUFFER( sizeof(DWORD) );


    //
    // ** system time, in ms, sec, min (date & time)
    //

#ifndef KMODE_RNG
    {
        PSYSTEMTIME psysTime = (PSYSTEMTIME)pbCurrentBuffer;

        VERIFY_BUFFER( sizeof( *psysTime ) );
        GetLocalTime(psysTime);
        UPDATE_BUFFER( sizeof( *psysTime ) );
    }
#else
    {

        PSYSTEM_TIMEOFDAY_INFORMATION pTimeOfDay;
        ULONG cbSystemInfo;

        pTimeOfDay = (PSYSTEM_TIMEOFDAY_INFORMATION)pbCurrentBuffer;

        VERIFY_BUFFER( sizeof(*pTimeOfDay) );

        _NtQuerySystemInformation(
                    SystemTimeOfDayInformation,
                    pTimeOfDay,
                    sizeof(*pTimeOfDay),
                    &cbSystemInfo
                    );

        UPDATE_BUFFER(  cbSystemInfo );
    }

#endif  // !KMODE_RNG

    //
    // ** hi-res performance counter (system counters)
    //

    {
        LARGE_INTEGER   *pliPerfCount = (PLARGE_INTEGER)pbCurrentBuffer;

        VERIFY_BUFFER( sizeof(*pliPerfCount) );

#ifndef KMODE_RNG
        QueryPerformanceCounter(pliPerfCount);
#else
///        ZwQueryPerformanceCounter(pliPerfCount, &liPerfFreq);
#endif  // !KMODE_RNG

        UPDATE_BUFFER( sizeof(*pliPerfCount) );
    }

#ifndef KMODE_RNG

    //
    // memory status
    //

    {
        MEMORYSTATUS *pmstMemStat = (MEMORYSTATUS *)pbCurrentBuffer;

        VERIFY_BUFFER( sizeof(*pmstMemStat) );

        pmstMemStat->dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus( pmstMemStat );

        UPDATE_BUFFER( sizeof(*pmstMemStat) );
    }

#endif  // !KMODE_RNG


    //
    // free disk clusters
    //

#ifndef KMODE_RNG

    {
        PDWORD pdwDiskInfo = (PDWORD)pbCurrentBuffer;

        VERIFY_BUFFER( (sizeof(DWORD) * 4) );

        GetDiskFreeSpace(
                    NULL,
                    &pdwDiskInfo[0],    // sectors per cluster
                    &pdwDiskInfo[1],    // bytes per sector
                    &pdwDiskInfo[2],    // number of free clusters
                    &pdwDiskInfo[3]     // total number of clusters
                    );

        UPDATE_BUFFER( (sizeof(DWORD) * 4) );
    }
#endif  // !KMODE_RNG


#ifndef KMODE_RNG
    {

#if 0
        //
        // hash the entire user environment block.
        // we do this instead of GetUserName & GetComputerName,
        // as the environment block contains these values, plus additional
        // values.
        //

        static BOOL fHashedEnv;
        static BYTE HashEnv[ MD4_LEN ];

        if( !fHashedEnv ) {

            LPVOID lpEnvBlock;
            BOOL fAnsi = FALSE;

            //
            // try the Unicode version first, as, on WinNT, this returns us
            // a pointer to the existing Unicode environment block, rather
            // than an allocated copy.  Fallback to ANSI if this fails (eg: Win9x)
            //

            lpEnvBlock = GetEnvironmentStringsW();
            if( lpEnvBlock == NULL )
            {
                lpEnvBlock = GetEnvironmentStringsA();
                fAnsi = TRUE;
            }


            if( lpEnvBlock != NULL ) {

                ULONG cbEntry;
                PBYTE pbEntry;
                MD4_CTX MD4Ctx;


                MD4Init( &MD4Ctx );

                pbEntry = (PBYTE)lpEnvBlock;
                cbEntry = 0;

                do {

                    if( !fAnsi ) {
                        pbEntry += (cbEntry + sizeof(WCHAR));
                        cbEntry = lstrlenW( (LPWSTR)pbEntry ) * sizeof(WCHAR);
                    } else {
                        pbEntry += (cbEntry + sizeof(CHAR));
                        cbEntry = lstrlenA( (LPSTR)pbEntry ) * sizeof(CHAR);
                    }

                    MD4Update(
                        &MD4Ctx,
                        (unsigned char *)pbEntry,
                        (unsigned int)cbEntry
                        );

                } while( cbEntry );


                MD4Final( &MD4Ctx );

                CopyMemory( HashEnv, MD4Ctx.digest, sizeof(HashEnv) );

                if( !fAnsi ) {
                    FreeEnvironmentStringsW( lpEnvBlock );
                } else {
                    FreeEnvironmentStringsA( lpEnvBlock );
                }
            }

            //
            // only try this once.  if it failed once, it will likely never
            // succeed.
            //

            fHashedEnv = TRUE;
        }


        VERIFY_BUFFER( (sizeof(HashEnv)) );
        CopyMemory( pbCurrentBuffer, HashEnv, sizeof(HashEnv) );
        UPDATE_BUFFER( (sizeof(HashEnv)) );

#else
        static DWORD dwComputerNameSize;
        static DWORD dwUserNameSize;
        static CHAR lpComputerName [MAX_COMPUTERNAME_LENGTH + 1];
        static CHAR lpUserName [UNLEN + 1];

        //
        // note use of two temp DWORDs - that's to keep the static DWORDs
        // thread safe
        //

        //
        // **SystemID
        //

        if(dwComputerNameSize == 0) {
            DWORD dwTempComputerNameSize = sizeof(lpComputerName) / sizeof(CHAR);

            if(GetComputerNameA(lpComputerName, &dwTempComputerNameSize))
                dwComputerNameSize = dwTempComputerNameSize;
        }

        if(dwComputerNameSize != 0) {
            // dwComputerNameSize = len not including null termination

            VERIFY_BUFFER( dwComputerNameSize );
            CopyMemory( pbCurrentBuffer, lpComputerName, dwComputerNameSize );
            UPDATE_BUFFER( dwComputerNameSize );
        }

        //
        // **UserID
        //

        if(dwUserNameSize == 0) {
            DWORD dwTempUserNameSize = sizeof(lpUserName) / sizeof(CHAR);

            if(GetUserNameA(lpUserName, &dwTempUserNameSize)) {
                // dwUserNameSize = len including null termination
                dwUserNameSize = dwTempUserNameSize - 1;
            }
        }

        if(dwUserNameSize != 0) {
            VERIFY_BUFFER( dwUserNameSize );
            CopyMemory( pbCurrentBuffer, lpUserName, dwUserNameSize );
            UPDATE_BUFFER( dwUserNameSize );
        }
#endif
    }
#endif  // !KMODE_RNG

    //
    // this code path has been moved to the end so that our CombineRand()
    // operation on NT mixes in with everything slammed into the
    // rand context buffer.
    //

#ifndef KMODE_RNG
    if(!IsRNGWinNT()) {

        //
        // only user info if we are not running on NT.
        // this prevents deadlocks on WinNT when the RNG is called from CSRSS
        //

        POINT   *ppoint;
        LONG    *plTime;

        //
        // cursor position
        //

        ppoint = (POINT*)pbCurrentBuffer;

        VERIFY_BUFFER( sizeof(*ppoint) );
        _GetCursorPos(ppoint);
        UPDATE_BUFFER( sizeof(*ppoint) );

        //
        // last messages' timestamp
        //

        plTime = (LONG*)pbCurrentBuffer;

        VERIFY_BUFFER( sizeof(*plTime) );
        *plTime = _GetMessageTime();
        UPDATE_BUFFER( sizeof(*plTime) );


    } else
#endif  // !KMODE_RNG
    {
        unsigned char *pbCounterState = (unsigned char*)pbCurrentBuffer;
        unsigned long cbCounterState = 64;

        VERIFY_BUFFER(cbCounterState);

        if(GatherCPUSpecificCounters( pbCounterState, &cbCounterState )) {
            UPDATE_BUFFER( cbCounterState );
        }


        //
        // call NtQuerySystemInformation on NT if available.
        //

        if( (void*)_NtQuerySystemInformation ) {

            PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION pSystemProcessorPerformanceInfo;
            PSYSTEM_PERFORMANCE_INFORMATION pSystemPerformanceInfo;
            PSYSTEM_EXCEPTION_INFORMATION pSystemExceptionInfo;
            PSYSTEM_LOOKASIDE_INFORMATION pSystemLookasideInfo;
            PSYSTEM_INTERRUPT_INFORMATION pSystemInterruptInfo;
            PSYSTEM_PROCESS_INFORMATION pSystemProcessInfo;
            ULONG cbSystemInfo;
            NTSTATUS Status;

            //
            // fixed length system info calls.
            //

            pSystemProcessorPerformanceInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)pbCurrentBuffer;

            VERIFY_BUFFER( sizeof(*pSystemProcessorPerformanceInfo) );

            Status = _NtQuerySystemInformation(
                        SystemProcessorPerformanceInformation,
                        pSystemProcessorPerformanceInfo,
                        sizeof(*pSystemProcessorPerformanceInfo),
                        &cbSystemInfo
                        );

            if ( NT_SUCCESS(Status) ) {
                UPDATE_BUFFER( cbSystemInfo );
            }

            pSystemPerformanceInfo = (PSYSTEM_PERFORMANCE_INFORMATION)pbCurrentBuffer;

            VERIFY_BUFFER( sizeof(*pSystemPerformanceInfo) );

            Status = _NtQuerySystemInformation(
                        SystemPerformanceInformation,
                        pSystemPerformanceInfo,
                        sizeof(*pSystemPerformanceInfo),
                        &cbSystemInfo
                        );

            if ( NT_SUCCESS(Status) ) {
                UPDATE_BUFFER(  cbSystemInfo );
            }

            pSystemExceptionInfo = (PSYSTEM_EXCEPTION_INFORMATION)pbCurrentBuffer;

            VERIFY_BUFFER( sizeof(*pSystemExceptionInfo) );

            Status = _NtQuerySystemInformation(
                        SystemExceptionInformation,
                        pSystemExceptionInfo,
                        sizeof(*pSystemExceptionInfo),
                        &cbSystemInfo
                        );

            if ( NT_SUCCESS(Status) ) {
                UPDATE_BUFFER( cbSystemInfo );
            }

            pSystemLookasideInfo = (PSYSTEM_LOOKASIDE_INFORMATION)pbCurrentBuffer;

            VERIFY_BUFFER( sizeof(*pSystemLookasideInfo) );

            Status = _NtQuerySystemInformation(
                        SystemLookasideInformation,
                        pSystemLookasideInfo,
                        sizeof(*pSystemLookasideInfo),
                        &cbSystemInfo
                        );

            if ( NT_SUCCESS(Status) ) {
                UPDATE_BUFFER( cbSystemInfo );
            }

            //
            // variable length system info calls.
            //

            pSystemInterruptInfo = (PSYSTEM_INTERRUPT_INFORMATION)pbCurrentBuffer;
            cbSystemInfo = cbBufferRemaining;

            Status = _NtQuerySystemInformation(
                        SystemInterruptInformation,
                        pSystemInterruptInfo,
                        cbSystemInfo,
                        &cbSystemInfo
                        );

            if ( NT_SUCCESS(Status) ) {
                UPDATE_BUFFER( cbSystemInfo );
            }

            pSystemProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pbCurrentBuffer;
            cbSystemInfo = cbBufferRemaining;

            Status = _NtQuerySystemInformation(
                        SystemProcessInformation,
                        pSystemProcessInfo,
                        cbSystemInfo,
                        &cbSystemInfo
                        );

            if ( NT_SUCCESS(Status) ) {
                UPDATE_BUFFER( cbSystemInfo );
            }

        } // _NtQuerySystemInformation
    }

#ifdef KMODE_RNG
#ifdef USE_HW_RNG
#ifdef _M_IX86
    // attempt to get bits from the INTEL HW RNG
    {
        DWORD rgdwHWRandom[NUM_HW_DWORDS_TO_GATHER];
        NTSTATUS Status;


        VERIFY_BUFFER( sizeof(rgdwHWRandom) );

        Status = QueryForHWRandomBits(
                    rgdwHWRandom,
                    NUM_HW_DWORDS_TO_GATHER
                    );

        if ( NT_SUCCESS(Status) ) {
            UPDATE_BUFFER( sizeof(rgdwHWRandom) );
        }

    }
#endif // _M_IX86
#endif // USE_HW_RNG
#endif // KMODE_RNG

finished:

    {
        RC4_KEYSTRUCT rc4Key;
        BYTE NewSeed[ sizeof(g_VeryLargeHash) ];
        BYTE LocalHash[ sizeof( g_VeryLargeHash ) ];
        DWORD cbBufferSize;

        CopyMemory( LocalHash, g_VeryLargeHash, sizeof(g_VeryLargeHash) );

        rc4_key( &rc4Key, sizeof(LocalHash), LocalHash );

        cbBufferSize = cbWorkingBuffer - cbBufferRemaining;
        if( cbBufferSize > cbWorkingBuffer )
            cbBufferSize = cbWorkingBuffer;

        fRet = VeryLargeHashUpdate(
                    pbWorkingBuffer,                    // buffer to hash
                    cbBufferSize,
                    LocalHash
                    );

        CopyMemory( NewSeed, LocalHash, sizeof(LocalHash) );
        CopyMemory( g_VeryLargeHash, LocalHash, sizeof(LocalHash) );
        rc4( &rc4Key, sizeof( NewSeed ), NewSeed );

        //
        // write seed out.
        //

        WriteSeed( NewSeed, sizeof(NewSeed) );

        rc4_key( &rc4Key, sizeof(LocalHash), LocalHash );
        rc4( &rc4Key, *pcbRandomKey, pbRandomKey );

        ZeroMemory( &rc4Key, sizeof(rc4Key) );

        if( pbWorkingBuffer ) {
            FREE( pbWorkingBuffer );
        }
    }


    return fRet;
}



#ifndef KMODE_RNG

BOOL
GatherRandomKeyFastUserMode(
    IN      BYTE            *pbUserSeed,
    IN      DWORD           cbUserSeed,
    IN  OUT BYTE            *pbRandomKey,
    IN  OUT DWORD           *pcbRandomKey
    )
/*++

    This routine attempts to gather RNG re-seed material for usermode callers
    from the Kernel mode version of the RNG.  This is accomplished by making
    a device IOCTL into the ksecdd.sys device driver.

--*/
{
    HANDLE hFile;
    NTSTATUS Status;

    if(!IsRNGWinNT())
        return FALSE;

    hFile = g_hKsecDD;

    if( hFile == NULL ) {

        UNICODE_STRING DriverName;
        OBJECT_ATTRIBUTES ObjA;
        IO_STATUS_BLOCK IOSB;
        HANDLE hPreviousValue;

        //
        // call via the ksecdd.sys device driver to get the random bits.
        //

        if( _NtOpenFile == NULL || _RtlInitUnicodeString == NULL ) {
            return FALSE;
        }

        //
        // have to use the Nt flavor of the file open call because it's a base
        // device not aliased to \DosDevices
        //

        _RtlInitUnicodeString( &DriverName, DD_KSEC_DEVICE_NAME_U );
        InitializeObjectAttributes(
                    &ObjA,
                    &DriverName,
                    OBJ_CASE_INSENSITIVE,
                    0,
                    0
                    );

        Status = _NtOpenFile(
                    &hFile,
                    SYNCHRONIZE | FILE_READ_DATA,
                    &ObjA,
                    &IOSB,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_SYNCHRONOUS_IO_ALERT
                    );


        if( !NT_SUCCESS(Status) )
            return FALSE;

        hPreviousValue = INTERLOCKEDCOMPAREEXCHANGEPOINTER(
                                        &g_hKsecDD,
                                        hFile,
                                        NULL
                                        );

        if( hPreviousValue != NULL ) {

            //
            // race condition, set current value to previously initialized version.
            //

            CloseHandle( hFile );
            hFile = hPreviousValue;
        }
    }

    return DeviceIoControl(
                hFile,
                IOCTL_KSEC_RNG_REKEY,   // indicate a RNG rekey
                pbUserSeed,             // input buffer (existing material)
                cbUserSeed,             // input buffer size
                pbRandomKey,            // output buffer
                *pcbRandomKey,          // output buffer size
                pcbRandomKey,           // bytes written to output buffer
                NULL
                );
}


BOOL
IsRNGWinNT(
    VOID
    )
/*++

    This function determines if we are running on Windows NT and furthermore,
    if it is appropriate to make use of certain user operations where the
    code is running.

    If the function returns TRUE, the caller cannot make calls to user
    based function and should use an alternative approach such as
    NtQuerySystemInformation.

    If the function returns FALSE, the caller can safely call user based
    functions to gather random material.

--*/
{
    static BOOL fIKnow = FALSE;

    // we assume WinNT in case of error.
    static BOOL fIsWinNT = TRUE;

    OSVERSIONINFO osVer;

    if(fIKnow)
        return(fIsWinNT);

    ZeroMemory(&osVer, sizeof(osVer));
    osVer.dwOSVersionInfoSize = sizeof(osVer);

    if( GetVersionEx(&osVer) ) {
        fIsWinNT = (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT);

        if( fIsWinNT ) {
            //
            // if we're on NT, collect entry point address.
            //
            HMODULE hNTDll = GetModuleHandleW( L"ntdll.dll" );

            if( hNTDll ) {
                _NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(
                                hNTDll,
                                "NtQuerySystemInformation"
                                );

                //
                // On WinNT, adjust the rekey param to be a much larger value
                // because we have more entropy to key from.
                //

                if( _NtQuerySystemInformation )
                    g_dwRC4RekeyParam = RC4_REKEY_PARAM_NT;

                _NtOpenFile = (NTOPENFILE)GetProcAddress(
                                hNTDll,
                                "NtOpenFile"
                                );

                _RtlInitUnicodeString = (RTLINITUNICODESTRING)GetProcAddress(
                                hNTDll,
                                "RtlInitUnicodeString"
                                );
            }
        } else {
            //
            // collect entry point addresses for Win95
            //
            HMODULE hUser32 = LoadLibraryA("user32.dll");

            if( hUser32 ) {
                _GetCursorPos = (GETCURSORPOS)GetProcAddress(
                                hUser32,
                                "GetCursorPos"
                                );

                _GetMessageTime = (GETMESSAGETIME)GetProcAddress(
                                hUser32,
                                "GetMessageTime"
                                );
            }

        }
    }

    // even on an error, this is as good as it gets
    fIKnow = TRUE;

    return fIsWinNT;
}

#endif  // !KMODE_RNG

