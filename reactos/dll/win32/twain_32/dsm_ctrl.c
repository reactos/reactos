/*
 * TWAIN32 Source Manager
 *
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

//#include "config.h"

#include <stdlib.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "twain.h"
#include "twain_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_CONTROL/DAT_IDENTITY/MSG_CLOSEDS */
TW_UINT16 TWAIN_CloseDS (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    DSM_twCC = TWCC_NODS;
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IDENTITY pIdentity = (pTW_IDENTITY) pData;
    activeDS *currentDS = NULL, *prevDS = NULL;

    TRACE ("DG_CONTROL/DAT_IDENTITY/MSG_CLOSEDS\n");

    for (currentDS = activeSources; currentDS; currentDS = currentDS->next)
    {
        if (currentDS->identity.Id == pIdentity->Id)
            break;
        prevDS = currentDS;
    }
    if (currentDS)
    {
        /* Only valid to close a data source if it is in state 4 */
        if (currentDS->currentState == 4)
        {
            sane_close (currentDS->deviceHandle);
            /* remove the data source from active data source list */
            if (prevDS)
                prevDS->next = currentDS->next;
            else
                activeSources = currentDS->next;
            HeapFree (GetProcessHeap(), 0, currentDS);
            twRC = TWRC_SUCCESS;
            DSM_twCC = TWCC_SUCCESS;
        }
        else
        {
            twRC = TWRC_FAILURE;
            DSM_twCC = TWCC_SEQERROR;
        }
    }
    else
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_NODS;
    }

    return twRC;
#endif
}

/* DG_CONTROL/DAT_IDENTITY/MSG_GETDEFAULT */
TW_UINT16 TWAIN_IdentityGetDefault (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    DSM_twCC = TWCC_NODS;
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IDENTITY pSourceIdentity = (pTW_IDENTITY) pData;

    TRACE("DG_CONTROL/DAT_IDENTITY/MSG_GETDEFAULT\n");

    if (!device_list)
    {
        if ((sane_get_devices (&device_list, SANE_FALSE) != SANE_STATUS_GOOD))
        {
            DSM_twCC = TWCC_NODS;
            return TWRC_FAILURE;
        }
    }

    /* FIXME: the default device is not necessarily the first device.  *
     * Users should be able to choose the default device               */
    if (device_list && device_list[0])
    {
        pSourceIdentity->Id = DSM_sourceId ++;
        strcpy (pSourceIdentity->ProductName, device_list[0]->name);
        strcpy (pSourceIdentity->Manufacturer, device_list[0]->vendor);
        strcpy (pSourceIdentity->ProductFamily, device_list[0]->model);
        pSourceIdentity->ProtocolMajor = TWON_PROTOCOLMAJOR;
        pSourceIdentity->ProtocolMinor = TWON_PROTOCOLMINOR;

        twRC = TWRC_SUCCESS;
        DSM_twCC = TWCC_SUCCESS;
    }
    else
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_NODS;
    }

    return twRC;
#endif
}

/* DG_CONTROL/DAT_IDENTITY/MSG_GETFIRST */
TW_UINT16 TWAIN_IdentityGetFirst (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    DSM_twCC = TWCC_NODS;
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IDENTITY pSourceIdentity;/* = (pTW_IDENTITY) pData;*/
    SANE_Status status;

    TRACE ("DG_CONTROL/DAT_IDENTITY/MSG_GETFIRST\n");

    status = sane_get_devices (&device_list, SANE_FALSE);
    if (status == SANE_STATUS_GOOD)
    {
        if (device_list[0])
        {
            pSourceIdentity->Id = DSM_sourceId ++;
            strcpy (pSourceIdentity->ProductName, device_list[0]->name);
            strcpy (pSourceIdentity->Manufacturer, device_list[0]->vendor);
            strcpy (pSourceIdentity->ProductFamily, device_list[0]->model);
            pSourceIdentity->ProtocolMajor = TWON_PROTOCOLMAJOR;
            pSourceIdentity->ProtocolMinor = TWON_PROTOCOLMINOR;
        }
        DSM_currentDevice = 1;
        twRC = TWRC_SUCCESS;
        DSM_twCC = TWCC_SUCCESS;
    }
    else if (status == SANE_STATUS_NO_MEM)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_LOWMEMORY;
    }
    else
    {
        WARN("sane_get_devices() failed: %s\n", sane_strstatus (status));
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_NODS;
    }

    return twRC;
#endif
}

/* DG_CONTROL/DAT_IDENTITY/MSG_GETNEXT */
TW_UINT16 TWAIN_IdentityGetNext (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    DSM_twCC = TWCC_SUCCESS;
    return TWRC_ENDOFLIST;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IDENTITY pSourceIdentity = (pTW_IDENTITY) pData;

    TRACE("DG_CONTROL/DAT_IDENTITY/MSG_GETNEXT\n");

    if (device_list && device_list[DSM_currentDevice])
    {
        pSourceIdentity->Id = DSM_sourceId ++;
        strcpy (pSourceIdentity->ProductName, device_list[DSM_currentDevice]->name);
        strcpy (pSourceIdentity->Manufacturer, device_list[DSM_currentDevice]->vendor);
        strcpy (pSourceIdentity->ProductFamily, device_list[DSM_currentDevice]->model);
        pSourceIdentity->ProtocolMajor = TWON_PROTOCOLMAJOR;
        pSourceIdentity->ProtocolMinor = TWON_PROTOCOLMINOR;
        DSM_currentDevice ++;

        twRC = TWRC_SUCCESS;
        DSM_twCC = TWCC_SUCCESS;
    }
    else
    {
        DSM_twCC = TWCC_SUCCESS;
        twRC = TWRC_ENDOFLIST;
    }

    return twRC;
#endif
}

/* DG_CONTROL/DAT_IDENTITY/MSG_OPENDS */
TW_UINT16 TWAIN_OpenDS (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    DSM_twCC = TWCC_NODS;
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS, i = 0;
    pTW_IDENTITY pIdentity = (pTW_IDENTITY) pData;
    activeDS *newSource;
    SANE_Status status;

    TRACE("DG_CONTROL/DAT_IDENTITY/MSG_OPENDS\n");

    if (DSM_currentState != 3)
    {
        DSM_twCC = TWCC_SEQERROR;
        return TWRC_FAILURE;
    }

    if (!device_list &&
       (sane_get_devices (&device_list, SANE_FALSE) != SANE_STATUS_GOOD))
    {
        DSM_twCC = TWCC_NODS;
        return TWRC_FAILURE;
    }

    if (pIdentity->ProductName[0] != '\0')
    {
        /* Make sure the source to be open exists in the device list */
        for (i = 0; device_list[i]; i ++)
        {
            if (strcmp (device_list[i]->name, pIdentity->ProductName) == 0)
                break;
        }
    }

    if (device_list[i])
    {
        /* the source is found in the device list */
        newSource = HeapAlloc (GetProcessHeap(), 0, sizeof (activeDS));
        if (newSource)
        {
            status = sane_open(device_list[i]->name,&newSource->deviceHandle);
            if (status == SANE_STATUS_GOOD)
            {
                /* Assign name and id for the opened data source */
                strcpy (pIdentity->ProductName, device_list[i]->name);
                pIdentity->Id = DSM_sourceId ++;
                /* add the data source to an internal active source list */
                newSource->next = activeSources;
                newSource->identity.Id = pIdentity->Id;
                strcpy (newSource->identity.ProductName, pIdentity->ProductName);
                newSource->currentState = 4; /*transition into state 4*/
                newSource->twCC = TWCC_SUCCESS;
                activeSources = newSource;
                twRC = TWRC_SUCCESS;
                DSM_twCC = TWCC_SUCCESS;
            }
            else
            {
                twRC = TWRC_FAILURE;
                DSM_twCC = TWCC_OPERATIONERROR;
            }
        }
        else
        {
            twRC = TWRC_FAILURE;
            DSM_twCC = TWCC_LOWMEMORY;
        }
    }
    else
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_NODS;
    }

    return twRC;
#endif
}

/* DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT */
TW_UINT16 TWAIN_UserSelect (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    return TWRC_SUCCESS;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;

    TRACE("DG_CONTROL/DAT_IDENTITY/MSG_USERSELECT\n");

    /* FIXME: we should replace xscanimage with our own  User Select UI */
    system("xscanimage");

    DSM_twCC = TWCC_SUCCESS;
    return twRC;
#endif
}

/* DG_CONTROL/DAT_PARENT/MSG_CLOSEDSM */
TW_UINT16 TWAIN_CloseDSM (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    activeDS *currentDS = activeSources, *nextDS;

    TRACE("DG_CONTROL/DAT_PARENT/MSG_CLOSEDSM\n");

    if (DSM_currentState == 3)
    {
        sane_exit ();
        DSM_initialized = FALSE;
        DSM_parentHWND = 0;
        DSM_currentState = 2;

        /* If there are data sources still open, close them now. */
        while (currentDS != NULL)
        {
            nextDS = currentDS->next;
            sane_close (currentDS->deviceHandle);
            HeapFree (GetProcessHeap(), 0, currentDS);
            currentDS = nextDS;
        }
        activeSources = NULL;
        DSM_twCC = TWCC_SUCCESS;
        twRC = TWRC_SUCCESS;
    }
    else
    {
        DSM_twCC = TWCC_SEQERROR;
        twRC = TWRC_FAILURE;
    }

    return twRC;
#endif
}

/* DG_CONTROL/DAT_PARENT/MSG_OPENDSM */
TW_UINT16 TWAIN_OpenDSM (pTW_IDENTITY pOrigin, TW_MEMREF pData)
{
#ifndef HAVE_SANE
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    SANE_Status status;
    SANE_Int version_code;

    TRACE("DG_CONTROL/DAT_PARENT/MSG_OPENDSM\n");

    if (DSM_currentState == 2)
    {
        if (!DSM_initialized)
        {
            DSM_initialized = TRUE;
            status = sane_init (&version_code, NULL);
            device_list = NULL;
            DSM_currentDevice = 0;
            DSM_sourceId = 0;
        }
        DSM_parentHWND = *(TW_HANDLE*)pData;
        DSM_currentState = 3; /* transition to state 3 */
        DSM_twCC = TWCC_SUCCESS;
        twRC = TWRC_SUCCESS;
    }
    else
    {
        /* operation invoked in invalid state */
        DSM_twCC = TWCC_SEQERROR;
        twRC = TWRC_FAILURE;
    }

    return twRC;
#endif
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
