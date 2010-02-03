/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/precomp.h
 * PURPOSE:         Precompiled header for LLB
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "ntdef.h"
#include "stdio.h"
#include "ioaccess.h"
#include "machtype.h"
#include "osloader.h"
#include "hw.h"
#include "fw.h"
#include "serial.h"
#include "video.h"
#include "keyboard.h"

VOID
DbgPrint(
    const char *fmt,
    ...
);

/* EOF */
