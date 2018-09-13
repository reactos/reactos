//////////////////////////////////////////////////////////////////////
//                      Microsoft Internet Explorer                 //
//                Copyright(c) Microsoft Corp., 1995-1996           //
//////////////////////////////////////////////////////////////////////
//
// REGUTIL.C - registry functions common between MSHTML and INETCPL.
//

// HISTORY:
//
// 8/7/96   t-gpease    created.
//

#include "inetcplp.h"

//
// Definintions
//

#define SMALLBUFFER 64

//
// Procedures
//
const TCHAR g_cszYes[] = TEXT("yes");
const TCHAR g_cszNo[]  = TEXT("no");


// Conveters and int into a string... ONLY BYTE VALUES!!
TCHAR *MyIntToStr(TCHAR *pBuf, BYTE iVal)
{
    int i, t;

    ASSERT(iVal < 1000);

    i=0;
    if (t = iVal/100)
    {
        pBuf[i] = L'0' + t;
        i++;
    }

    if ((t = (iVal % 100) / 10) || (i!=0))
    {
        pBuf[i] = L'0' + t;
        i++;
    }

    pBuf[i] = L'0' + iVal % 10;
    i++;

    pBuf[i] = L'\0';

    return pBuf;
}

// Read the registry for a string (REG_SZ) of comma separated RGB values
COLORREF RegGetColorRefString( HUSKEY huskey, LPTSTR RegValue, COLORREF Value)
{
    TCHAR SmallBuf[SMALLBUFFER];
    TCHAR *pBuf;
    DWORD cb;
	int iRed, iGreen, iBlue;

    cb = ARRAYSIZE(SmallBuf);
    if (SHRegQueryUSValue(huskey,
                          RegValue,
                          NULL,
                          (LPBYTE)&SmallBuf,
                          &cb,
                          FALSE,
                          NULL,
                          NULL) == ERROR_SUCCESS)
    {
        iRed = StrToInt(SmallBuf);
        pBuf = SmallBuf;

        // find the next comma
        while(pBuf && *pBuf && *pBuf!=L',')
            pBuf++;
        
        // if valid and not NULL...
        if (pBuf && *pBuf)
            pBuf++;         // increment

        iGreen = StrToInt(pBuf);

        // find the next comma
        while(pBuf && *pBuf && *pBuf!=L',')
            pBuf++;

        // if valid and not NULL...
        if (pBuf && *pBuf)
            pBuf++;         // increment

        iBlue = StrToInt(pBuf);

        // make sure all values are valid
		iRed    %= 256;
		iGreen  %= 256;
		iBlue   %= 256;

	    Value = RGB(iRed, iGreen, iBlue);
    }

    return Value;
}

// Writes the registry for a string (REG_SZ) of comma separated RGB values
COLORREF RegSetColorRefString( HUSKEY huskey, LPTSTR RegValue, COLORREF Value)
{
    TCHAR SmallBuf[SMALLBUFFER];
    TCHAR DigitBuf[4];  // that all we need for '255\0'
    int iRed, iGreen, iBlue;

    iRed   = GetRValue(Value);
    iGreen = GetGValue(Value);
    iBlue  = GetBValue(Value);

    ASSERT(ARRAYSIZE(SmallBuf) >= 3 + 3 + 3 + 2 + 1) // "255,255,255"

    MyIntToStr(SmallBuf, (BYTE)iRed);
    StrCat(SmallBuf, TEXT(","));
    StrCat(SmallBuf, MyIntToStr(DigitBuf, (BYTE)iGreen));
    StrCat(SmallBuf, TEXT(","));
    StrCat(SmallBuf, MyIntToStr(DigitBuf, (BYTE)iBlue));

    SHRegWriteUSValue(huskey,
                      RegValue,
                      REG_SZ,
                      (LPVOID)&SmallBuf,
                      (lstrlen(SmallBuf)+1) * sizeof(TCHAR),
                      SHREGSET_DEFAULT);

    //
    // BUGBUG: Should we do something if this fails?
    //

    return Value;
}


// Read the registry for a string (REG_SZ = "yes" | "no") and return a BOOL value
BOOL RegGetBooleanString(HUSKEY huskey, LPTSTR pszRegValue, BOOL bValue)
{
    TCHAR   szBuf[SMALLBUFFER];
    LPCTSTR  pszDefault;
    DWORD   cb;
    DWORD   cbDef;

    // get the default setting
    if (bValue)
        pszDefault = g_cszYes;
    else
        pszDefault = g_cszNo;
    
    cb = ARRAYSIZE(szBuf);
    cbDef = (lstrlen(pszDefault)+1)*sizeof(TCHAR); // +1 for null term
    if (SHRegQueryUSValue(huskey,
                          pszRegValue,
                          NULL,
                          (LPVOID)&szBuf,
                          &cb,
                          FALSE,
                          (LPVOID)pszDefault,
                          cbDef) == ERROR_SUCCESS)
    {
        if (!StrCmpI(szBuf, g_cszYes))
            bValue = TRUE;
        else if (!StrCmpI(szBuf, g_cszNo))
            bValue = FALSE;

        // else fall thru and return the Default that was passed in
    }

    return bValue;
}

// Write the registry for a string (REG_SZ) TRUE = "yes", FALSE = "no"
BOOL RegSetBooleanString(HUSKEY huskey, LPTSTR pszRegValue, BOOL bValue)
{
    TCHAR szBuf[SMALLBUFFER];

    if (bValue)
        StrCpyN(szBuf, g_cszYes, ARRAYSIZE(szBuf));
    else
        StrCpyN(szBuf, g_cszNo, ARRAYSIZE(szBuf));

    SHRegWriteUSValue(huskey,
                      pszRegValue,
                      REG_SZ,
                      (LPVOID)&szBuf, 
                      (lstrlen(szBuf)+1) * sizeof(TCHAR),
                      SHREGSET_DEFAULT);

    //
    // BUGBUG: Should we try to do something if this fails?
    //

    return bValue;
}

