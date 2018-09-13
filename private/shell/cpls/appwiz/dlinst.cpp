//
// Downlevel (NT4, win9X) Install/Unistall page
//

#include "priv.h"
#include "appwizid.h"
#include "dlinst.h"
#include "sccls.h"

//
// DonwLevelManager: Ugly singleton class
//
// Mainly there to keep state info and for its destructor
//

class CDLManager* g_pDLManager = NULL;

class CDLManager
{
public:
    // Rely on the fact that shell "new" zero out memory
    CDLManager() : _hrInit(E_FAIL)
    {
        _hrInit = CoInitialize(0);
        _szStatic[0] = 0;
        _szStatic2[0] = 0;
        _uiStatic = 0;
    }
    ~CDLManager()
    {
        if (_peia)
            _peia->Release();

        if (SUCCEEDED(_hrInit))
            CoUninitialize();
    }
public:
    void InitButtonsHandle(HWND hwndPage)
    {
        // No check for success: check before using

        if (!_hwndModifyUninstall)
        {
            _hwndModifyUninstall = GetDlgItem(hwndPage, IDC_MODIFYUNINSTALL);
            _rghwndButtons[IDC_MODIFY-IDC_BASEBUTTONS]    = GetDlgItem(hwndPage, IDC_MODIFY);
            _rghwndButtons[IDC_REPAIR-IDC_BASEBUTTONS]    = GetDlgItem(hwndPage, IDC_REPAIR);
            _rghwndButtons[IDC_UNINSTALL-IDC_BASEBUTTONS] = GetDlgItem(hwndPage, IDC_UNINSTALL);
        }
    }
    void SetVisibleButtons(BOOL bShow3Buttons)
    {
        // bShow3Buttons == TRUE will show the three buttons

        if (_hwndModifyUninstall)
            ShowWindow(_hwndModifyUninstall, bShow3Buttons?SW_HIDE:SW_SHOW);

        for (int i=0;i<3;++i)
        {
            if (_rghwndButtons[i])
                ShowWindow(_rghwndButtons[i], bShow3Buttons?SW_SHOW:SW_HIDE);
        }
    }
public:
    IEnumInstalledApps* _peia;

    HWND _hwndModifyUninstall;
    HWND _rghwndButtons[3];

    HRESULT _hrInit;

    TCHAR _szStatic[250];
    TCHAR _szStatic2[50];

    UINT _uiStatic;
};

//
// pcApps has to be already initialized, we only increment it
//
STDAPI DL_FillAppListBox(HWND hwndListBox, DWORD* pdwApps)
{
    ASSERT(IsWindow(hwndListBox));

    static CDLManager DLManager;

    g_pDLManager = &DLManager;

    ASSERT(g_pDLManager);

    HRESULT hres = E_FAIL;

    IShellAppManager * pam;

    if (SUCCEEDED(g_pDLManager->_hrInit))
    {
        hres = CoCreateInstance(CLSID_ShellAppManager, NULL, CLSCTX_INPROC_SERVER,
            IID_IShellAppManager, (LPVOID *)&pam);

        if (SUCCEEDED(hres))
        {
            // Initialize InstalledApp Enum if required

            if (!g_pDLManager->_peia)
                hres = pam->EnumInstalledApps(&g_pDLManager->_peia);

            if (SUCCEEDED(hres))
            {
                IInstalledApp* pia;

                while ((hres = g_pDLManager->_peia->Next(&pia)) == S_OK)
                {
                    APPINFODATA ais = {0};
                    ais.cbSize = sizeof(ais);
                    ais.dwMask = AIM_DISPLAYNAME;

                    pia->GetAppInfo(&ais);
            
                    if (ais.dwMask & AIM_DISPLAYNAME)
                    {
                        int iIndex = LB_ERR;
#ifdef UNICODE
                        iIndex = ListBox_AddString(hwndListBox, ais.pszDisplayName);
#else
                        PCHAR pszTmp = NULL;

                        // Get the size

                        int cbSize = WideCharToMultiByte(CP_ACP, 0, ais.pszDisplayName, -1, pszTmp, 0,
                            NULL, NULL);

                        if (cbSize)
                        {
                            pszTmp = (LPTSTR)LocalAlloc(LPTR, cbSize);

                            if (pszTmp)
                            {
                                cbSize = WideCharToMultiByte(CP_ACP, 0, ais.pszDisplayName, -1, pszTmp, cbSize,
                                    NULL, NULL);

                                if (cbSize)
                                    iIndex = ListBox_AddString(hwndListBox, pszTmp);

                                LocalFree(pszTmp);
                            }
                        }
#endif

                        // Did the operation succeed?
                        if (LB_ERR != iIndex)
                        {
                            // Is memory OK?
                            if (LB_ERRSPACE != iIndex)
                            {
                                // Yes
                                ListBox_SetItemData(hwndListBox, iIndex, pia);

                                ++(*pdwApps);
                            }
                            else
                            {
                                // No, better get out
                                pia->Release();
                                break;                         
                            }
                        }
                    }
                    else
                        pia->Release();

                }
            }
            pam->Release();
        }
    }

    return hres;
}

STDAPI_(BOOL) DL_ConfigureButtonsAndStatic(HWND hwndPage, HWND hwndListBox, int iSel)
{
    ASSERT(IsWindow(hwndPage));
    ASSERT(IsWindow(hwndListBox));
    ASSERT(0 <= iSel);

    UINT uiStatic = IDS_UNINSTINSTR_LEGACY;

    BOOL fret = FALSE;

    if (LB_ERR != iSel)
    {
        LRESULT lres = ListBox_GetItemData(hwndListBox, iSel);

        if (LB_ERR != lres)
        {
            fret = TRUE;

            IInstalledApp* pia = (IInstalledApp*)lres;

            DWORD dwActions = 0;

            pia->GetPossibleActions(&dwActions);

            dwActions &= (APPACTION_MODIFY|APPACTION_REPAIR|APPACTION_UNINSTALL|APPACTION_MODIFYREMOVE);

            g_pDLManager->InitButtonsHandle(hwndPage);

            if (dwActions & APPACTION_MODIFYREMOVE)
            {
                // Manage to show the right buttons

                g_pDLManager->SetVisibleButtons(FALSE);

                EnableWindow(g_pDLManager->_hwndModifyUninstall, TRUE);
            }
            else
            {
                if (dwActions & (APPACTION_MODIFY|APPACTION_REPAIR|APPACTION_UNINSTALL))
                {
                    // Manage to show the right buttons

                    g_pDLManager->SetVisibleButtons(TRUE);

                    // Enable the applicable buttons

                    EnableWindow(g_pDLManager->_rghwndButtons[IDC_MODIFY-IDC_BASEBUTTONS],
                        (dwActions&APPACTION_MODIFY)?TRUE:FALSE);

                    EnableWindow(g_pDLManager->_rghwndButtons[IDC_REPAIR-IDC_BASEBUTTONS],
                        (dwActions&APPACTION_REPAIR)?TRUE:FALSE);

                    EnableWindow(g_pDLManager->_rghwndButtons[IDC_UNINSTALL-IDC_BASEBUTTONS],
                        (dwActions&APPACTION_UNINSTALL)?TRUE:FALSE);

                    uiStatic = IDS_UNINSTINSTR_NEW;
                }
                else
                {
                    // Manage to show the right buttons

                    g_pDLManager->SetVisibleButtons(FALSE);

                    EnableWindow(g_pDLManager->_hwndModifyUninstall, FALSE);
                }
            }
        }
    }

    if (!(*g_pDLManager->_szStatic))
    {
        if(!LoadString(g_hinst, IDS_UNINSTINSTR, g_pDLManager->_szStatic, ARRAYSIZE(g_pDLManager->_szStatic)))
            *(g_pDLManager->_szStatic) = 0;
    }   
    
    if (*g_pDLManager->_szStatic && (g_pDLManager->_uiStatic != uiStatic))
    {
        TCHAR szMergedStatic[250];

        LoadString(g_hinst, uiStatic, g_pDLManager->_szStatic2, ARRAYSIZE(g_pDLManager->_szStatic2));

        wsprintf(szMergedStatic, g_pDLManager->_szStatic, g_pDLManager->_szStatic2);

        SetDlgItemText(hwndPage, IDC_UNINSTINSTR, szMergedStatic);

        g_pDLManager->_uiStatic = uiStatic;
    }

    return fret;
}

STDAPI_(BOOL) DL_InvokeAction(int iButtonID, HWND hwndPage, HWND hwndListBox, int iSel)
{
    BOOL fret = FALSE;

    // Get app from listbox selection

    LRESULT lres = ListBox_GetItemData(hwndListBox, iSel);
    
    if (LB_ERR != lres)
    {
        fret = TRUE;

        IInstalledApp* pia = (IInstalledApp*)lres;

        // Invoke action from button ID

        if (pia)
        {

            HWND hwndPropSheet = GetParent(hwndPage);

            ::EnableWindow(hwndPropSheet, FALSE);

            switch(iButtonID)
            {
                case IDC_MODIFY:
                    pia->Modify(hwndPropSheet);
                    break;
                case IDC_REPAIR:
                    // Pass FALSe, we don't want to reinstall, only repair
                    pia->Repair(FALSE);
                    break;
                case IDC_MODIFYUNINSTALL:
                case IDC_UNINSTALL:
                    pia->Uninstall(hwndPropSheet);
                    break;
                default:
                    //???
                    break;
            }

            ::EnableWindow(hwndPropSheet , TRUE);
        }
    }

    return fret;
}
