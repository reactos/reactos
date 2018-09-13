/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmdelete.c

Abstract:

    This module contains the delete object method (used to delete key
    control blocks  when last handle to a key is closed, and to delete
    keys marked for deletetion when last reference to them goes away.)

Author:

    Bryan M. Willman (bryanwi) 13-Nov-91

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpDeleteKeyObject)
#endif


VOID
CmpDeleteKeyObject(
    IN PVOID Object
    )
/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    the last reference to a particular Key object (or Key Root object)
    is destroyed.

    If the Key object going away holds the last reference to
    the extension it is associated with, that extension is destroyed.

Arguments:

    Object - supplies a pointer to a KeyRoot or Key, thus -> KEY_BODY.

Return Value:

    NONE.

--*/
{
    PCM_KEY_CONTROL_BLOCK   KeyControlBlock;
    PCM_KEY_BODY            KeyBody;

    PAGED_CODE();

    CMLOG(CML_MAJOR, CMS_NTAPI) {
        KdPrint(("CmpDeleteKeyObject: Object = %08lx\n", Object));
    }

    CmpLockRegistry();

    KeyBody = (PCM_KEY_BODY)Object;

    if (KeyBody->Type==KEY_BODY_TYPE) {
        KeyControlBlock = KeyBody->KeyControlBlock;

        //
        // the keybody should be initialized; when kcb is null, something went wrong
        // between the creation and the dereferenciation of the object
        //
        if( KeyControlBlock != NULL ) {

            //
            // Clean up any outstanding notifies attached to the KeyBody
            //
            CmpFlushNotify(KeyBody);

            //
            // Remove our reference to the KeyControlBlock, clean it up, perform any
            // pend-till-final-close operations.
            //
            // NOTE: Delete notification is seen at the parent of the deleted key,
            //       not the deleted key itself.  If any notify was outstanding on
            //       this key, it was cleared away above us.  Only parent/ancestor
            //       keys will see the report.
            //
            //
            // The dereference will free the KeyControlBlock.  If the key was deleted, it
            // has already been removed from the hash table, and relevent notifications
            // posted then as well.  All we are doing is freeing the tombstone.
            //
            // If it was not deleted, we're both cutting the kcb out of
            // the kcb list/tree, AND freeing its storage.
            //

            DELIST_KEYBODY_FROM_KEYBODY_LIST(KeyBody);
            CmpDereferenceKeyControlBlock(KeyControlBlock);
        }
    } else {
        //
        // This must be a predefined handle
        //  some sanity asserts
        //
        KeyControlBlock = KeyBody->KeyControlBlock;

        ASSERT( KeyBody->Type&REG_PREDEF_HANDLE_MASK);
        ASSERT( KeyControlBlock->Flags&KEY_PREDEF_HANDLE );
        
        if( KeyControlBlock != NULL ) {
            CmpDereferenceKeyControlBlock(KeyControlBlock);
        }

    }
    CmpUnlockRegistry();
    return;
}

