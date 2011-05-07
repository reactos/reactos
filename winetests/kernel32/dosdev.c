/*
 * Unit test suite for virtual substituted drive functions.
 *
 * Copyright 2011 Sam Arun Raj
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

static void test_DefineDosDeviceA1(void)
{
    /* Test using lowercase drive letters */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "m:";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp");
    ok(Result, "Failed to subst drive using lowercase drive letter\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp");
    ok(Result, "Failed to remove subst drive using lowercase drive letter\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
}

static void test_DefineDosDeviceA2(void)
{
    /* Test using trailing \ against drive letter */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "Q:\\";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp");
    ok(!Result, "Subst drive using trailing path seperator\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp");
    ok(!Result, "Subst drive using trailing path seperator\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present when it should not be created in the first place\n");
}

static void test_DefineDosDeviceA3(void)
{
    /* Test using arbitary string, not necessarily a DOS drive letter */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "!QHello:";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp");
    ok(Result, "Failed to subst drive using non-DOS drive name\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp");
    ok(Result, "Failed to subst drive using non-DOS drive name\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
}

static void test_DefineDosDeviceA4(void)
{
    /* Test remove without using DDD_EXACT_MATCH_ON_REMOVE */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "M:";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp");
    ok(Result, "Failed to subst drive\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION, Drive, NULL);
    ok(Result, "Failed to remove subst drive using NULL Target name\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
}

static void test_DefineDosDeviceA5(void)
{
    /* Test multiple adds and multiple removes in add order */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "M:";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp1");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp2");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp3");
    ok(Result, "Failed to subst drive\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp1");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp3") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp2");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp3") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp3");
    ok(Result, "Failed to remove subst drive\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
}

static void test_DefineDosDeviceA6(void)
{
    /* Test multiple adds and multiple removes in reverse order */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "M:";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp1");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp2");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp3");
    ok(Result, "Failed to subst drive\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp3");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp2") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp2");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp1") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp1");
    ok(Result, "Failed to remove subst drive\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
}

static void test_DefineDosDeviceA7(void)
{
    /* Test multiple adds and multiple removes out of order */
    CHAR Target[MAX_PATH];
    CHAR Drive[] = "M:";
    BOOL Result;

    Result = DefineDosDeviceA(0, Drive, "C:\\temp1");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp2");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp3");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp4");
    ok(Result, "Failed to subst drive\n");
    Result = DefineDosDeviceA(0, Drive, "C:\\temp5");
    ok(Result, "Failed to subst drive\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp2");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp5") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp5");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp4") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp1");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp4") == 0), "Subst drive is not pointing to correct target\n");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp3");
    ok(Result, "Failed to remove subst drive\n");
    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(Result, "Failed to query subst drive\n");
    if (Result)
        ok((_stricmp(Target, "\\??\\C:\\temp4") == 0), "Subst drive is not pointing to correct target");

    Result = DefineDosDeviceA(DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE, Drive, "C:\\temp4");
    ok(Result, "Failed to remove subst drive\n");

    Result = QueryDosDeviceA(Drive, Target, MAX_PATH);
    ok(!Result, "Subst drive is present even after remove attempt\n");
}

START_TEST(dosdev)
{
    test_DefineDosDeviceA1();
    test_DefineDosDeviceA2();
    test_DefineDosDeviceA3();
    test_DefineDosDeviceA4();
    test_DefineDosDeviceA5();
    test_DefineDosDeviceA6();
    test_DefineDosDeviceA7();
}
