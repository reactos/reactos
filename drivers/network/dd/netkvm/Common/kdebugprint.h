/*
 * This file contains debug-related definitions for kernel driver
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**********************************************************************
WARNING: this file is incompatible with Logo requirements
TODO:    Optional WPP technique
**********************************************************************/

#ifndef _K_DEBUG_PRINT_H
#define _K_DEBUG_PRINT_H

extern int nDebugLevel;
extern int bDebugPrint;


typedef void (*DEBUGPRINTFUNC)(const char *fmt, ...);
extern  DEBUGPRINTFUNC pDebugPrint;

void _LogOutEntry(int level, const char *s);
void _LogOutExitValue(int level, const char *s, ULONG value);
void _LogOutString(int level, const char *s);

#define DEBUG_ENTRY(level)  _LogOutEntry(level, __FUNCTION__)
#define DEBUG_EXIT_STATUS(level, status)  _LogOutExitValue(level, __FUNCTION__, status)
#define DPrintFunctionName(Level) _LogOutString(Level, __FUNCTION__)


#ifndef WPP_EVENT_TRACING

#define WPP_INIT_TRACING(a,b)
#define WPP_CLEANUP(a)

#define MAX_DEBUG_LEVEL 1

#define DPrintf(Level, Fmt) { if ( (Level) > MAX_DEBUG_LEVEL || (Level) > nDebugLevel || !bDebugPrint ) {} else { pDebugPrint Fmt; } }

#define DPrintfBypass(Level, Fmt) DPrintf(Level, Fmt)

#else

//#define WPP_USE_BYPASS


#define DPrintfAnyway(Level, Fmt) \
{ \
    if (bDebugPrint && (Level) <= nDebugLevel) \
    { \
        pDebugPrint Fmt; \
    } \
}

//{05F77115-E57E-49bf-90DF-C0E6B6478E5F}
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(NetKVM, (05F77115,E57E,49bf,90DF,C0E6B6478E5F),  \
        WPP_DEFINE_BIT(TRACE_DEBUG)\
                                    )


#define WPP_LEVEL_ENABLED(LEVEL) \
    (nDebugLevel >= (LEVEL))

#define WPP_LEVEL_LOGGER(LEVEL)      (WPP_CONTROL(WPP_BIT_ ## TRACE_DEBUG).Logger),


#if WPP_USE_BYPASS
#define DPrintfBypass(Level, Fmt) DPrintfAnyway(Level, Fmt)
#else
#define DPrintfBypass(Level, Fmt)
#endif

#define WPP_PRIVATE_ENABLE_CALLBACK     WppEnableCallback

extern VOID WppEnableCallback(
    __in LPCGUID Guid,
    __in __int64 Logger,
    __in BOOLEAN Enable,
    __in ULONG Flags,
    __in UCHAR Level);


#endif
#endif
