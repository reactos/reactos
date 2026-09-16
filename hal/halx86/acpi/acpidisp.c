/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ACPI power management services for the ACPI driver
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "dispatch.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern LIST_ENTRY HalpAcpiTableCacheList;
extern FAST_MUTEX HalpAcpiTableCacheLock;
extern PACPI_BIOS_MULTI_NODE HalpAcpiMultiNode;

/* FADT feature flags */
#define FADT_FLAG_WBINVD            0x00000001
#define FADT_FLAG_WBINVD_FLUSH      0x00000002
#define FADT_FLAG_FIX_RTC           0x00000040
#define FADT_FLAG_RTC_S4            0x00000080

/* PM1 event block bits */
#define PM1STS_PWRBTN               0x0100
#define PM1STS_RTC                  0x0400
#define PM1STS_WAK                  0x8000
#define PM1EN_TMR                   0x0001
#define PM1EN_RTC                   0x0400

/* PM1 control block bits */
#define PM1CNT_SCI_EN               0x0001
#define PM1CNT_BM_RLD               0x0002
#define PM1CNT_IGNORE               0x0200
#define PM1CNT_SLP_TYP_SHIFT        10
#define PM1CNT_SLP_TYP_MASK         0x7
#define PM1CNT_SLP_EN               0x2000

/* Bits kept across the SLP_TYP write; the spec's "Ignore" bit belongs to firmware */
#define PM1CNT_KEEP_ON_SLEEP        (PM1CNT_SCI_EN | PM1CNT_BM_RLD | PM1CNT_IGNORE)

/* RTC alarm registers and register B bits */
#define RTC_ALARM_SECONDS           0x01
#define RTC_ALARM_MINUTES           0x03
#define RTC_ALARM_HOURS             0x05
#define RTC_REG_B_24H               0x02
#define RTC_REG_B_DM                0x04
#define RTC_REG_B_SQWE              0x08
#define RTC_REG_B_UIE               0x10
#define RTC_REG_B_AIE               0x20
#define RTC_REG_B_SET               0x80

#define E820_TYPE_ACPI_NVS          4

#define HALP_TICKS_PER_SECOND       10000000LL

typedef struct _HALP_SX_TARGET
{
    UCHAR SlpTyp[2];
    BOOLEAN FirmwareResume;
    BOOLEAN FlushCaches;
    BOOLEAN Valid;
} HALP_SX_TARGET, *PHALP_SX_TARGET;

typedef struct _HALP_CMOS_WRITE
{
    UCHAR Register;
    UCHAR Value;
} HALP_CMOS_WRITE, *PHALP_CMOS_WRITE;

typedef enum _HALP_RTC_WAKE
{
    HalpRtcWakeNever,
    HalpRtcWakeAlways,
    HalpRtcWakeIfFadtAllows
} HALP_RTC_WAKE;

typedef struct _HALP_SX_DESCRIPTOR
{
    POWER_STATE_HANDLER_TYPE Type;
    UCHAR ValuesIndex;
    UCHAR RtcWake;
    BOOLEAN FirmwareResume;
    BOOLEAN FlushCaches;
    BOOLEAN NeedsWakeVector;
    BOOLEAN NeedsHibernate;
} HALP_SX_DESCRIPTOR, *PHALP_SX_DESCRIPTOR;

typedef struct _HALP_NVS_REGION
{
    PVOID Mapping;
    PVOID Copy;
    SIZE_T Length;
} HALP_NVS_REGION, *PHALP_NVS_REGION;

typedef struct _HALP_TABLE_LIST
{
    ULONG Count;
    PDESCRIPTION_HEADER Table[ANYSIZE_ARRAY];
} HALP_TABLE_LIST, *PHALP_TABLE_LIST;

static PACPI_DRIVER_CALLBACKS HalpAcpiCallbacks;
static HALP_SX_TARGET HalpSxTargets[PowerStateMaximum];

static PUSHORT HalpPm1StatusPort[2];
static PUSHORT HalpPm1EnablePort[2];
static PUSHORT HalpPm1ControlPort[2];
static USHORT HalpPm1TimerEnable;

static PFACS HalpFacs;
static PULONG HalpFirmwareWakeVector;
static PHALP_TABLE_LIST HalpTableList;

static BOOLEAN HalpControllerIntact = TRUE;
static BOOLEAN HalpWakeSourcesEnabled;
static BOOLEAN HalpRtcAlarmPending;
static TIME_FIELDS HalpRtcAlarmFields;

static BOOLEAN volatile HalpParked[MAXIMUM_PROCESSORS];
static BOOLEAN volatile HalpUnparkRequested;

static PHALP_NVS_REGION HalpNvsRegions;
static ULONG HalpNvsRegionCount;

/* PRIVATE FUNCTIONS **********************************************************/

static
USHORT
NTAPI
HalpPm1Read(
    _In_reads_(2) PUSHORT *Ports)
{
    USHORT Value = 0;
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (Ports[i] != NULL)
        {
            Value |= READ_PORT_USHORT(Ports[i]);
        }
    }

    return Value;
}

static
VOID
NTAPI
HalpPm1Write(
    _In_reads_(2) PUSHORT *Ports,
    _In_ USHORT Value)
{
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (Ports[i] != NULL)
        {
            WRITE_PORT_USHORT(Ports[i], Value);
        }
    }
}

static
VOID
NTAPI
HalpSetupPm1Ports(VOID)
{
    PFADT Fadt = &HalpFixedAcpiDescTable;
    ULONG EventBlock[2] = { Fadt->pm1a_evt_blk_io_port, Fadt->pm1b_evt_blk_io_port };
    ULONG ControlBlock[2] = { Fadt->pm1a_ctrl_blk_io_port, Fadt->pm1b_ctrl_blk_io_port };
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (EventBlock[i] != 0)
        {
            /* The enable register is the upper half of the event block */
            HalpPm1StatusPort[i] = (PUSHORT)(ULONG_PTR)EventBlock[i];
            HalpPm1EnablePort[i] = (PUSHORT)(ULONG_PTR)(EventBlock[i] + Fadt->pm1_evt_len / 2);
        }

        if (ControlBlock[i] != 0)
        {
            HalpPm1ControlPort[i] = (PUSHORT)(ULONG_PTR)ControlBlock[i];
        }
    }
}

static
BOOLEAN
NTAPI
HalpAcpiCallbacksValid(VOID)
{
    return (HalpAcpiCallbacks != NULL) &&
           (HalpAcpiCallbacks->Signature == ACPI_DRIVER_CALLBACKS_SIGNATURE) &&
           (HalpAcpiCallbacks->Version >= 1);
}

static
VOID
NTAPI
HalpSetSciGpes(
    _In_ BOOLEAN Enable)
{
    if (HalpAcpiCallbacksValid() && (HalpAcpiCallbacks->SciGpeControl != NULL))
    {
        HalpAcpiCallbacks->SciGpeControl(Enable);
    }
}

static
LONGLONG
NTAPI
HalpTimeUntil(
    _In_ PTIME_FIELDS Time)
{
    TIME_FIELDS Now;
    LARGE_INTEGER Current, Target;

    HalQueryRealTimeClock(&Now);
    RtlTimeFieldsToTime(&Now, &Current);
    RtlTimeFieldsToTime(Time, &Target);

    return Target.QuadPart - Current.QuadPart;
}

static
VOID
NTAPI
HalpProgramRtcAlarm(
    _In_ PTIME_FIELDS Time)
{
    UCHAR DayIndex = HalpFixedAcpiDescTable.day_alarm_index;
    UCHAR MonthIndex = HalpFixedAcpiDescTable.month_alarm_index;
    HALP_CMOS_WRITE Alarm[5];
    UCHAR RegisterB;
    ULONG Retry, i;

    /* Register 0 marks an alarm the FADT does not provide */
    Alarm[0].Register = RTC_ALARM_HOURS;
    Alarm[0].Value = INT_BCD(Time->Hour);
    Alarm[1].Register = RTC_ALARM_MINUTES;
    Alarm[1].Value = INT_BCD(Time->Minute);
    Alarm[2].Register = RTC_ALARM_SECONDS;
    Alarm[2].Value = INT_BCD(Time->Second);
    Alarm[3].Register = DayIndex;
    Alarm[3].Value = INT_BCD(Time->Day);

    /* A month alarm means nothing without a day alarm */
    Alarm[4].Register = (DayIndex != 0) ? MonthIndex : 0;
    Alarm[4].Value = INT_BCD(Time->Month);

    HalpAcquireCmosSpinLock();

    for (Retry = 0; (Retry < 2000) && (HalpReadCmos(RTC_REGISTER_A) & RTC_REG_A_UIP); Retry++)
        KeStallExecutionProcessor(10);

    for (i = 0; i < RTL_NUMBER_OF(Alarm); i++)
    {
        if (Alarm[i].Register != 0)
            HalpWriteCmos(Alarm[i].Register, Alarm[i].Value);
    }

    /* BCD, 24 hour mode with the alarm interrupt on; update, square wave and SET off */
    RegisterB = HalpReadCmos(RTC_REGISTER_B);
    RegisterB &= (UCHAR)~(RTC_REG_B_SET | RTC_REG_B_UIE | RTC_REG_B_SQWE | RTC_REG_B_DM);
    RegisterB |= RTC_REG_B_AIE | RTC_REG_B_24H;
    HalpWriteCmos(RTC_REGISTER_B, RegisterB);

    /* Drop anything already latched */
    HalpReadCmos(RTC_REGISTER_C);
    HalpReadCmos(RTC_REGISTER_D);

    HalpReleaseCmosSpinLock();
}

static
BOOLEAN
NTAPI
HalpIsNvsEntry(
    _In_ PACPI_E820_ENTRY Entry)
{
    /* The upper half of the type holds extended attributes */
    return ((ULONG)Entry->Type == E820_TYPE_ACPI_NVS) &&
           (Entry->Length.QuadPart != 0) &&
           (Entry->Length.HighPart == 0);
}

static
VOID
NTAPI
HalpFreeNvsRegions(
    _In_ PHALP_NVS_REGION Regions,
    _In_ ULONG Count)
{
    ULONG i;

    for (i = 0; i < Count; i++)
    {
        MmUnmapIoSpace(Regions[i].Mapping, Regions[i].Length);
    }

    ExFreePoolWithTag(Regions, TAG_HAL);
}

static
VOID
NTAPI
HalpCaptureFirmwareNvs(VOID)
{
    PACPI_BIOS_MULTI_NODE Node = HalpAcpiMultiNode;
    PACPI_E820_ENTRY Entry;
    PHALP_NVS_REGION Regions;
    PUCHAR Storage;
    SIZE_T Total = 0;
    ULONG Count = 0, Used = 0, i;

    if ((Node == NULL) || (HalpNvsRegions != NULL))
    {
        return;
    }

    for (i = 0; i < Node->Count; i++)
    {
        if (HalpIsNvsEntry(&Node->E820Entry[i]))
        {
            Count++;
            Total += Node->E820Entry[i].Length.LowPart;
        }
    }

    if (Count == 0)
    {
        return;
    }

    /* One block holds the region array followed by the saved contents */
    Regions = ExAllocatePoolWithTag(NonPagedPool,
                                    Count * sizeof(*Regions) + Total,
                                    TAG_HAL);
    if (Regions == NULL)
    {
        DPRINT1("The ACPI NVS area will not be preserved\n");
        return;
    }

    Storage = (PUCHAR)&Regions[Count];

    for (i = 0; (i < Node->Count) && (Used < Count); i++)
    {
        Entry = &Node->E820Entry[i];
        if (!HalpIsNvsEntry(Entry))
        {
            continue;
        }

        Regions[Used].Length = Entry->Length.LowPart;
        Regions[Used].Copy = Storage;
        Regions[Used].Mapping = MmMapIoSpace(Entry->Base, Regions[Used].Length, MmNonCached);
        if (Regions[Used].Mapping == NULL)
        {
            HalpFreeNvsRegions(Regions, Used);
            return;
        }

        Storage += Regions[Used].Length;
        Used++;
    }

    HalpNvsRegionCount = Used;
    HalpNvsRegions = Regions;
}

static
VOID
NTAPI
HalpReleaseFirmwareNvs(VOID)
{
    PHALP_NVS_REGION Regions = HalpNvsRegions;
    ULONG Count = HalpNvsRegionCount;

    if (Regions == NULL)
    {
        return;
    }

    HalpNvsRegions = NULL;
    HalpNvsRegionCount = 0;
    HalpFreeNvsRegions(Regions, Count);
}

static
VOID
NTAPI
HalpSyncFirmwareNvs(
    _In_ BOOLEAN Save)
{
    PHALP_NVS_REGION Region;
    ULONG i;

    for (i = 0; i < HalpNvsRegionCount; i++)
    {
        Region = &HalpNvsRegions[i];
        if (Save)
        {
            RtlCopyMemory(Region->Copy, Region->Mapping, Region->Length);
        }
        else
        {
            RtlCopyMemory(Region->Mapping, Region->Copy, Region->Length);
        }
    }
}

static
VOID
NTAPI
HalpParkProcessor(
    _In_ ULONG Processor)
{
    HalpParked[Processor] = TRUE;

    while (!HalpUnparkRequested)
    {
        YieldProcessor();
    }

    HalpParked[Processor] = FALSE;
}

static
VOID
NTAPI
HalpWaitForParkedProcessors(
    _In_ LONG NumberProcessors)
{
    LONG Parked;
    ULONG i;

    do
    {
        YieldProcessor();

        Parked = 1;
        for (i = 1; i < MAXIMUM_PROCESSORS; i++)
        {
            if (HalpParked[i])
            {
                Parked++;
            }
        }
    } while (Parked < NumberProcessors);
}

static
VOID
NTAPI
HalpUnparkProcessors(VOID)
{
    ULONG i;

    HalpUnparkRequested = TRUE;

    for (i = 1; i < MAXIMUM_PROCESSORS; i++)
    {
        while (HalpParked[i])
        {
            YieldProcessor();
        }
    }

    HalpUnparkRequested = FALSE;
}

static
VOID
NTAPI
HalpWriteSleepControl(
    _In_ PHALP_SX_TARGET Target)
{
    USHORT Value;
    ULONG i;

    for (i = 0; i < 2; i++)
    {
        if (HalpPm1ControlPort[i] == NULL)
        {
            continue;
        }

        Value = READ_PORT_USHORT(HalpPm1ControlPort[i]);
        Value &= PM1CNT_KEEP_ON_SLEEP;
        Value |= (USHORT)((Target->SlpTyp[i] & PM1CNT_SLP_TYP_MASK) << PM1CNT_SLP_TYP_SHIFT);
        Value |= PM1CNT_SLP_EN;
        WRITE_PORT_USHORT(HalpPm1ControlPort[i], Value);
    }
}

static
VOID
NTAPI
HalpClearPm1Status(
    _In_ USHORT Bits)
{
    /* PM1 status bits are write 1 to clear */
    HalpPm1Write(HalpPm1StatusPort, Bits);
}

static
BOOLEAN
NTAPI
HalpPrepareForSleep(VOID)
{
    BOOLEAN FirmwareOwnsRtc = (HalpFixedAcpiDescTable.flags & FADT_FLAG_FIX_RTC) != 0;
    BOOLEAN AlarmImminent;
    USHORT Enable, RtcEnable;

    /* No point in sleeping if the alarm is about to go off */
    AlarmImminent = HalpRtcAlarmPending &&
                    (HalpTimeUntil(&HalpRtcAlarmFields) <= HALP_TICKS_PER_SECOND);

    Enable = HalpPm1Read(HalpPm1EnablePort);
    HalpPm1TimerEnable = Enable & PM1EN_TMR;

    if (FirmwareOwnsRtc)
        RtcEnable = Enable & PM1EN_RTC;
    else
        RtcEnable = HalpRtcAlarmPending ? PM1EN_RTC : 0;

    HalpPm1Write(HalpPm1EnablePort,
                 (USHORT)((Enable & ~(PM1EN_TMR | PM1EN_RTC)) | RtcEnable));
    HalpClearPm1Status(HalpPm1Read(HalpPm1StatusPort));

    if (HalpWakeSourcesEnabled)
    {
        if (HalpAcpiCallbacksValid() && (HalpAcpiCallbacks->WakeGpeArm != NULL))
            HalpAcpiCallbacks->WakeGpeArm();
    }
    else
    {
        HalpSetSciGpes(FALSE);
    }

    HalpSyncFirmwareNvs(TRUE);

    return !AlarmImminent;
}

static
VOID
NTAPI
HalpEnterSleepState(
    _In_ PHALP_SX_TARGET Target)
{
    if (HalpRtcAlarmPending && (HalpTimeUntil(&HalpRtcAlarmFields) > 0))
    {
        HalpProgramRtcAlarm(&HalpRtcAlarmFields);
        HalpClearPm1Status(PM1STS_RTC);
    }

    HalpClearPm1Status(PM1STS_WAK);

    if (Target->FlushCaches &&
        (HalpFixedAcpiDescTable.flags & (FADT_FLAG_WBINVD | FADT_FLAG_WBINVD_FLUSH)))
    {
        __wbinvd();
    }

    HalpWriteSleepControl(Target);

    while (!(HalpPm1Read(HalpPm1StatusPort) & PM1STS_WAK))
    {
        YieldProcessor();
    }
}

static
VOID
NTAPI
HalpFinishWake(VOID)
{
    USHORT Enable;

    Enable = HalpPm1Read(HalpPm1EnablePort);
    Enable &= (USHORT)~(PM1EN_RTC | PM1EN_TMR);
    HalpPm1Write(HalpPm1EnablePort, (USHORT)(Enable | HalpPm1TimerEnable));

    HalpRtcAlarmPending = FALSE;

    if (HalpFirmwareWakeVector != NULL)
    {
        *HalpFirmwareWakeVector = 0;
    }

    HalpControllerIntact = FALSE;
    HalpSetSciGpes(TRUE);
    HalpSyncFirmwareNvs(FALSE);
}

static
NTSTATUS
NTAPI
HalpSleepBootProcessor(
    _In_ PHALP_SX_TARGET Target,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_opt_ PVOID SystemContext)
{
    USHORT Control[2] = { 0, 0 };
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    if (HalpPrepareForSleep())
    {
        for (i = 0; i < 2; i++)
        {
            if (HalpPm1ControlPort[i] != NULL)
            {
                Control[i] = READ_PORT_USHORT(HalpPm1ControlPort[i]);
            }
        }

        if (SystemHandler != NULL)
        {
            Status = SystemHandler(SystemContext);
        }

        if (Status == STATUS_SUCCESS)
        {
            HalpEnterSleepState(Target);
        }
        else if (HalpAcpiCallbacksValid())
        {
            /* Back from a hibernation image or a failed handler, so ACPI mode needs a restart */
            if (HalpAcpiCallbacks->AcpiModeEnable != NULL)
            {
                HalpAcpiCallbacks->AcpiModeEnable(TRUE);
            }

            HalpSetSciGpes(TRUE);
        }

        for (i = 0; i < 2; i++)
        {
            if (HalpPm1ControlPort[i] != NULL)
            {
                WRITE_PORT_USHORT(HalpPm1ControlPort[i], Control[i]);
            }
        }
    }

    HalpFinishWake();

    return Status;
}

static
NTSTATUS
NTAPI
HalpAcpiSleepHandler(
    _In_opt_ PVOID Context,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_opt_ PVOID SystemContext,
    _In_ LONG NumberProcessors,
    _In_opt_ LONG volatile *Number)
{
    PHALP_SX_TARGET Target = Context;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Flags;
    ULONG Processor;
    KIRQL EntryIrql;

    UNREFERENCED_PARAMETER(Number);

    /* FIXME: no real mode resume path for states that wake through the FACS vector */
    if ((Target == NULL) || Target->FirmwareResume)
    {
        return STATUS_NOT_SUPPORTED;
    }

    Flags = __readeflags();
    _disable();
    EntryIrql = KeGetCurrentIrql();
    Processor = KeGetCurrentProcessorNumber();

    if (Processor != 0)
    {
        HalpParkProcessor(Processor);
    }
    else
    {
        HalpWaitForParkedProcessors(NumberProcessors);
        Status = HalpSleepBootProcessor(Target, SystemHandler, SystemContext);
        HalpUnparkProcessors();
    }

    if (KeGetCurrentIrql() > EntryIrql)
    {
        KeLowerIrql(EntryIrql);
    }

    __writeeflags(Flags);
    return Status;
}

static
VOID
NTAPI
HalpRegisterSxHandler(
    _In_ POWER_STATE_HANDLER_TYPE Type,
    _In_ PHAL_ACPI_SX_VALUES Values,
    _In_ BOOLEAN RtcWake,
    _In_ BOOLEAN FirmwareResume,
    _In_ BOOLEAN FlushCaches)
{
    PHALP_SX_TARGET Target = &HalpSxTargets[Type];
    POWER_STATE_HANDLER Handler;
    NTSTATUS Status;

    /* FIXME: no real mode resume path, so states that wake through the FACS vector are not offered */
    if (FirmwareResume)
        return;

    Target->SlpTyp[0] = Values->SlpTypA;
    Target->SlpTyp[1] = Values->SlpTypB;
    Target->FirmwareResume = FirmwareResume;
    Target->FlushCaches = FlushCaches;
    Target->Valid = TRUE;

    RtlZeroMemory(&Handler, sizeof(Handler));
    Handler.Type = Type;
    Handler.RtcWake = RtcWake;
    Handler.Handler = HalpAcpiSleepHandler;
    Handler.Context = Target;

    Status = ZwPowerInformation(SystemPowerStateHandler,
                                &Handler,
                                sizeof(Handler),
                                NULL,
                                0);
    /* FIXME: NtPowerInformation does not take SystemPowerStateHandler yet */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Power state handler %d not registered: 0x%lx\n", Type, Status);
    }
}

static
BOOLEAN
NTAPI
HalpTableIdMatches(
    _In_ PDESCRIPTION_HEADER Header,
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId)
{
    if (Header->Signature != Signature)
    {
        return FALSE;
    }

    if ((OemId != NULL) &&
        (strncmp(OemId, (PCSTR)Header->OEMID, sizeof(Header->OEMID)) != 0))
    {
        return FALSE;
    }

    if ((OemTableId != NULL) &&
        (strncmp(OemTableId, (PCSTR)Header->OEMTableID, sizeof(Header->OEMTableID)) != 0))
    {
        return FALSE;
    }

    return TRUE;
}

static
PDESCRIPTION_HEADER
NTAPI
HalpLookupCachedTable(
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId)
{
    PACPI_CACHED_TABLE Cached;
    PLIST_ENTRY Entry;

    for (Entry = HalpAcpiTableCacheList.Flink;
         Entry != &HalpAcpiTableCacheList;
         Entry = Entry->Flink)
    {
        Cached = CONTAINING_RECORD(Entry, ACPI_CACHED_TABLE, Links);
        if (HalpTableIdMatches(&Cached->Header, Signature, OemId, OemTableId))
        {
            return &Cached->Header;
        }
    }

    return NULL;
}

static
_Success_(return != FALSE)
BOOLEAN
NTAPI
HalpReadFirmwareHeader(
    _In_ PHYSICAL_ADDRESS Address,
    _Out_ PDESCRIPTION_HEADER Header)
{
    PDESCRIPTION_HEADER Mapping;

    if (Address.QuadPart == 0)
    {
        return FALSE;
    }

    Mapping = MmMapIoSpace(Address, sizeof(*Header), MmNonCached);
    if (Mapping == NULL)
    {
        return FALSE;
    }

    RtlCopyMemory(Header, Mapping, sizeof(*Header));
    MmUnmapIoSpace(Mapping, sizeof(*Header));

    return TRUE;
}

static
PDESCRIPTION_HEADER
NTAPI
HalpCacheFirmwareTable(
    _In_ PHYSICAL_ADDRESS Address,
    _In_ ULONG Length,
    _In_ BOOLEAN ReuseIdentical)
{
    PDESCRIPTION_HEADER Mapping, Table = NULL;
    PACPI_CACHED_TABLE Cached;
    PLIST_ENTRY Entry;

    Mapping = MmMapIoSpace(Address, Length, MmNonCached);
    if (Mapping == NULL)
    {
        return NULL;
    }

    if (ReuseIdentical)
    {
        for (Entry = HalpAcpiTableCacheList.Flink;
             Entry != &HalpAcpiTableCacheList;
             Entry = Entry->Flink)
        {
            Cached = CONTAINING_RECORD(Entry, ACPI_CACHED_TABLE, Links);
            if ((Cached->Header.Signature == Mapping->Signature) &&
                (Cached->Header.Length == Length) &&
                RtlEqualMemory(&Cached->Header, Mapping, Length))
            {
                Table = &Cached->Header;
                break;
            }
        }
    }

    if (Table == NULL)
    {
        Cached = ExAllocatePoolWithTag(NonPagedPool,
                                       FIELD_OFFSET(ACPI_CACHED_TABLE, Header) + Length,
                                       TAG_HAL);
        if (Cached != NULL)
        {
            RtlCopyMemory(&Cached->Header, Mapping, Length);
            InsertTailList(&HalpAcpiTableCacheList, &Cached->Links);
            Table = &Cached->Header;
        }
    }

    MmUnmapIoSpace(Mapping, Length);
    return Table;
}

static
PDESCRIPTION_HEADER
NTAPI
HalpCachedRootTable(VOID)
{
    PDESCRIPTION_HEADER Root;

    Root = HalpLookupCachedTable(XSDT_SIGNATURE, NULL, NULL);
    if (Root == NULL)
    {
        Root = HalpLookupCachedTable(RSDT_SIGNATURE, NULL, NULL);
    }

    return Root;
}

static
ULONG
NTAPI
HalpRootTableEntryCount(
    _In_ PDESCRIPTION_HEADER Root)
{
    ULONG EntrySize;

    if (Root->Length <= sizeof(*Root))
    {
        return 0;
    }

    EntrySize = (Root->Signature == XSDT_SIGNATURE) ? sizeof(PHYSICAL_ADDRESS) : sizeof(ULONG);
    return (Root->Length - sizeof(*Root)) / EntrySize;
}

static
PHYSICAL_ADDRESS
NTAPI
HalpRootTableEntry(
    _In_ PDESCRIPTION_HEADER Root,
    _In_ ULONG Index)
{
    PHYSICAL_ADDRESS Address;

    if (Root->Signature == XSDT_SIGNATURE)
    {
        Address = ((PXSDT)Root)->Tables[Index];
    }
    else
    {
        Address.QuadPart = ((PRSDT)Root)->Tables[Index];
    }

    return Address;
}

static
PDESCRIPTION_HEADER
NTAPI
HalpCacheMatchingTable(
    _In_ PHYSICAL_ADDRESS Address,
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId)
{
    DESCRIPTION_HEADER Header;

    if (!HalpReadFirmwareHeader(Address, &Header) ||
        (Header.Length < sizeof(Header)) ||
        !HalpTableIdMatches(&Header, Signature, OemId, OemTableId))
    {
        return NULL;
    }

    return HalpCacheFirmwareTable(Address, Header.Length, FALSE);
}

static
PDESCRIPTION_HEADER
NTAPI
HalpFetchFirmwareTable(
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId)
{
    PDESCRIPTION_HEADER Root, Table = NULL;
    PHYSICAL_ADDRESS Address;
    ULONG Index, Count;
    PFADT Fadt;

    if ((Signature == RSDT_SIGNATURE) || (Signature == XSDT_SIGNATURE))
    {
        return NULL;
    }

    /* The DSDT is only referenced from the FADT */
    if (Signature == DSDT_SIGNATURE)
    {
        Fadt = (PFADT)HalpLookupCachedTable(FADT_SIGNATURE, NULL, NULL);
        if (Fadt == NULL)
        {
            return NULL;
        }

        Address.QuadPart = Fadt->dsdt;
        if ((Fadt->Header.Revision >= 3) &&
            (Fadt->Header.Length >= RTL_SIZEOF_THROUGH_FIELD(FADT, x_dsdt)) &&
            (Fadt->x_dsdt.QuadPart != 0))
        {
            Address = Fadt->x_dsdt;
        }

        return HalpCacheMatchingTable(Address, Signature, OemId, OemTableId);
    }

    Root = HalpCachedRootTable();
    if (Root == NULL)
    {
        return NULL;
    }

    Count = HalpRootTableEntryCount(Root);
    for (Index = 0; (Index < Count) && (Table == NULL); Index++)
    {
        Address = HalpRootTableEntry(Root, Index);
        Table = HalpCacheMatchingTable(Address, Signature, OemId, OemTableId);
    }

    return Table;
}

/**
 * @brief
 * Walks the table cache for tables handed out in the table list. Root tables,
 * the FACS and the DSDT are left out since they are reached some other way.
 *
 * @return
 * The number of such tables, which are stored in Tables when it is not NULL.
 */
static
ULONG
NTAPI
HalpCollectListedTables(
    _Out_writes_opt_(return) PDESCRIPTION_HEADER *Tables)
{
    PACPI_CACHED_TABLE Cached;
    PLIST_ENTRY Entry;
    ULONG Found = 0;

    for (Entry = HalpAcpiTableCacheList.Flink;
         Entry != &HalpAcpiTableCacheList;
         Entry = Entry->Flink)
    {
        Cached = CONTAINING_RECORD(Entry, ACPI_CACHED_TABLE, Links);

        switch (Cached->Header.Signature)
        {
            case RSDT_SIGNATURE:
            case XSDT_SIGNATURE:
            case FACS_SIGNATURE:
            case DSDT_SIGNATURE:
                break;

            default:
                if (Tables != NULL)
                    Tables[Found] = &Cached->Header;
                Found++;
                break;
        }
    }

    return Found;
}

static
VOID
NTAPI
HalpCacheRootEntries(
    _In_ PDESCRIPTION_HEADER Root)
{
    DESCRIPTION_HEADER Header;
    PHYSICAL_ADDRESS Address;
    ULONG Index, Count;

    /* Every entry is cached on its own, even when signatures repeat */
    Count = HalpRootTableEntryCount(Root);
    for (Index = 0; Index < Count; Index++)
    {
        Address = HalpRootTableEntry(Root, Index);
        if (HalpReadFirmwareHeader(Address, &Header) &&
            (Header.Length >= sizeof(Header)))
        {
            HalpCacheFirmwareTable(Address, Header.Length, TRUE);
        }
    }
}

static
VOID
NTAPI
HalpBuildTableList(VOID)
{
    PDESCRIPTION_HEADER Root;
    PHALP_TABLE_LIST List;
    ULONG Count;

    Root = HalpCachedRootTable();
    if (Root == NULL)
        return;

    HalpCacheRootEntries(Root);

    Count = HalpCollectListedTables(NULL);
    List = ExAllocatePoolWithTag(NonPagedPool,
                                 FIELD_OFFSET(HALP_TABLE_LIST, Table) +
                                 Count * sizeof(List->Table[0]),
                                 TAG_HAL);
    if (List == NULL)
        return;

    List->Count = HalpCollectListedTables(List->Table);
    HalpTableList = List;
}

static
PFACS
NTAPI
HalpMapFacs(VOID)
{
    DESCRIPTION_HEADER Header;
    PHYSICAL_ADDRESS Address;
    PFADT Fadt;

    if (HalpFacs != NULL)
    {
        return HalpFacs;
    }

    Fadt = HalpAcpiGetTable(NULL, FADT_SIGNATURE);
    if (Fadt == NULL)
    {
        /* Parameter 1 names the missing table */
        DPRINT1("HAL: No FADT to locate the FACS\n");
        KeBugCheckEx(ACPI_BIOS_ERROR, FADT_SIGNATURE, 0, 0, 0);
    }

    Address.QuadPart = Fadt->facs;
    if (HalpReadFirmwareHeader(Address, &Header) &&
        (Header.Signature == FACS_SIGNATURE) &&
        (Header.Length >= RTL_SIZEOF_THROUGH_FIELD(FACS, Version)))
    {
        HalpFacs = MmMapIoSpace(Address, Header.Length, MmNonCached);
    }

    return HalpFacs;
}

/* DISPATCH ENTRIES ***********************************************************/

static
VOID
NTAPI
HalpAcpiPmTimerOverflow(VOID)
{
    /* Timekeeping does not use the PM timer */
    NOTHING;
}

static
VOID
NTAPI
HalpAcpiReportSleepStates(
    _In_reads_(HAL_ACPI_SX_COUNT) PHAL_ACPI_SX_VALUES SxValues,
    _Out_ PULONG InterruptModel)
{
    /* Soft off is also used by the power button while the system is halted */
    static const HALP_SX_DESCRIPTOR SxStates[] =
    {
        /* Type                      Values RtcWake                  FwResume Flush  WakeVec Hiber */
        { PowerStateSleeping1,         0, HalpRtcWakeAlways,       FALSE,   TRUE,  FALSE,  FALSE },
        { PowerStateSleeping2,         1, HalpRtcWakeAlways,       TRUE,    TRUE,  TRUE,   FALSE },
        { PowerStateSleeping3,         2, HalpRtcWakeAlways,       TRUE,    TRUE,  TRUE,   FALSE },
        { PowerStateSleeping4Firmware, 2, HalpRtcWakeIfFadtAllows, TRUE,    TRUE,  TRUE,   TRUE  },
        { PowerStateSleeping4,         3, HalpRtcWakeIfFadtAllows, FALSE,   FALSE, FALSE,  TRUE  },
        { PowerStateShutdownOff,       4, HalpRtcWakeNever,        FALSE,   FALSE, FALSE,  FALSE },
    };
    const HALP_SX_DESCRIPTOR *State;
    BOOLEAN HaveWakeVector, Hibernate, RtcWake;
    ULONG i;

    PAGED_CODE();

    HalpWakeSourcesEnabled = TRUE;
    HalpRtcAlarmPending = FALSE;

    *InterruptModel = HalpInterruptControllerType;

    HaveWakeVector = (HalpFirmwareWakeVector != NULL);
    /* FIXME: the image restores only the boot processor, parked ones are not restarted */
    Hibernate = SxValues[3].Present && (KeNumberProcessors == 1);

    for (i = 0; i < RTL_NUMBER_OF(SxStates); i++)
    {
        State = &SxStates[i];

        if (!SxValues[State->ValuesIndex].Present ||
            (State->NeedsWakeVector && !HaveWakeVector) ||
            (State->NeedsHibernate && !Hibernate))
        {
            continue;
        }

        if (State->RtcWake == HalpRtcWakeIfFadtAllows)
            RtcWake = (HalpFixedAcpiDescTable.flags & FADT_FLAG_RTC_S4) != 0;
        else
            RtcWake = (State->RtcWake == HalpRtcWakeAlways);

        HalpRegisterSxHandler(State->Type,
                              &SxValues[State->ValuesIndex],
                              RtcWake,
                              State->FirmwareResume,
                              State->FlushCaches);
    }
}

static
ULONG
NTAPI
HalpAcpiCapabilityFlags(VOID)
{
    return 1;
}

static
BOOLEAN
NTAPI
HalpAcpiInterruptControllerIntact(VOID)
{
    return HalpControllerIntact;
}

static
VOID
NTAPI
HalpAcpiInterruptControllerRestore(VOID)
{
    HalpRestoreInterruptController();
    HalpControllerIntact = TRUE;
}

static
VOID
NTAPI
HalpAcpiLegacyPciBusLimit(
    _In_ ULONG BusNumber)
{
    if (BusNumber > HalpMaxPciBus)
    {
        HalpMaxPciBus = BusNumber;
    }
}

static
PVOID
NTAPI
HalpAcpiFindTable(
    _In_ ULONG Signature,
    _In_opt_ PCSTR OemId,
    _In_opt_ PCSTR OemTableId)
{
    PDESCRIPTION_HEADER Table;

    PAGED_CODE();

    ExAcquireFastMutex(&HalpAcpiTableCacheLock);

    Table = HalpLookupCachedTable(Signature, OemId, OemTableId);
    if (Table == NULL)
    {
        Table = HalpFetchFirmwareTable(Signature, OemId, OemTableId);
    }

    ExReleaseFastMutex(&HalpAcpiTableCacheLock);

    return Table;
}

static
PVOID
NTAPI
HalpAcpiRootPointer(VOID)
{
    return HalpAcpiMultiNode;
}

static
PVOID
NTAPI
HalpAcpiFacsTable(VOID)
{
    PFACS Facs;

    PAGED_CODE();

    ExAcquireFastMutex(&HalpAcpiTableCacheLock);
    Facs = HalpMapFacs();
    ExReleaseFastMutex(&HalpAcpiTableCacheLock);

    return Facs;
}

static
PVOID
NTAPI
HalpAcpiTableList(VOID)
{
    PHALP_TABLE_LIST List;

    PAGED_CODE();

    ExAcquireFastMutex(&HalpAcpiTableCacheLock);

    if (HalpTableList == NULL)
    {
        HalpBuildTableList();
    }
    List = HalpTableList;

    ExReleaseFastMutex(&HalpAcpiTableCacheLock);

    return List;
}

static
VOID
NTAPI
HalpAcpiSetWakeEnable(
    _In_ BOOLEAN Enable)
{
    HalpWakeSourcesEnabled = Enable;
    HalpRtcAlarmPending = FALSE;
}

static
VOID
NTAPI
HalpAcpiSetWakeAlarm(
    _In_ ULONGLONG AlarmTime,
    _In_ PTIME_FIELDS TimeFields)
{
    if (AlarmTime == 0)
    {
        HalpRtcAlarmPending = FALSE;
        return;
    }

    if (HalpTimeUntil(TimeFields) < 10 * HALP_TICKS_PER_SECOND)
    {
        DPRINT1("Wake alarm is less than 10 seconds away, not set\n");
        HalpRtcAlarmPending = FALSE;
        return;
    }

    HalpRtcAlarmFields = *TimeFields;
    HalpRtcAlarmPending = TRUE;
}

static
VOID
NTAPI
HalpAcpiPowerStateNotify(
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2)
{
    UNREFERENCED_PARAMETER(CallbackContext);

    if ((ULONG_PTR)Argument1 == PO_CB_SYSTEM_STATE_LOCK)
    {
        switch ((ULONG_PTR)Argument2)
        {
            /* A system power transition is about to start */
            case 0:
                HalpCaptureFirmwareNvs();
                break;

            /* The transition is over */
            case 1:
                HalpReleaseFirmwareNvs();
                break;

            default:
                break;
        }
    }
}

static const HAL_ACPI_SERVICES HalpAcpiServices =
{
    HAL_ACPI_SERVICES_SIGNATURE,
    HAL_ACPI_SERVICES_VERSION,
    HaliAcpiTimerInit,
    HalpAcpiPmTimerOverflow,
    HalpAcpiReportSleepStates,
    HalpAcpiCapabilityFlags,
    HalpAcpiInterruptControllerIntact,
    HalpAcpiInterruptControllerRestore,
    HaliPciInterfaceReadConfig,
    HaliPciInterfaceWriteConfig,
    HalpGetInterruptControllerVersion,
    HalpAcpiLegacyPciBusLimit,
    HalpIsInterruptInputValid,
    HalpAcpiFindTable,
    HalpAcpiRootPointer,
    HalpAcpiFacsTable,
    HalpAcpiTableList
};

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Powers the machine off when the power button is pressed while the system
 * sits halted after a bugcheck or with the boot display owning the screen.
 */
VOID
NTAPI
HalpAcpiPollPowerButton(VOID)
{
    PHALP_SX_TARGET SoftOff = &HalpSxTargets[PowerStateShutdownOff];
    BOOLEAN Halted;
    USHORT Pending;

    if (!SoftOff->Valid)
        return;

    Halted = (KiBugCheckData[0] != 0) || InbvCheckDisplayOwnership();
    if (!Halted)
        return;

    /* A press that is not part of a wake event */
    Pending = HalpPm1Read(HalpPm1StatusPort);
    if ((Pending & (PM1STS_PWRBTN | PM1STS_WAK)) != PM1STS_PWRBTN)
        return;

    /* Soft off is only attempted once */
    SoftOff->Valid = FALSE;

    HalpSetSciGpes(FALSE);
    HalpClearPm1Status(Pending);
    HalpWriteSleepControl(SoftOff);
}

/**
 * @brief
 * HalInitPowerManagement: takes the ACPI driver's callbacks and returns the
 * HAL services table.
 */
NTSTATUS
NTAPI
HaliInitPowerManagement(
    _In_ PPM_DISPATCH_TABLE PmDriverDispatchTable,
    _Out_ PPM_DISPATCH_TABLE *PmHalDispatchTable)
{
    UNICODE_STRING CallbackName = RTL_CONSTANT_STRING(L"\\Callback\\PowerState");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCALLBACK_OBJECT CallbackObject;
    PFACS Facs;
    NTSTATUS Status;

    PAGED_CODE();

    HalpAcpiCallbacks = (PACPI_DRIVER_CALLBACKS)PmDriverDispatchTable;
    if (!HalpAcpiCallbacksValid())
    {
        DPRINT1("Unrecognized ACPI driver callback table %p\n", PmDriverDispatchTable);
    }

    HalpSetupPm1Ports();

    *PmHalDispatchTable = (PPM_DISPATCH_TABLE)&HalpAcpiServices;

    HalSetWakeEnable = HalpAcpiSetWakeEnable;
    HalSetWakeAlarm = HalpAcpiSetWakeAlarm;

    InitializeObjectAttributes(&ObjectAttributes,
                               &CallbackName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = ExCreateCallback(&CallbackObject, &ObjectAttributes, FALSE, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No power state callback: 0x%lx\n", Status);
        return Status;
    }

    ExRegisterCallback(CallbackObject, HalpAcpiPowerStateNotify, NULL);

    Facs = HalpAcpiFacsTable();
    if (Facs != NULL)
    {
        HalpFirmwareWakeVector = &Facs->pFirmwareWakingVector;
    }

    return STATUS_SUCCESS;
}

/* EOF */
