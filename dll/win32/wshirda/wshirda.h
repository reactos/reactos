/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 IRDA Helper DLL
 * FILE:        include/ws2help.h
 * PURPOSE:     WinSock 2 IRDA Helper DLL
 */
#ifndef __WSHIRDA_H
#define __WSHIRDA_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <wsahelp.h>

#define EXPORT WINAPI

#endif /* __WSHIRDA_H */

/* EOF */
