

#include <windows.h>
#include "common.h"
#include "clipsrv.h"
#include "security.h"
#include "debugout.h"







#if 0
// Debugging code for IsUserLocal - prints out the SIDs that IsUserLocal
// gets.

/*
 *      HexDumpBytes
 */

void HexDumpBytes(
    char        *pv,
    unsigned    cb)
{
char        achHex[]="0123456789ABCDEF";
char        achOut[80];
unsigned    iOut;



    iOut = 0;

    while (cb)
        {
        if (iOut >= 78)
            {
            PINFO(achOut);
            iOut = 0;
            }

        achOut[iOut++] = achHex[(*pv >> 4) & 0x0f];
        achOut[iOut++] = achHex[*pv++ & 0x0f];
        achOut[iOut]   = '\0';
        cb--;
        }

    if (iOut)
        {
        PINFO(achOut);
        }

}








/*
 *      PrintSid
 */

void PrintSid(
    PSID sid)
{
DWORD cSubAuth;
DWORD i;


    PINFO(TEXT("\r\nSID: "));

    HexDumpBytes(GetSidIdentifierAuthority(sid), sizeof(SID_IDENTIFIER_AUTHORITY));

    cSubAuth = *GetSidSubAuthorityCount(sid);

    for (i = 0;i < cSubAuth; i++)
       {
       PINFO(TEXT("-"));
       HexDumpBytes(GetSidSubAuthority(sid, i), sizeof(DWORD));
       }
    PINFO(TEXT("\r\n"));

}

#else
#define PrintSid(x)
#endif









/*
 *      IsUserLocal
 *
 *  Purpose: Determine if the user context we're running in is
 *     interactive or remote.
 *
 *  Parameters: None.
 *
 *  Returns: TRUE if this is a locally logged-on user.
 */

BOOL IsUserLocal (
    HCONV hConv)
{
SID_IDENTIFIER_AUTHORITY NTAuthority = SECURITY_NT_AUTHORITY;
PSID         sidInteractive;
TOKEN_GROUPS *ptokgrp;
HANDLE       hToken;
DWORD        dwInfoSize;
unsigned     i;

CHAR         sz[MAX_USERNAME];
CHAR         szLocal[MAX_USERNAME];

DWORD        dw        = MAX_USERNAME;
DWORD        dwLocal   = MAX_USERNAME;
LPTSTR       lpsz      = &sz[0];
LPTSTR       lpszLocal = &szLocal[0];
BOOL         fRet      = FALSE;



    PINFO(TEXT("IsLocal ? "));

    // *** This is not a complete fix, but makes things better. This issue
    //     needs to be addressed because this fix changes clipbooks functionality
    //     and bypasses security in the case where two user names are the same
    //     of different domains.  Functionality changed because a user can now
    //     log onto a second computer and see non-shared pages.  Old behavior was
    //     that only shared pages could be viewed from a remote location even
    //     if the user was the same that created them.

    GetUserName(lpszLocal,&dwLocal);

    DdeImpersonateClient(hConv);
    GetUserName(lpsz,&dw);
    RevertToSelf();

    if (lstrcmp(lpszLocal,lpsz)==0)
        {
        PINFO(TEXT("User is Local\r\n"));
        return TRUE;
        }
    else
        {
        PINFO(TEXT("User is Not Local\r\n"));
        return FALSE;
        }



    // *** //

    if (!GetTokenHandle(&hToken))
        {
        PERROR(TEXT("IsUserLocal: Couldn't get token handle\r\n"));
        }
    else if (!AllocateAndInitializeSid (&NTAuthority, 1, SECURITY_INTERACTIVE_RID,
                                         0, 0, 0, 0, 0, 0, 0, &sidInteractive))
        {
        PERROR(TEXT("IsUserLocal: Couldn't get interactive SID\r\n"));
        }
    else
        {
        PrintSid(sidInteractive);

        GetTokenInformation(hToken, TokenGroups, ptokgrp, 0, &dwInfoSize);
        ptokgrp = LocalAlloc(LPTR, dwInfoSize);
        if (GetTokenInformation(hToken, TokenGroups, ptokgrp,
                 dwInfoSize, &dwInfoSize))
            {
            for (i = 0;i < ptokgrp->GroupCount;i++)
                {
                PrintSid(ptokgrp->Groups[i].Sid);

                if (EqualSid(ptokgrp->Groups[i].Sid, sidInteractive))
                    {
                    PINFO(TEXT("YES"));
                    fRet = TRUE;
                    break;
                    }
                else
                    {
                    PINFO(TEXT("no "));
                    }
                }
            }
        LocalFree(ptokgrp);

        FreeSid(sidInteractive);
        }



    PINFO(TEXT("\r\n"));

    return fRet;


}
