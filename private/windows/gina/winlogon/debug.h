//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-02-94   RichardW   Created
//
//----------------------------------------------------------------------------


#ifndef __DEBUG_H__
#define __DEBUG_H__

#if DBG

extern  DWORD   WinlogonInfoLevel;
extern  DWORD   GinaBreakFlags;

#define DebugLog(x) LogEvent x


void    LogEvent(long, const char *, ...);
void    InitDebugSupport(void);

#define DEB_ERROR           0x00000001
#define DEB_WARN            0x00000002
#define DEB_TRACE           0x00000004
#define DEB_TRACE_INIT      0x00000008
#define DEB_TRACE_TIMEOUT   0x00000010
#define DEB_TRACE_SAS       0x00000020
#define DEB_TRACE_STATE     0x00000040
#define DEB_TRACE_MPR       0x00000080
#define DEB_COOL_SWITCH     0x00000100
#define DEB_TRACE_PROFILE   0x00000200
#define DEB_DEBUG_LSA       0x00000400
#define DEB_DEBUG_MPR       0x00000800
#define DEB_DEBUG_NOWAIT    0x00001000
#define DEB_TRACE_MIGRATE   0x00002000
#define DEB_DEBUG_SERVICES  0x00004000
#define DEB_TRACE_SETUP     0x00008000
#define DEB_TRACE_SC        0x00010000
#define DEB_TRACE_NOTIFY    0x00020000
#define DEB_TRACE_JOB       0x00040000


#else

#define DebugLog(x)
#define InitDebugSupport()


#endif



#endif // __DEBUG_H__
