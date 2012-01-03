//#define NTDDI_VERSION   NTDDI_WIN2K
//#define _WIN32_WINNT    _WIN32_WINNT_WIN2K

#include <ntddk.h>
#include <stdio.h>

// NT4 DDK-compatibility
#ifndef RTL_CONSTANT_STRING
#   define RTL_CONSTANT_STRING(s) { sizeof( s ) - sizeof( (s)[0] ), sizeof( s ), s }
#endif

#ifndef OBJ_KERNEL_HANDLE
#   define OBJ_KERNEL_HANDLE       0x00000200L
#endif

#ifndef FILE_DEVICE_SECURE_OPEN
#   define FILE_DEVICE_SECURE_OPEN         0x00000100
#endif

#include "debug.h"
#include "usb.h"
#include "hcd.h"
#include "td.h"
#include "irplist.h"
#include "events.h"
#include "devmgr.h"
#include "hub.h"
#include "umss.h"
#include "mouse.h"
#include "keyboard.h"
#include "uhciver.h"
