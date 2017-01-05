/*
 * Copyright 2011 Samuel Serapión
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
 */
#include "ntlmssp.h"
#include <wincrypt.h>
#include "ciphers.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(ntlm);

#ifdef USE_CRYPTOAPI
HCRYPTPROV Prov;
#endif
PVOID LockedMemoryPtr = NULL;
ULONG LockedMemorySize = 0;

BOOL
NtlmInitializeRNG(VOID)
{
    BOOL ret = TRUE;
#ifdef USE_CRYPTOAPI
    /* prevent double initialization */
    if(Prov)
        return TRUE;

    ret = CryptAcquireContext(&Prov,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT);

    if(!ret)
        ERR("CryptAcquireContext failed with %x.\n",GetLastError());
#endif
    return ret;
}

VOID
NtlmTerminateRNG(VOID)
{
#ifdef USE_CRYPTOAPI
    if(Prov)
    {
        CryptReleaseContext(Prov,0);
        Prov = 0;
    }
#endif
}

NTSTATUS
NtlmGenerateRandomBits(VOID *Bits, ULONG Size)
{
#ifdef USE_CRYPTOAPI
    if(CryptGenRandom(Prov, Size, (BYTE*)Bits))
        return STATUS_SUCCESS;
#else
    if(RtlGenRandom(Bits, Size))
        return STATUS_SUCCESS;
#endif
    return STATUS_UNSUCCESSFUL;
}

BOOL
NtlmProtectMemory(VOID *Data, ULONG Size)
{
    rc4_key rc4key;

    if(Data == NULL || Size == 0)
        return TRUE;

    if(LockedMemoryPtr == NULL)
        return FALSE;

    rc4_init(&rc4key, (unsigned char*)LockedMemoryPtr, LockedMemorySize);
    rc4_crypt(&rc4key, (unsigned char *)Data,(unsigned char *)Data, Size);

    ZeroMemory(&rc4key, sizeof(rc4key));

    return TRUE;
}

BOOL
NtlmUnProtectMemory(VOID *Data, ULONG Size)
{
    return NtlmProtectMemory(Data, Size);
}

VOID
NtlmTerminateProtectedMemory(VOID)
{
    if(LockedMemoryPtr)
    {
        ZeroMemory(LockedMemoryPtr, LockedMemorySize);
        VirtualFree(LockedMemoryPtr, 0, MEM_RELEASE);
        LockedMemoryPtr = NULL;
    }
}

BOOL
NtlmInitializeProtectedMemory(VOID)
{
    /* key size of  the algorithm */
    LockedMemorySize = 256;

    LockedMemoryPtr = VirtualAlloc(NULL,
                                   LockedMemorySize,
                                   MEM_COMMIT,
                                   PAGE_READWRITE);

    if(!LockedMemoryPtr)
        return FALSE;

    /* do actual locking */
    VirtualLock(LockedMemoryPtr, LockedMemorySize);

    if(!NT_SUCCESS(NtlmGenerateRandomBits(LockedMemoryPtr, LockedMemorySize)))
    {
        NtlmTerminateProtectedMemory();
        return FALSE;
    }

    return TRUE;
}
