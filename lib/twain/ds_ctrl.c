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

//#include "config.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include "twain.h"
#include "twain_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_CONTROL/DAT_CAPABILITY/MSG_GET */
TW_UINT16 TWAIN_CapabilityGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                               TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GET\n");

    if (!pDest)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState < 4 || pSource->currentState > 7)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = TWAIN_SaneCapability (pSource, pCapability, MSG_GET);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        pSource->twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT */
TW_UINT16 TWAIN_CapabilityGetCurrent (pTW_IDENTITY pOrigin,
                                      pTW_IDENTITY pDest,TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GETCURRENT\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState < 4 || pSource->currentState > 7)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = TWAIN_SaneCapability (pSource, pCapability, MSG_GETCURRENT);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        pSource->twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT */
TW_UINT16 TWAIN_CapabilityGetDefault (pTW_IDENTITY pOrigin,
                                      pTW_IDENTITY pDest, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_GETDEFAULT\n");

    if (!pDest)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState < 4 || pSource->currentState > 7)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = TWAIN_SaneCapability (pSource, pCapability, MSG_GETDEFAULT);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        pSource->twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_QUERYSUPPORT */
TW_UINT16 TWAIN_CapabilityQuerySupport (pTW_IDENTITY pOrigin,
                                        pTW_IDENTITY pDest, TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_RESET */
TW_UINT16 TWAIN_CapabilityReset (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                 TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_CAPABILITY/MSG_RESET\n");

    if (!pDest)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState < 4 || pSource->currentState > 7)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = TWAIN_SaneCapability (pSource, pCapability, MSG_RESET);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        pSource->twCC = twCC;
    }

    return twRC;
}

/* DG_CONTROL/DAT_CAPABILITY/MSG_SET */
TW_UINT16 TWAIN_CapabilitySet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                               TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS, twCC = TWCC_SUCCESS;
    pTW_CAPABILITY pCapability = (pTW_CAPABILITY) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE ("DG_CONTROL/DAT_CAPABILITY/MSG_SET\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState != 4)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        twCC = TWAIN_SaneCapability (pSource, pCapability, MSG_SET);
        twRC = (twCC == TWCC_SUCCESS)?TWRC_SUCCESS:TWRC_FAILURE;
        pSource->twCC = twCC;
    }
    return twRC;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_GET */
TW_UINT16 TWAIN_CustomDSDataGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_CUSTOMDSDATA/MSG_SET */
TW_UINT16 TWAIN_CustomDSDataSet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_AUTOMATICCAPTUREDIRECTORY */
TW_UINT16 TWAIN_AutomaticCaptureDirectory (pTW_IDENTITY pOrigin,
                                           pTW_IDENTITY pDest,
                                           TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CHANGEDIRECTORY */
TW_UINT16 TWAIN_ChangeDirectory (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_COPY */
TW_UINT16 TWAIN_FileSystemCopy (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_CREATEDIRECTORY */
TW_UINT16 TWAIN_CreateDirectory (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_DELETE */
TW_UINT16 TWAIN_FileSystemDelete (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_FORMATMEDIA */
TW_UINT16 TWAIN_FormatMedia (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETCLOSE */
TW_UINT16 TWAIN_FileSystemGetClose (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETFIRSTFILE */
TW_UINT16 TWAIN_FileSystemGetFirstFile (pTW_IDENTITY pOrigin,
                                        pTW_IDENTITY pDest,
                                        TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETINFO */
TW_UINT16 TWAIN_FileSystemGetInfo (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_GETNEXTFILE */
TW_UINT16 TWAIN_FileSystemGetNextFile (pTW_IDENTITY pOrigin,
                                       pTW_IDENTITY pDest,
                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_FILESYSTEM/MSG_RENAME */
TW_UINT16 TWAIN_FileSystemRename (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT */
TW_UINT16 TWAIN_ProcessEvent (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_EVENT pEvent = (pTW_EVENT) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_EVENT/MSG_PROCESSEVENT\n");

    if (!pSource)
    {
         twRC = TWRC_FAILURE;
         DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState < 5 || pSource->currentState > 7)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        if (pSource->pendingEvent.TWMessage != MSG_NULL)
        {
            pEvent->TWMessage = pSource->pendingEvent.TWMessage;
            pSource->pendingEvent.TWMessage = MSG_NULL;
            twRC = TWRC_DSEVENT;
        }
        else
        {
            pEvent->TWMessage = MSG_NULL;  /* no message to the application */
            twRC = TWRC_NOTDSEVENT;
        }
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PASSTHRU/MSG_PASSTHRU */
TW_UINT16 TWAIN_PassThrough (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER */
TW_UINT16 TWAIN_PendingXfersEndXfer (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                     TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;
    activeDS *pSource = TWAIN_LookupSource (pData);

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_ENDXFER\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState != 6 && pSource->currentState != 7)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        if (pPendingXfers->Count != 0)
        {
            pPendingXfers->Count --;
            pSource->currentState = 6;
        }
        else
        {
            pSource->currentState = 5;
            /* Notify the application that it can close the data source */
            pSource->pendingEvent.TWMessage = MSG_CLOSEDSREQ;
        }
        twRC = TWRC_SUCCESS;
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_GET */
TW_UINT16 TWAIN_PendingXfersGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                 TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_GET\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState < 4 || pSource->currentState > 7)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_SEQERROR;
    }
    else
    {
        /* FIXME: we shouldn't return 1 here */
        pPendingXfers->Count = 1;
        twRC = TWRC_SUCCESS;
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET */
TW_UINT16 TWAIN_PendingXfersReset (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                   TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_PENDINGXFERS pPendingXfers = (pTW_PENDINGXFERS) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_PENDINGXFERS/MSG_RESET\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState != 6)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_SEQERROR;
    }
    else
    {
        pPendingXfers->Count = 0;
        pSource->currentState = 5;
        twRC = TWRC_SUCCESS;
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_PENDINGXFERS/MSG_STOPFEEDER */
TW_UINT16 TWAIN_PendingXfersStopFeeder (pTW_IDENTITY pOrigin,
                                        pTW_IDENTITY pDest, TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_GET */
TW_UINT16 TWAIN_SetupFileXferGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPXFER/MSG_GETDEFAULT */
TW_UINT16 TWAIN_SetupFileXferGetDefault (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}


/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_RESET */
TW_UINT16 TWAIN_SetupFileXferReset (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER/MSG_SET */
TW_UINT16 TWAIN_SetupFileXferSet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_GET */
TW_UINT16 TWAIN_SetupFileXfer2Get (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_GETDEFAULT */
TW_UINT16 TWAIN_SetupFileXfer2GetDefault (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                         TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_RESET */
TW_UINT16 TWAIN_SetupFileXfer2Reset (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPFILEXFER2/MSG_SET */
TW_UINT16 TWAIN_SetupFileXfer2Set (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_SETUPMEMXFER/MSG_GET */
TW_UINT16 TWAIN_SetupMemXferGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_STATUS/MSG_GET */
TW_UINT16 TWAIN_GetDSStatus (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                             TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_FAILURE;
    pTW_STATUS pSourceStatus = (pTW_STATUS) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE ("DG_CONTROL/DAT_STATUS/MSG_GET\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
        pSourceStatus->ConditionCode = TWCC_BADDEST;
    }
    else
    {
        twRC = TWRC_SUCCESS;
        pSourceStatus->ConditionCode = pSource->twCC;
        /* Reset the condition code */
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS */
TW_UINT16 TWAIN_DisableDSUserInterface (pTW_IDENTITY pOrigin,
                                        pTW_IDENTITY pDest, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_DISABLEDS\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState != 5)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        pSource->currentState = 4;
        twRC = TWRC_SUCCESS;
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS */
TW_UINT16 TWAIN_EnableDSUserInterface (pTW_IDENTITY pOrigin,
                                       pTW_IDENTITY pDest, TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_USERINTERFACE pUserInterface = (pTW_USERINTERFACE) pData;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE ("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDS\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState != 4)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        if (pUserInterface->ShowUI)
        {
            pSource->currentState = 5; /* Transitions to state 5 */
            /* FIXME: we should replace xscanimage with our own device UI */
            system ("xscanimage");
            pSource->currentState = 6;
            pSource->pendingEvent.TWMessage = MSG_XFERREADY;
        }
        else
        {
            /* no UI will be displayed, so source is ready to transfer data */
            pSource->pendingEvent.TWMessage = MSG_XFERREADY;
            pSource->currentState = 6; /* Transitions to state 6 directly */
        }

        pSource->hwndOwner = pUserInterface->hParent;
        twRC = TWRC_SUCCESS;
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY */
TW_UINT16 TWAIN_EnableDSUIOnly (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    activeDS *pSource = TWAIN_LookupSource (pDest);

    TRACE("DG_CONTROL/DAT_USERINTERFACE/MSG_ENABLEDSUIONLY\n");

    if (!pSource)
    {
        twRC = TWRC_FAILURE;
        DSM_twCC = TWCC_BADDEST;
    }
    else if (pSource->currentState != 4)
    {
        twRC = TWRC_FAILURE;
        pSource->twCC = TWCC_SEQERROR;
    }
    else
    {
        /* FIXME: we should replace xscanimage with our own UI */
        system ("xscanimage");
        pSource->currentState = 5;
        twRC = TWRC_SUCCESS;
        pSource->twCC = TWCC_SUCCESS;
    }

    return twRC;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_GET */
TW_UINT16 TWAIN_XferGroupGet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                              TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_CONTROL/DAT_XFERGROUP/MSG_SET */
TW_UINT16 TWAIN_XferGroupSet (pTW_IDENTITY pOrigin, pTW_IDENTITY pDest,
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}
