/* $Id: usercall.c,v 1.8 2000/02/22 20:50:07 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/usercall.c
 * PURPOSE:         2E interrupt handler
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 * UPDATE HISTORY:
 *                  ???
 */

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/symbol.h>
#include <internal/i386/segment.h>
#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>
#include <internal/service.h>

#include <ddk/defines.h>


extern KE_SERVICE_DESCRIPTOR_TABLE_ENTRY KeServiceDescriptorTable[];
extern KE_SERVICE_DESCRIPTOR_TABLE_ENTRY KeServiceDescriptorTableShadow[];


#define _STR(x) #x
#define STR(x) _STR(x)

void PsBeginThreadWithContextInternal(void);
   __asm__(
     "\n\t.global _PsBeginThreadWithContextInternal\n\t"
     "_PsBeginThreadWithContextInternal:\n\t"
//     "pushl $1\n\t"
//     "call _KeLowerIrql\n\t"
     "call _PiBeforeBeginThread\n\t"
//     "popl %eax\n\t"
     "popl %eax\n\t"
     "popl %eax\n\t"
     "popl %eax\n\t"
     "popl %eax\n\t"
     "popl %eax\n\t"
     "popl %eax\n\t"
     "popl %eax\n\t"
     "addl $112,%esp\n\t"
     "popl %gs\n\t"
     "popl %fs\n\t"
     "popl %es\n\t"
     "popl %ds\n\t"
     "popl %edi\n\t"
     "popl %esi\n\t"
     "popl %ebx\n\t"
     "popl %edx\n\t"
     "popl %ecx\n\t"
     "popl %eax\n\t"
     "popl %ebp\n\t"
     "iret\n\t");

VOID KiSystemCallHook(ULONG Nr, ...)
{
#ifdef TRACE_SYSTEM_CALLS
   va_list ap;
   ULONG i;

   va_start(ap, Nr);

   DbgPrint("%x/%d ", _SystemServiceTable[Nr].Function,Nr);
   DbgPrint("%x (", _SystemServiceTable[Nr].ParametersSize);
   for (i = 0; i < _SystemServiceTable[Nr].ParametersSize / 4; i++)
     {
	DbgPrint("%x, ", va_arg(ap, ULONG));
     }
   DbgPrint(")\n");
   assert_irql(PASSIVE_LEVEL);
   va_end(ap);
#endif
}

ULONG KiAfterSystemCallHook(ULONG NtStatus, PCONTEXT Context)
{
   if (NtStatus != STATUS_USER_APC)
     {
	return(NtStatus);
     }
   KiTestAlert(KeGetCurrentThread(), Context);
   return(NtStatus);
}

// This function should be used by win32k.sys to add its own user32/gdi32 services
// TableIndex is 0 based
// ServiceCountTable its not used at the moment
BOOLEAN
KeAddSystemServiceTable (
	PSSDT	SSDT,
	PULONG	ServiceCounterTable,
	ULONG	NumberOfServices,
	PSSPT	SSPT,
	ULONG	TableIndex
	)
{
    if (TableIndex > SSDT_MAX_ENTRIES - 1)
        return FALSE;

    /* check if descriptor table entry is free */
    if ((KeServiceDescriptorTable[TableIndex].SSDT != NULL) ||
        (KeServiceDescriptorTableShadow[TableIndex].SSDT != NULL))
        return FALSE;

    /* initialize the shadow service descriptor table */
    KeServiceDescriptorTableShadow[TableIndex].SSDT = SSDT;
    KeServiceDescriptorTableShadow[TableIndex].SSPT = SSPT;
    KeServiceDescriptorTableShadow[TableIndex].NumberOfServices = NumberOfServices;
    KeServiceDescriptorTableShadow[TableIndex].ServiceCounterTable = ServiceCounterTable;

    /* initialize the service descriptor table (not for win32k services) */
    if (TableIndex != 1)
    {
        KeServiceDescriptorTable[TableIndex].SSDT = SSDT;
        KeServiceDescriptorTable[TableIndex].SSPT = SSPT;
        KeServiceDescriptorTable[TableIndex].NumberOfServices = NumberOfServices;
        KeServiceDescriptorTable[TableIndex].ServiceCounterTable = ServiceCounterTable;
    }

    return TRUE;
}


void interrupt_handler2e(void);
   __asm__("\n\t.global _interrupt_handler2e\n\t"
           "_interrupt_handler2e:\n\t"

	   /* Save the user context */
	   "pushl %ebp\n\t"       /* Ebp */

	   "pushl %eax\n\t"       /* Eax */
	   "pushl %ecx\n\t"       /* Ecx */
	   "pushl %edx\n\t"       /* Edx */
	   "pushl %ebx\n\t"       /* Ebx */
	   "pushl %esi\n\t"       /* Esi */
	   "pushl %edi\n\t"       /* Edi */

	   "pushl %ds\n\t"        /* SegDs */
	   "pushl %es\n\t"        /* SegEs */
	   "pushl %fs\n\t"        /* SegFs */
	   "pushl %gs\n\t"        /* SegGs */

	   "subl $112,%esp\n\t"   /* FloatSave */

	   "pushl $0\n\t"         /* Dr7 */
	   "pushl $0\n\t"         /* Dr6 */
	   "pushl $0\n\t"         /* Dr3 */
	   "pushl $0\n\t"         /* Dr2 */
	   "pushl $0\n\t"         /* Dr1 */
	   "pushl $0\n\t"         /* Dr0 */

	   "pushl $0\n\t"         /* ContextFlags */

           /*  Set ES to kernel segment  */
           "movw  $"STR(KERNEL_DS)",%bx\n\t"
           "movw %bx,%es\n\t"

	   /* Save pointer to user context as argument to system call */
	   "pushl %esp\n\t"

           /*  Allocate new Kernel stack frame  */
           "movl %esp,%ebp\n\t"

           /*  Users's current stack frame pointer is source  */
           "movl %edx,%esi\n\t"

           /*  Determine system service table to use  */
           "cmpl  $0x0fff, %eax\n\t"
           "ja    useShadowTable\n\t"

           /*  Check to see if EAX is valid/inrange  */
           "cmpl  %es:_KeServiceDescriptorTable + 8, %eax\n\t"
           "jbe   serviceInRange\n\t"
           "movl  $"STR(STATUS_INVALID_SYSTEM_SERVICE)", %eax\n\t"
           "jmp   done\n\t"

           "serviceInRange:\n\t"

           /*  Allocate room for argument list from kernel stack  */
           "movl  %es:_KeServiceDescriptorTable + 12, %ecx\n\t"
           "movl  %es:(%ecx, %eax, 4), %ecx\n\t"
           "subl  %ecx, %esp\n\t"

           /*  Copy the arguments from the user stack to the kernel stack  */
           "movl %esp,%edi\n\t"
           "rep\n\tmovsb\n\t"

           /*  DS is now also kernel segment  */
           "movw %bx, %ds\n\t"

	   /* Call system call hook */
	   "pushl %eax\n\t"
	   "call _KiSystemCallHook\n\t"
	   "popl %eax\n\t"

           /*  Make the system service call  */
           "movl  %es:_KeServiceDescriptorTable, %ecx\n\t"
           "movl  %es:(%ecx, %eax, 4), %eax\n\t"
           "call  *%eax\n\t"

#if CHECKED
           /*  Bump Service Counter  */
#endif

           /*  Deallocate the kernel stack frame  */
           "movl %ebp,%esp\n\t"

	   /* Call the post system call hook and deliver any pending APCs */
	   "pushl %eax\n\t"
	   "call _KiAfterSystemCallHook\n\t"
	   "addl $8,%esp\n\t"

           "jmp  done\n\t"

           "useShadowTable:\n\t"

           "subl  $0x1000, %eax\n\t"

           /*  Check to see if EAX is valid/inrange  */
           "cmpl  %es:_KeServiceDescriptorTableShadow + 24, %eax\n\t"
           "jbe   shadowServiceInRange\n\t"
           "movl  $"STR(STATUS_INVALID_SYSTEM_SERVICE)", %eax\n\t"
           "jmp   done\n\t"

           "shadowServiceInRange:\n\t"

           /*  Allocate room for argument list from kernel stack  */
           "movl  %es:_KeServiceDescriptorTableShadow + 28, %ecx\n\t"
           "movl  %es:(%ecx, %eax, 4), %ecx\n\t"
           "subl  %ecx, %esp\n\t"

           /*  Copy the arguments from the user stack to the kernel stack  */
           "movl %esp,%edi\n\t"
           "rep\n\tmovsb\n\t"

           /*  DS is now also kernel segment  */
           "movw %bx,%ds\n\t"

	   /* Call system call hook */
	   "pushl %eax\n\t"
	   "call _KiSystemCallHook\n\t"
	   "popl %eax\n\t"

           /*  Make the system service call  */
           "movl  %es:_KeServiceDescriptorTableShadow + 16, %ecx\n\t"
           "movl  %es:(%ecx, %eax, 4), %eax\n\t"
           "call  *%eax\n\t"

#if CHECKED
           /*  Bump Service Counter  */
#endif

           /*  Deallocate the kernel stack frame  */
           "movl %ebp,%esp\n\t"

	   /* Call the post system call hook and deliver any pending APCs */
	   "pushl %eax\n\t"
	   "call _KiAfterSystemCallHook\n\t"
	   "addl $8,%esp\n\t"

           "done:\n\t"

           /*  Restore the user context  */
	   "addl $4,%esp\n\t"    /* UserContext */
	   "addl $24,%esp\n\t"   /* Dr[0-3,6-7] */
	   "addl $112,%esp\n\t"  /* FloatingSave */
	   "popl %gs\n\t"        /* SegGs */
	   "popl %fs\n\t"        /* SegFs */
	   "popl %es\n\t"        /* SegEs */
	   "popl %ds\n\t"        /* SegDs */

	   "popl %edi\n\t"       /* Edi */
	   "popl %esi\n\t"       /* Esi */
	   "popl %ebx\n\t"       /* Ebx */
	   "popl %edx\n\t"       /* Edx */
	   "popl %ecx\n\t"       /* Ecx */
	   "addl $4,%esp\n\t"       /* Eax (Not restored) */

	   "popl %ebp\n\t"       /* Ebp */

           "iret\n\t");
