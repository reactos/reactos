/*
 * Unit test for bcrypt functions
 *
 * Copyright 2014 Bruno Jesus
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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <bcrypt.h>

#include "wine/test.h"

static NTSTATUS (WINAPI *pBCryptGenRandom)(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbBuffer,
                                           ULONG cbBuffer, ULONG dwFlags);
static NTSTATUS (WINAPI *pBCryptGetFipsAlgorithmMode)(BOOLEAN *enabled);

static BOOL Init(void)
{
    HMODULE hbcrypt = LoadLibraryA("bcrypt.dll");
    if (!hbcrypt)
    {
        win_skip("bcrypt library not available\n");
        return FALSE;
    }

    pBCryptGenRandom = (void *)GetProcAddress(hbcrypt, "BCryptGenRandom");
    pBCryptGetFipsAlgorithmMode = (void *)GetProcAddress(hbcrypt, "BCryptGetFipsAlgorithmMode");

    return TRUE;
}

static void test_BCryptGenRandom(void)
{
    NTSTATUS ret;
    UCHAR buffer[256];

    if (!pBCryptGenRandom)
    {
        win_skip("BCryptGenRandom is not available\n");
        return;
    }

    ret = pBCryptGenRandom(NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, sizeof(buffer), 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, buffer, sizeof(buffer),
          BCRYPT_USE_SYSTEM_PREFERRED_RNG|BCRYPT_RNG_USE_ENTROPY_IN_BUFFER);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ret = pBCryptGenRandom(NULL, NULL, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);

    /* Zero sized buffer should work too */
    ret = pBCryptGenRandom(NULL, buffer, 0, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);

    /* Test random number generation - It's impossible for a sane RNG to return 8 zeros */
    memset(buffer, 0, 16);
    ret = pBCryptGenRandom(NULL, buffer, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got 0x%x\n", ret);
    ok(memcmp(buffer, buffer + 8, 8), "Expected a random number, got 0\n");
}

static void test_BCryptGetFipsAlgorithmMode(void)
{
    NTSTATUS ret;
    BOOLEAN enabled;

    if (!pBCryptGetFipsAlgorithmMode)
    {
        win_skip("BCryptGetFipsAlgorithmMode is not available\n");
        return;
    }

    ret = pBCryptGetFipsAlgorithmMode(&enabled);
    ok(ret == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got 0x%x\n", ret);

    ret = pBCryptGetFipsAlgorithmMode(NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got 0x%x\n", ret);
}

START_TEST(bcrypt)
{
    if (!Init())
        return;

    test_BCryptGenRandom();
    test_BCryptGetFipsAlgorithmMode();
}
