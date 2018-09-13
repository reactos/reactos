
/*****************************************************************************

                                A U D I T

    Name:       audit.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:

*****************************************************************************/



#include <windows.h>
#include "clipbook.h"
#include "auditchk.h"



//////////////////////////////////////////////////////////////////////////
//
// Purpose: Tests, enables, or disables the Security privilege, which
//    allows auditing to take place.
//
// Parameters:
//    fAudit - Flag, which can take on one of these values:
//       AUDIT_PRIVILEGE_CHECK - Turns on Security, then turns it off.
//          Used to test whether you CAN edit auditing.
//       AUDIT_PRIVILEGE_ON    - Turns on auditing privilege.
//       AUDIT_PRIVILEGE_OFF   - Turns off auditing privilege.
//
// Return: TRUE if the function succeeds, FALSE on failure.
//
//////////////////////////////////////////////////////////////////////////

BOOL AuditPrivilege(
    int fAudit)
{
HANDLE              hToken;
LUID                SecurityValue;
TOKEN_PRIVILEGES    tkp;
BOOL                fOK = FALSE;


    /* Retrieve a handle of the access token. */

    if (OpenProcessToken (GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
        {
        /*
         * Enable the SE_SECURITY_NAME privilege or disable
         * all privileges, depending on the fEnable flag.
         */

        if (LookupPrivilegeValue ((LPSTR)NULL,
                                  SE_SECURITY_NAME,
                                  &SecurityValue))
            {
            tkp.PrivilegeCount     = 1;
            tkp.Privileges[0].Luid = SecurityValue;


            // Try to turn on audit privilege

            if (AUDIT_PRIVILEGE_CHECK == fAudit || AUDIT_PRIVILEGE_ON == fAudit)
                {
                tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                AdjustTokenPrivileges (hToken,
                                       FALSE,
                                       &tkp,
                                       sizeof(TOKEN_PRIVILEGES),
                                       (PTOKEN_PRIVILEGES)NULL,
                                       (PDWORD)NULL);

                /* The return value of AdjustTokenPrivileges be texted. */
                if (GetLastError () == ERROR_SUCCESS)
                    {
                    fOK = TRUE;
                    }
                }


            // Try to turn OFF audit privilege

            if (AUDIT_PRIVILEGE_CHECK == fAudit || AUDIT_PRIVILEGE_OFF == fAudit)
                {
                AdjustTokenPrivileges (hToken,
                                       TRUE,
                                       NULL,
                                       0L,
                                       (PTOKEN_PRIVILEGES)NULL,
                                       (PDWORD)NULL);

                if (ERROR_SUCCESS == GetLastError () &&
                    AUDIT_PRIVILEGE_OFF == fAudit)
                    {
                    fOK = TRUE;
                    }
                }
            }
        }

    return fOK;

}
