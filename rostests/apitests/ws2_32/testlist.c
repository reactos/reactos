/*
 * PROJECT:     ws2_32.dll API tests
 * LICENSE:     GPLv2 or any later version
 * FILE:        apitests/ws2_32/testlist.c
 * PURPOSE:     Test list file
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#ifndef _WS2_32_TESTLIST_H
#define _WS2_32_TESTLIST_H

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))

#include "ws2_32.h"

/* include the tests */
#include "tests/ioctlsocket.c"
#include "tests/recv.c"

/* The List of tests */
TESTENTRY TestList[] =
{
    { L"ioctlsocket", Test_ioctlsocket },
    { L"recv", Test_recv }
};

/* The function that gives us the number of tests */
INT NumTests(void)
{
    return ARRAY_SIZE(TestList);
}

#endif
