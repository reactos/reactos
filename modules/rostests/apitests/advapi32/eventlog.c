/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Supplemental tests for Winetests' Event Logging functions
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include "precomp.h"

START_TEST(eventlog)
{
    static struct
    {
        /* Input */
        ULONG MaxDataSize;

        /* Output for Windows <= 2k3 | Windows Vista+ (or "old" ReactOS) */
        struct
        {
            BOOL  Success;
            DWORD LastError;
        } Result[2];
    } Tests[] =
    {
        /*
         * Tests for the different RPC boundaries on Windows.
         * See also the "ReportEvent" API on MSDN, section "Return value", at:
         * https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-reporteventa
         * for more details.
         */
        { 0xF000, { {TRUE, ERROR_SUCCESS}, {TRUE , ERROR_SUCCESS} } },
        { 0xF001, { {TRUE, ERROR_SUCCESS}, {FALSE, RPC_S_INVALID_BOUND} } },

        { 0x3FF66, { {TRUE, ERROR_SUCCESS}, {FALSE, RPC_S_INVALID_BOUND} } },
        { 0x3FF67, { {TRUE, ERROR_SUCCESS}, {FALSE, RPC_S_INVALID_BOUND} } },
        { 0x3FF68, { {TRUE, ERROR_SUCCESS}, {FALSE, RPC_S_INVALID_BOUND} } },

        /* Show that the maximum data size for an event can be as big as 0x3FFFF */
        { 0x3FFFE, { {TRUE, ERROR_SUCCESS /* or FALSE, ERROR_INVALID_PARAMETER on Win2k3 */}, {FALSE, RPC_S_INVALID_BOUND} } },
        { 0x3FFFF, { {TRUE, ERROR_SUCCESS /* or FALSE, ERROR_INVALID_PARAMETER on Win2k3 */}, {FALSE, RPC_S_INVALID_BOUND} } },
        { 0x40000, { {FALSE, RPC_X_BAD_STUB_DATA}, {FALSE, RPC_S_INVALID_BOUND} } },
    };

    UINT i;
    BOOL Success;
    DWORD LastError;
    HANDLE hEventLog;
    PVOID Data;

    /* We use the "Application" log for the different tests! */
    hEventLog = OpenEventLogW(NULL, L"Application");
    ok(hEventLog != NULL, "OpenEventLogW(NULL, L\"Application\") failed with error %lu\n", GetLastError());
    if (!hEventLog)
        return;

    for (i = 0; i < ARRAYSIZE(Tests); ++i)
    {
        Data = HeapAlloc(GetProcessHeap(), 0, Tests[i].MaxDataSize);
        ok(Data != NULL, "Failed to allocate memory for data of size %lu\n", Tests[i].MaxDataSize);
        if (Data)
        {
            RtlFillMemory(Data, Tests[i].MaxDataSize, 0xCA);

            ClearEventLog(hEventLog, NULL);

            SetLastError(ERROR_SUCCESS);
            Success = ReportEventW(hEventLog, EVENTLOG_INFORMATION_TYPE, 1, 1, NULL, 0, Tests[i].MaxDataSize, NULL, Data);
            LastError = GetLastError();
            /* Small adjustment */
            if (LastError == ERROR_ENVVAR_NOT_FOUND)
                LastError = ERROR_SUCCESS;

            ok( ( (Success == Tests[i].Result[0].Success) && (LastError == Tests[i].Result[0].LastError) ) ||
                broken( (Success == FALSE) && (LastError == ERROR_INVALID_PARAMETER) /* For Win2k3, see above */) // ||
                // broken( (Success == Tests[i].Result[1].Success) && (LastError == Tests[i].Result[1].LastError) /* For Vista+ */)
                ,
               "ReportEventW(%u) returned 0x%x with last error %lu, expected %s with last error %lu\n",
               i, Success, LastError, (Tests[i].Result[0].Success ? "TRUE" : "FALSE"), Tests[i].Result[0].LastError);

            HeapFree(GetProcessHeap(), 0, Data);
        }
    }

    ClearEventLog(hEventLog, NULL);

    CloseEventLog(hEventLog);
}
