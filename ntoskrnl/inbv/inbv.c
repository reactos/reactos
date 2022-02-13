/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Boot Video Driver support
 * COPYRIGHT:   Copyright 2007 Alex Ionescu (alex.ionescu@reactos.org)
 *              Copyright 2010 Aleksey Bragin (aleksey@reactos.org)
 *              Copyright 2015-2022 Hermès Bélusca-Maïto
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "inbv/logo.h"

/* GLOBALS *******************************************************************/

/*
 * Enable this define if you want Inbv to use coloured headless mode.
 */
// #define INBV_HEADLESS_COLORS

typedef struct _INBV_PROGRESS_STATE
{
    ULONG Floor;
    ULONG Ceiling;
    ULONG Bias;
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

static BOOLEAN InbvDisplayDebugStrings = FALSE;
static INBV_DISPLAY_STRING_FILTER InbvDisplayFilter = NULL;

ULONG ProgressBarLeft = 0, ProgressBarTop = 0;
BOOLEAN ShowProgressBar = FALSE;
static INBV_PROGRESS_STATE InbvProgressState;
static BT_PROGRESS_INDICATOR InbvProgressIndicator = {0, 25, 0};

static ULONG ResourceCount = 0;
static PUCHAR ResourceList[1 + IDB_MAX_RESOURCE]; // First entry == NULL, followed by 'ResourceCount' entries.


/*
 * Headless terminal text colors
 */

#ifdef INBV_HEADLESS_COLORS

// Conversion table CGA to ANSI color index
static const UCHAR CGA_TO_ANSI_COLOR_TABLE[16] =
{
    0,  // Black
    4,  // Blue
    2,  // Green
    6,  // Cyan
    1,  // Red
    5,  // Magenta
    3,  // Brown/Yellow
    7,  // Grey/White

    60, // Bright Black
    64, // Bright Blue
    62, // Bright Green
    66, // Bright Cyan
    61, // Bright Red
    65, // Bright Magenta
    63, // Bright Yellow
    67  // Bright Grey (White)
};

#define CGA_TO_ANSI_COLOR(CgaColor) \
    CGA_TO_ANSI_COLOR_TABLE[CgaColor & 0x0F]

#endif

// Default colors: text in white, background in black
static ULONG InbvTerminalTextColor = 37;
static ULONG InbvTerminalBkgdColor = 40;


/* FUNCTIONS *****************************************************************/

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
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Check for a match */
        if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &UpString, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &MpString, TRUE))
        {
            /* Break out */
            break;
        }
    }

    /* Check if we found it */
    if (NextEntry != ListHead)
    {
        /* Try to find the resource */
        ResourceInfo.Type = 2; // RT_BITMAP;
        ResourceInfo.Name = ResourceId;
        ResourceInfo.Language = 0;
        Status = LdrFindResource_U(LdrEntry->DllBase,
                                   &ResourceInfo,
                                   RESOURCE_DATA_LEVEL,
                                   &ResourceDataEntry);
        if (NT_SUCCESS(Status))
        {
            /* Access the resource */
            ULONG Size = 0;
            Status = LdrAccessResource(LdrEntry->DllBase,
                                       ResourceDataEntry,
                                       &Data,
                                       &Size);
            if ((Data) && (ResourceId < 3))
            {
                KiBugCheckData[4] ^= RtlComputeCrc32(0, Data, Size);
            }
            if (!NT_SUCCESS(Status)) Data = NULL;
        }
    }

    /* Return the pointer */
    return Data;
}

PUCHAR
NTAPI
InbvGetResourceAddress(
    _In_ ULONG ResourceNumber)
{
    /* Validate the resource number */
    if (ResourceNumber > ResourceCount) return NULL;

    /* Return the address */
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

    /* Quit if we're already installed */
    if (InbvBootDriverInstalled) return TRUE;

    /* Initialize the lock and check the current display state */
    KeInitializeSpinLock(&BootDriverLock);
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Reset the video mode in case we do not have a custom boot logo */
        CommandLine = (LoaderBlock->LoadOptions ? _strupr(LoaderBlock->LoadOptions) : NULL);
        ResetMode   = (CommandLine == NULL) || (strstr(CommandLine, "BOOTLOGO") == NULL);
    }

    /* Initialize the video */
    InbvBootDriverInstalled = VidInitialize(ResetMode);
    if (InbvBootDriverInstalled)
    {
        /* Find bitmap resources in the kernel */
        ResourceCount = min(Count, RTL_NUMBER_OF(ResourceList) - 1);
        for (i = 1; i <= ResourceCount; i++)
        {
            /* Do the lookup */
            ResourceList[i] = FindBitmapResource(LoaderBlock, i);
        }

        /* Set the progress bar ranges */
        InbvSetProgressBarSubset(0, 100);

        // BootAnimInitialize(LoaderBlock, Count);
    }

    /* Return install state */
    return InbvBootDriverInstalled;
}

VOID
NTAPI
InbvAcquireLock(VOID)
{
    KIRQL OldIrql;

    /* Check if we're at dispatch level or lower */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql <= DISPATCH_LEVEL)
    {
        /* Loop until the lock is free */
        while (!KeTestSpinLock(&BootDriverLock));

        /* Raise IRQL to dispatch level */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }

    /* Acquire the lock */
    KiAcquireSpinLock(&BootDriverLock);
    InbvOldIrql = OldIrql;
}

VOID
NTAPI
InbvReleaseLock(VOID)
{
    KIRQL OldIrql;

    /* Capture the old IRQL */
    OldIrql = InbvOldIrql;

    /* Release the driver lock */
    KiReleaseSpinLock(&BootDriverLock);

    /* If we were at dispatch level or lower, restore the old IRQL */
    if (InbvOldIrql <= DISPATCH_LEVEL) KeLowerIrql(OldIrql);
}

VOID
NTAPI
InbvEnableBootDriver(
    _In_ BOOLEAN Enable)
{
    /* Check if we're installed */
    if (InbvBootDriverInstalled)
    {
        /* Check for lost state */
        if (InbvDisplayState >= INBV_DISPLAY_STATE_LOST) return;

        /* Acquire the lock */
        InbvAcquireLock();

        /* Cleanup the screen if we own it */
        if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED) VidCleanUp();

        /* Set the new display state */
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED :
                                    INBV_DISPLAY_STATE_DISABLED;

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        /* Set the new display state */
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED :
                                    INBV_DISPLAY_STATE_DISABLED;
    }
}

VOID
NTAPI
InbvAcquireDisplayOwnership(VOID)
{
    /* Check if we have a callback and we're just acquiring it now */
    if ((InbvResetDisplayParameters) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_LOST))
    {
        /* Call the callback */
        InbvResetDisplayParameters(80, 50);
    }

    /* Acquire the display */
    InbvDisplayState = INBV_DISPLAY_STATE_OWNED;
}

VOID
NTAPI
InbvSetDisplayOwnership(
    _In_ BOOLEAN DisplayOwned)
{
    /* Set the new display state */
    InbvDisplayState = DisplayOwned ? INBV_DISPLAY_STATE_OWNED :
                                      INBV_DISPLAY_STATE_LOST;
}

BOOLEAN
NTAPI
InbvCheckDisplayOwnership(VOID)
{
    /* Return if we own it or not */
    return InbvDisplayState != INBV_DISPLAY_STATE_LOST;
}

INBV_DISPLAY_STATE
NTAPI
InbvGetDisplayState(VOID)
{
    /* Return the actual state */
    return InbvDisplayState;
}

BOOLEAN
NTAPI
InbvDisplayString(
    _In_ PCHAR String)
{
    /* Make sure we own the display */
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* If we're not allowed, return success anyway */
        if (!InbvDisplayDebugStrings) return TRUE;

        /* Check if a filter is installed */
        if (InbvDisplayFilter) InbvDisplayFilter(&String);

        /* Acquire the lock */
        InbvAcquireLock();

        /* Make sure we're installed and display the string */
        if (InbvBootDriverInstalled) VidDisplayString((PUCHAR)String);

        /* Print the string on the EMS port */
        HeadlessDispatch(HeadlessCmdPutString,
                         String,
                         strlen(String) + sizeof(ANSI_NULL),
                         NULL,
                         NULL);

        /* Release the lock */
        InbvReleaseLock();

        /* All done */
        return TRUE;
    }

    /* We don't own it, fail */
    return FALSE;
}

BOOLEAN
NTAPI
InbvEnableDisplayString(
    _In_ BOOLEAN Enable)
{
    BOOLEAN OldSetting;

    /* Get the old setting */
    OldSetting = InbvDisplayDebugStrings;

    /* Update it */
    InbvDisplayDebugStrings = Enable;

    /* Return the old setting */
    return OldSetting;
}

VOID
NTAPI
InbvInstallDisplayStringFilter(
    _In_ INBV_DISPLAY_STRING_FILTER DisplayFilter)
{
    /* Save the filter */
    InbvDisplayFilter = DisplayFilter;
}

BOOLEAN
NTAPI
InbvIsBootDriverInstalled(VOID)
{
    /* Return driver state */
    return InbvBootDriverInstalled;
}

VOID
NTAPI
InbvNotifyDisplayOwnershipLost(
    _In_ INBV_RESET_DISPLAY_PARAMETERS Callback)
{
    /* Check if we're installed */
    if (InbvBootDriverInstalled)
    {
        /* Acquire the lock and cleanup if we own the screen */
        InbvAcquireLock();
        if (InbvDisplayState != INBV_DISPLAY_STATE_LOST) VidCleanUp();

        /* Set the reset callback and display state */
        InbvResetDisplayParameters = Callback;
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        /* Set the reset callback and display state */
        InbvResetDisplayParameters = Callback;
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;
    }
}

BOOLEAN
NTAPI
InbvResetDisplay(VOID)
{
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the reset */
        VidResetDisplay(TRUE);
        return TRUE;
    }

    /* Nothing to reset */
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
    /* Just call bootvid */
    VidSetScrollRegion(Left, Top, Right, Bottom);
}

VOID
NTAPI
InbvSetTextColor(
    _In_ ULONG Color)
{
    HEADLESS_CMD_SET_COLOR HeadlessSetColor;

    /* Set color for EMS port */
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

    /* Update the text color */
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

    /* Make sure we own it */
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Check if we're installed */
        if (InbvBootDriverInstalled)
        {
            /* Call bootvid */
            VidSolidColorFill(Left, Top, Right, Bottom, (UCHAR)Color);
        }

        /* Set color for EMS port and clear display */
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
        HeadlessDispatch(HeadlessCmdClearDisplay,
                         NULL, 0,
                         NULL, NULL);

        /* Release the lock */
        InbvReleaseLock();
    }
}

VOID
NTAPI
InbvBitBlt(
    _In_ PUCHAR Buffer,
    _In_ ULONG X,
    _In_ ULONG Y)
{
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Do the blit */
        VidBitBlt(Buffer, X, Y);

        /* Release the lock */
        InbvReleaseLock();
    }
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
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the blit */
        VidBufferToScreenBlt(Buffer, X, Y, Width, Height, Delta);
    }
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
    /* Check if we're installed and we own it */
    if (InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the blit */
        VidScreenToBufferBlt(Buffer, X, Y, Width, Height, Delta);
    }
}

/**
 * @brief
 * Sets the screen coordinates of the loading progress bar and enable it.
 *
 * @param[in]   Left
 * @param[in]   Top
 * The left/top coordinates.
 *
 * @return None.
 **/
VOID
NTAPI
InbvSetProgressBarCoordinates(
    _In_ ULONG Left,
    _In_ ULONG Top)
{
    /* Update the coordinates */
    ProgressBarLeft = Left;
    ProgressBarTop  = Top;

    /* Enable the progress bar */
    ShowProgressBar = TRUE;
}

/**
 * @brief
 * Gives some progress feedback, without specifying any explicit number
 * of progress steps or percentage.
 * The corresponding percentage is derived from the progress indicator's
 * current count, capped to the number of expected calls to be made to
 * this function (default: 25, see @b InbvProgressIndicator.Expected).
 *
 * @return None.
 **/
CODE_SEG("INIT")
VOID
NTAPI
InbvIndicateProgress(VOID)
{
    ULONG Percentage;

    /* Increase progress */
    InbvProgressIndicator.Count++;

    /* Compute the new percentage - Don't go over 100% */
    Percentage = 100 * InbvProgressIndicator.Count /
                       InbvProgressIndicator.Expected;
    Percentage = min(Percentage, 99);

    if (Percentage != InbvProgressIndicator.Percentage)
    {
        /* Percentage has changed, update the progress bar */
        InbvProgressIndicator.Percentage = Percentage;
        InbvUpdateProgressBar(Percentage);
    }
}

/**
 * @brief
 * Specifies a progress percentage sub-range.
 * Further calls to InbvIndicateProgress() or InbvUpdateProgressBar()
 * will update the progress percentage relative to this sub-range.
 * In particular, the percentage provided to InbvUpdateProgressBar()
 * is relative to this sub-range.
 *
 * @param[in]   Floor
 * The lower bound percentage of the sub-range (default: 0).
 *
 * @param[in]   Ceiling
 * The upper bound percentage of the sub-range (default: 100).
 *
 * @return None.
 **/
VOID
NTAPI
InbvSetProgressBarSubset(
    _In_ ULONG Floor,
    _In_ ULONG Ceiling)
{
    /* Sanity checks */
    ASSERT(Floor < Ceiling);
    ASSERT(Ceiling <= 100);

    /* Update the progress bar state */
    InbvProgressState.Floor = Floor * 100;
    InbvProgressState.Ceiling = Ceiling * 100;
    InbvProgressState.Bias = Ceiling - Floor;
}

/**
 * @brief
 * Updates the progress bar percentage, relative to the current
 * percentage sub-range previously set by InbvSetProgressBarSubset().
 *
 * @param[in]   Percentage
 * The progress percentage, relative to the current sub-range.
 *
 * @return None.
 **/
VOID
NTAPI
InbvUpdateProgressBar(
    _In_ ULONG Percentage)
{
    ULONG TotalProgress;

    /* Make sure the progress bar is enabled, that we own and are installed */
    if (ShowProgressBar &&
        InbvBootDriverInstalled &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Compute the total progress and tick the progress bar */
        TotalProgress = InbvProgressState.Floor + (Percentage * InbvProgressState.Bias);
        // TotalProgress /= (100 * 100);

        BootAnimTickProgressBar(TotalProgress);
    }
}

NTSTATUS
NTAPI
NtDisplayString(IN PUNICODE_STRING DisplayString)
{
    NTSTATUS Status;
    UNICODE_STRING CapturedString;
    OEM_STRING OemString;
    ULONG OemLength;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* We require the TCB privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
        return STATUS_PRIVILEGE_NOT_HELD;

    /* Capture the string */
    Status = ProbeAndCaptureUnicodeString(&CapturedString, PreviousMode, DisplayString);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Do not display the string if it is empty */
    if (CapturedString.Length == 0 || CapturedString.Buffer == NULL)
    {
        Status = STATUS_SUCCESS;
        goto Quit;
    }

    /*
     * Convert the string since INBV understands only ANSI/OEM. Allocate the
     * string buffer in non-paged pool because INBV passes it down to BOOTVID.
     * We cannot perform the allocation using RtlUnicodeStringToOemString()
     * since its allocator uses PagedPool.
     */
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

    /* Display the string */
    InbvDisplayString(OemString.Buffer);

    /* Free the string buffer */
    ExFreePoolWithTag(OemString.Buffer, TAG_OSTR);

    Status = STATUS_SUCCESS;

Quit:
    /* Free the captured string */
    ReleaseCapturedUnicodeString(&CapturedString, PreviousMode);

    return Status;
}
