/*
 * TWAIN32 functions
 *
 * Copyright 2000 Shi Quan He <shiquan@cyberdude.com>
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"
#include "twain_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            DSM_currentState = 2;
            break;

        case DLL_PROCESS_DETACH:
            DSM_currentState = 1;
            break;
    }

    return TRUE;
}

TW_UINT16 TWAIN_SourceManagerHandler (
           pTW_IDENTITY pOrigin,
           TW_UINT16   DAT,
           TW_UINT16   MSG,
           TW_MEMREF   pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
        case DAT_IDENTITY:
            switch (MSG)
            {
                case MSG_CLOSEDS:
                    twRC = TWAIN_CloseDS (pOrigin, pData);
                    break;

                case MSG_GETDEFAULT:
                    twRC = TWAIN_IdentityGetDefault (pOrigin, pData);
                    break;

                case MSG_GETFIRST:
                    twRC = TWAIN_IdentityGetFirst (pOrigin, pData);
                    break;

                case MSG_GETNEXT:
                    twRC = TWAIN_IdentityGetNext (pOrigin, pData);
                    break;

                case MSG_OPENDS:
                    twRC = TWAIN_OpenDS (pOrigin, pData);
                    break;

                case MSG_USERSELECT:
                    twRC = TWAIN_UserSelect (pOrigin, pData);
                    break;

                default:
                    /* Unrecognized operation triplet */
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_PARENT:
            switch (MSG)
            {
                case MSG_CLOSEDSM:
                    twRC = TWAIN_CloseDSM (pOrigin, pData);
                    break;

                case MSG_OPENDSM:
                    twRC = TWAIN_OpenDSM (pOrigin, pData);
                    break;

                default:
                    /* Unrecognized operation triplet */
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
            }
            break;

        case DAT_STATUS:
            if (MSG == MSG_GET)
            {
                twRC = TWAIN_GetDSMStatus (pOrigin, pData);
            }
            else
            {
                twRC = TWRC_FAILURE;
                DSM_twCC = TWCC_BADPROTOCOL;
                WARN("unrecognized operation triplet\n");
            }
            break;

        default:
            twRC = TWRC_FAILURE;
            DSM_twCC = TWCC_BADPROTOCOL;
            WARN("unrecognized operation triplet\n");
            break;
    }

    return twRC;
}

TW_UINT16 TWAIN_SourceControlHandler (
           pTW_IDENTITY pOrigin,
           pTW_IDENTITY pDest,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    switch (DAT)
    {
        case DAT_CAPABILITY:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_CapabilityGet (pOrigin, pDest, pData);
                    break;
                case MSG_GETCURRENT:
                    twRC = TWAIN_CapabilityGetCurrent (pOrigin, pDest, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = TWAIN_CapabilityGetDefault (pOrigin, pDest, pData);
                    break;
                case MSG_QUERYSUPPORT:
                    twRC = TWAIN_CapabilityQuerySupport (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_CapabilityReset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_CapabilitySet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    WARN("unrecognized opertion triplet\n");
            }
            break;

        case DAT_CUSTOMDSDATA:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_CustomDSDataGet (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_CustomDSDataSet (pOrigin, pDest, pData);
                    break;
                default:
                    break;
            }
            break;

        case DAT_FILESYSTEM:
            switch (MSG)
            {
                /*case MSG_AUTOMATICCAPTUREDIRECTORY:
                    twRC = TWAIN_AutomaticCaptureDirectory
                               (pOrigin, pDest, pData);
                    break;*/
                case MSG_CHANGEDIRECTORY:
                    twRC = TWAIN_ChangeDirectory (pOrigin, pDest, pData);
                    break;
                /*case MSG_COPY:
                    twRC = TWAIN_FileSystemCopy (pOrigin, pDest, pData);
                    break;*/
                case MSG_CREATEDIRECTORY:
                    twRC = TWAIN_CreateDirectory (pOrigin, pDest, pData);
                    break;
                case MSG_DELETE:
                    twRC = TWAIN_FileSystemDelete (pOrigin, pDest, pData);
                    break;
                case MSG_FORMATMEDIA:
                    twRC = TWAIN_FormatMedia (pOrigin, pDest, pData);
                    break;
                case MSG_GETCLOSE:
                    twRC = TWAIN_FileSystemGetClose (pOrigin, pDest, pData);
                    break;
                case MSG_GETFIRSTFILE:
                    twRC = TWAIN_FileSystemGetFirstFile
                               (pOrigin, pDest, pData);
                    break;
                case MSG_GETINFO:
                    twRC = TWAIN_FileSystemGetInfo (pOrigin, pDest, pData);
                    break;
                case MSG_GETNEXTFILE:
                    twRC = TWAIN_FileSystemGetNextFile
                               (pOrigin, pDest, pData);
                    break;
                case MSG_RENAME:
                    twRC = TWAIN_FileSystemRename (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        case DAT_EVENT:
            if (MSG == MSG_PROCESSEVENT)
                twRC = TWAIN_ProcessEvent (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_PASSTHRU:
            if (MSG == MSG_PASSTHRU)
                twRC = TWAIN_PassThrough (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_PENDINGXFERS:
            switch (MSG)
            {
                case MSG_ENDXFER:
                    twRC = TWAIN_PendingXfersEndXfer (pOrigin, pDest, pData);
                    break;
                case MSG_GET:
                    twRC = TWAIN_PendingXfersGet (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_PendingXfersReset (pOrigin, pDest, pData);
                    break;
                /*case MSG_STOPFEEDER:
                    twRC = TWAIN_PendingXfersStopFeeder
                               (pOrigin, pDest, pData);
                    break;*/
                default:
                    twRC = TWRC_FAILURE;
            }
            break;

        case DAT_SETUPFILEXFER:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_SetupFileXferGet (pOrigin, pDest, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = TWAIN_SetupFileXferGetDefault
                               (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_SetupFileXferReset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_SetupFileXferSet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        /*case DAT_SETUPFILEXFER2:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_SetupFileXfer2Get (pOrigin, pDest, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = TWAIN_SetupFileXfer2GetDefault
                               (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_SetupFileXfer2Reset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_SetupFileXfer2Set (pOrigin, pDest, pData);
                    break;
            }
            break;*/
        case DAT_SETUPMEMXFER:
            if (MSG == MSG_GET)
                twRC = TWAIN_SetupMemXferGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_STATUS:
            if (MSG == MSG_GET)
                twRC = TWAIN_GetDSStatus (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_USERINTERFACE:
            switch (MSG)
            {
                case MSG_DISABLEDS:
                    twRC = TWAIN_DisableDSUserInterface
                               (pOrigin, pDest, pData);
                    break;
                case MSG_ENABLEDS:
                    twRC = TWAIN_EnableDSUserInterface
                               (pOrigin, pDest, pData);
                    break;
                case MSG_ENABLEDSUIONLY:
                    twRC = TWAIN_EnableDSUIOnly (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        case DAT_XFERGROUP:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_XferGroupGet (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_XferGroupSet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    break;
            }
            break;

        default:
            twRC = TWRC_FAILURE;
            break;
    }

    return twRC;
}

TW_UINT16 TWAIN_ControlGroupHandler (
           pTW_IDENTITY pOrigin,
           pTW_IDENTITY pDest,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    if (pDest)
    {
        /* This operation's destination is a source */
        twRC = TWAIN_SourceControlHandler (pOrigin, pDest, DAT, MSG, pData);
    }
    else
    {
        /* This operation's destination is the Source Manager */
        twRC = TWAIN_SourceManagerHandler (pOrigin, DAT, MSG, pData);
    }

    return twRC;
}

TW_UINT16 TWAIN_ImageGroupHandler (
           pTW_IDENTITY pOrigin,
           pTW_IDENTITY pDest,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;

    if (pDest == NULL)
    {
        DSM_twCC = TWCC_BADDEST;
        return TWRC_FAILURE;
    }

    switch (DAT)
    {
        case DAT_CIECOLOR:
            if (MSG == MSG_GET)
                twRC = TWAIN_CIEColorGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_EXTIMAGEINFO:
            if (MSG == MSG_GET)
                twRC = TWAIN_ExtImageInfoGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_GRAYRESPONSE:
            switch (MSG)
            {
                case MSG_RESET:
                    twRC = TWAIN_GrayResponseReset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_GrayResponseSet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;
        case DAT_IMAGEFILEXFER:
            if (MSG == MSG_GET)
                twRC = TWAIN_ImageFileXferGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGEINFO:
            if (MSG == MSG_GET)
                twRC = TWAIN_ImageInfoGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGELAYOUT:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_ImageLayoutGet (pOrigin, pDest, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = TWAIN_ImageLayoutGetDefault (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_ImageLayoutReset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_ImageLayoutSet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_IMAGEMEMXFER:
            if (MSG == MSG_GET)
                twRC = TWAIN_ImageMemXferGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_IMAGENATIVEXFER:
            if (MSG == MSG_GET)
                twRC = TWAIN_ImageNativeXferGet (pOrigin, pDest, pData);
            else
                twRC = TWRC_FAILURE;
            break;

        case DAT_JPEGCOMPRESSION:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_JPEGCompressionGet (pOrigin, pDest, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = TWAIN_JPEGCompressionGetDefault
                               (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_JPEGCompressionReset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_JPEGCompressionSet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        case DAT_PALETTE8:
            switch (MSG)
            {
                case MSG_GET:
                    twRC = TWAIN_Palette8Get (pOrigin, pDest, pData);
                    break;
                case MSG_GETDEFAULT:
                    twRC = TWAIN_Palette8GetDefault (pOrigin, pDest, pData);
                    break;
                case MSG_RESET:
                    twRC = TWAIN_Palette8Reset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_Palette8Set (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
            }
            break;

        case DAT_RGBRESPONSE:
            switch (MSG)
            {
                case MSG_RESET:
                    twRC = TWAIN_RGBResponseReset (pOrigin, pDest, pData);
                    break;
                case MSG_SET:
                    twRC = TWAIN_RGBResponseSet (pOrigin, pDest, pData);
                    break;
                default:
                    twRC = TWRC_FAILURE;
                    DSM_twCC = TWCC_BADPROTOCOL;
                    WARN("unrecognized operation triplet\n");
                    break;
            }
            break;

        default:
            twRC = TWRC_FAILURE;
            DSM_twCC = TWCC_BADPROTOCOL;
            WARN("unrecognized operation triplet\n");
    }
    return twRC;
}

TW_UINT16 TWAIN_AudioGroupHandler (
           pTW_IDENTITY pOrigin,
           pTW_IDENTITY pDest,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_FAILURE;

    switch (DAT)
    {
        case DAT_AUDIOFILEXFER:
            if (MSG == MSG_GET)
                twRC = TWAIN_AudioFileXferGet (pOrigin, pDest, pData);
            break;

        case DAT_AUDIOINFO:
            if (MSG == MSG_GET)
                twRC = TWAIN_AudioInfoGet (pOrigin, pDest, pData);
            break;

        case DAT_AUDIONATIVEXFER:
            if (MSG == MSG_GET)
                twRC = TWAIN_AudioNativeXferGet (pOrigin, pDest, pData);
            break;

        default:
            DSM_twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
            break;
    }

    return twRC;
}

/* Main entry point for the TWAIN library */
TW_UINT16 WINAPI
DSM_Entry (pTW_IDENTITY pOrigin,
           pTW_IDENTITY pDest,
           TW_UINT32    DG,
           TW_UINT16    DAT,
           TW_UINT16    MSG,
           TW_MEMREF    pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;  /* Return Code */

    TRACE("(DG=%ld DAT=%d MSG=%d)\n", DG, DAT, MSG);

    switch (DG)
    {
        case DG_CONTROL:
            twRC = TWAIN_ControlGroupHandler (pOrigin,pDest,DAT,MSG,pData);
            break;
        case DG_IMAGE:
            twRC = TWAIN_ImageGroupHandler (pOrigin,pDest,DAT,MSG,pData);
            break;
        case DG_AUDIO:
            twRC = TWAIN_AudioGroupHandler (pOrigin,pDest,DAT,MSG,pData);
            break;
        default:
            DSM_twCC = TWCC_BADPROTOCOL;
            twRC = TWRC_FAILURE;
    }

    return twRC;
}

/* A helper function that looks up a destination identity in the active
   source list */
activeDS *TWAIN_LookupSource (pTW_IDENTITY pDest)
{
    activeDS *pSource;

    for (pSource = activeSources; pSource; pSource = pSource->next)
        if (pSource->identity.Id == pDest->Id)
            break;

    return pSource;
}
