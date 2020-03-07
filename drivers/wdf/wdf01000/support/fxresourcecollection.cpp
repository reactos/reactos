#include "common/fxresource.h"
#include "common/mxmemory.h"


_Must_inspect_result_
FxIoResReqList*
FxIoResReqList::_CreateFromWdmList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PIO_RESOURCE_REQUIREMENTS_LIST WdmRequirementsList,
    __in UCHAR AccessFlags
    )
/*++

Routine Description:
    Allocates and populates an FxIoResReqList based on the WDM resource
    requirements list.

Arguments:
    WdmRequirementsList - a list of IO_RESOURCE_LISTs which will indicate how
                          to  fill in the returned collection object

    AccessFlags - permissions to associate with the newly created object

Return Value:
    a new object upon success, NULL upon failure

  --*/

{
    FxIoResReqList* pIoResReqList;
    ULONG i;

    pIoResReqList = new(FxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
        FxIoResReqList(FxDriverGlobals, AccessFlags);

    if (pIoResReqList != NULL)
    {
        PIO_RESOURCE_LIST pWdmResourceList;
        NTSTATUS status;

        if (WdmRequirementsList == NULL)
        {
            return pIoResReqList;
        }

        status = STATUS_SUCCESS;
        pWdmResourceList = &WdmRequirementsList->List[0];

        pIoResReqList->m_InterfaceType = WdmRequirementsList->InterfaceType;
        pIoResReqList->m_SlotNumber = WdmRequirementsList->SlotNumber;

        for (i = 0; i < WdmRequirementsList->AlternativeLists; i++)
        {
            FxIoResList *pResList;

            pResList = new(FxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
                FxIoResList(FxDriverGlobals, pIoResReqList);

            if (pResList != NULL)
            {
                status = pResList->AssignParentObject(pIoResReqList);

                //
                // Since we control our own lifetime, assigning the parent should
                // never fail.
                //
                ASSERT(NT_SUCCESS(status));

                status = pResList->BuildFromWdmList(&pWdmResourceList);
            }
            else
            {
                //
                // We failed to allocate a child collection.  Clean up
                // and break out of the loop.
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status))
            {
                break;
            }
        }

        if (!NT_SUCCESS(status))
        {
            //
            // Cleanup and return a NULL object
            //
            pIoResReqList->DeleteObject();
            pIoResReqList = NULL;
        }
    }

    return pIoResReqList;
}

_Must_inspect_result_
NTSTATUS
FxIoResList::BuildFromWdmList(
    __deref_in PIO_RESOURCE_LIST* WdmResourceList
    )
/*++

Routine Description:
    Builds up the collection with FxResourceIo objects based on the passed in
    WDM io resource list

Arguments:
    WdmResourceList - list which specifies the io resource objects to create

Return Value:
    NTSTATUS

  --*/
{
    PIO_RESOURCE_DESCRIPTOR pWdmDescriptor;
    ULONG i, count;
    NTSTATUS status;

    pWdmDescriptor = &(*WdmResourceList)->Descriptors[0];
    count = (*WdmResourceList)->Count;
    status = STATUS_SUCCESS;

    for (i = 0; i < count; i++)
    {
        //
        // Now create a new resource object for each resource
        // in our list.
        //
        FxResourceIo *pResource;

        pResource = new(GetDriverGlobals())
            FxResourceIo(GetDriverGlobals(), pWdmDescriptor);

        if (pResource == NULL)
        {
            //
            // We failed, clean up, and exit.  Since we are only
            // keeping references on the master collection, if
            // we free this, everything else will go away too.
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(status))
        {
            status = pResource->AssignParentObject(this);

            //
            // See notes in previous AssignParentObject as to why
            // we are asserting.
            //
            ASSERT(NT_SUCCESS(status));
            UNREFERENCED_PARAMETER(status);

            status = Add(pResource) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(status))
        {
            break;
        }

        pWdmDescriptor++;
    }

    if (NT_SUCCESS(status))
    {
        status = m_OwningList->Add(this) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status))
    {
        *WdmResourceList = (PIO_RESOURCE_LIST) pWdmDescriptor;
    }

    return status;
}

_Must_inspect_result_
PIO_RESOURCE_REQUIREMENTS_LIST
FxIoResReqList::CreateWdmList(
    VOID
    )
/*++

Routine Description:
    Creates a WDM io resource requirements list based off of the current
    contents of the collection

Arguments:
    None

Return Value:
    new WDM io resource requirements list allocated out of paged pool upon success,
    NULL upon failure or an empty list

  --*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST pRequirementsList;
    FxCollectionEntry *cur, *end;
    NTSTATUS status;
    ULONG totalDescriptors;
    ULONG size;
    ULONG count;
    ULONG tmp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    totalDescriptors = 0;
    pRequirementsList = NULL;

    count = Count();
    pFxDriverGlobals = GetDriverGlobals();

    if (count > 0)
    {
        //
        // The collection object should contain a set of child collections
        // with each of the various requirement lists.  Use the number of
        // these collections to determine the size of our requirements
        // list.
        //
        end = End();
        for (cur = Start(); cur != end; cur = cur->Next())
        {
            status = RtlULongAdd(totalDescriptors,
                                 ((FxIoResList *) cur->m_Object)->Count(),
                                 &totalDescriptors);

            if (!NT_SUCCESS(status))
            {
                goto Overflow;
            }
        }

        //
        // We now have enough information to determine how much memory we
        // need to allocate for our requirements list.
        //
        // size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
        //        (sizeof(IO_RESOURCE_LIST) * (count - 1)) +
        //        (sizeof(IO_RESOURCE_DESCRIPTOR) * totalDescriptors) -
        //         (sizeof(IO_RESOURCE_DESCRIPTOR) * count);
        //
        // sizeof(IO_RESOURCE_DESCRIPTOR) * count is subtracted off because
        // each IO_RESOURCE_LIST has an embedded IO_RESOURCE_DESCRIPTOR in it
        // and we don't want to overallocated.
        //

        //
        // To handle overflow each mathematical operation is split out into an
        // overflow safe call.
        //
        size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

        // sizeof(IO_RESOURCE_LIST) * (count - 1)
        status = RtlULongMult(sizeof(IO_RESOURCE_LIST), count - 1, &tmp);
        if (!NT_SUCCESS(status))
        {
            goto Overflow;
        }

        status = RtlULongAdd(size, tmp, &size);
        if (!NT_SUCCESS(status))
        {
            goto Overflow;
        }

        // (sizeof(IO_RESOURCE_DESCRIPTOR) * totalDescriptors)
        status = RtlULongMult(sizeof(IO_RESOURCE_DESCRIPTOR),
                              totalDescriptors,
                              &tmp);
        if (!NT_SUCCESS(status))
        {
            goto Overflow;
        }

        status = RtlULongAdd(size, tmp, &size);
        if (!NT_SUCCESS(status))
        {
            goto Overflow;
        }

        //  - sizeof(IO_RESOURCE_DESCRIPTOR) * Count() (note the subtraction!)
        status = RtlULongMult(sizeof(IO_RESOURCE_DESCRIPTOR), count, &tmp);
        if (!NT_SUCCESS(status))
        {
            goto Overflow;
        }

        // Sub, not Add!
        status = RtlULongSub(size, tmp, &size);
        if (!NT_SUCCESS(status))
        {
            goto Overflow;
        }

        pRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST)
            MxMemory::MxAllocatePoolWithTag(PagedPool, size, pFxDriverGlobals->Tag);

        if (pRequirementsList != NULL)
        {
            PIO_RESOURCE_LIST pList;
            FxResourceIo *pResource;

            pList = pRequirementsList->List;

            //
            // Start by zero initializing our structure
            //
            RtlZeroMemory(pRequirementsList, size);

            //
            // InterfaceType and BusNumber are unused for WDM, but InterfaceType
            // is used by the arbiters.
            //
            pRequirementsList->InterfaceType = m_InterfaceType;

            pRequirementsList->SlotNumber = m_SlotNumber;

            //
            // Now populate the requirements list with the resources from
            // our collections.
            //
            pRequirementsList->ListSize = size;
            pRequirementsList->AlternativeLists = Count();

            end = End();
            for (cur = Start(); cur != end; cur = cur->Next())
            {
                FxIoResList* pIoResList;
                PIO_RESOURCE_DESCRIPTOR pDescriptor;
                FxCollectionEntry *pIoResCur, *pIoResEnd;

                pIoResList = (FxIoResList*) cur->m_Object;

                pList->Version  = 1;
                pList->Revision = 1;
                pList->Count = pIoResList->Count();

                pDescriptor = pList->Descriptors;
                pIoResEnd = pIoResList->End();
                
                for (pIoResCur = pIoResList->Start();
                     pIoResCur != pIoResEnd;
                     pIoResCur = pIoResCur->Next())
                {

                    pResource = (FxResourceIo *) pIoResCur->m_Object;
                    RtlCopyMemory(pDescriptor,
                                  &pResource->m_Descriptor,
                                  sizeof(pResource->m_Descriptor));
                    pDescriptor++;
                }

                pList = (PIO_RESOURCE_LIST) pDescriptor;
            }
        }
    }

    return pRequirementsList;

Overflow:
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                        "Integer overflow occured when computing size of "
                        "IO_RESOURCE_REQUIREMENTS_LIST");

    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxCmResList::BuildFromWdmList(
    __in PCM_RESOURCE_LIST WdmResourceList,
    __in UCHAR AccessFlags
    )
/*++

Routine Description:
    Builds up the collection with FxResourceCm objects based on the passed in
    WDM io resource list.

Arguments:
    WdmResourceList - list which specifies the io resource objects to create

    AccessFlags - permissions to be associated with the list

Return Value:
    NTSTATUS

  --*/
{
    NTSTATUS status;

    //
    // Predispose to success
    //
    status = STATUS_SUCCESS;

    Clear();

    m_AccessFlags = AccessFlags;

    if (WdmResourceList != NULL)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
        ULONG count, i;

        //
        // We only expect to see one full resource descriptor.
        //
        ASSERT(WdmResourceList->Count == 1);

        count = WdmResourceList->List[0].PartialResourceList.Count;
        pDescriptor = WdmResourceList->List[0].PartialResourceList.PartialDescriptors;

        for (i = 0; i < count; i++, pDescriptor++)
        {
            FxResourceCm *pResource;

            pResource = new(GetDriverGlobals())
                FxResourceCm(GetDriverGlobals(), pDescriptor);

            if (pResource == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(status))
            {
                status = pResource->AssignParentObject(this);

                //
                // Since we control our own lifetime here, the assign should
                // always work.
                //
                ASSERT(NT_SUCCESS(status));

                status = Add(pResource) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status))
            {
                Clear();
                break;
            }
        }
    }

    return status;
}

_Must_inspect_result_
PCM_RESOURCE_LIST
FxCmResList::CreateWdmList(
    __in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType
    )
/*++

Routine Description:
    Allocates and initializes a WDM CM resource list based off of the current
    contents of this collection.

Arguments:
    PoolType - the pool type from which to allocate the resource list

Return Value:
    a new resource list upon success, NULL upon failure

  --*/
{
    PCM_RESOURCE_LIST pWdmResourceList;
    ULONG size;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pWdmResourceList = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    if (Count())
    {
        //
        // NOTE: This function assumes all resources are on the same bus
        // and therefore there is only one FULL_RESOURCE_DESCRIPTOR.
        //
        size = sizeof(CM_RESOURCE_LIST) +
               (sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (Count() - 1));

        pWdmResourceList = (PCM_RESOURCE_LIST)
            MxMemory::MxAllocatePoolWithTag(PoolType, size, pFxDriverGlobals->Tag);

        if (pWdmResourceList != NULL)
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
            FxCollectionEntry *cur, *end;

            RtlZeroMemory(pWdmResourceList, size);

            pWdmResourceList->Count = 1;  // We only return one full descriptor

            pWdmResourceList->List[0].PartialResourceList.Version  = 1;
            pWdmResourceList->List[0].PartialResourceList.Revision = 1;
            pWdmResourceList->List[0].PartialResourceList.Count = Count();

            pDescriptor =
                pWdmResourceList->List[0].PartialResourceList.PartialDescriptors;

            end = End();
            for (cur = Start(); cur != end; cur = cur->Next())
            {
                FxResourceCm *pResource;

                pResource = (FxResourceCm*) cur->m_Object;

                RtlCopyMemory(pDescriptor,
                              &pResource->m_Descriptor,
                              sizeof(pResource->m_Descriptor));
                pDescriptor++;
            }
        }
    }

    return pWdmResourceList;
}
