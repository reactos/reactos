
#ifndef __NTOS_SERVICE_H
#define __NTOS_SERVICE_H


/* number of entries in the service descriptor tables */
#define SSDT_MAX_ENTRIES 4


#ifndef __USE_W32API

/* System Service Dispatch Table */
typedef PVOID (NTAPI * SSDT)(VOID);
typedef SSDT * PSSDT;

/* System Service Parameters Table */
typedef UCHAR SSPT, *PSSPT;

typedef struct t_KeServiceDescriptorTableEntry {
                PSSDT               SSDT;
                PULONG              ServiceCounterTable;
                ULONG               NumberOfServices;
                PSSPT               SSPT;

} SSDT_ENTRY, *PSSDT_ENTRY;

#endif /* __USE_W32API */


/* --- NTOSKRNL.EXE --- */
#if defined(__NTOSKRNL__)
#ifdef __GNUC__
extern
SSDT_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] __declspec(dllexport);
#else /* __GNUC__ */
/* Microsft-style */
extern
__declspec(dllexport)
SSDT_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
#endif /* __GNUC__ */
#else /* __NTOSKRNL__ */
#ifdef __GNUC__
extern
SSDT_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] __declspec(dllimport);
#else /* __GNUC__ */
/* Microsft-style */
extern
__declspec(dllimport)
SSDT_ENTRY
KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
#endif /* __GNUC__ */
#endif /* __NTOSKRNL__ */

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

