/*
 * Copyright 2014 Michael Müller for Pipelight
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

#ifndef __WINE_HIGHLEVELMONITORCONFIGURATIONAPI_H
#define __WINE_HIGHLEVELMONITORCONFIGURATIONAPI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _MC_COLOR_TEMPERATURE
{
    MC_COLOR_TEMPERATURE_UNKNOWN,
    MC_COLOR_TEMPERATURE_4000K,
    MC_COLOR_TEMPERATURE_5000K,
    MC_COLOR_TEMPERATURE_6500K,
    MC_COLOR_TEMPERATURE_7500K,
    MC_COLOR_TEMPERATURE_8200K,
    MC_COLOR_TEMPERATURE_9300K,
    MC_COLOR_TEMPERATURE_10000K,
    MC_COLOR_TEMPERATURE_11500K
} MC_COLOR_TEMPERATURE, *LPMC_COLOR_TEMPERATURE;

typedef enum _MC_POSITION_TYPE
{
    MC_HORIZONTAL_POSITION,
    MC_VERTICAL_POSITION
} MC_POSITION_TYPE;

typedef enum _MC_SIZE_TYPE
{
    MC_WIDTH,
    MC_HEIGHT
} MC_SIZE_TYPE;

typedef enum _MC_DRIVE_TYPE
{
    MC_RED_DRIVE,
    MC_GREEN_DRIVE,
    MC_BLUE_DRIVE
} MC_DRIVE_TYPE;

typedef enum _MC_GAIN_TYPE
{
    MC_RED_GAIN,
    MC_GREEN_GAIN,
    MC_BLUE_GAIN
} MC_GAIN_TYPE;

typedef enum _MC_DISPLAY_TECHNOLOGY_TYPE
{
    MC_SHADOW_MASK_CATHODE_RAY_TUBE,
    MC_APERTURE_GRILL_CATHODE_RAY_TUBE,
    MC_THIN_FILM_TRANSISTOR,
    MC_LIQUID_CRYSTAL_ON_SILICON,
    MC_PLASMA,
    MC_ORGANIC_LIGHT_EMITTING_DIODE,
    MC_ELECTROLUMINESCENT,
    MC_MICROELECTROMECHANICAL,
    MC_FIELD_EMISSION_DEVICE
} MC_DISPLAY_TECHNOLOGY_TYPE, *LPMC_DISPLAY_TECHNOLOGY_TYPE;

#ifdef __cplusplus
}
#endif

#endif /* __WINE_HIGHLEVELMONITORCONFIGURATIONAPI_H */
