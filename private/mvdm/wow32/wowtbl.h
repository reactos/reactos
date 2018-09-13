/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, 1992, 1993 Microsoft Corporation
 *
 *  WOWTBL.H
 *  WOW32 API thunk table
 *
--*/



/* thunk table
 */
extern W32 aw32WOW[];


//
// the order of these must not change!  see kernel31\kdata.asm
//
typedef struct {
    WORD    kernel;
    WORD    dkernel;
    WORD    user;
    WORD    duser;
    WORD    gdi;
    WORD    dgdi;
    WORD    keyboard;
    WORD    sound;
    WORD    shell;
    WORD    winsock;
    WORD    toolhelp;
    WORD    mmedia;
    WORD    commdlg;
#ifdef FE_IME
    WORD    winnls;
#endif // FE_IME
#ifdef FE_SB
    WORD    wifeman;
#endif // FE_SB
} TABLEOFFSETS;
typedef TABLEOFFSETS UNALIGNED *PTABLEOFFSETS;


VOID InitThunkTableOffsets(VOID);

extern TABLEOFFSETS tableoffsets;


#ifdef DEBUG_OR_WOWPROFILE

extern PSZ apszModNames[];
extern INT nModNames;
extern INT cAPIThunks;



INT ModFromCallID(INT iFun);
PSZ GetModName(INT iFun);
INT GetOrdinal(INT iFun);
INT TableOffsetFromName(PSZ szTab);


#endif
