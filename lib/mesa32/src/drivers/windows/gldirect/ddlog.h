/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x (Win32)
*
* Description:  Logging functions.
*
****************************************************************************/

#ifndef __DDLOG_H
#define __DDLOG_H

#include <stdio.h>

#ifndef _USE_GLD3_WGL
#include "dderrstr.h" // ddraw/d3d error string
#endif

/*---------------------- Macros and type definitions ----------------------*/

typedef enum {
	DDLOG_NONE					= 0,			// No log output
	DDLOG_NORMAL				= 1,			// Log is kept open
	DDLOG_CRASHPROOF			= 2,			// Log is closed and flushed
	DDLOG_METHOD_FORCE_DWORD	= 0x7fffffff,
} DDLOG_loggingMethodType;

// Denotes type of message sent to the logging functions
typedef enum {
	DDLOG_INFO					= 0,			// Information only
	DDLOG_WARN					= 1,			// Warning only
	DDLOG_ERROR					= 2,			// Notify user of an error
	DDLOG_CRITICAL				= 3,			// Exceptionally severe error
	DDLOG_SYSTEM				= 4,			// System message. Not an error
												// but must always be printed.
	DDLOG_SEVERITY_FORCE_DWORD	= 0x7fffffff,	// Make enum dword
} DDLOG_severityType;

#ifdef _USE_GLD3_WGL
// Synomyms
#define GLDLOG_INFO		DDLOG_INFO
#define GLDLOG_WARN		DDLOG_WARN
#define GLDLOG_ERROR	DDLOG_ERROR
#define GLDLOG_CRITICAL	DDLOG_CRITICAL
#define GLDLOG_SYSTEM	DDLOG_SYSTEM
#endif

// The message that will be output to the log
static const char *ddlogSeverityMessages[] = {
	"INFO",
	"WARN",
	"ERROR",
	"*CRITICAL*",
	"System",
};

/*------------------------- Function Prototypes ---------------------------*/

#ifdef  __cplusplus
extern "C" {
#endif

void ddlogOpen(DDLOG_loggingMethodType LoggingMethod, DDLOG_severityType Severity);
void ddlogClose();
void ddlogMessage(DDLOG_severityType severity, LPSTR message);
void ddlogError(DDLOG_severityType severity, LPSTR message, HRESULT hResult);
void ddlogPrintf(DDLOG_severityType severity, LPSTR message, ...);
void ddlogWarnOption(BOOL bWarnOption);
void ddlogPathOption(LPSTR szPath);

#ifdef _USE_GLD3_WGL
// Synomyms
#define gldLogMessage	ddlogMessage
#define gldLogError		ddlogError
#define gldLogPrintf	ddlogPrintf
#endif

#ifdef  __cplusplus
}
#endif

#endif
