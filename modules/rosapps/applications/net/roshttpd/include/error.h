/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 */
#ifndef __ERROR_H
#define __ERROR_H

#include <windows.h>

#define TS(x) (LPTSTR)TEXT(x)

void ReportErrorStr(LPTSTR lpsText);

#endif /* __ERROR_H */
