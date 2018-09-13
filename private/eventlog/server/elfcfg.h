/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    elfcfg.h

Abstract:

    This file contains default settings for the eventlog service.

Author:

    Rajen Shah (rajens) 16-Aug-1991

Revision History:

--*/

#ifndef _EVENTDEFAULTS_
#define _EVENTDEFAULTS_

//
// Default for the Application log file
//
//

#define     ELF_DEFAULT_MODULE_NAME           ELF_APPLICATION_MODULE_NAME
#define     ELF_APPLICATION_DEFAULT_LOG_FILE  L"\\SystemRoot\\system32\\config\\appevent.evt"
#define     ELF_SYSTEM_DEFAULT_LOG_FILE       L"\\SystemRoot\\system32\\config\\sysevent.evt"
#define     ELF_SECURITY_DEFAULT_LOG_FILE     L"\\SystemRoot\\system32\\config\\secevent.evt"
#define     ELF_DEFAULT_MAX_FILE_SIZE         512*1024
#define     ELF_DEFAULT_RETENTION_PERIOD      1*24*3600

#define     ELF_GUEST_ACCESS_UNRESTRICTED     0
#define     ELF_GUEST_ACCESS_RESTRICTED       1

//
// Maximum size for the buffer that will read the key values from the
// registry.
//

#define     ELF_MAX_REG_KEY_INFO_SIZE       200

//
// String defines for the pre-defined nodes in the registry
// These are used to get to the appropriate nodes.
//

#define     REG_EVENTLOG_NODE_PATH  \
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Eventlog"

//SS: start of changes for replicated event logging across the cluster
#define     REG_CLUSSVC_NODE_PATH  \
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ClusSvc"

//SS: end of changes for replicated event logging across the cluster

//
// String defines for the values for each of the configured pieces of
// information in the eventlog\logfiles node.  These exist per logfile.
//
//

#define     VALUE_FILENAME              L"File"
#define     VALUE_MAXSIZE               L"Maxsize"
#define     VALUE_RETENTION             L"Retention"
#define     VALUE_RESTRICT_GUEST_ACCESS L"RestrictGuestAccess"
#define     VALUE_LOGPOPUP              L"LogFullPopup"
#define     VALUE_DEBUG                 L"DBFlags"

#endif // ifndef _EVENTDEFAULTS_
