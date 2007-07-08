/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/multimedia/portcls/helpers/ResourceList.c
 * PURPOSE:         Port Class driver / ResourceList implementation
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.h"
#include <portcls.h>
#include <stdunk.h>

typedef struct CResourceList
{
    union
    {
        IUnknown IUnknown;
        IResourceList IResourceList;
    };

    LONG m_ref_count;
    PUNKNOWN m_outer_unknown;

    PCM_RESOURCE_LIST translated_resources;
    PCM_RESOURCE_LIST untranslated_resources;
} CResourceList;


/*
    Basic IUnknown methods
*/

NTSTATUS
STDMETHODCALLTYPE
ResourceList_QueryInterface(
    IResourceList* this_container,
    IN  REFIID refiid,
    OUT PVOID* output)
{
    /* TODO */
    return STATUS_SUCCESS;
}

ULONG
STDMETHODCALLTYPE
ResourceList_AddRef(
    IResourceList* this_container)
{
    struct CUnknown* this = CONTAINING_RECORD(this_container, struct CUnknown, IUnknown);

/* fixme */
/*    ExInterlockedIncrement(&this->m_ref_count); */
    return this->m_ref_count;
}

ULONG
STDMETHODCALLTYPE
ResourceList_Release(
    IResourceList* this_container)
{
    struct CUnknown* this = CONTAINING_RECORD(this_container, struct CUnknown, IUnknown);

/* fixme */
/*    ExInterlockedDecrement(&this->m_ref_count); */

    if ( this->m_ref_count == 0 )
    {
        ExFreePool(this);
        return 0;
    }

    return this->m_ref_count;
}


/*
    IResourceList methods
*/

ULONG
STDMETHODCALLTYPE
ResourceList_NumberOfEntries(IResourceList* this_container)
{
    return 0;
}

ULONG
STDMETHODCALLTYPE
ResourceList_NumberOfEntriesOfType(
    IResourceList* this_container,
    IN  CM_RESOURCE_TYPE type)
{
    /* I guess the translated and untranslated lists will be same length? */

    CResourceList* this = CONTAINING_RECORD(this_container, CResourceList, IResourceList);
    ULONG index, count = 0;

    for ( index = 0; index < this->translated_resources->Count; index ++ )
    {
        PCM_FULL_RESOURCE_DESCRIPTOR full_desc = &this->translated_resources->List[index];
        ULONG sub_index;

        for ( sub_index = 0; sub_index < full_desc->PartialResourceList.Count; sub_index ++ )
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR partial_desc;
            partial_desc = &full_desc->PartialResourceList.PartialDescriptors[sub_index];

            if ( partial_desc->Type == type )
            {
                /* Yay! Finally found one that matches! */
                count ++;
            }
        }
    }

    DPRINT("Found %d\n", count);
    return count;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
STDMETHODCALLTYPE
ResourceList_FindTranslatedEntry(
    IResourceList* this_container,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    return NULL;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
STDMETHODCALLTYPE
ResourceList_FindUntranslatedEntry(
    IResourceList* this_container,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    return NULL;
}

NTSTATUS
STDMETHODCALLTYPE
ResourceList_AddEntry(
    IResourceList* this_container,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated)
{
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
ResourceList_AddEntryFromParent(
    IResourceList* this_container,
    IN  IResourceList* Parent,
    IN  CM_RESOURCE_TYPE Type,
    IN  ULONG Index)
{
    return STATUS_SUCCESS;
}

PCM_RESOURCE_LIST
STDMETHODCALLTYPE
ResourceList_TranslatedList(
    IResourceList* this_container)
{
    return NULL;
}

PCM_RESOURCE_LIST
STDMETHODCALLTYPE
ResourceList_UntranslatedList(
    IResourceList* this_container)
{
    return NULL;
}


/*
    ResourceList V-Table
*/
static const IResourceListVtbl ResourceListVtbl =
{
    /* IUnknown */
    ResourceList_QueryInterface,
    ResourceList_AddRef,
    ResourceList_Release,

    /* IResourceList */
    ResourceList_NumberOfEntries,
    ResourceList_NumberOfEntriesOfType,
    ResourceList_FindTranslatedEntry,
    ResourceList_FindUntranslatedEntry,
    ResourceList_AddEntry,
    ResourceList_AddEntryFromParent,
    ResourceList_TranslatedList,
    ResourceList_UntranslatedList
};


/*
    Factory for creating a resource list
*/
PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceList(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PCM_RESOURCE_LIST TranslatedResources,
    IN  PCM_RESOURCE_LIST UntranslatedResources)
{
    IResourceList* new_list = NULL;
    CResourceList* this = NULL;

    /* TODO: Validate parameters */

    DPRINT("PcNewResourceList\n");

    new_list = ExAllocatePoolWithTag(sizeof(IResourceList), PoolType, TAG_PORTCLASS);

    if ( ! new_list )
    {
        DPRINT("ExAllocatePoolWithTag failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Obtain our private data */
    this = CONTAINING_RECORD(new_list, CResourceList, IResourceList);

    ASSERT(this);

    /* Initialize */
    this->m_outer_unknown = OuterUnknown;
    this->translated_resources = TranslatedResources;
    this->untranslated_resources = UntranslatedResources;

    /* Increment our usage count and set the pointer to this object */
    *OutResourceList = new_list;
    ResourceListVtbl.AddRef(*OutResourceList);

    return STATUS_SUCCESS;
}

PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceSublist(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ParentList,
    IN  ULONG MaximumEntries)
{
    return STATUS_UNSUCCESSFUL;
}
