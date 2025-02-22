/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

#ifndef RTL_VISTA_H
#define RTL_VISTA_H

#undef _WIN32_WINNT
#undef WINVER
#define _WIN32_WINNT 0x600
#define WINVER 0x600

/* Main RTL Header */
#include "rtl.h"

#endif /* RTL_VISTA_H */
