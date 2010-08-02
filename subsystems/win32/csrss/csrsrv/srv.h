/* PSDK/NDK Headers */
#define NTOS_MODE_USER
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winnt.h>
#include <ndk/ntndk.h>

/* CSR Header */
//#include <csr/server.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Subsystem Manager Header */
#include <sm/helper.h>

/* Internal CSRSS Headers */
#include <api.h>
#include <csrplugin.h>
