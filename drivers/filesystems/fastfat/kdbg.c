/*
* FILE:             drivers/filesystems/fastfat/kdbg.c
* PURPOSE:          KDBG extension.
* COPYRIGHT:        See COPYING in the top level directory
* PROJECT:          ReactOS kernel
* PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
*/

/*  -------------------------------------------------------  INCLUDES  */

#include "vfat.h"

#define NDEBUG
#include <debug.h>

#include <stdio.h>

/*  --------------------------------------------------------  DEFINES  */

#ifdef KDBG
UNICODE_STRING DebugFile = {0, 0, NULL};

BOOLEAN
NTAPI
vfatKdbgHandler(
    IN PCHAR Command,
    IN ULONG Argc,
    IN PCH Argv[])
{
    ULONG Len;

    Len = strlen(Command);
    if (Len < sizeof("?fat."))
    {
        return FALSE;
    }

    if (Command[0] != '?' || Command[1] != 'f' ||
        Command[2] != 'a' || Command[3] != 't' ||
        Command[4] != '.')
    {
        return FALSE;
    }

    Command += (sizeof("?fat.") - sizeof(ANSI_NULL));
    if (strcmp(Command, "vols") == 0)
    {
        ULONG Count = 0;
        PLIST_ENTRY ListEntry;
        PDEVICE_EXTENSION DeviceExt;

        for (ListEntry = VfatGlobalData->VolumeListHead.Flink;
             ListEntry != &VfatGlobalData->VolumeListHead;
             ListEntry = ListEntry->Flink)
        {
            DeviceExt = CONTAINING_RECORD(ListEntry, DEVICE_EXTENSION, VolumeListEntry);
            DPRINT1("Volume: %p with VCB: %p\n", DeviceExt->VolumeDevice, DeviceExt);
            ++Count;
        }

        if (Count == 0)
        {
            DPRINT1("No volume found\n");
        }
    }
    else if (strcmp(Command, "files") == 0)
    {
        if (Argc != 2)
        {
            DPRINT1("Please provide a volume or a VCB!\n");
        }
        else
        {
            PLIST_ENTRY ListEntry;
            PDEVICE_EXTENSION DeviceExt;

            for (ListEntry = VfatGlobalData->VolumeListHead.Flink;
                 ListEntry != &VfatGlobalData->VolumeListHead;
                 ListEntry = ListEntry->Flink)
            {
                CHAR Volume[17];

                DeviceExt = CONTAINING_RECORD(ListEntry, DEVICE_EXTENSION, VolumeListEntry);
                sprintf(Volume, "%p", DeviceExt);
                if (strcmp(Volume, Argv[1]) == 0)
                {
                    break;
                }

                sprintf(Volume, "%p", DeviceExt->VolumeDevice);
                if (strcmp(Volume, Argv[1]) == 0)
                {
                    break;
                }

                DeviceExt = NULL;
            }

            if (DeviceExt == NULL)
            {
                DPRINT1("No volume %s found!\n", Argv[1]);
            }
            else
            {
                PVFATFCB Fcb;

                for (ListEntry = DeviceExt->FcbListHead.Flink;
                     ListEntry != &DeviceExt->FcbListHead;
                     ListEntry = ListEntry->Flink)
                {
                    Fcb = CONTAINING_RECORD(ListEntry, VFATFCB, FcbListEntry);
                    DPRINT1("FCB %p (ref: %d, oc: %d %s %s %s) for FO %p with path: %.*S\n",
                            Fcb, Fcb->RefCount, Fcb->OpenHandleCount,
                            ((Fcb->Flags & FCB_CLEANED_UP) ? "U" : "NU"),
                            ((Fcb->Flags & FCB_CLOSED) ? "C" : "NC"),
                            ((Fcb->Flags & FCB_DELAYED_CLOSE) ? "D" : "ND"),
                            Fcb->FileObject, Fcb->PathNameU.Length, Fcb->PathNameU.Buffer);
                }
            }
        }
    }
    else if (strcmp(Command, "setdbgfile") == 0)
    {
        if (Argc < 2)
        {
            if (DebugFile.Buffer != NULL)
            {
                ExFreePool(DebugFile.Buffer);
                DebugFile.Length = 0;
                DebugFile.MaximumLength = 0;
            }

            DPRINT1("Debug file reset\n");
        }
        else
        {
            NTSTATUS Status;
            ANSI_STRING Source;

            if (DebugFile.Buffer != NULL)
            {
                ExFreePool(DebugFile.Buffer);
                DebugFile.Length = 0;
                DebugFile.MaximumLength = 0;
            }

            RtlInitAnsiString(&Source, Argv[1]);
            Status = RtlAnsiStringToUnicodeString(&DebugFile, &Source, TRUE);
            if (NT_SUCCESS(Status))
            {
                DPRINT1("Debug file set to: %.*S\n", DebugFile.Length, DebugFile.Buffer);
            }
        }
    }
    else
    {
        DPRINT1("Unknown command: %s\n", Command);
    }

    return TRUE;
}
#endif
