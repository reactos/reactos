/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       PARSE.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        1 Jan, 1997
*
*  DESCRIPTION:
*   Helper parsing code for the default power schemes generator, MAKEINI.EXE.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <ntpoapi.h>

#include "parse.h"

extern char *g_pszLines[MAX_LINES];

/*******************************************************************************
*
*  StrTrimTrailingBlanks
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void StrTrimTrailingBlanks(char *psz)
{
    UINT i = 0;

    if (psz) {
        while (*psz) {
            psz++;
        }

        while (*--psz == ' ') {
            *psz = '\0';
        }
    }
}

/*******************************************************************************
*
*  StrToUpper
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void StrToUpper(char *pszDest, char *pszSrc)
{
    UINT i = 0;

    while (*pszSrc) {
        *pszDest = (char)toupper(*pszSrc);
        pszSrc++;
        pszDest++;
        if (++i == MAX_STR) {
            DefFatalExit(FALSE, "StrToUpper failure, source too large: %s\n", pszSrc);
        }
    }
    *pszDest = '\0';
}

/*******************************************************************************
*
*  GetTokens
*
*  DESCRIPTION:
*   Fill an array with tokens.
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetTokens(
    char    *pszSrc,
    UINT    uiMaxTokenSize,
    char    **pszTokens,
    UINT    uiMaxTokens,
    char    *pszDelimiters
)
{
    char    *psz;
    DWORD   dwSize;
    UINT    i = 0;

    psz = strtok(pszSrc, pszDelimiters);
    StrTrimTrailingBlanks(psz);

    while (psz) {

        if (i % 2) {
            printf(".");
        }
        dwSize = strlen(psz) + 1;
        if (dwSize > uiMaxTokenSize) {
            printf("GetTokens, Token to large: %s\n", psz);
            return 0;
        }
        if ((pszTokens[i] = (char *) malloc(dwSize)) != NULL) {
            strcpy(pszTokens[i], psz);
        }
        else {
            printf("GetTokens, Unable to allocate token buffer: %s\n", psz);
            return 0;
        }
        i++;
        if (i == uiMaxTokens) {
            printf("GetTokens, Too many tokens: %d\n", i);
            return 0;
        }
        psz = strtok(NULL, pszDelimiters);
        StrTrimTrailingBlanks(psz);
    }
    return i;
}

/*******************************************************************************
*
*  GetCheckLabelToken
*
*  DESCRIPTION:
*   Consume a label token. Check to be sure it matches the passed parameter.
*   Call fatal exit if it doesn't. Labels are always comma delimited. Sets up
*   strtok for subsequent calls.
*
*  PARAMETERS:
*   uiLine  - One based line index.
*
*******************************************************************************/

VOID GetCheckLabelToken(UINT uiLine, char *pszCheck)
{
    char szUpperCheck[MAX_STR];
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;

    pszTok = strtok(g_pszLines[uiLine - 1], DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE, "GetStringTokens failure, out of tokens. Last token: %s\n", szUpperTok);
    }

    StrToUpper(szUpperTok, pszTok);
    StrToUpper(szUpperCheck, pszCheck);

    if (strcmp(szUpperCheck, szUpperTok)) {
        DefFatalExit(FALSE, "GetStringTokens failure, check: %s doesn't match: %s\n",szUpperCheck, szUpperTok);
    }
}

/*******************************************************************************
*
*  GetPowerActionToken
*
*  DESCRIPTION:
*   Consume a token and return a power action.
*
*  PARAMETERS:
*
*******************************************************************************/

POWER_ACTION GetPowerActionToken(VOID)
{
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE,"GetPowerActionToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);

    if (!strcmp(szUpperTok, "NONE")) {
        return PowerActionNone;
    }

    if (!strcmp(szUpperTok, "DOZE")) {
        return PowerActionReserved;
    }

    if (!strcmp(szUpperTok, "SLEEP")) {
        return PowerActionSleep;
    }

    if (!strcmp(szUpperTok, "HIBERNATE")) {
        return PowerActionHibernate;
    }

    if (!strcmp(szUpperTok, "SHUTDOWN")) {
        return PowerActionShutdown;
    }

    if (!strcmp(szUpperTok, "SHUTDOWNRESET")) {
        return PowerActionShutdownReset;
    }

    if (!strcmp(szUpperTok, "SHUTDOWNOFF")) {
        return PowerActionShutdownOff;
    }

    DefFatalExit(FALSE,"GetPowerActionToken failure, check: %s doesn't match: \nNONE, DOZE, SLEEP, SHUTDOWN, SHUTDOWNRESET or SHUTDOWNOFF\n", szUpperTok);
}

/*******************************************************************************
*
*  GetFlagToken
*
*  DESCRIPTION:
*   Consume a token and return a flag value.
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetFlagToken(UINT uiFlag)
{
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE, "GetFlagToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);

    if (!strcmp(szUpperTok, "YES")) {
        return uiFlag;
    }

    if (!strcmp(szUpperTok, "TRUE")) {
        return uiFlag;
    }

    if (!strcmp(szUpperTok, "FALSE")) {
        return 0;
    }

    if (!strcmp(szUpperTok, "NO")) {
        return 0;
    }

    if (!strcmp(szUpperTok, "N/A")) {
        return 0;
    }
    DefFatalExit(FALSE, "GetFlagToken failure, check: %s doesn't match: YES, NO or N/A\n", szUpperTok);
}

/*******************************************************************************
*
*  GetPowerStateToken
*
*  DESCRIPTION:
*   Consume a token and return a power state.
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetPowerStateToken(VOID)
{
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE,"GetPowerStateToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);

    if (!strcmp(szUpperTok, "S0")) {
        return PowerSystemWorking;
    }
    if (!strcmp(szUpperTok, "S1")) {
        return PowerSystemSleeping1;
    }
    if (!strcmp(szUpperTok, "S2")) {
        return PowerSystemSleeping2;
    }
    if (!strcmp(szUpperTok, "S3")) {
        return PowerSystemSleeping3;
    }
    if (!strcmp(szUpperTok, "S4")) {
        return PowerSystemHibernate;
    }
    if (!strcmp(szUpperTok, "S5")) {
        return PowerSystemShutdown;
    }

    if (!strcmp(szUpperTok, "N/A")) {
        return PowerSystemUnspecified;
    }
    DefFatalExit(FALSE,"GetPowerStateToken failure, check: %s doesn't match: S0, S1, S2, S3, S4 or S5\n", szUpperTok);
}

/*******************************************************************************
*
*  GetINFTypeToken
*
*  DESCRIPTION:
*   Consume a token and return an INF type.
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetINFTypeToken(VOID)
{
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;
    UINT uiRet = 0;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE,"GetINFTypeToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);
    if (strstr(szUpperTok, "TYPICALINSTALL")) {
        uiRet |= TYPICAL;
    }
    if (strstr(szUpperTok, "COMPACTINSTALL")) {
        uiRet |= COMPACT;
    }
    if (strstr(szUpperTok, "CUSTOMINSTALL")) {
        uiRet |= CUSTOM;
    }
    if (strstr(szUpperTok, "PORTABLEINSTALL")) {
        uiRet |= PORTABLE;
    }
    if (strstr(szUpperTok, "SERVERINSTALL")) {
        uiRet |= SERVER;
    }

    if (!uiRet) {
        DefFatalExit(FALSE,"GetINFTypeToken failure, check: %s doesn't match install file type\n", szUpperTok);
    }
    return uiRet;
}

/*******************************************************************************
*
*  GetOSTypeToken
*
*  DESCRIPTION:
*   Consume a token and return an OS type.
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetOSTypeToken(VOID)
{
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;
    UINT uiRet = 0;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE,"GetOSTypeToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);

    if (strstr(szUpperTok, "WIN95")) {
        uiRet |= WIN_95;
    }
    if (strstr(szUpperTok, "NT")) {
        uiRet |= WIN_NT;
    }

    if (!uiRet) {
        DefFatalExit(FALSE,"GetOSTypeToken failure, check: %s doesn't match: WINNT, WIN95\n", szUpperTok);
    }
    return uiRet;
}



/*******************************************************************************
*
*  GetIntToken
*
*  DESCRIPTION:
*   Consume a token and return an integer. Verify the units if passed.
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetIntToken(char *pszUnits)
{
    char szUpperUnits[MAX_STR];
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;
    UINT i, uiMult = 1;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE,"GetIntToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);

    if (!strcmp(szUpperTok, "N/A")) {
        return 0;
    }

    if (!strcmp(szUpperTok, "DISABLED")) {
        return 0;
    }

    if (pszUnits) {
        StrToUpper(szUpperUnits, pszUnits);

        if ((pszUnits = strstr(szUpperTok, szUpperUnits)) == NULL) {
            DefFatalExit(FALSE, "GetIntToken failure, units: %s doesn't match: %s\n", szUpperTok, szUpperUnits);
        }

        if (!strcmp(szUpperUnits, "%")) {
            uiMult = 1;
        }
        else {
            if (!strcmp(szUpperUnits, "MIN")) {
                uiMult = 60;
            }
            else {
                DefFatalExit(FALSE, "GetIntToken failure, unknown  units: %s\n", szUpperUnits);
            }
        }
        // Strip off units.
        *pszUnits = '\0';
    }

    if (sscanf(szUpperTok, "%d", &i) == 1) {
        return i * uiMult;
    }
    DefFatalExit(FALSE,"GetIntToken failure, error converting: %s to integer\n", szUpperTok);
}

/*******************************************************************************
*
*  GetNAToken
*
*  DESCRIPTION:
*   Consume a N/A token.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID GetNAToken(VOID)
{
    static char szUpperTok[MAX_STR] = "No last token";
    char *pszTok;

    pszTok = strtok(NULL, DELIMITERS);
    StrTrimTrailingBlanks(pszTok);

    if (!pszTok) {
        DefFatalExit(FALSE, "GetNAToken failure, out of tokens. Last token: %s\n", szUpperTok);
    }
    StrToUpper(szUpperTok, pszTok);

    if (!strcmp(szUpperTok, "N/A")) {
        return;
    }
    DefFatalExit(FALSE, "GetNAToken failure, check: %s doesn't match: N/A\n", szUpperTok);
}

