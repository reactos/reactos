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

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(INIFILE);

PLIST_ENTRY IniGetFileSectionListHead(VOID)
{
    return &IniFileSectionListHead;
}

BOOLEAN IniOpenSection(PCSTR SectionName, ULONG_PTR* SectionId)
{
    PLIST_ENTRY Entry;
    PINI_SECTION Section;

    TRACE("IniOpenSection() SectionName = %s\n", SectionName);

    // Loop through each section and find the one we want
    for (Entry = IniFileSectionListHead.Flink;
         Entry != &IniFileSectionListHead;
         Entry = Entry->Flink)
    {
        Section = CONTAINING_RECORD(Entry, INI_SECTION, ListEntry);

        // Compare against the section name
        if (_stricmp(SectionName, Section->SectionName) == 0)
        {
            // We found it
            if (SectionId)
                *SectionId = (ULONG_PTR)Section;
            TRACE("IniOpenSection() Found it! SectionId = 0x%x\n", SectionId);
            return TRUE;
        }
    }

    TRACE("IniOpenSection() Section not found.\n");

    return FALSE;
}

ULONG IniGetNumSectionItems(ULONG_PTR SectionId)
{
    PINI_SECTION Section = (PINI_SECTION)SectionId;

    TRACE("IniGetNumSectionItems() SectionId = 0x%x\n", SectionId);
    TRACE("IniGetNumSectionItems() Item count = %d\n", Section->SectionItemCount);

    return Section->SectionItemCount;
}

PINI_SECTION_ITEM IniGetSettingByNumber(ULONG_PTR SectionId, ULONG SettingNumber)
{
    PINI_SECTION Section = (PINI_SECTION)SectionId;
    PLIST_ENTRY Entry;
    PINI_SECTION_ITEM SectionItem;

    // Loop through each section item and find the one we want
    for (Entry = Section->SectionItemList.Flink;
         Entry != &Section->SectionItemList;
         Entry = Entry->Flink)
    {
        SectionItem = CONTAINING_RECORD(Entry, INI_SECTION_ITEM, ListEntry);

        // Check to see if this is the setting we want
        if (SettingNumber == 0)
        {
            return SectionItem;
        }

        // Nope, keep going
        SettingNumber--;
    }
    return NULL;
}

ULONG IniGetSectionSettingNameSize(ULONG_PTR SectionId, ULONG SettingIndex)
{
    PINI_SECTION_ITEM SectionItem;

    // Retrieve requested setting
    SectionItem = IniGetSettingByNumber(SectionId, SettingIndex);
    if (!SectionItem)
        return 0;

    // Return the size of the string plus 1 for the null-terminator
    return (ULONG)(strlen(SectionItem->ItemName) + 1);
}

ULONG IniGetSectionSettingValueSize(ULONG_PTR SectionId, ULONG SettingIndex)
{
    PINI_SECTION_ITEM SectionItem;

    // Retrieve requested setting
    SectionItem = IniGetSettingByNumber(SectionId, SettingIndex);
    if (!SectionItem)
        return 0;

    // Return the size of the string plus 1 for the null-terminator
    return (ULONG)(strlen(SectionItem->ItemValue) + 1);
}

BOOLEAN IniReadSettingByNumber(ULONG_PTR SectionId, ULONG SettingNumber, PCHAR SettingName, ULONG NameSize, PCHAR SettingValue, ULONG ValueSize)
{
    PINI_SECTION_ITEM    SectionItem;
    TRACE(".001 NameSize = %d ValueSize = %d\n", NameSize, ValueSize);

    TRACE("IniReadSettingByNumber() SectionId = 0x%x\n", SectionId);

    // Retrieve requested setting
    SectionItem = IniGetSettingByNumber(SectionId, SettingNumber);
    if (!SectionItem)
    {
        TRACE("IniReadSettingByNumber() Setting number %d not found.\n", SettingNumber);
        return FALSE;
    }

    TRACE("IniReadSettingByNumber() Setting number %d found.\n", SettingNumber);
    TRACE("IniReadSettingByNumber() Setting name = %s\n", SectionItem->ItemName);
    TRACE("IniReadSettingByNumber() Setting value = %s\n", SectionItem->ItemValue);

    TRACE("1 NameSize = %d ValueSize = %d\n", NameSize, ValueSize);
    TRACE("2 NameSize = %d ValueSize = %d\n", NameSize, ValueSize);
    strncpy(SettingName, SectionItem->ItemName, NameSize - 1);
    SettingName[NameSize - 1] = '\0';
    TRACE("3 NameSize = %d ValueSize = %d\n", NameSize, ValueSize);
    strncpy(SettingValue, SectionItem->ItemValue, ValueSize - 1);
    SettingValue[ValueSize - 1] = '\0';
    TRACE("4 NameSize = %d ValueSize = %d\n", NameSize, ValueSize);
    DbgDumpBuffer(DPRINT_INIFILE, SettingName, NameSize);
    DbgDumpBuffer(DPRINT_INIFILE, SettingValue, ValueSize);

    return TRUE;
}

BOOLEAN IniReadSettingByName(ULONG_PTR SectionId, PCSTR SettingName, PCHAR Buffer, ULONG BufferSize)
{
    PINI_SECTION Section = (PINI_SECTION)SectionId;
    PLIST_ENTRY Entry;
    PINI_SECTION_ITEM SectionItem;

    TRACE("IniReadSettingByName() SectionId = 0x%x\n", SectionId);

    // Loop through each section item and find the one we want
    for (Entry = Section->SectionItemList.Flink;
         Entry != &Section->SectionItemList;
         Entry = Entry->Flink)
    {
        SectionItem = CONTAINING_RECORD(Entry, INI_SECTION_ITEM, ListEntry);

        // Check to see if this is the setting we want
        if (_stricmp(SettingName, SectionItem->ItemName) == 0)
        {
            TRACE("IniReadSettingByName() Setting \'%s\' found.\n", SettingName);
            TRACE("IniReadSettingByName() Setting value = %s\n", SectionItem->ItemValue);

            strncpy(Buffer, SectionItem->ItemValue, BufferSize - 1);
            Buffer[BufferSize - 1] = '\0';

            return TRUE;
        }
    }

    WARN("IniReadSettingByName() Setting \'%s\' not found.\n", SettingName);

    return FALSE;
}

BOOLEAN IniAddSection(PCSTR SectionName, ULONG_PTR* SectionId)
{
    PINI_SECTION Section;

    // Allocate a new section structure
    Section = FrLdrTempAlloc(sizeof(INI_SECTION), TAG_INI_SECTION);
    if (!Section)
    {
        return FALSE;
    }

    RtlZeroMemory(Section, sizeof(INI_SECTION));

    // Allocate the section name buffer
    Section->SectionName = FrLdrTempAlloc(strlen(SectionName) + sizeof(CHAR), TAG_INI_NAME);
    if (!Section->SectionName)
    {
        FrLdrTempFree(Section, TAG_INI_SECTION);
        return FALSE;
    }

    // Get the section name
    strcpy(Section->SectionName, SectionName);
    InitializeListHead(&Section->SectionItemList);

    // Add it to the section list head
    IniFileSectionCount++;
    InsertHeadList(&IniFileSectionListHead, &Section->ListEntry);

    *SectionId = (ULONG_PTR)Section;

    return TRUE;
}

VOID IniFreeSection(PINI_SECTION Section)
{
    PLIST_ENTRY ListEntry;
    PINI_SECTION_ITEM SectionItem;

    // Loop while there are section items
    while (!IsListEmpty(&Section->SectionItemList))
    {
        // Remove the section item
        ListEntry = RemoveHeadList(&Section->SectionItemList);
        SectionItem = CONTAINING_RECORD(ListEntry, INI_SECTION_ITEM, ListEntry);

        // Free it
        FrLdrTempFree(SectionItem->ItemName, TAG_INI_NAME);
        FrLdrTempFree(SectionItem->ItemValue, TAG_INI_VALUE);
        FrLdrTempFree(SectionItem, TAG_INI_SECTION_ITEM);
    }

    FrLdrTempFree(Section->SectionName, TAG_INI_NAME);
    FrLdrTempFree(Section, TAG_INI_SECTION);
}

VOID IniCleanup(VOID)
{
    PLIST_ENTRY ListEntry;
    PINI_SECTION Section;

    // Loop while there are sections
    while (!IsListEmpty(&IniFileSectionListHead))
    {
        // Remove the section
        ListEntry = RemoveHeadList(&IniFileSectionListHead);
        Section = CONTAINING_RECORD(ListEntry, INI_SECTION, ListEntry);

        // Free it
        IniFreeSection(Section);
    }
}

BOOLEAN IniAddSettingValueToSection(ULONG_PTR SectionId, PCSTR SettingName, PCSTR SettingValue)
{
    PINI_SECTION Section = (PINI_SECTION)SectionId;
    PINI_SECTION_ITEM SectionItem;

    // Allocate a new item structure
    SectionItem = FrLdrTempAlloc(sizeof(INI_SECTION_ITEM), TAG_INI_SECTION_ITEM);
    if (!SectionItem)
    {
        return FALSE;
    }

    RtlZeroMemory(SectionItem, sizeof(INI_SECTION_ITEM));

    // Allocate the setting name buffer
    SectionItem->ItemName = FrLdrTempAlloc(strlen(SettingName) + 1, TAG_INI_NAME);
    if (!SectionItem->ItemName)
    {
        FrLdrTempFree(SectionItem, TAG_INI_SECTION_ITEM);
        return FALSE;
    }

    // Allocate the setting value buffer
    SectionItem->ItemValue = FrLdrTempAlloc(strlen(SettingValue) + 1, TAG_INI_VALUE);
    if (!SectionItem->ItemValue)
    {
        FrLdrTempFree(SectionItem->ItemName, TAG_INI_NAME);
        FrLdrTempFree(SectionItem, TAG_INI_SECTION_ITEM);
        return FALSE;
    }

    strcpy(SectionItem->ItemName, SettingName);
    strcpy(SectionItem->ItemValue, SettingValue);

    // Add it to the current section
    Section->SectionItemCount++;
    InsertTailList(&Section->SectionItemList, &SectionItem->ListEntry);

    return TRUE;
}

BOOLEAN IniModifySettingValue(ULONG_PTR SectionId, PCSTR SettingName, PCSTR SettingValue)
{
    PINI_SECTION Section = (PINI_SECTION)SectionId;
    PLIST_ENTRY Entry;
    PINI_SECTION_ITEM SectionItem;
    PCHAR NewItemValue;

    // Loop through each section item and find the one we want
    for (Entry = Section->SectionItemList.Flink;
         Entry != &Section->SectionItemList;
         Entry = Entry->Flink)
    {
        SectionItem = CONTAINING_RECORD(Entry, INI_SECTION_ITEM, ListEntry);

        // Check to see if this is the setting we want
        if (_stricmp(SectionItem->ItemName, SettingName) == 0)
        {
            break;
        }
        // Nope, keep going
    }
    // If the section item does not exist, create it
    if (Entry == &Section->SectionItemList)
    {
        return IniAddSettingValueToSection(SectionId, SettingName, SettingValue);
    }

    // Reallocate the new setting value buffer
    NewItemValue = FrLdrTempAlloc(strlen(SettingValue) + 1, TAG_INI_VALUE);
    if (!NewItemValue)
    {
        // We failed, bail out
        return FALSE;
    }
    FrLdrTempFree(SectionItem->ItemValue, TAG_INI_VALUE);
    SectionItem->ItemValue = NewItemValue;

    strcpy(SectionItem->ItemValue, SettingValue);

    return TRUE;
}
