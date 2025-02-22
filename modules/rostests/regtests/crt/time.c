/*
 * PROJECT:         ReactOS CRT regression tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            rostests/regtests/crt/time.c
 * PURPOSE:         Tests for time functions of the CRT
 * PROGRAMMERS:     Gregor Schneider
 */

#include <stdio.h>
#include <time.h>
#include <wine/test.h>

void Test_asctime()
{
    /* Test asctime */
    struct tm time;
    char* timestr;
    char explowtime[] = "Mon Jun 04 00:30:20 1909\n"; /* XP's crt returns new line after the string */

    time.tm_hour = 0;
    time.tm_mday = 4;
    time.tm_min = 30;
    time.tm_mon = 5;
    time.tm_sec = 20;
    time.tm_wday = 1;
    time.tm_yday = 200;
    time.tm_year = 9;

    timestr = asctime(&time);
    ok(!strcmp(timestr, explowtime), "Wrong time returned, got %s\n", timestr);
}

void Test_ctime()
{
    /* Test border ctime cases */
    time_t time;
    time = -15;
    ok(ctime(&time) == NULL, "ctime doesn't return NULL for invalid parameters\n");
    time = -5000000;
    ok(ctime(&time) == NULL,  "ctime doesn't return NULL for invalid parameters\n");
}

START_TEST(time)
{
    Test_asctime();
    Test_ctime();
}

