/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific definitions
 * COPYRIGHT:   Copyright 2021 Scott Maday <coldasdryice1@gmail.com>
 */
#pragma once

#define IEEE_802_ADDR_LENGTH 6

#define MAX_RESET_ATTEMPTS 10
#define MAX_EEPROM_READ_ATTEMPTS 10000

#define MAXIMUM_MULTICAST_ADDRESSES 16

/* Ethernet frame header */
typedef struct _ETH_HEADER
{
    UCHAR Destination[IEEE_802_ADDR_LENGTH];
    UCHAR Source[IEEE_802_ADDR_LENGTH];
    USHORT PayloadType;
} ETH_HEADER, *PETH_HEADER;
