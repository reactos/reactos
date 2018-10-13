/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for Lock/UnlockServiceDatabase and QueryServiceLockStatusA/W
 * PROGRAMMER:      Hermès BÉLUSCA - MAÏTO
 */

#include "precomp.h"

#define TESTING_SERVICE     L"Spooler"

static void Test_LockUnlockServiceDatabase(void)
{
    BOOL bError = FALSE;

    SC_HANDLE hScm  = NULL;
    SC_LOCK   hLock = NULL;

    /* First of all, try to lock / unlock the services database with invalid handles */
    SetLastError(0xdeadbeef);
    hScm  = NULL;
    hLock = LockServiceDatabase(hScm);
    ok(hLock == NULL, "hLock = 0x%p, expected 0\n", hLock);
    ok_err(ERROR_INVALID_HANDLE);

    SetLastError(0xdeadbeef);
    hScm  = (SC_HANDLE)(ULONG_PTR)0xdeadbeefdeadbeefull;
    hLock = LockServiceDatabase(hScm);
    ok(hLock == NULL, "hLock = 0x%p, expected 0\n", hLock);
    ok_err(ERROR_INVALID_HANDLE);

/** This test seems to make this application crash on Windows 7... I do not know why... **/
    SetLastError(0xdeadbeef);
    hLock = NULL;
    bError = UnlockServiceDatabase(hLock);
    ok(bError == FALSE, "bError = %u, expected FALSE\n", bError);
    ok_err(ERROR_INVALID_SERVICE_LOCK);
/*****************************************************************************************/

    SetLastError(0xdeadbeef);
    hLock = (SC_LOCK)(ULONG_PTR)0xdeadbeefdeadbeefull;
    bError = UnlockServiceDatabase(hLock);
    ok(bError == FALSE, "bError = %u, expected FALSE\n", bError);
    ok_err(ERROR_INVALID_SERVICE_LOCK);


    /* Then, try to lock the services database without having rights */
    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with LockUnlockServiceDatabase test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    SetLastError(0xdeadbeef);
    hLock = LockServiceDatabase(hScm);
    ok(hLock == NULL, "hLock = 0x%p, expected 0\n", hLock);
    ok_err(ERROR_ACCESS_DENIED);

    if (hLock)
        UnlockServiceDatabase(hLock);
    CloseServiceHandle(hScm);

    /* Try to lock the services database with good rights */
    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_LOCK);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with LockUnlockServiceDatabase test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    SetLastError(0xdeadbeef);
    hLock = LockServiceDatabase(hScm);
    ok_err(ERROR_SUCCESS);
    ok(hLock != NULL, "hLock = 0x%p, expected non-zero\n", hLock);

    /* Now unlock it */
    if (hLock)
    {
        SetLastError(0xdeadbeef);
        bError = UnlockServiceDatabase(hLock);
        ok(bError == TRUE, "bError = %u, expected TRUE\n", bError);
        ok_err(ERROR_SUCCESS);
    }


cleanup:
    if (hScm)
        CloseServiceHandle(hScm);

    return;
}

static void Test_LockUnlockServiceDatabaseWithServiceStart(void)
{
    BOOL bError = FALSE;

    SC_HANDLE hScm  = NULL;
    SC_HANDLE hSvc  = NULL;
    SC_LOCK   hLock = NULL;

    LPQUERY_SERVICE_CONFIGW lpConfig = NULL;
    DWORD dwRequiredSize = 0;
    SERVICE_STATUS status;
    BOOL bWasRunning = FALSE;
    DWORD dwOldStartType = 0;

    /* Open the services database */
    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_LOCK);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with LockUnlockServiceDatabaseWithServiceStart test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Grab a handle to the testing service */
    SetLastError(0xdeadbeef);
    hSvc = OpenServiceW(hScm, TESTING_SERVICE, SERVICE_START | SERVICE_STOP | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
    ok(hSvc != NULL, "hSvc = 0x%p, expected non-null, error=0x%08lx\n", hSvc, GetLastError());
    if (!hSvc)
    {
        skip("Cannot open a handle to service %S; cannot proceed with LockUnlockServiceDatabaseWithServiceStart test\n", TESTING_SERVICE);
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Lock the services database */
    SetLastError(0xdeadbeef);
    hLock = LockServiceDatabase(hScm);
    ok(hLock != NULL, "hLock = 0x%p, expected non-zero, error=0x%08lx\n", hLock, GetLastError());
    if (!hLock)
    {
        skip("Cannot lock the services database; cannot proceed with LockUnlockServiceDatabaseWithServiceStart test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* To proceed further, firstly attempt to stop the testing service */
    QueryServiceConfigW(hSvc, NULL, 0, &dwRequiredSize);
    lpConfig = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    QueryServiceConfigW(hSvc, lpConfig, dwRequiredSize, &dwRequiredSize);
    dwOldStartType = lpConfig->dwStartType;
    HeapFree(GetProcessHeap(), 0, lpConfig);
    if (dwOldStartType == SERVICE_DISABLED)
    {
        ChangeServiceConfigW(hSvc,
                             SERVICE_NO_CHANGE,
                             SERVICE_DEMAND_START,
                             SERVICE_NO_CHANGE,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    }
    QueryServiceStatus(hSvc, &status);
    bWasRunning = (status.dwCurrentState != SERVICE_STOPPED);
    if (bWasRunning)
    {
        ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
        Sleep(1000); /* Wait 1 second for the service to stop */
    }

    /* Now try to start it (this test won't work under Windows Vista / 7 / 8) */
    SetLastError(0xdeadbeef);
    bError = StartServiceW(hSvc, 0, NULL);
    ok(bError == FALSE, "bError = %u, expected FALSE\n", bError);
    ok_err(ERROR_SERVICE_DATABASE_LOCKED);
    Sleep(1000); /* Wait 1 second for the service to start */

    /* Stop the testing service */
    ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
    Sleep(1000); /* Wait 1 second for the service to stop */

    /* Now unlock the services database */
    SetLastError(0xdeadbeef);
    bError = UnlockServiceDatabase(hLock);
    ok(bError == TRUE, "bError = %u, expected TRUE\n", bError);
    ok_err(ERROR_SUCCESS);

    /* Try to start again the service, this time the database unlocked */
    SetLastError(0xdeadbeef);
    bError = StartServiceW(hSvc, 0, NULL);
    ok(bError == TRUE, "bError = %u, expected TRUE\n", bError);
    ok_err(ERROR_SUCCESS);
    Sleep(1000); /* Wait 1 second for the service to start */

    /* Stop the testing service */
    ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
    Sleep(1000); /* Wait 1 second for the service to stop */

    /* Restore its original state */
    if (bWasRunning)
    {
        StartServiceW(hSvc, 0, NULL);
    }

    if (dwOldStartType == SERVICE_DISABLED)
    {
        ChangeServiceConfigW(hSvc,
                             SERVICE_NO_CHANGE,
                             SERVICE_DISABLED,
                             SERVICE_NO_CHANGE,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
    }


cleanup:
    if (hSvc)
        CloseServiceHandle(hSvc);

    if (hScm)
        CloseServiceHandle(hScm);

    return;
}

static void Test_QueryLockStatusW(void)
{
    BOOL bError = FALSE;

    SC_HANDLE hScm  = NULL;
    SC_LOCK   hLock = NULL;
    LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus = NULL;
    DWORD dwRequiredSize = 0;

    /* Firstly try to get lock status with invalid handles */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INVALID_HANDLE, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INVALID_HANDLE);
    ok(dwRequiredSize == 0, "dwRequiredSize is non-zero, expected zero\n");

    /* Open the services database without having rights */
    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Try to get lock status */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_ACCESS_DENIED, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_ACCESS_DENIED);
    ok(dwRequiredSize == 0, "dwRequiredSize is non-zero, expected zero\n");

    CloseServiceHandle(hScm);


    /*
     * Query only the lock status.
     */

    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_QUERY_LOCK_STATUS);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database wasn't previously locked */
    ok(lpLockStatus->fIsLocked == 0, "lpLockStatus->fIsLocked = %lu, expected 0\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner == 0, "*lpLockStatus->lpLockOwner != \"\\0\", expected \"\\0\"\n");
    ok(lpLockStatus->dwLockDuration == 0, "lpLockStatus->dwLockDuration = %lu, expected 0\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);

    CloseServiceHandle(hScm);


    /*
     * Now, try to lock the database and check its lock status.
     */

    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_LOCK | SC_MANAGER_QUERY_LOCK_STATUS);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database wasn't previously locked */
    ok(lpLockStatus->fIsLocked == 0, "lpLockStatus->fIsLocked = %lu, expected 0\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner == 0, "*lpLockStatus->lpLockOwner != \"\\0\", expected \"\\0\"\n");
    ok(lpLockStatus->dwLockDuration == 0, "lpLockStatus->dwLockDuration = %lu, expected 0\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);


    /*
     * Try again, this time with the database locked.
     */

    SetLastError(0xdeadbeef);
    hLock = LockServiceDatabase(hScm);
    ok(hLock != NULL, "hLock = 0x%p, expected non-zero\n", hLock);
    ok_err(ERROR_SUCCESS);

    Sleep(1000); /* Wait 1 second to let lpLockStatus->dwLockDuration increment */

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database is locked */
    ok(lpLockStatus->fIsLocked != 0, "lpLockStatus->fIsLocked = %lu, expected non-zero\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner != 0, "*lpLockStatus->lpLockOwner = \"\\0\", expected non-zero\n");
    ok(lpLockStatus->dwLockDuration != 0, "lpLockStatus->dwLockDuration = %lu, expected non-zero\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);


    /*
     * Last try, with the database again unlocked.
     */

    SetLastError(0xdeadbeef);
    bError = UnlockServiceDatabase(hLock);
    ok(bError == TRUE, "bError = %u, expected TRUE\n", bError);
    ok_err(ERROR_SUCCESS);
    hLock = NULL;

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusW test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusW(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database is unlocked */
    ok(lpLockStatus->fIsLocked == 0, "lpLockStatus->fIsLocked = %lu, expected 0\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner == 0, "*lpLockStatus->lpLockOwner != \"\\0\", expected \"\\0\"\n");
    ok(lpLockStatus->dwLockDuration == 0, "lpLockStatus->dwLockDuration = %lu, expected 0\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);


cleanup:
    if (hLock)
        UnlockServiceDatabase(hLock);

    if (hScm)
        CloseServiceHandle(hScm);

    return;
}

static void Test_QueryLockStatusA(void)
{
    BOOL bError = FALSE;

    SC_HANDLE hScm  = NULL;
    SC_LOCK   hLock = NULL;
    LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus = NULL;
    DWORD dwRequiredSize = 0;

    /* Firstly try to get lock status with invalid handles */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INVALID_HANDLE, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INVALID_HANDLE);
    ok(dwRequiredSize == 0, "dwRequiredSize is non-zero, expected zero\n");

    /* Open the services database without having rights */
    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Try to get lock status */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_ACCESS_DENIED, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_ACCESS_DENIED);
    ok(dwRequiredSize == 0, "dwRequiredSize is non-zero, expected zero\n");

    CloseServiceHandle(hScm);


    /*
     * Query only the lock status.
     */

    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerA(NULL, NULL, SC_MANAGER_QUERY_LOCK_STATUS);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database wasn't previously locked */
    ok(lpLockStatus->fIsLocked == 0, "lpLockStatus->fIsLocked = %lu, expected 0\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner == 0, "*lpLockStatus->lpLockOwner != \"\\0\", expected \"\\0\"\n");
    ok(lpLockStatus->dwLockDuration == 0, "lpLockStatus->dwLockDuration = %lu, expected 0\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);

    CloseServiceHandle(hScm);


    /*
     * Now, try to lock the database and check its lock status.
     */

    SetLastError(0xdeadbeef);
    hScm = OpenSCManagerA(NULL, NULL, SC_MANAGER_LOCK | SC_MANAGER_QUERY_LOCK_STATUS);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }
    ok_err(ERROR_SUCCESS);

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database wasn't previously locked */
    ok(lpLockStatus->fIsLocked == 0, "lpLockStatus->fIsLocked = %lu, expected 0\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner == 0, "*lpLockStatus->lpLockOwner != \"\\0\", expected \"\\0\"\n");
    ok(lpLockStatus->dwLockDuration == 0, "lpLockStatus->dwLockDuration = %lu, expected 0\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);


    /*
     * Try again, this time with the database locked.
     */

    SetLastError(0xdeadbeef);
    hLock = LockServiceDatabase(hScm);
    ok(hLock != NULL, "hLock = 0x%p, expected non-zero\n", hLock);
    ok_err(ERROR_SUCCESS);

    Sleep(1000); /* Wait 1 second to let lpLockStatus->dwLockDuration increment */

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database is locked */
    ok(lpLockStatus->fIsLocked != 0, "lpLockStatus->fIsLocked = %lu, expected non-zero\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner != 0, "*lpLockStatus->lpLockOwner = \"\\0\", expected non-zero\n");
    ok(lpLockStatus->dwLockDuration != 0, "lpLockStatus->dwLockDuration = %lu, expected non-zero\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);


    /*
     * Last try, with the database again unlocked.
     */

    SetLastError(0xdeadbeef);
    bError = UnlockServiceDatabase(hLock);
    ok(bError == TRUE, "bError = %u, expected TRUE\n", bError);
    ok_err(ERROR_SUCCESS);
    hLock = NULL;

    /* Get the needed size */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     NULL,
                                     0,
                                     &dwRequiredSize);
    ok(bError == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "(bError, GetLastError()) = (%u, 0x%08lx), expected (FALSE, 0x%08lx)\n", bError, GetLastError(), (DWORD)ERROR_INSUFFICIENT_BUFFER);
    ok(dwRequiredSize != 0, "dwRequiredSize is zero, expected non-zero\n");
    if (dwRequiredSize == 0)
    {
        skip("Required size is null; cannot proceed with QueryLockStatusA test\n");
        goto cleanup;
    }

    /* Allocate memory */
    lpLockStatus = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize);
    if (lpLockStatus == NULL)
    {
        skip("Cannot allocate %lu bytes of memory\n", dwRequiredSize);
        goto cleanup;
    }

    /* Get the actual value */
    SetLastError(0xdeadbeef);
    bError = QueryServiceLockStatusA(hScm,
                                     lpLockStatus,
                                     dwRequiredSize,
                                     &dwRequiredSize);
    ok(bError, "bError = %u, expected TRUE\n", bError);

    /* These conditions must be verified iff the services database is unlocked */
    ok(lpLockStatus->fIsLocked == 0, "lpLockStatus->fIsLocked = %lu, expected 0\n", lpLockStatus->fIsLocked);
    ok(lpLockStatus->lpLockOwner != NULL, "lpLockStatus->lpLockOwner is null, expected non-null\n");
    ok(lpLockStatus->lpLockOwner && *lpLockStatus->lpLockOwner == 0, "*lpLockStatus->lpLockOwner != \"\\0\", expected \"\\0\"\n");
    ok(lpLockStatus->dwLockDuration == 0, "lpLockStatus->dwLockDuration = %lu, expected 0\n", lpLockStatus->dwLockDuration);

    HeapFree(GetProcessHeap(), 0, lpLockStatus);


cleanup:
    if (hLock)
        UnlockServiceDatabase(hLock);

    if (hScm)
        CloseServiceHandle(hScm);

    return;
}


START_TEST(LockServiceDatabase)
{
    Test_LockUnlockServiceDatabase();
    Test_LockUnlockServiceDatabaseWithServiceStart();
    Test_QueryLockStatusW();
    Test_QueryLockStatusA();
}
