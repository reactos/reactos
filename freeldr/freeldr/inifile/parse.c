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
#include <ui.h>
#include <fs.h>
#include <rtl.h>
#include <mm.h>
#include <debug.h>


VOID IniParseFile(PUCHAR IniFileData, ULONG IniFileSize)
{
}

ULONG IniGetNextLineSize(PUCHAR IniFileData, ULONG IniFileSize, ULONG CurrentOffset)
{
	ULONG	Idx;
	ULONG	LineCharCount = 0;

	// Loop through grabbing chars until we hit the end of the
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

	// Check the first character (skipping whitespace)
	// and make sure that it is an opening bracket
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

	// Count the characters up until the closing bracket of EOL
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

	// Count the characters up until the closing bracket of EOL
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

}
