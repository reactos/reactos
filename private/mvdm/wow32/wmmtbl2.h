/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMMTBL2.h
 *  WOW32 16-bit MultiMedia API tables
 *
 *  History:
 *  Created 21-Jan-1992 by Mike Tricker (miketri], after jeffpar
 *  Gutted  21-Feb-1995 by Dave Hart, after anonymous SYSUK1/SYSUK8
--*/


    {W32FUN(UNIMPLEMENTEDAPI,                 "DUMMYENTRY",               MOD_MMEDIA,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_MMEDIA,   0)},
    {W32FUN(WMM32CallProc32,                  "MMCallProc32",             MOD_MMEDIA,   sizeof(MMCALLPROC3216))},
