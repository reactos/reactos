/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NEC uPD1990A/uPD4990A RTC header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define RTC_IO_o_DATA    0x20
    /* Input terminals */
    #define RTC_DATA_INPUT                0x20
    #define RTC_CLOCK                     0x10
    #define RTC_STROBE                    0x08
    /* Commands, shift register 40 bit */
    #define RTC_CMD_REGISTER_HOLD         0x00
    #define RTC_CMD_REGISTER_SHIFT        0x01
    #define RTC_CMD_TIME_SET_COUNTER_HOLD 0x02
    #define RTC_CMD_TIME_READ             0x03
    #define RTC_CMD_TIMING_PULSE_64_HZ    0x04
    #define RTC_CMD_TIMING_PULSE_256_HZ   0x05
    #define RTC_CMD_TIMING_PULSE_2048_HZ  0x06
    #define RTC_CMD_SERIAL_TRANSFER_MODE  0x07
    /* Serial data commands, shift register 52 bit (uPD4990A only) */
    #define RTC_CMD_TIMING_PULSE_4096_HZ  0x07
    #define RTC_CMD_TIMING_PULSE_1_S_INT  0x08
    #define RTC_CMD_TIMING_PULSE_10_S_INT 0x09
    #define RTC_CMD_TIMING_PULSE_30_S_INT 0x0A
    #define RTC_CMD_TIMING_PULSE_60_S_INT 0x0B
    #define RTC_CMD_INTERRUPT_RESET       0x0C
    #define RTC_CMD_INTERRUPT_START       0x0D
    #define RTC_CMD_INTERRUPT_STOP        0x0E
    #define RTC_CMD_TEST_MODE             0x0F

#define RTC_IO_o_MODE    0x22

#define RTC_IO_o_INT_CLOCK_DIVISOR    0x128
    #define RTC_INT_CLOCK_DIVISOR_64      0x00
    #define RTC_INT_CLOCK_DIVISOR_32      0x01
    #define RTC_INT_CLOCK_DIVISOR_0       0x02
    #define RTC_INT_CLOCK_DIVISOR_16      0x03

#define RTC_IO_i_MODE    0x22
#define RTC_IO_i_INTERRUPT_RESET      0x128
