#pragma once

#include <ntddk.h>
#include <windef.h>
#include <ks.h>
#define NOBITMAP
#include <mmreg.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <bdasup.h>

#define YDEBUG
#include <debug.h>


typedef struct
{
    LIST_ENTRY Entry;
    PKSFILTERFACTORY FilterFactoryInstance;
    PBDA_FILTER_TEMPLATE FilterTemplate;
}BDA_FILTER_INSTANCE_ENTRY, *PBDA_FILTER_INSTANCE_ENTRY;

typedef struct
{
    BOOLEAN Initialized;
    KSPIN_LOCK FilterFactoryInstanceListLock;
    LIST_ENTRY FilterFactoryInstanceList;
}BDA_GLOBAL, *PBDA_GLOBAL;


extern BDA_GLOBAL g_Settings;


PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes);

VOID
FreeItem(
    IN PVOID Item);
