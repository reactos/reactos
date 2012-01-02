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

#define STRICT
#include <windows.h>

#include "ddlog.h"
#include "gld_driver.h"

// ***********************************************************************

static char						ddlogbuf[256];
static FILE*					fpDDLog = NULL; // Log file pointer
static char						szDDLogName[_MAX_PATH] = {"gldirect.log"}; 	// Filename of the log
static DDLOG_loggingMethodType	ddlogLoggingMethod = DDLOG_NONE; 	// Default to No Logging
static DDLOG_severityType		ddlogDebugLevel;
static BOOL						bUIWarning = FALSE;	// MessageBox warning ?

// ***********************************************************************

void ddlogOpen(
	DDLOG_loggingMethodType LoggingMethod,
	DDLOG_severityType Severity)
{
	if (fpDDLog != NULL) {
		// Tried to re-open the log
		ddlogMessage(DDLOG_WARN, "Tried to re-open the log file\n");
		return;
	}

	ddlogLoggingMethod = LoggingMethod;
	ddlogDebugLevel = Severity;

	if (ddlogLoggingMethod == DDLOG_NORMAL) {
		fpDDLog = fopen(szDDLogName, "wt");
        if (fpDDLog == NULL)
            return;
    }

	ddlogMessage(DDLOG_SYSTEM, "\n");
	ddlogMessage(DDLOG_SYSTEM, "-> Logging Started\n");
}

// ***********************************************************************

void ddlogClose()
{
	// Determine whether the log is already closed
	if (fpDDLog == NULL && ddlogLoggingMethod == DDLOG_NORMAL)
		return; // Nothing to do.

	ddlogMessage(DDLOG_SYSTEM, "<- Logging Ended\n");

	if (ddlogLoggingMethod == DDLOG_NORMAL) {
		fclose(fpDDLog);
		fpDDLog = NULL;
	}
}

// ***********************************************************************

void ddlogMessage(
	DDLOG_severityType severity,
	LPSTR message)
{
	char buf[256];

	// Bail if logging is disabled
	if (ddlogLoggingMethod == DDLOG_NONE)
		return;

	if (ddlogLoggingMethod == DDLOG_CRASHPROOF)
		fpDDLog = fopen(szDDLogName, "at");

	if (fpDDLog == NULL)
		return;

	if (severity >= ddlogDebugLevel) {
		sprintf(buf, "DDLog: (%s) %s", ddlogSeverityMessages[severity], message);
		fputs(buf, fpDDLog); // Write string to file
		OutputDebugString(buf); // Echo to debugger
	}

	if (ddlogLoggingMethod == DDLOG_CRASHPROOF) {
		fflush(fpDDLog); // Write info to disk
		fclose(fpDDLog);
		fpDDLog = NULL;
	}

	// Popup message box if critical error
	if (bUIWarning && severity == DDLOG_CRITICAL) {
		MessageBox(NULL, buf, "GLDirect", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
	}
}

// ***********************************************************************

// Write a string value to the log file
void ddlogError(
	DDLOG_severityType severity,
	LPSTR message,
	HRESULT hResult)
{
#ifdef _USE_GLD3_WGL
	char dxErrStr[1024];
	_gldDriver.GetDXErrorString(hResult, &dxErrStr[0], sizeof(dxErrStr));
	if (FAILED(hResult)) {
		sprintf(ddlogbuf, "DDLog: %s %8x:[ %s ]\n", message, hResult, dxErrStr);
	} else
		sprintf(ddlogbuf, "DDLog: %s\n", message);
#else
	if (FAILED(hResult)) {
		sprintf(ddlogbuf, "DDLog: %s %8x:[ %s ]\n", message, hResult, DDErrorToString(hResult));
	} else
		sprintf(ddlogbuf, "DDLog: %s\n", message);
#endif
	ddlogMessage(severity, ddlogbuf);
}

// ***********************************************************************

void ddlogPrintf(
	DDLOG_severityType severity,
	LPSTR message,
	...)
{
	va_list args;

	va_start(args, message);
	vsprintf(ddlogbuf, message, args);
	va_end(args);

	lstrcat(ddlogbuf, "\n");

	ddlogMessage(severity, ddlogbuf);
}

// ***********************************************************************

void ddlogWarnOption(
	BOOL bWarnOption)
{
	bUIWarning = bWarnOption;
}

// ***********************************************************************

void ddlogPathOption(
	LPSTR szPath)
{
	char szPathName[_MAX_PATH];

	strcpy(szPathName, szPath);
    strcat(szPathName, "\\");
    strcat(szPathName, szDDLogName);
    strcpy(szDDLogName, szPathName);
}

// ***********************************************************************
