/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            subsystems/mvdm/ntvdm/hardware/video/svga.c
 * PURPOSE:         SuperVGA hardware emulation (Cirrus Logic CL-GD5434 compatible)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

#define NDEBUG
#include <debug.h>

#include "emulator.h"
#include "svga.h"
#include <bios/vidbios.h>

#include "memory.h"
#include "io.h"
#include "clock.h"

#include "../../console/video.h"

/* PRIVATE VARIABLES **********************************************************/

#define WRAP_OFFSET(x) ((VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_ADDR_WRAP) \
                       ? ((x) & 0xFFFFF) : LOWORD(x))

static CONST DWORD MemoryBase[] = { 0xA0000, 0xA0000, 0xB0000, 0xB8000 };
static CONST DWORD MemorySize[] = { 0x20000, 0x10000, 0x08000, 0x08000 };

#define USE_REACTOS_COLORS
// #define USE_DOSBOX_COLORS

#if defined(USE_REACTOS_COLORS)

// ReactOS colors
static CONST COLORREF VgaDefaultPalette[VGA_MAX_COLORS] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x10, 0x10, 0x10), RGB(0x20, 0x20, 0x20), RGB(0x35, 0x35, 0x35),
    RGB(0x45, 0x45, 0x45), RGB(0x55, 0x55, 0x55), RGB(0x65, 0x65, 0x65), RGB(0x75, 0x75, 0x75),
    RGB(0x8A, 0x8A, 0x8A), RGB(0x9A, 0x9A, 0x9A), RGB(0xAA, 0xAA, 0xAA), RGB(0xBA, 0xBA, 0xBA),
    RGB(0xCA, 0xCA, 0xCA), RGB(0xDF, 0xDF, 0xDF), RGB(0xEF, 0xEF, 0xEF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0xFF), RGB(0x41, 0x00, 0xFF), RGB(0x82, 0x00, 0xFF), RGB(0xBE, 0x00, 0xFF),
    RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0x00, 0xBE), RGB(0xFF, 0x00, 0x82), RGB(0xFF, 0x00, 0x41),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x41, 0x00), RGB(0xFF, 0x82, 0x00), RGB(0xFF, 0xBE, 0x00),
    RGB(0xFF, 0xFF, 0x00), RGB(0xBE, 0xFF, 0x00), RGB(0x82, 0xFF, 0x00), RGB(0x41, 0xFF, 0x00),
    RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0x41), RGB(0x00, 0xFF, 0x82), RGB(0x00, 0xFF, 0xBE),
    RGB(0x00, 0xFF, 0xFF), RGB(0x00, 0xBE, 0xFF), RGB(0x00, 0x82, 0xFF), RGB(0x00, 0x41, 0xFF),
    RGB(0x82, 0x82, 0xFF), RGB(0x9E, 0x82, 0xFF), RGB(0xBE, 0x82, 0xFF), RGB(0xDF, 0x82, 0xFF),
    RGB(0xFF, 0x82, 0xFF), RGB(0xFF, 0x82, 0xDF), RGB(0xFF, 0x82, 0xBE), RGB(0xFF, 0x82, 0x9E),
    RGB(0xFF, 0x82, 0x82), RGB(0xFF, 0x9E, 0x82), RGB(0xFF, 0xBE, 0x82), RGB(0xFF, 0xDF, 0x82),
    RGB(0xFF, 0xFF, 0x82), RGB(0xDF, 0xFF, 0x82), RGB(0xBE, 0xFF, 0x82), RGB(0x9E, 0xFF, 0x82),
    RGB(0x82, 0xFF, 0x82), RGB(0x82, 0xFF, 0x9E), RGB(0x82, 0xFF, 0xBE), RGB(0x82, 0xFF, 0xDF),
    RGB(0x82, 0xFF, 0xFF), RGB(0x82, 0xDF, 0xFF), RGB(0x82, 0xBE, 0xFF), RGB(0x82, 0x9E, 0xFF),
    RGB(0xBA, 0xBA, 0xFF), RGB(0xCA, 0xBA, 0xFF), RGB(0xDF, 0xBA, 0xFF), RGB(0xEF, 0xBA, 0xFF),
    RGB(0xFF, 0xBA, 0xFF), RGB(0xFF, 0xBA, 0xEF), RGB(0xFF, 0xBA, 0xDF), RGB(0xFF, 0xBA, 0xCA),
    RGB(0xFF, 0xBA, 0xBA), RGB(0xFF, 0xCA, 0xBA), RGB(0xFF, 0xDF, 0xBA), RGB(0xFF, 0xEF, 0xBA),
    RGB(0xFF, 0xFF, 0xBA), RGB(0xEF, 0xFF, 0xBA), RGB(0xDF, 0xFF, 0xBA), RGB(0xCA, 0xFF, 0xBA),
    RGB(0xBA, 0xFF, 0xBA), RGB(0xBA, 0xFF, 0xCA), RGB(0xBA, 0xFF, 0xDF), RGB(0xBA, 0xFF, 0xEF),
    RGB(0xBA, 0xFF, 0xFF), RGB(0xBA, 0xEF, 0xFF), RGB(0xBA, 0xDF, 0xFF), RGB(0xBA, 0xCA, 0xFF),
    RGB(0x00, 0x00, 0x71), RGB(0x1C, 0x00, 0x71), RGB(0x39, 0x00, 0x71), RGB(0x55, 0x00, 0x71),
    RGB(0x71, 0x00, 0x71), RGB(0x71, 0x00, 0x55), RGB(0x71, 0x00, 0x39), RGB(0x71, 0x00, 0x1C),
    RGB(0x71, 0x00, 0x00), RGB(0x71, 0x1C, 0x00), RGB(0x71, 0x39, 0x00), RGB(0x71, 0x55, 0x00),
    RGB(0x71, 0x71, 0x00), RGB(0x55, 0x71, 0x00), RGB(0x39, 0x71, 0x00), RGB(0x1C, 0x71, 0x00),
    RGB(0x00, 0x71, 0x00), RGB(0x00, 0x71, 0x1C), RGB(0x00, 0x71, 0x39), RGB(0x00, 0x71, 0x55),
    RGB(0x00, 0x71, 0x71), RGB(0x00, 0x55, 0x71), RGB(0x00, 0x39, 0x71), RGB(0x00, 0x1C, 0x71),
    RGB(0x39, 0x39, 0x71), RGB(0x45, 0x39, 0x71), RGB(0x55, 0x39, 0x71), RGB(0x61, 0x39, 0x71),
    RGB(0x71, 0x39, 0x71), RGB(0x71, 0x39, 0x61), RGB(0x71, 0x39, 0x55), RGB(0x71, 0x39, 0x45),
    RGB(0x71, 0x39, 0x39), RGB(0x71, 0x45, 0x39), RGB(0x71, 0x55, 0x39), RGB(0x71, 0x61, 0x39),
    RGB(0x71, 0x71, 0x39), RGB(0x61, 0x71, 0x39), RGB(0x55, 0x71, 0x39), RGB(0x45, 0x71, 0x39),
    RGB(0x39, 0x71, 0x39), RGB(0x39, 0x71, 0x45), RGB(0x39, 0x71, 0x55), RGB(0x39, 0x71, 0x61),
    RGB(0x39, 0x71, 0x71), RGB(0x39, 0x61, 0x71), RGB(0x39, 0x55, 0x71), RGB(0x39, 0x45, 0x71),
    RGB(0x51, 0x51, 0x71), RGB(0x59, 0x51, 0x71), RGB(0x61, 0x51, 0x71), RGB(0x69, 0x51, 0x71),
    RGB(0x71, 0x51, 0x71), RGB(0x71, 0x51, 0x69), RGB(0x71, 0x51, 0x61), RGB(0x71, 0x51, 0x59),
    RGB(0x71, 0x51, 0x51), RGB(0x71, 0x59, 0x51), RGB(0x71, 0x61, 0x51), RGB(0x71, 0x69, 0x51),
    RGB(0x71, 0x71, 0x51), RGB(0x69, 0x71, 0x51), RGB(0x61, 0x71, 0x51), RGB(0x59, 0x71, 0x51),
    RGB(0x51, 0x71, 0x51), RGB(0x51, 0x71, 0x59), RGB(0x51, 0x71, 0x61), RGB(0x51, 0x71, 0x69),
    RGB(0x51, 0x71, 0x71), RGB(0x51, 0x69, 0x71), RGB(0x51, 0x61, 0x71), RGB(0x51, 0x59, 0x71),
    RGB(0x00, 0x00, 0x41), RGB(0x10, 0x00, 0x41), RGB(0x20, 0x00, 0x41), RGB(0x31, 0x00, 0x41),
    RGB(0x41, 0x00, 0x41), RGB(0x41, 0x00, 0x31), RGB(0x41, 0x00, 0x20), RGB(0x41, 0x00, 0x10),
    RGB(0x41, 0x00, 0x00), RGB(0x41, 0x10, 0x00), RGB(0x41, 0x20, 0x00), RGB(0x41, 0x31, 0x00),
    RGB(0x41, 0x41, 0x00), RGB(0x31, 0x41, 0x00), RGB(0x20, 0x41, 0x00), RGB(0x10, 0x41, 0x00),
    RGB(0x00, 0x41, 0x00), RGB(0x00, 0x41, 0x10), RGB(0x00, 0x41, 0x20), RGB(0x00, 0x41, 0x31),
    RGB(0x00, 0x41, 0x41), RGB(0x00, 0x31, 0x41), RGB(0x00, 0x20, 0x41), RGB(0x00, 0x10, 0x41),
    RGB(0x20, 0x20, 0x41), RGB(0x28, 0x20, 0x41), RGB(0x31, 0x20, 0x41), RGB(0x39, 0x20, 0x41),
    RGB(0x41, 0x20, 0x41), RGB(0x41, 0x20, 0x39), RGB(0x41, 0x20, 0x31), RGB(0x41, 0x20, 0x28),
    RGB(0x41, 0x20, 0x20), RGB(0x41, 0x28, 0x20), RGB(0x41, 0x31, 0x20), RGB(0x41, 0x39, 0x20),
    RGB(0x41, 0x41, 0x20), RGB(0x39, 0x41, 0x20), RGB(0x31, 0x41, 0x20), RGB(0x28, 0x41, 0x20),
    RGB(0x20, 0x41, 0x20), RGB(0x20, 0x41, 0x28), RGB(0x20, 0x41, 0x31), RGB(0x20, 0x41, 0x39),
    RGB(0x20, 0x41, 0x41), RGB(0x20, 0x39, 0x41), RGB(0x20, 0x31, 0x41), RGB(0x20, 0x28, 0x41),
    RGB(0x2D, 0x2D, 0x41), RGB(0x31, 0x2D, 0x41), RGB(0x35, 0x2D, 0x41), RGB(0x3D, 0x2D, 0x41),
    RGB(0x41, 0x2D, 0x41), RGB(0x41, 0x2D, 0x3D), RGB(0x41, 0x2D, 0x35), RGB(0x41, 0x2D, 0x31),
    RGB(0x41, 0x2D, 0x2D), RGB(0x41, 0x31, 0x2D), RGB(0x41, 0x35, 0x2D), RGB(0x41, 0x3D, 0x2D),
    RGB(0x41, 0x41, 0x2D), RGB(0x3D, 0x41, 0x2D), RGB(0x35, 0x41, 0x2D), RGB(0x31, 0x41, 0x2D),
    RGB(0x2D, 0x41, 0x2D), RGB(0x2D, 0x41, 0x31), RGB(0x2D, 0x41, 0x35), RGB(0x2D, 0x41, 0x3D),
    RGB(0x2D, 0x41, 0x41), RGB(0x2D, 0x3D, 0x41), RGB(0x2D, 0x35, 0x41), RGB(0x2D, 0x31, 0x41),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00)
};

#elif defined(USE_DOSBOX_COLORS)

// DOSBox colors
static CONST COLORREF VgaDefaultPalette[VGA_MAX_COLORS] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x14, 0x14, 0x14), RGB(0x20, 0x20, 0x20), RGB(0x2C, 0x2C, 0x2C),
    RGB(0x38, 0x38, 0x38), RGB(0x45, 0x45, 0x45), RGB(0x51, 0x51, 0x51), RGB(0x61, 0x61, 0x61),
    RGB(0x71, 0x71, 0x71), RGB(0x82, 0x82, 0x82), RGB(0x92, 0x92, 0x92), RGB(0xA2, 0xA2, 0xA2),
    RGB(0xB6, 0xB6, 0xB6), RGB(0xCB, 0xCB, 0xCB), RGB(0xE3, 0xE3, 0xE3), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0xFF), RGB(0x41, 0x00, 0xFF), RGB(0x7D, 0x00, 0xFF), RGB(0xBE, 0x00, 0xFF),
    RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0x00, 0xBE), RGB(0xFF, 0x00, 0x7D), RGB(0xFF, 0x00, 0x41),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x41, 0x00), RGB(0xFF, 0x7D, 0x00), RGB(0xFF, 0xBE, 0x00),
    RGB(0xFF, 0xFF, 0x00), RGB(0xBE, 0xFF, 0x00), RGB(0x7D, 0xFF, 0x00), RGB(0x41, 0xFF, 0x00),
    RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0x41), RGB(0x00, 0xFF, 0x7D), RGB(0x00, 0xFF, 0xBE),
    RGB(0x00, 0xFF, 0xFF), RGB(0x00, 0xBE, 0xFF), RGB(0x00, 0x7D, 0xFF), RGB(0x00, 0x41, 0xFF),
    RGB(0x7D, 0x7D, 0xFF), RGB(0x9E, 0x7D, 0xFF), RGB(0xBE, 0x7D, 0xFF), RGB(0xDF, 0x7D, 0xFF),
    RGB(0xFF, 0x7D, 0xFF), RGB(0xFF, 0x7D, 0xDF), RGB(0xFF, 0x7D, 0xBE), RGB(0xFF, 0x7D, 0x9E),

    RGB(0xFF, 0x7D, 0x7D), RGB(0xFF, 0x9E, 0x7D), RGB(0xFF, 0xBE, 0x7D), RGB(0xFF, 0xDF, 0x7D),
    RGB(0xFF, 0xFF, 0x7D), RGB(0xDF, 0xFF, 0x7D), RGB(0xBE, 0xFF, 0x7D), RGB(0x9E, 0xFF, 0x7D),
    RGB(0x7D, 0xFF, 0x7D), RGB(0x7D, 0xFF, 0x9E), RGB(0x7D, 0xFF, 0xBE), RGB(0x7D, 0xFF, 0xDF),
    RGB(0x7D, 0xFF, 0xFF), RGB(0x7D, 0xDF, 0xFF), RGB(0x7D, 0xBE, 0xFF), RGB(0x7D, 0x9E, 0xFF),
    RGB(0xB6, 0xB6, 0xFF), RGB(0xC7, 0xB6, 0xFF), RGB(0xDB, 0xB6, 0xFF), RGB(0xEB, 0xB6, 0xFF),
    RGB(0xFF, 0xB6, 0xFF), RGB(0xFF, 0xB6, 0xEB), RGB(0xFF, 0xB6, 0xDB), RGB(0xFF, 0xB6, 0xC7),
    RGB(0xFF, 0xB6, 0xB6), RGB(0xFF, 0xC7, 0xB6), RGB(0xFF, 0xDB, 0xB6), RGB(0xFF, 0xEB, 0xB6),
    RGB(0xFF, 0xFF, 0xB6), RGB(0xEB, 0xFF, 0xB6), RGB(0xDB, 0xFF, 0xB6), RGB(0xC7, 0xFF, 0xB6),
    RGB(0xB6, 0xFF, 0xB6), RGB(0xB6, 0xFF, 0xC7), RGB(0xB6, 0xFF, 0xDB), RGB(0xB6, 0xFF, 0xEB),
    RGB(0xB6, 0xFF, 0xFF), RGB(0xB6, 0xEB, 0xFF), RGB(0xB6, 0xDB, 0xFF), RGB(0xB6, 0xC7, 0xFF),
    RGB(0x00, 0x00, 0x71), RGB(0x1C, 0x00, 0x71), RGB(0x38, 0x00, 0x71), RGB(0x55, 0x00, 0x71),
    RGB(0x71, 0x00, 0x71), RGB(0x71, 0x00, 0x55), RGB(0x71, 0x00, 0x38), RGB(0x71, 0x00, 0x1C),
    RGB(0x71, 0x00, 0x00), RGB(0x71, 0x1C, 0x00), RGB(0x71, 0x38, 0x00), RGB(0x71, 0x55, 0x00),
    RGB(0x71, 0x71, 0x00), RGB(0x55, 0x71, 0x00), RGB(0x38, 0x71, 0x00), RGB(0x1C, 0x71, 0x00),
    RGB(0x00, 0x71, 0x00), RGB(0x00, 0x71, 0x1C), RGB(0x00, 0x71, 0x38), RGB(0x00, 0x71, 0x55),
    RGB(0x00, 0x71, 0x71), RGB(0x00, 0x55, 0x71), RGB(0x00, 0x38, 0x71), RGB(0x00, 0x1C, 0x71),

    RGB(0x38, 0x38, 0x71), RGB(0x45, 0x38, 0x71), RGB(0x55, 0x38, 0x71), RGB(0x61, 0x38, 0x71),
    RGB(0x71, 0x38, 0x71), RGB(0x71, 0x38, 0x61), RGB(0x71, 0x38, 0x55), RGB(0x71, 0x38, 0x45),
    RGB(0x71, 0x38, 0x38), RGB(0x71, 0x45, 0x38), RGB(0x71, 0x55, 0x38), RGB(0x71, 0x61, 0x38),
    RGB(0x71, 0x71, 0x38), RGB(0x61, 0x71, 0x38), RGB(0x55, 0x71, 0x38), RGB(0x45, 0x71, 0x38),
    RGB(0x38, 0x71, 0x38), RGB(0x38, 0x71, 0x45), RGB(0x38, 0x71, 0x55), RGB(0x38, 0x71, 0x61),
    RGB(0x38, 0x71, 0x71), RGB(0x38, 0x61, 0x71), RGB(0x38, 0x55, 0x71), RGB(0x38, 0x45, 0x71),
    RGB(0x51, 0x51, 0x71), RGB(0x59, 0x51, 0x71), RGB(0x61, 0x51, 0x71), RGB(0x69, 0x51, 0x71),
    RGB(0x71, 0x51, 0x71), RGB(0x71, 0x51, 0x69), RGB(0x71, 0x51, 0x61), RGB(0x71, 0x51, 0x59),
    RGB(0x71, 0x51, 0x51), RGB(0x71, 0x59, 0x51), RGB(0x71, 0x61, 0x51), RGB(0x71, 0x69, 0x51),
    RGB(0x71, 0x71, 0x51), RGB(0x69, 0x71, 0x51), RGB(0x61, 0x71, 0x51), RGB(0x59, 0x71, 0x51),
    RGB(0x51, 0x71, 0x51), RGB(0x51, 0x71, 0x59), RGB(0x51, 0x71, 0x61), RGB(0x51, 0x71, 0x69),
    RGB(0x51, 0x71, 0x71), RGB(0x51, 0x69, 0x71), RGB(0x51, 0x61, 0x71), RGB(0x51, 0x59, 0x71),
    RGB(0x00, 0x00, 0x41), RGB(0x10, 0x00, 0x41), RGB(0x20, 0x00, 0x41), RGB(0x30, 0x00, 0x41),
    RGB(0x41, 0x00, 0x41), RGB(0x41, 0x00, 0x30), RGB(0x41, 0x00, 0x20), RGB(0x41, 0x00, 0x10),
    RGB(0x41, 0x00, 0x00), RGB(0x41, 0x10, 0x00), RGB(0x41, 0x20, 0x00), RGB(0x41, 0x30, 0x00),
    RGB(0x41, 0x41, 0x00), RGB(0x30, 0x41, 0x00), RGB(0x20, 0x41, 0x00), RGB(0x10, 0x41, 0x00),

    RGB(0x00, 0x41, 0x00), RGB(0x00, 0x41, 0x10), RGB(0x00, 0x41, 0x20), RGB(0x00, 0x41, 0x30),
    RGB(0x00, 0x41, 0x41), RGB(0x00, 0x30, 0x41), RGB(0x00, 0x20, 0x41), RGB(0x00, 0x10, 0x41),
    RGB(0x20, 0x20, 0x41), RGB(0x28, 0x20, 0x41), RGB(0x30, 0x20, 0x41), RGB(0x38, 0x20, 0x41),
    RGB(0x41, 0x20, 0x41), RGB(0x41, 0x20, 0x38), RGB(0x41, 0x20, 0x30), RGB(0x41, 0x20, 0x28),
    RGB(0x41, 0x20, 0x20), RGB(0x41, 0x28, 0x20), RGB(0x41, 0x30, 0x20), RGB(0x41, 0x38, 0x20),
    RGB(0x41, 0x41, 0x20), RGB(0x38, 0x41, 0x20), RGB(0x30, 0x41, 0x20), RGB(0x28, 0x41, 0x20),
    RGB(0x20, 0x41, 0x20), RGB(0x20, 0x41, 0x28), RGB(0x20, 0x41, 0x30), RGB(0x20, 0x41, 0x38),
    RGB(0x20, 0x41, 0x41), RGB(0x20, 0x38, 0x41), RGB(0x20, 0x30, 0x41), RGB(0x20, 0x28, 0x41),
    RGB(0x2C, 0x2C, 0x41), RGB(0x30, 0x2C, 0x41), RGB(0x34, 0x2C, 0x41), RGB(0x3C, 0x2C, 0x41),
    RGB(0x41, 0x2C, 0x41), RGB(0x41, 0x2C, 0x3C), RGB(0x41, 0x2C, 0x34), RGB(0x41, 0x2C, 0x30),
    RGB(0x41, 0x2C, 0x2C), RGB(0x41, 0x30, 0x2C), RGB(0x41, 0x34, 0x2C), RGB(0x41, 0x3C, 0x2C),
    RGB(0x41, 0x41, 0x2C), RGB(0x3C, 0x41, 0x2C), RGB(0x34, 0x41, 0x2C), RGB(0x30, 0x41, 0x2C),
    RGB(0x2C, 0x41, 0x2C), RGB(0x2C, 0x41, 0x30), RGB(0x2C, 0x41, 0x34), RGB(0x2C, 0x41, 0x3C),
    RGB(0x2C, 0x41, 0x41), RGB(0x2C, 0x3C, 0x41), RGB(0x2C, 0x34, 0x41), RGB(0x2C, 0x30, 0x41),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00)
};

#endif

/*
 * Default 16-color palette for foreground and background
 * (corresponding flags in comments).
 * Taken from subsystems/win32/winsrv/consrv/frontends/gui/conwnd.c
 */
static const COLORREF ConsoleColors[16] =
{
    RGB(0, 0, 0),       // (Black)
    RGB(0, 0, 128),     // BLUE
    RGB(0, 128, 0),     // GREEN
    RGB(0, 128, 128),   // BLUE  | GREEN
    RGB(128, 0, 0),     // RED
    RGB(128, 0, 128),   // BLUE  | RED
    RGB(128, 128, 0),   // GREEN | RED
    RGB(192, 192, 192), // BLUE  | GREEN | RED

    RGB(128, 128, 128), // (Grey)  INTENSITY
    RGB(0, 0, 255),     // BLUE  | INTENSITY
    RGB(0, 255, 0),     // GREEN | INTENSITY
    RGB(0, 255, 255),   // BLUE  | GREEN | INTENSITY
    RGB(255, 0, 0),     // RED   | INTENSITY
    RGB(255, 0, 255),   // BLUE  | RED   | INTENSITY
    RGB(255, 255, 0),   // GREEN | RED   | INTENSITY
    RGB(255, 255, 255)  // BLUE  | GREEN | RED | INTENSITY
};

/// ConsoleFramebuffer
static PVOID ActiveFramebuffer = NULL; // Active framebuffer, points to
                                       // either TextFramebuffer or a
                                       // valid graphics framebuffer.
static HPALETTE TextPaletteHandle = NULL;
static HPALETTE PaletteHandle = NULL;

/*
 * Text mode -- we always keep a valid text mode framebuffer
 * even if we are in graphics mode. This is needed in order
 * to keep a consistent VGA state. However, each time the VGA
 * detaches from the console (and reattaches to it later on),
 * this text mode framebuffer is recreated.
 */
static PCHAR_CELL TextFramebuffer = NULL;

/*
 * Graphics mode
 */
static PBYTE GraphicsFramebuffer = NULL;


// static HANDLE ConsoleMutex = NULL;
// /* DoubleVision support */
// static BOOLEAN DoubleWidth  = FALSE;
// static BOOLEAN DoubleHeight = FALSE;





/*
 * VGA Hardware
 */
static BYTE VgaMemory[VGA_NUM_BANKS * SVGA_BANK_SIZE];

static BYTE VgaLatchRegisters[VGA_NUM_BANKS] = {0, 0, 0, 0};

static BYTE VgaMiscRegister;
static BYTE VgaFeatureRegister;

static BYTE VgaSeqIndex = VGA_SEQ_RESET_REG;
static BYTE VgaSeqRegisters[SVGA_SEQ_MAX_REG];

static BYTE VgaCrtcIndex = VGA_CRTC_HORZ_TOTAL_REG;
static BYTE VgaCrtcRegisters[SVGA_CRTC_MAX_REG];

static BYTE VgaGcIndex = VGA_GC_RESET_REG;
static BYTE VgaGcRegisters[SVGA_GC_MAX_REG];

static BOOLEAN VgaAcLatch = FALSE;
static BOOLEAN VgaAcPalDisable = TRUE;
static BYTE VgaAcIndex = VGA_AC_PAL_0_REG;
static BYTE VgaAcRegisters[VGA_AC_MAX_REG];

static BYTE VgaDacMask  = 0xFF;
static BYTE VgaDacLatchCounter = 0;
static BYTE VgaDacLatch[3];

static BOOLEAN VgaDacReadWrite = FALSE;
static WORD VgaDacIndex = 0;
static BYTE VgaDacRegisters[VGA_PALETTE_SIZE];

// static VGA_REGISTERS VgaRegisters;

static ULONGLONG HorizontalRetraceCycle = 0ULL;
static PHARDWARE_TIMER HSyncTimer;
static DWORD ScanlineCounter = 0;
static DWORD StartAddressLatch = 0;
static DWORD ScanlineSizeLatch = 0;

static BOOLEAN NeedsUpdate = FALSE;
static BOOLEAN ModeChanged = FALSE;
static BOOLEAN CursorChanged  = FALSE;
static BOOLEAN PaletteChanged = FALSE;

static UINT SvgaHdrCounter = 0;
static BYTE SvgaHiddenRegister = 0;

typedef enum _SCREEN_MODE
{
    TEXT_MODE,
    GRAPHICS_MODE
} SCREEN_MODE, *PSCREEN_MODE;

static SCREEN_MODE ScreenMode = TEXT_MODE;
static COORD CurrResolution   = {0};

static SMALL_RECT UpdateRectangle = { 0, 0, 0, 0 };




/** HACK!! **/
#include "../../console/video.c"
/** HACK!! **/





/* PRIVATE FUNCTIONS **********************************************************/

static inline DWORD VgaGetVideoBaseAddress(VOID)
{
    return MemoryBase[(VgaGcRegisters[VGA_GC_MISC_REG] >> 2) & 0x03];
}

static inline DWORD VgaGetAddressSize(VOID)
{
    if (VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES)
    {
        /* Packed pixel addressing */
        return 1;
    }
    else if (VgaCrtcRegisters[VGA_CRTC_UNDERLINE_REG] & VGA_CRTC_UNDERLINE_DWORD)
    {
        /* Double-word addressing */
        return 4; // sizeof(DWORD)
    }
    else if (VgaCrtcRegisters[VGA_CRTC_MODE_CONTROL_REG] & VGA_CRTC_MODE_CONTROL_BYTE)
    {
        /* Byte addressing */
        return 1; // sizeof(BYTE)
    }
    else
    {
        /* Word addressing */
        return 2; // sizeof(WORD)
    }
}

static inline DWORD VgaTranslateAddress(DWORD Address)
{
    DWORD Offset = LOWORD(Address - VgaGetVideoBaseAddress());
    DWORD ExtOffset = ((VgaGcRegisters[SVGA_GC_EXT_MODE_REG] & SVGA_GC_EXT_MODE_WND_B) && (Offset & (1 << 15)))
                      ? VgaGcRegisters[SVGA_GC_OFFSET_1_REG]
                      : VgaGcRegisters[SVGA_GC_OFFSET_0_REG];

    /* Check for chain-4 and odd-even mode */
    if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
    {
        /* Clear the lowest two bits since they're used to select the bank */
        Offset &= ~3;
    }
    else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
    {
        /* Clear the lowest bit since it's used to select odd/even */
        Offset &= ~1;
    }

    if (ExtOffset)
    {
        /* Add the extended offset */
        Offset += ExtOffset << ((VgaGcRegisters[SVGA_GC_EXT_MODE_REG] & SVGA_GC_EXT_MODE_GRAN) ? 14 : 12);
    }

    /* Return the offset on plane 0 */
    return Offset;
}

static inline BYTE VgaTranslateByteForWriting(BYTE Data, BYTE Plane)
{
    BYTE WriteMode = VgaGcRegisters[VGA_GC_MODE_REG] & 0x03;
    BYTE BitMask = VgaGcRegisters[VGA_GC_BITMASK_REG];

    if (WriteMode == 1)
    {
        /* In write mode 1 just return the latch register */
        return VgaLatchRegisters[Plane];
    }

    if (WriteMode != 2)
    {
        /* Write modes 0 and 3 rotate the data to the right first */
        BYTE RotateCount = VgaGcRegisters[VGA_GC_ROTATE_REG] & 0x07;
        Data = LOBYTE(((DWORD)Data >> RotateCount) | ((DWORD)Data << (8 - RotateCount)));
    }
    else
    {
        /* Write mode 2 expands the appropriate bit to all 8 bits */
        Data = (Data & (1 << Plane)) ? 0xFF : 0x00;
    }

    if (WriteMode == 0)
    {
        /*
         * In write mode 0, the enable set/reset register decides if the
         * set/reset bit should be expanded to all 8 bits.
         */
        if (VgaGcRegisters[VGA_GC_ENABLE_RESET_REG] & (1 << Plane))
        {
            /* Copy the bit from the set/reset register to all 8 bits */
            Data = (VgaGcRegisters[VGA_GC_RESET_REG] & (1 << Plane)) ? 0xFF : 0x00;
        }
    }

    if (WriteMode != 3)
    {
        /* Write modes 0 and 2 then perform a logical operation on the data and latch */
        BYTE LogicalOperation = (VgaGcRegisters[VGA_GC_ROTATE_REG] >> 3) & 0x03;

        if (LogicalOperation == 1) Data &= VgaLatchRegisters[Plane];
        else if (LogicalOperation == 2) Data |= VgaLatchRegisters[Plane];
        else if (LogicalOperation == 3) Data ^= VgaLatchRegisters[Plane];
    }
    else
    {
        /* For write mode 3, we AND the bitmask with the data, which is used as the new bitmask */
        BitMask &= Data;

        /* Then we expand the bit in the set/reset field */
        Data = (VgaGcRegisters[VGA_GC_RESET_REG] & (1 << Plane)) ? 0xFF : 0x00;
    }

    /* Bits cleared in the bitmask are replaced with latch register bits */
    Data = (Data & BitMask) | (VgaLatchRegisters[Plane] & (~BitMask));

    /* Return the byte */
    return Data;
}

static inline ULONG VgaGetClockFrequency(VOID)
{
    BYTE Numerator, Denominator;

    if (VgaSeqRegisters[SVGA_SEQ_MCLK_REG] & SVGA_SEQ_MCLK_VCLK)
    {
        /* The VCLK is being generated using the MCLK */
        ULONG Clock = (VGA_CLOCK_BASE * (VgaSeqRegisters[SVGA_SEQ_MCLK_REG] & 0x3F)) >> 3;

        if (VgaSeqRegisters[SVGA_SEQ_VCLK3_DENOMINATOR_REG] & 1)
        {
            /* Use only half of the MCLK as the VCLK */
            Clock >>= 1;
        }

        return Clock;
    }

    switch ((VgaMiscRegister >> 2) & 3)
    {
        case 0:
        {
            Numerator = VgaSeqRegisters[SVGA_SEQ_VCLK0_NUMERATOR_REG];
            Denominator = VgaSeqRegisters[SVGA_SEQ_VCLK0_DENOMINATOR_REG];
            break;
        }

        case 1:
        {
            Numerator = VgaSeqRegisters[SVGA_SEQ_VCLK1_NUMERATOR_REG];
            Denominator = VgaSeqRegisters[SVGA_SEQ_VCLK1_DENOMINATOR_REG];
            break;
        }

        case 2:
        {
            Numerator = VgaSeqRegisters[SVGA_SEQ_VCLK2_NUMERATOR_REG];
            Denominator = VgaSeqRegisters[SVGA_SEQ_VCLK2_DENOMINATOR_REG];
            break;
        }

        case 3:
        {
            Numerator = VgaSeqRegisters[SVGA_SEQ_VCLK3_NUMERATOR_REG];
            Denominator = VgaSeqRegisters[SVGA_SEQ_VCLK3_DENOMINATOR_REG];
            break;
        }
    }

    /* The numerator is 7-bit */
    Numerator &= ~(1 << 7);

    /* If bit 7 is clear, the denominator is 5-bit */
    if (!(Denominator & (1 << 7))) Denominator &= ~(1 << 6);

    /* Bit 0 of the denominator is the post-scalar bit */
    if (Denominator & 1) Denominator &= ~1;
    else Denominator >>= 1;

    /* Return the clock frequency in Hz */
    return (VGA_CLOCK_BASE * Numerator) / Denominator;
}

static VOID VgaResetSequencer(VOID)
{
    /* Lock extended SVGA registers */
    VgaSeqRegisters[SVGA_SEQ_UNLOCK_REG] = SVGA_SEQ_LOCKED;

    /* Initialize the VCLKs */
    VgaSeqRegisters[SVGA_SEQ_VCLK0_NUMERATOR_REG]   = 0x66;
    VgaSeqRegisters[SVGA_SEQ_VCLK0_DENOMINATOR_REG] = 0x3B;
    VgaSeqRegisters[SVGA_SEQ_VCLK1_NUMERATOR_REG]   = 0x5B;
    VgaSeqRegisters[SVGA_SEQ_VCLK1_DENOMINATOR_REG] = 0x2F;
    VgaSeqRegisters[SVGA_SEQ_VCLK2_NUMERATOR_REG]   = 0x45;
    VgaSeqRegisters[SVGA_SEQ_VCLK2_DENOMINATOR_REG] = 0x30;
    VgaSeqRegisters[SVGA_SEQ_VCLK3_NUMERATOR_REG]   = 0x7E;
    VgaSeqRegisters[SVGA_SEQ_VCLK3_DENOMINATOR_REG] = 0x33;

    /* 50 MHz MCLK, not being used as the VCLK */
    VgaSeqRegisters[SVGA_SEQ_MCLK_REG] = 0x1C;
}

static VOID VgaRestoreDefaultPalette(PPALETTEENTRY Entries, USHORT NumOfEntries)
{
    USHORT i;

    /* Copy the colors of the default palette to the DAC and console palette */
    for (i = 0; i < NumOfEntries; i++)
    {
        /* Set the palette entries */
        Entries[i].peRed   = GetRValue(VgaDefaultPalette[i]);
        Entries[i].peGreen = GetGValue(VgaDefaultPalette[i]);
        Entries[i].peBlue  = GetBValue(VgaDefaultPalette[i]);
        Entries[i].peFlags = 0;

        /* Set the DAC registers */
        VgaDacRegisters[i * 3]     = VGA_COLOR_TO_DAC(GetRValue(VgaDefaultPalette[i]));
        VgaDacRegisters[i * 3 + 1] = VGA_COLOR_TO_DAC(GetGValue(VgaDefaultPalette[i]));
        VgaDacRegisters[i * 3 + 2] = VGA_COLOR_TO_DAC(GetBValue(VgaDefaultPalette[i]));
    }
}

static BOOLEAN VgaInitializePalette(VOID)
{
    UINT i;
    BOOLEAN Result = FALSE;
    LPLOGPALETTE Palette, TextPalette;

    /* Allocate storage space for the palettes */
    Palette = RtlAllocateHeap(RtlGetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              sizeof(LOGPALETTE) +
                                  VGA_MAX_COLORS * sizeof(PALETTEENTRY));
    TextPalette = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(LOGPALETTE) +
                                      (VGA_AC_PAL_F_REG + 1) * sizeof(PALETTEENTRY));
    if ((Palette == NULL) || (TextPalette == NULL)) goto Cleanup;

    /* Initialize the palettes */
    Palette->palVersion = TextPalette->palVersion = 0x0300;
    Palette->palNumEntries = VGA_MAX_COLORS;
    TextPalette->palNumEntries = VGA_AC_PAL_F_REG + 1;

    /* Restore the default graphics palette */
    VgaRestoreDefaultPalette(Palette->palPalEntry, Palette->palNumEntries);

    /* Set the default text palette */
    for (i = 0; i < TextPalette->palNumEntries; i++)
    {
        /* Set the palette entries */
        TextPalette->palPalEntry[i].peRed   = GetRValue(ConsoleColors[i]);
        TextPalette->palPalEntry[i].peGreen = GetGValue(ConsoleColors[i]);
        TextPalette->palPalEntry[i].peBlue  = GetBValue(ConsoleColors[i]);
        TextPalette->palPalEntry[i].peFlags = 0;
    }

    /* Create the palettes */
    PaletteHandle = CreatePalette(Palette);
    TextPaletteHandle = CreatePalette(TextPalette);

    if (PaletteHandle != NULL && TextPaletteHandle != NULL)
    {
        /* The palettes have been created successfully */
        Result = TRUE;
    }

Cleanup:
    /* Free the palettes */
    if (Palette) RtlFreeHeap(RtlGetProcessHeap(), 0, Palette);
    if (TextPalette) RtlFreeHeap(RtlGetProcessHeap(), 0, TextPalette);

    if (!Result)
    {
        /* Something failed, delete the palettes */
        if (PaletteHandle) DeleteObject(PaletteHandle);
        if (TextPaletteHandle) DeleteObject(TextPaletteHandle);
    }

    return Result;
}

static VOID VgaResetPalette(VOID)
{
    PALETTEENTRY Entries[VGA_MAX_COLORS];

    /* Restore the default palette */
    VgaRestoreDefaultPalette(Entries, VGA_MAX_COLORS);
    SetPaletteEntries(PaletteHandle, 0, VGA_MAX_COLORS, Entries);
    PaletteChanged = TRUE;
}

static BOOL VgaEnterNewMode(SCREEN_MODE NewScreenMode, PCOORD Resolution)
{
    /* Check if the new mode is alphanumeric */
    if (NewScreenMode == TEXT_MODE)
    {
        /* Enter new text mode */

        if (!VgaConsoleCreateTextScreen(// &TextFramebuffer,
                                        Resolution,
                                        TextPaletteHandle))
        {
            DisplayMessage(L"An unexpected VGA error occurred while switching into text mode. Error: %u", GetLastError());
            EmulatorTerminate();
            return FALSE;
        }

        /* The active framebuffer is now the text framebuffer */
        ActiveFramebuffer = TextFramebuffer;

        /* Set the screen mode flag */
        ScreenMode = TEXT_MODE;

        return TRUE;
    }
    else
    {
        /* Enter graphics mode */

        if (!VgaConsoleCreateGraphicsScreen(// &GraphicsFramebuffer,
                                            Resolution,
                                            PaletteHandle))
        {
            DisplayMessage(L"An unexpected VGA error occurred while switching into graphics mode. Error: %u", GetLastError());
            EmulatorTerminate();
            return FALSE;
        }

        /* The active framebuffer is now the graphics framebuffer */
        ActiveFramebuffer = GraphicsFramebuffer;

        /* Set the screen mode flag */
        ScreenMode = GRAPHICS_MODE;

        return TRUE;
    }
}

static VOID VgaLeaveCurrentMode(VOID)
{
    /* Leave the current video mode */
    if (ScreenMode == GRAPHICS_MODE)
    {
        VgaConsoleDestroyGraphicsScreen();

        /* Cleanup the video data */
        GraphicsFramebuffer = NULL;
    }
    else
    {
        VgaConsoleDestroyTextScreen();

        /* Cleanup the video data */
        // TextFramebuffer = NULL;
        // NEVER SET the ALWAYS-SET TextFramebuffer pointer to NULL!!
    }

    /* Reset the active framebuffer */
    ActiveFramebuffer = NULL;
}

static VOID VgaChangeMode(VOID)
{
    COORD NewResolution = VgaGetDisplayResolution();
    SCREEN_MODE NewScreenMode =
        !(VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA) ? TEXT_MODE
                                                                 : GRAPHICS_MODE;

    /*
     * Do not switch to a different screen mode + resolution if the new settings
     * are the same as the old ones. Just repaint the full screen.
     */
    if ((ScreenMode == NewScreenMode) && // CurrResolution == NewResolution
        (CurrResolution.X == NewResolution.X && CurrResolution.Y == NewResolution.Y))
    {
        goto Quit;
    }

    // FIXME: Wouldn't it be preferrable to switch to the new console SB
    // *ONLY* if we succeeded in setting the new mode??

    /* Leave the current video mode */
    VgaLeaveCurrentMode(); // ScreenMode

    /* Update the current resolution */
    CurrResolution = NewResolution;

    /* Change the screen mode */
    if (!VgaEnterNewMode(NewScreenMode, &CurrResolution))
        return;

Quit:

    /* Trigger a full update of the screen */
    NeedsUpdate = TRUE;
    UpdateRectangle.Left = 0;
    UpdateRectangle.Top  = 0;
    UpdateRectangle.Right  = CurrResolution.X;
    UpdateRectangle.Bottom = CurrResolution.Y;

    /* Reset the mode change flag */
    ModeChanged = FALSE;
}

static inline VOID VgaMarkForUpdate(SHORT Row, SHORT Column)
{
    /* Check if this is the first time the rectangle is updated */
    if (!NeedsUpdate)
    {
        UpdateRectangle.Left = UpdateRectangle.Top = MAXSHORT;
        UpdateRectangle.Right = UpdateRectangle.Bottom = MINSHORT;
    }

    /* Expand the rectangle to include the point */
    UpdateRectangle.Left = min(UpdateRectangle.Left, Column);
    UpdateRectangle.Right = max(UpdateRectangle.Right, Column);
    UpdateRectangle.Top = min(UpdateRectangle.Top, Row);
    UpdateRectangle.Bottom = max(UpdateRectangle.Bottom, Row);

    /* Set the update request flag */
    NeedsUpdate = TRUE;
}

static VOID VgaUpdateFramebuffer(VOID)
{
    SHORT i, j, k;
    DWORD AddressSize = VgaGetAddressSize();
    DWORD Address = StartAddressLatch;
    BYTE BytePanning = (VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] >> 5) & 3;
    WORD LineCompare = VgaCrtcRegisters[VGA_CRTC_LINE_COMPARE_REG]
                       | ((VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_LC8) << 4)
                       | ((VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & VGA_CRTC_MAXSCANLINE_LC9) << 3);
    BYTE PixelShift = VgaAcRegisters[VGA_AC_HORZ_PANNING_REG] & 0x0F;

    /*
     * If the console framebuffer is NULL, that means something
     * went wrong earlier and this is the final display refresh.
     */
    if (ActiveFramebuffer == NULL) return;

    /* Check if we are in text or graphics mode */
    if (ScreenMode == GRAPHICS_MODE)
    {
        /* Graphics mode */
        PBYTE GraphicsBuffer = (PBYTE)ActiveFramebuffer;
        DWORD InterlaceHighBit = VGA_INTERLACE_HIGH_BIT;
        SHORT X;

        /*
         * Synchronize access to the graphics framebuffer
         * with the console framebuffer mutex.
         */
        WaitForSingleObject(ConsoleMutex, INFINITE);

        /* Shift the high bit right by 1 in odd/even mode */
        if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
        {
            InterlaceHighBit >>= 1;
        }

        if (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & VGA_CRTC_MAXSCANLINE_DOUBLE)
        {
            /* Halve the line compare value */
            LineCompare >>= 1;
        }
        else
        {
            /* Divide the line compare value by the maximum scan line */
            LineCompare /= 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);
        }

        /* Loop through the scanlines */
        for (i = 0; i < CurrResolution.Y; i++)
        {
            if (i == LineCompare)
            {
                if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_PPM)
                {
                    /*
                     * Disable the pixel shift count and byte panning
                     * for the rest of the display cycle
                     */
                    PixelShift = 0;
                    BytePanning = 0;
                }

                /* Reset the address, but assume the preset row scan is 0 */
                Address = BytePanning;
            }

            if ((VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_OE) && (i & 1))
            {
                /* Odd-numbered line in interlaced mode - set the high bit */
                Address |= InterlaceHighBit;
            }

            /* Loop through the pixels */
            for (j = 0; j < CurrResolution.X; j++)
            {
                BYTE PixelData = 0;

                /* Apply horizontal pixel panning */
                if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                {
                    X = j + ((PixelShift >> 1) & 0x03);
                }
                else
                {
                    X = j + ((PixelShift < 8) ? PixelShift : -1);
                }

                if (VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES)
                {
                    // TODO: Check for high color modes

                    /* 256 color mode */
                    PixelData = VgaMemory[Address + X];
                }
                else
                {
                    /* Check the shifting mode */
                    if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_SHIFT256)
                    {
                        /* 4 bits shifted from each plane */

                        /* Check if this is 16 or 256 color mode */
                        if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                        {
                            /* One byte per pixel */
                            PixelData = VgaMemory[WRAP_OFFSET((Address + (X / VGA_NUM_BANKS)) * AddressSize)
                                                  * VGA_NUM_BANKS + (X % VGA_NUM_BANKS)];
                        }
                        else
                        {
                            /* 4-bits per pixel */

                            PixelData = VgaMemory[WRAP_OFFSET((Address + (X / (VGA_NUM_BANKS * 2))) * AddressSize)
                                                  * VGA_NUM_BANKS + ((X / 2) % VGA_NUM_BANKS)];

                            /* Check if we should use the highest 4 bits or lowest 4 */
                            if ((X % 2) == 0)
                            {
                                /* Highest 4 */
                                PixelData >>= 4;
                            }
                            else
                            {
                                /* Lowest 4 */
                                PixelData &= 0x0F;
                            }
                        }
                    }
                    else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_SHIFTREG)
                    {
                        /* Check if this is 16 or 256 color mode */
                        if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                        {
                            // TODO: NOT IMPLEMENTED
                            DPRINT1("8-bit interleaved mode is not implemented!\n");
                        }
                        else
                        {
                            /*
                             * 2 bits shifted from plane 0 and 2 for the first 4 pixels,
                             * then 2 bits shifted from plane 1 and 3 for the next 4
                             */
                            DWORD BankNumber = (X / 4) % 2;
                            DWORD Offset = Address + (X / 8);
                            BYTE LowPlaneData = VgaMemory[WRAP_OFFSET(Offset * AddressSize) * VGA_NUM_BANKS + BankNumber];
                            BYTE HighPlaneData = VgaMemory[WRAP_OFFSET(Offset * AddressSize) * VGA_NUM_BANKS + (BankNumber + 2)];

                            /* Extract the two bits from each plane */
                            LowPlaneData  = (LowPlaneData  >> (6 - ((X % 4) * 2))) & 0x03;
                            HighPlaneData = (HighPlaneData >> (6 - ((X % 4) * 2))) & 0x03;

                            /* Combine them into the pixel */
                            PixelData = LowPlaneData | (HighPlaneData << 2);
                        }
                    }
                    else
                    {
                        /* 1 bit shifted from each plane */

                        /* Check if this is 16 or 256 color mode */
                        if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                        {
                            /* 8 bits per pixel, 2 on each plane */

                            for (k = 0; k < VGA_NUM_BANKS; k++)
                            {
                                /* The data is on plane k, 4 pixels per byte */
                                BYTE PlaneData = VgaMemory[WRAP_OFFSET((Address + (X >> 2)) * AddressSize) * VGA_NUM_BANKS + k];

                                /* The mask of the first bit in the pair */
                                BYTE BitMask = 1 << (((3 - (X % VGA_NUM_BANKS)) * 2) + 1);

                                /* Bits 0, 1, 2 and 3 come from the first bit of the pair */
                                if (PlaneData & BitMask) PixelData |= 1 << k;

                                /* Bits 4, 5, 6 and 7 come from the second bit of the pair */
                                if (PlaneData & (BitMask >> 1)) PixelData |= 1 << (k + 4);
                            }
                        }
                        else
                        {
                            /* 4 bits per pixel, 1 on each plane */

                            for (k = 0; k < VGA_NUM_BANKS; k++)
                            {
                                BYTE PlaneData = VgaMemory[WRAP_OFFSET((Address + (X >> 3)) * AddressSize) * VGA_NUM_BANKS + k];

                                /* If the bit on that plane is set, set it */
                                if (PlaneData & (1 << (7 - (X % 8)))) PixelData |= 1 << k;
                            }
                        }
                    }
                }

                if (!(VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT))
                {
                    /*
                     * In 16 color mode, the value is an index to the AC registers
                     * if external palette access is disabled, otherwise (in case
                     * of palette loading) it is a blank pixel.
                     */

                    if (VgaAcPalDisable)
                    {
                        if (!(VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_P54S))
                        {
                            /* Bits 4 and 5 are taken from the palette register */
                            PixelData = ((VgaAcRegisters[VGA_AC_COLOR_SEL_REG] << 4) & 0xC0)
                                        | (VgaAcRegisters[PixelData & 0x0F] & 0x3F);
                        }
                        else
                        {
                            /* Bits 4 and 5 are taken from the color select register */
                            PixelData = (VgaAcRegisters[VGA_AC_COLOR_SEL_REG] << 4)
                                        | (VgaAcRegisters[PixelData & 0x0F] & 0x0F);
                        }
                    }
                    else
                    {
                        PixelData = 0;
                    }
                }

                /* Take into account DoubleVision mode when checking for pixel updates */
                if (DoubleWidth && DoubleHeight)
                {
                    /* Now check if the resulting pixel data has changed */
                    if (GraphicsBuffer[(i * 2 * CurrResolution.X * 2) + (j * 2)] != PixelData)
                    {
                        /* Yes, write the new value */
                        GraphicsBuffer[(i * 2 * CurrResolution.X * 2) + (j * 2)] = PixelData;
                        GraphicsBuffer[(i * 2 * CurrResolution.X * 2) + (j * 2 + 1)] = PixelData;
                        GraphicsBuffer[((i * 2 + 1) * CurrResolution.X * 2) + (j * 2)] = PixelData;
                        GraphicsBuffer[((i * 2 + 1) * CurrResolution.X * 2) + (j * 2 + 1)] = PixelData;

                        /* Mark the specified pixel as changed */
                        VgaMarkForUpdate(i, j);
                    }
                }
                else if (DoubleWidth && !DoubleHeight)
                {
                    /* Now check if the resulting pixel data has changed */
                    if (GraphicsBuffer[(i * CurrResolution.X * 2) + (j * 2)] != PixelData)
                    {
                        /* Yes, write the new value */
                        GraphicsBuffer[(i * CurrResolution.X * 2) + (j * 2)] = PixelData;
                        GraphicsBuffer[(i * CurrResolution.X * 2) + (j * 2 + 1)] = PixelData;

                        /* Mark the specified pixel as changed */
                        VgaMarkForUpdate(i, j);
                    }
                }
                else if (!DoubleWidth && DoubleHeight)
                {
                    /* Now check if the resulting pixel data has changed */
                    if (GraphicsBuffer[(i * 2 * CurrResolution.X) + j] != PixelData)
                    {
                        /* Yes, write the new value */
                        GraphicsBuffer[(i * 2 * CurrResolution.X) + j] = PixelData;
                        GraphicsBuffer[((i * 2 + 1) * CurrResolution.X) + j] = PixelData;

                        /* Mark the specified pixel as changed */
                        VgaMarkForUpdate(i, j);
                    }
                }
                else // if (!DoubleWidth && !DoubleHeight)
                {
                    /* Now check if the resulting pixel data has changed */
                    if (GraphicsBuffer[i * CurrResolution.X + j] != PixelData)
                    {
                        /* Yes, write the new value */
                        GraphicsBuffer[i * CurrResolution.X + j] = PixelData;

                        /* Mark the specified pixel as changed */
                        VgaMarkForUpdate(i, j);
                    }
                }
            }

            if ((VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_OE) && (i & 1))
            {
                /* Clear the high bit */
                Address &= ~InterlaceHighBit;
            }

            if (!(VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_OE) || (i & 1))
            {
                /* Move to the next scanline */
                Address += ScanlineSizeLatch;
            }
        }

        /*
         * Release the console framebuffer mutex
         * so that we allow for repainting.
         */
        ReleaseMutex(ConsoleMutex);
    }
    else
    {
        /* Text mode */
        DWORD CurrentAddr;
        PCHAR_CELL CharBuffer = (PCHAR_CELL)ActiveFramebuffer;
        CHAR_CELL CharInfo;

        /*
         * Technically, the horizontal panning and preset row count should
         * affect text mode too. However, it works on pixels and not characters,
         * so we can't support it currently.
         */

        /* Loop through the scanlines */
        for (i = 0; i < CurrResolution.Y; i++)
        {
            /* Loop through the characters */
            for (j = 0; j < CurrResolution.X; j++)
            {
                CurrentAddr = WRAP_OFFSET((Address + j) * AddressSize);

                /* Plane 0 holds the character itself */
                CharInfo.Char = VgaMemory[CurrentAddr * VGA_NUM_BANKS];

                /* Plane 1 holds the attribute */
                CharInfo.Attributes = VgaMemory[CurrentAddr * VGA_NUM_BANKS + 1];

                /* Now check if the resulting character data has changed */
                if ((CharBuffer[i * CurrResolution.X + j].Char != CharInfo.Char) ||
                    (CharBuffer[i * CurrResolution.X + j].Attributes != CharInfo.Attributes))
                {
                    /* Yes, write the new value */
                    CharBuffer[i * CurrResolution.X + j] = CharInfo;

                    /* Mark the specified cell as changed */
                    VgaMarkForUpdate(i, j);
                }
            }

            /* Move to the next scanline */
            Address += ScanlineSizeLatch;
        }
    }
}

static VOID VgaUpdateTextCursor(VOID)
{
    BOOL CursorVisible = !(VgaCrtcRegisters[VGA_CRTC_CURSOR_START_REG] & 0x20);
    BYTE CursorStart   =   VgaCrtcRegisters[VGA_CRTC_CURSOR_START_REG] & 0x1F;
    BYTE CursorEnd     =   VgaCrtcRegisters[VGA_CRTC_CURSOR_END_REG]   & 0x1F;

    DWORD ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;
    BYTE TextSize = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);
    WORD Location = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_LOW_REG],
                             VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_HIGH_REG]);

    /* Just return if we are not in text mode */
    if (ScreenMode != TEXT_MODE) return;

    /* Add the cursor skew to the location */
    Location += (VgaCrtcRegisters[VGA_CRTC_CURSOR_END_REG] >> 5) & 0x03;

    VgaConsoleUpdateTextCursor(CursorVisible, CursorStart, CursorEnd,
                               TextSize, ScanlineSize, Location);

    /* Reset the cursor changed flag */
    CursorChanged = FALSE;
}

static BYTE WINAPI VgaReadPort(USHORT Port)
{
    DPRINT("VgaReadPort: Port 0x%X\n", Port);

    if (Port != VGA_DAC_MASK) SvgaHdrCounter = 0;

    switch (Port)
    {
        case VGA_MISC_READ:
            return VgaMiscRegister;

        case VGA_INSTAT0_READ:
            return 0; // Not implemented

        case VGA_INSTAT1_READ_MONO:
        case VGA_INSTAT1_READ_COLOR:
        {
            BYTE Result = 0;
            BOOLEAN Vsync, Hsync;
            ULONGLONG Cycles = CurrentCycleCount;
            ULONG CyclesPerMicrosecond = (ULONG)((CurrentIps + 500000ULL) / 1000000ULL);
            ULONG Dots = (VgaSeqRegisters[VGA_SEQ_CLOCK_REG] & 1) ? 9 : 8;
            ULONG Clock = VgaGetClockFrequency() / 1000000;
            ULONG HblankStart, HblankEnd;
            ULONG HblankDuration;
            ULONG VerticalRetraceStart = VgaCrtcRegisters[VGA_CRTC_START_VERT_RETRACE_REG];
            ULONG VerticalRetraceEnd;

            VerticalRetraceStart |= (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VRS8) << 6;
            VerticalRetraceStart |= (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VRS9) << 2;

            VerticalRetraceEnd = VerticalRetraceStart + (VgaCrtcRegisters[VGA_CRTC_END_VERT_RETRACE_REG] & 0x0F);

            if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA)
            {
                BYTE MaximumScanLine = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);

                if (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & VGA_CRTC_MAXSCANLINE_DOUBLE)
                {
                    VerticalRetraceStart <<= 1;
                    VerticalRetraceEnd <<= 1;
                }
                else
                {
                    VerticalRetraceStart *= MaximumScanLine;
                    VerticalRetraceEnd *= MaximumScanLine;
                }
            }

            /* Calculate the horizontal blanking duration in cycles */
            HblankStart = VgaCrtcRegisters[VGA_CRTC_START_HORZ_BLANKING_REG] & 0x1F;
            HblankEnd = VgaCrtcRegisters[VGA_CRTC_END_HORZ_BLANKING_REG] & 0x1F;
            if (HblankEnd < HblankStart) HblankEnd |= 0x20;
            HblankDuration = ((HblankEnd - HblankStart) * Dots
                             * CyclesPerMicrosecond + (Clock >> 1)) / Clock;

            Vsync = ScanlineCounter >= VerticalRetraceStart && ScanlineCounter <= VerticalRetraceEnd;
            Hsync = (Cycles - HorizontalRetraceCycle) < (ULONGLONG)HblankDuration;

            /* Reset the AC latch */
            VgaAcLatch = FALSE;

            /* Set a flag if there is a vertical or horizontal retrace */
            if (Vsync || Hsync) Result |= VGA_STAT_DD;

            /* Set an additional flag if there was a vertical retrace */
            if (Vsync) Result |= VGA_STAT_VRETRACE;

            return Result;
        }

        case VGA_FEATURE_READ:
            return VgaFeatureRegister;

        case VGA_AC_INDEX:
            return VgaAcIndex | (VgaAcPalDisable ? 0x20 : 0x00);

        case VGA_AC_READ:
            return VgaAcRegisters[VgaAcIndex];

        case VGA_SEQ_INDEX:
            return VgaSeqIndex;

        case VGA_SEQ_DATA:
            return VgaSeqRegisters[VgaSeqIndex];

        case VGA_DAC_MASK:
        {
            if (SvgaHdrCounter == 4)
            {
                SvgaHdrCounter = 0;
                return SvgaHiddenRegister;
            }
            else
            {
                SvgaHdrCounter++;
                return VgaDacMask;
            }
        }

        case VGA_DAC_READ_INDEX:
            /* This returns the read/write state */
            return (VgaDacReadWrite ? 0 : 3);

        case VGA_DAC_WRITE_INDEX:
            return VgaDacIndex;

        case VGA_DAC_DATA:
        {
            /* Ignore reads in write mode */
            if (!VgaDacReadWrite)
            {
                BYTE Data = VgaDacRegisters[VgaDacIndex * 3 + VgaDacLatchCounter];
                VgaDacLatchCounter++;

                if (VgaDacLatchCounter == 3)
                {
                    /* Reset the latch counter and increment the palette index */
                    VgaDacLatchCounter = 0;
                    VgaDacIndex++;
                    VgaDacIndex %= VGA_MAX_COLORS;
                }

                return Data;
            }

            break;
        }

        case VGA_CRTC_INDEX_MONO:
        case VGA_CRTC_INDEX_COLOR:
            return VgaCrtcIndex;

        case VGA_CRTC_DATA_MONO:
        case VGA_CRTC_DATA_COLOR:
            return VgaCrtcRegisters[VgaCrtcIndex];

        case VGA_GC_INDEX:
            return VgaGcIndex;

        case VGA_GC_DATA:
            return VgaGcRegisters[VgaGcIndex];

        default:
            DPRINT1("VgaReadPort: Unknown port 0x%X\n", Port);
            break;
    }

    return 0;
}

static inline VOID VgaWriteSequencer(BYTE Data)
{
    /* Save the value */
    VgaSeqRegisters[VgaSeqIndex & VGA_SEQ_INDEX_MASK] = Data;

    /* Check the index */
    switch (VgaSeqIndex & VGA_SEQ_INDEX_MASK)
    {
        case SVGA_SEQ_UNLOCK_REG:
        {
            if ((Data & SVGA_SEQ_UNLOCK_MASK) == SVGA_SEQ_UNLOCKED)
            {
                /* Unlock SVGA extensions */
                VgaSeqRegisters[SVGA_SEQ_UNLOCK_REG] = SVGA_SEQ_UNLOCKED;
            }
            else
            {
                /* Lock SVGA extensions */
                VgaSeqRegisters[SVGA_SEQ_UNLOCK_REG] = SVGA_SEQ_LOCKED;
            }

            break;
        }
    }
}

static inline VOID VgaWriteGc(BYTE Data)
{
    /* Save the value */
    VgaGcRegisters[VgaGcIndex & VGA_GC_INDEX_MASK] = Data;

    /* Check the index */
    switch (VgaGcIndex & VGA_GC_INDEX_MASK)
    {
        case VGA_GC_MISC_REG:
        {
            /* Remove any existing VGA memory hook */
            MemRemoveFastMemoryHook((PVOID)0xA0000, 0x20000);

            if (VgaMiscRegister & VGA_MISC_RAM_ENABLED)
            {
                UCHAR MemoryMap = (VgaGcRegisters[VGA_GC_MISC_REG] >> 2) & 0x03;

                /* Register a memory hook */
                MemInstallFastMemoryHook(UlongToPtr(MemoryBase[MemoryMap]),
                                         MemorySize[MemoryMap],
                                         VgaReadMemory,
                                         VgaWriteMemory);
            }

            /* The GC misc register decides if it's text or graphics mode */
            ModeChanged = TRUE;
            break;
        }
    }
}

static inline VOID VgaWriteCrtc(BYTE Data)
{
    /* Save the value */
    VgaCrtcRegisters[VgaCrtcIndex & VGA_CRTC_INDEX_MASK] = Data;

    /* Check the index */
    switch (VgaCrtcIndex & VGA_CRTC_INDEX_MASK)
    {
        case VGA_CRTC_END_HORZ_DISP_REG:
        case VGA_CRTC_VERT_DISP_END_REG:
        case VGA_CRTC_OVERFLOW_REG:
        case VGA_CRTC_MAX_SCAN_LINE_REG:
        {
            /* The video mode has changed */
            ModeChanged = TRUE;
            break;
        }

        case VGA_CRTC_CURSOR_LOC_LOW_REG:
        case VGA_CRTC_CURSOR_LOC_HIGH_REG:
        case VGA_CRTC_CURSOR_START_REG:
        case VGA_CRTC_CURSOR_END_REG:
        {
            /* Set the cursor changed flag */
            CursorChanged = TRUE;
            break;
        }
    }
}

static inline VOID VgaWriteDac(BYTE Data)
{
    UINT i;
    PALETTEENTRY Entry;

    /* Store the value in the latch */
    VgaDacLatch[VgaDacLatchCounter++] = Data;
    if (VgaDacLatchCounter < 3) return;

    /* Reset the latch counter */
    VgaDacLatchCounter = 0;

    /* Set the DAC register values */
    VgaDacRegisters[VgaDacIndex * 3]     = VgaDacLatch[0];
    VgaDacRegisters[VgaDacIndex * 3 + 1] = VgaDacLatch[1];
    VgaDacRegisters[VgaDacIndex * 3 + 2] = VgaDacLatch[2];

    /* Fill the entry structure */
    Entry.peRed = VGA_DAC_TO_COLOR(VgaDacLatch[0]);
    Entry.peGreen = VGA_DAC_TO_COLOR(VgaDacLatch[1]);
    Entry.peBlue = VGA_DAC_TO_COLOR(VgaDacLatch[2]);
    Entry.peFlags = 0;

    /* Update the palette entry */
    SetPaletteEntries(PaletteHandle, VgaDacIndex, 1, &Entry);

    /* Check which text palette entries are affected */
    for (i = 0; i <= VGA_AC_PAL_F_REG; i++)
    {
        if (VgaAcRegisters[i] == VgaDacIndex)
        {
            /* Update the text palette entry */
            SetPaletteEntries(TextPaletteHandle, i, 1, &Entry);
        }
    }

    /* Set the palette changed flag */
    PaletteChanged = TRUE;

    /* Update the index */
    VgaDacIndex++;
    VgaDacIndex %= VGA_MAX_COLORS;
}

static inline VOID VgaWriteAc(BYTE Data)
{
    PALETTEENTRY Entry;

    ASSERT(VgaAcIndex < VGA_AC_MAX_REG);

    /* Save the value */
    if (VgaAcIndex <= VGA_AC_PAL_F_REG)
    {
        if (VgaAcPalDisable) return;

        // DbgPrint("    AC Palette Writing %d to index %d\n", Data, VgaAcIndex);
        if (VgaAcRegisters[VgaAcIndex] != Data)
        {
            /* Update the AC register */
            VgaAcRegisters[VgaAcIndex] = Data;

            /* Fill the entry structure */
            Entry.peRed = VGA_DAC_TO_COLOR(VgaDacRegisters[Data * 3]);
            Entry.peGreen = VGA_DAC_TO_COLOR(VgaDacRegisters[Data * 3 + 1]);
            Entry.peBlue = VGA_DAC_TO_COLOR(VgaDacRegisters[Data * 3 + 2]);
            Entry.peFlags = 0;

            /* Update the palette entry and set the palette change flag */
            SetPaletteEntries(TextPaletteHandle, VgaAcIndex, 1, &Entry);
            PaletteChanged = TRUE;
        }
    }
    else
    {
        VgaAcRegisters[VgaAcIndex] = Data;
    }
}

static VOID WINAPI VgaWritePort(USHORT Port, BYTE Data)
{
    DPRINT("VgaWritePort: Port 0x%X, Data 0x%02X\n", Port, Data);

    switch (Port)
    {
        case VGA_MISC_WRITE:
        {
            VgaMiscRegister = Data;

            if (VgaMiscRegister & 0x01)
            {
                /* Color emulation */
                DPRINT1("Color emulation\n");

                /* Register the new I/O Ports */
                RegisterIoPort(0x3D4, VgaReadPort, VgaWritePort);   // VGA_CRTC_INDEX_COLOR
                RegisterIoPort(0x3D5, VgaReadPort, VgaWritePort);   // VGA_CRTC_DATA_COLOR
                RegisterIoPort(0x3DA, VgaReadPort, VgaWritePort);   // VGA_INSTAT1_READ_COLOR, VGA_FEATURE_WRITE_COLOR

                /* Unregister the old ones */
                UnregisterIoPort(0x3B4);    // VGA_CRTC_INDEX_MONO
                UnregisterIoPort(0x3B5);    // VGA_CRTC_DATA_MONO
                UnregisterIoPort(0x3BA);    // VGA_INSTAT1_READ_MONO, VGA_FEATURE_WRITE_MONO
            }
            else
            {
                /* Monochrome emulation */
                DPRINT1("Monochrome emulation\n");

                /* Register the new I/O Ports */
                RegisterIoPort(0x3B4, VgaReadPort, VgaWritePort);   // VGA_CRTC_INDEX_MONO
                RegisterIoPort(0x3B5, VgaReadPort, VgaWritePort);   // VGA_CRTC_DATA_MONO
                RegisterIoPort(0x3BA, VgaReadPort, VgaWritePort);   // VGA_INSTAT1_READ_MONO, VGA_FEATURE_WRITE_MONO

                /* Unregister the old ones */
                UnregisterIoPort(0x3D4);    // VGA_CRTC_INDEX_COLOR
                UnregisterIoPort(0x3D5);    // VGA_CRTC_DATA_COLOR
                UnregisterIoPort(0x3DA);    // VGA_INSTAT1_READ_COLOR, VGA_FEATURE_WRITE_COLOR
            }

            /* Remove any existing VGA memory hook */
            MemRemoveFastMemoryHook((PVOID)0xA0000, 0x20000);

            if (VgaMiscRegister & VGA_MISC_RAM_ENABLED)
            {
                UCHAR MemoryMap = (VgaGcRegisters[VGA_GC_MISC_REG] >> 2) & 0x03;

                /* Register a memory hook */
                MemInstallFastMemoryHook(UlongToPtr(MemoryBase[MemoryMap]),
                                         MemorySize[MemoryMap],
                                         VgaReadMemory,
                                         VgaWriteMemory);
            }

            break;
        }

        case VGA_FEATURE_WRITE_MONO:
        case VGA_FEATURE_WRITE_COLOR:
        {
            VgaFeatureRegister = Data;
            break;
        }

        case VGA_AC_INDEX:
        // case VGA_AC_WRITE:
        {
            if (!VgaAcLatch)
            {
                /* Change the index */
                BYTE Index = Data & 0x1F;
                if (Index < VGA_AC_MAX_REG) VgaAcIndex = Index;

                /*
                 * Change palette protection by checking for
                 * the Palette Address Source bit.
                 */
                VgaAcPalDisable = (Data & 0x20) ? TRUE : FALSE;
            }
            else
            {
                /* Write the data */
                VgaWriteAc(Data);
            }

            /* Toggle the latch */
            VgaAcLatch = !VgaAcLatch;
            break;
        }

        case VGA_SEQ_INDEX:
        {
            /* Set the sequencer index register */
            if ((Data & 0x1F) < SVGA_SEQ_MAX_UNLOCKED_REG
                && (Data & 0x1F) != VGA_SEQ_MAX_REG)
            {
                VgaSeqIndex = Data;
            }

            break;
        }

        case VGA_SEQ_DATA:
        {
            /* Call the sequencer function */
            VgaWriteSequencer(Data);
            break;
        }

        case VGA_DAC_MASK:
        {
            if (SvgaHdrCounter == 4) SvgaHiddenRegister = Data;
            else VgaDacMask = Data;

            break;
        }

        case VGA_DAC_READ_INDEX:
        {
            VgaDacReadWrite = FALSE;
            VgaDacIndex = Data;
            VgaDacLatchCounter = 0;
            break;
        }

        case VGA_DAC_WRITE_INDEX:
        {
            VgaDacReadWrite = TRUE;
            VgaDacIndex = Data;
            VgaDacLatchCounter = 0;
            break;
        }

        case VGA_DAC_DATA:
        {
            /* Ignore writes in read mode */
            if (VgaDacReadWrite) VgaWriteDac(Data & 0x3F);
            break;
        }

        case VGA_CRTC_INDEX_MONO:
        case VGA_CRTC_INDEX_COLOR:
        {
            /* Set the CRTC index register */
            if (((Data & VGA_CRTC_INDEX_MASK) < SVGA_CRTC_MAX_UNLOCKED_REG)
                && ((Data & VGA_CRTC_INDEX_MASK) < SVGA_CRTC_UNUSED0_REG
                || (Data & VGA_CRTC_INDEX_MASK) > SVGA_CRTC_UNUSED6_REG)
                && (Data & VGA_CRTC_INDEX_MASK) != SVGA_CRTC_UNUSED7_REG)
            {
                VgaCrtcIndex = Data;
            }

            break;
        }

        case VGA_CRTC_DATA_MONO:
        case VGA_CRTC_DATA_COLOR:
        {
            /* Call the CRTC function */
            VgaWriteCrtc(Data);
            break;
        }

        case VGA_GC_INDEX:
        {
            /* Set the GC index register */
            if ((Data & VGA_GC_INDEX_MASK) < SVGA_GC_MAX_UNLOCKED_REG
                && (Data & VGA_GC_INDEX_MASK) != SVGA_GC_UNUSED0_REG
                && ((Data & VGA_GC_INDEX_MASK) < SVGA_GC_UNUSED1_REG
                || (Data & VGA_GC_INDEX_MASK) > SVGA_GC_UNUSED10_REG))
            {
                VgaGcIndex = Data;
            }

            break;
        }

        case VGA_GC_DATA:
        {
            /* Call the GC function */
            VgaWriteGc(Data);
            break;
        }

        default:
            DPRINT1("VgaWritePort: Unknown port 0x%X, Data 0x%02X\n", Port, Data);
            break;
    }

    SvgaHdrCounter = 0;
}

static inline VOID VgaVerticalRetrace(VOID)
{
    /* If nothing has changed, just return */
    // if (!ModeChanged && !CursorChanged && !PaletteChanged && !NeedsUpdate)
        // return;

    /* Change the display mode */
    if (ModeChanged) VgaChangeMode();

    /* Change the text cursor appearance */
    if (CursorChanged) VgaUpdateTextCursor();

    if (PaletteChanged)
    {
        /* Trigger a full update of the screen */
        NeedsUpdate = TRUE;
        UpdateRectangle.Left = 0;
        UpdateRectangle.Top  = 0;
        UpdateRectangle.Right  = CurrResolution.X;
        UpdateRectangle.Bottom = CurrResolution.Y;

        PaletteChanged = FALSE;
    }

    /* Update the contents of the framebuffer */
    VgaUpdateFramebuffer();

    /* Ignore if there's nothing to update */
    if (!NeedsUpdate) return;

    DPRINT("Updating screen rectangle (%d, %d, %d, %d)\n",
           UpdateRectangle.Left,
           UpdateRectangle.Top,
           UpdateRectangle.Right,
           UpdateRectangle.Bottom);

    VgaConsoleRepaintScreen(&UpdateRectangle);

    /* Clear the update flag */
    NeedsUpdate = FALSE;
}

static VOID FASTCALL VgaHorizontalRetrace(ULONGLONG ElapsedTime)
{
    ULONG VerticalTotal = VgaCrtcRegisters[VGA_CRTC_VERT_TOTAL_REG];
    ULONG VerticalRetraceStart = VgaCrtcRegisters[VGA_CRTC_START_VERT_RETRACE_REG];
    BOOLEAN BeforeVSync;
    ULONG ElapsedCycles = CurrentCycleCount - HorizontalRetraceCycle;
    ULONG Dots = (VgaSeqRegisters[VGA_SEQ_CLOCK_REG] & 1) ? 9 : 8;
    ULONG HorizTotalDots = ((ULONG)VgaCrtcRegisters[VGA_CRTC_HORZ_TOTAL_REG] + 5) * Dots;
    BYTE MaximumScanLine = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);
    ULONG HSyncsPerSecond, HSyncs;
    UNREFERENCED_PARAMETER(ElapsedTime);

    if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT) HorizTotalDots >>= 1;

    HSyncsPerSecond = VgaGetClockFrequency() / HorizTotalDots;
    HSyncs = (ElapsedCycles * HSyncsPerSecond + (CurrentIps >> 1)) / CurrentIps;
    if (HSyncs == 0) HSyncs = 1;

    VerticalTotal |= (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VT8) << 8;
    VerticalTotal |= (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VT9) << 4;

    VerticalRetraceStart |= (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VRS8) << 6;
    VerticalRetraceStart |= (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VRS9) << 2;

    if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA)
    {
        if (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & VGA_CRTC_MAXSCANLINE_DOUBLE)
        {
            VerticalRetraceStart <<= 1;
            VerticalTotal <<= 1;
        }
        else
        {
            VerticalRetraceStart *= MaximumScanLine;
            VerticalTotal *= MaximumScanLine;
        }
    }

    /* Set the cycle */
    HorizontalRetraceCycle = CurrentCycleCount;

    /* Increment the scanline counter, but make sure we don't skip any part of the vertical retrace */
    BeforeVSync = (ScanlineCounter < VerticalRetraceStart);
    ScanlineCounter += HSyncs;
    if (BeforeVSync && ScanlineCounter >= VerticalRetraceStart) ScanlineCounter = VerticalRetraceStart;

    if (ScanlineCounter == VerticalRetraceStart)
    {
        /* Save the scanline size */
        ScanlineSizeLatch = ((DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG]
                            + (((DWORD)VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_OFFSET_BIT8) << 4)) * 2;
        if (VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES) ScanlineSizeLatch <<= 2;

        /* Save the starting address */
        StartAddressLatch = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_START_ADDR_LOW_REG],
                                     VgaCrtcRegisters[VGA_CRTC_START_ADDR_HIGH_REG])
                            + ((VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_ADDR_BIT16) << 16)
                            + ((VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_ADDR_BITS1718) << 15)
                            + ((VgaCrtcRegisters[SVGA_CRTC_OVERLAY_REG] & SVGA_CRTC_EXT_ADDR_BIT19) << 12)
                            + (VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] & 0x1F) * ScanlineSizeLatch
                            + ((VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] >> 5) & 3);
    }

    if (ScanlineCounter > VerticalTotal)
    {
        ScanlineCounter = 0;
        VgaVerticalRetrace();
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

COORD VgaGetDisplayResolution(VOID)
{
    COORD Resolution;
    BYTE MaximumScanLine = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);

    /* The low 8 bits are in the display registers */
    Resolution.X = VgaCrtcRegisters[VGA_CRTC_END_HORZ_DISP_REG];
    Resolution.Y = VgaCrtcRegisters[VGA_CRTC_VERT_DISP_END_REG];

    /* Set the top bits from the overflow register */
    if (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VDE8)
    {
        Resolution.Y |= 1 << 8;
    }
    if (VgaCrtcRegisters[VGA_CRTC_OVERFLOW_REG] & VGA_CRTC_OVERFLOW_VDE9)
    {
        Resolution.Y |= 1 << 9;
    }

    /* Increase the values by 1 */
    Resolution.X++;
    Resolution.Y++;

    if (VgaGcRegisters[VGA_GC_MISC_REG] & VGA_GC_MISC_NOALPHA)
    {
        /* In "High Resolution" mode, the width of a character is always 8 pixels */
        if (VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES)
        {
            Resolution.X *= 8;
        }
        else
        {
            /* Multiply the horizontal resolution by the 9/8 dot mode */
            Resolution.X *= (VgaSeqRegisters[VGA_SEQ_CLOCK_REG] & VGA_SEQ_CLOCK_98DM)
                            ? 8 : 9;

            /* The horizontal resolution is halved in 8-bit mode */
            if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT) Resolution.X /= 2;
        }
    }

    if (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & VGA_CRTC_MAXSCANLINE_DOUBLE)
    {
        /* Halve the vertical resolution */
        Resolution.Y >>= 1;
    }
    else
    {
        /* Divide the vertical resolution by the maximum scan line (== font size in text mode) */
        Resolution.Y /= MaximumScanLine;
    }

    /* Return the resolution */
    return Resolution;
}

VOID VgaRefreshDisplay(VOID)
{
    /* Save the scanline size */
    ScanlineSizeLatch = ((DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG]
                        + (((DWORD)VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_OFFSET_BIT8) << 4)) * 2;
    if (VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES) ScanlineSizeLatch <<= 2;

    /* Save the starting address */
    StartAddressLatch = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_START_ADDR_LOW_REG],
                                 VgaCrtcRegisters[VGA_CRTC_START_ADDR_HIGH_REG])
                        + ((VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_ADDR_BIT16) << 16)
                        + ((VgaCrtcRegisters[SVGA_CRTC_EXT_DISPLAY_REG] & SVGA_CRTC_EXT_ADDR_BITS1718) << 15)
                        + ((VgaCrtcRegisters[SVGA_CRTC_OVERLAY_REG] & SVGA_CRTC_EXT_ADDR_BIT19) << 12)
                        + (VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] & 0x1F) * ScanlineSizeLatch
                        + ((VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] >> 5) & 3);

    VgaVerticalRetrace();
}

VOID FASTCALL VgaReadMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    DWORD i;
    DWORD VideoAddress;
    PUCHAR BufPtr = (PUCHAR)Buffer;

    DPRINT("VgaReadMemory: Address 0x%08X, Size %lu\n", Address, Size);

    /* Ignore if video RAM access is disabled */
    if (!Size) return;
    if ((VgaMiscRegister & VGA_MISC_RAM_ENABLED) == 0) return;

    if (!(VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_READ))
    {
        VideoAddress = VgaTranslateAddress(Address);

        /* Check for packed pixel, chain-4, and odd-even mode */
        if (VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES)
        {
            /* Just copy from the video memory */
            PVOID VideoMemory = &VgaMemory[VideoAddress + (Address & 3)];

            switch (Size)
            {
                case sizeof(UCHAR):
                    *(PUCHAR)Buffer = *(PUCHAR)VideoMemory;
                    return;

                case sizeof(USHORT):
                    *(PUSHORT)Buffer = *(PUSHORT)VideoMemory;
                    return;

                case sizeof(ULONG):
                    *(PULONG)Buffer = *(PULONG)VideoMemory;
                    return;

                case sizeof(ULONGLONG):
                    *(PULONGLONG)Buffer = *(PULONGLONG)VideoMemory;
                    return;

                default:
#if defined(__GNUC__)
                    __builtin_memcpy(Buffer, VideoMemory, Size);
#else
                    RtlCopyMemory(Buffer, VideoMemory, Size);
#endif
            }
        }
        else if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
        {
            i = 0;

            /* Write the unaligned part first */
            if (Address & 3)
            {
                switch (Address & 3)
                {
                    case 1:
                        BufPtr[i++] = VgaMemory[VideoAddress * VGA_NUM_BANKS + 1];
                    case 2:
                        BufPtr[i++] = VgaMemory[VideoAddress * VGA_NUM_BANKS + 2];
                    case 3:
                        BufPtr[i++] = VgaMemory[VideoAddress * VGA_NUM_BANKS + 3];
                }

                VideoAddress += 4;
            }

            /* Copy the aligned dwords */
            while ((i + 3) < Size)
            {
                *(PULONG)&BufPtr[i] = *(PULONG)&VgaMemory[VideoAddress * VGA_NUM_BANKS];

                i += 4;
                VideoAddress += 4;
            }

            /* Write the remaining part */
            if (i < Size)
            {
                switch (Size - i - 3)
                {
                    case 3:
                        BufPtr[i] = VgaMemory[VideoAddress * VGA_NUM_BANKS + ((Address + i) & 3)];
                        i++;
                    case 2:
                        BufPtr[i] = VgaMemory[VideoAddress * VGA_NUM_BANKS + ((Address + i) & 3)];
                        i++;
                    case 1:
                        BufPtr[i] = VgaMemory[VideoAddress * VGA_NUM_BANKS + ((Address + i) & 3)];
                        i++;
                }
            }
        }
        else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
        {
            i = 0;

            /* Check if the starting address is odd */
            if (Address & 1)
            {
                BufPtr[i++] = VgaMemory[VideoAddress * VGA_NUM_BANKS + 1];
                VideoAddress += 2;
            }

            while (i < (Size - 1))
            {
                *(PUSHORT)&BufPtr[i] = *(PUSHORT)&VgaMemory[VideoAddress * VGA_NUM_BANKS];

                i += 2;
                VideoAddress += 2;
            }

            /* Check if there is one more byte to read */
            if (i == Size - 1) BufPtr[i] = VgaMemory[VideoAddress * VGA_NUM_BANKS + ((Address + i) & 1)];
        }
        else
        {
            /* Use the selected map */
            BYTE Plane = VgaGcRegisters[VGA_GC_READ_MAP_SEL_REG] & 0x03;

            for (i = 0; i < Size; i++)
            {
                /* Copy the value to the buffer */
                BufPtr[i] = VgaMemory[(VideoAddress++) * VGA_NUM_BANKS + Plane];
            }
        }
    }
    else
    {
        const ULONG BitExpandInvertTable[] =
        {
            0xFFFFFFFF, 0xFFFFFF00, 0xFFFF00FF, 0xFFFF0000,
            0xFF00FFFF, 0xFF00FF00, 0xFF0000FF, 0xFF000000,
            0x00FFFFFF, 0x00FFFF00, 0x00FF00FF, 0x00FF0000,
            0x0000FFFF, 0x0000FF00, 0x000000FF, 0x00000000
        };

        ULONG ColorCompareBytes = BitExpandInvertTable[VgaGcRegisters[VGA_GC_COLOR_COMPARE_REG] & 0x0F];
        ULONG ColorIgnoreBytes = BitExpandInvertTable[VgaGcRegisters[VGA_GC_COLOR_IGNORE_REG] & 0x0F];

        /*
         * These values can also be computed in the following way, but using the table seems to be faster:
         *
         * ColorCompareBytes = VgaGcRegisters[VGA_GC_COLOR_COMPARE_REG] * 0x000204081;
         * ColorCompareBytes &= 0x01010101;
         * ColorCompareBytes = ~((ColorCompareBytes << 8) - ColorCompareBytes);
         *
         * ColorIgnoreBytes = VgaGcRegisters[VGA_GC_COLOR_IGNORE_REG] * 0x000204081;
         * ColorIgnoreBytes &= 0x01010101;
         * ColorIgnoreBytes = ~((ColorIgnoreBytes << 8) - ColorIgnoreBytes);
         */

        /* Loop through each byte */
        for (i = 0; i < Size; i++)
        {
            ULONG PlaneData = 0;

            /* This should always return a plane 0 address */
            VideoAddress = VgaTranslateAddress(Address + i);

            /* Read all 4 planes */
            PlaneData = *(PULONG)&VgaMemory[VideoAddress * VGA_NUM_BANKS];

            /* Reverse the bytes for which the color compare register is zero */
            PlaneData ^= ColorCompareBytes;

            /* Apply the color ignore register */
            PlaneData |= ColorIgnoreBytes;

            /* Store the value in the buffer */
            BufPtr[i] = (PlaneData & (PlaneData >> 8) & (PlaneData >> 16) & (PlaneData >> 24)) & 0xFF;
        }
    }

    /* Load the latch registers */
    VideoAddress = VgaTranslateAddress(Address + Size - 1);
    *(PULONG)VgaLatchRegisters = *(PULONG)&VgaMemory[WRAP_OFFSET(VideoAddress) * VGA_NUM_BANKS];
}

BOOLEAN FASTCALL VgaWriteMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    DWORD i, j;
    DWORD VideoAddress;
    PUCHAR BufPtr = (PUCHAR)Buffer;

    DPRINT("VgaWriteMemory: Address 0x%08X, Size %lu\n", Address, Size);

    /* Ignore if video RAM access is disabled */
    if ((VgaMiscRegister & VGA_MISC_RAM_ENABLED) == 0) return TRUE;

    /* Also ignore if write access to all planes is disabled */
    if ((VgaSeqRegisters[VGA_SEQ_MASK_REG] & 0x0F) == 0x00) return TRUE;

    if (!(VgaSeqRegisters[SVGA_SEQ_EXT_MODE_REG] & SVGA_SEQ_EXT_MODE_HIGH_RES))
    {
        /* Loop through each byte */
        for (i = 0; i < Size; i++)
        {
            VideoAddress = VgaTranslateAddress(Address + i);

            for (j = 0; j < VGA_NUM_BANKS; j++)
            {
                /* Make sure the page is writeable */
                if (!(VgaSeqRegisters[VGA_SEQ_MASK_REG] & (1 << j))) continue;

                /* Check if this is chain-4 mode */
                if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
                {
                    if (((Address + i) & 0x03) != j)
                    {
                        /* This plane will not be accessed */
                        continue;
                    }
                }

                /* Check if this is odd-even mode */
                if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
                {
                    if (((Address + i) & 0x01) != (j & 1))
                    {
                        /* This plane will not be accessed */
                        continue;
                    }
                }

                /* Copy the value to the VGA memory */
                VgaMemory[VideoAddress * VGA_NUM_BANKS + j] = VgaTranslateByteForWriting(BufPtr[i], j);
            }
        }
    }
    else
    {
        PVOID VideoMemory;

        // TODO: Apply the page write mask!
        // TODO: Check whether the write mode stuff applies to packed-pixel modes

        /* Just copy to the video memory */
        VideoAddress = VgaTranslateAddress(Address);
        VideoMemory = &VgaMemory[VideoAddress + (Address & 3)];

        switch (Size)
        {
            case sizeof(UCHAR):
                *(PUCHAR)VideoMemory = *(PUCHAR)Buffer;
                return TRUE;

            case sizeof(USHORT):
                *(PUSHORT)VideoMemory = *(PUSHORT)Buffer;
                return TRUE;

            case sizeof(ULONG):
                *(PULONG)VideoMemory = *(PULONG)Buffer;
                return TRUE;

            case sizeof(ULONGLONG):
                *(PULONGLONG)VideoMemory = *(PULONGLONG)Buffer;
                return TRUE;

            default:
#if defined(__GNUC__)
                __builtin_memcpy(VideoMemory, Buffer, Size);
#else
                RtlCopyMemory(VideoMemory, Buffer, Size);
#endif
        }
    }

    return TRUE;
}

VOID VgaClearMemory(VOID)
{
    RtlZeroMemory(VgaMemory, sizeof(VgaMemory));
}

VOID VgaWriteTextModeFont(UINT FontNumber, CONST UCHAR* FontData, UINT Height)
{
    UINT i, j;
    ASSERT(Height <= VGA_MAX_FONT_HEIGHT);

    for (i = 0; i < VGA_FONT_CHARACTERS; i++)
    {
        /* Write the character */
        for (j = 0; j < Height; j++)
        {
            VgaMemory[(i * VGA_MAX_FONT_HEIGHT + j) * VGA_NUM_BANKS + VGA_FONT_BANK] = FontData[i * Height + j];
        }

        /* Clear the unused part */
        for (j = Height; j < VGA_MAX_FONT_HEIGHT; j++)
        {
            VgaMemory[(i * VGA_MAX_FONT_HEIGHT + j) * VGA_NUM_BANKS + VGA_FONT_BANK] = 0;
        }
    }
}

BOOLEAN VgaInitialize(HANDLE TextHandle)
{
    if (!VgaConsoleInitialize(TextHandle)) return FALSE;

    /* Clear the SEQ, GC, CRTC and AC registers */
    RtlZeroMemory(VgaSeqRegisters , sizeof(VgaSeqRegisters ));
    RtlZeroMemory(VgaGcRegisters  , sizeof(VgaGcRegisters  ));
    RtlZeroMemory(VgaCrtcRegisters, sizeof(VgaCrtcRegisters));
    RtlZeroMemory(VgaAcRegisters  , sizeof(VgaAcRegisters  ));

    /* Initialize the VGA palette and fail if it isn't successfully created */
    if (!VgaInitializePalette()) return FALSE;
    /***/ VgaResetPalette(); /***/

    /* Reset the sequencer */
    VgaResetSequencer();

    /* Clear the VGA memory */
    VgaClearMemory();

    /* Register the I/O Ports */
    RegisterIoPort(0x3CC, VgaReadPort,         NULL);   // VGA_MISC_READ
    RegisterIoPort(0x3C2, VgaReadPort, VgaWritePort);   // VGA_MISC_WRITE, VGA_INSTAT0_READ
    RegisterIoPort(0x3CA, VgaReadPort,         NULL);   // VGA_FEATURE_READ
    RegisterIoPort(0x3C0, VgaReadPort, VgaWritePort);   // VGA_AC_INDEX, VGA_AC_WRITE
    RegisterIoPort(0x3C1, VgaReadPort,         NULL);   // VGA_AC_READ
    RegisterIoPort(0x3C4, VgaReadPort, VgaWritePort);   // VGA_SEQ_INDEX
    RegisterIoPort(0x3C5, VgaReadPort, VgaWritePort);   // VGA_SEQ_DATA
    RegisterIoPort(0x3C6, VgaReadPort, VgaWritePort);   // VGA_DAC_MASK
    RegisterIoPort(0x3C7, VgaReadPort, VgaWritePort);   // VGA_DAC_READ_INDEX
    RegisterIoPort(0x3C8, VgaReadPort, VgaWritePort);   // VGA_DAC_WRITE_INDEX
    RegisterIoPort(0x3C9, VgaReadPort, VgaWritePort);   // VGA_DAC_DATA
    RegisterIoPort(0x3CE, VgaReadPort, VgaWritePort);   // VGA_GC_INDEX
    RegisterIoPort(0x3CF, VgaReadPort, VgaWritePort);   // VGA_GC_DATA

    /* CGA ports for compatibility, unimplemented */
    RegisterIoPort(0x3D8, VgaReadPort, VgaWritePort);   // CGA_MODE_CTRL_REG
    RegisterIoPort(0x3D9, VgaReadPort, VgaWritePort);   // CGA_PAL_CTRL_REG

    HSyncTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED, HZ_TO_NS(31469), VgaHorizontalRetrace);

    /* Return success */
    return TRUE;
}

VOID VgaCleanup(VOID)
{
    /* Do a final display refresh */
    VgaRefreshDisplay();

    DestroyHardwareTimer(HSyncTimer);

    /* Leave the current video mode */
    VgaLeaveCurrentMode(); // ScreenMode

    MemRemoveFastMemoryHook((PVOID)0xA0000, 0x20000);

    VgaConsoleCleanup();
}

/* EOF */
