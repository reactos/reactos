/*
 * newdev.h
 *
 * Driver installation DLL interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __NEWDEV_H
#define __NEWDEV_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* UpdateDriverForPlugAndPlayDevices.InstallFlags constants */
#define INSTALLFLAG_FORCE                 0x00000001
#define INSTALLFLAG_READONLY              0x00000002
#define INSTALLFLAG_NONINTERACTIVE        0x00000004
#define INSTALLFLAG_BITS                  0x00000007

BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesA(
  HWND  hwndParent,
  LPCSTR  HardwareId,
  LPCSTR  FullInfPath,
  DWORD  InstallFlags,
  PBOOL  bRebootRequired  OPTIONAL);

BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesW(
  HWND  hwndParent,
  LPCWSTR  HardwareId,
  LPCWSTR  FullInfPath,
  DWORD  InstallFlags,
  PBOOL  bRebootRequired  OPTIONAL);

#ifdef UNICODE
#define UpdateDriverForPlugAndPlayDevices UpdateDriverForPlugAndPlayDevicesW
#else
#define UpdateDriverForPlugAndPlayDevices UpdateDriverForPlugAndPlayDevicesA
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif /* __NEWDEV_H */
