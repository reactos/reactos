#pragma once

/*	cpu.h
	definitions for intel ia-32 cpu and cpu.lib
	(c) 2010 Jose M. Catena Gomez
*/

#include <cpu_c.h>	// compiler specific

#pragma pack(push, 1)

// eflags
typedef union {
	i32u x;
	struct	{
		i32u Carry:1;				// 0.0 0x00000001 c carry
		i32u Rsv01:1;				// 0.1 0x00000002 1
		i32u Parity:1;				// 0.2 0x00000004 p parity
		i32u Rsv03:1;				// 0.3 0x00000008 0
		i32u Aux:1;					// 0.4 0x00000010 a aux
		i32u Rsv05:1;				// 0.5 0x00000020 0
		i32u Zero:1;				// 0.6 0x00000040 z zero
		i32u Sign:1;				// 0.7 0x00000080 s sign
		i32u Trap:1;				// 1.0 0x00000100 t trap (single step enable)
		i32u IntEnable:1;			// 1.1 0x00000200 i interrupt enable
		i32u Direction:1;			// 1.2 0x00000400 d direction
		i32u Overflow:1;			// 1.3 0x00000800 o overflow
		i32u IoPrivilege:2;			// 1.4 0x00003000 iop io privilege
		i32u NestedTask:1;			// 1.6 0x00004000 nt nested task
		i32u Rsv0f:1;				// 1.7 0x00008000 0
		i32u Resume:1;				// 2.0 0x00010000 r resume
		i32u Vir8086:1;				// 2.1 0x00020000 vm virtual 8086 mode
		i32u Align:1;				// 2.2 0x00040000 a alignment check
		i32u VirIntFlag:1;			// 2.3 0x00080000 vif virtual interrupt flag
		i32u VirIntPending:1;		// 2.4 0x00100000 vip virtual interrupt pending
		i32u id:1;					// 2.5 0x00200000 id
		i32u rsv16:2;				// 2.6 0x00C00000 0
		i32u rsv18;					// 3.0 0xFF000000 0
	};								// 4.0
} CPUR_F;

typedef union {
	i32u x;
	struct {
		i32u ProtMode:1;			// 0.0 0x00000001 pm protection enable
		i32u FpuMon:1;				// 0.1 0x00000002 mp monitor coprocessor
		i32u FpuEmul;				// 0.2 0x00000004 em emulation fpu
		i32u FpuSwitch:1;			// 0.3 0x00000008 ts task switch, autosave fpu state
		i32u FupExt:1;				// 0.4 0x00000010 et extension type = 1 (obsolete)
		i32u FpuErr:1;				// 0.5 0x00000020 ne numeric error = 1 (obsolete) 
		i32u Rsv06:2;				// 0.6
		i32u Rsv08:8;				// 1.0
		i32u WrProt:1;				// 2.0 0x00010000 write protect
		i32u Rsv21:1;				// 2.1
		i32u Align:1;				// 2.2 0x00040000 am alignment mask
		i32u Rsv23:5;				// 2.3
		i32u Rsv30:5;				// 3.0
		i32u NoWrThrough:1;			// 3.5 0x20000000 nw disable write trough/back
		i32u NoCache:1;				// 3.6 0x40000000 cd cache disable
		i32u Paging:1;				// 3.7 0x80000000 pg paging enable
	};								// 4.0 
} CPUR_CR0;

typedef union {
	i32u x;
	struct {
		i32u Rsv0:3;				// 0.0
		i32u PagWrThrough:1;		// 0.3 0x00000008 pwt page level write through (0=write back)
		i32u PagNoCache:1;			// 0.4 0x00000010 pcd page level cache disable
		i32u Rsv05:2;				// 0.5
		i32u Rsv08:4;				// 1.0
		i32u PagDirBase:20;			// 1.4 pd page directory base
	};								// 4.0
} CPUR_CR3;

typedef union {
	i32u x;
	struct {
		i32u VmIntExt:1;			// 0.0 0x00000001 vme enable int/trap redirection to virtual machine
		i32u VmIntPm:1;				// 0.1 0x00000002 pvi enable virtual interrupts in protectd mode
		i32u NoTimeStamp:1;			// 0.2 0x00000004 tsd disable access to rdtsc in user mode
		i32u NoDbgCr:1;				// 0.3 0x00000008 de disable access to cr4 and cr5 debug register aliases
		i32u PagBig:1;				// 0.4 0x00000010 pse enable 4 mbyte pages
		i32u PAE:1;					// 0.5 0x00000020 pae enable physical address extension
		i32u MachineCheck:1;		// 0.6 0x00000040 mce enable machine chack exception
		i32u PagGlobal:1;			// 0.7 0x00000080 pge enable global pages
		i32u PerfCnt:1;				// 1.0 0x00000100 pce enable access to performance counter from user mode
		i32u Sse:1;					// 1.1 0x00000200 osfxr enable sse, fxsave, fxrstor
		i32u SseExcept:1;			// 1.2 0x00000400 osxmmexcpt enable sse exceptions
		i32u Rsv13:2;				// 1.3
		i32u VmExt:1;				// 1.5 0x00002000 vmxe enable virtual machine extensions
		i32u SafeExt:1;				// 1.6 0x00004000 smx enable safer mode extensions
		i32u Rsv17:3;				// 1.7
		i32u Xsave:1;				// 2.2 0x0004000 osxsave enable xsave/xrstor
		i32u Rsv23:13;				// 2.3
	};								// 4.0
} CPUR_CR4;

typedef union {
	i32u x;
	struct {
		i32u TaskPriority:4;		// 0.0 0x0000000f tpl interrupt task priority (0 to enable all)
		i32u Rsv04:28;				// 0.4
	};
} CPUR_CR8;

// GDT Global Descriptor Table Entry
typedef struct {
	i16u limit0;			// 0
	i16u base0;				// 2
	i8u base16;				// 4
	i8u type:5;				// 5.0
	i8u dpl:2;				// 5.5 privilege
	i8u p:1;				// 5.7 47 present
	i8u limit16:4;			// 6.0
	i8u avl:1;				// 6.4 available (ros sys)
	i8u siz:2;				// 6.5 default size: 0=16 bits, 2=32 bits, 1=64 bits
	i8u g:1;				// 6.7 granularity
	i8u base24;				// 7 
} CPU_GDTE;					// 8 bytes

// IDT Interrupt Descriptor Table Entry
typedef struct {
	i16u offset0;			// 0
	i16u sel;				// 2 segment selector
	i8u	rsv20;				// 4
	i8u type:2;				// 5.0 type: 1=task, 2=interrupt, 3=trap
	i8u rsv2a:1;			// 5.2 1
	i8u siz:1;				// 5.3 size: 0=16, 1=32
	i8u rsv2c:1;			// 5.4 0
	i8u dpl:2;				// 5.5 privilege
	i8u p:1;				// 5.7 present
	i16u offset16;			// 6
} CPU_IDTE;					// 8 bytes

// descriptor table reg (gdtr / idtr)
typedef struct {
	i16u rsv00;
	i16u limit;
	i32u base;
} CPUR_DT;

typedef struct {
	i32u eax;
	i32u ecx;
	i32u edx;
	i32u ebx;
	i32u esp;
	i32u ebp;
	i32u esi;
	i32u edi;
} CPUR_GP;

typedef struct {
	i16u cs;
	i16u ss;
	i16u ds;
	i16u es;
	i16u fs;
	i16u gs;
} CPUR_SEG;

typedef struct {
	i32u cr0;
	i32u cr2;			// page fault linear address
	i32u cr3;			// cr3 page directory
	i32u cr4;
	i32u cr8;
	CPUR_DT gdtr;		// global descriptor table
	CPUR_DT idtr;		// interrupt descriptor table
	i16u ldtr;			// local descriptor table
	i16u tr;			// task
} CPUR_C;

typedef struct {
	i32u dr0;
	i32u dr1;
	i32u dr2;
	i32u dr3;
	i32u dr4;
	i32u dr5;
	i32u dr6;
	i32u dr7;
	i32u dr8;
} CPUR_DBG;

typedef struct {
	CPUR_F f;
	CPUR_GP gp;
	CPUR_SEG seg;
	CPUR_C c;
} CPUR_ALL;

// task state segment
// must be aligned to 0x80, no page boundary allowed
typedef struct {
	i16u PrevTask;			// 00
	i16u rsv02;				// 02
	i32u esp0;				// 04
	i16u ss0;				// 08
	i16u rsv0a;				// 0a
	i32u esp1;				// 0c
	i16u ss1;				// 10
	i16u rsv12;				// 12
	i32u esp2;				// 14
	i16u ss2;				// 18
	i16u rsv1e;				// 1a
	i32u cr3;				// 1c
	i32u eip;				// 20
	i32u eflags;			// 24
	i32u eax;				// 28
	i32u ecx;				// 2c
	i32u edx;				// 30
	i32u ebx;				// 34
	i32u esp;				// 38
	i32u ebp;				// 3c
	i32u esi;				// 40
	i32u edi;				// 44
	i16u es;				// 48
	i16u rsv4a;				// 4a
	i16u cs;				// 4c
	i16u rsv4e;				// 4e
	i16u ss;				// 50
	i16u rsv52;				// 52
	i16u ds;				// 54
	i16u rsv56;				// 56
	i16u fs;				// 58
	i16u rsv5a;				// 5a
	i16u gs;				// 5c
	i16u rsv5e;				// 5e
	i16u ldtr;				// 60
	i16u rsv62;				// 62
	i16u DbgTrap;			// 64
	i16u IoMap;				// 66
} CPU_TSS_, PCPU_TSS;		// 68
typedef _ALIGN(0x80) CPU_TSS_ CPU_TSS;

#pragma pack(pop)

void _CDECL CpuRegSave(CPUR_ALL *p, i32 flags);
#define CPUREGSAVE_GP		1	// always saved anyway
#define CPUREGSAVE_SEG		2
#define CPUREGSAVE_C		4
#define CPUREGSAVE_FPU		8	// unimplemented

