#ifndef PPC_JMPBUF_H
#define PPC_JMPBUF_H

/* Derived from ppc grub */
typedef struct sigjmp_buf {
    unsigned long r1;
    unsigned long locals[32-14];
    unsigned long LR, CR;
} sigjmp_buf;

#endif/*PPC_JMPBUF_H*/
