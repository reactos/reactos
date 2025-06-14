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
#include <pseh/pseh2.h>
#include <versionhelpers.h>

static BOOL g_bVista = FALSE;

static HRESULT
IsQSForwardMockup(_In_opt_ REFGUID pguidCmdGroup, _In_ ULONG cCmds, _In_ OLECMD *prgCmds)
{
    DWORD cmdFlags = 0;
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
                cmdFlags |= CMD_FLAG_NOT_SUPPORTED; // Not supported
                continue;
            }

            if (cmdID <= OLECMDID_PASTE || cmdID == OLECMDID_SELECTALL)
            {
                cmdFlags |= CMD_FLAG_SUPPORTED_BASIC;
                continue;
            }

            if (cmdID <= OLECMDID_UPDATECOMMANDS ||
                (OLECMDID_HIDETOOLBARS <= cmdID && cmdID != OLECMDID_ENABLE_INTERACTION))
            {
                cmdFlags |= CMD_FLAG_NOT_SUPPORTED; // Not supported
                continue;
            }

            cmdFlags |= CMD_FLAG_SUPPORTED_ADVANCED;
        }
    }
    else
    {
        if (!IsEqualGUID(&CGID_Explorer, pguidCmdGroup))
        {
            if (g_bVista)
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
                cmdFlags |= CMD_FLAG_SUPPORTED_BASIC;
                break;
            }
        }
    }

    if (!cmdFlags || (cmdFlags & CMD_FLAG_NOT_SUPPORTED))
        return OLECMDERR_E_NOTSUPPORTED; // Not supported

    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, cmdFlags);
}

static VOID TEST_IsQSForward(VOID)
{
    OLECMD cmds[2];
    LONG cmdID, cmdID2;
    HRESULT ret1, ret2;
    ULONG cCmds;
    BOOL bExcept1, bExcept2;
    const GUID *pGUID;
    enum { LOW_VALUE = -1, HIGH_VALUE = OLECMDID_CLOSE };

    cmds[0].cmdf = 0;

    cCmds = 0;
    pGUID = NULL;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;

        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

        ret1 = IsQSForward(pGUID, cCmds, NULL);
        ret2 = IsQSForwardMockup(pGUID, cCmds, NULL);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    cCmds = 1;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;

        pGUID = NULL;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

        ret1 = ret2 = 0xDEADFACE;
        bExcept1 = bExcept2 = FALSE;

        _SEH2_TRY
        {
            ret1 = IsQSForward(pGUID, cCmds, NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            bExcept1 = TRUE;
        }
        _SEH2_END;

        _SEH2_TRY
        {
            ret2 = IsQSForwardMockup(pGUID, cCmds, NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            bExcept2 = TRUE;
        }
        _SEH2_END;

        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        ok((bExcept1 && bExcept2), "cmdID: %ld (%d vs %d)\n", cmdID, bExcept1, bExcept2);

        pGUID = &CGID_Explorer;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

        pGUID = &IID_IUnknown;
        ret1 = IsQSForward(pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    cCmds = 2;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;

        pGUID = NULL;
        for (cmdID2 = LOW_VALUE; cmdID2 <= HIGH_VALUE; ++cmdID2)
        {
            cmds[1].cmdID = cmdID2;
            ret1 = IsQSForward(pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }

        pGUID = &CGID_Explorer;
        for (cmdID2 = LOW_VALUE; cmdID2 <= HIGH_VALUE; ++cmdID2)
        {
            cmds[1].cmdID = cmdID2;
            ret1 = IsQSForward(pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }
    }
}

START_TEST(IsQSForward)
{
    g_bVista = IsWindowsVistaOrGreater();

    TEST_IsQSForward();
}
