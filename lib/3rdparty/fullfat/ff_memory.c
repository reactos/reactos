/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/

/**
 *	@file		ff_memory.c
 *	@author		James Walmsley
 *	@ingroup	MEMORY
 *
 *	@defgroup	MEMORY	FullFAT Memory Access Routines
 *	@brief		Handles memory access in a portable way.
 *
 *	Provides simple, fast, and portable access to memory routines.
 *	These are only used to read data from buffers. That are LITTLE ENDIAN
 *	due to the FAT specification.
 *
 *	These routines may need to be modified to your platform.
 *
 **/

#include "ff_memory.h"
#include "ff_config.h"

#ifdef FF_LITTLE_ENDIAN

/**
 *	@public
 *	@brief	8 bit memory access routines. 
 **/
/*
	These functions swap the byte-orders of shorts and longs. A getChar function is provided
	incase there is a system that doesn't have byte-wise access to all memory.

	These functions can be replaced with your own platform specific byte-order swapping routines
	for more efficiency.

	The provided functions should work on almost all platforms.
*/
FF_T_UINT8 FF_getChar(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset) {
	return (FF_T_UINT8) (pBuffer[offset]);
}

FF_T_UINT16 FF_getShort(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset) {
	return (FF_T_UINT16) (pBuffer[offset] & 0x00FF) | ((FF_T_UINT16) (pBuffer[offset+1] << 8) & 0xFF00);
}

FF_T_UINT32 FF_getLong(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset) {
	return (FF_T_UINT32) (pBuffer[offset] & 0x000000FF) | ((FF_T_UINT32) (pBuffer[offset+1] << 8) & 0x0000FF00) | ((FF_T_UINT32) (pBuffer[offset+2] << 16) & 0x00FF0000) | ((FF_T_UINT32) (pBuffer[offset+3] << 24) & 0xFF000000);
}

void FF_putChar(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset, FF_T_UINT8 Value) {
	pBuffer[offset] = Value;
}

void FF_putShort(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset, FF_T_UINT16 Value) {
	FF_T_UINT8 *Val		= (FF_T_UINT8 *) &Value;
	pBuffer[offset]		= Val[0];
	pBuffer[offset + 1] = Val[1];
}

void FF_putLong(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset, FF_T_UINT32 Value) {
	FF_T_UINT8 *Val		= (FF_T_UINT8 *) &Value;
	pBuffer[offset]		= Val[0];
	pBuffer[offset + 1] = Val[1];
	pBuffer[offset + 2] = Val[2];
	pBuffer[offset + 3] = Val[3];
}

#endif

#ifdef FF_BIG_ENDIAN
/*
	These haven't been tested or checked. They should work in theory :)
	Please contact james@worm.me.uk if they don't work, and also any fix.
*/
FF_T_UINT8 FF_getChar(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset) {
        return (FF_T_UINT8) (pBuffer[offset]);
}

FF_T_UINT16 FF_getShort(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset) {
        return (FF_T_UINT16) ((pBuffer[offset] & 0xFF00)  << 8) | ((FF_T_UINT16) (pBuffer[offset+1]) & 0x00FF);
}

FF_T_UINT32 FF_getLong(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset) {
        return (FF_T_UINT32) ((pBuffer[offset] << 24) & 0xFF0000) | ((FF_T_UINT32) (pBuffer[offset+1] << 16) & 0x00FF0000) | ((FF_T_UINT32) (pBuffer[offset+2] << 8) & 0x0000FF00) | ((FF_T_UINT32) (pBuffer[offset+3]) & 0x000000FF);
}

void FF_putChar(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset, FF_T_UINT8 Value) {
	pBuffer[offset] = Value;
}

void FF_putShort(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset, FF_T_UINT16 Value) {
	FF_T_UINT8 *Val		= (FF_T_UINT8 *) &Value;
	pBuffer[offset]		= Val[1];
	pBuffer[offset + 1] = Val[0];
}

void FF_putLong(FF_T_UINT8 *pBuffer, FF_T_UINT16 offset, FF_T_UINT32 Value) {
    FF_T_UINT8 *Val		= (FF_T_UINT8 *) &Value;
	pBuffer[offset]		= Val[3];
	pBuffer[offset + 1] = Val[2];
	pBuffer[offset + 2] = Val[1];
	pBuffer[offset + 3] = Val[0];
}
#endif


