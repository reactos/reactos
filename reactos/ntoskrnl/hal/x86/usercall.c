/* $Id: usercall.c,v 1.5 1999/11/02 08:55:39 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/usercall.c
 * PURPOSE:         2E interrupt handler
 * PROGRAMMER:      ???
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

extern SERVICE_TABLE _SystemServiceTable[];

/* The service dispatcher will take the service number passed in
 * by the user mode process, logical and it with ServiceNumberMask
 * and compare the resulting value with ServiceNumberValue.  If the
 * value matches, The passed service number will be and'ed with the
 * inverse of ServiceNumberMask to obtain the index into the ServiceTable
 * for the service to call
 */
typedef struct _HAL_DISPATCH_TABLE_ENTRY
{
  DWORD  ServiceNumberMask;
  DWORD  ServiceNumberValue;
  PSERVICE_TABLE  ServiceTable;
  DWORD  TableCount;
} HAL_DISPATCH_TABLE_ENTRY, *PHAL_DISPATCH_TABLE_ENTRY;

static KSPIN_LOCK DispatchTableLock = {0,};
static DWORD DispatchTableCount = 0;
static HAL_DISPATCH_TABLE_ENTRY DispatchTables[16];

NTSTATUS HalRegisterServiceTable(DWORD  Mask, 
                                 DWORD  Value, 
                                 PSERVICE_TABLE  Table,
                                 DWORD  Count)
{
  NTSTATUS  Status;
  KIRQL  OldLvl;
  
  KeAcquireSpinLock(&DispatchTableLock, &OldLvl);

  Status = STATUS_SUCCESS;

  /* FIXME: should check for invalid/overlapping service tables  */
  DispatchTables[DispatchTableCount].ServiceNumberMask = Mask;
  DispatchTables[DispatchTableCount].ServiceNumberValue = Value;
  DispatchTables[DispatchTableCount].ServiceTable = Table;
  DispatchTables[DispatchTableCount].TableCount = Count;
  DispatchTableCount++;
  
  KeReleaseSpinLock(&DispatchTableLock, OldLvl);

  return  Status;
}

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

VOID KiSystemCallHook(ULONG Nr)
{
//   DbgPrint("KiSystemCallHook(Nr %d) %d\n", Nr, KeGetCurrentIrql());
//   DbgPrint("SystemCall %x\n", _SystemServiceTable[Nr].Function);
   assert_irql(PASSIVE_LEVEL);
}

void interrupt_handler2e(void);
   __asm__("\n\t.global _interrupt_handler2e\n\t"
           "_interrupt_handler2e:\n\t"
           
           /*  Save the users context  */
           "pushl %ds\n\t"
           "pushl %es\n\t"
           "pushl %esi\n\t"
           "pushl %edi\n\t"
           "pushl %ebp\n\t"
           "pushl %ebx\n\t"
           
           /*  Set ES to kernel segment  */
           "movw  $"STR(KERNEL_DS)",%bx\n\t"
           "movw %bx,%es\n\t"
           
           /*  Allocate new Kernel stack frame  */
           "movl %esp,%ebp\n\t"
           
           /*  Users's current stack frame pointer is source  */
           "movl %edx,%esi\n\t"

           /* FIXME: determine system service table to use  */
           /* FIXME: chech to see if SS is valid/inrange  */
           
           /*  Allocate room for argument list from kernel stack  */
           "movl %es:__SystemServiceTable(,%eax,8),%ecx\n\t"
           "subl %ecx,%esp\n\t"
           
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
           "movl %ds:__SystemServiceTable+4(,%eax,8),%eax\n\t"
           "call *%eax\n\t"
           
           /*  Deallocate the kernel stack frame  */
           "movl %ebp,%esp\n\t"
           
           /*  Restore the user context  */
           "popl %ebx\n\t"
           "popl %ebp\n\t"
           "popl %edi\n\t"
           "popl %esi\n\t"
           "popl %es\n\t"
           "popl %ds\n\t"
           "iret\n\t");


