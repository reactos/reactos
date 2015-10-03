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

/* PRIVATE VARIABLES **********************************************************/

static CONST DWORD MemoryBase[] = { 0xA0000, 0xA0000, 0xB0000, 0xB8000 };
static CONST DWORD MemorySize[] = { 0x20000, 0x10000, 0x08000, 0x08000 };

/*
 * Activate this line if you want to use the real
 * RegisterConsoleVDM API of ReactOS/Windows.
 */
// #define USE_REAL_REGISTERCONSOLEVDM

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

/*
 * Console interface -- VGA-mode-agnostic
 */
// WARNING! This structure *MUST BE* in sync with the one in consrv/include/conio_winsrv.h
typedef struct _CHAR_CELL
{
    CHAR Char;
    BYTE Attributes;
} CHAR_CELL, *PCHAR_CELL;
C_ASSERT(sizeof(CHAR_CELL) == 2);

static PVOID ConsoleFramebuffer = NULL; // Active framebuffer, points to
                                        // either TextFramebuffer or a
                                        // valid graphics framebuffer.
static HPALETTE TextPaletteHandle = NULL;
static HPALETTE PaletteHandle = NULL;

static HANDLE StartEvent = NULL;
static HANDLE EndEvent   = NULL;
static HANDLE AnotherEvent = NULL;

static CONSOLE_CURSOR_INFO         OrgConsoleCursorInfo;
static CONSOLE_SCREEN_BUFFER_INFO  OrgConsoleBufferInfo;


static HANDLE ScreenBufferHandle = NULL;
static PVOID  OldConsoleFramebuffer = NULL;


/*
 * Text mode -- we always keep a valid text mode framebuffer
 * even if we are in graphics mode. This is needed in order
 * to keep a consistent VGA state. However, each time the VGA
 * detaches from the console (and reattaches to it later on),
 * this text mode framebuffer is recreated.
 */
static HANDLE TextConsoleBuffer = NULL;
static CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
static COORD  TextResolution = {0};
static PCHAR_CELL TextFramebuffer = NULL;

/*
 * Graphics mode
 */
static HANDLE GraphicsConsoleBuffer = NULL;
static PVOID  GraphicsFramebuffer = NULL;
static HANDLE ConsoleMutex = NULL;
/* DoubleVision support */
static BOOLEAN DoubleWidth  = FALSE;
static BOOLEAN DoubleHeight = FALSE;


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

static ULONGLONG VerticalRetraceCycle   = 0ULL;
static ULONGLONG HorizontalRetraceCycle = 0ULL;
static PHARDWARE_TIMER VSyncTimer;
static PHARDWARE_TIMER HSyncTimer;

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

/* RegisterConsoleVDM EMULATION ***********************************************/

#include <ntddvdeo.h>

#ifdef USE_REAL_REGISTERCONSOLEVDM

#define __RegisterConsoleVDM        RegisterConsoleVDM
#define __InvalidateConsoleDIBits   InvalidateConsoleDIBits

#else

/*
 * This private buffer, per-console, is used by
 * RegisterConsoleVDM and InvalidateConsoleDIBits.
 */
static COORD VDMBufferSize  = {0};
static PCHAR_CELL VDMBuffer = NULL;

static PCHAR_INFO CharBuff  = NULL; // This is a hack, which is unneeded
                                    // for the real RegisterConsoleVDM and
                                    // InvalidateConsoleDIBits

BOOL
WINAPI
__RegisterConsoleVDM(IN DWORD dwRegisterFlags,
                     IN HANDLE hStartHardwareEvent,
                     IN HANDLE hEndHardwareEvent,
                     IN HANDLE hErrorHardwareEvent,
                     IN DWORD dwUnusedVar,
                     OUT LPDWORD lpVideoStateLength,
                     OUT PVOID* lpVideoState, // PVIDEO_HARDWARE_STATE_HEADER*
                     IN PVOID lpUnusedBuffer,
                     IN DWORD dwUnusedBufferLength,
                     IN COORD dwVDMBufferSize,
                     OUT PVOID* lpVDMBuffer)
{
    UNREFERENCED_PARAMETER(hErrorHardwareEvent);
    UNREFERENCED_PARAMETER(dwUnusedVar);
    UNREFERENCED_PARAMETER(lpVideoStateLength);
    UNREFERENCED_PARAMETER(lpVideoState);
    UNREFERENCED_PARAMETER(lpUnusedBuffer);
    UNREFERENCED_PARAMETER(dwUnusedBufferLength);

    SetLastError(0);
    DPRINT1("__RegisterConsoleVDM(%d)\n", dwRegisterFlags);

    if (lpVDMBuffer == NULL) return FALSE;

    if (dwRegisterFlags != 0)
    {
        // if (hStartHardwareEvent == NULL || hEndHardwareEvent == NULL) return FALSE;
        if (VDMBuffer != NULL) return FALSE;

        VDMBufferSize = dwVDMBufferSize;

        /* HACK: Cache -- to be removed in the real implementation */
        CharBuff = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   VDMBufferSize.X * VDMBufferSize.Y
                                                   * sizeof(*CharBuff));
        ASSERT(CharBuff);

        VDMBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    VDMBufferSize.X * VDMBufferSize.Y
                                                    * sizeof(*VDMBuffer));
        *lpVDMBuffer = VDMBuffer;
        return (VDMBuffer != NULL);
    }
    else
    {
        /* HACK: Cache -- to be removed in the real implementation */
        if (CharBuff) RtlFreeHeap(RtlGetProcessHeap(), 0, CharBuff);
        CharBuff = NULL;

        if (VDMBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, VDMBuffer);
        VDMBuffer = NULL;

        VDMBufferSize.X = VDMBufferSize.Y = 0;

        return TRUE;
    }
}

BOOL
__InvalidateConsoleDIBits(IN HANDLE hConsoleOutput,
                          IN PSMALL_RECT lpRect)
{
    if ((hConsoleOutput == TextConsoleBuffer) && (VDMBuffer != NULL))
    {
        /* HACK: Write the cached data to the console */

        COORD Origin = { lpRect->Left, lpRect->Top };
        SHORT i, j;

        ASSERT(CharBuff);

        for (i = 0; i < VDMBufferSize.Y; i++)
        {
            for (j = 0; j < VDMBufferSize.X; j++)
            {
                CharBuff[i * VDMBufferSize.X + j].Char.AsciiChar = VDMBuffer[i * VDMBufferSize.X + j].Char;
                CharBuff[i * VDMBufferSize.X + j].Attributes     = VDMBuffer[i * VDMBufferSize.X + j].Attributes;
            }
        }

        WriteConsoleOutputA(hConsoleOutput,
                            CharBuff,
                            VDMBufferSize,
                            Origin,
                            lpRect);
    }

    return InvalidateConsoleDIBits(hConsoleOutput, lpRect);
}

#endif

/* PRIVATE FUNCTIONS **********************************************************/

static inline DWORD VgaGetAddressSize(VOID);
static VOID VgaUpdateTextCursor(VOID);

static inline DWORD VgaGetVideoBaseAddress(VOID)
{
    return MemoryBase[(VgaGcRegisters[VGA_GC_MISC_REG] >> 2) & 0x03];
}

static VOID VgaUpdateCursorPosition(VOID)
{
    /*
     * Update the cursor position in the VGA registers.
     */
    WORD Offset = ConsoleInfo.dwCursorPosition.Y * TextResolution.X +
                  ConsoleInfo.dwCursorPosition.X;

    VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_LOW_REG]  = LOBYTE(Offset);
    VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_HIGH_REG] = HIBYTE(Offset);

    VgaUpdateTextCursor();
}

static BOOL VgaAttachToConsoleInternal(PCOORD Resolution)
{
    BOOL Success;
    ULONG Length = 0;
    PVIDEO_HARDWARE_STATE_HEADER State;

#ifdef USE_REAL_REGISTERCONSOLEVDM
    PCHAR_INFO CharBuff = NULL;
#endif
    SHORT i, j;
    DWORD AddressSize, ScanlineSize;
    DWORD Address = 0;
    DWORD CurrentAddr;
    SMALL_RECT ConRect;
    COORD Origin = { 0, 0 };

    ASSERT(TextFramebuffer == NULL);

    TextResolution = *Resolution;

    /*
     * Windows 2k3 winsrv.dll calls NtVdmControl(VdmQueryVdmProcess == 14, &ConsoleHandle);
     * in the two following APIs:
     * SrvRegisterConsoleVDM  (corresponding Win32 API: RegisterConsoleVDM)
     * SrvVDMConsoleOperation (corresponding Win32 API: ??)
     * to check whether the current process is a VDM process, and fails otherwise
     * with the error 0xC0000022 (STATUS_ACCESS_DENIED).
     *
     * It is worth it to notice that also basesrv.dll does the same only for the
     * BaseSrvIsFirstVDM API...
     */

    /* Register with the console server */
    Success =
    __RegisterConsoleVDM(1,
                         StartEvent,
                         EndEvent,
                         AnotherEvent, // NULL,
                         0,
                         &Length, // NULL, <-- putting this (and null in the next var) makes the API returning error 12 "ERROR_INVALID_ACCESS"
                         (PVOID*)&State, // NULL,
                         NULL,
                         0,
                         TextResolution,
                         (PVOID*)&TextFramebuffer);
    if (!Success)
    {
        DisplayMessage(L"RegisterConsoleVDM failed with error %d\n", GetLastError());
        EmulatorTerminate();
        return FALSE;
    }

#ifdef USE_REAL_REGISTERCONSOLEVDM
    CharBuff = RtlAllocateHeap(RtlGetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               TextResolution.X * TextResolution.Y
                                                * sizeof(*CharBuff));
    ASSERT(CharBuff);
#endif

    /* Retrieve the latest console information */
    GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);

    /* Resize the console */
    ConRect.Left   = 0;
    ConRect.Right  = ConRect.Left + Resolution->X - 1;
    ConRect.Bottom = max(ConsoleInfo.dwCursorPosition.Y, Resolution->Y - 1);
    ConRect.Top    = ConRect.Bottom - Resolution->Y + 1;
    /*
     * Use this trick to effectively resize the console buffer and window,
     * because:
     * - SetConsoleScreenBufferSize fails if the new console screen buffer size
     *   is smaller than the current console window size, and:
     * - SetConsoleWindowInfo fails if the new console window size is larger
     *   than the current console screen buffer size.
     */
    Success = SetConsoleScreenBufferSize(TextConsoleBuffer, *Resolution);
    DPRINT1("(attach) SetConsoleScreenBufferSize(1) %s with error %d\n", Success ? "succeeded" : "failed", GetLastError());
    Success = SetConsoleWindowInfo(TextConsoleBuffer, TRUE, &ConRect);
    DPRINT1("(attach) SetConsoleWindowInfo %s with error %d\n", Success ? "succeeded" : "failed", GetLastError());
    Success = SetConsoleScreenBufferSize(TextConsoleBuffer, *Resolution);
    DPRINT1("(attach) SetConsoleScreenBufferSize(2) %s with error %d\n", Success ? "succeeded" : "failed", GetLastError());

    /* Update the saved console information */
    GetConsoleScreenBufferInfo(TextConsoleBuffer, &ConsoleInfo);

    /*
     * Copy console data into VGA memory
     */

    /* Read the data from the console into the framebuffer... */
    ConRect.Left   = ConRect.Top = 0;
    ConRect.Right  = TextResolution.X;
    ConRect.Bottom = TextResolution.Y;

    ReadConsoleOutputA(TextConsoleBuffer,
                       CharBuff,
                       TextResolution,
                       Origin,
                       &ConRect);

    /* ... and copy the framebuffer into the VGA memory */
    AddressSize  = VgaGetAddressSize();
    ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;

    /* Loop through the scanlines */
    for (i = 0; i < TextResolution.Y; i++)
    {
        /* Loop through the characters */
        for (j = 0; j < TextResolution.X; j++)
        {
            CurrentAddr = LOWORD((Address + j) * AddressSize);

            /* Store the character in plane 0 */
            VgaMemory[CurrentAddr] = CharBuff[i * TextResolution.X + j].Char.AsciiChar;

            /* Store the attribute in plane 1 */
            VgaMemory[CurrentAddr + VGA_BANK_SIZE] = (BYTE)CharBuff[i * TextResolution.X + j].Attributes;
        }

        /* Move to the next scanline */
        Address += ScanlineSize;
    }

#ifdef USE_REAL_REGISTERCONSOLEVDM
    if (CharBuff) RtlFreeHeap(RtlGetProcessHeap(), 0, CharBuff);
#endif

    VgaUpdateCursorPosition();

    return TRUE;
}

static VOID VgaDetachFromConsoleInternal(VOID)
{
    ULONG dummyLength;
    PVOID dummyPtr;
    COORD dummySize = {0};

    /* Deregister with the console server */
    __RegisterConsoleVDM(0,
                         NULL,
                         NULL,
                         NULL,
                         0,
                         &dummyLength,
                         &dummyPtr,
                         NULL,
                         0,
                         dummySize,
                         &dummyPtr);

    TextFramebuffer = NULL;
}

static BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR)
        return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

static inline DWORD VgaGetAddressSize(VOID)
{
    if (VgaCrtcRegisters[VGA_CRTC_UNDERLINE_REG] & VGA_CRTC_UNDERLINE_DWORD)
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

static inline DWORD VgaTranslateReadAddress(DWORD Address)
{
    DWORD Offset = LOWORD(Address - VgaGetVideoBaseAddress());
    BYTE Plane;

    /* Check for chain-4 and odd-even mode */
    if (VgaSeqRegisters[VGA_SEQ_MEM_REG] & VGA_SEQ_MEM_C4)
    {
        /* The lowest two bits are the plane number */
        Plane = Offset & 0x03;
        Offset &= ~3;
    }
    else if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_OE)
    {
        /* The LSB is the plane number */
        Plane = Offset & 0x01;
        Offset &= ~1;
    }
    else
    {
        /* Use the read mode */
        Plane = VgaGcRegisters[VGA_GC_READ_MAP_SEL_REG] & 0x03;
    }

    /* Return the offset on plane 0 for read mode 1 */
    if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_READ) return Offset;
    else return Offset + Plane * VGA_BANK_SIZE;
}

static inline DWORD VgaTranslateWriteAddress(DWORD Address)
{
    DWORD Offset = LOWORD(Address - VgaGetVideoBaseAddress());

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

static VOID VgaSetActiveScreenBuffer(HANDLE ScreenBuffer)
{
    ASSERT(ScreenBuffer);

    /* Set the active buffer and reattach the VDM UI to it */
    SetConsoleActiveScreenBuffer(ScreenBuffer);
    ConsoleReattach(ScreenBuffer);
}

static BOOL VgaEnterGraphicsMode(PCOORD Resolution)
{
    DWORD i;
    CONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    BYTE BitmapInfoBuffer[VGA_BITMAP_INFO_SIZE];
    LPBITMAPINFO BitmapInfo = (LPBITMAPINFO)BitmapInfoBuffer;
    LPWORD PaletteIndex = (LPWORD)(BitmapInfo->bmiColors);

    LONG Width  = Resolution->X;
    LONG Height = Resolution->Y;

    /* Use DoubleVision mode if the resolution is too small */
    DoubleWidth = (Width < VGA_MINIMUM_WIDTH);
    if (DoubleWidth) Width *= 2;
    DoubleHeight = (Height < VGA_MINIMUM_HEIGHT);
    if (DoubleHeight) Height *= 2;

    /* Fill the bitmap info header */
    RtlZeroMemory(&BitmapInfo->bmiHeader, sizeof(BitmapInfo->bmiHeader));
    BitmapInfo->bmiHeader.biSize   = sizeof(BitmapInfo->bmiHeader);
    BitmapInfo->bmiHeader.biWidth  = Width;
    BitmapInfo->bmiHeader.biHeight = Height;
    BitmapInfo->bmiHeader.biBitCount = 8;
    BitmapInfo->bmiHeader.biPlanes   = 1;
    BitmapInfo->bmiHeader.biCompression = BI_RGB;
    BitmapInfo->bmiHeader.biSizeImage   = Width * Height /* * 1 == biBitCount / 8 */;

    /* Fill the palette data */
    for (i = 0; i < (VGA_PALETTE_SIZE / 3); i++) PaletteIndex[i] = (WORD)i;

    /* Fill the console graphics buffer info */
    GraphicsBufferInfo.dwBitMapInfoLength = VGA_BITMAP_INFO_SIZE;
    GraphicsBufferInfo.lpBitMapInfo = BitmapInfo;
    GraphicsBufferInfo.dwUsage = DIB_PAL_COLORS;

    /* Create the buffer */
    GraphicsConsoleBuffer = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                      NULL,
                                                      CONSOLE_GRAPHICS_BUFFER,
                                                      &GraphicsBufferInfo);
    if (GraphicsConsoleBuffer == INVALID_HANDLE_VALUE) return FALSE;

    /* Save the framebuffer address and mutex */
    GraphicsFramebuffer = GraphicsBufferInfo.lpBitMap;
    ConsoleMutex = GraphicsBufferInfo.hMutex;

    /* Clear the framebuffer */
    RtlZeroMemory(GraphicsFramebuffer, BitmapInfo->bmiHeader.biSizeImage);

    /* Set the active buffer */
    VgaSetActiveScreenBuffer(GraphicsConsoleBuffer);

    /* The active framebuffer is now the graphics framebuffer */
    ConsoleFramebuffer = GraphicsFramebuffer;

    /* Set the graphics mode palette */
    SetConsolePalette(GraphicsConsoleBuffer,
                      PaletteHandle,
                      SYSPAL_NOSTATIC256);

    /* Set the screen mode flag */
    ScreenMode = GRAPHICS_MODE;

    return TRUE;
}

static VOID VgaLeaveGraphicsMode(VOID)
{
    /* Release the console framebuffer mutex */
    ReleaseMutex(ConsoleMutex);

    /* Switch back to the default console text buffer */
    // VgaSetActiveScreenBuffer(TextConsoleBuffer);

    /* Cleanup the video data */
    CloseHandle(ConsoleMutex);
    ConsoleMutex = NULL;
    GraphicsFramebuffer = NULL;
    CloseHandle(GraphicsConsoleBuffer);
    GraphicsConsoleBuffer = NULL;

    /* Reset the active framebuffer */
    ConsoleFramebuffer = NULL;

    DoubleWidth  = FALSE;
    DoubleHeight = FALSE;
}

static BOOL VgaEnterTextMode(PCOORD Resolution)
{
    /* Switch to the text buffer */
    // FIXME: Wouldn't it be preferrable to switch to it AFTER we reset everything??
    VgaSetActiveScreenBuffer(TextConsoleBuffer);

    /* Adjust the text framebuffer if we changed the resolution */
    if (TextResolution.X != Resolution->X ||
        TextResolution.Y != Resolution->Y)
    {
        VgaDetachFromConsoleInternal();

        /*
         * VgaAttachToConsoleInternal sets TextResolution
         * to the new resolution and updates ConsoleInfo.
         */
        if (!VgaAttachToConsoleInternal(Resolution))
        {
            DisplayMessage(L"An unexpected error occurred!\n");
            EmulatorTerminate();
            return FALSE;
        }
    }
    else
    {
        VgaUpdateCursorPosition();
    }

    /* The active framebuffer is now the text framebuffer */
    ConsoleFramebuffer = TextFramebuffer;

    /*
     * Set the text mode palette.
     *
     * INFORMATION: This call should fail on Windows (and therefore
     * we get the default palette and our external behaviour is
     * just like Windows' one), but it should success on ReactOS
     * (so that we get console palette changes even for text-mode
     * screen-buffers, which is a new feature on ReactOS).
     */
    SetConsolePalette(TextConsoleBuffer,
                      TextPaletteHandle,
                      SYSPAL_NOSTATIC256);

    /* Set the screen mode flag */
    ScreenMode = TEXT_MODE;

    return TRUE;
}

static VOID VgaLeaveTextMode(VOID)
{
    /* Reset the active framebuffer */
    ConsoleFramebuffer = NULL;
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
    if (ScreenMode == GRAPHICS_MODE)
        VgaLeaveGraphicsMode();
    else
        VgaLeaveTextMode();

    /* Update the current resolution */
    CurrResolution = NewResolution;

    /* The new screen mode will be updated via the VgaEnterText/GraphicsMode functions */

    /* Check if the new mode is alphanumeric */
    if (NewScreenMode == TEXT_MODE)
    {
        /* Enter new text mode */
        if (!VgaEnterTextMode(&CurrResolution))
        {
            DisplayMessage(L"An unexpected VGA error occurred while switching into text mode. Error: %u", GetLastError());
            EmulatorTerminate();
            return;
        }
    }
    else
    {
        /* Enter graphics mode */
        if (!VgaEnterGraphicsMode(&CurrResolution))
        {
            DisplayMessage(L"An unexpected VGA error occurred while switching into graphics mode. Error: %u", GetLastError());
            EmulatorTerminate();
            return;
        }
    }

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

static VOID VgaUpdateFramebuffer(VOID)
{
    SHORT i, j, k;
    DWORD AddressSize = VgaGetAddressSize();
    DWORD ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;
    BYTE PresetRowScan = VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] & 0x1F;
    DWORD Address = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_START_ADDR_LOW_REG],
                             VgaCrtcRegisters[VGA_CRTC_START_ADDR_HIGH_REG])
                    + PresetRowScan * ScanlineSize
                    + ((VgaCrtcRegisters[VGA_CRTC_PRESET_ROW_SCAN_REG] >> 5) & 3);

    /*
     * If the console framebuffer is NULL, that means something
     * went wrong earlier and this is the final display refresh.
     */
    if (ConsoleFramebuffer == NULL) return;

    /* Check if we are in text or graphics mode */
    if (ScreenMode == GRAPHICS_MODE)
    {
        /* Graphics mode */
        PBYTE GraphicsBuffer = (PBYTE)ConsoleFramebuffer;
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

        /* Loop through the scanlines */
        for (i = 0; i < CurrResolution.Y; i++)
        {
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
                    X = j + ((VgaAcRegisters[VGA_AC_HORZ_PANNING_REG] & 0x0F) >> 1);
                }
                else
                {
                    X = j + (VgaAcRegisters[VGA_AC_HORZ_PANNING_REG] & 0x0F);
                }

                /* Check the shifting mode */
                if (VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_SHIFT256)
                {
                    /* 4 bits shifted from each plane */

                    /* Check if this is 16 or 256 color mode */
                    if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT)
                    {
                        /* One byte per pixel */
                        PixelData = VgaMemory[(X % VGA_NUM_BANKS) * VGA_BANK_SIZE
                                              + LOWORD((Address + (X / VGA_NUM_BANKS))
                                              * AddressSize)];
                    }
                    else
                    {
                        /* 4-bits per pixel */

                        PixelData = VgaMemory[(X % VGA_NUM_BANKS) * VGA_BANK_SIZE
                                              + LOWORD((Address + (X / (VGA_NUM_BANKS * 2)))
                                              * AddressSize)];

                        /* Check if we should use the highest 4 bits or lowest 4 */
                        if (((X / VGA_NUM_BANKS) % 2) == 0)
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
                        BYTE LowPlaneData = VgaMemory[BankNumber * VGA_BANK_SIZE + LOWORD(Offset * AddressSize)];
                        BYTE HighPlaneData = VgaMemory[(BankNumber + 2) * VGA_BANK_SIZE + LOWORD(Offset * AddressSize)];

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
                            BYTE PlaneData = VgaMemory[k * VGA_BANK_SIZE
                                                       + LOWORD((Address + (X / VGA_NUM_BANKS))
                                                       * AddressSize)];

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
                            BYTE PlaneData = VgaMemory[k * VGA_BANK_SIZE
                                                       + LOWORD((Address + (X / (VGA_NUM_BANKS * 2)))
                                                       * AddressSize)];

                            /* If the bit on that plane is set, set it */
                            if (PlaneData & (1 << (7 - (X % 8)))) PixelData |= 1 << k;
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
                    PixelData = (VgaAcPalDisable ? VgaAcRegisters[PixelData & 0x0F]
                                                 : 0);
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
                Address += ScanlineSize;
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
        PCHAR_CELL CharBuffer = (PCHAR_CELL)ConsoleFramebuffer;
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
                CurrentAddr = LOWORD((Address + j) * AddressSize);

                /* Plane 0 holds the character itself */
                CharInfo.Char = VgaMemory[CurrentAddr];

                /* Plane 1 holds the attribute */
                CharInfo.Attributes = VgaMemory[CurrentAddr + VGA_BANK_SIZE];

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
            Address += ScanlineSize;
        }
    }
}

static VOID VgaUpdateTextCursor(VOID)
{
    COORD Position;
    CONSOLE_CURSOR_INFO CursorInfo;

    BOOL CursorVisible = !(VgaCrtcRegisters[VGA_CRTC_CURSOR_START_REG] & 0x20);
    BYTE CursorStart   =   VgaCrtcRegisters[VGA_CRTC_CURSOR_START_REG] & 0x1F;
    BYTE CursorEnd     =   VgaCrtcRegisters[VGA_CRTC_CURSOR_END_REG]   & 0x1F;

    DWORD ScanlineSize = (DWORD)VgaCrtcRegisters[VGA_CRTC_OFFSET_REG] * 2;
    BYTE TextSize = 1 + (VgaCrtcRegisters[VGA_CRTC_MAX_SCAN_LINE_REG] & 0x1F);
    WORD Location = MAKEWORD(VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_LOW_REG],
                             VgaCrtcRegisters[VGA_CRTC_CURSOR_LOC_HIGH_REG]);

    /* Just return if we are not in text mode */
    if (ScreenMode != TEXT_MODE) return;

    if (CursorStart < CursorEnd)
    {
        /* Visible cursor */
        CursorInfo.bVisible = CursorVisible;
        CursorInfo.dwSize   = (100 * (CursorEnd - CursorStart)) / TextSize;
    }
    else
    {
        /* Hidden cursor */
        CursorInfo.bVisible = FALSE;
        CursorInfo.dwSize   = 1; // The size needs to be non-zero for SetConsoleCursorInfo to succeed.
    }

    /* Add the cursor skew to the location */
    Location += (VgaCrtcRegisters[VGA_CRTC_CURSOR_END_REG] >> 5) & 0x03;

    /* Find the coordinates of the new position */
    Position.X = (SHORT)(Location % ScanlineSize);
    Position.Y = (SHORT)(Location / ScanlineSize);

    DPRINT("VgaUpdateTextCursor: X = %d ; Y = %d\n", Position.X, Position.Y);

    /* Update the physical cursor */
    SetConsoleCursorInfo(TextConsoleBuffer, &CursorInfo);
    SetConsoleCursorPosition(TextConsoleBuffer, Position);

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
            ULONGLONG Cycles = GetCycleCount();
            ULONG CyclesPerMicrosecond = (ULONG)((GetCycleSpeed() + 500000ULL) / 1000000ULL);
            ULONG Dots = (VgaSeqRegisters[VGA_SEQ_CLOCK_REG] & 1) ? 9 : 8;
            ULONG Clock = VgaGetClockFrequency() / 1000000;
            ULONG HorizTotalDots = ((ULONG)VgaCrtcRegisters[VGA_CRTC_HORZ_TOTAL_REG] + 5) * Dots;
            ULONG VblankStart, VblankEnd, HblankStart, HblankEnd;
            ULONG HblankDuration, VblankDuration;

            /* Calculate the vertical blanking duration in cycles */
            VblankStart = VgaCrtcRegisters[VGA_CRTC_START_VERT_BLANKING_REG] & 0x7F;
            VblankEnd = VgaCrtcRegisters[VGA_CRTC_END_VERT_BLANKING_REG] & 0x7F;
            if (VblankEnd < VblankStart) VblankEnd |= 0x80;
            VblankDuration = ((VblankEnd - VblankStart) * HorizTotalDots
                             * CyclesPerMicrosecond + (Clock >> 1)) / Clock;

            /* Calculate the horizontal blanking duration in cycles */
            HblankStart = VgaCrtcRegisters[VGA_CRTC_START_HORZ_BLANKING_REG] & 0x1F;
            HblankEnd = VgaCrtcRegisters[VGA_CRTC_END_HORZ_BLANKING_REG] & 0x1F;
            if (HblankEnd < HblankStart) HblankEnd |= 0x20;
            HblankDuration = ((HblankEnd - HblankStart) * Dots
                             * CyclesPerMicrosecond + (Clock >> 1)) / Clock;

            Vsync = (Cycles - VerticalRetraceCycle) < (ULONGLONG)VblankDuration;
            Hsync = (Cycles - HorizontalRetraceCycle) < (ULONGLONG)HblankDuration;

            /* Reset the AC latch */
            VgaAcLatch = FALSE;

            /* Reverse the polarity, if needed */
            if (VgaMiscRegister & VGA_MISC_VSYNCP) Vsync = !Vsync;
            if (VgaMiscRegister & VGA_MISC_HSYNCP) Hsync = !Hsync;

            /* Set a flag if there is a vertical or horizontal retrace */
            if (Vsync || Hsync) Result |= VGA_STAT_DD;

            /* Set an additional flag if there was a vertical retrace */
            if (Vsync) Result |= VGA_STAT_VRETRACE;

            return Result;
        }

        case VGA_FEATURE_READ:
            return VgaFeatureRegister;

        case VGA_AC_INDEX:
            return VgaAcIndex;

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
                MemInstallFastMemoryHook((PVOID)MemoryBase[MemoryMap],
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
                MemInstallFastMemoryHook((PVOID)MemoryBase[MemoryMap],
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

static VOID FASTCALL VgaVerticalRetrace(ULONGLONG ElapsedTime)
{
    HANDLE ConsoleBufferHandle = NULL;

    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Set the vertical retrace cycle */
    VerticalRetraceCycle = GetCycleCount();

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

    /* Check if we are in text or graphics mode */
    if (ScreenMode == GRAPHICS_MODE)
    {
        /* Graphics mode */
        ConsoleBufferHandle = GraphicsConsoleBuffer;

        /* In DoubleVision mode, scale the update rectangle */
        if (DoubleWidth)
        {
            UpdateRectangle.Left *= 2;
            UpdateRectangle.Right = UpdateRectangle.Right * 2 + 1;
        }
        if (DoubleHeight)
        {
            UpdateRectangle.Top *= 2;
            UpdateRectangle.Bottom = UpdateRectangle.Bottom * 2 + 1;
        }
    }
    else
    {
        /* Text mode */
        ConsoleBufferHandle = TextConsoleBuffer;
    }

    /* Redraw the screen */
    __InvalidateConsoleDIBits(ConsoleBufferHandle, &UpdateRectangle);

    /* Clear the update flag */
    NeedsUpdate = FALSE;
}

static VOID FASTCALL VgaHorizontalRetrace(ULONGLONG ElapsedTime)
{
    UNREFERENCED_PARAMETER(ElapsedTime);

    /* Set the cycle */
    HorizontalRetraceCycle = GetCycleCount();
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
        /* Multiply the horizontal resolution by the 9/8 dot mode */
        Resolution.X *= (VgaSeqRegisters[VGA_SEQ_CLOCK_REG] & VGA_SEQ_CLOCK_98DM)
                        ? 8 : 9;

        /* The horizontal resolution is halved in 8-bit mode */
        if (VgaAcRegisters[VGA_AC_CONTROL_REG] & VGA_AC_CONTROL_8BIT) Resolution.X /= 2;
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

BOOLEAN VgaGetDoubleVisionState(PBOOLEAN Horizontal, PBOOLEAN Vertical)
{
    if (GraphicsConsoleBuffer == NULL) return FALSE;
    if (Horizontal) *Horizontal = DoubleWidth;
    if (Vertical)   *Vertical   = DoubleHeight;
    return TRUE;
}

VOID VgaRefreshDisplay(VOID)
{
    VgaVerticalRetrace(0);
}

VOID FASTCALL VgaReadMemory(ULONG Address, PVOID Buffer, ULONG Size)
{
    DWORD i, j;
    DWORD VideoAddress;
    PUCHAR BufPtr = (PUCHAR)Buffer;

    DPRINT("VgaReadMemory: Address 0x%08X, Size %lu\n", Address, Size);

    /* Ignore if video RAM access is disabled */
    if ((VgaMiscRegister & VGA_MISC_RAM_ENABLED) == 0) return;

    if (!(VgaGcRegisters[VGA_GC_MODE_REG] & VGA_GC_MODE_READ))
    {
        /* Loop through each byte */
        for (i = 0; i < Size; i++)
        {
            VideoAddress = VgaTranslateReadAddress(Address + i);

            /* Copy the value to the buffer */
            BufPtr[i] = VgaMemory[VideoAddress];
        }
    }
    else
    {
        /* Loop through each byte */
        for (i = 0; i < Size; i++)
        {
            BYTE Result = 0xFF;

            /* This should always return a plane 0 address for read mode 1 */
            VideoAddress = VgaTranslateReadAddress(Address + i);

            for (j = 0; j < VGA_NUM_BANKS; j++)
            {
                /* Don't consider ignored banks */
                if (!(VgaGcRegisters[VGA_GC_COLOR_IGNORE_REG] & (1 << j))) continue;

                if (VgaGcRegisters[VGA_GC_COLOR_COMPARE_REG] & (1 << j))
                {
                    /* Comparing with 11111111 */
                    Result &= VgaMemory[j * VGA_BANK_SIZE + LOWORD(VideoAddress)];
                }
                else
                {
                    /* Comparing with 00000000 */
                    Result &= ~(VgaMemory[j * VGA_BANK_SIZE + LOWORD(VideoAddress)]);
                }
            }

            /* Copy the value to the buffer */
            BufPtr[i] = Result;
        }
    }

    /* Load the latch registers */
    VgaLatchRegisters[0] = VgaMemory[LOWORD(VideoAddress)];
    VgaLatchRegisters[1] = VgaMemory[VGA_BANK_SIZE + LOWORD(VideoAddress)];
    VgaLatchRegisters[2] = VgaMemory[(2 * VGA_BANK_SIZE) + LOWORD(VideoAddress)];
    VgaLatchRegisters[3] = VgaMemory[(3 * VGA_BANK_SIZE) + LOWORD(VideoAddress)];
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

    /* Loop through each byte */
    for (i = 0; i < Size; i++)
    {
        VideoAddress = VgaTranslateWriteAddress(Address + i);

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
            VgaMemory[VideoAddress + j * VGA_BANK_SIZE] = VgaTranslateByteForWriting(BufPtr[i], j);
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
    PUCHAR FontMemory = (PUCHAR)&VgaMemory[VGA_BANK_SIZE * VGA_FONT_BANK + (FontNumber * VGA_FONT_SIZE)];

    ASSERT(Height <= VGA_MAX_FONT_HEIGHT);

    for (i = 0 ; i < VGA_FONT_CHARACTERS; i++)
    {
        /* Write the character */
        for (j = 0; j < Height; j++)
        {
            FontMemory[i * VGA_MAX_FONT_HEIGHT + j] = FontData[i * Height + j];
        }

        /* Clear the unused part */
        for (j = Height; j < VGA_MAX_FONT_HEIGHT; j++)
        {
            FontMemory[i * VGA_MAX_FONT_HEIGHT + j] = 0;
        }
    }
}

VOID ScreenEventHandler(PWINDOW_BUFFER_SIZE_RECORD ScreenEvent)
{
    DPRINT1("Screen events not handled\n");
}

BOOL VgaAttachToConsole(VOID)
{
    if (TextResolution.X == 0 || TextResolution.Y == 0)
        DPRINT1("VgaAttachToConsole -- TextResolution uninitialized\n");

    if (TextResolution.X == 0) TextResolution.X = 80;
    if (TextResolution.Y == 0) TextResolution.Y = 25;

    // VgaDetachFromConsoleInternal();

    /*
     * VgaAttachToConsoleInternal sets TextResolution
     * to the new resolution and updates ConsoleInfo.
     */
    if (!VgaAttachToConsoleInternal(&TextResolution))
    {
        DisplayMessage(L"An unexpected error occurred!\n");
        EmulatorTerminate();
        return FALSE;
    }

    /* Restore the original screen buffer */
    VgaSetActiveScreenBuffer(ScreenBufferHandle);
    ScreenBufferHandle = NULL;

    /* Restore the screen state */
    if (ScreenMode == TEXT_MODE)
    {
        /* The text mode framebuffer was recreated */
        ConsoleFramebuffer = TextFramebuffer;
    }
    else
    {
        /* The graphics mode framebuffer is unchanged */
        ConsoleFramebuffer = OldConsoleFramebuffer;
    }
    OldConsoleFramebuffer = NULL;

    return TRUE;
}

VOID VgaDetachFromConsole(VOID)
{
    BOOL Success;

    SMALL_RECT ConRect;

    VgaDetachFromConsoleInternal();

    /* Save the screen state */
    if (ScreenMode == TEXT_MODE)
        ScreenBufferHandle = TextConsoleBuffer;
    else
        ScreenBufferHandle = GraphicsConsoleBuffer;

    /* Reset the active framebuffer */
    OldConsoleFramebuffer = ConsoleFramebuffer;
    ConsoleFramebuffer = NULL;

    /* Restore the original console size */
    ConRect.Left   = ConRect.Top = 0;
    ConRect.Right  = ConRect.Left + OrgConsoleBufferInfo.srWindow.Right  - OrgConsoleBufferInfo.srWindow.Left;
    ConRect.Bottom = ConRect.Top  + OrgConsoleBufferInfo.srWindow.Bottom - OrgConsoleBufferInfo.srWindow.Top ;
    /* See the following trick explanation in VgaAttachToConsoleInternal */
    Success = SetConsoleScreenBufferSize(TextConsoleBuffer, OrgConsoleBufferInfo.dwSize);
    DPRINT1("(detach) SetConsoleScreenBufferSize(1) %s with error %d\n", Success ? "succeeded" : "failed", GetLastError());
    Success = SetConsoleWindowInfo(TextConsoleBuffer, TRUE, &ConRect);
    DPRINT1("(detach) SetConsoleWindowInfo %s with error %d\n", Success ? "succeeded" : "failed", GetLastError());
    Success = SetConsoleScreenBufferSize(TextConsoleBuffer, OrgConsoleBufferInfo.dwSize);
    DPRINT1("(detach) SetConsoleScreenBufferSize(2) %s with error %d\n", Success ? "succeeded" : "failed", GetLastError());

    /* Restore the original cursor shape */
    SetConsoleCursorInfo(TextConsoleBuffer, &OrgConsoleCursorInfo);

    // FIXME: Should we copy back the screen data to the screen buffer??
    // WriteConsoleOutputA(...);

    // FIXME: Should we change cursor POSITION??
    // VgaUpdateTextCursor();

    ///* Update the physical cursor */
    //SetConsoleCursorInfo(TextConsoleBuffer, &CursorInfo);
    //SetConsoleCursorPosition(TextConsoleBuffer, Position /*OrgConsoleBufferInfo.dwCursorPosition*/);

    /* Restore the old text-mode screen buffer */
    VgaSetActiveScreenBuffer(TextConsoleBuffer);
}

BOOLEAN VgaInitialize(HANDLE TextHandle)
{
    /* Save the default text-mode console output handle */
    if (!IsConsoleHandle(TextHandle)) return FALSE;
    TextConsoleBuffer = TextHandle;

    /* Save the original cursor and console screen buffer information */
    if (!GetConsoleCursorInfo(TextConsoleBuffer, &OrgConsoleCursorInfo) ||
        !GetConsoleScreenBufferInfo(TextConsoleBuffer, &OrgConsoleBufferInfo))
    {
        return FALSE;
    }
    ConsoleInfo = OrgConsoleBufferInfo;

    /* Clear the SEQ, GC, CRTC and AC registers */
    RtlZeroMemory(VgaSeqRegisters , sizeof(VgaSeqRegisters ));
    RtlZeroMemory(VgaGcRegisters  , sizeof(VgaGcRegisters  ));
    RtlZeroMemory(VgaCrtcRegisters, sizeof(VgaCrtcRegisters));
    RtlZeroMemory(VgaAcRegisters  , sizeof(VgaAcRegisters  ));

    /* Initialize the VGA palette and fail if it isn't successfully created */
    if (!VgaInitializePalette()) return FALSE;
    /***/ VgaResetPalette(); /***/

    /* Switch to the text buffer, but do not enter into a text mode */
    VgaSetActiveScreenBuffer(TextConsoleBuffer);

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
    VSyncTimer = CreateHardwareTimer(HARDWARE_TIMER_ENABLED, HZ_TO_NS(60), VgaVerticalRetrace);

    /* Return success */
    return TRUE;
}

VOID VgaCleanup(VOID)
{
    /* Do a final display refresh */
    VgaRefreshDisplay();

    DestroyHardwareTimer(VSyncTimer);
    DestroyHardwareTimer(HSyncTimer);

    /* Leave the current video mode */
    if (ScreenMode == GRAPHICS_MODE)
        VgaLeaveGraphicsMode();
    else
        VgaLeaveTextMode();

    VgaDetachFromConsole();
    MemRemoveFastMemoryHook((PVOID)0xA0000, 0x20000);

    CloseHandle(AnotherEvent);
    CloseHandle(EndEvent);
    CloseHandle(StartEvent);
}

/* EOF */
