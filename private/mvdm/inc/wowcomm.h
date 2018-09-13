/*++ BUILD Version: 0002
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWCOMM.H
 *  Constants, macros, etc common to WOW16/WOW32
 *
 *  History:
 *  Created 28-Apr-1993 by Craig Jones (v-cjones)
 *
--*/
#ifndef __WOWCOMM__  // has this been included before?
#define __WOWCOMM__

#ifndef UNALIGNED    // this makes UNALIGNED visible only to 32-bit stuff
#define UNALIGNED    // and invisible to 16-bit stuff
#endif

/* XLATOFF */
#pragma pack(1)
/* XLATON */

// COMDEB - Communications Device Equipment Block.  (copied from ibmcom.inc)
//
// This is essentially a superset of the DCB used outside of this
// module. The DCB is contained within the DEB as the first fields.
// The fields which follow are data and status fields which
// are unique to this implementation.
//
// AltQInAddr and AltQOutAddr are alternate queue pointers which are used when
// in "supervisor" mode.  Supervisor mode is a processor mode other than the
// one which Windows normally runs in.  In standard mode Windows, supervisor
// mode is REAL mode.  In enhanced mode Windows, supervisor mode is RING 0
// protected mode.  For more details see comments in IBMINT.ASM.

// RS232 Data Equip Block
typedef struct _COMDEB16 {  /* cdeb16 */
  BYTE   ComDCB;          // size of this struct
  WORD   ComErr;          // Non-zero if I/O error
  WORD   Port;            // Base I/O Address
  WORD   NotifyHandle;
  WORD   NotifyFlags;
  WORD   RecvTrigger;     // char count threshold for calling
  WORD   SendTrigger;     // char count threshold for calling

// The following fields are specific to com ports only
  WORD   IRQhook;         // ptr to IRQ_Hook_Struc
  WORD   NextDEB;         // ptr to next DEB that is sharing IRQ
  WORD   XOffPoint;       // Q count where XOff is sent
  WORD   EvtMask;         // Mask of events to check for
  WORD   EvtWord;         // Event flags
  DWORD  QInAddr;         // Address of the queue
  DWORD  AltQInAddr;      // Addr of queue in "supervisor" mode
  WORD   QInSize;         // Length of queue in bytes
  DWORD  QOutAddr;        // Address of the queue
  DWORD  AltQOutAddr;     // Addr of queue in "supervisor" mode
  WORD   QOutSize;        // Length of queue in bytes
  WORD   QInCount;        // Number of bytes currently in queue
  WORD   QInGet;          // Offset into queue to get bytes from
  WORD   QInPut;          // Offset into queue to put bytes in
  WORD   QOutCount;       // Number of bytes currently in queue
  WORD   QOutGet;         // Offset into queue to get bytes from
  WORD   QOutPut;         // Offset into queue to put bytes in
  BYTE   EFlags;          // Extended flags
  BYTE   MSRShadow;       // Modem Status Register Shadow
  BYTE   ErrorMask;       // Default error-checking mask
  BYTE   RxMask;          // Character mask
  BYTE   ImmedChar;       // Char to be transmitted immediately
  BYTE   HSFlag;          // Handshake flag
  BYTE   HHSLines;        // 8250 DTR/RTS bits for handshaking
  BYTE   OutHHSLines;     // Lines that must be high to output
  BYTE   MSRMask;         // Mask of Modem Lines to check
  BYTE   MSRInfinite;     // Mask of MSR lines that must be high
  BYTE   IntVecNum;       // Interrupt vector number
  BYTE   LSRShadow;       // Line Status Register shadow
  WORD   QOutMod;         // Characters sent mod xOnLim ENQ/ETX [rkh]
  DWORD  VCD_data;
  BYTE   VCDflags;
  BYTE   MiscFlags;       // still more flags
} COMDEB16;
typedef COMDEB16 UNALIGNED *PCOMDEB16;

// In 3.0 MSRShadow had this relationship to EvtWord and major COM apps all
// use this offset of 35 to get to MSRShadow so that they can determine the
// current status of the Modem Status bits.  We need to maintain this offset
// so that these apps will continue to run.

/* XLATOFF */
#pragma pack()
/* XLATON */

#endif // __WOWCOMM__

