/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Boot Video Driver support
 * COPYRIGHT:   Copyright 2007 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2010 Aleksey Bragin (aleksey@reactos.org)
 *              Copyright 2015-2022 Hermès Bélusca-Maïto
 */

#include <ntoskrnl.h>
#include "inbv/logo.h"
#define NDEBUG
#include <debug.h>

/* AGENT-MODIFIED: GOP video function prototypes for UEFI boot */
BOOLEAN NTAPI GopVidInitialize(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID    NTAPI GopVidCleanUp(VOID);
VOID    NTAPI GopVidResetDisplay(_In_ BOOLEAN HalReset);
VOID    NTAPI GopVidSolidColorFill(_In_ ULONG Left, _In_ ULONG Top, _In_ ULONG Right, _In_ ULONG Bottom, _In_ UCHAR Color);
VOID    NTAPI GopVidBufferToScreenBlt(_In_reads_bytes_(Delta * Height) PUCHAR Buffer, _In_ ULONG Left, _In_ ULONG Top, _In_ ULONG Width, _In_ ULONG Height, _In_ ULONG Delta);
VOID    NTAPI GopVidScreenToBufferBlt(_Out_writes_bytes_(Delta * Height) PUCHAR Buffer, _In_ ULONG Left, _In_ ULONG Top, _In_ ULONG Width, _In_ ULONG Height, _In_ ULONG Delta);
VOID    NTAPI GopVidDisplayString(_In_z_ PUCHAR String);
VOID    NTAPI GopVidBitBlt(_In_ PUCHAR Buffer, _In_ ULONG Left, _In_ ULONG Top);

/* GLOBALS *******************************************************************/

/* Enable this define if you want Inbv to use coloured headless mode. */
// #define INBV_HEADLESS_COLORS

typedef struct _INBV_PROGRESS_STATE
{
    ULONG Floor;   /* 0..100 */
    ULONG Ceiling; /* 0..100 */
    ULONG Bias;    /* Ceiling - Floor */
} INBV_PROGRESS_STATE;

typedef struct _BT_PROGRESS_INDICATOR
{
    ULONG Count;
    ULONG Expected;
    ULONG Percentage;
} BT_PROGRESS_INDICATOR, *PBT_PROGRESS_INDICATOR;

static KSPIN_LOCK BootDriverLock;
static KIRQL InbvOldIrql;
static INBV_DISPLAY_STATE InbvDisplayState = INBV_DISPLAY_STATE_DISABLED;
BOOLEAN InbvBootDriverInstalled = FALSE;
static INBV_RESET_DISPLAY_PARAMETERS InbvResetDisplayParameters = NULL;

/* AGENT-MODIFIED: Track if we're using GOP video (UEFI) or bootvid (BIOS) */
static BOOLEAN InbvUsingGopVideo = FALSE;

static BOOLEAN InbvDisplayDebugStrings = FALSE;
static INBV_DISPLAY_STRING_FILTER InbvDisplayFilter = NULL;

ULONG ProgressBarLeft = 0, ProgressBarTop = 0;
BOOLEAN ShowProgressBar = FALSE;
static INBV_PROGRESS_STATE InbvProgressState;
static BT_PROGRESS_INDICATOR InbvProgressIndicator = {0, 25, 0};

static ULONG ResourceCount = 0;
static PUCHAR ResourceList[1 + IDB_MAX_RESOURCES]; // First entry == NULL, followed by 'ResourceCount' entries.

/*
 * Headless terminal text colors
 */
#ifdef INBV_HEADLESS_COLORS
static const UCHAR CGA_TO_ANSI_COLOR_TABLE[16] =
{
    0, 4, 2, 6, 1, 5, 3, 7,
    60, 64, 62, 66, 61, 65, 63, 67
};
#define CGA_TO_ANSI_COLOR(CgaColor) CGA_TO_ANSI_COLOR_TABLE[(CgaColor) & 0x0F]
#endif

/* Default colors: text in white, background in black */
static ULONG InbvTerminalTextColor = 37;
static ULONG InbvTerminalBkgdColor = 40;

/* FUNCTIONS *****************************************************************/

#define RT_BITMAP   MAKEINTRESOURCE(2)

CODE_SEG("INIT")
static
PVOID
FindBitmapResource(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG ResourceId)
{
    UNICODE_STRING UpString = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    UNICODE_STRING MpString = RTL_CONSTANT_STRING(L"ntkrnlmp.exe");
    PLIST_ENTRY NextEntry, ListHead;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    LDR_RESOURCE_INFO ResourceInfo;
    NTSTATUS Status;
    PVOID Data = NULL;

    /* Loop the driver list */
    ListHead = &LoaderBlock->LoadOrderListHead;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &UpString, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &MpString, TRUE))
        {
            break;
        }
    }

    if (NextEntry != ListHead)
    {
        ResourceInfo.Type = RT_BITMAP;
        ResourceInfo.Name = ResourceId;
        ResourceInfo.Language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

        Status = LdrFindResource_U(LdrEntry->DllBase,
                                   &ResourceInfo,
                                   RESOURCE_DATA_LEVEL,
                                   &ResourceDataEntry);
        if (NT_SUCCESS(Status))
        {
            ULONG Size = 0;
            Status = LdrAccessResource(LdrEntry->DllBase, ResourceDataEntry, &Data, &Size);
            if ((Data) && (ResourceId < 3))
            {
                KiBugCheckData[4] ^= RtlComputeCrc32(0, Data, Size);
            }
            if (!NT_SUCCESS(Status)) Data = NULL;
        }
    }

    return Data;
}

PUCHAR
NTAPI
InbvGetResourceAddress(
    _In_ ULONG ResourceNumber)
{
    if (ResourceNumber > ResourceCount) return NULL;
    return ResourceList[ResourceNumber];
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
InbvDriverInitialize(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG Count)
{
    PCHAR CommandLine;
    BOOLEAN ResetMode = FALSE; // By default do not reset the video mode
    ULONG i;

    if (InbvBootDriverInstalled) return TRUE;

    KeInitializeSpinLock(&BootDriverLock);

    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        CommandLine = (LoaderBlock->LoadOptions ? _strupr(LoaderBlock->LoadOptions) : NULL);
        ResetMode   = (CommandLine == NULL) || (strstr(CommandLine, "BOOTLOGO") == NULL);
    }

    /* AGENT-MODIFIED: UEFI vs BIOS selection */
    if (LoaderBlock->FirmwareInformation.FirmwareTypeEfi)
    {
        DPRINT1("[AGENT] UEFI boot detected, initializing GOP video driver\n");
        InbvBootDriverInstalled = GopVidInitialize(LoaderBlock);
        InbvUsingGopVideo = InbvBootDriverInstalled;

        if (!InbvBootDriverInstalled)
        {
            DPRINT1("[AGENT] GOP init failed, falling back to bootvid\n");
            InbvBootDriverInstalled = VidInitialize(ResetMode);
            InbvUsingGopVideo = FALSE;
        }
    }
    else
    {
        DPRINT1("[AGENT] BIOS boot detected, initializing bootvid\n");
        InbvBootDriverInstalled = VidInitialize(ResetMode);
        InbvUsingGopVideo = FALSE;
    }

    if (InbvBootDriverInstalled)
    {
        ResourceCount = min(Count, RTL_NUMBER_OF(ResourceList) - 1);
        for (i = 1; i <= ResourceCount; i++)
            ResourceList[i] = FindBitmapResource(LoaderBlock, i);

        /* 0..100 subset by default */
        InbvSetProgressBarSubset(0, 100);
        // BootAnimInitialize(LoaderBlock, Count);
    }

    return InbvBootDriverInstalled;
}

/* ---- Lock helpers (fixed for proper IRQL semantics) --------------------- */

VOID
NTAPI
InbvAcquireLock(VOID)
{
    /* Raise to DPC and acquire at DPC level */
    KeRaiseIrql(DISPATCH_LEVEL, &InbvOldIrql);
    KeAcquireSpinLockAtDpcLevel(&BootDriverLock);
}

VOID
NTAPI
InbvReleaseLock(VOID)
{
    /* Release at DPC and restore previous IRQL */
    KeReleaseSpinLockFromDpcLevel(&BootDriverLock);
    KeLowerIrql(InbvOldIrql);
}

/* ------------------------------------------------------------------------ */

VOID
NTAPI
InbvEnableBootDriver(
    _In_ BOOLEAN Enable)
{
    if (InbvBootDriverInstalled)
    {
        if (InbvDisplayState >= INBV_DISPLAY_STATE_LOST) return;

        InbvAcquireLock();

        /* Clean screen using the active backend if we own it */
        if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
        {
            if (InbvUsingGopVideo)
                GopVidCleanUp();
            else
                VidCleanUp();
        }

        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED
                                  : INBV_DISPLAY_STATE_DISABLED;

        InbvReleaseLock();
    }
    else
    {
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED
                                  : INBV_DISPLAY_STATE_DISABLED;
    }
}

VOID
NTAPI
InbvAcquireDisplayOwnership(VOID)
{
    if ((InbvResetDisplayParameters) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_LOST))
    {
        InbvResetDisplayParameters(80, 50);
    }

    InbvDisplayState = INBV_DISPLAY_STATE_OWNED;
}

VOID
NTAPI
InbvSetDisplayOwnership(
    _In_ BOOLEAN DisplayOwned)
{
    InbvDisplayState = DisplayOwned ? INBV_DISPLAY_STATE_OWNED
                                    : INBV_DISPLAY_STATE_LOST;
}

BOOLEAN
NTAPI
InbvCheckDisplayOwnership(VOID)
{
    return InbvDisplayState != INBV_DISPLAY_STATE_LOST;
}

INBV_DISPLAY_STATE
NTAPI
InbvGetDisplayState(VOID)
{
    return InbvDisplayState;
}

BOOLEAN
NTAPI
InbvDisplayString(
    _In_ PCHAR String)
{
    if (InbvDisplayState != INBV_DISPLAY_STATE_OWNED)
        return FALSE;

    if (!InbvDisplayDebugStrings)
        return TRUE;

    if (InbvDisplayFilter) InbvDisplayFilter(&String);

    InbvAcquireLock();

    if (InbvBootDriverInstalled)
    {
        if (InbvUsingGopVideo)
            GopVidDisplayString((PUCHAR)String); /* May be a stub */
        else
            VidDisplayString((PUCHAR)String);
    }

    HeadlessDispatch(HeadlessCmdPutString,
                     String,
                     strlen(String) + sizeof(ANSI_NULL),
                     NULL,
                     NULL);

    InbvReleaseLock();
    return TRUE;
}

BOOLEAN
NTAPI
InbvEnableDisplayString(
    _In_ BOOLEAN Enable)
{
    BOOLEAN OldSetting = InbvDisplayDebugStrings;
    InbvDisplayDebugStrings = Enable;
    return OldSetting;
}

VOID
NTAPI
InbvInstallDisplayStringFilter(
    _In_ INBV_DISPLAY_STRING_FILTER DisplayFilter)
{
    InbvDisplayFilter = DisplayFilter;
}

BOOLEAN
NTAPI
InbvIsBootDriverInstalled(VOID)
{
    return InbvBootDriverInstalled;
}

VOID
NTAPI
InbvNotifyDisplayOwnershipLost(
    _In_ INBV_RESET_DISPLAY_PARAMETERS Callback)
{
    if (InbvBootDriverInstalled)
    {
        InbvAcquireLock();

        if (InbvDisplayState != INBV_DISPLAY_STATE_LOST)
        {
            /* AGENT-MODIFIED: Cleanup using the active backend */
            if (InbvUsingGopVideo)
                GopVidCleanUp();
            else
                VidCleanUp();
        }

        InbvResetDisplayParameters = Callback;
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;

        InbvReleaseLock();
    }
    else
    {
        InbvResetDisplayParameters = Callback;
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;
    }
}

BOOLEAN
NTAPI
InbvResetDisplay(VOID)
{
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        if (InbvUsingGopVideo)
            GopVidResetDisplay(TRUE);
        else
            VidResetDisplay(TRUE);
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
InbvSetScrollRegion(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom)
{
    /* Text mode scroll region only applies to BOOTVID (VGA text). */
    if (!InbvUsingGopVideo)
        VidSetScrollRegion(Left, Top, Right, Bottom);
    /* else: GOP path has no text-mode scroll; intentionally no-op. */
}

VOID
NTAPI
InbvSetTextColor(
    _In_ ULONG Color)
{
    HEADLESS_CMD_SET_COLOR HeadlessSetColor;

#ifdef INBV_HEADLESS_COLORS
    InbvTerminalTextColor = 30 + CGA_TO_ANSI_COLOR(Color);
#else
    InbvTerminalTextColor = 37;
#endif
    HeadlessSetColor.TextColor = InbvTerminalTextColor;
    HeadlessSetColor.BkgdColor = InbvTerminalBkgdColor;
    HeadlessDispatch(HeadlessCmdSetColor,
                     &HeadlessSetColor,
                     sizeof(HeadlessSetColor),
                     NULL,
                     NULL);

    /* BOOTVID can update text color; GOP backend has no text-mode attributes. */
    if (!InbvUsingGopVideo)
        VidSetTextColor(Color);
}

VOID
NTAPI
InbvSolidColorFill(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG Right,
    _In_ ULONG Bottom,
    _In_ ULONG Color)
{
    HEADLESS_CMD_SET_COLOR HeadlessSetColor;

    if (InbvDisplayState != INBV_DISPLAY_STATE_OWNED)
        return;

    InbvAcquireLock();

    if (InbvBootDriverInstalled)
    {
        if (InbvUsingGopVideo)
            GopVidSolidColorFill(Left, Top, Right, Bottom, (UCHAR)Color);
        else
            VidSolidColorFill(Left, Top, Right, Bottom, (UCHAR)Color);
    }

#ifdef INBV_HEADLESS_COLORS
    InbvTerminalBkgdColor = 40 + CGA_TO_ANSI_COLOR(Color);
#else
    InbvTerminalBkgdColor = 40;
#endif
    HeadlessSetColor.TextColor = InbvTerminalTextColor;
    HeadlessSetColor.BkgdColor = InbvTerminalBkgdColor;
    HeadlessDispatch(HeadlessCmdSetColor,
                     &HeadlessSetColor,
                     sizeof(HeadlessSetColor),
                     NULL,
                     NULL);
    HeadlessDispatch(HeadlessCmdClearDisplay, NULL, 0, NULL, NULL);

    InbvReleaseLock();
}

VOID
NTAPI
InbvBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y)
{
    if (!(InbvBootDriverInstalled &&
          (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)))
        return;

    InbvAcquireLock();

    if (InbvUsingGopVideo)
        GopVidBitBlt(Buffer, X, Y);
    else
        VidBitBlt(Buffer, X, Y);

    InbvReleaseLock();
}

VOID
NTAPI
InbvBufferToScreenBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    if (!(InbvBootDriverInstalled &&
          (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)))
        return;

    InbvAcquireLock();

    if (InbvUsingGopVideo)
        GopVidBufferToScreenBlt(Buffer, X, Y, Width, Height, Delta);
    else
        VidBufferToScreenBlt(Buffer, X, Y, Width, Height, Delta);

    InbvReleaseLock();
}

VOID
NTAPI
InbvScreenToBufferBlt(
    _Out_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG Delta)
{
    if (!(InbvBootDriverInstalled &&
          (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)))
        return;

    InbvAcquireLock();

    if (InbvUsingGopVideo)
        GopVidScreenToBufferBlt(Buffer, X, Y, Width, Height, Delta);
    else
        VidScreenToBufferBlt(Buffer, X, Y, Width, Height, Delta);

    InbvReleaseLock();
}

/* Progress bar API **********************************************************/

VOID
NTAPI
InbvSetProgressBarCoordinates(
    _In_ ULONG Left,
    _In_ ULONG Top)
{
    ProgressBarLeft = Left;
    ProgressBarTop  = Top;
    ShowProgressBar = TRUE;
}

CODE_SEG("INIT")
VOID
NTAPI
InbvIndicateProgress(VOID)
{
    ULONG Percentage;

    InbvProgressIndicator.Count++;

    Percentage = 100 * InbvProgressIndicator.Count /
                       InbvProgressIndicator.Expected;
    Percentage = min(Percentage, 99);

    if (Percentage != InbvProgressIndicator.Percentage)
    {
        InbvProgressIndicator.Percentage = Percentage;
        InbvUpdateProgressBar(Percentage);
    }
}

VOID
NTAPI
InbvSetProgressBarSubset(
    _In_ ULONG Floor,
    _In_ ULONG Ceiling)
{
    ASSERT(Floor < Ceiling);
    ASSERT(Ceiling <= 100);

    InbvProgressState.Floor   = Floor;             /* 0..100 */
    InbvProgressState.Ceiling = Ceiling;           /* 0..100 */
    InbvProgressState.Bias    = Ceiling - Floor;   /* 0..100 */
}

VOID
NTAPI
InbvUpdateProgressBar(
    _In_ ULONG Percentage)
{
    ULONG TotalProgress;

    if (ShowProgressBar &&
        InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Map subrange [Floor, Ceiling] using integer math, clamp to 0..100 */
        TotalProgress = InbvProgressState.Floor +
                        (Percentage * InbvProgressState.Bias) / 100;
        if (TotalProgress > 100) TotalProgress = 100;

        BootAnimTickProgressBar(TotalProgress);
    }
}

/* NtDisplayString ***********************************************************/

NTSTATUS
NTAPI
NtDisplayString(
    IN PUNICODE_STRING DisplayString)
{
    NTSTATUS Status;
    UNICODE_STRING CapturedString;
    OEM_STRING OemString;
    ULONG OemLength;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    Status = ProbeAndCaptureUnicodeString(&CapturedString, PreviousMode, DisplayString);
    if (!NT_SUCCESS(Status))
        return Status;

    if (CapturedString.Length == 0 || CapturedString.Buffer == NULL)
    {
        Status = STATUS_SUCCESS;
        goto Quit;
    }

    OemLength = RtlUnicodeStringToOemSize(&CapturedString);
    if (OemLength > MAXUSHORT)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        goto Quit;
    }

    RtlInitEmptyAnsiString((PANSI_STRING)&OemString, NULL, (USHORT)OemLength);
    OemString.Buffer = ExAllocatePoolWithTag(NonPagedPool, OemLength, TAG_OSTR);
    if (OemString.Buffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    Status = RtlUnicodeStringToOemString(&OemString, &CapturedString, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(OemString.Buffer, TAG_OSTR);
        goto Quit;
    }

    InbvDisplayString(OemString.Buffer);

    ExFreePoolWithTag(OemString.Buffer, TAG_OSTR);
    Status = STATUS_SUCCESS;

Quit:
    ReleaseCapturedUnicodeString(&CapturedString, PreviousMode);
    return Status;
}
