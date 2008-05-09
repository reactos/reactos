/*
 * Unit tests for service functions
 *
 * Copyright (c) 2007 Paul Vriens
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winsvc.h"
#include "winnls.h"
#include "lmcons.h"

#include "wine/test.h"

static const CHAR spooler[] = "Spooler"; /* Should be available on all platforms */

static void test_open_scm(void)
{
    SC_HANDLE scm_handle;

    /* No access rights */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, 0);
    ok(scm_handle != NULL, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3, Vista */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Unknown database name */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, "DoesNotExist", SC_MANAGER_CONNECT);
    ok(!scm_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle); /* Just in case */

    /* MSDN says only ServiceActive is allowed, or NULL */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, SERVICES_FAILED_DATABASEA, SC_MANAGER_CONNECT);
    ok(!scm_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_DATABASE_DOES_NOT_EXIST, "Expected ERROR_DATABASE_DOES_NOT_EXIST, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle); /* Just in case */

    /* Remote unknown host */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA("DOESNOTEXIST", SERVICES_ACTIVE_DATABASEA, SC_MANAGER_CONNECT);
    ok(!scm_handle, "Expected failure\n");
    todo_wine
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE, "Expected RPC_S_SERVER_UNAVAILABLE, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle); /* Just in case */

    /* Proper call with an empty hostname */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA("", SERVICES_ACTIVE_DATABASEA, SC_MANAGER_CONNECT);
    ok(scm_handle != NULL, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS          /* W2K3, Vista */ ||
       GetLastError() == ERROR_ENVVAR_NOT_FOUND /* NT4 */ ||
       GetLastError() == 0xdeadbeef             /* XP */ ||
       GetLastError() == ERROR_IO_PENDING       /* W2K */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING, ERROR_ENVVAR_NOT_FOUND or 0xdeadbeef, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Again a correct one */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    ok(scm_handle != NULL, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3, Vista */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle);
}

static void test_open_svc(void)
{
    SC_HANDLE scm_handle, svc_handle;
    CHAR displayname[4096];
    DWORD displaysize;

    /* All NULL (invalid access rights) */
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(NULL, NULL, 0);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* TODO: Add some tests with invalid handles. These produce errors on Windows but crash on Wine */

    /* NULL service */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, NULL, GENERIC_READ);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Nonexistent service */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, "deadbeef", GENERIC_READ);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST, "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Proper SCM handle but different access rights */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, "Spooler", GENERIC_WRITE);
    if (!svc_handle && (GetLastError() == ERROR_ACCESS_DENIED))
        skip("Not enough rights to get a handle to the service\n");
    else
    {
        ok(svc_handle != NULL, "Expected success\n");
        ok(GetLastError() == ERROR_SUCCESS    /* W2K3, Vista */ ||
           GetLastError() == ERROR_IO_PENDING /* W2K */ ||
           GetLastError() == 0xdeadbeef       /* XP, NT4 */,
           "Expected ERROR_SUCCESS or 0xdeadbeef, got %d\n", GetLastError());
        CloseServiceHandle(svc_handle);
    }

    /* Test to show we can't open a service with the displayname */

    /* Retrieve the needed size for the buffer */
    displaysize = 0;
    GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    /* Get the displayname */
    GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    /* Try to open the service with this displayname */
    svc_handle = OpenServiceA(scm_handle, displayname, GENERIC_READ);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST, "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    /* Just in case */
    CloseServiceHandle(svc_handle);

    CloseServiceHandle(scm_handle);
}

static void test_create_delete_svc(void)
{
    SC_HANDLE scm_handle, svc_handle1;
    CHAR username[UNLEN + 1], domain[MAX_PATH];
    DWORD user_size = UNLEN + 1;
    CHAR account[UNLEN + 3];
    static const CHAR servicename         [] = "Winetest";
    static const CHAR pathname            [] = "we_dont_care.exe";
    static const CHAR empty               [] = "";
    static const CHAR password            [] = "secret";
    BOOL spooler_exists = FALSE;
    BOOL ret;
    CHAR display[4096];
    DWORD display_size = sizeof(display);

    /* Get the username and turn it into an account to be used in some tests */
    GetUserNameA(username, &user_size);
    /* Get the domainname to cater for that situation */
    if (GetEnvironmentVariableA("USERDOMAIN", domain, MAX_PATH))
        sprintf(account, "%s\\%s", domain, username);
    else
        sprintf(account, ".\\%s", username);

    /* All NULL */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    /* Only a valid handle to the Service Control Manager */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, W2K3, XP, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Now with a servicename */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, W2K3, XP, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Or just a binary name */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, NULL, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, W2K3, XP, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Both servicename and binary name (We only have connect rights) */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* They can even be empty at this stage of parameter checking */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, empty, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the Service Control Manager with minimal rights for creation
     * (Verified with 'SC_MANAGER_ALL_ACCESS &~ SC_MANAGER_CREATE_SERVICE')
     */
    CloseServiceHandle(scm_handle);
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }

    /* TODO: It looks like account (ServiceStartName) and (maybe) password are checked at this place */

    /* Empty strings for servicename and binary name are checked */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, empty, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, empty, NULL, 0, 0, 0, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %d\n", GetLastError());

    /* Valid call (as we will see later) except for the empty binary name (to proof it's indeed
     * an ERROR_INVALID_PARAMETER)
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_DISABLED, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Windows checks if the 'service type', 'access type' and the combination of them are valid, so let's test that */

    /* Illegal (service-type, which is used as a mask can't have a mix. Except the one with
     * SERVICE_INTERACTIVE_PROCESS which will be tested below in a valid call)
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (SERVICE_INTERACTIVE_PROCESS is only allowed with SERVICE_WIN32_OWN_PROCESS or SERVICE_WIN32_SHARE_PROCESS) */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_FILE_SYSTEM_DRIVER | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (this combination is only allowed when the LocalSystem account (ServiceStartName) is used)
     * Not having a correct account would have resulted in an ERROR_INVALID_SERVICE_ACCOUNT.
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, account, password);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (start-type is not a mask and should only be one of the possibilities)
     * Remark : 'OR'-ing them could result in a valid possibility (but doesn't make sense as
     * it's most likely not the wanted start-type)
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_AUTO_START | SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (SERVICE_BOOT_START and SERVICE_SYSTEM_START are only allowed for driver services) */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_BOOT_START, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* The service already exists (check first, just in case) */
    svc_handle1 = OpenServiceA(scm_handle, spooler, GENERIC_READ);
    if (svc_handle1)
    {
        spooler_exists = TRUE;
        CloseServiceHandle(svc_handle1);
        SetLastError(0xdeadbeef);
        svc_handle1 = CreateServiceA(scm_handle, spooler, NULL, 0, SERVICE_WIN32_OWN_PROCESS,
                                     SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
        ok(!svc_handle1, "Expected failure\n");
        ok(GetLastError() == ERROR_SERVICE_EXISTS, "Expected ERROR_SERVICE_EXISTS, got %d\n", GetLastError());
    }
    else
        skip("Spooler service doesn't exist\n");

    /* To find an existing displayname we check the 'Spooler' service. Although the registry
     * doesn't show DisplayName on NT4, this call will return a displayname which is equal
     * to the servicename and can't be used as well for a new displayname.
     */
    if (spooler_exists)
    {
        ret = GetServiceDisplayNameA(scm_handle, spooler, display, &display_size);

        if (!ret)
            skip("Could not retrieve a displayname for the Spooler service\n");
        else
        {
            svc_handle1 = CreateServiceA(scm_handle, servicename, display, 0, SERVICE_WIN32_OWN_PROCESS,
                                         SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
            ok(!svc_handle1, "Expected failure\n");
            ok(GetLastError() == ERROR_DUPLICATE_SERVICE_NAME,
               "Expected ERROR_DUPLICATE_SERVICE_NAME, got %d\n", GetLastError());
        }
    }
    else
        skip("Could not retrieve a displayname (Spooler service doesn't exist)\n");

    /* Windows doesn't care about the access rights for creation (which makes
     * sense as there is no service yet) as long as there are sufficient
     * rights to the manager.
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle1 != NULL, "Could not create the service : %d\n", GetLastError());
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3, Vista */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());

    /* DeleteService however must have proper rights */
    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle1);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the service with minimal rights for deletion.
     * (Verified with 'SERVICE_ALL_ACCESS &~ DELETE')
     */
    CloseServiceHandle(svc_handle1);
    svc_handle1 = OpenServiceA(scm_handle, servicename, DELETE);

    /* Now that we have the proper rights, we should be able to delete */
    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle1);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());

    CloseServiceHandle(svc_handle1);

    CloseServiceHandle(scm_handle);

    /* Wait a while. One of the following tests also does a CreateService for the
     * same servicename and this would result in an ERROR_SERVICE_MARKED_FOR_DELETE
     * error if we do this to quick. Vista seems more picky then the others.
     */
    Sleep(1000);

    /* And a final NULL check */
    SetLastError(0xdeadbeef);
    ret = DeleteService(NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
}

static void test_get_displayname(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    CHAR displayname[4096];
    WCHAR displaynameW[2048];
    DWORD displaysize, tempsize, tempsizeW;
    static const CHAR deadbeef[] = "Deadbeef";
    static const WCHAR spoolerW[] = {'S','p','o','o','l','e','r',0};
    static const CHAR servicename[] = "Winetest";
    static const CHAR pathname[] = "we_dont_care.exe";

    /* Having NULL for the size of the buffer will crash on W2K3 */

    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(NULL, NULL, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(scm_handle, NULL, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    displaysize = sizeof(displayname);
    ret = GetServiceDisplayNameA(scm_handle, NULL, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test for nonexistent service */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());

    /* Check if 'Spooler' exists */
    svc_handle = OpenServiceA(scm_handle, spooler, GENERIC_READ);
    if (!svc_handle)
    {
        skip("Spooler service doesn't exist\n");
        CloseServiceHandle(scm_handle);
        return;
    }
    CloseServiceHandle(svc_handle);

    /* Retrieve the needed size for the buffer */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    tempsize = displaysize;

    displaysize = 0;
    ret = GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(displaysize == tempsize, "Buffer size mismatch (%d vs %d)\n", tempsize, displaysize);

    /* Buffer is too small */
    SetLastError(0xdeadbeef);
    displaysize = (tempsize / 2);
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == tempsize, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* First try with a buffer that should be big enough to hold
     * the ANSI string (and terminating character). This succeeds on Windows
     *  although when asked (see above 2 tests) it will return twice the needed size.
     */
    SetLastError(0xdeadbeef);
    displaysize = (tempsize / 2) + 1;
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(ret, "Expected success\n");
    ok(displaysize == ((tempsize / 2) + 1), "Expected no change for the needed buffer size\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());

    /* Now with the original returned size */
    SetLastError(0xdeadbeef);
    displaysize = tempsize;
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(ret, "Expected success\n");
    ok(displaysize == tempsize, "Expected no change for the needed buffer size\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());

    /* And with a bigger than needed buffer */
    SetLastError(0xdeadbeef);
    displaysize = tempsize * 2;
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    /* Test that shows that if the buffersize is enough, it's not changed */
    ok(displaysize == tempsize * 2, "Expected no change for the needed buffer size\n");
    ok(lstrlen(displayname) == tempsize/2,
       "Expected the buffer to be twice the length of the string\n") ;

    /* Do the buffer(size) tests also for GetServiceDisplayNameW */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Buffer is too small */
    SetLastError(0xdeadbeef);
    tempsizeW = displaysize;
    displaysize = tempsizeW / 2;
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize = tempsizeW, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Now with the original returned size */
    SetLastError(0xdeadbeef);
    displaysize = tempsizeW;
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize = tempsizeW, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* And with a bigger than needed buffer */
    SetLastError(0xdeadbeef);
    displaysize = tempsizeW + 1; /* This caters for the null terminating character */
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, displaynameW, &displaysize);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    ok(displaysize == tempsizeW, "Expected the needed buffersize\n");
    ok(lstrlenW(displaynameW) == displaysize,
       "Expected the buffer to be the length of the string\n") ;
    ok(tempsize / 2 == tempsizeW,
       "Expected the needed buffersize (in bytes) to be the same for the A and W call\n");

    CloseServiceHandle(scm_handle);

    /* Test for a service without a displayname (which is valid). This should return
     * the servicename itself.
     */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }

    SetLastError(0xdeadbeef);
    svc_handle = CreateServiceA(scm_handle, servicename, NULL, DELETE,
                                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle != NULL, "Could not create the service : %d\n", GetLastError());
    if (!svc_handle)
    {
        CloseServiceHandle(scm_handle);
        return;
    }

    /* Retrieve the needed size for the buffer */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameA(scm_handle, servicename, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == lstrlen(servicename) * 2,
       "Expected the displaysize to be twice the size of the servicename\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Buffer is too small */
    SetLastError(0xdeadbeef);
    tempsize = displaysize;
    displaysize = (tempsize / 2);
    ret = GetServiceDisplayNameA(scm_handle, servicename, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == tempsize, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Get the displayname */
    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(scm_handle, servicename, displayname, &displaysize);
    ok(ret, "Expected success\n");
    ok(!lstrcmpi(displayname, servicename),
       "Expected displayname to be %s, got %s\n", servicename, displayname);
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());

    /* Delete the service */
    ret = DeleteService(svc_handle);
    ok(ret, "Expected success\n");

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);

    /* Wait a while. Just in case one of the following tests does a CreateService again */
    Sleep(1000);
}

static void test_get_servicekeyname(void)
{
    SC_HANDLE scm_handle, svc_handle;
    CHAR servicename[4096];
    CHAR displayname[4096];
    WCHAR servicenameW[4096];
    WCHAR displaynameW[4096];
    DWORD servicesize, displaysize, tempsize;
    BOOL ret;
    static const CHAR deadbeef[] = "Deadbeef";
    static const WCHAR deadbeefW[] = {'D','e','a','d','b','e','e','f',0};

    /* Having NULL for the size of the buffer will crash on W2K3 */

    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(NULL, NULL, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    servicesize = 200;
    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(scm_handle, NULL, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);

    /* Valid handle and buffer but no displayname */
    servicesize = 200;
    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(scm_handle, NULL, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    todo_wine ok(servicesize == 200, "Service size expected 1, got %d\n", servicesize);

    /* Test for nonexistent displayname */
    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(scm_handle, deadbeef, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);

    servicesize = 15;
    strcpy(servicename, "ABC");
    ret = GetServiceKeyNameA(scm_handle, deadbeef, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    todo_wine ok(servicesize == 15, "Service size expected 15, got %d\n", servicesize);
    ok(servicename[0] == 0, "Service name not empty\n");

    servicesize = 15;
    servicenameW[0] = 'A';
    ret = GetServiceKeyNameW(scm_handle, deadbeefW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    todo_wine ok(servicesize == 15, "Service size expected 15, got %d\n", servicesize);
    ok(servicenameW[0] == 0, "Service name not empty\n");

    servicesize = 0;
    strcpy(servicename, "ABC");
    ret = GetServiceKeyNameA(scm_handle, deadbeef, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);
    ok(servicename[0] == 'A', "Service name changed\n");

    servicesize = 0;
    servicenameW[0] = 'A';
    ret = GetServiceKeyNameW(scm_handle, deadbeefW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    todo_wine ok(servicesize == 2, "Service size expected 2, got %d\n", servicesize);
    ok(servicenameW[0] == 'A', "Service name changed\n");

    /* Check if 'Spooler' exists */
    svc_handle = OpenServiceA(scm_handle, spooler, GENERIC_READ);
    if (!svc_handle)
    {
        skip("Spooler service doesn't exist\n");
        CloseServiceHandle(scm_handle);
        return;
    }
    CloseServiceHandle(svc_handle);

    /* Get the displayname for the 'Spooler' service */
    GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);

    /* Retrieve the needed size for the buffer */
    SetLastError(0xdeadbeef);
    servicesize = 0;
    ret = GetServiceKeyNameA(scm_handle, displayname, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Valid call with the correct buffersize */
    SetLastError(0xdeadbeef);
    tempsize = servicesize;
    servicesize *= 2;
    ret = GetServiceKeyNameA(scm_handle, displayname, servicename, &servicesize);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    if (ret)
    {
        ok(lstrlen(servicename) == tempsize/2,
           "Expected the buffer to be twice the length of the string\n") ;
        ok(!lstrcmpi(servicename, spooler), "Expected %s, got %s\n", spooler, servicename);
        ok(servicesize == (tempsize * 2),
           "Expected servicesize not to change if buffer not insufficient\n") ;
    }

    MultiByteToWideChar(CP_ACP, 0, displayname, -1, displaynameW, sizeof(displaynameW)/2);
    SetLastError(0xdeadbeef);
    servicesize *= 2;
    ret = GetServiceKeyNameW(scm_handle, displaynameW, servicenameW, &servicesize);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    if (ret)
    {
        ok(lstrlen(servicename) == tempsize/2,
           "Expected the buffer to be twice the length of the string\n") ;
        ok(servicesize == lstrlenW(servicenameW),
           "Expected servicesize not to change if buffer not insufficient\n") ;
    }

    SetLastError(0xdeadbeef);
    servicesize = 3;
    ret = GetServiceKeyNameW(scm_handle, displaynameW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(servicenameW[0] == 0, "Buffer not empty\n");

    CloseServiceHandle(scm_handle);
}

static void test_close(void)
{
    SC_HANDLE handle;
    BOOL ret;

    /* NULL handle */
    SetLastError(0xdeadbeef);
    ret = CloseServiceHandle(NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* TODO: Add some tests with invalid handles. These produce errors on Windows but crash on Wine */

    /* Proper call */
    handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    ret = CloseServiceHandle(handle);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_IO_PENDING /* W2K */ ||
       GetLastError() == ERROR_SUCCESS    /* W2K3 */ ||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */,
       "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
}

static void test_sequence(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    QUERY_SERVICE_CONFIGA *config;
    DWORD given, needed;
    static const CHAR servicename [] = "Winetest";
    static const CHAR displayname [] = "Winetest dummy service";
    static const CHAR displayname2[] = "Winetest dummy service (2)";
    static const CHAR pathname    [] = "we_dont_care.exe";
    static const CHAR dependencies[] = "Master1\0Master2\0+MasterGroup1\0";
    static const CHAR password    [] = "";
    static const CHAR empty       [] = "";
    static const CHAR localsystem [] = "LocalSystem";

    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }
    else
        ok(scm_handle != NULL, "Could not get a handle to the manager: %d\n", GetLastError());

    if (!scm_handle) return;

    /* Create a dummy service */
    SetLastError(0xdeadbeef);
    svc_handle = CreateServiceA(scm_handle, servicename, displayname, GENERIC_ALL,
        SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, SERVICE_DISABLED, SERVICE_ERROR_IGNORE,
        pathname, NULL, NULL, dependencies, NULL, password);

    if (!svc_handle && (GetLastError() == ERROR_SERVICE_EXISTS))
    {
        /* We try and open the service and do the rest of the tests. Some could
         * fail if the tests were changed between these runs.
         */
        trace("Deletion probably didn't work last time\n");
        SetLastError(0xdeadbeef);
        svc_handle = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
        if (!svc_handle && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights to open the service\n");
            CloseServiceHandle(scm_handle);        
            return;
        }
        ok(svc_handle != NULL, "Could not open the service : %d\n", GetLastError());
    }
    else if (!svc_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to create the service\n");
        CloseServiceHandle(scm_handle);        
        return;
    }
    else
        ok(svc_handle != NULL, "Could not create the service : %d\n", GetLastError());

    if (!svc_handle) return;

    /* TODO:
     * Before we do a QueryServiceConfig we should check the registry. This will make sure
     * that the correct keys are used.
     */

    /* Request the size for the buffer */
    SetLastError(0xdeadbeef);
    ret = QueryServiceConfigA(svc_handle, NULL, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    config = HeapAlloc(GetProcessHeap(), 0, needed);
    given = needed;
    SetLastError(0xdeadbeef);
    ret = QueryServiceConfigA(svc_handle, config, given, &needed);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */,
        "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    todo_wine
    {
    ok(given == needed, "Expected the given (%d) and needed (%d) buffersizes to be equal\n", given, needed);
    }
    ok(config->lpBinaryPathName && config->lpLoadOrderGroup && config->lpDependencies && config->lpServiceStartName &&
        config->lpDisplayName, "Expected all string struct members to be non-NULL\n");
    ok(config->dwServiceType == (SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS),
        "Expected SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, got %d\n", config->dwServiceType);
    ok(config->dwStartType == SERVICE_DISABLED, "Expected SERVICE_DISABLED, got %d\n", config->dwStartType);
    ok(config->dwErrorControl == SERVICE_ERROR_IGNORE, "Expected SERVICE_ERROR_IGNORE, got %d\n", config->dwErrorControl);
    ok(!strcmp(config->lpBinaryPathName, pathname), "Expected '%s', got '%s'\n", pathname, config->lpBinaryPathName);
    ok(!strcmp(config->lpLoadOrderGroup, empty), "Expected an empty string, got '%s'\n", config->lpLoadOrderGroup);
    ok(config->dwTagId == 0, "Expected 0, got %d\n", config->dwTagId);
    /* TODO: Show the double 0 terminated string */
    todo_wine
    {
    ok(!memcmp(config->lpDependencies, dependencies, sizeof(dependencies)), "Wrong string\n");
    }
    ok(!strcmp(config->lpServiceStartName, localsystem), "Expected 'LocalSystem', got '%s'\n", config->lpServiceStartName);
    ok(!strcmp(config->lpDisplayName, displayname), "Expected '%s', got '%s'\n", displayname, config->lpDisplayName);
    
    ok(ChangeServiceConfigA(svc_handle, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_ERROR_NORMAL, NULL, "TestGroup2", NULL, NULL, NULL, NULL, displayname2),
        "ChangeServiceConfig failed (err=%d)\n", GetLastError());

    QueryServiceConfigA(svc_handle, NULL, 0, &needed);
    config = HeapReAlloc(GetProcessHeap(), 0, config, needed);
    ok(QueryServiceConfigA(svc_handle, config, needed, &needed), "QueryServiceConfig failed\n");
    ok(config->lpBinaryPathName && config->lpLoadOrderGroup && config->lpDependencies && config->lpServiceStartName &&
        config->lpDisplayName, "Expected all string struct members to be non-NULL\n");
    ok(config->dwServiceType == (SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS),
        "Expected SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, got %d\n", config->dwServiceType);
    ok(config->dwStartType == SERVICE_DISABLED, "Expected SERVICE_DISABLED, got %d\n", config->dwStartType);
    ok(config->dwErrorControl == SERVICE_ERROR_NORMAL, "Expected SERVICE_ERROR_NORMAL, got %d\n", config->dwErrorControl);
    ok(!strcmp(config->lpBinaryPathName, pathname), "Expected '%s', got '%s'\n", pathname, config->lpBinaryPathName);
    ok(!strcmp(config->lpLoadOrderGroup, "TestGroup2"), "Expected 'TestGroup2', got '%s'\n", config->lpLoadOrderGroup);
    ok(config->dwTagId == 0, "Expected 0, got %d\n", config->dwTagId);
    ok(!strcmp(config->lpServiceStartName, localsystem), "Expected 'LocalSystem', got '%s'\n", config->lpServiceStartName);
    ok(!strcmp(config->lpDisplayName, displayname2), "Expected '%s', got '%s'\n", displayname2, config->lpDisplayName);

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Expected success\n");
    ok(GetLastError() == ERROR_SUCCESS    /* W2K3 */||
       GetLastError() == 0xdeadbeef       /* NT4, XP, Vista */ ||
       GetLastError() == ERROR_IO_PENDING /* W2K */,
        "Expected ERROR_SUCCESS, ERROR_IO_PENDING or 0xdeadbeef, got %d\n", GetLastError());
    
    CloseServiceHandle(svc_handle);

    /* Wait a while. The following test does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(scm_handle);
    HeapFree(GetProcessHeap(), 0, config);
}

static void test_queryconfig2(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    DWORD expected, needed;
    BYTE buffer[MAX_PATH];
    LPSERVICE_DESCRIPTIONA pConfig = (LPSERVICE_DESCRIPTIONA)buffer;
    static const CHAR servicename [] = "Winetest";
    static const CHAR displayname [] = "Winetest dummy service";
    static const CHAR pathname    [] = "we_dont_care.exe";
    static const CHAR dependencies[] = "Master1\0Master2\0+MasterGroup1\0";
    static const CHAR password    [] = "";
    static const CHAR description [] = "Description";
    HMODULE dllhandle = GetModuleHandleA("advapi32.dll");
    BOOL (WINAPI *pChangeServiceConfig2A)(SC_HANDLE,DWORD,LPVOID)
            = (void*)GetProcAddress(dllhandle, "ChangeServiceConfig2A");
    BOOL (WINAPI *pQueryServiceConfig2A)(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD)
            = (void*)GetProcAddress(dllhandle, "QueryServiceConfig2A");
    BOOL (WINAPI *pQueryServiceConfig2W)(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD)
            = (void*)GetProcAddress(dllhandle, "QueryServiceConfig2W");
    if(!pQueryServiceConfig2A)
    {
        skip("function QueryServiceConfig2A not present\n");
        return;
    }

    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle)
    {
	if(GetLastError() == ERROR_ACCESS_DENIED)
            skip("Not enough rights to get a handle to the manager\n");
        else
            ok(FALSE, "Could not get a handle to the manager: %d\n", GetLastError());
        return;
    }

    /* Create a dummy service */
    SetLastError(0xdeadbeef);
    svc_handle = CreateServiceA(scm_handle, servicename, displayname, GENERIC_ALL,
        SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, SERVICE_DISABLED, SERVICE_ERROR_IGNORE,
        pathname, NULL, NULL, dependencies, NULL, password);

    if (!svc_handle)
    {
        if(GetLastError() == ERROR_SERVICE_EXISTS)
        {
            /* We try and open the service and do the rest of the tests. Some could
             * fail if the tests were changed between these runs.
             */
            trace("Deletion probably didn't work last time\n");
            SetLastError(0xdeadbeef);
            svc_handle = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
            if (!svc_handle)
            {
                if(GetLastError() == ERROR_ACCESS_DENIED)
                    skip("Not enough rights to open the service\n");
                else
                    ok(FALSE, "Could not open the service : %d\n", GetLastError());
                CloseServiceHandle(scm_handle);
                return;
            }
        }
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            skip("Not enough rights to create the service\n");
            CloseServiceHandle(scm_handle);
            return;
        }
        ok(svc_handle != NULL, "Could not create the service : %d\n", GetLastError());
	if (!svc_handle)
        {
            CloseServiceHandle(scm_handle);
            return;
        }
    }
    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle,0xfff0,buffer,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_LEVEL == GetLastError(), "expected error ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle,0xfff0,buffer,sizeof(SERVICE_DESCRIPTIONA),NULL);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_LEVEL == GetLastError(), "expected error ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA),NULL);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_ADDRESS == GetLastError(), "expected error ERROR_INVALID_ADDRESS, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok((ERROR_INVALID_ADDRESS == GetLastError()) || (ERROR_INSUFFICIENT_BUFFER == GetLastError()),
       "expected error ERROR_INVALID_ADDRESS or ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,sizeof(SERVICE_DESCRIPTIONA),NULL);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_ADDRESS == GetLastError(), "expected error ERROR_INVALID_ADDRESS, got %d\n", GetLastError());

    needed = 0;
    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA)-1,&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == sizeof(SERVICE_DESCRIPTIONA), "got %d\n", needed);

    needed = 0;
    pConfig->lpDescription = (LPSTR)0xdeadbeef;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(needed == sizeof(SERVICE_DESCRIPTIONA), "got %d\n", needed);
    ok(!pConfig->lpDescription, "expected lpDescription to be NULL, got %p\n", pConfig->lpDescription);

    SetLastError(0xdeadbeef);
    needed = 0;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,0,&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == sizeof(SERVICE_DESCRIPTIONA), "got %d\n", needed);

    if(!pChangeServiceConfig2A)
    {
        skip("function ChangeServiceConfig2A not present\n");
        goto cleanup;
    }

    pConfig->lpDescription = (LPSTR) description;
    ret = pChangeServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer);
    ok(ret, "ChangeServiceConfig2A failed\n");
    if (!ret) {
        goto cleanup;
    }

    SetLastError(0xdeadbeef);
    needed = 0;
    expected = sizeof(SERVICE_DESCRIPTIONA) + sizeof(description) * sizeof(WCHAR); /* !! */
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == expected, "expected needed to be %d, got %d\n", expected, needed);

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,needed-1,&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,needed,&needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfig->lpDescription && !strcmp(description,pConfig->lpDescription),
        "expected lpDescription to be %s, got %s\n",description ,pConfig->lpDescription);

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer, needed + 1,&needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfig->lpDescription && !strcmp(description,pConfig->lpDescription),
        "expected lpDescription to be %s, got %s\n",description ,pConfig->lpDescription);

    if(!pQueryServiceConfig2W)
    {
        skip("function QueryServiceConfig2W not present\n");
        goto cleanup;
    }
    SetLastError(0xdeadbeef);
    needed = 0;
    expected = sizeof(SERVICE_DESCRIPTIONW) + sizeof(WCHAR) * sizeof(description);
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,0,&needed);
    ok(!ret, "expected QueryServiceConfig2W to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == expected, "expected needed to be %d, got %d\n", expected, needed);

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer, needed,&needed);
    ok(ret, "expected QueryServiceConfig2W to succeed\n");

cleanup:
    DeleteService(svc_handle);

    CloseServiceHandle(svc_handle);

    /* Wait a while. The following test does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(scm_handle);
}

static void test_refcount(void)
{
    SC_HANDLE scm_handle, svc_handle1, svc_handle2, svc_handle3, svc_handle4, svc_handle5;
    static const CHAR servicename         [] = "Winetest";
    static const CHAR pathname            [] = "we_dont_care.exe";
    BOOL ret;

    /* Get a handle to the Service Control Manager */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }

    /* Create a service */
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL,
                                 SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle1 != NULL, "Expected success\n");

    /* Get a handle to this new service */
    svc_handle2 = OpenServiceA(scm_handle, servicename, GENERIC_READ);
    ok(svc_handle2 != NULL, "Expected success\n");

    /* Get another handle to this new service */
    svc_handle3 = OpenServiceA(scm_handle, servicename, GENERIC_READ);
    ok(svc_handle3 != NULL, "Expected success\n");

    /* Check if we can close the handle to the Service Control Manager */
    ret = CloseServiceHandle(scm_handle);
    ok(ret, "Expected success\n");

    /* Get a new handle to the Service Control Manager */
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    ok(scm_handle != NULL, "Expected success\n");

    /* Get a handle to this new service */
    svc_handle4 = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
    ok(svc_handle4 != NULL, "Expected success\n");

    /* Delete the service */
    ret = DeleteService(svc_handle4);
    ok(ret, "Expected success\n");

    /* We cannot create the same service again as it's still marked as 'being deleted'.
     * The reason is that we still have 4 open handles to this service even though we
     * closed the handle to the Service Control Manager in between.
     */
    SetLastError(0xdeadbeef);
    svc_handle5 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL,
                                 SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    todo_wine
    {
    ok(!svc_handle5, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE,
       "Expected ERROR_SERVICE_MARKED_FOR_DELETE, got %d\n", GetLastError());
    }

    /* FIXME: Remove this when Wine is fixed */
    if (svc_handle5)
    {
        DeleteService(svc_handle5);
        CloseServiceHandle(svc_handle5);
    }

    /* Close all the handles to the service and try again */
    ret = CloseServiceHandle(svc_handle4);
    ok(ret, "Expected success\n");
    ret = CloseServiceHandle(svc_handle3);
    ok(ret, "Expected success\n");
    ret = CloseServiceHandle(svc_handle2);
    ok(ret, "Expected success\n");
    ret = CloseServiceHandle(svc_handle1);
    ok(ret, "Expected success\n");

    /* Wait a while. Doing a CreateService too soon will result again
     * in an ERROR_SERVICE_MARKED_FOR_DELETE error.
     */
    Sleep(1000);

    /* We succeed now as all handles are closed (tested this also with a long SLeep() */
    svc_handle5 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL,
                                 SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle5 != NULL, "Expected success\n");

    /* Delete the service */
    ret = DeleteService(svc_handle5);
    ok(ret, "Expected success\n");

    /* Wait a while. Just in case one of the following tests does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(svc_handle5);
    CloseServiceHandle(scm_handle);
}

START_TEST(service)
{
    SC_HANDLE scm_handle;

    /* Bail out if we are on win98 */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        skip("OpenSCManagerA is not implemented, we are most likely on win9x\n");
        return;
    }
    CloseServiceHandle(scm_handle);

    /* First some parameter checking */
    test_open_scm();
    test_open_svc();
    test_create_delete_svc();
    test_get_displayname();
    test_get_servicekeyname();
    test_close();
    /* Test the creation, querying and deletion of a service */
    test_sequence();
    test_queryconfig2();
    /* The main reason for this test is to check if any refcounting is used
     * and what the rules are
     */
    test_refcount();
}
