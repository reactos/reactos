/* PSDK/NDK Headers */
#define NTOS_MODE_USER
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winnt.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/ldrfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>
#include <ndk/umfuncs.h>

/* CSR Header */
//#include <csr/server.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Subsystem Manager Header */
#include <sm/helper.h>

/* Internal CSRSS Headers */
#include <api.h>
#include <csrplugin.h>

/* Defines */
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))
#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
