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
#include <page.h>
#include <pgtable.h>

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

start_info_t *HYPERVISOR_start_info;
extern char shared_info[PAGE_SIZE];

/* _start is the default name ld will use as the entry point.  When xen
 * loads the domain, it will start execution at the elf entry point.  */

void _start()
{
  pgd_t *pgd;
  int ptetab_ma;
  int idx;
  int pte_ma;
  mmu_update_t req;
  control_if_t *CtrlIf;

  /*
   * Grab start_info
   */
  /* The linux build setup_guest() put a start_info_t* into %esi.
   * =S is inline asm code for get output from reg %esi.
   */
  asm("":"=S" (HYPERVISOR_start_info));

  /* To write to the xen virtual console, we need to map in the
   * shared page used by the the domain controller interface.  The
   * HYPERVISOR_start_info struct identifies the page table and
   * shared_info pages.
   *
   * The following code maps the shared_info mfn (machine frame number)
   * into this domains address space over the shared_info[] page.
   */


  /*
   * map shared_info page
   */
  /* The pgd page (page global directory - level 2 page table) is
   * constructed by setup_guest() in tools/libxc/xc_linux_build.c
   * Lookup the machine address of ptetab in pgd to construct the
   * machine address of the pte entry for shared_info,
   * and then call mmu_update to change mapping.
   */
  pgd = (pgd_t*)HYPERVISOR_start_info->pt_base;
  ptetab_ma = pgd_val(pgd[pgd_index((unsigned long)shared_info)])
              & (PAGE_MASK);
  idx = pte_index((unsigned long)shared_info);
  pte_ma = ptetab_ma + (idx*sizeof(pte_t));

  req.ptr  = pte_ma;
  req.val  = HYPERVISOR_start_info->shared_info|7;
  HYPERVISOR_mmu_update(&req, 1, NULL);

  /*
   * Setup control interface
   */
  XenCtrlIfEvtchn = HYPERVISOR_start_info->domain_controller_evtchn;
  CtrlIf = ((control_if_t *)((char *)shared_info + 2048));

  /* Sync up with shared indexes. */
  FRONT_RING_ATTACH(&XenCtrlIfTxRing, &CtrlIf->tx_ring);
  BACK_RING_ATTACH(&XenCtrlIfRxRing, &CtrlIf->rx_ring);

  /* Start freeldr */
  XenActive = TRUE;
  BootMain(NULL);
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

ULONG
XenMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize)
  {
    XEN_UNIMPLEMENTED("XenMemGetMemoryMap");
    return 0;
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

/* Create shared_info page.  This page is mapped over by the real shared
 * info page
 */
asm(".align 0x1000; shared_info:;.skip 0x1000;");


/* emit the elf segment Xen builder expects in kernel image */ asm(".section __xen_guest;"
    ".ascii \"GUEST_OS=linux,GUEST_VER=2.6,XEN_VER=3.0,VIRT_BASE=0x00008000\";"
    ".ascii \",LOADER=generic\";"
    ".ascii \",PT_MODE_WRITABLE\";"
    ".byte  0;"
    );

/* EOF */
