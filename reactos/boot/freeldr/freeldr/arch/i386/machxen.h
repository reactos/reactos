/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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
 */

#ifndef __I386_MACHXEN_H_
#define __I386_MACHXEN_H_

#include "i386mem.h"
#include "mm.h"

#include <rosxen.h>
#include <xen.h>
#include <ctrl_if.h>
#include <io/domain_controller.h>

extern BOOL XenActive;
extern start_info_t *XenStartInfo;
extern shared_info_t *XenSharedInfo;

VOID XenMachInit(char *CmdLine);

VOID XenCtrlIfInit();
BOOL XenCtrlIfSendMessageNoblock(ctrl_msg_t *Msg);
VOID XenCtrlIfSendMessageBlock(ctrl_msg_t *Msg);
VOID XenCtrlIfDiscardResponses();
BOOL XenCtrlIfTransmitterEmpty();
VOID XenCtrlIfRegisterReceiver(u8 Type, ctrl_msg_handler_t Hnd);
VOID XenCtrlIfHandleEvent();
VOID XenCtrlIfSendResponse(ctrl_msg_t *Msg);

VOID XenEvtchnRegisterDisk(unsigned DiskEvtchn);
VOID XenEvtchnRegisterCtrlIf(unsigned CtrlIfEvtchn);
VOID XenEvtchnDisableEvents();
VOID XenEvtchnEnableEvents();

VOID XenConsInit();
VOID XenConsPutChar(int Ch);
BOOL XenConsKbHit();
int XenConsGetCh();
VOID XenConsFlush();
VOID XenConsFlushWait();

VOID XenVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE XenVideoSetDisplayMode(char *DisplayMode, BOOL Init);
VOID XenVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG XenVideoGetBufferSize(VOID);
VOID XenVideoSetTextCursorPosition(ULONG X, ULONG Y);
VOID XenVideoHideShowTextCursor(BOOL Show);
VOID XenVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID XenVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOL XenVideoIsPaletteFixed(VOID);
VOID XenVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID XenVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID XenVideoSync(VOID);

ULONG XenMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);
VOID XenMemInit(start_info_t *StartInfo);
u32 XenMemVirtualToMachine(void *VirtualAddress);
int XenMemGrantForeignAccess(domid_t DomId, void *VirtAddr, BOOL ReadOnly);
VOID XenMemInstallPageDir(PPAGE_DIRECTORY_X86 NewPageDir);

BOOL XenDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOL XenDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOL XenDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
ULONG XenDiskGetCacheableBlockCount(ULONG DriveNumber);
VOID XenDiskHandleEvent();

VOID XenRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

VOID XenHwDetect(VOID);

VOID XenBootReactOS(VOID);

VOID XenDie(VOID);

VOID XenHypervisorCallback(VOID);
VOID XenFailsafeCallback(VOID);

#endif /* __I386_MACHXEN_H_ */

/* EOF */
