#ifndef _SRV_H
#define _SRV_H

/* PSDK/NDK Headers */
#define NTOS_MODE_USER
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winreg.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/dbgktypes.h>
#include <ndk/mmfuncs.h>
#include <ndk/umfuncs.h>

/* CSR Header */
//#include <csr/server.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Subsystem Manager Header */
#include <sm/helper.h>
#include <sm/smmsg.h>

/* Internal CSRSS Headers */
#include <api.h>
#include <csrplugin.h>

extern HANDLE CsrHeap;

#define SM_REG_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"

#define SESSION_ROOT        L"\\Sessions"
#define GLOBAL_ROOT         L"\\GLOBAL??"
#define SYMLINK_NAME        L"SymbolicLink"
#define SB_PORT_NAME        L"SbAbiPort"
#define CSR_PORT_NAME       L"ApiPort"
#define UNICODE_PATH_SEP    L"\\"

/* Defines */
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))
#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))

#endif
