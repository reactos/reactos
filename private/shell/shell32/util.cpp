#include "shellprv.h"
#include "util.h"

#include <advpub.h>         // For REGINSTALL
#include <ntverp.h>
#include <urlmon.h>
#include <shlwapi.h>
#include "shldisp.h"
#include <malloc.h>
#include "shitemid.h"
#include "datautil.h"
#include <perhist.h>    // IPersistHistory is defined here.
#include <mmhelper.h>
#include "lm.h"

#include "ids.h"
#include "views.h"
#include "ole2dup.h"
#include <vdate.h>
#include <regstr.h>
#include "unicpp\dutil.h"
#include <stdlib.h>

#include "prop.h"
#include "ftascstr.h"   // for CFTAssocStore
#include "ftcmmn.h"     // for MAX_APPFRIENDLYNAME
#include "ascstr.h"     // for IAssocInfo class
#include "fstreex.h"    // for CFSFolder_CreateFolder
#include "cscuiext.h"

#include "netview.h"    // for SHWNetGetConnection
#include <cscapi.h>     // for CSCQueryFileStatus

//The following is defined in shell32\unicpp\dutil.cpp
void GetRegLocation(LPTSTR lpszResult, DWORD cchResult, LPCTSTR lpszKey, LPCTSTR lpszScheme);

#define DM_STRICT       TF_WARNING  // audit menus, etc.
#define DM_STRICT2      0           // verbose

// used by SHRegGetCLSIDKey and MoveCLSIDs
#define EXPLORER_CLSID TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\")
#define SOFTWARE_CLASSES_CLSID TEXT("Software\\Classes\\CLSID\\")

// used by IsPathInOpenWithKillList
#define SZ_REGKEY_FILEASSOCIATION TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileAssociation")

//
// We need to put this one in per-instance data section because during log-off
// and log-on, this info needs to be re-read from the registry.
//
// REGSHELLSTATE is the version of the SHELLSTATE that goes into the
// registry.  When loading a REGSHELLSTATE, you have to study the
// cbSize to see if it's a downlevel structure and upgrade it accordingly.
//

typedef struct REGSHELLSTATE
{
    UINT cbSize;
    SHELLSTATE ss;
} REGSHELLSTATE;

#define REGSHELLSTATE_SIZE_WIN95 (sizeof(UINT)+SHELLSTATE_SIZE_WIN95)  // Win95 Gold
#define REGSHELLSTATE_SIZE_NT4   (sizeof(UINT)+SHELLSTATE_SIZE_NT4)    // Win95 OSR / NT 4
#define REGSHELLSTATE_SIZE_IE4   (sizeof(UINT)+SHELLSTATE_SIZE_IE4)    // IE 4, 4.01
#define REGSHELLSTATE_SIZE_WIN2K (sizeof(UINT)+SHELLSTATE_SIZE_WIN2K)  // ie5, win2k

// If the SHELLSTATE size changes, we need to add a new define
// above and new upgrade code in SHRefreshSettings
#ifdef DEBUG
void snafu () {COMPILETIME_ASSERT(REGSHELLSTATE_SIZE_WIN2K == sizeof(REGSHELLSTATE));}
#endif DEBUG

REGSHELLSTATE * g_pShellState = 0;

//
// We need to put this one in per-instance data section because during log-off
// and log-on, this info needs to be re-read from the registry.
//
HMODULE g_hmodWinMM = NULL;

STDAPI_(HMODULE) LoadMM()
{
    if (g_hmodWinMM == (HMODULE)-1)
        return NULL;   // Could not load Multimedia...

    if (!g_hmodWinMM)
    {
        g_hmodWinMM = LoadLibrary(TEXT("winmm.dll"));
        if (!g_hmodWinMM)
        {
            g_hmodWinMM = (HMODULE)-1;
            return NULL;
        }
    }
    return g_hmodWinMM;
}

typedef BOOL (WINAPI *PLAYSOUNDFN)(LPCTSTR lpsz, HANDLE hMod, DWORD dwFlags);
typedef UINT (WINAPI *UINTVOIDFN)();

STDAPI_(void) SHPlaySound(LPCTSTR pszSound)
{
    DWORD cbSize = 0;
    TCHAR szKey[CCH_KEYMAX];

    // check the registry first
    // if there's nothing registered, we blow off the play,
    // but we don't set the MM_DONTLOAD flag so taht if they register
    // something we will play it
    wsprintf(szKey, TEXT("AppEvents\\Schemes\\Apps\\Explorer\\%s\\.current"), pszSound);

    if ((SHRegQueryValue(HKEY_CURRENT_USER, szKey, NULL, (LPLONG) &cbSize) == ERROR_SUCCESS) && cbSize)
    {
        HMODULE hMM = LoadMM();
        if (hMM)
        {
            UINTVOIDFN pfnwaveOutGetNumDevs = (UINTVOIDFN)GetProcAddress(hMM, "waveOutGetNumDevs");
            if (pfnwaveOutGetNumDevs && pfnwaveOutGetNumDevs())
            {
#ifdef UNICODE
                PLAYSOUNDFN pfnPlaySound = (PLAYSOUNDFN)GetProcAddress(hMM, "PlaySoundW");
#else
                PLAYSOUNDFN pfnPlaySound = (PLAYSOUNDFN)GetProcAddress(hMM, "PlaySoundA");
#endif
                if (pfnPlaySound)
                    pfnPlaySound(pszSound, NULL, SND_ALIAS | SND_APPLICATION | SND_ASYNC);
            }
        }
    }
}


// helper function to set whether shift or control is down at the start of the invoke
// operation, so others down the line can check this instead of calling GetAsyncKeyState themselves
STDAPI_(void) SetICIKeyModifiers(DWORD* pfMask)
{
    ASSERT(pfMask);

    if (GetKeyState(VK_SHIFT) < 0)
    {
        *pfMask |= CMIC_MASK_SHIFT_DOWN;
    }

    if (GetKeyState(VK_CONTROL) < 0)
    {
        *pfMask |= CMIC_MASK_CONTROL_DOWN;
    }
}


// sane way to get the msg pos into a point, mostly needed for win32
void GetMsgPos(POINT *ppt)
{
    DWORD dw = GetMessagePos();

    ppt->x = GET_X_LPARAM(dw);
    ppt->y = GET_Y_LPARAM(dw);
}

/*  This gets the number of consecutive chrs of the same kind.  This is used
 *  to parse the time picture.  Returns 0 on error.
 */

int GetPict(WCHAR ch, LPWSTR wszStr)
{
    int count = 0;
    while (ch == *wszStr++)
        count++;

    return count;
}

DWORD CALLBACK _PropSheetThreadProc(void *pvPropStuff)
{
    DWORD dwRet;
    PROPSTUFF * pps = (PROPSTUFF *) pvPropStuff;

    OleInitialize(0);

    dwRet = pps->lpStartAddress(pps);

    // cleanup
    if (pps->pdtobj)
        pps->pdtobj->Release();

    if (pps->pidlParent)
        ILFree(pps->pidlParent);

    if (pps->psf)
        pps->psf->Release();

    LocalFree(pps);

    OleUninitialize();

    return dwRet;
}


//
// helper function for pulling ITypeInfo out of our typelib
//
STDAPI Shell32GetTypeInfo(LCID lcid, UUID uuid, ITypeInfo **ppITypeInfo)
{
    HRESULT    hr;
    ITypeLib  *pITypeLib;

    // Just in case we can't find the type library anywhere
    *ppITypeInfo = NULL;

    /*
     * The type libraries are registered under 0 (neutral),
     * 7 (German), and 9 (English) with no specific sub-
     * language, which would make them 407 or 409 and such.
     * If you are sensitive to sub-languages, then use the
     * full LCID instead of just the LANGID as done here.
     */
    hr = LoadRegTypeLib(LIBID_Shell32, 1, 0, PRIMARYLANGID(lcid), &pITypeLib);

    /*
     * If LoadRegTypeLib fails, try loading directly with
     * LoadTypeLib, which will register the library for us.
     * Note that there's no default case here because the
     * prior switch will have filtered lcid already.
     *
     * NOTE:  You should prepend your DIR registry key to the
     * .TLB name so you don't depend on it being it the PATH.
     * This sample will be updated later to reflect this.
     */
    if (FAILED(hr))
    {
        OLECHAR wszPath[MAX_PATH];
#ifdef UNICODE
        GetModuleFileName(HINST_THISDLL, wszPath, ARRAYSIZE(wszPath));
#else
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(HINST_THISDLL, szPath, ARRAYSIZE(szPath));
        MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, ARRAYSIZE(wszPath));
#endif

        switch (PRIMARYLANGID(lcid))
        {
        case LANG_NEUTRAL:
        case LANG_ENGLISH:
            hr=LoadTypeLib(wszPath, &pITypeLib);
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        //Got the type lib, get type info for the interface we want
        hr = pITypeLib->GetTypeInfoOfGuid(uuid, ppITypeInfo);
        pITypeLib->Release();
    }

    return hr;
}


// BUGBUG (reinerf)
// the fucking alpha cpp compiler seems to fuck up the goddam type "LPITEMIDLIST", so to work
// around the fucking peice of shit compiler we pass the last param as an void *instead of a LPITEMIDLIST

STDAPI_(void) SHLaunchPropSheet(LPTHREAD_START_ROUTINE lpStartAddress, IDataObject * pdtobj, LPCTSTR pStartPage, IShellFolder * psf, void *pidl)
{
    LPITEMIDLIST pidlParent = (LPITEMIDLIST)pidl;
    UINT cbStartPage = !IS_INTRESOURCE(pStartPage) ? ((lstrlen(pStartPage) + 1) * SIZEOF(TCHAR)) : 0 ;
    PROPSTUFF * pps = (PROPSTUFF *)LocalAlloc(LPTR, SIZEOF(PROPSTUFF) + cbStartPage);
    if (pps)
    {
        pps->lpStartAddress = lpStartAddress;

        if (pdtobj) 
        {
            pps->pdtobj = pdtobj;
            pdtobj->AddRef();
        }

        if (pidlParent)
            pps->pidlParent = ILClone(pidlParent);

        if (psf) 
        {
            pps->psf = psf;
            psf->AddRef();
        }

        pps->pStartPage = pStartPage;
        if (!IS_INTRESOURCE(pStartPage))
        {
            pps->pStartPage = (LPTSTR)(pps + 1);
            lstrcpy((LPTSTR)(pps->pStartPage), pStartPage);
        }

        SHCreateThread(_PropSheetThreadProc, pps, CTF_INSIST | CTF_PROCESS_REF, NULL);
    }
}

/*  This picks up the values in wValArray, converts them
 *  in a string containing the formatted date.
 *  wValArray should contain Month-Day-Year (in that order).
 */

int CreateDate(WORD *wValArray, LPWSTR wszOutStr)
{
  int     i;
  int     cchPictPart;
  WORD    wDigit;
  WORD    wIndex;
  WORD    wTempVal;
  LPWSTR  pwszPict, pwszInStr;
  WCHAR   wszShortDate[30];      // need more room for LOCALE_SSHORTDATE

  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, wszShortDate, ARRAYSIZE(wszShortDate));
  pwszPict = wszShortDate;
  pwszInStr = wszOutStr;

  for (i=0; (i < 5) && (*pwszPict); i++)
  {
      cchPictPart = GetPict(*pwszPict, pwszPict);
      switch (*pwszPict)
      {
          case TEXT('M'):
          case TEXT('m'):
          {
              wIndex = 0;
              break;
          }

          case TEXT('D'):
          case TEXT('d'):
          {
              //
              // if short date style && *pszPict is 'd' &&
              // cchPictPart is more than equal 3,
              // then it is the day of the week.
              //
              if (cchPictPart >= 3)
              {
                  pwszPict += cchPictPart;
                  continue;
              }
              wIndex = 1;
              break;
          }

          case TEXT('Y'):
          case TEXT('y'):
          {
              wIndex = 2;
              if (cchPictPart == 4)
              {
                  if (wValArray[2] >=100)
                  {
                      *pwszInStr++ = TEXT('2');
                      *pwszInStr++ = TEXT('0');
                      wValArray[2]-= 100;
                  }
                  else
                  {
                      *pwszInStr++ = TEXT('1');
                      *pwszInStr++ = TEXT('9');
                  }
              }
              else if (wValArray[2] >=100)  // handle year 2000
                  wValArray[2]-= 100;

              break;
          }

          case TEXT('g'):
          {
              // era string
              pwszPict += cchPictPart;
              while (*pwszPict == TEXT(' ')) pwszPict++;
              continue;
          }

          case TEXT('\''):
          {
             while (*pwszPict && *++pwszPict != TEXT('\'')) ;
             continue;
          }

          default:
          {
              goto CDFillIn;
              break;
          }
      }

      /* This assumes that the values are of two digits only. */
      wTempVal = wValArray[wIndex];

      wDigit = wTempVal / 10;
      if (wDigit)
          *pwszInStr++ = (TCHAR)(wDigit + TEXT('0'));
      else if (cchPictPart > 1)
          *pwszInStr++ = TEXT('0');

      *pwszInStr++ = (TCHAR)((wTempVal % 10) + TEXT('0'));

      pwszPict += cchPictPart;

CDFillIn:
      /* Add the separator. */
      while ((*pwszPict) &&
             (*pwszPict != TEXT('\'')) &&
             (*pwszPict != TEXT('M')) && (*pwszPict != TEXT('m')) &&
             (*pwszPict != TEXT('D')) && (*pwszPict != TEXT('d')) &&
             (*pwszPict != TEXT('Y')) && (*pwszPict != TEXT('y')))
      {
          *pwszInStr++ = *pwszPict++;
      }
  }

  *pwszInStr = TEXT('\0');

  return lstrlenW(wszOutStr);
}


#define DATEMASK        0x001F
#define MONTHMASK       0x01E0
#define MINUTEMASK      0x07E0
#define SECONDSMASK     0x001F

int WINAPI GetDateString(WORD wDate, LPWSTR wszStr)
{
  WORD  wValArray[3];

  wValArray[0] = (wDate & MONTHMASK) >> 5;              /* Month */
  wValArray[1] = (wDate & DATEMASK);                    /* Date  */
  wValArray[2] = (wDate >> 9) + 80;                     /* Year  */

  return CreateDate(wValArray, wszStr);
}

WORD WINAPI ParseDateString(LPCWSTR pszStr)
{
    //
    // We need to loop through the string and extract off the month/day/year
    // We will do it in the order of the NlS definition...
    //
    WORD    wParts[3];
    int     i;
    int     cchPictPart;
    WORD    wIndex;
    WORD    wTempVal;
    WCHAR   szShortDate[30];    // need more room for LOCALE_SSHORTDATE
    LPWSTR  pszPict;

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, szShortDate, ARRAYSIZE(szShortDate));
    pszPict = szShortDate;

    while (*pszPict && (*pszPict == *pszStr))
    {
        pszPict++;
        pszStr++;
    }

    for (i=0; (i < 5) && (*pszPict); i++)
    {
        cchPictPart = GetPict(*pszPict, pszPict);
        switch (*pszPict)
        {
        case TEXT('M'):
        case TEXT('m'):
            wIndex = 0;
            break;

        case TEXT('D'):
        case TEXT('d'):
            //
            // if short date style && *pszPict is 'd' &&
            // cchPictPart is more than equal 3,
            // then it is the day of the week.
            if (cchPictPart >= 3)
            {
                pszPict += cchPictPart;
                continue;
            }
            wIndex = 1;
            break;

        case TEXT('Y'):
        case TEXT('y'):
            wIndex = 2;
            break;

        case TEXT('g'):
        {
            // era string
            pszPict += cchPictPart;
            while (*pszPict == TEXT(' ')) pszPict++;
            continue;
        }

        case TEXT('\''):
        {
            while (*pszPict && *++pszPict != TEXT('\'')) ;
            continue;
        }

        default:
           return(0);
        }

        // We now want to loop through each of the characters while
        // they are numbers and build the number;
        //
        wTempVal = 0;
        while ((*pszStr >= TEXT('0')) && (*pszStr <= TEXT('9')))
        {
            wTempVal = wTempVal * 10 + (WORD)(*pszStr - TEXT('0'));
            pszStr++;
        }
        wParts[wIndex] = wTempVal;

        // Now make sure we have the correct separator
        pszPict += cchPictPart;
        if (*pszPict != *pszStr)
        {
           return(0);
        }
        while (*pszPict && (*pszPict == *pszStr))
        {
            //
            //  The separator can actually be more than one character
            //  in length.
            //
            pszPict++;  // align to the next field
            pszStr++;   // Align to next field
        }
    }

    //
    // Do some simple checks to see if the date looks half way reasonable.
    //
    if (wParts[2] < 80)
        wParts[2] += (2000 - 1900);  // Wrap to next century but leave as two digits...
    if (wParts[2] >= 1900)
        wParts[2] -= 1900;  // Get rid of Century
    if ((wParts[0] == 0) || (wParts[0] > 12) ||
            (wParts[1] == 0) || (wParts[1] > 31) ||
            (wParts[2] >= 200))
    {
        return(0);
    }

    // We now have the three parts so lets construct the date value

    // Now construct the date number
    return ((wParts[2] - 80) << 9) + (wParts[0] << 5) + wParts[1];
}


STDAPI_(BOOL) IsNullTime(const FILETIME *pft)
{
    FILETIME ftNull = {0, 0};
    return CompareFileTime(&ftNull, pft) == 0;
}


STDAPI_(BOOL) TouchFile(LPCTSTR pszFile)
{
    BOOL bRet = FALSE;
    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_OPEN_NO_RECALL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME st;
        FILETIME ft;

        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);

        bRet = SetFileTime(hFile, &ft, &ft, &ft);
        CloseHandle(hFile);
    }
    return bRet;
}

void Int64ToStr(LONGLONG n, LPTSTR lpBuffer)
{
    TCHAR szTemp[40];
    LONGLONG  iChr;

    iChr = 0;

    do {
        szTemp[iChr++] = TEXT('0') + (TCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = '\0';
}

//
//  Obtain NLS info about how numbers should be grouped.
//
//  The annoying thing is that LOCALE_SGROUPING and NUMBERFORMAT
//  have different ways of specifying number grouping.
//
//          LOCALE      NUMBERFMT      Sample   Country
//
//          3;0         3           1,234,567   United States
//          3;2;0       32          12,34,567   India
//          3           30           1234,567   ??
//
//  Not my idea.  That's the way it works.
//
//  Bonus treat - Win9x doesn't support complex number formats,
//  so we return only the first number.
//
UINT GetNLSGrouping(void)
{
    UINT grouping;
    LPTSTR psz;
    TCHAR szGrouping[32];

    // If no locale info, then assume Western style thousands
    if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, ARRAYSIZE(szGrouping)))
        return 3;

    grouping = 0;
    psz = szGrouping;
#ifdef WINNT
    for (;;)
    {
        if (*psz == '0') break;             // zero - stop

        else if ((UINT)(*psz - '0') < 10)   // digit - accumulate it
            grouping = grouping * 10 + (UINT)(*psz - '0');

        else if (*psz)                      // punctuation - ignore it
            { }

        else                                // end of string, no "0" found
        {
            grouping = grouping * 10;       // put zero on end (see examples)
            break;                          // and finished
        }

        psz++;
    }
#else
    // Win9x - take only the first grouping
    grouping = StrToInt(szGrouping);
#endif

    return grouping;
}

// takes a DWORD add commas etc to it and puts the result in the buffer
STDAPI_(LPTSTR) AddCommas64(LONGLONG n, LPTSTR pszResult)
{
    // BUGBUGBC: 40 is bogus, it requires callers to know their buffer must
    //  be 40

    TCHAR  szTemp[MAX_COMMA_NUMBER_SIZE];
    TCHAR  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits=0;
    nfmt.LeadingZero=0;
    nfmt.Grouping = GetNLSGrouping();
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder= 0;

    Int64ToStr(n, szTemp);

    // BUGBUG:: Should have passed in size..
    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt, pszResult, MAX_COMMA_NUMBER_SIZE) == 0)
        lstrcpy(pszResult, szTemp);

    return pszResult;
}

// takes a DWORD add commas etc to it and puts the result in the buffer
STDAPI_(LPTSTR) AddCommas(DWORD n, LPTSTR pszResult)
{
    return AddCommas64(n, pszResult);
}

STDAPI_(LPTSTR) ShortSizeFormat64(LONGLONG n, LPTSTR szBuf)
{
    return StrFormatByteSize64(n, szBuf, 40);
}

STDAPI_(LPTSTR) ShortSizeFormat(DWORD n, LPTSTR szBuf)
{
    return StrFormatByteSize64(n, szBuf, 40);
}



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: Int64ToString
//
// DESCRIPTION:
//    Converts the numeric value of a LONGLONG to a text string.
//    The string may optionally be formatted to include decimal places
//    and commas according to current user locale settings.
//
// ARGUMENTS:
//    n
//       The 64-bit integer to format.
//
//    szOutStr
//       Address of the destination buffer.
//
//    nSize
//       Number of characters in the destination buffer.
//
//    bFormat
//       TRUE  = Format per locale settings.
//       FALSE = Leave number unformatted.
//
//    pFmt
//       Address of a number format structure of type NUMBERFMT.
//       If NULL, the function automatically provides this information
//       based on the user's default locale settings.
//
//    dwNumFmtFlags
//       Encoded flag word indicating which members of *pFmt to use in
//       formatting the number.  If a bit is clear, the user's default
//       locale setting is used for the corresponding format value.  These
//       constants can be OR'd together.
//
//          NUMFMT_IDIGITS
//          NUMFMT_ILZERO
//          NUMFMT_SGROUPING
//          NUMFMT_SDECIMAL
//          NUMFMT_STHOUSAND
//          NUMFMT_INEGNUMBER
//
///////////////////////////////////////////////////////////////////////////////
INT WINAPI Int64ToString(LONGLONG n, LPTSTR szOutStr, UINT nSize, BOOL bFormat,
                         NUMBERFMT *pFmt, DWORD dwNumFmtFlags)
{
    INT nResultSize;
    TCHAR szBuffer[_MAX_PATH + 1];
    NUMBERFMT NumFmt;
    TCHAR szDecimalSep[5];
    TCHAR szThousandSep[5];
    
    ASSERT(NULL != szOutStr);
    
    //
    // Use only those fields in caller-provided NUMBERFMT structure
    // that correspond to bits set in dwNumFmtFlags.  If a bit is clear,
    // get format value from locale info.
    //
    if (bFormat)
    {
        TCHAR szInfo[20];
        
        if (NULL == pFmt)
            dwNumFmtFlags = 0;  // Get all format data from locale info.
        
        if (dwNumFmtFlags & NUMFMT_IDIGITS)
        {
            NumFmt.NumDigits = pFmt->NumDigits;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szInfo, ARRAYSIZE(szInfo));
            NumFmt.NumDigits = StrToLong(szInfo);
        }
        
        if (dwNumFmtFlags & NUMFMT_ILZERO)
        {
            NumFmt.LeadingZero = pFmt->LeadingZero;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szInfo, ARRAYSIZE(szInfo));
            NumFmt.LeadingZero = StrToLong(szInfo);
        }
        
        if (dwNumFmtFlags & NUMFMT_SGROUPING)
        {
            NumFmt.Grouping = pFmt->Grouping;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szInfo, ARRAYSIZE(szInfo));
            NumFmt.Grouping = StrToLong(szInfo);
        }
        
        if (dwNumFmtFlags & NUMFMT_SDECIMAL)
        {
            NumFmt.lpDecimalSep = pFmt->lpDecimalSep;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep, ARRAYSIZE(szDecimalSep));
            NumFmt.lpDecimalSep = szDecimalSep;
        }
        
        if (dwNumFmtFlags & NUMFMT_STHOUSAND)
        {
            NumFmt.lpThousandSep = pFmt->lpThousandSep;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandSep, ARRAYSIZE(szThousandSep));
            NumFmt.lpThousandSep = szThousandSep;
        }
        
        if (dwNumFmtFlags & NUMFMT_INEGNUMBER)
        {
            NumFmt.NegativeOrder = pFmt->NegativeOrder;
        }
        else
        {
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, szInfo, ARRAYSIZE(szInfo));
            NumFmt.NegativeOrder  = StrToLong(szInfo);
        }
        
        pFmt = &NumFmt;
    }
    
    Int64ToStr(n, szBuffer);
    
    //
    //  Format the number string for the locale if the caller wants a
    //  formatted number string.
    //
    if (bFormat)
    {
        if ( 0 != ( nResultSize = GetNumberFormat( LOCALE_USER_DEFAULT,  // User's locale
            0,                            // No flags
            szBuffer,                     // Unformatted number string
            pFmt,                         // Number format info
            szOutStr,                     // Output buffer
            nSize )) )                    // Chars in output buffer.
        {
            //
            //  Remove nul terminator char from return size count.
            //
            --nResultSize;
        }
    }
    else
    {
        //
        //  GetNumberFormat call failed, so just return the number string
        //  unformatted.
        //
        lstrcpyn(szOutStr, szBuffer, nSize);
        nResultSize = lstrlen(szOutStr);
    }
    
    return nResultSize;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: LargeIntegerToString
//
// DESCRIPTION:
//    Converts the numeric value of a LARGE_INTEGER to a text string.
//    The string may optionally be formatted to include decimal places
//    and commas according to current user locale settings.
//
// ARGUMENTS:
//    pN
//       Address of the large integer to format.
//
//    See description of Int64ToString for remaining arguments.
//
///////////////////////////////////////////////////////////////////////////////
INT WINAPI LargeIntegerToString(LARGE_INTEGER *pN, LPTSTR szOutStr, UINT nSize,
                                BOOL bFormat, NUMBERFMT *pFmt,
                                DWORD dwNumFmtFlags)
{
    ASSERT(NULL != pN);
    return Int64ToString(pN->QuadPart, szOutStr, nSize, bFormat, pFmt, dwNumFmtFlags);
}



#define ISSEP(c)   ((c) == TEXT('=')  || (c) == TEXT(','))
#define ISWHITE(c) ((c) == TEXT(' ')  || (c) == TEXT('\t') || (c) == TEXT('\n') || (c) == TEXT('\r'))
#define ISNOISE(c) ((c) == TEXT('"'))
#define EOF     26

#define QUOTE   TEXT('"')
#define COMMA   TEXT(',')
#define SPACE   TEXT(' ')
#define EQUAL   TEXT('=')

/* BOOL ParseField(szData,n,szBuf,iBufLen)
 *
 * Given a line from SETUP.INF, will extract the nth field from the string
 * fields are assumed separated by comma's.  Leading and trailing spaces
 * are removed.
 *
 * ENTRY:
 *
 * szData    : pointer to line from SETUP.INF
 * n         : field to extract. ( 1 based )
 *             0 is field before a '=' sign
 * szDataStr : pointer to buffer to hold extracted field
 * iBufLen   : size of buffer to receive extracted field.
 *
 * EXIT: returns TRUE if successful, FALSE if failure.
 *
 */
BOOL WINAPI ParseField(LPCTSTR szData, int n, LPTSTR szBuf, int iBufLen)
{
    BOOL  fQuote = FALSE;
    LPCTSTR pszInf = szData;
    LPTSTR ptr;
    int   iLen = 1;
    
    if (!szData || !szBuf)
        return FALSE;
    
        /*
        * find the first separator
    */
    while (*pszInf && !ISSEP(*pszInf))
    {
        if (*pszInf == QUOTE)
            fQuote = !fQuote;
        pszInf = CharNext(pszInf);
    }
    
    if (n == 0 && *pszInf != TEXT('='))
        return FALSE;
    
    if (n > 0 && *pszInf == TEXT('=') && !fQuote)
        // Change szData to point to first field
        szData = ++pszInf; // Ok for DBCS
    
                           /*
                           *   locate the nth comma, that is not inside of quotes
    */
    fQuote = FALSE;
    while (n > 1)
    {
        while (*szData)
        {
            if (!fQuote && ISSEP(*szData))
                break;
            
            if (*szData == QUOTE)
                fQuote = !fQuote;
            
            szData = CharNext(szData);
        }
        
        if (!*szData)
        {
            szBuf[0] = 0;      // make szBuf empty
            return FALSE;
        }
        
        szData = CharNext(szData); // we could do ++ here since we got here
        // after finding comma or equal
        n--;
    }
    
    /*
    * now copy the field to szBuf
    */
    while (ISWHITE(*szData))
        szData = CharNext(szData); // we could do ++ here since white space can
    // NOT be a lead byte
    fQuote = FALSE;
    ptr = szBuf;      // fill output buffer with this
    while (*szData)
    {
        if (*szData == QUOTE)
        {
            //
            // If we're in quotes already, maybe this
            // is a double quote as in: "He said ""Hello"" to me"
            //
            if (fQuote && *(szData+1) == QUOTE)    // Yep, double-quoting - QUOTE is non-DBCS
            {
                if (iLen < iBufLen)
                {
                    *ptr++ = QUOTE;
                    ++iLen;
                }
                szData++;                   // now skip past 1st quote
            }
            else
                fQuote = !fQuote;
        }
        else if (!fQuote && ISSEP(*szData))
            break;
        else
        {
            if ( iLen < iBufLen )
            {
                *ptr++ = *szData;                  // Thank you, Dave
                ++iLen;
            }
            
            if ( IsDBCSLeadByte(*szData) && (iLen < iBufLen) )
            {
                *ptr++ = szData[1];
                ++iLen;
            }
        }
        szData = CharNext(szData);
    }
    /*
    * remove trailing spaces
    */
    while (ptr > szBuf)
    {
        ptr = CharPrev(szBuf, ptr);
        if (!ISWHITE(*ptr))
        {
            ptr = CharNext(ptr);
            break;
        }
    }
    *ptr = 0;
    return TRUE;
}


// Sets and clears the "wait" cursor.
// REVIEW UNDONE - wait a specific period of time before actually bothering
// to change the cursor.
// REVIEW UNDONE - support for SetWaitPercent();
//    BOOL bSet   TRUE if you want to change to the wait cursor, FALSE if
//                you want to change it back.
void WINAPI SetAppStartingCursor(HWND hwnd, BOOL bSet)
{
#ifdef WINNT
    DWORD dwTargetProcID;
#endif
    //g_hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    if (hwnd && IsWindow(hwnd)) {
        HWND hwndOwner;
        while((NULL != (hwndOwner = GetParent(hwnd))) || (NULL != (hwndOwner = GetWindow(hwnd, GW_OWNER)))) {
            hwnd = hwndOwner;
        }

#ifdef WINNT
        // SendNotify is documented to only work in-process (and can
        // crash if we pass the pnmhdr across process boundaries on
        // NT, because DLLs aren't all shared in one address space).
        // So, if this SendNotify would go cross-process, blow it off.

        GetWindowThreadProcessId(hwnd, &dwTargetProcID);

        if (GetCurrentProcessId() == dwTargetProcID)
#endif
            SendNotify(hwnd, NULL, bSet ? NM_STARTWAIT : NM_ENDWAIT, NULL);
    }
}

HWND WINAPI GetTopLevelAncestor(HWND hWnd)
{
    HWND hwndTemp;

    while ((hwndTemp=GetParent(hWnd)) != NULL)
    {
        hWnd = hwndTemp;
    }

    return(hWnd);
}


#ifdef DEBUG // {

//***   IS_* -- character classification routines
// ENTRY/EXIT
//  ch      TCHAR to be checked
//  return  TRUE if in range, FALSE if not
#define IS_LOWER(ch)    InRange(ch, TEXT('a'), TEXT('z'))
#define IS_UPPER(ch)    InRange(ch, TEXT('A'), TEXT('Z'))
#define IS_ALPHA(ch)    (IS_LOWER(ch) || IS_UPPER(ch))
#define IS_DIGIT(ch)    InRange(ch, TEXT('0'), TEXT('9'))
#define TO_UPPER(ch)    ((ch) - TEXT('a') + TEXT('A'))

//***   BMAP_* -- bitmap routines
// ENTRY/EXIT
//  pBits       ptr to bitmap (array of bytes)
//  iBit        bit# to be manipulated
//  return      various...
// DESCRIPTION
//  BMAP_TEST   check bit #iBit of bitmap pBits
//  BMAP_SET    set   bit #iBit of bitmap pBits
// NOTES
//  warning: no overflow checks
#define BMAP_INDEX(iBit)        ((iBit) / 8)
#define BMAP_MASK(iBit)         (1 << ((iBit) % 8))
#define BMAP_BYTE(pBits, iBit)  (((char *)pBits)[BMAP_INDEX(iBit)])

#define BMAP_TEST(pBits, iBit)  (BMAP_BYTE(pBits, iBit) & BMAP_MASK(iBit))
#define BMAP_SET(pBits, iBit)   (BMAP_BYTE(pBits, iBit) |= BMAP_MASK(iBit))

//***   DBGetMnemonic -- get menu mnemonic
// ENTRY/EXIT
//  return  mnemonic if found, o.w. 0
// NOTES
//  we handle and skip escaped-& ('&&')
//
TCHAR DBGetMnemonic(LPTSTR pszName)
{
    for (; *pszName != 0; pszName = CharNext(pszName)) {
        if (*pszName == TEXT('&')) {
            pszName = CharNext(pszName);    // skip '&'
            if (*pszName != TEXT('&'))
                return *pszName;
            ASSERT(0);  // untested! (but should work...)
            pszName = CharNext(pszName);    // skip 2nd '&'
        }
    }
    // this one happens a lot w/ weird things like "", "..", "..."
    return 0;
}

//***   DBCheckMenu -- check menu for 'style' conformance
// DESCRIPTION
//  currently we just check for mnemonic collisions (and only a-z,0-9)
void DBCheckMenu(HMENU hmChk)
{
    long bfAlpha = 0;
    long bfDigit = 0;
    long *pbfMne;
    int nItem;
    int iMne;
    TCHAR chMne;
    TCHAR szName[256];      // BUGBUG #define XXX 256
    MENUITEMINFO miiChk;

    if (!DM_STRICT)
        return;

    for (nItem = GetMenuItemCount(hmChk) - 1; nItem >= 0; nItem--) {
        miiChk.cbSize = SIZEOF(MENUITEMINFO);
        miiChk.fMask = MIIM_TYPE|MIIM_DATA;
        // We need to reset this every time through the loop in case
        // menus DON'T have IDs
        miiChk.fType = MFT_STRING;
        miiChk.dwTypeData = szName;
        szName[0] = 0;
        miiChk.dwItemData = 0;
        miiChk.cch        = ARRAYSIZE(szName);

        if (!GetMenuItemInfo(hmChk, nItem, TRUE, &miiChk)) {
            TraceMsg(TF_WARNING, "dbcm: fail iMenu=%d (skip)", nItem);
            continue;
        }

        if (! (miiChk.fType & MFT_STRING)) {
            // skip separators, etc.
            continue;
        }

        chMne = DBGetMnemonic(szName);
        if (chMne == 0 || ! (IS_ALPHA(chMne) || IS_DIGIT(chMne))) {
            // this one actually happens a lot w/ chMne==0
            if (DM_STRICT2)
                TraceMsg(TF_WARNING, "dbcm: skip iMenu=%d mne=%c", nItem, chMne ? chMne : TEXT('0'));
            continue;
        }

        if (IS_LOWER(chMne)) {
            chMne = TO_UPPER(chMne);
        }

        if (IS_UPPER(chMne)) {
            iMne = chMne - TEXT('A');
            pbfMne = &bfAlpha;
        }
        else if (IS_DIGIT(chMne)) {
            iMne = chMne - TEXT('0');
            pbfMne = &bfDigit;
        }
        else {
            ASSERT(0);
            continue;
        }

        if (BMAP_TEST(pbfMne, iMne)) {
            TraceMsg(TF_ERROR, "dbcm: mnemonic collision hm=%x iM=%d szMen=%s",
                hmChk, nItem, szName);
        }

        BMAP_SET(pbfMne, iMne);
    }

    return;
}

#else // }{
#define DBCheckMenu(hmChk)  0
#endif // }

// Copy a menu onto the beginning or end of another menu
// Adds uIDAdjust to each menu ID (pass in 0 for no adjustment)
// Will not add any item whose adjusted ID is greater than uMaxIDAdjust
// (pass in 0xffff to allow everything)
// Returns one more than the maximum adjusted ID that is used
//

UINT WINAPI Shell_MergeMenus(HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags)
{
    int nItem;
    HMENU hmSubMenu;
    BOOL bAlreadySeparated;
    MENUITEMINFO miiSrc;
    TCHAR szName[256];
    UINT uTemp, uIDMax = uIDAdjust;

    if (!hmDst || !hmSrc)
    {
        goto MM_Exit;
    }

    nItem = GetMenuItemCount(hmDst);
    if (uInsert >= (UINT)nItem)
    {
        uInsert = (UINT)nItem;
        bAlreadySeparated = TRUE;
    }
    else
    {
        bAlreadySeparated = _SHIsMenuSeparator(hmDst, uInsert);;
    }

    if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
    {
        // Add a separator between the menus
        InsertMenu(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        bAlreadySeparated = TRUE;
    }


    // Go through the menu items and clone them
    for (nItem = GetMenuItemCount(hmSrc) - 1; nItem >= 0; nItem--)
    {
        miiSrc.cbSize = SIZEOF(MENUITEMINFO);
        miiSrc.fMask = MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_TYPE | MIIM_DATA;
        // We need to reset this every time through the loop in case
        // menus DON'T have IDs
        miiSrc.fType = MFT_STRING;
        miiSrc.dwTypeData = szName;
        miiSrc.dwItemData = 0;
        miiSrc.cch        = ARRAYSIZE(szName);

        if (!GetMenuItemInfo(hmSrc, nItem, TRUE, &miiSrc))
        {
            continue;
        }

        // If it's a separator, then add it.  If the separator has a
        // submenu, then the caller is smoking crash and needs their butt kicked.
        if ((miiSrc.fType & MFT_SEPARATOR) && EVAL(!miiSrc.hSubMenu))
        {
            // This is a separator; don't put two of them in a row
            if (bAlreadySeparated && miiSrc.wID == -1 && !(uFlags & MM_DONTREMOVESEPS))
            {
                continue;
            }

            bAlreadySeparated = TRUE;
        }
        else if (miiSrc.hSubMenu)
        {
            if (uFlags & MM_SUBMENUSHAVEIDS)
            {
                // Adjust the ID and check it
                miiSrc.wID += uIDAdjust;
                if (miiSrc.wID > uIDAdjustMax)
                {
                    continue;
                }

                if (uIDMax <= miiSrc.wID)
                {
                    uIDMax = miiSrc.wID + 1;
                }
            }
            else
            {
                // Don't set IDs for submenus that didn't have
                // them already
                miiSrc.fMask &= ~MIIM_ID;
            }

            hmSubMenu = miiSrc.hSubMenu;
            miiSrc.hSubMenu = CreatePopupMenu();
            if (!miiSrc.hSubMenu)
            {
                goto MM_Exit;
            }

            uTemp = Shell_MergeMenus(miiSrc.hSubMenu, hmSubMenu, 0, uIDAdjust,
                uIDAdjustMax, uFlags&MM_SUBMENUSHAVEIDS);
            if (uIDMax <= uTemp)
            {
                uIDMax = uTemp;
            }

            bAlreadySeparated = FALSE;
        }
        else
        {
            // Adjust the ID and check it
            miiSrc.wID += uIDAdjust;
            if (miiSrc.wID > uIDAdjustMax)
            {
                continue;
            }

            if (uIDMax <= miiSrc.wID)
            {
                uIDMax = miiSrc.wID + 1;
            }

            bAlreadySeparated = FALSE;
        }

        if (!EVAL(InsertMenuItem(hmDst, uInsert, TRUE, &miiSrc)))
        {
            goto MM_Exit;
        }
    }

    // Ensure the correct number of separators at the beginning of the
    // inserted menu items
    if (uInsert == 0)
    {
        if (bAlreadySeparated && !(uFlags & MM_DONTREMOVESEPS))
        {
            DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
        }
    }
    else
    {
        if (_SHIsMenuSeparator(hmDst, uInsert-1))
        {
            if (bAlreadySeparated && !(uFlags & MM_DONTREMOVESEPS))
            {
                DeleteMenu(hmDst, uInsert, MF_BYPOSITION);
            }
        }
        else
        {
            if ((uFlags & MM_ADDSEPARATOR) && !bAlreadySeparated)
            {
                // Add a separator between the menus
                InsertMenu(hmDst, uInsert, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            }
        }
    }

MM_Exit:
#ifdef DEBUG
    DBCheckMenu(hmDst);
#endif
    return(uIDMax);
}

#define REG_PREV_OS_VERSION  TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\PrevOsVersion")
#define REG_VAL_PLATFORM_ID  TEXT("PlatformId")
#define REG_VAL_MINORVERSION TEXT("MinorVersion")

//
// The following function is called, when we detect that IE4 was installed in this machine earlier.
// We want to see if that IE4 was there because of Win98 (if so, the ActiveDesktop is OFF by default)
//

BOOL    WasPrevOsWin98()
{
    HKEY    hk;
    DWORD   dwType;
    DWORD   dwPlatformId, dwMinorVersion;
    DWORD   dwDataLength;

    // 99/04/09 #319056 vtan: We'll assume that previous OS is known on
    // a Win9x upgrade from keys written by setup. If no keys are present
    // we'll assume NT4 upgrade where with IE4 integrated shell the default
    // was ON.
    BOOL fWin98 = FALSE;

    // See it the prev OS info is available. Caution: This info gets written in registry by 
    // NT setup at the far end of the setup process (after all our DLLs are already registered).
    // So, we use extra care here to see of that key and values really exist or not!
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_PREV_OS_VERSION, 0, KEY_READ, &hk) == ERROR_SUCCESS)
    {
        dwType = 0;
        dwDataLength = sizeof(dwPlatformId);
        if(RegQueryValueEx(hk, REG_VAL_PLATFORM_ID, NULL, &dwType, (LPBYTE)(&dwPlatformId), &dwDataLength) == ERROR_SUCCESS)
        {
            dwType = 0;
            dwDataLength = sizeof(dwMinorVersion);
            if(RegQueryValueEx(hk, REG_VAL_MINORVERSION, NULL, &dwType, (LPBYTE)(&dwMinorVersion), &dwDataLength) == ERROR_SUCCESS)
            {
                if((dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (dwMinorVersion > 0))
                    fWin98 = TRUE;   //This is Win98 for sure!
                else
                    fWin98 = FALSE;  //The prev OS is NOT win98 for sure!
            }
        }
        RegCloseKey(hk);
    }

    return fWin98;
}


void _SetIE4DefaultShellState(SHELLSTATE *pss)
{
    pss->fDoubleClickInWebView = TRUE;
    pss->fShowInfoTip = TRUE;
    pss->fWebView = TRUE;

#ifdef WINNT
    pss->fDesktopHTML = (g_bRunOnNT5 ? FALSE : !SHIsLowMemoryMachine(ILMM_IE4));
#else
    pss->fDesktopHTML = (g_bRunOnMemphis ? FALSE : !SHIsLowMemoryMachine(ILMM_IE4));
#endif

    // IE4 defaulted to fDesktopHTML on, and on NT5 upgrade we don't
    // want to override that (and probably not on Win98 upgrade, but
    // it's too late for that).  To determine this here, check a
    // uniquely IE4 reg-key.  (Note, this will catch the case if the
    // user *modified* their desktop.  If they just went with the
    // defaults, this key won't be there and we'll remove AD.)
    TCHAR   lpszDeskcomp[MAX_PATH];
    DWORD   dwType = 0, dwDeskHtmlVersion = 0;
    DWORD   dwDataLength = sizeof(dwDeskHtmlVersion);

    GetRegLocation(lpszDeskcomp, SIZECHARS(lpszDeskcomp), REG_DESKCOMP_COMPONENTS, NULL);

    SHGetValue(HKEY_CURRENT_USER, lpszDeskcomp, REG_VAL_COMP_VERSION, &dwType, (LPBYTE)(&dwDeskHtmlVersion), &dwDataLength);

    // 99/05/03 #292269: Notice the difference in the order of the
    // bits. The current structure (IE4 and later) has fShowSysFiles
    // between fNoConfirmRecycle and fShowCompColor. Move the bit
    // based on the size of the struct and reset fShowSysFiles to TRUE.

    // WIN95 SHELLSTATE struct bit fields
    //  BOOL fShowAllObjects : 1;
    //  BOOL fShowExtensions : 1;
    //  BOOL fNoConfirmRecycle : 1;
    //  BOOL fShowCompColor  : 1;
    //  UINT fRestFlags : 13;

    // IE4 SHELLSTATE struct bit fields
    //  BOOL fShowAllObjects : 1;
    //  BOOL fShowExtensions : 1;
    //  BOOL fNoConfirmRecycle : 1;
    //  BOOL fShowSysFiles : 1;
    //  BOOL fShowCompColor : 1;
    //  BOOL fDoubleClickInWebView : 1;
    //  BOOL fDesktopHTML : 1;
    //  BOOL fWin95Classic : 1;
    //  BOOL fDontPrettyPath : 1;
    //  BOOL fShowAttribCol : 1;
    //  BOOL fMapNetDrvBtn : 1;
    //  BOOL fShowInfoTip : 1;
    //  BOOL fHideIcons : 1;
    //  BOOL fWebView : 1;
    //  BOOL fFilter : 1;
    //  BOOL fShowSuperHidden : 1;

    if ((g_pShellState->cbSize == REGSHELLSTATE_SIZE_WIN95) || (g_pShellState->cbSize == REGSHELLSTATE_SIZE_NT4))
    {
        pss->fShowCompColor = BOOLIFY(pss->fShowSysFiles);
        pss->fShowSysFiles = TRUE;
    }
    if (dwDeskHtmlVersion == IE4_DESKHTML_VERSION)
        pss->fDesktopHTML = !WasPrevOsWin98();   //This is an upgrade from IE4; but, the registry is not updated yet.
    else
    {
        if (dwDeskHtmlVersion > IE4_DESKHTML_VERSION)
        {
            DWORD   dwOldHtmlVersion = 0;
            dwDataLength = sizeof(dwOldHtmlVersion);
            // This is NT5 or above! Check to see if we have "UpgradedFrom" value.
            // NOTE: The "UpgradedFrom" value is at "...\Desktop" and NOT at "..\Desktop\Components"
            // This is because the "Components" key gets destroyed very often.
            SHGetValue(HKEY_CURRENT_USER, REG_DESKCOMP, REG_VAL_COMP_UPGRADED_FROM, &dwType, (LPBYTE)&dwOldHtmlVersion, &dwDataLength);

            // 99/05/17 #333384 vtan: Check for IE5 as an old version too. The current version
            // is now 0x0110 (from 0x010F) and this causes the HKCU\Software\Microsoft\Internet
            // Explorer\Desktop\UpgradedFrom value to get created in CDeskHtmlProp_RegUnReg().
            // This is executed by IE4UINIT.EXE as well as REGSVR32.EXE with the "/U" parameter
            // so this field should be present at the time this executes. Note this only
            // executes the once on upgrade because the ShellState will get written.
                
            if ((dwOldHtmlVersion == IE4_DESKHTML_VERSION) || (dwOldHtmlVersion == IE5_DESKHTML_VERSION))
                pss->fDesktopHTML = !WasPrevOsWin98();   //This is an upgrade from IE4;
        }
    }
}


//
// This function checks if the caller is running in an explorer process.
//
STDAPI_(BOOL) IsProcessAnExplorer() 
{
    return BOOLFROMPTR(GetModuleHandle(TEXT("EXPLORER.EXE")));
}


//
// Is this the main shell process? (eg the one that owns the desktop window)
//
// NOTE: if the desktop window has not been created, we assume that this is NOT the
//       main shell process and return FALSE;
//
STDAPI_(BOOL) IsMainShellProcess()
{
    static int s_fIsMainShellProcess = -1;

    if (s_fIsMainShellProcess == -1)
    {
        HWND hwndDesktop = GetShellWindow();

        if (hwndDesktop) 
        {
            s_fIsMainShellProcess = (int)IsWindowInProcess(hwndDesktop);
    
            if ((s_fIsMainShellProcess != 0) && !IsProcessAnExplorer())
            {
                TraceMsg(TF_WARNING, "IsMainShellProcess: the main shell process (owner of the desktop) is NOT an explorer window?!?");
            }
        }
        else
        {
#ifdef FULL_DEBUG
            // only spew on FULL_DEBUG to cut down on chattyness in normal debug builds
            TraceMsg(TF_WARNING, "IsMainShellProcess: hwndDesktop does not exist, assuming we are NOT the main shell process");
#endif // FULL_DEBUG

            return FALSE;
        }
    }

    return s_fIsMainShellProcess ? TRUE : FALSE;
}


BOOL _RefreshSettingsFromReg()
{
    BOOL    fNeedToUpdateReg = FALSE;
    
    ASSERTCRITICAL;

    static REGSHELLSTATE ShellStateBuf = {0,};
    DWORD cbSize;

    if (NULL == g_pShellState)
    {
        g_pShellState = &ShellStateBuf;
        cbSize = sizeof(ShellStateBuf);
    }
    else
    {
        cbSize = g_pShellState->cbSize;
    }
    
    HKEY hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
    if (hkey)
    {
        LONG lres = SHQueryValueEx(hkey, TEXT("ShellState"), NULL, NULL, (LPBYTE)g_pShellState, &cbSize);

        if (ERROR_MORE_DATA == lres)
        {
            // since we default to the static buffer, what's in the registry must be bigger
            // than our current size...
            ASSERT(cbSize > sizeof(ShellStateBuf));

            // since we cache our allocation in the global, we should take this
            // code path only once...
            ASSERT(&ShellStateBuf == g_pShellState);

            g_pShellState = (REGSHELLSTATE*)LocalAlloc(LPTR, cbSize);
            if (g_pShellState)
            {
                lres = SHQueryValueEx(hkey, TEXT("ShellState"), NULL, NULL, (LPBYTE)g_pShellState, &cbSize);
                if (ERROR_SUCCESS != lres)
                {
                    LocalFree(g_pShellState);
                    // no need to reassign g_pShellState -- that will happen below
                }
            }
        }

        if (ERROR_SUCCESS != lres)
        {
            // the alloc/read may have failed?
            g_pShellState = &ShellStateBuf;
            g_pShellState->cbSize = 0;  // I don't trust SHQueryValueEx on failure...
        }
        RegCloseKey(hkey);
    }

    // Upgrade what we read out of the registry
    //
    if ((g_pShellState->cbSize == REGSHELLSTATE_SIZE_WIN95) ||
        (g_pShellState->cbSize == REGSHELLSTATE_SIZE_NT4))
    {
        // Upgrade Win95 bits.  Too bad our defaults weren't
        // FALSE for everything, because whacking these bits
        // breaks roaming.  Of course, if you ever roam to
        // a Win95 machine, it whacks all bits to zero...
        //
        _SetIE4DefaultShellState(&g_pShellState->ss);
        
        g_pShellState->ss.version = SHELLSTATEVERSION;
        g_pShellState->cbSize = SIZEOF(REGSHELLSTATE);

        fNeedToUpdateReg = TRUE;
    }
    else if (g_pShellState->cbSize >= REGSHELLSTATE_SIZE_IE4)
    {
        // Since the version field was new to IE4, this should be true:
        ASSERT(g_pShellState->ss.version >= SHELLSTATEVERSION_IE4);

        if(g_pShellState->ss.version < SHELLSTATEVERSION)
            fNeedToUpdateReg = TRUE; //Since the version # read from the registry is old!
            
        // Upgrade to current version here - make sure we don't
        // stomp on bits unnecessarily, as that will break roaming...
        //
        if (g_pShellState->ss.version == SHELLSTATEVERSION_IE4)
        {
            // IE4.0 shipped with verion = 9; The SHELLSTATEVERSION was changed to 10 later.
            // But the structure size or defaults have not changed since IE4.0 shipped. So,
            // the following code treats version 9 as the same as version 10. If we do not do this,
            // the IE4.0 users who upgrade to Memphis or IE4.01 will lose all their settings 
            // (Bug #62389).
            g_pShellState->ss.version = 10;
        }
        
        // Must be saved state from this or an uplevel platform.  Don't touch the bits.
        ASSERT(g_pShellState->ss.version >= SHELLSTATEVERSION);

        // Since this could be an upgrade from Win98, the fWebView bit, which was not used in win98
        // could be zero; We must set the default value of fWebView = ON here and later in 
        // _RefreshSettings() functions we read from Advanced\WebView value and reset it (if it is there).
        // If Advanced\WebView is not there, then this must be an upgrade from Win98 and WebView should be
        // turned ON.
        g_pShellState->ss.fWebView = TRUE;

        if (g_pShellState->cbSize >= REGSHELLSTATE_SIZE_WIN2K)
        {
            //g_pShellState->ss.fFilter = FALSE;
            //g_pShellState->ss.fShowSuperHidden = FALSE;
            //g_pShellState->ss.fSepProcess  = FALSE;
        }
    }
    else
    {
        //We could not read anything from reg. Initialize all fields.
        // 0 should be the default for *most* everything
        g_pShellState->cbSize = SIZEOF(REGSHELLSTATE);

        g_pShellState->ss.iSortDirection = 1;

        _SetIE4DefaultShellState(&g_pShellState->ss);
        
        g_pShellState->ss.version = SHELLSTATEVERSION;

        fNeedToUpdateReg = TRUE;
    }

    // Apply restrictions
    //Note: This restriction supercedes the NOACTIVEDESKTOP!
    if (SHRestricted(REST_FORCEACTIVEDESKTOPON))
    {
        g_pShellState->ss.fDesktopHTML = TRUE;
    }
    else
    {
        if (SHRestricted(REST_NOACTIVEDESKTOP))
        {
            //Note this restriction is superceded by FORCEACTIVEDESKTOPON!
            g_pShellState->ss.fDesktopHTML = FALSE;
        }
    }

    if (SHRestricted(REST_NOWEBVIEW))
    {
        g_pShellState->ss.fWebView = FALSE;
    }

    // ClassicShell restriction makes all web view off and forces even more win95 behaviours
    // so we still need to fDoubleClickInWebView off.
    if (SHRestricted(REST_CLASSICSHELL))
    {
        g_pShellState->ss.fWin95Classic = TRUE;
        g_pShellState->ss.fDoubleClickInWebView = FALSE;
        g_pShellState->ss.fWebView = FALSE;
        g_pShellState->ss.fDesktopHTML = FALSE;
    }

    if (SHRestricted(REST_DONTSHOWSUPERHIDDEN))
    {
        g_pShellState->ss.fShowSuperHidden = FALSE;
    }

    if (SHRestricted(REST_SEPARATEDESKTOPPROCESS))
    {
        g_pShellState->ss.fSepProcess = TRUE;
    }
    
    if(fNeedToUpdateReg)
    {
        // There is a need to update ShellState in registry. Do it only if current procees is
        // an Explorer process.
        //
        // Because, only when the explorer process is running, we can be
        // assured that the NT5 setup is complete and all the PrevOsVersion info is available and
        // _SetIE4DefaultShellState() and WasPrevOsWin98() etc, would have set the proper value
        // for fDesktopHHTML. If we don't do the following check, we will end-up updating the
        // registry the first time someone (like setup) called SHGetSettings() and that would be
        // too early to update the ShellState since we don't have all the info needed to decide if
        // fDesktopHTML needs to be ON or OFF based on previous OS, previous IE version etc.,
        fNeedToUpdateReg = IsProcessAnExplorer();
    }

    return (fNeedToUpdateReg);
}

EXTERN_C HANDLE g_hSettings = NULL;     //  global shell settings counter
LONG g_lProcessSettingsCount = -1;      //  current process's count
const GUID GUID_ShellSettingsChanged = { 0x7cb834f0, 0x527b, 0x11d2, {0x9d, 0x1f, 0x00, 0x00, 0xf8, 0x05, 0xca, 0x57}}; // 7cb834f0-527b-11d2-9d1f-0000f805ca57

HANDLE _GetSettingsCounter()
{
    return SHGetCachedGlobalCounter(&g_hSettings, &GUID_ShellSettingsChanged);
}
  
BOOL _QuerySettingsChanged(void)
{
    long lGlobalCount = SHGlobalCounterGetValue(_GetSettingsCounter());
    if (g_lProcessSettingsCount != lGlobalCount)
    {
        g_lProcessSettingsCount = lGlobalCount;
        return TRUE;
    }
    return FALSE;
}

//
//  SHRefreshSettings now just invalidates the settings cache.
//  so that the next time that SHGetSetSettings() is called
//  it will reread all the settings
//
STDAPI_(void) SHRefreshSettings(void)
{
    SHGlobalCounterIncrement(_GetSettingsCounter());
}

// this needs to get called periodically to re-fetch the settings from the
// registry as we no longer store them in a share data segment
BOOL _RefreshSettings(void)
{
    BOOL    fNeedToUpdateReg = FALSE;
    
    ENTERCRITICAL;

    fNeedToUpdateReg = _RefreshSettingsFromReg();

    // get the advanced options.
    // they are stored as individual values so that policy editor can change it.
    HKEY hkeyAdv = SHGetExplorerSubHkey( HKEY_CURRENT_USER, TEXT("Advanced"), FALSE);
    if (hkeyAdv)
    {
        DWORD dwData;
        DWORD dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("Hidden"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            // Map the obsolete value of 0 to 2.
            if (dwData == 0)
                dwData = 2;
            g_pShellState->ss.fShowAllObjects = (dwData == 1);
            g_pShellState->ss.fShowSysFiles = (dwData == 2);                
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("ShowCompColor"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowCompColor = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("HideFileExt"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowExtensions = (BOOL)dwData ? FALSE : TRUE;
        }

        dwSize = sizeof(dwData);
        
        if (SHQueryValueEx(hkeyAdv, TEXT("DontPrettyPath"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fDontPrettyPath = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("ShowInfoTip"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowInfoTip = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("HideIcons"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fHideIcons = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (SHQueryValueEx(hkeyAdv, TEXT("MapNetDrvBtn"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fMapNetDrvBtn = (BOOL)dwData;
        }

        dwSize = sizeof(dwData);

        if (!SHRestricted(REST_CLASSICSHELL))
        {
            if (SHQueryValueEx(hkeyAdv, TEXT("WebView"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
            {
                g_pShellState->ss.fWebView = (BOOL)dwData;
            }
            else
            {
                //If Advanced/WebView value is not there, then this could be an upgrade from win98/IE4 
                // where we stored this info in DEFFOLDERSETTINGS in Explorer\Streams\Settings.
                // See if that info is there; If so, use it!
                DEFFOLDERSETTINGS dfs;
                DWORD dwType, cbData = SIZEOF(dfs);
                HKEY    hkeyExp = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
                if (hkeyExp)
                {
                    if((ERROR_SUCCESS == SHGetValue(hkeyExp, TEXT("Streams"), TEXT("Settings"), &dwType, &dfs, &cbData)) &&
                       (dwType == REG_BINARY))
                    {
                        //DefFolderSettings is there; Check if this is the correct struct.
                        //Note:In Win98/IE4, we wrongly initialized dwStructVersion to zero.
                        if ((cbData == SIZEOF(dfs)) && 
                            ((dfs.dwStructVersion == 0) || (dfs.dwStructVersion == DFS_NASH_VER)))
                        {
                            g_pShellState->ss.fWebView = ((dfs.bUseVID) && (dfs.vid == VID_WebView));
                        }   
                    }
                    RegCloseKey(hkeyExp);
                }
            }
        }
        
        dwSize = sizeof(dwData);
        
        if (SHQueryValueEx(hkeyAdv, TEXT("Filter"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fFilter = (BOOL)dwData;
        }

        if (SHQueryValueEx(hkeyAdv, TEXT("ShowSuperHidden"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fShowSuperHidden = (BOOL)dwData;
        }
        
        if (SHQueryValueEx(hkeyAdv, TEXT("SeparateProcess"), NULL, NULL, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            g_pShellState->ss.fSepProcess = (BOOL)dwData;
        }

        RegCloseKey(hkeyAdv);
    }
    else
    {
        // Hey, if the advanced key is not there, this must be a Win9x upgrade...
        // Fortunately the SHELLSTATE defaults and the not-in-registry defaults
        // are the same, so we don't need any auto-propogate code here.
    }

    //  this process is now in sync
    g_lProcessSettingsCount = SHGlobalCounterGetValue(_GetSettingsCounter());
    
    LEAVECRITICAL;

    return fNeedToUpdateReg;
}

// This function moves SHELLSTATE settings into the Advanced Settings
// portion of the registry.  If no SHELLSTATE is passed in, it uses
// the current state stored in the registry.
//
void Install_AdvancedShellSettings(SHELLSTATE * pss)
{
    HKEY hkeyAdv;
    DWORD dw;
    BOOL fCrit = FALSE;

    if (NULL == pss)
    {
        // Get the current values in the registry or the default values
        // as determined by the following function.
        //
        // we'll be partying on g_pShellState, so grab the critical section here
        //
        ENTERCRITICALNOASSERT;
        fCrit = TRUE;

        // Win95 and NT5 kept the SHELLSTATE bits in the registry up to date,
        // but apparently IE4 only kept the ADVANCED section up to date.
        // It would be nice to call _RefreshSettingsFromReg(); here, but
        // that won't keep IE4 happy.  Call _RefreshSettings() instead.
        _RefreshSettings();

        pss = &g_pShellState->ss;
    }

    hkeyAdv = SHGetExplorerSubHkey( HKEY_CURRENT_USER, TEXT("Advanced"), TRUE);
    if (hkeyAdv) 
    {
        DWORD dwData;

        dw = sizeof(dwData);
        dwData = (DWORD)(pss->fShowAllObjects ? 1 : 2);
        RegSetValueEx(hkeyAdv, TEXT("Hidden") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowCompColor ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("ShowCompColor") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowExtensions ? 0 : 1;
        RegSetValueEx(hkeyAdv, TEXT("HideFileExt") ,0, REG_DWORD, (LPBYTE)&dwData, dw);
    
        dwData = (DWORD)pss->fDontPrettyPath ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("DontPrettyPath") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowInfoTip ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("ShowInfoTip") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fHideIcons ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("HideIcons") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fMapNetDrvBtn ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("MapNetDrvBtn") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fWebView ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("WebView") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fFilter ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("Filter") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fShowSuperHidden ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("SuperHidden") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        dwData = (DWORD)pss->fSepProcess ? 1 : 0;
        RegSetValueEx(hkeyAdv, TEXT("SeparateProcess") ,0, REG_DWORD, (LPBYTE)&dwData, dw);

        RegCloseKey(hkeyAdv);            
    }

    if (fCrit)
    {
        LEAVECRITICALNOASSERT;
    }
}

STDAPI_(void) SHGetSetSettings(LPSHELLSTATE lpss, DWORD dwMask, BOOL bSet)
{
    //Does the ShellState in Reg is old? If so, we need to update it!
    BOOL    fUpdateShellStateInReg = FALSE;  //Assume, no need to update it!
    
    if (!lpss && !dwMask && bSet)
    {
        // this was a special way to call 
        // SHRefreshSettings() from an external module.
        // special case it out now.
        SHRefreshSettings();
        return;
    }

    if (!g_pShellState || _QuerySettingsChanged())
    {
        // if it hasn't been init'd or we are setting the values. We must do it
        // on the save case because it may have changed in the registry since
        // we last fetched it..
        fUpdateShellStateInReg = _RefreshSettings();
    }
    else if (g_pShellState)
    {
        // _RefreshSettingsFromReg sets g_pShellState to non null value
        // and then starts stuffing values into it and all within our
        // glorious critsec, but unless we check for the critsec here,
        // we will start partying on g_pShellState before it is finished
        // loading.
        ENTERCRITICAL;
        LEAVECRITICAL;
    }

    BOOL fSave = FALSE;
    BOOL fSaveAdvanced = FALSE;
    
    if (bSet)
    {
        if ((dwMask & SSF_SHOWALLOBJECTS) && (g_pShellState->ss.fShowAllObjects != lpss->fShowAllObjects))
        {
            g_pShellState->ss.fShowAllObjects = lpss->fShowAllObjects;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWSYSFILES) && (g_pShellState->ss.fShowSysFiles != lpss->fShowSysFiles))
        {
            g_pShellState->ss.fShowSysFiles = lpss->fShowSysFiles;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWEXTENSIONS) && (g_pShellState->ss.fShowExtensions != lpss->fShowExtensions))
        {
            g_pShellState->ss.fShowExtensions = lpss->fShowExtensions;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWCOMPCOLOR) && (g_pShellState->ss.fShowCompColor != lpss->fShowCompColor))
        {
            g_pShellState->ss.fShowCompColor = lpss->fShowCompColor;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_NOCONFIRMRECYCLE) && (g_pShellState->ss.fNoConfirmRecycle != lpss->fNoConfirmRecycle))
        {
            g_pShellState->ss.fNoConfirmRecycle = lpss->fNoConfirmRecycle;
            fSave = TRUE;
        }

        if ((dwMask & SSF_DOUBLECLICKINWEBVIEW) && (g_pShellState->ss.fDoubleClickInWebView != lpss->fDoubleClickInWebView))
        {
            if (!SHRestricted(REST_CLASSICSHELL))
            {
                g_pShellState->ss.fDoubleClickInWebView = lpss->fDoubleClickInWebView;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_DESKTOPHTML) && (g_pShellState->ss.fDesktopHTML != lpss->fDesktopHTML))
        {
            if (!SHRestricted(REST_NOACTIVEDESKTOP) && !SHRestricted(REST_CLASSICSHELL) 
                                                    && !SHRestricted(REST_FORCEACTIVEDESKTOPON))
            {
                g_pShellState->ss.fDesktopHTML = lpss->fDesktopHTML;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_WIN95CLASSIC) && (g_pShellState->ss.fWin95Classic != lpss->fWin95Classic))
        {
            if (!SHRestricted(REST_CLASSICSHELL))
            {
                g_pShellState->ss.fWin95Classic = lpss->fWin95Classic;
                fSave = TRUE;
            }
        }

        if ((dwMask & SSF_WEBVIEW) && (g_pShellState->ss.fWebView != lpss->fWebView))
        {
            if (!SHRestricted(REST_NOWEBVIEW) && !SHRestricted(REST_CLASSICSHELL))
            {
                g_pShellState->ss.fWebView = lpss->fWebView;
                fSaveAdvanced = TRUE;
            }
        }

        if ((dwMask & SSF_DONTPRETTYPATH) && (g_pShellState->ss.fDontPrettyPath != lpss->fDontPrettyPath))
        {
            g_pShellState->ss.fDontPrettyPath = lpss->fDontPrettyPath;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWINFOTIP) && (g_pShellState->ss.fShowInfoTip != lpss->fShowInfoTip))
        {
            g_pShellState->ss.fShowInfoTip = lpss->fShowInfoTip;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_HIDEICONS) && (g_pShellState->ss.fHideIcons != lpss->fHideIcons))
        {
            g_pShellState->ss.fHideIcons = lpss->fHideIcons;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_MAPNETDRVBUTTON) && (g_pShellState->ss.fMapNetDrvBtn != lpss->fMapNetDrvBtn))
        {
            g_pShellState->ss.fMapNetDrvBtn = lpss->fMapNetDrvBtn;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SORTCOLUMNS) &&
            ((g_pShellState->ss.lParamSort != lpss->lParamSort) || (g_pShellState->ss.iSortDirection != lpss->iSortDirection)))
        {
            g_pShellState->ss.iSortDirection = lpss->iSortDirection;
            g_pShellState->ss.lParamSort = lpss->lParamSort;
            fSave = TRUE;
        }

        if (dwMask & SSF_HIDDENFILEEXTS)
        {
            // Setting hidden extensions is not supported
        }

        if ((dwMask & SSF_FILTER) && (g_pShellState->ss.fFilter != lpss->fFilter))
        {
            g_pShellState->ss.fFilter = lpss->fFilter;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SHOWSUPERHIDDEN) && (g_pShellState->ss.fShowSuperHidden != lpss->fShowSuperHidden))
        {
            g_pShellState->ss.fShowSuperHidden = lpss->fShowSuperHidden;
            fSaveAdvanced = TRUE;
        }

        if ((dwMask & SSF_SEPPROCESS) && (g_pShellState->ss.fSepProcess != lpss->fSepProcess))
        {
            g_pShellState->ss.fSepProcess = lpss->fSepProcess;
            fSaveAdvanced = TRUE;
        }

    }

    if(fUpdateShellStateInReg || fSave || fSaveAdvanced)
    {
        // Write out the SHELLSTATE even if only fSaveAdvanced just to
        // make sure everything stays in sync.
        // We save 8 extra bytes for the ExcludeFileExts stuff.
        // Oh well.
        HKEY hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        if (hkey)
        {
            RegSetValueEx(hkey, TEXT("ShellState"), 0L, REG_BINARY,
                          (LPBYTE)g_pShellState, g_pShellState->cbSize);

            RegCloseKey(hkey);
        }
    }

    if (fUpdateShellStateInReg || fSaveAdvanced)
    {
        // SHRefreshSettingsPriv overwrites the SHELLSTATE values with whatever
        // the user specifies in View.FolderOptions.View.AdvancedSettings dialog.
        // These values are stored elsewhere in the registry, so we
        // better migrate SHELLSTATE to that part of the registry now.
        //
        // Might as well only do this if the registry settings we care about change.
        Install_AdvancedShellSettings(&g_pShellState->ss);
    }

    if (fSave || fSaveAdvanced)
    {
        // Let apps know the state has changed.
        SHRefreshSettings();
        SHSendMessageBroadcast(WM_SETTINGCHANGE, 0, (LPARAM)TEXT("ShellState"));
    }

    if(!bSet)
    {
        if (dwMask & SSF_SHOWALLOBJECTS)
        {
            lpss->fShowAllObjects = g_pShellState->ss.fShowAllObjects;  // users "show hidden" setting
        }

        if (dwMask & SSF_SHOWSYSFILES)
        {
            lpss->fShowSysFiles = g_pShellState->ss.fShowSysFiles;  // this is ignored
        }

        if (dwMask & SSF_SHOWEXTENSIONS)
        {
            lpss->fShowExtensions = g_pShellState->ss.fShowExtensions;
        }

        if (dwMask & SSF_SHOWCOMPCOLOR)
        {
            lpss->fShowCompColor = g_pShellState->ss.fShowCompColor;
        }

        if (dwMask & SSF_NOCONFIRMRECYCLE)
        {
            lpss->fNoConfirmRecycle = g_pShellState->ss.fNoConfirmRecycle;
        }

        if (dwMask & SSF_DOUBLECLICKINWEBVIEW)
        {
            lpss->fDoubleClickInWebView = g_pShellState->ss.fDoubleClickInWebView;
        }

        if (dwMask & SSF_DESKTOPHTML)
        {
            lpss->fDesktopHTML = g_pShellState->ss.fDesktopHTML;
        }

        if (dwMask & SSF_WIN95CLASSIC)
        {
            lpss->fWin95Classic = g_pShellState->ss.fWin95Classic;
        }
        if (dwMask & SSF_WEBVIEW)
        {
            lpss->fWebView = g_pShellState->ss.fWebView;
        }

        if (dwMask & SSF_DONTPRETTYPATH)
        {
            lpss->fDontPrettyPath = g_pShellState->ss.fDontPrettyPath;
        }

        if (dwMask & SSF_SHOWINFOTIP)
        {
            lpss->fShowInfoTip = g_pShellState->ss.fShowInfoTip;
        }

        if (dwMask & SSF_HIDEICONS)
        {
            lpss->fHideIcons = g_pShellState->ss.fHideIcons;
        }

        if (dwMask & SSF_MAPNETDRVBUTTON)
        {
            lpss->fMapNetDrvBtn = g_pShellState->ss.fMapNetDrvBtn;
        }

        if (dwMask & SSF_HIDDENFILEEXTS)
        {
            *lpss->pszHiddenFileExts = 0;   // we don't hide files by ext anymore
        }

        if (dwMask & SSF_SORTCOLUMNS)
        {
            lpss->iSortDirection = g_pShellState->ss.iSortDirection;
            lpss->lParamSort = g_pShellState->ss.lParamSort;
        }

        if (dwMask & SSF_FILTER)
        {
            lpss->fFilter = g_pShellState->ss.fFilter;
        }

        if (dwMask & SSF_SHOWSUPERHIDDEN)
        {
            lpss->fShowSuperHidden = g_pShellState->ss.fShowSuperHidden;
        }

        if (dwMask & SSF_SEPPROCESS)
        {
            lpss->fSepProcess = g_pShellState->ss.fSepProcess;
        }
    }
}

// A public version of the Get function so ISVs can track the shell flag state
//
STDAPI_(void) SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask)
{
    SHELLSTATE ss={0};

    // SSF_HIDDENFILEEXTS and SSF_SORTCOLUMNS don't work with
    // the SHELLFLAGSTATE struct, make sure they are off
    // (because the corresponding SHELLSTATE fields don't
    // exist in SHELLFLAGSTATE.)
    //
    dwMask &= ~(SSF_HIDDENFILEEXTS | SSF_SORTCOLUMNS);

    SHGetSetSettings(&ss, dwMask, FALSE);

    // copy the dword of flags out
    *((DWORD *)lpsfs) = *((DWORD *)(&ss));
}

//----------------------------------------------------------------------------
// app compatibility HACK stuff. the following stuff including CheckWinIniForAssocs()
// is used by the new version of SHDOCVW
// and EXPLORER.EXE to patch the registry for old win31 apps.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL _PathIsExe(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    lstrcpy(szPath, pszPath);
    PathRemoveBlanks(szPath);

    return PathIsExe(szPath);
}

// tests to see if pszSubFolder is the same as or a sub folder of pszParent
// in:
//      pszFolder       parent folder to test against
//                      this may be a CSIDL value if the HIWORD() is 0
//      pszSubFolder    possible sub folder
//
// example: 
//      TRUE    pszFolder = c:\windows, pszSubFolder = c:\windows\system
//      TRUE    pszFolder = c:\windows, pszSubFolder = c:\windows
//      FALSE   pszFolder = c:\windows, pszSubFolder = c:\winnt
//

STDAPI_(BOOL) PathIsEqualOrSubFolder(LPCTSTR pszFolder, LPCTSTR pszSubFolder)
{
    TCHAR szParent[MAX_PATH], szCommon[MAX_PATH];

    if (!IS_INTRESOURCE(pszFolder))
        lstrcpyn(szParent, pszFolder, ARRAYSIZE(szParent));
    else
        SHGetFolderPath(NULL, PtrToUlong((void *) pszFolder) | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szParent);

    //  PathCommonPrefix() always removes the slash on common
    return szParent[0] && PathRemoveBackslash(szParent)
        && PathCommonPrefix(szParent, pszSubFolder, szCommon) 
        && lstrcmpi(szParent, szCommon) == 0;
}

// pass an array of CSIDL values (-1 terminated)

STDAPI_(BOOL) PathIsEqualOrSubFolderOf(const UINT rgFolders[], LPCTSTR pszSubFolder)
{
    int i;

    for (i = 0; rgFolders[i] != -1; i++)
    {
        if (PathIsEqualOrSubFolder(MAKEINTRESOURCE(rgFolders[i]), pszSubFolder))
            return TRUE;
    }
    return FALSE;
}


// many net providers (Vines and PCNFS) don't 
// like "C:\" frormat volumes names, this code returns "C:" format
// for use with WNet calls

STDAPI_(LPTSTR) PathBuildSimpleRoot(int iDrive, LPTSTR pszDrive)
{
    pszDrive[0] = iDrive + TEXT('A');
    pszDrive[1] = TEXT(':');
    pszDrive[2] = 0;
    return pszDrive;
}

//----------------------------------------------------------------------------
// Return TRUE for exe, com, bat, pif and lnk.
BOOL ReservedExtension(LPCTSTR pszExt)
{
    TCHAR szExt[5];  // Dot+ext+null.

    lstrcpyn(szExt, pszExt, ARRAYSIZE(szExt));
    PathRemoveBlanks(szExt);
    if (PathIsExe(szExt) || (lstrcmpi(szExt, TEXT(".lnk")) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

//----------------------------------------------------------------------------
TCHAR const c_szRegPathIniExtensions[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Extensions");
extern TCHAR const c_szShellOpenCommand[];


STDAPI_(LONG) RegSetString(HKEY hk, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    return RegSetValue(hk, pszSubKey, REG_SZ, pszValue, (lstrlen(pszValue) + 1) * sizeof(TCHAR));
}

//----------------------------------------------------------------------------
STDAPI_(BOOL) RegSetValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPCTSTR psz)
{
    return (NOERROR == SHSetValue(hkey, pszSubKey, pszValue, REG_SZ, psz, CbFromCch(lstrlen(psz) +1)));
}

//----------------------------------------------------------------------------
STDAPI_(BOOL) RegGetValueString(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR psz, DWORD cb)
{
    return (!GetSystemMetrics(SM_CLEANBOOT)
    &&  (NOERROR == SHGetValue(hkey, pszSubKey, pszValue, NULL, psz, &cb)));
}

//----------------------------------------------------------------------------
// Returns TRUE if there is a proper shell\open\command for the given
// extension that matches the given command line.
// NB This is for MGX Designer which registers an extension and commands for
// printing but relies on win.ini extensions for Open. We need to detect that
// there's no open command and add the appropriate entries to the registry.
// If the given extension maps to a type name we return that in pszTypeName
// otherwise it will be null.
// FMH This also affects Asymetric Compel which makes a new .CPL association.
// We need to merge it up into our Control Panel .CPL association.  We depend
// on Control Panels NOT having a proper Open so users can see both verb sets.
// NB pszLine is the original line from win.ini eg foo.exe /bar ^.fred, see
// comments below...
BOOL Reg_ShellOpenForExtension(LPCTSTR pszExt, LPTSTR pszCmdLine,
    LPTSTR pszTypeName, int cchTypeName, LPCTSTR pszLine)
{
    TCHAR sz[MAX_PATH];
    TCHAR szExt[MAX_PATH];
    LONG cb;

    if (pszTypeName)
        pszTypeName[0] = TEXT('\0');

    // Is the extension registed at all?
    cb = SIZEOF(sz);
    sz[0] = TEXT('\0');
    if (SHRegQueryValue(HKEY_CLASSES_ROOT, pszExt, sz, &cb) == ERROR_SUCCESS)
    {
        // Is there a file type?
        if (*sz)
        {
            // Yep, check there.
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension has a file type name %s.", sz);
            lstrcpy(szExt, sz);
            if (pszTypeName)
                lstrcpyn(pszTypeName, sz, cchTypeName);
        }
        else
        {
            // No, check old style associations.
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension has no file type name.", pszExt);
            lstrcpy(szExt, pszExt);
        }

        // See if there's an open command.
        lstrcat(szExt, TEXT("\\"));
        lstrcat(szExt, c_szShellOpenCommand);
        cb = SIZEOF(sz);
        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szExt, sz, &cb) == ERROR_SUCCESS)
        {
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension %s already registed with an open command.", pszExt);
            // NB We want to compare the paths only, not the %1 stuff.
            if (PathIsRelative(pszCmdLine))
            {
                int cch;
                // If a relative path was passed in, we may have a fully qualifed
                // one that is now in the registry... In that case we should
                // say that it matches...
                LPTSTR pszT = PathGetArgs(sz);

                if (pszT)
                {
                    *(pszT-1) = TEXT('\0');
                }

                PathUnquoteSpaces(sz);

                PathRemoveBlanks(pszCmdLine);

                cch = lstrlen(sz) - lstrlen(pszCmdLine);

                if ((cch >= 0) && (lstrcmpi(sz+cch, pszCmdLine) == 0))
                {
                    // DebugMsg(DM_TRACE, "c.r_rofe: Open commands match.");
                    return TRUE;
                }

                lstrcat(pszCmdLine, TEXT(" "));    // Append blank back on...
            }
            else
            {
                // If absolute path we can cheat for matches
                *(sz+lstrlen(pszCmdLine)) = TEXT('\0');
                if (lstrcmpi(sz, pszCmdLine) == 0)
                {
                    // DebugMsg(DM_TRACE, "c.r_rofe: Open commands match.");
                    return TRUE;
                }
            }

            // DebugMsg(DM_TRACE, "c.r_rofe: Open commands don't match.");

            // Open commands don't match, check to see if it's because the ini
            // changed (return FALSE so the change is reflected in the registry) or
            // if the registry changed (return TRUE so we keep the registry the way
            // it is.
            if (RegGetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, pszExt, sz, SIZEOF(sz)))
            {
                if (lstrcmpi(sz, pszLine) == 0)
                    return TRUE;
            }

            return FALSE;
        }
        else
        {
            // DebugMsg(DM_TRACE, "c.r_rofe: Extension %s already registed but with no open command.", pszExt);
            return FALSE;
        }
    }

    // DebugMsg(DM_TRACE, "c.r_rofe: No open command for %s.", pszExt);

    return FALSE;
}

//----------------------------------------------------------------------------
// This function will read in the extensions section of win.ini to see if
// there are any old style associations that we have not accounted for.
// NB Some apps screw up if their extensions magically disappear from the
// extensions section so DON'T DELETE the old entries from win.ini.
#define CWIFA_SIZE  4096

STDAPI_(void) CheckWinIniForAssocs(void)
{

    LPTSTR pszBuf;
    int cbRet;
    LPTSTR pszLine;
    TCHAR szExtension[MAX_PATH];
    TCHAR szTypeName[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH];
    LPTSTR pszExt;
    LPTSTR pszT;
    BOOL fAssocsMade = FALSE;

    szExtension[0]=TEXT('.');
    szExtension[1]=TEXT('\0');

// BUGBUG - BobDay - This code doesn't handle larger section
    pszBuf = (LPTSTR)LocalAlloc(LPTR, CWIFA_SIZE*SIZEOF(TCHAR));
    if (!pszBuf)
        return; // Could not allocate the memory
    cbRet = (int)GetProfileSection(TEXT("Extensions"), pszBuf, CWIFA_SIZE);

    //
    // We now walk through the list to find any items that is not
    // in the registry.
    //
    for (pszLine = pszBuf; *pszLine; pszLine += lstrlen(pszLine)+1)
    {
    // Get the extension for this file into a buffer.
    pszExt = StrChr(pszLine, TEXT('='));
    if (pszExt == NULL)
        continue;   // skip this line

    szExtension[0]=TEXT('.');
    // lstrcpyn will put the null terminator for us.
    // We should now have something like .xls in szExtension.
    lstrcpyn(szExtension+1, pszLine, (int)(pszExt-pszLine)+1);

    // Ignore extensions bigger than dot + 3 chars.
    if (lstrlen(szExtension) > 4)
    {
        DebugMsg(DM_ERROR, TEXT("CheckWinIniForAssocs: Invalid extension, skipped."));
        continue;
    }

    pszLine = pszExt+1;     // Points to after the =;
    while (*pszLine == TEXT(' '))
        pszLine++;  // skip blanks

    // Now find the ^ in the command line.
    pszExt = StrChr(pszLine, TEXT('^'));
    if (pszExt == NULL)
        continue;       // dont process

    // Now setup  the command line
    // WARNING: This assumes only 1 ^ and it assumes the extension...
    lstrcpyn(szCmdLine, pszLine, (int)(pszExt-pszLine)+1);

    // Don't bother moving over invalid entries (like the busted .hlp
    // entry VB 3.0 creates).
    if (!_PathIsExe(szCmdLine))
    {
        DebugMsg(DM_ERROR, TEXT("c.cwia: Invalid app, skipped."));
        continue;
    }

    if (ReservedExtension(szExtension))
    {
        DebugMsg(DM_ERROR, TEXT("c.cwia: Invalid extension (%s), skipped."), szExtension);
        continue;
    }

    // Now see if there is already a mapping for this extension.
    if (Reg_ShellOpenForExtension(szExtension, szCmdLine, szTypeName, ARRAYSIZE(szTypeName), pszLine))
    {
        // Yep, Setup the initial list of ini extensions in the registry if they are
        // not there already.
        if (!RegGetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, szExtension, szTypeName, SIZEOF(szTypeName)))
        {
            RegSetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, szExtension, pszLine);
        }
        continue;
    }

    // No mapping.

    // HACK for Expert Home Design. They put an association in win.ini
    // (which we propagate as typeless) but then register a type and a
    // print command the first time they run - stomping on our propagated
    // Open command. The fix is to put their open command under the proper
    // type instead of leaving it typeless.
    if (lstrcmpi(szExtension, TEXT(".dgw")) == 0)
    {
        if (lstrcmpi(PathFindFileName(szCmdLine), TEXT("designw.exe ")) == 0)
        {
            // Put in a ProgID for them.
            RegSetValue(HKEY_CLASSES_ROOT, szExtension, REG_SZ, TEXT("HDesign"), 0L);
            // Force Open command under their ProgID.
            TraceMsg(DM_TRACE, "c.cwifa: Expert Home Design special case hit.");
            lstrcpy(szTypeName, TEXT("HDesign"));
        }
    }

    //
    // HACK for Windows OrgChart which does not register OLE1 class
    // if ".WOC" is registered in the registry.
    //
    if (lstrcmpi(szExtension, TEXT(".WOC")) == 0)
    {
        if (lstrcmpi(PathFindFileName(szCmdLine), TEXT("WINORG.EXE ")) == 0)
        {
            DebugMsg(DM_ERROR, TEXT("c.cwia: HACK: Found WINORG (%s, %s), skipped."), szExtension, pszLine);
            continue;
        }
    }

    // Record that we're about to move things over in the registry so we won't keep
    // doing it all the time.
    RegSetValueString(HKEY_LOCAL_MACHINE, c_szRegPathIniExtensions, szExtension, pszLine);

    lstrcat(szCmdLine, TEXT("%1"));

    // see if there are anything else to copy out...
    pszExt++;    // get beyond the ^
    pszT = szExtension;
    while (*pszExt && (CharLower((LPTSTR)(DWORD)*pszExt) == CharLower((LPTSTR)(DWORD)*pszT)))
    {
        // Look for the next character...
        pszExt++;
        pszT++;
    }
    if (*pszExt)
        lstrcat(szCmdLine, pszExt); // add the rest onto the command line

    // Now lets make the actual association.
    // We need to add on the right stuff onto the key...
    if (*szTypeName)
        lstrcpy(szExtension, szTypeName);

        lstrcat(szExtension, TEXT("\\"));
        lstrcat(szExtension, c_szShellOpenCommand);
        RegSetValue(HKEY_CLASSES_ROOT, szExtension, REG_SZ, szCmdLine, 0L);
        // DebugMsg(DM_TRACE, "c.cwifa: %s %s", szExtension, szCmdLine);

        fAssocsMade = TRUE;
    }

    // If we made any associations we should let the cabinet know.
    //
    // Now call off to the notify function.
    if (fAssocsMade)
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    // And cleanup our allocation
    LocalFree((HLOCAL)pszBuf);
}

typedef struct
{
    INT     iFlag;
    LPCTSTR pszKey;
    LPCTSTR pszValue;
} RESTRICTIONITEMS;

#define SZ_RESTRICTED_ACTIVEDESKTOP             L"ActiveDesktop"
#define REGSTR_VAL_RESTRICTRUNW                 L"RestrictRun"
#define REGSTR_VAL_PRINTERS_HIDETABSW           L"NoPrinterTabs"
#define REGSTR_VAL_PRINTERS_NODELETEW           L"NoDeletePrinter"
#define REGSTR_VAL_PRINTERS_NOADDW              L"NoAddPrinter"

const SHRESTRICTIONITEMS c_rgRestrictionItems[] =
{
    {REST_NORUN,                   L"Explorer", L"NoRun"},
    {REST_NOCLOSE,                 L"Explorer", L"NoClose"},
    {REST_NOSAVESET ,              L"Explorer", L"NoSaveSettings"},
    {REST_NOFILEMENU,              L"Explorer", L"NoFileMenu"},
    {REST_NOSETFOLDERS,            L"Explorer", L"NoSetFolders"},
    {REST_NOSETTASKBAR,            L"Explorer", L"NoSetTaskbar"},
    {REST_NODESKTOP,               L"Explorer", L"NoDesktop"},
    {REST_NOFIND,                  L"Explorer", L"NoFind"},
    {REST_NODRIVES,                L"Explorer", L"NoDrives"},
    {REST_NODRIVEAUTORUN,          L"Explorer", L"NoDriveAutoRun"},
    {REST_NODRIVETYPEAUTORUN,      L"Explorer", L"NoDriveTypeAutoRun"},
    {REST_NONETHOOD,               L"Explorer", L"NoNetHood"},
    {REST_STARTBANNER,             L"Explorer", L"NoStartBanner"},
    {REST_RESTRICTRUN,             L"Explorer", REGSTR_VAL_RESTRICTRUNW},
    {REST_NOPRINTERTABS,           L"Explorer", REGSTR_VAL_PRINTERS_HIDETABSW},
    {REST_NOPRINTERDELETE,         L"Explorer", REGSTR_VAL_PRINTERS_NODELETEW},
    {REST_NOPRINTERADD,            L"Explorer", REGSTR_VAL_PRINTERS_NOADDW},
    {REST_NOSTARTMENUSUBFOLDERS,   L"Explorer", L"NoStartMenuSubFolders"},
    {REST_MYDOCSONNET,             L"Explorer", L"MyDocsOnNet"},
    {REST_NOEXITTODOS,             L"WinOldApp", L"NoRealMode"},
    {REST_ENFORCESHELLEXTSECURITY, L"Explorer", L"EnforceShellExtensionSecurity"},
    {REST_NOCOMMONGROUPS,          L"Explorer", L"NoCommonGroups"},
    {REST_LINKRESOLVEIGNORELINKINFO,L"Explorer", L"LinkResolveIgnoreLinkInfo"},
    {REST_NOWEB,                   L"Explorer", L"NoWebMenu"},
    {REST_NOTRAYCONTEXTMENU,       L"Explorer", L"NoTrayContextMenu"},
    {REST_NOVIEWCONTEXTMENU,       L"Explorer", L"NoViewContextMenu"},
    {REST_NONETCONNECTDISCONNECT,  L"Explorer", L"NoNetConnectDisconnect"},
    {REST_STARTMENULOGOFF,         L"Explorer", L"StartMenuLogoff"},
    {REST_NOSETTINGSASSIST,        L"Explorer", L"NoSettingsWizards"},

#ifdef WINNT // hydra specific restrictions
    {REST_NODISCONNECT,           L"Explorer", L"NoDisconnect"},
    {REST_NOSECURITY,             L"Explorer", L"NoNTSecurity"  },
    {REST_NOFILEASSOCIATE,        L"Explorer", L"NoFileAssociate"  },
#endif

    // New for IE4
    {REST_NOINTERNETICON,          L"Explorer", L"NoInternetIcon"},
    {REST_NORECENTDOCSHISTORY,     L"Explorer", L"NoRecentDocsHistory"},
    {REST_NORECENTDOCSMENU,        L"Explorer", L"NoRecentDocsMenu"},
    {REST_NOACTIVEDESKTOP,         L"Explorer", L"NoActiveDesktop"},
    {REST_NOACTIVEDESKTOPCHANGES,  L"Explorer", L"NoActiveDesktopChanges"},
    {REST_NOFAVORITESMENU,         L"Explorer", L"NoFavoritesMenu"},
    {REST_CLEARRECENTDOCSONEXIT,   L"Explorer", L"ClearRecentDocsOnExit"},
    {REST_CLASSICSHELL,            L"Explorer", L"ClassicShell"},
    {REST_NOCUSTOMIZEWEBVIEW,      L"Explorer", L"NoCustomizeWebView"},
    {REST_NOHTMLWALLPAPER,         SZ_RESTRICTED_ACTIVEDESKTOP, L"NoHTMLWallPaper"},
    {REST_NOCHANGINGWALLPAPER,     SZ_RESTRICTED_ACTIVEDESKTOP, L"NoChangingWallPaper"},
    {REST_NODESKCOMP,              SZ_RESTRICTED_ACTIVEDESKTOP, L"NoComponents"},
    {REST_NOADDDESKCOMP,           SZ_RESTRICTED_ACTIVEDESKTOP, L"NoAddingComponents"},
    {REST_NODELDESKCOMP,           SZ_RESTRICTED_ACTIVEDESKTOP, L"NoDeletingComponents"},
    {REST_NOCLOSEDESKCOMP,         SZ_RESTRICTED_ACTIVEDESKTOP, L"NoClosingComponents"},
    {REST_NOCLOSE_DRAGDROPBAND,    L"Explorer", L"NoCloseDragDropBands"},
    {REST_NOMOVINGBAND,            L"Explorer", L"NoMovingBands"},
    {REST_NOEDITDESKCOMP,          SZ_RESTRICTED_ACTIVEDESKTOP, L"NoEditingComponents"},
    {REST_NORESOLVESEARCH,         L"Explorer", L"NoResolveSearch"},
    {REST_NORESOLVETRACK,          L"Explorer", L"NoResolveTrack"},
    {REST_FORCECOPYACLWITHFILE,    L"Explorer", L"ForceCopyACLWithFile"},
    {REST_NOLOGO3CHANNELNOTIFY,    L"Explorer", L"NoMSAppLogo5ChannelNotify"},
    {REST_NOFORGETSOFTWAREUPDATE,  L"Explorer", L"NoForgetSoftwareUpdate"},
    {REST_GREYMSIADS,              L"Explorer", L"GreyMSIAds"},

    // More start menu Restritions for 4.01
    {REST_NOSETACTIVEDESKTOP,      L"Explorer", L"NoSetActiveDesktop"},
    {REST_NOUPDATEWINDOWS,         L"Explorer", L"NoWindowsUpdate"},
    {REST_NOCHANGESTARMENU,        L"Explorer", L"NoChangeStartMenu"},
    {REST_NOFOLDEROPTIONS,         L"Explorer", L"NoFolderOptions"},
    {REST_NOCSC,                   L"Explorer", L"NoSyncAll"},

    // NT5 shell restrictions
    {REST_HASFINDCOMPUTERS,        L"Explorer", L"FindComputers"},
    {REST_RUNDLGMEMCHECKBOX,       L"Explorer", L"MemCheckBoxInRunDlg"},
    {REST_INTELLIMENUS,            L"Explorer", L"IntelliMenus"},
    {REST_SEPARATEDESKTOPPROCESS,  L"Explorer", L"SeparateProcess"}, // this one was actually checked in IE4 in shdocvw, but not here. Duh.
    {REST_MaxRecentDocs,           L"Explorer", L"MaxRecentDocs"},
    {REST_NOCONTROLPANEL,          L"Explorer", L"NoControlPanel"},     // Remove only the control panel from the Settings menu
    {REST_ENUMWORKGROUP,           L"Explorer", L"EnumWorkgroup"}, 
    {REST_ARP_ShowPostSetup,       L"AddRemoveProgs", L"ShowPostSetup"}, 
    {REST_ARP_NOARP,               L"AddRemoveProgs", L"NoAddRemovePrograms"}, 
    {REST_ARP_NOREMOVEPAGE,        L"AddRemoveProgs", L"NoRemovePage"}, 
    {REST_ARP_NOADDPAGE,           L"AddRemoveProgs", L"NoAddPage"}, 
    {REST_ARP_NOWINSETUPPAGE,      L"AddRemoveProgs", L"NoWindowsSetupPage"}, 
    {REST_NOCHANGEMAPPEDDRIVELABEL, L"Explorer", L"NoChangeMappedDriveLabel"},
    {REST_NOCHANGEMAPPEDDRIVECOMMENT, L"Explorer", L"NoChangeMappedDriveComment"},
    {REST_NONETWORKCONNECTIONS,    L"Explorer", L"NoNetworkConnections"},
    {REST_FORCESTARTMENULOGOFF,    L"Explorer", L"ForceStartMenuLogoff"},
    {REST_NOWEBVIEW,               L"Explorer", L"NoWebView"},
    {REST_NOCUSTOMIZETHISFOLDER,   L"Explorer", L"NoCustomizeThisFolder"},
    {REST_NOENCRYPTION,            L"Explorer", L"NoEncryption"},
    {REST_ALLOWFRENCRYPTION,       L"Explorer", L"AllowFrenchEncryption"},
    {REST_DONTSHOWSUPERHIDDEN,     L"Explorer", L"DontShowSuperHidden"},
    {REST_NOSHELLSEARCHBUTTON,     L"Explorer", L"NoShellSearchButton"},
    {REST_NOHARDWARETAB,           L"Explorer", L"NoHardwareTab"},
    {REST_NORUNASINSTALLPROMPT,    L"Explorer", L"NoRunasInstallPrompt"},
    {REST_PROMPTRUNASINSTALLNETPATH, L"Explorer", L"PromptRunasInstallNetPath"},
    {REST_NOMANAGEMYCOMPUTERVERB,  L"Explorer", L"NoManageMyComputerVerb"},
    {REST_NORECENTDOCSNETHOOD,     L"Explorer", L"NoRecentDocsNetHood"},
    {REST_DISALLOWRUN,             L"Explorer", L"DisallowRun"},
    {REST_NOWELCOMESCREEN,         L"Explorer", L"NoWelcomeScreen"},
    {REST_RESTRICTCPL,             L"Explorer", L"RestrictCpl"},
    {REST_DISALLOWCPL,             L"Explorer", L"DisallowCpl"},
    {REST_NOSMBALLOONTIP,          L"Explorer", L"NoSMBalloonTip"},
    {REST_NOSMHELP,                L"Explorer", L"NoSMHelp"},
    {REST_NOWINKEYS,               L"Explorer", L"NoWinKeys"},
    {REST_NOENCRYPTONMOVE,         L"Explorer", L"NoEncryptOnMove"},
    {REST_NOLOCALMACHINERUN,       L"Explorer", L"DisableLocalMachineRun"},
    {REST_NOCURRENTUSERRUN,        L"Explorer", L"DisableCurrentUserRun"},
    {REST_NOLOCALMACHINERUNONCE,   L"Explorer", L"DisableLocalMachineRunOnce"},
    {REST_NOCURRENTUSERRUNONCE,    L"Explorer", L"DisableCurrentUserRunOnce"},
    {REST_FORCEACTIVEDESKTOPON,    L"Explorer", L"ForceActiveDesktopOn"},
    {REST_NOCOMPUTERSNEARME,       L"Explorer", L"NoComputersNearMe"},
    {REST_NOVIEWONDRIVE,           L"Explorer", L"NoViewOnDrive"},
    {REST_NOSMMYDOCS,              L"Explorer", L"NoSMMyDocs"},
    {0, NULL, NULL},
};

DWORD g_rgRestrictionItemValues[ARRAYSIZE(c_rgRestrictionItems) - 1 ] = { 0 };

EXTERN_C HANDLE g_hRestrictions = NULL;
LONG g_lProcessRestrictionsCount = -1; //  current process's count

HANDLE _GetRestrictionsCounter()
{
    return SHGetCachedGlobalCounter(&g_hRestrictions, &GUID_Restrictions);
}

BOOL _QueryRestrictionsChanged(void)
{
    long lGlobalCount = SHGlobalCounterGetValue(_GetRestrictionsCounter());
    if (g_lProcessRestrictionsCount != lGlobalCount)
    {
        g_lProcessRestrictionsCount = lGlobalCount;
        return TRUE;
    }
    return FALSE;
}


// Returns DWORD vaolue if any of the specified restrictions are in place.
// 0 otherwise.

STDAPI_(DWORD) SHRestricted(RESTRICTIONS rest)
{
    // The cache may be invalid. Check first! We have to use
    // a global named semaphore in case this function is called
    // from a process other than the shell process. (And we're
    // sharing the same count between shell32 and shdocvw.)
    if (_QueryRestrictionsChanged())
    {
        memset(g_rgRestrictionItemValues, (BYTE)-1, SIZEOF(g_rgRestrictionItemValues));
    }

    return SHRestrictionLookup(rest, NULL, c_rgRestrictionItems, g_rgRestrictionItemValues);
}

STDAPI_(BOOL) SHIsRestricted(HWND hwnd, RESTRICTIONS rest)
{
    if (SHRestricted(rest))
    {
        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
            MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
        return TRUE;
    }
    return FALSE;
}


// Called by Explorer.exe when things change so that we can zero our global
// data on ini changed status. Wparam and lparam are from a WM_SETTINGSCHANGED/WM_WININICHANGE
// message.

STDAPI_(void) SHSettingsChanged(WPARAM wParam, LPARAM lParam)
{
    if (!lParam || 
        lstrcmpi(TEXT("Policy"), (LPCTSTR)lParam) == 0 ||
        lstrcmpi(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"), (LPCTSTR)lParam) == 0)
    {
        SHGlobalCounterIncrement(_GetRestrictionsCounter());
    }
}

LONG SHRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, DWORD *pRes, DWORD *lpType, LPBYTE lpData, DWORD *lpcbData)
{
    return SHQueryValueExA(hKey, lpValueName, pRes, lpType, lpData, lpcbData);
}

LONG SHRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD *pRes, DWORD *lpType, LPBYTE lpData, DWORD *lpcbData)
{
    return SHQueryValueExW(hKey, lpValueName, pRes, lpType, lpData, lpcbData);
}

STDAPI_(LONG) SHRegQueryValueA(HKEY hKey, LPCSTR lpSubKey, LPSTR lpValue, LONG *lpcbValue)
{
    LONG l;
    HKEY ChildKey;

    if ((lpSubKey == NULL) || (*lpSubKey == 0)) 
    {
        ChildKey = hKey;
    } 
    else 
    {
        l = RegOpenKeyExA(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &ChildKey);
        if (l != ERROR_SUCCESS)
            return l;
    }

    l = SHQueryValueExA(ChildKey, NULL, NULL, NULL, (LPBYTE)lpValue, (DWORD *)lpcbValue);

    if (ChildKey != hKey)
        RegCloseKey(ChildKey);

    if (l == ERROR_FILE_NOT_FOUND) 
    {
        if (lpValue)
            *lpValue = 0;
        if (lpcbValue)
            *lpcbValue = SIZEOF(CHAR);
        l = ERROR_SUCCESS;
    }
    return l;
}

STDAPI_(LONG) SHRegQueryValueW(HKEY hKey, LPCWSTR lpSubKey, LPWSTR lpValue, LONG *lpcbValue)
{
    LONG l;
    HKEY ChildKey;

    if ((lpSubKey == NULL) || (*lpSubKey == 0)) 
    {
        ChildKey = hKey;
    } 
    else 
    {
        l = RegOpenKeyExW( hKey, lpSubKey, 0, KEY_QUERY_VALUE, &ChildKey );
        if (l != ERROR_SUCCESS)
            return l;
    }

    l = SHQueryValueExW(ChildKey, NULL, NULL, NULL, (LPBYTE)lpValue, (DWORD *)lpcbValue);

    if (ChildKey != hKey)
        RegCloseKey(ChildKey);

    if (l == ERROR_FILE_NOT_FOUND) 
    {
        if (lpValue)
            *lpValue = 0;
        if (lpcbValue)
            *lpcbValue = SIZEOF(WCHAR);
        l = ERROR_SUCCESS;
    }
    return l;
}


void SHRegCloseKeys(HKEY ahkeys[], UINT ckeys)
{
    UINT ikeys;
    for (ikeys = 0; ikeys < ckeys; ikeys++) 
    {
        if (ahkeys[ikeys]) 
        {
            RegCloseKey(ahkeys[ikeys]);
            ahkeys[ikeys] = NULL;
        }
    }
}


#ifdef WINNT
// On Win95, RegDeleteKey deletes the key and all subkeys.  On NT, RegDeleteKey
// fails if there are any subkeys.  On NT, we'll make shell code that assumes
// the Win95 behavior work by mapping SHRegDeleteKey to a helper function that
// does the recursive delete.

LONG SHRegDeleteKeyW(HKEY hKey, LPCTSTR lpSubKey)
{
    // Forward to shlwapi's implementation.
    return SHDeleteKey(hKey, lpSubKey);
}
#endif


//----------------------------------------------------------------------------
STDAPI_(BOOL) SHWinHelp(HWND hwndMain, LPCTSTR lpszHelp, UINT usCommand, ULONG_PTR ulData)
{
    // Try to show help
    if (!WinHelp(hwndMain, lpszHelp, usCommand, ulData))
    {
        // Problem.
        ShellMessageBox(HINST_THISDLL, hwndMain,
                MAKEINTRESOURCE(IDS_WINHELPERROR),
                MAKEINTRESOURCE(IDS_WINHELPTITLE),
                MB_ICONHAND | MB_OK);
        return FALSE;
    }
    return TRUE;
}


STDAPI_(void) MoveCLSIDs()
{
    HKEY  hkeyTo;
    DWORD dwDisp;

    // note that we're trying to open the key first and then create it if opening failed
    // although we could have just tried to create it (it would open it if it existed)
    // because dwDisp always returns REG_OPENED_EXISTING_KEY (BUG!!!!)
    // so we cannot find out if the key existed or ont
    if (RegOpenKeyEx(HKEY_CURRENT_USER, EXPLORER_CLSID, 0, KEY_ALL_ACCESS, &hkeyTo) != ERROR_SUCCESS)
    {
        if (RegCreateKeyEx(HKEY_CURRENT_USER, EXPLORER_CLSID, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyTo, &dwDisp) == ERROR_SUCCESS)
        {
            HKEY hkeyFrom;

            if (RegOpenKeyEx(HKEY_CURRENT_USER, SOFTWARE_CLASSES_CLSID, 0, KEY_ALL_ACCESS, &hkeyFrom) == ERROR_SUCCESS)
            {
                DWORD dwIndex;
                DWORD cchSubKeySize;
                DWORD cchClassSize;
                TCHAR szSubKey[255];
                TCHAR szClass[255];

                // we only want to copy subkeys not the values of SW_CLASSES_CLSID
                // so enumerate all the keys and copy them one by one
                // we're doing that in the reverse order because we need to delete them
                // at the same
                // dwIndex holds the number of keys after below's call
                if (ERROR_SUCCESS == RegQueryInfoKey(hkeyFrom, NULL, NULL, NULL, 
                                                     &dwIndex, NULL, NULL, NULL, 
                                                     NULL, NULL, NULL, NULL))
                {
                    // note that dwIndex is pre-decremented in call to RegEnumKeyEx below
                    // because the first time the call is made dwIndex holds the number of 
                    // sub keys and keys are zero indexed so -1
                    cchSubKeySize = ARRAYSIZE(szSubKey);
                    cchClassSize  = ARRAYSIZE(szClass);

                    for (;
                         RegEnumKeyEx(hkeyFrom, --dwIndex, szSubKey, &cchSubKeySize, NULL, szClass, &cchClassSize, NULL) == ERROR_SUCCESS;
                         cchSubKeySize = ARRAYSIZE(szSubKey), cchClassSize = ARRAYSIZE(szClass))
                    {
                        HKEY hkey;

                        if (RegCreateKeyEx(hkeyTo, szSubKey, 0, szClass, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwDisp) == ERROR_SUCCESS)
                        {
                            if (SHCopyKey(hkeyFrom, szSubKey, hkey, 0) == ERROR_SUCCESS)
                                SHRegDeleteKey(hkeyFrom, szSubKey);
                        }
                    }
                }
                RegCloseKey(hkeyFrom);
            }
            RegCloseKey(hkeyTo);
        }
    }
    else
    {
        RegCloseKey(hkeyTo);
    }
}


STDAPI StringToStrRet(LPCTSTR pszName, STRRET *pStrRet)
{
#ifdef UNICODE
    pStrRet->uType = STRRET_WSTR;
    return SHStrDup(pszName, &pStrRet->pOleStr);
#else
    pStrRet->uType = STRRET_CSTR;
    lstrcpyn(pStrRet->cStr, pszName, ARRAYSIZE(pStrRet->cStr));
    return NOERROR;
#endif
}

STDAPI StringToStrRetW(LPCWSTR pszName, STRRET *pStrRet)
{
    pStrRet->uType = STRRET_WSTR;
    return SHStrDupW(pszName, &pStrRet->pOleStr);
}


STDAPI ResToStrRet(UINT id, STRRET *pStrRet)
{
#ifdef UNICODE
    HRESULT hres;
    TCHAR szTemp[MAX_PATH];

    pStrRet->uType = STRRET_WSTR;
    LoadString(HINST_THISDLL, id, szTemp, ARRAYSIZE(szTemp));
    hres = SHStrDup(szTemp, &pStrRet->pOleStr);
    return hres;
#else
    LoadString(HINST_THISDLL, id, pStrRet->cStr, ARRAYSIZE(pStrRet->cStr));
    pStrRet->uType = STRRET_CSTR;
    return NOERROR;
#endif
}

STDAPI_(LPCTSTR) SkipServerSlashes(LPCTSTR pszName)
{
    for (pszName; *pszName && *pszName == TEXT('\\'); pszName++);

    return pszName;
}


UINT g_uCodePage = 0; 
BOOL g_bFarEastPlatForm = 42;   // magic uninited value

// which platform we're running

STDAPI_(BOOL) IsFarEastPlatform()
{
    if (g_bFarEastPlatForm == 42)
    {
        g_bFarEastPlatForm = (g_uCodePage == 932 || g_uCodePage == 936 || g_uCodePage == 949 || g_uCodePage == 950);
    }
    return g_bFarEastPlatForm;
}


int WINAPI lstrcmpiNoDBCS(LPCTSTR lpsz1, LPCTSTR lpsz2)
{
    TCHAR sz1[MAX_PATH];
    TCHAR sz2[MAX_PATH];

    if (IsFarEastPlatform())
    {
        if (lpsz1 && lpsz2)
        {
#ifdef WINNT
            //
            // Don't expect LCMAP_LINGUISTIC_CASING will ignore
            // double-byte alphabetic character even though Win95J does.
            //
            lstrcpy(sz1, lpsz1);
            lstrcpy(sz2, lpsz2);
            CharLowerNoDBCS(sz1);
            CharLowerNoDBCS(sz2);
#else     // WINNT
            int  cch;
            cch = lstrlen(lpsz1) + 1;
            LCMapString(LOCALE_SYSTEM_DEFAULT,
                        LCMAP_LOWERCASE | LCMAP_LINGUISTIC_CASING,
                        lpsz1, cch, sz1, cch);

            cch = lstrlen(lpsz2) + 1;
            LCMapString(LOCALE_SYSTEM_DEFAULT,
                        LCMAP_LOWERCASE | LCMAP_LINGUISTIC_CASING,
                        lpsz2, cch, sz2, cch);
#endif     // WINNT
            return lstrcmp(sz1, sz2);
        }
        return -1; // returns not equal anyway.
    }
    else
        return lstrcmpi(lpsz1, lpsz2);
}


LPCTSTR SkipLeadingSlashes(LPCTSTR pszURL)
{
    LPCTSTR pszURLStart;

    ASSERT(IS_VALID_STRING_PTR(pszURL, -1));

    pszURLStart = pszURL;

    // Skip two leading slashes.

    if (pszURL[0] == TEXT('/') && pszURL[1] == TEXT('/'))
        pszURLStart += 2;

    ASSERT(IS_VALID_STRING_PTR(pszURL, -1) &&
           IsStringContained(pszURL, pszURLStart));

    return pszURLStart;
}


#undef PropVariantClear

STDAPI PropVariantClearLazy(PROPVARIANT *pvar)
{
    switch(pvar->vt)
    {
    case VT_I4:
    case VT_UI4:
    case VT_EMPTY:
    case VT_FILETIME:
        // No operation
        break;

    // SHAlloc matches the CoTaskMemFree functions and will init OLE if it must be
    // loaded.
    case VT_LPSTR:
        SHFree( pvar->pszVal );
        break;
    case VT_LPWSTR:
        SHFree( pvar->pwszVal );
        break;

    default:
        return PropVariantClear(pvar);  // real version in OLE32
    }
    return S_OK;

}

//===========================================================================
#if defined(FULL_DEBUG)
#include <deballoc.c>
#endif // defined(FULL_DEBUG)


/*********************************************************************\
    DESCRIPTION:
        Return S_OK if all of the items are HTML or CDF references.
    Otherwise, return S_FALSE.
\*********************************************************************/
HRESULT IsDeskCompHDrop(IDataObject * pido)
{
    HRESULT hr = S_FALSE;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    ASSERT(pido);

    // asking for CF_HDROP
    hr = pido->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        HDROP hDrop = (HDROP)medium.hGlobal;
        DRAGINFO di;

        // BUGBUG: we don't deal with non TCHAR stuff here
        di.uSize = SIZEOF(di);
        if (DragQueryInfo(hDrop, &di))
        {
            if (di.lpFileList)
            {
                LPTSTR pszCurrPath = di.lpFileList;

                while (pszCurrPath && pszCurrPath[0])
                {
                    // Is this file not acceptable to create a Desktop Component?
                    if (!PathIsContentType(pszCurrPath, SZ_CONTENTTYPE_HTML) &&
                        !PathIsContentType(pszCurrPath, SZ_CONTENTTYPE_CDF))
                    {
                        // Yes, I don't recognize this file as being acceptable.
                        hr = S_FALSE;
                        break;
                    }
                    pszCurrPath += lstrlen(pszCurrPath) + 1;
                }

                SHFree(di.lpFileList);
            }
        }
        else
        {
            // NOTE: Win95/NT4 dont have this fix, you will fault if you hit this case!
            AssertMsg(FALSE, TEXT("hDrop contains the opposite TCHAR (UNICODE when on ANSI)"));
        }
        ReleaseStgMedium(&medium);
    }

    return hr;
}

HRESULT _LocalAddDTI(LPCSTR pszUrl, HWND hwnd, int x, int y, int nType)
{
    IActiveDesktop * pad;
    HRESULT hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void **)&pad);
    if (SUCCEEDED(hr))
    {
        COMPONENT comp = {
            sizeof(COMPONENT),              //Size of this structure
            0,                              //For Internal Use: Set it always to zero.
            nType,              //One of COMP_TYPE_*
            TRUE,           // Is this component enabled?
            FALSE,             // Had the component been modified and not yet saved to disk?
            FALSE,          // Is the component scrollable?
            {
                sizeof(COMPPOS),             //Size of this structure
                x - GetSystemMetrics(SM_XVIRTUALSCREEN),    //Left of top-left corner in screen co-ordinates.
                y - GetSystemMetrics(SM_YVIRTUALSCREEN),    //Top of top-left corner in screen co-ordinates.
                -1,            // Width in pixels.
                -1,           // Height in pixels.
                10000,            // Indicates the Z-order of the component.
                TRUE,         // Is the component resizeable?
                TRUE,        // Resizeable in X-direction?
                TRUE,        // Resizeable in Y-direction?
                -1,    //Left of top-left corner as percent of screen width
                -1     //Top of top-left corner as percent of screen height
            },              // Width, height etc.,
            L"\0",          // Friendly name of component.
            L"\0",          // URL of the component.
            L"\0",          // Subscrined URL.
            IS_NORMAL       // ItemState
        };
        SHAnsiToUnicode(pszUrl, comp.wszSource, ARRAYSIZE(comp.wszSource));
        SHAnsiToUnicode(pszUrl, comp.wszFriendlyName, ARRAYSIZE(comp.wszFriendlyName));
        SHAnsiToUnicode(pszUrl, comp.wszSubscribedURL, ARRAYSIZE(comp.wszSubscribedURL));

        hr = pad->AddDesktopItemWithUI(hwnd, &comp, DTI_ADDUI_DISPSUBWIZARD);
        pad->Release();
    }
    return hr;
}

/*********************************************************************\
    DESCRIPTION:
        Create Desktop Components for each item.
\*********************************************************************/
HRESULT ExecuteDeskCompHDrop(LPTSTR pszMultipleUrls, HWND hwnd, int x, int y)
{
    IActiveDesktop * pad;
    HRESULT hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void **)&pad);
    if (SUCCEEDED(hr))
    {
        COMPONENT comp = {
            sizeof(COMPONENT),              //Size of this structure
            0,                              //For Internal Use: Set it always to zero.
            COMP_TYPE_WEBSITE,              //One of COMP_TYPE_*
            TRUE,           // Is this component enabled?
            FALSE,             // Had the component been modified and not yet saved to disk?
            FALSE,          // Is the component scrollable?
            {
                sizeof(COMPPOS),             //Size of this structure
                x - GetSystemMetrics(SM_XVIRTUALSCREEN),    //Left of top-left corner in screen co-ordinates.
                y - GetSystemMetrics(SM_YVIRTUALSCREEN),    //Top of top-left corner in screen co-ordinates.
                -1,            // Width in pixels.
                -1,           // Height in pixels.
                10000,            // Indicates the Z-order of the component.
                TRUE,         // Is the component resizeable?
                TRUE,        // Resizeable in X-direction?
                TRUE,        // Resizeable in Y-direction?
                -1,    //Left of top-left corner as percent of screen width
                -1     //Top of top-left corner as percent of screen height
            },              // Width, height etc.,
            L"\0",          // Friendly name of component.
            L"\0",          // URL of the component.
            L"\0",          // Subscrined URL.
            IS_NORMAL       // ItemState
        };
        while (pszMultipleUrls[0])
        {
            SHTCharToUnicode(pszMultipleUrls, comp.wszSource, ARRAYSIZE(comp.wszSource));
            SHTCharToUnicode(pszMultipleUrls, comp.wszFriendlyName, ARRAYSIZE(comp.wszFriendlyName));
            SHTCharToUnicode(pszMultipleUrls, comp.wszSubscribedURL, ARRAYSIZE(comp.wszSubscribedURL));

            hr = pad->AddDesktopItemWithUI(hwnd, &comp, DTI_ADDUI_DISPSUBWIZARD);
            pszMultipleUrls += lstrlen(pszMultipleUrls) + 1;
        }

        pad->Release();
    }

    return hr;
}

typedef struct {
    LPSTR pszUrl;
    LPTSTR pszMultipleUrls;
    BOOL fMultiString;
    HWND hwnd;
    DWORD dwFlags;
    int x;
    int y;
} CREATEDESKCOMP;


/*********************************************************************\
    DESCRIPTION:
        Create Desktop Components for one or mowe items.  We need to start
    a thread to do this because it may take a while and we don't want
    to block the UI thread because dialogs may be displayed.
\*********************************************************************/
DWORD CALLBACK _CreateDeskComp_ThreadProc(void *pvCreateDeskComp)
{
    CREATEDESKCOMP * pcdc = (CREATEDESKCOMP *) pvCreateDeskComp;

    HRESULT hres = OleInitialize(0);
    if (EVAL(SUCCEEDED(hres)))
    {
        if (pcdc->fMultiString)
        {
            hres = ExecuteDeskCompHDrop(pcdc->pszMultipleUrls, pcdc->hwnd, pcdc->x, pcdc->y);
            SHFree(pcdc->pszMultipleUrls);
        }
        else if (pcdc->dwFlags & DESKCOMP_URL)
        {
            hres = _LocalAddDTI(pcdc->pszUrl, pcdc->hwnd, pcdc->x, pcdc->y, COMP_TYPE_WEBSITE);
            Str_SetPtrA(&(pcdc->pszUrl), NULL);
        }
        else if (pcdc->dwFlags & DESKCOMP_IMAGE)
        {
            hres = _LocalAddDTI(pcdc->pszUrl, pcdc->hwnd, pcdc->x, pcdc->y, COMP_TYPE_PICTURE);
        }
        OleUninitialize();
    }

    LocalFree(pcdc);
    return 0;
}


/*********************************************************************\
    DESCRIPTION:
        Create Desktop Components for one or mowe items.  We need to start
    a thread to do this because it may take a while and we don't want
    to block the UI thread because dialogs may be displayed.
\*********************************************************************/
HRESULT CreateDesktopComponents(LPCSTR pszUrl, IDataObject * pido, HWND hwnd, DWORD dwFlags, int x, int y)
{
    HRESULT hr = E_OUTOFMEMORY;
    CREATEDESKCOMP * pcdc = (CREATEDESKCOMP *)LocalAlloc(LPTR, SIZEOF(CREATEDESKCOMP));

    // Create Thread....
    if (pcdc)
    {
        pcdc->pszUrl = NULL; // In case of failure.
        pcdc->pszMultipleUrls = NULL; // In case of failure.
        pcdc->fMultiString = (pido ? TRUE : FALSE);
        pcdc->hwnd = hwnd;
        pcdc->dwFlags = dwFlags;
        pcdc->x = x;
        pcdc->y = y;

        if (!pcdc->fMultiString)
            Str_SetPtrA(&(pcdc->pszUrl), pszUrl);
        else
        {
            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            STGMEDIUM medium;

            // asking for CF_HDROP
            hr = pido->GetData(&fmte, &medium);
            if (SUCCEEDED(hr))
            {
                HDROP hDrop = (HDROP)medium.hGlobal;
                DRAGINFO di;

                // BUGBUG: we don't deal with non TCHAR stuff here
                di.uSize = SIZEOF(di);
                if (DragQueryInfo(hDrop, &di))
                    pcdc->pszMultipleUrls = di.lpFileList;
                else
                {
                    // NOTE: Win95/NT4 dont have this fix, you will fault if you hit this case!
                    AssertMsg(FALSE, TEXT("hDrop contains the opposite TCHAR (UNICODE when on ANSI)"));
                }
                ReleaseStgMedium(&medium);
            }
        }

        if (pcdc->pszUrl || pcdc->pszMultipleUrls)
        {
            SHCreateThread(_CreateDeskComp_ThreadProc, pcdc, CTF_INSIST | CTF_PROCESS_REF, NULL);
            hr = S_OK;
        }
    }

    return hr;
}


#ifndef UNICODE
// Simply counts # of chars in an MBCS string. Net needed for Unicode build
// This assumes we won't handle codepages other than CP_ACP...
int CountChar(LPCTSTR pcsz)
{
    CPINFO cpinfo;
    ASSERT(pcsz);
    if (GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0])
    {
        int i = 0;
        while (*pcsz)
        {
            i++;
            pcsz = CharNext(pcsz);
        }
        return i;
    }
    else
        return lstrlen(pcsz);
}
#endif

/****************************************************************************

        FUNCTION: CenterWindow (HWND, HWND)

        PURPOSE:  Center one window over another

        COMMENTS:

        Dialog boxes take on the screen position that they were designed at,
        which is not always appropriate. Centering the dialog over a particular
        window usually results in a better position.

****************************************************************************/
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     wScreen, hScreen, xNew, yNew;
    HDC     hdc;
    
    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;
    
    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;
    
    // Get the display limits
    hdc = GetDC (hwndChild);
    wScreen = GetDeviceCaps (hdc, HORZRES);
    hScreen = GetDeviceCaps (hdc, VERTRES);
    ReleaseDC (hwndChild, hdc);
    
    // Calculate new X position, then adjust for screen
    xNew = rParent.left + ((wParent - wChild) /2);
    if (xNew < 0) {
        xNew = 0;
    } else if ((xNew+wChild) > wScreen) {
        xNew = wScreen - wChild;
    }
    
    // Calculate new Y position, then adjust for screen
    yNew = rParent.top  + ((hParent - hChild) /2);
    if (yNew < 0) {
        yNew = 0;
    } else if ((yNew+hChild) > hScreen) {
        yNew = hScreen - hChild;
    }
    
    // Set it, and return
    return SetWindowPos (hwndChild, NULL,
        xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// This is exported, as ordinal 184.  It was in shell32\smrttile.c, but no-one was using it
// internally, and it does not appear that anyone external is using it (verified that taskman
// on NT and W95 uses the win32 api's CascadeWindows and TileWindows.  This could probably be
// removed altogether.                                                    (t-saml, 12/97)
STDAPI_(WORD) ArrangeWindows(HWND hwndParent, WORD flags, LPCRECT lpRect, WORD chwnd, const HWND *ahwnd)
{
    ASSERT(0);
    return 0;
}

#define CXIMAGEGAP      6

void    DrawMenuItem(DRAWITEMSTRUCT* lpdi, LPCTSTR lpszMenuText, UINT iIcon)
{
    if ((lpdi->itemAction & ODA_SELECT) || (lpdi->itemAction & ODA_DRAWENTIRE))
    {
        int x, y;
        SIZE sz;
        RECT rc;

        // Draw the image (if there is one).

        GetTextExtentPoint(lpdi->hDC, lpszMenuText, lstrlen(lpszMenuText), &sz);
        
        if (lpdi->itemState & ODS_SELECTED)
        {
            SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_MENUTEXT));
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_MENU));
        }
        
        rc = lpdi->rcItem;
        rc.left += +2*CXIMAGEGAP+g_cxSmIcon;
        
        DrawText(lpdi->hDC,lpszMenuText,lstrlen(lpszMenuText),
            &rc,DT_SINGLELINE|DT_VCENTER);
        if (iIcon != -1)
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
            ImageList_Draw(g_himlIconsSmall, iIcon, lpdi->hDC, x, y, ILD_TRANSPARENT);
        } 
        else 
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
        }
    }
}

LRESULT MeasureMenuItem(MEASUREITEMSTRUCT *lpmi, LPCTSTR lpszMenuText)
{
    LRESULT lres = FALSE;
            
    // Get the rough height of an item so we can work out when to break the
    // menu. User should really do this for us but that would be useful.
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        // REVIEW cache out the menu font?
        NONCLIENTMETRICS ncm;
        ncm.cbSize = SIZEOF(NONCLIENTMETRICS);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE))
        {
            HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
            if (hfont)
            {
                SIZE sz;
                HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                GetTextExtentPoint(hdc, lpszMenuText, lstrlen(lpszMenuText), &sz);
                lpmi->itemHeight = max (g_cySmIcon+CXIMAGEGAP/2, ncm.iMenuHeight);
                lpmi->itemWidth = g_cxSmIcon + 2*CXIMAGEGAP + sz.cx;
                lpmi->itemWidth = 2*CXIMAGEGAP + sz.cx;
                SelectObject(hdc, hfontOld);
                DeleteObject(hfont);
                lres = TRUE;
            }
        }
        ReleaseDC(NULL, hdc);
    }   
    return lres;
}

STDAPI_(BOOL) IsPathInOpenWithKillList(LPCTSTR pszPath)
{
    // return TRUE for invalid path
    if (!pszPath || !*pszPath)
        return TRUE;

    // get file name
    BOOL fRet = FALSE;
    LPCTSTR pchFile = PathFindFileName(pszPath);
    DWORD cb;
    HKEY hkey;

    if (SUCCEEDED(AssocQueryKey(ASSOCF_OPEN_BYEXENAME, ASSOCKEY_APP, pchFile, NULL, &hkey)))
    {
        //  just check for the existence of the value....
        if (ERROR_SUCCESS == SHQueryValueEx(hkey, TEXT("NoOpenWith"), NULL, NULL, NULL, NULL))
        {
            fRet = TRUE;
        }

        RegCloseKey(hkey);
    }

    //  if the app didnt specify, use our permanent list.
    if (!fRet && ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_FILEASSOCIATION, TEXT("KillList"), NULL, NULL, &cb) 
            && cb > SIZEOF(TCHAR))
    {
        // check the kill list from registry
        LPTSTR pszKillList = (LPTSTR)LocalAlloc(LPTR, cb);
        if (pszKillList)
        {
            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_FILEASSOCIATION, TEXT("KillList"), NULL, pszKillList, &cb) )
            {
                if (StrStrI(pszKillList, pchFile))
                {
                    fRet = TRUE;
                }
            }
            LocalFree(pszKillList);
        }
    }
    
    return fRet;
}


/*
    GetFileDescription retrieves the friendly name from a file's verion rsource.
    The first language we try will be the first item in the
    "\VarFileInfo\Translations" section;  if there's nothing there,
    we try the one coded into the IDS_VN_FILEVERSIONKEY resource string.
    If we can't even load that, we just use English (040904E4).  We
    also try English with a null codepage (04090000) since many apps
    were stamped according to an old spec which specified this as
    the required language instead of 040904E4.
    
    If there is no FileDescription in version resource, return the file name.

    Parameters:
        LPCTSTR pszPath: full path of the file
        LPTSTR pszDesc: pointer to the buffer to receive friendly name. If NULL, 
                        *pcchDesc will be set to the length of friendly name in 
                        characters, including ending NULL, on successful return.
        UINT *pcchDesc: length of the buffer in characters. On successful return,
                        it contains number of characters copied to the buffer,
                        including ending NULL.

    Return:
        TRUE on success, and FALSE otherwise
*/
BOOL WINAPI GetFileDescription(LPCTSTR pszPath, LPTSTR pszDesc, UINT *pcchDesc)
{
    TCHAR szVersionKey[60];         /* big enough for anything we need */
    LPTSTR pszVersionKey = NULL;

    // Try same language as this program
    if (LoadString(HINST_THISDLL, IDS_VN_FILEVERSIONKEY, szVersionKey, ARRAYSIZE(szVersionKey)))
    {
        StrCatBuff(szVersionKey, TEXT("FileDescription"), SIZECHARS(szVersionKey));
        pszVersionKey = szVersionKey;
    }
    
    //  just use the default cut list
    return SHGetFileDescription(pszPath, pszVersionKey, NULL, pszDesc, pcchDesc);
}

HRESULT PathFromDataObject(IDataObject *pdtobj, LPTSTR pszPath, UINT cchPath)
{
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;
    
    HRESULT hres = pdtobj->GetData(&fmte, &medium);

    if (SUCCEEDED(hres))
    {
        if (DragQueryFile((HDROP)medium.hGlobal, 0, pszPath, cchPath))
            hres = S_OK;
        else
            hres = E_FAIL;
            
        ReleaseStgMedium(&medium);
    }

    return hres;
}

HRESULT PidlFromDataObject(IDataObject *pdtobj, LPITEMIDLIST * ppidlTarget)
{
    HRESULT hres;

    *ppidlTarget = NULL;

    // If the data object has a HIDA, then use it.  This allows us to
    // access pidls inside data objects that aren't filesystem objects.
    // (It's also faster than extracting the path and converting it back
    // to a pidl.  Difference:  pidls for files on the desktop
    // are returned in original form instead of being converted to
    // a CSIDL_DESKTOPDIRECTORY-relative pidl.  I think this is a good thing.)

    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    if (pida)
    {
        *ppidlTarget = HIDA_ILClone(pida, 0);
        HIDA_ReleaseStgMedium(pida, &medium);
        hres = *ppidlTarget ? NOERROR : E_OUTOFMEMORY;
    }
    else
    {
        // No HIDA available; go for a filename

        // This string is also used to store an URL in case it's an URL file
        TCHAR szPath[MAX_URL_STRING];

        hres = PathFromDataObject(pdtobj, szPath, ARRAYSIZE(szPath));

        if (SUCCEEDED(hres))
        {
            *ppidlTarget = ILCreateFromPath(szPath);
            hres = *ppidlTarget ? NOERROR : E_OUTOFMEMORY;
        }
    }

    return hres;
}

//
// move here from netviewx.c
//

int WINAPI SHOutOfMemoryMessageBox(HWND hwnd, LPTSTR pszTitle, UINT fuStyle)
{
    TCHAR szOutOfMemory[128];
    TCHAR szTitle[128];
    int ret;

    szTitle[0] = 0;

    if (pszTitle==NULL)
    {
        GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle));
        pszTitle = szTitle;
    }

    szOutOfMemory[0] = 0;

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ERROR_OUTOFMEMORY, 0, szOutOfMemory,
                          ARRAYSIZE(szOutOfMemory), NULL);
    ret = MessageBox(hwnd, szOutOfMemory, pszTitle, fuStyle | MB_SETFOREGROUND);
    if (ret == -1)
    {
       DebugMsg(DM_TRACE, TEXT("regular message box failed, trying sysmodal"));
       ret = MessageBox(hwnd, szOutOfMemory, pszTitle, fuStyle | MB_SYSTEMMODAL);
    }

    return ret;
}


#define SZ_PATHLIST_SEPARATOR          TEXT(";")

BOOL SafePathListAppend(LPTSTR pszDestPath, DWORD cchDestSize, LPCTSTR pszPathToAdd)
{
    BOOL fResult = FALSE;

    if ((lstrlen(pszDestPath) + lstrlen(pszPathToAdd) + 1) < (long) cchDestSize)
    {
        // Do we need to add a ";" between pszDestPath and pszPathToAdd?
        if (pszDestPath[0] &&
            (SZ_PATHLIST_SEPARATOR[0] != pszDestPath[lstrlen(pszDestPath)-1]))
        {
            StrCat(pszDestPath, SZ_PATHLIST_SEPARATOR);
        }

        StrCat(pszDestPath, pszPathToAdd);
        fResult = TRUE;
    }

    return fResult;
}

bool IsDiscardablePropertySet(const FMTID & fmtid)
{
    if (IsEqualGUID(fmtid, FMTID_DiscardableInformation))
        return true;

    return false;
}

bool IsDiscardableStream(LPCTSTR pszStreamName)
{

    static const LPCTSTR _apszDiscardableStreams[] =
    {   
        // Mike Hillberg claims this stream is discardable, and is used to
        //  hold a few state bytes for property set information

        TEXT(":{4c8cc155-6c1e-11d1-8e41-00c04fb9386d}:$DATA")
    };

    for (int i = 0; i < ARRAYSIZE(_apszDiscardableStreams); i++)
    {
        if (0 == lstrcmpi(_apszDiscardableStreams[i], pszStreamName))
            return TRUE;
    }

    return FALSE;
}

#ifdef WINNT        // Requires NtXXXX API's not avail on Win9X

// GetDownlevelCopyDataLossText
//
// If data will be lost on a downlevel copy from NTFS to FAT, we return
// a string containing a description of the data that will be lost, 
// suitable for display to the user.  String must be freed by the caller.
//
// If nothing will be lost, a NULL is returned.
//
// pbDirIsSafe points to a BOOL passed in by the caller.  On return, if
// *pbDirIsSafe has been set to TRUE, no further data loss could occur
// in this directory
//
// Davepl 01-Mar-98

#define NT_FAILED(x) NT_ERROR(x)   // More consistent name for this macro

LPWSTR GetDownlevelCopyDataLossText(LPCWSTR pszSrcObject, LPCWSTR pszDestDir, BOOL bIsADir, BOOL * pbDirIsSafe)
{
    BOOL                            bMultiStream = FALSE;
    NTSTATUS                        NtStatus;
    OBJECT_ATTRIBUTES               SrcObjectAttributes; 
    OBJECT_ATTRIBUTES               DestObjectAttributes; 
    IO_STATUS_BLOCK                 IoStatusBlock;
    HANDLE SrcObjectHandle            = INVALID_HANDLE_VALUE; 
    HANDLE DestPathHandle           = INVALID_HANDLE_VALUE; 
    UNICODE_STRING                  UnicodeSrcObject;
    UNICODE_STRING                  UnicodeDestPath;
    IPropertySetStorage           * pPropSetStorage = NULL;
    IEnumSTATPROPSETSTG           * pEnumSetStorage = NULL;

    *pbDirIsSafe = FALSE;

    // pAttributeInfo will point to enough stack to hold the 
    // FILE_FS_ATTRIBUTE_INFORMATION and worst-case filesystem name

    size_t cbAttributeInfo          = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                                         MAX_PATH * sizeof(TCHAR);
    PFILE_FS_ATTRIBUTE_INFORMATION  pAttributeInfo = 
                                      (PFILE_FS_ATTRIBUTE_INFORMATION) _alloca(cbAttributeInfo);

    // Covert the conventional paths to UnicodePath descriptors

    RtlInitUnicodeString(&UnicodeSrcObject, pszSrcObject);
    if( !RtlDosPathNameToNtPathName_U(pszSrcObject,
                                      &UnicodeSrcObject,                            
                                      NULL,
                                      NULL)) {
        AssertMsg(FALSE, TEXT("RtlDosPathNameToNtPathName_U failed for source."));
        return NULL;
    }

    RtlInitUnicodeString(&UnicodeDestPath, pszDestDir);
    if( !RtlDosPathNameToNtPathName_U(pszDestDir,
                                      &UnicodeDestPath,                            
                                      NULL,
                                      NULL)) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeSrcObject.Buffer );    
        AssertMsg(FALSE, TEXT("RtlDosPathNameToNtPathName_U failed for dest."));
        return NULL;
    }

    // Build an NT object descriptor from the UnicodeSrcObject

    InitializeObjectAttributes(&SrcObjectAttributes,            
                              &UnicodeSrcObject,
                              OBJ_CASE_INSENSITIVE,            
                              NULL,            
                              NULL );    
    InitializeObjectAttributes(&DestObjectAttributes,            
                              &UnicodeDestPath,
                              OBJ_CASE_INSENSITIVE,            
                              NULL,            
                              NULL );    

    // Open the file for generic read, and the dest path for attribute read

    NtStatus = NtOpenFile(&SrcObjectHandle,
                          FILE_GENERIC_READ,  
                          &SrcObjectAttributes,            
                          &IoStatusBlock,
                          FILE_SHARE_READ,
                          (bIsADir ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE));

    if (NT_FAILED(NtStatus))
    {
        AssertMsg(FALSE, TEXT("NtOpenObject failed for source %08x\n"), NtStatus);        
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeSrcObject.Buffer );    
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeDestPath.Buffer );    
        return NULL;
    }

    NtStatus = NtOpenFile(&DestPathHandle,
                          FILE_READ_ATTRIBUTES,  
                          &DestObjectAttributes,            
                          &IoStatusBlock,
                          FILE_SHARE_READ,
                          FILE_DIRECTORY_FILE);

    if (NT_FAILED(NtStatus))
    {
        AssertMsg(FALSE, TEXT("NtOpenObject failed for dest path %08x\n"), NtStatus);
        NtClose(SrcObjectHandle);
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeSrcObject.Buffer );    
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeDestPath.Buffer );    
        return NULL;
    }

    // Incrementally try allocation sizes for the ObjectStreamInformation,
    // then retrieve the actual stream info

    BYTE * pBuffer = NULL;

    __try   // Current and future allocations and handles free'd by __finally block
    {    
        size_t cbBuffer;
        LPTSTR pszUserMessage = NULL;

        // Quick check of filesystem type for this file

        NtStatus = NtQueryVolumeInformationFile(
                    SrcObjectHandle,
                    &IoStatusBlock,
                    (BYTE *) pAttributeInfo,
                    cbAttributeInfo,
                    FileFsAttributeInformation
                    );

        if (NT_FAILED(NtStatus)) 
            return NULL;

        // If the source filesystem isn't NTFS, we can just bail now

        pAttributeInfo->FileSystemName[ 
            (pAttributeInfo->FileSystemNameLength / sizeof(WCHAR)) ] = L'\0';

        if (0 == StrStrIW(pAttributeInfo->FileSystemName, L"NTFS"))   
        {   
            *pbDirIsSafe = TRUE;
            return NULL;
        }

        NtStatus = NtQueryVolumeInformationFile(
                    DestPathHandle,
                    &IoStatusBlock,
                    (BYTE *) pAttributeInfo,
                    cbAttributeInfo,
                    FileFsAttributeInformation
                    );

        if (NT_FAILED(NtStatus)) 
            return NULL;

        // If the target filesystem is NTFS, no stream loss will happen

        pAttributeInfo->FileSystemName[ 
            (pAttributeInfo->FileSystemNameLength / sizeof(WCHAR)) ] = L'\0';
        if (StrStrIW(pAttributeInfo->FileSystemName, L"NTFS"))
        {
            *pbDirIsSafe = TRUE;
            return NULL;
        }

        // At this point we know we're doing an NTFS->FAT copy, so we need
        // to find out whether or not the source file has multiple streams

        // pBuffer will point to enough memory to hold the worst case for
        // a single stream.  

        cbBuffer = sizeof(FILE_STREAM_INFORMATION) + MAX_PATH * sizeof(WCHAR);
        if (NULL == (pBuffer = (BYTE *) LocalAlloc(LPTR, cbBuffer)))
            return NULL;
        do
        {
            BYTE * pOldBuffer = pBuffer;
            if (NULL == (pBuffer = (BYTE *) LocalReAlloc(pBuffer, cbBuffer, LMEM_MOVEABLE)))
            {
                LocalFree(pOldBuffer);
                return NULL;
            }

            NtStatus = NtQueryInformationFile(SrcObjectHandle, &IoStatusBlock, pBuffer, cbBuffer,
                                            FileStreamInformation);
            cbBuffer *= 2;
        } while (STATUS_BUFFER_OVERFLOW == NtStatus);
        
        if (NT_SUCCESS(NtStatus))
        {
            FILE_STREAM_INFORMATION * pStreamInfo = (FILE_STREAM_INFORMATION *) pBuffer;
            BOOL bLastPass = (0 == pStreamInfo->NextEntryOffset);

            if (bIsADir)
            {
                // From experimentation, it seems that if there's only one stream on a directory and
                // it has a zero-length name, its a vanilla directory

                if ((0 == pStreamInfo->NextEntryOffset) && (0 == pStreamInfo->StreamNameLength))
                    return NULL;
            }
            else // File
            {
                // Single stream only if first stream has no next offset

                if ((0 == pStreamInfo->NextEntryOffset) && (pBuffer == (BYTE *) pStreamInfo))
                    return NULL;  
            }

            for(;;)
            {
                int i;
                TCHAR szText[MAX_PATH];

                // Table of known stream names and the string IDs that we actually want to show
                // to the user instead of the raw stream name.

                static const struct _ADATATYPES
                {
                    LPCTSTR m_pszStreamName;
                    UINT    m_idTextID;    
                    BOOL    m_bDiscardable;
                }
                _aDataTypes[] =
                {
                    { TEXT("::"),              0                 , FALSE   },
                    { TEXT(":AFP_AfpInfo:"),   IDS_MACINFOSTREAM , TRUE   },
                    { TEXT(":AFP_Resource:"),  IDS_MACRESSTREAM  , TRUE   }
                };

                if (FALSE == IsDiscardableStream(pStreamInfo->StreamName))
                {
                    for (i = 0; i < ARRAYSIZE(_aDataTypes); i++)
                    {
                    
                        // Can't use string compare since they choke on the \005 character
                        // used in property storage streams
                        int cbComp = min(lstrlen(pStreamInfo->StreamName) * sizeof(TCHAR), 
                                         lstrlen(_aDataTypes[i].m_pszStreamName) * sizeof(TCHAR));
                        if (0 == memcmp(_aDataTypes[i].m_pszStreamName, 
                                        pStreamInfo->StreamName, 
                                        cbComp))
                        {
                            break;
                        }
                    }

                    size_t cch = 0;
                    if (i == ARRAYSIZE(_aDataTypes))
                    {
                        // Not found, so use the actual stream name, unless it has a \005
                        // at the beginning of its name, in which case we'll pick this one
                        // up when we check for property sets.

                        if (pStreamInfo->StreamName[1] ==  TEXT('\005'))
                            cch = 0;
                        else
                        {
                            lstrcpy(szText, pStreamInfo->StreamName);
                            cch = lstrlen(szText);
                        }
                    }
                    else
                    {
                        // We found this stream in our table of well-known streams, so
                        // load the string which the user will see describing this stream,
                        // as we likely have a more useful name than the stream itself.
            
		        
                        if (_aDataTypes[i].m_bDiscardable)
                            cch = 0;
                        else
                            cch = _aDataTypes[i].m_idTextID ? 
                                      LoadString(HINST_THISDLL, _aDataTypes[i].m_idTextID, szText, ARRAYSIZE(szText)) 
                                      : 0;
                    }

                    // Reallocate the overall buffer to be large enough to add this new
                    // stream description, plus 2 chars for the crlf

                    if (cch)
                    {
                        LPTSTR pszOldMessage = pszUserMessage;
                        if (pszOldMessage)
                        {
                            pszUserMessage = (TCHAR *) LocalReAlloc(pszOldMessage, 
                                                                    (lstrlen(pszUserMessage) + cch + 3) * sizeof(TCHAR),
                                                                    LMEM_MOVEABLE);
                            if (pszUserMessage)
                                lstrcat(pszUserMessage, TEXT("\r\n"));
                        }
                        else
                            pszUserMessage = (TCHAR *) LocalAlloc(LPTR, (cch + 1) * sizeof(TCHAR));
                                                              

                        if (NULL == pszUserMessage)
                            return pszOldMessage; // Can't grow it, but at least return what we know so far

                        lstrcat(pszUserMessage, szText);
                    }
                }

                if (bLastPass)
                    break;                

                pStreamInfo = (FILE_STREAM_INFORMATION *) (((BYTE *) pStreamInfo) + pStreamInfo->NextEntryOffset);
                bLastPass = (0 == pStreamInfo->NextEntryOffset);
            } 
            
        }
        // Now look for native property NTFS sets

        if (SUCCEEDED(StgOpenStorageEx(pszSrcObject, 
                                       STGM_READ | STGM_DIRECT | STGM_SHARE_DENY_WRITE, 
                                       STGFMT_FILE,
                                       0,0,0,
                                       IID_IPropertySetStorage,
                                       (void **) &pPropSetStorage)))
        {
            // Enum the property set storages available for this file

            if (SUCCEEDED(pPropSetStorage->Enum(&pEnumSetStorage)))
            {
                const size_t BLOCKS_TO_REQUEST = 10;
                STATPROPSETSTG  statPropSet[BLOCKS_TO_REQUEST] ;
                ULONG cSets ;
                TCHAR szText[MAX_PATH];

                // Enum the property sets available in this property set storage
                
                while( SUCCEEDED( pEnumSetStorage->Next(BLOCKS_TO_REQUEST, statPropSet, &cSets ) ) && cSets > 0 )
                {
                    // For each property set we receive, open it and enumerate the
                    // properties contained withing it

                    for( ULONG iSet=0; iSet < cSets; iSet++ )
                    {
                        if (FALSE == IsDiscardablePropertySet(statPropSet[iSet].fmtid))
                        {
                            size_t cch = 0;

                            static const struct _AKNOWNPSETS
                            {
                                const FMTID * m_pFMTID;
                                UINT          m_idTextID;    
                            }
                            _aKnownPsets[] =
                            {
                                { &FMTID_SummaryInformation,          IDS_DOCSUMINFOSTREAM        },
                                { &FMTID_DocSummaryInformation,       IDS_SUMINFOSTREAM           },
                                { &FMTID_UserDefinedProperties,       IDS_USERDEFPROP             },
                                { &FMTID_ImageSummaryInformation,     IDS_IMAGEINFO               },
                                { &FMTID_AudioSummaryInformation,     IDS_AUDIOSUMINFO            },
                                { &FMTID_VideoSummaryInformation,     IDS_VIDEOSUMINFO            },
                                { &FMTID_MediaFileSummaryInformation, IDS_MEDIASUMINFO            }
                            };
                            
                            // First try to map the fmtid to a better name than what the api gave us

                            for (int i = 0; i < ARRAYSIZE(_aKnownPsets); i++)
                            {
                                if (IsEqualGUID(*(_aKnownPsets[i].m_pFMTID), statPropSet[iSet].fmtid))
                                {
                                    cch = LoadString(HINST_THISDLL, _aKnownPsets[i].m_idTextID, szText, ARRAYSIZE(szText));
                                    break;
                                }
                            }
                            
                            // No useful name... use Unidentied User Properties

                            if (0 == cch)
                                cch = LoadString(HINST_THISDLL, 
                                                     IDS_UNKNOWNPROPSET, 
                                                     szText, ARRAYSIZE(szText));

                            if (cch)
                            {
                                LPTSTR pszOldMessage = pszUserMessage;
                                if (pszOldMessage)
                                {
                                    pszUserMessage = (TCHAR *) LocalReAlloc(pszOldMessage, 
                                                                            (lstrlen(pszUserMessage) + cch + 3) * sizeof(TCHAR),
                                                                            LMEM_MOVEABLE);
                                    if (pszUserMessage)
                                        lstrcat(pszUserMessage, TEXT("\r\n"));
                                }
                                else
                                    pszUserMessage = (TCHAR *) LocalAlloc(LPTR, (cch + 1) * sizeof(TCHAR));

                                if (NULL == pszUserMessage)
                                    pszUserMessage = pszOldMessage; // Can't grow it, but at least keep what we know so far
                                else
                                    lstrcat(pszUserMessage, szText); // Index 1 to skip the \005
                            }
                        }
                    }
                }
                pEnumSetStorage->Release();
                pEnumSetStorage = NULL;
            }
            pPropSetStorage->Release();
            pPropSetStorage = NULL;
        }

        return pszUserMessage;
    }
    __finally   // Cleanup
    {
        if (pBuffer)
            LocalFree(pBuffer);
        if (pPropSetStorage)
            pPropSetStorage->Release();
        if (pEnumSetStorage)
            pEnumSetStorage->Release();

        NtClose(SrcObjectHandle);
        NtClose(DestPathHandle);
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeSrcObject.Buffer );    
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeDestPath.Buffer );    
    }

    return NULL;
}

#endif

// lstrcmp? uses thread lcid.  but in UI visual sorting, we need
// to use the user's choice. (thus the u in ustrcmp)
int _ustrcmp(LPCTSTR psz1, LPCTSTR psz2, BOOL fCaseInsensitive)
{
    COMPILETIME_ASSERT(CSTR_LESS_THAN == 1);
    COMPILETIME_ASSERT(CSTR_EQUAL  == 2);
    COMPILETIME_ASSERT(CSTR_GREATER_THAN  == 3);
    return (CompareString(LOCALE_USER_DEFAULT, 
                         fCaseInsensitive ? NORM_IGNORECASE : 0,
                         psz1, -1, psz2, -1) - CSTR_EQUAL);
}

void HWNDWSPrintf(HWND hwnd, LPCTSTR psz, BOOL fCompact)
{
    TCHAR szTemp[2048];
    TCHAR szTemp1[2048];
    TCHAR szPath[MAX_PATH];
    
    if (fCompact) 
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        lstrcpy(szPath, psz);
        PathCompactPath(NULL, szPath, (rc.right - rc.left)*3/4);
        psz = szPath;
    }
    GetWindowText(hwnd, szTemp, ARRAYSIZE(szTemp));
    wsprintf(szTemp1, szTemp, psz);
    SetWindowText(hwnd, szTemp1);
}



BOOL WINAPI Priv_Str_SetPtrW(WCHAR * UNALIGNED * ppwzCurrent, LPCWSTR pwzNew)
{
    LPWSTR pwzOld;
    LPWSTR pwzNewCopy = NULL;

    if (pwzNew)
    {
        int cchLength = lstrlenW(pwzNew);

        // alloc a new buffer w/ room for the null terminator
        pwzNewCopy = (LPWSTR) LocalAlloc(LPTR, (cchLength + 1) * SIZEOF(WCHAR));

        if (!pwzNewCopy)
            return FALSE;

        StrCpyNW(pwzNewCopy, pwzNew, cchLength + 1);
    }

    pwzOld = (LPWSTR) InterlockedExchangePointer((void * *)ppwzCurrent, pwzNewCopy);

    if (pwzOld)
        LocalFree(pwzOld);

    return TRUE;
}

// combines pidlParent with part of pidl, upto pidlNext, example:
//
// in:
//      pidlParent      [c:] [windows]
//      pidl                           [system] [foo.txt]
//      pidlNext                              --^
//
// returns:
//                      [c:] [windows] [system]
//

STDAPI_(LPITEMIDLIST) ILCombineParentAndFirst(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlNext)
{
    ULONG cbParent = ILGetSize(pidlParent);
    ULONG cbRest   = (ULONG)((ULONG_PTR)pidlNext - (ULONG_PTR)pidl);
    LPITEMIDLIST pidlNew = _ILCreate(cbParent + cbRest);
    if (pidlNew)
    {
        cbParent -= SIZEOF(pidlParent->mkid.cb);
        memcpy(pidlNew, pidlParent, cbParent);
        memcpy((BYTE *)pidlNew + cbParent, pidl, cbRest);
        ASSERT(_ILSkip(pidlNew, cbParent + cbRest)->mkid.cb == 0);
    }
    return pidlNew;
}


STDAPI_(LPTSTR) DumpPidl(LPCITEMIDLIST pidl)
{
#ifdef DEBUG
    static TCHAR szBuf[MAX_PATH];
    TCHAR szTmp[MAX_PATH];
    USHORT cb;
    LPTSTR pszT;

    szBuf[0] = 0;

    if (NULL == pidl)
    {
        lstrcatn(szBuf, TEXT("Empty pidl"), ARRAYSIZE(szBuf));
        return szBuf;
    }

    while (!ILIsEmpty(pidl))
    {
        cb = pidl->mkid.cb;
        wsprintf(szTmp, TEXT("cb:%x id:"), cb);
        lstrcatn(szBuf, szTmp, ARRAYSIZE(szBuf));

        switch (SIL_GetType(pidl) & SHID_TYPEMASK)
        {
        case SHID_ROOT:                pszT = TEXT("SHID_ROOT"); break;
        case SHID_ROOT_REGITEM:        pszT = TEXT("SHID_ROOT_REGITEM"); break;
        case SHID_COMPUTER:            pszT = TEXT("SHID_COMPUTER"); break;
        case SHID_COMPUTER_1:          pszT = TEXT("SHID_COMPUTER_1"); break;
        case SHID_COMPUTER_REMOVABLE:  pszT = TEXT("SHID_COMPUTER_REMOVABLE"); break;
        case SHID_COMPUTER_FIXED:      pszT = TEXT("SHID_COMPUTER_FIXED"); break;
        case SHID_COMPUTER_REMOTE:     pszT = TEXT("SHID_COMPUTER_REMOTE"); break;
        case SHID_COMPUTER_CDROM:      pszT = TEXT("SHID_COMPUTER_CDROM"); break;
        case SHID_COMPUTER_RAMDISK:    pszT = TEXT("SHID_COMPUTER_RAMDISK"); break;
        case SHID_COMPUTER_7:          pszT = TEXT("SHID_COMPUTER_7"); break;
        case SHID_COMPUTER_DRIVE525:   pszT = TEXT("SHID_COMPUTER_DRIVE525"); break;
        case SHID_COMPUTER_DRIVE35:    pszT = TEXT("SHID_COMPUTER_DRIVE35"); break;
        case SHID_COMPUTER_NETDRIVE:   pszT = TEXT("SHID_COMPUTER_NETDRIVE"); break;
        case SHID_COMPUTER_NETUNAVAIL: pszT = TEXT("SHID_COMPUTER_NETUNAVAIL"); break;
        case SHID_COMPUTER_C:          pszT = TEXT("SHID_COMPUTER_C"); break;
        case SHID_COMPUTER_D:          pszT = TEXT("SHID_COMPUTER_D"); break;
        case SHID_COMPUTER_REGITEM:    pszT = TEXT("SHID_COMPUTER_REGITEM"); break;
        case SHID_COMPUTER_MISC:       pszT = TEXT("SHID_COMPUTER_MISC"); break;
        case SHID_FS:                  pszT = TEXT("SHID_FS"); break;
        case SHID_FS_TYPEMASK:         pszT = TEXT("SHID_FS_TYPEMASK"); break;
        case SHID_FS_DIRECTORY:        pszT = TEXT("SHID_FS_DIRECTORY"); break;
        case SHID_FS_FILE:             pszT = TEXT("SHID_FS_FILE"); break;
        case SHID_FS_UNICODE:          pszT = TEXT("SHID_FS_UNICODE"); break;
        case SHID_FS_DIRUNICODE:       pszT = TEXT("SHID_FS_DIRUNICODE"); break;
        case SHID_FS_FILEUNICODE:      pszT = TEXT("SHID_FS_FILEUNICODE"); break;
        case SHID_NET:                 pszT = TEXT("SHID_NET"); break;
        case SHID_NET_DOMAIN:          pszT = TEXT("SHID_NET_DOMAIN"); break;
        case SHID_NET_SERVER:          pszT = TEXT("SHID_NET_SERVER"); break;
        case SHID_NET_SHARE:           pszT = TEXT("SHID_NET_SHARE"); break;
        case SHID_NET_FILE:            pszT = TEXT("SHID_NET_FILE"); break;
        case SHID_NET_GROUP:           pszT = TEXT("SHID_NET_GROUP"); break;
        case SHID_NET_NETWORK:         pszT = TEXT("SHID_NET_NETWORK"); break;
        case SHID_NET_RESTOFNET:       pszT = TEXT("SHID_NET_RESTOFNET"); break;
        case SHID_NET_SHAREADMIN:      pszT = TEXT("SHID_NET_SHAREADMIN"); break;
        case SHID_NET_DIRECTORY:       pszT = TEXT("SHID_NET_DIRECTORY"); break;
        case SHID_NET_TREE:            pszT = TEXT("SHID_NET_TREE"); break;
        case SHID_NET_REGITEM:         pszT = TEXT("SHID_NET_REGITEM"); break;
        case SHID_NET_PRINTER:         pszT = TEXT("SHID_NET_PRINTER"); break;
        default:                       pszT = TEXT("unknown"); break;
        }
        lstrcatn(szBuf, pszT, ARRAYSIZE(szBuf));

        if (SIL_GetType(pidl) & SHID_JUNCTION)
        {
            lstrcatn(szBuf, TEXT(", junction"), ARRAYSIZE(szBuf));
        }

        pidl = _ILNext(pidl);

        if (!ILIsEmpty(pidl))
        {
            lstrcatn(szBuf, TEXT("; "), ARRAYSIZE(szBuf));
        }
    }

    return szBuf;
#else
    return TEXT("");
#endif // DEBUG
}


STDAPI GetDomainWorkgroupIDList(LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    IShellFolder *psfDesktop;
    HRESULT hres = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hres))
    {
        TCHAR szName[MAX_PATH];

        lstrcpy(szName, TEXT("\\\\"));

        if (RegGetValueString(HKEY_LOCAL_MACHINE, 
                TEXT("SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName"),
                TEXT("ComputerName"), szName + 2, sizeof(szName) - 2 * sizeof(TCHAR)))
        {
            WCHAR wszName[MAX_PATH];

            SHTCharToUnicode(szName, wszName, ARRAYSIZE(wszName));

            hres = psfDesktop->ParseDisplayName(NULL, NULL, wszName, NULL, ppidl, NULL);
            if (SUCCEEDED(hres))
                ILRemoveLastID(*ppidl);
        }
        else
            hres = E_FAIL;

        psfDesktop->Release();
    }
    return hres;
}

HRESULT SaveShortcutInFolder(int csidl, LPTSTR pszName, IShellLink *psl)
{
    TCHAR szPath[MAX_PATH];

    HRESULT hres = SHGetFolderPath(NULL, csidl | CSIDL_FLAG_CREATE, NULL, 0, szPath);
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;

        hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
        if (SUCCEEDED(hres))
        {
            PathAppend(szPath, pszName);

            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(szPath, wszPath, ARRAYSIZE(wszPath));

            hres = ppf->Save(wszPath, TRUE);
            ppf->Release();
        }
    }
    return hres;
}

BOOL ShouldCreateNetHoodShortcuts(BOOL *pbWorkgroup)
{
#if WINNT
    BOOL bShould = FALSE;
    LPTSTR pszDomain = NULL;
    DWORD dwRest = SHRestricted(REST_NOCOMPUTERSNEARME);
    NETSETUP_JOIN_STATUS nsjs;

    // read the currently joined workgroup or domain, and create the link accordingly

    if ( NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &nsjs))
    {
        if ( dwRest )
            *pbWorkgroup = FALSE;
        else
            *pbWorkgroup = (nsjs == NetSetupWorkgroupName);

        HKEY hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        if (hkey)
        {       
            TCHAR szValue[80], szJoined[MAX_PATH];
            DWORD dwSize = SIZEOF(szValue);

            // encode the workgroup/domain into the string, so we know whats what.
            wnsprintf(szJoined, ARRAYSIZE(szJoined), TEXT("%d,%d,%s"), dwRest, nsjs, pszDomain);

            bShould = (SHQueryValueEx(hkey, TEXT("Last Domain"), NULL, NULL, szValue, &dwSize) != ERROR_SUCCESS)||
                      (lstrcmp(szValue, szJoined) != 0);

            if (bShould)
                RegSetValueEx(hkey, TEXT("Last Domain"), 0, REG_SZ, (BYTE *)szJoined, (lstrlen(szJoined) + 1) * sizeof(TCHAR));

            RegCloseKey(hkey);
        }
        NetApiBufferFree(pszDomain);
    }
    return bShould;
#else
    *pbWorkgroup = TRUE;    // BUGBUG, this is wrong
    return TRUE;            // BUGBUG, compute this as above (WINNT case)
#endif
}

STDAPI_(BOOL) CreateNetHoodShortcuts()
{
    BOOL bWorkgroup;
    TCHAR szName[MAX_PATH];

    if (ShouldCreateNetHoodShortcuts(&bWorkgroup))
    {
        if (bWorkgroup)
        {
            LPITEMIDLIST pidl;
            HRESULT hres = GetDomainWorkgroupIDList(&pidl);
            if (SUCCEEDED(hres))
            {
                IShellLink *psl;
                hres = CoCreateInstance(CLSID_FolderShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
                if (SUCCEEDED(hres))
                {

                    psl->SetIDList(pidl);

                    LoadString(g_hinst, IDS_COMPUTERSNEARMETIP, szName, ARRAYSIZE(szName));
                    psl->SetDescription(szName);

                    LoadString(g_hinst, IDS_COMPUTERSNEARMELNK, szName, ARRAYSIZE(szName));

                    SaveShortcutInFolder(CSIDL_NETHOOD, szName, psl);
                    psl->Release();
                }
                ILFree(pidl);
            }
        }
        else
        {
            TCHAR szPath[MAX_PATH+1] = { 0 };

            HRESULT hres = SHGetFolderPath(NULL, CSIDL_NETHOOD | CSIDL_FLAG_CREATE, NULL, 0, szPath);
            if (SUCCEEDED(hres))
            {
                SHFILEOPSTRUCT shfo = { NULL, 
                                        FO_DELETE, 
                                        szPath, 
                                        NULL, 
                                        FOF_SILENT|FOF_NOCONFIRMATION, 
                                        FALSE,
                                        NULL, 
                                        NULL };

                LoadString(g_hinst, IDS_COMPUTERSNEARMELNK, szName, ARRAYSIZE(szName));
                PathAppend(szPath, szName);

                SHFileOperation(&shfo);
            }
        }
    }
    return TRUE;
}



// TrackPopupMenu does not work, if the hwnd does not have
// the input focus. We believe this is a bug in USER

STDAPI_(BOOL) SHTrackPopupMenu(HMENU hmenu, UINT wFlags, int x, int y,
                                 int wReserved, HWND hwndOwner, LPCRECT lprc)
{
    int iRet = FALSE;
    HWND hwndDummy = CreateWindow(c_szStatic, NULL,
                           0, x, y, 1, 1, HWND_DESKTOP,
                           NULL, HINST_THISDLL, NULL);
    if (hwndDummy)
    {
        HWND hwndPrev = GetForegroundWindow();  // to restore

        SetForegroundWindow(hwndDummy);
        SetFocus(hwndDummy);
        iRet = TrackPopupMenu(hmenu, wFlags, x, y, wReserved, hwndDummy, lprc);

        //
        // We MUST unlock the destination window before changing its Z-order.
        //
        DAD_DragLeave();

        if (iRet && hwndOwner)
        {
            // non-cancel item is selected. Make the hwndOwner foreground.
            SetForegroundWindow(hwndOwner);
            SetFocus(hwndOwner);
        }
        else
        {
            // The user canceled the menu.
            // Restore the previous foreground window (before destroying hwndDummy).
            if (hwndPrev)
                SetForegroundWindow(hwndPrev);
        }

        DestroyWindow(hwndDummy);
    }

    return iRet;
}

//
// user does not support pop-up only menu.
//
STDAPI_(HMENU) SHLoadPopupMenu(HINSTANCE hinst, UINT id)
{
    HMENU hmenuParent = LoadMenu(hinst, MAKEINTRESOURCE(id));
    if (hmenuParent) 
    {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }
    return NULL;
}

STDAPI_(void) PathToAppPathKey(LPCTSTR pszPath, LPTSTR pszKey, int cchKey)
{
    // Use the szTemp variable of pseem to build key to the programs specific
    // key in the registry as well as other things...
    StrCpyN(pszKey, c_szAppPaths, cchKey);
    lstrcatn(pszKey, TEXT("\\"), cchKey);
    lstrcatn(pszKey, PathFindFileName(pszPath), cchKey);

    // Currently we will only look up .EXE if an extension is not
    // specified
    if (*PathFindExtension(pszKey) == 0)
    {
        lstrcatn(pszKey, c_szDotExe, cchKey);
    }
}

// out:
//      pszResultPath   assumed to be MAX_PATH in length

STDAPI_(BOOL) PathToAppPath(LPCTSTR pszPath, LPTSTR pszResultPath)
{
    TCHAR szRegKey[MAX_PATH];
    LONG cbData = MAX_PATH * SIZEOF(TCHAR);

    VDATEINPUTBUF(pszResultPath, TCHAR, MAX_PATH);
    
    PathToAppPathKey(pszPath, szRegKey, ARRAYSIZE(szRegKey));

    return SHRegQueryValue(HKEY_LOCAL_MACHINE, szRegKey, pszResultPath, &cbData) == ERROR_SUCCESS;
}

HWND GetTopParentWindow(HWND hwnd)
{
    if (IsWindow(hwnd))
    {
        HWND hwndParent;
        while (NULL != (hwndParent = GetWindow(hwnd, GW_OWNER)))
            hwnd = hwndParent;
    }
    else
        hwnd = NULL;

    return hwnd;
}

//----------------------------------------------------------------------------
// SHProcessMessagesUntilEvent:
// this does a message loop until an event or a timeout occurs
//
DWORD SHProcessMessagesUntilEvent(HWND hwnd, HANDLE hEvent, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwEndTime = GetTickCount() + dwTimeout;
    LONG lWait = (LONG)dwTimeout;
    DWORD dwReturn;

    for (;;)
    {
        dwReturn = MsgWaitForMultipleObjects(1, &hEvent,
                FALSE, lWait, QS_ALLINPUT);

        // were we signalled or did we time out?
        if (dwReturn != (WAIT_OBJECT_0 + 1))
        {
            break;
        }

        // we woke up because of messages.
        while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            ASSERT(msg.message != WM_QUIT);
            TranslateMessage(&msg);
            if (msg.message == WM_SETCURSOR) {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
            } else {
                DispatchMessage(&msg);
            }
        }

        // calculate new timeout value
        if (dwTimeout != INFINITE)
        {
            lWait = (LONG)dwEndTime - GetTickCount();
        }
    }

    return dwReturn;
}

BOOL PathIsShortcut(LPCTSTR pszFile)
{
    if (PathIsLnk(pszFile))
    {
        return TRUE;    // quick short-circut for perf
    }
    else
    {
        SHFILEINFO sfi;

        sfi.dwAttributes = SFGAO_LINK;  // setup in param (SHGFI_ATTR_SPECIFIED requires this)
        return SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES | SHGFI_ATTR_SPECIFIED) && 
            (sfi.dwAttributes & SFGAO_LINK);
    }
}


HRESULT SHGetAssociations(LPCITEMIDLIST pidl, void **ppvQueryAssociations)
{
    HRESULT hr = SHGetUIObjectFromFullPIDL(pidl, NULL, IID_IQueryAssociations, ppvQueryAssociations);

    if (FAILED(hr))
    {
        //  this means that the folder doesnt support 
        //  the IQueryAssociations.  so we will
        //  just check to see if this is a folder.
        //
        //  some shellextensions mask the FILE system
        //  and want the file system associations to show
        //  up for their items.  so we will try a simple pidl
        //
        TCHAR sz[MAX_PATH];
        ULONG rgfAttrs = SFGAO_FOLDER | SFGAO_BROWSABLE | SFGAO_FILESYSTEM;
        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, sz, ARRAYSIZE(sz), &rgfAttrs)))
        {
            if (rgfAttrs & SFGAO_FILESYSTEM)
            {
                WIN32_FIND_DATA fd = {0};
                LPITEMIDLIST pidlSimple;

                if (rgfAttrs & SFGAO_FOLDER)
                    fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

                if (SUCCEEDED(SHSimpleIDListFromFindData(sz, &fd, &pidlSimple)))
                {
                    hr = SHGetUIObjectFromFullPIDL(pidlSimple, NULL, IID_IQueryAssociations, ppvQueryAssociations);
                    ILFree(pidlSimple);
                }
            }

            if (FAILED(hr) && (rgfAttrs & (SFGAO_FOLDER | SFGAO_BROWSABLE)))
            {
                IQueryAssociations *pqa;
                if (SUCCEEDED(AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa))))
                {
                    hr = pqa->Init(0, L"Folder", NULL, NULL);

                    if (SUCCEEDED(hr))
                        *ppvQueryAssociations = (void *)pqa;
                    else
                        pqa->Release();
                }
            }
        }
    }
    return hr;
}

//
//  SHGetAssocKeys() retrieves an array of class keys 
//  from a pqa.
//
//  if the caller is just interested in the primary class key,
//  call with cKeys == 1.  the return value is the number of keys
//  inserted into the array.
//
DWORD SHGetAssocKeys(IQueryAssociations *pqa, HKEY *rgKeys, DWORD cKeys)
{
    ASSERT(cKeys);

    if (SUCCEEDED(pqa->GetKey(0, ASSOCKEY_CLASS, NULL, rgKeys)))
    {
        DWORD cBaseKeys = 0;
        //  we have the class key
        while (--cKeys)
        {
            //  get the base class keys
            //  cBaseKey indicates depth of base here
            if (FAILED(pqa->GetKey(0, ASSOCKEY_BASECLASS, MAKEINTRESOURCEW(cBaseKeys), &(rgKeys[cBaseKeys + 1]))))
                break;

            ASSERT(rgKeys[cBaseKeys + 1]);
            cBaseKeys++;
        }

        return cBaseKeys + 1;
    }

    return 0;
}
        

// NOTE:
//  this API returns a win32 file system path for the item in the name space
//  ideally this is the parsing (SFGAO_FORPARSING) name for the item when the items has SFGAO_FILESYSTEM
//  on it. BUT, win95 would return paths for things that did not have SFGAO_FILESYSTEM.
//  that includes file system junction points (.zip/.cab files, Folder.{guid} things)
//

STDAPI_(BOOL) SHGetPathFromIDListEx(LPCITEMIDLIST pidl, LPTSTR pszPath, UINT uOpts)
{
    VDATEINPUTBUF(pszPath, TCHAR, MAX_PATH);
    HRESULT hr;

    *pszPath = 0;    // zero output buffer

    if (!pidl) 
        return FALSE;   // bad params

    if (ILIsEmpty(pidl))
    {
        // desktop special case because we can not depend on the desktop
        // returning a file system path (APP compat)
        hr = SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, pszPath);
        if (hr == S_FALSE)
            hr = E_FAIL;
    }
    else
    {
        // HRESULT hrInit = SHCoInitialize();   // only do this is some ISV app reqires this

        IShellFolder *psf;
        LPCITEMIDLIST pidlLast;
        hr = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlLast);
        if (SUCCEEDED(hr))
        {
            STRRET str;
            hr = psf->GetDisplayNameOf(pidlLast, SHGDN_FORPARSING, &str);
            if (SUCCEEDED(hr))
                hr = StrRetToBuf(&str, pidlLast, pszPath, MAX_PATH);

            if (SUCCEEDED(hr))
            {
                DWORD dwAttributes = SFGAO_FILESYSTEM;
                hr = psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlLast, &dwAttributes);
                if (SUCCEEDED(hr) && !(dwAttributes & SFGAO_FILESYSTEM))
                {
#if 0
                    // here is where we simulate the old behavior for
                    // items that don't have SFGAO_FILESYSTEM, if it smells like
                    // a path lets keep it
                    if ((PathGetDriveNumber(pszPath) == -1) &&
                        !PathIsUNC(pszPath))
                    {
                        hr = E_FAIL;      // not a file system guy, fail this
                        *pszPath = 0;
                    }
#elif 0
                    hr = E_FAIL;      // not a file system guy, fail this
                    *pszPath = 0;
#else
                    CLSID clsid;
                    
                    hr = IUnknown_GetClassID(psf, &clsid);
                    if (FAILED(hr) || clsid != CLSID_NetworkServer)
                    {
                        hr = E_FAIL;
                        *pszPath = 0;
                    }
#endif
                }
            }
            psf->Release();
        }

        // SHCoUninitialize(hrInit);
    }

    if (SUCCEEDED(hr) && (uOpts & GPFIDL_ALTNAME))
    {
        TCHAR szShort[MAX_PATH];
        if (GetShortPathName(pszPath, szShort, ARRAYSIZE(szShort)))
        {
            lstrcpy(pszPath, szShort);
        }
    }
    return SUCCEEDED(hr);
}

STDAPI_(BOOL) SHGetPathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    //
    // We simply calls EX function with uOpts = 0 ie no special options.
    //
    return SHGetPathFromIDListEx(pidl, pszPath, 0);
}

#ifdef UNICODE

#define CBHUMHEADER     14
inline BOOL _DoHummingbirdHack(LPCITEMIDLIST pidl)
{
    static const char rgchHum[] = {(char)0xe8, (char)0x03, 0,0,0,0,0,0,(char)0x10,0};

    return (pidl && pidl->mkid.cb > CBHUMHEADER) && ILIsEmpty(_ILNext(pidl))
    && (0 == memcmp(_ILSkip(pidl, 4), rgchHum, sizeof(rgchHum)))
    && GetModuleHandle(TEXT("heshell"));
}

STDAPI_(BOOL) SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath)
{
    WCHAR wszPath[MAX_PATH];

    *pszPath = 0;  // Assume error

    if (SHGetPathFromIDListW(pidl, wszPath))
    {
        // Thunk the output result string back to ANSI.  If the conversion fails,
        // or if the default char is used, we fail the API call.

        if (0 == WideCharToMultiByte(CP_ACP, 0, wszPath, -1, pszPath, MAX_PATH, NULL, NULL))
        {
            return FALSE;  // BUGBUG (DavePl) Note failure only due to text thunking
        }
        return TRUE;        // warning, word perfect tests explictly for TRUE (== TRUE)
    }
    else if (_DoHummingbirdHack(pidl))
    {
        //
        //  HACKHACK - hummingbird really sucks here because we used to suck more - Zekel 7-OCT-99
        //  hummingbird's shell extension passes us a pidl that is relative to 
        //  itself.  however SHGetPathFromIDList() used to try some weird stuff
        //  when it didnt recognize the pidl.  in this case it would combine the
        //  relative pidl with the CSIDL_DESKTOPDIRECTORY pidl, and then ask for
        //  the path.  due to crappy parameter validation we would actually return
        //  back a path with a string from inside their relative pidl.  the path 
        //  of course doesnt exist at all, but hummingbird fails to initialize its 
        //  subfolders if we fail here.  they dont do anything with the path except 
        //  look for a slash.  (if the slash is missing they will fault.)
        //
        SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, pszPath);
        PathAppendA(pszPath, (LPCSTR)_ILSkip(pidl, CBHUMHEADER));
        return TRUE;
    }
    
    return FALSE;
}

#else

STDAPI_(BOOL) SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath)
{
    TCHAR szPathName[MAX_PATH];

    if (SHGetPathFromIDListA(pidl, szPathName))
    {
        SHAnsiToUnicode(szPathName, pszPath, MAX_PATH);
        return TRUE;
    }

    pszPath[0] = 0;
    return FALSE;
}

#endif

//
//  Race-condition-free version.
//
STDAPI_(HANDLE) SHGetCachedGlobalCounter(HANDLE *phCache, const GUID *pguid)
{
    if (!*phCache) 
    {
        HANDLE h = SHGlobalCounterCreate(*pguid);
        if (SHInterlockedCompareExchange(phCache, h, 0))
        {
            // some other thread raced with us, throw away our copy
            SHGlobalCounterDestroy(h);
        }
    }
    return *phCache;
}

//
//  Race-condition-free version.
//
STDAPI_(void) SHDestroyCachedGlobalCounter(HANDLE *phCache)
{
    HANDLE h = InterlockedExchangePointer(phCache, NULL);
    if (h)
    {
        SHGlobalCounterDestroy(h);
    }
}

//
//  Use this function when you want to lazy-create and cache some object.
//  It's safe in the multithreaded case where two people lazy-create the
//  object and both try to put it into the cache.
//
STDAPI_(void) SetUnknownOnSuccess(HRESULT hres, IUnknown *punk, IUnknown **ppunkToSet)
{
    if (SUCCEEDED(hres))
    {
        if (SHInterlockedCompareExchange((void **)ppunkToSet, punk, 0))
            punk->Release();  // race, someone did this already
    }
}

//
//  Create and cache a tracking folder.
//
//  pidlRoot    = where the folder should reside; can be MAKEINTIDLIST(csidl).
//  csidlTarget = the csidl we should track, CSIDL_FLAG_CREATE is allowed
//  ppsfOut     = Receives the cached folder
//
//  If there is already a folder in the cache, succeeds vacuously.
//
STDAPI SHCacheTrackingFolder(LPCITEMIDLIST pidlRoot, int csidlTarget, IShellFolder2 **ppsfCache)
{
    HRESULT hres = S_OK;

    if (!*ppsfCache)
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};
        IShellFolder2 *psf;
        LPITEMIDLIST pidl;

        pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?
        pfti.csidl = csidlTarget | CSIDL_FLAG_PFTI_TRACKTARGET;

        if (IS_INTRESOURCE(pidlRoot))
        {
            hres = SHGetFolderLocation(NULL, PtrToInt(pidlRoot), NULL, 0, &pidl);
        }
        else
        {
            pidl = const_cast<LPITEMIDLIST>(pidlRoot);
        }

        if (SUCCEEDED(hres))
            hres = CFSFolder_CreateFolder(NULL, pidl, &pfti, IID_IShellFolder2, (LPVOID *)&psf);

        SetUnknownOnSuccess(hres, psf, (IUnknown **)ppsfCache);

        if (pidl != pidlRoot)
            ILFree(pidl);

    }
    return hres;
}

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject)
{
    RECT    rc;
    POINT   pt;

    GetWindowRect(hwndTarget, &rc);

    //
    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    DAD_DragEnterEx2(hwndTarget, pt, pdtObject);
    return;
}

void _DragMove(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    POINT pt;

    GetWindowRect(hwndTarget, &rc);

    //
    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    DAD_DragMove(pt);
    return; 
}

STDAPI FindFileOrFolders_GetDefaultSearchGUID(IShellFolder2 *psf, LPGUID pGuid)
{
    if (SHRestricted(REST_NOFIND))
    {
        *pGuid = GUID_NULL;
        return E_NOTIMPL;
    }

    *pGuid = SRCID_SFileSearch;
    return S_OK;
}


// Helper function to save IPersistHistory stream for you
//
HRESULT SavePersistHistory(IUnknown* punk, IStream* pstm)
{
    IPersistHistory* pPH;
    HRESULT hres = punk->QueryInterface(IID_IPersistHistory, (void**)&pPH);
    if (SUCCEEDED(hres))
    {
        CLSID clsid;
        hres = pPH->GetClassID(&clsid);
        if (SUCCEEDED(hres))
        {
            hres = IStream_Write(pstm, &clsid, SIZEOF(clsid));
            if (SUCCEEDED(hres))
            {
                hres = pPH->SaveHistory(pstm);
            }
        }
        pPH->Release();
    }
    return hres;
}

STDAPI Stream_WriteStringA(IStream *pstm, LPCSTR psz)
{
    SHORT cch = (SHORT)lstrlenA(psz);
    HRESULT hres = pstm->Write(&cch, SIZEOF(cch), NULL);
    if (SUCCEEDED(hres))
        hres = pstm->Write(psz, cch * SIZEOF(*psz), NULL);

    return hres;
}

STDAPI Stream_WriteStringW(IStream *pstm, LPCWSTR psz)
{
    SHORT cch = (SHORT)lstrlenW(psz);
    HRESULT hres = pstm->Write(&cch, SIZEOF(cch), NULL);
    if (SUCCEEDED(hres))
        hres = pstm->Write(psz, cch * SIZEOF(*psz), NULL);

    return hres;
}

STDAPI Stream_WriteString(IStream *pstm, LPCTSTR psz, BOOL bWideInStream)
{
    HRESULT hres;
    if (bWideInStream)
    {
        WCHAR wszBuf[MAX_PATH];
        SHTCharToUnicode(psz, wszBuf, ARRAYSIZE(wszBuf));
        hres = Stream_WriteStringW(pstm, wszBuf);
    }
    else
    {
        CHAR szBuf[MAX_PATH];
        SHTCharToAnsi(psz, szBuf, ARRAYSIZE(szBuf));
        hres = Stream_WriteStringA(pstm, szBuf);
    }
    return hres;
}

STDAPI Stream_ReadStringA(IStream *pstm, LPSTR pszBuf, UINT cchBuf)
{
    *pszBuf = 0;

    USHORT cch;
    HRESULT hres = pstm->Read(&cch, SIZEOF(cch), NULL);   // size of data
    if (SUCCEEDED(hres))
    {
        if (cch >= (USHORT)cchBuf)
        {
            DebugMsg(DM_TRACE, TEXT("truncating string read(%d to %d)"), cch, cchBuf);
            cch = (USHORT)cchBuf - 1;   // leave room for null terminator
        }

        hres = pstm->Read(pszBuf, cch, NULL);
        if (SUCCEEDED(hres))
            pszBuf[cch] = 0;      // add NULL terminator
    }
    return hres;
}

STDAPI Stream_ReadStringW(IStream *pstm, LPWSTR pwszBuf, UINT cchBuf)
{
    *pwszBuf = 0;

    USHORT cch;
    HRESULT hres = pstm->Read(&cch, SIZEOF(cch), NULL);   // size of data
    if (SUCCEEDED(hres))
    {
        if (cch >= (USHORT)cchBuf)
        {
            DebugMsg(DM_TRACE, TEXT("truncating string read(%d to %d)"), cch, cchBuf);
            cch = (USHORT)cchBuf - 1;   // leave room for null terminator
        }

        hres = pstm->Read(pwszBuf, cch * SIZEOF(*pwszBuf), NULL);
        if (SUCCEEDED(hres))
            pwszBuf[cch] = 0;      // add NULL terminator
    }
    return hres;
}

STDAPI Stream_ReadString(IStream *pstm, LPTSTR psz, UINT cchBuf, BOOL bWideInStream)
{
    HRESULT hres;
    if (bWideInStream)
    {
#ifdef UNICODE
        hres = Stream_ReadStringW(pstm, psz, cchBuf);
#else
        WCHAR wszBuf[MAX_PATH];
        hres = Stream_ReadStringW(pstm, wszBuf, ARRAYSIZE(wszBuf));
        if (SUCCEEDED(hres))
            SHUnicodeToTChar(wszBuf, psz, cchBuf);
#endif
    }
    else
    {
#ifdef UNICODE
        CHAR szAnsiBuf[MAX_PATH];
        hres = Stream_ReadStringA(pstm, szAnsiBuf, ARRAYSIZE(szAnsiBuf));
        if (SUCCEEDED(hres))
            SHAnsiToUnicode(szAnsiBuf, psz, cchBuf);
#else
        hres = Stream_ReadStringA(pstm, psz, cchBuf);
#endif
    }
    return hres;
}

STDAPI Str_SetFromStream(IStream *pstm, LPTSTR *ppsz, BOOL bWideInStream)
{
    TCHAR szBuf[MAX_PATH];
    HRESULT hres = Stream_ReadString(pstm, szBuf, ARRAYSIZE(szBuf), bWideInStream);
    if (SUCCEEDED(hres))
        if (!Str_SetPtr(ppsz, szBuf)) hres = E_OUTOFMEMORY;
    return hres;
}
LPSTR ThunkStrToAnsi(LPCWSTR pszW, CHAR *pszA, UINT cchA)
{
    if (pszW)
    {
        SHUnicodeToAnsi(pszW, pszA, cchA);
        return pszA;
    }
    return NULL;
}

LPWSTR ThunkStrToWide(LPCSTR pszA, LPWSTR pszW, DWORD cchW)
{
    if (pszA)
    {
        SHAnsiToUnicode(pszA, pszW, cchW);
        return pszW;
    }
    return NULL;
}

#define ThunkSizeAnsi(pwsz)       WideCharToMultiByte(CP_ACP,0,(pwsz),-1,NULL,0,NULL,NULL)
#define ThunkSizeWide(psz)       MultiByteToWideChar(CP_ACP,0,(psz),-1,NULL,0)

STDAPI SEI2ICIX(LPSHELLEXECUTEINFO pei, LPCMINVOKECOMMANDINFOEX pici, void **ppvFree)
{
    HRESULT hr = S_OK;
    
    *ppvFree = NULL;
    ZeroMemory(pici, SIZEOF(CMINVOKECOMMANDINFOEX));
    
    pici->cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
    pici->fMask = (pei->fMask & SEE_VALID_CMIC_BITS);
    pici->hwnd = pei->hwnd;
    pici->nShow = pei->nShow;
    pici->dwHotKey = pei->dwHotKey;
    pici->lpTitle = NULL;

    //  the pei->hIcon can have multiple meanings...
    if ((pei->fMask & SEE_MASK_HMONITOR) && pei->hIcon)
    {
        //  in this case we want the hMonitor to 
        //  make it through to where the pcm calls shellexec 
        //  again.
        RECT rc;
        if (GetMonitorRect((HMONITOR)pei->hIcon, &rc))
        {
            //  default to the top left corner of 
            //  the monitor.  it is just the monitor 
            //  that is relevant here.
            pici->ptInvoke.x = rc.left;
            pici->ptInvoke.y = rc.top;
            pici->fMask |= CMIC_MASK_PTINVOKE;
        }
    }
    else 
        pici->hIcon = pei->hIcon;
        

#ifdef UNICODE
    pici->lpVerbW       = pei->lpVerb;
    pici->lpParametersW = pei->lpParameters;
    pici->lpDirectoryW  = pei->lpDirectory;


    //  we need to thunk the strings down.  first get the length of all the buffers
    DWORD cbVerb = ThunkSizeAnsi(pei->lpVerb);
    DWORD cbParameters = ThunkSizeAnsi(pei->lpParameters);
    DWORD cbDirectory = ThunkSizeAnsi(pei->lpDirectory);
    DWORD cbTotal = cbVerb + cbParameters + cbDirectory;

    if (cbTotal)
    {
        *ppvFree = LocalAlloc(LPTR, cbVerb + cbParameters + cbDirectory);
        
        if (*ppvFree)
        {
            LPSTR pch = (LPSTR) *ppvFree;
            
            pici->lpVerb = ThunkStrToAnsi(pei->lpVerb, pch, cbVerb);
            pch += cbVerb;
            pici->lpParameters  = ThunkStrToAnsi(pei->lpParameters, pch, cbParameters);
            pch += cbParameters;
            pici->lpDirectory   = ThunkStrToAnsi(pei->lpDirectory, pch, cbDirectory);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    pici->fMask |= CMIC_MASK_UNICODE;
    
#else
    pici->lpVerb       = pei->lpVerb;
    pici->lpParameters = pei->lpParameters;
    pici->lpDirectory  = pei->lpDirectory;
#endif

    return hr;
}

STDAPI ICIX2SEI(LPCMINVOKECOMMANDINFOEX pici, LPSHELLEXECUTEINFO pei)
{   
    //  perhaps we should allow just plain ici's, and do the thunk in here, but 
    //  it looks like all the callers want to do the thunk themselves...
    //  HRESULT hr = S_OK;
    
    ZeroMemory(pei, SIZEOF(SHELLEXECUTEINFO));
    pei->cbSize = SIZEOF(SHELLEXECUTEINFO);
    pei->fMask = pici->fMask & SEE_VALID_CMIC_BITS;

    // If we are doing this assync, than we will abort this thread
    // as soon as the shellexecute completes.  If the app holds open
    // a dde conversation, this may hang them.  This happens on W95 base
    // with winword95
    if (pici->fMask & SEE_MASK_ASYNCOK)
        pei->fMask |= SEE_MASK_FLAG_DDEWAIT;
        
    pei->hwnd = pici->hwnd;
    pei->nShow = pici->nShow;
    pei->dwHotKey = pici->dwHotKey;

    if (pici->fMask & CMIC_MASK_ICON)
        pei->hIcon = pici->hIcon;
    else if (pici->fMask & CMIC_MASK_PTINVOKE)
    {
        pei->hIcon = (HANDLE)MonitorFromPoint(pici->ptInvoke, MONITOR_DEFAULTTONEAREST);
        pei->fMask |= SEE_MASK_HMONITOR;
    }
    
#ifdef UNICODE
    //  we assume that there will always be the UNICODE params
    //  for us to use here if we are compiled UNICODE.
    ASSERT(pici->fMask & CMIC_MASK_UNICODE);

    pei->lpParameters = pici->lpParametersW;
    pei->lpDirectory  = pici->lpDirectoryW;

    if (!IS_INTRESOURCE(pici->lpVerbW))
        pei->lpVerb   = pici->lpVerbW;

    //  BUGBUG:  why does the title equate to the class?? - ZekeL - 27-MAY-98
    if (pici->fMask & CMIC_MASK_HASTITLE)
        pei->lpClass      = pici->lpTitleW;
#else
    pei->lpParameters = pici->lpParameters;
    pei->lpDirectory  = pici->lpDirectory;

    if (!IS_INTRESOURCE(pici->lpVerb))
        pei->lpVerb   = pici->lpVerb;

    //  BUGBUG:  why does the title equate to the class?? - ZekeL - 27-MAY-98
    if (pici->fMask & CMIC_MASK_HASTITLE)
        pei->lpClass      = pici->lpTitle;
#endif

    //  if we have to do any thunking in here, we 
    //  will have a real return hr.
    return S_OK;
}

STDAPI ICI2ICIX(LPCMINVOKECOMMANDINFO piciIn, LPCMINVOKECOMMANDINFOEX piciOut, void **ppvFree)
{
    HRESULT hr = S_OK;
    ASSERT(piciIn->cbSize >= SIZEOF(CMINVOKECOMMANDINFO));
    *ppvFree = NULL;

    ZeroMemory(piciOut, SIZEOF(CMINVOKECOMMANDINFOEX));
    memcpy(piciOut, piciIn, min(SIZEOF(CMINVOKECOMMANDINFOEX), piciIn->cbSize));
    piciOut->cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);

    
#ifdef UNICODE
    //  if the UNICODE params arent there, we must put them there
    if (!(piciIn->cbSize >= CMICEXSIZE_NT4) || !(piciIn->fMask & CMIC_MASK_UNICODE))
    {
        DWORD cchDirectory = ThunkSizeWide(piciOut->lpDirectory);
        DWORD cchTitle = ThunkSizeWide(piciOut->lpTitle);
        DWORD cchParameters = ThunkSizeWide(piciOut->lpParameters);
        DWORD cchVerb = 0;
        if (!IS_INTRESOURCE(piciOut->lpVerb))
            cchVerb = ThunkSizeWide(piciOut->lpVerb);

        DWORD cchTotal = (cchDirectory + cchTitle + cchVerb + cchParameters);

        if (cchTotal)
        {
            *ppvFree = LocalAlloc(LPTR, SIZEOF(WCHAR) * cchTotal);
            
            if (*ppvFree)
            {
                LPWSTR pch = (LPWSTR) *ppvFree;
                piciOut->lpDirectoryW = ThunkStrToWide(piciOut->lpDirectory, pch, cchDirectory);
                pch += cchDirectory;
                piciOut->lpTitleW = ThunkStrToWide(piciOut->lpTitle, pch, cchTitle);
                pch += cchTitle;
                piciOut->lpParametersW = ThunkStrToWide(piciOut->lpParameters, pch, cchParameters);
                pch += cchParameters;

                //only thunk if it is a string...
                if (!IS_INTRESOURCE(piciOut->lpVerb))
                    piciOut->lpVerbW = ThunkStrToWide(piciOut->lpVerb, pch, cchVerb);
                else
                    piciOut->lpVerbW = (LPCWSTR)piciOut->lpVerb;
            }
            else 
                hr = E_OUTOFMEMORY;
        }

        piciOut->fMask |= CMIC_MASK_UNICODE;
    }
#endif

    return hr;
}


// Helper function to convert Ansi string to allocated BSTR
STDAPI_(BSTR) SysAllocStringA(LPCSTR psz)
{
    WCHAR wsz[INFOTIPSIZE];  // assumes INFOTIPSIZE number of chars max

    SHAnsiToUnicode(psz, wsz, ARRAYSIZE(wsz));
    return SysAllocString(wsz);

}


IProgressDialog * CProgressDialog_CreateInstance(UINT idTitle, UINT idAnimation, HINSTANCE hAnimationInst)
{
    IProgressDialog * ppd;
    
    if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void **)&ppd)))
    {
        WCHAR wzTitle[MAX_PATH];

        EVAL(SUCCEEDED(ppd->SetAnimation(hAnimationInst, idAnimation)));
        if (EVAL(LoadStringW(HINST_THISDLL, idTitle, wzTitle, ARRAYSIZE(wzTitle))))
            EVAL(SUCCEEDED(ppd->SetTitle(wzTitle)));
    }

    return ppd;
}

STDAPI GetCurFolderImpl(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl)
{
    if (pidl)
        return SHILClone(pidl, ppidl);

    *ppidl = NULL;      
    return S_FALSE; // success but empty
}


//
// converts a simple PIDL to a real PIDL by converting to display name and then
// reparsing the name
//
STDAPI SHGetRealIDL(IShellFolder *psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST *ppidlReal)
{
    *ppidlReal = NULL;      // clear output

    STRRET str;
    HRESULT hr = IShellFolder_GetDisplayNameOf(psf, pidlSimple, SHGDN_FORPARSING | SHGDN_INFOLDER, &str, 0);
    if (SUCCEEDED(hr))
    {
        WCHAR szPath[MAX_PATH];
        hr = StrRetToBufW(&str, pidlSimple, szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            DWORD dwAttrib = SFGAO_FILESYSTEM;
            if (SUCCEEDED(psf->GetAttributesOf(1, &pidlSimple, &dwAttrib)) && !(dwAttrib & SFGAO_FILESYSTEM))
            {
                // not a file sys object, some name spaces (WinCE) support
                // parse, but don't do a good job, in this case
                // return the input as the output
                hr = SHILClone(pidlSimple, ppidlReal);
            }
            else
            {
                hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, szPath, NULL, ppidlReal, NULL);
                if (E_INVALIDARG == hr || E_NOTIMPL == hr)
                {
                    // name space does not support parse, assume pidlSimple is OK
                    hr = SHILClone(pidlSimple, ppidlReal);
                }
            }
        }
    }
    return hr;
}

//  trial and error has shown that
//  16k is a good number for FTP,
//  thus we will use that as our default buffer
#define CBOPTIMAL    (16 * 1024)

STDAPI CopyStreamUI(IStream *pstmSrc, IStream *pstmDest, IProgressDialog *pdlg)
{
    HRESULT hr = E_FAIL;
    ULONGLONG ullMax;

    if (pdlg)
    {
        STATSTG stat = {0};
        if (FAILED(pstmSrc->Stat(&stat, STATFLAG_NONAME)))
            pdlg = NULL;
        else
            ullMax = stat.cbSize.QuadPart;
    }

    if (!pdlg)
    {
        const static ULARGE_INTEGER s_ulMax = {-1, -1};
        hr = pstmSrc->CopyTo(pstmDest, s_ulMax, NULL, NULL);

        //  BUBBUGREMOVE - URLMON has bug which breaks CopyTo() - Zekel
        //  fix URLMON and then we can remove this garbage.
        //  so we will fake it here
    }
    
    if (FAILED(hr))
    {
        // try doing it by hand
        void *pv = LocalAlloc(LPTR, CBOPTIMAL);
        BYTE buf[1024];
        ULONG cbBuf, cbRead, cbBufReal;
        ULONGLONG ullCurr = 0;
        
        //  need to reset the streams,
        //  because CopyTo() doesnt guarantee any kind 
        //  of state
        IStream_Reset(pstmSrc);
        IStream_Reset(pstmDest);

        //  if we werent able to get the 
        //  best size, just use a little stack space :)
        if (pv)
            cbBufReal = CBOPTIMAL;
        else
        {
            pv = buf;
            cbBufReal = SIZEOF(buf);
        }

        cbBuf = cbBufReal;
        while (SUCCEEDED(pstmSrc->Read(pv, cbBuf, &cbRead)))
        {
            if (pdlg)
            {
                if (pdlg->HasUserCancelled())
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    break;
                }

                ullCurr += cbBuf;

                //  BUGBUG - urlmon doesnt always fill in the correct value for cbBuf returned
                //  so we need to make sure we dont pass bigger curr than max
                pdlg->SetProgress64(min(ullCurr, ullMax), ullMax);

            }
            
            if (!cbRead)
            {
                hr = S_OK;
                break;
            }

            hr = IStream_Write(pstmDest, pv, cbRead);
            if (S_OK != hr)
                break;  // failure!

            cbBuf = cbBufReal;
        }

        if (pv != buf)
            LocalFree(pv);
    }
    
    return hr;
            
}

STDAPI CopyStream(IStream *pstmSrc, IStream *pstmDest)
{
    return CopyStreamUI(pstmSrc, pstmDest, NULL);
}

STDAPI_(BOOL) IsWindowInProcess(HWND hwnd)
{
    DWORD idProcess;
    
    GetWindowThreadProcessId(hwnd, &idProcess);
    return idProcess == GetCurrentProcessId();
}

STDAPI_(DWORD) BindCtx_GetMode(IBindCtx *pbc, DWORD grfModeDefault)
{
    if (pbc)
    {
        BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
        if (SUCCEEDED(pbc->GetBindOptions(&bo)))
            grfModeDefault = bo.grfMode;
    }
    return grfModeDefault;
}


class CFileSysBindData: public IFileSystemBindData
{ 
public:
    CFileSysBindData();
    
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IFileSystemBindData
    STDMETHODIMP SetFindData(const WIN32_FIND_DATAW *pfd);
    STDMETHODIMP GetFindData(WIN32_FIND_DATAW *pfd);

private:
    ~CFileSysBindData();
    
    LONG _cRef;
    WIN32_FIND_DATAW _fd;
};


CFileSysBindData::CFileSysBindData() : _cRef(1)
{
    ZeroMemory(&_fd, sizeof(_fd));
}

CFileSysBindData::~CFileSysBindData()
{
}

HRESULT CFileSysBindData::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysBindData, IFileSystemBindData), // IID_IFileSystemBindData
         { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysBindData::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysBindData::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CFileSysBindData::SetFindData(const WIN32_FIND_DATAW *pfd)
{
    _fd = *pfd;
    return S_OK;
}

HRESULT CFileSysBindData::GetFindData(WIN32_FIND_DATAW *pfd) 
{
    *pfd = _fd;
    return S_OK;
}

STDAPI SHCreateFileSysBindCtx(const WIN32_FIND_DATA *pfd, IBindCtx **ppbc)
{
    HRESULT hr;
    IFileSystemBindData *pfsbd = new CFileSysBindData();
    if (pfsbd)
    {
        if (pfd)
        {
#ifdef UNICODE
            pfsbd->SetFindData(pfd);
#else
            WIN32_FIND_DATAW fdw;
            memcpy(&fdw, pfd, FIELD_OFFSET(WIN32_FIND_DATAW, cFileName));
            SHTCharToUnicode(pfd->cFileName, fdw.cFileName, ARRAYSIZE(fdw.cFileName));
            SHTCharToUnicode(pfd->cAlternateFileName, fdw.cAlternateFileName, ARRAYSIZE(fdw.cAlternateFileName));
            pfsbd->SetFindData(&fdw);
#endif
        }

        hr = CreateBindCtx(0, ppbc);
        if (SUCCEEDED(hr))
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfMode = STGM_CREATE;
            (*ppbc)->SetBindOptions(&bo);
            (*ppbc)->RegisterObjectParam(STR_FILE_SYS_BIND_DATA, pfsbd);
        }
        pfsbd->Release();
    }
    else
    {
        *ppbc = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// returns S_OK if this is a simple bind ctx
// out:
//      optional (may be NULL) pfd
//
STDAPI SHIsFileSysBindCtx(IBindCtx *pbc, WIN32_FIND_DATA *pfd)
{
    HRESULT hr = S_FALSE; // default to no
    IUnknown *punk;
    if (pbc && SUCCEEDED(pbc->GetObjectParam(STR_FILE_SYS_BIND_DATA, &punk)))
    {
        IFileSystemBindData *pfsbd;
        if (SUCCEEDED(punk->QueryInterface(IID_IFileSystemBindData, (void **)&pfsbd)))
        {
            hr = S_OK;    // yes
            if (pfd)
            {
#ifdef UNICODE
                pfsbd->GetFindData(pfd);
#else
                WIN32_FIND_DATAW fdw;
                pfsbd->GetFindData(&fdw);
                memcpy(pfd, &fdw, FIELD_OFFSET(WIN32_FIND_DATAW, cFileName));
                SHUnicodeToTChar(fdw.cFileName, pfd->cFileName, ARRAYSIZE(pfd->cFileName));
                SHUnicodeToTChar(fdw.cAlternateFileName, pfd->cAlternateFileName, ARRAYSIZE(pfd->cAlternateFileName));
#endif
            }
            pfsbd->Release();
        }
        punk->Release();
    }
    return hr;
}


STDAPI SHCreateSkipBindCtx(IUnknown *punkToSkip, IBindCtx **ppbc)
{
    HRESULT hr = CreateBindCtx(0, ppbc);
    if (SUCCEEDED(hr))
    {
        // NULL clsid means bind context that skips all junction points
        if (punkToSkip)
        {
            (*ppbc)->RegisterObjectParam(STR_SKIP_BINDING_CLSID, punkToSkip);
        }
        else
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;
            (*ppbc)->SetBindOptions(&bo);
        }
    }
    return hr;
}

// We've overloaded the meaning of the BIND_OPTS flag
// BIND_JUSTTESTEXISTENCE to mean don't bind to junctions
// when evaluating paths...so check it here...
//
//  pbc         optional bind context (can be NULL)
//  pclsidSkip  optional CLSID to test. if null we test for skiping all
//              junction binding, not just on this specific CLSID
//
STDAPI_(BOOL) SHSkipJunctionBinding(IBindCtx *pbc, const CLSID *pclsidSkip)
{
    if (pbc)
    {
        BIND_OPTS bo = {sizeof(BIND_OPTS), 0};  // Requires size filled in.
        return (SUCCEEDED(pbc->GetBindOptions(&bo)) && 
                bo.grfFlags == BIND_JUSTTESTEXISTENCE) ||
                (pclsidSkip && SHSkipJunction(pbc, pclsidSkip));     // should we skip this specific CLSID?
    }
    return FALSE;   // do junction binding, no context provided
}

// return a relative IDList to pszFolder given the find data for that item

STDAPI SHCreateFSIDList(LPCTSTR pszFolder, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    TCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidl;

    *ppidl = NULL;

    PathCombine(szPath, pszFolder, pfd->cFileName);

    HRESULT hr = SHSimpleIDListFromFindData(szPath, pfd, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = SHILClone(ILFindLastID(pidl), ppidl);
        ILFree(pidl);
    }
    return hr;
}


STDAPI InvokeVerbOnItems(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IShellFolder *psf, UINT cidl, LPCITEMIDLIST *apidl)
{
    IContextMenu *pcm;
    HRESULT hr = psf->GetUIObjectOf(hwnd, cidl, apidl, IID_IContextMenu, NULL, (void **)&pcm);
    if (SUCCEEDED(hr))
    {
        CMINVOKECOMMANDINFOEX ici =
        {
            SIZEOF(CMINVOKECOMMANDINFOEX),
            uFlags | CMIC_MASK_UNICODE,
            hwnd,
            NULL,
            NULL,
            NULL,
            SW_NORMAL,
        };
        CHAR szVerbA[128];
        WCHAR szVerbW[128];
        SHTCharToAnsi(pszVerb, szVerbA, ARRAYSIZE(szVerbA));
        SHTCharToUnicode(pszVerb, szVerbW, ARRAYSIZE(szVerbW));

        ici.lpVerb = szVerbA;
        ici.lpVerbW = szVerbW;

        hr = pcm->InvokeCommand((CMINVOKECOMMANDINFO*)&ici);
        pcm->Release();
    }
    return hr;
}

//
// BUGBUG: this depends on the HDROP being in the TCHAR sense of 
// this code. that does not need to be. to fix this use DragQueryFile()
// to extrac the file names into the list.
//
STDAPI_(void) TransferDelete(HWND hwnd, HDROP hDrop, UINT fOptions)
{
    DRAGINFO di = { SIZEOF(DRAGINFO), 0 };
    if (DragQueryInfo(hDrop, &di))
    {
        FILEOP_FLAGS fFileop;
        if (fOptions & SD_SILENT)
        {
            fFileop = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_ALLOWUNDO;
        }
        else
        {
            fFileop = ((fOptions & SD_NOUNDO) || (GetAsyncKeyState(VK_SHIFT) < 0)) ? 0 : FOF_ALLOWUNDO;

            if (fOptions & SD_WARNONNUKE)
            {
                // we pass this so the user is warned that they will loose
                // data during a move-to-recycle bin operation
                fFileop |= FOF_WANTNUKEWARNING;
            }

            if (!(fOptions & SD_USERCONFIRMATION))
                fFileop |= FOF_NOCONFIRMATION;
        }

        SHFILEOPSTRUCT fo = {
            hwnd,
            FO_DELETE,
            di.lpFileList,
            NULL,
            fFileop,
        };

        SHFileOperation(&fo);
        SHFree(di.lpFileList);
    }
}


STDAPI DeleteFilesInDataObjectEx(HWND hwnd, UINT uFlags, IDataObject *pdtobj, UINT fOptions)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        fOptions |= (uFlags & CMIC_MASK_FLAG_NO_UI) ? SD_SILENT : SD_USERCONFIRMATION;

        if ((uFlags & CMIC_MASK_SHIFT_DOWN) || (GetKeyState(VK_SHIFT) < 0))
        {
            fOptions |= SD_NOUNDO;
        }

        TransferDelete(hwnd, (HDROP)medium.hGlobal, fOptions);

        ReleaseStgMedium(&medium);

        SHChangeNotifyHandleEvents();
    }
    return hr;
}

STDAPI DeleteFilesInDataObject(HWND hwnd, UINT uFlags, IDataObject *pdtobj)
{
    return DeleteFilesInDataObjectEx(hwnd, uFlags, pdtobj, 0);
}


STDAPI InvokeVerbOnDataObj(HWND hwnd, LPCTSTR pszVerb, UINT uFlags, IDataObject *pdtobj)
{
    HRESULT hr;

    if (0 == lstrcmpi(c_szDelete, pszVerb))
        hr = DeleteFilesInDataObject(hwnd, uFlags, pdtobj);
    else
        hr = E_FAIL;

    if (hr != S_OK)
    {
        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            IShellFolder *psf;
            LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
            if (pidlParent &&
                SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidlParent, (void **)&psf)))
            {
                LPCITEMIDLIST *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, pida->cidl * sizeof(LPCITEMIDLIST));
                if (ppidl)
                {
                    for (UINT i = 0; i < pida->cidl; i++) 
                    {
                        ppidl[i] = IDA_GetIDListPtr(pida, i);
                    }
                    hr = InvokeVerbOnItems(hwnd, pszVerb, uFlags, psf, pida->cidl, ppidl);
                    LocalFree(ppidl);
                }
                psf->Release();
            }
            HIDA_ReleaseStgMedium(pida, &medium);
        }
    }
    return hr;
}

STDAPI GetItemCLSID(IShellFolder2 *psf, LPCITEMIDLIST pidlLast, CLSID *pclsid)
{
    VARIANT var;
    HRESULT hr = psf->GetDetailsEx(pidlLast, &SCID_DESCRIPTIONID, &var);
    if (SUCCEEDED(hr))
    {
        SHDESCRIPTIONID did;
        hr = VariantToBuffer(&var, (void *)&did, sizeof(did));
        if (SUCCEEDED(hr))
            *pclsid = did.clsid;

        VariantClear(&var);
    }
    return hr;
}

// in:
//      pidl    fully qualified IDList to test. we will bind to the
//              parent of this and ask it for the CLSID
// out:
//      pclsid  return the CLSID of the item

STDAPI GetCLSIDFromIDList(LPCITEMIDLIST pidl, CLSID *pclsid)
{
    IShellFolder2 *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToIDListParent(pidl, IID_IShellFolder2, (void **)&psf, &pidlLast);
    if (SUCCEEDED(hr))
    {
        hr = GetItemCLSID(psf, pidlLast, pclsid);
        psf->Release();
    }
    return hr;
}

// test to see if this IDList is in the net hood name space scoped by clsid
// for example pass CLSID_NetworkPlaces or CLSID_MyComputer

STDAPI_(BOOL) IsIDListInNameSpace(LPCITEMIDLIST pidl, const CLSID *pclsid)
{
    BOOL bInNet = FALSE;
    LPITEMIDLIST pidlFirst = ILCloneFirst(pidl);
    if (pidlFirst)
    {
        CLSID clsid;
        bInNet = SUCCEEDED(GetCLSIDFromIDList(pidlFirst, &clsid)) && IsEqualCLSID(clsid, *pclsid);
        ILFree(pidlFirst);
    }
    return bInNet;
}

#define FILE_ATTRIBUTE_SH (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)

struct {
    LPCTSTR pszFile;
    BOOL bDeleteIfEmpty;
    DWORD dwAttributes;
} const c_aFilesToFix[] = {
    { TEXT("X:\\autoexec.bat"), TRUE,   FILE_ATTRIBUTE_HIDDEN },
    { TEXT("X:\\autoexec.000"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.old"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.bak"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.dos"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\autoexec.win"), TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\config.sys"),   TRUE,   FILE_ATTRIBUTE_HIDDEN },
    { TEXT("X:\\config.dos"),   TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\config.win"),   TRUE,   FILE_ATTRIBUTE_SH },
    { TEXT("X:\\command.com"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\command.dos"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\logo.sys"),     FALSE,  FILE_ATTRIBUTE_SH },

    { TEXT("X:\\msdos.---"),    FALSE,  FILE_ATTRIBUTE_SH },    // Win9x backup of msdos.* 

    { TEXT("X:\\boot.ini"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\boot.bak"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\boot.---"),     FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\bootsect.dos"), FALSE,  FILE_ATTRIBUTE_SH },

    { TEXT("X:\\bootlog.txt"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x first boot log
    { TEXT("X:\\bootlog.prv"),  FALSE,  FILE_ATTRIBUTE_SH },

    { TEXT("X:\\ffastun.ffa"),  FALSE,  FILE_ATTRIBUTE_SH },    // Office 97 only used hidden, O2K uses SH
    { TEXT("X:\\ffastun.ffl"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffastun.ffx"),  FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffastun0.ffx"), FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\ffstunt.ffl"),  FALSE,  FILE_ATTRIBUTE_SH },

    { TEXT("X:\\sms.ini"),      FALSE,  FILE_ATTRIBUTE_SH },    // SMS
    { TEXT("X:\\sms.new"),      FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\sms_time.dat"), FALSE,  FILE_ATTRIBUTE_SH },
    { TEXT("X:\\smsdel.dat"),   FALSE,  FILE_ATTRIBUTE_SH },

    { TEXT("X:\\mpcsetup.log"), FALSE,  FILE_ATTRIBUTE_HIDDEN },// Microsoft Proxy Server

    { TEXT("X:\\detlog.txt"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win9x PNP detection log
    { TEXT("X:\\detlog.old"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win9x PNP detection log

    { TEXT("X:\\setuplog.txt"), FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\setuplog.old"), FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log

    { TEXT("X:\\suhdlog.dat"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\suhdlog.---"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log
    { TEXT("X:\\suhdlog.bak"),  FALSE,  FILE_ATTRIBUTE_SH },    // Win9x setup log

    { TEXT("X:\\system.1st"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win95 system.dat backup
    { TEXT("X:\\netlog.txt"),   FALSE,  FILE_ATTRIBUTE_SH },    // Win9x network setup log file

    { TEXT("X:\\setup.aif"),    FALSE,  FILE_ATTRIBUTE_SH },    // NT4 unattended setup script
    { TEXT("X:\\catlog.wci"),   FALSE,  FILE_ATTRIBUTE_HIDDEN },// index server folder

    { TEXT("X:\\cmsstorage.lst"), FALSE,  FILE_ATTRIBUTE_SH },  // Microsoft Media Manager
};

void PathSetSystemDrive(TCHAR *pszPath)
{
    TCHAR szWin[MAX_PATH];

    GetWindowsDirectory(szWin, ARRAYSIZE(szWin));
    *pszPath = szWin[0];
}

void PrettyPath(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
    PathSetSystemDrive(szPath);
    PathMakePretty(PathFindFileName(szPath));  // fix up the file spec part

    MoveFile(pszPath, szPath);      // rename to the good name.
}

void DeleteEmptyFile(LPCTSTR pszPath)
{
    WIN32_FIND_DATA fd;
    HANDLE hfind = FindFirstFile(pszPath, &fd);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        FindClose(hfind);
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
             (fd.nFileSizeHigh == 0) && (fd.nFileSizeLow == 0))
            DeleteFile(pszPath);
    }
}

STDAPI_(void) CleanupFileSystem()
{
    PrettyPath(TEXT("X:\\WINDOWS"));
    PrettyPath(TEXT("X:\\WINNT"));

    for (int i = 0; i < ARRAYSIZE(c_aFilesToFix); i++)
    {
        TCHAR szPath[MAX_PATH];

        lstrcpyn(szPath, c_aFilesToFix[i].pszFile, ARRAYSIZE(szPath));
        PathSetSystemDrive(szPath);

        if (c_aFilesToFix[i].bDeleteIfEmpty)
            DeleteEmptyFile(szPath);

        SetFileAttributes(szPath, c_aFilesToFix[i].dwAttributes);
    }
}


STDAPI SHILPrepend(LPITEMIDLIST pidlToPrepend, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    if (!*ppidl)
    {
        *ppidl = pidlToPrepend;
        hr = S_OK;
    }   
    else
    {
        LPITEMIDLIST pidlSave = *ppidl;             // append to the list
        hr = SHILCombine(pidlToPrepend, pidlSave, ppidl);
        ILFree(pidlSave);
        ILFree(pidlToPrepend);
    }
    return hr;
}

// in:
//      pidlToAppend    this item is appended to *ppidl and freed
//
// in/out:
//      *ppidl  idlist to append to, if empty gets pidlToAppend
//
STDAPI SHILAppend(LPITEMIDLIST pidlToAppend, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    if (!*ppidl)
    {
        *ppidl = pidlToAppend;
        hr = S_OK;
    }   
    else
    {
        LPITEMIDLIST pidlSave = *ppidl;             // append to the list
        hr = SHILCombine(pidlSave, pidlToAppend, ppidl);
        ILFree(pidlSave);
        ILFree(pidlToAppend);
    }
    return hr;
}


STDAPI SHSimpleIDListFromFindData(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = SHCreateFileSysBindCtx(pfd, &pbc);
        if (SUCCEEDED(hr))
        {
            WCHAR wszPath[MAX_PATH];

            // Must use a private buffer because ParseDisplayName takes a non-const pointer
            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

            hr = psfDesktop->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
            pbc->Release();
        }
        psfDesktop->Release();
    }

    if (FAILED(hr))
        *ppidl = NULL;
    return hr;
}

STDAPI SHSimpleIDListFromFindData2(IShellFolder *psf, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    ASSERT(psf);
    
    IBindCtx *pbc;
    hr = SHCreateFileSysBindCtx(pfd, &pbc);
    if (SUCCEEDED(hr))
    {
        WCHAR wszPath[MAX_PATH];

        // Must use a private buffer because ParseDisplayName takes a non-const pointer
        SHTCharToUnicode(pfd->cFileName, wszPath, ARRAYSIZE(wszPath));

        hr = psf->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
        pbc->Release();
    }

    if (FAILED(hr))
        *ppidl = NULL;
    return hr;
}

STDAPI_(LPITEMIDLIST) SHSimpleIDListFromPath(LPCTSTR pszPath)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHSimpleIDListFromFindData(pszPath, NULL, &pidl);
    ASSERT(SUCCEEDED(hr) ? pidl != NULL : pidl == NULL);
    return pidl;
}

// convert a full file system IDList into one relative to the desktop
// in:
//      pidlFS == [my computer] [c:] [windows] [desktop] [dir] [foo.txt]
// returns:
//      [dir] [foo.txt]

STDAPI_(LPITEMIDLIST) SHLogILFromFSIL(LPCITEMIDLIST pidlFS)
{
    LPITEMIDLIST pidlChild = NULL;
    LPITEMIDLIST pidlDesktop = SHCloneSpecialIDList(NULL, CSIDL_DESKTOPDIRECTORY, TRUE);
    if (pidlDesktop)
    {
        pidlChild = ILFindChild(pidlDesktop, pidlFS);
        if (pidlChild)
            pidlChild = ILClone(pidlChild);
        ILFree(pidlDesktop);
    }
    return pidlChild;
}

//
// Returns:
//  The resource index (of SHELL232.DLL) of the appropriate icon.
//
STDAPI_(UINT) SILGetIconIndex(LPCITEMIDLIST pidl, const ICONMAP aicmp[], UINT cmax)
{
    UINT uType = (pidl->mkid.abID[0] & SHID_TYPEMASK);
    for (UINT i = 0; i < cmax; i++) 
    {
        if (aicmp[i].uType == uType) 
        {
            return aicmp[i].indexResource;
        }
    }
    
    return II_DOCUMENT;   // default
}

STDAPI_(LPITEMIDLIST) VariantToIDList(const VARIANT *pv)
{
    LPITEMIDLIST pidl = NULL;
    VARIANT v;

    if (pv->vt == (VT_BYREF | VT_VARIANT) && pv->pvarVal)
        v = *pv->pvarVal;
    else
        v = *pv;

    switch (v.vt)
    {
    case VT_I2:
        v.lVal = (long)v.iVal;
        // Fall through

    case VT_I4:
    case VT_UI4:
        pidl = SHCloneSpecialIDList(NULL, v.lVal, TRUE);
        break;

    case VT_BSTR:
        pidl = ILCreateFromPathW(v.bstrVal);
        break;

    case VT_ARRAY | VT_UI1:
        pidl = ILClone((LPCITEMIDLIST)v.parray->pvData);
        break;

    case VT_DISPATCH | VT_BYREF:
        if (v.ppdispVal == NULL)
            break;

        v.pdispVal = *v.ppdispVal;

        // fall through...

    case VT_DISPATCH:
        SHGetIDListFromUnk(v.pdispVal, &pidl);
        break;
    }
    return pidl;
}

STDAPI VariantToBuffer(const VARIANT* pvar, void *pv, UINT cb)
{
    if (pvar && pvar->vt == (VT_ARRAY | VT_UI1))
    {
        memcpy(pv, pvar->parray->pvData, cb);
        return TRUE;
    }
    return FALSE;
}

STDAPI VariantToGUID(const VARIANT *pvar, GUID *pguid)
{
    return VariantToBuffer(pvar, pguid, sizeof(*pguid));
}


// helper function for optional Variants that should be TStrs...
// will allocate it for you if you pass NULL pszBuf

STDAPI_(LPTSTR) VariantToStr(const VARIANT *pvar, LPTSTR pszBuf, int cchBuf)
{
    VARIANT v;

    if (pszBuf)
        *pszBuf = 0;

    if (pvar->vt == (VT_BYREF | VT_VARIANT) && pvar->pvarVal)
        v = *pvar->pvarVal;
    else
        v = *pvar;

    if (v.vt == VT_BSTR && v.bstrVal)
    {
        if (!pszBuf)
        {
            TCHAR szBuf[INFOTIPSIZE];
            SHUnicodeToTChar(v.bstrVal, szBuf, ARRAYSIZE(szBuf));
            return StrDup(szBuf);
        }
        else
        {
            SHUnicodeToTChar(v.bstrVal, pszBuf, cchBuf);
            return pszBuf;
        }
    }
    else if (v.vt == VT_DATE)
    {
        USHORT wDosDate, wDosTime;

        VariantTimeToDosDateTime(v.date, &wDosDate, &wDosTime);
        DosTimeToDateTimeString(wDosDate, wDosTime, pszBuf, cchBuf, 0);
    }
    return NULL;
}

STDAPI_(LPCWSTR) VariantToStrW(const VARIANT *pvar)
{
    LPCWSTR psz = NULL;
    VARIANT v;

    if (pvar->vt == (VT_BYREF | VT_VARIANT) && pvar->pvarVal)
        v = *pvar->pvarVal;
    else
        v = *pvar;

    if (v.vt == VT_BSTR)
        psz = v.bstrVal;
    return psz;
}


STDAPI InitVariantFromBuffer(VARIANT *pvar, const void *pv, UINT cb)
{
    HRESULT hr;
    SAFEARRAY *psa = SafeArrayCreateVector(VT_UI1, 0, cb);   // create a one-dimensional safe array
    if (psa) 
    {
        memcpy(psa->pvData, pv, cb);

        memset(pvar, 0, sizeof(*pvar));  // VariantInit()
        pvar->vt = VT_ARRAY | VT_UI1;
        pvar->parray = psa;
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}


STDAPI InitVariantFromIDList(VARIANT* pvar, LPCITEMIDLIST pidl)
{
    return InitVariantFromBuffer(pvar, pidl, ILGetSize(pidl));
}

STDAPI InitVariantFromGUID(VARIANT *pvar, GUID *pguid)
{
    return InitVariantFromBuffer(pvar, pguid, SIZEOF(*pguid));
}

// returns:
//      S_OK    success
//      S_FALSE string was empty
//      E_OUTOFMEMORY
STDAPI InitVariantFromStr(VARIANT *pvar, LPCTSTR psz)
{
    HRESULT hr;
    VariantInit(pvar);
    pvar->vt = VT_BSTR;

    if (psz && *psz)
    {
        pvar->bstrVal = SysAllocStringT(psz);
        hr = pvar->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        pvar->bstrVal = NULL;
        hr = S_FALSE;
    }
    return hr;
}

// VARIANT -> STRRET
STDAPI VariantToStrRet(const VARIANT *pv, STRRET *pstrret)
{
    if (pv->vt == VT_BSTR && pv->bstrVal)
        return StringToStrRetW(pv->bstrVal, pstrret);
    return E_FAIL;
}

// convert STRRET -> VARIANT
// note, this frees the STRRET contents

STDAPI InitVariantFromStrRet(STRRET *pstrret, LPCITEMIDLIST pidl, VARIANT *pv)
{
    WCHAR szTemp[INFOTIPSIZE];
    HRESULT hres = StrRetToBufW(pstrret, pidl, szTemp, ARRAYSIZE(szTemp));
    if (SUCCEEDED(hres))
    {
        pv->vt = VT_BSTR;
        pv->bstrVal = SysAllocString(szTemp);
        hres = pv->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    return hres;
}


BOOL IsSelf(UINT cidl, LPCITEMIDLIST *apidl)
{
    return cidl == 0 || (cidl == 1 && (apidl == NULL || apidl[0] == NULL || ILIsEmpty(apidl[0])));
}

//
// GetMenuIndexFromCanonicalVerb
//
// This is a helper function for finding a specific verb's index in a context menu
//
STDAPI_(UINT) GetMenuIndexForCanonicalVerb(HMENU hMenu, IContextMenu* pcm, UINT idCmdFirst, LPCWSTR pwszVerb)
{
    int iItem = -1;
    int cMenuItems = GetMenuItemCount(hMenu);

    for (iItem = 0; iItem < cMenuItems; iItem++)
    {
        MENUITEMINFO mii = {0};

        mii.cbSize = SIZEOF(MENUITEMINFO);
        mii.fMask = MIIM_TYPE | MIIM_ID;

        if (GetMenuItemInfo(hMenu, iItem, MF_BYPOSITION, &mii) &&
            !(mii.fType & MFT_SEPARATOR) &&
            (mii.wID != -1) &&
            (mii.wID >= idCmdFirst))
        {
            WCHAR szItemName[80];
            CHAR aszVerb[80];

            // try both GCS_VERBA and GCS_VERBW in case it only supports one of them
            SHUnicodeToAnsi(pwszVerb, aszVerb, ARRAYSIZE(aszVerb));

            if (SUCCEEDED(pcm->GetCommandString(mii.wID - idCmdFirst, GCS_VERBA, NULL, (LPSTR)szItemName, ARRAYSIZE(szItemName))))
            {
                if (lstrcmpiA((LPCSTR)szItemName, aszVerb) == 0)
                {
                    // found it
                    break;
                }
            }
            else
            {
                if (SUCCEEDED(pcm->GetCommandString(mii.wID - idCmdFirst, GCS_VERBW, NULL, (LPSTR)szItemName, ARRAYSIZE(szItemName))) &&
                    (StrCmpIW(szItemName, pwszVerb) == 0))
                {
                    // found it
                    break;
                }
            }
        }
    }

    if (iItem == cMenuItems)
    {
        // went through all the menuitems and didn't find it
        iItem = -1;
    }

    return iItem;
}


//
// GetIconLocationFromExt
// 
// Given "txt" or ".txt" return "C:\WINNT\System32\Notepad.exe" and the index into this file for the icon.
//
STDAPI GetIconLocationFromExt(IN LPTSTR pszExt, OUT LPTSTR pszIconPath, UINT cchIconPath, OUT LPINT piIconIndex)
{
    IAssocStore* pas;
    IAssocInfo* pai;
    HRESULT hr;
    
    RIPMSG(pszIconPath && IS_VALID_STRING_PTR(pszIconPath, cchIconPath), "GetIconLocationFromExt: caller passed bad pszIconPath");
    
    if (!pszExt || !pszExt[0] || !pszIconPath)
        return E_INVALIDARG;
    
    pszIconPath[0] = TEXT('\0');
    
    pas = new CFTAssocStore();
    if (pas)
    {
        hr = pas->GetAssocInfo(pszExt, AIINIT_EXT, &pai); 
        if (SUCCEEDED(hr))
        { 
            DWORD cchSize = cchIconPath;
        
            // BUGBUG (reinerf) - what if cchSize isint enough?
            hr = pai->GetString(AISTR_ICONLOCATION, pszIconPath, &cchSize); 
        
            if (SUCCEEDED(hr))
            {
                *piIconIndex = PathParseIconLocation(pszIconPath);
            }
        
            pai->Release();
        }
        delete pas;
    }
    else
        hr = E_OUTOFMEMORY;
    
    return hr;
}


#if 0 // DEAD CODE

STDAPI SHGetUIObjectOf(LPTSTR pszPath, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;

    //convert the path to a pidl
    HRESULT hr;
    LPITEMIDLIST pidl = ILCreateFromPath(pszPath);
    if (pidl)
    {
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf;
            //bind to the parent to be able to GetUIObjectOf from it
            hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), NULL);
    
            if (SUCCEEDED(hr))
            {
                hr = psf->GetUIObjectOf(1, (LPCITEMIDLIST*)&pidl, riid, 0, ppvOut);
                psf->Release();
            }
            ILFree(pidl);
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}
#endif


/*++
This routine checks whether the country code is CTRY_FRANCE.
Arguments:
none
 
Return Value:
    TRUE    locale is not France
    FALSE   locale is France
--*/
BOOL InFrance(VOID)
{
    BOOL fRet = TRUE;   // strange, we are in france by default

    //
    // Check if the default language is Standard French
    //
    LCID lcidDefault = GetSystemDefaultLCID();
    if (LANGIDFROMLCID(lcidDefault) != 0x40c)
    {
        TCHAR szCountryCode[10];
        //
        // Check if the users's country is set to FRANCE
        //
        if (GetLocaleInfo(lcidDefault, LOCALE_ICOUNTRY, szCountryCode, ARRAYSIZE(szCountryCode)))
        {
            fRet = (CTRY_FRANCE == StrToInt(szCountryCode));
        }
    }
    return fRet;
}


/*++
Routine Description:
This routine checks to see if we are allowed to encrypt files.  Generally, we
cannot do this in France, but if they have changed the rules after we ship, then 
a restriction might allow us to do it.  Other rules could be added here, but for now,
we are concerned only with the French.

--*/
BOOL AllowedToEncrypt()
{
    if (SHRestricted(REST_NOENCRYPTION))        //generic "no encryption" policy
        return FALSE;

    // In France without the removing of the restriction
    if (InFrance() && !SHRestricted(REST_ALLOWFRENCRYPTION))        
        return FALSE;

    return TRUE;
}

STDAPI SHFindFirstFile(LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind)
{
    HRESULT hr;
#ifdef WINNT
    // instead of the ...erm HACK in the reserved word (win95), use the super new (NT only) FindFileEx instead
    // this is not guaranteed to filter, it is a "hint" according to the manual
    FINDEX_SEARCH_OPS eOps = FindExSearchNameMatch;
    if (pfd->dwReserved0 == 0x56504347)
    {
        eOps = FindExSearchLimitToDirectories;
        pfd->dwReserved0 = 0;
    }
    *phfind = FindFirstFileEx(pszPath, FindExInfoStandard, pfd, eOps, NULL, 0);
#else
    *phfind = FindFirstFile(pszPath, pfd);
#endif
    if (*phfind == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if ((err == ERROR_NO_MORE_FILES ||      // what dos returns
             err == ERROR_FILE_NOT_FOUND) &&    // win32 return for dir searches
            PathIsWild(pszPath))
        {
            hr = S_FALSE;         // convert search to empty success (probalby a root)
        }
        else
            hr = HRESULT_FROM_WIN32(err);
    }
    else
        hr = S_OK;
    return hr;
}


void _GoModal(HWND hwnd, IUnknown *punkEnableModless, BOOL fModal)
{
    if (hwnd)
        IUnknown_EnableModless(punkEnableModless, !fModal);
}

// in:
//      hwnd    NULL means no UI

HRESULT _RetryNetwork(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind)
{
    HRESULT hres;
    TCHAR szT[MAX_PATH];
    DWORD err;

    if (PathIsUNC(pszPath))
    {
        NETRESOURCE rc = { 0, RESOURCETYPE_ANY, 0, 0, NULL, szT, NULL, NULL} ;

        lstrcpy(szT, pszPath);
        PathStripToRoot(szT);

        _GoModal(hwnd, punkEnableModless, TRUE);
        err = WNetAddConnection3(hwnd, &rc, NULL, NULL, (hwnd ? (CONNECT_TEMPORARY | CONNECT_INTERACTIVE) : CONNECT_TEMPORARY));
        _GoModal(hwnd, punkEnableModless, FALSE);
    }
    else
    {
        TCHAR szDrive[4];

        szDrive[0] = pszPath[0];
        szDrive[1] = TEXT(':');
        szDrive[2] = 0;

        _GoModal(hwnd, punkEnableModless, TRUE);
        err = WNetRestoreConnection(hwnd, szDrive);
        _GoModal(hwnd, punkEnableModless, FALSE);
        if (err == WN_SUCCESS)
        {
            // refresh drive info... generate change notify
            szDrive[2] = TEXT('\\');
            szDrive[3] = 0;

            InvalidateDriveType(DRIVEID(szDrive));
            SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, szDrive, NULL);
        }
        else if (err != ERROR_OUTOFMEMORY)
        {
            err = WN_CANCEL;    // user cancel (they saw UI) == ERROR_CANCELLED
        }
    }

    if (err == WN_SUCCESS)
        hres = SHFindFirstFile(pszPath, pfd, phfind);
    else
        hres = HRESULT_FROM_WIN32(err);

    return hres;
}


typedef struct {
    HWND hDlg;
    LPCTSTR pszPath;
    WIN32_FIND_DATA *pfd;
    HANDLE *phfind;
    HRESULT hres;
    UINT _msgQueryCancelAutoPlay;
} RETRY_DATA;

#define IDT_RETRY    1

BOOL _IsUnformatedMediaResult(HRESULT hres)
{
    return hres == HRESULT_FROM_WIN32(ERROR_GEN_FAILURE) ||         // Win9x
           hres == HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_MEDIA) ||  // NT4
           hres == HRESULT_FROM_WIN32(ERROR_NOT_DOS_DISK) ||        // Could happen, I think.
           hres == HRESULT_FROM_WIN32(ERROR_SECTOR_NOT_FOUND) ||    // Happened on Magnatized disk
           hres == HRESULT_FROM_WIN32(ERROR_CRC) ||                 // Happened on Magnatized disk
           hres == HRESULT_FROM_WIN32(ERROR_UNRECOGNIZED_VOLUME);   // NT5
}

BOOL _IsEmptyDeviceResult(HRESULT hres)
{
    return hres == HRESULT_FROM_WIN32(ERROR_NOT_READY) ||       // normal case
           hres == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); // SCSI ZIP drive does this
}

BOOL_PTR CALLBACK RetryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RETRY_DATA *prd = (RETRY_DATA *)GetWindowLongPtr(hDlg, DWLP_USER);
    switch (uMsg) 
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        prd = (RETRY_DATA *)lParam;
        prd->hDlg = hDlg;
        {
            TCHAR szFormat[128], szText[MAX_PATH];
            GetDlgItemText(hDlg, IDD_TEXT, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szText, ARRAYSIZE(szText), szFormat, prd->pszPath[0]);
            SetDlgItemText(hDlg, IDD_TEXT, szText);

            lstrcpyn(szText, prd->pszPath, ARRAYSIZE(szText));
            PathStripToRoot(szText);

            // get info about the file.
            SHFILEINFO sfi = {0};
            SHGetFileInfo(szText, FILE_ATTRIBUTE_DIRECTORY, &sfi, SIZEOF(sfi),
                SHGFI_USEFILEATTRIBUTES |
                SHGFI_ICON | SHGFI_LARGEICON | SHGFI_ADDOVERLAYS);

            ReplaceDlgIcon(prd->hDlg, IDD_ICON, sfi.hIcon);
        }
        SetTimer(prd->hDlg, IDT_RETRY, 2000, NULL);
        break;

    case WM_DESTROY:
        ReplaceDlgIcon(prd->hDlg, IDD_ICON, NULL);
        break;

    case WM_TIMER:
        prd->hres = SHFindFirstFile(prd->pszPath, prd->pfd, prd->phfind);
        // we are good or they inserted unformatted disk
        if (SUCCEEDED(prd->hres) || _IsUnformatedMediaResult(prd->hres))
            EndDialog(hDlg, IDRETRY);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDCANCEL:
            prd->hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;
        }
        break;

    default:
        if (prd)    // Will be NULL on WM_SETFONT
        {
            if (!prd->_msgQueryCancelAutoPlay)
                prd->_msgQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));

            // Is the message QueryCancelAutoPlay ?
            if (uMsg == prd->_msgQueryCancelAutoPlay)
            {
                 // Yes, Cancel the Auto Play
                 SetWindowLongPtr(hDlg, DWLP_MSGRESULT,  1);
                 return TRUE;
            }
        }
        return FALSE;
    }
    
    return TRUE;
}

HRESULT _RetryLocalVolume(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind)
{
    RETRY_DATA rd = {0};
    TCHAR szPath[MAX_PATH];

    // We want to accept "A:\dir1\*.*" and pass only
    // "A:\*.*" to RetryDlgProc so it will finish
    // when "A:\" exists and we will create "dir1" later.
    StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
    PathStripToRoot(szPath);
    PathAppend(szPath, TEXT("*.*"));

    rd.pszPath = szPath;
    rd.pfd = pfd;
    rd.phfind = phfind;
    rd.hres = E_OUTOFMEMORY;

    _GoModal(hwnd, punkEnableModless, TRUE);
    DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_RETRYFOLDERENUM), hwnd, RetryDlgProc, (LPARAM)&rd);
    _GoModal(hwnd, punkEnableModless, FALSE);
    return rd.hres;
}


/***********************************************************************\
    DESCRIPTION:
        If the string was formatted as a UNC or Drive path, offer to
    create the directory path if it doesn't exist.

    PARAMETER:
        RETURN: S_OK It exists.
                FAILURE(): Caller should not display error UI because either
                        error UI was displayed or the user didn't want to create
                        the directory.
\***********************************************************************/
HRESULT _OfferToCreateDir(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszDir, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    int nResult = IDYES;

    StrCpyN(szPath, pszDir, ARRAYSIZE(szPath));
    PathRemoveFileSpec(szPath); // wild card removed

    if (SHPPFW_ASKDIRCREATE & dwFlags)
    {
        if (hwnd)
        {
            _GoModal(hwnd, punkEnableModless, TRUE);
            nResult = ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_CREATEFOLDERPROMPT),
                            MAKEINTRESOURCE(IDS_FOLDERDOESNTEXIST),
                            (MB_YESNO | MB_ICONQUESTION),
                            szPath);
            _GoModal(hwnd, punkEnableModless, FALSE);
        }
        else
            nResult = IDNO;
    }

    if (IDYES == nResult)
    {
        _GoModal(hwnd, punkEnableModless, TRUE);
        // SHCreateDirectoryEx() will display Error UI.
        DWORD err = SHCreateDirectoryEx(hwnd, szPath, NULL);
        hr = HRESULT_FROM_WIN32(err);
        _GoModal(hwnd, punkEnableModless, FALSE);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);    // Until we get a valid directory, we can't do the download.
    }

    return hr;
}


/***********************************************************************\
    DESCRIPTION:
        See if the path w/o the spec exits.  Examples:
    pszPath="C:\dir1\dir2\*.*",           Test="C:\dir1\dir2\"
    pszPath="\\unc\share\dir1\dir2\*.*",  Test="\\unc\share\dir1\dir2\"
\***********************************************************************/
BOOL PathExistsWithOutSpec(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];

    StrCpyN(szPath, pszPath, ARRAYSIZE(szPath));
    PathRemoveFileSpec(szPath);

    return PathFileExists(szPath);
}


/***********************************************************************\
    DESCRIPTION:
        See if the Drive or UNC share exists.
    Examples:
    Path="C:\dir1\dir2\*.*",        Test="C:\"
    Path="\\unc\share\dir1\*.*",    Test="\\unc\share\"
\***********************************************************************/
BOOL PathExistsRoot(LPCTSTR pszPath)
{
    TCHAR szRoot[MAX_PATH];

    StrCpyN(szRoot, pszPath, ARRAYSIZE(szRoot));
    PathStripToRoot(szRoot);

    return PathFileExists(szRoot);
}


// like the Win32 FindFirstFile() but post UI and returns errors in hresult
// in:
//      hwnd    NULL -> disable UI (but do net reconnects, etc)
//              non NULL enable UI including insert disk, format disk, net logon
//
// returns:
//      S_OK    hfind and find data are filled in with result
//      S_FALSE no results, but media is present (hfind == INVALID_HANDLE_VALUE)
//              (this is the empty enum case)
//      FAILED() win32 error codes in the hresult

STDAPI SHFindFirstFileRetry(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pszPath, WIN32_FIND_DATA *pfd, HANDLE *phfind, DWORD dwFlags)
{
    HRESULT hr = SHFindFirstFile(pszPath, pfd, phfind);

    if (FAILED(hr))
    {
        BOOL fNet = PathIsUNC(pszPath) || IsDisconnectedNetDrive(DRIVEID(pszPath));
        if (fNet)
        {
            hr = _RetryNetwork(hwnd, punkEnableModless, pszPath, pfd, phfind);
        }
        else if (hwnd)
        {
            if (PathIsRemovable(pszPath))
            {
                if (_IsEmptyDeviceResult(hr))
                {
                    hr = _RetryLocalVolume(hwnd, punkEnableModless, pszPath, pfd, phfind);
                }
            }
            
            // disk should be in now, see if it needs format
            if (_IsUnformatedMediaResult(hr))
            {
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

#ifdef WINNT
                BOOL fMountedOnFolder = FALSE;
                TCHAR szMountPath[MAX_PATH];

                // first check if it is mounted on a folder
                if (GetVolumePathName(pszPath, szMountPath, ARRAYSIZE(szMountPath)))
                {
                    if (g_bRunOnNT5 && (0 != szMountPath[2]) && (0 != szMountPath[3]))
                    {
                        fMountedOnFolder = TRUE;
                    }
                }

                if (fMountedOnFolder)
                {
                    _GoModal(hwnd, punkEnableModless, TRUE);
                    ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_MVUNFORMATTED), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                        (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK), 
                                        szMountPath);
                    _GoModal(hwnd, punkEnableModless, FALSE);                    
                }
                else
                {
#endif //WINNT
                    int iDrive = PathGetDriveNumber(pszPath);
                    _GoModal(hwnd, punkEnableModless, TRUE);
                    int nResult = ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_UNFORMATTED), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                       (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_YESNO), 
                                       (DWORD)(iDrive + TEXT('A')));
                    _GoModal(hwnd, punkEnableModless, FALSE);
  
                    if (IDYES == nResult) 
                    {
                        _GoModal(hwnd, punkEnableModless, TRUE);
                        DWORD dwError = SHFormatDrive(hwnd, iDrive, SHFMT_ID_DEFAULT, 0);
                        _GoModal(hwnd, punkEnableModless, FALSE);
                
                        switch (dwError) 
                        {
                            case SHFMT_ERROR:
                            case SHFMT_NOFORMAT:
                                _GoModal(hwnd, punkEnableModless, TRUE);
                                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_NOFMT), MAKEINTRESOURCE(IDS_FORMAT_TITLE),
                                        (MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK), (DWORD)(iDrive + TEXT('A')));
                                _GoModal(hwnd, punkEnableModless, FALSE);
                                break;

                            default:
                                hr = SHFindFirstFile(pszPath, pfd, phfind);  // try again after format
                        }
                    }
#ifdef WINNT
                } 
#endif //WINNT
            }
        }

        // If the caller wants us to create the directory (with or without asking), we
        // need to see if we can display UI and if either that root exists (D:\ or \\unc\share\)
        // Note that for PERF we want to check the full path.
        if (((SHPPFW_DIRCREATE | SHPPFW_ASKDIRCREATE) & dwFlags) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr) &&
                !PathExistsWithOutSpec(pszPath) && PathExistsRoot(pszPath))
        {
            hr = _OfferToCreateDir(hwnd, punkEnableModless, pszPath, dwFlags);
            if (INVALID_HANDLE_VALUE != *phfind)
            {
                FindClose(*phfind);
                *phfind = INVALID_HANDLE_VALUE;
            }

            if (SUCCEEDED(hr))
                hr = SHFindFirstFile(pszPath, pfd, phfind);  // try again after format
        }

        if (FAILED(hr) && hwnd && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
        {
            TCHAR szPath[MAX_PATH];

            UINT idTemplate = IDS_ENUMERR_FSTEMPLATE;    // "%2 is not accessible.\n\n%1"

            if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
                idTemplate = IDS_ENUMERR_PATHNOTFOUND;    // "%2 is not accessible.\n\nThis folder was moved or removed."

            lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
            PathRemoveFileSpec(szPath); // wild card removed

            _GoModal(hwnd, punkEnableModless, TRUE);
            SHEnumErrorMessageBox(hwnd, idTemplate, hr, szPath, fNet, MB_OK | MB_ICONHAND);
            _GoModal(hwnd, punkEnableModless, FALSE);
        }
    }
    return hr;
}


// TODO: Use the code in: \\orville\razzle\src\private\sm\sfc\dll\fileio.c to register
//       for CD/DVD inserts instead of constantly pegging the CPU and drive.  Too bad
//       it doesn't work for floppies.  SfcGetPathType(), RegisterForDevChange(), 
//       SfcQueueCallback(), SfcIsTargetAvailable()
STDAPI SHPathPrepareForWrite(HWND hwnd, IUnknown *punkEnableModless, LPCTSTR pwzPath, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    WIN32_FIND_DATA wfd;
    HANDLE hFind = NULL;

    StrCpyN(szPath, pwzPath, ARRAYSIZE(szPath));
    if (SHPPFW_IGNOREFILENAME & dwFlags)
        PathRemoveFileSpec(szPath);      // Strip file name so we just check the dir.

    // We can't do anything about just a UNC server. "\\server" (no share)
    if (!PathIsUNCServer(szPath))
    {
        PathAppend(szPath, TEXT("*.*"));
        hr = SHFindFirstFileRetry(hwnd, punkEnableModless, szPath, &wfd, &hFind, dwFlags);
        if (S_OK == hr)
            FindClose(hFind);
        else
        {
            if (S_FALSE == hr)
            {
                // S_FALSE from SHFindFirstFileRetry() means it exists but there
                // isn't a handle.  We want to return S_OK for Yes, and E_FAIL or S_FALSE for no.
                hr = S_OK;
            }
        }
    }

    return hr;
}


#ifdef UNICODE
STDAPI SHPathPrepareForWriteA(HWND hwnd, IUnknown *punkEnableModless, LPCSTR pszPath, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH];

    SHAnsiToTChar(pszPath, szPath, ARRAYSIZE(szPath));
    return SHPathPrepareForWrite(hwnd, punkEnableModless, szPath, dwFlags);
}
#else // UNICODE
STDAPI SHPathPrepareForWriteW(HWND hwnd, IUnknown *punkEnableModless, LPCWSTR pszPath, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH];

    SHUnicodeToTChar(pszPath, szPath, ARRAYSIZE(szPath));
    return SHPathPrepareForWrite(hwnd, punkEnableModless, szPath, dwFlags);
}

#endif // UNICODE


//
//  public export of SHBindToIDlist() has a slightly different
//  name so that we dont get compile link problems on legacy versions
//  of the shell.  shdocvw and browseui need to call SHBindToParentIDList()
STDAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    return SHBindToIDListParent(pidl, riid, ppv, ppidlLast);
}


// helper function to extract the target of a link file

STDAPI GetPathFromLinkFile(LPCTSTR pszLinkPath, LPTSTR pszTargetPath, int cchTargetPath)
{
    IShellLink* psl;
    HRESULT hr = LoadFromFile(&CLSID_ShellLink, pszLinkPath, IID_PPV_ARG(IShellLink, &psl));
    if (SUCCEEDED(hr)) 
    { 
        IShellLinkDataList* psldl;
        hr = psl->QueryInterface(IID_PPV_ARG(IShellLinkDataList, &psldl));
        if (SUCCEEDED(hr)) 
        {
            EXP_DARWIN_LINK* pexpDarwin;

            hr = psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);
            if (SUCCEEDED(hr))
            { 
                // woah, this is a darwin link. darwin links don't really have a path so we
                // will fail in this case
                SHUnicodeToTChar(pexpDarwin->szwDarwinID, pszTargetPath, cchTargetPath);
                LocalFree(pexpDarwin);
                hr = S_FALSE;
            }
            else
            {                
                hr = psl->GetPath(pszTargetPath, cchTargetPath, NULL, NULL);

                // BUGBUG (reinerf) - we might try getting the path from the idlist if
                // pszTarget is empty (eg a link to "Control Panel" will return empyt string).

            }
            psldl->Release();
        }
        psl->Release();
    }

    return hr;
}

// get the target folder for a folder pidl. this deals with the case where a folder
// is an alias to a real folder, Folder Shortcuts, etc.

STDAPI SHGetTargetFolderPath(LPCITEMIDLIST pidlFolder, LPTSTR pszPath, UINT cchBuf)
{
    // VDATEINPUTBUF(pszPath, TCHAR, MAX_PATH);
    *pszPath = 0;

    // likely should ASSERT() that pidlFolder has SFGAO_FOLDER
    IShellLink *psl;
    if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pidlFolder, NULL, IID_PPV_ARG(IShellLink, &psl))))
    {
        LPITEMIDLIST pidlTarget;
        if (S_OK == psl->GetIDList(&pidlTarget))
        {
            SHGetPathFromIDList(pidlTarget, pszPath);   // make sure it is a path
            ILFree(pidlTarget);
        }
        psl->Release();
    }

    // No its not a folder shortcut. Get the Path normally.
    if (*pszPath == 0)
        SHGetPathFromIDList(pidlFolder, pszPath);

    return *pszPath ? S_OK : E_FAIL;
}

// a .EXE that is registered in the app paths key, this implies it is installed

STDAPI_(BOOL) PathIsRegisteredProgram(LPCTSTR pszPath)
{
    TCHAR szTemp[MAX_PATH];
    //
    //  PathIsBinaryExe() returns TRUE for .exe, .com
    //  PathIsExe()       returns TRUE for .exe, .com, .bat, .cmd, .pif
    //
    //  we dont want to treat .pif files as EXE files, because the
    //  user sees them as links.
    //
    return PathIsBinaryExe(pszPath) && PathToAppPath(pszPath, szTemp);
}

STDAPI LoadFromFile(const CLSID *pclsid, LPCTSTR pszFile, REFIID riid, void **ppv)
{
    *ppv = NULL;

    IPersistFile *ppf;
    HRESULT hr = SHCoCreateInstance(NULL, pclsid, NULL, IID_IPersistFile, (void **)&ppf);
    if (SUCCEEDED(hr))
    {
        WCHAR wszPath[MAX_PATH];
        SHTCharToUnicode(pszFile, wszPath, ARRAYSIZE(wszPath));

        hr = ppf->Load(wszPath, STGM_READ);
        if (SUCCEEDED(hr))
            hr = ppf->QueryInterface(riid, ppv);
        ppf->Release();
    }
    return hr;
}

STDAPI_(void) DosTimeToDateTimeString(WORD wDate, WORD wTime, LPTSTR pszText, UINT cchText, int fmt)
{
    FILETIME ft;
    DWORD dwFlags = FDTF_DEFAULT;

    // Netware directories do not have dates...
    if (wDate == 0)
    {
        *pszText = 0;
        return;
    }

    DosDateTimeToFileTime(wDate, wTime, &ft);
    switch (fmt) {
    case LVCFMT_LEFT_TO_RIGHT :
        dwFlags |= FDTF_LTRDATE;
        break;
    case LVCFMT_RIGHT_TO_LEFT :
        dwFlags |= FDTF_RTLDATE;
        break;
    }
    SHFormatDateTime(&ft, &dwFlags, pszText, cchText);
}

#ifdef UNICODE
STDAPI_(DWORD)
SHGetIniStringUTF7(LPCWSTR lpSection, LPCWSTR lpKey, LPWSTR lpBuf, DWORD nSize, LPCWSTR lpFile)
{
    if (*lpKey == CH_CANBEUNICODE)
        return SHGetIniString(lpSection, lpKey+1, lpBuf, nSize, lpFile);
    else
        return GetPrivateProfileString(lpSection, lpKey, szNULL, lpBuf, nSize, lpFile);
}

STDAPI_(BOOL)
SHSetIniStringUTF7(LPCWSTR lpSection, LPCWSTR lpKey, LPCWSTR lpString, LPCWSTR lpFile)
{
    if (*lpKey == CH_CANBEUNICODE)
        return SHSetIniString(lpSection, lpKey+1, lpString, lpFile);
    else
        return WritePrivateProfileString(lpSection, lpKey, lpString, lpFile);
}
#endif

STDAPI_(BOOL) ShowSuperHidden()
{
    BOOL bRet = FALSE;
    
    if (!SHRestricted(REST_DONTSHOWSUPERHIDDEN))
    {
        SHELLSTATE ss;
        
        SHGetSetSettings(&ss, SSF_SHOWSUPERHIDDEN, FALSE);
        bRet = ss.fShowSuperHidden;
    }
    return bRet;
}

STDAPI_(void) ReplaceDlgIcon(HWND hDlg, UINT id, HICON hIcon)
{
    hIcon = (HICON)SendDlgItemMessage(hDlg, id, STM_SETICON, (WPARAM)hIcon, 0);
    if (hIcon)
        DestroyIcon(hIcon);
}

STDAPI_(LONG) GetOfflineShareStatus(LPCTSTR pcszPath)
{
    ASSERT(pcszPath);
    LONG lResult = CSC_SHARESTATUS_INACTIVE;
    HWND hwndCSCUI = FindWindow(STR_CSCHIDDENWND_CLASSNAME, STR_CSCHIDDENWND_TITLE);
    if (hwndCSCUI)
    {
        WCHAR wszPath[MAX_PATH];
        SHTCharToUnicode(pcszPath, wszPath, ARRAYSIZE(wszPath));
        COPYDATASTRUCT cds;
        cds.dwData = CSCWM_GETSHARESTATUS;
        cds.cbData = sizeof(WCHAR) * (lstrlenW(wszPath) + 1);
        cds.lpData = wszPath;
        lResult = (LONG) SendMessage(hwndCSCUI, WM_COPYDATA, 0, (LPARAM) &cds);
    }
    return lResult;
}

#define _FLAG_CSC_COPY_STATUS_LOCALLY_DIRTY         (FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED   | \
                                                     FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED | \
                                                     FLAG_CSC_COPY_STATUS_LOCALLY_DELETED         | \
                                                     FLAG_CSC_COPY_STATUS_LOCALLY_CREATED)

// These are defined in shellapi.h, but require _WIN32_WINNT >= 0x0500.
// This file is currently compiled with _WIN32_WINNT = 0x0400.  Rather
// that futz with the compile settings, just #define duplicates here.
// They'd better not change (ever) since this is a documented API.
#ifndef OFFLINE_STATUS_LOCAL
#define OFFLINE_STATUS_LOCAL        0x0001
#define OFFLINE_STATUS_REMOTE       0x0002
#define OFFLINE_STATUS_INCOMPLETE   0x0004
#endif

STDAPI SHIsFileAvailableOffline(LPCWSTR pwszPath, LPDWORD pdwStatus)
{
    HRESULT hr = E_INVALIDARG;
    TCHAR szUNC[MAX_PATH];

    szUNC[0] = TEXT('\0');

    if (pdwStatus)
    {
        *pdwStatus = 0;
    }

    //
    // Need full UNC path (TCHAR) for calling CSC APIs.
    // (Non-net paths are "not cached" by definition.)
    //
    if (pwszPath && pwszPath[0])
    {
        if (PathIsUNCW(pwszPath))
        {
            SHUnicodeToTChar(pwszPath, szUNC, ARRAYSIZE(szUNC));
        }
        else if (L':' == pwszPath[1] && L':' != pwszPath[0])
        {
            // Check for mapped net drive
            TCHAR szPath[MAX_PATH];
            SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath));

            DWORD dwLen = ARRAYSIZE(szUNC);
            if (NOERROR == SHWNetGetConnection(szPath, szUNC, &dwLen))
            {
                // Got \\server\share, append the rest
                PathAppend(szUNC, PathSkipRoot(szPath));
            }
            // else not mapped
        }
    }

    // Do we have a UNC path?
    if (szUNC[0])
    {
        // Assume CSC not running
        hr = E_FAIL;

        if (CSCIsCSCEnabled())
        {
            DWORD dwCscStatus = 0;

            // Assume cached
            hr = S_OK;

            if (!CSCQueryFileStatus(szUNC, &dwCscStatus, NULL, NULL))
            {
                // Not cached, return failure
                DWORD dwErr = GetLastError();
                if (NOERROR == dwErr)
                    dwErr = ERROR_PATH_NOT_FOUND;
                hr = HRESULT_FROM_WIN32(dwErr);
            }
            else if (pdwStatus)
            {
                // File is cached, and caller wants extra status info
                DWORD dwResult = 0;
                BOOL bDirty = FALSE;

                // Is it a sparse file?
                // Note: CSC always marks directories as sparse
                if ((dwCscStatus & FLAG_CSC_COPY_STATUS_IS_FILE) &&
                    (dwCscStatus & FLAG_CSC_COPY_STATUS_SPARSE))
                {
                    dwResult |= OFFLINE_STATUS_INCOMPLETE;
                }

                // Is it dirty?
                if (dwCscStatus & _FLAG_CSC_COPY_STATUS_LOCALLY_DIRTY)
                {
                    bDirty = TRUE;
                }

                // Get share status
                PathStripToRoot(szUNC);
                dwCscStatus = 0;
                if (CSCQueryFileStatus(szUNC, &dwCscStatus, NULL, NULL))
                {
                    if (dwCscStatus & FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP)
                    {
                        // Server offline --> all opens are local (only)
                        dwResult |= OFFLINE_STATUS_LOCAL;
                    }
                    else if (bDirty)
                    {
                        // Server online, but file is dirty --> open is remote
                        dwResult |= OFFLINE_STATUS_REMOTE;
                    }
                    else
                    {
                        // Server is online and file is in sync --> open is both
                        dwResult |= OFFLINE_STATUS_LOCAL | OFFLINE_STATUS_REMOTE;

                        if ((dwCscStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) == FLAG_CSC_SHARE_STATUS_VDO)
                        {
                            // BUGBUG The share is VDO, but that only affects files
                            // opened for execution. Is there a way to tell whether
                            // the file is only open locally?
                        }
                    }
                }
                else
                {
                    // Very strange. CSCQueryFileStatus succeeded for the file,
                    // but failed for the share.  Assume no active connection
                    // exists and the server is online (open both).
                    dwResult |= OFFLINE_STATUS_LOCAL | OFFLINE_STATUS_REMOTE;
                }

                *pdwStatus = dwResult;
            }
        }
    }

    return hr;
}

STDAPI_(BOOL) GetShellClassInfo(LPCTSTR pszPath, LPTSTR pszKey, LPTSTR pszBuffer, DWORD cchBuffer)
{
    TCHAR szIniFile[MAX_PATH];

    pszBuffer[0] = 0;

    lstrcpyn(szIniFile, pszPath, ARRAYSIZE(szIniFile));
    PathAppend(szIniFile, TEXT("Desktop.ini"));

    return (SHGetIniString(TEXT(".ShellClassInfo"), pszKey, pszBuffer, cchBuffer, szIniFile) ? TRUE : FALSE);
}

STDAPI_(BOOL) GetShellClassInfoInfoTip(LPCTSTR pszPath, LPTSTR pszBuffer, DWORD cchBuffer)
{
    return GetShellClassInfo(pszPath, TEXT("InfoTip"), pszBuffer, cchBuffer);
}

STDAPI_(BOOL) GetShellClassInfoHTMLInfoTipFile(LPCTSTR pszPath, LPTSTR pszBuffer, DWORD cchBuffer)
{
    TCHAR szHTMLInfoTipFile[MAX_PATH];

    BOOL fRet = GetShellClassInfo(pszPath, TEXT("HTMLInfoTipFile"),
        szHTMLInfoTipFile, ARRAYSIZE(szHTMLInfoTipFile));

    if (fRet)
    {
        LPTSTR psz = szHTMLInfoTipFile;

        if (StrCmpNI(TEXT("file://"), psz, 7) == 0) // ARRAYSIZE(TEXT("file://"))
        {
            psz += 7;   // ARRAYSIZE(TEXT("file://"))
        }

        PathCombine(psz, pszPath, psz);
        lstrcpyn(pszBuffer, psz, cchBuffer);
    }

    return fRet;
}

TCHAR const c_szUserAppData[] = TEXT("%userappdata%");
void ExpandUserAppData(LPTSTR pszFile)
{
    //Check if the given string has %UserAppData%
    LPTSTR psz = StrChr(pszFile, TEXT('%'));
    if (psz)
    {
        if (!StrCmpNI(psz, c_szUserAppData, ARRAYSIZE(c_szUserAppData)-1))
        {
            TCHAR szTempBuff[MAX_PATH];
            if (SHGetSpecialFolderPath(NULL, szTempBuff, CSIDL_APPDATA, TRUE))
            {
                PathAppend(szTempBuff, psz + lstrlen(c_szUserAppData));

                //Copy back to the input buffer!
                lstrcpy(psz, szTempBuff);
            }
        }
    }
}


TCHAR const c_szWebDir[] = TEXT("%WebDir%");
void ExpandWebDir(LPTSTR pszFile, int cch)
{
    //Check if the given string has %WebDir%
    LPTSTR psz = StrChr(pszFile, TEXT('%'));
    if (psz)
    {
        if (!StrCmpNI(psz, c_szWebDir, ARRAYSIZE(c_szWebDir) - 1))
        {
            LPTSTR pszFileName = PathFindFileName(pszFile);
            if (pszFileName && (pszFileName != psz))
            {
                TCHAR szTempBuff[MAX_PATH];
                StrCpyN(szTempBuff, pszFileName, ARRAYSIZE(szTempBuff));
                SHGetWebFolderFilePath(szTempBuff, pszFile, cch);
            }
        }
    }
}


void ExpandOtherVariables(LPTSTR pszFile, int cch)
{
    ExpandUserAppData(pszFile);
    ExpandWebDir(pszFile, cch);
}


void SubstituteWebDir(LPTSTR pszFile, int cch)
{
    TCHAR szWebDirPath[MAX_PATH];
    if (SUCCEEDED(SHGetWebFolderFilePath(TEXT("folder.htt"), szWebDirPath, ARRAYSIZE(szWebDirPath))))
    {
        PathRemoveFileSpec(szWebDirPath);

        LPTSTR pszTemp = StrStrI(pszFile, szWebDirPath);
        if (pszTemp)
        {
            StrCpy(pszTemp, c_szWebDir);
            PathAppend(pszTemp, pszTemp + lstrlen(szWebDirPath));
        }
    }
}

