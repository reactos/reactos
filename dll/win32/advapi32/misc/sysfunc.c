/*
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/advapi32/misc/sysfun.c
 * PURPOSE:         advapi32.dll system functions (undocumented)
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990413 EA	created
 *	19990415 EA
 *	20080424 Ported from WINE
 */

#include <advapi32.h>
#include <ntsecapi.h>
#include <ksecioctl.h>
#include <md4.h>
#include <md5.h>
#include <rc4.h>

static const unsigned char CRYPT_LMhash_Magic[8] =
    { 'K', 'G', 'S', '!', '@', '#', '$', '%' };

/******************************************************************************
 * SystemFunction001  [ADVAPI32.@]
 *
 * Encrypts a single block of data using DES
 *
 * PARAMS
 *   data    [I] data to encrypt    (8 bytes)
 *   key     [I] key data           (7 bytes)
 *   output  [O] the encrypted data (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS
WINAPI SystemFunction001(const BYTE *data, const BYTE *key, LPBYTE output)
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DEShash(output, key, data);
    return STATUS_SUCCESS;
}


/******************************************************************************
 * SystemFunction002  [ADVAPI32.@]
 *
 * Decrypts a single block of data using DES
 *
 * PARAMS
 *   data    [I] data to decrypt    (8 bytes)
 *   key     [I] key data           (7 bytes)
 *   output  [O] the decrypted data (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS
WINAPI SystemFunction002(const BYTE *data, const BYTE *key, LPBYTE output)
{
    if (!data || !output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DESunhash(output, key, data);
    return STATUS_SUCCESS;
}


/******************************************************************************
 * SystemFunction003  [ADVAPI32.@]
 *
 * Hashes a key using DES and a fixed datablock
 *
 * PARAMS
 *   key     [I] key data    (7 bytes)
 *   output  [O] hashed key  (8 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS
WINAPI SystemFunction003(const BYTE *key, LPBYTE output)
{
    if (!output)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DEShash(output, key, CRYPT_LMhash_Magic);
    return STATUS_SUCCESS;
}


/******************************************************************************
 * SystemFunction004  [ADVAPI32.@]
 *
 * Encrypts a block of data with DES in ECB mode, preserving the length
 *
 * PARAMS
 *   data    [I] data to encrypt
 *   key     [I] key data (up to 7 bytes)
 *   output  [O] buffer to receive encrypted data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_BUFFER_TOO_SMALL     if the output buffer is too small
 *  Failure: STATUS_INVALID_PARAMETER_2  if the key is zero length
 *
 * NOTES
 *  Encrypt buffer size should be input size rounded up to 8 bytes
 *   plus an extra 8 bytes.
 */
NTSTATUS
WINAPI SystemFunction004(const struct ustring *in,
                                  const struct ustring *key,
                                  struct ustring *out)
{
    union {
        unsigned char uc[8];
          unsigned int  ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int crypt_len, ofs;

    if (key->Length<=0)
        return STATUS_INVALID_PARAMETER_2;

    crypt_len = ((in->Length+7)&~7);
    if (out->MaximumLength < (crypt_len+8))
        return STATUS_BUFFER_TOO_SMALL;

    data.ui[0] = in->Length;
    data.ui[1] = 1;

    if (key->Length<sizeof deskey)
    {
        memset(deskey, 0, sizeof deskey);
        memcpy(deskey, key->Buffer, key->Length);
    }
    else
        memcpy(deskey, key->Buffer, sizeof deskey);

    CRYPT_DEShash(out->Buffer, deskey, data.uc);

    for(ofs=0; ofs<(crypt_len-8); ofs+=8)
        CRYPT_DEShash(out->Buffer+8+ofs, deskey, in->Buffer+ofs);

    memset(data.uc, 0, sizeof data.uc);
    memcpy(data.uc, in->Buffer+ofs, in->Length +8-crypt_len);
    CRYPT_DEShash(out->Buffer+8+ofs, deskey, data.uc);

    out->Length = crypt_len+8;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction005  [ADVAPI32.@]
 *
 * Decrypts a block of data with DES in ECB mode
 *
 * PARAMS
 *   data    [I] data to decrypt
 *   key     [I] key data (up to 7 bytes)
 *   output  [O] buffer to receive decrypted data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_BUFFER_TOO_SMALL     if the output buffer is too small
 *  Failure: STATUS_INVALID_PARAMETER_2  if the key is zero length
 *
 */
NTSTATUS
WINAPI SystemFunction005(const struct ustring *in,
                         const struct ustring *key,
                         struct ustring *out)
{
    union {
        unsigned char uc[8];
        unsigned int  ui[2];
    } data;
    unsigned char deskey[7];
    unsigned int ofs, crypt_len;

    if (key->Length<=0)
        return STATUS_INVALID_PARAMETER_2;

    if (key->Length<sizeof deskey)
    {
        memset(deskey, 0, sizeof deskey);
        memcpy(deskey, key->Buffer, key->Length);
    }
    else
        memcpy(deskey, key->Buffer, sizeof deskey);

    CRYPT_DESunhash(data.uc, deskey, in->Buffer);

    if (data.ui[1] != 1)
        return STATUS_UNKNOWN_REVISION;

    crypt_len = data.ui[0];
    if (crypt_len > out->MaximumLength)
        return STATUS_BUFFER_TOO_SMALL;

    for (ofs=0; (ofs+8)<crypt_len; ofs+=8)
        CRYPT_DESunhash(out->Buffer+ofs, deskey, in->Buffer+ofs+8);

    if (ofs<crypt_len)
    {
        CRYPT_DESunhash(data.uc, deskey, in->Buffer+ofs+8);
        memcpy(out->Buffer+ofs, data.uc, crypt_len-ofs);
    }

    out->Length = crypt_len;

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction007  [ADVAPI32.@]
 *
 * MD4 hash a unicode string
 *
 * PARAMS
 *   string  [I] the string to hash
 *   output  [O] the md4 hash of the string (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS
WINAPI SystemFunction007(const UNICODE_STRING *string, LPBYTE hash)
{
    MD4_CTX ctx;

    MD4Init( &ctx );
    MD4Update( &ctx, (const BYTE *)string->Buffer, string->Length );
    MD4Final( &ctx );
    memcpy( hash, ctx.digest, 0x10 );

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction008  [ADVAPI32.@]
 *
 * Creates a LM response from a challenge and a password hash
 *
 * PARAMS
 *   challenge  [I] Challenge from authentication server
 *   hash       [I] NTLM hash (from SystemFunction006)
 *   response   [O] response to send back to the server
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 * NOTES
 *  see http://davenport.sourceforge.net/ntlm.html#theLmResponse
 *
 */
NTSTATUS
WINAPI SystemFunction008(const BYTE *challenge, const BYTE *hash, LPBYTE response)
{
    BYTE key[7*3];

    if (!challenge || !response)
        return STATUS_UNSUCCESSFUL;

    memset(key, 0, sizeof key);
    memcpy(key, hash, 0x10);

    CRYPT_DEShash(response, key, challenge);
    CRYPT_DEShash(response+8, key+7, challenge);
    CRYPT_DEShash(response+16, key+14, challenge);

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction009  [ADVAPI32.@]
 *
 * Seems to do the same as SystemFunction008...
 */
NTSTATUS
WINAPI SystemFunction009(const BYTE *challenge, const BYTE *hash, LPBYTE response)
{
    return SystemFunction008(challenge, hash, response);
}

/******************************************************************************
 * SystemFunction010  [ADVAPI32.@]
 * SystemFunction011  [ADVAPI32.@]
 *
 * MD4 hashes 16 bytes of data
 *
 * PARAMS
 *   unknown []  seems to have no effect on the output
 *   data    [I] pointer to data to hash (16 bytes)
 *   output  [O] the md4 hash of the data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 */
NTSTATUS
WINAPI SystemFunction010(LPVOID unknown, const BYTE *data, LPBYTE hash)
{
    MD4_CTX ctx;

    MD4Init( &ctx );
    MD4Update( &ctx, data, 0x10 );
    MD4Final( &ctx );
    memcpy( hash, ctx.digest, 0x10 );

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction012  [ADVAPI32.@]
 * SystemFunction014  [ADVAPI32.@]
 * SystemFunction016  [ADVAPI32.@]
 * SystemFunction018  [ADVAPI32.@]
 * SystemFunction020  [ADVAPI32.@]
 * SystemFunction022  [ADVAPI32.@]
 *
 * Encrypts two DES blocks with two keys
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (two lots of 7 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL  if the input or output buffer is NULL
 */
NTSTATUS
WINAPI SystemFunction012(const BYTE *in, const BYTE *key, LPBYTE out)
{
    if (!in || !out)
        return STATUS_UNSUCCESSFUL;

    CRYPT_DEShash(out, key, in);
    CRYPT_DEShash(out+8, key+7, in+8);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction013  [ADVAPI32.@]
 * SystemFunction015  [ADVAPI32.@]
 * SystemFunction017  [ADVAPI32.@]
 * SystemFunction019  [ADVAPI32.@]
 * SystemFunction021  [ADVAPI32.@]
 * SystemFunction023  [ADVAPI32.@]
 *
 * Decrypts two DES blocks with two keys
 *
 * PARAMS
 *   data    [I] data to decrypt (16 bytes)
 *   key     [I] key data (two lots of 7 bytes)
 *   output  [O] buffer to receive decrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL  if the input or output buffer is NULL
 */
NTSTATUS
WINAPI SystemFunction013(const BYTE *in, const BYTE *key, LPBYTE out)
{
    if (!in || !out)
        return STATUS_UNSUCCESSFUL;
    CRYPT_DESunhash(out, key, in);
    CRYPT_DESunhash(out+8, key+7, in+8);
    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction024  [ADVAPI32.@]
 *
 * Encrypts two DES blocks with a 32 bit key...
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (4 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 */
NTSTATUS
WINAPI SystemFunction024(const BYTE *in, const BYTE *key, LPBYTE out)
{
    BYTE deskey[0x10];

    memcpy(deskey, key, 4);
    memcpy(deskey+4, key, 4);
    memcpy(deskey+8, key, 4);
    memcpy(deskey+12, key, 4);

    CRYPT_DEShash(out, deskey, in);
    CRYPT_DEShash(out+8, deskey+7, in+8);

    return STATUS_SUCCESS;
}

/******************************************************************************
 * SystemFunction025  [ADVAPI32.@]
 *
 * Decrypts two DES blocks with a 32 bit key...
 *
 * PARAMS
 *   data    [I] data to encrypt (16 bytes)
 *   key     [I] key data (4 bytes)
 *   output  [O] buffer to receive encrypted data (16 bytes)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 */
NTSTATUS
WINAPI SystemFunction025(const BYTE *in, const BYTE *key, LPBYTE out)
{
    BYTE deskey[0x10];

    memcpy(deskey, key, 4);
    memcpy(deskey+4, key, 4);
    memcpy(deskey+8, key, 4);
    memcpy(deskey+12, key, 4);

    CRYPT_DESunhash(out, deskey, in);
    CRYPT_DESunhash(out+8, deskey+7, in+8);

    return STATUS_SUCCESS;
}

/**********************************************************************
 *
 * @unimplemented
 */
INT
WINAPI
SystemFunction028(INT a, INT b)
{
    //NDRCContextBinding()
    //SystemFunction034()
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 28;
}


/**********************************************************************
 *
 * @unimplemented
 */
INT
WINAPI
SystemFunction029(INT a, INT b)
{
    //I_RpcBindingIsClientLocal()
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 29;
}


/******************************************************************************
 * SystemFunction030   (ADVAPI32.@)
 *
 * Tests if two blocks of 16 bytes are equal
 *
 * PARAMS
 *  b1,b2   [I] block of 16 bytes
 *
 * RETURNS
 *  TRUE  if blocks are the same
 *  FALSE if blocks are different
 */
BOOL
WINAPI SystemFunction030(LPCVOID b1, LPCVOID b2)
{
	return !memcmp(b1, b2, 0x10);
}


/******************************************************************************
 * SystemFunction032  [ADVAPI32.@]
 *
 * Encrypts a string data using ARC4
 *
 * PARAMS
 *   data    [I/O] data to encrypt
 *   key     [I] key data
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_UNSUCCESSFUL
 *
 * NOTES
 *  see http://web.it.kth.se/~rom/ntsec.html#crypto-strongavail
 */
NTSTATUS
WINAPI SystemFunction032(struct ustring *data, const struct ustring *key)
{
    RC4_CONTEXT a4i;

    rc4_init(&a4i, key->Buffer, key->Length);
    rc4_crypt(&a4i, data->Buffer, data->Length);

    return STATUS_SUCCESS;
}


/**********************************************************************
 *
 * @unimplemented
 */
INT
WINAPI
SystemFunction033(INT a, INT b)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 33;
}

/**********************************************************************
 *
 * @unimplemented
 */
INT
WINAPI
SystemFunction034(INT a, INT b)
{
    //RpcBindingToStringBindingW
    //I_RpcMapWin32Status
    //RpcStringBindingParseW
    //RpcStringFreeW
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 34;
}


/******************************************************************************
 * SystemFunction035   (ADVAPI32.@)
 *
 * Described here:
http://disc.server.com/discussion.cgi?disc=148775;article=942;title=Coding%2FASM%2FSystem
 *
 * NOTES
 *  Stub, always return TRUE.
 */
BOOL WINAPI SystemFunction035(LPCSTR lpszDllFilePath)
{
    //FIXME("%s: stub\n", debugstr_a(lpszDllFilePath));
    return TRUE;
}

/******************************************************************************
 * SystemFunction036   (ADVAPI32.@)
 *
 * MSDN documents this function as RtlGenRandom and declares it in ntsecapi.h
 *
 * PARAMS
 *  pbBuffer [O] Pointer to memory to receive random bytes.
 *  dwLen    [I] Number of random bytes to fetch.
 *
 * RETURNS
 *  Always TRUE in my tests
 */
BOOLEAN
WINAPI
SystemFunction036(PVOID pbBuffer, ULONG dwLen)
{
    ////////////////////////////////////////////////////////////////
    //////////////////// B I G   W A R N I N G  !!! ////////////////
    // This function will output numbers based on the tick count. //
    // It will NOT OUPUT CRYPTOGRAPHIC-SAFE RANDOM NUMBERS !!!    //
    ////////////////////////////////////////////////////////////////

    DWORD dwSeed;
    PBYTE pBuffer;
    ULONG uPseudoRandom;
    LARGE_INTEGER time;
    static ULONG uCounter = 17;

    if(!pbBuffer || !dwLen)
    {
        /* This function always returns TRUE, even if invalid parameters were passed. (verified under WinXP SP2) */
        return TRUE;
    }

    /* Get the first seed from the performance counter */
    QueryPerformanceCounter(&time);
    dwSeed = time.LowPart ^ time.HighPart ^ RtlUlongByteSwap(uCounter++);

    /* We will access the buffer bytewise */
    pBuffer = (PBYTE)pbBuffer;

    do
    {
        /* Use the pseudo random number generator RtlRandom, which outputs a 4-byte value and a new seed */
        uPseudoRandom = RtlRandom(&dwSeed);

        do
        {
            /* Get each byte from the pseudo random number and store it in the buffer */
            *pBuffer = (BYTE)(uPseudoRandom >> 8 * (dwLen % 3) & 0xFF);
            ++pBuffer;
        } while(--dwLen % 3);
    } while(dwLen);

    return TRUE;
}

HANDLE KsecDeviceHandle;

static
NTSTATUS
KsecOpenDevice()
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KsecDD");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE DeviceHandle;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&DeviceHandle,
                        FILE_READ_DATA | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (InterlockedCompareExchangePointer(&KsecDeviceHandle, DeviceHandle, NULL) != NULL)
    {
        NtClose(DeviceHandle);
    }

    return STATUS_SUCCESS;
}

VOID
CloseKsecDdHandle(VOID)
{
    /* Check if we already opened a handle to ksecdd */
    if (KsecDeviceHandle != NULL)
    {
        /* Close it */
        CloseHandle(KsecDeviceHandle);
        KsecDeviceHandle = NULL;
    }
}

static
NTSTATUS
KsecDeviceIoControl(
    ULONG IoControlCode,
    PVOID InputBuffer,
    SIZE_T InputBufferLength,
    PVOID OutputBuffer,
    SIZE_T OutputBufferLength)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Check if we already have a handle */
    if (KsecDeviceHandle == NULL)
    {
        /* Try to open the device */
        Status = KsecOpenDevice();
        if (!NT_SUCCESS(Status))
        {
            //ERR("Failed to open handle to KsecDd driver!\n");
            return Status;
        }
    }

    /* Call the driver */
    Status = NtDeviceIoControlFile(KsecDeviceHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IoControlCode,
                                   InputBuffer,
                                   InputBufferLength,
                                   OutputBuffer,
                                   OutputBufferLength);

    return Status;
}

/*
   These functions have nearly identical prototypes to CryptProtectMemory and CryptUnprotectMemory,
   in crypt32.dll.
 */

/******************************************************************************
 * SystemFunction040   (ADVAPI32.@)
 *
 * MSDN documents this function as RtlEncryptMemory and declares it in ntsecapi.h.
 *
 * PARAMS
 *  memory [I/O] Pointer to memory to encrypt.
 *  length [I] Length of region to encrypt in bytes.
 *  flags  [I] Control whether other processes are able to decrypt the memory.
 *    RTL_ENCRYPT_OPTION_SAME_PROCESS
 *    RTL_ENCRYPT_OPTION_CROSS_PROCESS
 *    RTL_ENCRYPT_OPTION_SAME_LOGON
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: NTSTATUS error code
 *
 * NOTES
 *  length must be a multiple of RTL_ENCRYPT_MEMORY_SIZE.
 *  If flags are specified when encrypting, the same flag value must be given
 *  when decrypting the memory.
 */
NTSTATUS
WINAPI
SystemFunction040(
    _Inout_ PVOID Memory,
    _In_ ULONG MemoryLength,
    _In_ ULONG OptionFlags)
{
    ULONG IoControlCode;

    if (OptionFlags == RTL_ENCRYPT_OPTION_SAME_PROCESS)
    {
        IoControlCode = IOCTL_KSEC_ENCRYPT_SAME_PROCESS;
    }
    else if (OptionFlags == RTL_ENCRYPT_OPTION_CROSS_PROCESS)
    {
        IoControlCode = IOCTL_KSEC_ENCRYPT_CROSS_PROCESS;
    }
    else if (OptionFlags == RTL_ENCRYPT_OPTION_SAME_LOGON)
    {
        IoControlCode = IOCTL_KSEC_ENCRYPT_SAME_LOGON;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

	return KsecDeviceIoControl(IoControlCode, Memory, MemoryLength, Memory, MemoryLength);
}

/******************************************************************************
 * SystemFunction041  (ADVAPI32.@)
 *
 * MSDN documents this function as RtlDecryptMemory and declares it in ntsecapi.h.
 *
 * PARAMS
 *  memory [I/O] Pointer to memory to decrypt.
 *  length [I] Length of region to decrypt in bytes.
 *  flags  [I] Control whether other processes are able to decrypt the memory.
 *    RTL_ENCRYPT_OPTION_SAME_PROCESS
 *    RTL_ENCRYPT_OPTION_CROSS_PROCESS
 *    RTL_ENCRYPT_OPTION_SAME_LOGON
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: NTSTATUS error code
 *
 * NOTES
 *  length must be a multiple of RTL_ENCRYPT_MEMORY_SIZE.
 *  If flags are specified when encrypting, the same flag value must be given
 *  when decrypting the memory.
 */
NTSTATUS
WINAPI
SystemFunction041(
    _Inout_ PVOID Memory,
    _In_ ULONG MemoryLength,
    _In_ ULONG OptionFlags)
{
    ULONG IoControlCode;

    if (OptionFlags == RTL_ENCRYPT_OPTION_SAME_PROCESS)
    {
        IoControlCode = IOCTL_KSEC_DECRYPT_SAME_PROCESS;
    }
    else if (OptionFlags == RTL_ENCRYPT_OPTION_CROSS_PROCESS)
    {
        IoControlCode = IOCTL_KSEC_DECRYPT_CROSS_PROCESS;
    }
    else if (OptionFlags == RTL_ENCRYPT_OPTION_SAME_LOGON)
    {
        IoControlCode = IOCTL_KSEC_DECRYPT_SAME_LOGON;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

	return KsecDeviceIoControl(IoControlCode, Memory, MemoryLength, Memory, MemoryLength);
}

/* EOF */
