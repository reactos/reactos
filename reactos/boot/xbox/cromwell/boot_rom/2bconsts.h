/*
 *
 * includes for startup code in a form usable by the .S files
 *
 */

  /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define PCI_CFG_ADDR 0x0CF8
#define PCI_CFG_DATA 0x0CFC

#define MTRR_DEF 0x2ff
#define MTRR_DEF_TYPE 0x800
#define MTRR_PHYSBASE 0x200
#define MTRR_LAST 0x20F
#define WB_CACHE 6
#define BASE0_H 0
#define BASE0_L WB_CACHE
#define MASK0_H 0x0F
#define MASK0_L 0xFC000800
#define BASE1_H 0
#define BASE1_L 0xFFF80005
#define MASK1_H 0x0F
#define MASK1_L 0x0FFF80800

#define I2C_IO_BASE 0xc000

#define BUS_0 0
#define BUS_1 1

#define DEV_0 0
#define DEV_1 1
#define DEV_2 2
#define DEV_3 3
#define DEV_4 4
#define DEV_5 5
#define DEV_6 6
#define DEV_7 7
#define DEV_8 8
#define DEV_9 9
#define DEV_a 0xa
#define DEV_b 0xb
#define DEV_c 0xc
#define DEV_d 0xd
#define DEV_e 0xe
#define DEV_f 0xf
#define DEV_10 0x10
#define DEV_11 0x11
#define DEV_12 0x12
#define DEV_13 0x13
#define DEV_14 0x14
#define DEV_15 0x15
#define DEV_16 0x16
#define DEV_17 0x17
#define DEV_18 0x18
#define DEV_19 0x19
#define DEV_1a 0x1a
#define DEV_1b 0x1b
#define DEV_1c 0x1c
#define DEV_1d 0x1d
#define DEV_1e 0x1e
#define DEV_1f 0x1f

#define FUNC_0 0
