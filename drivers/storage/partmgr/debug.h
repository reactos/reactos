/*
 * PROJECT:     Partition manager driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug helpers
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <reactos/debug.h>

#define ERR(fmt, ...) ERR__(DPFLTR_DISK_ID, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) WARN__(DPFLTR_DISK_ID, fmt, ##__VA_ARGS__)
#define TRACE(fmt, ...) TRACE__(DPFLTR_DISK_ID, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) INFO__(DPFLTR_DISK_ID, fmt, ##__VA_ARGS__)
// #define FDPRINT(lvl, fmt, ...) FDPRINT__(DPFLTR_DISK_ID, lvl, fmt, __VA_ARGS__)
