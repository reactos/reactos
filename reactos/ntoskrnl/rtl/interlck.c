
void InterlockedIncrement(void);
   __asm__("\n\t.global _InterlockedIncrement\n\t"
       "_InterlockedIncrement:\n\t"
       "pushl %ebp\n\t"
       "movl  %esp,%ebp\n\t"
       "pushl %eax\n\t"
       "pushl %ebx\n\t"
       "movl $1,%eax\n\t"
       "movl 8(%ebp),%ebx\n\t"
       "xaddl %eax,(%ebx)\n\t"
       "popl %ebx\n\t"
       "popl %eax\n\t"
       "movl %ebp,%esp\n\t"
       "popl %ebp\n\t"
       "ret\n\t");
       
       
void InterlockedDecrement(void);
   __asm__("\n\t.global _InterlockedDecrement\n\t"
       "_InterlockedDecrement:\n\t"
       "movl $0xffffffff,%eax\n\t"
       "movl 4(%esp),%ebx\n\t"
       "xaddl %eax,(%ebx)\n\t"
       "decl %eax\n\t"
       "ret\n\t");
       
void InterlockedExchange(void);
   __asm__("\n\t.global _InterlockedExchange\n\t"
       "_InterlockedExchange:\n\t"
       "pushl %ebp\n\t"
       "movl  %esp,%ebp\n\t"
       "pushl %eax\n\t"
       "pushl %ebx\n\t"
       "movl  12(%ebp),%eax\n\t"
       "movl  8(%ebp),%ebx\n\t"
       "xchgl %eax,(%ebx)\n\t"
       "popl  %ebx\n\t"
       "popl  %eax\n\t"
       "movl  %ebp,%esp\n\t"
       "popl  %ebp\n\t"
       "ret\n\t");

void InterlockedExchangeAdd(void);
   __asm__("\n\t.global _InterlockedExchangeAdd\n\t"
       "_InterlockedExchangeAdd:\n\t"
       "movl 8(%esp),%eax\n\t"
       "movl 4(%esp),%ebx\n\t"
       "xaddl %eax,(%ebx)\n\t"
       "ret\n\t");

void InterlockedCompareExchange(void);
   __asm__("\n\t.global _InterlockedCompareExchange\n\t"
       "_InterlockedCompareExchange:\n\t"
       "movl 12(%esp),%eax\n\t"
       "movl 8(%esp),%edx\n\t"
       "movl 4(%esp),%ebx\n\t"
       "cmpxchg %edx,(%ebx)\n\t"
       "movl %edx,%eax\n\t"
       "ret\n\t");
