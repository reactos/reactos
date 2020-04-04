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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "twain.h"
#include "twain_i.h"
#include "resource.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static TW_UINT16 DSM_initialized;	/* whether Source Manager is initialized */
static TW_UINT32 DSM_sourceId;		/* source id generator */
static TW_UINT16 DSM_currentDevice;	/* keep track of device during enumeration */
static HWND DSM_parent;
static UINT event_message;

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
		ERR("Failed to load TWAIN Source %s\n", debugstr_a(dsname));
		return;
	}
	dsEntry = (DSENTRYPROC)GetProcAddress(hmod, "DS_Entry"); 
	if (!dsEntry) {
		ERR("Failed to find DS_Entry() in TWAIN DS %s\n", debugstr_a(dsname));
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

/* DG_CONTROL/DAT_NULL/MSG_CLOSEDSREQ|MSG_DEVICEEVENT|MSG_XFERREADY */
TW_UINT16 TWAIN_ControlNull (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, activeDS *pSource, TW_UINT16 MSG, TW_MEMREF pData)
{
    struct pending_message *message;

    TRACE ("DG_CONTROL/DAT_NULL MSG=%i\n", MSG);

    if (MSG != MSG_CLOSEDSREQ &&
        MSG != MSG_DEVICEEVENT &&
        MSG != MSG_XFERREADY)
    {
        DSM_twCC = TWCC_BADPROTOCOL;
        return TWRC_FAILURE;
    }

    message = HeapAlloc(GetProcessHeap(), 0, sizeof(*message));
    if (!message)
    {
        DSM_twCC = TWCC_LOWMEMORY;
        return TWRC_FAILURE;
    }

    message->msg = MSG;
    list_add_tail(&pSource->pending_messages, &message->entry);

    /* Delphi twain only sends us messages from one window, and it
       doesn't even give us the real handle to that window. Other
       applications might decide to forward messages sent to DSM_parent
       or to the one supplied to ENABLEDS. So let's try very hard to
       find a window that will work. */
    if (DSM_parent)
        PostMessageW(DSM_parent, event_message, 0, 0);
    if (pSource->ui_window && pSource->ui_window != DSM_parent)
        PostMessageW(pSource->ui_window, event_message, 0, 0);
    if (pSource->event_window && pSource->event_window != pSource->ui_window &&
        pSource->event_window != DSM_parent)
        PostMessageW(pSource->event_window, event_message, 0, 0);
    PostMessageW(0, event_message, 0, 0);

    return TWRC_SUCCESS;
}

/* Filters MSG_PROCESSEVENT messages before reaching the data source */
TW_UINT16 TWAIN_ProcessEvent (pTW_IDENTITY pOrigin, activeDS *pSource, TW_MEMREF pData)
{
    TW_EVENT *event = (TW_EVENT*)pData;
    MSG *msg = (MSG*)event->pEvent;
    TW_UINT16 result = TWRC_NOTDSEVENT;

    TRACE("%x,%x\n", msg->message, event_message);

    if (msg->message == event_message)
    {
        if (!list_empty (&pSource->pending_messages))
        {
            struct list *entry = list_head (&pSource->pending_messages);
            struct pending_message *message = LIST_ENTRY(entry, struct pending_message, entry);
            event->TWMessage = message->msg;
            list_remove (entry);
            TRACE("<-- %x\n", event->TWMessage);
        }
        else
            event->TWMessage = MSG_NULL;
        result = TWRC_DSEVENT;
    }

    if (msg->hwnd)
    {
        MSG dummy;
        pSource->event_window = msg->hwnd;
        if (!list_empty (&pSource->pending_messages) &&
            !PeekMessageW(&dummy, msg->hwnd, event_message, event_message, PM_NOREMOVE))
        {
            PostMessageW(msg->hwnd, event_message, 0, 0);
        }
    }

    return result;
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
		ERR("Failed to load TWAIN Source %s\n", debugstr_a(modname));
		DSM_twCC = TWCC_OPERATIONERROR;
                HeapFree(GetProcessHeap(), 0, newSource);
		return TWRC_FAILURE;
	}
	newSource->hmod = hmod; 
	newSource->dsEntry = (DSENTRYPROC)GetProcAddress(hmod, "DS_Entry"); 
	/* Assign id for the opened data source */
	pIdentity->Id = DSM_sourceId ++;
	if (TWRC_SUCCESS != newSource->dsEntry (pOrigin, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, pIdentity)) {
		DSM_twCC = TWCC_OPERATIONERROR;
                HeapFree(GetProcessHeap(), 0, newSource);
                DSM_sourceId--;
		return TWRC_FAILURE;
	}
	/* add the data source to an internal active source list */
	newSource->next = activeSources;
	newSource->identity.Id = pIdentity->Id;
	strcpy (newSource->identity.ProductName, pIdentity->ProductName);
        list_init(&newSource->pending_messages);
        newSource->ui_window = NULL;
        newSource->event_window = NULL;
	activeSources = newSource;
	DSM_twCC = TWCC_SUCCESS;
	return TWRC_SUCCESS;
}

typedef struct {
    pTW_IDENTITY origin;
    pTW_IDENTITY result;
} userselect_data;

static INT_PTR CALLBACK userselect_dlgproc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        userselect_data *data = (userselect_data*)lparam;
        int i;
        HWND sourcelist;
        BOOL any_devices = FALSE;

        SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)data);

        sourcelist = GetDlgItem(hwnd, IDC_LISTSOURCE);

        for (i=0; i<nrdevices; i++)
        {
            TW_IDENTITY *id = &devices[i].identity;
            LRESULT index;

            if ((id->SupportedGroups & data->origin->SupportedGroups) == 0)
                continue;

            index = SendMessageA(sourcelist, LB_ADDSTRING, 0, (LPARAM)id->ProductName);
            SendMessageW(sourcelist, LB_SETITEMDATA, (WPARAM)index, (LPARAM)i);
            any_devices = TRUE;
        }

        if (any_devices)
        {
            EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);

            /* FIXME: Select the supplied product name or default source. */
            SendMessageW(sourcelist, LB_SETCURSEL, 0, 0);
        }

        return TRUE;
    }
    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;
    case WM_COMMAND:
        if (wparam == MAKEWPARAM(IDCANCEL, BN_CLICKED))
        {
            EndDialog(hwnd, 0);
            return TRUE;
        }
        else if (wparam == MAKEWPARAM(IDOK, BN_CLICKED) ||
                 wparam == MAKEWPARAM(IDC_LISTSOURCE, LBN_DBLCLK))
        {
            userselect_data *data = (userselect_data*)GetWindowLongPtrW(hwnd, DWLP_USER);
            HWND sourcelist;
            LRESULT index;

            sourcelist = GetDlgItem(hwnd, IDC_LISTSOURCE);

            index = SendMessageW(sourcelist, LB_GETCURSEL, 0, 0);

            if (index == LB_ERR)
                return TRUE;

            index = SendMessageW(sourcelist, LB_GETITEMDATA, (WPARAM)index, 0);

            *data->result = devices[index].identity;

            /* FIXME: Save this as the default source */

            EndDialog(hwnd, 1);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT */
TW_UINT16 TWAIN_UserSelect (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
    userselect_data param = {pOrigin, pData};
    HWND parent = DSM_parent;

    TRACE("DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT SupportedGroups=0x%x ProductName=%s\n",
        pOrigin->SupportedGroups, wine_dbgstr_a(param.result->ProductName));

    twain_autodetect();

    if (!IsWindow(parent))
        parent = NULL;

    if (DialogBoxParamW(DSM_hinstance, MAKEINTRESOURCEW(DLG_USERSELECT),
        parent, userselect_dlgproc, (LPARAM)&param) == 0)
    {
        TRACE("canceled\n");
        DSM_twCC = TWCC_SUCCESS;
        return TWRC_CANCEL;
    }

    TRACE("<-- %s\n", wine_dbgstr_a(param.result->ProductName));
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
        DSM_parent = NULL;
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
                event_message = RegisterWindowMessageA("WINE TWAIN_32 EVENT");
		DSM_currentDevice = 0;
		DSM_initialized = TRUE;
		DSM_twCC = TWCC_SUCCESS;
		twRC = TWRC_SUCCESS;
	} else {
		/* operation invoked in invalid state */
		DSM_twCC = TWCC_SEQERROR;
		twRC = TWRC_FAILURE;
	}
        DSM_parent = (HWND)pData;
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
