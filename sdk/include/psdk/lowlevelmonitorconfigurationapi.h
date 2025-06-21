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

#ifndef __WINE_LOWLEVELMONITORCONFIGURATIONAPI_H
#define __WINE_LOWLEVELMONITORCONFIGURATIONAPI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MC_TIMING_REPORT
{
    BYTE bTimingStatusByte;
    DWORD dwHorizontalFrequencyInHZ;
    DWORD dwVerticalFrequencyInHZ;
} MC_TIMING_REPORT, *LPMC_TIMING_REPORT;

typedef enum _MC_VCP_CODE_TYPE
{
    MC_MOMENTARY,
    MC_SET_PARAMETER
} MC_VCP_CODE_TYPE, *LPMC_VCP_CODE_TYPE;

#ifdef __cplusplus
}
#endif

#endif /* __WINE_LOWLEVELMONITORCONFIGURATIONAPI_H */
