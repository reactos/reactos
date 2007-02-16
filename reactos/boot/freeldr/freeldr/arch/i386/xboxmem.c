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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Note: much of this code was based on knowledge and/or code developed
 * by the Xbox Linux group: http://www.xbox-linux.org
 */

#include <freeldr.h>

static ULONG InstalledMemoryMb = 0;
static ULONG AvailableMemoryMb = 0;

#define TEST_SIZE     0x200
#define TEST_PATTERN1 0xaa
#define TEST_PATTERN2 0x55

VOID
XboxMemInit(VOID)
{
  UCHAR ControlRegion[TEST_SIZE];
  PVOID MembaseTop = (PVOID)(64 * 1024 * 1024);
  PVOID MembaseLow = (PVOID)0;

  (*(PULONG)(0xfd000000 + 0x100200)) = 0x03070103 ;
  (*(PULONG)(0xfd000000 + 0x100204)) = 0x11448000 ;

  WRITE_PORT_ULONG((ULONG*) 0xcf8, CONFIG_CMD(0, 0, 0x84));
  WRITE_PORT_ULONG((ULONG*) 0xcfc, 0x7ffffff);             /* Prep hardware for 128 Mb */

  InstalledMemoryMb = 64;
  memset(ControlRegion, TEST_PATTERN1, TEST_SIZE);
  memset(MembaseTop, TEST_PATTERN1, TEST_SIZE);
  __asm__ ("wbinvd\n");

  if (0 == memcmp(MembaseTop, ControlRegion, TEST_SIZE))
    {
      /* Looks like there is memory .. maybe a 128MB box */
      memset(ControlRegion, TEST_PATTERN2, TEST_SIZE);
      memset(MembaseTop, TEST_PATTERN2, TEST_SIZE);
      __asm__ ("wbinvd\n");
      if (0 == memcmp(MembaseTop, ControlRegion, TEST_SIZE))
        {
          /* Definitely looks like there is memory */
          if (0 == memcmp(MembaseLow, ControlRegion, TEST_SIZE))
            {
              /* Hell, we find the Test-string at 0x0 too ! */
              InstalledMemoryMb = 64;
            }
          else
            {
              InstalledMemoryMb = 128;
            }
        }
    }

  /* Set hardware for amount of memory detected */
  WRITE_PORT_ULONG((ULONG*) 0xcf8, CONFIG_CMD(0, 0, 0x84));
  WRITE_PORT_ULONG((ULONG*) 0xcfc, InstalledMemoryMb * 1024 * 1024 - 1);

  AvailableMemoryMb = InstalledMemoryMb;
}

ULONG
XboxMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize)
{
  ULONG EntryCount = 0;

  /* Synthesize memory map */
  if (1 <= MaxMemoryMapSize)
    {
      /* Available RAM block */
      BiosMemoryMap[0].BaseAddress = 0;
      BiosMemoryMap[0].Length = AvailableMemoryMb * 1024 * 1024;
      BiosMemoryMap[0].Type = BiosMemoryUsable;
      EntryCount = 1;
    }

  if (2 <= MaxMemoryMapSize)
    {
      /* Video memory */
      BiosMemoryMap[1].BaseAddress = AvailableMemoryMb * 1024 * 1024;
      BiosMemoryMap[1].Length = (InstalledMemoryMb - AvailableMemoryMb) * 1024 * 1024;
      BiosMemoryMap[1].Type = BiosMemoryReserved;
      EntryCount = 2;
    }

  return EntryCount;
}

PVOID
XboxMemReserveMemory(ULONG MbToReserve)
{
  if (0 == InstalledMemoryMb)
    {
      /* Hmm, seems we're not initialized yet */
      XboxMemInit();
    }

  if (AvailableMemoryMb < MbToReserve)
    {
      /* Can't satisfy the request */
      return NULL;
    }

  AvailableMemoryMb -= MbToReserve;

  /* Top of available memory points to the space just reserved */
  return (PVOID) (AvailableMemoryMb * 1024 * 1024);
}

/* EOF */
