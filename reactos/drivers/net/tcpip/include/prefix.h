/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/info.h
 * PURPOSE:     TdiQueryInformation definitions
 */
#ifndef __PREFIX_H
#define __PREFIX_H

/* Prefix List Entry */
typedef struct _PREFIX_LIST_ENTRY {
    DEFINE_TAG
    LIST_ENTRY ListEntry;    /* Entry on list */
    ULONG RefCount;          /* Reference count */
    PIP_INTERFACE Interface; /* Pointer to interface */
    PIP_ADDRESS Prefix;      /* Pointer to prefix */
    UINT PrefixLength;       /* Length of prefix */
} PREFIX_LIST_ENTRY, *PPREFIX_LIST_ENTRY;

extern LIST_ENTRY PrefixListHead;
extern KSPIN_LOCK PrefixListLock;

VOID InitPLE();
PPREFIX_LIST_ENTRY CreatePLE(PIP_INTERFACE IF, PIP_ADDRESS Prefix, UINT Len);
VOID DestroyPLE(PPREFIX_LIST_ENTRY PLE);
VOID DestroyPLEs();

#endif/*__PREFIX_H*/
