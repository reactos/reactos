#include <windows.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/symbol.h>
#include <internal/i386/segment.h>
#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>


#define _STR(x) #x
#define STR(x) _STR(x)

void PsBeginThreadWithContextInternal(void);
   __asm__("\n\t.global _PsBeginThreadWithContextInternal\n\t"
     "_PsBeginThreadWithContextInternal:\n\t"
     "pushl $0\n\t"
     "call _KeLowerIrql\n\t"
     "popl %eax\n\t"
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

void interrupt_handler2e(void);
   __asm__("\n\t.global _interrupt_handler2e\n\t"
     "_interrupt_handler2e:\n\t"
     "pushl %ds\n\t"
     "pushl %es\n\t"
     "pushl %esi\n\t"
     "pushl %edi\n\t"
     "pushl %ebp\n\t"
     "pushl %ebx\n\t"
     "movw  $"STR(KERNEL_DS)",%bx\n\t"
     "movw %bx,%es\n\t"
     "movl %esp,%ebp\n\t"
     "movl %edx,%esi\n\t"
     "movl %es:__SystemServiceTable(,%eax,8),%ecx\n\t"
     "subl %ecx,%esp\n\t"
     "movl %esp,%edi\n\t"
     "rep\n\tmovsb\n\t"
     "movw %bx,%ds\n\t"
     "movl %ds:__SystemServiceTable+4(,%eax,8),%eax\n\t"
     "call %eax\n\t"
     "movl %ebp,%esp\n\t"
     "popl %ebx\n\t"
     "popl %ebp\n\t"
     "popl %edi\n\t"
     "popl %esi\n\t"
     "popl %es\n\t"
     "popl %ds\n\t"
     "iret\n\t");


