#include "shprv.h"
#include "util.h"

static char tsep[2] = {0,0};

//
//  SHELL.DLL needs almost none of this code anymore!
//
#ifdef DEAD_CODE

BOOL FAR PASCAL IsStringInList(LPSTR lpS, LPSTR lpList)
{
    while (*lpList)
      {
	if (!lstrcmpi(lpS,lpList))
	    return TRUE;
	lpList += lstrlen(lpList) + 1;
      }
    return FALSE;
}

LPSTR FAR PASCAL GRealloc(LPSTR lp, WORD size, LPCATCHBUF lpcatch)
{
    HANDLE h,h1;

    h = (HANDLE)GlobalHandle(HIWORD(lp));
    GlobalUnlock(h);
    h1 = GlobalReAlloc(h,(DWORD)size,GHND);
    if (!h1)
      {
	if (lpcatch)
	  {
	    GlobalFree(h);
	    Throw(lpcatch,SE_ERR_OOM);
	  }
      }
    else
	h = h1;

    return GlobalLock(h);
}

LPSTR FAR PASCAL GAlloc(WORD size, LPCATCHBUF lpcatch)
{
    HANDLE h;

    h = GlobalAlloc(GHND | GMEM_SHARE,(DWORD)size);
    if (!h)
      {
	if (lpcatch)
	    Throw(lpcatch,SE_ERR_OOM);
	else
	    return NULL;
      }
    return GlobalLock(h);
}

LPSTR FAR PASCAL GFree(LPSTR lp)
{
    HANDLE h;

    if (!lp)
	return NULL;

    h = (HANDLE)GlobalHandle(HIWORD(lp));
    GlobalUnlock(h);
    GlobalFree(h);

    return NULL;
}

#endif  // DEAD_CODE

//-------------------------------------------------------------------
// Helper function for 32 bit control panel to get the list of drivers
//
//
HDRVR WINAPI ShellGetNextDriverName(HDRVR hdrv, LPSTR pszName, int cbName)
{
    hdrv=GetNextDriver(hdrv, GND_FIRSTINSTANCEONLY);
    if (hdrv != NULL)
    {
	GetModuleFileName(GetDriverModuleHandle(hdrv), pszName, cbName);
    }
    return(hdrv);
}

#ifdef DEAD_CODE

// returns a pointer into the env block to a NULL terminatd enviornment variable
DWORD WINAPI GetEnvironmentVariable(LPCSTR lpszName, LPSTR lpszValue, DWORD cchMax)
{
    LPCSTR lpszEnv, lpszEqual;
    char szNameBuf[80];

    // Get the environment table's address.

    for (lpszEnv = GetDOSEnvironment(); lpszEnv && *lpszEnv; lpszEnv += lstrlen(lpszEnv) + 1)
    {
	// Look for the '=' delimiter.
	lpszEqual = StrChr(lpszEnv, '=');
	if (!lpszEqual)
	    continue;
	
	// Is this entry's left-hand side the variable we're looking for?
	lstrcpyn(szNameBuf, lpszEnv, min(sizeof(szNameBuf), lpszEqual - lpszEnv + 1));
	
	if (!lstrcmpi(lpszName, szNameBuf))
        {
	    // Yes.  Return the right-hand side.
            lstrcpyn(lpszValue, lpszEqual+1, (int)cchMax);
	    return lstrlen(lpszValue);
        }
    }

   return 0;
}

#endif // DEAD_CODE

#define ISSEP(c)   ((c) == '='  || (c) == ',')
#define ISWHITE(c) ((c) == ' '  || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define ISNOISE(c) ((c) == '"')
#define EOF     26

#define QUOTE   '"'
#define SPACE   ' '
#define EQUAL   '='

// takes a DWORD add commas etc to it and puts the result in the buffer
LPSTR WINAPI AddCommas(DWORD dw, LPSTR lpBuff, int iBufLen, BOOL fSigned)
{
  int   len, count;
  char  szTemp[20];
  char  szResult[20];
  BOOL  fHasMinus = FALSE;
  LPSTR pTemp;
  LPSTR p;


  if(tsep[0] == 0)
  {
      GetProfileString("Intl","sThousand",",",tsep,sizeof(tsep));
  }

  len = wsprintf(szTemp, (fSigned ? "%ld" : "%lu"), dw);

  pTemp = szTemp + len - 1;

  if (fSigned && szTemp[0]=='-')
  {
      /* Has a leading negative sign so adjust for this
       */
      fHasMinus = TRUE;

      p = szResult + len + ((len - 2) / 3);
  }
  else
  {
      p = szResult + len + ((len - 1) / 3);
  }

  *p-- = '\0';  // null terimnate the string we are building

  count = 1;
  while (pTemp >= szTemp) {
          *p-- = *pTemp--;
          if (count == 3) {
                  count = 1;
                  if (p > szResult)
			  *p-- = tsep[0];
          } else
                  count++;
  }

  lstrcpyn(lpBuff, szResult, iBufLen);

  return  lpBuff;
}

#ifdef DEAD_CODE

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
BOOL WINAPI ParseField(LPCSTR szData, int n, LPSTR szBuf, int iBufLen)
{
   BOOL  fQuote = FALSE;
   LPCSTR pszInf = szData;
   LPSTR ptr;
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
      pszInf = AnsiNext(pszInf);
      }

   if (n == 0 && *pszInf != '=')
      return FALSE;

   if (n > 0 && *pszInf == '=' && !fQuote)
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

         szData = AnsiNext(szData);
         }

      if (!*szData)
         {
         szBuf[0] = 0;      // make szBuf empty
         return FALSE;
         }

      szData = AnsiNext(szData); // we could do ++ here since we got here
                                 // after finding comma or equal
      n--;
      }

   /*
   * now copy the field to szBuf
   */
   while (ISWHITE(*szData))
      szData = AnsiNext(szData); // we could do ++ here since white space can
                                 // NOT be a lead byte
   fQuote = FALSE;
   ptr = szBuf;      // fill output buffer with this
   while (*szData)
      {
      if (*szData == QUOTE)
         fQuote = !fQuote;
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
      szData = AnsiNext(szData);
      }
   /*
   * remove trailing spaces and '"'s
   */
   while (ptr > szBuf)
      {
      ptr = AnsiPrev(szBuf, ptr);
      if (!ISWHITE(*ptr) && !ISNOISE(*ptr))
         {
         ptr = AnsiNext(ptr);
         break;
         }
      }
   *ptr = 0;
   return TRUE;
}

#endif // DEAD_CODE

#ifdef DEAD_CODE

// Sets and clears the "wait" cursor.
// REVIEW UNDONE - wait a specific period of time before actually bothering
// to change the cursor.
// REVIEW UNDONE - support for SetWaitPercent();
//    BOOL bSet   TRUE if you want to change to the wait cursor, FALSE if
//                you want to change it back.
BOOL WINAPI SetWaitCursor(BOOL bSet)
{
	static HCURSOR g_hCursor = NULL;
	static int g_cPending = 0;

	BOOL fReturn;

    //DebugMsg(DM_TRACE, "SetWaitCursor Enter: %d = bSet, %d = g_cPending", bSet, g_cPending);

        if (bSet)
        {
                // We want a wait cursor...
                if (!g_cPending)
                {
                        g_hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
			fReturn = FALSE;
                }
		else
		{
                        SetCursor(LoadCursor(NULL, IDC_WAIT));
			fReturn = TRUE;
		}
                ShowCursor(TRUE);
                g_cPending++;
        }
        else
        {
		// We don't want a wait cursor.
                g_cPending--;
                ShowCursor(FALSE);
                if (g_cPending == 0)
                {
                        SetCursor(g_hCursor);
                        g_hCursor = NULL;
			fReturn = TRUE;
                }
                else if (g_cPending < 0)
                {
                        g_cPending = 0;
			fReturn = FALSE;
                    DebugMsg(DM_TRACE, "error: SetWaitCursor(FALSE) called too many times");
                }
        }
	return fReturn;
}

#endif // DEAD_CODE
