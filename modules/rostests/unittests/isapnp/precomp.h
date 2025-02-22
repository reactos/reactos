/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Precompiled header for isapnp_unittest
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

typedef PVOID PDEVICE_OBJECT;

#define UNIT_TEST
#include <isapnphw.h>
#include <isapnpres.h>

/* KERNEL DEFINITIONS (MOCK) **************************************************/

#define PAGED_CODE()
#define CODE_SEG(segment)
#define DPRINT(...)  do { if (0) { trace(__VA_ARGS__); } } while (0)
#define DPRINT1(...) do { if (0) { trace(__VA_ARGS__); } } while (0)
#define KeStallExecutionProcessor(MicroSeconds)

FORCEINLINE
PVOID
ExAllocatePoolWithTag(ULONG PoolType, SIZE_T NumberOfBytes, ULONG Tag)
{
    PULONG_PTR Mem = HeapAlloc(GetProcessHeap(), 0, NumberOfBytes + 2 * sizeof(PVOID));
    if (Mem == NULL)
        return NULL;

    Mem[0] = NumberOfBytes;
    Mem[1] = Tag;

    return (PVOID)(Mem + 2);
}

FORCEINLINE
PVOID
ExAllocatePoolZero(ULONG PoolType, SIZE_T NumberOfBytes, ULONG Tag)
{
    PVOID Result = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

    if (Result != NULL)
        RtlZeroMemory(Result, NumberOfBytes);

    return Result;
}

FORCEINLINE
VOID
ExFreePoolWithTag(PVOID MemPtr, ULONG Tag)
{
    PULONG_PTR Mem = MemPtr;

    Mem -= 2;
    ok(Mem[1] == Tag, "Tag is %lx, expected %lx\n", Tag, Mem[1]);
    HeapFree(GetProcessHeap(), 0, Mem);
}

FORCEINLINE
SIZE_T
GetPoolAllocSize(PVOID MemPtr)
{
    PVOID* Mem = MemPtr;

    Mem -= 2;
    return (SIZE_T)Mem[0];
}

/* ISAPNP DRIVER DEFINITIONS (MOCK) *******************************************/

#define TAG_ISAPNP 'pasI'

typedef struct _ISAPNP_FDO_EXTENSION
{
    LIST_ENTRY DeviceListHead;
    ULONG DeviceCount;
    ULONG Cards;
    PUCHAR ReadDataPort;
} ISAPNP_FDO_EXTENSION, *PISAPNP_FDO_EXTENSION;

typedef struct _ISAPNP_PDO_EXTENSION
{
    PISAPNP_LOGICAL_DEVICE IsaPnpDevice;

    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

    PCM_RESOURCE_LIST ResourceList;
    ULONG ResourceListSize;
} ISAPNP_PDO_EXTENSION, *PISAPNP_PDO_EXTENSION;

/* TEST DEFINITIONS ***********************************************************/

typedef enum _ISAPNP_STATE
{
    IsaWaitForKey = 0,
    IsaSleep = 1,
    IsaIsolation = 2,
    IsaConfgure = 3
} ISAPNP_STATE;

typedef struct _ISAPNP_CARD_LOGICAL_DEVICE
{
    UCHAR Registers[0xFF];
} ISAPNP_CARD_LOGICAL_DEVICE, *PISAPNP_CARD_LOGICAL_DEVICE;

#define TEST_MAX_SUPPORTED_DEVICES     7

typedef struct _ISAPNP_CARD
{
    ISAPNP_STATE State;
    UCHAR LfsrCount;
    UCHAR Lfsr;
    UCHAR SelectNumberReg;
    UCHAR DeviceNumberReg;
    UCHAR SerialIsolationIdx;
    UCHAR SerialIdResponse;
    UCHAR IsolationRead;
    PUCHAR PnpRom;
    PUCHAR ReadDataPort;
    ULONG RomIdx;
    ULONG RomSize;
    ULONG LogicalDevices;
    ISAPNP_CARD_LOGICAL_DEVICE LogDev[TEST_MAX_SUPPORTED_DEVICES];
} ISAPNP_CARD, *PISAPNP_CARD;

UCHAR
NTAPI
READ_PORT_UCHAR(
    _In_ PUCHAR Port);

VOID
NTAPI
WRITE_PORT_UCHAR(
    _In_ PUCHAR Port,
    _In_ UCHAR Value);

VOID
IsaBusCreateCard(
    _Inout_ PISAPNP_CARD Card,
    _In_ PVOID PnpRom,
    _In_ ULONG RomSize,
    _In_ ULONG LogicalDevices);

VOID
DrvCreateCard1(
    _In_ PISAPNP_CARD Card);

VOID
DrvTestCard1Dev1Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

VOID
DrvTestCard1Dev2Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

VOID
DrvTestCard1Dev3Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

VOID
DrvTestCard1Dev4Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

VOID
DrvTestCard1Dev5Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

VOID
DrvTestCard1Dev6Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

VOID
DrvTestCard1Dev7Resources(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST ReqList);

PCM_RESOURCE_LIST
DrvTestCard1Dev6CreateConfigurationResources(VOID);

VOID
DrvTestCard1Dev6ConfigurationResult(
    _In_ PISAPNP_CARD_LOGICAL_DEVICE LogDev);

VOID
DrvCreateCard2(
    _In_ PISAPNP_CARD Card);

#define expect_resource_list_header(ResourceList, ExpectedIface, ExpectedCount)       \
    do {                                                                              \
    ok_eq_int((ResourceList)->List[0].InterfaceType, (ExpectedIface));                \
    ok_eq_ulong((ResourceList)->List[0].BusNumber, 0UL);                              \
    ok_eq_int((ResourceList)->List[0].PartialResourceList.Version, 1); /* 0 */        \
    ok_eq_int((ResourceList)->List[0].PartialResourceList.Revision, 1); /* 0x3000 */  \
    ok_eq_ulong((ResourceList)->List[0].PartialResourceList.Count, (ExpectedCount));  \
    } while (0)

#define expect_requirements_list_header(ReqList, ExpectedIface, ExpectedCount)       \
    do {                                                                             \
    ok_eq_int((ReqList)->InterfaceType, (ExpectedIface));                            \
    ok_eq_ulong((ReqList)->BusNumber, 0UL);                                          \
    ok_eq_ulong((ReqList)->SlotNumber, 0UL);                                         \
    ok_eq_ulong((ReqList)->AlternativeLists, (ExpectedCount));                       \
    } while (0)

#define expect_alt_list_header(AltList, ExpectedCount)  \
    do {                                                \
    ok_eq_int((AltList)->Version, 1);                   \
    ok_eq_int((AltList)->Revision, 1);                  \
    ok_eq_ulong((AltList)->Count, (ExpectedCount));     \
    } while (0)

#define expect_port_req(Desc, ExpectedOption, ExpectedFlags, ExpectedShare,      \
                        ExpectedLength, ExpectedAlign, ExpectedMin, ExpectedMax) \
    do {                                                                         \
    ok((Desc)->Type == CmResourceTypePort,                                       \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypePort);      \
    ok((Desc)->Option == (ExpectedOption),                                       \
       "Desc->Option = %u, expected %u\n", (Desc)->Option, (ExpectedOption));    \
    ok((Desc)->Flags == (ExpectedFlags),                                         \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));       \
    ok((Desc)->ShareDisposition == (ExpectedShare),                              \
       "Desc->ShareDisposition = %u, expected %u\n",                             \
       (Desc)->ShareDisposition, (ExpectedShare));                               \
    ok((Desc)->u.Port.Length == (ExpectedLength),                                \
       "Desc->u.Port.Length = %lx, expected %lx\n",                              \
       (Desc)->u.Port.Length, (ExpectedLength));                                 \
    ok((Desc)->u.Port.Alignment == (ExpectedAlign),                              \
       "Desc->u.Port.Alignment = %lu, expected %lu\n",                           \
       (Desc)->u.Port.Alignment, (ExpectedAlign));                               \
    ok((Desc)->u.Port.MinimumAddress.QuadPart == (ExpectedMin),                  \
       "Desc->u.Port.MinimumAddress = 0x%I64x, expected 0x%I64x\n",              \
       (Desc)->u.Port.MinimumAddress.QuadPart, (ExpectedMin));                   \
    ok((Desc)->u.Port.MaximumAddress.QuadPart == (ExpectedMax),                  \
       "Desc->u.Port.MaximumAddress = 0x%I64x, expected 0x%I64x\n",              \
       (Desc)->u.Port.MaximumAddress.QuadPart, (ExpectedMax));                   \
    } while (0)

#define expect_irq_req(Desc, ExpectedOption, ExpectedFlags, ExpectedShare,       \
                       ExpectedMin, ExpectedMax)                                 \
    do {                                                                         \
    ok((Desc)->Type == CmResourceTypeInterrupt,                                  \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeInterrupt); \
    ok((Desc)->Option == (ExpectedOption),                                       \
       "Desc->Option = %u, expected %u\n", (Desc)->Option, (ExpectedOption));    \
    ok((Desc)->Flags == (ExpectedFlags),                                         \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));       \
    ok((Desc)->ShareDisposition == (ExpectedShare),                              \
       "Desc->ShareDisposition = %u, expected %u\n",                             \
       (Desc)->ShareDisposition, (ExpectedShare));                               \
    ok((Desc)->u.Interrupt.MinimumVector == (ExpectedMin),                       \
       "Desc->u.Interrupt.MinimumVector = %lu, expected %lu\n",                  \
       (Desc)->u.Interrupt.MinimumVector, (ExpectedMin));                        \
    ok((Desc)->u.Interrupt.MaximumVector == (ExpectedMax),                       \
       "Desc->u.Interrupt.MaximumVector = %lu, expected %lu\n",                  \
       (Desc)->u.Interrupt.MaximumVector, (ExpectedMax));                        \
    } while (0)

#define expect_dma_req(Desc, ExpectedOption, ExpectedFlags, ExpectedShare,       \
                       ExpectedMin, ExpectedMax)                                 \
    do {                                                                         \
    ok((Desc)->Type == CmResourceTypeDma,                                        \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeDma);       \
    ok((Desc)->Option == (ExpectedOption),                                       \
       "Desc->Option = %u, expected %u\n", (Desc)->Option, (ExpectedOption));    \
    ok((Desc)->Flags == (ExpectedFlags),                                         \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));       \
    ok((Desc)->ShareDisposition == (ExpectedShare),                              \
       "Desc->ShareDisposition = %u, expected %u\n",                             \
       (Desc)->ShareDisposition, (ExpectedShare));                               \
    ok((Desc)->u.Dma.MinimumChannel == (ExpectedMin),                            \
       "Desc->u.Dma.MinimumChannel = %lu, expected %lu\n",                       \
       (Desc)->u.Dma.MinimumChannel, (ExpectedMin));                             \
    ok((Desc)->u.Dma.MaximumChannel == (ExpectedMax),                            \
       "Desc->u.Dma.MaximumChannel = %lu, expected %lu\n",                       \
       (Desc)->u.Dma.MaximumChannel, (ExpectedMax));                             \
    } while (0)

#define expect_mem_req(Desc, ExpectedOption, ExpectedFlags, ExpectedShare,       \
                       ExpectedLength, ExpectedAlign, ExpectedMin, ExpectedMax)  \
    do {                                                                         \
    ok((Desc)->Type == CmResourceTypeMemory,                                     \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeMemory);    \
    ok((Desc)->Option == (ExpectedOption),                                       \
       "Desc->Option = %u, expected %u\n", (Desc)->Option, (ExpectedOption));    \
    ok((Desc)->Flags == (ExpectedFlags),                                         \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));       \
    ok((Desc)->ShareDisposition == (ExpectedShare),                              \
       "Desc->ShareDisposition = %u, expected %u\n",                             \
       (Desc)->ShareDisposition, (ExpectedShare));                               \
    ok((Desc)->u.Memory.Length == (ExpectedLength),                              \
       "Desc->u.Memory.Length = %lx, expected %lx\n",                            \
       (Desc)->u.Memory.Length, (ExpectedLength));                               \
    ok((Desc)->u.Memory.Alignment == (ExpectedAlign),                            \
       "Desc->u.Memory.Alignment = %lx, expected %lx\n",                         \
       (Desc)->u.Memory.Alignment, (ExpectedAlign));                             \
    ok((Desc)->u.Memory.MinimumAddress.QuadPart == (ExpectedMin),                \
       "Desc->u.Memory.MinimumAddress = 0x%I64x, expected 0x%I64x\n",            \
       (Desc)->u.Memory.MinimumAddress.QuadPart, (ExpectedMin));                 \
    ok((Desc)->u.Memory.MaximumAddress.QuadPart == (ExpectedMax),                \
       "Desc->u.Memory.MaximumAddress = 0x%I64x, expected 0x%I64x\n",            \
       (Desc)->u.Memory.MaximumAddress.QuadPart, (ExpectedMax));                 \
    } while (0)

#define expect_cfg_req(Desc, ExpectedOption, ExpectedFlags, ExpectedShare,        \
                       ExpectedPriority, ExpectedRes1, ExpectedRes2)              \
    do {                                                                          \
    ok((Desc)->Type == CmResourceTypeConfigData,                                  \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeConfigData); \
    ok((Desc)->Option == (ExpectedOption),                                        \
       "Desc->Option = %u, expected %u\n", (Desc)->Option, (ExpectedOption));     \
    ok((Desc)->Flags == (ExpectedFlags),                                          \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));        \
    ok((Desc)->ShareDisposition == (ExpectedShare),                               \
       "Desc->ShareDisposition = %u, expected %u\n",                              \
       (Desc)->ShareDisposition, (ExpectedShare));                                \
    ok((Desc)->u.ConfigData.Priority == (ExpectedPriority),                       \
       "Desc->u.ConfigData.Priority = %lx, expected %lx\n",                       \
       (Desc)->u.ConfigData.Priority, (ExpectedPriority));                        \
    ok((Desc)->u.ConfigData.Reserved1 == (ExpectedRes1),                          \
       "Desc->u.ConfigData.Reserved1 = %lx, expected %lx\n",                      \
       (Desc)->u.ConfigData.Reserved2, (ExpectedRes1));                           \
    ok((Desc)->u.ConfigData.Reserved2 == (ExpectedRes2),                          \
       "Desc->u.ConfigData.Reserved2 = %lx, expected %lx\n",                      \
       (Desc)->u.ConfigData.Reserved2, (ExpectedRes2));                           \
    } while (0)

#define expect_port_res(Desc, ExpectedFlags, ExpectedShare, ExpectedLength, ExpectedStart) \
    do {                                                                                   \
    ok((Desc)->Type == CmResourceTypePort,                                                 \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypePort);                \
    ok((Desc)->Flags == (ExpectedFlags),                                                   \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));                 \
    ok((Desc)->ShareDisposition == (ExpectedShare),                                        \
       "Desc->ShareDisposition = %u, expected %u\n",                                       \
       (Desc)->ShareDisposition, (ExpectedShare));                                         \
    ok((Desc)->u.Port.Length == (ExpectedLength),                                          \
       "Desc->u.Port.Length = %lx, expected %lx\n",                                        \
       (Desc)->u.Port.Length, (ExpectedLength));                                           \
    ok((Desc)->u.Port.Start.QuadPart == (ExpectedStart),                                   \
       "Desc->u.Port.Start = 0x%I64x, expected 0x%I64x\n",                                 \
       (Desc)->u.Port.Start.QuadPart, (ExpectedStart));                                    \
    } while (0)

#define expect_irq_res(Desc, ExpectedFlags, ExpectedShare,                                 \
                       ExpectedLevel, ExpectedVector, ExpectedAffinity)                    \
    do {                                                                                   \
    ok((Desc)->Type == CmResourceTypeInterrupt,                                            \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeInterrupt);           \
    ok((Desc)->Flags == (ExpectedFlags),                                                   \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));                 \
    ok((Desc)->ShareDisposition == (ExpectedShare),                                        \
       "Desc->ShareDisposition = %u, expected %u\n",                                       \
       (Desc)->ShareDisposition, (ExpectedShare));                                         \
    ok((Desc)->u.Interrupt.Level == (ExpectedLevel),                                       \
       "Desc->u.Interrupt.Level = %lu\n", (Desc)->u.Interrupt.Level);                      \
    ok((Desc)->u.Interrupt.Vector == (ExpectedVector),                                     \
       "Desc->u.Interrupt.Vector = %lu\n", (Desc)->u.Interrupt.Vector);                    \
    ok((Desc)->u.Interrupt.Affinity == (ExpectedAffinity),                                 \
       "Desc->u.Interrupt.Affinity = %Ix\n", (Desc)->u.Interrupt.Affinity);                \
    } while (0)

#define expect_dma_res(Desc, ExpectedFlags, ExpectedShare, ExpectedChannel)                \
    do {                                                                                   \
    ok((Desc)->Type == CmResourceTypeDma,                                                  \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeDma);                 \
    ok((Desc)->Flags == (ExpectedFlags),                                                   \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));                 \
    ok((Desc)->ShareDisposition == (ExpectedShare),                                        \
       "Desc->ShareDisposition = %u, expected %u\n",                                       \
       (Desc)->ShareDisposition, (ExpectedShare));                                         \
    ok((Desc)->u.Dma.Channel == (ExpectedChannel),                                         \
       "Desc->u.Dma.Channel = %lu, expected %lu\n",                                        \
       (Desc)->u.Dma.Channel, (ExpectedChannel));                                          \
    ok((Desc)->u.Dma.Port == 0ul,                                                          \
       "Desc->u.Dma.Port = %lu, expected %lu\n",                                           \
       (Desc)->u.Dma.Port, 0ul);                                                           \
    ok((Desc)->u.Dma.Reserved1 == 0ul,                                                     \
       "Desc->u.Dma.Reserved1 = %lx, expected 0\n", (Desc)->u.Dma.Reserved1);              \
    } while (0)

#define expect_mem_res(Desc, ExpectedFlags, ExpectedShare, ExpectedLength, ExpectedStart)  \
    do {                                                                                   \
    ok((Desc)->Type == CmResourceTypeMemory,                                               \
       "Desc->Type = %u, expected %u\n", (Desc)->Type, CmResourceTypeMemory);              \
    ok((Desc)->Flags == (ExpectedFlags),                                                   \
       "Desc->Flags = %x, expected %x\n", (Desc)->Flags, (ExpectedFlags));                 \
    ok((Desc)->ShareDisposition == (ExpectedShare),                                        \
       "Desc->ShareDisposition = %u, expected %u\n",                                       \
       (Desc)->ShareDisposition, (ExpectedShare));                                         \
    ok((Desc)->u.Memory.Length == (ExpectedLength),                                        \
       "Desc->u.Memory.Length = %lx, expected %lx\n",                                      \
       (Desc)->u.Memory.Length, (ExpectedLength));                                         \
    ok((Desc)->u.Memory.Start.QuadPart == (ExpectedStart),                                 \
       "Desc->u.Memory.Start = 0x%I64x, expected 0x%I64x\n",                               \
       (Desc)->u.Memory.Start.QuadPart, (ExpectedStart));                                  \
    } while (0)
