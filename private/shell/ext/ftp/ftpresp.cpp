/*****************************************************************************
 *
 *    ftpresp.cpp - Parsing FTP responses
 *
 *****************************************************************************/

#include "priv.h"


/*****************************************************************************\
    FUNCTION: FindEndOfStrOrLine

    DESCRIPTION:
        Find the end of the line ('\n') or the end of the string ('\0').
\*****************************************************************************/
LPWIRESTR FindEndOfStrOrLine(LPWIRESTR pszString)
{
    while (*pszString != '\0')
    {
        if (('\n' == pszString[0]))
        {
            while (('\n' == pszString[0]))
               pszString++;

            break;
        }
        pszString++;
    }

    return pszString;
}


/*****************************************************************************\
    FUNCTION: FindFirstMajorResponse

    DESCRIPTION:
\*****************************************************************************/
LPWIRESTR FindFirstMajorResponse(LPWIRESTR pszResponse)
{
    while ((pszResponse[0]) && ('-' != pszResponse[3]))
        pszResponse = FindEndOfStrOrLine(pszResponse);

    return pszResponse;
}


/*****************************************************************************\
    FUNCTION: GetNextResponseSection

    DESCRIPTION:
\*****************************************************************************/
LPWIRESTR GetNextResponseSection(LPWIRESTR pszCompleteResponse, LPWIRESTR * ppszResponseStart)
{
    LPWIRESTR pszNextResponse = NULL;

    // There may be a few minor responses.  Skip over them...
    pszCompleteResponse = FindFirstMajorResponse(pszCompleteResponse);

    // Were we never able to fine a major response?
    if (!pszCompleteResponse[0])
        return NULL;    // No, so return failure.

    // We are off to find the next major response.
    //    We should be looking at a response code.
    ASSERT('-' == pszCompleteResponse[3]);

    // Slop saves us here
    //    Extended response.  Copy until we see the match.
    //  As we copy, we also clean up the lines, removing
    //  the random punctuation servers prepend to continuations.
    //
    //  wu-ftp prepends the extended response code to each line:
    //
    //  230-Welcome to ftp.foo.com.  Please read the rules
    //  230-and regulations in the file RULES.
    //  230 Guest login ok, access restrictions apply.
    //
    //  Microsoft Internet Information Server prepends a space:
    //
    //  230-This is ftp.microsoft.com.  See the index.txt file
    //   in the root directory for more information.
    //  230 Anonymous user logged in as anonymous.
    //
    WIRECHAR szResponseNumber[5];            // example: "230-"
    WIRECHAR szResponseEnd[5];                // example: "230 "
    StrCpyNA(szResponseNumber, pszCompleteResponse, ARRAYSIZE(szResponseNumber));
    ASSERT(4 == lstrlenA(szResponseNumber));
    StrCpyNA(szResponseEnd, szResponseNumber, ARRAYSIZE(szResponseEnd));
    szResponseEnd[3] = ' ';

    pszNextResponse = pszCompleteResponse;
    *ppszResponseStart = pszCompleteResponse;
    do
    {
        //  Skip past the header.
        if (!StrCmpNA(szResponseNumber, pszNextResponse, 4))
            pszNextResponse += 4;    // wu-ftp
        else if ((pszNextResponse[0] == ' ') && (!StrCmpNA(szResponseNumber, &pszNextResponse[1], 4)))
            pszNextResponse += 5;    // ftp.microsoft.com
        else if (pszNextResponse[0] == ' ')
            pszNextResponse++;    // IIS

        //  Skip the rest of the line.
        pszNextResponse = FindEndOfStrOrLine(pszNextResponse);
    }
    while (pszNextResponse[0] && StrCmpNA(pszNextResponse, szResponseEnd, 4));
        /* Now gobble the trailer */

    if ('\0' == pszNextResponse[0])
        pszNextResponse = NULL;     // We are at the end.

    return pszNextResponse;
}


/*****************************************************************************\
    FUNCTION: StripResponseHeaders

    DESCRIPTION:
\*****************************************************************************/
void StripResponseHeaders(LPWIRESTR pszResponse)
{
    //    We should be looking at a response code.
    if ((3 < lstrlenA(pszResponse)) && (pszResponse[3] == '-'))
    {
        LPWIRESTR pszIterator = pszResponse;
        WIRECHAR szResponseNumber[5];            // example: "230-"
        WIRECHAR szResponseEnd[5];                // example: "230 "
        BOOL fFirstPass = TRUE;

        StrCpyNA(szResponseNumber, pszResponse, ARRAYSIZE(szResponseNumber));
        ASSERT(4 == lstrlenA(szResponseNumber));
        StrCpyNA(szResponseEnd, szResponseNumber, ARRAYSIZE(szResponseEnd));
        szResponseEnd[3] = ' ';

        do
        {
            //  Skip past the header.
            if (!StrCmpNA(szResponseNumber, pszIterator, 4))
                RemoveCharsFromStringA(pszIterator, 3);    // wu-ftp
            else if ((pszIterator[0] == ' ') && (!StrCmpNA(szResponseNumber, &pszIterator[1], 4)))
                RemoveCharsFromStringA(pszIterator, 4);    // ftp.microsoft.com
            else if (pszIterator[0] == ' ')
                NULL;    // IIS

            if (fFirstPass)
            {
                fFirstPass = FALSE;
                RemoveCharsFromStringA(pszIterator, 1);    // IIS
            }
            else
                pszIterator[0] = ' ';    // Make that new line a space.

            //  Skip the rest of the line.
            pszIterator = FindEndOfStrOrLine(pszIterator);
        }
        while (pszIterator[0] && StrCmpNA(pszIterator, szResponseEnd, 4));
        
        RemoveCharsFromStringA(pszIterator, 4);         // Now gobble the trailer
    }
}


/*****************************************************************************\
    FUNCTION: GetMOTDMessage

    DESCRIPTION:
\*****************************************************************************/
LPWIRESTR GetMOTDMessage(LPWIRESTR pwResponse, DWORD cchResponse)
{
    LPWIRESTR pszMOTD = NULL;
    LPWIRESTR pszLast = &pwResponse[lstrlenA(pwResponse)];
    LPWIRESTR pszNext = pwResponse;
    LPWIRESTR pszEnd = NULL;

    while (pszNext = GetNextResponseSection(pszNext, &pszLast))
        pszEnd = pszNext;

    if (pszEnd)
        pszEnd[0] = '\0';   // Terminate it so we don't get the minor responses after our response.
    pszMOTD = (LPWIRESTR) GlobalAlloc(GPTR, (lstrlenA(pszLast) + 1) * sizeof(WIRECHAR));
    if (EVAL(pszMOTD))
    {
        StrCpyA(pszMOTD, pszLast);
        StripResponseHeaders(pszMOTD);
    }

    return pszMOTD;
}


/*****************************************************************************\
    FUNCTION: GetFtpResponse

    DESCRIPTION:
        Get the MOTD from the Response
\*****************************************************************************/
CFtpGlob * GetFtpResponse(CWireEncoding * pwe)
{
    CFtpGlob * pfg = NULL;
    DWORD cchResponse = 0;
    LPWIRESTR pwWireResponse;
    DWORD dwError;

    InternetGetLastResponseInfoWrap(TRUE, &dwError, NULL, &cchResponse);
    cchResponse++;                /* +1 for the terminating 0 */

    pwWireResponse = (LPWIRESTR)LocalAlloc(LPTR, cchResponse * sizeof(WIRECHAR));
    if (EVAL(pwWireResponse))
    {
        if (SUCCEEDED(InternetGetLastResponseInfoWrap(TRUE, &dwError, pwWireResponse, &cchResponse)))
        {
            LPWIRESTR pwMOTD = GetMOTDMessage(pwWireResponse, cchResponse);
            if (pwMOTD)
            {
                LPWSTR pwzDisplayMOTD;
                DWORD cchSize = (lstrlenA(pwMOTD) + 1);

                pwzDisplayMOTD = (LPWSTR)GlobalAlloc(LPTR, cchSize * sizeof(WCHAR));
                if (pwzDisplayMOTD)
                {
                    pwe->WireBytesToUnicode(NULL, pwMOTD, WIREENC_IMPROVE_ACCURACY, pwzDisplayMOTD, cchSize);

                    pfg = CFtpGlob_CreateStr(pwzDisplayMOTD);
                    if (!(EVAL(pfg)))
                        GlobalFree(pwzDisplayMOTD);    // Couldn't track message
                }

                GlobalFree(pwMOTD);
            }
        }
        LocalFree(pwWireResponse);
    }

    return pfg;
}


