/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SaferIdentifyLevel
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

#include <winsafer.h>

#define SaferIdentifyLevel(c, p, h, r) SaferIdentifyLevel(c, (PSAFER_CODE_PROPERTIES)(p), h, r)

START_TEST(SaferIdentifyLevel)
{
    BOOL ret;
    DWORD error;
    SAFER_LEVEL_HANDLE handle;
    SAFER_CODE_PROPERTIES_V2 props[16];

    StartSeh()
        SetLastError(0xbadbad00);
        ret = SaferIdentifyLevel(0, NULL, NULL, NULL);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(error == ERROR_NOACCESS, "error = %lu\n", error);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        (VOID)SaferIdentifyLevel(0, NULL, &handle, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        ZeroMemory(props, sizeof(props));
        SetLastError(0xbadbad00);
        ret = SaferIdentifyLevel(16, props, &handle, NULL);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(error == ERROR_BAD_LENGTH, "error = %lu\n", error);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        ZeroMemory(props, sizeof(props));
        SetLastError(0xbadbad00);
        ret = SaferIdentifyLevel(1, props, NULL, NULL);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(error == ERROR_NOACCESS, "error = %lu\n", error);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        handle = InvalidPointer;
        ZeroMemory(props, sizeof(props));
        SetLastError(0xbadbad00);
        ret = SaferIdentifyLevel(1, props, &handle, NULL);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(handle == InvalidPointer, "handle = %p\n", handle);
        ok(error == ERROR_BAD_LENGTH, "error = %lu\n", error);
        if (handle && handle != InvalidPointer)
            SaferCloseLevel(handle);
    EndSeh(STATUS_SUCCESS);

    /* Struct sizes */
    StartSeh()
        handle = InvalidPointer;
        ZeroMemory(props, sizeof(props));
        props[0].cbSize = sizeof(SAFER_CODE_PROPERTIES_V1);
        SetLastError(0xbadbad00);
        ret = SaferIdentifyLevel(1, props, &handle, NULL);
        error = GetLastError();
        ok(ret == TRUE, "ret = %d\n", ret);
        ok(handle != NULL && handle != INVALID_HANDLE_VALUE && handle != InvalidPointer, "handle = %p\n", handle);
        ok(error == 0xbadbad00, "error = %lu\n", error);
        if (handle && handle != InvalidPointer)
        {
            ret = SaferCloseLevel(handle);
            ok(ret == TRUE, "ret = %d\n", ret);
        }
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        handle = InvalidPointer;
        ZeroMemory(props, sizeof(props));
        props[0].cbSize = sizeof(SAFER_CODE_PROPERTIES_V2);
        SetLastError(0xbadbad00);
        ret = SaferIdentifyLevel(1, props, &handle, NULL);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(handle == InvalidPointer, "handle = %p\n", handle);
        ok(error == ERROR_BAD_LENGTH, "error = %lu\n", error);
        if (handle && handle != InvalidPointer)
            SaferCloseLevel(handle);
    EndSeh(STATUS_SUCCESS);

    /* Test SaferCloseLevel too */
    StartSeh()
        ret = SaferCloseLevel(NULL);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(error == ERROR_INVALID_HANDLE, "error = %lu\n", error);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        ret = SaferCloseLevel(INVALID_HANDLE_VALUE);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(error == ERROR_INVALID_HANDLE, "error = %lu\n", error);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        ret = SaferCloseLevel(InvalidPointer);
        error = GetLastError();
        ok(ret == FALSE, "ret = %d\n", ret);
        ok(error == ERROR_INVALID_HANDLE, "error = %lu\n", error);
    EndSeh(STATUS_SUCCESS);
}
