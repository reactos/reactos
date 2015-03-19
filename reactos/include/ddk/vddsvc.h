/*
 * vddsvc.h
 *
 * Windows NT Device Driver Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Hermes Belusca-Maito (hermes.belusca@sfr.fr)
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

#pragma once

#ifndef _NT_VDD
#include <nt_vdd.h>
#endif

/*
 * Interrupts services
 */
#define ICA_MASTER  0
#define ICA_SLAVE   1

VOID
WINAPI
call_ica_hw_interrupt(
  _In_ INT  ms,
  _In_ BYTE line,
  _In_ INT  count);

#define VDDSimulateInterrupt(ms, line, count) \
    call_ica_hw_interrupt((ms), (line), (count)) // Windows specifies a count of 1 ...


/*
 * Memory services
 */

#ifdef i386

PBYTE
WINAPI
MGetVdmPointer(
  _In_ ULONG Address,
  _In_ ULONG Size,
  _In_ BOOLEAN ProtectedMode);

#define Sim32GetVDMPointer(Address, Size, Mode) \
    MGetVdmPointer((Address), (Size), (Mode))

#define Sim32FlushVDMPointer(Address, Size, Buffer, Mode) TRUE

#else

PBYTE
WINAPI
Sim32GetVDMPointer(
  _In_ ULONG Address,
  _In_ ULONG Size,
  _In_ BOOLEAN ProtectedMode);

BOOLEAN
WINAPI
Sim32FlushVDMPointer(
  _In_ ULONG Address,
  _In_ ULONG Size,
  _In_ PBYTE Buffer,
  _In_ BOOLEAN ProtectedMode);

#endif

PBYTE
WINAPI
Sim32pGetVDMPointer(
  _In_ ULONG Address,
  _In_ BOOLEAN ProtectedMode);

/* This API appears to have been never implemented anywhere... */
#define Sim32FreeVDMPointer(Address, Size, Buffer, Mode) TRUE

#define GetVDMAddress(usSeg, usOff) (((ULONG)(usSeg) << 4) + (ULONG)(usOff))

#define GetVDMPointer(Address, Size, Mode) \
    Sim32GetVDMPointer(Address, Size, Mode)

#define FlushVDMPointer(Address, Size, Buffer, Mode) \
    Sim32FlushVDMPointer(Address, Size, Buffer, Mode)

#define FreeVDMPointer(Address, Size, Buffer, Mode) \
    Sim32FreeVDMPointer(Address, Size, Buffer, Mode)


/*
 * Registers manipulation
 */
PVOID  WINAPI getIntelRegistersPointer(VOID);

#ifndef _M_MIPS

ULONG  WINAPI getEAX(VOID);
VOID   WINAPI setEAX(ULONG);
USHORT WINAPI getAX(VOID);
VOID   WINAPI setAX(USHORT);
UCHAR  WINAPI getAH(VOID);
VOID   WINAPI setAH(UCHAR);
UCHAR  WINAPI getAL(VOID);
VOID   WINAPI setAL(UCHAR);

ULONG  WINAPI getEBX(VOID);
VOID   WINAPI setEBX(ULONG);
USHORT WINAPI getBX(VOID);
VOID   WINAPI setBX(USHORT);
UCHAR  WINAPI getBH(VOID);
VOID   WINAPI setBH(UCHAR);
UCHAR  WINAPI getBL(VOID);
VOID   WINAPI setBL(UCHAR);

ULONG  WINAPI getECX(VOID);
VOID   WINAPI setECX(ULONG);
USHORT WINAPI getCX(VOID);
VOID   WINAPI setCX(USHORT);
UCHAR  WINAPI getCH(VOID);
VOID   WINAPI setCH(UCHAR);
UCHAR  WINAPI getCL(VOID);
VOID   WINAPI setCL(UCHAR);

ULONG  WINAPI getEDX(VOID);
VOID   WINAPI setEDX(ULONG);
USHORT WINAPI getDX(VOID);
VOID   WINAPI setDX(USHORT);
UCHAR  WINAPI getDH(VOID);
VOID   WINAPI setDH(UCHAR);
UCHAR  WINAPI getDL(VOID);
VOID   WINAPI setDL(UCHAR);



ULONG  WINAPI getESP(VOID);
VOID   WINAPI setESP(ULONG);
USHORT WINAPI getSP(VOID);
VOID   WINAPI setSP(USHORT);

ULONG  WINAPI getEBP(VOID);
VOID   WINAPI setEBP(ULONG);
USHORT WINAPI getBP(VOID);
VOID   WINAPI setBP(USHORT);

ULONG  WINAPI getESI(VOID);
VOID   WINAPI setESI(ULONG);
USHORT WINAPI getSI(VOID);
VOID   WINAPI setSI(USHORT);

ULONG  WINAPI getEDI(VOID);
VOID   WINAPI setEDI(ULONG);
USHORT WINAPI getDI(VOID);
VOID   WINAPI setDI(USHORT);

ULONG  WINAPI getEIP(VOID);
VOID   WINAPI setEIP(ULONG);
USHORT WINAPI getIP(VOID);
VOID   WINAPI setIP(USHORT);

USHORT WINAPI getCS(VOID);
VOID   WINAPI setCS(USHORT);
USHORT WINAPI getSS(VOID);
VOID   WINAPI setSS(USHORT);
USHORT WINAPI getDS(VOID);
VOID   WINAPI setDS(USHORT);
USHORT WINAPI getES(VOID);
VOID   WINAPI setES(USHORT);
USHORT WINAPI getFS(VOID);
VOID   WINAPI setFS(USHORT);
USHORT WINAPI getGS(VOID);
VOID   WINAPI setGS(USHORT);

ULONG  WINAPI getCF(VOID);
VOID   WINAPI setCF(ULONG);
ULONG  WINAPI getPF(VOID);
VOID   WINAPI setPF(ULONG);
ULONG  WINAPI getAF(VOID);
VOID   WINAPI setAF(ULONG);
ULONG  WINAPI getZF(VOID);
VOID   WINAPI setZF(ULONG);
ULONG  WINAPI getSF(VOID);
VOID   WINAPI setSF(ULONG);
ULONG  WINAPI getIF(VOID);
VOID   WINAPI setIF(ULONG);
ULONG  WINAPI getDF(VOID);
VOID   WINAPI setDF(ULONG);
ULONG  WINAPI getOF(VOID);
VOID   WINAPI setOF(ULONG);

ULONG  WINAPI getEFLAGS(VOID);
VOID   WINAPI setEFLAGS(ULONG);

USHORT WINAPI getMSW(VOID);
VOID   WINAPI setMSW(USHORT);

#else

ULONG  WINAPI c_getEAX(VOID);
VOID   WINAPI c_setEAX(ULONG);
USHORT WINAPI c_getAX(VOID);
VOID   WINAPI c_setAX(USHORT);
UCHAR  WINAPI c_getAH(VOID);
VOID   WINAPI c_setAH(UCHAR);
UCHAR  WINAPI c_getAL(VOID);
VOID   WINAPI c_setAL(UCHAR);

ULONG  WINAPI c_getEBX(VOID);
VOID   WINAPI c_setEBX(ULONG);
USHORT WINAPI c_getBX(VOID);
VOID   WINAPI c_setBX(USHORT);
UCHAR  WINAPI c_getBH(VOID);
VOID   WINAPI c_setBH(UCHAR);
UCHAR  WINAPI c_getBL(VOID);
VOID   WINAPI c_setBL(UCHAR);

ULONG  WINAPI c_getECX(VOID);
VOID   WINAPI c_setECX(ULONG);
USHORT WINAPI c_getCX(VOID);
VOID   WINAPI c_setCX(USHORT);
UCHAR  WINAPI c_getCH(VOID);
VOID   WINAPI c_setCH(UCHAR);
UCHAR  WINAPI c_getCL(VOID);
VOID   WINAPI c_setCL(UCHAR);

ULONG  WINAPI c_getEDX(VOID);
VOID   WINAPI c_setEDX(ULONG);
USHORT WINAPI c_getDX(VOID);
VOID   WINAPI c_setDX(USHORT);
UCHAR  WINAPI c_getDH(VOID);
VOID   WINAPI c_setDH(UCHAR);
UCHAR  WINAPI c_getDL(VOID);
VOID   WINAPI c_setDL(UCHAR);



ULONG  WINAPI c_getESP(VOID);
VOID   WINAPI c_setESP(ULONG);
USHORT WINAPI c_getSP(VOID);
VOID   WINAPI c_setSP(USHORT);

ULONG  WINAPI c_getEBP(VOID);
VOID   WINAPI c_setEBP(ULONG);
USHORT WINAPI c_getBP(VOID);
VOID   WINAPI c_setBP(USHORT);

ULONG  WINAPI c_getESI(VOID);
VOID   WINAPI c_setESI(ULONG);
USHORT WINAPI c_getSI(VOID);
VOID   WINAPI c_setSI(USHORT);

ULONG  WINAPI c_getEDI(VOID);
VOID   WINAPI c_setEDI(ULONG);
USHORT WINAPI c_getDI(VOID);
VOID   WINAPI c_setDI(USHORT);

ULONG  WINAPI c_getEIP(VOID);
VOID   WINAPI c_setEIP(ULONG);
USHORT WINAPI c_getIP(VOID);
VOID   WINAPI c_setIP(USHORT);

USHORT WINAPI c_getCS(VOID);
VOID   WINAPI c_setCS(USHORT);
USHORT WINAPI c_getSS(VOID);
VOID   WINAPI c_setSS(USHORT);
USHORT WINAPI c_getDS(VOID);
VOID   WINAPI c_setDS(USHORT);
USHORT WINAPI c_getES(VOID);
VOID   WINAPI c_setES(USHORT);
USHORT WINAPI c_getFS(VOID);
VOID   WINAPI c_setFS(USHORT);
USHORT WINAPI c_getGS(VOID);
VOID   WINAPI c_setGS(USHORT);

ULONG  WINAPI c_getCF(VOID);
VOID   WINAPI c_setCF(ULONG);
ULONG  WINAPI c_getPF(VOID);
VOID   WINAPI c_setPF(ULONG);
ULONG  WINAPI c_getAF(VOID);
VOID   WINAPI c_setAF(ULONG);
ULONG  WINAPI c_getZF(VOID);
VOID   WINAPI c_setZF(ULONG);
ULONG  WINAPI c_getSF(VOID);
VOID   WINAPI c_setSF(ULONG);
ULONG  WINAPI c_getIF(VOID);
VOID   WINAPI c_setIF(ULONG);
ULONG  WINAPI c_getDF(VOID);
VOID   WINAPI c_setDF(ULONG);
ULONG  WINAPI c_getOF(VOID);
VOID   WINAPI c_setOF(ULONG);

USHORT WINAPI c_getMSW(VOID);
VOID   WINAPI c_setMSW(USHORT);

#endif

/* EOF */
