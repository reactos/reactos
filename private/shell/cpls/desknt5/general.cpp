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


#ifndef WINNT
// codes for SelectFontSize
#define DPI_INVALID   (-1)
#define DPI_FROMCOMBO (-2)
#define DPI_CUSTOM    (-3)
#endif

extern "C" {
extern INT_PTR CALLBACK CustomFontDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
extern int  g_iNewDPI;     //defined in lookdlg.c; 
}

//BUGBUG: (dli) This should be put in regstr.h
static const TCHAR sc_szRegFontSize[]           = REGSTR_PATH_SETUP TEXT("\\") REGSTR_VAL_FONTSIZE;
static const TCHAR sc_szQuickResRegName[]       = TEXT("Taskbar Display Controls");
static const TCHAR sc_szQuickResCommandLine[]  = TEXT("RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY");
static const TCHAR sc_szQuickResClass[]  = TEXT("SysQuickRes");
static const char c_szQuickResCommandLine[]  = "RunDLL deskcp16.dll,QUICKRES_RUNDLLENTRY";
#ifndef WINNT
static const TCHAR sc_szFonts[] = TEXT("fonts");
static const TCHAR sc_szBootDesc[] = TEXT("boot.description");
static const TCHAR sc_szAspect[] = TEXT("aspect");
#endif

//-----------------------------------------------------------------------------
static const DWORD sc_GeneralHelpIds[] =
{
    // font size
    IDC_FONTSIZEGRP,   NO_HELP, // IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE, 
    IDC_FONT_SIZE_STR, IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE, 
    IDC_FONT_SIZE,     IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE,    
    IDC_CUSTFONTPER,   IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE, 

    // group box
    IDC_DYNA,          NO_HELP, // IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DYNA,     

    // radio buttons
    IDC_DYNA_TEXT,     NO_HELP,
    IDC_NODYNA,        IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_RESTART, 
    IDC_YESDYNA,       IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DONT_RESTART,
    IDC_SHUTUP,        IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_ASK_ME, 

    0, 0
};

BOOL IsUserAdmin(VOID);


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
        void _InitDynaCDSPreference();
#ifndef WINNT
        void _InstallQuickRes(BOOL fEnable);
        void _InitQuickResCheckbox();
        void _ApplyQuickResCheckbox();
#endif
    public:
        CGeneralDlg(BOOL fFoceSmallFont);
        void InitGeneralDlg(HWND hDlg);
        void SetFontSizeText( int cdpi );
        BOOL ChangeFontSize();
        void HandleGeneralApply(HWND hDlg);
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
        WritePrivateProfileString(g_szBoot, szRHS, szLHS, g_szSystemIni);
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
    WritePrivateProfileString(sc_szBootDesc, sc_szAspect, szBuf, g_szSystemIni);

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

//
//  The purpose of this function is to check if the OriginalDPI value was already saved in the 
// per-machine part of the registry. If this is NOT present, then get the system DPI and save it 
// there. We want to do it just once for a machine. When an end-user logsin, we need to detect if
// we need to change the sizes of UI fonts based on DPI change. We use the "AppliedDPI", which is 
// stored on the per-user branch of the registry to determine the dpi change. If this "AppliedDPI"
// is missing, then this OriginalDPI value will be used. If this value is also missing, that 
// implies that nobody has changed the system DPI value.
//
void SaveOriginalDPI()
{
    //See if the "OriginalDPI value is present under HKEY_LOCAL_MACHINE.
    HKEY hk;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     SZ_CONTROLPANEL,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     &hk) == ERROR_SUCCESS)
    {
        int   iOriginalDPI;
        DWORD dwDataSize = sizeof(iOriginalDPI);
        
        if (RegQueryValueEx(hk, SZ_ORIGINALDPI, NULL, NULL,
                            (LPBYTE)&iOriginalDPI, &dwDataSize) != ERROR_SUCCESS)
        {
            //"OriginalDPI" value is not present in the registry. Now, get the DPI
            // and save it as the "OriginalDPI"
            HDC hdc = GetDC(NULL);
            iOriginalDPI = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
            
            DWORD   dwSize = sizeof(iOriginalDPI);

            //Save the current DPI as the "OriginalDPI" value.
            RegSetValueEx(hk,
                            SZ_ORIGINALDPI,
                            NULL,
                            REG_DWORD,
                            (LPBYTE) &iOriginalDPI,
                            dwSize);
        }
        
        RegCloseKey(hk);
    }
}

BOOL CGeneralDlg::ChangeFontSize()
{
    int cFontSize = 0;
    int cForcedFontSize;
    int cUIfontSize;

    //
    // Change font size if necessary
    //

    int i = ComboBox_GetCurSel(_hwndFontList);

    if (i != CB_ERR ) {

        TCHAR awcDesc[10];

        cFontSize = (int)ComboBox_GetItemData(_hwndFontList, i);

        if ( (cFontSize != CB_ERR) &&
             (cFontSize != 0) &&
             (cFontSize != _cLogPix)) {

            //
            // The user has changed the fonts.
            // Lets make sure they want this.
            //

            if (FmtMessageBox(_hDlg,
                              MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION,
                              ID_DSP_TXT_CHANGE_FONT,
                              ID_DSP_TXT_NEW_FONT)
                == IDNO) {

                return FALSE;

            }

            //Save the original DPI before the DPI ever changed.
            SaveOriginalDPI();
            
            cUIfontSize = cForcedFontSize = cFontSize;
            if (_idCustomFonts == i) {
                BOOL predefined = FALSE;
                int count = ComboBox_GetCount(_hwndFontList);
                int max = -1, min = 0xffff, dpi;
                for (i = 0; i < count; i++) {
                    if (i == _idCustomFonts) 
                        continue;

                    dpi  = (int)ComboBox_GetItemData(_hwndFontList, i);

                    if (dpi == cFontSize)  {
                        predefined = TRUE;
                        break;
                    }

                    if (dpi < min) min = dpi;
                    if (dpi > max) max = dpi;
                }

                if (!predefined) {
                    if ((cFontSize > max) || (cFontSize + (max-min)/2 > max))
                        cForcedFontSize = max;
                    else 
                        cForcedFontSize = min;

                    // Currently our Custom font picker allows end-users to pick upto 500% of 
                    // normal font size; But, when we enlarge the UI fonts to this size, the 
                    // system becomes un-usable after reboot. So, what we do here is to allow
                    // ui font sizes to grow up to 200% and then all further increases are 
                    // proportionally reduced such that the maximum possible ui font size is only
                    // 250%. (i.e the range 200% to 500% is mapped on to a range of 200% to 250%)
                    // In other words, we allow upto 192 dpi (200%) with no modification. For any
                    // DPI greater than 192, we proportionately reduce it such that the highest DPI
                    // is only 240!
                    if(cUIfontSize > 192)
                        cUIfontSize = 192 + ((cUIfontSize - 192)/6);
                }
            }

            //
            // Call setup to change the font size.
            //

            wsprintf(awcDesc, TEXT("%d"), cForcedFontSize);
            
#ifdef WINNT // DLI: Memphis does not have syssetup.dll 
            if (SetupChangeFontSize(_hDlg, awcDesc) == NO_ERROR) 
#else
            if (DeskCPLSetFontSize(awcDesc))
#endif
            {
                //
                // Font change has succeeded; Now there is a new DPI to be applied to UI fonts!
                // 
                g_iNewDPI = cUIfontSize;
                
                //
                // A font size change will require a system reboot.
                //

                PropSheet_RestartWindows(GetParent(_hDlg));
#ifdef WINNT
                if (cForcedFontSize != cFontSize) {
                    HKEY hkFont;
                    DWORD dwSize;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     SZ_FONTDPI_PROF,
                                     0,
                                     KEY_WRITE,
                                     &hkFont) == ERROR_SUCCESS) {
                        dwSize = sizeof(DWORD);
                        RegSetValueEx(hkFont,
                                      SZ_LOGPIXELS,
                                      NULL,
                                      REG_DWORD,
                                      (LPBYTE) &cFontSize,
                                      dwSize);
                        RegCloseKey(hkFont);
                    }
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     SZ_FONTDPI,
                                     0,
                                     KEY_WRITE,
                                     &hkFont) == ERROR_SUCCESS) {
                        dwSize = sizeof(DWORD);
                        RegSetValueEx(hkFont,
                                      SZ_LOGPIXELS,
                                      NULL,
                                      REG_DWORD,
                                      (LPBYTE) &cFontSize,
                                      dwSize);
                        RegCloseKey(hkFont);
                    }
                }
#endif
            }
            else
            {
                //
                // Setup failed.
                //

                FmtMessageBox(_hDlg,
                              MB_ICONSTOP | MB_OK,
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
    _hDlg = hDlg;
    _hwndFontList = GetDlgItem(_hDlg, IDC_FONT_SIZE);
    _InitFontList();
#ifndef WINNT
    _InitQuickResCheckbox();
#endif
    _iDynaOrg = -1;
}

void CGeneralDlg::_InitDynaCDSPreference()
{
    int iDyna = GetDynaCDSPreference();
    if(iDyna != _iDynaOrg)
    {
        _iDynaOrg = iDyna;

        CheckDlgButton(_hDlg, IDC_SHUTUP, FALSE);
        CheckDlgButton(_hDlg, IDC_YESDYNA, FALSE);
        CheckDlgButton(_hDlg, IDC_NODYNA, FALSE);

        if (_iDynaOrg & DCDSF_ASK)
            CheckDlgButton(_hDlg, IDC_SHUTUP, TRUE);
        else if (_iDynaOrg & DCDSF_DYNA)
            CheckDlgButton(_hDlg, IDC_YESDYNA, TRUE);
        else
            CheckDlgButton(_hDlg, IDC_NODYNA, TRUE);
    }
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
    TCHAR szBuf2[100];

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


#else
    HKEY hkFonts, hkEnumFont;
    int index;
    TCHAR szBuf[100];
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

#endif
    //
    // Put in the custom fonts string
    //

    LoadString(hInstance, ID_DSP_CUSTOM_FONTS, szBuf2, SIZEOF(szBuf2));
    _idCustomFonts = ComboBox_AddString(_hwndFontList, szBuf2);

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

    
    //
    // !!! We currently change the font size by calling SetupChangeFontSize
    // !!! This function request the user to be an administrator.
    //
        
    EnableWindow(_hwndFontList, IsUserAdmin());
    
    return TRUE;
}


void CGeneralDlg::SetFontSizeText( int cdpi )
{
    HWND hwndCustFontPer;
    TCHAR achStr[80];
    TCHAR achFnt[120];

    if (cdpi == CDPI_NORMAL)
    {
        LoadString(hInstance, ID_DSP_NORMAL_FONTSIZE_TEXT, achStr, sizeof(achStr));
        wsprintf(achFnt, achStr, cdpi);
    }
    else
    {
        LoadString(hInstance, ID_DSP_CUSTOM_FONTSIZE_TEXT, achStr, sizeof(achStr));
        wsprintf(achFnt, achStr, (100 * cdpi + 50) / CDPI_NORMAL, cdpi );
    }

    hwndCustFontPer = GetDlgItem(_hDlg, IDC_CUSTFONTPER);
    SendMessage(hwndCustFontPer, WM_SETTEXT, 0, (LPARAM)achFnt);
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
        dpi = (int)ComboBox_GetItemData(_hwndFontList, i);
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

void CGeneralDlg::HandleGeneralApply(HWND hDlg)
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
        SetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE, iDynaNew);
        _iDynaOrg = iDynaNew;
    }

#ifndef WINNT
    _ApplyQuickResCheckbox();
#endif

    BOOL bSuccess = ChangeFontSize();
    
    long lRet = (bSuccess ? PSNRET_NOERROR: PSNRET_INVALID_NOCHANGEPAGE);
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRet);
}

void CGeneralDlg::HandleFontSelChange()
{
    //
    // Warn the USER font changes will not be seen until after
    // reboot
    //
    int iCurSel;
    int cdpi;

    iCurSel = ComboBox_GetCurSel(_hwndFontList);
    cdpi = (int)ComboBox_GetItemData(_hwndFontList, iCurSel);
    if (iCurSel == _idCustomFonts) {

        InitDragSizeClass();
        cdpi = (int)DialogBoxParam(hInstance, MAKEINTRESOURCE(DLG_CUSTOMFONT),
                              _hDlg, CustomFontDlgProc, cdpi );
        if (cdpi != 0)
            ComboBox_SetItemData(_hwndFontList, _idCustomFonts, cdpi);
        else
            cdpi = (int)ComboBox_GetItemData(_hwndFontList, _idCustomFonts);
    }

    if (cdpi != _cLogPix)
    {
        FmtMessageBox(_hDlg,
                      MB_ICONINFORMATION,
                      ID_DSP_TXT_CHANGE_FONT,
                      ID_DSP_TXT_FONT_LATER);
        PropSheet_Changed(GetParent(_hDlg), _hDlg);
    }

    this->SetFontSizeText(cdpi);
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

#ifndef WINNT

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

#endif


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
            case PSN_SETACTIVE:
                _InitDynaCDSPreference();
                return TRUE;
            case PSN_APPLY:
                HandleGeneralApply(hDlg);
                return TRUE;
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
            case IDC_FONT_SIZE:
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
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("display.hlp"), HELP_WM_HELP,
            (DWORD_PTR)(LPTSTR)sc_GeneralHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, TEXT("display.hlp"), HELP_CONTEXTMENU,
            (DWORD_PTR)(LPTSTR)sc_GeneralHelpIds);
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
INT_PTR CALLBACK
GeneralPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CGeneralDlg * pcgd = (CGeneralDlg * )GetWindowLongPtr(hDlg, DWLP_USER);
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
                SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pcgd);
                return TRUE;
            }

            break;
            
        case WM_DESTROY:
            if (pcgd)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, NULL);
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
INT_PTR CALLBACK AskDynaCDSProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
    int *pTemp;

    switch (msg)
    {
    case WM_INITDIALOG:
        if ((pTemp = (int *)lp) != NULL)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pTemp);
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
            if ((pTemp = (int *)GetWindowLongPtr(hDlg, DWLP_USER)) != NULL)
            {
                *pTemp = IsDlgButtonChecked(hDlg, IDC_YESDYNA)? DCDSF_DYNA : 0;

                if (!IsDlgButtonChecked(hDlg, IDC_SHUTUP))
                    *pTemp |= DCDSF_ASK;

                SetDisplayCPLPreference(REGSTR_VAL_DYNASETTINGSCHANGE, *pTemp);
            }

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
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


BOOL IsUserAdmin(VOID)

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

--*/

{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL bRet;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token)) 
    {
        return(FALSE);
    }

    bRet = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token, TokenGroups, NULL, 0, &BytesRequired) && 
       (GetLastError() == ERROR_INSUFFICIENT_BUFFER) && 
       (Groups = (PTOKEN_GROUPS)malloc(BytesRequired)) && 
       GetTokenInformation(Token, TokenGroups, Groups, BytesRequired, &BytesRequired)) 
    {
        bRet = AllocateAndInitializeSid(&NtAuthority,
                                        2,
                                        SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS,
                                        0, 0, 0, 0, 0, 0,
                                        &AdministratorsGroup);

        if(bRet) 
        {
            //
            // See if the user has the administrator group.
            //
            bRet = FALSE;
            for (i = 0; i < Groups->GroupCount; i++) 
            {
                if(EqualSid(Groups->Groups[i].Sid, AdministratorsGroup)) 
                {
                    bRet = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //
    if(Groups) 
    {
        free(Groups);
    }

    CloseHandle(Token);

    return(bRet);
}

