
#pragma once

#define InsertAscendingList(ListHead, NewEntry, Type, ListEntryField, SortField) \
do { \
    PLIST_ENTRY current = (ListHead)->Flink; \
    while (current != (ListHead)) \
    { \
        if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >= \
            (NewEntry)->SortField) \
        { \
            break; \
        } \
        current = current->Flink; \
    } \
\
    InsertTailList(current, &((NewEntry)->ListEntryField)); \
} while (0)
