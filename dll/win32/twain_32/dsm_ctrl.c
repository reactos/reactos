/*
 * TWAIN32 Source Manager
 *
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "twain_i.h"

static TW_UINT16 DSM_initialized;	/* whether Source Manager is initialized */
static TW_UINT32 DSM_sourceId;		/* source id generator */
static TW_UINT16 DSM_currentDevice;	/* keep track of device during enumeration */

struct all_devices {
	char 		*modname;
	TW_IDENTITY	identity;
};

static int nrdevices = 0;
static struct all_devices *devices = NULL;

static void
twain_add_onedriver(const char *dsname) {
	HMODULE 	hmod;
	DSENTRYPROC	dsEntry;
	TW_IDENTITY	fakeOrigin;
	TW_IDENTITY	sourceId;
	TW_UINT16	ret;

	hmod = LoadLibraryA(dsname);
	if (!hmod) {
		ERR("Failed to load TWAIN Source %s\n", dsname);
		return;
	}
	dsEntry = (DSENTRYPROC)GetProcAddress(hmod, "DS_Entry"); 
	if (!dsEntry) {
		ERR("Failed to find DS_Entry() in TWAIN DS %s\n", dsname);
		return;
	}
	/* Loop to do multiple detects, mostly for sane.ds and gphoto2.ds */
	do {
		int i;

		sourceId.Id 		= DSM_sourceId;
		sourceId.ProtocolMajor	= TWON_PROTOCOLMAJOR;
		sourceId.ProtocolMinor	= TWON_PROTOCOLMINOR;
		ret = dsEntry (&fakeOrigin, DG_CONTROL, DAT_IDENTITY, MSG_GET, &sourceId);
		if (ret != TWRC_SUCCESS) {
			ERR("Source->(DG_CONTROL,DAT_IDENTITY,MSG_GET) failed!\n");
                        break;
		}
		TRACE("Manufacturer: %s\n",	debugstr_a(sourceId.Manufacturer));
		TRACE("ProductFamily: %s\n",	debugstr_a(sourceId.ProductFamily));
		TRACE("ProductName: %s\n",	debugstr_a(sourceId.ProductName));

		for (i=0;i<nrdevices;i++) {
			if (!strcmp(sourceId.ProductName,devices[i].identity.ProductName))
				break;
		}
		if (i < nrdevices)
			break;
		if (nrdevices)
			devices = HeapReAlloc(GetProcessHeap(), 0, devices, sizeof(devices[0])*(nrdevices+1));
		else
			devices = HeapAlloc(GetProcessHeap(), 0, sizeof(devices[0]));
		if ((devices[nrdevices].modname = HeapAlloc(GetProcessHeap(), 0, strlen(dsname) + 1)))
			lstrcpyA(devices[nrdevices].modname, dsname);
		devices[nrdevices].identity = sourceId;
		nrdevices++;
		DSM_sourceId++;
	} while (1);
	FreeLibrary (hmod);
}

static BOOL detectionrun = FALSE;

static void
twain_autodetect(void) {
	if (detectionrun) return;
        detectionrun = TRUE;

	twain_add_onedriver("sane.ds");
	twain_add_onedriver("gphoto2.ds");
#if 0
	twain_add_onedriver("c:\\windows\\Twain_32\\Largan\\sp503a.ds");
	twain_add_onedriver("c:\\windows\\Twain_32\\vivicam10\\vivicam10.ds");
	twain_add_onedriver("c:\\windows\\Twain_32\\ws30slim\\sp500a.ds");
#endif
}

/* DG_CONTROL/DAT_IDENTITY/MSG_CLOSEDS */
TW_UINT16 TWAIN_CloseDS (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	TW_UINT16 twRC = TWRC_SUCCESS;
	pTW_IDENTITY pIdentity = (pTW_IDENTITY) pData;
	activeDS *currentDS = NULL, *prevDS = NULL;

	TRACE ("DG_CONTROL/DAT_IDENTITY/MSG_CLOSEDS\n");

	for (currentDS = activeSources; currentDS; currentDS = currentDS->next) {
		if (currentDS->identity.Id == pIdentity->Id)
			break;
		prevDS = currentDS;
	}
	if (!currentDS) {
		DSM_twCC = TWCC_NODS;
		return TWRC_FAILURE;
	}
	twRC = currentDS->dsEntry (pOrigin, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, pData);
	/* This causes crashes due to still open Windows, so leave out for now.
	 * FreeLibrary (currentDS->hmod);
	 */
	if (prevDS)
		prevDS->next = currentDS->next;
	else
		activeSources = currentDS->next;
	HeapFree (GetProcessHeap(), 0, currentDS);
	if (twRC == TWRC_SUCCESS)
		DSM_twCC = TWCC_SUCCESS;
	else /* FIXME: unclear how to get TWCC */
		DSM_twCC = TWCC_SEQERROR;
	return twRC;
}

/* DG_CONTROL/DAT_IDENTITY/MSG_GETDEFAULT */
TW_UINT16 TWAIN_IdentityGetDefault (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	pTW_IDENTITY pSourceIdentity = (pTW_IDENTITY) pData;

	TRACE("DG_CONTROL/DAT_IDENTITY/MSG_GETDEFAULT\n");
	DSM_twCC = TWCC_NODS;
	twain_autodetect();
	if (!nrdevices)
		return TWRC_FAILURE;
	*pSourceIdentity = devices[0].identity;
	DSM_twCC = TWCC_SUCCESS;
	return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_IDENTITY/MSG_GETFIRST */
TW_UINT16 TWAIN_IdentityGetFirst (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	pTW_IDENTITY pSourceIdentity = (pTW_IDENTITY) pData;

	TRACE ("DG_CONTROL/DAT_IDENTITY/MSG_GETFIRST\n");
	twain_autodetect();
	if (!nrdevices) {
		TRACE ("no entries found.\n");
		DSM_twCC = TWCC_NODS;
		return TWRC_FAILURE;
	}
	DSM_currentDevice = 0;
	*pSourceIdentity = devices[DSM_currentDevice++].identity;
	return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_IDENTITY/MSG_GETNEXT */
TW_UINT16 TWAIN_IdentityGetNext (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	pTW_IDENTITY pSourceIdentity = (pTW_IDENTITY) pData;

	TRACE("DG_CONTROL/DAT_IDENTITY/MSG_GETNEXT\n");
	if (!nrdevices || (DSM_currentDevice == nrdevices)) {
		DSM_twCC = TWCC_SUCCESS;
		return TWRC_ENDOFLIST;
	}
	*pSourceIdentity = devices[DSM_currentDevice++].identity;
	return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_IDENTITY/MSG_OPENDS */
TW_UINT16 TWAIN_OpenDS (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	TW_UINT16 i = 0;
	pTW_IDENTITY pIdentity = (pTW_IDENTITY) pData;
	activeDS *newSource;
	const char *modname = NULL;
	HMODULE hmod;

	TRACE("DG_CONTROL/DAT_IDENTITY/MSG_OPENDS\n");
        TRACE("pIdentity is %s\n", pIdentity->ProductName);
	if (!DSM_initialized) {
		FIXME("seq error\n");
		DSM_twCC = TWCC_SEQERROR;
		return TWRC_FAILURE;
	}
	twain_autodetect();
	if (!nrdevices) {
		FIXME("no devs.\n");
		DSM_twCC = TWCC_NODS;
		return TWRC_FAILURE;
	}

	if (pIdentity->ProductName[0] != '\0') {
		/* Make sure the source to be opened exists in the device list */
		for (i = 0; i<nrdevices; i++)
			if (!strcmp (devices[i].identity.ProductName, pIdentity->ProductName))
				break;
		if (i == nrdevices)
			i = 0;
	} /* else use the first device */

	/* the source is found in the device list */
	newSource = HeapAlloc (GetProcessHeap(), 0, sizeof (activeDS));
	if (!newSource) {
		DSM_twCC = TWCC_LOWMEMORY;
		FIXME("Out of memory.\n");
		return TWRC_FAILURE;
	}
	hmod = LoadLibraryA(devices[i].modname);
	if (!hmod) {
		ERR("Failed to load TWAIN Source %s\n", modname);
		DSM_twCC = TWCC_OPERATIONERROR;
                HeapFree(GetProcessHeap(), 0, newSource);
		return TWRC_FAILURE;
	}
	newSource->hmod = hmod; 
	newSource->dsEntry = (DSENTRYPROC)GetProcAddress(hmod, "DS_Entry"); 
	if (TWRC_SUCCESS != newSource->dsEntry (pOrigin, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, pIdentity)) {
		DSM_twCC = TWCC_OPERATIONERROR;
                HeapFree(GetProcessHeap(), 0, newSource);
		return TWRC_FAILURE;
	}
	/* Assign name and id for the opened data source */
	pIdentity->Id = DSM_sourceId ++;
	/* add the data source to an internal active source list */
	newSource->next = activeSources;
	newSource->identity.Id = pIdentity->Id;
	strcpy (newSource->identity.ProductName, pIdentity->ProductName);
	activeSources = newSource;
	DSM_twCC = TWCC_SUCCESS;
	return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT */
TW_UINT16 TWAIN_UserSelect (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	pTW_IDENTITY	selected = (pTW_IDENTITY)pData;

	if (!nrdevices) {
		DSM_twCC = TWCC_OPERATIONERROR;
		return TWRC_FAILURE;
	}
	*selected = devices[0].identity;
	DSM_twCC = TWCC_SUCCESS;
	return TWRC_SUCCESS;
}

/* DG_CONTROL/DAT_PARENT/MSG_CLOSEDSM */
TW_UINT16 TWAIN_CloseDSM (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    activeDS *currentDS = activeSources, *nextDS;

    TRACE("DG_CONTROL/DAT_PARENT/MSG_CLOSEDSM\n");

    if (DSM_initialized)
    {
        DSM_initialized = FALSE;

        /* If there are data sources still open, close them now. */
        while (currentDS != NULL)
        {
            nextDS = currentDS->next;
	    currentDS->dsEntry (pOrigin, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, pData);
            HeapFree (GetProcessHeap(), 0, currentDS);
            currentDS = nextDS;
        }
        activeSources = NULL;
        DSM_twCC = TWCC_SUCCESS;
        return TWRC_SUCCESS;
    } else {
        DSM_twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }
}

/* DG_CONTROL/DAT_PARENT/MSG_OPENDSM */
TW_UINT16 TWAIN_OpenDSM (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	TW_UINT16 twRC = TWRC_SUCCESS;

	TRACE("DG_CONTROL/DAT_PARENT/MSG_OPENDSM\n");
        if (!DSM_initialized) {
		DSM_currentDevice = 0;
		DSM_initialized = TRUE;
		DSM_twCC = TWCC_SUCCESS;
		twRC = TWRC_SUCCESS;
	} else {
		/* operation invoked in invalid state */
		DSM_twCC = TWCC_SEQERROR;
		twRC = TWRC_FAILURE;
	}
	return twRC;
}

/* DG_CONTROL/DAT_STATUS/MSG_GET */
TW_UINT16 TWAIN_GetDSMStatus (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
	pTW_STATUS pSourceStatus = (pTW_STATUS) pData;

	TRACE ("DG_CONTROL/DAT_STATUS/MSG_GET\n");

	pSourceStatus->ConditionCode = DSM_twCC;
	DSM_twCC = TWCC_SUCCESS;  /* clear the condition code */
	return TWRC_SUCCESS;
}
