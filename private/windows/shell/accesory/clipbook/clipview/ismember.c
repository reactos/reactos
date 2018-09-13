
/*****************************************************************************

                            I S M E M B E R

    Name:       ismember.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This file contains the function to check the user is a member
        of a given group.

    History:
        21-Jan-1994     John Fu, reformat and cleanup.

*****************************************************************************/


#include <windows.h>
#include "clipbook.h"
#include "ismember.h"
#include "security.h"
#include "debugout.h"




/*
 *      IsUserMember
 *
 *  Purpose: Determine if the current user is a member of the given group.
 *
 *  Parameters:
 *     psidGroup - Pointer to a SID describing the group.
 *
 *  Returns: TRUE if the user is a member of the group, FALSE
 *     otherwise
 */


BOOL IsUserMember(
    PSID    psidGroup)
{
TOKEN_GROUPS    *ptokgrp;
HANDLE          hToken;
BOOL            fRet = FALSE;
DWORD           dwInfoSize;
unsigned        i;



    PINFO(TEXT("IsMember of ? "));
    PrintSid(psidGroup);

    if (!GetTokenHandle(&hToken))
        {
        PERROR(TEXT("IsUserMember: Couldn't get token handle\r\n"));
        return FALSE;
        }


    GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwInfoSize);

    if (ptokgrp = LocalAlloc(LPTR, dwInfoSize))
        {
        if (GetTokenInformation(hToken, TokenGroups, ptokgrp,
                 dwInfoSize, &dwInfoSize))
            {
            for (i = 0;i < ptokgrp->GroupCount;i++)
                {
                PrintSid(ptokgrp->Groups[i].Sid);

                if (EqualSid(ptokgrp->Groups[i].Sid, psidGroup))
                    {
                    PINFO(TEXT("YES"));
                    fRet = TRUE;
                    break;
                    }
                }
            }
        LocalFree(ptokgrp);
        }


    if (!fRet)
        {
        TOKEN_USER *ptokusr;

        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwInfoSize);

        if (ptokusr = LocalAlloc(LPTR, dwInfoSize))
            {
            if (GetTokenInformation(hToken, TokenUser, ptokusr,
                  dwInfoSize, &dwInfoSize))
                {
                if (EqualSid(ptokusr->User.Sid, psidGroup))
                    {
                    PINFO(TEXT("YES"));
                    fRet = TRUE;
                    }
                }
            LocalFree(ptokusr);
            }
        }



    PINFO(TEXT("\r\n"));

    return fRet;

}
