/*
 * 
 */

#ifndef __INCLUDE_DDK_I386_TSS_H
#define __INCLUDE_DDK_I386_TSS_H

#define KTSS_ESP0 (0x4)

#ifndef __ASM__

typedef struct _KTSS
{
  USHORT PreviousTask;
  USHORT Reserved1;
  ULONG  Esp0;
  USHORT Ss0;
  USHORT Reserved2;
  ULONG  Esp1;
  USHORT Ss1;
  USHORT Reserved3;
  ULONG  Esp2;
  USHORT Ss2;
  USHORT Reserved4;
  ULONG  Cr3;
  ULONG  Eip;
  ULONG  Eflags;
  ULONG  Eax;
  ULONG  Ecx;
  ULONG  Edx;
  ULONG  Ebx;
  ULONG  Esp;
  ULONG  Ebp;
  ULONG  Esi;
  ULONG  Edi;
  USHORT Es;
  USHORT Reserved5;
  USHORT Cs;
  USHORT Reserved6;
  USHORT Ss;
  USHORT Reserved7;
  USHORT Ds;
  USHORT Reserved8;
  USHORT Fs;
  USHORT Reserved9;
  USHORT Gs;
  USHORT Reserved10;
  USHORT Ldt;
  USHORT Reserved11;
  USHORT Trap;
  USHORT IoMapBase;
  UCHAR  IoBitmap[1];
} KTSS __attribute__((packed));

#endif /* not __ASM__ */

#endif /* __INCLUDE_DDK_I386_TSS_H */
