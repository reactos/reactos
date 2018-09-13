//*************************************************************
//
//  Debugging functions header file
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************



//
// Debug Levels
//

#define DL_NONE     0x00000000
#define DL_NORMAL   0x00000001
#define DL_VERBOSE  0x00000002
#define DL_LOGFILE  0x00010000
#define DL_DEBUGGER 0x00020000

extern DWORD dwDebugLevel;


//
// Debug message types
//

#define DM_WARNING  0
#define DM_ASSERT   1
#define DM_VERBOSE  2


//
// Debug macros
//

#define DebugMsg(x) if (dwDebugLevel != DL_NONE) \
                        _DebugMsg x


//
// Debug function proto-types
//


void _DebugMsg(UINT mask, LPCTSTR pszMsg, ...);


#define SETUP_LOAD    1
#define WINLOGON_LOAD 2
void InitDebugSupport( DWORD dwLoadFlags );


#if DBG

#define DmAssert(x) if (!(x)) \
                        _DebugMsg(DM_ASSERT,TEXT("Userenv.dll assertion ") TEXT(#x) TEXT(" failed\n"));

#else

#define DmAssert(x)

#endif // DBG
