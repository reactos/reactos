/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        error.cpp
 * PURPOSE:     Error reporting
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <error.h>
#include <stdio.h>

void ReportErrorStr(LPTSTR lpsText)
{
    wprintf((__wchar_t*)lpsText);
}
