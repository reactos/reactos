/*
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
 *  Based on code posted by David Becker <becker@cs.duke.edu> to the
 *  xen-devel mailing list.
 */

#include "freeldr.h"
#include "machxen.h"
#include "i386.h"

#include <rosxen.h>
#include <xen.h>
#include <hypervisor.h>

BOOL XenActive = FALSE;

static trap_info_t trap_table[] = {
        {  0, 0, FLAT_RING1_CS, (unsigned long)i386DivideByZero           },
        {  1, 0, FLAT_RING1_CS, (unsigned long)i386DebugException         },
        {  2, 0, FLAT_RING1_CS, (unsigned long)i386NMIException           },
        {  3, 3, FLAT_RING1_CS, (unsigned long)i386Breakpoint             },
        {  4, 3, FLAT_RING1_CS, (unsigned long)i386Overflow               },
        {  5, 3, FLAT_RING1_CS, (unsigned long)i386BoundException         },
        {  6, 0, FLAT_RING1_CS, (unsigned long)i386InvalidOpcode          },
        {  7, 0, FLAT_RING1_CS, (unsigned long)i386FPUNotAvailable        },
        {  8, 0, FLAT_RING1_CS, (unsigned long)i386DoubleFault            },
        {  9, 0, FLAT_RING1_CS, (unsigned long)i386CoprocessorSegment     },
        { 10, 0, FLAT_RING1_CS, (unsigned long)i386InvalidTSS             },
        { 11, 0, FLAT_RING1_CS, (unsigned long)i386SegmentNotPresent      },
        { 12, 0, FLAT_RING1_CS, (unsigned long)i386StackException         },
        { 13, 0, FLAT_RING1_CS, (unsigned long)i386GeneralProtectionFault },
        { 14, 0, FLAT_RING1_CS, (unsigned long)i386PageFault              },
        { 16, 0, FLAT_RING1_CS, (unsigned long)i386CoprocessorError       },
        { 17, 0, FLAT_RING1_CS, (unsigned long)i386AlignmentCheck         },
        { 18, 0, FLAT_RING1_CS, (unsigned long)i386MachineCheck           },
        {  0, 0,             0, 0                                         }
};

static void
XenTrapSaveDR(unsigned long *DRRegs)
{
  unsigned Reg;

  for (Reg = 0; Reg < 8; Reg++)
    {
      DRRegs[Reg] = HYPERVISOR_get_debugreg(Reg);
    }
}

static void
XenShutdownHandler(ctrl_msg_t *Msg, unsigned long Id)
{
  XenCtrlIfSendResponse(Msg);

  /* FIXME we don't do suspend/resume yet */
  if (CMSG_SHUTDOWN_SUSPEND == Msg->subtype)
    {
      return;
    }

  XenConsFlushWait();
  while (1)
    {
      switch(Msg->subtype)
        {
        case CMSG_SHUTDOWN_REBOOT:
          HYPERVISOR_reboot();
          break;
        case CMSG_SHUTDOWN_POWEROFF:
        default:
          HYPERVISOR_shutdown();
          break;
        }
    }
}

VOID
XenMachInit(char *CmdLine)
{
  /* Setup vtbl */
  MachVtbl.ConsPutChar = XenConsPutChar;
  MachVtbl.ConsKbHit = XenConsKbHit;
  MachVtbl.ConsGetCh = XenConsGetCh;
  MachVtbl.VideoClearScreen = XenVideoClearScreen;
  MachVtbl.VideoSetDisplayMode = XenVideoSetDisplayMode;
  MachVtbl.VideoGetDisplaySize = XenVideoGetDisplaySize;
  MachVtbl.VideoGetBufferSize = XenVideoGetBufferSize;
  MachVtbl.VideoSetTextCursorPosition = XenVideoSetTextCursorPosition;
  MachVtbl.VideoSetTextCursorPosition = XenVideoSetTextCursorPosition;
  MachVtbl.VideoHideShowTextCursor = XenVideoHideShowTextCursor;
  MachVtbl.VideoPutChar = XenVideoPutChar;
  MachVtbl.VideoCopyOffScreenBufferToVRAM = XenVideoCopyOffScreenBufferToVRAM;
  MachVtbl.VideoIsPaletteFixed = XenVideoIsPaletteFixed;
  MachVtbl.VideoSetPaletteColor = XenVideoSetPaletteColor;
  MachVtbl.VideoGetPaletteColor = XenVideoGetPaletteColor;
  MachVtbl.VideoSync = XenVideoSync;
  MachVtbl.GetMemoryMap = XenMemGetMemoryMap;
  MachVtbl.DiskGetBootVolume = i386DiskGetBootVolume;
  MachVtbl.DiskGetSystemVolume = i386DiskGetSystemVolume;
  MachVtbl.DiskGetBootPath = i386DiskGetBootPath;
  MachVtbl.DiskGetBootDevice = i386DiskGetBootDevice;
  MachVtbl.DiskBootingFromFloppy = i386DiskBootingFromFloppy;
  MachVtbl.DiskReadLogicalSectors = XenDiskReadLogicalSectors;
  MachVtbl.DiskGetPartitionEntry = XenDiskGetPartitionEntry;
  MachVtbl.DiskGetDriveGeometry = XenDiskGetDriveGeometry;
  MachVtbl.DiskGetCacheableBlockCount = XenDiskGetCacheableBlockCount;
  MachVtbl.RTCGetCurrentDateTime = XenRTCGetCurrentDateTime;
  MachVtbl.HwDetect = XenHwDetect;
  MachVtbl.BootReactOS = XenBootReactOS;
  MachVtbl.Die = XenDie;

  XenCtrlIfInit();

  HYPERVISOR_set_callbacks(FLAT_RING1_CS, (unsigned long) XenHypervisorCallback,
                           FLAT_RING1_CS, (unsigned long) XenFailsafeCallback);

  i386TrapSaveDRHook = XenTrapSaveDR;
  HYPERVISOR_set_trap_table(trap_table);

  XenConsInit();
  XenCtrlIfRegisterReceiver(CMSG_SHUTDOWN, XenShutdownHandler);

  /* Ready to receive events now */
  XenEvtchnEnableEvents();
}

VOID
XenDie()
{
  XenConsFlushWait();
  while (1)
    {
      HYPERVISOR_shutdown();
    }
}

/* _start is the default name ld will use as the entry point.  When xen
 * loads the domain, it will start execution at the elf entry point.  */

void _start()
{
  extern void *start;               /* Where is freeldr loaded? */
  start_info_t *StartInfo;
  void *OldStackPtr, *NewStackPtr;
  void *OldStackPage, *NewStackPage;

  /*
   * Grab start_info
   * 
   * The linux build setup_guest() put a start_info_t* into %esi.
   * =S is inline asm code for get output from reg %esi.
   */
  asm("":"=S" (StartInfo));

  /*
   * Set up memory
   */
  XenMemInit(StartInfo);

  /* Now move the stack to low mem */
  /* Copy the stack page */
  __asm__ __volatile__("mov %%esp,%%eax\n" : "=a" (OldStackPtr));
  OldStackPage = (void *) ROUND_DOWN((unsigned long) OldStackPtr, PAGE_SIZE);
  NewStackPage = (void *)((char *) &start - PAGE_SIZE);
  memcpy(NewStackPage, OldStackPage, PAGE_SIZE);
  /* Switch the stack around. */
  NewStackPtr = (void *)((char *) NewStackPage + (OldStackPtr - OldStackPage));
  __asm__ __volatile__("mov %%eax,%%esp\n" : : "a"(NewStackPtr));
  /* Don't use stack based variables after this */

  /* Start freeldr */
  XenActive = TRUE;
  i386BootDrive = 0x80;
  i386BootPartition = 0xff;
  BootMain(XenStartInfo->cmd_line);

  /* Shouldn't get here */
  XenDie();
}

/* emit the elf segment Xen builder expects in kernel image */
asm(".section __xen_guest;"
    ".ascii \"GUEST_OS=linux,GUEST_VER=2.6\";"
#if XEN_VER == 2
    ".ascii \",XEN_VER=2.0\";"
#else
    ".ascii \",XEN_VER=3.0\";"
#endif
    ".ascii \",VIRT_BASE=0x00008000\";"
    ".ascii \",LOADER=generic\";"
    ".ascii \",PT_MODE_WRITABLE\";"
    ".byte  0;"
    );

/* EOF */
