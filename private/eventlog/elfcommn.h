/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    elfcommn.h

Abstract:

    Common defines for client and server.

Author:

    Rajen Shah (rajens) 12-Aug-1991

Revision History:

--*/

#ifndef _ELFCOMMON_
#define _ELFCOMMON_

//
// Current default names of modules supported
//

#define     ELF_MAX_LOG_MODULES 256

#define     ELF_SYSTEM_MODULE_NAME            L"System"
#define     ELF_APPLICATION_MODULE_NAME       L"Application"
#define     ELF_SECURITY_MODULE_NAME          L"Security"

#define     ELF_SYSTEM_MODULE_NAME_ASCII      "System"
#define     ELF_APPLICATION_MODULE_NAME_ASCII "Application"
#define     ELF_SECURITY_MODULE_NAME_ASCII    "Security"


//
// Version numbers for the file header and the client
//

#define     ELF_VERSION_MAJOR    0x0001
#define     ELF_VERSION_MINOR    0x0001

//
// The following are definitions for the Flags field in the context handle.
//
// ELF_LOG_HANDLE_INVALID is used to indicate that the handle is no
//                        longer valid - i.e. the contents of the file
//                        or the file itself have changed. It is used for
//                        READs to cause the reader to "resync".
//
// ELF_LOG_HANDLE_BACKUP_LOG indicates that this was created with the
//                        OpenBackupEventlog API and is not an active log.
//                        This means we do some additional work at close time
//                        and we disallow clear, backup, write and
//                        ChangeNotify operations.
//
// ELF_LOG_HANDLE_REMOTE_HANDLE indicates that this handle was created via
//                        a remote RPC call.  This handle cannot be used for
//                        ElfChangeNotify
//
// ELF_LOG_HANDLE_GENERATE_ON_CLOSE indicates that NtCloseAuditAlarm must
//                        be called when this handle is closed.  This flag
//                        is set when an audit is generated on open.
//

#define     ELF_LOG_HANDLE_INVALID_FOR_READ     0x0001
#define     ELF_LOG_HANDLE_BACKUP_LOG           0x0002
#define     ELF_LOG_HANDLE_REMOTE_HANDLE        0x0004
#define     ELF_LOG_HANDLE_LAST_READ_FORWARD    0x0008
#define     ELF_LOG_HANDLE_GENERATE_ON_CLOSE    0x0010

#endif /* _ELFCOMMON_ */
