/*
 * Copyright 2020 Jactry Zeng for CodeWeavers
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

#ifndef AXEXTEND_ENUM_H
#define AXEXTEND_ENUM_H

typedef enum tagAnalogVideoStandard
{
    AnalogVideo_None     = 0x00000000,
    AnalogVideo_NTSC_M   = 0x00000001,
    AnalogVideo_NTSC_M_J = 0x00000002,
    AnalogVideo_NTSC_433 = 0x00000004,
    AnalogVideo_PAL_B    = 0x00000010,
    AnalogVideo_PAL_D    = 0x00000020,
    AnalogVideo_PAL_G    = 0x00000040,
    AnalogVideo_PAL_H    = 0x00000080,
    AnalogVideo_PAL_I    = 0x00000100,
    AnalogVideo_PAL_M    = 0x00000200,
    AnalogVideo_PAL_N    = 0x00000400,
    AnalogVideo_PAL_60   = 0x00000800,
    AnalogVideo_SECAM_B  = 0x00001000,
    AnalogVideo_SECAM_D  = 0x00002000,
    AnalogVideo_SECAM_G  = 0x00004000,
    AnalogVideo_SECAM_H  = 0x00008000,
    AnalogVideo_SECAM_K  = 0x00010000,
    AnalogVideo_SECAM_K1 = 0x00020000,
    AnalogVideo_SECAM_L  = 0x00040000,
    AnalogVideo_SECAM_L1 = 0x00080000,
    AnalogVideo_PAL_N_COMBO = 0x00100000,
    AnalogVideoMask_MCE_NTSC = AnalogVideo_NTSC_M | AnalogVideo_NTSC_M_J | AnalogVideo_NTSC_433 | AnalogVideo_PAL_M | AnalogVideo_PAL_N | AnalogVideo_PAL_60 | AnalogVideo_PAL_N_COMBO,
    AnalogVideoMask_MCE_PAL = AnalogVideo_PAL_B | AnalogVideo_PAL_D | AnalogVideo_PAL_G | AnalogVideo_PAL_H | AnalogVideo_PAL_I,
    AnalogVideoMask_MCE_SECAM = AnalogVideo_SECAM_B | AnalogVideo_SECAM_D  | AnalogVideo_SECAM_G |AnalogVideo_SECAM_H |AnalogVideo_SECAM_K | AnalogVideo_SECAM_K1 |AnalogVideo_SECAM_L | AnalogVideo_SECAM_L1,
}AnalogVideoStandard;

typedef enum tagTunerInputType
{
    TunerInputCable,
    TunerInputAntenna
} TunerInputType;

#endif /* AXEXTEND_ENUM_H */
