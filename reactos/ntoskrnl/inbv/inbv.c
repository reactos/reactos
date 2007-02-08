/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "bootvid/bootvid.h"

/* GLOBALS *******************************************************************/

KSPIN_LOCK BootDriverLock;
KIRQL InbvOldIrql;
INBV_DISPLAY_STATE InbvDisplayState;
BOOLEAN InbvBootDriverInstalled;
BOOLEAN InbvDisplayDebugStrings;
INBV_DISPLAY_STRING_FILTER InbvDisplayFilter;
ULONG ProgressBarLeft, ProgressBarTop;
BOOLEAN ShowProgressBar;
INBV_PROGRESS_STATE InbvProgressState;
INBV_RESET_DISPLAY_PARAMETERS InbvResetDisplayParameters;
ULONG ResourceCount;
PUCHAR ResourceList[64];
BOOLEAN SysThreadCreated;
ROT_BAR_TYPE RotBarSelection;
ULONG PltRotBarStatus;

/* FUNCTIONS *****************************************************************/

PVOID
NTAPI
FindBitmapResource(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                   IN ULONG ResourceId)
{
    UNICODE_STRING UpString = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    UNICODE_STRING MpString = RTL_CONSTANT_STRING(L"ntkrnlmp.exe");
    PLIST_ENTRY NextEntry, ListHead;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    LDR_RESOURCE_INFO ResourceInfo;
    ULONG Size;
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
        if ((RtlEqualUnicodeString(&LdrEntry->BaseDllName, &UpString, TRUE)) ||
            (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &MpString, TRUE)))
        {
            /* Break out */
            break;
        }
    }

    /* Check if we found it */
    if (NextEntry != ListHead)
    {
        /* Try to find the resource */
        ResourceInfo.Type = 2;
        ResourceInfo.Name = ResourceId;
        ResourceInfo.Language = 0;
        Status = LdrFindResource_U(LdrEntry->DllBase,
                                   &ResourceInfo,
                                   RESOURCE_DATA_LEVEL,
                                   &ResourceDataEntry);
        if (NT_SUCCESS(Status))
        {
            /* Access the resource */
            Status = LdrAccessResource(LdrEntry->DllBase,
                                       ResourceDataEntry,
                                       &Data,
                                       &Size);
            if (!NT_SUCCESS(Status)) Data = NULL;
        }
    }

    /* Return the pointer */
    return Data;
}

BOOLEAN
NTAPI
InbvDriverInitialize(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                     IN ULONG Count)
{
    PCHAR CommandLine;
    BOOLEAN CustomLogo = FALSE;
    ULONG i;

    /* Quit if we're already installed */
    if (InbvBootDriverInstalled) return TRUE;

    /* Initialize the lock and check the current display state */
    KeInitializeSpinLock(&BootDriverLock);
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Check if we have a custom boot logo */
        CommandLine = _strupr(LoaderBlock->LoadOptions);
        CustomLogo = strstr(CommandLine, "BOOTLOGO") ? TRUE: FALSE;
    }

    /* Initialize the video */
    InbvBootDriverInstalled = VidInitialize(!CustomLogo);
    if (InbvBootDriverInstalled)
    {
        /* Find bitmap resources in the kernel */
        ResourceCount = Count;
        for (i = 0; i < Count; i++)
        {
            /* Do the lookup */
            ResourceList[i] = FindBitmapResource(LoaderBlock, i);
        }

        /* Set the progress bar ranges */
        InbvSetProgressBarSubset(0, 100);
    }

    /* Return install state */
    return InbvBootDriverInstalled;
}

VOID
NTAPI
InbvAcquireLock(VOID)
{
    /* Check if we're below dispatch level */
    InbvOldIrql = KeGetCurrentIrql();
    if (InbvOldIrql < DISPATCH_LEVEL)
    {
        /* Raise IRQL to dispatch level */
        InbvOldIrql = KfRaiseIrql(DISPATCH_LEVEL);
    }

    /* Acquire the lock */
    KiAcquireSpinLock(&BootDriverLock);
}

VOID
NTAPI
InbvReleaseLock(VOID)
{
    /* Release the driver lock */
    KiReleaseSpinLock(&BootDriverLock);

    /* If we were below dispatch level, lower IRQL back */
    if (InbvOldIrql < DISPATCH_LEVEL) KfLowerIrql(InbvOldIrql);
}

VOID
NTAPI
InbvEnableBootDriver(IN BOOLEAN Enable)
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
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED:
                                    INBV_DISPLAY_STATE_DISABLED;

        /* Release the lock */
        InbvReleaseLock();
    }
    else
    {
        /* Set the new display state */
        InbvDisplayState = Enable ? INBV_DISPLAY_STATE_OWNED:
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
InbvSetDisplayOwnership(IN BOOLEAN DisplayOwned)
{
    /* Set the new display state */
    InbvDisplayState = DisplayOwned ? INBV_DISPLAY_STATE_OWNED:
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
InbvDisplayString(IN PCHAR String)
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
        if (InbvBootDriverInstalled) VidDisplayString((PUCHAR) String);

        /* Call Headless (We don't support headless for now)
        HeadlessDispatch(DISPLAY_STRING); */

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
InbvEnableDisplayString(IN BOOLEAN Enable)
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
InbvInstallDisplayStringFilter(IN INBV_DISPLAY_STRING_FILTER Filter)
{
    /* Save the filter */
    InbvDisplayFilter = Filter;
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
InbvNotifyDisplayOwnershipLost(IN INBV_RESET_DISPLAY_PARAMETERS Callback)
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
    if ((InbvBootDriverInstalled) &&
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
InbvSetScrollRegion(IN ULONG Left,
                    IN ULONG Top,
                    IN ULONG Width,
                    IN ULONG Height)
{
    /* Just call bootvid */
    VidSetScrollRegion(Left, Top, Width, Height);
}

VOID
NTAPI
InbvSetTextColor(IN ULONG Color)
{
    /* FIXME: Headless */

    /* Update the text color */
    VidSetTextColor(Color);
}

VOID
NTAPI
InbvSolidColorFill(IN ULONG Left,
                   IN ULONG Top,
                   IN ULONG Width,
                   IN ULONG Height,
                   IN ULONG Color)
{
    /* Make sure we own it */
    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /* Check if we're installed */
        if (InbvBootDriverInstalled)
        {
            /* Call bootvid */
            VidSolidColorFill(Left, Top, Width, Height, Color);
        }

        /* FIXME: Headless */

        /* Release the lock */
        InbvReleaseLock();
    }
}

VOID
NTAPI
InbvUpdateProgressBar(IN ULONG Progress)
{
    ULONG FillCount, Left = 0;

    /* Make sure the progress bar is enabled, that we own and are installed */
    if ((ShowProgressBar) &&
        (InbvBootDriverInstalled) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Calculate the fill count */
        FillCount = InbvProgressState.Bias * Progress + InbvProgressState.Floor;
        FillCount *= 18;
        FillCount /= 10000;

        /* Start fill loop */
        while (FillCount)
        {
            /* Acquire the lock */
            InbvAcquireLock();

            /* Fill the progress bar */
            VidSolidColorFill(Left + ProgressBarLeft,
                              ProgressBarTop,
                              Left + ProgressBarLeft + 7,
                              ProgressBarTop + 7,
                              11);

            /* Release the lock */
            InbvReleaseLock();

            /* Update the X position */
            Left += 9;
            FillCount--;
        }
    }
}

VOID
NTAPI
InbvBufferToScreenBlt(IN PUCHAR Buffer,
                      IN ULONG X,
                      IN ULONG Y,
                      IN ULONG Width,
                      IN ULONG Height,
                      IN ULONG Delta)
{
    /* Check if we're installed and we own it */
    if ((InbvBootDriverInstalled) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the blit */
        VidBufferToScreenBlt(Buffer, X, Y, Width, Height, Delta);
    }
}

VOID
NTAPI
InbvBitBlt(IN PUCHAR Buffer,
           IN ULONG X,
           IN ULONG Y)
{
    /* Check if we're installed and we own it */
    if ((InbvBootDriverInstalled) &&
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
InbvScreenToBufferBlt(IN PUCHAR Buffer,
                      IN ULONG X,
                      IN ULONG Y,
                      IN ULONG Width,
                      IN ULONG Height,
                      IN ULONG Delta)
{
    /* Check if we're installed and we own it */
    if ((InbvBootDriverInstalled) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Do the blit */
        VidScreenToBufferBlt(Buffer, X, Y, Width, Height, Delta);
    }
}

VOID
NTAPI
InbvSetProgressBarCoordinates(IN ULONG Left,
                              IN ULONG Top)
{
    /* Update the coordinates */
    ProgressBarLeft = Left;
    ProgressBarTop = Top;

    /* Enable the progress bar */
    ShowProgressBar = TRUE;
}

VOID
NTAPI
InbvSetProgressBarSubset(IN ULONG Floor,
                         IN ULONG Ceiling)
{
    /* Sanity checks */
    ASSERT(Floor < Ceiling);
    ASSERT(Ceiling <= 100);

    /* Update the progress bar state */
    InbvProgressState.Floor = Floor * 100;
    InbvProgressState.Ceiling = Ceiling * 100;
    InbvProgressState.Bias = (Ceiling * 100) - Floor;
}

PUCHAR
NTAPI
InbvGetResourceAddress(IN ULONG ResourceNumber)
{
    /* Validate the resource number */
    if (ResourceNumber > ResourceCount) return NULL;

    /* Return the address */
    return ResourceList[ResourceNumber--];
}

NTSTATUS
NTAPI
NtDisplayString(IN PUNICODE_STRING DisplayString)
{
    OEM_STRING OemString;

    /* Convert the string to OEM and display it */
    RtlUnicodeStringToOemString(&OemString, DisplayString, TRUE);
    InbvDisplayString(OemString.Buffer);
    RtlFreeOemString(&OemString);

    /* Return success */
    return STATUS_SUCCESS;
}

VOID
NTAPI
DisplayBootBitmap(IN BOOLEAN SosMode)
{
    PVOID Bitmap, Header;
    ROT_BAR_TYPE TempRotBarSelection = RB_UNSPECIFIED;

    /* Check if the system thread has already been created */
    if (SysThreadCreated)
    {
        /* Reset the progress bar */
        InbvAcquireLock();
        RotBarSelection = 0;
        InbvReleaseLock();
    }

    /* Check if this is SOS mode */
    ShowProgressBar = FALSE;
    if (SosMode)
    {
        /* Check if this is a server OS */
        if (SharedUserData->NtProductType == NtProductWinNt)
        {
            /* It's not, set workstation settings */
            InbvSetTextColor(15);
            InbvSolidColorFill(0, 0, 639, 479, 7);
            InbvSolidColorFill(0, 421, 639, 479, 1);

            /* Get resources */
            Bitmap = InbvGetResourceAddress(6);
            Header = InbvGetResourceAddress(7);
        }
        else
        {
            /* Set server settings */
            InbvSetTextColor(14);
            InbvSolidColorFill(0, 0, 639, 479, 6);
            InbvSolidColorFill(0, 421, 639, 479, 1);

            /* Get resources */
            Bitmap = InbvGetResourceAddress(14);
            Header = InbvGetResourceAddress(15);
        }

        /* Set the scrolling region */
        InbvSetScrollRegion(32, 80, 631, 400);

        /* Make sure we have resources */
        if ((Bitmap) && (Header))
        {
            /* BitBlt them on the screen */
            InbvBitBlt(Header, 0, 419);
            InbvBitBlt(Bitmap, 0, 0);
        }
    }
    else
    {
        /* Is the boot driver installed? */
        if (!InbvBootDriverInstalled) return;

        /* FIXME: TODO, display full-screen bitmap */
    }

    /* Do we have a system thread? */
    if (SysThreadCreated)
    {
        /* We do, set the progress bar location */
        InbvAcquireLock();
        RotBarSelection = TempRotBarSelection;
        //InbvRotBarInit();
        InbvReleaseLock();
    }
}

VOID
NTAPI
FinalizeBootLogo(VOID)
{
    /* Acquire lock and check the display state */
    InbvAcquireLock();
    if (InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED)
    {
        /* Clear the screen */
        VidSolidColorFill(0, 0, 639, 479, 0);
    }

    /* Reset progress bar and lock */
    PltRotBarStatus = 3;
    InbvReleaseLock();
}
