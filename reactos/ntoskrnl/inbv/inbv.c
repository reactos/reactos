/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "bootvid/bootvid.h"

/* GLOBALS *******************************************************************/

KSPIN_LOCK BootDriverLock;
KIRQL InbvOldIrql;
INBV_DISPLAY_STATE InbvDisplayState;
BOOLEAN InbvBootDriverInstalled = FALSE;
BOOLEAN InbvDisplayDebugStrings = FALSE;
INBV_DISPLAY_STRING_FILTER InbvDisplayFilter;
ULONG ProgressBarLeft, ProgressBarTop;
BOOLEAN ShowProgressBar = FALSE;
INBV_PROGRESS_STATE InbvProgressState;
INBV_RESET_DISPLAY_PARAMETERS InbvResetDisplayParameters;
ULONG ResourceCount;
PUCHAR ResourceList[64];
BOOLEAN SysThreadCreated = FALSE;
ROT_BAR_TYPE RotBarSelection;
ULONG PltRotBarStatus;
BT_PROGRESS_INDICATOR InbvProgressIndicator = {0, 25, 0};

/* FADING FUNCTION ***********************************************************/

/** From include/psdk/wingdi.h **/
typedef struct tagRGBQUAD
{
    UCHAR    rgbBlue;
    UCHAR    rgbGreen;
    UCHAR    rgbRed;
    UCHAR    rgbReserved;
} RGBQUAD,*LPRGBQUAD;
/*******************************/

static RGBQUAD _MainPalette[16];

#define PALETTE_FADE_STEPS  15
#define PALETTE_FADE_TIME   20 * 1000 /* 20ms */

/** From bootvid/precomp.h **/
//
// Bitmap Header
//
typedef struct tagBITMAPINFOHEADER
{
    ULONG biSize;
    LONG biWidth;
    LONG biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG biCompression;
    ULONG biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    ULONG biClrUsed;
    ULONG biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
/****************************/

//
// Needed prototypes
//
VOID NTAPI InbvAcquireLock(VOID);
VOID NTAPI InbvReleaseLock(VOID);

static VOID
NTAPI
BootImageFadeIn(VOID)
{
    UCHAR PaletteBitmapBuffer[sizeof(BITMAPINFOHEADER) + sizeof(_MainPalette)];
    PBITMAPINFOHEADER PaletteBitmap = (PBITMAPINFOHEADER)PaletteBitmapBuffer;
    LPRGBQUAD Palette = (LPRGBQUAD)(PaletteBitmapBuffer + sizeof(BITMAPINFOHEADER));

    ULONG Iteration, Index, ClrUsed;

    /* Check if we're installed and we own it */
    if ((InbvBootDriverInstalled) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Acquire the lock */
        InbvAcquireLock();

        /*
         * Build a bitmap containing the fade in palette. The palette entries
         * are then processed in a loop and set using VidBitBlt function.
         */
        ClrUsed = sizeof(_MainPalette) / sizeof(_MainPalette[0]);
        RtlZeroMemory(PaletteBitmap, sizeof(BITMAPINFOHEADER));
        PaletteBitmap->biSize = sizeof(BITMAPINFOHEADER);
        PaletteBitmap->biBitCount = 4;
        PaletteBitmap->biClrUsed = ClrUsed;

        /*
         * Main animation loop.
         */
        for (Iteration = 0; Iteration <= PALETTE_FADE_STEPS; ++Iteration)
        {
            for (Index = 0; Index < ClrUsed; Index++)
            {
                Palette[Index].rgbRed = (UCHAR)
                    (_MainPalette[Index].rgbRed * Iteration / PALETTE_FADE_STEPS);
                Palette[Index].rgbGreen = (UCHAR)
                    (_MainPalette[Index].rgbGreen * Iteration / PALETTE_FADE_STEPS);
                Palette[Index].rgbBlue = (UCHAR)
                    (_MainPalette[Index].rgbBlue * Iteration / PALETTE_FADE_STEPS);
            }

            VidBitBlt(PaletteBitmapBuffer, 0, 0);

            /* Wait for a bit. */
            KeStallExecutionProcessor(PALETTE_FADE_TIME);
        }

        /* Release the lock */
        InbvReleaseLock();

        /* Wait for a bit. */
        KeStallExecutionProcessor(PALETTE_FADE_TIME);
    }
}

/* FUNCTIONS *****************************************************************/

PVOID
NTAPI
INIT_FUNCTION
FindBitmapResource(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                   IN ULONG ResourceId)
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
        ResourceInfo.Type = 2; //RT_BITMAP;
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

BOOLEAN
NTAPI
INIT_FUNCTION
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
    InbvBootDriverInstalled = VidInitialize(FALSE);
    if (InbvBootDriverInstalled)
    {
        /* Now reset the display, but only if there's a custom boot logo */
        VidResetDisplay(CustomLogo);

        /* Find bitmap resources in the kernel */
        ResourceCount = min(IDB_CLUSTER_SERVER, Count);
        for (i = 1; i <= Count; i++)
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
INIT_FUNCTION
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
                    IN ULONG Right,
                    IN ULONG Bottom)
{
    /* Just call bootvid */
    VidSetScrollRegion(Left, Top, Right, Bottom);
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
                   IN ULONG Right,
                   IN ULONG Bottom,
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
            VidSolidColorFill(Left, Top, Right, Bottom, (UCHAR)Color);
        }

        /* FIXME: Headless */

        /* Release the lock */
        InbvReleaseLock();
    }
}

VOID
NTAPI
INIT_FUNCTION
InbvUpdateProgressBar(IN ULONG Progress)
{
    ULONG FillCount, BoundedProgress;

    /* Make sure the progress bar is enabled, that we own and are installed */
    if ((ShowProgressBar) &&
        (InbvBootDriverInstalled) &&
        (InbvDisplayState == INBV_DISPLAY_STATE_OWNED))
    {
        /* Compute fill count */
        BoundedProgress = (InbvProgressState.Floor / 100) + Progress;
        FillCount = 121 * (InbvProgressState.Bias * BoundedProgress) / 1000000;

        /* Acquire the lock */
        InbvAcquireLock();

        /* Fill the progress bar */
        VidSolidColorFill(ProgressBarLeft,
                          ProgressBarTop,
                          ProgressBarLeft + FillCount,
                          ProgressBarTop + 12,
                          15);

        /* Release the lock */
        InbvReleaseLock();
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

VOID
NTAPI
INIT_FUNCTION
InbvIndicateProgress(VOID)
{
    ULONG Percentage;

    /* Increase progress */
    InbvProgressIndicator.Count++;

    /* Compute new percentage */
    Percentage = min(100 * InbvProgressIndicator.Count /
                           InbvProgressIndicator.Expected,
                     99);
    if (Percentage != InbvProgressIndicator.Percentage)
    {
        /* Percentage has moved, update the progress bar */
        InbvProgressIndicator.Percentage = Percentage;
        InbvUpdateProgressBar(Percentage);
    }
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
INIT_FUNCTION
DisplayBootBitmap(IN BOOLEAN TextMode)
{
    PBITMAPINFOHEADER BitmapInfoHeader;
    LPRGBQUAD Palette;

    PVOID Header, Band, Text, Screen;
    ROT_BAR_TYPE TempRotBarSelection = RB_UNSPECIFIED;

#ifdef CORE_6781_resolved
    UCHAR Buffer[64];
#endif

    /* Check if the system thread has already been created */
    if (SysThreadCreated)
    {
        /* Reset the progress bar */
        InbvAcquireLock();
        RotBarSelection = RB_UNSPECIFIED;
        InbvReleaseLock();
    }

    /* Check if this is text mode */
    ShowProgressBar = FALSE;
    if (TextMode)
    {
        /* Check if this is a server OS */
        if (SharedUserData->NtProductType == NtProductWinNt)
        {
            /* It's not, set workstation settings */
            InbvSetTextColor(15);
            InbvSolidColorFill(0, 0, 639, 479, 7);
            InbvSolidColorFill(0, 421, 639, 479, 1);

            /* Get resources */
            Header = InbvGetResourceAddress(IDB_LOGO_HEADER);
            Band = InbvGetResourceAddress(IDB_LOGO_BAND);
        }
        else
        {
            /* Set server settings */
            InbvSetTextColor(14);
            InbvSolidColorFill(0, 0, 639, 479, 6);
            InbvSolidColorFill(0, 421, 639, 479, 1);

            /* Get resources */
            Header = InbvGetResourceAddress(IDB_SERVER_HEADER);
            Band = InbvGetResourceAddress(IDB_SERVER_BAND);
        }

        /* Set the scrolling region */
        InbvSetScrollRegion(32, 80, 631, 400);

        /* Make sure we have resources */
        if ((Header) && (Band))
        {
            /* BitBlt them on the screen */
            InbvBitBlt(Band, 0, 419);
            InbvBitBlt(Header, 0, 0);
        }
    }
    else
    {
        /* Is the boot driver installed? */
        Text = NULL;
        if (!InbvBootDriverInstalled) return;

        /* Load the standard boot screen */
        Screen = InbvGetResourceAddress(IDB_BOOT_LOGO);
        if (SharedUserData->NtProductType == NtProductWinNt)
        {
            /* Workstation product, display appropriate status bar color */
            InbvGetResourceAddress(IDB_BAR_PRO);
        }
        else
        {
            /* Display correct branding based on server suite */
            if (ExVerifySuite(StorageServer))
            {
                /* Storage Server Edition */
                Text = InbvGetResourceAddress(IDB_STORAGE_SERVER);
            }
            else if (ExVerifySuite(ComputeServer))
            {
                /* Compute Cluster Edition */
                Text = InbvGetResourceAddress(IDB_CLUSTER_SERVER);
            }
            else
            {
                /* Normal edition */
                Text = InbvGetResourceAddress(IDB_SERVER_LOGO);
            }

            /* Server product, display appropriate status bar color */
            InbvGetResourceAddress(IDB_BAR_SERVER);
        }

        /* Make sure we had a logo */
        if (Screen)
        {
            /* Choose progress bar */
            TempRotBarSelection = RB_SQUARE_CELLS;

            /*
             * Save the main image palette and replace it with black palette,
             * so that we can do fade in effect later.
             */
            BitmapInfoHeader = (PBITMAPINFOHEADER)Screen;
            Palette = (LPRGBQUAD)((PUCHAR)Screen + BitmapInfoHeader->biSize);
            RtlCopyMemory(_MainPalette, Palette, sizeof(_MainPalette));
            RtlZeroMemory(Palette, sizeof(_MainPalette));

            /* Blit the background */
            InbvBitBlt(Screen, 0, 0);

            /* Set progress bar coordinates and display it */
            InbvSetProgressBarCoordinates(259, 352);

            /* Display the boot logo and fade it in */
            BootImageFadeIn();

#ifdef CORE_6781_resolved
            /* Check for non-workstation products */
            if (SharedUserData->NtProductType != NtProductWinNt)
            {
                /* Overwrite part of the logo for a server product */
                InbvScreenToBufferBlt(Buffer, 413, 237, 7, 7, 8);
                InbvSolidColorFill(418, 230, 454, 256, 0);
                InbvBufferToScreenBlt(Buffer, 413, 237, 7, 7, 8);

                /* In setup mode, you haven't selected a SKU yet */
                if (ExpInTextModeSetup) Text = NULL;
            }
#endif
        }

#ifdef CORE_6781_resolved
        /* Draw the SKU text if it exits */
        if (Text) InbvBitBlt(Text, 180, 121);
#else
        DBG_UNREFERENCED_LOCAL_VARIABLE(Text);
#endif

        /* Draw the progress bar bit */
//      if (Bar) InbvBitBlt(Bar, 0, 0);

        /* Set filter which will draw text display if needed */
        InbvInstallDisplayStringFilter(DisplayFilter);
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
INIT_FUNCTION
DisplayFilter(PCHAR *String)
{
    /* Windows hack to skip first dots */
    static BOOLEAN DotHack = TRUE;

    /* If "." is given set *String to empty string */
    if(DotHack && strcmp(*String, ".") == 0)
        *String = "";

    if(**String)
    {
        /* Remove the filter */
        InbvInstallDisplayStringFilter(NULL);

        DotHack = FALSE;

        /* Draw text screen */
        DisplayBootBitmap(TRUE);
    }
}

VOID
NTAPI
INIT_FUNCTION
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
