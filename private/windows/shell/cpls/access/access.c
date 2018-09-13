// **************************************************************************
// Access.c
//
// Accessability Property sheet page creator
//
// **************************************************************************

#include "Access.h"

#ifdef  UNICODE     // Windows uses UNICODE
#define _UNICODE    // but tchar.h uses _UNICODE
#endif

DWORD g_dwOrigFKFlags;
BOOL g_bFKOn;

#include <stdlib.h>
#include <stddef.h>
#include <tchar.h>

#define OLDDISABLED     32760

#ifndef FKF_VALID
#define FKF_VALID           0x0000007F
#endif

#ifndef SKF_VALID
#define SKF_VALID           0x000001FF
#endif

#ifndef MKF_VALID
#define MKF_VALID           0x000000FF
#endif

#ifndef ATF_VALID
#define ATF_VALID           0x00000003
#endif

#ifndef SSF_VALID
#define SSF_VALID           0x00000007
#endif

#ifndef TKF_VALID
#define TKF_VALID           0x0000003F
#endif

//////////////////////////////////////////////////////////////////////////

// collection of data that represents the saved accessability state
typedef struct ACCSTATE   // as
{
    // Keyboard property page
    STICKYKEYS     sk;
    FILTERKEYS     fk;
    TOGGLEKEYS     tk;
    BOOL           fExtraKeyboardHelp;

    // Sound Property page
    SOUNDSENTRY    ss;
    BOOL           fShowSounds;

    // Display Property page
    HIGHCONTRAST   hc;
    TCHAR          szDefaultScheme[256];  // hc.lpszDefaultScheme

    // Mouse Property page
    MOUSEKEYS      mk;

    // General Property page
    BOOL               fShowWarnMsgOnFeatureActivate;
    BOOL               fPlaySndOnFeatureActivate;

    ACCESSTIMEOUT  ato;
    SERIALKEYS     serk;
    TCHAR          szActivePort[MAX_PATH];  // serk.szActivePort
    TCHAR          szPort[MAX_PATH];                // serk.szPort
} ACCSTATE, *PACCSTATE;


//////////////////////////////////////////////////////////////////////////
extern BOOL g_SPISetValue = FALSE;

static ACCSTATE s_asOrg;          // original settings from app start-up
static ACCSTATE s_asPrev;         // previous saved settings

extern BOOL  g_fWinNT = -1;       // TRUE if we're running on NT and must
                                  // disable some features

extern BOOL  g_fSaveSettings = TRUE;
extern BOOL  g_fShowWarnMsgOnFeatureActivate = TRUE;
extern BOOL  g_fPlaySndOnFeatureActivate = TRUE;
extern BOOL  g_fCopyToLogon = FALSE;
extern BOOL  g_fCopyToDefault = FALSE;
// Keyboard property page
// extern STICKYKEYS     g_sk = {0};
STICKYKEYS     g_sk;
FILTERKEYS     g_fk;
   // g_dwLastBounceKeySetting, g_nLastRepeatDelay, g_nLastRepeatRate
   //  and g_nLastWait are part of FilterKeys
   DWORD g_dwLastBounceKeySetting = 0;
   DWORD g_nLastRepeatDelay = 0;
   DWORD g_nLastRepeatRate = 0;
   DWORD g_nLastWait = 0;

TOGGLEKEYS     g_tk;
BOOL           g_fExtraKeyboardHelp = TRUE;

// Sound Property page
SOUNDSENTRY    g_ss;
BOOL           g_fShowSounds;

// Display Property page
HIGHCONTRAST   g_hc;

// Mouse Property page
MOUSEKEYS      g_mk;

// General Property page
ACCESSTIMEOUT  g_ato;
SERIALKEYS     g_serk;
TCHAR          g_szActivePort[256];
TCHAR          g_szPort[256];


//////////////////////////////////////////////////////////////////////////

void CopyHighContrast(LPHIGHCONTRAST phcDest, LPHIGHCONTRAST phcSrc)
{
    LPTSTR lpszDefaultScheme = phcDest->lpszDefaultScheme;

    memcpy(phcDest, phcSrc, sizeof(*phcDest));
    phcDest->lpszDefaultScheme = lpszDefaultScheme;

    if (NULL != phcDest->lpszDefaultScheme)
    {
        lstrcpy(phcDest->lpszDefaultScheme, phcSrc->lpszDefaultScheme);
    }
}

//////////////////////////////////////////////////////////////////////////

BOOL IsHighContrastEqual(LPHIGHCONTRAST phcDest, LPHIGHCONTRAST phcSrc)
{
    BOOL fIsEqual = FALSE;
    LPTSTR lpszDefaultScheme = phcDest->lpszDefaultScheme;

    // Temporarily make the pointers match
    phcDest->lpszDefaultScheme = phcSrc->lpszDefaultScheme;

    // match the bits of the structures and the pointed to data
    fIsEqual = (0 == memcmp(phcDest, phcSrc, sizeof(*phcDest)) &&
                0 == lstrcmp(lpszDefaultScheme, phcSrc->lpszDefaultScheme));

    phcDest->lpszDefaultScheme = lpszDefaultScheme;

    return(fIsEqual);
}


//////////////////////////////////////////////////////////////////////////

void CopySerialKeys(LPSERIALKEYS pskDest, LPSERIALKEYS pskSrc)
{
    LPTSTR lpszActivePort = pskDest->lpszActivePort;
    LPTSTR lpszPort = pskDest->lpszPort;

    memcpy(pskDest, pskSrc, sizeof(*pskDest));
    pskDest->lpszActivePort = lpszActivePort;

    if (NULL != pskDest->lpszActivePort)
    {
        lstrcpy(pskDest->lpszActivePort, pskSrc->lpszActivePort);
    }

    pskDest->lpszPort = lpszPort;
    if (NULL != pskDest->lpszPort)
    {
        lstrcpy(pskDest->lpszPort, pskSrc->lpszPort);
    }
}

//////////////////////////////////////////////////////////////////////////

BOOL IsSerialKeysEqual(LPSERIALKEYS pskDest, LPSERIALKEYS pskSrc)
{
    BOOL fIsEqual = FALSE;
    LPTSTR lpszActivePort = pskDest->lpszActivePort;
    LPTSTR lpszPort = pskDest->lpszPort;

    // Temporarily make the pointers match
    pskDest->lpszActivePort = pskSrc->lpszActivePort;
    pskDest->lpszPort = pskSrc->lpszPort;

    // match the bits of the structures and the pointed to data
    fIsEqual = (0 == memcmp(pskDest, pskSrc, sizeof(*pskDest)) &&
        (NULL == lpszActivePort ||
                0 == lstrcmp(lpszActivePort, pskSrc->lpszActivePort)) &&
        (NULL == lpszPort ||
                0 == lstrcmp(lpszPort, pskSrc->lpszPort)));

    pskDest->lpszActivePort = lpszActivePort;
    pskDest->lpszPort = lpszPort;

    return(fIsEqual);
}

//////////////////////////////////////////////////////////////////////////

BOOL IsAccStateEqual(PACCSTATE pasDest, PACCSTATE pasSrc)
{
    BOOL fIsEqual = FALSE;
    HIGHCONTRAST   hc = pasDest->hc;
    SERIALKEYS     serk = pasDest->serk;
    int nLen;

    // Clear out the unused sections of the string buffers
    nLen = lstrlen(pasDest->szDefaultScheme);
    memset(&pasDest->szDefaultScheme[nLen], 0,
        sizeof(pasDest->szDefaultScheme)-nLen*sizeof(*pasDest->szDefaultScheme));

    nLen = lstrlen(pasDest->szActivePort);
    memset(&pasDest->szActivePort[nLen], 0,
        sizeof(pasDest->szActivePort)-nLen*sizeof(*pasDest->szActivePort));

    nLen = lstrlen(pasDest->szPort);
    memset(&pasDest->szPort[nLen], 0,
            sizeof(pasDest->szPort)-nLen*sizeof(*pasDest->szPort));

    nLen = lstrlen(pasSrc->szDefaultScheme);
    memset(&pasSrc->szDefaultScheme[nLen], 0,
            sizeof(pasSrc->szDefaultScheme)-nLen*sizeof(*pasSrc->szDefaultScheme));

    nLen = lstrlen(pasSrc->szActivePort);
    memset(&pasSrc->szActivePort[nLen], 0,
            sizeof(pasSrc->szActivePort)-nLen*sizeof(*pasSrc->szActivePort));

    nLen = lstrlen(pasSrc->szActivePort);
    memset(&pasSrc->szPort[nLen], 0,
            sizeof(pasSrc->szPort)-nLen*sizeof(*pasSrc->szPort));

    // Temporarily make the elements with pointers match
    pasDest->hc = pasSrc->hc;
    pasDest->serk = pasSrc->serk;

    // match the bits of the structures and the elements with pointers
    fIsEqual = (0 == memcmp(pasDest, pasSrc, sizeof(*pasDest)) &&
            IsHighContrastEqual(&hc, &pasSrc->hc) &&
            IsSerialKeysEqual(&serk, &pasSrc->serk));

    pasDest->hc = hc;
    pasDest->serk = serk;

    return(fIsEqual);
}


//////////////////////////////////////////////////////////////////////////


int WINAPI RegQueryInt (int nDefault, HKEY hkey, LPTSTR lpSubKey, LPTSTR lpValueName) {

   DWORD dwType;
   DWORD dwVal = nDefault;
   DWORD cbData = sizeof(int);
   if (ERROR_SUCCESS == RegOpenKeyEx(hkey, lpSubKey, 0, KEY_QUERY_VALUE, &hkey)) {
      RegQueryValueEx(hkey, lpValueName, NULL, &dwType, (PBYTE) &dwVal, &cbData);
      RegCloseKey(hkey);
   }
   return(dwVal);
}


//////////////////////////////////////////////////////////////////////////


BOOL WINAPI RegSetInt (HKEY hkey, LPTSTR lpSubKey, LPTSTR lpValueName, int nVal) {
   BOOL fOk = FALSE;
   DWORD dwDisposition;
   LONG lRet;

   if (ERROR_SUCCESS == RegCreateKeyEx(hkey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
      KEY_SET_VALUE, NULL, &hkey, &dwDisposition)) {

      lRet = RegSetValueEx(hkey, lpValueName, 0, REG_DWORD, (CONST BYTE *) &nVal, sizeof(nVal));
      fOk = (ERROR_SUCCESS == lRet);
      RegCloseKey(hkey);
   }
   return fOk;
}


//////////////////////////////////////////////////////////////////////////


void WINAPI RegQueryStr(
   LPTSTR lpDefault,
   HKEY hkey,
   LPTSTR lpSubKey,
   LPTSTR lpValueName,
   LPTSTR lpszValue,
   DWORD cbData) // note this is bytes, not characters.
{
   DWORD dwType;

   lstrcpy(lpszValue, lpDefault);
   if (ERROR_SUCCESS == RegOpenKeyEx(hkey, lpSubKey, 0, KEY_QUERY_VALUE, &hkey)) {
      RegQueryValueEx(hkey, lpValueName, NULL, &dwType, (PBYTE) lpszValue, &cbData);
      RegCloseKey(hkey);
   }
}

/***************************************************************************\
**AccessWriteProfileString
*
* History:
* 12-19-95 a-jimhar 	Created (was called AccessWriteProfileString)
* 02-08-95 a-jimhar     revised and moved from accrare.c to access.c
\***************************************************************************/
BOOL RegSetStr(
    HKEY hkey,
    LPCTSTR lpSection,
    LPCTSTR lpKeyName,
    LPCTSTR lpString)
{
    BOOL fRet = FALSE;
    LONG lErr;
    DWORD dwDisposition;

    lErr = RegCreateKeyEx(
            hkey,
            lpSection,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hkey,
            &dwDisposition);

    if (ERROR_SUCCESS == lErr)
    {
        if (NULL != lpString)
        {
            lErr = RegSetValueEx(
                    hkey,
                    lpKeyName,
                    0,
                    REG_SZ,
                    (CONST BYTE *)lpString,
                    (lstrlen(lpString) + 1) * sizeof(*lpString));
        }
        else
        {
            lErr = RegSetValueEx(
                    hkey,
                    lpKeyName,
                    0,
                    REG_SZ,
                    (CONST BYTE *)__TEXT(""),
                    1 * sizeof(*lpString));
        }

        if (ERROR_SUCCESS == lErr)
        {
            fRet = TRUE;
        }
        RegCloseKey(hkey);
    }
    return(fRet);
}


DWORD WINAPI RegQueryStrDW(
    DWORD dwDefault,
    HKEY hkey,
    LPTSTR lpSubKey,
    LPTSTR lpValueName)
{
    DWORD dwRet = dwDefault;
    TCHAR szTemp[40];
    TCHAR szDefault[40];

    const LPTSTR pwszd = __TEXT("%d");

    wsprintf(szDefault, pwszd, dwDefault);

    RegQueryStr(
        szDefault,
        hkey,
        lpSubKey,
        lpValueName,
        szTemp,
        sizeof(szTemp));

    dwRet = _ttol(szTemp);

    return dwRet;
}


BOOL RegSetStrDW(
    HKEY hkey,
    LPTSTR lpSection,
    LPCTSTR lpKeyName,
    DWORD dwValue)
{
    BOOL fRet;
    TCHAR szTemp[40];
    const LPTSTR pwszd = __TEXT("%d");

    wsprintf(szTemp, pwszd, dwValue);
    fRet = RegSetStr(hkey, lpSection, lpKeyName, szTemp);

    return fRet;
}


//////////////////////////////////////////////////////////////////////////


/*------------------------------------------------------------------
 * Function void KillAccStat()
 *
 * Purpose     Check if accstat is already running.  If it is we need
 *             to check to see if it should be.  It should only be running
 *             if each feature that is on also has the 'show status on
 *             screen flag checked.  If not we want to kill accstat.
 *
 * Params:     None
 *
 * Return:     TRUE if we had to kill accstat
 *             FALSE if accstat not running/valid session
 *------------------------------------------------------------------*/

void KillAccStat (void) {
   BOOL fCanTurnOff = FALSE;     // Can we turn off accstat due to invalid feature?
   BOOL fValidFeature = FALSE;   // Are there any valid features?

   // Accstat may be running.  Determine if it should be running
   // We need to check the FilterKeys, MouseKeys and StickyKeys
   if (g_sk.dwFlags & SKF_STICKYKEYSON)
      if (!(g_sk.dwFlags & SKF_INDICATOR))
         fCanTurnOff = TRUE;   // A mismatched flag - we MAY be able to turn off.
      else
         fValidFeature = TRUE; // A valid feature - we CAN't turn off accstat.

   if (g_fk.dwFlags & FKF_FILTERKEYSON)
      if (!(g_fk.dwFlags & FKF_INDICATOR))
         fCanTurnOff = TRUE;   // A mismatched flag - we MAY be able to turn off.
      else
         fValidFeature = TRUE; // A valid feature - we CAN't turn off accstat.

   if (g_mk.dwFlags & MKF_MOUSEKEYSON)
      if (!(g_mk.dwFlags & MKF_INDICATOR))
         fCanTurnOff = TRUE;   // A mismatched flag - we MAY be able to turn off.
      else
         fValidFeature = TRUE; // A valid feature - we CAN't turn off accstat.

   // Now we have two flags: fCanTurnOff is TRUE if there is a mismatched flag set
   // ie, feature on, indicator off.  ValidFeature is TRUE if any feature has
   // ON and INDICATOR set which implies accstat must remain active.
   if (!fValidFeature && fCanTurnOff) {
      TCHAR szBuf[256];
      HWND hwndAccStat;
      LoadString(g_hinst, IDS_ACCSTAT_WINDOW_TITLE, szBuf, ARRAY_SIZE(szBuf));
      if (IsWindow(hwndAccStat = FindWindow(NULL, szBuf))) {
         // Note sending 1 as the lParam tells accstat to shutup and
         // go away NOW.
         SendMessage(hwndAccStat, WM_SYSCOMMAND, SC_CLOSE, 1);
      }
   }
}


//////////////////////////////////////////////////////////////////////////


void WINAPI GetAccessibilitySettings (void) {
   BOOL fUpdate;

   if (g_fWinNT == -1) {
      OSVERSIONINFO osvi;
      osvi.dwOSVersionInfoSize = sizeof(osvi);
      GetVersionEx(&osvi);
      g_fWinNT = (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
   }

   g_fShowWarnMsgOnFeatureActivate = (BOOL) RegQueryInt(TRUE, HKEY_CURRENT_USER,
      GENERAL_KEY, WARNING_SOUNDS);

   s_asPrev.fShowWarnMsgOnFeatureActivate = g_fShowWarnMsgOnFeatureActivate;

   // Query the Sound On Activation entry
   g_fPlaySndOnFeatureActivate = (BOOL) RegQueryInt(TRUE, HKEY_CURRENT_USER,
      GENERAL_KEY, SOUND_ON_ACTIVATION);

   s_asPrev.fPlaySndOnFeatureActivate = g_fPlaySndOnFeatureActivate;

   g_fSaveSettings = TRUE;

   // Keyboard property page
   g_sk.cbSize = sizeof(g_sk);
   AccessSystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(g_sk), &g_sk, 0);
   s_asPrev.sk = g_sk;

   g_fk.cbSize = sizeof(g_fk);
   AccessSystemParametersInfo(SPI_GETFILTERKEYS, sizeof(g_fk), &g_fk, 0);
   g_fk.dwFlags |= FKF_AVAILABLE;

   // FILTERKEYS used to use OLDDISABLED as it's "unused" flag.  This doesn't
   // work very well on NT (SPI_SETFILTERKEYS calls fail).  We now use 0
   // for disabled values.  Take this opertunity to change any OLDDISABLED
   // values to 0 and save if needed.

   fUpdate = FALSE;

   if (OLDDISABLED == g_fk.iBounceMSec)
   {
      g_fk.iBounceMSec = 0;
      fUpdate = TRUE;
   }
   if (OLDDISABLED == g_fk.iDelayMSec)
   {
      g_fk.iDelayMSec = 0;
      fUpdate = TRUE;
   }
   if (OLDDISABLED == g_fk.iRepeatMSec)
   {
      g_fk.iRepeatMSec = 0;
      fUpdate = TRUE;
   }
   if (OLDDISABLED == g_fk.iWaitMSec)
   {
       g_fk.iWaitMSec = 0;
       fUpdate = TRUE;
   }

   if (fUpdate)
   {
        AccessSystemParametersInfo(
                SPI_SETFILTERKEYS, sizeof(g_fk), &g_fk, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
   }

   s_asPrev.fk = g_fk;
   // fix Filter keys bug
   g_dwOrigFKFlags = g_fk.dwFlags;
   g_bFKOn = g_fk.dwFlags & FKF_FILTERKEYSON;

   // g_dwLastBounceKeySetting, g_nLastRepeatDelay, g_nLastRepeatRate
   // and g_nLastWait are part of FilterKeys

   if (0 != g_fk.iBounceMSec) {
      // Bounce keys enabeled
      g_fk.iDelayMSec = 0;
      g_fk.iRepeatMSec = 0;
      g_fk.iWaitMSec = 0;

      g_dwLastBounceKeySetting = g_fk.iBounceMSec;
      g_nLastRepeatDelay = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_REPEAT_DELAY);
      g_nLastRepeatRate = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_REPEAT_RATE);
      g_nLastWait = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_WAIT);
   }
   else
   {
      if (0 == g_fk.iDelayMSec)
      {
          g_fk.iRepeatMSec = 0;
      }
      g_dwLastBounceKeySetting = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_BOUNCE_SETTING);
      g_nLastRepeatDelay = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_REPEAT_DELAY);
      g_nLastRepeatRate = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_REPEAT_RATE);
      if (0 != g_fk.iWaitMSec)
      {
         g_nLastWait = g_fk.iWaitMSec;
      }
      else
      {
         g_nLastWait = RegQueryInt(0, HKEY_CURRENT_USER, FILTER_KEY, LAST_WAIT);
      }
   }

   g_tk.cbSize = sizeof(g_tk);
   AccessSystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(g_tk), &g_tk, 0);
   s_asPrev.tk = g_tk;

   AccessSystemParametersInfo(SPI_GETKEYBOARDPREF, 0, &g_fExtraKeyboardHelp, 0);
   s_asPrev.fExtraKeyboardHelp = g_fExtraKeyboardHelp;

   // Sound Property page
   g_ss.cbSize = sizeof(g_ss);
   AccessSystemParametersInfo(SPI_GETSOUNDSENTRY, sizeof(g_ss), &g_ss, 0);
   s_asPrev.ss = g_ss;

   SystemParametersInfo(SPI_GETSHOWSOUNDS, 0, &g_fShowSounds, 0);

   // BUG, BUG GetSystemMetrics() is not updating value on reboot :a-anilk
   // g_fShowSounds = GetSystemMetrics(SM_SHOWSOUNDS);
   s_asPrev.fShowSounds = g_fShowSounds;

   // Display Property page
   g_hc.cbSize = sizeof(g_hc);
   AccessSystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(g_hc), &g_hc, 0);

   // Currently NT will not store these flags.  We fake them so we
   // can tell if they actually changed.

   s_asPrev.hc.lpszDefaultScheme = s_asPrev.szDefaultScheme;
   CopyHighContrast(&s_asPrev.hc, &g_hc);

   // Mouse Property page
   g_mk.cbSize = sizeof(g_mk);
   AccessSystemParametersInfo(SPI_GETMOUSEKEYS, sizeof(g_mk), &g_mk, 0);
   s_asPrev.mk = g_mk;

   // General Property page
   g_ato.cbSize = sizeof(g_ato);
   AccessSystemParametersInfo(SPI_GETACCESSTIMEOUT, sizeof(g_ato), &g_ato, 0);
   s_asPrev.ato = g_ato;

   g_serk.cbSize = sizeof(g_serk);
   g_serk.lpszActivePort = g_szActivePort;
   g_serk.lpszPort = g_szPort;
   AccessSystemParametersInfo(SPI_GETSERIALKEYS, sizeof(g_serk), &g_serk, 0);

   s_asPrev.serk.lpszActivePort = s_asPrev.szActivePort;
   s_asPrev.serk.lpszPort = s_asPrev.szPort;
   CopySerialKeys(&s_asPrev.serk, &g_serk);

   if (NULL == s_asOrg.hc.lpszDefaultScheme)
   {
      // s_asOrg has not yet been initialized
      s_asOrg = s_asPrev;
      s_asOrg.hc.lpszDefaultScheme = s_asOrg.szDefaultScheme;
      s_asOrg.serk.lpszActivePort = s_asOrg.szActivePort;
      s_asOrg.serk.lpszPort = s_asOrg.szPort;
   }
}


//////////////////////////////////////////////////////////////////////////

//a-anilk: Change, Admin options, Keyboard flags: 05/06/99
void WINAPI SetAccessibilitySettings (void) {
   HKEY hkey;
   DWORD dwDisposition;
   UINT fWinIni = SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE;
   BOOL fAnyNotifyChange = FALSE;

   g_SPISetValue = TRUE;

   SetCursor(LoadCursor(NULL, IDC_WAIT));

   if (g_fShowWarnMsgOnFeatureActivate) {
      g_hc.dwFlags |= HCF_CONFIRMHOTKEY;
      g_fk.dwFlags |= FKF_CONFIRMHOTKEY;
      g_sk.dwFlags |= SKF_CONFIRMHOTKEY;
      g_mk.dwFlags |= MKF_CONFIRMHOTKEY;
      g_tk.dwFlags |= TKF_CONFIRMHOTKEY;
   } else {
      g_hc.dwFlags &= ~HCF_CONFIRMHOTKEY;
      g_fk.dwFlags &= ~FKF_CONFIRMHOTKEY;
      g_sk.dwFlags &= ~SKF_CONFIRMHOTKEY;
      g_mk.dwFlags &= ~MKF_CONFIRMHOTKEY;
      g_tk.dwFlags &= ~TKF_CONFIRMHOTKEY;
   }

   if (g_fPlaySndOnFeatureActivate) {
      g_hc.dwFlags  |= HCF_HOTKEYSOUND;
      g_fk.dwFlags  |= FKF_HOTKEYSOUND;
      g_sk.dwFlags  |= SKF_HOTKEYSOUND;
      g_mk.dwFlags  |= MKF_HOTKEYSOUND;
      g_tk.dwFlags  |= TKF_HOTKEYSOUND;
      g_ato.dwFlags |= ATF_ONOFFFEEDBACK;
   } else {
      g_hc.dwFlags  &= ~HCF_HOTKEYSOUND;
      g_fk.dwFlags  &= ~FKF_HOTKEYSOUND;
      g_sk.dwFlags  &= ~SKF_HOTKEYSOUND;
      g_mk.dwFlags  &= ~MKF_HOTKEYSOUND;
      g_tk.dwFlags  &= ~TKF_HOTKEYSOUND;
      g_ato.dwFlags &= ~ATF_ONOFFFEEDBACK;
   }


   // Keyboard property page

   if (0 != memcmp(&g_sk, &s_asPrev.sk, sizeof(g_sk)))
   {
      if (g_fWinNT)
      {
         g_sk.dwFlags &= SKF_VALID;
      }
      AccessSystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(g_sk), &g_sk, fWinIni);
      s_asPrev.sk = g_sk;
      fAnyNotifyChange = TRUE;
   }

	if (g_bFKOn)
		g_fk.dwFlags |= FKF_FILTERKEYSON;
	else
		g_fk.dwFlags &= ~FKF_FILTERKEYSON;

	g_dwOrigFKFlags = g_fk.dwFlags;

   if (0 != memcmp(&g_fk, &s_asPrev.fk, sizeof(g_fk)))
   {
      if (g_fWinNT)
      {
         g_fk.dwFlags &= FKF_VALID;
      }

      // g_dwLastBounceKeySetting, g_nLastRepeatDelay, g_nLastRepeatRate
      // and g_nLastWait are part of FilterKeys

      if (0 != g_fk.iBounceMSec) {
         // Bounce keys enabeled
         g_fk.iDelayMSec = 0;
         g_fk.iRepeatMSec = 0;
         g_fk.iWaitMSec = 0;

         g_dwLastBounceKeySetting = g_fk.iBounceMSec;
      }
      else
      {
         g_nLastWait = g_fk.iWaitMSec;
         if (0 != g_fk.iDelayMSec)
         {
            // Slow key enabled
            g_nLastRepeatDelay = g_fk.iDelayMSec;
            g_nLastRepeatRate = g_fk.iRepeatMSec;
         }
         else
         {
            // neither Bounce or Slow
            g_fk.iRepeatMSec = 0;
         }
      }

      if (2000 < g_fk.iBounceMSec)
      {
         g_fk.iBounceMSec =  2000;
      }
      if (2000 < g_fk.iDelayMSec)
      {
         g_fk.iDelayMSec =  2000;
      }
      if (2000 < g_fk.iRepeatMSec)
      {
         g_fk.iRepeatMSec =  2000;
      }
      if (2000 < g_fk.iWaitMSec)
      {
         g_fk.iWaitMSec = 2000;
      }

      AccessSystemParametersInfo(SPI_SETFILTERKEYS, sizeof(g_fk), &g_fk, fWinIni);
      s_asPrev.fk = g_fk;

      fAnyNotifyChange = TRUE;
   }

   // always save these
   RegSetInt(HKEY_CURRENT_USER, FILTER_KEY, LAST_BOUNCE_SETTING, g_dwLastBounceKeySetting);
   RegSetInt(HKEY_CURRENT_USER, FILTER_KEY, LAST_REPEAT_DELAY, g_nLastRepeatDelay);
   RegSetInt(HKEY_CURRENT_USER, FILTER_KEY, LAST_REPEAT_RATE, g_nLastRepeatRate);
   RegSetInt(HKEY_CURRENT_USER, FILTER_KEY, LAST_WAIT, g_nLastWait);

   if (0 != memcmp(&g_tk, &s_asPrev.tk, sizeof(g_tk)))
   {
      if (g_fWinNT)
      {
         g_tk.dwFlags &= TKF_VALID;
      }
      AccessSystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(g_tk), &g_tk, fWinIni);
      s_asPrev.tk = g_tk;
      fAnyNotifyChange = TRUE;
   }

   if (g_fExtraKeyboardHelp != s_asPrev.fExtraKeyboardHelp)
   {
	   // Set this too. Some controls check this flag...0x100B
      AccessSystemParametersInfo(SPI_SETKEYBOARDCUES, 0, (PVOID)g_fExtraKeyboardHelp, fWinIni);

      AccessSystemParametersInfo(SPI_SETKEYBOARDPREF, g_fExtraKeyboardHelp, 0, fWinIni);
      s_asPrev.fExtraKeyboardHelp = g_fExtraKeyboardHelp;
      fAnyNotifyChange = TRUE;
   }

   // Display Property page

   // BUGBUG a-jimhar 03-22-96 verify changes to display property page save
   // code when display page is added back in on NT

   if (!IsHighContrastEqual(&g_hc, &s_asPrev.hc))
   {
      AccessSystemParametersInfo(SPI_SETHIGHCONTRAST, sizeof(g_hc), &g_hc, fWinIni);
      if (ERROR_SUCCESS == RegCreateKeyEx(
         HKEY_CURRENT_USER,
         HC_KEY,
         0,
         __TEXT(""),
         REG_OPTION_NON_VOLATILE,
         KEY_SET_VALUE,
         NULL,
         &hkey,
         &dwDisposition))
      {
         RegSetValueEx(hkey, HIGHCONTRAST_SCHEME, 0, REG_SZ, (PBYTE) g_hc.lpszDefaultScheme,
            (lstrlen(g_hc.lpszDefaultScheme) + 1) * sizeof(*g_hc.lpszDefaultScheme));
         RegSetValueEx(hkey, VOLATILE_SCHEME, 0, REG_SZ, (PBYTE) g_hc.lpszDefaultScheme,
            (lstrlen(g_hc.lpszDefaultScheme) + 1) * sizeof(*g_hc.lpszDefaultScheme));
         RegCloseKey(hkey);
         hkey = NULL;
      }
      CopyHighContrast(&s_asPrev.hc, &g_hc);
      fAnyNotifyChange = TRUE;
   }

   // Mouse Property page
   if (0 != memcmp(&g_mk, &s_asPrev.mk, sizeof(g_mk)))
   {
      if (g_fWinNT)
      {
         g_mk.dwFlags &= MKF_VALID;
      }
      AccessSystemParametersInfo(SPI_SETMOUSEKEYS, sizeof(g_mk), &g_mk, fWinIni);
      s_asPrev.mk = g_mk;
      fAnyNotifyChange = TRUE;
   }

   // General Property page
   if (g_fPlaySndOnFeatureActivate) {
      g_ato.dwFlags |= ATF_ONOFFFEEDBACK;
   } else {
      g_ato.dwFlags &= ~ATF_ONOFFFEEDBACK;
   }

   if (0 != memcmp(&g_ato, &s_asPrev.ato, sizeof(g_ato)))
   {
      if (g_fWinNT)
      {
         g_ato.dwFlags &= ATF_VALID;
      }
      AccessSystemParametersInfo(SPI_SETACCESSTIMEOUT, sizeof(g_ato), &g_ato, fWinIni);
      s_asPrev.ato = g_ato;
      fAnyNotifyChange = TRUE;
   }

   if (!IsSerialKeysEqual(&g_serk, &s_asPrev.serk))
   {
      AccessSystemParametersInfo(SPI_SETSERIALKEYS, sizeof(g_serk), &g_serk, fWinIni);
      CopySerialKeys(&s_asPrev.serk, &g_serk);
      fAnyNotifyChange = TRUE;
   }

   if (g_fSaveSettings) {
      if (RegCreateKeyEx(HKEY_CURRENT_USER, GENERAL_KEY, 0, __TEXT(""), REG_OPTION_NON_VOLATILE,
         KEY_SET_VALUE, NULL, &hkey, &dwDisposition) == ERROR_SUCCESS) {

         // Save the Warning Sounds entry
          if (g_fShowWarnMsgOnFeatureActivate != s_asPrev.fShowWarnMsgOnFeatureActivate)
          {
               RegSetValueEx(hkey, WARNING_SOUNDS, 0, REG_DWORD, (PBYTE) &g_fShowWarnMsgOnFeatureActivate,
                  sizeof(g_fShowWarnMsgOnFeatureActivate));
               s_asPrev.fShowWarnMsgOnFeatureActivate = g_fShowWarnMsgOnFeatureActivate;
          }

         // Save the Sound On Activation entry
          if (g_fPlaySndOnFeatureActivate != s_asPrev.fPlaySndOnFeatureActivate)
          {
              RegSetValueEx(hkey, SOUND_ON_ACTIVATION, 0, REG_DWORD, (PBYTE) &g_fPlaySndOnFeatureActivate,
                sizeof(g_fPlaySndOnFeatureActivate));
              s_asPrev.fPlaySndOnFeatureActivate = g_fPlaySndOnFeatureActivate;
          }
         RegCloseKey(hkey);
         hkey = NULL;
      }
   }

   // Sound Property page
   if (0 != memcmp(&g_ss, &s_asPrev.ss, sizeof(g_ss)))
   {
      if (g_fWinNT)
      {
         g_ss.dwFlags &= SSF_VALID;
      }
      AccessSystemParametersInfo(SPI_SETSOUNDSENTRY, sizeof(g_ss), &g_ss, fWinIni);
      s_asPrev.ss = g_ss;
      fAnyNotifyChange = TRUE;
   }


   // We do the sound property page last because the SPI_SETSHOWSOUNDS call is used
   // to send out notifications.  We make this call if either g_fShowSounds changed
   // or we need to send out notifications
   // Changed Nov.18 '98 to send out WM_SETTINGSCHANGE Seperately.

   if (g_fShowSounds != s_asPrev.fShowSounds /*||
      (fAnyNotifyChange && g_fSaveSettings)*/)
   {
      // if (g_fSaveSettings) fWinIni |= SPIF_SENDWININICHANGE;

      AccessSystemParametersInfo(SPI_SETSHOWSOUNDS, g_fShowSounds, NULL, fWinIni);
      s_asPrev.fShowSounds = g_fShowSounds;
   }

   g_SPISetValue = FALSE;

   // Do Admin options
   SaveDefaultSettings(g_fCopyToLogon, g_fCopyToDefault);

   SetCursor(LoadCursor(NULL, IDC_ARROW));
}


//////////////////////////////////////////////////////////////////////////


INT_PTR WINAPI KeyboardDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI SoundDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI GeneralDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI DisplayDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI MouseDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define MAX_PAGES 10


// ************************************************************************
// OpenAccessPropertySheet
// Opens property sheet
// ************************************************************************

BOOL OpenAccessPropertySheet (HWND hwnd, int nID) {
   HPROPSHEETPAGE rPages[MAX_PAGES];
   PROPSHEETPAGE psp;
   PROPSHEETHEADER psh;
   INT_PTR nPsRet;

   KillAccStat();
   GetAccessibilitySettings();

   // Simple errorchecking - only allow control to move to tabs 0-4.
   // Any tab request greater than 4 is invalid - so default to tab 0
   if ((nID < 0) || (nID > 4)) nID = 0;

   // Initialize the property sheets
   psh.dwSize = sizeof(psh);
   // SteveDon 5-26-98
   // no longer use PSH_PROPTITLE because we want it to read "Accessibility Options"
   // rather than "Accessibility Properties" or "Properties for Accessibility"
   psh.dwFlags = 0;     // psh.dwFlags = PSH_PROPTITLE; // | PSH_PROPSHEETPAGE | PSP_USEICONID;
   psh.hwndParent = hwnd;
   psh.hInstance = g_hinst;
   psh.pszCaption = MAKEINTRESOURCE(IDS_PROPERTY_TITLE); //ACCESSIBILITY);
   psh.pszIcon = MAKEINTRESOURCE(IDI_ACCESS);
   psh.nPages = 0;
   psh.nStartPage = 0;
   psh.phpage = rPages;

   // Add First Sheet, keyboard
   psp.dwSize = sizeof(psp);
   psp.dwFlags = PSP_DEFAULT;
   psp.hInstance = g_hinst;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_KEYBOARD);
   psp.pfnDlgProc = KeyboardDlg;
   psp.lParam = 0;

   psh.phpage[psh.nPages] = CreatePropertySheetPage(&psp);
   if (psh.phpage[psh.nPages]) psh.nPages++;

   // Add second sheet, Sound
   psp.dwSize = sizeof(psp);
   psp.dwFlags = PSP_DEFAULT;
   psp.hInstance = g_hinst;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_SOUND);
   psp.pfnDlgProc = SoundDlg;
   psp.lParam = 0;

   psh.phpage[psh.nPages] = CreatePropertySheetPage(&psp);
   if (psh.phpage[psh.nPages]) psh.nPages++;

   // Add third sheet, Display
   psp.dwSize = sizeof(psp);
   psp.dwFlags = PSP_DEFAULT;
   psp.hInstance = g_hinst;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_DISPLAY);
   psp.pfnDlgProc = DisplayDlg;
   psp.lParam = 0;

   psh.phpage[psh.nPages] = CreatePropertySheetPage(&psp);
   if (psh.phpage[psh.nPages]) psh.nPages++;

   // Add fourth sheet, Mouse
   psp.dwSize = sizeof(psp);
   psp.dwFlags = PSP_DEFAULT;
   psp.hInstance = g_hinst;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_MOUSE);
   psp.pfnDlgProc = MouseDlg;
   psp.lParam = 0;

   psh.phpage[psh.nPages] = CreatePropertySheetPage(&psp);
   if (psh.phpage[psh.nPages]) psh.nPages++;

   // Add fifth sheet, General
   psp.dwSize = sizeof(psp);
   psp.dwFlags = PSP_DEFAULT;
   psp.hInstance = g_hinst;
   psp.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
   psp.pfnDlgProc = GeneralDlg;
   psp.lParam = 0;

   psh.phpage[psh.nPages] = CreatePropertySheetPage(&psp);
   if (psh.phpage[psh.nPages]) psh.nPages++;

   // Simple errorchecking - only allow control to move to tabs 0 to psh.nPages
   // Any tab request greater than psh.nPages is invalid
   if (0 <= nID && nID < (int)psh.nPages)
   {
      psh.nStartPage = nID;
   }

   nPsRet = PropertySheet(&psh);

   if ( nPsRet <= 0 )
       return FALSE;
   else
       return TRUE;
}

///////////////////////////////// End of File /////////////////////////////////
