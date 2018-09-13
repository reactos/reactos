/******************************************************************************

  Source File:  Profile Information Page.CPP

  This implements the class used to display the profile information page in the
  property page handler for the shell extension.

  Copyright (c) 1996 by Microsoft Corporation

  A Pretty Penny Enterprises Production

  Change History:
  11-01-96  a-robkj@microsoft.com pieced this one together for the first time

******************************************************************************/

#include    "ICMUI.H"

#include    "Resource.H"

static const TCHAR  sacDefaultCMM[] = _TEXT("icm32.dll");

//  It looks like the way to make the icon draw is to subclass the Icon control
//  in the window.  So, here's a Window Procedure for the subclass

//  CProfileInformationPage member functions

//  Class Constructor

CProfileInformationPage::CProfileInformationPage(HINSTANCE hiWhere,
                                                 LPCTSTR lpstrTarget) {
    m_pcpTarget = NULL;
    m_csProfile = lpstrTarget;
    m_psp.dwSize = sizeof m_psp;
    m_psp.dwFlags |= PSP_USETITLE;
    m_psp.hInstance = hiWhere;
    m_psp.pszTemplate = MAKEINTRESOURCE(ProfilePropertyPage);
    m_psp.pszTitle = MAKEINTRESOURCE(ProfilePropertyString);
}

//  Class destructor

CProfileInformationPage::~CProfileInformationPage() {
    if (m_pcpTarget) {
        delete m_pcpTarget;
    }
}

//  Dialog box (property sheet) initialization

BOOL    CProfileInformationPage::OnInit() {

    m_pcpTarget = new CProfile(m_csProfile);

    if (m_pcpTarget) {

        //  Retrieve the 'desc' key, and put it in the description field
        SetDlgItemTextA(m_hwnd, ProfileDescription,
            m_pcpTarget->TagContents('desc', 4));

        //  Get the copyright info from the 'cprt' tag
        SetDlgItemTextA(m_hwnd, ProfileProducerInfo,
            m_pcpTarget->TagContents('cprt'));

        //  Get the profile info from the 'vued' tag, not 'K007' tag
        LPCSTR lpAdditionalInfo = m_pcpTarget->TagContents('vued',4);

        if (lpAdditionalInfo) {
            SetDlgItemTextA(m_hwnd, AdditionalProfileInfo, lpAdditionalInfo);
        } else {
            CString csNoAdditionalInfo;
            csNoAdditionalInfo.Load(NoAdditionalInfo);
            SetDlgItemTextA(m_hwnd, AdditionalProfileInfo, (LPCSTR)csNoAdditionalInfo);
        }

        //  Set the CMM description and bitmap- these are supposed
        //  to come from the CMM.

        //  Get the CMM Name- this must be in char form

        union {
            char    acCMM[5];
            DWORD   dwCMM;
        };

        dwCMM = m_pcpTarget->GetCMM();
        acCMM[4] = '\0';

        //  Use it to form a key into the ICM registry.  If we find it, get
        //  the CMM name.  If we don't, then use the default CMM name (icm32)

#ifdef UNICODE
        CString csKey =
            CString(_TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\")) +
            (LPCTSTR) CString(acCMM);
#else
        CString csKey =
            CString(_TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ICM\\")) +
            (LPCTSTR) CString(acCMM);
#endif

        HKEY    hkCMM;

        if  (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, csKey, &hkCMM)) {
            TCHAR   acValue[MAX_PATH];

            dwCMM = MAX_PATH;

            if  (ERROR_SUCCESS == RegEnumValue(hkCMM, 0, acValue, &dwCMM, NULL,
                 NULL, NULL, NULL))
                 csKey = acValue;
            else
                csKey = sacDefaultCMM;

            RegCloseKey(hkCMM);
        }
        else
            csKey = sacDefaultCMM;

        //  See if we can get an instance handle for the DLL...

        HINSTANCE   hi = LoadLibrary(csKey);

        if  (!hi)
            return  TRUE;     //  Nothing to do, here, let the defaults prevail.

        //  Get description and icon identifier from CMS dll

        DWORD dwCMMIcon = 0, dwCMMDescription = 0;

typedef BOOL (*FPCMGETINFO)(DWORD);

        FPCMGETINFO fpCMGetInfo;

        fpCMGetInfo = (FPCMGETINFO) GetProcAddress(hi,"CMGetInfo");

        if (fpCMGetInfo) {

            dwCMMIcon = (*fpCMGetInfo)(CMM_LOGOICON);
            dwCMMDescription = (*fpCMGetInfo)(CMM_DESCRIPTION);

            if (dwCMMDescription) {
                //  Write the description, if there is one.
                csKey.Load(dwCMMDescription, hi);
                if  ((LPCTSTR) csKey)
                    SetDlgItemText(m_hwnd, CMMDescription, csKey);
            }

            if (dwCMMIcon) {
                //  Change/Create the Icon, if there is one.
                HICON   hiCMM = LoadIcon(hi, MAKEINTRESOURCE(dwCMMIcon));
                if  (hiCMM)
                    SendDlgItemMessage(m_hwnd, CMMIcon, STM_SETICON, (WPARAM) hiCMM, 0);
            }
        }

        return  TRUE;
    } else {
        return  FALSE;
    }
}

BOOL    CProfileInformationPage::OnDestroy() {

    if (m_pcpTarget) {
        delete m_pcpTarget;
        m_pcpTarget = (CProfile *) NULL;
    }

    return FALSE;  // still need to handle this message by def. proc.
}


