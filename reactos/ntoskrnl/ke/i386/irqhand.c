
#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>


#define _STR(x) #x
#define STR(x) _STR(x)


#define IRQ_HANDLER_FIRST(x,y)  \
      void irq_handler_##y (void);   \
       __asm__("\n\t.global _irq_handler_"##x"\n\t_irq_handler_"##x":\n\t" \
        "pusha\n\t"                     \
        "pushl %ds\n\t"                 \
        "pushl %es\n\t"                 \
        "pushl %fs\n\t"                  \
        "movl  $0xceafbeef,%eax\n\t"      \
        "pushl %eax\n\t"                \
        "movw  $"STR(KERNEL_DS)",%ax\n\t"       \
        "movw  %ax,%ds\n\t"             \
        "movw  %ax,%es\n\t"             \
        "inb   $0x21,%al\n\t"               \
        "orb   $1<<"##x",%al\n\t"       \
        "outb  %al,$0x21\n\t"               \
        "pushl %esp\n\t"                \
        "pushl $"##x"\n\t"              \
        "call  _KiInterruptDispatch\n\t"\
        "popl  %eax\n\t"                \
        "popl  %eax\n\t"                \
        "popl  %eax\n\t"                \
        "popl  %fs\n\t"                 \
        "popl  %es\n\t"                 \
        "popl  %ds\n\t"                 \
        "popa\n\t"                      \
        "iret\n\t")

#define IRQ_HANDLER_SECOND(x,y)  \
      void irq_handler_##y (void);   \
       __asm__("\n\t.global _irq_handler_"##x"\n\t_irq_handler_"##x":\n\t" \
        "pusha\n\t"                     \
        "pushl %ds\n\t"                 \
        "pushl %es\n\t"                 \
        "pushl %fs\n\t"                 \
        "movl  $0xceafbeef,%eax\n\t"      \
        "pushl %eax\n\t"                \
        "movw  $"STR(KERNEL_DS)",%ax\n\t"       \
        "movw  %ax,%ds\n\t"             \
        "movw  %ax,%es\n\t"             \
        "inb   $0xa1,%al\n\t"               \
        "orb   $1<<("##x"-8),%al\n\t"       \
        "outb     %al,$0xa1\n\t"               \
        "pushl %esp\n\t"                \
        "pushl $"##x"\n\t"              \
        "call  _KiInterruptDispatch\n\t"\
        "popl  %eax\n\t"                \
        "popl  %eax\n\t"                \
        "popl  %eax\n\t"                \
        "popl  %fs\n\t"                 \
        "popl  %es\n\t"                 \
        "popl  %ds\n\t"                 \
        "popa\n\t"                      \
        "iret\n\t")


IRQ_HANDLER_FIRST ("0",0);
IRQ_HANDLER_FIRST ("1",1);
IRQ_HANDLER_FIRST ("2",2);
IRQ_HANDLER_FIRST ("3",3);
IRQ_HANDLER_FIRST ("4",4);
IRQ_HANDLER_FIRST ("5",5);
IRQ_HANDLER_FIRST ("6",6);
IRQ_HANDLER_FIRST ("7",7);
IRQ_HANDLER_SECOND ("8",8);
IRQ_HANDLER_SECOND ("9",9);
IRQ_HANDLER_SECOND ("10",10);
IRQ_HANDLER_SECOND ("11",11);
IRQ_HANDLER_SECOND ("12",12);
IRQ_HANDLER_SECOND ("13",13);
IRQ_HANDLER_SECOND ("14",14);
IRQ_HANDLER_SECOND ("15",15);

