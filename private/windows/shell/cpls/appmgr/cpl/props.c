/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    props.c

Abstract:

    This module contains the code for application properties.

Author:

    Dave Hastings (daveh) creation-date 23-Nov-1997

Revision History:

--*/
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

BOOL CALLBACK
InstallPropDlg(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL CALLBACK
GenPropDlg(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL CALLBACK
OfflinePropDlg(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
ShowProperties(
    PAPPLICATIONDESCRIPTOR AppDescriptor,
    HWND Parent
    )
{
    PIPAPPLICATIONPROPERTIES Properties;
    WCHAR ProductName[256];
    WCHAR ProductVersion[256];
    WCHAR Publisher[256];
    WCHAR ProductID[256];
    WCHAR RegisteredOwner[256];
    WCHAR RegisteredCompany[256];
    WCHAR Language[256];
    WCHAR SupportUrl[256];
    WCHAR SupportTelephone[256];
    WCHAR HelpFile[256];
    WCHAR InstallLocation[256];
    WCHAR InstallSource[256];
    WCHAR RequiredByPolicy[256];
    WCHAR AdministrativeContact[256];
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;

    Properties.ProductNameSize = 256;
    Properties.ProductName = ProductName;
    Properties.ProductVersionSize = 256;
    Properties.ProductVersion = ProductVersion;
    Properties.PublisherSize = 256;
    Properties.Publisher = Publisher;
    Properties.ProductIDSize = 256;
    Properties.ProductID = ProductID;
    Properties.RegisteredOwnerSize = 256;
    Properties.RegisteredOwner = RegisteredOwner;
    Properties.RegisteredCompanySize = 256;
    Properties.RegisteredCompany = RegisteredCompany;
    Properties.LanguageSize = 256;
    Properties.Language = Language;
    Properties.SupportUrlSize = 256;
    Properties.SupportUrl = SupportUrl;
    Properties.SupportTelephoneSize = 256;
    Properties.SupportTelephone = SupportTelephone;
    Properties.HelpFileSize = 256;
    Properties.HelpFile = HelpFile;
    Properties.InstallLocationSize = 256;
    Properties.InstallLocation = InstallLocation;
    Properties.InstallSourceSize = 256;
    Properties.InstallSource = InstallSource;
    Properties.RequiredByPolicySize = 256;
    Properties.RequiredByPolicy = RequiredByPolicy;
    Properties.AdministrativeContactSize = 256;
    Properties.AdministrativeContact = AdministrativeContact;

    AppDescriptor->Actions.GetProperties(
        AppDescriptor->Identifier,
        &Properties
        );

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_DEFAULT;
    psp[0].hInstance = Instance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_GENERAL_PROPERTIES);
    psp[0].pfnDlgProc = GenPropDlg;
    psp[0].lParam = (LPARAM)&Properties;
    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_DEFAULT;
    psp[1].hInstance = Instance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_INSTALL_PROPERTIES);
    psp[1].pfnDlgProc = InstallPropDlg;
    psp[1].lParam = (LPARAM)&Properties;
    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_DEFAULT;
    psp[2].hInstance = Instance;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_OFFLINE_PROPERTIES);
    psp[2].pfnDlgProc = OfflinePropDlg;
    psp[2].lParam = (LPARAM)&Properties;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
    psh.hwndParent = Parent;
    psh.hInstance = Instance;
    psh.pszCaption = L"";
    psh.nPages = 3;
    psh.nStartPage = 0;
    psh.ppsp = psp;

    PropertySheet(&psh);

}

BOOL CALLBACK
GenPropDlg(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This is the dialog function for the general properties page.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    PPIPAPPLICATIONPROPERTIES Properties;

    if (Msg == WM_INITDIALOG) {
        Properties = ((PPIPAPPLICATIONPROPERTIES)((LPPROPSHEETPAGE)lParam)->lParam);

        //
        // Set the text for the properties
        //
        SetDlgItemText(Dialog, IDC_PRODUCT_NAME, Properties->ProductName);
        SetDlgItemText(Dialog, IDC_VERSION, Properties->ProductVersion);
        SetDlgItemText(Dialog, IDC_PUBLISHER, Properties->Publisher);
        SetDlgItemText(Dialog, IDC_PRODUCT_ID, Properties->ProductID);
        SetDlgItemText(Dialog, IDC_REGISTERED_OWNER, Properties->RegisteredOwner);
        SetDlgItemText(Dialog, IDC_REGISTERED_COMPANY, Properties->RegisteredCompany);
        SetDlgItemText(Dialog, IDC_LANGUAGE, Properties->Language);
        SetDlgItemText(Dialog, IDC_SUPPORT_URL, Properties->SupportUrl);
        SetDlgItemText(Dialog, IDC_SUPPORT_TELEPHONE, Properties->SupportTelephone);
        SetDlgItemText(Dialog, IDC_HELP_FILE, Properties->HelpFile);

    }

    return FALSE;
}

BOOL CALLBACK
InstallPropDlg(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PPIPAPPLICATIONPROPERTIES Properties;

    if (Msg == WM_INITDIALOG) {
        Properties = ((PPIPAPPLICATIONPROPERTIES)((LPPROPSHEETPAGE)lParam)->lParam);

        //
        // Set the text for the properties
        //
        SetDlgItemText(Dialog, IDC_INSTALL_LOCATION, Properties->InstallLocation);
        SetDlgItemText(Dialog, IDC_INSTALL_SOURCE, Properties->InstallSource);
        SetDlgItemText(Dialog, IDC_REQUIRED_POLICY, Properties->RequiredByPolicy);
        SetDlgItemText(Dialog, IDC_ADMINISTRATIVE_CONTACT, Properties->AdministrativeContact);
    }

    return FALSE;
}

BOOL CALLBACK
OfflinePropDlg(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This is the dialog function for the offline properties page.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    if (Msg == WM_INITDIALOG) {

        SendMessage(
            GetDlgItem(Dialog, IDC_RADIO_ONLINE), 
            BM_SETCHECK, 
            (WPARAM)BST_CHECKED, 
            0
            );
    }

    return FALSE;
}