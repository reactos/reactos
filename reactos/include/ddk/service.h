
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
                PULONG              ServiceCounterTable;
                unsigned int        NumberOfServices;
                PSSPT               SSPT;

} KE_SERVICE_DESCRIPTOR_TABLE_ENTRY, *PKE_SERVICE_DESCRIPTOR_TABLE_ENTRY;

#pragma pack()


/* --- NTOSKRNL.EXE --- */
#if defined(__NTOSKRNL__)
extern
KE_SERVICE_DESCRIPTOR_TABLE_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] __declspec(dllexport);
#else
extern
KE_SERVICE_DESCRIPTOR_TABLE_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] __declspec(dllimport);
#endif

extern
KE_SERVICE_DESCRIPTOR_TABLE_ENTRY
KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];


BOOLEAN
STDCALL
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	);

#endif

