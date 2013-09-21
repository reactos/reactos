/*
 * Copyright 2012 Andrew Eikum for CodeWeavers
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

#define COBJMACROS

#include <stdio.h>

#include "windows.h"
#include "winnetwk.h"
#include "wine/test.h"

static void test_WNetGetUniversalName(void)
{
    DWORD ret;
    char buffer[1024];
    DWORD drive_type, info_size, fail_size;
    char driveA[] = "A:\\";
    char driveandpathA[] = "A:\\file.txt";
    WCHAR driveW[] = {'A',':','\\',0};

    for(; *driveA <= 'Z'; ++*driveA,  ++*driveandpathA, ++*driveW){
        drive_type = GetDriveTypeW(driveW);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);

        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            /* WN_NO_NET_OR_BAD_PATH (DRIVE_FIXED) returned from the virtual drive (usual Q:)
               created by the microsoft application virtualization client */
            ok((ret == WN_NOT_CONNECTED) || (ret == WN_NO_NET_OR_BAD_PATH),
                "WNetGetUniversalNameA(%s, ...) returned %u (drive_type: %u)\n",
                driveA, ret, drive_type);

        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);

        fail_size = 0;
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine ok(ret == WN_BAD_VALUE, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameW gave wrong error: %u\n", driveA, ret);

        fail_size = sizeof(driveA) / sizeof(char) - 1;
        ret = WNetGetUniversalNameA(driveA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_MORE_DATA, "WNetGetUniversalNameA failed: %08x\n", ret);

        ret = WNetGetUniversalNameA(driveandpathA, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameW(driveW, UNIVERSAL_NAME_INFO_LEVEL,
                buffer, &info_size);

        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameW failed: %08x\n", ret);
        else
            ok((ret == WN_NOT_CONNECTED) || (ret == WN_NO_NET_OR_BAD_PATH),
                "WNetGetUniversalNameW(%s, ...) returned %u (drive_type: %u)\n",
                wine_dbgstr_w(driveW), ret, drive_type);
        if(drive_type != DRIVE_REMOTE)
            ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);
    }
}

static void test_WNetGetRemoteName(void)
{
    DWORD ret;
    char buffer[1024];
    DWORD drive_type, info_size, fail_size;
    char driveA[] = "A:\\";
    char driveandpathA[] = "A:\\file.txt";
    WCHAR driveW[] = {'A',':','\\',0};

    for(; *driveA <= 'Z'; ++*driveA,  ++*driveandpathA, ++*driveW){
        drive_type = GetDriveTypeW(driveW);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, &info_size);
        todo_wine{
        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameA gave wrong error: %u\n", driveA, ret);
        }
        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);

        fail_size = 0;
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, &fail_size);
        todo_wine{
        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_BAD_VALUE, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameA gave wrong error: %u\n", driveA, ret);
        }
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, NULL);
        todo_wine ok(ret == WN_BAD_POINTER, "WNetGetUniversalNameA failed: %08x\n", ret);

        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                NULL, &info_size);

        todo_wine{
        if(((GetVersion() & 0x8000ffff) == 0x00000004) || /* NT40 */
           (drive_type == DRIVE_REMOTE))
            ok(ret == WN_BAD_POINTER, "WNetGetUniversalNameA failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_BAD_VALUE,
                "(%s) WNetGetUniversalNameA gave wrong error: %u\n", driveA, ret);        }

        fail_size = sizeof(driveA) / sizeof(char) - 1;
        ret = WNetGetUniversalNameA(driveA, REMOTE_NAME_INFO_LEVEL,
                buffer, &fail_size);
        if(drive_type == DRIVE_REMOTE)
            todo_wine ok(ret == WN_MORE_DATA, "WNetGetUniversalNameA failed: %08x\n", ret);

        ret = WNetGetUniversalNameA(driveandpathA, REMOTE_NAME_INFO_LEVEL,
                buffer, &info_size);
        if(drive_type == DRIVE_REMOTE)
          todo_wine ok(ret == WN_NO_ERROR, "WNetGetUniversalNameA failed: %08x\n", ret);

        info_size = sizeof(buffer);
        ret = WNetGetUniversalNameW(driveW, REMOTE_NAME_INFO_LEVEL,
                buffer, &info_size);
        todo_wine{
        if(drive_type == DRIVE_REMOTE)
            ok(ret == WN_NO_ERROR, "WNetGetUniversalNameW failed: %08x\n", ret);
        else
            ok(ret == WN_NOT_CONNECTED || ret == WN_NO_NET_OR_BAD_PATH,
                "(%s) WNetGetUniversalNameW gave wrong error: %u\n", driveA, ret);
        }
        ok(info_size == sizeof(buffer), "Got wrong size: %u\n", info_size);
    }
}

START_TEST(mpr)
{
    test_WNetGetUniversalName();
    test_WNetGetRemoteName();
}
