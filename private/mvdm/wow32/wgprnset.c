/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGPRNSET.C
 *  WOW32 printer setup support routines
 *
 *  These routines help a Win 3.0 task to complete the printer set-up,
 *  when a user initiates the printer setup from the file menu of an
 *  application.
 *
 *  History:
 *  Created 18-Apr-1991 by Chandan Chauhan (ChandanC)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wgprnset.c);

DLLENTRYPOINTS  spoolerapis[WOW_SPOOLERAPI_COUNT] =  {"EXTDEVICEMODE", NULL,
                                    "DEVICEMODE", NULL,
                                    "DEVICECAPABILITIES", NULL,
                                    "OpenPrinterA", NULL,
                                    "StartDocPrinterA", NULL,
                                    "StartPagePrinter", NULL,
                                    "EndPagePrinter", NULL,
                                    "EndDocPrinter", NULL,
                                    "ClosePrinter", NULL,
                                    "WritePrinter", NULL,
                                    "DeletePrinter", NULL,
                                    "GetPrinterDriverDirectoryA", NULL,
                                    "AddPrinterA", NULL,
                                    "AddPrinterDriverA", NULL,
                                    "AddPortExA",NULL};


/****************************************************************************
*                                                                           *
*  ULONG FASTCALL   WG32DeviceMode (PVDMFRAME pFrame)                                *
*                                                                           *
*  (hWnd, hModule, lpDeviceName, lpOutPut)                                  *
*                                                                           *
*   This function passes WDevMode structure (which is per wow task) to      *
*   Win32 printer driver ExtDeviceMode API. This structure is then          *
*   initialized by the printer driver based on the user input.              *
*                                                                           *
*   Later on, when a WOW task creates a dc (by CreateDC API), the device    *
*   mode (WDevMode) structure associated with this wow task is passed along *
*   with the CreateDC API. Which contains the printer setup information     *
*   needed to print the document.                                           *
*                                                                           *
****************************************************************************/
ULONG FASTCALL   WG32DeviceMode (PVDMFRAME pFrame)
{

    register PDEVICEMODE16 parg16;
    PSZ      psz3 = NULL;
    PSZ      psz4 = NULL;
    ULONG    l    = 0;
    HWND     hwnd32;

    GETARGPTR(pFrame, sizeof(DEVICEMODE16), parg16);

    // copy all 16-bit params now since 16-bit memory may move if this calls
    // into a 16-bit fax driver
    hwnd32 = HWND32(parg16->f1);

    if(parg16->f3) {
        if(!(psz3 = malloc_w_strcpy_vp16to32(parg16->f3, FALSE, 0)))  
            goto ExitPath;
    }
    if(parg16->f4) {
        if(!(psz4 = malloc_w_strcpy_vp16to32(parg16->f4, FALSE, 0)))  
            goto ExitPath;
    }


    // invalidate all flat ptrs to 16:16 memory now!
    FREEARGPTR(parg16);

    if (!(*spoolerapis[WOW_DEVICEMODE].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            goto ExitPath;
        }
    }

    // this can callback into a 16-bit fax driver!
    (*spoolerapis[WOW_DEVICEMODE].lpfn)(hwnd32, NULL, psz3, psz4);

    l = 1;

ExitPath:
    if(psz3) {
        free_w(psz3);
    }
    if(psz4) {
        free_w(psz4);
    }

    RETURN(l);  // DeviceMode returns void. Charisma checks the return value!
}





/*****************************************************************************
*                                                                            *
*  ULONG FASTCALL   WG32ExtDeviceMode (PVDMFRAME pFrame)                              *
*                                                                            *
*  INT     (hWnd, hDriver, lpDevModeOutput, lpDeviceName, lpPort,            *
*                     lpDevModeInput, lpProfile, wMode)                      *
*                                                                            *
*   This function is same as DeviceMode except that the wow task supplies    *
*   a DeviceMode structure. Apart from it, this API can be called in         *
*   different modes.                                                         *
*                                                                            *
*****************************************************************************/
ULONG FASTCALL   WG32ExtDeviceMode (PVDMFRAME pFrame)
{
    UINT      cb;
    LONG      l = 0;
    HWND      hWnd1;
    WORD      wMode8;
    PSZ       psz4 = NULL;
    PSZ       psz5 = NULL;
    PSZ       psz7 = NULL;
    VPVOID    vpdm3, vpdm6;
    LPDEVMODE lpdmInput6;
    LPDEVMODE lpdmOutput3;
    register  PEXTDEVICEMODE16 parg16;


    GETARGPTR(pFrame, sizeof(EXTDEVICEMODE16), parg16);

    // copy the 16-bit parameters into local vars since this may callback
    // into a 16-bit fax driver and cause 16-bit memory to move
    hWnd1  = HWND32(parg16->f1);
    vpdm3  = FETCHDWORD(parg16->f3);
    vpdm6  = FETCHDWORD(parg16->f6);
    wMode8 = FETCHWORD(parg16->f8);

    if(parg16->f4) {
        if(!(psz4 = malloc_w_strcpy_vp16to32(parg16->f4, FALSE, 0)))  
            goto ExitPath;
    }
    if(parg16->f5) {
        if(!(psz5 = malloc_w_strcpy_vp16to32(parg16->f5, FALSE, 0)))  
            goto ExitPath;
    }
    if(parg16->f7) {
        if(!(psz7 = malloc_w_strcpy_vp16to32(parg16->f7, FALSE, 0)))
            goto ExitPath;
    }

    FREEARGPTR(parg16);
    // all flat ptrs to 16:16 memory are now invalid!!

    if (!(*spoolerapis[WOW_EXTDEVICEMODE].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            goto ExitPath;
        }
    }

    lpdmInput6 = ThunkDevMode16to32(FETCHDWORD(vpdm6));

    /* if they want output buffer size OR they want to fill output buffer */
    if( (wMode8 == 0) || (wMode8 & DM_OUT_BUFFER) ) {

        /* get required size for output buffer */
        l = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(hWnd1,
                                                   NULL,
                                                   NULL,
                                                   psz4,
                                                   psz5,
                                                   lpdmInput6,
                                                   psz7,
                                                   0);

        // adjust size for WOW handling (see notes in wstruc.c)
        if(l > 0) {
            l += sizeof(WOWDM31);
            cb = (UINT)l;
        }

        /* if caller wants output buffer filled... */
        if( (wMode8 != 0) && (vpdm3 != 0L) && l > 0 ) {

            if( lpdmOutput3 = malloc_w(l) ) {

                l = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(hWnd1,
                                                           NULL,
                                                           lpdmOutput3,
                                                           psz4,
                                                           psz5,
                                                           lpdmInput6,
                                                           psz7,
                                                           wMode8);

                /* Data in lpdmOutput3 is only valid with IDOK return. */
                if( l == IDOK ) {

                    // do our WOW magic on this before we give it to the app
                    ThunkDevMode32to16(vpdm3, lpdmOutput3, cb);
                }

                free_w(lpdmOutput3);
            }
            else {
                l = -1L;
            }
        }
    }

    /* else call for cases where they don't want to fill the output buffer */
    else {

        l = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(hWnd1,
                                                   NULL,
                                                   NULL,
                                                   psz4,
                                                   psz5,
                                                   lpdmInput6,
                                                   psz7,
                                                   wMode8);
    }

    if( lpdmInput6 ) {
        free_w(lpdmInput6);
    }

ExitPath:
    if(psz4) {
        free_w(psz4);
    }
    if(psz5) {
        free_w(psz5);
    }
    if(psz7) {
        free_w(psz7);
    }

    RETURN((ULONG)l);

}




ULONG FASTCALL   WG32DeviceCapabilities (PVDMFRAME pFrame)
{
    LONG      l=0L, cb;
    WORD      fwCap3;
    PBYTE     pOutput4, pOutput32;
    VPVOID    vpOutput4;
    PSZ       psz1 = NULL;
    PSZ       psz2 = NULL;
    LPDEVMODE lpdmInput5;
    DWORD     dwDM5;
    register  PDEVICECAPABILITIES16 parg16;

    GETARGPTR(pFrame, sizeof(DEVICECAPABILITIES16), parg16);

    // copy the 16-bit parameters into local vars since this may callback
    // into a 16-bit fax driver and cause 16-bit memory to move
    if(parg16->f1) {
        if(!(psz1 = malloc_w_strcpy_vp16to32(parg16->f1, FALSE, 0)))  
            goto ExitPath;
    }
    if(parg16->f2) {
        if(!(psz2 = malloc_w_strcpy_vp16to32(parg16->f2, FALSE, 0)))  
            goto ExitPath;
    }

    fwCap3 = FETCHWORD(parg16->f3);

    vpOutput4 = FETCHDWORD(parg16->f4);

    dwDM5 = FETCHDWORD(parg16->f5);

    FREEARGPTR(parg16);
    // all flat ptrs to 16:16 memory are now invalid!!

    if (!(*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", 
                                            spoolerapis, 
                                            WOW_SPOOLERAPI_COUNT)) {
            goto ExitPath;
        }
    }

    lpdmInput5 = ThunkDevMode16to32(dwDM5);

    LOGDEBUG(LOG_TRACE, ("WG32DeviceCapabilities %d\n", fwCap3));

    switch (fwCap3) {

        // These ones do not fill up an output Buffer

        case DC_FIELDS:
        case DC_DUPLEX:
        case DC_SIZE:
        case DC_EXTRA:
        case DC_VERSION:
        case DC_DRIVER:
        case DC_TRUETYPE:
        case DC_ORIENTATION:
        case DC_COPIES:

            l = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1,
                                                            psz2, 
                                                            fwCap3, 
                                                            NULL, 
                                                            lpdmInput5);

            LOGDEBUG(LOG_TRACE, ("WG32DeviceCapabilities simple case returned %d\n", l));

            // adjust for WOW handling of devmodes  // see notes in wstruc.c
            if(fwCap3 == DC_SIZE) {
            
                // we always convert NT DevModes to Win3.1 DevModes
                WOW32WARNMSGF((l==sizeof(DEVMODE)),
                              ("WG32DeviceCapabilities: Unexpected DevMode size: %d\n",l));
                if(l == sizeof(DEVMODE)) {
                    l = sizeof(DEVMODE31); 
                }
            }
            // adjust DriverExtra to allow for difference between NT devmodes
            // & Win3.1 devmodes + our secret WOW stuff at the end
            else if(fwCap3 == DC_EXTRA) {
                l += WOW_DEVMODEEXTRA;
            }
            // we tell them Win3.1 for the spec version too
            else if(fwCap3 == DC_VERSION) {
                l = WOW_DEVMODE31SPEC; // tell 'em the spec version is Win3.1
            }

            break;

#ifdef DBCS
// not supported the following indexes.
    case DC_MINEXTENT:
    case DC_MAXEXTENT:

#ifdef DBCS_LATER
#if DBG
LOGDEBUG(0,("WG32DeviceCapabilities more complicated:"));
#endif
        pOutput = malloc_w(8);

        if (pOutput) {
            l = DEVICECAPABILITIES(psz1, psz2, parg16->f3, pOutput, pdmInput5);

            if (l >= 0) {
#if DBG
LOGDEBUG(0,("Copying %d points from %0x to %0x\n", l, pOutput, pb4));
#endif
                putpoint16(parg16->f4, 1, pOutput);
            }
            free_w(pOutput);
        }
        else {
            l = -1;
        }
#endif // DBCS_LATER
        l = -1;         // always error return
        break;
#endif // DBCS

        // These require an output buffer
        case DC_PAPERS:
        case DC_PAPERSIZE:
#ifndef DBCS
        case DC_MINEXTENT:
        case DC_MAXEXTENT:
#endif // !DBCS
        case DC_BINS:
        case DC_BINNAMES:
        case DC_ENUMRESOLUTIONS:
        case DC_FILEDEPENDENCIES:
        case DC_PAPERNAMES:
    
            LOGDEBUG(LOG_TRACE, ("WG32DeviceCapabilities more complicated:\n"));

            // We've got to figure out how much memory we will need
            GETMISCPTR(vpOutput4, pOutput4);
            if (pOutput4) { 

                cb = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1, 
                                                                 psz2, 
                                                                 fwCap3, 
                                                                 NULL, 
                                                                 lpdmInput5);

                FREEPSZPTR(pOutput4); // invalidate -16bit memory may have moved

                LOGDEBUG(LOG_TRACE, ("we need %d bytes ", cb));

                if (cb > 0) {

                    switch (fwCap3) {

                        case DC_PAPERS:
                            cb *= 2;
                            break;

                        case DC_BINNAMES:
                            cb *= 24;
                            break;

                        case DC_BINS:
                            cb*=2;
                            break;
    
                        case DC_FILEDEPENDENCIES:
                        case DC_PAPERNAMES:
                            cb *= 64;
                            break;

                        case DC_MAXEXTENT:
                        case DC_MINEXTENT:
                        case DC_PAPERSIZE:
                            cb *= 8;

                            LOGDEBUG(LOG_TRACE, ("DC_PAPERSIZE called: Needed %d bytes\n", cb));

                            break;

                        case DC_ENUMRESOLUTIONS:
                            cb *= sizeof(LONG)*2;
                            break;

                    } // end switch

                    pOutput32 = malloc_w(cb);

                    if (pOutput32) {

                        l = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1, psz2, fwCap3, pOutput32, lpdmInput5);

                        if (l >= 0) {

                            GETMISCPTR(vpOutput4, pOutput4);

                            switch (fwCap3) {

                                case DC_PAPERS:
                                    if (CURRENTPTD()->dwWOWCompatFlags &
                                        WOWCF_RESETPAPER29ANDABOVE) {

                                        // wordperfect for windows 5.2 GPs if
                                        // papertype is > 0x28. so reset such 
                                        // paper types to 0x1. In particular 
                                        // this happens if the selected printer
                                        // is Epson LQ-510.
                                        //                       - nanduri

                                        LONG i = l;
                                        while(i--) {
                                            if (((LPWORD)pOutput32)[i] > 0x28) {
                                                ((LPWORD)pOutput32)[i] = 0x1;
                                            }
                                        }
                                    } // end if

                                    RtlCopyMemory(pOutput4, pOutput32, cb);
                                    break;

                                case DC_MAXEXTENT:
                                case DC_MINEXTENT:
                                case DC_PAPERSIZE:
                                    LOGDEBUG(LOG_TRACE, ("Copying %d points from %0x to %0x\n", l, pOutput32, pOutput4));

                                    putpoint16(vpOutput4, l,(LPPOINT)pOutput32);
                                    break;

                                default:
                                    LOGDEBUG(LOG_TRACE, ("Copying %d bytes from %0x to %0x\n",cb, pOutput32, pOutput4));

                                    RtlCopyMemory(pOutput4, pOutput32, cb);
                                    break;

                            } // end switch

                            FLUSHVDMPTR(vpOutput4, (USHORT)cb, pOutput4);
                            FREEPSZPTR(pOutput4);

                        } // end if

                        free_w(pOutput32);

                    } else
                        l = -1;
                } else
                    l = cb;


            } else {


                l = (*spoolerapis[WOW_DEVICECAPABILITIES].lpfn)(psz1,
                                                                psz2, 
                                                                fwCap3, 
                                                                NULL, 
                                                                lpdmInput5);

                LOGDEBUG(LOG_TRACE, ("No Output buffer specified: Returning %d\n", l));
            }

            break;

        default:
            LOGDEBUG(LOG_TRACE, ("!!!! WG32DeviceCapabilities unhandled %d\n", fwCap3));
            l = -1L;
            break;

    } // end switch

    if (lpdmInput5) {
        free_w(lpdmInput5);
    }
ExitPath:
    if(psz1) {
        free_w(psz1);
    }
    if(psz2) {
        free_w(psz2);
    }

    RETURN(l);
}




BOOL LoadLibraryAndGetProcAddresses(char *name, DLLENTRYPOINTS *p, int num)
{
    int     i;
    HINSTANCE   hInst;

    if (!(hInst = SafeLoadLibrary (name))) {
        WOW32ASSERTMSGF (FALSE, ("WOW::LoadLibraryAndGetProcAddresses: LoadLibrary('%s') failed\n", name));
        return FALSE;
    }

    for (i = 0; i < num ; i++) {
        p[i].lpfn = (void *) GetProcAddress (hInst, (p[i].name));
        WOW32ASSERTMSGF (p[i].lpfn, ("WOW::LoadLibraryAndGetProcAddresses: GetProcAddress(%s, '%s') failed\n", name, p[i].name));
    }

    return TRUE;
}

#ifdef i386
/*
 * "Safe" version of LoadLibrary which preserves floating-point state
 * across the load.  This is critical on x86 because the FP state being
 * preserved is the 16-bit app's state.  MSVCRT.DLL is one offender which
 * changes the Precision bits in its Dll init routine.
 *
 * On RISC, this is an alias for LoadLibrary
 *
 */
HINSTANCE SafeLoadLibrary(char *name)
{
    HINSTANCE hInst;
    BYTE FpuState[108];

    // Save the 487 state
    _asm {
        lea    ecx, [FpuState]
        fsave  [ecx]
    }

    hInst = LoadLibrary(name);

    // Restore the 487 state
    _asm {
        lea    ecx, [FpuState]
        frstor [ecx]
    }

    return hInst;
}
#endif  //i386
