/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX client binding for drivers built in tree
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Binds the calling driver to ucx01000 and fills UcxFunctions and
 * UcxDriverGlobals. Call from DriverEntry after WdfDriverCreate.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxStubBindClass(VOID);

/** Drops the binding taken by UcxStubBindClass. Safe to call when not bound. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxStubUnbindClass(VOID);

#ifdef __cplusplus
}
#endif
