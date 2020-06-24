/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug helpers
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#define NDEBUG
#include <debug.h>

#define DBGLVL_DISK (1 << 31)
#define DBGLVL_PNP (1 << 30)

#define ERR(fmt, ...) ERR__(DPFLTR_USBSTOR_ID, fmt, __VA_ARGS__)
#define WARN(fmt, ...) WARN__(DPFLTR_USBSTOR_ID, fmt, __VA_ARGS__)
#define TRACE(fmt, ...) TRACE__(DPFLTR_USBSTOR_ID, fmt, __VA_ARGS__)
#define INFO(fmt, ...) INFO__(DPFLTR_USBSTOR_ID, fmt, __VA_ARGS__)
#define FDPRINT(lvl, fmt, ...) FDPRINT__(DPFLTR_USBSTOR_ID, lvl, fmt, __VA_ARGS__)
