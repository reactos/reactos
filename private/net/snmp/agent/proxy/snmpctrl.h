/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmpctrl.h

Abstract:

    Service definitions.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef snmpctrl_h
#define snmpctrl_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define SNMP_SERVICE_CONTROL_BASE 128

#define SNMP_SERVICE_LOGLEVEL_BASE SNMP_SERVICE_CONTROL_BASE
#define SNMP_SERVICE_LOGLEVEL_MIN  0
#define SNMP_SERVICE_LOGLEVEL_MAX  20

#define SNMP_SERVICE_LOGTYPE_BASE \
    (SNMP_SERVICE_LOGLEVEL_BASE + SNMP_SERVICE_LOGLEVEL_MAX + 1)
#define SNMP_SERVICE_LOGTYPE_MIN  0
#define SNMP_SERVICE_LOGTYPE_MAX  7


//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------
#if !defined(NT)

#define IDS_STRING_BASE             4096
#define IDS_APP_NAME                IDS_STRING_BASE + 0
#define IDS_TITLE_BAR               IDS_STRING_BASE + 1
#define IDS_HELP_STRING             IDS_STRING_BASE + 2
#define IDS_QUEST_STRING            IDS_STRING_BASE + 3
#define IDS_CLOSE_STRING            IDS_STRING_BASE + 4
#define IDS_DESTROY_STRING          IDS_STRING_BASE + 5
#define IDS_HELP_TEXT1              IDS_STRING_BASE + 6
#define IDS_HELP_TEXT2              IDS_STRING_BASE + 7
#define IDS_HELP_TEXT3              IDS_STRING_BASE + 8

#endif
//------------------------------- END ---------------------------------------

#endif /* snmpctrl_h */
