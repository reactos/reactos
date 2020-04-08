#include "precomp.h"

//
// Minimal Attribute Controller Registers initialization command stream.
// Compatible EGA.
//
USHORT AT_Initialization[] =
{
    /* Reset ATC to index mode */
    IB,
    VGA_BASE_IO_PORT + ATT_INITIALIZE_PORT_COLOR /* INPUT_STATUS_1_COLOR */,

    /* Write the AC registers */
    METAOUT+ATCOUT,
    VGA_BASE_IO_PORT + ATT_ADDRESS_PORT /* ATT_DATA_WRITE_PORT */,
    16, 0,  // Values Count and Start Index
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, // Palette indices 0-5
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, // Palette indices 6-11
    0x0C, 0x0D, 0x0E, 0x0F,             // Palette indices 12-15

    /* Reset ATC to index mode */
    IB,
    VGA_BASE_IO_PORT + ATT_INITIALIZE_PORT_COLOR /* INPUT_STATUS_1_COLOR */,

    /* Enable screen and disable palette access */
    OB,
    VGA_BASE_IO_PORT + ATT_ADDRESS_PORT /* ATT_DATA_WRITE_PORT */,
    VIDEO_ENABLE,

    /* End of Stream */
    EOD
};

//
// 640x480 256-color 60Hz mode (BIOS mode 12) set command stream for VGA.
// Adapted from win32ss/drivers/miniport/vga_new/vgadata.c
//
USHORT VGA_640x480[] =
{
    /* Write the Sequencer Registers */
    OWM,
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT,
    VGA_NUM_SEQUENCER_PORTS,    // Values Count (5)
    // HI: Value in SEQ_DATA_PORT, LO: Register index in SEQ_ADDRESS_PORT
    0x0100, // Synchronous reset on
    0x0101, // 8-Dot Mode
    0x0F02, // Memory Plane Write Enable on all planes 0-3
    0x0003, // No character set selected
    0x0604, // Disable Odd/Even host mem addressing; Enable Extended Memory

    /* Write the Miscellaneous Register */
    OB,
    VGA_BASE_IO_PORT + MISC_OUTPUT_REG_WRITE_PORT,
    0xE3,   // V/H-SYNC polarity, Odd/Even High page select, RAM enable,
            // I/O Address select (1: color/graphics adapter)

    /* Enable Graphics Mode */
    OW,
    VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT,
    // HI: Value in GRAPH_DATA_PORT, LO: Register index in GRAPH_ADDRESS_PORT
    0x506,  // Select A0000h-AFFFFh memory region, Disable Alphanumeric mode

    /* Synchronous reset off */
    OW,
    VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT,
    // HI: Value in SEQ_DATA_PORT, LO: Register index in SEQ_ADDRESS_PORT
    0x0300, // Synchronous reset off (LO: IND_SYNC_RESET, HI: END_SYNC_RESET_VALUE)

    /* Unlock CRTC registers 0-7 */
    OW,
    VGA_BASE_IO_PORT + CRTC_ADDRESS_PORT_COLOR,
    0x511,

    /* Write the CRTC registers */
    METAOUT+INDXOUT,
    VGA_BASE_IO_PORT + CRTC_ADDRESS_PORT_COLOR,
    VGA_NUM_CRTC_PORTS, 0,              // Values Count (25) and Start Index
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
    0xFF,

    /* Reset ATC to index mode */
    IB,
    VGA_BASE_IO_PORT + ATT_INITIALIZE_PORT_COLOR /* INPUT_STATUS_1_COLOR */,

    /* Write the AC registers */
    METAOUT+ATCOUT,
    VGA_BASE_IO_PORT + ATT_ADDRESS_PORT /* ATT_DATA_WRITE_PORT */,
    VGA_NUM_ATTRIB_CONT_PORTS, 0,       // Values Count (21) and Start Index
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, // Palette indices 0-5
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, // Palette indices 6-11
    0x0C, 0x0D, 0x0E, 0x0F,             // Palette indices 12-15
    0x01, 0x00, 0x0F, 0x00, 0x00,

    /* Write the GC registers */
    METAOUT+INDXOUT,
    VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT,
    VGA_NUM_GRAPH_CONT_PORTS, 0,        // Values Count (9) and Start Index
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x0F, 0xFF,

    /* Set the PEL mask */
    OB,
    VGA_BASE_IO_PORT + DAC_PIXEL_MASK_PORT,
    0xFF,

    /* Reset ATC to index mode */
    IB,
    VGA_BASE_IO_PORT + ATT_INITIALIZE_PORT_COLOR /* INPUT_STATUS_1_COLOR */,

    /* Enable screen and disable palette access */
    OB,
    VGA_BASE_IO_PORT + ATT_ADDRESS_PORT /* ATT_DATA_WRITE_PORT */,
    VIDEO_ENABLE,

    /* End of Stream */
    EOD
};
