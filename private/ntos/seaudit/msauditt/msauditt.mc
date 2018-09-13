;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msauditt.mc
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
;    form of the file (private\ntos\seaudit\msauditt\msauditt.mc).  Please make
;    all changes to the .mc form of the file.
;
;
;
;--*/
;
;#ifndef _MSAUDITT_
;#define _MSAUDITT_
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
;//        Type:  SE_ADT_SUCCESSFUL_LOGON
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//

MessageId=0x0001
        SymbolicName=SE_ADT_SUCCESSFUL_LOGON
        Language=English
Successful Logon -
             Successful Logon
.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_UNSUCCESSFUL_LOGON
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//
;//

MessageId=0x0002
        SymbolicName=SE_ADT_UNSUCCESSFUL_LOGON
        Language=English
Failed Logon -
             Unsuccessful Logon
.


;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SUCCESSFUL_OBJECT_OPEN
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//
;//

MessageId=0x0003
        SymbolicName=SE_ADT_SUCCESSFUL_OBJECT_OPEN
        Language=English
Successful Object Open -
        Successful Object Open
.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_UNSUCCESSFUL_OBJECT_OPEN
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//
;//

MessageId=0x0004
        SymbolicName=SE_ADT_UNSUCCESSFUL_OBJECT_OPEN
        Language=English
Unsuccessful Object Open -
        Unsuccessful Object Open
.


;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SYSTEM_RESTART
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x0005
        SymbolicName=SE_ADT_SYSTEM_RESTART
        Language=English
System has been rebooted -
        System has been rebooted
.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_HANDLE_ALLOCATION
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x0006
        SymbolicName=SE_ADT_HANDLE_ALLOCATION
        Language=English
A handle has been allocated
        A handle has been allocated
.


;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SUCC_PRIV_SERVICE
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//

MessageId=0x0007
        SymbolicName=SE_ADT_SUCC_PRIV_SERVICE
        Language=English
A privilege was successfully used in a system service
        A privilege was successfully used in a system service

.


;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_FAILED_PRIV_SERVICE
;//
;//
;// Parameter Strings:
;//
;//
;//

MessageId=0x0008
        SymbolicName=SE_ADT_FAILED_PRIV_SERVICE
        Language=English
A privilege check in a system service failed
        A privilege check in a system service failed

.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SUCC_PRIV_OBJECT
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x0009
        SymbolicName=SE_ADT_SUCC_PRIV_OBJECT
        Language=English
An attempt to access a privileged object succeeded
        An attempt to access a privileged object succeeded

.


;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_FAILED_PRIV_OBJECT
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x000A
        SymbolicName=SE_ADT_FAILED_PRIV_OBJECT
        Language=English
An attempt to access a privileged object failed
        An attempt to access a privileged object failed

.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SUCC_PRIV_SERVICE
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x000B
        SymbolicName=SE_ADT_SUCC_PRIV_SERVICE
        Language=English
An attempt to execute a privileged service succeeded.
        An attempt to execute a privileged service succeeded.

.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_OBJECT_CLOSE
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x000C
        SymbolicName=SE_ADT_OBJECT_CLOSE
        Language=English
An object was closed
        An object was closed

.




;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SUCC_OBJECT_REFERENCE
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x000D
        SymbolicName=SE_ADT_SUCC_OBJECT_REFERENCE
        Language=English
A named object was accessed but no handle was created
        A named object was accessed but no handle was created

.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_FAILED_OBJECT_REFERENCE
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x000E
        SymbolicName=SE_ADT_FAILED_OBJECT_REFERENCE
        Language=English
An attempt to reference a named object was denied
        An attempt to reference a named object was denied

.



;////////////////////////////////////////////////
;//
;//        Type:  SE_ADT_SHUTDOWN
;//
;//
;// Parameter Strings:
;//
;//      String1 - Description
;//
;//

MessageId=0x000E
        SymbolicName=SE_ADT_SHUTDOWN
        Language=English
The system was shut down.
        The system was shut down.

.


;/*lint +e767 */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _MSAUDITT_
