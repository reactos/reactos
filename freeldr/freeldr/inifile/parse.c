/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include "ini.h"
#include <rtl.h>
#include <mm.h>
#include <debug.h>


PINI_SECTION		IniFileSectionListHead = NULL;
ULONG				IniFileSectionCount = 0;
ULONG				IniFileSettingCount = 0;


BOOL IniParseFile(PUCHAR IniFileData, ULONG IniFileSize)
{
	ULONG				CurrentOffset;
	ULONG				CurrentLineNumber;
	PUCHAR				IniFileLine;
	ULONG				IniFileLineSize;
	ULONG				LineLength;
	PINI_SECTION		CurrentSection = NULL;
	PINI_SECTION_ITEM	CurrentItem = NULL;

	DbgPrint((DPRINT_INIFILE, "IniParseFile() IniFileSize: %d\n", IniFileSize));

	// Start with an 80-byte buffer
	IniFileLineSize = 80;
	IniFileLine = MmAllocateMemory(IniFileLineSize);
	if (!IniFileLine)
	{
		return FALSE;
	}

	// Loop through each line and parse it
	CurrentLineNumber = 0;
	CurrentOffset = 0;
	while (CurrentOffset < IniFileSize)
	{
		// First check the line size and increase our buffer if necessary
		if (IniFileLineSize < IniGetNextLineSize(IniFileData, IniFileSize, CurrentOffset))
		{
			IniFileLineSize = IniGetNextLineSize(IniFileData, IniFileSize, CurrentOffset);
			MmFreeMemory(IniFileLine);
			IniFileLine = MmAllocateMemory(IniFileLineSize);
			if (!IniFileLine)
			{
				return FALSE;
			}
		}

		// Get the line of data
		CurrentOffset = IniGetNextLine(IniFileData, IniFileSize, IniFileLine, IniFileLineSize, CurrentOffset);
		LineLength = strlen(IniFileLine);

		// If it is a blank line or a comment then skip it
		if (IniIsLineEmpty(IniFileLine, LineLength) || IniIsCommentLine(IniFileLine, LineLength))
		{
			CurrentLineNumber++;
			continue;
		}

		// Check if it is a new section
		if (IniIsSectionName(IniFileLine, LineLength))
		{
			// Allocate a new section structure
			CurrentSection = MmAllocateMemory(sizeof(INI_SECTION));
			if (!CurrentSection)
			{
				MmFreeMemory(IniFileLine);
				return FALSE;
			}

			RtlZeroMemory(CurrentSection, sizeof(INI_SECTION));

			// Allocate the section name buffer
			CurrentSection->SectionName = MmAllocateMemory(IniGetSectionNameSize(IniFileLine, LineLength));
			if (!CurrentSection->SectionName)
			{
				MmFreeMemory(CurrentSection);
				MmFreeMemory(IniFileLine);
				return FALSE;
			}

			// Get the section name
			IniExtractSectionName(CurrentSection->SectionName, IniFileLine, LineLength);

			// Add it to the section list head
			IniFileSectionCount++;
			if (IniFileSectionListHead == NULL)
			{
				IniFileSectionListHead = CurrentSection;
			}
			else
			{
				RtlListInsertTail((PLIST_ITEM)IniFileSectionListHead, (PLIST_ITEM)CurrentSection);
			}
			
			CurrentLineNumber++;
			continue;
		}

		// Check if it is a setting
		if (IniIsSetting(IniFileLine, LineLength))
		{
			// First check to make sure we're inside a [section]
			if (CurrentSection == NULL)
			{
				printf("Error: freeldr.ini:%d: Setting \'%s\' found outside of a [section].\n", CurrentLineNumber, IniFileLine);
				CurrentLineNumber++;
				continue;
			}

			// Allocate a new item structure
			CurrentItem = MmAllocateMemory(sizeof(INI_SECTION_ITEM));
			if (!CurrentItem)
			{
				MmFreeMemory(IniFileLine);
				return FALSE;
			}

			RtlZeroMemory(CurrentItem, sizeof(INI_SECTION_ITEM));

			// Allocate the setting name buffer
			CurrentItem->ItemName = MmAllocateMemory(IniGetSettingNameSize(IniFileLine, LineLength));
			if (!CurrentItem->ItemName)
			{
				MmFreeMemory(CurrentItem);
				MmFreeMemory(IniFileLine);
				return FALSE;
			}

			// Allocate the setting value buffer
			CurrentItem->ItemValue = MmAllocateMemory(IniGetSettingValueSize(IniFileLine, LineLength));
			if (!CurrentItem->ItemValue)
			{
				MmFreeMemory(CurrentItem);
				MmFreeMemory(IniFileLine);
				return FALSE;
			}

			// Get the section name
			IniExtractSettingName(CurrentItem->ItemName, IniFileLine, LineLength);
			IniExtractSettingValue(CurrentItem->ItemValue, IniFileLine, LineLength);

			// Add it to the current section
			IniFileSettingCount++;
			CurrentSection->SectionItemCount++;
			if (CurrentSection->SectionItemList == NULL)
			{
				CurrentSection->SectionItemList = CurrentItem;
			}
			else
			{
				RtlListInsertTail((PLIST_ITEM)CurrentSection->SectionItemList, (PLIST_ITEM)CurrentItem);
			}

			CurrentLineNumber++;
			continue;
		}

		CurrentLineNumber++;
	}

	DbgPrint((DPRINT_INIFILE, "Parsed %d sections and %d settings.\n", IniFileSectionCount, IniFileSettingCount));
	DbgPrint((DPRINT_INIFILE, "IniParseFile() done.\n"));

	return TRUE;
}

ULONG IniGetNextLineSize(PUCHAR IniFileData, ULONG IniFileSize, ULONG CurrentOffset)
{
	ULONG	Idx;
	ULONG	LineCharCount = 0;

	// Loop through counting chars until we hit the end of the
	// file or we encounter a new line char
	for (Idx=0; (CurrentOffset < IniFileSize); CurrentOffset++)
	{
		// Increment the line character count
		LineCharCount++;

		// Check for new line char
		if (IniFileData[CurrentOffset] == '\n')
		{
			CurrentOffset++;
			break;
		}
	}

	// Add one for the NULL-terminator
	LineCharCount++;

	// Send back line character count
	return LineCharCount;
}

ULONG IniGetNextLine(PUCHAR IniFileData, ULONG IniFileSize, PUCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset)
{
	ULONG	Idx;

	// Loop through grabbing chars until we hit the end of the
	// file or we encounter a new line char
	for (Idx=0; (CurrentOffset < IniFileSize); CurrentOffset++)
	{
		// If we haven't exceeded our buffer size yet
		// then store another char
		if (Idx < (BufferSize - 1))
		{
			Buffer[Idx++] = IniFileData[CurrentOffset];
		}

		// Check for new line char
		if (IniFileData[CurrentOffset] == '\n')
		{
			CurrentOffset++;
			break;
		}
	}

	// Terminate the string
	Buffer[Idx] = '\0';

	// Get rid of newline & linefeed characters (if any)
	if((Buffer[strlen(Buffer)-1] == '\n') || (Buffer[strlen(Buffer)-1] == '\r'))
		Buffer[strlen(Buffer)-1] = '\0';
	if((Buffer[strlen(Buffer)-1] == '\n') || (Buffer[strlen(Buffer)-1] == '\r'))
		Buffer[strlen(Buffer)-1] = '\0';

	// Send back new offset
	return CurrentOffset;
}

BOOL IniIsLineEmpty(PUCHAR LineOfText, ULONG TextLength)
{
	ULONG	Idx;

	// Check for text (skipping whitespace)
	for (Idx=0; Idx<TextLength; Idx++)
	{
		if ((LineOfText[Idx] == ' ') ||
			(LineOfText[Idx] == '\t') ||
			(LineOfText[Idx] == '\n') ||
			(LineOfText[Idx] == '\r'))
		{
			continue;
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL IniIsCommentLine(PUCHAR LineOfText, ULONG TextLength)
{
	ULONG	Idx;

	// Check the first character (skipping whitespace)
	// and make sure that it is an opening bracket
	for (Idx=0; Idx<TextLength; Idx++)
	{
		if ((LineOfText[Idx] == ' ') ||
			(LineOfText[Idx] == '\t'))
		{
			continue;
		}
		else if (LineOfText[Idx] == INI_FILE_COMMENT_CHAR)
		{
			return TRUE;
		}
		else
		{
			break;
		}
	}

	return FALSE;
}

BOOL IniIsSectionName(PUCHAR LineOfText, ULONG TextLength)
{
	ULONG	Idx;

	// Check the first character (skipping whitespace)
	// and make sure that it is an opening bracket
	for (Idx=0; Idx<TextLength; Idx++)
	{
		if ((LineOfText[Idx] == ' ') ||
			(LineOfText[Idx] == '\t'))
		{
			continue;
		}
		else if (LineOfText[Idx] == '[')
		{
			return TRUE;
		}
		else
		{
			break;
		}
	}

	return FALSE;
}

ULONG IniGetSectionNameSize(PUCHAR SectionNameLine, ULONG LineLength)
{
	ULONG	Idx;
	ULONG	NameSize;

	// Find the opening bracket (skipping whitespace)
	for (Idx=0; Idx<LineLength; Idx++)
	{
		if ((SectionNameLine[Idx] == ' ') ||
			(SectionNameLine[Idx] == '\t'))
		{
			continue;
		}
		else //if (SectionNameLine[Idx] == '[')
		{
			break;
		}
	}

	// Skip past the opening bracket
	Idx++;

	// Count the characters up until the closing bracket or EOL
	for (NameSize=0; Idx<LineLength; Idx++)
	{
		if ((SectionNameLine[Idx] == ']') ||
			(SectionNameLine[Idx] == '\0'))
		{
			break;
		}

		// Increment the count
		NameSize++;
	}

	// Add one for the NULL-terminator
	NameSize++;

	return NameSize;
}

VOID IniExtractSectionName(PUCHAR SectionName, PUCHAR SectionNameLine, ULONG LineLength)
{
	ULONG	Idx;
	ULONG	DestIdx;

	// Find the opening bracket (skipping whitespace)
	for (Idx=0; Idx<LineLength; Idx++)
	{
		if ((SectionNameLine[Idx] == ' ') ||
			(SectionNameLine[Idx] == '\t'))
		{
			continue;
		}
		else //if (SectionNameLine[Idx] == '[')
		{
			break;
		}
	}

	// Skip past the opening bracket
	Idx++;

	// Count the characters up until the closing bracket or EOL
	for (DestIdx=0; Idx<LineLength; Idx++)
	{
		if ((SectionNameLine[Idx] == ']') ||
			(SectionNameLine[Idx] == '\0'))
		{
			break;
		}

		// Grab a character and increment DestIdx
		SectionName[DestIdx] = SectionNameLine[Idx];
		DestIdx++;
	}

	// Terminate the string
	SectionName[DestIdx] = '\0';
}

BOOL IniIsSetting(PUCHAR LineOfText, ULONG TextLength)
{
	ULONG	Idx;

	// Basically just check for an '=' equals sign
	for (Idx=0; Idx<TextLength; Idx++)
	{
		if (LineOfText[Idx] == '=')
		{
			return TRUE;
		}
	}

	return FALSE;
}

ULONG IniGetSettingNameSize(PUCHAR SettingNameLine, ULONG LineLength)
{
	ULONG	Idx;
	ULONG	NameSize;

	// Skip whitespace
	for (Idx=0; Idx<LineLength; Idx++)
	{
		if ((SettingNameLine[Idx] == ' ') ||
			(SettingNameLine[Idx] == '\t'))
		{
			continue;
		}
		else
		{
			break;
		}
	}

	// Count the characters up until the '=' equals sign or EOL
	for (NameSize=0; Idx<LineLength; Idx++)
	{
		if ((SettingNameLine[Idx] == '=') ||
			(SettingNameLine[Idx] == '\0'))
		{
			break;
		}

		// Increment the count
		NameSize++;
	}

	// Add one for the NULL-terminator
	NameSize++;

	return NameSize;
}

ULONG IniGetSettingValueSize(PUCHAR SettingValueLine, ULONG LineLength)
{
	ULONG	Idx;
	ULONG	ValueSize;

	// Skip whitespace
	for (Idx=0; Idx<LineLength; Idx++)
	{
		if ((SettingValueLine[Idx] == ' ') ||
			(SettingValueLine[Idx] == '\t'))
		{
			continue;
		}
		else
		{
			break;
		}
	}

	// Skip the characters up until the '=' equals sign or EOL
	for (; Idx<LineLength; Idx++)
	{
		if (SettingValueLine[Idx] == '=')
		{
			Idx++;
			break;
		}

		// If we hit EOL then obviously the value size is zero
		if (SettingValueLine[Idx] == '\0')
		{
			return 0;
		}
	}

	// Count the characters up until the EOL
	for (ValueSize=0; Idx<LineLength; Idx++)
	{
		if (SettingValueLine[Idx] == '\0')
		{
			break;
		}

		// Increment the count
		ValueSize++;
	}

	// Add one for the NULL-terminator
	ValueSize++;

	return ValueSize;
}

VOID IniExtractSettingName(PUCHAR SettingName, PUCHAR SettingNameLine, ULONG LineLength)
{
	ULONG	Idx;
	ULONG	DestIdx;

	// Skip whitespace
	for (Idx=0; Idx<LineLength; Idx++)
	{
		if ((SettingNameLine[Idx] == ' ') ||
			(SettingNameLine[Idx] == '\t'))
		{
			continue;
		}
		else
		{
			break;
		}
	}

	// Get the characters up until the '=' equals sign or EOL
	for (DestIdx=0; Idx<LineLength; Idx++)
	{
		if ((SettingNameLine[Idx] == '=') ||
			(SettingNameLine[Idx] == '\0'))
		{
			break;
		}

		// Grab a character and increment DestIdx
		SettingName[DestIdx] = SettingNameLine[Idx];
		DestIdx++;
	}

	// Terminate the string
	SettingName[DestIdx] = '\0';
}

VOID IniExtractSettingValue(PUCHAR SettingValue, PUCHAR SettingValueLine, ULONG LineLength)
{
	ULONG	Idx;
	ULONG	DestIdx;

	// Skip whitespace
	for (Idx=0; Idx<LineLength; Idx++)
	{
		if ((SettingValueLine[Idx] == ' ') ||
			(SettingValueLine[Idx] == '\t'))
		{
			continue;
		}
		else
		{
			break;
		}
	}

	// Skip the characters up until the '=' equals sign or EOL
	for (; Idx<LineLength; Idx++)
	{
		if (SettingValueLine[Idx] == '=')
		{
			Idx++;
			break;
		}

		// If we hit EOL then obviously the value size is zero
		if (SettingValueLine[Idx] == '\0')
		{
			SettingValue[0] = '\0';
			return;
		}
	}

	// Get the characters up until the EOL
	for (DestIdx=0; Idx<LineLength; Idx++)
	{
		if (SettingValueLine[Idx] == '\0')
		{
			break;
		}

		// Grab a character and increment DestIdx
		SettingValue[DestIdx] = SettingValueLine[Idx];
		DestIdx++;
	}

	// Terminate the string
	SettingValue[DestIdx] = '\0';
}
