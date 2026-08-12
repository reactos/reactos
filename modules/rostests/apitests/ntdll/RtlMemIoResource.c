/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for the Rtl memory/port resource descriptor encoding API:
 *              RtlCmEncodeMemIoResource, RtlCmDecodeMemIoResource,
 *              RtlIoEncodeMemIoResource, RtlIoDecodeMemIoResource and
 *              RtlFindClosestEncodableLength.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"
#include <limits.h>

static ULONGLONG (NTAPI *pRtlCmDecodeMemIoResource)(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Start);

static NTSTATUS (NTAPI *pRtlCmEncodeMemIoResource)(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Start);

static NTSTATUS (NTAPI *pRtlFindClosestEncodableLength)(
    _In_ ULONGLONG SourceLength,
    _Out_ PULONGLONG TargetLength);

static ULONGLONG (NTAPI *pRtlIoDecodeMemIoResource)(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Alignment,
    _Out_opt_ PULONGLONG MinimumAddress,
    _Out_opt_ PULONGLONG MaximumAddress);

static NTSTATUS (NTAPI *pRtlIoEncodeMemIoResource)(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Alignment,
    _In_ ULONGLONG MinimumAddress,
    _In_ ULONGLONG MaximumAddress);

static
void
Test_CmEncode(void)
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR Desc, Saved;
    NTSTATUS Status;

    /* Unsupported types must fail without touching the descriptor */
    memset(&Desc, 0x55, sizeof(Desc));
    Saved = Desc;
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeNull, 0x1000, 0);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&Desc, &Saved, sizeof(Desc)), "Descriptor modified on failure\n");
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeInterrupt, 0x1000, 0);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeDma, 0x1000, 0);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&Desc, &Saved, sizeof(Desc)), "Descriptor modified on failure\n");

    /* Basic port descriptor */
    memset(&Desc, 0x55, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypePort, 0x100, 0x3F8);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypePort);
    ok_eq_ulong(Desc.u.Port.Length, 0x100UL);
    ok_eq_hex64(Desc.u.Port.Start.QuadPart, 0x3F8);
    /* The port path does not clean stale large flags */
    ok_eq_hex(Desc.Flags, 0x5555);

    /* A port length cannot exceed 32 bits, and the failure leaves no writes */
    memset(&Desc, 0x55, sizeof(Desc));
    Saved = Desc;
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypePort, 0x100000000ULL, 0);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&Desc, &Saved, sizeof(Desc)), "Descriptor modified on failure\n");

    /* Small memory length: plain descriptor, stale large flags are cleared */
    memset(&Desc, 0, sizeof(Desc));
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_48 | CM_RESOURCE_MEMORY_READ_ONLY;
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x1000, 0xFFFFFFFFFF000000ULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemory);
    ok_eq_ulong(Desc.u.Memory.Length, 0x1000UL);
    ok_eq_hex64(Desc.u.Memory.Start.QuadPart, 0xFFFFFFFFFF000000ULL);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_READ_ONLY);

    /* Asking for MemoryLarge with a small length demotes to plain Memory */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemoryLarge, MAXULONG, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemory);
    ok_eq_ulong(Desc.u.Memory.Length, MAXULONG);

    /* Asking for plain Memory with a big length promotes to MemoryLarge (40-bit) */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x100000000ULL, 0x8000000000ULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemoryLarge);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_40);
    ok_eq_ulong(Desc.u.Memory.Length, 0x01000000UL);
    ok_eq_hex64(Desc.u.Memory.Start.QuadPart, 0x8000000000ULL);

    /* 40-bit tier upper bound */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemoryLarge, CM_RESOURCE_MEMORY_LARGE_40_MAXLEN, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_40);
    ok_eq_ulong(Desc.u.Memory.Length, MAXULONG);

    /* Low 8 bits set in the 40-bit tier: not encodable */
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x100000080ULL, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);

    /* 48-bit tier */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x10000000000ULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemoryLarge);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_48);
    ok_eq_ulong(Desc.u.Memory.Length, 0x01000000UL);

    /*
     * The tier is picked by magnitude alone: a 256-byte-aligned length in the
     * 48-bit tier still fails because it is not 64K-aligned
     */
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x10000000100ULL, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);

    /* 64-bit tier and its upper bound */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x1000000000000ULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_64);
    ok_eq_ulong(Desc.u.Memory.Length, 0x10000UL);

    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, CM_RESOURCE_MEMORY_LARGE_64_MAXLEN, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_64);
    ok_eq_ulong(Desc.u.Memory.Length, MAXULONG);

    /* Low 32 bits set in the 64-bit tier, and beyond-the-max lengths */
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x100000000000100ULL, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, CM_RESOURCE_MEMORY_LARGE_64_MAXLEN + 1, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
    Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, ULLONG_MAX, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
}

static
void
Test_CmDecode(void)
{
    static const struct
    {
        ULONGLONG Length;
        USHORT ExpectedFlags;
    } RoundTrips[] =
    {
        { 0x1000,               0                           },
        { MAXULONG,             0                           },
        { 0x100000000ULL,       CM_RESOURCE_MEMORY_LARGE_40 },
        { 0xFFFFFFFF00ULL,      CM_RESOURCE_MEMORY_LARGE_40 },
        { 0x10000000000ULL,     CM_RESOURCE_MEMORY_LARGE_48 },
        { 0xFFFFFFFF0000ULL,    CM_RESOURCE_MEMORY_LARGE_48 },
        { 0x1000000000000ULL,   CM_RESOURCE_MEMORY_LARGE_64 },
        { 0xFFFFFFFF00000000ULL, CM_RESOURCE_MEMORY_LARGE_64 },
    };
    CM_PARTIAL_RESOURCE_DESCRIPTOR Desc;
    ULONGLONG Length, Start;
    NTSTATUS Status;
    ULONG i;

    /* Plain port descriptor decodes verbatim */
    memset(&Desc, 0, sizeof(Desc));
    Desc.Type = CmResourceTypePort;
    Desc.u.Port.Start.QuadPart = 0x1122334455667788ULL;
    Desc.u.Port.Length = 0x1234;
    Start = 0;
    Length = pRtlCmDecodeMemIoResource(&Desc, &Start);
    ok_eq_hex64(Length, 0x1234);
    ok_eq_hex64(Start, 0x1122334455667788ULL);

    /* Start is optional */
    Length = pRtlCmDecodeMemIoResource(&Desc, NULL);
    ok_eq_hex64(Length, 0x1234);

    /* Large flags are ignored for plain port/memory types */
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_64;
    Length = pRtlCmDecodeMemIoResource(&Desc, NULL);
    ok_eq_hex64(Length, 0x1234);

    /* Hand-built large forms decode to the shifted length */
    memset(&Desc, 0, sizeof(Desc));
    Desc.Type = CmResourceTypeMemoryLarge;
    Desc.u.Memory.Length = 0x01000000;
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_40;
    ok_eq_hex64(pRtlCmDecodeMemIoResource(&Desc, NULL), 0x100000000ULL);
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_48;
    ok_eq_hex64(pRtlCmDecodeMemIoResource(&Desc, NULL), 0x10000000000ULL);
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_64;
    ok_eq_hex64(pRtlCmDecodeMemIoResource(&Desc, NULL), 0x100000000000000ULL);

    /* A large type without any large flag decodes to zero, Start still returned */
    Desc.Flags = 0;
    Desc.u.Memory.Start.QuadPart = 0xABCD;
    Start = 0;
    ok_eq_hex64(pRtlCmDecodeMemIoResource(&Desc, &Start), 0);
    ok_eq_hex64(Start, 0xABCD);

    /* Encode/decode round-trips across all tiers */
    for (i = 0; i < ARRAYSIZE(RoundTrips); i++)
    {
        memset(&Desc, 0, sizeof(Desc));
        Status = pRtlCmEncodeMemIoResource(&Desc,
                                           CmResourceTypeMemory,
                                           RoundTrips[i].Length,
                                           0x40000000ULL + i);
        ok(Status == STATUS_SUCCESS, "[%lu] encode failed: 0x%lx\n", i, Status);
        ok(Desc.Flags == RoundTrips[i].ExpectedFlags,
           "[%lu] Flags = 0x%x, expected 0x%x\n",
           i, Desc.Flags, RoundTrips[i].ExpectedFlags);
        Start = 0;
        Length = pRtlCmDecodeMemIoResource(&Desc, &Start);
        ok(Length == RoundTrips[i].Length,
           "[%lu] Length = 0x%I64x, expected 0x%I64x\n",
           i, Length, RoundTrips[i].Length);
        ok(Start == 0x40000000ULL + i,
           "[%lu] Start = 0x%I64x\n", i, Start);
    }
}

static
void
Test_IoEncode(void)
{
    IO_RESOURCE_DESCRIPTOR Desc, Saved;
    NTSTATUS Status;

    /* Unsupported types fail without touching the descriptor */
    memset(&Desc, 0x55, sizeof(Desc));
    Saved = Desc;
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeInterrupt, 0x1000, 4, 0, 0xFFFF);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&Desc, &Saved, sizeof(Desc)), "Descriptor modified on failure\n");

    /* Basic port requirement */
    memset(&Desc, 0x55, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypePort, 0x100, 4, 0x1000, 0xFFFF);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypePort);
    ok_eq_ulong(Desc.u.Port.Length, 0x100UL);
    ok_eq_ulong(Desc.u.Port.Alignment, 4UL);
    ok_eq_hex64(Desc.u.Port.MinimumAddress.QuadPart, 0x1000);
    ok_eq_hex64(Desc.u.Port.MaximumAddress.QuadPart, 0xFFFF);

    /* Port length/alignment above 32 bits are rejected, no writes */
    memset(&Desc, 0x55, sizeof(Desc));
    Saved = Desc;
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypePort, 0x100000000ULL, 4, 0, 0xFFFF);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypePort, 0x100, 0x100000000ULL, 0, 0xFFFF);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok(!memcmp(&Desc, &Saved, sizeof(Desc)), "Descriptor modified on failure\n");

    /* Small memory requirement stays plain and clears stale large flags */
    memset(&Desc, 0, sizeof(Desc));
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_40 | CM_RESOURCE_MEMORY_PREFETCHABLE;
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x1000, 0x1000, 0, 0xFFFFFFFFULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemory);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_PREFETCHABLE);
    ok_eq_ulong(Desc.u.Memory.Length, 0x1000UL);
    ok_eq_ulong(Desc.u.Memory.Alignment, 0x1000UL);
    ok_eq_hex64(Desc.u.Memory.MinimumAddress.QuadPart, 0);
    ok_eq_hex64(Desc.u.Memory.MaximumAddress.QuadPart, 0xFFFFFFFFULL);

    /* Large length: a small alignment is scaled up to the tier granularity */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       0x100000000ULL, 1,
                                       0, 0xFFFFFFFFFFULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemoryLarge);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_40);
    ok_eq_ulong(Desc.u.Memory.Length, 0x01000000UL);
    ok_eq_ulong(Desc.u.Memory.Alignment, 1UL);   /* i.e. 0x100 bytes */
    ok_eq_hex64(Desc.u.Memory.MaximumAddress.QuadPart, 0xFFFFFFFFFFULL);

    /* Zero alignment encodes as zero */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x100000000ULL, 0, 0, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(Desc.u.Memory.Alignment, 0UL);

    /* A non-power-of-two alignment is doubled until it fits the granularity */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x100000000ULL, 0x81, 0, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(Desc.u.Memory.Alignment, 0x81UL);   /* 0x81 << 8 == 0x8100 bytes */

    /*
     * A small length with a >4GB alignment still selects the large form
     * (the plain path needs both to fit in 32 bits)
     */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       0x1000, 0x100000000ULL, 0, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_uint(Desc.Type, CmResourceTypeMemoryLarge);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_40);
    ok_eq_ulong(Desc.u.Memory.Length, 0x10UL);
    ok_eq_ulong(Desc.u.Memory.Alignment, 0x01000000UL);

    /* Length not representable at the tier granularity */
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x100000080ULL, 1, 0, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory, 0x10000000100ULL, 1, 0, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);

    /* Alignment above the tier picked by the length */
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       0x100000000ULL, 0x10000000000ULL, 0, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);

    /* Alignment whose scaling overflows 64 bits */
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       0x1000000000000ULL, 0x8000000000000001ULL, 0, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);

    /* 64-bit tier */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       0x1000000000000ULL, 0x100000000ULL,
                                       0x100000000ULL, 0xFFFFFFFFFFFFFFFFULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Desc.Flags, CM_RESOURCE_MEMORY_LARGE_64);
    ok_eq_ulong(Desc.u.Memory.Length, 0x10000UL);
    ok_eq_ulong(Desc.u.Memory.Alignment, 1UL);
    ok_eq_hex64(Desc.u.Memory.MinimumAddress.QuadPart, 0x100000000ULL);
    ok_eq_hex64(Desc.u.Memory.MaximumAddress.QuadPart, 0xFFFFFFFFFFFFFFFFULL);

    /* Length beyond every tier */
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       ULLONG_MAX, 1, 0, 0);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
}

static
void
Test_IoDecode(void)
{
    IO_RESOURCE_DESCRIPTOR Desc;
    ULONGLONG Length, Alignment, Minimum, Maximum;
    NTSTATUS Status;

    /* Plain memory requirement */
    memset(&Desc, 0, sizeof(Desc));
    Desc.Type = CmResourceTypeMemory;
    Desc.u.Memory.Length = 0x2000;
    Desc.u.Memory.Alignment = 0x1000;
    Desc.u.Memory.MinimumAddress.QuadPart = 0x100000;
    Desc.u.Memory.MaximumAddress.QuadPart = 0xFFFFFFFFULL;
    Alignment = Minimum = Maximum = 0;
    Length = pRtlIoDecodeMemIoResource(&Desc, &Alignment, &Minimum, &Maximum);
    ok_eq_hex64(Length, 0x2000);
    ok_eq_hex64(Alignment, 0x1000);
    ok_eq_hex64(Minimum, 0x100000);
    ok_eq_hex64(Maximum, 0xFFFFFFFFULL);

    Length = pRtlIoDecodeMemIoResource(&Desc, NULL, NULL, NULL);
    ok_eq_hex64(Length, 0x2000);

    /* Length and alignment both scale */
    memset(&Desc, 0, sizeof(Desc));
    Desc.Type = CmResourceTypeMemoryLarge;
    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_40;
    Desc.u.Memory.Length = 0x01000000;
    Desc.u.Memory.Alignment = 0x01000000;
    Alignment = 0;
    Length = pRtlIoDecodeMemIoResource(&Desc, &Alignment, NULL, NULL);
    ok_eq_hex64(Length, 0x100000000ULL);
    ok_eq_hex64(Alignment, 0x100000000ULL);

    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_48;
    Length = pRtlIoDecodeMemIoResource(&Desc, &Alignment, NULL, NULL);
    ok_eq_hex64(Length, 0x10000000000ULL);
    ok_eq_hex64(Alignment, 0x10000000000ULL);

    Desc.Flags = CM_RESOURCE_MEMORY_LARGE_64;
    Length = pRtlIoDecodeMemIoResource(&Desc, &Alignment, NULL, NULL);
    ok_eq_hex64(Length, 0x100000000000000ULL);
    ok_eq_hex64(Alignment, 0x100000000000000ULL);

    /* A large type without large flags decodes to zero, bounds still returned */
    Desc.Flags = 0;
    Desc.u.Memory.MinimumAddress.QuadPart = 0x1234;
    Desc.u.Memory.MaximumAddress.QuadPart = 0x5678;
    Alignment = Minimum = Maximum = ULLONG_MAX;
    Length = pRtlIoDecodeMemIoResource(&Desc, &Alignment, &Minimum, &Maximum);
    ok_eq_hex64(Length, 0);
    ok_eq_hex64(Alignment, 0);
    ok_eq_hex64(Minimum, 0x1234);
    ok_eq_hex64(Maximum, 0x5678);

    /* Encode/decode round-trip: the rounded-up alignment is what comes back */
    memset(&Desc, 0, sizeof(Desc));
    Status = pRtlIoEncodeMemIoResource(&Desc, CmResourceTypeMemory,
                                       0x10000000000ULL, 0x1000,
                                       0x10000, 0xFFFFFFFFFFFFULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Alignment = Minimum = Maximum = 0;
    Length = pRtlIoDecodeMemIoResource(&Desc, &Alignment, &Minimum, &Maximum);
    ok_eq_hex64(Length, 0x10000000000ULL);
    ok_eq_hex64(Alignment, 0x10000);   /* 0x1000 rounded up to the 64K granularity */
    ok_eq_hex64(Minimum, 0x10000);
    ok_eq_hex64(Maximum, 0xFFFFFFFFFFFFULL);
}

static
void
Test_FindClosest(void)
{
    static const struct
    {
        ULONGLONG Source;
        ULONGLONG Expected;
    } Cases[] =
    {
        /* 32-bit values are always exact */
        { 0,                        0                       },
        { 0x1234,                   0x1234                  },
        { MAXULONG,                 MAXULONG                },
        /* 40-bit tier: rounded up to 256 bytes */
        { 0x100000000ULL,           0x100000000ULL          },
        { 0x100000001ULL,           0x100000100ULL          },
        { 0xFFFFFFFF00ULL,          0xFFFFFFFF00ULL         },
        /* just past the 40-bit tier: 64K granularity now */
        { 0xFFFFFFFF01ULL,          0x10000000000ULL        },
        { 0x10000000001ULL,         0x10000010000ULL        },
        { 0xFFFFFFFF0000ULL,        0xFFFFFFFF0000ULL       },
        /* just past the 48-bit tier: 4G granularity */
        { 0xFFFFFFFF0001ULL,        0x1000000000000ULL      },
        { 0x1000000000000ULL,       0x1000000000000ULL      },
        { 0x1000000000001ULL,       0x1000100000000ULL      },
        { 0xFFFFFFFF00000000ULL,    0xFFFFFFFF00000000ULL   },
    };
    CM_PARTIAL_RESOURCE_DESCRIPTOR Desc;
    ULONGLONG Target;
    NTSTATUS Status;
    ULONG i;

    for (i = 0; i < ARRAYSIZE(Cases); i++)
    {
        Target = ULLONG_MAX;
        Status = pRtlFindClosestEncodableLength(Cases[i].Source, &Target);
        ok(Status == STATUS_SUCCESS, "[%lu] failed: 0x%lx\n", i, Status);
        ok(Target == Cases[i].Expected,
           "[%lu] Target = 0x%I64x, expected 0x%I64x\n",
           i, Target, Cases[i].Expected);

        /* The returned length must itself be encodable */
        memset(&Desc, 0, sizeof(Desc));
        Status = pRtlCmEncodeMemIoResource(&Desc, CmResourceTypeMemory, Target, 0);
        ok(Status == STATUS_SUCCESS, "[%lu] closest length not encodable: 0x%lx\n", i, Status);
    }

    /* Beyond the 64-bit tier there is nothing to round to */
    Target = ULLONG_MAX;
    Status = pRtlFindClosestEncodableLength(CM_RESOURCE_MEMORY_LARGE_64_MAXLEN + 1, &Target);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
    ok_eq_hex64(Target, 0);
    Status = pRtlFindClosestEncodableLength(ULLONG_MAX, &Target);
    ok_eq_hex(Status, STATUS_UNSUCCESSFUL);
}

START_TEST(RtlMemIoResource)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");

    pRtlCmDecodeMemIoResource = (PVOID)GetProcAddress(hNtdll, "RtlCmDecodeMemIoResource");
    pRtlCmEncodeMemIoResource = (PVOID)GetProcAddress(hNtdll, "RtlCmEncodeMemIoResource");
    pRtlFindClosestEncodableLength = (PVOID)GetProcAddress(hNtdll, "RtlFindClosestEncodableLength");
    pRtlIoDecodeMemIoResource = (PVOID)GetProcAddress(hNtdll, "RtlIoDecodeMemIoResource");
    pRtlIoEncodeMemIoResource = (PVOID)GetProcAddress(hNtdll, "RtlIoEncodeMemIoResource");

    if (!pRtlCmDecodeMemIoResource || !pRtlCmEncodeMemIoResource ||
        !pRtlFindClosestEncodableLength ||
        !pRtlIoDecodeMemIoResource || !pRtlIoEncodeMemIoResource)
    {
        skip("Rtl*MemIoResource API not available (NT 6.0+ only)\n");
        return;
    }

    Test_CmEncode();
    Test_CmDecode();
    Test_IoEncode();
    Test_IoDecode();
    Test_FindClosest();
}
