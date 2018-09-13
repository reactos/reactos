/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1998  Intel Corporation

Module Name:

    kdpcpu.h

Abstract:

    Machine specific kernel debugger data types and constants

Author:

    Mark Lucovsky (markl) 29-Aug-1990

Revision History:

--*/

#ifndef _KDPCPU_
#define _KDPCPU_

	// IA64 instruction is in a 128-bit bundle. Each bundle consists of 3 instruction slots. 
	// Each instruction slot is 41-bit long.
	//
	// 
	//            127           87 86           46 45            5 4      1 0
	//            ------------------------------------------------------------
	//            |    slot 2     |    slot 1     |    slot 0     |template|S|
	//            ------------------------------------------------------------
	//
	//            127        96 95         64 63          32 31             0
	//            ------------------------------------------------------------
	//            |  byte 3    |  byte 2     |  byte 1      |   byte 0       |
	//            ------------------------------------------------------------
	//
	// This presents two incompatibilities with conventional processors:
	// 		1. The IA64 IP address is at the bundle bundary. The instruction slot number is 
	//		   stored in ISR.ei at the time of exception.
	//		2. The 41-bit instruction format is not byte-aligned.
	//
	// Break instruction insertion must be done with proper bit-shifting to align with the selected 
	// instruction slot. Further, to insert break instruction insertion at a specific slot, we must
	// be able to specify instruction slot as part of the address. We therefore define an EM address as
	// bundle address + slot number with the least significant two bit always zero:
	//
	//			31                 4 3  2  1  0
	//			--------------------------------
	//			|  bundle address    |slot#|0 0|          
	//			--------------------------------
	//
	// The EM address as defined is the byte-aligned address that is closest to the actual instruction slot.
	// i.e., The EM instruction address of slot #0 is equal to bundle address.
	//                                     slot #1 is equal to bundle address + 4.
	//                                     slot #2 is equal to bundle address + 8.
 
	//
	//  Upon exception, the bundle address is kept in IIP, and the instruction slot which caused 
	//  the exception is in ISR.ei. Kernel exception handler will construct the flat address and
	//  export it in ExceptionRecord.ExceptionAddress. 


#define KDP_BREAKPOINT_TYPE  ULONGLONG          // 64-bit ULONGLONG type is needed to cover 41-bit EM break instruction.
#define KDP_BREAKPOINT_ALIGN 0x3                // An EM address consists of bundle and slot number and is 32-bit aligned.
#define KDP_BREAKPOINT_VALUE (BREAK_INSTR | (DEBUG_STOP_BREAKPOINT << 6))	

#endif // _KDPCPU_
