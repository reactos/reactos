/*
 * PROJECT:     ReactOS crypto library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     AES-CTR DRBG primitive
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "aes-ctr.h"
#include "sha2.h"
#include <intrin.h>

#define VERIFY(x) do { if (!(x)) __int2c(); } while (0)

/* Volatile-based zeroing that won't be optimized away */
static inline void
SecureWipe(
    _Out_writes_bytes_(Length) void* Buffer,
    _In_ size_t Length)
{
    volatile uint8_t* p = (volatile uint8_t*)Buffer;
    while (Length--)
        *p++ = 0;
}

static inline
ULONG64
ReadTimeStampCounter(void)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    return __rdtsc();
#elif defined(_M_ARM)
    // See https://learn.microsoft.com/en-us/cpp/build/overview-of-arm-abi-conventions?view=msvc-170
    return __rdpmccntr64();
#elif defined(_M_ARM64)
    // See https://github.com/microsoft/SymCrypt/blob/083be6ca37d7e20ae90b883fe044ee7444d448fe/modules/windows/kernel/entropy_accumulator.c#L146C20-L146C54
    return _ReadStatusReg(ARM64_PMCCNTR_EL0);
#else
#error Implement me!
#endif
}

static inline uint64_t
AtomicIncrementCounter(volatile uint64_t* Counter)
{
#ifdef _WIN64
    return _InterlockedIncrement64((long long*)Counter);
#else
    uint64_t current, prev;
    for (current = *Counter; ; current = prev)
    {
         prev = _InterlockedCompareExchange64((long long*)Counter, current + 1, current);
         if (prev == current)
             return current + 1;
    }
#endif
}

void
AES_CTR_Init(
    _Out_ PAES_CTR_CTX AesCtrContext,
    _In_reads_bytes_(EntropyLength) const uint8_t* Entropy,
    _In_ size_t EntropyLength)
{
    SHA256_CTX sha256Ctx;
    uint8_t keyDataBuffer[32];

    VERIFY(EntropyLength >= 32);

    /* Derive the AES key: SHA-256(Entropy) */
    SHA256_Init(&sha256Ctx);
    SHA256_Update(&sha256Ctx, Entropy, EntropyLength);
    SHA256_Final(keyDataBuffer, &sha256Ctx);

    /* Create the 256 bit AES key */
    VERIFY(aes_setup(keyDataBuffer, 32, 0, &AesCtrContext->AesKey) == CRYPT_OK);

    /* Derive Counter/Complement seed: SHA-256(Tsc || Entropy)
       This ensures the IV material is independent of the key. */
    uint64_t tsc = ReadTimeStampCounter();
    SHA256_Init(&sha256Ctx);
    SHA256_Update(&sha256Ctx, (uint8_t*)&tsc, sizeof(tsc));
    SHA256_Update(&sha256Ctx, Entropy, EntropyLength);
    SHA256_Final(keyDataBuffer, &sha256Ctx);

    /* Initialize the counter and complement, mixing in the TSC.
       Use the same TSC so both fields get the same quality entropy. */
    AesCtrContext->Counter = *(uint64_t*)(keyDataBuffer + 0) ^ tsc;
    AesCtrContext->Complement = *(uint64_t*)(keyDataBuffer + 8) ^ _rotl64(tsc, 31);

    /* Wipe sensitive key material from the stack */
    SecureWipe(&sha256Ctx, sizeof(sha256Ctx));
    SecureWipe(keyDataBuffer, sizeof(keyDataBuffer));
}

void
AES_CTR_GenRandomBytes(
    _Inout_ PAES_CTR_CTX AesCtrContext,
    _Out_writes_bytes_(Length) void* Buffer,
    _In_ size_t Length)
{
    union
    {
        struct
        {
            uint64_t CallUniq;
            uint64_t BlockUniq;
        };
        uint8_t Data[16];
    } block;

    /* Initialize the block.
       CallUniq provides cross-call uniqueness (atomic).
       BlockUniq provides intra-call uniqueness (incremented per block). */
    block.CallUniq = AtomicIncrementCounter(&AesCtrContext->Counter);
    block.BlockUniq = AesCtrContext->Complement + ReadTimeStampCounter();

    /* Update the complement. This is not atomic, but it doesn't
       have to be, as we only want to share some extra bits of entropy. */
    AesCtrContext->Complement = _rotl64(block.BlockUniq, 5);

    /* Generate full blocks of random bytes */
    uint8_t* bufferPointer = Buffer;
    size_t remainingBytes = Length;
    while (remainingBytes >= 16)
    {
        aes_ecb_encrypt(block.Data, bufferPointer, &AesCtrContext->AesKey);
        block.BlockUniq++;
        bufferPointer += 16;
        remainingBytes -= 16;
    }

    /* Generate the final partial block if necessary */
    if (remainingBytes > 0)
    {
        uint8_t encryptedBlockData[16];
        aes_ecb_encrypt(block.Data, encryptedBlockData, &AesCtrContext->AesKey);
        memcpy(bufferPointer, encryptedBlockData, remainingBytes);
        SecureWipe(encryptedBlockData, sizeof(encryptedBlockData));
    }

    SecureWipe(&block, sizeof(block));
}
