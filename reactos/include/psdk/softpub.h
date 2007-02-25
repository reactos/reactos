/*
 * Copyright (C) 2006 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_SOFTPUB_H
#define __WINE_SOFTPUB_H

#include <wintrust.h>

#define WINTRUST_ACTION_GENERIC_CERT_VERIFY \
    { 0x189a3842, 0x3041, 0x11d1, { 0x85,0xe1,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#define WINTRUST_ACTION_GENERIC_CHAIN_VERIFY \
    { 0xfc451c16, 0xac75, 0x11d1, { 0xb4,0xb8,0x00,0xc0,0x4f,0xb6,0x6e,0xa0 }}

#define WINTRUST_ACTION_GENERIC_VERIFY_V2 \
    { 0xaac56b,   0xcd44, 0x11d0, { 0x8c,0xc2,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#define WINTRUST_ACTION_TRUSTPROVIDER_TEST \
    { 0x573e31f8, 0xddba, 0x11d0, { 0x8c,0xcb,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#define OFFICESIGN_ACTION_VERIFY \
    { 0x5555c2cd, 0x17fb, 0x11d1, { 0x85,0xc4,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#define DRIVER_ACTION_VERIFY \
    { 0xf750e6c3, 0x38ee, 0x11d1, { 0x85,0xe5,0x00,0xc0,0x4f,0xc2,0x95,0xee }}

#endif /* __WINE_SOFTPUB_H */
