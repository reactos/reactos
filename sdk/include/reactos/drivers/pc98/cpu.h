/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CPU I/O ports header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define CPU_IO_o_RESET          0xF0
#define CPU_IO_o_A20_UNMASK     0xF2

#define CPU_IO_o_A20_CONTROL    0xF6
    #define CPU_A20_ENABLE          0x02
    #define CPU_A20_DISABLE         0x03

#define CPU_IO_o_FPU_BUSY_LATCH 0xF8

/*
 * ARTIC (A Relative Time Indication Counter) - 24-bit binary up counter
 */
#define CPU_IO_o_ARTIC_DELAY    0x5F /* Constant delay (about 600 ns) */
#define CPU_IO_i_ARTIC_0        0x5C
#define CPU_IO_i_ARTIC_1        0x5D
#define CPU_IO_i_ARTIC_2        0x5E
#define CPU_IO_i_ARTIC_3        0x5F

#define ARTIC_FREQUENCY        307200
#define ARTIC_FREQUENCY_0_1    ARTIC_FREQUENCY
#define ARTIC_FREQUENCY_2_3    (ARTIC_FREQUENCY >> 8)
