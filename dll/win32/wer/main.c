/*
 * Copyright 2010 Louis Lenders
 * Copyright 2010 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "werapi.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wer);

typedef struct {
    struct list entry;
    WER_REPORT_INFORMATION info;
    WER_REPORT_TYPE reporttype;
    WCHAR eventtype[1];
} report_t;


static CRITICAL_SECTION report_table_cs;
static CRITICAL_SECTION_DEBUG report_table_cs_debug =
{
    0, 0, &report_table_cs,
    { &report_table_cs_debug.ProcessLocksList, &report_table_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": report_table_cs") }
};
static CRITICAL_SECTION report_table_cs = { &report_table_cs_debug, -1, 0, 0, 0, 0 };

static struct list report_table = LIST_INIT(report_table);

static const WCHAR regpath_exclude[] = L"Software\\Microsoft\\Windows Error Reporting\\ExcludedApplications";

/***********************************************************************
 * WerAddExcludedApplication (wer.@)
 *
 * Add an application to the user specific or the system wide exclusion list
 *
 * PARAMS
 *  exeName  [i] The application name
 *  allUsers [i] for all users (TRUE) or for the current user (FALSE)
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerAddExcludedApplication(PCWSTR exeName, BOOL allUsers)
{
    HKEY hkey;
    DWORD value = 1;
    LPWSTR bs;

    TRACE("(%s, %d)\n",debugstr_w(exeName), allUsers);
    if (!exeName || !exeName[0])
        return E_INVALIDARG;

    bs = wcsrchr(exeName, '\\');
    if (bs) {
        bs++;   /* skip the backslash */
        if (!bs[0]) {
            return E_INVALIDARG;
        }
    } else
        bs = (LPWSTR) exeName;

    if (!RegCreateKeyW(allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, regpath_exclude, &hkey)) {
        RegSetValueExW(hkey, bs, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD));
        RegCloseKey(hkey);
        return S_OK;
    }
    return E_ACCESSDENIED;
}

/***********************************************************************
 * WerRemoveExcludedApplication (wer.@)
 *
 * remove an application from the exclusion list
 *
 * PARAMS
 *  exeName  [i] The application name
 *  allUsers [i] for all users (TRUE) or for the current user (FALSE)
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerRemoveExcludedApplication(PCWSTR exeName, BOOL allUsers)
{
    HKEY hkey;
    LPWSTR bs;
    LONG lres;

    TRACE("(%s, %d)\n",debugstr_w(exeName), allUsers);
    if (!exeName || !exeName[0])
        return E_INVALIDARG;

    bs = wcsrchr(exeName, '\\');
    if (bs) {
        bs++;   /* skip the backslash */
        if (!bs[0]) {
            return E_INVALIDARG;
        }
    } else
        bs = (LPWSTR) exeName;

    if (!RegCreateKeyW(allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, regpath_exclude, &hkey)) {
        lres = RegDeleteValueW(hkey, bs);
        RegCloseKey(hkey);
        return lres ? __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND) : S_OK;
    }
    return E_ACCESSDENIED;
}

/***********************************************************************
 * WerReportAddDump (wer.@)
 *
 * Add a dump of dumpType to hReportHandle.
 *
 * PARAMS
 *  hReportHandle      [i] error reporting handle to add the dump
 *  hProcess           [i] handle to the regarding process
 *  hThread            [o] handle to the regarding thread
 *  dumpType           [i] type of the dump
 *  pExceptionParam    [o] pointer to a WER_EXCEPTION_INFORMATION
 *  pDumpCustomOptions [o] pointer to a WER_DUMP_CUSTOM_OPTIONS
 *  dwFlags            [i] flag to control the heap dump
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerReportAddDump(HREPORT hReportHandle, HANDLE hProcess, HANDLE hThread,
                                WER_DUMP_TYPE dumpType, PWER_EXCEPTION_INFORMATION pExceptionParam,
                                PWER_DUMP_CUSTOM_OPTIONS pDumpCustomOptions, DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %d, %p, %p, %lu) :stub\n", hReportHandle, hProcess, hThread, dumpType,
          pExceptionParam, pDumpCustomOptions, dwFlags);

    return E_NOTIMPL;
}

/***********************************************************************
 * WerReportAddFile (wer.@)
 *
 * Add a file to an error report handle.
 *
 * PARAMS
 *  hreport [i] error reporting handle to add the file
 *  path    [i] path to the file to add
 *  type    [i] type of the file to add
 *  flags   [i] flags for the file
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerReportAddFile(HREPORT hreport, PCWSTR path, WER_FILE_TYPE type, DWORD flags)
{
    FIXME("(%p, %s, %d, 0x%lx) :stub\n", hreport, debugstr_w(path), type, flags);

    return S_OK;
}

/***********************************************************************
 * WerReportCloseHandle (wer.@)
 *
 * Close an error reporting handle and free associated resources
 *
 * PARAMS
 *  hreport [i] error reporting handle to close
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerReportCloseHandle(HREPORT hreport)
{
    report_t * report = (report_t *) hreport;
    report_t * cursor;
    BOOL found = FALSE;

    TRACE("(%p)\n", hreport);
    EnterCriticalSection(&report_table_cs);
    if (report) {
        LIST_FOR_EACH_ENTRY(cursor, &report_table, report_t, entry)
        {
            if (cursor == report) {
                found = TRUE;
                list_remove(&report->entry);
                break;
            }
        }
    }
    LeaveCriticalSection(&report_table_cs);
    if (!found)
        return E_INVALIDARG;

    free(report);

    return S_OK;
}

/***********************************************************************
 * WerReportCreate (wer.@)
 *
 * Create an error report in memory and return a related HANDLE
 *
 * PARAMS
 *  eventtype  [i] a name for the event type
 *  reporttype [i] what type of report should be created
 *  reportinfo [i] NULL or a ptr to a struct with some detailed information
 *  phandle    [o] ptr, where the resulting handle should be saved
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 * NOTES
 *  The event type must be registered at microsoft. Predefined types are
 *  "APPCRASH" as the default on Windows, "Crash32" and "Crash64"
 *
 */
HRESULT WINAPI WerReportCreate(PCWSTR eventtype, WER_REPORT_TYPE reporttype, PWER_REPORT_INFORMATION reportinfo, HREPORT *phandle)
{
    report_t *report;

    TRACE("(%s, %d, %p, %p)\n", debugstr_w(eventtype), reporttype, reportinfo, phandle);
    if (reportinfo) {
        TRACE(".wzFriendlyEventName: %s\n", debugstr_w(reportinfo->wzFriendlyEventName));
        TRACE(".wzApplicationName: %s\n", debugstr_w(reportinfo->wzApplicationName));
    }

    if (phandle)  *phandle = NULL;
    if (!eventtype || !eventtype[0] || !phandle || (reporttype >= WerReportInvalid)) {
        return E_INVALIDARG;
    }

    report = calloc(1, FIELD_OFFSET(report_t, eventtype[lstrlenW(eventtype) + 1]));
    if (!report)
        return __HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

    lstrcpyW(report->eventtype, eventtype);
    report->reporttype = reporttype;

    if (reportinfo) {
        report->info = *reportinfo;
    } else {
        FIXME("build report information from scratch for %p\n", report);
    }

    EnterCriticalSection(&report_table_cs);
    list_add_head(&report_table, &report->entry);
    LeaveCriticalSection(&report_table_cs);

    *phandle = report;
    TRACE("=> %p\n", report);
    return S_OK;
}

/***********************************************************************
 * WerReportSetParameter (wer.@)
 *
 * Set one of 10 parameter / value pairs for a report handle
 *
 * PARAMS
 *  hreport [i] error reporting handle to add the parameter
 *  id      [i] parameter to set (WER_P0 up to WER_P9)
 *  name    [i] optional name of the parameter
 *  value   [i] value of the parameter
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerReportSetParameter(HREPORT hreport, DWORD id, PCWSTR name, PCWSTR value)
{
    FIXME("(%p, %ld, %s, %s) :stub\n", hreport, id, debugstr_w(name), debugstr_w(value));

    return S_OK;
}

/***********************************************************************
 * WerReportSubmit (wer.@)
 *
 * Ask the user for permission and send the error report
 * then kill or restart the application, when requested
 *
 * PARAMS
 *  hreport [i] error reporting handle to send
 *  consent [i] current transmit permission
 *  flags   [i] flag to select dialog, transmission snd restart options
 *  presult [o] ptr, where the transmission result should be saved
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT error code
 *
 */
HRESULT WINAPI WerReportSubmit(HREPORT hreport, WER_CONSENT consent, DWORD flags, PWER_SUBMIT_RESULT presult)
{
    FIXME("(%p, %d, 0x%lx, %p) :stub\n", hreport, consent, flags, presult);

    if(!presult)
        return E_INVALIDARG;

    *presult = WerDisabled;
    return S_OK;
}

/***********************************************************************
 * WerReportSetUIOption (wer.@)
 */
HRESULT WINAPI WerReportSetUIOption(HREPORT hreport, WER_REPORT_UI uitype, PCWSTR value)
{
    FIXME("(%p, %d, %s) :stub\n", hreport, uitype, debugstr_w(value));
    return E_NOTIMPL;
}
