/*
 * Copyright 2000 Corel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _TWAIN32_H
#define _TWAIN32_H

#ifdef HAVE_SANE
# include <sane/sane.h>
#endif
#include "twain.h"
#include "windef.h"

/* internel information about an active data source */
typedef struct tagActiveDS
{
    struct tagActiveDS	*next;			/* next active DS */
    TW_IDENTITY		identity;		/* identity */
    TW_UINT16		currentState;		/* current state */
    TW_EVENT		pendingEvent;		/* pending event to be sent to
                                                   application */
    TW_UINT16		twCC;			/* condition code */
    HWND		hwndOwner;		/* window handle of the app */
#ifdef HAVE_SANE
    SANE_Handle		deviceHandle;		/* device handle */
    SANE_Parameters     sane_param;             /* parameters about the image
                                                   transfered */
#endif
    /* Capabiblities */
    TW_UINT16		capXferMech;		/* ICAP_XFERMECH */
} activeDS;

TW_UINT16 DSM_initialized;      /* whether Source Manager is initialized */
TW_UINT16 DSM_currentState;     /* current state of Source Manager */
TW_UINT16 DSM_twCC;             /* current condition code of Source Manager */
TW_HANDLE DSM_parentHWND;       /* window handle of the Source's "parent" */
TW_UINT32 DSM_sourceId;         /* source id generator */
TW_UINT16 DSM_currentDevice;    /* keep track of device during enumeration */
#ifdef HAVE_SANE
const SANE_Device **device_list;/* a list of all sane devices */
#endif
activeDS *activeSources;	/* list of active data sources */

/* Helper functions */
extern activeDS *TWAIN_LookupSource (pTW_IDENTITY pDest);
extern TW_UINT16 TWAIN_SaneCapability (activeDS *pSource,
        pTW_CAPABILITY pCapability, TW_UINT16 action);

/*  */
extern TW_UINT16 TWAIN_ControlGroupHandler (
	pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
    TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 TWAIN_ImageGroupHandler (
	pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
	TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 TWAIN_AudioGroupHandler (
	pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
	TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
extern TW_UINT16 TWAIN_SourceManagerHandler (
	pTW_IDENTITY pOrigin, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);

/* Implementation of operation triplets (From Application to Source Manager) */
extern TW_UINT16 TWAIN_CloseDS
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_IdentityGetDefault
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_IdentityGetFirst
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_IdentityGetNext
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_OpenDS
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_UserSelect
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_CloseDSM
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_OpenDSM
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);
extern TW_UINT16 TWAIN_GetDSMStatus
           (pTW_IDENTITY pOrigin, TW_MEMREF pData);

/* Implementation of operation triplets
 * From Application to Source (Control Information) */
TW_UINT16 TWAIN_CapabilityGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CapabilityGetCurrent
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,TW_MEMREF pData);
TW_UINT16 TWAIN_CapabilityGetDefault
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CapabilityQuerySupport
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CapabilityReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CapabilitySet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CustomDSDataGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CustomDSDataSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_AutomaticCaptureDirectory
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ChangeDirectory
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemCopy
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_CreateDirectory
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemDelete
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FormatMedia
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemGetClose
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemGetFirstFile
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemGetInfo
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemGetNextFile
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_FileSystemRename
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ProcessEvent
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_PassThrough
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_PendingXfersEndXfer
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_PendingXfersGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_PendingXfersReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_PendingXfersStopFeeder
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXferGetDefault
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXferReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXferSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXfer2Get
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXfer2GetDefault
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXfer2Reset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupFileXfer2Set
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_SetupMemXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_GetDSStatus
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_DisableDSUserInterface
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_EnableDSUserInterface
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_EnableDSUIOnly
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_XferGroupGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_XferGroupSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);

/* Implementation of operation triplets
 * From Application to Source (Image Information) */
TW_UINT16 TWAIN_CIEColorGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ExtImageInfoGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_GrayResponseReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_GrayResponseSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageFileXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageInfoGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageLayoutGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageLayoutGetDefault
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageLayoutReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageLayoutSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageMemXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_ImageNativeXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_JPEGCompressionGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_JPEGCompressionGetDefault
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_JPEGCompressionReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_JPEGCompressionSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_Palette8Get
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_Palette8GetDefault
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_Palette8Reset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_Palette8Set
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_RGBResponseReset
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_RGBResponseSet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);

/* Implementation of operation triplets
 * From Application to Source (Audio Information) */
TW_UINT16 TWAIN_AudioFileXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_AudioInfoGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);
TW_UINT16 TWAIN_AudioNativeXferGet
    (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_MEMREF pData);

/* Implementation of TWAIN capabilities */
TW_UINT16 TWAIN_ICAPXferMech
    (activeDS *pSource, pTW_CAPABILITY pCapability, TW_UINT16 action);

#endif
