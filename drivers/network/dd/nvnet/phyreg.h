/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PHY layer register definitions
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/* IEEE 802.3 */
#define MII_CONTROL              0x00
#define     MII_CR_AUTONEG_RESTART  0x0200
#define     MII_CR_POWER_DOWN       0x0800
#define     MII_CR_AUTONEG          0x1000
#define     MII_CR_RESET            0x8000
#define MII_STATUS               0x01
#define     MII_SR_LINK_STATUS      0x0004
#define     MII_SR_AUTONEG_COMPLETE 0x0020
#define MII_PHY_ID1              0x02
#define MII_PHY_ID2              0x03
#define MII_AUTONEG_ADVERTISE    0x04
#define     MII_ADV_10T_HD          0x0020
#define     MII_ADV_10T_FD          0x0040
#define     MII_ADV_100T_HD         0x0080
#define     MII_ADV_100T_FD         0x0100
#define     MII_ADV_100T4           0x0200
#define     MII_ADV_PAUSE_SYM       0x0400
#define     MII_ADV_PAUSE_ASYM      0x0800
#define MII_AUTONEG_LINK_PARTNER 0x05
#define     MII_LP_10T_HD           0x0020
#define     MII_LP_10T_FD           0x0040
#define     MII_LP_100T_HD          0x0080
#define     MII_LP_100T_FD          0x0100
#define     MII_LP_PAUSE_SYM        0x0400
#define     MII_LP_PAUSE_ASYM       0x0800
#define MII_AUTONEG_EXPANSION    0x06
#define     MII_EXP_LP_AUTONEG      0x0001
#define MII_MASTER_SLAVE_CONTROL 0x09
#define     MII_MS_CR_1000T_HD      0x0100
#define     MII_MS_CR_1000T_FD      0x0200
#define MII_MASTER_SLAVE_STATUS  0x0A
#define     MII_MS_SR_1000T_FD      0x0800
