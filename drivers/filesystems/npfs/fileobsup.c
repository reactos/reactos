/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/fileobsup.c
 * PURPOSE:     Pipes File Object Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_FILEOBSUP)

/* FUNCTIONS ******************************************************************/

NODE_TYPE_CODE
NTAPI
NpDecodeFileObject(IN PFILE_OBJECT FileObject,
                   OUT PVOID* PrimaryContext OPTIONAL,
                   OUT PNP_CCB* Ccb,
                   OUT PULONG NamedPipeEnd OPTIONAL)
{
    ULONG_PTR Context;
    PNP_CCB Node;
    PAGED_CODE();

    Context = (ULONG_PTR)FileObject->FsContext;
    if ((Context) && (Context != 1))
    {
        if (NamedPipeEnd) *NamedPipeEnd = Context & 1;

        Node = (PVOID)(Context & ~1);

        switch (Node->NodeType)
        {
            case NPFS_NTC_VCB:
                return NPFS_NTC_VCB;

            case NPFS_NTC_ROOT_DCB:
                *Ccb = FileObject->FsContext2;
                if (PrimaryContext) *PrimaryContext = Node;
                return NPFS_NTC_ROOT_DCB;

            case NPFS_NTC_CCB:
                *Ccb = Node;
                if (PrimaryContext) *PrimaryContext = Node->Fcb;
                return NPFS_NTC_CCB;

            default:
                NpBugCheck(Node->NodeType, 0, 0);
                break;
            }
    }

    return 0;
}

VOID
NTAPI
NpSetFileObject(IN PFILE_OBJECT FileObject,
                IN PVOID PrimaryContext,
                IN PVOID Ccb,
                IN ULONG NamedPipeEnd)
{
    BOOLEAN FileIsPipe;
    PAGED_CODE();

    if (!FileObject) return;

    if ((PrimaryContext) && (((PNP_CCB)PrimaryContext)->NodeType == NPFS_NTC_CCB))
    {
        FileIsPipe = TRUE;
        if (NamedPipeEnd == FILE_PIPE_SERVER_END)
        {
            PrimaryContext = (PVOID) ((ULONG_PTR) PrimaryContext | 1);
        }
    }
    else
    {
        FileIsPipe = FALSE;
    }

    FileObject->FsContext = PrimaryContext;
    FileObject->FsContext2 = Ccb;
    FileObject->PrivateCacheMap = (PVOID)1;
    if (FileIsPipe) FileObject->Flags |= FO_NAMED_PIPE;
}

/* EOF */
