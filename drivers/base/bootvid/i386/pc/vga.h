/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * PURPOSE:         Definitions for VGA
 * PROGRAMMERS:     Copyright (c) 1992  Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */

#ifndef _BOOTVID_VGA_H_
#define _BOOTVID_VGA_H_

#pragma once

#include "cmdcnst.h"

//
// Base address of VGA memory range.  Also used as base address of VGA
// memory when loading a font, which is done with the VGA mapped at A0000.
//

#define MEM_VGA      0xA0000
#define MEM_VGA_SIZE 0x20000

//
// For memory mapped IO
//

#define MEMORY_MAPPED_IO_OFFSET (0xB8000 - 0xA0000)

//
// Port definitions for filling the ACCESS_RANGES structure in the miniport
// information, defines the range of I/O ports the VGA spans.
// There is a break in the IO ports - a few ports are used for the parallel
// port. Those cannot be defined in the ACCESS_RANGE, but are still mapped
// so all VGA ports are in one address range.
//

#define VGA_BASE_IO_PORT      0x000003B0
#define VGA_START_BREAK_PORT  0x000003BB
#define VGA_END_BREAK_PORT    0x000003C0
#define VGA_MAX_IO_PORT       0x000003DF

//
// VGA register definitions
//
// eVb: 3.1 [VGA] - Use offsets from the VGA Port Address instead of absolute
#define CRTC_ADDRESS_PORT_MONO      0x0004  // CRT Controller Address and
#define CRTC_DATA_PORT_MONO         0x0005  // Data registers in mono mode
#define FEAT_CTRL_WRITE_PORT_MONO   0x000A  // Feature Control write port
                                            // in mono mode
#define INPUT_STATUS_1_MONO         0x000A  // Input Status 1 register read
                                            // port in mono mode
#define ATT_INITIALIZE_PORT_MONO    INPUT_STATUS_1_MONO
                                            // Register to read to reset
                                            // Attribute Controller index/data

#define ATT_ADDRESS_PORT            0x0010  // Attribute Controller Address and
#define ATT_DATA_WRITE_PORT         0x0010  // Data registers share one port
                                            // for writes, but only Address is
                                            // readable at 0x3C0
#define ATT_DATA_READ_PORT          0x0011  // Attribute Controller Data reg is
                                            // readable here
#define MISC_OUTPUT_REG_WRITE_PORT  0x0012  // Miscellaneous Output reg write
                                            // port
#define INPUT_STATUS_0_PORT         0x0012  // Input Status 0 register read
                                            // port
#define VIDEO_SUBSYSTEM_ENABLE_PORT 0x0013  // Bit 0 enables/disables the
                                            // entire VGA subsystem
#define SEQ_ADDRESS_PORT            0x0014  // Sequence Controller Address and
#define SEQ_DATA_PORT               0x0015  // Data registers
#define DAC_PIXEL_MASK_PORT         0x0016  // DAC pixel mask reg
#define DAC_ADDRESS_READ_PORT       0x0017  // DAC register read index reg,
                                            // write-only
#define DAC_STATE_PORT              0x0017  // DAC state (read/write),
                                            // read-only
#define DAC_ADDRESS_WRITE_PORT      0x0018  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x0019  // DAC data transfer reg
#define FEAT_CTRL_READ_PORT         0x001A  // Feature Control read port
#define MISC_OUTPUT_REG_READ_PORT   0x001C  // Miscellaneous Output reg read
                                            // port
#define GRAPH_ADDRESS_PORT          0x001E  // Graphics Controller Address
#define GRAPH_DATA_PORT             0x001F  // and Data registers

#define CRTC_ADDRESS_PORT_COLOR     0x0024  // CRT Controller Address and
#define CRTC_DATA_PORT_COLOR        0x0025  // Data registers in color mode
#define FEAT_CTRL_WRITE_PORT_COLOR  0x002A  // Feature Control write port
#define INPUT_STATUS_1_COLOR        0x002A  // Input Status 1 register read
                                            // port in color mode
// eVb: 3.2 [END]
#define ATT_INITIALIZE_PORT_COLOR   INPUT_STATUS_1_COLOR
                                            // Register to read to reset
                                            // Attribute Controller index/data
                                            // toggle in color mode

//
// VGA indexed register indexes.
//

#define IND_CURSOR_START        0x0A    // index in CRTC of the Cursor Start
#define IND_CURSOR_END          0x0B    //  and End registers
#define IND_CURSOR_HIGH_LOC     0x0E    // index in CRTC of the Cursor Location
#define IND_CURSOR_LOW_LOC      0x0F    //  High and Low Registers
#define IND_VSYNC_END           0x11    // index in CRTC of the Vertical Sync
                                        //  End register, which has the bit
                                        //  that protects/unprotects CRTC
                                        //  index registers 0-7
#define IND_CR2C                0x2C    // Nordic LCD Interface Register
#define IND_CR2D                0x2D    // Nordic LCD Display Control
#define IND_SET_RESET_ENABLE    0x01    // index of Set/Reset Enable reg in GC
#define IND_DATA_ROTATE         0x03    // index of Data Rotate reg in GC
#define IND_READ_MAP            0x04    // index of Read Map reg in Graph Ctlr
#define IND_GRAPH_MODE          0x05    // index of Mode reg in Graph Ctlr
#define IND_GRAPH_MISC          0x06    // index of Misc reg in Graph Ctlr
#define IND_BIT_MASK            0x08    // index of Bit Mask reg in Graph Ctlr
#define IND_SYNC_RESET          0x00    // index of Sync Reset reg in Seq
#define IND_MAP_MASK            0x02    // index of Map Mask in Sequencer
#define IND_MEMORY_MODE         0x04    // index of Memory Mode reg in Seq
#define IND_CRTC_PROTECT        0x11    // index of reg containing regs 0-7 in
                                        //  CRTC
#define IND_CRTC_COMPAT         0x34    // index of CRTC Compatibility reg
                                        //  in CRTC
#define IND_PERF_TUNING         0x16    // index of performance tuning in Seq
#define START_SYNC_RESET_VALUE  0x01    // value for Sync Reset reg to start
                                        //  synchronous reset
#define END_SYNC_RESET_VALUE    0x03    // value for Sync Reset reg to end
                                        //  synchronous reset

//
// Values for Attribute Controller Index register to turn video off
// and on, by setting bit 5 to 0 (off) or 1 (on).
//

#define VIDEO_DISABLE 0
#define VIDEO_ENABLE  0x20

#define INDEX_ENABLE_AUTO_START 0x31

// Masks to keep only the significant bits of the Graphics Controller and
// Sequencer Address registers. Masking is necessary because some VGAs, such
// as S3-based ones, don't return unused bits set to 0, and some SVGAs use
// these bits if extensions are enabled.
//

#define GRAPH_ADDR_MASK 0x0F
#define SEQ_ADDR_MASK   0x07

//
// Mask used to toggle Chain4 bit in the Sequencer's Memory Mode register.
//

#define CHAIN4_MASK 0x08

//
// Value written to the Read Map register when identifying the existence of
// a VGA in VgaInitialize. This value must be different from the final test
// value written to the Bit Mask in that routine.
//

#define READ_MAP_TEST_SETTING 0x03

//
// Default text mode setting for various registers, used to restore their
// states if VGA detection fails after they've been modified.
//

#define MEMORY_MODE_TEXT_DEFAULT 0x02
#define BIT_MASK_DEFAULT 0xFF
#define READ_MAP_DEFAULT 0x00


//
// Palette-related info.
//

//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF

//
// Highest valid palette register index
//

#define VIDEO_MAX_PALETTE_REGISTER 0x0F

//
// Mode into which to put the VGA before starting a VDM, so it's a plain
// vanilla VGA.  (This is the mode's index in ModesVGA[], currently standard
// 80x25 text mode.)
//

#define DEFAULT_MODE 0

//
// Number of bytes to save in each plane.
//

#define VGA_PLANE_SIZE 0x10000

//
// Number of each type of indexed register in a standard VGA, used by
// validator and state save/restore functions.
//
// Note: VDMs currently only support basic VGAs only.
//

#define VGA_NUM_SEQUENCER_PORTS     5
#define VGA_NUM_CRTC_PORTS         25
#define VGA_NUM_GRAPH_CONT_PORTS    9
#define VGA_NUM_ATTRIB_CONT_PORTS  21
#define VGA_NUM_DAC_ENTRIES       256

#define EXT_NUM_GRAPH_CONT_PORTS    0
#define EXT_NUM_SEQUENCER_PORTS     0
#define EXT_NUM_CRTC_PORTS          0
#define EXT_NUM_ATTRIB_CONT_PORTS   0
#define EXT_NUM_DAC_ENTRIES         0

#endif /* _BOOTVID_VGA_H_ */
