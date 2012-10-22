#ifndef _SRV_H
#define _SRV_H

/* PSDK/NDK Headers */
#define NTOS_MODE_USER
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winnt.h>
#include <ndk/ntndk.h>

/* CSR Header */
#include <csr/csrsrv.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Subsystem Manager Header */
#include <sm/helper.h>
#include <sm/smmsg.h>

/* Internal CSRSS Headers */
#include "include/api.h"
#include "include/csrplugin.h"

/* Defines */
#define SM_REG_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"

#define SESSION_ROOT        L"\\Sessions"
#define GLOBAL_ROOT         L"\\GLOBAL??"
#define SYMLINK_NAME        L"SymbolicLink"
#define SB_PORT_NAME        L"SbAbiPort"
#define CSR_PORT_NAME       L"ApiPort"
#define UNICODE_PATH_SEP    L"\\"

#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))
#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))

#endif
