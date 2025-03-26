/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Reparse points test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>
#include "IoCreateFile.h"

static CHAR MountedPointFileName[] = "\\Device\\Kmtest-IoCreateFile\\MountedPoint";
static CHAR SymlinkedFileName[] = "\\Device\\Kmtest-IoCreateFile\\Symlinked";
static CHAR NonSymlinkedFileName[] = "\\Device\\Kmtest-IoCreateFile\\NonSymlinked";

START_TEST(IoCreateFile)
{
    DWORD Error;

    KmtRunKernelTest("IoCreateFile");

    Error = KmtLoadAndOpenDriver(L"IoCreateFile", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;

    Error = KmtSendStringToDriver(IOCTL_CALL_CREATE, NonSymlinkedFileName);
    ok(Error == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lx\n", Error);
    Error = KmtSendStringToDriver(IOCTL_CALL_CREATE, SymlinkedFileName);
    ok(Error == ERROR_CANT_ACCESS_FILE, "Expected ERROR_CANT_ACCESS_FILE, got %lx\n", Error); /* FIXME */
    Error = KmtSendStringToDriver(IOCTL_CALL_CREATE, MountedPointFileName);
    ok(Error == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lx\n", Error);

    Error = KmtSendToDriver(IOCTL_DISABLE_SYMLINK);
    ok(Error == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lx\n", Error);

    Error = KmtSendStringToDriver(IOCTL_CALL_CREATE, NonSymlinkedFileName);
    ok(Error == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lx\n", Error);
    Error = KmtSendStringToDriver(IOCTL_CALL_CREATE, SymlinkedFileName);
    ok(Error == ERROR_MR_MID_NOT_FOUND, "Expected ERROR_MR_MID_NOT_FOUND, got %lx\n", Error);
    Error = KmtSendStringToDriver(IOCTL_CALL_CREATE, MountedPointFileName);
    ok(Error == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lx\n", Error);

    KmtCloseDriver();
    KmtUnloadDriver();
}
