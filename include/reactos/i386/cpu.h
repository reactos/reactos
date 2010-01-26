#pragma once

/*	cpu.h
	definitions for intel ia-32 cpu and cpu.lib
	(c) 2010 Jose M. Catena Gomez
*/

#include <cpu_c.h>	// compiler specific

#pragma pack(push, 1)

// GDT Global Descriptor Table Entry
typedef struct
{
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
typedef struct
{
	i16u offset0;			// 0
	i16u sel;				// 2
	i8u	rsv20;				// 4
	i8u type:2;				// 5.0 type: 1=task, 2=interrupt, 3=trap
	i8u rsv2a:1;			// 5.2 1
	i8u siz:1;				// 5.3 size: 0=16, 1=32
	i8u rsv2c:1;			// 5.4 0
	i8u dpl:2;				// 5.5 privilege
	i8u p;					// 5.7 present
	i16u offset16;			// 6
} CPU_IDTE;					// 8 bytes

// descriptor table reg (gdtr / idtr)
typedef struct
{
	i16u rsv00;
	i16u limit;
	i32u base;
} CPUR_DT;

// eflags
typedef struct
{
	union
	{
		i32u f;
		struct
		{
			i32u c:1;			// 0.0 carry
			i32u rsv01:1;		// 0.1 1
			i32u p:1;			// 0.2 parity
			i32u rsv03:1;		// 0.3 0
			i32u a:1;			// 0.4 aux
			i32u rsv05:1;		// 0.5 0
			i32u z:1;			// 0.6 zero
			i32u s:1;			// 0.7 sign
			i32u trap:1;		// 1.0 trap
			i32u ie:1;			// 1.1 interrupt enable
			i32u d:1;			// 1.2 direction
			i32u o:1;			// 1.3 overflow
			i32u iopl:2;		// 1.4 io privilege
			i32u nt:1;			// 1.6 nested task
			i32u rsv0f:1;		// 1.7 0
			i32u r:1;			// 2.0 resume
			i32u vm:1;			// 2.1 virtual 8086 mode
			i32u ac:1;			// 2.2 alignment check
			i32u vif:1;			// 2.3 virtual interrupt flag
			i32u vip:1;			// 2.4 virtual interrupt pending
			i32u id:1;			// 2.5 id
			i32u rsv16:10;		// 2.6 0
		};
	};
} CPUR_F;

typedef struct
{
	i32u eax;
	i32u ecx;
	i32u edx;
	i32u ebx;
	i32u esp;
	i32u ebp;
	i32u esi;
	i32u edi;
} CPUR_GP;

typedef struct
{
	i16u cs;
	i16u ss;
	i16u ds;
	i16u es;
	i16u fs;
	i16u gs;
} CPUR_SEG;

typedef struct
{
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

typedef struct
{
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

typedef struct
{
	CPUR_F f;
	CPUR_GP gp;
	CPUR_SEG seg;
	CPUR_C c;
} CPUR_ALL;

// task state segment
// must be aligned to 0x80, no page boundary allowed
typedef struct _ALIGN(0x80)
{
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
} CPU_TSS;					// 68

#define CPUREGSAVE_GP		1	// always saved anyway
#define CPUREGSAVE_SEG		2
#define CPUREGSAVE_C		4
#define CPUREGSAVE_FPU		8	// unimplemented

#pragma pack(pop)

void _CDECL CpuRegSave(CPUR_ALL *p, i32 flags);
