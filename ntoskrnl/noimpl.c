#include <debug.h>

#if 0
HeadlessDispatch
IoInitializeCrashDump
KeI386AbiosCall
KeI386Call16BitCStyleFunction
KeI386Call16BitFunction
KeI386GetLid
KeI386ReleaseLid
KeI386SetGdtSelector
KeSetTimeUpdateNotifyRoutine
MmCommitSessionMappedView
ObIsDosDeviceLocallyMapped
ObSetHandleAttributes
PfxFindPrefix
PfxInitialize
PfxInsertPrefix
PfxRemovePrefix
RtlTraceDatabaseAdd
RtlTraceDatabaseCreate
RtlTraceDatabaseDestroy
RtlTraceDatabaseEnumerate
RtlTraceDatabaseFind
RtlTraceDatabaseLock
RtlTraceDatabaseUnlock
RtlTraceDatabaseValidate
SeTokenObjectType
VfFailDeviceNode
VfFailDriver
VfFailSystemBIOS
VfIsVerificationEnabled
WmiFlushTrace
WmiGetClock
WmiQueryTrace
WmiStartTrace
WmiStopTrace
WmiUpdateTrace
XIPDispatch
#endif

typedef struct
{
	int d_0000;		// 00
	int d_0004;		// 04
	// ...
} HEADLESS_GLOBALS;
HEADLESS_GLOBALS *HeadlessGlobals = 0;

// PAGEHDLS.HdlspDispatch
// internal fn called by HeadlessDispacth
NTSTATUS
HdlspDispatch(int Prm1, int Prm2, int Prm3, void *Prm4, int Prm5)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
__stdcall HeadlessDispatch(int Prm1, int Prm2, int Prm3, void *Prm4, int Prm5)
{
	// Prm1 = cmd ?
	// Prm2
	// Prm3
	// Prm4 = ptr to buffer / struct
	// Prm5 = size of buf/struct pointed to by Prm4
	if(HeadlessGlobals && HeadlessGlobals->d_0004)
	{
		return HdlspDispatch(Prm1, Prm2, Prm3, Prm4, Prm5);
	}
	switch(Prm1)
	{
	case 1:
		return STATUS_UNSUCCESSFUL;
	case 2:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x10:
		if(!Prm4 || !Prm5)
			return STATUS_INVALID_PARAMETER;
		memset(Prm4, 0, Prm5);
		return 0;
	default:
		return 0;
	}
}

// IoInitializeCrashDump
// introduced in xp sp3
NTSTATUS
__stdcall IoInitializeCrashDump(HANDLE Handle)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
__stdcall KeI386AbiosCall(int Prm1, int Prm2, int Prm3, int Prm4)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
__stdcall KeI386Call16BitCStyleFunction(int Prm1, int Prm2, int Prm3, int Prm4)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
__stdcall KeI386Call16BitFunction(int Prm1)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
__stdcall KeI386GetLid(int Prm1, int Prm2, int Prm3, int Prm4, int Prm5)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
__stdcall KeI386ReleaseLid(int Prm1, int Prm2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
__stdcall KeI386SetGdtSelector(int Prm1, int Prm2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

PTIME_UPDATE_NOTIFY_ROUTINE KiTimeUpdateNotifyRoutine;

NTKERNELAPI void _FASTCALL KeSetTimeUpdateNotifyRoutine(PTIME_UPDATE_NOTIFY_ROUTINE NotifyRoutine)
{
	UNIMPLEMENTED;
	KiTimeUpdateNotifyRoutine = NotifyRoutine;
}

NTSTATUS __stdcall MmCommitSessionMappedView(int Prm1, int Prm2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

// xp sp3
NTSTATUS __stdcall ObIsDosDeviceLocallyMapped(i32 Prm1, i8 *Prm2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS __stdcall ObSetHandleAttributes(i32 Prm1, i16 *Prm2, i8 Prm3)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

typedef struct
{
	i16 d_00;			// 0x00
	i16 d_02;			// 0x02
	i32 d_04;			// 0x04
	void *d_08;			// 0x08
	i32 d_0C;			// 0x0C
	i32 d_10;			// 0x10
	PSTRING Prefix;		// 0x14
} PREFIX_TABLE_ENTRY, *PPREFIX_TABLE_ENTRY;

typedef struct _PREFIX_TABLE
{
	i16 d_00;
	i16 d_02;						// 0x02 MaxNameLength
	struct _PREFIX_TABLE *d_04;		// 0x04 PPREFIX_TABLE_ENTRY or PPREFIX_TABLE
	
} PREFIX_TABLE, *PPREFIX_TABLE;

PPREFIX_TABLE_ENTRY __stdcall PfxFindPrefix(PPREFIX_TABLE PfxTbl,PSTRING Name)
{
	UNIMPLEMENTED;
	return 0;
}

void __stdcall PfxInitialize(PPREFIX_TABLE PfxTbl)
{
	PfxTbl->d_00 = 0;
	PfxTbl->d_02 = 0x200;
	PfxTbl->d_04 = PfxTbl;
}

i8 __stdcall PfxInsertPrefix(PPREFIX_TABLE Tbl, PSTRING Prefix, PPREFIX_TABLE_ENTRY Entry)
{
	UNIMPLEMENTED;
	return 0;
}

void __stdcall PfxRemovePrefix(PPREFIX_TABLE Tbl, PPREFIX_TABLE_ENTRY Entry)
{
	UNIMPLEMENTED;
}

i8 __stdcall RtlTraceDatabaseAdd(i32 Prm1, i32 Prm2, void *Prm3, i32 Prm4)
{
	UNIMPLEMENTED;
	return 0;
}

void * __stdcall RtlTraceDatabaseCreate(i32 Prm1, i32 Prm2, i32 Prm3, i32u Tag, i32 Prm5)
{
	UNIMPLEMENTED;
	return 0;
}

i8 __stdcall RtlTraceDatabaseDestroy(void *Prm1)
{
	UNIMPLEMENTED;
	return 0;
}

i8 __stdcall RtlTraceDatabaseEnumerate(void *Prm1, void *Prm2, i32 Prm3)
{
	UNIMPLEMENTED;
	return 0;
}

i8 __stdcall RtlTraceDatabaseFind(void *Prm1, i32 Prm2, i32 Prm3, i32 Prm4)
{
	UNIMPLEMENTED;
	return 0;
}

i8 __stdcall RtlTraceDatabaseLock(void *Prm1)
{
	UNIMPLEMENTED;
	return 0;
}

i8 __stdcall RtlTraceDatabaseUnlock(void *Prm1)
{
	UNIMPLEMENTED;
	return 0;
}

i8 __stdcall RtlTraceDatabaseValidate(void *Prm1)
{
	UNIMPLEMENTED;
	return 0;
}

POBJECT_TYPE SeTokenObjectType = 0;

void __cdecl VfFailDeviceNode(struct _DRIVER_OBJECT *DriverObject,int Prm2 ,int Prm3, int Prm4 ,int Prm5, int Prm6 ,int Prm7, int Prm8)
{
	UNIMPLEMENTED;
}

void __cdecl VfFailDriver(i32 Prm1, i32 Prm2, i32 Prm3, i32 Prm4, i32 Prm5, i32 Prm6)
{
	UNIMPLEMENTED;
}


void __cdecl VfFailSystemBIOS(i32 Prm1, i32 Prm2, i32 Prm3, i32 Prm4, i32 Prm5, i32 Prm6, i32 Prm7)
{
	UNIMPLEMENTED;
}

i8 VfIsVerificationEnabled(i32 Prm1, struct _DRIVER_OBJECT *DriverObject)
{
	UNIMPLEMENTED;
	return 0;
}


NTSTATUS __stdcall WmiFlushTrace(void *Prm1)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

// params?
i64 __fastcall WmiGetClock(void)
{
	UNIMPLEMENTED;
	return 0;	// time
}

NTSTATUS __stdcall WmiQueryTrace(void *Prm1)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS __stdcall WmiStartTrace(void *Prm1)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

__stdcall WmiStopTrace(void *Prm1)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS __stdcall WmiUpdateTrace(void *Prm1)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS __stdcall XIPDispatch(i32 Prm1, void *Dst,i32 Prm2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

// VOID NTAPI TstFunc(void) {};
