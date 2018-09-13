/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFMEM.C
 *  User interface dialogs for GROUP_MEM
 *
 *  History:
 *  Created 04-Jan-1993 1:10pm by Jeff Parsons
 */

#include "shellprv.h"
#pragma hdrstop

BINF abinfMem[] = {
    {IDC_HMA,           BITNUM(MEMINIT_NOHMA)   | 0x80},
    {IDC_GLOBALPROTECT, BITNUM(MEMINIT_GLOBALPROTECT) },
};

VINF avinfMem[] = {
    {FIELD_OFFSET(PROPMEM,wMinLow), VINF_AUTOMINMAX, IDC_LOWMEM, MEMLOW_MIN, MEMLOW_MAX, IDS_BAD_MEMLOW},
    {FIELD_OFFSET(PROPMEM,wMinEMS), VINF_AUTOMINMAX, IDC_EMSMEM, MEMEMS_MIN, MEMEMS_MAX, IDS_BAD_MEMEMS},
    {FIELD_OFFSET(PROPMEM,wMinXMS), VINF_AUTOMINMAX, IDC_XMSMEM, MEMXMS_MIN, MEMXMS_MAX, IDS_BAD_MEMXMS},
};

VINF avinfEnvMem[] = {
    {FIELD_OFFSET(PROPENV,cbEnvironment), VINF_AUTO, IDC_ENVMEM, ENVSIZE_MIN, ENVSIZE_MAX, IDS_BAD_ENVIRONMENT},
    {FIELD_OFFSET(PROPENV,wMaxDPMI), VINF_AUTO, IDC_DPMIMEM, ENVDPMI_MIN, ENVDPMI_MAX, IDS_BAD_MEMDPMI},
};

// Per-dialog data

#define MEMINFO_RELAUNCH        0x0001          // relaunch required to take effect

#define EMS_NOEMS               0x0001          // EMS no supported in protmode
#define EMS_EMM386              0x0002          // EM386 is installed
#define EMS_QEMM                0x0004          // Third-party mmgr installed
#define EMS_RMPAGEFRAME         0x0008          // Page frame present in real mode
#define EMS_SYSINIDISABLE       0x0010          // EMS forced off by system.ini

#ifdef WINNT
#define flEmsSupport() (EMS_EMM386 | EMS_RMPAGEFRAME)
#endif

typedef struct MEMINFO {        /* mi */
    PPROPLINK ppl;                              // pointer to property info
    DWORD     flMemInfo;                        // initially zero thx to LocalAlloc(LPTR)
    DWORD     flEms;                            // EMS support flags
} MEMINFO;
typedef MEMINFO *PMEMINFO;      /* pmi */


// Private function prototypes

BOOL GetSetMemProps(HWND hDlg, GETSETFN lpfn, PPROPLINK ppl, LPPROPMEM lpmem, LPPROPENV lpenv, int idError);
void InitMemDlg(HWND hDlg, PMEMINFO pmi);
void ApplyMemDlg(HWND hDlg, PMEMINFO pmi);
void AdjustEmsControls(HWND hDlg, PMEMINFO pmi);
void ExplainNoEms(HWND hDlg, PMEMINFO pmi);

// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
        IDC_CONVMEMLBL,      IDH_DOS_MEMORY_CONV,
        IDC_LOWMEM,          IDH_DOS_MEMORY_CONV,
        IDC_GLOBALPROTECT,   IDH_DOS_MEMORY_CONV_GLOBAL,
        IDC_EXPMEMGRP,       IDH_COMM_GROUPBOX,
        IDC_EXPMEMLBL,       IDH_DOS_MEMORY_EXP,
        IDC_EMSMEM,          IDH_DOS_MEMORY_EXP,
        IDC_EXTMEMGRP,       IDH_COMM_GROUPBOX,
        IDC_XMSMEM,          IDH_DOS_MEMORY_EXT,
        IDC_EXTMEMLBL,       IDH_DOS_MEMORY_EXT,
        IDC_DPMIMEMGRP,      IDH_COMM_GROUPBOX,
        IDC_DPMIMEM,         IDH_DOS_MEMORY_DPMI,
        IDC_DPMIMEMLBL,      IDH_DOS_MEMORY_DPMI,
        IDC_HMA,             IDH_DOS_MEMORY_EXT_HMA,
        IDC_CONVMEMGRP,      IDH_COMM_GROUPBOX,
        IDC_LOCALENVLBL,     IDH_DOS_PROGRAM_ENVIRSZ,
        IDC_ENVMEM,          IDH_DOS_PROGRAM_ENVIRSZ,
        IDC_REALMODEDISABLE, IDH_DOS_REALMODEPROPS,
        IDC_NOEMSDETAILS,    IDH_DOS_MEMORY_NOEMS_DETAILS,
        0, 0
};


BOOL_PTR CALLBACK DlgMemProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fError;
    PMEMINFO pmi;
    FunctionName(DlgMemProc);

    pmi = (PMEMINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        // allocate dialog instance data
        if (NULL != (pmi = (PMEMINFO)LocalAlloc(LPTR, SIZEOF(MEMINFO)))) {
            pmi->ppl = (PPROPLINK)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pmi);
            InitMemDlg(hDlg, pmi);
        } else {
            EndDialog(hDlg, FALSE);     // fail the dialog create
        }
        break;

    case WM_DESTROY:
        // free the pmi
        if (pmi) {
            EVAL(LocalFree(pmi) == NULL);
            SetWindowLongPtr(hDlg, DWLP_USER, 0);
        }
        break;

    HELP_CASES(rgdwHelp)                // Handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam)) {

        case IDC_ENVMEM:
        case IDC_LOWMEM:
        case IDC_EMSMEM:
        case IDC_XMSMEM:
        case IDC_DPMIMEM:
            if (HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_EDITCHANGE) {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                pmi->flMemInfo |= MEMINFO_RELAUNCH;
            }
            break;

        case IDC_HMA:
        case IDC_GLOBALPROTECT:
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                if (LOWORD(wParam) != IDC_GLOBALPROTECT)
                    pmi->flMemInfo |= MEMINFO_RELAUNCH;
            }
            break;

        case IDC_NOEMSDETAILS:
            if (HIWORD(wParam) == BN_CLICKED) {
                ExplainNoEms(hDlg, pmi);
            }
            return FALSE;               // return 0 if we process WM_COMMAND

        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            AdjustRealModeControls(pmi->ppl, hDlg);
            AdjustEmsControls(hDlg, pmi);
                                        // make sure DWL_MSGRESULT is zero,
                                        // otherwise the prsht code thinks we
                                        // "failed" this notify and switches
                                        // to another (sometimes random) page -JTP
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
            break;

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            fError = ValidateDlgInts(hDlg, avinfMem, ARRAYSIZE(avinfMem));
            fError |= ValidateDlgInts(hDlg, avinfEnvMem, ARRAYSIZE(avinfEnvMem));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, fError);
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyMemDlg(hDlg, pmi);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


BOOL GetSetMemProps(HWND hDlg, GETSETFN lpfn, PPROPLINK ppl, LPPROPMEM lpmem, LPPROPENV lpenv, int idError)
{
    if (!(*lpfn)(ppl, MAKELP(0,GROUP_MEM),
                        lpmem, SIZEOF(*lpmem), GETPROPS_NONE) ||
        !(*lpfn)(ppl, MAKELP(0,GROUP_ENV),
                        lpenv, SIZEOF(*lpenv), GETPROPS_NONE)) {
        Warning(hDlg, (WORD)idError, (WORD)MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    return TRUE;
}


void InitMemDlg(HWND hDlg, PMEMINFO pmi)
{
    PROPMEM mem;
    PROPENV env;
    PPROPLINK ppl = pmi->ppl;
    FunctionName(InitMemDlg);

    if (!GetSetMemProps(hDlg, PifMgr_GetProperties, ppl, &mem, &env, IDS_QUERY_ERROR))
        return;

    SetDlgBits(hDlg, abinfMem, ARRAYSIZE(abinfMem), mem.flMemInit);
    SetDlgInts(hDlg, avinfMem, ARRAYSIZE(avinfMem), (LPVOID)&mem);
    SetDlgInts(hDlg, avinfEnvMem, ARRAYSIZE(avinfEnvMem), (LPVOID)&env);

    /* Disallow "None" as a valid setting for "Conventional memory" */
    SendDlgItemMessage(hDlg, IDC_LOWMEM, CB_DELETESTRING,
        (WPARAM)SendDlgItemMessage(hDlg, IDC_LOWMEM, CB_FINDSTRING,
                                   (WPARAM)-1, (LPARAM)(LPTSTR)g_szNone), 0L);

    pmi->flEms = flEmsSupport();
    AdjustEmsControls(hDlg, pmi);
}


void ApplyMemDlg(HWND hDlg, PMEMINFO pmi)
{
    PROPMEM mem;
    PROPENV env;
    PPROPLINK ppl = pmi->ppl;
    FunctionName(ApplyMemDlg);

    if (!GetSetMemProps(hDlg, PifMgr_GetProperties, ppl, &mem, &env, IDS_UPDATE_ERROR))
        return;

    GetDlgBits(hDlg, abinfMem, ARRAYSIZE(abinfMem), &mem.flMemInit);
    GetDlgInts(hDlg, avinfMem, ARRAYSIZE(avinfMem), (LPVOID)&mem);
    GetDlgInts(hDlg, avinfEnvMem, ARRAYSIZE(avinfEnvMem), (LPVOID)&env);

    if (GetSetMemProps(hDlg, PifMgr_SetProperties, ppl, &mem, &env, IDS_UPDATE_ERROR)) {
        if (ppl->hwndNotify) {
            ppl->flProp |= PROP_NOTIFY;
            PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(mem), (LPARAM)MAKELP(0,GROUP_MEM));
            PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(env), (LPARAM)MAKELP(0,GROUP_ENV));
        }
        if (ppl->hVM && (pmi->flMemInfo & MEMINFO_RELAUNCH)) {
            pmi->flMemInfo &= ~MEMINFO_RELAUNCH;
            Warning(hDlg, IDS_MEMORY_RELAUNCH, MB_ICONWARNING | MB_OK);
        }
    }
}

void HideAndDisable(HWND hwnd)
{
    ShowWindow(hwnd, SW_HIDE);
    EnableWindow(hwnd, FALSE);
}

void AdjustEmsControls(HWND hDlg, PMEMINFO pmi)
{
    if (!(pmi->ppl->flProp & PROP_REALMODE)) {
        /*
         *  When not marked as PROP_REALMODE, all the EMS-related controls
         *  are visible.  We need to choose which set to disable.
         *
         *  We cheat, because we know that there are only two controls
         *  in both cases, and they come right after each other.
         */
        UINT uiHide;
        if (pmi->flEms & EMS_NOEMS) {
            uiHide = IDC_EXPMEMLBL;
            CTASSERTF(IDC_EXPMEMLBL + 1 == IDC_EMSMEM);
        } else {
            uiHide = IDC_NOEMS;
            CTASSERTF(IDC_NOEMS + 1 == IDC_NOEMSDETAILS);
        }
        HideAndDisable(GetDlgItem(hDlg, uiHide));
        HideAndDisable(GetDlgItem(hDlg, uiHide+1));
    }
}


void ExplainNoEms(HWND hDlg, PMEMINFO pmi)
{
    WORD idsHelp;
    TCHAR szMsg[MAX_STRING_SIZE];

    /*
     * Here is where we stare at all the bits to try to figure
     * out what recommendation to make.
     */
    ASSERTTRUE(pmi->flEms & EMS_NOEMS);

    if (pmi->flEms & EMS_SYSINIDISABLE) {
        /*
         * System.ini contains the line NOEMMDRIVER=1.
         */
        idsHelp = IDS_SYSINI_NOEMS;
    } else if (pmi->flEms & EMS_RMPAGEFRAME) {
        /*
         * Had page-frame in real mode, which means that some protmode
         * guy must've screwed it up.
         */
        idsHelp = IDS_RING0_NOEMS;
    } else if (pmi->flEms & EMS_EMM386) {
        /*
         * No page-frame in real mode, and EMM386 was in charge,
         * so it's EMM386's fault.
         */
        idsHelp = IDS_EMM386_NOEMS;
    } else {
        /*
         * No page-frame in real mode, and QEMM was in charge,
         * so it's QEMM's fault.
         */
        idsHelp = IDS_QEMM_NOEMS;
    }

    if (LoadStringSafe(hDlg, idsHelp+1, szMsg, ARRAYSIZE(szMsg))) {
        Warning(hDlg, idsHelp, MB_OK, (LPCTSTR)szMsg);
    }
}
