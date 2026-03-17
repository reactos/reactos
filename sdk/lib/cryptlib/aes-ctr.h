/*
 * PROJECT:     ReactOS crypto library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header for AES-CTR DRBG primitive
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include <stdint.h>
#include "tomcrypt.h"

typedef struct _AES_CTR_CTX
{
    aes_key AesKey;
    _Interlocked_operand_ volatile uint64_t Counter;
    uint64_t volatile Complement;
} AES_CTR_CTX, *PAES_CTR_CTX;

void
AES_CTR_Init(
    _Out_ PAES_CTR_CTX AesCtrContext,
    _In_reads_bytes_(EntropyLength) const uint8_t* Entropy,
    _In_ size_t EntropyLength);

void
AES_CTR_GenRandomBytes(
    _Inout_ PAES_CTR_CTX AesCtrContext,
    _Out_writes_bytes_(Length) void* Buffer,
    _In_ size_t Length);
