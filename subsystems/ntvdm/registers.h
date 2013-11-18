/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            registers.c
 * PURPOSE:         Exported functions for manipulating registers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _REGISTERS_H_
#define _REGISTERS_H_

/* INCLUDES *******************************************************************/

ULONG EmulatorGetRegister(ULONG Register);
VOID EmulatorSetRegister(ULONG Register, ULONG Value);
BOOLEAN EmulatorGetFlag(ULONG Flag);
VOID EmulatorSetFlag(ULONG Flag);
VOID EmulatorClearFlag(ULONG Flag);
VOID EmulatorSetStack(WORD Segment, DWORD Offset);


ULONG getEAX(VOID);
VOID setEAX(ULONG);
USHORT getAX(VOID);
VOID setAX(USHORT);
UCHAR getAH(VOID);
VOID setAH(UCHAR);
UCHAR getAL(VOID);
VOID setAL(UCHAR);

ULONG getEBX(VOID);
VOID setEBX(ULONG);
USHORT getBX(VOID);
VOID setBX(USHORT);
UCHAR getBH(VOID);
VOID setBH(UCHAR);
UCHAR getBL(VOID);
VOID setBL(UCHAR);

ULONG getECX(VOID);
VOID setECX(ULONG);
USHORT getCX(VOID);
VOID setCX(USHORT);
UCHAR getCH(VOID);
VOID setCH(UCHAR);
UCHAR getCL(VOID);
VOID setCL(UCHAR);

ULONG getEDX(VOID);
VOID setEDX(ULONG);
USHORT getDX(VOID);
VOID setDX(USHORT);
UCHAR getDH(VOID);
VOID setDH(UCHAR);
UCHAR getDL(VOID);
VOID setDL(UCHAR);



ULONG getESP(VOID);
VOID setESP(ULONG);
USHORT getSP(VOID);
VOID setSP(USHORT);

ULONG getEBP(VOID);
VOID setEBP(ULONG);
USHORT getBP(VOID);
VOID setBP(USHORT);

ULONG getESI(VOID);
VOID setESI(ULONG);
USHORT getSI(VOID);
VOID setSI(USHORT);

ULONG getEDI(VOID);
VOID setEDI(ULONG);
USHORT getDI(VOID);
VOID setDI(USHORT);

ULONG getEIP(VOID);
VOID setEIP(ULONG);
USHORT getIP(VOID);
VOID setIP(USHORT);

USHORT getCS(VOID);
VOID setCS(USHORT);
USHORT getSS(VOID);
VOID setSS(USHORT);
USHORT getDS(VOID);
VOID setDS(USHORT);
USHORT getES(VOID);
VOID setES(USHORT);
USHORT getFS(VOID);
VOID setFS(USHORT);
USHORT getGS(VOID);
VOID setGS(USHORT);

ULONG getCF(VOID);
VOID setCF(ULONG);
ULONG getPF(VOID);
VOID setPF(ULONG);
ULONG getAF(VOID);
VOID setAF(ULONG);
ULONG getZF(VOID);
VOID setZF(ULONG);
ULONG getSF(VOID);
VOID setSF(ULONG);
ULONG getIF(VOID);
VOID setIF(ULONG);
ULONG getDF(VOID);
VOID setDF(ULONG);
ULONG getOF(VOID);
VOID setOF(ULONG);

ULONG getEFLAGS(VOID);
VOID setEFLAGS(ULONG);

USHORT getMSW(VOID);
VOID setMSW(USHORT);

#endif // _REGISTERS_H_

/* EOF */
