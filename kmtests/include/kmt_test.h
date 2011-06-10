/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite test declarations
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#ifndef _KMTEST_TEST_H_
#define _KMTEST_TEST_H_

#include <kmt_log.h>

typedef void KMT_TESTFUNC(void);

typedef struct
{
    const char *TestName;
    KMT_TESTFUNC *TestFunction;
} KMT_TEST, *PKMT_TEST;

typedef const KMT_TEST CKMT_TEST, *PCKMT_TEST;

extern const KMT_TEST TestList[];

#endif /* !defined _KMTEST_TEST_H_ */
