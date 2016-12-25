/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Hard error message test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

/* TODO: don't require user interaction, test Io* routines,
 *       test NTSTATUS values with special handling */

static
VOID
SetParameters(
    OUT PULONG_PTR Parameters,
    IN INT Count,
    ...)
{
    INT i;
    va_list Arguments;
    va_start(Arguments, Count);
    for (i = 0; i < Count; ++i)
        Parameters[i] = va_arg(Arguments, ULONG_PTR);
    va_end(Arguments);
}

#define NoResponse 27

#define CheckHardError(ErrStatus, UnicodeStringMask, ResponseOption,    \
                        ExpectedStatus, ExpectedResponse,               \
                        NumberOfParameters, ...) do                     \
{                                                                       \
    SetParameters(HardErrorParameters, NumberOfParameters, __VA_ARGS__);\
    Response = NoResponse;                                              \
    _SEH2_TRY {                                                         \
        Status = ExRaiseHardError(ErrStatus,                            \
                                  NumberOfParameters,                   \
                                  UnicodeStringMask,                    \
                                  HardErrorParameters,                  \
                                  ResponseOption,                       \
                                  &Response);                           \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                         \
        Status = _SEH2_GetExceptionCode();                              \
    } _SEH2_END;                                                        \
    ok_eq_hex(Status, ExpectedStatus);                                  \
    ok_eq_ulong(Response, (ULONG)ExpectedResponse);                     \
} while (0)

#define CheckInformationalHardError(ErrStatus, String, Thread,          \
                                        ExpectedStatus, ExpectedRet) do \
{                                                                       \
    Status = STATUS_SUCCESS;                                            \
    Ret = !ExpectedRet;                                                 \
    _SEH2_TRY {                                                         \
        Ret = IoRaiseInformationalHardError(ErrStatus,                  \
                                            String,                     \
                                            Thread);                    \
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {                         \
        Status = _SEH2_GetExceptionCode();                              \
    } _SEH2_END;                                                        \
    ok_eq_hex(Status, ExpectedStatus);                                  \
    ok_eq_bool(Ret, ExpectedRet);                                       \
} while (0)

static
VOID
TestHardError(
    BOOLEAN InteractivePart1,
    BOOLEAN InteractivePart2,
    BOOLEAN InteractivePart3,
    BOOLEAN InteractivePart4)
{
    NTSTATUS Status;
    ULONG Response;
    WCHAR StringBuffer1[] = L"Parameter1+Garbage";
    CHAR StringBuffer1Ansi[] = "Parameter1+Garbage";
    WCHAR StringBuffer2[] = L"Parameter2+Garbage";
    UNICODE_STRING String1 = RTL_CONSTANT_STRING(StringBuffer1);
    ANSI_STRING String1Ansi = RTL_CONSTANT_STRING(StringBuffer1Ansi);
    UNICODE_STRING String2 = RTL_CONSTANT_STRING(StringBuffer2);
    ULONG_PTR HardErrorParameters[6];
    BOOLEAN Ret;

    String1.Length = sizeof L"Parameter1" - sizeof UNICODE_NULL;
    String1Ansi.Length = sizeof "Parameter1" - sizeof ANSI_NULL;
    String2.Length = sizeof L"Parameter2" - sizeof UNICODE_NULL;

    if (InteractivePart1)
    {
    CheckHardError(0x40000000,                  0, OptionOk,                STATUS_SUCCESS,            ResponseOk,             0, 0);                          // outputs a box :|
    CheckHardError(0x40000001,                  0, OptionOk,                STATUS_SUCCESS,            ResponseOk,             4, 1, 2, 3, 4);                 // outputs a box :|
    CheckHardError(0x40000002,                  0, OptionOk,                STATUS_SUCCESS,            ResponseOk,             5, 1, 2, 3, 4, 5);              // outputs a box :|
    CheckHardError(0x40000003,                  0, OptionOk,                STATUS_SUCCESS,            ResponseNotHandled,     6, 1, 2, 3, 4, 5, 6);           // TODO: interactive on ROS
    }

    CheckHardError(0x40000004,                  0, OptionShutdownSystem,    STATUS_PRIVILEGE_NOT_HELD, ResponseNotHandled,     0, 0);
    if (InteractivePart1)
    {
    // TODO: these 2 are interactive on ROS
    CheckHardError(0x40000005,                  0, OptionOkNoWait,          STATUS_SUCCESS,            ResponseOk,             0, 0);                          // outputs a balloon notification
    CheckHardError(0x4000000f,                  0, OptionOkNoWait,          STATUS_SUCCESS,            ResponseOk,             0, 0);                          // outputs a balloon notification
    CheckHardError(0x40000006,                  0, OptionAbortRetryIgnore,  STATUS_SUCCESS,            ResponseAbort,          0, 0);                          // outputs a box :|
    CheckHardError(0x40000006,                  0, OptionAbortRetryIgnore,  STATUS_SUCCESS,            ResponseRetry,          0, 0);                          // outputs a box :|
    CheckHardError(0x40000006,                  0, OptionAbortRetryIgnore,  STATUS_SUCCESS,            ResponseIgnore,         0, 0);                          // outputs a box :|
    CheckHardError(0x40000008,                  0, OptionCancelTryContinue, STATUS_SUCCESS,            ResponseCancel,         0, 0);                          // outputs a box :|
    CheckHardError(0x40000008,                  0, OptionCancelTryContinue, STATUS_SUCCESS,            ResponseTryAgain,       0, 0);                          // outputs a box :|
    CheckHardError(0x40000008,                  0, OptionCancelTryContinue, STATUS_SUCCESS,            ResponseContinue,       0, 0);                          // outputs a box :|
    CheckHardError(0x40000010,                  0, OptionOkCancel,          STATUS_SUCCESS,            ResponseOk,             0, 0);                          // outputs a box :|
    CheckHardError(0x40000010,                  0, OptionOkCancel,          STATUS_SUCCESS,            ResponseCancel,         0, 0);                          // outputs a box :|
    CheckHardError(0x40000011,                  0, OptionRetryCancel,       STATUS_SUCCESS,            ResponseRetry,          0, 0);                          // outputs a box :|
    CheckHardError(0x40000011,                  0, OptionRetryCancel,       STATUS_SUCCESS,            ResponseCancel,         0, 0);                          // outputs a box :|
    CheckHardError(0x40000012,                  0, OptionYesNo,             STATUS_SUCCESS,            ResponseYes,            0, 0);                          // outputs a box :|
    CheckHardError(0x40000012,                  0, OptionYesNo,             STATUS_SUCCESS,            ResponseNo,             0, 0);                          // outputs a box :|
    CheckHardError(0x40000013,                  0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            0, 0);                          // outputs a box :|
    CheckHardError(0x40000013,                  0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             0, 0);                          // outputs a box :|
    CheckHardError(0x40000013,                  0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         0, 0);                          // outputs a box :|
    }
    CheckHardError(0x40000009,                  0, 9,                       STATUS_SUCCESS,            ResponseNotHandled,     0, 0);
    CheckHardError(0x4000000a,                  0, 10,                      STATUS_SUCCESS,            ResponseNotHandled,     0, 0);
    CheckHardError(0x4000000b,                  0, 11,                      STATUS_SUCCESS,            ResponseNotHandled,     0, 0);
    CheckHardError(0x4000000c,                  0, 12,                      STATUS_SUCCESS,            ResponseNotHandled,     0, 0);
    CheckHardError(0x4000000d,                  0, MAXULONG / 2 + 1,        STATUS_SUCCESS,            ResponseNotHandled,     0, 0);
    CheckHardError(0x4000000d,                  0, MAXULONG,                STATUS_SUCCESS,            ResponseNotHandled,     0, 0);

    if (InteractivePart2)
    {
    /* try a message with one parameter */
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            1, &String1);                   // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, &String1);                   // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         0, &String1);                   // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         0, &String1);                   // outputs a box :|
    /* give too many parameters */
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        2, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            4, &String1, &String2, 0, 0);   // outputs a box :|
    /* try with stuff that's not a UNICODE_STRING */
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             1, &String1Ansi);               // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             1, L"Parameter1");              // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             1, "Parameter1");               // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, 1234);                       // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, NULL);                       // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, &String1Ansi);               // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, L"Parameter1");              // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, "Parameter1");               // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, 1234);                       // outputs a box :|
    CheckHardError(STATUS_DLL_NOT_FOUND,        0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, NULL);                       // outputs a box :|
    }
    if (InteractivePart3)
    {
    /* try a message with one parameter */
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, &String1);                   // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, &String1);                   // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     0, &String1);                   // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     0, &String1);                   // outputs a box :|
    /* give too many parameters */
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 2, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseOk,             3, &String1, &String2, 0);      // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseOk,             4, &String1, &String2, 0, 0);   // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 3, OptionOkNoWait,          STATUS_SUCCESS,            ResponseOk,             4, &String1, &String2, 0, 0);   // outputs a balloon notification
    /* try with stuff that's not a UNICODE_STRING */
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, &String1Ansi);               // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, L"Parameter1");              // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, "Parameter1");               // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, 1234);                       // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, NULL);                       // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, &String1Ansi);               // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, L"Parameter1");              // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, "Parameter1");               // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, 1234);                       // outputs a box :|
    CheckHardError(STATUS_SERVICE_NOTIFICATION, 0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNotHandled,     1, NULL);                       // outputs a box :|
    }
    if (InteractivePart4)
    {
    /* try a message with one parameter */
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            1, &String1);                   // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, &String1);                   // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         0, &String1);                   // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         0, &String1);                   // outputs a box :|
    /* give too many parameters */
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       2, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            2, &String1, &String2);         // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       3, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseYes,            4, &String1, &String2, 0, 0);   // outputs a box :|
    /* try with stuff that's not a UNICODE_STRING */
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             1, &String1Ansi);               // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             1, L"Parameter1");              // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseNo,             1, "Parameter1");               // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, 1234);                       // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, NULL);                       // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, &String1Ansi);               // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, L"Parameter1");              // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, "Parameter1");               // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, 1234);                       // outputs a box :|
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseCancel,         1, NULL);                       // outputs a box :|

    // TODO: these 3 are interactive on ROS
    CheckInformationalHardError(STATUS_WAIT_0,               NULL,     NULL, STATUS_SUCCESS, TRUE);                                                            // outputs a balloon notification
    CheckInformationalHardError(STATUS_DLL_NOT_FOUND,        &String1, NULL, STATUS_SUCCESS, TRUE);                                                            // outputs a balloon notification
    CheckInformationalHardError(STATUS_DLL_NOT_FOUND,        NULL,     NULL, STATUS_SUCCESS, TRUE);                                                            // outputs a balloon notification
    }
    CheckInformationalHardError(STATUS_SERVICE_NOTIFICATION, &String1, NULL, STATUS_SUCCESS, FALSE);

    ok_bool_true(IoSetThreadHardErrorMode(TRUE), "IoSetThreadHardErrorMode returned");
    ok_bool_true(IoSetThreadHardErrorMode(FALSE), "IoSetThreadHardErrorMode returned");
    ok_bool_false(IoSetThreadHardErrorMode(FALSE), "IoSetThreadHardErrorMode returned");
    CheckHardError(STATUS_FATAL_APP_EXIT,       0, OptionYesNoCancel,       STATUS_SUCCESS,            ResponseReturnToCaller, 0, 0);
    CheckHardError(STATUS_FATAL_APP_EXIT,       1, OptionYesNoCancel,       STATUS_ACCESS_VIOLATION,   NoResponse,             1, NULL);
    CheckInformationalHardError(STATUS_WAIT_0,               NULL,     NULL, STATUS_SUCCESS, FALSE);
    CheckInformationalHardError(STATUS_DLL_NOT_FOUND,        &String1, NULL, STATUS_SUCCESS, FALSE);
    CheckInformationalHardError(STATUS_DLL_NOT_FOUND,        NULL,     NULL, STATUS_SUCCESS, FALSE);
    CheckInformationalHardError(STATUS_SERVICE_NOTIFICATION, &String1, NULL, STATUS_SUCCESS, FALSE);
    ok_bool_false(IoSetThreadHardErrorMode(TRUE), "IoSetThreadHardErrorMode returned");
}

START_TEST(ExHardError)
{
    TestHardError(FALSE, FALSE, FALSE, FALSE);
}

/* Here's how to do the interactive test:
 * - First there will be a few messages random messages. If there's
 *   multiple options available, the same box will appear multiple times --
 *   click the buttons in order from left to right
 * - After that, you must verify the error parameters. You should always
 *   see Parameter1 or Parameter2 for strings, and 0x12345678 for numbers.
 *   if there's a message saying an exception occured during processing,
 *   click cancel. If there's a bad parameter (Parameter1+, Parameter1+Garbage
 *   or an empty string for example), click no. Otherwise click yes. */
START_TEST(ExHardErrorInteractive)
{
    TestHardError(TRUE, TRUE, TRUE, TRUE);
}
