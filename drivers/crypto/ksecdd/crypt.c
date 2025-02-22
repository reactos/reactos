/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

MD5_CTX KsecLoadTimeStartMd5s[2];
DES3_KEY KsecGlobalDes3Key;
AES_KEY KsecGlobalAesKey;

typedef struct _KSEC_PROCESS_DATA
{
    PEPROCESS Process;
    HANDLE ProcessId;
    LONGLONG CreateTime;
    ULONG_PTR DirectoryTableBase;
} KSEC_PROCESS_DATA, *PKSEC_PROCESS_DATA;

typedef struct _KSEC_LOGON_DATA
{
    LUID LogonId;
} KSEC_LOGON_DATA, *PKSEC_LOGON_DATA;

#if 0
void PrintKeyData(PUCHAR KeyData)
{
    ULONG i;
    for (i = 0; i < 32; i++)
    {
        DbgPrint("%02X", KeyData[i]);
    }
    DbgPrint("\n");
}
#endif

VOID
NTAPI
KsecInitializeEncryptionSupport (
    VOID)
{
    KSEC_ENTROPY_DATA EntropyData;
    MD5_CTX Md5Context;
    UCHAR KeyDataBuffer[32];

    KsecGatherEntropyData(&EntropyData);
    MD5Init(&Md5Context);
    MD5Update(&Md5Context, (PVOID)&EntropyData, sizeof(EntropyData));
    KsecLoadTimeStartMd5s[0] = Md5Context;
    MD5Final(&Md5Context);
    RtlCopyMemory(KeyDataBuffer, &Md5Context.digest, 16);

    KsecGatherEntropyData(&EntropyData);
    Md5Context = KsecLoadTimeStartMd5s[0];
    MD5Update(&Md5Context, (PVOID)&EntropyData, sizeof(EntropyData));
    KsecLoadTimeStartMd5s[1] = Md5Context;
    MD5Final(&Md5Context);
    RtlCopyMemory(&KeyDataBuffer[16], &Md5Context.digest, 16);

    /* Create the global keys */
    aes_setup(KeyDataBuffer, 32, 0, &KsecGlobalAesKey);
    des3_setup(KeyDataBuffer, 24, 0, &KsecGlobalDes3Key);

    /* Erase the temp data */
    RtlSecureZeroMemory(KeyDataBuffer, sizeof(KeyDataBuffer));
    RtlSecureZeroMemory(&Md5Context, sizeof(Md5Context));
}

static
VOID
KsecGetKeyData (
    _Out_ UCHAR KeyData[32],
    _In_ ULONG OptionFlags)
{
    MD5_CTX Md5Contexts[2];
    KSEC_PROCESS_DATA ProcessData;
    KSEC_LOGON_DATA LogonData;
    PEPROCESS CurrentProcess;
    PACCESS_TOKEN Token;

    /* We need to generate the key, start with our load MD5s */
    Md5Contexts[0] = KsecLoadTimeStartMd5s[0];
    Md5Contexts[1] = KsecLoadTimeStartMd5s[1];

    /* Get the current process */
    CurrentProcess = PsGetCurrentProcess();

    if (OptionFlags == RTL_ENCRYPT_OPTION_SAME_PROCESS)
    {
        /* Hash some process specific data to generate the key */
        RtlZeroMemory(&ProcessData, sizeof(ProcessData));
        ProcessData.Process = CurrentProcess;
        ProcessData.ProcessId = CurrentProcess->UniqueProcessId;
        ProcessData.CreateTime = PsGetProcessCreateTimeQuadPart(CurrentProcess);
        ProcessData.DirectoryTableBase = CurrentProcess->Pcb.DirectoryTableBase[0];
        MD5Update(&Md5Contexts[0], (PVOID)&ProcessData, sizeof(ProcessData));
        MD5Update(&Md5Contexts[1], (PVOID)&ProcessData, sizeof(ProcessData));
    }
    else if (OptionFlags == RTL_ENCRYPT_OPTION_SAME_LOGON)
    {
        /* Hash the logon id to generate the key */
        RtlZeroMemory(&LogonData, sizeof(LogonData));
        Token = PsReferencePrimaryToken(CurrentProcess);
        SeQueryAuthenticationIdToken(Token, &LogonData.LogonId);
        PsDereferencePrimaryToken(Token);
        MD5Update(&Md5Contexts[0], (PVOID)&LogonData, sizeof(LogonData));
        MD5Update(&Md5Contexts[1], (PVOID)&LogonData, sizeof(LogonData));
    }
    else if (OptionFlags == RTL_ENCRYPT_OPTION_CROSS_PROCESS)
    {
        /* Use the original MD5s to generate the global key */
        NOTHING;
    }
    else
    {
        /* Must not pass anything else */
        ASSERT(FALSE);
    }

    /* Finalize the MD5s */
    MD5Final(&Md5Contexts[0]);
    MD5Final(&Md5Contexts[1]);

    /* Copy the md5 data */
    RtlCopyMemory(KeyData, &Md5Contexts[0].digest, 16);
    RtlCopyMemory((PUCHAR)KeyData + 16, &Md5Contexts[1].digest, 16);

    /* Erase the temp data */
    RtlSecureZeroMemory(&Md5Contexts, sizeof(Md5Contexts));
}

static
VOID
KsecGetDes3Key (
    _Out_ PDES3_KEY Des3Key,
    _In_ ULONG OptionFlags)
{
    UCHAR KeyDataBuffer[32];

    /* Check if the caller allows cross process encryption */
    if (OptionFlags == RTL_ENCRYPT_OPTION_CROSS_PROCESS)
    {
        /* Return our global cached DES3 key */
        *Des3Key = KsecGlobalDes3Key;
    }
    else
    {
        /* Setup the key */
        KsecGetKeyData(KeyDataBuffer, OptionFlags);
        des3_setup(KeyDataBuffer, 24, 0, Des3Key);

        /* Erase the temp data */
        RtlSecureZeroMemory(KeyDataBuffer, sizeof(KeyDataBuffer));
    }
}

static
VOID
KsecGetAesKey (
    _Out_ PAES_KEY AesKey,
    _In_ ULONG OptionFlags)
{
    UCHAR KeyDataBuffer[32];

    /* Check if the caller allows cross process encryption */
    if (OptionFlags == RTL_ENCRYPT_OPTION_CROSS_PROCESS)
    {
        /* Return our global cached AES key */
        *AesKey = KsecGlobalAesKey;
    }
    else
    {
        /* Setup the key */
        KsecGetKeyData(KeyDataBuffer, OptionFlags);
        aes_setup(KeyDataBuffer, 32, 0, AesKey);

        /* Erase the temp data */
        RtlSecureZeroMemory(KeyDataBuffer, sizeof(KeyDataBuffer));
    }
}

static
VOID
KsecEncryptMemoryDes3 (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    UCHAR EncryptedBlockData[8];
    DES3_KEY Des3Key;

    /* Get they triple DES key */
    KsecGetDes3Key(&Des3Key, OptionFlags);

    /* Do the triple DES encryption */
    while (Length >= sizeof(EncryptedBlockData))
    {
        des3_ecb_encrypt(Buffer, EncryptedBlockData, &Des3Key);
        RtlCopyMemory(Buffer, EncryptedBlockData, sizeof(EncryptedBlockData));
        Buffer = (PUCHAR)Buffer + sizeof(EncryptedBlockData);
        Length -= sizeof(EncryptedBlockData);
    }

    /* Erase the key data */
    RtlSecureZeroMemory(&Des3Key, sizeof(Des3Key));
}

static
VOID
KsecDecryptMemoryDes3 (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    UCHAR BlockData[8];
    DES3_KEY Des3Key;

    /* Get they triple DES key */
    KsecGetDes3Key(&Des3Key, OptionFlags);

    /* Do the triple DES decryption */
    while (Length >= sizeof(BlockData))
    {
        des3_ecb_decrypt(Buffer, BlockData, &Des3Key);
        RtlCopyMemory(Buffer, BlockData, sizeof(BlockData));
        Buffer = (PUCHAR)Buffer + sizeof(BlockData);
        Length -= sizeof(BlockData);
    }

    /* Erase the key data */
    RtlSecureZeroMemory(&Des3Key, sizeof(Des3Key));
}

static
VOID
KsecEncryptMemoryAes (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    UCHAR EncryptedBlockData[16];
    AES_KEY AesKey;

    /* Get they AES key */
    KsecGetAesKey(&AesKey, OptionFlags);

    /* Do the AES encryption */
    while (Length >= sizeof(EncryptedBlockData))
    {
        aes_ecb_encrypt(Buffer, EncryptedBlockData, &AesKey);
        RtlCopyMemory(Buffer, EncryptedBlockData, sizeof(EncryptedBlockData));
        Buffer = (PUCHAR)Buffer + sizeof(EncryptedBlockData);
        Length -= sizeof(EncryptedBlockData);
    }

    /* Erase the key data */
    RtlSecureZeroMemory(&AesKey, sizeof(AesKey));
}

static
VOID
KsecDecryptMemoryAes (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    UCHAR BlockData[16];
    AES_KEY AesKey;

    /* Get they AES key */
    KsecGetAesKey(&AesKey, OptionFlags);

    /* Do the AES decryption */
    while (Length >= sizeof(BlockData))
    {
        aes_ecb_decrypt(Buffer, BlockData, &AesKey);
        RtlCopyMemory(Buffer, BlockData, sizeof(BlockData));
        Buffer = (PUCHAR)Buffer + sizeof(BlockData);
        Length -= sizeof(BlockData);
    }

    /* Erase the key data */
    RtlSecureZeroMemory(&AesKey, sizeof(AesKey));
}

NTSTATUS
NTAPI
KsecEncryptMemory (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    /* Validate parameter */
    if (OptionFlags > RTL_ENCRYPT_OPTION_SAME_LOGON)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if the length is not 16 bytes aligned */
    if (Length & 15)
    {
        /* Is it at least 8 bytes aligned? */
        if (Length & 7)
        {
            /* No, we can't deal with it! */
            return STATUS_INVALID_PARAMETER;
        }

        /* Use triple DES encryption */
        KsecEncryptMemoryDes3(Buffer, Length, OptionFlags);
    }
    else
    {
        /* Use AES encryption */
        KsecEncryptMemoryAes(Buffer, Length, OptionFlags);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KsecDecryptMemory (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags)
{
    /* Validate parameter */
    if (OptionFlags > RTL_ENCRYPT_OPTION_SAME_LOGON)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if the length is not 16 bytes aligned */
    if (Length & 15)
    {
        /* Is it at least 8 bytes aligned? */
        if (Length & 7)
        {
            /* No, we can't deal with it! */
            return STATUS_INVALID_PARAMETER;
        }

        /* Use triple DES encryption */
        KsecDecryptMemoryDes3(Buffer, Length, OptionFlags);
    }
    else
    {
        /* Use AES encryption */
        KsecDecryptMemoryAes(Buffer, Length, OptionFlags);
    }

    return STATUS_SUCCESS;
}
