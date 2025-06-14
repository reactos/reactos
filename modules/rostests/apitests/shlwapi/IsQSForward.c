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

static DWORD
IsQSForwardMockup(_In_opt_ REFGUID pguidCmdGroup, _In_ ULONG cCmds, _In_ OLECMD *prgCmds)
{
    DWORD ret = 0;
    OLECMDID cmdID;
    ULONG iCmd;
    // FIXME: Give these flags better names
    enum { SUPPORTED_1 = 0x1, SUPPORTED_2 = 0x2, NOT_SUPPORTED = 0x4 };

    if ((LONG)cCmds <= 0)
        return OLECMDERR_E_NOTSUPPORTED;

    if (!pguidCmdGroup)
    {
        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            cmdID = prgCmds[iCmd].cmdID;
            if (cmdID <= OLECMDID_PROPERTIES)
            {
                ret |= NOT_SUPPORTED; // Not supported
                continue;
            }

            if (cmdID <= OLECMDID_PASTE || cmdID == OLECMDID_SELECTALL)
            {
                ret |= SUPPORTED_1;
                continue;
            }

            if (cmdID <= OLECMDID_UPDATECOMMANDS ||
                (OLECMDID_HIDETOOLBARS <= cmdID && cmdID != OLECMDID_ENABLE_INTERACTION))
            {
                ret |= NOT_SUPPORTED; // Not supported
                continue;
            }

            ret |= SUPPORTED_2;
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
                ret |= SUPPORTED_1;
                break;
            }
        }
    }

    if (!ret || (ret & NOT_SUPPORTED))
        return OLECMDERR_E_NOTSUPPORTED; // Not supported

    return ret;
}

START_TEST(IsQSForward)
{
    OLECMD cmds[1];
    LONG cmdID;
    LONG ret1, ret2;
    ULONG cCmds;
    const GUID *pGUID = NULL;

    cmds[0].cmdf = 0;

    cCmds = 0;
    for (cmdID = -999; cmdID <= OLECMDID_MEDIA_PLAYBACK; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    cCmds = _countof(cmds);
    for (cmdID = -999; cmdID <= OLECMDID_MEDIA_PLAYBACK; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    pGUID = &CGID_Explorer;
    for (cmdID = -999; cmdID <= OLECMDID_MEDIA_PLAYBACK; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    pGUID = &IID_IUnknown;
    for (cmdID = -999; cmdID <= OLECMDID_MEDIA_PLAYBACK; ++cmdID)
    {
        cmds[0].cmdID = cmdID;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }
}
