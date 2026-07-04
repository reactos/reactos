/*
 * PROJECT:     ReactOS win32 DLLs
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ReactOS emulation layer for crypt32 unixlib calls
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#include "crypt32_private.h"
#include <winnls.h> // Workaround: Needed by encode.c

#define __wine_init_unix_call() 0

static inline int __reactos_call_unix_process_attach(void* Args)
{
    return 0;
}

static inline int __reactos_call_unix_process_detach(void* Args)
{
    return 0;
}

// Implemented in reactos/unixlib.c
int __reactos_call_unix_open_cert_store(void* Args);
int __reactos_call_unix_close_cert_store(void* Args);
int __reactos_call_unix_import_store_key(void* Args);
int __reactos_call_unix_import_store_cert(void* Args);
int __reactos_call_unix_enum_root_certs(void* Args);

#define WINE_UNIX_CALL(code,args) __reactos_call_ ## code(args)
