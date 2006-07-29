/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ob_x.h
* PURPOSE:         Intenral Inlined Functions for the Object Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

VOID
static __inline
ObpReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* Check if we have a security descriptor */
    if (ObjectCreateInfo->SecurityDescriptor)
    {
        /* Release it */
        SeReleaseSecurityDescriptor(ObjectCreateInfo->SecurityDescriptor,
                                    ObjectCreateInfo->ProbeMode,
                                    TRUE);
        ObjectCreateInfo->SecurityDescriptor = NULL;
    }
}

PVOID
static __inline
ObpAllocateCapturedAttributes(IN PP_NPAGED_LOOKASIDE_NUMBER Type)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PVOID Buffer;
    PNPAGED_LOOKASIDE_LIST List;

    /* Get the P list first */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].P;

    /* Attempt allocation */
    List->L.TotalAllocates++;
    Buffer = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
    if (!Buffer)
    {
        /* Let the balancer know that the P list failed */
        List->L.AllocateMisses++;

        /* Try the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].L;
        List->L.TotalAllocates++;
        Buffer = (PVOID)InterlockedPopEntrySList(&List->L.ListHead);
        if (!Buffer)
        {
            /* Let the balancer know the L list failed too */
            List->L.AllocateMisses++;

            /* Allocate it */
            Buffer = List->L.Allocate(List->L.Type, List->L.Size, List->L.Tag);
        }
    }

    /* Return buffer */
    return Buffer;
}

VOID
static __inline
ObpFreeCapturedAttributes(IN PVOID Buffer,
                          IN PP_NPAGED_LOOKASIDE_NUMBER Type)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PNPAGED_LOOKASIDE_LIST List;

    /* Use the P List */
    List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].P;
    List->L.TotalFrees++;

    /* Check if the Free was within the Depth or not */
    if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
    {
        /* Let the balancer know */
        List->L.FreeMisses++;

        /* Use the L List */
        List = (PNPAGED_LOOKASIDE_LIST)Prcb->PPLookasideList[Type].L;
        List->L.TotalFrees++;

        /* Check if the Free was within the Depth or not */
        if (ExQueryDepthSList(&List->L.ListHead) >= List->L.Depth)
        {
            /* All lists failed, use the pool */
            List->L.FreeMisses++;
            List->L.Free(Buffer);
        }
    }
    else
    {
        /* The free was within the Depth */
        InterlockedPushEntrySList(&List->L.ListHead,
                                  (PSINGLE_LIST_ENTRY)Buffer);
    }
}

VOID
static __inline
ObpReleaseCapturedName(IN PUNICODE_STRING Name)
{
    /* We know this is a pool-allocation if the size doesn't match */
    if (Name->MaximumLength != 248)
    {
        ExFreePool(Name->Buffer);
    }
    else
    {
        /* Otherwise, free from the lookaside */
        ObpFreeCapturedAttributes(Name, LookasideNameBufferList);
    }
}

VOID
static __inline
ObpFreeAndReleaseCapturedAttributes(IN POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
    /* First release the attributes, then free them from the lookaside list */
    ObpReleaseCapturedAttributes(ObjectCreateInfo);
    ObpFreeCapturedAttributes(ObjectCreateInfo, LookasideCreateInfoList);
}

