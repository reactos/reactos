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
    return buffer;
}

void
NtlmFree(
    IN PVOID Buffer,
    IN BOOL FromPrivateLsaHeap)
{
    if (Buffer)
    {
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

VOID
NtlmStructWriteStrA(
    IN void* dataStart,
    IN ULONG dataSize,
    OUT PCHAR* pDataFieldA,
    IN const char* dataA,
    IN OUT PBYTE* pOffset)
{
    int datalen = (strlen(dataA) + 1) * sizeof(char);
    if (*pOffset < (PBYTE)dataStart)
    {
        ERR("Invalid offset\n");
        return;
    }
    if (*pOffset + datalen > (PBYTE)dataStart + dataSize)
    {
        ERR("Out of bounds!\n");
        return;
    }

    memcpy(*pOffset, dataA, datalen);
    *pDataFieldA = (char*)*pOffset;
    *pOffset += datalen;
}

VOID
NtlmStructWriteStrW(
    IN void* dataStart,
    IN ULONG dataSize,
    OUT PWCHAR* pDataFieldW,
    IN const WCHAR* dataW,
    IN OUT PBYTE* pOffset)
{
    int datalen = (wcslen(dataW) + 1) * sizeof(WCHAR);
    if (*pOffset < (PBYTE)dataStart)
    {
        ERR("Invalid offset\n");
        return;
    }
    if (*pOffset + datalen > (PBYTE)dataStart + dataSize)
    {
        ERR("Out of bounds!\n");
        return;
    }

    memcpy(*pOffset, dataW, datalen);
    *pDataFieldW = (WCHAR*)*pOffset;
    *pOffset += datalen;
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
NtlmInit(
    _In_ NTLM_MODE mode)
{
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
