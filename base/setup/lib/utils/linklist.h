/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Linked list support macros
 * COPYRIGHT:   Copyright 2005-2018 ReactOS Team
 */

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

/* EOF */
