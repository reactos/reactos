//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       pages.cxx
//
//  Contents:   property pages for provider, domain/workgroup, server, share
//
//  History:    26-Sep-95        BruceFo     Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "resource.h"
#include "helpids.h"
#include "ext.hxx"
#include "pages.hxx"
#include "util.hxx"

////////////////////////////////////////////////////////////////////////////

//
//  This is the minimum version number necessary to
//  actually display a version number.  If we get a
//  machine with a major version number less that this
//  value, we don't display the version number.
//

#define MIN_DISPLAY_VER  2

////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CPage::PageCallback, static public
//
//  Synopsis:
//
//--------------------------------------------------------------------------

UINT CALLBACK
CPage::PageCallback(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE ppsp
    )
{
    switch (uMsg)
    {
    case PSPCB_CREATE:
        return 1;       // allow creation

    case PSPCB_RELEASE:
    {
        CPage* pThis = (CPage*)ppsp->lParam;
        delete pThis;   // do this LAST!
        return 0;       // ignored
    }

    default:
        appDebugOut((DEB_ERROR, "CPage::PageCallback, unknown page callback message %d\n", uMsg));
        return 0;

    } // end switch
}

//+-------------------------------------------------------------------------
//
//  Method:     CPage::DlgProcPage, static public
//
//  Synopsis:   Dialog Procedure for all CPage
//
//--------------------------------------------------------------------------

BOOL CALLBACK
CPage::DlgProcPage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CPage* pThis = NULL;

    if (msg==WM_INITDIALOG)
    {
        PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
        pThis = (CPage*)psp->lParam;
        SetWindowLong(hwnd,GWL_USERDATA,(LPARAM)pThis);
    }
    else
    {
        pThis = (CPage*) GetWindowLong(hwnd,GWL_USERDATA);
    }

    if (pThis != NULL)
    {
        return pThis->_PageProc(hwnd,msg,wParam,lParam);
    }
    else
    {
        return FALSE;
    }
}


//+--------------------------------------------------------------------------
//
//  Method:     CPage::CPage, public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------------------

CPage::CPage(
    IN CNetObj* pNetObj
    )
    :
    _pNetObj(pNetObj)
{
    INIT_SIG(CPage);

    if (NULL != _pNetObj)
    {
        //
        // We need to addref this because we are using member variable
        // of this class.
        //
        _pNetObj->AddRef();
    }
}


//+--------------------------------------------------------------------------
//
//  Method:     CPage::~CPage, public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------------------

CPage::~CPage()
{
    CHECK_SIG(CPage);

    if (NULL != _pNetObj)
    {
        _pNetObj->Release();
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::InitInstance, public
//
//  Synopsis:   Part II of the constuctor process
//
//  Notes:      We don't want to handle any errors in constuctor, so this
//              method is necessary for the second phase error detection.
//
//--------------------------------------------------------------------------

HRESULT
CPage::InitInstance(
    VOID
    )
{
    CHECK_SIG(CPage);
    appDebugOut((DEB_ITRACE, "CPage::InitInstance\n"));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::_PageProc, private
//
//  Synopsis:   Dialog Procedure for this object
//
//--------------------------------------------------------------------------

BOOL
CPage::_PageProc(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    CHECK_SIG(CPage);

    static DWORD aHelpIds[] =
    {
        IDC_NETWORK,        IDH_NET_NETWORK,
        IDC_NETWORK_ICON,   IDH_NET_NETWORK,
        IDC_SERVER,         IDH_NET_SERVER,
        IDC_SERVER_ICON,    IDH_NET_SERVER,
        IDC_SERVER_TEXT,    IDH_NET_SERVER,
        IDC_COMMENT,        IDH_NET_COMMENT,
        IDC_COMMENT_TEXT,   IDH_NET_COMMENT,
        IDC_WORKGROUP_OR_DOMAIN,   IDH_NET_WORKGROUP_OR_DOMAIN_NAME,
        IDC_WORKGROUP_OR_DOMAIN_NAME,   IDH_NET_WORKGROUP_OR_DOMAIN_NAME,
        IDC_WORKGROUP_OR_DOMAIN_NAME_2_ICON, IDH_NET_WORKGROUP_OR_DOMAIN_NAME_2,
        IDC_WORKGROUP_OR_DOMAIN_NAME_2, IDH_NET_WORKGROUP_OR_DOMAIN_NAME_2,
        IDC_TYPE,           IDH_NET_TYPE,
        IDC_TYPE_TEXT,      IDH_NET_TYPE,
        IDC_SHARE,          IDH_NET_SHARE,
        IDC_SHARE_ICON,     IDH_NET_SHARE,
        IDC_WRKGRP_TYPE,    IDH_NET_WRKGRP_TYPE,
        IDC_WRKGRP_TYPE_TEXT,   IDH_NET_WRKGRP_TYPE,
        0,0
    };

    switch (msg)
    {
    case WM_INITDIALOG:
        _hwndPage = hwnd;
        return _OnInitDialog(hwnd, (HWND)wParam, lParam);

    case WM_NOTIFY:
        return _OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam);

    case WM_HELP:
    {
        LPHELPINFO lphi = (LPHELPINFO)lParam;

        if (lphi->iContextType == HELPINFO_WINDOW)  // a control
        {
            WCHAR szHelp[50];
            LoadString(g_hInstance, IDS_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
            WinHelp(
                (HWND)lphi->hItemHandle,
                szHelp,
                HELP_WM_HELP,
                (DWORD)(LPVOID)aHelpIds);
        }
        break;
    }

    case WM_CONTEXTMENU:
    {
        WCHAR szHelp[50];
        LoadString(g_hInstance, IDS_HELPFILENAME, szHelp, ARRAYLEN(szHelp));
        WinHelp(
            (HWND)wParam,
            szHelp,
            HELP_CONTEXTMENU,
            (DWORD)(LPVOID)aHelpIds);
        break;
    }

    } // end switch (msg)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::_OnInitDialog, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CPage::_OnInitDialog(
    IN HWND hwnd,
    IN HWND hwndFocus,
    IN LPARAM lInitParam
    )
{
    CHECK_SIG(CPage);
    appDebugOut((DEB_ITRACE, "_OnInitDialog\n"));

    appAssert(NULL != _pNetObj);
    LPNETRESOURCE pnr = (LPNETRESOURCE)_pNetObj->_bufNetResource;
    appAssert(NULL != pnr);

    if (RESOURCEDISPLAYTYPE_NETWORK == pnr->dwDisplayType)
    {
        return _OnInitNetwork(hwnd, pnr);
    }
    else if (RESOURCEDISPLAYTYPE_DOMAIN == pnr->dwDisplayType)
    {
        return _OnInitDomain(hwnd, pnr);
    }
    else if (RESOURCEDISPLAYTYPE_SERVER == pnr->dwDisplayType)
    {
        return _OnInitServerOrShare(hwnd, pnr, TRUE);
    }
    else if (RESOURCEDISPLAYTYPE_SHARE == pnr->dwDisplayType)
    {
        return _OnInitServerOrShare(hwnd, pnr, FALSE);
    }
    else
    {
        appAssert(!"CNetObj::AddPages. Unknown net resource type!\n");
        return TRUE;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::_OnInitNetwork, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CPage::_OnInitNetwork(
    IN HWND hwnd,
    IN LPNETRESOURCE pnr
    )
{
    CHECK_SIG(CPage);
    appAssert(NULL != pnr);

    SetDlgItemText(hwnd, IDC_NETWORK, pnr->lpProvider);
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::_OnInitDomain, private
//
//  Synopsis:   WM_INITDIALOG handler
//
//--------------------------------------------------------------------------

BOOL
CPage::_OnInitDomain(
    IN HWND hwnd,
    LPNETRESOURCE pnr
    )
{
    CHECK_SIG(CPage);
    appAssert(NULL != pnr);

    SetDlgItemText(hwnd, IDC_WORKGROUP_OR_DOMAIN_NAME_2, pnr->lpRemoteName);
    SetDlgItemText(hwnd, IDC_WRKGRP_TYPE,              pnr->lpProvider);
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPage::_OnInitServerOrShare, private
//
//  Synopsis:   WM_INITDIALOG handler for Server and Share objects
//
//--------------------------------------------------------------------------

BOOL
CPage::_OnInitServerOrShare(
    IN HWND hwnd,
    IN LPNETRESOURCE pnr,
    IN BOOL fServer         // TRUE == server, FALSE == share
    )
{
    CHECK_SIG(CPage);
    appAssert(NULL != pnr);

    CWaitCursor wait;

    NET_API_STATUS err;
    LPTSTR pszRealServerName = pnr->lpRemoteName;
    appAssert(NULL != pszRealServerName);

    // Figure out server name without UNC prefix
    if (pszRealServerName[0] == TEXT('\\') && pszRealServerName[1] == TEXT('\\'))
    {
        pszRealServerName += 2;
    }

    if (!fServer)
    {
        // Get share name
        LPTSTR pszShareName = wcschr(pszRealServerName, TEXT('\\'));
        appAssert(NULL != pszShareName);
        *pszShareName++ = TEXT('\0');    // NOTE: NULL-terminating server name
        SetDlgItemText(hwnd, IDC_SHARE, pszShareName);
    }

    SetDlgItemText(hwnd, IDC_SERVER, pszRealServerName);

    // Get server information
    PSERVER_INFO_101 pServerInfo = NULL;
    err = NetServerGetInfo(pnr->lpRemoteName, 101, (LPBYTE*)&pServerInfo);
    if (NERR_Success != err)
    {
        MyErrorDialog(hwnd, MessageFromError(err), pszRealServerName);
        return TRUE;
    }

    // NOTE: I now have a pServerInfo to delete

    appAssert(NULL != pServerInfo);

    if (fServer)
    {
        if (NULL != pServerInfo->sv101_comment)
        {
            SetDlgItemText(hwnd, IDC_COMMENT, pServerInfo->sv101_comment);
        }
    }

    _SetServerType(hwnd, IDC_TYPE, pServerInfo);

    DWORD svtype = pServerInfo->sv101_type; // save type for later use
    NetApiBufferFree(pServerInfo);          // get rid of it so we don't have to worry about it

    UINT idLogonType = IDS_LOGON_WORKGROUP;
    if (svtype & SV_TYPE_NT)
    {
        // It's an NT server. See if it is in a workgroup or a domain.
        LPWSTR pszDomainName;
        BOOL bIsWorkgroupName;
        err = MyNetpGetDomainNameEx(pnr->lpRemoteName, &pszDomainName, &bIsWorkgroupName);
        if (err != NERR_Success)
        {
            MyErrorDialog(hwnd, MessageFromError(err), pszRealServerName);
            return TRUE;
        }
        SetDlgItemText(hwnd, IDC_WORKGROUP_OR_DOMAIN_NAME, pszDomainName);
        NetApiBufferFree(pszDomainName);

        if (!bIsWorkgroupName)
        {
            idLogonType = IDS_LOGON_DOMAIN;
        }
    }
    else
    {
        // It's not an NT server, so assume it's a workgroup.
        PWKSTA_INFO_100 pWorkstationInfo = NULL;
        err = NetWkstaGetInfo(pnr->lpRemoteName, 100, (LPBYTE*)&pWorkstationInfo);
        if (NERR_Success != err)
        {
            MyErrorDialog(hwnd, MessageFromError(err), pszRealServerName);
            return TRUE;
        }

        appAssert(NULL != pWorkstationInfo);

        if (NULL != pWorkstationInfo->wki100_langroup)
        {
            SetDlgItemText(hwnd, IDC_WORKGROUP_OR_DOMAIN_NAME, pWorkstationInfo->wki100_langroup);
        }
        NetApiBufferFree(pWorkstationInfo);
    }

    WCHAR szLogonType[80];
    LoadString(g_hInstance, idLogonType, szLogonType, ARRAYLEN(szLogonType));
    SetDlgItemText(hwnd, IDC_WORKGROUP_OR_DOMAIN, szLogonType);

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::_OnNotify, private
//
//  Synopsis:   WM_NOTIFY handler
//
//--------------------------------------------------------------------------

BOOL
CPage::_OnNotify(
    IN HWND hwnd,
    IN int idCtrl,
    IN LPNMHDR phdr
    )
{
    CHECK_SIG(CPage);

    switch (phdr->code)
    {
    case PSN_APPLY:
    case PSN_RESET:         // cancel
        SetWindowLong(hwnd, DWL_MSGRESULT, FALSE); // go away
        return TRUE;

    case PSN_KILLACTIVE:    // change to another page
        SetWindowLong(hwnd, DWL_MSGRESULT, PSNRET_NOERROR);
        return FALSE;

    case PSN_SETACTIVE:
    case PSN_QUERYCANCEL:
        return FALSE;

    } // end switch (phdr->code)

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPage::_SetServerType, private
//
//  Synopsis:   Sets a server type control
//
//--------------------------------------------------------------------------

VOID
CPage::_SetServerType(
    HWND hwnd,
    int idControl,
    PSERVER_INFO_101 pinfo
    )
{
    UINT idType, idRole;
    HRESULT msgFormat;

    //
    //  Determine the server's type (NT, LM, etc) and role.
    //

    DWORD type = pinfo->sv101_type;

    if (type & SV_TYPE_NT)
    {
        idType = IDS_SERVER_TYPE_WINNT;
    }
    else
    if (type & SV_TYPE_WINDOWS)
    {
        idType = IDS_SERVER_TYPE_WINDOWS95;
    }
    else
    if (type & SV_TYPE_WFW)
    {
        idType = IDS_SERVER_TYPE_WFW;
    }
    else
    {
        idType = IDS_SERVER_TYPE_LANMAN;
    }

    if (type & SV_TYPE_DOMAIN_CTRL)
    {
        idRole = IDS_ROLE_PRIMARY;
    }
    else
    if (type & SV_TYPE_DOMAIN_BAKCTRL)
    {
        idRole = IDS_ROLE_BACKUP;
    }
    else
    if (type & SV_TYPE_SERVER_NT)
    {
        idRole = IDS_ROLE_SERVER;
    }
    else
    {
        idRole = IDS_ROLE_WKSTA;
    }

    UINT verMajor = pinfo->sv101_version_major & MAJOR_VERSION_MASK;
    UINT verMinor = pinfo->sv101_version_minor;

    msgFormat = (verMajor < MIN_DISPLAY_VER || (type & SV_TYPE_WINDOWS))
                    ? MSG_TYPE_FORMAT_UNKNOWN
                    : MSG_TYPE_FORMAT
                    ;

    WCHAR szType[80];
    LoadString(g_hInstance, idType, szType, ARRAYLEN(szType));

    WCHAR szRole[80];
    LoadString(g_hInstance, idRole, szRole, ARRAYLEN(szRole));

    DWORD aInserts[4];
    aInserts[0] = (DWORD)szType;
    aInserts[1] = verMajor;
    aInserts[2] = verMinor;
    aInserts[3] = (DWORD)szRole;

    WCHAR szBuffer[256];
    DWORD dwReturn = FormatMessage(
                             FORMAT_MESSAGE_FROM_HMODULE
                                | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                             g_hInstance,
                             msgFormat,
                             LANG_SYSTEM_DEFAULT,
                             szBuffer,
                             ARRAYLEN(szBuffer),
                             (va_list*)aInserts);
    if (0 == dwReturn)   // couldn't find message
    {
        appDebugOut((DEB_IERROR,
            "FormatMessage failed, 0x%08lx\n",
            GetLastError()));

        szBuffer[0] = TEXT('\0');
    }

    SetDlgItemText(hwnd, idControl, szBuffer);
}
