/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for SEH
 * COPYRIGHT:   Copyright 2020 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <apitest.h>
#include "stdio.h"
#include <pseh/pseh2.h>

int seh0001();
int seh0002();
int seh0003();
int seh0004();
int seh0005();
int seh0006();
int seh0007();
int seh0008();
int seh0009();
int seh0010();
int seh0011();
int seh0012();
int seh0013();
int seh0014();
int seh0015();
int seh0016();
int seh0017();
int seh0018();
int seh0019();
int seh0020();
int seh0021();
int seh0022();
int seh0023();
int seh0024();
int seh0025();
int seh0026();
int seh0027();
int seh0028();
int seh0029();
int seh0030();
int seh0031();
int seh0032();
int seh0033();
int seh0034();
int seh0035();
int seh0036();
int seh0037();
int seh0038();
int seh0039();
int seh0040();
int seh0041();
int seh0042();
int seh0043();
int seh0044();
int seh0045();
int seh0046();
int seh0047();
int seh0048();
int seh0049();
int seh0050();
int seh0051();
int seh0052();
int seh0053();
int seh0054();
int seh0055();
int seh0056();
int seh0057();
int seh0058();

#define run_test(test) \
    _SEH2_TRY \
    { \
        ok_int(test(), 0); \
    } \
    _SEH2_EXCEPT(1) \
    { \
        ok(0, "Exception while running test " #test "\n"); \
    } \
    _SEH2_END

START_TEST(ms_seh)
{
    run_test(seh0001);
    run_test(seh0002);
    run_test(seh0003);
    run_test(seh0004);
    run_test(seh0005);
    run_test(seh0006);
    run_test(seh0007);
    run_test(seh0008);
    run_test(seh0009);
    run_test(seh0010);
    run_test(seh0011);
    run_test(seh0012);
    run_test(seh0013);
    run_test(seh0014);
    run_test(seh0015);
    run_test(seh0016);
    run_test(seh0017);
    run_test(seh0018);
    run_test(seh0019);
    run_test(seh0020);
    run_test(seh0021);
    run_test(seh0022);
    run_test(seh0023);
    run_test(seh0024);
    run_test(seh0025);
    run_test(seh0026);
    run_test(seh0027);
    run_test(seh0028);
    run_test(seh0029);
    run_test(seh0030);
    run_test(seh0031);
    run_test(seh0032);
    run_test(seh0033);
    run_test(seh0034);
    run_test(seh0035);
    run_test(seh0036);
    run_test(seh0037);
    run_test(seh0038);
    run_test(seh0039);
    run_test(seh0040);
    run_test(seh0041);
    run_test(seh0042);
    run_test(seh0043);
    run_test(seh0044);
    run_test(seh0045);
    run_test(seh0046);
    run_test(seh0047);
    run_test(seh0048);
    run_test(seh0049);
    run_test(seh0050);
    run_test(seh0051);
    run_test(seh0052);
    run_test(seh0053);
    run_test(seh0054);
#if !defined(_PSEH3_H_)
    run_test(seh0055);
#endif
    run_test(seh0056);
    run_test(seh0057);
    run_test(seh0058);
}
