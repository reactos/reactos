/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    prtq32.h

Abstract:

    Print queue administration.

Author:

    Don Ryan (donryan) 14-Jun-1995

Environment:

    User Mode - Win32

Revision History:

    14-Jun-1995 donryan     Munged from windows\printman\security.c.

--*/

#ifndef _PRTQ32_H_
#define _PRTQ32_H_

#include <windows.h>
#include <sedapi.h>
#include <winspool.h>
#include <commctrl.h>
#include <comctrlp.h>

#define SED_ID_PERMS    0
#define SED_ID_AUDIT    1
#define SED_ID_OWNER    2

// Indexes into the APPLICATION_ACCESSES structure:

#define PERMS_NOACC 0   // No Access allowed             
#define PERMS_PRINT 1   // Print permission              
#define PERMS_DOCAD 2   // Document Administer permission
#define PERMS_ADMIN 3   // Administer permission         
#define PERMS_COUNT 4   // Total number of permissions   

#define PERMS_AUDIT_PRINT                   0
#define PERMS_AUDIT_ADMINISTER              1
#define PERMS_AUDIT_DELETE                  2
#define PERMS_AUDIT_CHANGE_PERMISSIONS      3
#define PERMS_AUDIT_TAKE_OWNERSHIP          4
#define PERMS_AUDIT_COUNT                   5

// Help IDs passed to the permissions editor:

#define ID_HELP_PERMISSIONS_MAIN_DLG        160
#define ID_HELP_PERMISSIONS_ADD_USER_DLG    170

#define ID_HELP_SERVERVIEWER                180

#define ID_HELP_AUDITING_MAIN_DLG           190
#define ID_HELP_AUDITING_ADD_USER_DLG       195

#define ID_HELP_PERMISSIONS_LOCAL_GROUP     210
#define ID_HELP_PERMISSIONS_GLOBAL_GROUP    220
#define ID_HELP_PERMISSIONS_FIND_ACCOUNT    230

#define ID_HELP_AUDITING_LOCAL_GROUP        240
#define ID_HELP_AUDITING_GLOBAL_GROUP       250
#define ID_HELP_AUDITING_FIND_ACCOUNT       260

#define ID_HELP_TAKE_OWNERSHIP              270

#define IDS_PRINTER                         600
#define IDS_NOACCESS                        601
#define IDS_PRINT                           602
#define IDS_ADMINISTERDOCUMENTS             603
#define IDS_ADMINISTER                      604
#define IDS_AUDIT_PRINT                     605
#define IDS_AUDIT_ADMINISTER                606
#define IDS_AUDIT_DELETE                    607
#define IDS_CHANGE_PERMISSIONS              608
#define IDS_TAKE_OWNERSHIP                  609

typedef struct _QUEUE { 
    DWORD   AccessGranted; 
    LPTSTR  pServerName;
    LPTSTR  pPrinterName;
    HANDLE  hPrinter;
} QUEUE, *PQUEUE;

typedef struct _SECURITY_CONTEXT
{
    SECURITY_INFORMATION SecurityInformation;
    PQUEUE               pPrinterContext;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    HANDLE               hPrinter;
} SECURITY_CONTEXT, *PSECURITY_CONTEXT;

// For OpenPrinterForSpecifiedAccess
#define PRINTER_ACCESS_HIGHEST_PERMITTED    0x41ACCE55
#define PRINTER_ACCESS_DENIED               0x00000000

#define ZERO_OUT(p) (memset((p), 0, sizeof(*(p))))

#endif // _PRTQ32_H_
