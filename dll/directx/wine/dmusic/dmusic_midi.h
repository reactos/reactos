/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "stdarg.h"
#include "stddef.h"

#include "windef.h"
#include "winbase.h"

enum midi_message
{
    MIDI_NOTE_OFF = 0x80,
    MIDI_NOTE_ON = 0x90,
    MIDI_KEY_PRESSURE = 0xa0,
    MIDI_CONTROL_CHANGE = 0xb0,
    MIDI_PROGRAM_CHANGE = 0xc0,
    MIDI_CHANNEL_PRESSURE = 0xd0,
    MIDI_PITCH_BEND_CHANGE = 0xe0,
    MIDI_SYSEX1 = 0xf0,
    MIDI_SYSEX2 = 0xf7,
    MIDI_SYSTEM_RESET = 0xff,
    MIDI_META = 0xff,
};

enum midi_control
{
    MIDI_CC_BANK_MSB = 0x00,
    MIDI_CC_BANK_LSB = 0x20,
};
