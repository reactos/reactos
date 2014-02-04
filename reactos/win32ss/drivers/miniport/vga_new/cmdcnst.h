/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            boot/drivers/video/miniport/vga/cmdcnst.h
 * PURPOSE:         Command Code Definitions for VGA Command Streams
 * PROGRAMMERS:     Copyright (c) 1992  Microsoft Corporation
 */

#pragma once
 
//--------------------------------------------------------------------------
//   Definition of the set/clear mode command language.
//
//   Each command is composed of a major portion and a minor portion.
//   The major portion of a command can be found in the most significant
//   nibble of a command byte, while the minor portion is in the least
//   significant portion of a command byte.
//
//   maj  minor      Description
//   ---- -----      --------------------------------------------
//   00              End of data
//
//   10              in and out type commands as described by flags
//        flags:
//
//        xxxx
//        ||||
//        |||+-------- unused
//        ||+--------- 0/1 single/multiple values to output (in's are always 
//        |+---------- 0/1 8/16 bit operation                  single)
//        +----------- 0/1 out/in instruction
//
//       Outs
//       ----------------------------------------------
//       0           reg:W val:B
//       2           reg:W cnt:W val1:B val2:B...valN:B
//       4           reg:W val:W
//       6           reg:W cnt:W val1:W val2:W...valN:W
//
//       Ins
//       ----------------------------------------------
//       8           reg:W
//       a           reg:W cnt:W
//       c           reg:W
//       e           reg:W cnt:W
//
//   20              Special purpose outs
//       00          do indexed outs for seq, crtc, and gdc
//                   indexreg:W cnt:B startindex:B val1:B val2:B...valN:B
//       01          do indexed outs for atc
//                   index-data_reg:W cnt:B startindex:B val1:B val2:B...valN:B
//       02          do masked outs
//                   indexreg:W andmask:B xormask:B
//
//   F0              Nop
//
//---------------------------------------------------------------------------

// some useful equates - major commands

#define EOD     0x000                   // end of data
#define INOUT   0x010                   // do ins or outs
#define METAOUT 0x020                   // do special types of outs
#define NCMD    0x0f0                   // Nop command


// flags for INOUT major command

//#define UNUSED    0x01                    // reserved
#define MULTI   0x02                    // multiple or single outs
#define BW      0x04                    // byte/word size of operation
#define IO      0x08                    // out/in instruction

// minor commands for metout

#define INDXOUT 0x00                    // do indexed outs
#define ATCOUT  0x01                    // do indexed outs for atc
#define MASKOUT 0x02                    // do masked outs using and-xor masks


// composite inout type commands

#define OB      (INOUT)                 // output 8 bit value
#define OBM     (INOUT+MULTI)           // output multiple bytes
#define OW      (INOUT+BW)              // output single word value
#define OWM     (INOUT+BW+MULTI)        // output multiple words

#define IB      (INOUT+IO)              // input byte
#define IBM     (INOUT+IO+MULTI)        // input multiple bytes
#define IW      (INOUT+IO+BW)           // input word
#define IWM     (INOUT+IO+BW+MULTI)     // input multiple words
 
/* EOF */
