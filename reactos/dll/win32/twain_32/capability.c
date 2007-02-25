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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "twain.h"
#include "twain_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

TW_UINT16 TWAIN_SaneCapability (activeDS *pSource, pTW_CAPABILITY pCapability,
                                TW_UINT16 action)
{
    TW_UINT16 twCC = TWCC_SUCCESS;

    TRACE("capability=%d action=%d\n", pCapability->Cap, action);

    switch (pCapability->Cap)
    {
        case CAP_DEVICEEVENT:
        case CAP_ALARMS:
        case CAP_ALARMVOLUME:
        case ACAP_AUDIOFILEFORMAT:
        case ACAP_XFERMECH:
        case ICAP_AUTOMATICBORDERDETECTION:
        case ICAP_AUTOMATICDESKEW:
        case ICAP_AUTODISCARDBLANKPAGES:
        case ICAP_AUTOMATICROTATE:
        case ICAP_FLIPROTATION:
        case CAP_AUTOMATICCAPTURE:
        case CAP_TIMEBEFOREFIRSTCAPTURE:
        case CAP_TIMEBETWEENCAPTURES:
        case CAP_AUTOSCAN:
        case CAP_CLEARBUFFERS:
        case CAP_MAXBATCHBUFFERS:
        case ICAP_BARCODEDETECTIONENABLED:
        case ICAP_SUPPORTEDBARCODETYPES:
        case ICAP_BARCODEMAXSEARCHPRIORITIES:
        case ICAP_BARCODESEARCHPRIORITIES:
        case ICAP_BARCODESEARCHMODE:
        case ICAP_BARCODEMAXRETRIES:
        case ICAP_BARCODETIMEOUT:
        case CAP_EXTENDEDCAPS:
        case CAP_SUPPORTEDCAPS:
        case ICAP_FILTER:
        case ICAP_GAMMA:
        case ICAP_PLANARCHUNKY:
        case ICAP_BITORDERCODES:
        case ICAP_CCITTKFACTOR:
        case ICAP_COMPRESSION:
        case ICAP_JPEGPIXELTYPE:
        /*case ICAP_JPEGQUALITY:*/
        case ICAP_PIXELFLAVORCODES:
        case ICAP_TIMEFILL:
        case CAP_DEVICEONLINE:
        case CAP_DEVICETIMEDATE:
        case CAP_SERIALNUMBER:
        case ICAP_EXPOSURETIME:
        case ICAP_FLASHUSED2:
        case ICAP_IMAGEFILTER:
        case ICAP_LAMPSTATE:
        case ICAP_LIGHTPATH:
        case ICAP_NOISEFILTER:
        case ICAP_OVERSCAN:
        case ICAP_PHYSICALHEIGHT:
        case ICAP_PHYSICALWIDTH:
        case ICAP_UNITS:
        case ICAP_ZOOMFACTOR:
        case CAP_PRINTER:
        case CAP_PRINTERENABLED:
        case CAP_PRINTERINDEX:
        case CAP_PRINTERMODE:
        case CAP_PRINTERSTRING:
        case CAP_PRINTERSUFFIX:
        case CAP_AUTHOR:
        case CAP_CAPTION:
        case CAP_TIMEDATE:
        case ICAP_AUTOBRIGHT:
        case ICAP_BRIGHTNESS:
        case ICAP_CONTRAST:
        case ICAP_HIGHLIGHT:
        case ICAP_ORIENTATION:
        case ICAP_ROTATION:
        case ICAP_SHADOW:
        case ICAP_XSCALING:
        case ICAP_YSCALING:
        case ICAP_BITDEPTH:
        case ICAP_BITDEPTHREDUCTION:
        case ICAP_BITORDER:
        case ICAP_CUSTHALFTONE:
        case ICAP_HALFTONES:
        case ICAP_PIXELFLAVOR:
        case ICAP_PIXELTYPE:
        case ICAP_THRESHOLD:
        case CAP_LANGUAGE:
        case ICAP_FRAMES:
        case ICAP_MAXFRAMES:
        case ICAP_SUPPORTEDSIZES:
        case CAP_AUTOFEED:
        case CAP_CLEARPAGE:
        case CAP_FEEDERALIGNMENT:
        case CAP_FEEDERENABLED:
        case CAP_FEEDERLOADED:
        case CAP_FEEDERORDER:
        case CAP_FEEDPAGE:
        case CAP_PAPERBINDING:
        case CAP_PAPERDETECTABLE:
        case CAP_REACQUIREALLOWED:
        case CAP_REWINDPAGE:
        case ICAP_PATCHCODEDETECTIONENABLED:
        case ICAP_SUPPORTEDPATCHCODETYPES:
        case ICAP_PATCHCODEMAXSEARCHPRIORITIES:
        case ICAP_PATCHCODESEARCHPRIORITIES:
        case ICAP_PATCHCODESEARCHMODE:
        case ICAP_PATCHCODEMAXRETRIES:
        case ICAP_PATCHCODETIMEOUT:
        case CAP_BATTERYMINUTES:
        case CAP_BATTERYPERCENTAGE:
        case CAP_POWERDOWNTIME:
        case CAP_POWERSUPPLY:
        case ICAP_XNATIVERESOLUTION:
        case ICAP_XRESOLUTION:
        case ICAP_YNATIVERESOLUTION:
        case ICAP_YRESOLUTION:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        case CAP_XFERCOUNT:
            /* This is a required capability that every source need to
               support but we havev't implemented yet. */
            twCC = TWCC_SUCCESS;
            break;
        /*case ICAP_COMPRESSION:*/
        case ICAP_IMAGEFILEFORMAT:
        case ICAP_TILES:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        case ICAP_XFERMECH:
            twCC = TWAIN_ICAPXferMech (pSource, pCapability, action);
            break;
        case ICAP_UNDEFINEDIMAGESIZE:
        case CAP_CAMERAPREVIEWUI:
        case CAP_ENABLEDSUIONLY:
        case CAP_INDICATORS:
        case CAP_UICONTROLLABLE:
            twCC = TWCC_CAPUNSUPPORTED;
            break;
        default:
            twCC = TWRC_FAILURE;

    }

    return twCC;
}

TW_BOOL TWAIN_OneValueSet (pTW_CAPABILITY pCapability, TW_UINT32 value)
{
    pCapability->hContainer = (TW_HANDLE)GlobalAlloc (0, sizeof(TW_ONEVALUE));

    if (pCapability->hContainer)
    {
        pTW_ONEVALUE pVal = GlobalLock ((HGLOBAL) pCapability->hContainer);
        pVal->ItemType = TWTY_UINT32;
        pVal->Item = value;
        GlobalUnlock ((HGLOBAL) pCapability->hContainer);
        return TRUE;
    }
    else
        return FALSE;
}

TW_BOOL TWAIN_OneValueGet (pTW_CAPABILITY pCapability, TW_UINT32 *pValue)
{
    pTW_ONEVALUE pVal = GlobalLock ((HGLOBAL) pCapability->hContainer);

    if (pVal)
    {
        *pValue = pVal->Item;
        GlobalUnlock ((HGLOBAL) pCapability->hContainer);
        return TRUE;
    }
    else
        return FALSE;
}

/* ICAP_XFERMECH */
TW_UINT16 TWAIN_ICAPXferMech (activeDS *pSource, pTW_CAPABILITY pCapability,
                              TW_UINT16 action)
{
    TRACE("ICAP_XFERMECH\n");

    switch (action)
    {
        case MSG_GET:
            if (pCapability->ConType == TWON_ONEVALUE)
            {
                if (!TWAIN_OneValueSet (pCapability, pSource->capXferMech))
                    return TWCC_LOWMEMORY;
            }
            break;
        case MSG_SET:
            if (pCapability->ConType == TWON_ONEVALUE)
            {
		TW_UINT32 xfermechtemp;
                if (!TWAIN_OneValueGet (pCapability, &xfermechtemp))
                    return TWCC_LOWMEMORY;
		pSource->capXferMech = xfermechtemp;
            }
            else if (pCapability->ConType == TWON_ENUMERATION)
            {

            }
            break;
        case MSG_GETCURRENT:
            if (!TWAIN_OneValueSet (pCapability, pSource->capXferMech))
                return TWCC_LOWMEMORY;
            break;
        case MSG_GETDEFAULT:
            if (!TWAIN_OneValueSet (pCapability, TWSX_NATIVE))
                return TWCC_LOWMEMORY;
            break;
        case MSG_RESET:
            pSource->capXferMech = TWSX_NATIVE;
            break;
    }
    return TWCC_SUCCESS;
}
