//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       printwrp.cxx
//
//  Contents:   Wrappers for special printing-specific wrappers for the
//              Trident project.
//
//              Any Unicode parameters/structure fields/buffers are converted
//              to ANSI, and then the corresponding ANSI version of the function
//              is called.
//
//              The printing-specific feature of these wrappers is their direct
//              relationship to the fact that we avoid converting the DEVMODE
//              structure back and forth, i.e. on unicode platforms we operate
//              with DEVMODEW, and on non-unicode platforms with DEVMODEA.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef X_PRINTWRP_HXX_
#define X_PRINTWRP_HXX_
#include "printwrp.hxx"
#endif

#ifdef WIN16
#ifndef X_DEFS16_H_
#define X_DEFS16_H_
#include "defs16.h"
#endif
#else
#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif
#endif // WIN16

// IMPORTANT: We are not converting the DEVMODE structure back and forth
// from ASCII to Unicode on Win95 anymore because we are not touching the
// two strings or any other member.  Converting the DEVMODE structure can
// be tricky because of potential and common discrepancies between the
// value of the dmSize member and sizeof(DEVMODE).  (25155)

HDC WINAPI
CreateDCForPrintingInternal(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
#ifndef WIN16
    if (g_fUnicodePlatform)
    {
        return CreateDCW(lpszDriver, lpszDevice, lpszOutput, lpInitData);
    }
    else
    {
        CStrIn      strDriver(lpszDriver);
        CStrIn      strDevice(lpszDevice);
        CStrIn      strOutput(lpszOutput);

        return CreateDCA(strDriver, strDevice, strOutput, (CONST DEVMODEA *)lpInitData);
    }
#else
    return CreateDC(lpszDriver, lpszDevice, lpszOutput, (CONST DEVMODE *)lpInitData);
#endif // !WIN16
}


HDC WINAPI
CreateICForPrintingInternal(
        LPCWSTR             lpszDriver,
        LPCWSTR             lpszDevice,
        LPCWSTR             lpszOutput,
        CONST DEVMODEW *    lpInitData)
{
#ifndef WIN16
    if (g_fUnicodePlatform)
    {
        return CreateICW(lpszDriver, lpszDevice, lpszOutput, lpInitData);
    }
    else
    {
        CStrIn      strDriver(lpszDriver);
        CStrIn      strDevice(lpszDevice);
        CStrIn      strOutput(lpszOutput);

        return CreateICA(strDriver, strDevice, strOutput, (CONST DEVMODEA *)lpInitData);
    }
#else
    return CreateIC(lpszDriver, lpszDevice, lpszOutput, (CONST DEVMODE *)lpInitData);
#endif // !WIN16
}

#ifndef WIN16
DVTARGETDEVICE *
TargetDeviceWFromTargetDeviceA( DVTARGETDEVICE *ptd )
{
    // NOTE: Only the DEVMODE structure is in the wrong (ascii) format!

    DEVMODEA  *         lpdma = NULL;
    DVTARGETDEVICE *    ptdW  = NULL;

    if (!ptd || !ptd->tdExtDevmodeOffset)
        goto Cleanup;

    lpdma = (DEVMODEA *) (((BYTE *)ptd) + ptd->tdExtDevmodeOffset);

    // If the reported size is too small for our conception of a DEVMODEA, don't risk a GPF
    // in our code and bail out now.
    if ( (DWORD)lpdma->dmSize + lpdma->dmDriverExtra < offsetof(DEVMODEA, dmLogPixels) )
        goto Cleanup;

    ptdW = (DVTARGETDEVICE *) CoTaskMemAlloc( ptd->tdSize + (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME) );

    if (ptdW)
    {
        // Copy the entire structure up to DEVMODE part.
        memcpy(ptdW, ptd, ptd->tdExtDevmodeOffset);

        // Account for the increase of the two DEVMODE unicode strings.
        ptdW->tdSize += (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);

        // Convert the devmode structure.
        {
            DEVMODEW  * lpdmw = (DEVMODEW *) (((BYTE *)ptdW) + ptdW->tdExtDevmodeOffset);
            CStrOut     strDeviceName( lpdmw->dmDeviceName, CCHDEVICENAME );
            CStrOut     strFormName( lpdmw->dmFormName, CCHFORMNAME );

            // Copy the first string (CCHDEVICENAME).
            lstrcpyA( strDeviceName, (LPCSTR)lpdma->dmDeviceName );
            strDeviceName.ConvertIncludingNul();

            // Copy the gap between strings.
            memcpy( &lpdmw->dmSpecVersion,
                    &lpdma->dmSpecVersion,
                    offsetof(DEVMODEA, dmFormName) -
                    offsetof(DEVMODEA, dmSpecVersion) );

            // Copy the second string (CCHFORMNAME).
            lstrcpyA( strFormName, (LPCSTR)lpdma->dmFormName );
            strFormName.ConvertIncludingNul();

            // Copy the last part including the driver-specific DEVMODE part (dmDriverExtra).
            memcpy( &lpdmw->dmLogPixels,
                    &lpdma->dmLogPixels,
                    (DWORD)lpdma->dmSize + lpdma->dmDriverExtra -
                    offsetof(DEVMODEA, dmLogPixels) );

            // Correct the dmSize member by accounting for larger unicode strings.
            lpdmw->dmSize += (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);
        }
    }

Cleanup:
    
    return ptdW;
}
#endif // !WIN16

