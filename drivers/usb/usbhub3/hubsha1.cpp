/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     SHA-1 (FIPS 180-4) for generated container ids
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "hubid.h"

static
inline
ULONG
NTAPI
HubSha1Rotate(
    _In_ ULONG Value,
    _In_ ULONG Bits)
{
    return (Value << Bits) | (Value >> (32 - Bits));
}

/* One 64 byte block, FIPS 180-4 6.1.2 */
static
VOID
NTAPI
HubSha1Compress(
    _Inout_updates_(5) PULONG State,
    _In_reads_bytes_(64) const UCHAR* Block)
{
    ULONG Schedule[80];
    ULONG Work[5];
    ULONG Mix;
    ULONG Constant;
    ULONG Next;
    ULONG Round;

    for (Round = 0; Round < 16; Round++)
    {
        Schedule[Round] = ((ULONG)Block[Round * 4] << 24) |
                          ((ULONG)Block[Round * 4 + 1] << 16) |
                          ((ULONG)Block[Round * 4 + 2] << 8) |
                          (ULONG)Block[Round * 4 + 3];
    }

    for (Round = 16; Round < 80; Round++)
    {
        Schedule[Round] = HubSha1Rotate(Schedule[Round - 3] ^ Schedule[Round - 8] ^
                                        Schedule[Round - 14] ^ Schedule[Round - 16], 1);
    }

    RtlCopyMemory(Work, State, sizeof(Work));

    for (Round = 0; Round < 80; Round++)
    {
        if (Round < 20)
        {
            Mix = (Work[1] & Work[2]) | (~Work[1] & Work[3]);
            Constant = 0x5A827999;
        }
        else if (Round < 40)
        {
            Mix = Work[1] ^ Work[2] ^ Work[3];
            Constant = 0x6ED9EBA1;
        }
        else if (Round < 60)
        {
            Mix = (Work[1] & Work[2]) | (Work[1] & Work[3]) | (Work[2] & Work[3]);
            Constant = 0x8F1BBCDC;
        }
        else
        {
            Mix = Work[1] ^ Work[2] ^ Work[3];
            Constant = 0xCA62C1D6;
        }

        Next = HubSha1Rotate(Work[0], 5) + Mix + Work[4] + Constant + Schedule[Round];
        Work[4] = Work[3];
        Work[3] = Work[2];
        Work[2] = HubSha1Rotate(Work[1], 30);
        Work[1] = Work[0];
        Work[0] = Next;
    }

    for (Round = 0; Round < 5; Round++)
        State[Round] += Work[Round];
}

VOID
NTAPI
HubSha1Init(
    _Out_ HubSha1* Context)
{
    RtlZeroMemory(Context, sizeof(*Context));
    Context->State[0] = 0x67452301;
    Context->State[1] = 0xEFCDAB89;
    Context->State[2] = 0x98BADCFE;
    Context->State[3] = 0x10325476;
    Context->State[4] = 0xC3D2E1F0;
}

VOID
NTAPI
HubSha1Update(
    _Inout_ HubSha1* Context,
    _In_reads_bytes_(Length) const VOID* Data,
    _In_ SIZE_T Length)
{
    const UCHAR* Bytes = (const UCHAR*)Data;
    SIZE_T Chunk;

    Context->TotalBytes += Length;

    while (Length != 0)
    {
        Chunk = min(Length, sizeof(Context->Block) - Context->BlockUsed);
        RtlCopyMemory(&Context->Block[Context->BlockUsed], Bytes, Chunk);
        Context->BlockUsed += (ULONG)Chunk;
        Bytes += Chunk;
        Length -= Chunk;

        if (Context->BlockUsed == sizeof(Context->Block))
        {
            HubSha1Compress(Context->State, Context->Block);
            Context->BlockUsed = 0;
        }
    }
}

/* Pads with 0x80, zeros and the message length in bits, big endian (FIPS 180-4 5.1.1) */
VOID
NTAPI
HubSha1Final(
    _Inout_ HubSha1* Context,
    _Out_writes_bytes_all_(HUB_SHA1_DIGEST_BYTES) PUCHAR Digest)
{
    ULONG64 Bits = Context->TotalBytes * 8;
    ULONG Index;

    Context->Block[Context->BlockUsed++] = 0x80;

    if (Context->BlockUsed > sizeof(Context->Block) - 8)
    {
        RtlZeroMemory(&Context->Block[Context->BlockUsed], sizeof(Context->Block) - Context->BlockUsed);
        HubSha1Compress(Context->State, Context->Block);
        Context->BlockUsed = 0;
    }

    RtlZeroMemory(&Context->Block[Context->BlockUsed], sizeof(Context->Block) - 8 - Context->BlockUsed);

    for (Index = 0; Index < 8; Index++)
        Context->Block[56 + Index] = (UCHAR)(Bits >> (56 - Index * 8));

    HubSha1Compress(Context->State, Context->Block);

    for (Index = 0; Index < HUB_SHA1_DIGEST_BYTES; Index++)
        Digest[Index] = (UCHAR)(Context->State[Index / 4] >> (24 - (Index % 4) * 8));

    RtlSecureZeroMemory(Context, sizeof(*Context));
}
