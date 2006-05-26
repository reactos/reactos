/*
 * Unit test suite
 *
 * Copyright 2006 Stefan Leichter
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

#include "wine/test.h"
#include "winbase.h"

#define CDROM   "CDROM"
#define FLOPPY  "FLOPPY"
#define HARDISK "HARDDISK"
#define LANMAN  "LANMANREDIRECTOR"
#define RAMDISK "RAMDISK"

static void test_query_dos_deviceA(void)
{
    char drivestr[] = "a:";
    char *p, buffer[2000];
    DWORD ret;
    for (;drivestr[0] <= 'z'; drivestr[0]++) {
        ret = QueryDosDeviceA( drivestr, buffer, sizeof(buffer));
        if(ret) {
            for (p = buffer; *p; p++) *p = toupper(*p);
            todo_wine
            ok( strstr( buffer, CDROM)   || strstr( buffer, FLOPPY) ||
                strstr( buffer, HARDISK) || strstr( buffer, LANMAN) ||
                strstr( buffer, RAMDISK), "expect the string %s contains %s,%s,%s,%s or %s\n",
                buffer, CDROM, FLOPPY, HARDISK, LANMAN, RAMDISK);
        }
    }
}

START_TEST(volume)
{
    test_query_dos_deviceA();
}
