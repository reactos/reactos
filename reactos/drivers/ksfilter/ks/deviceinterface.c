#include "priv.h"

NTSTATUS
KspSetDeviceInterfacesState(
    IN PLIST_ENTRY ListHead,
    IN BOOL Enable)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Entry;
    PSYMBOLIC_LINK_ENTRY SymEntry;


    Entry = ListHead->Flink;
    while(Entry != ListHead)
    {
        /* fetch symbolic link entry */
        SymEntry = (PSYMBOLIC_LINK_ENTRY)CONTAINING_RECORD(Entry, SYMBOLIC_LINK_ENTRY, Entry);
        /* set device interface state */
        Status = IoSetDeviceInterfaceState(&SymEntry->SymbolicLink, Enable);
        /* check for success */
        if (!NT_SUCCESS(Status))
            return Status;
        /* get next entry */
        Entry = Entry->Flink;
    }
    /* return result */
    return Status;
}

NTSTATUS
KspFreeDeviceInterfaces(
    IN PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Entry;
    PSYMBOLIC_LINK_ENTRY SymEntry;

    while(!IsListEmpty(ListHead))
    {
        /* remove first entry */
        Entry = RemoveHeadList(ListHead);

        /* fetch symbolic link entry */
        SymEntry = (PSYMBOLIC_LINK_ENTRY)CONTAINING_RECORD(Entry, SYMBOLIC_LINK_ENTRY, Entry);

        /* free device interface string */
        RtlFreeUnicodeString(&SymEntry->SymbolicLink);
        /* free entry item */
        FreeItem(Entry);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KspRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN ULONG CategoriesCount,
    IN GUID const*Categories,
    IN PUNICODE_STRING ReferenceString,
    OUT PLIST_ENTRY SymbolicLinkList)
{
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;
    PSYMBOLIC_LINK_ENTRY SymEntry;

    for(Index = 0; Index < CategoriesCount; Index++)
    {
        /* allocate a symbolic link entry */
        SymEntry = AllocateItem(NonPagedPool, sizeof(SYMBOLIC_LINK_ENTRY));
        /* check for success */
        if (!SymEntry)
            return STATUS_INSUFFICIENT_RESOURCES;

        /* now register device interface */
        Status = IoRegisterDeviceInterface(PhysicalDeviceObject, 
                                           &Categories[Index],
                                           ReferenceString, /* see damn bug 4566 */
                                           &SymEntry->SymbolicLink);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to register device interface %x\n", Status);

            /* free entry */
            FreeItem(SymEntry);

            /* return result */
            return Status;
        }
        /* insert symbolic link entry */
        InsertTailList(SymbolicLinkList, &SymEntry->Entry);
    }

    /* return result */
    return Status;
}


