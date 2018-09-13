#ifndef __WATCHDOG_H
#define __WATCHDOG_H
///////////////////////////////////////////////////////////////////////////////
/*  File: watchdog.h

    Description: The CWatchDog class is the main control object for the
        disk quota watchdog applet.  A client merely creates a CWatchDog
        and tells it to "Run()".

            CWatchDog

        To run, the object does the following:

        1. Enumerates all local and connected volumes on the machine.
        2. For any volumes that support quotas, quota statistics are 
           gathered for both the volume and the local user on the volume.
           Statistics are maintained in a list of CStatistics objects; one
           object for each volume/user pair.
        3. Once all information has been gathered, a list of action objects
           (CActionEmail, CActionPopup) are created.  System policy and 
           previous notification history are considered when creating action
           objects.
        4. When all action objects are created, they are performed.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _SHLOBJ_H_
#   include <shlobj.h>
#endif

#ifndef __MAPISEND_H
#   include "mapisend.h"
#endif

#ifndef __STATS_H
#   include "stats.h"
#endif

#ifndef __POLICY_H
#   include "policy.h"
#endif

#ifndef __HISTORY_H
#   include "history.h"
#endif

#ifndef __ACTION_H
#   include "action.h"
#endif

//
// This guy controls the whole thing.
// Just create a CWatchDog object and call Run().
// That's all there is to it.
//
class CWatchDog
{
    public:
        CWatchDog(HANDLE htokenUser);
        ~CWatchDog(VOID);

        HRESULT Run(VOID);

    private:
        HANDLE          m_htokenUser;   // User account token.
        CPolicy         m_policy;       // Controls what actions occur.
        CHistory        m_history;      // Controls notification frequency.
        CStatisticsList m_statsList;    // User & Volume quota info.
        CArray<CAction *> m_actionList; // List of actions to perform.
        CMapiSession    m_mapiSession;  // MAPI session object for sending email

        HRESULT GatherQuotaStatistics(IShellFolder *psfDrives);

        HRESULT BuildActionList(VOID);

        HRESULT BuildPopupDialogActions(VOID);

        HRESULT BuildEmailActions(VOID);

        HRESULT DoActions(VOID);

        VOID ClearActionList(VOID);

        LPBYTE GetUserSid(HANDLE htokenUser);

        BOOL ShouldReportTheseStats(const CStatistics& stats);

        BOOL ShouldSendEmail(VOID);

        BOOL ShouldPopupDialog(VOID);

        BOOL ShouldDoAnyNotifications(VOID);

        //
        // Prevent copy.
        //
        CWatchDog(const CWatchDog& rhs);
        CWatchDog& operator = (const CWatchDog& rhs);
};

#endif // __WATCHDOG_H

