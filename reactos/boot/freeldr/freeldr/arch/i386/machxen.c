/* $Id: machpc.c 12672 2005-01-01 00:42:18Z chorns $
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
 *  Based on code posted by David Becker <becker@cs.duke.edu> to the
 *  xen-devel mailing list.
 */

#include "freeldr.h"
#include "machxen.h"

#include <rosxen.h>
#include <xen.h>
#include <hypervisor.h>

BOOL XenActive = FALSE;
ctrl_front_ring_t XenCtrlIfTxRing;
ctrl_back_ring_t XenCtrlIfRxRing;
int XenCtrlIfEvtchn;

VOID
XenMachInit(VOID)
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
  MachVtbl.VideoPrepareForReactOS = XenVideoPrepareForReactOS;
  MachVtbl.GetMemoryMap = XenMemGetMemoryMap;
  MachVtbl.DiskReadLogicalSectors = XenDiskReadLogicalSectors;
  MachVtbl.DiskGetPartitionEntry = XenDiskGetPartitionEntry;
  MachVtbl.DiskGetDriveGeometry = XenDiskGetDriveGeometry;
  MachVtbl.DiskGetCacheableBlockCount = XenDiskGetCacheableBlockCount;
  MachVtbl.RTCGetCurrentDateTime = XenRTCGetCurrentDateTime;
  MachVtbl.HwDetect = XenHwDetect;
}

/* _start is the default name ld will use as the entry point.  When xen
 * loads the domain, it will start execution at the elf entry point.  */

void *dummy;
void _start()
{
  extern void *start;               /* Where is freeldr loaded? */
  start_info_t *StartInfo;
  control_if_t *CtrlIf;
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

  /*
   * Setup control interface
   */
  XenCtrlIfEvtchn = XenStartInfo->domain_controller_evtchn;
  CtrlIf = ((control_if_t *)((char *)XenSharedInfo + 2048));

  /* Sync up with shared indexes. */
  FRONT_RING_ATTACH(&XenCtrlIfTxRing, &CtrlIf->tx_ring);
  BACK_RING_ATTACH(&XenCtrlIfRxRing, &CtrlIf->rx_ring);

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
  BootMain(XenStartInfo->cmd_line);

  /* Shouldn't get here */
  HYPERVISOR_shutdown();
}

#define XEN_UNIMPLEMENTED(routine) \
  printf(routine " unimplemented. Shutting down\n"); \
  HYPERVISOR_shutdown()

BOOL
XenConsKbHit()
  {
    XEN_UNIMPLEMENTED("XenConsKbHit");
    return FALSE;
  }

int
XenConsGetCh()
  {
    XEN_UNIMPLEMENTED("XenConsGetCh");
    return 0;
  }

VOID
XenVideoClearScreen(UCHAR Attr)
  {
    XEN_UNIMPLEMENTED("XenVideoClearScreen");
  }

VIDEODISPLAYMODE
XenVideoSetDisplayMode(char *DisplayMode, BOOL Init)
  {
    XEN_UNIMPLEMENTED("XenVideoSetDisplayMode");
    return VideoTextMode;
  }

VOID
XenVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
  {
    XEN_UNIMPLEMENTED("XenVideoGetDisplaySize");
  }

ULONG
XenVideoGetBufferSize(VOID)
  {
    XEN_UNIMPLEMENTED("XenVideoGetBufferSize");
    return 0;
  }

VOID
XenVideoSetTextCursorPosition(ULONG X, ULONG Y)
  {
    XEN_UNIMPLEMENTED("XenVideoSetTextCursorPosition");
  }

VOID
XenVideoHideShowTextCursor(BOOL Show)
  {
    XEN_UNIMPLEMENTED("XenVideoHideShowTextCursor");
  }

VOID
XenVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
  {
    XEN_UNIMPLEMENTED("XenVideoPutChar");
  }

VOID
XenVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
  {
    XEN_UNIMPLEMENTED("XenVideoCopyOffScreenBufferToVRAM");
  }

BOOL
XenVideoIsPaletteFixed(VOID)
  {
    XEN_UNIMPLEMENTED("XenVideoIsPaletteFixed");
    return TRUE;
  }

VOID
XenVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
  {
    XEN_UNIMPLEMENTED("XenVideoSetPaletteColor");
  }

VOID
XenVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue)
  {
    XEN_UNIMPLEMENTED("XenVideoGetPaletteColor");
  }

VOID
XenVideoSync(VOID)
  {
    XEN_UNIMPLEMENTED("XenVideoSync");
  }

VOID
XenVideoPrepareForReactOS(VOID)
  {
    XEN_UNIMPLEMENTED("XenVideoPrepareForReactOS");
  }

BOOL
XenDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber,
                          ULONG SectorCount, PVOID Buffer)
  {
    XEN_UNIMPLEMENTED("XenDiskReadLogicalSectors");
    return FALSE;
  }

BOOL
XenDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber,
                         PPARTITION_TABLE_ENTRY PartitionTableEntry)
  {
    XEN_UNIMPLEMENTED("XenDiskGetPartitionEntry");
    return FALSE;
  }

BOOL
XenDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry)
  {
    XEN_UNIMPLEMENTED("XenDiskGetDriveGeometry");
    return FALSE;
  }

ULONG
XenDiskGetCacheableBlockCount(ULONG DriveNumber)
  {
    XEN_UNIMPLEMENTED("XenDiskGetCacheableBlockCount");
    return FALSE;
  }

VOID
XenRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day,
                         PULONG Hour, PULONG Minute, PULONG Second)
  {
    XEN_UNIMPLEMENTED("XenRTCGetCurrentDateTime");
  }

VOID
XenHwDetect(VOID)
  {
    XEN_UNIMPLEMENTED("XenHwDetect");
  }

/* emit the elf segment Xen builder expects in kernel image */ asm(".section __xen_guest;"
    ".ascii \"GUEST_OS=linux,GUEST_VER=2.6,XEN_VER=3.0,VIRT_BASE=0x00008000\";"
    ".ascii \",LOADER=generic\";"
    ".ascii \",PT_MODE_WRITABLE\";"
    ".byte  0;"
    );

/* EOF */
