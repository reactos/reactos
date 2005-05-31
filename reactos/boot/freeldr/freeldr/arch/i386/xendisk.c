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
 * Based on XenoLinux drivers/xen/blkfront/blkfront.c
 * Copyright (c) 2003-2004, Keir Fraser & Steve Hand
 * Modifications by Mark A. Williamson are (c) Intel Research Cambridge
 * Copyright (c) 2004, Christian Limpach
 * Copyright (c) 2004, Andrew Warfield
 * Copyright (c) 2005, Christopher Clark
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "freeldr.h"
#include "machxen.h"
#include "evtchn.h"
#include "io/blkif.h"
#if 2 != XEN_VER
#include "io/ring.h"
#endif /* XEN_VER */

#define DISK_STATE_CLOSED       0
#define DISK_STATE_DISCONNECTED 1
#define DISK_STATE_CONNECTED    2

typedef struct _XENDISKINFO
{
  ULONGLONG SectorCount;
  ULONG DriveNumber;
  ULONG BytesPerSector;
  blkif_vdev_t XenDevice;
  u16 XenInfo;
} XENDISKINFO, *PXENDISKINFO;

static BOOL XenDiskInitialized = FALSE;
static unsigned XenDiskState = DISK_STATE_CLOSED;
static unsigned XenDiskEvtchn;
#if 2 == XEN_VER
static blkif_ring_t *XenDiskRing = NULL;
static BLKIF_RING_IDX XenDiskRespCons; /* Response consumer for comms ring. */
static BLKIF_RING_IDX XenDiskReqProd;  /* Private request producer.         */
#else
static blkif_front_ring_t XenDiskRing;
#endif /* XEN_VER */
static BOOL XenDiskRspReceived;
static unsigned XenDiskCount;
static PXENDISKINFO XenDiskInfo;

static void *XenDiskScratchPage;
static u32 XenDiskScratchMachine;
#ifdef CONFIG_XEN_BLKDEV_GRANT
static int XenDiskScratchGrantRef;
#endif /* CONFIG_XEN_BLKDEV_GRANT */

static void *
XenDiskAllocatePageAlignedMemory(unsigned Size)
{
  void *Area;

  Area = MmAllocateMemory(Size);
  if (0 != ((ULONG_PTR) Area & (PAGE_SIZE - 1)))
    {
      printf("Got an unaligned page\n");
      XenDie();
    }

  return Area;
}

/* Tell the controller to bring up the interface. */
static void
XenDiskSendInterfaceConnect(void)
{
  ctrl_msg_t CMsg;
  blkif_fe_interface_connect_t *Msg;

  CMsg.type    = CMSG_BLKIF_FE,
  CMsg.subtype = CMSG_BLKIF_FE_INTERFACE_CONNECT,
  CMsg.length  = sizeof(blkif_fe_interface_connect_t),
  Msg = (void*)CMsg.msg;

  Msg->handle      = 0;
#if 2 == XEN_VER
  Msg->shmem_frame = (XenMemVirtualToMachine(XenDiskRing) >> PAGE_SHIFT);
#else /* XEN_VER */
  Msg->shmem_frame = (XenMemVirtualToMachine(XenDiskRing.sring) >> PAGE_SHIFT);
#endif /* XEN_VER */

  XenCtrlIfSendMessageBlock(&CMsg);
}

/* Move from CLOSED to DISCONNECTED state. */
static void
XenDiskDisconnect(void)
{
#if 2 == XEN_VER
  if (NULL != XenDiskRing)
    {
      MmFreeMemory(XenDiskRing);
    }

  XenDiskRing = (blkif_ring_t *)XenDiskAllocatePageAlignedMemory(PAGE_SIZE);
  XenDiskRing->req_prod = 0;
  XenDiskRing->resp_prod = 0;
  XenDiskRespCons = 0;
  XenDiskReqProd = 0;
#else /* XEN_VER */
  blkif_sring_t *SRing;

  if (NULL != XenDiskRing.sring)
    {
      MmFreeMemory(XenDiskRing.sring);
    }

  SRing = (blkif_sring_t *)XenDiskAllocatePageAlignedMemory(PAGE_SIZE);
  SHARED_RING_INIT(SRing);
  FRONT_RING_INIT(&XenDiskRing, SRing, PAGE_SIZE);
#endif /* XEN_VER */

  XenDiskState  = DISK_STATE_DISCONNECTED;
  XenDiskSendInterfaceConnect();
}

VOID
XenDiskHandleEvent()
{
  XenDiskRspReceived = TRUE;
}

static void
XenDiskConnect(blkif_fe_interface_status_t *Status)
{
  XenDiskEvtchn = Status->evtchn;
  XenEvtchnRegisterDisk(XenDiskEvtchn);

#ifdef CONFIG_XEN_BLKDEV_GRANT
  XenDiskScratchGrantRef = XenMemGrantForeignAccess(Status->domid,
                                                    XenDiskScratchPage,
                                                    FALSE);
#endif /* CONFIG_XEN_BLKDEV_GRANT */

  XenDiskState = DISK_STATE_CONNECTED;
}

static void
XenDiskFree(void)
{
  XenDiskState = DISK_STATE_DISCONNECTED;

  /* Free resources associated with old device channel. */
#if 2 == XEN_VER
  if (NULL != XenDiskRing)
    {
      MmFreeMemory(XenDiskRing);
      XenDiskRing = NULL;
    }
#else /* XEN_VER */
  if (NULL != XenDiskRing.sring)
    {
      MmFreeMemory(XenDiskRing.sring);
      XenDiskRing.sring = NULL;
    }
#endif /* XEN_VER */

  XenDiskEvtchn = 0;
}

static void
XenDiskReset()
{
  XenDiskFree();
  XenDiskDisconnect();
}

static void
XenDiskStatus(blkif_fe_interface_status_t *Status)
{
  switch (Status->status)
    {
    case BLKIF_INTERFACE_STATUS_DISCONNECTED:
      switch (XenDiskState)
        {
        case DISK_STATE_CLOSED:
          XenDiskDisconnect();
          break;
        case DISK_STATE_DISCONNECTED:
        case DISK_STATE_CONNECTED:
          /* unexpected(status); */ /* occurs during suspend/resume */
          XenDiskReset();
          break;
        }
        break;

    case BLKIF_INTERFACE_STATUS_CONNECTED:
      switch (XenDiskState)
        {
        case DISK_STATE_CLOSED:
          XenDiskDisconnect();
          XenDiskConnect(Status);
          break;
        case DISK_STATE_DISCONNECTED:
          XenDiskConnect(Status);
          break;
        case DISK_STATE_CONNECTED:
          XenDiskConnect(Status);
          break;
        }
      break;

    default:
      break;
    }
}

static void
XenDiskMsgHandler(ctrl_msg_t *Msg, unsigned long Id)
{
  switch (Msg->subtype)
    {
    case CMSG_BLKIF_FE_INTERFACE_STATUS:
      XenDiskStatus((blkif_fe_interface_status_t *)
                    &Msg->msg[0]);
      break;
    default:
      Msg->length = 0;
      break;
    }

  XenCtrlIfSendResponse(Msg);
}

static void
XenDiskSendDriverStatus(BOOL Up)
{
  ctrl_msg_t CMsg;
  blkif_fe_driver_status_t *Msg = (void*) CMsg.msg;

  CMsg.type = CMSG_BLKIF_FE;
  CMsg.subtype = CMSG_BLKIF_FE_DRIVER_STATUS;
  CMsg.length  = sizeof(blkif_fe_driver_status_t);

  Msg->status = (Up ? BLKIF_DRIVER_STATUS_UP : BLKIF_DRIVER_STATUS_DOWN);

  XenCtrlIfSendMessageBlock(&CMsg);
}

static void
XenDiskWaitForState(unsigned State)
{
  /* FIXME maybe introduce a timeout here? */
  while (TRUE)
    {
      XenEvtchnDisableEvents();
      if (XenDiskState == State)
        {
          XenEvtchnEnableEvents();
          return;
        }
      HYPERVISOR_block();
    }
}

static BOOL
XenDiskControlSend(blkif_request_t *Req, blkif_response_t *Rsp)
{
#if 2 != XEN_VER
  blkif_request_t *DReq;
#endif /* XEN_VER */
  blkif_response_t *SRsp;

  while (TRUE)
    {
      XenEvtchnDisableEvents();
#if 2 == XEN_VER
      if (BLKIF_RING_SIZE != (XenDiskReqProd - XenDiskRespCons))
#else /* XEN_VER */
      if (! RING_FULL(&XenDiskRing))
#endif
        {
          break;
        }
      HYPERVISOR_block();
    }

  if (DISK_STATE_CONNECTED != XenDiskState)
    {
      XenEvtchnEnableEvents();
      return FALSE;
    }

#if 2 == XEN_VER
  XenDiskRing->ring[MASK_BLKIF_IDX(XenDiskReqProd)].req = *Req;
  XenDiskReqProd++;
  XenDiskRing->req_prod = XenDiskReqProd;
#else /* XEN_VER */
  DReq = RING_GET_REQUEST(&XenDiskRing, XenDiskRing.req_prod_pvt);
  *DReq = *Req;

  XenDiskRing.req_prod_pvt++;
  RING_PUSH_REQUESTS(&XenDiskRing);
#endif /* XEN_VER */
  XenDiskRspReceived = FALSE;

  XenEvtchnEnableEvents();

  notify_via_evtchn(XenDiskEvtchn);

  while (TRUE)
    {
      XenEvtchnDisableEvents();
      if (XenDiskRspReceived)
        {
          break;
        }
      HYPERVISOR_block();
    }

  if (DISK_STATE_CONNECTED != XenDiskState)
    {
      XenEvtchnEnableEvents();
      return FALSE;
    }

#if 2 == XEN_VER
  SRsp = &XenDiskRing->ring[MASK_BLKIF_IDX(XenDiskRespCons)].resp;
  *Rsp = *SRsp;
  XenDiskRespCons++;
#else /* XEN_VER */
  SRsp = RING_GET_RESPONSE(&XenDiskRing, XenDiskRing.rsp_cons);
  *Rsp = *SRsp;
  XenDiskRing.rsp_cons++;
#endif /* XEN_VER */

  XenDiskRspReceived = FALSE;
  XenEvtchnEnableEvents();

  return TRUE;
}

static void
XenDiskProbe()
{
  blkif_response_t Rsp;
  blkif_request_t Req;
  unsigned Disk;
  vdisk_t *Probe;
  ULONG FloppyNumber, CDRomNumber, HarddiskNumber;

  memset(&Req, 0, sizeof(blkif_request_t));
  Req.operation = BLKIF_OP_PROBE;
  Req.nr_segments = 1;
#ifdef CONFIG_XEN_BLKDEV_GRANT
  Req.frame_and_sects[0] = (((u32) XenDiskScratchGrantRef) << 16) | 7;
#else /* CONFIG_XEN_BLKDEV_GRANT */
  Req.frame_and_sects[0] = XenDiskScratchMachine | 7;
#endif /* CONFIG_XEN_BLKDEV_GRANT */

  if (! XenDiskControlSend(&Req, &Rsp))
    {
      printf("Unexpected disk disconnect\n");
      XenDie();
    }

  if (Rsp.status <= 0)
    {
      printf("Could not probe disks (%d)\n", Rsp.status);
      XenDie();
    }

  XenDiskCount = Rsp.status;
  XenDiskInfo = MmAllocateMemory(XenDiskCount * sizeof(XENDISKINFO));
  if (NULL == XenDiskInfo)
    {
      XenDie();
    }
  Probe = (vdisk_t *) XenDiskScratchPage;
  FloppyNumber = 0x00;
  HarddiskNumber = 0x80;
  CDRomNumber = 0x9f;
  for (Disk = 0; Disk < XenDiskCount; Disk++)
    {
      XenDiskInfo[Disk].SectorCount = Probe[Disk].capacity;
      if (0 != (Probe[Disk].info & VDISK_REMOVABLE))
        {
          XenDiskInfo[Disk].DriveNumber = FloppyNumber++;
        }
      else if (0 != (Probe[Disk].info & VDISK_CDROM))
        {
          XenDiskInfo[Disk].DriveNumber = CDRomNumber--;
        }
      else
        {
          XenDiskInfo[Disk].DriveNumber = HarddiskNumber++;
        }
      XenDiskInfo[Disk].BytesPerSector = Probe[Disk].sector_size;
      XenDiskInfo[Disk].XenDevice = Probe[Disk].device;
      XenDiskInfo[Disk].XenInfo = Probe[Disk].info;
    }
}

static void
XenDiskInit()
{
  XenDiskState = DISK_STATE_CLOSED;

  if (NULL == XenDiskScratchPage)
    {
      XenDiskScratchPage = XenDiskAllocatePageAlignedMemory(PAGE_SIZE);
      XenDiskScratchMachine = XenMemVirtualToMachine(XenDiskScratchPage);
    }

  XenCtrlIfRegisterReceiver(CMSG_BLKIF_FE, XenDiskMsgHandler);

  XenDiskSendDriverStatus(TRUE);
  XenDiskWaitForState(DISK_STATE_CONNECTED);

  XenDiskProbe();
}

static PXENDISKINFO
XenDiskFindDrive(ULONG DriveNumber)
{
  PXENDISKINFO DiskInfo;
  unsigned Disk;

  DiskInfo = NULL;
  for (Disk = 0; DiskInfo == NULL && Disk < XenDiskCount; Disk++)
    {
      if (DriveNumber == XenDiskInfo[Disk].DriveNumber)
        {
          DiskInfo = XenDiskInfo + Disk;
        }
    }

  return DiskInfo;
}

BOOL
XenDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber,
                          ULONG SectorCount, PVOID Buffer)
{
  blkif_response_t Rsp;
  blkif_request_t Req;
  ULONG Count;
  PXENDISKINFO DiskInfo;

  if (! XenDiskInitialized)
    {
      XenDiskInitialized = TRUE;
      XenDiskInit();
    }

  DiskInfo = XenDiskFindDrive(DriveNumber);
  if (NULL == DiskInfo)
    {
      return FALSE;
    }

  while (0 < SectorCount)
    {
      Count = SectorCount;
      if (PAGE_SIZE / DiskInfo->BytesPerSector < Count)
        {
          Count = PAGE_SIZE / DiskInfo->BytesPerSector;
        }
      memset(&Req, 0, sizeof(blkif_request_t));
      Req.operation = BLKIF_OP_READ;
      Req.nr_segments = 1;
      Req.device = DiskInfo->XenDevice;
      Req.id = 0;
      Req.sector_number = SectorNumber;
#ifdef CONFIG_XEN_BLKDEV_GRANT
      Req.frame_and_sects[0] = (((u32) XenDiskScratchGrantRef) << 16)
                               | (Count - 1);
#else /* CONFIG_XEN_BLKDEV_GRANT */
      Req.frame_and_sects[0] = XenDiskScratchMachine | (Count - 1);
#endif /* CONFIG_XEN_BLKDEV_GRANT */

      if (! XenDiskControlSend(&Req, &Rsp))
        {
          return FALSE;
        }
      if (BLKIF_RSP_OKAY != Rsp.status)
        {
          return FALSE;
        }
      memcpy(Buffer, XenDiskScratchPage, Count * DiskInfo->BytesPerSector);
      SectorCount -= Count;
      SectorNumber += Count;
      Buffer = (void *)((char *) Buffer + Count * DiskInfo->BytesPerSector);
    }

  return TRUE;
}

BOOL
XenDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry)
{
  PXENDISKINFO DiskInfo;

  DiskInfo = XenDiskFindDrive(DriveNumber);
  if (NULL == DiskInfo)
    {
      return FALSE;
    }

  if (DiskInfo->SectorCount < 255)
    {
      DriveGeometry->Sectors = DiskInfo->SectorCount;
      DriveGeometry->Heads = 1;
      DriveGeometry->Cylinders = 1;
    }
  else if (DiskInfo->SectorCount < 255 * 63)
    {
      DriveGeometry->Sectors = 255;
      DriveGeometry->Heads = DiskInfo->SectorCount / 255;
      DriveGeometry->Cylinders = 1;
    }
  else
    {
      DriveGeometry->Sectors = 255;
      DriveGeometry->Heads = 63;
      DriveGeometry->Cylinders = DiskInfo->SectorCount / (255 * 63);
    }
  DriveGeometry->BytesPerSector = DiskInfo->BytesPerSector;

  return TRUE;
}

ULONG
XenDiskGetCacheableBlockCount(ULONG DriveNumber)
{
  /* 64 seems a nice number, it is used by the machpc code for LBA devices */
  return 64;
}

BOOL
XenDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber,
                         PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  /* Just use the standard routine */
  return DiskGetPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
}

/* EOF */
