/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            svga.h
 * PURPOSE:         SuperVGA hardware emulation (Cirrus Logic CL-GD5434 compatible)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _SVGA_H_
#define _SVGA_H_

/* DEFINES ********************************************************************/

#define VGA_NUM_BANKS 4
#define VGA_BANK_SIZE 0x10000
#define VGA_MAX_COLORS 256
#define VGA_PALETTE_SIZE (VGA_MAX_COLORS * 3)
#define VGA_BITMAP_INFO_SIZE (sizeof(BITMAPINFOHEADER) + 2 * (VGA_PALETTE_SIZE / 3))
#define VGA_MINIMUM_WIDTH 400
#define VGA_MINIMUM_HEIGHT 300
#define VGA_DAC_TO_COLOR(x) (((x) << 2) | ((x) >> 4))
#define VGA_COLOR_TO_DAC(x) ((x) >> 2)
#define VGA_INTERLACE_HIGH_BIT (1 << 13)
#define VGA_FONT_BANK 2
#define VGA_FONT_CHARACTERS 256
#define VGA_MAX_FONT_HEIGHT 32
#define VGA_FONT_SIZE (VGA_FONT_CHARACTERS * VGA_MAX_FONT_HEIGHT)
#define VGA_CLOCK_BASE 14318181

#define SVGA_IS_UNLOCKED (VgaSeqRegisters[SVGA_SEQ_UNLOCK_REG] == SVGA_SEQ_UNLOCKED)
#define SVGA_BANK_SIZE 0x100000

#define SVGA_SEQ_MAX_UNLOCKED_REG  (SVGA_IS_UNLOCKED ? SVGA_SEQ_MAX_REG  : SVGA_SEQ_EXT_MODE_REG)
#define SVGA_CRTC_MAX_UNLOCKED_REG (SVGA_IS_UNLOCKED ? SVGA_CRTC_MAX_REG : VGA_CRTC_MAX_REG)
#define SVGA_GC_MAX_UNLOCKED_REG   (SVGA_IS_UNLOCKED ? SVGA_GC_MAX_REG   : VGA_GC_MAX_REG)

/* Register I/O ports */

#define VGA_MISC_READ       0x3CC
#define VGA_MISC_WRITE      0x3C2

#define VGA_INSTAT0_READ    0x3C2

#define VGA_INSTAT1_READ_MONO   0x3BA
#define VGA_INSTAT1_READ_COLOR  0x3DA

#define VGA_FEATURE_READ        0x3CA
#define VGA_FEATURE_WRITE_MONO  0x3BA
#define VGA_FEATURE_WRITE_COLOR 0x3DA

#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_AC_READ         0x3C1

#define VGA_SEQ_INDEX       0x3C4
#define VGA_SEQ_DATA        0x3C5

#define VGA_DAC_MASK        0x3C6
#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA        0x3C9

#define VGA_CRTC_INDEX_MONO     0x3B4
#define VGA_CRTC_DATA_MONO      0x3B5
#define VGA_CRTC_INDEX_COLOR    0x3D4
#define VGA_CRTC_DATA_COLOR     0x3D5

#define VGA_GC_INDEX        0x3CE
#define VGA_GC_DATA         0x3CF

#define VGA_SEQ_INDEX_MASK  0x1F
#define VGA_GC_INDEX_MASK   0x3F
#define VGA_CRTC_INDEX_MASK 0x3F



//
// Miscellaneous and Status Registers
//

/* Miscellaneous register bits */
#define VGA_MISC_COLOR          (1 << 0)
#define VGA_MISC_RAM_ENABLED    (1 << 1)
// #define VGA_MISC_CSEL1         (1 << 2)
// #define VGA_MISC_CSEL2         (1 << 3)
#define VGA_MISC_OE_PAGESEL     (1 << 5)
#define VGA_MISC_HSYNCP         (1 << 6)
#define VGA_MISC_VSYNCP         (1 << 7)

/* Status register flags */
#define VGA_STAT_DD             (1 << 0)
#define VGA_STAT_VRETRACE       (1 << 3)


//
// Sequencer Registers
//

/* Sequencer reset register bits */
#define VGA_SEQ_RESET_AR    (1 << 0)
#define VGA_SEQ_RESET_SR    (1 << 1)

/* Sequencer clock register bits */
#define VGA_SEQ_CLOCK_98DM  (1 << 0)
#define VGA_SEQ_CLOCK_SLR   (1 << 2)
#define VGA_SEQ_CLOCK_DCR   (1 << 3)
#define VGA_SEQ_CLOCK_S4    (1 << 4)
#define VGA_SEQ_CLOCK_SD    (1 << 5)

/* Sequencer memory register bits */
#define VGA_SEQ_MEM_EXT     (1 << 1)
#define VGA_SEQ_MEM_OE_DIS  (1 << 2)
#define VGA_SEQ_MEM_C4      (1 << 3)

#define SVGA_SEQ_LOCKED      0x0F
#define SVGA_SEQ_UNLOCK_MASK 0x17
#define SVGA_SEQ_UNLOCKED    0x12

/* Sequencer extended mode register bits */
#define SVGA_SEQ_EXT_MODE_HIGH_RES     (1 << 0)

/* Sequencer extended control register bits */
#define SVGA_SEQ_EXT_CONTROL_MMIO      (1 << 2)
#define SVGA_SEQ_EXT_CONTROL_MMIO_HIGH (1 << 6)

/* MCLK register bits */
#define SVGA_SEQ_MCLK_VCLK (1 << 6)

enum
{
    VGA_SEQ_RESET_REG,
    VGA_SEQ_CLOCK_REG,
    VGA_SEQ_MASK_REG,
    VGA_SEQ_CHAR_REG,
    VGA_SEQ_MEM_REG,
    VGA_SEQ_MAX_REG,
    SVGA_SEQ_UNLOCK_REG,
    SVGA_SEQ_EXT_MODE_REG,
    SVGA_SEQ_EEPROM_REG,
    SVGA_SEQ_SCRATCH_0_REG,
    SVGA_SEQ_SCRATCH_1_REG,
    SVGA_SEQ_VCLK0_NUMERATOR_REG,
    SVGA_SEQ_VCLK1_NUMERATOR_REG,
    SVGA_SEQ_VCLK2_NUMERATOR_REG,
    SVGA_SEQ_VCLK3_NUMERATOR_REG,
    SVGA_SEQ_DRAM_REG,
    SVGA_SEQ_CURSOR_X_REG,
    SVGA_SEQ_CURSOR_Y_REG,
    SVGA_SEQ_CURSOR_ATTR_REG,
    SVGA_SEQ_CURSOR_PATTERN_REG,
    SVGA_SEQ_SCRATCH_2_REG,
    SVGA_SEQ_SCRATCH_3_REG,
    SVGA_SEQ_PERFORMANCE_REG,
    SVGA_SEQ_EXT_CONTROL_REG,
    SVGA_SEQ_SIG_GEN_CONTROL_REG,
    SVGA_SEQ_SIG_GEN_RESULT_LOW_REG,
    SVGA_SEQ_SIG_GEN_RESULT_HIGH_REG,
    SVGA_SEQ_VCLK0_DENOMINATOR_REG,
    SVGA_SEQ_VCLK1_DENOMINATOR_REG,
    SVGA_SEQ_VCLK2_DENOMINATOR_REG,
    SVGA_SEQ_VCLK3_DENOMINATOR_REG,
    SVGA_SEQ_MCLK_REG,
    SVGA_SEQ_MAX_REG
};

//
// CRT Controller Registers
//

/* CRTC overflow register bits */
#define VGA_CRTC_OVERFLOW_VT8       (1 << 0)
#define VGA_CRTC_OVERFLOW_VDE8      (1 << 1)
#define VGA_CRTC_OVERFLOW_VRS8      (1 << 2)
#define VGA_CRTC_OVERFLOW_SVB8      (1 << 3)
#define VGA_CRTC_OVERFLOW_LC8       (1 << 4)
#define VGA_CRTC_OVERFLOW_VT9       (1 << 5)
#define VGA_CRTC_OVERFLOW_VDE9      (1 << 6)
#define VGA_CRTC_OVERFLOW_VRS9      (1 << 7)

/* CRTC underline register bits */
#define VGA_CRTC_UNDERLINE_DWORD    (1 << 6)

/* CRTC max scanline register bits */
#define VGA_CRTC_MAXSCANLINE_DOUBLE (1 << 7)

/* CRTC mode control register bits */
#define VGA_CRTC_MODE_CONTROL_WRAP  (1 << 5)
#define VGA_CRTC_MODE_CONTROL_BYTE  (1 << 6)
#define VGA_CRTC_MODE_CONTROL_SYNC  (1 << 7)

enum
{
    VGA_CRTC_HORZ_TOTAL_REG,
    VGA_CRTC_END_HORZ_DISP_REG,
    VGA_CRTC_START_HORZ_BLANKING_REG,
    VGA_CRTC_END_HORZ_BLANKING_REG,
    VGA_CRTC_START_HORZ_RETRACE_REG,
    VGA_CRTC_END_HORZ_RETRACE_REG,
    VGA_CRTC_VERT_TOTAL_REG,
    VGA_CRTC_OVERFLOW_REG,
    VGA_CRTC_PRESET_ROW_SCAN_REG,
    VGA_CRTC_MAX_SCAN_LINE_REG,
    VGA_CRTC_CURSOR_START_REG,
    VGA_CRTC_CURSOR_END_REG,
    VGA_CRTC_START_ADDR_HIGH_REG,
    VGA_CRTC_START_ADDR_LOW_REG,
    VGA_CRTC_CURSOR_LOC_HIGH_REG,
    VGA_CRTC_CURSOR_LOC_LOW_REG,
    VGA_CRTC_VERT_RETRACE_START_REG,
    VGA_CRTC_VERT_RETRACE_END_REG,
    VGA_CRTC_VERT_DISP_END_REG,
    VGA_CRTC_OFFSET_REG,
    VGA_CRTC_UNDERLINE_REG,
    VGA_CRTC_START_VERT_BLANKING_REG,
    VGA_CRTC_END_VERT_BLANKING_REG,
    VGA_CRTC_MODE_CONTROL_REG,
    VGA_CRTC_LINE_COMPARE_REG,
    VGA_CRTC_MAX_REG,
    SVGA_CRTC_INTERLACE_END_REG = VGA_CRTC_MAX_REG,
    SVGA_CRTC_MISC_CONTROL_REG,
    SVGA_CRTC_EXT_DISPLAY_REG,
    SVGA_CRTC_SYNC_ADJUST_REG,
    SVGA_CRTC_OVERLAY_REG,
    SVGA_CRTC_UNUSED0_REG,
    SVGA_CRTC_UNUSED1_REG,
    SVGA_CRTC_UNUSED2_REG,
    SVGA_CRTC_UNUSED3_REG,
    SVGA_CRTC_UNUSED4_REG,
    SVGA_CRTC_UNUSED5_REG,
    SVGA_CRTC_UNUSED6_REG,
    SVGA_CRTC_PART_STATUS_REG,
    SVGA_CRTC_UNUSED7_REG,
    SVGA_CRTC_ID_REG,
    SVGA_CRTC_MAX_REG
};


//
// Graphics Controller Registers
//

/* Graphics controller mode register bits */
#define VGA_GC_MODE_READ        (1 << 3)
#define VGA_GC_MODE_OE          (1 << 4)
#define VGA_GC_MODE_SHIFTREG    (1 << 5)
#define VGA_GC_MODE_SHIFT256    (1 << 6)

/* Graphics controller miscellaneous register bits */
#define VGA_GC_MISC_NOALPHA     (1 << 0)
#define VGA_GC_MISC_OE          (1 << 1)

enum
{
    VGA_GC_RESET_REG,
    VGA_GC_ENABLE_RESET_REG,
    VGA_GC_COLOR_COMPARE_REG,
    VGA_GC_ROTATE_REG,
    VGA_GC_READ_MAP_SEL_REG,
    VGA_GC_MODE_REG,
    VGA_GC_MISC_REG,
    VGA_GC_COLOR_IGNORE_REG,
    VGA_GC_BITMASK_REG,
    VGA_GC_MAX_REG,
    SVGA_GC_OFFSET_0_REG = VGA_GC_MAX_REG,
    SVGA_GC_OFFSET_1_REG,
    SVGA_GC_EXT_MODE_REG,
    SVGA_GC_COLOR_COMPARE_REG,
    SVGA_GC_BITMASK_REG,
    SVGA_GC_POWER_MANAGEMENT_REG,
    SVGA_GC_UNUSED0_REG,
    SVGA_GC_BACKGROUND_1_REG,
    SVGA_GC_FOREGROUND_1_REG,
    SVGA_GC_BACKGROUND_2_REG,
    SVGA_GC_FOREGROUND_2_REG,
    SVGA_GC_BACKGROUND_3_REG,
    SVGA_GC_FOREGROUND_3_REG,
    SVGA_GC_UNUSED1_REG,
    SVGA_GC_UNUSED2_REG,
    SVGA_GC_UNUSED3_REG,
    SVGA_GC_UNUSED4_REG,
    SVGA_GC_UNUSED5_REG,
    SVGA_GC_UNUSED6_REG,
    SVGA_GC_UNUSED7_REG,
    SVGA_GC_UNUSED8_REG,
    SVGA_GC_UNUSED9_REG,
    SVGA_GC_UNUSED10_REG,
    SVGA_GC_BLT_WIDTH_LOW_REG,
    SVGA_GC_BLT_WIDTH_HIGH_REG,
    SVGA_GC_BLT_HEIGHT_LOW_REG,
    SVGA_GC_BLT_HEIGHT_HIGH_REG,
    SVGA_GC_BLT_DEST_PITCH_LOW_REG,
    SVGA_GC_BLT_DEST_PITCH_HIGH_REG,
    SVGA_GC_BLT_SRC_PITCH_LOW_REG,
    SVGA_GC_BLT_SRC_PITCH_HIGH_REG,
    SVGA_GC_BLT_DEST_START_0_REG,
    SVGA_GC_BLT_DEST_START_1_REG,
    SVGA_GC_BLT_DEST_START_2_REG,
    SVGA_GC_UNUSED11_REG,
    SVGA_GC_BLT_SRC_START_0_REG,
    SVGA_GC_BLT_SRC_START_1_REG,
    SVGA_GC_BLT_SRC_START_2_REG,
    SVGA_GC_BLT_DEST_MASK_REG,
    SVGA_GC_BLT_MODE_REG,
    SVGA_GC_BLT_STATUS_REG,
    SVGA_GC_BLT_ROP_REG,
    SVGA_GC_BLT_EXT_MODE_REG,
    SVGA_GC_MAX_REG
};


//
// Attribute Controller Registers
// They are a relinquish of the CGA/EGA era.
//

/* AC mode control register bits */
#define VGA_AC_CONTROL_ATGE     (1 << 0)
#define VGA_AC_CONTROL_MONO     (1 << 1)
#define VGA_AC_CONTROL_LGE      (1 << 2)
#define VGA_AC_CONTROL_BLINK    (1 << 3)
#define VGA_AC_CONTROL_PPM      (1 << 5)
#define VGA_AC_CONTROL_8BIT     (1 << 6)
#define VGA_AC_CONTROL_P54S     (1 << 7)

enum
{
    VGA_AC_PAL_0_REG,
    VGA_AC_PAL_1_REG,
    VGA_AC_PAL_2_REG,
    VGA_AC_PAL_3_REG,
    VGA_AC_PAL_4_REG,
    VGA_AC_PAL_5_REG,
    VGA_AC_PAL_6_REG,
    VGA_AC_PAL_7_REG,
    VGA_AC_PAL_8_REG,
    VGA_AC_PAL_9_REG,
    VGA_AC_PAL_A_REG,
    VGA_AC_PAL_B_REG,
    VGA_AC_PAL_C_REG,
    VGA_AC_PAL_D_REG,
    VGA_AC_PAL_E_REG,
    VGA_AC_PAL_F_REG,
    VGA_AC_CONTROL_REG,
    VGA_AC_OVERSCAN_REG,
    VGA_AC_COLOR_PLANE_REG,
    VGA_AC_HORZ_PANNING_REG,
    VGA_AC_COLOR_SEL_REG,
    VGA_AC_MAX_REG
};


typedef struct _VGA_REGISTERS
{
    UCHAR Misc;
    UCHAR Sequencer[VGA_SEQ_MAX_REG];
    UCHAR CRT[VGA_CRTC_MAX_REG];
    UCHAR Graphics[VGA_GC_MAX_REG];
    UCHAR Attribute[VGA_AC_MAX_REG];
} VGA_REGISTERS, *PVGA_REGISTERS;

typedef struct _SVGA_REGISTERS
{
    UCHAR Misc;
    UCHAR Hidden;
    UCHAR Sequencer[SVGA_SEQ_MAX_REG];
    UCHAR CRT[SVGA_CRTC_MAX_REG];
    UCHAR Graphics[SVGA_GC_MAX_REG];
    UCHAR Attribute[VGA_AC_MAX_REG];
} SVGA_REGISTERS, *PSVGA_REGISTERS;

/* FUNCTIONS ******************************************************************/

VOID ScreenEventHandler(PWINDOW_BUFFER_SIZE_RECORD ScreenEvent);
BOOL VgaAttachToConsole(VOID);
VOID VgaDetachFromConsole(VOID);

COORD VgaGetDisplayResolution(VOID);
VOID VgaRefreshDisplay(VOID);
VOID FASTCALL VgaReadMemory(ULONG Address, PVOID Buffer, ULONG Size);
BOOLEAN FASTCALL VgaWriteMemory(ULONG Address, PVOID Buffer, ULONG Size);
VOID VgaWriteTextModeFont(UINT FontNumber, CONST UCHAR *FontData, UINT Height);
VOID VgaClearMemory(VOID);
BOOLEAN VgaGetDoubleVisionState(PBOOLEAN Horizontal, PBOOLEAN Vertical);

BOOLEAN VgaInitialize(HANDLE TextHandle);
VOID VgaCleanup(VOID);

#endif // _SVGA_H_

/* EOF */
