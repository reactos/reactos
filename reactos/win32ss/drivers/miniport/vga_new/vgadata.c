/*
 * PROJECT:         ReactOS VGA Miniport Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/miniport/vga_new/vgadata.c
 * PURPOSE:         Handles switching to VGA Modes and holds VGA Built-in Modes
 * PROGRAMMERS:     Copyright (c) 1992  Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */

#include "vga.h"

//
// This structure describes to which ports access is required.
//

VIDEO_ACCESS_RANGE VgaAccessRange[] = {
{
    {{VGA_BASE_IO_PORT, 0x00000000}},            // 64-bit linear base address
                                                 // of range
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT + 1, // # of ports
    1,                                           // range is in I/O space
    1,                                           // range should be visible
    0                                            // range should be shareable
},
{
    {{VGA_END_BREAK_PORT, 0x00000000}},
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    0
},

//
// This next region also includes Memory mapped IO.  In MMIO, the ports are
// repeated every 256 bytes from b8000 to bff00.
//

{
    {{MEM_VGA, 0x00000000}},
    MEM_VGA_SIZE,
    0,
    1,
    0
},
// eVb: 4.1 [VGA] - Add ATI/Mach64 VGA registers
//
// ATI Registers
//

{
    {{0x1CE, 0x00000000}},
    2,
    1,
    1,
    0
},
{
    {{0x2E8, 0x00000000}},
    8,
    1,
    1,
    0
}
// eVb: 4.1 [END]
};

//
// 640x480 256-color 60Hz mode (BIOS mode 12) set command string for
// VGA.
//
// eVb: 4.2 [VGA] - Add VGA command streams instead of Cirrus
USHORT VGA_640x480[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    5,                              // count
    0x100,                          // start sync reset
    0x0101,0x0F02,0x0003,0x0604,    // program up sequencer

    OB,                             // misc. register
    MISC_OUTPUT_REG_WRITE_PORT,
    0xE3,

    OW,                             // text/graphics bit
    GRAPH_ADDRESS_PORT,
    0x506,
    
    OW,                             // end sync reset
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // unprotect crtc 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x511,                        

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    VGA_NUM_CRTC_PORTS,0,            // count, startindex
    0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,0x40,0x0,0x0,0x0,0x0,0x0,0x0,
    0xEA,0x8C,0xDF,0x28,0x0,0xE7,0x4,0xE3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program atc registers
    ATT_ADDRESS_PORT,
    VGA_NUM_ATTRIB_CONT_PORTS,0,    // count, startindex
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x17, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
    0x3D, 0x3E, 0x3F, 0x3F, 0x01, 0x00,
    0x0F, 0x00, 0x00,

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    VGA_NUM_GRAPH_CONT_PORTS,0,     // count, startindex
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x05, 0x0F, 0xFF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,
    
    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

//
// 720x400 color text mode (BIOS mode 3) set command string for
// VGA.
//

USHORT VGA_TEXT_0[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    5,                              // count
    0x100,                          // start sync reset
    0x0101,0x0302,0x0003,0x0204,    // program up sequencer

    OB,                             // misc. register
    MISC_OUTPUT_REG_WRITE_PORT,
    0x67,

    OW,                             // text/graphics bit
    GRAPH_ADDRESS_PORT,
    0x0e06,
    
    OW,                             // end sync reset
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // unprotect crtc 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0xE11,                        

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    VGA_NUM_CRTC_PORTS,0,           // count, startindex
    0x5F,0x4F,0x50,0x82,0x55,0x81,0xBF,0x1F,0x00,0x4F,0xD,0xE,0x0,0x0,0x0,0x0,
    0x9c,0x8E,0x8F,0x28,0x1F,0x96,0xB9,0xA3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program atc registers
    ATT_ADDRESS_PORT,
    VGA_NUM_ATTRIB_CONT_PORTS,0,    // count, startindex
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x17, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
    0x3D, 0x3E, 0x3F, 0x3F, 0x04, 0x00,
    0x0F, 0x08, 0x00,

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    VGA_NUM_GRAPH_CONT_PORTS,0,     // count, startindex
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
    0x0E, 0x00, 0xFF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,
    
    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};

//
// 640x400 color text mode (BIOS mode 3) set command string for
// VGA.
//

USHORT VGA_TEXT_1[] = {
    OWM,                            // begin setmode
    SEQ_ADDRESS_PORT,
    5,                              // count
    0x100,                          // start sync reset
    0x0101,0x0302,0x0003,0x0204,    // program up sequencer

    OB,                             // misc. register
    MISC_OUTPUT_REG_WRITE_PORT,
    0xA3,

    OW,                             // text/graphics bit
    GRAPH_ADDRESS_PORT,
    0x0e06,
    
    OW,                             // end sync reset
    SEQ_ADDRESS_PORT,
    IND_SYNC_RESET,

    OB,
    SEQ_DATA_PORT,
    END_SYNC_RESET_VALUE,

    OW,                             // unprotect crtc 0-7
    CRTC_ADDRESS_PORT_COLOR,
    0x511,                        

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    VGA_NUM_CRTC_PORTS,0,           // count, startindex
    0x5F,0x4F,0x50,0x82,0x55,0x81,0xBF,0x1F,0x00,0x4D,0xB,0xC,0x0,0x0,0x0,0x0,
    0x83,0x85,0x5D,0x28,0x1F,0x63,0xBA,0xA3,0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,

    METAOUT+ATCOUT,                 // program atc registers
    ATT_ADDRESS_PORT,
    VGA_NUM_ATTRIB_CONT_PORTS,0,    // count, startindex
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x17, 0x38, 0x39, 0x3A, 0x3B, 0x3C,
    0x3D, 0x3E, 0x3F, 0x3F, 0x04, 0x00,
    0x0F, 0x00, 0x00,

    METAOUT+INDXOUT,                // program gdc registers
    GRAPH_ADDRESS_PORT,
    VGA_NUM_GRAPH_CONT_PORTS,0,     // count, startindex
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
    0x0E, 0x00, 0xFF,

    OB,
    DAC_PIXEL_MASK_PORT,
    0xFF,

    IB,                             // prepare atc for writing
    INPUT_STATUS_1_COLOR,
    
    OB,                             // turn video on.
    ATT_ADDRESS_PORT,
    VIDEO_ENABLE,

    EOD
};
// eVb: 4.2 [END]  
//
// Video mode table - contains information and commands for initializing each
// mode. These entries must correspond with those in VIDEO_MODE_VGA. The first
// entry is commented; the rest follow the same format, but are not so
// heavily commented.
//
// eVb: 4.3 [VGA] - Add VGA, ModeX and SVGA mode instead of Cirrus Modes
VIDEOMODE ModesVGA[] =
{
    // Color text mode 3, 720x400, 9x16 char cell (VGA).
    //
    {
       VIDEO_MODE_BANKED | VIDEO_MODE_COLOR,  // flags that this mode is a color mode, but not graphics
        4,                 // four planes
        1,                 // one bit of colour per plane
        80, 25,            // 80x25 text resolution
        720, 400,          // 720x400 pixels on screen
        1,                 // only support one frequency, non-interlaced
        160, 0x10000,      // 160 bytes per scan line, 64K of CPU-addressable bitmap
        FALSE,
        0x3, VGA_TEXT_0,   // Mode 3, I/O initialization stream
        0xA0000,           // Physical address at 0xA0000
        0x18000, 0x8000,
        0x20000,           // 2 banks of 64K, 128KB total memory
        720,               // 720 pixels per scan line
        FALSE,
        0
    },
    
    //
    // Color text mode 3, 640x350, 8x14 char cell (EGA).
    //
    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR,
        4, 1,
        80, 25, 
        640, 350,
        1,
        160, 0x10000,
        FALSE,
        0x3, VGA_TEXT_1,
        0xA0000,
        0x18000, 0x8000,
        0x20000,
        640,
        FALSE,
        0
    },
    
    //
    //
    // Standard VGA Color graphics mode 0x12, 640x480 16 colors.
    //
    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        4, 1,
        80, 30, 
        640, 480,
        1,
        80, 0x10000,
        FALSE,
        0x12, VGA_640x480,
        0xA0000,
        0, 0x20000,
        0x20000,
        640,
        FALSE,
        0
    },

    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        8, 1,
        0, 0, 
        320, 200,
        70,
        80, 0x10000,
        FALSE,
        0x3, NULL,
        0xA0000,
        0, 0x20000,
        0x20000,
        320,
        FALSE,
        0
    },
    
    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        8, 1,
        0, 0, 
        320, 240,
        60,
        80, 0x10000,
        FALSE,
        0x3, NULL,
        0xA0000,
        0, 0x20000,
        0x20000,
        320,
        FALSE,
        0
    },
    
    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        8, 1,
        0, 0, 
        320, 400,
        70,
        80, 0x10000,
        FALSE,
        0x3, NULL,
        0xA0000,
        0, 0x20000,
        0x20000,
        320,
        FALSE,
        0
    },

    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        8, 1,
        0, 0, 
        320, 480,
        60,
        80, 0x10000,
        FALSE,
        0x3, NULL,
        0xA0000,
        0, 0x20000,
        0x20000,
        320,
        FALSE,
        0
    },
    
    //
    // 800x600 16 colors.
    //
    {
        VIDEO_MODE_BANKED | VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS,
        4, 1,
        100, 37,
        800, 600,
        1,
        100, 0x10000,
        FALSE,
        (0x102 << 16) | VBE_SET_VBE_MODE, NULL,
        0xA0000,
        0, 0x20000,
        0x20000,
        800,
        FALSE,
        0
    },
};

ULONG NumVideoModes = sizeof(ModesVGA) / sizeof(VIDEOMODE);
PVIDEOMODE VgaModeList;
// eVb: 4.3 [END]

//
//
// Data used to set the Graphics and Sequence Controllers to put the
// VGA into a planar state at A0000 for 64K, with plane 2 enabled for
// reads and writes, so that a font can be loaded, and to disable that mode.
//

// Settings to enable planar mode with plane 2 enabled.
//

USHORT EnableA000Data[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0204,     // Read Map = plane 2
    0x0005, // Graphics Mode = read mode 0, write mode 0
    0x0406, // Graphics Miscellaneous register = A0000 for 64K, not odd/even,
            //  graphics mode
    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0402, // Map Mask = write to plane 2 only
    0x0404, // Memory Mode = not odd/even, not full memory, graphics mode
    0x0300,  // end sync reset
    EOD
};

//
// Settings to disable the font-loading planar mode.
//

USHORT DisableA000Color[] = {
    OWM,
    SEQ_ADDRESS_PORT,
    1,
    0x0100,

    OWM,
    GRAPH_ADDRESS_PORT,
    3,
    0x0004, 0x1005, 0x0E06,

    OWM,
    SEQ_ADDRESS_PORT,
    3,
    0x0302, 0x0204, 0x0300,  // end sync reset
    EOD

};
