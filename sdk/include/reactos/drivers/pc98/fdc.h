/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NEC uPD765A FDC header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define FDC1_IO_BASE             0x90
#define FDC2_IO_BASE             0xC8

#define FDC_IO_o_MODE_SWITCH     0xBE
#define FDC_IO_o_EMODE_SWITCH    0x4BE
#define FDC_IO_i_MODE            0xBE
#define FDC_IO_i_EMODE           0x4BE

/*
 * FDC registers offsets
 */
#define FDC_o_DATA           0x02
#define FDC_o_CONTROL        0x04

#define FDC_i_STATUS         0x00
#define FDC_i_DATA           0x02
#define FDC_i_READ_SWITCH    0x04
