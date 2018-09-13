#ifndef __HISTORY_H
#define __HISTORY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: history.h

    Description: To prevent the watchdog from excessive repeate notifications,
        the system administrator can set a minimum period for notification
        silence.  This class (CHistory) manages the reading of this setting
        along with remembering the last time an action occured.  A client
        of the object is able to ask it if a given action should be performed
        on the basis of past actions.
        
            CHistory


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef __DSKQUOTA_REG_PARAMS_H
#   include "regparam.h"
#endif

class CPolicy;

class CHistory
{
    public:
        CHistory(CPolicy& policy);
        ~CHistory(VOID) { };

        BOOL ShouldPopupDialog(VOID);
        BOOL ShouldSendEmail(VOID);
        BOOL ShouldDoAnyNotifications(VOID);
        VOID RecordDialogPoppedUp(VOID);
        VOID RecordEmailSent(VOID);
    private:
        RegParamTable m_RegParams;
        CPolicy&      m_policy;    // To get min email/popup periods.

        static VOID GetSysTime(LPFILETIME pftOut);
        static INT CalcDiffMinutes(const FILETIME& ftA, const FILETIME& ftB);

        //
        // Registry parameter value names.
        //
        static const TCHAR SZ_REG_LAST_NOTIFY_EMAIL_TIME[];
        static const TCHAR SZ_REG_LAST_NOTIFY_POPUP_TIME[];

        //
        // Prevent copy.
        //
        CHistory(const CHistory& rhs);
        CHistory& operator = (const CHistory& rhs);
};

#endif //__HISTORY_H
