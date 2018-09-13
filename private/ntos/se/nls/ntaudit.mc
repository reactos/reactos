;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    ntaudit.mc
;
;Abstract:
;
;    Constant definitions for the NT Audit Event Messages.
;
;Author:
;
;    Jim Kelly (JimK) 30-Mar-1992
;
;Revision History:
;
;Notes:
;
;    The .h and .res forms of this file are generated from the .mc
;    form of the file (private\ntos\se\nls\ntaudit.mc).  Please make
;    all changes to the .mc form of the file.
;
;
;
;--*/
;
;#ifndef _NTAUDIT_
;#define _NTAUDIT_
;
;/*lint -e767 */  // Don't complain about different definitions // winnt


MessageIdTypedef=ULONG

SeverityNames=(None=0x0)

FacilityNames=(None=0x0)



MessageId=0x0000
        Language=English
Unused message ID
.
;// Message ID 0 is unused - just used to flush out the diagram


;
;/////////////////////////////////////////////////////////////////////////
;//                                                                     //
;// Logon Messages Follow                                               //
;//                                                                     //
;//                                                                     //
;/////////////////////////////////////////////////////////////////////////
;

;////////////////////////////////////////////////
;//
;//   Module Catagory:  SE_ADT_SUCCESSFUL_LOGON
;//
;//        Event Type:  Successful Logon
;//
;// Parameter Strings:
;//
;//      String1 - User name
;//
;//      String2 - LogonSessionLuid.HighPart (32-bit value)
;//
;//      String3 - LogonSessionLuid.LowPart (32-bit value)
;//

MessageId=0x0001
        SymbolicName=SE_EVENTID_SUCCESSFUL_LOGON
        Language=English
Successful Logon -
             User: %S
             Logon session ID: {%ld,%ld}.
.


;////////////////////////////////////////////////
;//
;//   Module Catagory:  SE_ADT_UNSUCCESSFUL_LOGON
;//
;//        Event Type:  Unknown user/password logon attempt
;//
;// Parameter Strings:
;//
;//      String1 - User name
;//
;//
;//

MessageId=0x0002
        SymbolicName=SE_EVENTID_UNKNOWN_USER_OR_PWD
        Language=English
Failed Logon -
             User: %S
             Reason: Unknown user name or password.
.


;////////////////////////////////////////////////
;//
;//   Module Catagory:  SE_ADT_UNSUCCESSFUL_LOGON
;//
;//        Event Type:  Time restriction logon failure
;//
;// Parameter Strings:
;//
;//      String1 - User name
;//
;//
;//

MessageId=0x0003
        SymbolicName=SE_EVENTID_ACCOUNT_TIME_RESTR
        Language=English
Failed Logon -
             User: %S
             Reason: Account time restriction violation.
.


;////////////////////////////////////////////////
;//
;//   Module Catagory:  SE_ADT_UNSUCCESSFUL_LOGON
;//
;//        Event Type:  Account Disabled
;//
;// Parameter Strings:
;//
;//      String1 - User name
;//
;//
;//

MessageId=0x0004
        SymbolicName=SE_EVENTID_ACCOUNT_DISABLED
        Language=English
Failed Logon -
             User: %S
             Reason: Account Disabled.
.



;/*lint +e767 */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _NTAUDIT_
