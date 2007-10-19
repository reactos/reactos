/*
 * Unit test suite for Icmp.dll functions
 *
 * Copyright 2006 Steven Edwards
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * TODO:
 * It seems under Windows XP, 2003 and Vista these functions are not implemented
 * in iphlpapi. Move the implementation and tests there.
 */

#include <windows.h>
#include "wine/test.h"

HANDLE WINAPI IcmpCreateFile(void);
BOOL WINAPI IcmpCloseHandle(HANDLE handle);

HANDLE handle;

static void test_IcmpCreateFile(void)
{
    handle=IcmpCreateFile();
    ok(handle!=INVALID_HANDLE_VALUE,"Failed to create icmp file handle\n");
}

static void test_IcmpCloseHandle(void)
{
    BOOL result;
    result=IcmpCloseHandle(handle);
    ok(result!=FALSE,"Failed to close icmp file handle\n");
}

START_TEST(icmp)
{
    test_IcmpCreateFile();
    test_IcmpCloseHandle();
}
