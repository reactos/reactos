
#ifndef __NTOS_SERVICE_H
#define __NTOS_SERVICE_H


/* number of entries in the service descriptor tables */
#define SSDT_MAX_ENTRIES 4


#ifndef __USE_W32API

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

} SSDT_ENTRY, *PSSDT_ENTRY;

#pragma pack()

#endif /* __USE_W32API */


/* --- NTOSKRNL.EXE --- */
#if defined(__NTOSKRNL__)
extern
SSDT_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] __declspec(dllexport);
#else
extern
SSDT_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] __declspec(dllimport);
#endif

extern
SSDT_ENTRY
KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];


#ifndef __USE_W32API

BOOLEAN
STDCALL
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	);

#endif /* __USE_W32API */

#endif /* __NTOS_SERVICE_H */

