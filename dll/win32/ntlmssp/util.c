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

#include "ntlmssp.h"
#include "protocol.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

void*
NtlmAllocate(
    IN size_t Size)
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
            buffer = NtlmLsaFuncTable->AllocateLsaHeap(Size);
            if (buffer != NULL)
                RtlZeroMemory(buffer, Size);
            break;
        case NtlmUserMode:
            buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
            break;
        default:
            ERR("NtlmState unknown!\n");
            break;
    }
    return buffer;
}

void
NtlmFree(
    IN void* Buffer)
{
    if (Buffer)
    {
        switch (NtlmMode)
        {
            case NtlmLsaMode:
                NtlmLsaFuncTable->FreeLsaHeap(Buffer);
                break;
            case NtlmUserMode:
                HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, Buffer);
                break;
            default:
                ERR("NtlmState unknown!\n");
                break;
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
    if(pInputDesc->cBuffers < BufferIndex)
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
             if (!NT_SUCCESS(NtlmLsaFuncTable->MapBuffer(Buffer, Buffer)))
                 return FALSE;
         }
         *pOutBuffer = Buffer;
         return TRUE;
     }
    return FALSE;
}

SECURITY_STATUS
NtlmBlobToExtStringRef(IN PSecBuffer InputBuffer,
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
NtlmCreateExtWStrFromBlob(
    IN PSecBuffer InputBuffer,
    IN NTLM_BLOB Blob,
    IN OUT PEXT_STRING_W OutputStr)
{
    PBYTE pData;

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
    /* Handle NULL value */
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
NtlmWriteDataBufToBlob(
    IN PVOID OutputBuffer,
    IN PNTLM_DATABUF pDataBuf,
    IN OUT PNTLM_BLOB OutputBlob,
    IN OUT PULONG_PTR OffSet)
{
    if (pDataBuf)
        NtlmWriteToBlob(OutputBuffer, pDataBuf->pData, pDataBuf->bUsed, OutputBlob, OffSet);
    else
        NtlmWriteToBlob(OutputBuffer, NULL, 0, OutputBlob, OffSet);
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

BOOL
NtlmDataBufAlloc(
    IN PNTLM_DATABUF pAvData,
    IN ULONG initlen,
    IN BOOL doZeroMem)
{
    pAvData->pData = NtlmAllocate(initlen);
    if (pAvData == NULL)
        return FALSE;
    pAvData->bUsed = 0;
    pAvData->bAllocated = initlen;
    if (doZeroMem)
        RtlZeroMemory(pAvData->pData, initlen);
    return TRUE;
}

void
NtlmDataBufFree(
    IN OUT PNTLM_DATABUF pAvData)
{
    if ((pAvData->pData == NULL) ||
        (pAvData->bAllocated == 0))
        return;
    NtlmFree(pAvData->pData);
    pAvData->pData = NULL;
    pAvData->bAllocated = 0;
    pAvData->bUsed = 0;
}
