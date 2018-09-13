/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Imports.h

Abstract:


Author:

    David J. Gilman (davegi) 04-May-1992

Environment:

    Win32, User Mode

--*/

#if ! defined( _IMPORTS_ )
#define _IMPORTS_

#include "vapi.h"

typedef struct imports {
    gdap_t (FAR PASCAL *_VapGdaGet)( void );
    WORD   (FAR PASCAL *_VapQGetEnv)(char far *, char far *, int);
    unsigned    (FAR PASCAL *_VapCbAllocSb)(SB, unsigned, int);
    unsigned    (FAR PASCAL *_VapCbReallocSb)(SB, unsigned, int);
    unsigned long   (FAR PASCAL *_VapLcbAllocSb)(SB, unsigned long, int);
    unsigned long   (FAR PASCAL *_VapLcbReallocSb)(SB, unsigned long, int);
    VOID    (FAR PASCAL *_VapFreeSb)(SB);
    SB      (FAR PASCAL *_VapSbScanNext)(SB, SB, int);
    unsigned    (FAR PASCAL *_VapCbSizeSb)(SB);
    BOOL    (FAR PASCAL *_VapFUsedSb)( SB );
    VOID FAR *  (FAR PASCAL *_VapLpLockSb)( SB );
    VOID    (FAR PASCAL *_VapUnlockSb)( SB );
    VOID FAR *  (FAR PASCAL *_VapLpLockHp)(HP);
    VOID (FAR PASCAL *_VapUnlockHp)(HP);
    int  (FAR PASCAL        *_VapUpdateFileName)(char far *, int, int);
    int  (FAR PASCAL        *_VapUpdateLineCount)(int, int, int);
    int  (FAR   cdecl   *_VapIoOpen)(char far *, int, ...);
    int  (FAR   cdecl   *_VapIoClose)(int);
    int  (FAR   cdecl   *_VapIoRead)(int, char far *, unsigned int);
    int  (FAR   cdecl   *_VapIoWrite)(int, char far *, unsigned int);
    int  (FAR   cdecl   *_VapVioOpen)(char far *, int, ...);
    int  (FAR   cdecl   *_VapVioClose)(int);
    int  (FAR   cdecl   *_VapVioRead)(int, char far *, unsigned int);
    int  (FAR   cdecl   *_VapVioWrite)(int, char far *, unsigned int);
    int  (FAR   cdecl   *_VapVioCntl)(int, int, ...);
    long (FAR   cdecl   *_VapVioSeek)(int, long, int);
    long (FAR   cdecl   *_VapIoSeek)(int, long, int);
    int  (FAR   cdecl   *_VapIoGetFTime)(int, FTIMEDATE far *);
    BOOL (FAR   PASCAL  *_KnLoadBasedSeg)(SB, UCHAR FAR *, UCHAR FAR *, USHORT);
    void (FAR   cdecl   *_GlobalDump)(int);
    void (FAR   cdecl   *_GlobalDump1)(int, int);
    void (FAR   cdecl   *_SbDump)(int);
    void (FAR   cdecl   *_SbDump1)(int, int);

} IMPORTS;

#endif // _IMPORTS_
