/****************************************************************************/
/*									    */
/*  enpenv.c -								    */
/*									    */
/*	Routines for expanding environment strings                          */
/*									    */
/****************************************************************************/

#include "shprv.h"

#define MAXENVVARLEN 128        // Max size of an environment var LHS+RHS.
#define MAXENVSTRLEN 256        // The max size of buffer used for
                                // environment variable expansion.

//-------------------------------------------------------------------------
LPSTR WINAPI FindEnvironmentString
    // Look up the given env var and pass back a pointer to it's value.
    // Returns NULL iff the variable isn't in the environment.
    (
    LPSTR szEnvVar  // The environment variable to look for, null
                    // terminated.
    )
{
  LPSTR lpEnv;        // The environment.
  char ach[40];
  int i;

  for (lpEnv=GetDOSEnvironment(); *lpEnv; lpEnv+=lstrlen(lpEnv)+1)
    {
      for(i=0; lpEnv[i] != '=' && lpEnv[i] != 0 && i<sizeof(ach)-1; i++)
          ach[i] = lpEnv[i];

      if (lpEnv[i] != '=')
        continue;

      ach[i] = 0;

      if (lstrcmpi(ach, szEnvVar) == 0)
        return lpEnv + i + 1;
    }

  // Couldn't find it - just return NULL.
  return NULL;
}



//-------------------------------------------------------------------------
DWORD WINAPI DoEnvironmentSubst
    // The given string is parsed and all environment variables
    // are expanded. If the expansion doesn't over fill the buffer
    // then the length of the new string will be returned in the
    // hiword and TRUE in the low word.  If the expansion would over
    // fill the buffer then the original string is left unexpanded,
    // the original length in the high word and FALSE in the low word.
    // The length of the string is in bytes and excludes the terminating
    // NULL.
    (
    LPSTR sz,    // The input string.
    UINT cbSz    // The limit of characters in the input string inc null.
    )
    {
    LPSTR pch1;                 // Ptr to char in input string..
    LPSTR pchEnvVar;            // Ptr to start of environment variable (LHS).
                                // Actually this points to the last % found.
    BOOL fPercent;              // True if we hit a percent sign.
    char szExpanded[MAXENVSTRLEN+1];       // Expanded string.
    WORD cb;                    // Count of bytes copied.
    LPSTR szEnvVal;             // The value of an environment variable (RHS).
    WORD cbLeft;                // Bytes left in destination string.
    WORD cbszEnvVal;            // Bytes in environment value.
    WORD iEnvVar;               // Byte index to start of env var in
                                // szExpanded (the % actually);
    // Convert String
    AnsiToOem(sz,sz);

    fPercent = FALSE;
    cb = 0;

    for(pch1 = sz; *pch1 && cb < MAXENVSTRLEN; pch1++)
        {
        if (*pch1 == '%')
            {
            if (!fPercent)
                {
                // Found first %, note where it is.
                pchEnvVar = pch1;
                fPercent = TRUE;
                szExpanded[cb++] = *pch1;
                continue;
                }
             else
                {
                // Found a 2nd %.
                // Is the previous char a % ?
                if (pch1 - 1 == pchEnvVar)
                    {
                    // Yep, %% so just ignore this one and look for
                    // another pair.
                    fPercent = FALSE;
                    continue;
                    }
                else
                    {
                    // This is an environment variable.

                    // Stomp on the % to so we can search for just the
                    // word without the %'s.
                    *pch1 = '\0';

                    szEnvVal = FindEnvironmentString(pchEnvVar+1);
                    if (szEnvVal)
                        {
                        iEnvVar = 1+cb-(pch1-pchEnvVar);
                        cbLeft = MAXENVSTRLEN-iEnvVar;
                        cbszEnvVal = lstrlen(szEnvVal);

                        // Check how big the new string is going to be.
                        if (cbLeft < cbszEnvVal)
                            {
                            // It's not going to fit so just quit.
                            goto ExitFalse;
                            }
                        // else just copy it.
                        lstrcpy(&szExpanded[iEnvVar-1], szEnvVal);
                        cb = (iEnvVar+cbszEnvVal)-1;
                        fPercent = FALSE;
                        }
                    else
                        {
                        // Environment variable doesn't exist.
                        // Just copy the % and leave the rest as it is.
                        szExpanded[cb++] = '%';
                        fPercent = FALSE;
                        }
                    // Put the percent back.
                    *pch1 = '%';
                    }
                }
            }
        else
            {
            // just an ordinary char so copy it.
            szExpanded[cb++] = *pch1;
	    if (IsDBCSLeadByte(*pch1))
		{
		if (cb >= MAXENVSTRLEN)
		    {
		    --cb;
		    break;
		    }
		szExpanded[cb++] = *(++pch1);
		}
            }
        } // For.

    // Finish up.
    // Check if we ran out of room.
    if (cb < cbSz)
        {
        // Nope.

        // NULL terminate it.
        szExpanded[cb] = '\0';

        // Copy it back.
        lstrcpy(sz, szExpanded);

        // Convert string back ansi.
        OemToAnsi(sz,sz);

        return MAKELONG(cb,TRUE);
        }

    // Else we ran out of room.
ExitFalse:
    OemToAnsi(sz, sz);
    return MAKELONG(lstrlen(sz),FALSE);
    }
