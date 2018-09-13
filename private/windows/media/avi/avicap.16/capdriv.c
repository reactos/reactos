/****************************************************************************
 *
 *   capdriv.c
 * 
 *   Smartdrv control.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and 
 *    distribute the Sample Files (and/or any modified version) in 
 *    any way you find useful, provided that you agree that 
 *    Microsoft has no warranty obligations or liability for any 
 *    Sample Application Files which are modified. 
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>

#pragma optimize ("", off)

// SD_CACHE_DRIVE:
// FUNCTION:
//        Enables and disables read or write caching for a particular
//        drive unit.  Returns the cache state of the drive in DL. Get
//        takes no action, but simply returns cache state for drive unit
//        in DL.
//
//        INPUT:
//                AX=MULT_SMARTDRV  (4A10h)
//                BX=SD_CACHE_DRIVE (3)
//                DL=CACHE_DRIVE_<get,read|write enable|disable>
//                BP=unit number of drive
//        OUTPUT:
//                DL=cache state of unit:
//                        Bit 7 set -> no caching enabled for this unit
//                        Bit 7 not set -> read caching enabled for this unit
//                        Bit 6 set -> write caching not enabled for this unit
//                        Bit 6 not set -> write caching enabled for this unit
//                        -1 -> not a cachable drive
//        USES:
//                ALL except DS,ES
//

#define MULT_SMARTDRV               0x4a10
#define SD_CACHE_DRIVE              3
#define CACHE_DRIVE_GET             0
#define CACHE_DRIVE_READ_ENABLE     1
#define CACHE_DRIVE_READ_DISABLE    2
#define CACHE_DRIVE_WRITE_ENABLE    3
#define CACHE_DRIVE_WRITE_DISABLE   4

#define F_WRITE_CACHE  (1 << 7)
#define F_READ_CACHE   (1 << 6)

WORD NEAR PASCAL SmartDrvCache(int iDrive, BYTE cmd)
{
    WORD    w;

    _asm {
        push    bp
        mov     ax, MULT_SMARTDRV
        mov     bx, SD_CACHE_DRIVE
        mov     dl, cmd
        mov     bp, iDrive
        int     2fh
        mov     al,dl
        xor     ah,ah
        pop     bp
        mov     w,ax
    }

    return w;
}

WORD FAR PASCAL SmartDrv(char chDrive, WORD w)
{
    WORD wCur;
    int  iDrive;

    iDrive = (chDrive | 0x20) - 'a';

    wCur = SmartDrvCache(iDrive, CACHE_DRIVE_GET);

    if (w & F_WRITE_CACHE)
        SmartDrvCache(iDrive, CACHE_DRIVE_WRITE_DISABLE);
    else
        SmartDrvCache(iDrive, CACHE_DRIVE_WRITE_ENABLE);

    if (w & F_READ_CACHE)
        SmartDrvCache(iDrive, CACHE_DRIVE_READ_DISABLE);
    else
        SmartDrvCache(iDrive, CACHE_DRIVE_READ_ENABLE);

    return wCur;
}

#pragma optimize ("", on)

