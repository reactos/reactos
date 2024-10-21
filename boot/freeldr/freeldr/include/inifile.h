/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#define INI_FILE_COMMENT_CHAR    ';'

#define TAG_INI_FILE 'FinI'
#define TAG_INI_SECTION 'SinI'
#define TAG_INI_SECTION_ITEM 'IinI'
#define TAG_INI_NAME 'NinI'
#define TAG_INI_VALUE 'VinI'

// This structure describes a single .ini file item
// The item format in the .ini file is:
// Name=Value
typedef struct
{
    LIST_ENTRY    ListEntry;
    PCHAR        ItemName;
    PCHAR        ItemValue;

} INI_SECTION_ITEM, *PINI_SECTION_ITEM;

// This structure describes a .ini file section
// The section format in the .ini file is:
// [Section Name]
// This structure has a list of section items with
// one INI_SECTION_ITEM for each line in the section
typedef struct
{
    LIST_ENTRY            ListEntry;
    PCHAR                SectionName;
    ULONG                    SectionItemCount;
    LIST_ENTRY    SectionItemList; // Contains PINI_SECTION_ITEM structures

} INI_SECTION, *PINI_SECTION;

extern    LIST_ENTRY        IniFileSectionListHead;
extern    BOOLEAN            IniFileSectionInitialized;
extern    ULONG                    IniFileSectionCount;
extern    ULONG                    IniFileSettingCount;

BOOLEAN    IniParseFile(PCHAR IniFileData, ULONG IniFileSize);
ULONG        IniGetNextLineSize(PCHAR IniFileData, ULONG IniFileSize, ULONG CurrentOffset);
ULONG        IniGetNextLine(PCHAR IniFileData, ULONG IniFileSize, PCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset);
BOOLEAN    IniIsLineEmpty(PCHAR LineOfText, ULONG TextLength);
BOOLEAN    IniIsCommentLine(PCHAR LineOfText, ULONG TextLength);
BOOLEAN    IniIsSectionName(PCHAR LineOfText, ULONG TextLength);
ULONG        IniGetSectionNameSize(PCHAR SectionNameLine, ULONG LineLength);
VOID    IniExtractSectionName(PCHAR SectionName, PCHAR SectionNameLine, ULONG LineLength);
BOOLEAN    IniIsSetting(PCHAR LineOfText, ULONG TextLength);
ULONG        IniGetSettingNameSize(PCHAR SettingNameLine, ULONG LineLength);
ULONG        IniGetSettingValueSize(PCHAR SettingValueLine, ULONG LineLength);
VOID    IniExtractSettingName(PCHAR SettingName, PCHAR SettingNameLine, ULONG LineLength);
VOID    IniExtractSettingValue(PCHAR SettingValue, PCHAR SettingValueLine, ULONG LineLength);

BOOLEAN    IniFileInitialize(VOID);

BOOLEAN    IniOpenSection(PCSTR SectionName, ULONG_PTR* SectionId);
ULONG        IniGetNumSectionItems(ULONG_PTR SectionId);
ULONG        IniGetSectionSettingNameSize(ULONG_PTR SectionId, ULONG SettingIndex);
ULONG        IniGetSectionSettingValueSize(ULONG_PTR SectionId, ULONG SettingIndex);
BOOLEAN    IniReadSettingByNumber(ULONG_PTR SectionId, ULONG SettingNumber, PCHAR SettingName, ULONG NameSize, PCHAR SettingValue, ULONG ValueSize);
BOOLEAN    IniReadSettingByName(ULONG_PTR SectionId, PCSTR SettingName, PCHAR Buffer, ULONG BufferSize);
BOOLEAN    IniAddSection(PCSTR SectionName, ULONG_PTR* SectionId);
BOOLEAN    IniAddSettingValueToSection(ULONG_PTR SectionId, PCSTR SettingName, PCSTR SettingValue);
BOOLEAN IniModifySettingValue(ULONG_PTR SectionId, PCSTR SettingName, PCSTR SettingValue);
VOID IniCleanup(VOID);
PLIST_ENTRY IniGetFileSectionListHead(VOID);
