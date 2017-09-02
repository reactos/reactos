#ifndef __SHUTDOWN_PRECOMP_H
#define __SHUTDOWN_PRECOMP_H

/* INCLUDES ******************************************************************/

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>

#include <conutils.h>

#include "resource.h"

/* DEFINES *******************************************************************/

#define MAX_MESSAGE_SIZE    512
#define MAX_MAJOR_CODE      256
#define MAX_MINOR_CODE      65536
#define MAX_TIMEOUT         315360000

/* Reason Code List */
typedef struct _REASON
{
    LPWSTR prefix;
    int major;
    int minor;
    DWORD flag;
} REASON, *PREASON;

/* Used to determine how to shutdown the system. */
struct CommandLineOptions
{
    BOOL abort;
    BOOL force;
    BOOL logoff;
    BOOL restart;
    BOOL shutdown;
    BOOL document_reason;
    BOOL hibernate;
    DWORD shutdown_delay;
    LPWSTR remote_system;
    LPWSTR message;
    DWORD reason;
    BOOL show_gui;
};

extern const DWORD defaultReason;

/* PROTOTYPES *****************************************************************/

/* misc.c */
BOOL CheckCommentLength(LPCWSTR);
DWORD ParseReasonCode(LPCWSTR);
VOID DisplayError(DWORD dwError);

/* gui.c */
BOOL ShutdownGuiMain(struct CommandLineOptions opts);

#endif /* __SHUTDOWN_PRECOMP_H */
