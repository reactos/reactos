/*
 * miniport.h
 *
 * Type definitions for miniport drivers
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

#ifndef __MINIPORT_H
#define __MINIPORT_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#define EMULATOR_READ_ACCESS              0x01
#define EMULATOR_WRITE_ACCESS             0x02

typedef enum _EMULATOR_PORT_ACCESS_TYPE {
	Uchar,
	Ushort,
	Ulong
} EMULATOR_PORT_ACCESS_TYPE, *PEMULATOR_PORT_ACCESS_TYPE;


typedef struct _EMULATOR_ACCESS_ENTRY {
  ULONG  BasePort;
  ULONG  NumConsecutivePorts;
  EMULATOR_PORT_ACCESS_TYPE  AccessType;
  UCHAR  AccessMode;
  UCHAR  StringSupport;
  PVOID  Routine;
} EMULATOR_ACCESS_ENTRY, *PEMULATOR_ACCESS_ENTRY;

#ifndef VIDEO_ACCESS_RANGE_DEFINED /* also in video.h */
#define VIDEO_ACCESS_RANGE_DEFINED
typedef struct _VIDEO_ACCESS_RANGE {
  PHYSICAL_ADDRESS  RangeStart;
  ULONG  RangeLength;
  UCHAR  RangeInIoSpace;
  UCHAR  RangeVisible;
  UCHAR  RangeShareable;
  UCHAR  RangePassive;
} VIDEO_ACCESS_RANGE, *PVIDEO_ACCESS_RANGE;
#endif

typedef VOID DDKAPI
(*PBANKED_SECTION_ROUTINE)(
  IN ULONG  ReadBank,
  IN ULONG  WriteBank,
  IN PVOID  Context);

#ifdef __cplusplus
}
#endif

#endif /* __MINIPORT_H */
