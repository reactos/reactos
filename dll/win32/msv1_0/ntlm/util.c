/*
 * Copyright 2011 Samuel Serapion
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <precomp.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

#define NTLM_ALLOC_TAG "NTLM"
#define NTLM_ALLOC_TAG_SIZE strlen(NTLM_ALLOC_TAG)

void*
NtlmAllocate(
    IN size_t Size,
    IN BOOL UsePrivateLsaHeap)
{
    PVOID buffer = NULL;

    if(Size == 0)
    {
        ERR("Allocating 0 bytes!\n");
        return NULL;
    }

    Size += NTLM_ALLOC_TAG_SIZE;

    switch(NtlmMode)
    {
        case NtlmLsaMode:
        {
            if (UsePrivateLsaHeap)
                buffer = LsaFunctions->AllocatePrivateHeap(Size);
            else
                buffer = LsaFunctions->AllocateLsaHeap(Size);

            if (buffer != NULL)
                RtlZeroMemory(buffer, Size);
            break;
        }
        case NtlmUserMode:
        {
            buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
            break;
        }
        default:
        {
            ERR("NtlmState unknown!\n");
            break;
        }
    }

    memcpy(buffer, NTLM_ALLOC_TAG, NTLM_ALLOC_TAG_SIZE);
    buffer = (PBYTE)buffer + NTLM_ALLOC_TAG_SIZE;

    return buffer;
}

void
NtlmFree(
    IN PVOID Buffer,
    IN BOOL FromPrivateLsaHeap)
{
    if (Buffer)
    {
        Buffer = (PBYTE)Buffer - NTLM_ALLOC_TAG_SIZE;
        ASSERT(memcmp(Buffer, NTLM_ALLOC_TAG, NTLM_ALLOC_TAG_SIZE) == 0);
        *(char*)Buffer = 'D';

        switch (NtlmMode)
        {
            case NtlmLsaMode:
            {
                if (FromPrivateLsaHeap)
                    LsaFunctions->FreePrivateHeap(Buffer);
                else
                    LsaFunctions->FreeLsaHeap(Buffer);
                break;
            }
            case NtlmUserMode:
            {
                HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, Buffer);
                break;
            }
            default:
            {
                ERR("NtlmState unknown!\n");
                break;
            }
        }
    }
    else
    {
        ERR("Trying to free NULL!\n");
    }
}

BOOLEAN
NtlmHasIntervalElapsed(IN LARGE_INTEGER Start,
                       IN LONG Timeout)
{
    LARGE_INTEGER now;
    LARGE_INTEGER elapsed;
    LARGE_INTEGER interval;

    /* timeout is never */
    if (Timeout > 0xffffffff)
        return FALSE;

    /* get current time */
    NtQuerySystemTime(&now);
    elapsed.QuadPart = now.QuadPart - Start.QuadPart;

    /* convert from milliseconds into 100ns */
    interval.QuadPart = Int32x32To64(Timeout, 10000);

    /* time overflowed or elapsed is greater than interval */
    if (elapsed.QuadPart < 0 || elapsed.QuadPart > interval.QuadPart )
        return TRUE;

    return FALSE;
}

/* check if loaded during system setup */
/* from base/services/umpnpmgr/umpnpmgr.c */
BOOL
SetupIsActive(VOID)
{
    HKEY hKey = NULL;
    DWORD regType, active, size;
    LONG rc;
    BOOL ret = FALSE;

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_QUERY_VALUE, &hKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    size = sizeof(DWORD);
    rc = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, &regType, (LPBYTE)&active, &size);
    if (rc != ERROR_SUCCESS)
        goto cleanup;
    if (regType != REG_DWORD || size != sizeof(DWORD))
        goto cleanup;

    ret = (active != 0);

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);

    TRACE("System setup in progress? %S\n", ret ? L"YES" : L"NO");

   return ret;
}

BOOLEAN
NtlmGetSecBuffer(IN OPTIONAL PSecBufferDesc pInputDesc,
                 IN ULONG BufferIndex,
                 OUT PSecBuffer *pOutBuffer,
                 IN BOOLEAN OutputToken)
{
    PSecBuffer Buffer;

    ASSERT(pOutBuffer != NULL);
    if (!pInputDesc)
    {
        *pOutBuffer = NULL;
        return TRUE;
    }

    /* check version */
    if (pInputDesc->ulVersion != SECBUFFER_VERSION)
        return FALSE;

    /* check how many buffers we have */
    if (BufferIndex >= pInputDesc->cBuffers)
        return FALSE;

    /* get buffer */
    Buffer = &pInputDesc->pBuffers[BufferIndex];

    /* detect a SECBUFFER_TOKEN */
    if ((Buffer->BufferType & (~SECBUFFER_READONLY)) == SECBUFFER_TOKEN)
    {
        /* detect read only buffer */
        if (OutputToken && (Buffer->BufferType & SECBUFFER_READONLY))
            return  FALSE;

        /* LSA server must map the user provided buffer into its address space */
        if(inLsaMode)
        {
            if (!NT_SUCCESS(LsaFunctions->MapBuffer(Buffer, Buffer)))
                return FALSE;
        }
        *pOutBuffer = Buffer;
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief returns a specific BufferType from an
 *        PSecBufferDesc struct.
 * @param pInputDesc
 * @param BufferType that should returned
 * @param BufferIndex if pInputDesc has more buffers of the same type
 *      you can choose the second(1) or third(2) and so on
 * @param OutputBuffer TRUE checks if the buffer is writeable
 * @param pOutBuffer returned buffer
 * @return
 */
BOOLEAN
NtlmGetSecBufferType(
    _In_ OPTIONAL PSecBufferDesc pInputDesc,
    _In_ ULONG BufferType,
    _In_ ULONG BufferIndex,
    _In_ BOOLEAN OutputBuffer,
    _Out_ PSecBuffer *pOutBuffer)
{
    int i;
    PSecBuffer Buffer;

    ASSERT(pOutBuffer != NULL);
    if (!pInputDesc)
    {
        *pOutBuffer = NULL;
        return TRUE;
    }

    // check version
    if (pInputDesc->ulVersion != SECBUFFER_VERSION)
        return FALSE;

    // check how many buffers we have */
    if (BufferIndex >= pInputDesc->cBuffers)
        return FALSE;

    *pOutBuffer = NULL;
    for (i = 0; i < pInputDesc->cBuffers; i++)
    {
        Buffer = &pInputDesc->pBuffers[i];
        // is buffer type is what we want?
        if ((Buffer->BufferType & (~SECBUFFER_ATTRMASK)) != BufferType)
            continue;
        // respect index
        if (BufferIndex == 0)
        {
            *pOutBuffer = Buffer;
            break;
        }
        BufferIndex--;
    }
    // if we need it for output we check for readonly.
    if (OutputBuffer && (Buffer->BufferType & SECBUFFER_READONLY))
        return FALSE;

    // LSA server must map the user provided buffer into its address space
    if ((*pOutBuffer) && (inLsaMode))
    {
        if (!NT_SUCCESS(LsaFunctions->MapBuffer(*pOutBuffer, *pOutBuffer)))
            return FALSE;
    }

    return (*pOutBuffer != NULL);
}

SECURITY_STATUS
NtlmBlobToExtStringRef(
  IN PSecBuffer InputBuffer,
  IN NTLM_BLOB Blob,
  IN OUT PEXT_STRING OutputStr)
{
    /* check blob is not beyond the bounds of the input buffer */
    if(Blob.Offset >= InputBuffer->cbBuffer ||
       Blob.Offset + Blob.Length > InputBuffer->cbBuffer)
    {
        ERR("blob points beyond buffer bounds!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* convert blob into a string */
    OutputStr->bAllocated = Blob.MaxLength;
    OutputStr->Buffer = ((PBYTE)InputBuffer->pvBuffer) + Blob.Offset;
    OutputStr->bUsed = Blob.Length;

    return SEC_E_OK;
}

SECURITY_STATUS
NtlmCopyBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    OUT PVOID dst,
    IN ULONG len)
{
    /* check blob is not beyond the bounds of the input buffer */
    if(Blob.Offset >= InputBuffer->cbBuffer ||
       Blob.Offset + Blob.Length > InputBuffer->cbBuffer)
    {
        ERR("blob points beyond buffer bounds!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* convert blob into a string */
    memcpy(dst, ((PBYTE)InputBuffer->pvBuffer) + Blob.Offset, len);
    return SEC_E_OK;
}

SECURITY_STATUS
NtlmCreateExtWStrFromBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PEXT_STRING_W OutputStr)
{
    PBYTE pData;

    if (Blob.Length == 0)
    {
        ExtWStrInit(OutputStr, NULL);
        return SEC_E_OK;
    }

    /* check blob is not beyond the bounds of the input buffer */
    if(Blob.Offset >= InputBuffer->cbBuffer ||
       Blob.Offset + Blob.Length > InputBuffer->cbBuffer)
    {
        ERR("blob points beyond buffer bounds!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* convert blob into a string */
    pData = ((PBYTE)InputBuffer->pvBuffer) + Blob.Offset;
    ExtWStrInit(OutputStr, NULL);
    ExtWStrSetN(OutputStr, (WCHAR*)pData, Blob.Length / sizeof(WCHAR));

    return SEC_E_OK;
}

SECURITY_STATUS
NtlmCreateExtAStrFromBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PEXT_STRING_A OutputStr)
{
    PBYTE pData;

    if (Blob.Length == 0)
    {
        ExtWStrInit(OutputStr, NULL);
        return SEC_E_OK;
    }

    /* check blob is not beyond the bounds of the input buffer */
    if(Blob.Offset >= InputBuffer->cbBuffer ||
       Blob.Offset + Blob.Length > InputBuffer->cbBuffer)
    {
        ERR("blob points beyond buffer bounds!\n");
        return SEC_E_INVALID_TOKEN;
    }

    /* convert blob into a string */
    pData = ((PBYTE)InputBuffer->pvBuffer) + Blob.Offset;
    ExtAStrInit(OutputStr, NULL);
    ExtAStrSetN(OutputStr, (char*)pData, Blob.Length / sizeof(char));

    return SEC_E_OK;
}

NTSTATUS
NtlmUStrAllocAndCopyBlob(
    IN PSecBuffer InputBuffer,
    IN PNTLM_BLOB Blob,
    IN OUT PUNICODE_STRING OutputStr)
{
    PBYTE pData;

    if (Blob->Length == 0)
    {
        RtlInitUnicodeString(OutputStr, NULL);
        return STATUS_SUCCESS;
    }

    /* check blob is not beyond the bounds of the input buffer */
    if(Blob->Offset >= InputBuffer->cbBuffer ||
       Blob->Offset + Blob->Length > InputBuffer->cbBuffer)
    {
        ERR("blob points beyond buffer bounds!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* convert blob into a string */
    pData = ((PBYTE)InputBuffer->pvBuffer) + Blob->Offset;
    OutputStr->Length = Blob->Length;
    OutputStr->MaximumLength = Blob->Length + sizeof(char);
    OutputStr->Buffer = (PWCHAR)NtlmAllocate(OutputStr->MaximumLength, FALSE);
    if (OutputStr->Buffer == NULL)
    {
        ERR("Cannot allocate memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(OutputStr->Buffer, pData, OutputStr->Length);
    OutputStr->Buffer[OutputStr->Length] = '\0';

    return STATUS_SUCCESS;
}
// replacement for NtlmCreateExtAStrFromBlob
NTSTATUS
NtlmAStrAllocAndCopyBlob(
    IN PSecBuffer InputBuffer,
    IN PNTLM_BLOB Blob,
    IN OUT PSTRING OutputStr)
{
    PBYTE pData;

    if (Blob->Length == 0)
    {
        RtlInitString(OutputStr, NULL);
        return STATUS_SUCCESS;
    }

    /* check blob is not beyond the bounds of the input buffer */
    if(Blob->Offset >= InputBuffer->cbBuffer ||
       Blob->Offset + Blob->Length > InputBuffer->cbBuffer)
    {
        ERR("blob points beyond buffer bounds!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* convert blob into a string */
    pData = ((PBYTE)InputBuffer->pvBuffer) + Blob->Offset;
    OutputStr->Length = Blob->Length;
    OutputStr->MaximumLength = Blob->Length + sizeof(char);
    OutputStr->Buffer = (PCHAR)NtlmAllocate(OutputStr->MaximumLength, FALSE);
    if (OutputStr->Buffer == NULL)
    {
        ERR("Cannot allocate memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(OutputStr->Buffer, pData, OutputStr->Length);
    OutputStr->Buffer[OutputStr->Length] = '\0';

    return STATUS_SUCCESS;
}

BOOL
NtlmAStrAlloc(
    IN OUT PSTRING Dst,
    IN size_t SizeInBytes)
{
    Dst->Length = 0;
    Dst->MaximumLength = SizeInBytes;
    Dst->Buffer = NtlmAllocate(SizeInBytes, FALSE);
    return (Dst->Buffer != NULL);
}

BOOL
NtlmUStrAlloc(
    IN OUT PUNICODE_STRING Dst,
    IN size_t SizeInBytes)
{
    Dst->Length = 0;
    Dst->MaximumLength = SizeInBytes;
    Dst->Buffer = NtlmAllocate(SizeInBytes, FALSE);
    return (Dst->Buffer != NULL);
}

VOID
NtlmUStrFree(
    IN PUNICODE_STRING String)
{
    if ((String == NULL) ||
        (String->Buffer == NULL) ||
        (String->MaximumLength == 0))
        return;
    NtlmFree(String->Buffer, FALSE);
    String->Buffer = NULL;
    String->MaximumLength = 0;
}

VOID
NtlmAStrFree(
    IN PSTRING String)
{
    if ((String == NULL) ||
        (String->Buffer == NULL) ||
        (String->MaximumLength == 0))
        return;
    NtlmFree(String->Buffer, FALSE);
    String->Buffer = NULL;
    String->MaximumLength = 0;
}

VOID
NtlmWriteToBlob(IN PVOID pOutputBuffer,
                IN void* buffer,
                IN ULONG len,
                IN OUT PNTLM_BLOB OutputBlob,
                IN OUT PULONG_PTR OffSet)
{
    /* Handle NULL value */
    if ((buffer == NULL) || (len == 0))
    {
        OutputBlob->Length = 0;
        OutputBlob->MaxLength = 0;
        OutputBlob->Offset = (ULONG)(*OffSet - (ULONG_PTR)pOutputBuffer);
        return;
    }

    /* copy string to target location */
    memcpy((PVOID)*OffSet, buffer, len);

    /* set blob fields */
    OutputBlob->Length = OutputBlob->MaxLength = len;
    OutputBlob->Offset = (ULONG)(*OffSet - (ULONG_PTR)pOutputBuffer);

    /* move the offset to the end of the string we just copied */
    *OffSet += len;
}

VOID
NtlmAppendToBlob(IN void* buffer,
                 IN ULONG len,
                 IN OUT PNTLM_BLOB OutputBlob,
                 IN OUT PULONG_PTR OffSet)
{
    /* copy string to target location */
    memcpy((PVOID)*OffSet, buffer, len);

    /* set blob fields */
    OutputBlob->Length += len;
    OutputBlob->MaxLength += len;

    /* move the offset to the end of the string we just copied */
    *OffSet += len;
}

VOID
NtlmExtStringToBlob(IN PVOID OutputBuffer,
                    IN PEXT_STRING InStr,
                    IN OUT PNTLM_BLOB OutputBlob,
                    IN OUT PULONG_PTR OffSet)
{
    /*  Handle NULL value */
    if (!InStr)
    {
        OutputBlob->Length = 0;
        OutputBlob->MaxLength = 0;
        OutputBlob->Offset = (ULONG)(*OffSet - (ULONG_PTR)OutputBuffer);
        return;
    }

    /* copy string to target location */
    if(InStr->Buffer)
        memcpy((PVOID)*OffSet, InStr->Buffer, InStr->bUsed);

    /* set blob fields */
    OutputBlob->Length = OutputBlob->MaxLength = InStr->bUsed;
    OutputBlob->Offset = (ULONG)(*OffSet - (ULONG_PTR)OutputBuffer);

    /* move the offset to the end of the string we just copied */
    *OffSet += InStr->bUsed;
}

/**
 * For a description lool to W version of this function.
 */

__declspec(noinline)
BOOL
NtlmStructWriteStrA(
    IN PVOID DataStart,
    IN ULONG DataSize,
    OUT PCHAR* DstDataAPtr,
    IN const char* SrcDataA,
    IN ULONG SrcDataLen,
    IN OUT PBYTE* AbsoluteOffsetPtr,
    IN BOOL TerminateWith0)
{
    ULONG SrcDataMaxLen;

    if (SrcDataLen == 0)
        SrcDataLen = strlen(SrcDataA);

    SrcDataMaxLen = SrcDataLen;
    if (TerminateWith0)
        SrcDataMaxLen += sizeof(char);

    if (*AbsoluteOffsetPtr < (PBYTE)DataStart)
    {
        ERR("Invalid offset\n");
        return FALSE;
    }
    if (*AbsoluteOffsetPtr + SrcDataMaxLen > (PBYTE)DataStart + DataSize)
    {
        ERR("Out of bounds!\n");
        return FALSE;
    }

    memcpy(*AbsoluteOffsetPtr, SrcDataA, SrcDataLen);
    *DstDataAPtr = (char*)*AbsoluteOffsetPtr;
    if (TerminateWith0)
        (*DstDataAPtr)[SrcDataLen / sizeof(char)] = 0;
    *AbsoluteOffsetPtr += SrcDataMaxLen;
    return TRUE;
}

/**
 * @brief Helper to fill a WCHAR-String in a struct.
 *        The stringdata is append to the struct. The
 *        function does not allocate memory.
 * @param DataStart start addres of the struct
 * @param DataSize size of allocated memory (including payload)
 * @param WriteRelativeOffset If TRUE the address which will be written
 *            to pDataFieldW will be relative to DataStart.
 *            If FALSE it will be an absolut address.
 * @param DataFieldW  Pointer to the WCHAR* datafield. The adress
 *            of the data will be written to it.
 * @param SrcDataW Data to write/append at pOffset (payload). pOffset
 *            will be increased after writing data.
 * @param SrcDataLen if 0 it will be autodetected by assuming a
 *            0-terminating string.
 *            SrcDataLen is the length in bytes without terminator.
 * @param AbsoluteOffset Current absolute offset. Will be increased by
 *            data length.
 * @return FALSE if something went wrong
 */
BOOL
NtlmStructWriteStrW(
    IN PVOID DataStart,
    IN ULONG DataSize,
    OUT PWCHAR* DstDataWPtr,
    IN const WCHAR* SrcDataW,
    IN ULONG SrcDataLen,
    IN OUT PBYTE* AbsoluteOffsetPtr,
    IN BOOL TerminateWith0)
{
    ULONG SrcDataMaxLen;

    if (SrcDataLen == 0)
        SrcDataLen = wcslen(SrcDataW) * sizeof(WCHAR);

    SrcDataMaxLen = SrcDataLen;
    if (TerminateWith0)
        SrcDataMaxLen += sizeof(WCHAR);

    if (*AbsoluteOffsetPtr < (PBYTE)DataStart)
    {
        ERR("Invalid offset\n");
        return FALSE;
    }
    if (*AbsoluteOffsetPtr + SrcDataMaxLen > (PBYTE)DataStart + DataSize)
    {
        ERR("Out of bounds!\n");
        return FALSE;
    }

    memcpy(*AbsoluteOffsetPtr, SrcDataW, SrcDataLen);
    *DstDataWPtr = (WCHAR*)*AbsoluteOffsetPtr;
    if (TerminateWith0)
        (*DstDataWPtr)[SrcDataLen / sizeof(WCHAR)] = 0;
    *AbsoluteOffsetPtr += SrcDataMaxLen;

    return TRUE;
}

BOOL
NtlmUStrWriteToStruct(
    IN PVOID DataStart,
    IN ULONG DataSize,
    OUT PUNICODE_STRING DstData,
    IN const PUNICODE_STRING SrcData,
    IN OUT PBYTE* AbsoluteOffsetPtr,
    IN BOOL TerminateWith0)
{
    if (!NtlmStructWriteStrW(
        DataStart, DataSize,
        &DstData->Buffer,
        SrcData->Buffer, SrcData->Length,
        AbsoluteOffsetPtr,
        TerminateWith0))
        return FALSE;

    DstData->Length = SrcData->Length;
    DstData->MaximumLength = SrcData->Length;
    if (TerminateWith0)
        SrcData->MaximumLength += sizeof(WCHAR);

    return TRUE;
}

BOOL
NtlmAStrWriteToStruct(
    IN PVOID DataStart,
    IN ULONG DataSize,
    OUT PSTRING DstData,
    IN const PSTRING SrcData,
    IN OUT PBYTE* AbsoluteOffsetPtr,
    IN BOOL TerminateWith0)
{
    if (!NtlmStructWriteStrA(
        DataStart, DataSize,
        &DstData->Buffer,
        SrcData->Buffer, SrcData->Length,
        AbsoluteOffsetPtr,
        TerminateWith0))
        return FALSE;

    DstData->Length = SrcData->Length;
    DstData->MaximumLength = SrcData->Length;
    if (TerminateWith0)
        DstData->MaximumLength += sizeof(char);

    return TRUE;
}

PVOID
StrUtilAlloc(
    IN size_t Size)
{
    return NtlmAllocate(Size, FALSE);
}

VOID
StrUtilFree(
    IN PVOID Buffer)
{
    NtlmFree(Buffer, FALSE);
}

VOID
NtlmInitExtStrWFromUnicodeString(
    OUT PEXT_STRING_W Dest,
    IN PUNICODE_STRING Src,
    IN BOOLEAN SrcSetToNULL)
{
    Dest->bUsed = Src->Length;
    Dest->bAllocated = Src->MaximumLength;
    Dest->Buffer = (PBYTE)Src->Buffer;
    if (SrcSetToNULL)
    {
        Src->Length = 0;
        Src->MaximumLength = 0;
        Src->Buffer = NULL;
    }
}

VOID
NtlmInitUnicodeStringFromExtStrW(
    OUT PUNICODE_STRING Dest,
    IN PEXT_STRING_W Src,
    IN BOOLEAN SrcSetToNULL)
{
    Dest->Length = Src->bUsed;
    Dest->MaximumLength = Src->bAllocated;
    Dest->Buffer = (PWCHAR)Src->Buffer;
    if (SrcSetToNULL)
    {
        Src->bUsed = 0;
        Src->bAllocated = 0;
        Src->Buffer = NULL;
    }
}

VOID
NtlmInit(
    _In_ NTLM_MODE mode)
{
    __wine_dbch_ntlm.flags = 0xff;

    if (NtlmMode != NtlmUnknownMode)
    {
        if (mode != NtlmMode)
        {
            WARN("Initializing in different mode ... skipping!");
            return;
        }
    }
    NtlmMode = mode;

    init_strutil(StrUtilAlloc, StrUtilFree);
    NtlmInitializeGlobals();
    NtlmInitializeRNG();
    NtlmInitializeProtectedMemory();
    NtlmCredentialInitialize();
    NtlmContextInitialize();
    if (NtlmMode == NtlmUserMode)
    {
        NtlmUsrContextInitialize();
    }

}

VOID
NtlmFini(VOID)
{
    NtlmContextTerminate();
    NtlmCredentialTerminate();
    NtlmTerminateRNG();
    NtlmTerminateProtectedMemory();
    NtlmTerminateGlobals();
}

NTSTATUS
NtStatusToSecStatus(
    IN SECURITY_STATUS SecStatus)
{
    //FIXME
    return SecStatus;
}

/* SAM Helpers * /

// TODO NtlmSamGetUserHandle / NtlmSamCloseUserHandle
// THIS code is copied from LogonUserEx2. If we integrate
// it to master we should check if we can replace the copied
// code in LogonUserEx2 with a call to NtlmSamGetUserHandle and
// NtlmSamCloseUserHandle.
// This functions is used by protocol.c:SvrRetrieveUserPwdHashs
NTSTATUS
NtlmSamOpenUser(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING UserDom,
    OUT PNTLM_SAM_HANDLES SamHandles)
{
    NTSTATUS Status;
    PRPC_SID AccountDomainSid;
    RPC_UNICODE_STRING Names[1];
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};

    SamHandles->ServerHandle = NULL;
    SamHandles->DomainHandle = NULL;
    SamHandles->UserHandle = NULL;

    / * Get the account domain SID * /
    Status = GetAccountDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetAccountDomainSid() failed (Status 0x%lx)\n", Status);
        return Status;
    }

    / * Connect to the SAM server * /
    Status = SamIConnect(NULL,
                         &SamHandles->ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamIConnect() failed (Status 0x%lx)\n", Status);
        goto done;
    }

    / * Open the account domain * /
    Status = SamrOpenDomain(SamHandles->ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &SamHandles->DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrOpenDomain failed (Status 0x%lx)\n", Status);
        goto done;
    }

    Names[0].Length = UserName->Length;
    Names[0].MaximumLength = UserName->MaximumLength;
    Names[0].Buffer = UserName->Buffer;

    / * Try to get the RID for the user name * /
    Status = SamrLookupNamesInDomain(SamHandles->DomainHandle,
                                     1,
                                     Names,
                                     &RelativeIds,
                                     &Use);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrLookupNamesInDomain failed (Status 0x%lx)\n", Status);
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    / * Fail, if it is not a user account * /
    if (Use.Element[0] != SidTypeUser)
    {
        ERR("Account is not a user account!\n");
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    / * Open the user object * /
    Status = SamrOpenUser(SamHandles->DomainHandle,
                          USER_READ_GENERAL | USER_READ_LOGON |
                          USER_READ_ACCOUNT | USER_READ_PREFERENCES, / * FIXME * /
                          RelativeIds.Element[0],
                          &SamHandles->UserHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamrOpenUser failed (Status 0x%lx)\n", Status);
        goto done;
    }
done:
    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);
    if (!NT_SUCCESS(Status))
        NtlmSamCloseUserHandle(FALSE, 0, SamHandles);
    return Status;
}

NTSTATUS
NtlmSamCloseUserHandle(
    IN BOOLEAN UpdateUserLogonState,
    IN NTSTATUS LogonStatus,
    IN PNTLM_SAM_HANDLES SamHandles)
{
    if ((UpdateUserLogonState) &&
        (SamHandles->UserHandle != NULL) &&
        (LogonStatus == STATUS_SUCCESS || LogonStatus == STATUS_WRONG_PASSWORD))
    {
        SAMPR_USER_INFO_BUFFER InternalInfo;

        RtlZeroMemory(&InternalInfo, sizeof(InternalInfo));

        if (LogonStatus == STATUS_SUCCESS)
            InternalInfo.Internal2.Flags = USER_LOGON_SUCCESS;
        else
            InternalInfo.Internal2.Flags = USER_LOGON_BAD_PASSWORD;

        SamrSetInformationUser(SamHandles->UserHandle,
                               UserInternal2Information,
                               &InternalInfo);
    }

    if (SamHandles->UserHandle != NULL)
        SamrCloseHandle(SamHandles->UserHandle);
    if (SamHandles->ServerHandle != NULL)
        SamrCloseHandle(SamHandles->ServerHandle);
    if (SamHandles->DomainHandle != NULL)
        SamrCloseHandle(SamHandles->DomainHandle);
    SamHandles->ServerHandle = NULL;
    SamHandles->DomainHandle = NULL;
    SamHandles->UserHandle = NULL;

    return STATUS_SUCCESS;
}*/

BOOL
NtlmFixupAndValidateUStr(
    IN OUT PUNICODE_STRING String,
    IN ULONG_PTR FixupOffset)
{
    NTSTATUS Status;

    if (String->Length)
    {
        String->Buffer = FIXUP_POINTER(String->Buffer, FixupOffset);
        String->MaximumLength = String->Length;
    }
    else
    {
        String->Buffer = NULL;
        String->MaximumLength = 0;
    }

    Status = RtlValidateUnicodeString(0, String);
    return NT_SUCCESS(Status);
}

BOOL
NtlmFixupAStr(
    IN OUT PSTRING String,
    IN ULONG_PTR FixupOffset)
{
    if (String->Length)
    {
        String->Buffer = (PCHAR)FIXUP_POINTER(String->Buffer, FixupOffset);
        String->MaximumLength = String->Length;
    }
    else
    {
        String->Buffer = NULL;
        String->MaximumLength = 0;
    }

    return TRUE;
}

NTSTATUS
NtlmAllocateClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG BufferLength,
    IN OUT PNTLM_CLIENT_BUFFER Buffer)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Buffer)
        return STATUS_NO_MEMORY;

    Buffer->LocalBuffer = NtlmAllocate(BufferLength, FALSE);
    if (!Buffer->LocalBuffer)
        return STATUS_NO_MEMORY;

    if ((HANDLE)ClientRequest == INVALID_HANDLE_VALUE)
    {
        Buffer->ClientBaseAddress = Buffer->LocalBuffer;
        //if (!ClientBaseAddress)
        //    return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = DispatchTable.AllocateClientBuffer(ClientRequest,
                                                    BufferLength,
                                                    &Buffer->ClientBaseAddress);
        if (!NT_SUCCESS(Status))
        {
            NtlmFree(Buffer->LocalBuffer, FALSE);
            Buffer->LocalBuffer == NULL;
        }
        //FIXME: Maybe we have to free ClientBaseAddress if something
        //       goes wrong ...? I'm not sure about that ...
    }
    return Status;
}

NTSTATUS
NtlmCopyToClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN ULONG BufferLength,
    IN OUT PNTLM_CLIENT_BUFFER Buffer)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ((HANDLE)ClientRequest == INVALID_HANDLE_VALUE)
    {
        // If ClientRequest ist INVALID_HANDLE_VALUE
        // Buffer->LocalBuffer == Buffer->ClientBaseAddress
        if (Buffer->ClientBaseAddress != Buffer->LocalBuffer)
        {
            ERR("Buffer->ClientBaseAddress != Buffer->LocalBuffer (something must be wrong!)\n");
            return STATUS_INTERNAL_ERROR;
        }
    }
    else
    {
        if (!Buffer->ClientBaseAddress ||
            !Buffer->LocalBuffer)
        {
            ERR("Invalid Buffer - not allocated!\n");
            return STATUS_NO_MEMORY;
        }
        Status = DispatchTable.CopyToClientBuffer(ClientRequest,
                                                  BufferLength,
                                                  Buffer->ClientBaseAddress,
                                                  Buffer->LocalBuffer);
    }
    return Status;
}

VOID
NtlmFreeClientBuffer(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN BOOL FreeClientBuffer,
    IN OUT PNTLM_CLIENT_BUFFER Buffer)
{
    if (!Buffer->ClientBaseAddress)
        return;

    if ((HANDLE)ClientRequest == INVALID_HANDLE_VALUE)
    {
        if (Buffer->ClientBaseAddress != Buffer->LocalBuffer)
        {
            ERR("Buffer->ClientBaseAddress != Buffer->LocalBuffer (something must be wrong!)\n");
            return;
        }
        // LocalBuffer and ClientBaseAddress is the same
        // so we have only to free it if FreeClientBuffer is TRUE.
        Buffer->LocalBuffer == NULL;
        if (FreeClientBuffer)
        {
            NtlmFree(Buffer->ClientBaseAddress, FALSE);
            Buffer->ClientBaseAddress == NULL;
        }
    }
    else
    {
        NtlmFree(Buffer->LocalBuffer, FALSE);
        Buffer->LocalBuffer == NULL;
        if (FreeClientBuffer)
            DispatchTable.FreeClientBuffer(ClientRequest,
                                           Buffer->ClientBaseAddress);
        Buffer->ClientBaseAddress == NULL;
    }
}
