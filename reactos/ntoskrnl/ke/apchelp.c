
void KeApcProlog(void);
   __asm__("\n\t.global _KeApcProlog\n\t"
      "_KeApcProlog:\n\t"
      "pusha\n\t"
      "pushl %eax\n\t"
      "call _KeApcProlog2\n\t"
      "popl %eax\n\t"
      "popa\n\t"
      "popl %eax\n\t"
      "iret\n\t");

