/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/video/miniport/vmx_svga/vmware.h
 * PURPOSE:         VMWARE SVGA-II Card Registers and Definitions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

//
// IT'S OVER 9000 THOUSAND!!!!!!!!!!
//
#define SVGA_MAGIC                  0x900000

//
// Known VMWARE SVGA Versions
//
#define SVGA_VERSION_2              2
#define SVGA_VERSION_1              1
#define SVGA_VERSION_0              0

//
// Known VMWARE SVGA IDs
//
#define SVGA_MAKE_ID(x)             (SVGA_MAGIC << 8 | (x))
#define SVGA_ID_2                   SVGA_MAKE_ID(SVGA_VERSION_2)
#define SVGA_ID_1                   SVGA_MAKE_ID(SVGA_VERSION_1)
#define SVGA_ID_0                   SVGA_MAKE_ID(SVGA_VERSION_0)
#define SVGA_ID_INVALID             0xFFFFFFFF

//
// Card Capabilities
//
#define SVGA_CAP_NONE               0x00000000
#define SVGA_CAP_RECT_FILL	        0x00000001
#define SVGA_CAP_RECT_COPY	        0x00000002
#define SVGA_CAP_RECT_PAT_FILL      0x00000004
#define SVGA_CAP_LEGACY_OFFSCREEN   0x00000008
#define SVGA_CAP_RASTER_OP	        0x00000010
#define SVGA_CAP_CURSOR		        0x00000020
#define SVGA_CAP_CURSOR_BYPASS	    0x00000040
#define SVGA_CAP_CURSOR_BYPASS_2    0x00000080
#define SVGA_CAP_8BIT_EMULATION     0x00000100
#define SVGA_CAP_ALPHA_CURSOR       0x00000200
#define SVGA_CAP_GLYPH              0x00000400
#define SVGA_CAP_GLYPH_CLIPPING     0x00000800
#define SVGA_CAP_OFFSCREEN_1        0x00001000
#define SVGA_CAP_ALPHA_BLEND        0x00002000
#define SVGA_CAP_3D                 0x00004000
#define SVGA_CAP_EXTENDED_FIFO      0x00008000
#define SVGA_CAP_MULTIMON           0x00010000
#define SVGA_CAP_PITCHLOCK          0x00020000
#define SVGA_CAP_IRQMASK            0x00040000
#define SVGA_CAP_DISPLAY_TOPOLOGY   0x00080000

//
// Port Offsets and Base in PCI Space
//
#define SVGA_LEGACY_BASE_PORT	    0x4560
#define SVGA_INDEX_PORT		        0x0
#define SVGA_VALUE_PORT		        0x1
#define SVGA_BIOS_PORT		        0x2
#define SVGA_NUM_PORTS		        0x3
#define SVGA_IRQSTATUS_PORT     	0x8

//
// Invalid display ID
//
#define SVGA_INVALID_DISPLAY_ID     0xFFFFFFFF

//
// Global Maximums
//
#define SVGA_MAX_BITS_PER_PIXEL	    32
#define SVGA_MAX_DEPTH              24
#define SVGA_MAX_DISPLAYS           10
#define SVGA_MAX_PSEUDOCOLOR_DEPTH	8
#define SVGA_MAX_PSEUDOCOLORS		(1 << SVGA_MAX_PSEUDOCOLOR_DEPTH)
#define SVGA_NUM_PALETTE_REGS       (3 * SVGA_MAX_PSEUDOCOLORS)
#define SVGA_FB_MAX_SIZE                                                    \
   ((((SVGA_MAX_WIDTH * SVGA_MAX_HEIGHT *                                   \
       SVGA_MAX_BITS_PER_PIXEL / 8) >> PAGE_SHIFT) + 1) << PAGE_SHIFT)

//
// Card Registers
//
typedef enum _VMX_SVGA_REGISTERS
{
    SVGA_REG_ID,
    SVGA_REG_ENABLE,
    SVGA_REG_WIDTH,
    SVGA_REG_HEIGHT,
    SVGA_REG_MAX_WIDTH,
    SVGA_REG_MAX_HEIGHT,
    SVGA_REG_DEPTH,
    SVGA_REG_BITS_PER_PIXEL,
    SVGA_REG_PSEUDOCOLOR,
    SVGA_REG_RED_MASK,
    SVGA_REG_GREEN_MASK,
    SVGA_REG_BLUE_MASK,
    SVGA_REG_BYTES_PER_LINE,
    SVGA_REG_FB_START,
    SVGA_REG_FB_OFFSET,
    SVGA_REG_VRAM_SIZE,
    SVGA_REG_FB_SIZE,
    SVGA_REG_CAPABILITIES,
    SVGA_REG_MEM_START,
    SVGA_REG_MEM_SIZE,
    SVGA_REG_CONFIG_DONE,
    SVGA_REG_SYNC,
    SVGA_REG_BUSY,
    SVGA_REG_GUEST_ID,
    SVGA_REG_CURSOR_ID,
    SVGA_REG_CURSOR_X,
    SVGA_REG_CURSOR_Y,
    SVGA_REG_CURSOR_ON,
    SVGA_REG_HOST_BITS_PER_PIXEL,
    SVGA_REG_SCRATCH_SIZE,
    SVGA_REG_MEM_REGS,
    SVGA_REG_NUM_DISPLAYS,
    SVGA_REG_PITCHLOCK,
    SVGA_REG_IRQMASK,
    SVGA_REG_NUM_GUEST_DISPLAYS,
    SVGA_REG_DISPLAY_ID,
    SVGA_REG_DISPLAY_IS_PRIMARY,
    SVGA_REG_DISPLAY_POSITION_X,
    SVGA_REG_DISPLAY_POSITION_Y,
    SVGA_REG_DISPLAY_WIDTH,
    SVGA_REG_DISPLAY_HEIGHT,
    SVGA_REG_TOP,
} VMX_SVGA_REGISTERS;
