/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for GetCPInfo
 * PROGRAMMER:      Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

START_TEST(GetCPInfo)
{
    CPINFO CpInfo;
    BOOL Ret;

    Ret = GetCPInfo(CP_ACP, &CpInfo);
    ok_int(Ret, TRUE);

    memset(&CpInfo, 0xAA, sizeof(CpInfo));
    Ret = GetCPInfo(-1, &CpInfo);
    ok_int(Ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);
    ok_int(CpInfo.MaxCharSize, 0xAAAAAAAA);

    memset(&CpInfo, 0xAA, sizeof(CpInfo));
    Ret = GetCPInfo(CP_ACP, NULL);
    ok_int(Ret, FALSE);
    ok_err(ERROR_INVALID_PARAMETER);
    ok_int(CpInfo.MaxCharSize, 0xAAAAAAAA);

    memset(&CpInfo, 0xAA, sizeof(CpInfo));
    Ret = GetCPInfo(1361,  &CpInfo);
    ok_int(Ret, TRUE);
    ok_int(CpInfo.MaxCharSize, 2);
    ok_char(CpInfo.DefaultChar[0], 0x3F);
    ok_char(CpInfo.DefaultChar[1], 0x00);
    ok_char(CpInfo.LeadByte[0], 0x84);
    ok_char(CpInfo.LeadByte[1], 0xD3);
    ok_char(CpInfo.LeadByte[2], 0xD8);
    ok_char(CpInfo.LeadByte[3], 0xDE);
    ok_char(CpInfo.LeadByte[4], 0xE0);
    ok_char(CpInfo.LeadByte[5], 0xF9);
    ok_char(CpInfo.LeadByte[6], 0);
    ok_char(CpInfo.LeadByte[7], 0);
    ok_char(CpInfo.LeadByte[8], 0);
    ok_char(CpInfo.LeadByte[9], 0);
    ok_char(CpInfo.LeadByte[10], 0);
    ok_char(CpInfo.LeadByte[11], 0);

}
