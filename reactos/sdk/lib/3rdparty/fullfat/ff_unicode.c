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
 *	@file		ff_unicode.c
 *	@author		James Walmsley
 *	@ingroup	UNICODE
 *
 *	@defgroup	UNICODE	FullFAT UNICODE Library
 *	@brief		Portable UNICODE Transformation Library for FullFAT
 *
 **/

#include "ff_unicode.h"
#include "string.h"

// UTF-8 Routines

/*
   UCS-4 range (hex.)           UTF-8 octet sequence (binary)
   0000 0000-0000 007F   0xxxxxxx
   0000 0080-0000 07FF   110xxxxx 10xxxxxx
   0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx

   0001 0000-001F FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
   0020 0000-03FF FFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx	-- We don't encode these because we won't receive them. (Invalid UNICODE).
   0400 0000-7FFF FFFF   1111110x 10xxxxxx ... 10xxxxxx					-- We don't encode these because we won't receive them. (Invalid UNICODE).
*/

FF_T_UINT FF_GetUtf16SequenceLen(FF_T_UINT16 usLeadChar) {
	if((usLeadChar & 0xFC00) == 0xD800) {
		return 2;
	}
	return 1;
}

/*
	Returns the number of UTF-8 units read.
	Will not exceed ulSize UTF-16 units. (ulSize * 2 bytes).
*/
/*
   UCS-4 range (hex.)           UTF-8 octet sequence (binary)
   0000 0000-0000 007F   0xxxxxxx
   0000 0080-0000 07FF   110xxxxx 10xxxxxx
   0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx

   0001 0000-001F FFFF   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
   0020 0000-03FF FFFF   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx	-- We don't encode these because we won't receive them. (Invalid UNICODE).
   0400 0000-7FFF FFFF   1111110x 10xxxxxx ... 10xxxxxx					-- We don't encode these because we won't receive them. (Invalid UNICODE).
*/
FF_T_SINT32 FF_Utf8ctoUtf16c(FF_T_UINT16 *utf16Dest, const FF_T_UINT8 *utf8Source, FF_T_UINT32 ulSize) {
	FF_T_UINT32			ulUtf32char;
	FF_T_UINT16			utf16Source = 0;
	register FF_T_INT	uiSequenceNumber = 0;

	while((*utf8Source & (0x80 >> (uiSequenceNumber)))) {	// Count number of set bits before a zero.
		uiSequenceNumber++;
	}

	if(!uiSequenceNumber) {
		uiSequenceNumber++;
	}

	if(!ulSize) {
		return FF_ERR_UNICODE_DEST_TOO_SMALL;		
	}

	switch(uiSequenceNumber) {
		case 1:
                        utf16Source = (FF_T_UINT16) *utf8Source;
                        memcpy(utf16Dest,&utf16Source,sizeof(FF_T_UINT16));
			//bobtntfullfat *utf16Dest = (FF_T_UINT16) *utf8Source;
			break;

		case 2:
                        utf16Source =(FF_T_UINT16) ((*utf8Source & 0x1F) << 6) | ((*(utf8Source + 1) & 0x3F));
                        memcpy(utf16Dest,&utf16Source,sizeof(FF_T_UINT16));
			//bobtntfullfat *utf16Dest = (FF_T_UINT16) ((*utf8Source & 0x1F) << 6) | ((*(utf8Source + 1) & 0x3F));
			break;

		case 3:
                        utf16Source =(FF_T_UINT16) ((*utf8Source & 0x0F) << 12) | ((*(utf8Source + 1) & 0x3F) << 6) | ((*(utf8Source + 2) & 0x3F));
                        memcpy(utf16Dest,&utf16Source,sizeof(FF_T_UINT16));
			//bobtntfullfat *utf16Dest = (FF_T_UINT16) ((*utf8Source & 0x0F) << 12) | ((*(utf8Source + 1) & 0x3F) << 6) | ((*(utf8Source + 2) & 0x3F));
			break;

		case 4:
			// Convert to UTF-32 and then into UTF-16
			if(ulSize < 2) {
				return FF_ERR_UNICODE_DEST_TOO_SMALL;
			}
			ulUtf32char = (FF_T_UINT16) ((*utf8Source & 0x0F) << 18) | ((*(utf8Source + 1) & 0x3F) << 12) | ((*(utf8Source + 2) & 0x3F) << 6) | ((*(utf8Source + 3) & 0x3F));
                        
                        utf16Source = (FF_T_UINT16) (((ulUtf32char - 0x10000) & 0xFFC00) >> 10) | 0xD800;
                        memcpy(utf16Dest,&utf16Source,sizeof(FF_T_UINT16));                        
                        utf16Source = (FF_T_UINT16) (((ulUtf32char - 0x10000) & 0x003FF) >> 00) | 0xDC00;
                        memcpy(utf16Dest+1,&utf16Source,sizeof(FF_T_UINT16));                                                
			//bobtntfullfat *(utf16Dest + 0) = (FF_T_UINT16) (((ulUtf32char - 0x10000) & 0xFFC00) >> 10) | 0xD800;
			//bobtntfullfat *(utf16Dest + 1) = (FF_T_UINT16) (((ulUtf32char - 0x10000) & 0x003FF) >> 00) | 0xDC00;
			break;

		default:
			break;
	}

	return uiSequenceNumber;
}


/*
	Returns the number of UTF-8 units required to encode the UTF-16 sequence.
	Will not exceed ulSize UTF-8 units. (ulSize  * 1 bytes).
*/
FF_T_SINT32 FF_Utf16ctoUtf8c(FF_T_UINT8 *utf8Dest, const FF_T_UINT16 *utf16Source, FF_T_UINT32 ulSize) {
	FF_T_UINT32	ulUtf32char;
	FF_T_UINT16	ulUtf16char;

	if(!ulSize) {
		return FF_ERR_UNICODE_DEST_TOO_SMALL;
	}

        memcpy(&ulUtf16char, utf16Source, sizeof(FF_T_UINT16));
	if((/*bobtntfullfat *utf16Source*/ulUtf16char & 0xF800) == 0xD800) {	// A surrogate sequence was encountered. Must transform to UTF32 first.
                ulUtf32char  = ((FF_T_UINT32) (ulUtf16char & 0x003FF) << 10) + 0x10000;
		//bobtntfullfat ulUtf32char  = ((FF_T_UINT32) (*(utf16Source + 0) & 0x003FF) << 10) + 0x10000;

                memcpy(&ulUtf16char, utf16Source + 1, sizeof(FF_T_UINT16));                
		if((/*bobtntfullfat *(utf16Source + 1)*/ulUtf16char & 0xFC00) != 0xDC00) {
			return FF_ERR_UNICODE_INVALID_SEQUENCE;	// Invalid UTF-16 sequence.
		}
		ulUtf32char |= ((FF_T_UINT32) (/*bobtntfullfat *(utf16Source + 1)*/ulUtf16char & 0x003FF));

	} else {
		ulUtf32char = (FF_T_UINT32) /*bobtntfullfat *utf16Source*/ulUtf16char;
	}

	// Now convert to the UTF-8 sequence.
	if(ulUtf32char < 0x00000080) {	// Single byte UTF-8 sequence.
		*(utf8Dest + 0) = (FF_T_UINT8) ulUtf32char;
		return 1;
	}

	if(ulUtf32char < 0x00000800) {	// Double byte UTF-8 sequence.
		if(ulSize < 2) {
			return FF_ERR_UNICODE_DEST_TOO_SMALL;
		}
		*(utf8Dest + 0) = (FF_T_UINT8) (0xC0 | ((ulUtf32char >> 6) & 0x1F));
		*(utf8Dest + 1) = (FF_T_UINT8) (0x80 | ((ulUtf32char >> 0) & 0x3F));
		return 2;
	}

	if(ulUtf32char < 0x00010000) {	// Triple byte UTF-8 sequence.
		if(ulSize < 3) {
			return FF_ERR_UNICODE_DEST_TOO_SMALL;
		}
		*(utf8Dest + 0) = (FF_T_UINT8) (0xE0 | ((ulUtf32char >> 12) & 0x0F));
		*(utf8Dest + 1) = (FF_T_UINT8) (0x80 | ((ulUtf32char >> 6 ) & 0x3F));
		*(utf8Dest + 2) = (FF_T_UINT8) (0x80 | ((ulUtf32char >> 0 ) & 0x3F));
		return 3;
	}

	if(ulUtf32char < 0x00200000) {	// Quadruple byte UTF-8 sequence.
		if(ulSize < 4) {
			return FF_ERR_UNICODE_DEST_TOO_SMALL;
		}
		*(utf8Dest + 0) = (FF_T_UINT8) (0xF0 | ((ulUtf32char >> 18) & 0x07));
		*(utf8Dest + 1) = (FF_T_UINT8) (0x80 | ((ulUtf32char >> 12) & 0x3F));
		*(utf8Dest + 2) = (FF_T_UINT8) (0x80 | ((ulUtf32char >> 6 ) & 0x3F));
		*(utf8Dest + 3) = (FF_T_UINT8) (0x80 | ((ulUtf32char >> 0 ) & 0x3F));
		return 4;
	}

	return FF_ERR_UNICODE_INVALID_CODE;	// Invalid Charachter
}


// UTF-16 Support Functions

// Converts a UTF-32 Charachter into its equivalent UTF-16 sequence.
FF_T_SINT32 FF_Utf32ctoUtf16c(FF_T_UINT16 *utf16Dest, FF_T_UINT32 utf32char, FF_T_UINT32 ulSize) {

	// Check that its a valid UTF-32 wide-char!	

	if(utf32char >= 0xD800 && utf32char <= 0xDFFF) {	// This range is not a valid Unicode code point.
		return FF_ERR_UNICODE_INVALID_CODE; // Invalid charachter.
	}

	if(utf32char < 0x10000) {
		*utf16Dest = (FF_T_UINT16) utf32char; // Simple conversion! Char comes within UTF-16 space (without surrogates).
		return 1;
	}

	if(ulSize < 2) {
		return FF_ERR_UNICODE_DEST_TOO_SMALL;	// Not enough UTF-16 units to record this charachter.
	}

	if(utf32char < 0x00200000) {
		// Conversion to a UTF-16 Surrogate pair!
		//valueImage = utf32char - 0x10000;
		
		*(utf16Dest + 0) = (FF_T_UINT16) (((utf32char - 0x10000) & 0xFFC00) >> 10) | 0xD800;
		*(utf16Dest + 1) = (FF_T_UINT16) (((utf32char - 0x10000) & 0x003FF) >> 00) | 0xDC00;
		
		return 2;	// Surrogate pair encoded value.
	}
	
	return FF_ERR_UNICODE_INVALID_CODE;	// Invalid Charachter
}

// Converts a UTF-16 sequence into its equivalent UTF-32 code point.
FF_T_SINT32 FF_Utf16ctoUtf32c(FF_T_UINT32 *utf32Dest, const FF_T_UINT16 *utf16Source) {
	
	if((*utf16Source & 0xFC00) != 0xD800) {	// Not a surrogate sequence.
		*utf32Dest = (FF_T_UINT32) *utf16Source;
		return 1;	// A single UTF-16 item was used to represent the charachter.
	}
	
	*utf32Dest  = ((FF_T_UINT32) (*(utf16Source + 0) & 0x003FF) << 10) + 0x10000;
	
	if((*(utf16Source + 1) & 0xFC00) != 0xDC00) {
		return FF_ERR_UNICODE_INVALID_SEQUENCE;	// Invalid UTF-16 sequence.
	}
	*utf32Dest |= ((FF_T_UINT32) (*(utf16Source + 1) & 0x003FF));
	return 2;	// 2 utf-16 units make up the Unicode code-point.
}


/*
	Returns the total number of UTF-16 items required to represent
	the provided UTF-32 string in UTF-16 form.
*/
/*
FF_T_UINT FF_Utf32GetUtf16Len(const FF_T_UINT32 *utf32String) {
	FF_T_UINT utf16len = 0;

	while(*utf32String) {
		if(*utf32String++ <= 0xFFFF) {
			utf16len++;
		} else {
			utf16len += 2;
		}
	}
	
	return utf16len;
}*/


// String conversions

FF_T_SINT32 FF_Utf32stoUtf8s(FF_T_UINT8 *Utf8String, FF_T_UINT32 *Utf32String) {
	int i = 0,y = 0;

	FF_T_UINT16 utf16buffer[2];

	while(Utf32String[i]) {
		// Convert to a UTF16 char.
		FF_Utf32ctoUtf16c(utf16buffer, Utf32String[i], 2);
		// Now convert the UTF16 to UTF8 sequence.
		y += FF_Utf16ctoUtf8c(&Utf8String[y], utf16buffer, 4);
		i++;
	}

	Utf8String[y] = '\0';

	return 0;
}
