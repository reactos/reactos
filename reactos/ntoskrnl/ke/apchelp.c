
void KeApcProlog(void);
   __asm__("\n\t.global _KeApcProlog\n\t"
      "_KeApcProlog:\n\t"
      "pusha\n\t"
      "call _KeApcProlog2\n\t"
      "popa\n\t"
      "iret\n\t");

