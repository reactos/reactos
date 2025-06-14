/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for IsQSForward
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <docobj.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <versionhelpers.h>

static HRESULT
IsQSForwardMockup(_In_opt_ REFGUID pguidCmdGroup, _In_ ULONG cCmds, _In_ OLECMD *prgCmds)
{
    HRESULT ret = S_OK;
    OLECMDID cmdID;
    ULONG iCmd;
    enum {
        CMD_FLAG_SUPPORTED_BASIC = 0x1,
        CMD_FLAG_SUPPORTED_ADVANCED = 0x2,
        CMD_FLAG_NOT_SUPPORTED = 0x4,
    };

    //TRACE("(%s, %lu, %p)\n", wine_dbgstr_guid(pguidCmdGroup), cCmds, prgCmds);

    if ((LONG)cCmds <= 0)
        return OLECMDERR_E_NOTSUPPORTED;

    if (!pguidCmdGroup)
    {
        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            cmdID = prgCmds[iCmd].cmdID;
            if (cmdID <= OLECMDID_PROPERTIES)
            {
                ret |= CMD_FLAG_NOT_SUPPORTED; // Not supported
                continue;
            }

            if (cmdID <= OLECMDID_PASTE || cmdID == OLECMDID_SELECTALL)
            {
                ret |= CMD_FLAG_SUPPORTED_BASIC;
                continue;
            }

            if (cmdID <= OLECMDID_UPDATECOMMANDS ||
                (OLECMDID_HIDETOOLBARS <= cmdID && cmdID != OLECMDID_ENABLE_INTERACTION))
            {
                ret |= CMD_FLAG_NOT_SUPPORTED; // Not supported
                continue;
            }

            ret |= CMD_FLAG_SUPPORTED_ADVANCED;
        }
    }
    else
    {
        if (!IsEqualGUID(&CGID_Explorer, pguidCmdGroup))
        {
            if (IsWindowsVistaOrGreater())
                return OLECMDERR_E_UNKNOWNGROUP;
            else
                return OLECMDERR_E_NOTSUPPORTED;
        }

        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            cmdID = prgCmds[iCmd].cmdID;
            if (cmdID == OLECMDID_SELECTALL ||
                (OLECMDID_SHOWFIND <= cmdID && cmdID <= OLECMDID_SHOWPRINT))
            {
                ret |= CMD_FLAG_SUPPORTED_BASIC;
                break;
            }
        }
    }

    if (!ret || (ret & CMD_FLAG_NOT_SUPPORTED))
        return OLECMDERR_E_NOTSUPPORTED; // Not supported

    return ret;
}

START_TEST(IsQSForward)
{
    OLECMD cmds[2];
    LONG cmdID, cmdID2;
    HRESULT ret1, ret2;
    ULONG cCmds;
    const GUID *pGUID = NULL;
    enum { LOW_VALUE = -99, HIGH_VALUE = OLECMDID_MEDIA_PLAYBACK };

    cmds[0].cmdf = 0;

    cCmds = 0;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    cCmds = 1;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    pGUID = &CGID_Explorer;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    pGUID = &IID_IUnknown;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    cCmds = 2;
    pGUID = NULL;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        for (cmdID2 = LOW_VALUE; cmdID2 <= HIGH_VALUE; ++cmdID2)
        {
            cmds[0].cmdID = cmdID;
            cmds[1].cmdID = cmdID2;
            ret1 = IsQSForward(pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }
    }

    pGUID = &CGID_Explorer;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        for (cmdID2 = LOW_VALUE; cmdID2 <= HIGH_VALUE; ++cmdID2)
        {
            cmds[1].cmdID = cmdID2;
            ret1 = IsQSForward(pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }
    }

    pGUID = &IID_IUnknown;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        for (cmdID2 = LOW_VALUE; cmdID2 <= HIGH_VALUE; ++cmdID2)
        {
            cmds[1].cmdID = cmdID2;
            ret1 = IsQSForward(pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }
    }
}
