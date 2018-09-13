/**************************************************************************\
 * Module Name: general.cpp
 *
 * Contains all the code to manage multiple devices
 *
 * Copyright (c) Microsoft Corp.  1995-1996 All Rights Reserved
 *
 * NOTES:
 *
 * History: Create by dli on 7/21/97
 *
 \**************************************************************************/


#include "precomp.h"
#include "settings.hxx"


#define DCDSF_DYNA (0x0001)
#define DCDSF_ASK  (0x0002)

#define DCDSF_PROBABLY      (DCDSF_ASK  | DCDSF_DYNA)
#define DCDSF_PROBABLY_NOT  (DCDSF_ASK  |          0)
#define DCDSF_YES           (0          | DCDSF_DYNA)
#define DCDSF_NO            (0          |          0)

#define COMPATWARN_DONT 1

#ifndef WINNT
// codes for SelectFontSize
#define DPI_INVALID   (-1)
#define DPI_FROMCOMBO (-2)
#define DPI_CUSTOM    (-3)
#endif

extern "C" {
extern BOOL CALLBACK CustomFontDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
}

//BUGBUG: (dli) This should be put in regstr.h
#define REGSTR_VAL_DYNACOLORCHANGE TEXT("DynaColorChange")
#define REGSTR_VAL_COMPATWARNING   TEXT("MultiMonitorCompatibilityWarning")
static const TCHAR sc_szDeskAppletSoftwareKey[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display");
static const TCHAR sc_szRegFontSize[]           = REGSTR_PATH_SETUP TEXT("\\") REGSTR_VAL_FONTSIZE;
static const TCHAR sc_szQuickResRegName[]       = TEXT("Taskbar Display Controls");
static const TCHAR sc_szQuickResCommandLine[]  = TEXT("RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY");
static const TCHAR sc_szQuickResClass[]  = TEXT("SysQuickRes");
static const char c_szQuickResCommandLine[]  = "RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY";
#ifndef WINNT
static const TCHAR sc_szBoot[] = TEXT("boot");
static const TCHAR sc_szSystemIni[] = TEXT("system.ini");
static const TCHAR sc_szFonts[] = TEXT("fonts");
static const TCHAR sc_szBootDesc[] = TEXT("boot.description");
static const TCHAR sc_szAspect[] = TEXT("aspect");
#endif

//-----------------------------------------------------------------------------
static const DWORD sc_GeneralHelpIds[] =
{
   // General Page
    ID_DSP_FONTSIZEGRP, IDH_GENERAL_FONTSIZEGRP,
    IDC_DYNA,           IDH_GENERAL_DYNA,
    IDC_NODYNA,         IDH_GENERAL_NODYNA, 
    IDC_YESDYNA,        IDH_GENERAL_YESDYNA,  
    IDC_SHUTUP,        IDH_GENERAL_SHUTUP,   
    0, 0
};

/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/

int GetDisplayCPLPreference(LPCTSTR szRegVal)
{
    int val = -1;
    HKEY hk;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, sc_szDeskAppletSoftwareKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        TCHAR sz[64];
        DWORD cb = sizeof(sz);

        *sz = 0;
        if ((RegQueryValueEx(hk, szRegVal, NULL, NULL,
            (LPBYTE)sz, &cb) == ERROR_SUCCESS) && *sz)
        {
            val = (int)MyStrToLong(sz);
        }

        RegCloseKey(hk);
    }

    if (val == -1 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, sc_szDeskAppletSoftwareKey, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        TCHAR sz[64];
        DWORD cb = sizeof(sz);

        *sz = 0;
        if ((RegQueryValueEx(hk, szRegVal, NULL, NULL,
            (LPBYTE)sz, &cb) == ERROR_SUCCESS) && *sz)
        {
            val = (int)MyStrToLong(sz);
        }

        RegCloseKey(hk);
    }

    return val;
}

int GetDynaCDSPreference()
{
//DLI: until we figure out if this command line stuff is still needed. 
//    if (g_fCommandLineModeSet)
//        return DCDSF_YES;

    int iRegVal = GetDisplayCPLPreference(REGSTR_VAL_DYNACOLORCHANGE);
    if (iRegVal == -1)
        iRegVal = DCDSF_PROBABLY_NOT;
    return iRegVal;
}

void SetDisplayCPLPreference(LPCTSTR szRegVal, int val)
{
    HKEY hk;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, sc_szDeskAppletSoftwareKey, 0, TEXT(""), 0, KEY_WRITE, NULL, &hk, NULL) ==
        ERROR_SUCCESS)
    {
        TCHAR sz[64];

        wsprintf(sz, TEXT("%d"), val);
        RegSetValueEx(hk, szRegVal, NULL, REG_SZ,
            (LPBYTE)sz, lstrlen(sz) + 1);

        RegCloseKey(hk);
    }
}


/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/
class CGeneralDlg {
    private:
        int _idCustomFonts;
        int _iDynaOrg;
        HWND _hwndFontList;
        HWND _hDlg;
        //
        // current log pixels of the screen.
        // does not change !
        int _cLogPix;
        BOOL _fForceSmallFont;
        BOOL _InitFontList();
        void _InstallQuickRes(BOOL fEnable);
        void _InitQuickResCheckbox();
        void _ApplyQuickResCheckbox();
    public:
        CGeneralDlg(BOOL fFoceSmallFont);
        void InitGeneralDlg(HWND hDlg);
        void SetFontSizeText( int cdpi );
        BOOL ChangeFontSize();
        BOOL HandleGeneralApply(HWND hDlg);
        void HandleFontSelChange();
        void ForceSmallFont();
        LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

CGeneralDlg::CGeneralDlg(BOOL fFoceSmallFont) : _fForceSmallFont(fFoceSmallFont), _idCustomFonts(-1)
{
    HKEY hkFont;
    DWORD cb;
    //
    // For font size just always use the one of the current screen.
    // Whether or not we are testing the current screen.
    //

    _cLogPix = 96;

    //
    // If the size does not match what is in the registry, then install
    // the new one
    //
#ifdef WINNT
    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      SZ_FONTDPI_PROF,
                      0,
                      KEY_READ,
                      &hkFont) == ERROR_SUCCESS) ||
        (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      SZ_FONTDPI,
                      0,
                      KEY_READ,
                      &hkFont) == ERROR_SUCCESS))

    {

        cb = sizeof(DWORD);

        if (RegQueryValueEx(hkFont,
                            SZ_LOGPIXELS,
                            NULL,
                            NULL,
                            (LPBYTE) &_cLogPix,
                            &cb) != ERROR_SUCCESS) {

            _cLogPix = 96;

        }

#else
        TCHAR szBuf[10];

   if (RegOpenKeyEx(HKEY_CURRENT_CONFIG, REGSTR_PATH_DISPLAYSETTINGS, 0, KEY_READ, &hkFont) == ERROR_SUCCESS)
   {
       cb = SIZEOF(szBuf);
       
       if (RegQueryValueEx(hkFont, REGSTR_VAL_DPILOGICALX, NULL, NULL, (LPBYTE) szBuf, &cb) == ERROR_SUCCESS)
       {
           _cLogPix = MyStrToLong(szBuf);
       }
       else
       {
           _cLogPix = 96;
       }   

#endif
       RegCloseKey(hkFont);

    }
};

#ifndef WINNT

#define ABS(x) ((x) < 0 ? -(x) : (x))

/*
** given an arbitrary DPI, find the fontsize set that should be used
** for the DPI.  enumerate the possible sizes and choose the closest one.
*/
BOOL NEAR PASCAL DeskCPLFontSize_GetRealSize(HKEY hkFontSize, LPCTSTR lpFontSize, LPTSTR lpRealSize)
{
    HKEY hk;
    TCHAR szBuf[100];
    int i;
    
    if (RegOpenKeyEx(hkFontSize, (LPTSTR)lpFontSize, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        // the font size is in there as is.  use it.
        lstrcpy(lpRealSize, lpFontSize);
        RegCloseKey(hk);
    }
    else
    {
        int iWant, iBest, iCur;
        DWORD cchBuf = SIZEOF(szBuf);
        iBest = 0;
        iWant = MyStrToLong(lpFontSize);
        // find closest match
        for (i=0; ; i++)
        {
            // enumerate the fonts
            if (RegEnumKeyEx(hkFontSize,i,(LPTSTR)szBuf,&cchBuf, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                break;  // Bail if no more keys.

            iCur = MyStrToLong(szBuf);
            if ((!iBest) || (ABS(iBest - iWant) > ABS(iCur - iWant)))
            {
                iBest = iCur;
            }
        }

        if (!iBest)
            return FALSE;

        wsprintf(lpRealSize, TEXT("%d"), iBest);
    }
    return TRUE;
}

// NOTE: (dli) We should really call the SetupX api Display_SetFontSize for this
// but we don't want to go through thunking. So directly access the registry 
BOOL DeskCPLSetFontSize(LPCTSTR lpszFontSize)
{
    TCHAR szBuf[100], szRealSize[50];
    TCHAR szLHS[100], szRHS[100];
    HKEY hkFontSize=NULL, hkIniStuff=NULL, hkSettings=NULL,hk=NULL;
    BOOL bRet = FALSE;
    DWORD dw;
    int i;
    HDC hdc;
    DWORD cbData1, cbData2;
    DWORD cbLHS;

    //
    //  validate the font size.
    //
    i = MyStrToLong(lpszFontSize);
    if (i < 50 || i > 1000)
        goto error;
    
    // open the fonts key.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sc_szRegFontSize, 0, KEY_READ, &hkFontSize) != ERROR_SUCCESS)
        goto error;
    
    //  open the HKEY_CURRENT_CONFIG\Display\Settings key
    //
    if (RegCreateKeyEx(HKEY_CURRENT_CONFIG, REGSTR_PATH_DISPLAYSETTINGS, 0, TEXT(""), 0, KEY_WRITE, NULL, &hkSettings, NULL) != ERROR_SUCCESS)
        goto error;

    // first, get the "real" fontsize to be installed
    if (!DeskCPLFontSize_GetRealSize(hkFontSize, lpszFontSize, szRealSize))
        goto error;
    /*
    ** set up the new fontsize
    */
    // first, changes to system.ini
    lstrcpy(szBuf, szRealSize);
    lstrcat(szBuf, TEXT("\\"));
    lstrcat(szBuf, REGSTR_KEY_SYSTEM);
    
    if (RegOpenKeyEx(hkFontSize, szBuf, 0, KEY_READ, &hkIniStuff) != ERROR_SUCCESS)
        goto error;

    if (RegCreateKeyEx(HKEY_CURRENT_CONFIG, REGSTR_PATH_DISPLAYSETTINGS,  0, TEXT(""), 0, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
        goto error;

    for (dw=0; ; dw++)
    {
        cbData1 = SIZEOF(szLHS);
        cbData2 = SIZEOF(szRHS);
        if (RegEnumValue(hkIniStuff, dw, szLHS, &cbData1, NULL, NULL, (LPBYTE)szRHS, &cbData2) != ERROR_SUCCESS)
            break;

        // remember, the registry has LH and RH reversed here
        cbLHS = lstrlen(szLHS) * SIZEOF(TCHAR);
        RegSetValueEx(hk, szRHS, 0, REG_SZ, (LPBYTE)szLHS, cbLHS);

        // erase (or write) the value in SYSTEM.INI
        //
        // we want to write the system fonts to SYSTEM.INI even though we
        // dont read them from there.  so old stupid apps will still work.
        //
        WritePrivateProfileString(sc_szBoot, szRHS, szLHS, sc_szSystemIni);
    }
    RegCloseKey(hkIniStuff);
    RegCloseKey(hk);

    // second, changes to win.ini
    lstrcpy(szBuf, szRealSize);
    lstrcat(szBuf, TEXT("\\"));
    lstrcat(szBuf, REGSTR_KEY_USER);
    if (RegOpenKeyEx(hkFontSize, szBuf, 0, KEY_READ, &hkIniStuff) != ERROR_SUCCESS)
        goto error;

    RegDeleteKey(HKEY_CURRENT_CONFIG, REGSTR_PATH_FONTS);

    if (RegCreateKeyEx(HKEY_CURRENT_CONFIG, REGSTR_PATH_FONTS, 0, TEXT(""), 0, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
        goto error;

    for (dw=0; ; dw++)
    {
        cbData1 = SIZEOF(szLHS);
        cbData2 = SIZEOF(szRHS);
        // remember, the registry has LH and RH reversed here
        if (RegEnumValue(hkIniStuff, dw, szLHS, &cbData1, NULL, NULL, (LPBYTE)szRHS, &cbData2) != ERROR_SUCCESS)
            break;

        
        // remember, the registry has LH and RH reversed here
        cbLHS = lstrlen(szLHS) * SIZEOF(TCHAR);
        RegSetValueEx(hk, szRHS, NULL, REG_SZ, (LPBYTE)szLHS, cbLHS);

        //
        // dont write the font to WIN.INI![Fonts], delete it.
        //
        WriteProfileString(sc_szFonts, szRHS, NULL);
    }
    RegCloseKey(hkIniStuff);
    RegCloseKey(hk);

    /*
    ** mark the new current font size
    */
    cbLHS = lstrlen(lpszFontSize) * SIZEOF(TCHAR);
    RegSetValueEx(hkSettings, REGSTR_VAL_DPILOGICALX, NULL, REG_SZ, (LPBYTE)lpszFontSize, cbLHS);
    RegSetValueEx(hkSettings, REGSTR_VAL_DPILOGICALY, NULL, REG_SZ, (LPBYTE)lpszFontSize, cbLHS);
    RegSetValueEx(hkSettings, REGSTR_VAL_DPIPHYSICALX, NULL, REG_SZ, (LPBYTE)lpszFontSize, cbLHS);
    RegSetValueEx(hkSettings, REGSTR_VAL_DPIPHYSICALY, NULL, REG_SZ, (LPBYTE)lpszFontSize, cbLHS);

    // write silly DPI settings in system.ini
    wsprintf(szBuf, TEXT("100,%s,%s"), (LPTSTR)lpszFontSize, (LPTSTR)lpszFontSize);
    WritePrivateProfileString(sc_szBootDesc, sc_szAspect, szBuf, sc_szSystemIni);

    bRet = TRUE;

    hdc = GetDC(NULL);
    if (MyStrToLong(lpszFontSize) != GetDeviceCaps(hdc, LOGPIXELSX))
        bRet++;     // need a restart
    ReleaseDC(NULL, hdc);

// BUGBUG error recovery?  can it be done here?  is it needed?

error:
    if (hkFontSize) RegCloseKey(hkFontSize);
    if (hkSettings) RegCloseKey(hkSettings);
    return bRet;
}

#endif

BOOL CGeneralDlg::ChangeFontSize()
{
    int cFontSize = 0;
    //
    // Change font size if necessary
    //

    int i = ComboBox_GetCurSel(_hwndFontList);

    if (i != CB_ERR ) {

        TCHAR awcDesc[10];

        cFontSize = ComboBox_GetItemData(_hwndFontList, i);

        if ( (cFontSize != CB_ERR) &&
             (cFontSize != 0) &&
             (cFontSize != _cLogPix)) {

            //
            // The user has changed the fonts.
            // Lets make sure they want this.
            //

            if (FmtMessageBox(_hDlg,
                              MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_NEW_FONT)
                == IDNO) {

                return FALSE;

            }

            //
            // Call setup to change the font size.
            //

            wsprintf(awcDesc, TEXT("%d"), cFontSize);
            
#ifdef WINNT // DLI: Memphis does not have syssetup.dll 
            if (SetupChangeFontSize(_hDlg, awcDesc) == NO_ERROR)
#else
            if (DeskCPLSetFontSize(awcDesc))
#endif
            {
                //
                // A font size change will require a system reboot.
                //

                PropSheet_RestartWindows(GetParent(_hDlg));
            }
            else
            {
                //
                // Setup failed.
                //

                FmtMessageBox(_hDlg,
                              MB_ICONSTOP | MB_OK,
                              FALSE,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_ADMIN_INSTALL);

                return FALSE;
            }
        }
    }

    if (cFontSize == 0)
    {
        //
        // If we could not read the inf, then ignore the font selection
        // and don't force the reboot on account of that.
        //

        cFontSize = _cLogPix;
    }

    return TRUE;
}

void CGeneralDlg::InitGeneralDlg(HWND hDlg)
{
    _iDynaOrg = GetDynaCDSPreference();
    _hDlg = hDlg;
    _hwndFontList = GetDlgItem(_hDlg, ID_DSP_FONTSIZE);
    _InitFontList();
    _InitQuickResCheckbox();
    if (_iDynaOrg & DCDSF_ASK)
        CheckDlgButton(hDlg, IDC_SHUTUP, TRUE);
    else if (_iDynaOrg & DCDSF_DYNA)
        CheckDlgButton(hDlg, IDC_YESDYNA, TRUE);
    else
        CheckDlgButton(hDlg, IDC_NODYNA, TRUE);
}

// Init Font sizes 
//
// For NT:
// Read the supported fonts out of the inf file(s)
// Select was the user currently has.
//
// For WIN95:

BOOL CGeneralDlg::_InitFontList() {

    int i;
    ASSERT(_hwndFontList);
    ULONG uCurSel = (ULONG) -1;
    int cPix = 0;
    
#ifdef WINNT
    HINF InfFileHandle;
    INFCONTEXT infoContext;

    //
    // Get all font entries from both inf files
    //

    InfFileHandle = SetupOpenInfFile(TEXT("font.inf"),
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);

    if (InfFileHandle != INVALID_HANDLE_VALUE)
    {
        if (SetupFindFirstLine(InfFileHandle,
                               TEXT("Font Sizes"),
                               NULL,
                               &infoContext))
        {
            while(TRUE)
            {
                TCHAR awcDesc[LINE_LEN];

                if (SetupGetStringField(&infoContext,
                                        0,
                                        awcDesc,
                                        SIZEOF(awcDesc),
                                        NULL) &&
                    SetupGetIntField(&infoContext,
                                     1,
                                     &cPix))
                {
                    //
                    // Add it to the list box
                    //

                    i = ComboBox_AddString(_hwndFontList, awcDesc);
                    if (i != CB_ERR)
                    {
                        ComboBox_SetItemData(_hwndFontList, i, cPix);
                        if (_cLogPix == cPix)
                            uCurSel = i;
                    }
                }

                //
                // Try to get the next line.
                //

                if (!SetupFindNextLine(&infoContext,
                                       &infoContext))
                {
                    break;
                }
            }
        }

        SetupCloseInfFile(InfFileHandle);
    }

    //
    // Put in the custom fonts string and select it if
    // we couldn't find the listing we needed above
    //
    {
        LPTSTR lpszCustFonts;

        lpszCustFonts = FmtSprint(ID_DSP_CUSTOM_FONTS);

        _idCustomFonts = ComboBox_AddString(_hwndFontList, lpszCustFonts);
        LocalFree(lpszCustFonts);
    }

#else
    HKEY hkFonts, hkEnumFont;
    int index;
    TCHAR szBuf[100];
    TCHAR szBuf2[100];
    DWORD cbData;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sc_szRegFontSize, 0, KEY_READ, &hkFonts) != ERROR_SUCCESS)
    {
        LoadString(hInstance, IDS_UNKNOWN, szBuf, SIZEOF(szBuf));
        ComboBox_AddString(_hwndFontList, szBuf);
        ComboBox_SetCurSel(_hwndFontList, 0);
        return FALSE;
    }

    for (i=0; ; i++)
    {
        DWORD cchBuf = ARRAYSIZE(szBuf);
        // enumerate the fonts
        if (RegEnumKeyEx(hkFonts,i,szBuf,&cchBuf, NULL,NULL, NULL, NULL)
            != ERROR_SUCCESS)
            break;  // Bail if no more keys.

        if (RegOpenKeyEx(hkFonts, szBuf, 0, KEY_READ, &hkEnumFont) != ERROR_SUCCESS)
            break;

        cbData = SIZEOF(szBuf2);

        if (RegQueryValueEx(hkEnumFont, REGSTR_VAL_DESCRIPTION, NULL, NULL,
                            (LPBYTE)szBuf2, &cbData) == ERROR_SUCCESS)
        {
            index = ComboBox_AddString(_hwndFontList, szBuf2);
            if (index != CB_ERR)
            {
                cPix = (int)MyStrToLong(szBuf);
                ComboBox_SetItemData(_hwndFontList, index, cPix);
                if (_cLogPix == cPix)
                    uCurSel = i;
            }
        }

        RegCloseKey(hkEnumFont);
    }
    RegCloseKey(hkFonts);


    LoadString(hInstance, IDS_OTHERFONT, szBuf2, SIZEOF(szBuf2));

    _idCustomFonts = ComboBox_AddString(_hwndFontList, szBuf2);
#endif
    if (_idCustomFonts != CB_ERR)
        ComboBox_SetItemData(_hwndFontList, _idCustomFonts, _cLogPix);
    
    if (uCurSel == (ULONG) -1) {
        uCurSel = _idCustomFonts;
    }


    if (_fForceSmallFont && (_cLogPix == 96))
        this->ForceSmallFont();
    else
    {
        //
        // Select the right entry.
        //
        ComboBox_SetCurSel(_hwndFontList, uCurSel);
        this->SetFontSizeText( _cLogPix );
    }
    
    return TRUE;
}


void CGeneralDlg::SetFontSizeText( int cdpi )
{
    LPTSTR pszFontSize;
    HWND hwndCustFontPer;
    if (cdpi == CDPI_NORMAL)
        pszFontSize = FmtSprint( ID_DSP_NORMAL_FONTSIZE_TEXT, cdpi );
    else
        pszFontSize = FmtSprint( ID_DSP_CUSTOM_FONTSIZE_TEXT, (100 * cdpi) / CDPI_NORMAL, cdpi );

    hwndCustFontPer = GetDlgItem(_hDlg, ID_DSP_CUSTFONTPER);
    SendMessage(hwndCustFontPer, WM_SETTEXT, 0, (LPARAM)pszFontSize );

    LocalFree(pszFontSize);
}


//
// ForceSmallFont method
//
//
void CGeneralDlg::ForceSmallFont() {
    int i, iSmall, dpiSmall, dpi;
    //
    // Set small font size in the listbox.
    //
    iSmall = CB_ERR;
    dpiSmall = 9999;
    for (i=0; i <=1; i++)
    {
        dpi = ComboBox_GetItemData(_hwndFontList, i);
        if (dpi == CB_ERR)
            continue;

        if (dpi < dpiSmall || iSmall < CB_ERR)
        {
            iSmall = i;
            dpiSmall = dpi;
        }
    }

    if (iSmall != -1)
        ComboBox_SetCurSel(_hwndFontList, iSmall);
    this->SetFontSizeText(dpiSmall);
    EnableWindow(_hwndFontList, FALSE);
}

BOOL CGeneralDlg::HandleGeneralApply(HWND hDlg)
{
    int iDynaNew;

    if (IsDlgButtonChecked(hDlg, IDC_YESDYNA))
        iDynaNew= DCDSF_YES;
    else if (IsDlgButtonChecked(hDlg, IDC_NODYNA))
        iDynaNew= DCDSF_NO;
    else
        iDynaNew = DCDSF_PROBABLY;

    if (iDynaNew != _iDynaOrg)
    {
        SetDisplayCPLPreference(REGSTR_VAL_DYNACOLORCHANGE, iDynaNew);
        _iDynaOrg = iDynaNew;
    }

    _ApplyQuickResCheckbox();
    this->ChangeFontSize();
    
    return TRUE;
}

void CGeneralDlg::HandleFontSelChange()
{
    //
    // Can not change a font during setup.
    // run the control panel later on.
    //

    if (gbExecMode == EXEC_SETUP) {

        ULONG i;

        FmtMessageBox(_hDlg,
                      MB_ICONINFORMATION,
                      FALSE,
                      ID_DSP_TXT_CHANGE_FONT,
                      ID_DSP_TXT_FONT_IN_SETUP_MODE);

        //
        // Reset the value in the listbox.
        // We only have two entries for now !
        //

        for (i=0; i <=1; i++)
        {
            if (ComboBox_GetItemData(_hwndFontList, i) == _cLogPix)
                ComboBox_SetCurSel(_hwndFontList, i);
        }

        this->SetFontSizeText(_cLogPix);

    } else {

        //
        // Warn the USER font changes will not be seen until after
        // reboot
        //
        int iCurSel;
        int cdpi;

        iCurSel = ComboBox_GetCurSel(_hwndFontList);
        cdpi = ComboBox_GetItemData(_hwndFontList, iCurSel);
        if (iCurSel == _idCustomFonts) {

            InitDragSizeClass();
            cdpi = DialogBoxParam(hInstance, MAKEINTRESOURCE(DLG_CUSTOMFONT),
                                  _hDlg, CustomFontDlgProc, cdpi );
            if (cdpi != 0) 
                ComboBox_SetItemData(_hwndFontList, _idCustomFonts, cdpi);
            else 
                cdpi = ComboBox_GetItemData(_hwndFontList, _idCustomFonts);
        }
            
        if (cdpi != _cLogPix)
        {
            FmtMessageBox(_hDlg,
                          MB_ICONINFORMATION,
                          FALSE,
                          ID_DSP_TXT_CHANGE_FONT,
                          ID_DSP_TXT_FONT_LATER);
            this->SetFontSizeText(cdpi);
            PropSheet_Changed(GetParent(_hDlg), _hDlg);
        }
    }

}

void StartStopQuickRes(HWND hDlg, BOOL fEnable)
{
    HWND hwnd;

TryAgain:
    hwnd = FindWindow(sc_szQuickResClass, NULL);

    if (fEnable)
    {
        if (!hwnd)
            WinExec(c_szQuickResCommandLine, SW_SHOWNORMAL);

        return;
    }

    if (hwnd)
    {
        SendMessage(hwnd, WM_CLOSE, 0, 0L);
        goto TryAgain;
    }
}

void CGeneralDlg::_InstallQuickRes(BOOL fEnable)
{
    HKEY hk = NULL;

    if (fEnable)
    {
        if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_RUN, 0, TEXT(""), 0, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS)
        {
            RegSetValueEx(hk, sc_szQuickResRegName, NULL, REG_SZ,
                (LPBYTE)sc_szQuickResCommandLine,
                lstrlen(sc_szQuickResCommandLine) + 1);
        }
    }
    else if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_RUN, 0, KEY_WRITE, &hk) == ERROR_SUCCESS)
        RegDeleteValue(hk, sc_szQuickResRegName);

    if (hk)
        RegCloseKey(hk);
}

void CGeneralDlg::_ApplyQuickResCheckbox()
{
    BOOL fEnable = (IsDlgButtonChecked(_hDlg, IDC_SHOWQUICKRES) == BST_CHECKED);
    _InstallQuickRes(fEnable);
    StartStopQuickRes(_hDlg, fEnable);
}

void CGeneralDlg::_InitQuickResCheckbox()
{
    BOOL fResult = FALSE;
    HKEY hk;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_RUN, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        TCHAR szBuf[4];
        DWORD dwLen = 0;

        fResult = (RegQueryValueEx(hk, sc_szQuickResRegName, NULL, NULL,
            (LPBYTE)szBuf, &dwLen) == ERROR_MORE_DATA);

        RegCloseKey(hk);
    }

    CheckDlgButton(_hDlg, IDC_SHOWQUICKRES, fResult? BST_CHECKED : BST_UNCHECKED);
}

LRESULT CALLBACK CGeneralDlg::WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnm;

    switch (message)
    {
    case WM_INITDIALOG:
        InitGeneralDlg(hDlg);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR *)lParam;
        switch (lpnm->code)
        {
            case PSN_APPLY:
                return HandleGeneralApply(hDlg);
            default:
                return FALSE;
        }
        break;
        
    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_NODYNA:
            case IDC_YESDYNA:
            case IDC_SHUTUP:
                if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                break;
            case ID_DSP_FONTSIZE:
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                    case CBN_SELCHANGE:
                        HandleFontSelChange();
                        break;
                    default:
                        break;
                }
                break;

            case IDC_SHOWQUICKRES:
                PropSheet_Changed(GetParent(hDlg), hDlg);
                break;
                
            default:
                break;
        }
        break;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("mds.hlp"), HELP_WM_HELP,
            (DWORD)(LPTSTR)sc_GeneralHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, TEXT("mds.hlp"), HELP_CONTEXTMENU,
            (DWORD)(LPTSTR)sc_GeneralHelpIds);
        break;
  
    default:
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// Callback functions PropertySheet can use
//
BOOL CALLBACK
GeneralPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CGeneralDlg * pcgd = (CGeneralDlg * )GetWindowLong(hDlg, DWL_USER);
    switch (message)
    {
        case WM_INITDIALOG:
            ASSERT(!pcgd);
            ASSERT(lParam);

            pcgd = new CGeneralDlg((BOOL)((LPPROPSHEETPAGE)lParam)->lParam);
            if(pcgd)
            {
                // now we need to init
                pcgd->InitGeneralDlg(hDlg);
                SetWindowLong(hDlg, DWL_USER, (LPARAM)pcgd);
                return TRUE;
            }

            break;
            
        case WM_DESTROY:
            if (pcgd)
            {
                SetWindowLong(hDlg, DWL_USER, NULL);
                delete pcgd;
            }
            break;

        default:
            if (pcgd)
                return pcgd->WndProc(hDlg, message, wParam, lParam);
            break;
    }

    return FALSE;
}

/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/
BOOL CALLBACK AskDynaCDSProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    int *pTemp;

    switch (msg)
    {
    case WM_INITDIALOG:
        if ((pTemp = (int *)lp) != NULL)
        {
            SetWindowLong(hDlg, DWL_USER, (LONG)pTemp);
            CheckDlgButton(hDlg, (*pTemp & DCDSF_DYNA)?
                IDC_YESDYNA : IDC_NODYNA, BST_CHECKED);
        }
        else
            EndDialog(hDlg, -1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp, lp))
        {
        case IDOK:
            if ((pTemp = (int *)GetWindowLong(hDlg, DWL_USER)) != NULL)
            {
                *pTemp = IsDlgButtonChecked(hDlg, IDC_YESDYNA)? DCDSF_DYNA : 0;

                if (!IsDlgButtonChecked(hDlg, IDC_SHUTUP))
                    *pTemp |= DCDSF_ASK;

                SetDisplayCPLPreference(REGSTR_VAL_DYNACOLORCHANGE, *pTemp);
            }
            //
            // fall through
            //
        case IDCANCEL:
            EndDialog(hDlg, (GET_WM_COMMAND_ID(wp, lp) == IDOK));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

int  AskDynaCDS(HWND hOwner)
{
    int val = GetDynaCDSPreference();

    if (val & DCDSF_ASK)
    {
        switch (DialogBoxParam(hInstance, MAKEINTRESOURCE(DLG_ASKDYNACDS),
            hOwner, AskDynaCDSProc, (LPARAM)(int *)&val))
        {
        case 0:         // user cancelled
            return -1;

        case -1:        // dialog could not be displayed
            val = DCDSF_NO;
            break;
        }
    }

    return ((val & DCDSF_DYNA) == DCDSF_DYNA);
}

/*--------------------------------------------------------------------------*
 *--------------------------------------------------------------------------*/
BOOL CALLBACK WarnCompatProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp, lp))
        {
        case IDOK:

            if (IsDlgButtonChecked(hDlg, IDC_SHUTUP))
                SetDisplayCPLPreference(REGSTR_VAL_COMPATWARNING, COMPATWARN_DONT);
            //
            // fall through
            //
        case IDCANCEL:
            EndDialog(hDlg, (GET_WM_COMMAND_ID(wp, lp) == IDOK));
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL WarnUserAboutCompatibility(HWND hOwner)
{
    if (GetDisplayCPLPreference(REGSTR_VAL_COMPATWARNING) == -1)
    {
        return (DialogBox(hInstance, MAKEINTRESOURCE(DLG_COMPATWARN), hOwner, WarnCompatProc) == IDOK);
    }
    return TRUE;
}
