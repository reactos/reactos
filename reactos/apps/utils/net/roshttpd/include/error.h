/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/error.h
 */
#ifndef __ERROR_H
#define __ERROR_H

#include <windows.h>

#define TS(x) (LPTSTR)_T(x)

void ReportErrorStr(LPTSTR lpsText);

#endif /* __ERROR_H */
