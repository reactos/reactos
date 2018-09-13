#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgSveDef.h"


// JMC: This is taken from access.cpl

/***********************************************************************/
// CopyKey( hKey, hKeyDst, name )
//     create the destination key
//     for each value
//         CopyValue
//     for each subkey
//         CopyKey

DWORD CopyKey( HKEY hkeySrc, HKEY hkeyDst, LPSTR szKey )
{
    HKEY hkeyOld, hkeyNew;
    char szValue[128];
    BYTE szData[128];
    char szBuffer[128];
    DWORD iStatus;
    UINT nValue, nKey;
    DWORD iValueLen, iDataLen;
	DWORD dwType;

    iStatus = RegOpenKeyA( hkeySrc, szKey, &hkeyOld );
    if( iStatus != ERROR_SUCCESS )
        return iStatus;
    iStatus = RegOpenKeyA( hkeyDst, szKey, &hkeyNew );
    if( iStatus != ERROR_SUCCESS )
    {
        iStatus = RegCreateKeyA( hkeyDst, szKey, &hkeyNew );
        if( iStatus != ERROR_SUCCESS )
        {
            RegCloseKey( hkeyOld );
            return iStatus;
        }
    }
    //*********** copy the values **************** //

    for( nValue = 0, iValueLen=sizeof szValue, iDataLen=sizeof szValue;
         ERROR_SUCCESS == (iStatus = RegEnumValueA(hkeyOld,
                                                  nValue,
                                                  szValue,
                                                  &iValueLen,
                                                  NULL, // reserved
                                                  &dwType, // don't need type
                                                  szData,
                                                  &iDataLen ) );
         nValue ++, iValueLen=sizeof szValue, iDataLen=sizeof szValue )
     {
         iStatus = RegSetValueExA( hkeyNew,
                                  szValue,
                                  0, // reserved
                                  dwType,
                                  szData,
                                  iDataLen);
     }
    if( iStatus != ERROR_NO_MORE_ITEMS )
    {
        RegCloseKey( hkeyOld );
        RegCloseKey( hkeyNew );
        return iStatus;
    }

    //*********** copy the subtrees ************** //

    for( nKey = 0;
         ERROR_SUCCESS == (iStatus = RegEnumKeyA(hkeyOld,nKey,szBuffer,sizeof(szBuffer)));
         nKey ++ )
     {
         iStatus = CopyKey( hkeyOld, hkeyNew, szBuffer );
         if( iStatus != ERROR_NO_MORE_ITEMS && iStatus != ERROR_SUCCESS )
            {
                RegCloseKey( hkeyOld );
                RegCloseKey( hkeyNew );
                return iStatus;
            }
     }
    RegCloseKey( hkeyOld );
    RegCloseKey( hkeyNew );
    if( iStatus == ERROR_NO_MORE_ITEMS )
        return ERROR_SUCCESS;
    else
        return iStatus;
}

DWORD SaveLookToDefaultUser( void )
{
    DWORD iStatus;
    HKEY hkeyDst;

    iStatus  = RegOpenKeyA( HKEY_USERS, ".DEFAULT", &hkeyDst );
    if( iStatus != ERROR_SUCCESS )
        return iStatus;
    iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, "Control Panel\\Desktop");
    iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, "Control Panel\\Colors");
    RegCloseKey( hkeyDst );
    return iStatus;
}

DWORD SaveAccessibilityToDefaultUser( void )
{
    DWORD iStatus;
    HKEY hkeyDst;

    iStatus  = RegOpenKeyA( HKEY_USERS, ".DEFAULT", &hkeyDst );
    if( iStatus != ERROR_SUCCESS )
        return iStatus;
    iStatus = CopyKey( HKEY_CURRENT_USER, hkeyDst, "Control Panel\\Accessibility");
    RegCloseKey( hkeyDst );
    return iStatus;
}



CSaveForDefaultUserPg::CSaveForDefaultUserPg( 
    LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZSAVEASDEFAULTTITLE, IDS_WIZSAVEASDEFAULTSUBTITLE)
{
	m_dwPageId = IDD_WIZWORKSTATIONDEFAULT;
    ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CSaveForDefaultUserPg::~CSaveForDefaultUserPg(
    VOID
    )
{
}

LRESULT CSaveForDefaultUserPg::OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
{
	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_CHECKSAVESETTINGTODEFAULT)))
	{
		SaveAccessibilityToDefaultUser();
		// JMC Check for admin privleges for both callse
		if(ERROR_SUCCESS != SaveLookToDefaultUser())
			StringTableMessageBox(m_hwnd, IDS_WIZERRORNEEDADMINTEXT, IDS_WIZERRORNEEDADMINTITLE, MB_OK);
	}
	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}

LRESULT
CSaveForDefaultUserPg::OnCommand(
								 HWND hwnd,
								 WPARAM wParam,
								 LPARAM lParam
								 )
{
	LRESULT lResult = 1;
	
	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID 	 = LOWORD(wParam);
	HWND hwndCtl	 = (HWND)lParam;
	
	return lResult;
}

LRESULT
CSaveForDefaultUserPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIO2), TRUE);
	return 1;
}
