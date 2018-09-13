//---------------------------------------------------------------------------
// Isz.h : String resource IDs for WOW32
//
// Copyright (c) Microsoft Corporation, 1990-1995
//---------------------------------------------------------------------------

#define CCH_MAX_STRING_RESOURCE 512

//
// String resource IDs must start at 0 and continue consecutively until
// the the last critical string, so that they can be used to index
// aszCriticalStrings in the most straightforward fashion.
//

#define iszApplicationError        0x0
#define iszTheWin16Subsystem       0x1
#define iszChooseClose             0x2
#define iszChooseCancel            0x3
#define iszChooseIgnore            0x4
#define iszCausedException         0x5
#define iszCausedAV                0x6
#define iszCausedStackOverflow     0x7
#define iszCausedAlignmentFault    0x8
#define iszCausedIllegalInstr      0x9
#define iszCausedInPageError       0xa
#define iszCausedIntDivideZero     0xb
#define iszCausedFloatException    0xc
#define iszChooseIgnoreAlignment   0xd

#define CRITICAL_STRING_COUNT      0xe

#define iszWIN16InternalError      0x100
#define iszSystemError             0x101
#define iszCantEndTask             0x102
#define iszUnableToEndSelTask      0x103
#define iszNotResponding           0x104
#define iszEventHook               0x105
#define iszApplication             0x106
#define iszStartupFailed           0x107
#define iszOLEMemAllocFailedFatal  0x108
#define iszOLEMemAllocFailed       0x109

#define iszWowFaxLocalPort         0x10a

#define iszMisMatchedBinary        0x10b
#define iszMisMatchedBinaryTitle   0x10c

//
// Macro to fetch critical string pointer based on name without preceeding isz
//

#define CRITSTR(name)      (aszCriticalStrings[isz##name])

#ifndef WOW32_C
extern LPSTR aszCriticalStrings[];
#endif
