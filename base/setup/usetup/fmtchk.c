/*
 * PROJECT:     ReactOS text-mode setup
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Filesystem Format and ChkDsk support functions
 * COPYRIGHT:   Copyright 2003 Casper S. Hornstrup <chorns@users.sourceforge.net>
 *              Copyright 2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

static PPROGRESSBAR ProgressBar = NULL;

/* FORMAT FUNCTIONS **********************************************************/

static
BOOLEAN
NTAPI
FormatCallback(
    _In_ CALLBACKCOMMAND Command,
    _In_ ULONG Modifier,
    _In_ PVOID Argument)
{
    switch (Command)
    {
        case PROGRESS:
        {
            PULONG Percent = (PULONG)Argument;
            DPRINT("%lu percent completed\n", *Percent);
            ProgressSetStep(ProgressBar, *Percent);
            break;
        }

#if 0
        case OUTPUT:
        {
            PTEXTOUTPUT output = (PTEXTOUTPUT)Argument;
            DPRINT("%s\n", output->Output);
            break;
        }
#endif

        case DONE:
        {
#if 0
            PBOOLEAN Success = (PBOOLEAN)Argument;
            if (*Success == FALSE)
            {
                DPRINT("FormatEx was unable to complete successfully.\n\n");
            }
#endif
            DPRINT("Done\n");
            break;
        }

        default:
            DPRINT("Unknown callback %lu\n", (ULONG)Command);
            break;
    }

    return TRUE;
}

VOID
StartFormat(
    _Inout_ PFORMAT_VOLUME_INFO FmtInfo,
    _In_ PFILE_SYSTEM_ITEM SelectedFileSystem)
{
    ASSERT(SelectedFileSystem && SelectedFileSystem->FileSystem);

    // TODO: Think about which values could be defaulted...
    FmtInfo->FileSystemName = SelectedFileSystem->FileSystem;
    FmtInfo->MediaFlag = FMIFS_HARDDISK;
    FmtInfo->Label = NULL;
    FmtInfo->QuickFormat = SelectedFileSystem->QuickFormat;
    FmtInfo->ClusterSize = 0;
    FmtInfo->Callback = FormatCallback;

    ProgressBar = CreateProgressBar(6,
                                    yScreen - 14,
                                    xScreen - 7,
                                    yScreen - 10,
                                    10,
                                    24,
                                    TRUE,
                                    MUIGetString(STRING_FORMATTINGPART));

    ProgressSetStepCount(ProgressBar, 100);
}

VOID
EndFormat(
    _In_ NTSTATUS Status)
{
    DestroyProgressBar(ProgressBar);
    ProgressBar = NULL;

    DPRINT("FormatPartition() finished with status 0x%08lx\n", Status);
}

/* CHKDSK FUNCTIONS **********************************************************/

static
BOOLEAN
NTAPI
ChkdskCallback(
    _In_ CALLBACKCOMMAND Command,
    _In_ ULONG Modifier,
    _In_ PVOID Argument)
{
    switch (Command)
    {
        default:
            DPRINT("Unknown callback %lu\n", (ULONG)Command);
            break;
    }

    return TRUE;
}

VOID
StartCheck(
    _Inout_ PCHECK_VOLUME_INFO ChkInfo)
{
    // TODO: Think about which values could be defaulted...
    ChkInfo->FixErrors = TRUE;
    ChkInfo->Verbose = FALSE;
    ChkInfo->CheckOnlyIfDirty = TRUE;
    ChkInfo->ScanDrive = FALSE;
    ChkInfo->Callback = ChkdskCallback;

    ProgressBar = CreateProgressBar(6,
                                    yScreen - 14,
                                    xScreen - 7,
                                    yScreen - 10,
                                    10,
                                    24,
                                    TRUE,
                                    MUIGetString(STRING_CHECKINGDISK));

    ProgressSetStepCount(ProgressBar, 100);
}

VOID
EndCheck(
    _In_ NTSTATUS Status)
{
    DestroyProgressBar(ProgressBar);
    ProgressBar = NULL;

    DPRINT("ChkdskPartition() finished with status 0x%08lx\n", Status);
}

/* EOF */
