/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header for x86 / x64 CPU-level support
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#define CPU_VENDOR_STR_LEN 13

VOID
NTAPI
KiGetCpuVendorString(
    _Out_writes_z_(CPU_VENDOR_STR_LEN) CHAR VendorString[CPU_VENDOR_STR_LEN]);

CPU_VENDORS
NTAPI
KiIdentifyCpuVendor(
    _In_reads_z_(CPU_VENDOR_STR_LEN) const CHAR VendorString[CPU_VENDOR_STR_LEN]);
