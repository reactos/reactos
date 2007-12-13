/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /* 2003-01-06  andy@warmcat.com  Created
 */
 
 // header for BootFlash.c

// callback events
// note that if you receive *_START, you will always receive *_END even if an error is detected
 typedef enum {
 	EE_ERASE_START=1,
	EE_ERASE_UPDATE,  // dwPos runs from 0 to dwExtent-1
	EE_ERASE_END,
	EE_ERASE_ERROR,  // dwPos indicates error offset from start of flash
	EE_PROGRAM_START,
	EE_PROGRAM_UPDATE,  // dwPos runs from 0 to dwExtent-1
	EE_PROGRAM_END,
	EE_PROGRAM_ERROR,  // dwPos indicates error offset from start of flash, b7..b0 = read data, b15..b8 = written data
	EE_VERIFY_START,
	EE_VERIFY_UPDATE,  // dwPos runs from 0 to dwExtent-1
	EE_VERIFY_END,
	EE_VERIFY_ERROR  // dwPos indicates error offset from start of flash, b7..b0 = read data, b15..b8 = written data
 } ENUM_EVENTS;

 	// callback typedef
typedef bool (*CALLBACK_FLASH)(void * pvoidObjectFlash, ENUM_EVENTS ee, DWORD dwPos, DWORD dwExtent);

typedef struct {
 	volatile BYTE * volatile m_pbMemoryMappedStartAddress; // fill on entry
	BYTE m_bManufacturerId;
	BYTE m_bDeviceId;
	char m_szFlashDescription[256];
 	char m_szAdditionalErrorInfo[256];
	DWORD m_dwLengthInBytes;
	DWORD m_dwStartOffset;
	DWORD m_dwLengthUsedArea;
	CALLBACK_FLASH m_pcallbackFlash;
	bool m_fDetectedUsing28xxxConventions;
	bool m_fIsBelievedCapableOfWriteAndErase;
} OBJECT_FLASH;


typedef struct {
	BYTE m_bManufacturerId;
	BYTE m_bDeviceId;
 	char m_szFlashDescription[32];
	DWORD m_dwLengthInBytes;
} KNOWN_FLASH_TYPE;

// requires pof->m_pbMemoryMappedStartAddress set to start address of flash in memory on entry

int BootReflashAndReset(BYTE *pbNewData, DWORD dwStartOffset, DWORD dwLength);
void BootReflashAndReset_RAM(BYTE *pbNewData, DWORD dwStartOffset, DWORD dwLength);

bool BootFlashGetDescriptor( OBJECT_FLASH *pof, KNOWN_FLASH_TYPE * pkft );
bool BootFlashEraseMinimalRegion( OBJECT_FLASH *pof);
bool BootFlashProgram( OBJECT_FLASH *pof, BYTE *pba );

