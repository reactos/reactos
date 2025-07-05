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

static const BOOL g_bWin8 = IsWindows8OrGreater();

static HRESULT
IsQSForwardMockup(_In_opt_ const GUID *pguidCmdGroup, _In_ ULONG cCmds, _In_ OLECMD *prgCmds)
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
            cmdID = (OLECMDID)prgCmds[iCmd].cmdID;
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
        if (!IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
        {
            if (g_bWin8)
                return OLECMDERR_E_UNKNOWNGROUP;
            else
                return OLECMDERR_E_NOTSUPPORTED;
        }

        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            cmdID = (OLECMDID)prgCmds[iCmd].cmdID;
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

static HRESULT
MayQSForwardMockup(
    _In_ IUnknown *lpUnknown,
    _In_ INT nUnknown,
    _In_opt_ const GUID *riidCmdGrp,
    _In_ ULONG cCmds,
    _Inout_ OLECMD *prgCmds,
    _Inout_ OLECMDTEXT *pCmdText)
{
    //TRACE("(%p, %d, %s, %d, %p, %p)\n",
    //      lpUnknown, nUnknown, wine_dbgstr_guid(riidCmdGrp), cCmds, prgCmds, pCmdText);

    HRESULT hr = IsQSForwardMockup(riidCmdGrp, cCmds, prgCmds);
    if (FAILED(hr) || !HRESULT_CODE(hr) || nUnknown <= 0)
        return OLECMDERR_E_NOTSUPPORTED;

    return IUnknown_QueryStatus(lpUnknown, *riidCmdGrp, cCmds, prgCmds, pCmdText);
}

static HRESULT
MayExecForwardMockup(
    _In_ IUnknown *lpUnknown,
    _In_ INT nUnknown,
    _In_opt_ const GUID *pguidCmdGroup,
    _In_ DWORD nCmdID,
    _In_ DWORD nCmdexecopt,
    _In_ VARIANT *pvaIn,
    _Inout_ VARIANT *pvaOut)
{
    //TRACE("(%p, %d, %s, %d, %d, %p, %p)\n", lpUnknown, nUnknown,
    //      wine_dbgstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt, pvaIn, pvaOut);

    OLECMD cmd = { nCmdID };
    HRESULT hr = IsQSForwardMockup(pguidCmdGroup, 1, &cmd);
    if (FAILED(hr) || !HRESULT_CODE(hr) || nUnknown <= 0)
        return OLECMDERR_E_NOTSUPPORTED;

    return IUnknown_Exec(lpUnknown, *pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
}

static BOOL g_bQueryStatus = FALSE;
static BOOL g_bExec = FALSE;

class CDummyClass : public IOleCommandTarget
{
public:
    CDummyClass() { }
    virtual ~CDummyClass() { }

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj) override
    {
        if (!ppvObj)
            return E_POINTER;
        *ppvObj = NULL;
        if (riid == IID_IUnknown || riid == IID_IOleCommandTarget)
            *ppvObj = this;
        if (*ppvObj)
        {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        return 2;
    }

    // ** IOleCommandTarget methods **
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText) override
    {
        g_bQueryStatus = TRUE;
        return 0xBEEFCAFE;
    }
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override
    {
        g_bExec = TRUE;
        return 0x2BAD2BAD;
    }

    static void *operator new(size_t size)
    {
        return LocalAlloc(LPTR, size);
    }
    static void operator delete(void *ptr)
    {
        LocalFree(ptr);
    }
    static void operator delete(void *ptr, size_t size)
    {
        LocalFree(ptr);
    }
};

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

        ret1 = IsQSForward(*pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

        ret1 = IsQSForward(*pGUID, cCmds, NULL);
        ret2 = IsQSForwardMockup(pGUID, cCmds, NULL);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
    }

    cCmds = 1;
    for (cmdID = LOW_VALUE; cmdID <= HIGH_VALUE; ++cmdID)
    {
        cmds[0].cmdID = cmdID;

        pGUID = NULL;
        ret1 = IsQSForward(*pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

        ret1 = ret2 = 0xDEADFACE;
        bExcept1 = bExcept2 = FALSE;

        _SEH2_TRY
        {
            ret1 = IsQSForward(*pGUID, cCmds, NULL);
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
        ret1 = IsQSForward(*pGUID, cCmds, cmds);
        ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
        ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

        pGUID = &IID_IUnknown;
        ret1 = IsQSForward(*pGUID, cCmds, cmds);
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
            ret1 = IsQSForward(*pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }

        pGUID = &CGID_Explorer;
        for (cmdID2 = LOW_VALUE; cmdID2 <= HIGH_VALUE; ++cmdID2)
        {
            cmds[1].cmdID = cmdID2;
            ret1 = IsQSForward(*pGUID, cCmds, cmds);
            ret2 = IsQSForwardMockup(pGUID, cCmds, cmds);
            ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
        }
    }
}

static VOID TEST_MayQSForwardMockup(VOID)
{
    CDummyClass dummy;
    LONG cmdID;
    OLECMD cmds[1];
    OLECMDTEXT cmdText;
    HRESULT ret1, ret2;

    // Testing OLECMDID_PROPERTIES support
    cmds[0].cmdID = cmdID = OLECMDID_PROPERTIES;

    g_bQueryStatus = g_bExec = FALSE;
    ret1 = MayQSForward(&dummy, 1, CGID_Explorer, _countof(cmds), cmds, &cmdText);
    ok_int(g_bQueryStatus, FALSE);
    ok_int(g_bExec, FALSE);

    g_bQueryStatus = g_bExec = FALSE;
    ret2 = MayQSForwardMockup(&dummy, 1, &CGID_Explorer, _countof(cmds), cmds, &cmdText);
    ok_int(g_bQueryStatus, FALSE);
    ok_int(g_bExec, FALSE);

    ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

    // Testing OLECMDID_SHOWFIND support
    cmds[0].cmdID = cmdID = OLECMDID_SHOWFIND;

    g_bQueryStatus = g_bExec = FALSE;
    ret1 = MayQSForward(&dummy, 1, CGID_Explorer, _countof(cmds), cmds, &cmdText);
    ok_int(g_bQueryStatus, TRUE);
    ok_int(g_bExec, FALSE);

    g_bQueryStatus = g_bExec = FALSE;
    ret2 = MayQSForwardMockup(&dummy, 1, &CGID_Explorer, _countof(cmds), cmds, &cmdText);
    ok_int(g_bQueryStatus, TRUE);
    ok_int(g_bExec, FALSE);

    ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
}

static VOID TEST_MayExecForwardMockup(VOID)
{
    CDummyClass dummy;
    LONG cmdID;
    HRESULT ret1, ret2;

    // Testing OLECMDID_PROPERTIES support
    cmdID = OLECMDID_PROPERTIES;

    g_bQueryStatus = g_bExec = FALSE;
    ret1 = MayExecForward(&dummy, 1, CGID_Explorer, cmdID, 0, NULL, NULL);
    ok_int(g_bQueryStatus, FALSE);
    ok_int(g_bExec, FALSE);

    g_bQueryStatus = g_bExec = FALSE;
    ret2 = MayExecForwardMockup(&dummy, 1, &CGID_Explorer, cmdID, 0, NULL, NULL);
    ok_int(g_bQueryStatus, FALSE);
    ok_int(g_bExec, FALSE);

    ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);

    // Testing OLECMDID_SHOWFIND support
    cmdID = OLECMDID_SHOWFIND;

    g_bQueryStatus = g_bExec = FALSE;
    ret1 = MayExecForward(&dummy, 1, CGID_Explorer, cmdID, 0, NULL, NULL);
    ok_int(g_bQueryStatus, FALSE);
    ok_int(g_bExec, TRUE);

    g_bQueryStatus = g_bExec = FALSE;
    ret2 = MayExecForwardMockup(&dummy, 1, &CGID_Explorer, cmdID, 0, NULL, NULL);
    ok_int(g_bQueryStatus, FALSE);
    ok_int(g_bExec, TRUE);

    ok(ret1 == ret2, "cmdID: %ld (%ld vs %ld)\n", cmdID, ret1, ret2);
}

START_TEST(IsQSForward)
{
    TEST_IsQSForward();
    TEST_MayQSForwardMockup();
    TEST_MayExecForwardMockup();
}
