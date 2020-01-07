/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     debug utils
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#ifndef _DBGUTIL_H_
#define _DBGUTIL_H_

void PrintHexDumpMax(
    IN int length,
    IN PBYTE buffer,
    IN int printmax);
void PrintHexDump(
    IN DWORD length,
    IN PBYTE buffer);

#define trace_ustr(ustr) \
    trace("%s (Buffer 0x%p) %.*S\n", #ustr, (ustr).Buffer, (ustr).Length / sizeof(WCHAR), (ustr).Buffer)
#define trace_ustrp(ustrp) \
    (ustrp != NULL) ? \
    trace_ustr(*ustrp) : \
    trace("%s <NULL>\n", #ustrp)
#define trace_str(str) \
    trace("%s (Buffer 0x%p) %.*s\n", #str, (str).Buffer, (str).Length / sizeof(CHAR), (str).Buffer)
#define trace_strp(strp) \
    (strp != NULL) ? \
    trace_str(*strp) : \
    trace("%s <NULL>\n", #strp)

#endif
