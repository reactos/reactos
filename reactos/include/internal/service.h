
#ifndef __INTERNAL_SERVICE_H
#define __INTERNAL_SERVICE_H


/* number of entries in the service descriptor tables */
#define SSDT_MAX_ENTRIES 4


#pragma pack(1)

// System Service Dispatch Table
typedef struct t_SSDT {
                ULONG           SysCallPtr;
} SSDT, *PSSDT;

// System Service Parameters Table
typedef struct t_SSPT   {
                unsigned int    ParamBytes;
} SSPT, *PSSPT;

typedef struct t_KeServiceDescriptorTableEntry {
                PSSDT               SSDT;
                unsigned long*      ServiceCounterTable;
                unsigned int        NumberOfServices;
                PSSPT               SSPT;

} KE_SERVICE_DESCRIPTOR_TABLE_ENTRY, *PKE_SERVICE_DESCRIPTOR_TABLE_ENTRY;

#pragma pack()

BOOLEAN
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	);

#endif

