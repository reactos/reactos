/*****************************************************************/
/**                  Microsoft Windows for Workgroups                **/
/**              Copyright (C) Microsoft Corp., 1991-1992            **/
/*****************************************************************/ 

/* NPMSG.CPP -- Implementation of MsgBox subroutine.
 *
 * History:
 *    05/06/93    gregj    Created
 */

#include "npcommon.h"
#include "npmsg.h"
#include "npstring.h"

#include <mluisupp.h>

extern "C" {
#include <netlib.h>
};

LPSTR pszTitle = NULL;

int MsgBox( HWND hwndOwner, UINT idMsg, UINT wFlags, const NLS_STR **apnls /* = NULL */ )
{
    if (pszTitle == NULL) {
        pszTitle = new char[MAX_RES_STR_LEN];
        if (pszTitle != NULL) {
            MLLoadString(IDS_MSGTITLE, pszTitle, MAX_RES_STR_LEN );
            UINT cbTitle = ::strlenf(pszTitle) + 1;
            delete pszTitle;
            pszTitle = new char[cbTitle];
            if (pszTitle != NULL)
                MLLoadStringA(IDS_MSGTITLE, pszTitle, cbTitle);
        }
    }
    NLS_STR nlsMsg( MAX_RES_STR_LEN );
    if (apnls == NULL)
        nlsMsg.LoadString((unsigned short) idMsg );
    else
        nlsMsg.LoadString((unsigned short) idMsg, apnls );
    return ::MessageBox( hwndOwner, nlsMsg, pszTitle, wFlags | MB_SETFOREGROUND );
}
