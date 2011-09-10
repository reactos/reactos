/* $Id$
 *
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Note: Most of this code comes from the old file "i386mem.c", which
 *       was Copyright (C) 1998-2003 Brian Palmer <brianp@sginet.com>
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(MEMORY);

static ULONG
PcMemGetExtendedMemorySize(VOID)
{
  REGS RegsIn;
  REGS RegsOut;
  ULONG MemorySize;

  TRACE("GetExtendedMemorySize()\n");

  /* Int 15h AX=E801h
   * Phoenix BIOS v4.0 - GET MEMORY SIZE FOR >64M CONFIGURATIONS
   *
   * AX = E801h
   * Return:
   * CF clear if successful
   * AX = extended memory between 1M and 16M, in K (max 3C00h = 15MB)
   * BX = extended memory above 16M, in 64K blocks
   * CX = configured memory 1M to 16M, in K
   * DX = configured memory above 16M, in 64K blocks
   * CF set on error
   */
  RegsIn.w.ax = 0xE801;
  Int386(0x15, &RegsIn, &RegsOut);

  TRACE("Int15h AX=E801h\n");
  TRACE("AX = 0x%x\n", RegsOut.w.ax);
  TRACE("BX = 0x%x\n", RegsOut.w.bx);
  TRACE("CX = 0x%x\n", RegsOut.w.cx);
  TRACE("DX = 0x%x\n", RegsOut.w.dx);
  TRACE("CF set = %s\n\n", (RegsOut.x.eflags & EFLAGS_CF) ? "TRUE" : "FALSE");

  if (INT386_SUCCESS(RegsOut))
    {
      /* If AX=BX=0000h the use CX and DX */
      if (RegsOut.w.ax == 0)
        {
          /* Return extended memory size in K */
          MemorySize = RegsOut.w.dx * 64;
          MemorySize += RegsOut.w.cx;
          return MemorySize;
        }
      else
        {
          /* Return extended memory size in K */
          MemorySize = RegsOut.w.bx * 64;
          MemorySize += RegsOut.w.ax;
          return MemorySize;
        }
    }

  /* If we get here then Int15 Func E801h didn't work */
  /* So try Int15 Func 88h */

  /* Int 15h AH=88h
   * SYSTEM - GET EXTENDED MEMORY SIZE (286+)
   *
   * AH = 88h
   * Return:
   * CF clear if successful
   * AX = number of contiguous KB starting at absolute address 100000h
   * CF set on error
   * AH = status
   * 80h invalid command (PC,PCjr)
   * 86h unsupported function (XT,PS30)
   */
  RegsIn.b.ah = 0x88;
  Int386(0x15, &RegsIn, &RegsOut);

  TRACE("Int15h AH=88h\n");
  TRACE("AX = 0x%x\n", RegsOut.w.ax);
  TRACE("CF set = %s\n\n", (RegsOut.x.eflags & EFLAGS_CF) ? "TRUE" : "FALSE");

  if (INT386_SUCCESS(RegsOut) && RegsOut.w.ax != 0)
    {
      MemorySize = RegsOut.w.ax;
      return MemorySize;
    }

  /* If we get here then Int15 Func 88h didn't work */
  /* So try reading the CMOS */
  WRITE_PORT_UCHAR((PUCHAR)0x70, 0x31);
  MemorySize = READ_PORT_UCHAR((PUCHAR)0x71);
  MemorySize = (MemorySize & 0xFFFF);
  MemorySize = (MemorySize << 8);

  TRACE("Int15h Failed\n");
  TRACE("CMOS reports: 0x%x\n", MemorySize);

  return MemorySize;
}

static ULONG
PcMemGetConventionalMemorySize(VOID)
{
  REGS Regs;

  TRACE("GetConventionalMemorySize()\n");

  /* Int 12h
   * BIOS - GET MEMORY SIZE
   *
   * Return:
   * AX = kilobytes of contiguous memory starting at absolute address 00000h
   *
   * This call returns the contents of the word at 0040h:0013h;
   * in PC and XT, this value is set from the switches on the motherboard
   */
  Regs.w.ax = 0;
  Int386(0x12, &Regs, &Regs);

  TRACE("Int12h\n");
  TRACE("AX = 0x%x\n\n", Regs.w.ax);

  return (ULONG)Regs.w.ax;
}

static ULONG
PcMemGetBiosMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize)
{
  REGS Regs;
  ULONG MapCount;

  TRACE("GetBiosMemoryMap()\n");

  /* Int 15h AX=E820h
   * Newer BIOSes - GET SYSTEM MEMORY MAP
   *
   * AX = E820h
   * EAX = 0000E820h
   * EDX = 534D4150h ('SMAP')
   * EBX = continuation value or 00000000h to start at beginning of map
   * ECX = size of buffer for result, in bytes (should be >= 20 bytes)
   * ES:DI -> buffer for result
   * Return:
   * CF clear if successful
   * EAX = 534D4150h ('SMAP')
   * ES:DI buffer filled
   * EBX = next offset from which to copy or 00000000h if all done
   * ECX = actual length returned in bytes
   * CF set on error
   * AH = error code (86h)
   */
  Regs.x.eax = 0x0000E820;
  Regs.x.edx = 0x534D4150; /* ('SMAP') */
  Regs.x.ebx = 0x00000000;
  Regs.x.ecx = sizeof(BIOS_MEMORY_MAP);
  Regs.w.es = BIOSCALLBUFSEGMENT;
  Regs.w.di = BIOSCALLBUFOFFSET;
  for (MapCount = 0; MapCount < MaxMemoryMapSize; MapCount++)
    {
      Int386(0x15, &Regs, &Regs);

      TRACE("Memory Map Entry %d\n", MapCount);
      TRACE("Int15h AX=E820h\n");
      TRACE("EAX = 0x%x\n", Regs.x.eax);
      TRACE("EBX = 0x%x\n", Regs.x.ebx);
      TRACE("ECX = 0x%x\n", Regs.x.ecx);
      TRACE("CF set = %s\n", (Regs.x.eflags & EFLAGS_CF) ? "TRUE" : "FALSE");

      /* If the BIOS didn't return 'SMAP' in EAX then
       * it doesn't support this call */
      if (Regs.x.eax != 0x534D4150)
        {
          break;
        }

      /* Copy data to caller's buffer */
      RtlCopyMemory(&BiosMemoryMap[MapCount], (PVOID)BIOSCALLBUFFER, Regs.x.ecx);

      TRACE("BaseAddress: 0x%p\n", (PVOID)(ULONG_PTR)BiosMemoryMap[MapCount].BaseAddress);
      TRACE("Length: 0x%p\n", (PVOID)(ULONG_PTR)BiosMemoryMap[MapCount].Length);
      TRACE("Type: 0x%x\n", BiosMemoryMap[MapCount].Type);
      TRACE("Reserved: 0x%x\n", BiosMemoryMap[MapCount].Reserved);
      TRACE("\n");

      /* If the continuation value is zero or the
       * carry flag is set then this was
       * the last entry so we're done */
      if (Regs.x.ebx == 0x00000000 || !INT386_SUCCESS(Regs))
        {
          MapCount++;
          TRACE("End Of System Memory Map!\n\n");
          break;
        }

      /* Setup the registers for the next call */
      Regs.x.eax = 0x0000E820;
      Regs.x.edx = 0x534D4150; /* ('SMAP') */
      /* Regs.x.ebx = 0x00000001;  Continuation value already set by the BIOS */
      Regs.x.ecx = sizeof(BIOS_MEMORY_MAP);
      Regs.w.es = BIOSCALLBUFSEGMENT;
      Regs.w.di = BIOSCALLBUFOFFSET;
    }

  return MapCount;
}

ULONG
PcMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize)
{
  ULONG EntryCount;

  EntryCount = PcMemGetBiosMemoryMap(BiosMemoryMap, MaxMemoryMapSize);

  /* If the BIOS didn't provide a memory map, synthesize one */
  if (0 == EntryCount && 2 <= MaxMemoryMapSize)
    {
      /* Conventional memory */
      BiosMemoryMap[0].BaseAddress = 0;
      BiosMemoryMap[0].Length = PcMemGetConventionalMemorySize() * 1024;
      BiosMemoryMap[0].Type = BiosMemoryUsable;
      /* Extended memory */
      BiosMemoryMap[1].BaseAddress = 1024 * 1024;
      BiosMemoryMap[1].Length = PcMemGetExtendedMemorySize() * 1024;
      BiosMemoryMap[1].Type = BiosMemoryUsable;
      EntryCount = 2;
    }

  return EntryCount;
}

/* EOF */
