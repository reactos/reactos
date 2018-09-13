/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmclose.c

Abstract:

    This module contains the close object method.

Author:

    Bryan M. Willman (bryanwi) 07-Jan-92

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpCloseKeyObject)
#endif

VOID
CmpCloseKeyObject(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )
/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    a Key object (or Key Root object) is closed.

    It's function is to do cleanup processing by waking up any notifies
    pending on the handle.  This keeps the key object from hanging around
    forever because a synchronous notify is stuck on it somewhere.

    All other cleanup, in particular, the freeing of storage, will be
    done in CmpDeleteKeyObject.

Arguments:

    Process - ignored

    Object - supplies a pointer to a KeyRoot or Key, thus -> KEY_BODY.

    GrantedAccess, ProcessHandleCount, SystemHandleCount - ignored

Return Value:

    NONE.

--*/
{
    PCM_KEY_BODY        KeyBody;
    PCM_NOTIFY_BLOCK    NotifyBlock;

    PAGED_CODE();
    CMLOG(CML_MAJOR, CMS_NTAPI|CMS_POOL) {
        KdPrint(("CmpCloseKeyObject: Object = %08lx\n", Object));
    }

    if( SystemHandleCount > 1 ) {
        //
        // There are still has open handles on this key. Do nothing
        //
        return;
    }

    CmpLockRegistry();

    KeyBody = (PCM_KEY_BODY)Object;

    //
    // Check the type, it will be something else if we are closing a predefined
    // handle key
    //
    if (KeyBody->Type == KEY_BODY_TYPE) {
        //
        // Clean up any outstanding notifies attached to the KeyBody
        //
        if (KeyBody->NotifyBlock != NULL) {
            //
            // Post all PostBlocks waiting on the NotifyBlock
            //
            NotifyBlock = KeyBody->NotifyBlock;
            if (IsListEmpty(&(NotifyBlock->PostList)) == FALSE) {
                CmLockHive((PCMHIVE)(KeyBody->KeyControlBlock->KeyHive));
                CmpPostNotify(NotifyBlock,
                              NULL,
                              0,
                              STATUS_NOTIFY_CLEANUP,
                              NULL);
                CmUnlockHive((PCMHIVE)(KeyBody->KeyControlBlock->KeyHive));
            }
        }
    }

    CmpUnlockRegistry();
    return;
}
