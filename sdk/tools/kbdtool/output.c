/*
 * PROJECT:         ReactOS Build Tools [Keyboard Layout Compiler]
 * LICENSE:         BSD - See COPYING.BSD in the top level directory
 * FILE:            tools/kbdtool/output.c
 * PURPOSE:         Output Logic (Source Builder)
 * PROGRAMMERS:     ReactOS Foundation
 */

/* INCLUDES *******************************************************************/

#include "kbdtool.h"

/* GLOBALS ********************************************************************/

ULONG gStringIdForDescriptions = 1000;
ULONG gStringIdForLanguageNames = 1100;
ULONG gStringIdForLocaleName = 1200;
time_t Clock;
struct tm *Now;
CHAR gCharName[32];

/* FUNCTIONS ******************************************************************/

PCHAR
WChName(IN ULONG Char,
        IN BOOLEAN AddZero)
{
    PCHAR p;

    /* Check for invalid character */
    if (Char == -1)
    {
        /* No name */
        strcpy(gCharName, "WCH_NONE");
    }
    else if ((Char > 31) && (Char < 128))
    {
        /* Use our global buffer */
        p = gCharName;

        /* Add the first quote unless this was a zero-string */
        if (!AddZero) *p++ = '\'';

        /* Now replace any other quote or comment character with a slash */
        if ((Char == '\"') || (Char == '\'') || (Char == '\\')) *p++ = '\\';

        /* Now plug in the character */
        *p++ = Char;

        /* And add the terminating quote, unless this was a zero-string */
        if (!AddZero) *p++ = '\'';

        /* Terminate the buffer */
        *p = '\0';
    }
    else
    {
        /* Handle special cases */
        if (Char == '\b')
        {
            /* Bell */
            strcpy(gCharName, "'\\b'");
        }
        else if (Char == '\n')
        {
            /* New line */
            strcpy(gCharName, "'\\n'");
        }
        else if (Char == '\r')
        {
            /* Return */
            strcpy(gCharName, "'\\r'");
        }
        else if (!AddZero)
        {
            /* Char value, in hex */
            sprintf(gCharName, "0x%04x", Char);
        }
        else
        {
            /* Char value, C-style */
            sprintf(gCharName, "\\x%04x", Char);
        }
    }

    /* Return the name */
    return gCharName;
}

VOID
PrintNameTable(FILE* FileHandle,
               PKEYNAME KeyName,
               BOOL DeadKey)
{
    CHAR CharBuffer[255];
    PKEYNAME NextName;
    PCHAR Name, Buffer;
    ULONG i;

    /* Loop all key names */
    while (KeyName)
    {
        /* Go to the next key name */
        NextName = KeyName->Next;

        /* Remember the name and our buffer address */
        Name = KeyName->Name;
        Buffer = CharBuffer;

        /* Check if it's an IDS name */
        if (strncmp(Name, "IDS_", 4))
        {
            /* No, so start parsing it. First, handle initial quote */
            if (*Name != '\"') *Buffer++ = '\"';

            /* Next, parse the name */
            while (*Name)
            {
                /* Check if this is a C-style hex string */
                if ((*Name != '\\') || ((*(Name + 1) != 'x') && (*(Name + 1) != 'X')))
                {
                    /* It's not, so just copy straight into our buffer */
                    *Buffer++ = *Name++;
                }
                else
                {
                    /* Continue scanning as long as this is a C-style hex string */
                    while ((*Name == '\\') && ((*(Name + 1) == 'x') || (*(Name + 1) == 'X')))
                    {
                        /* Copy 6 characters */
                        for (i = 0; (*Name) && (i < 6); i++) *Buffer++ = *Name++;
                    }

                    /* Check if we still have something at the end */
                    if (*Name)
                    {
                        /* Terminate our buffer */
                        *Buffer++ = '\"';
                        *Buffer++ = ' ';
                        *Buffer++ = 'L';
                        *Buffer++ = '\"';
                    }
                }
            }

            /*  Check for terminating quote */
            if (*(Buffer - 1) != '\"') *Buffer++ = '\"';

            /* Terminate the buffer */
            *Buffer++ = '\0';
        }
        else
        {
            /* Not yet supported */
            printf("IDS Entries not yet handled!\n");
            exit(1);
        }

        /* Is this a dead key? */
        if (DeadKey)
        {
            /* Not yet handled */
            printf("Dead keys not yet handled\n");
            exit(1);
        }
        else
        {
            /* Print the entry */
            fprintf(FileHandle, "    0x%02x,    L%s,\n", KeyName->Code, CharBuffer);
        }

        /* Cleanup allocation */
        free(KeyName->Name);
        free(KeyName);

        /* Move on */
        KeyName = NextName;
    }

    /* Is this a dead key? */
    if (DeadKey)
    {
        /* Not yet handled */
        printf("Dead keys not yet handled\n");
        exit(1);
    }
    else
    {
        /* Terminate the table */
        fprintf(FileHandle, "    0   ,    NULL\n");
    }
}

BOOLEAN
kbd_h(IN PLAYOUT Layout)
{
    CHAR OutputFile[13];
    FILE *FileHandle;
    ULONG i;
    CHAR UndefChar;
    USHORT SubCode;

    /* Build the keyboard name */
    strcpy(OutputFile, gKBDName);
    strcat(OutputFile, ".H");

    /* Open it */
    FileHandle = fopen(OutputFile, "wt");
    if (!FileHandle)
    {
        /* Fail */
        printf(" %12s : can't open for write.\n", OutputFile);
        return FALSE;
    }

    /* Print the module header */
    fprintf(FileHandle,
            "/****************************** Module Header ******************************\\\n"
            "* Module Name: %s\n*\n* keyboard layout header\n"
            "*\n"
            "* Copyright (c) 2009, ReactOS Foundation\n"
            "*\n"
            "* Various defines for use by keyboard input code.\n*\n* History:\n"
            "*\n"
            "* created by KBDTOOL v%d.%02d %s*\n"
            "\\***************************************************************************/\n\n",
            OutputFile,
            gVersion,
            gSubVersion,
            asctime(Now));

    /* Print out the includes and defines */
    fprintf(FileHandle,
            "/*\n"
            " * kbd type should be controlled by cl command-line argument\n"
            " *\\n"
            "#define KBD_TYPE 4\n\n"
            "/*\n"
            "* Include the basis of all keyboard table values\n"
            "*/\n"
            "#include \"kbd.h\"\n");

    /* Now print out the virtual key conversion table */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "* The table below defines the virtual keys for various keyboard types where\n"
            "* the keyboard differ from the US keyboard.\n"
            "*\n"
            "* _EQ() : all keyboard types have the same virtual key for this scancode\n"
            "* _NE() : different virtual keys for this scancode, depending on kbd type\n"
            "*\n"
            "*     +------+ +----------+----------+----------+----------+----------+----------+\n"
            "*     | Scan | |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |\n"
            "*     | code | |   type 1 |   type 2 |   type 3 |   type 4 |   type 5 |   type 6 |\n"
            "\\****+-------+_+----------+----------+----------+----------+----------+----------+*/\n\n");

    /* Loop all keys */
    for (i = 0; i < 110; i++)
    {
        /* Check if we processed this key */
        if (Layout->Entry[i].Processed)
        {
            /* Check if it redefined a virtual key */
            if (Layout->Entry[i].VirtualKey != Layout->Entry[i].OriginalVirtualKey)
            {
                /* Do we have a subcode? */
                SubCode = Layout->Entry[i].ScanCode & 0xFF00;
                if (SubCode)
                {
                    /* Which kind is it? */
                    if (SubCode == 0xE000)
                    {
                        /* Extended 0 */
                        UndefChar = 'X';
                    }
                    else
                    {
                        /* Illegal */
                        if (SubCode != 0xE100)
                        {
                            /* Unrecognized */
                            printf("Weird scancode value %04x: expected xx, E0xx, or E1xx\n", SubCode);
                            exit(1);
                        }

                        /* Extended 1 */
                        UndefChar = 'Y';
                    }
                }
                else
                {
                    /* Normal key */
                    UndefChar = 'T';
                }

                /* Print out the virtual key redefinition */
                fprintf(FileHandle,
                        "#undef %c%02X\n#define %c%02X _EQ(%43s%23s\n",
                        UndefChar,
                        Layout->Entry[i].ScanCode,
                        UndefChar,
                        Layout->Entry[i].ScanCode,
                        getVKName(Layout->Entry[i].VirtualKey, 0),
                        ")");
            }
        }
    }

    /* Cleanup and close */
    fprintf(FileHandle,"\n");
    fclose(FileHandle);

    /* We made it */
    return TRUE;
}

BOOLEAN
kbd_rc(IN PKEYNAME DescriptionData,
       IN PKEYNAME LanguageData)
{
    CHAR OutputFile[13];
    CHAR InternalName[13];
    CHAR TimeBuffer[5];
    FILE *FileHandle;
    ULONG Length;
    PCHAR p;
    PKEYNAME NextDescription, NextLanguage;

    /* Build the keyboard name and internal name */
    strcpy(OutputFile, gKBDName);
    strcat(OutputFile, ".RC");
    strcpy(InternalName, gKBDName);
    for (p = InternalName; *p; p++) *p = tolower(*p);

    /* Open it */
    FileHandle = fopen(OutputFile, "wb");
    if (!FileHandle)
    {
        /* Fail */
        printf(" %12s : can't open for write.\n", OutputFile);
        return FALSE;
    }

    /* Check if we have copyright */
    Length = strlen(gCopyright);
    if (!Length)
    {
        /* Set time string */
        strftime(TimeBuffer, 5, "%Y", Now);

        /* Add copyright character */
        strcpy(gCopyright, "(C)");

        /* Add the year */
        strcat(gCopyright, TimeBuffer);

        /* Add blank company */
        strcat(gCopyright, " ");
    }

    /* Write the resource file header */
    fprintf(FileHandle,
            "#include \"winver.h\"\r\n"
            "1 VERSIONINFO\r\n"
            " FILEVERSION       1,0,%d,%d\r\n"
            " PRODUCTVERSION    1,0,%d,%d\r\n"
            " FILEFLAGSMASK 0x3fL\r\n"
            " FILEFLAGS 0x0L\r\n"
            "FILEOS 0x40004L\r\n"
            " FILETYPE VFT_DLL\r\n"
            " FILESUBTYPE VFT2_DRV_KEYBOARD\r\n"
            "BEGIN\r\n"
            "    BLOCK \"StringFileInfo\"\r\n"
            "    BEGIN\r\n"
            "        BLOCK \"000004B0\"\r\n"
            "        BEGIN\r\n"
            "            VALUE \"CompanyName\",     \"%s\\0\"\r\n"
            "            VALUE \"FileDescription\", \"%s Keyboard Layout\\0\"\r\n"
            "            VALUE \"FileVersion\",     \"1, 0, %d, %d\\0\"\r\n",
            gVersion,
            gSubVersion,
            gVersion,
            gSubVersion,
            gCompany,
            gDescription,
            gVersion,
            gSubVersion);

    /* Continue writing it */
    fprintf(FileHandle,
            "            VALUE \"InternalName\",    \"%s (%d.%d)\\0\"\r\n"
            "            VALUE \"ProductName\",\"%s\\0\"\r\n"
            "            VALUE \"Release Information\",\"%s\\0\"\r\n"
            "            VALUE \"LegalCopyright\",  \"%s\\0\"\r\n"
            "            VALUE \"OriginalFilename\",\"%s\\0\"\r\n"
            "            VALUE \"ProductVersion\",  \"1, 0, %d, %d\\0\"\r\n"
            "        END\r\n"
            "    END\r\n"
            "    BLOCK \"VarFileInfo\"\r\n"
            "    BEGIN\r\n"
            "        VALUE \"Translation\", 0x0000, 0x04B0\r\n"
            "    END\r\n"
            "END\r\n",
            InternalName,
            gVersion,
            gSubVersion,
            "Created by ReactOS KbdTool",
            "Created by ReactOS KbdTool",
            gCopyright,
            InternalName,
            gVersion,
            gSubVersion);

    /* Now check if we have a locale name */
    Length = strlen(gLocaleName);
    if (Length)
    {
        /* Write the stringtable header */
        fprintf(FileHandle,
                "\r\nSTRINGTABLE DISCARDABLE\r\nLANGUAGE %d, %d\r\n",
                9,
                1);
        fprintf(FileHandle, "BEGIN\r\n");

        /* Language or locale? */
        if (strchr(gLocaleName, '\"'))
        {
            /* Write the language name */
            fprintf(FileHandle, "    %d    %s\r\n", gStringIdForLanguageNames, gLocaleName);
        }
        else
        {
            /* Write the locale name */
            fprintf(FileHandle, "    %d    \"%s\"\r\n", gStringIdForLocaleName, gLocaleName);
        }

        /* Terminate the entry */
        fprintf(FileHandle, "END\r\n\r\n");
    }

    /* Check for description information */
    while (DescriptionData)
    {
        /* Remember the next pointer */
        NextDescription = DescriptionData->Next;

        /* Write the header */
        fprintf(FileHandle,
                "\r\nSTRINGTABLE DISCARDABLE\r\nLANGUAGE %d, %d\r\n",
                DescriptionData->Code & 0x3FF,
                DescriptionData->Code >> 10);
        fprintf(FileHandle, "BEGIN\r\n");

        /* Quoted string or not? */
        if (strchr(DescriptionData->Name, '\"'))
        {
            /* Write the description name */
            fprintf(FileHandle, "    %d    %s\r\n", gStringIdForDescriptions, DescriptionData->Name);
        }
        else
        {
            /* Write the description name */
            fprintf(FileHandle, "    %d    \"%s\"\r\n", gStringIdForDescriptions, DescriptionData->Name);
        }

        /* Terminate the entry */
        fprintf(FileHandle, "END\r\n\r\n");

        /* Free the allocation */
        free(DescriptionData->Name);
        free(DescriptionData);

        /* Move to the next entry */
        DescriptionData = NextDescription;
    }

    /* Check for language information */
    while (LanguageData)
    {
        /* Remember the next pointer */
        NextLanguage = LanguageData->Next;

        /* Write the header */
        fprintf(FileHandle,
                "\r\nSTRINGTABLE DISCARDABLE\r\nLANGUAGE %d, %d\r\n",
                LanguageData->Code & 0x3FF,
                LanguageData->Code >> 10);
        fprintf(FileHandle, "BEGIN\r\n");

        /* Quoted string or not? */
        if (strchr(LanguageData->Name, '\"'))
        {
            /* Write the language name */
            fprintf(FileHandle, "    %d    %s\r\n", gStringIdForLanguageNames, LanguageData->Name);
        }
        else
        {
            /* Write the language name */
            fprintf(FileHandle, "    %d    \"%s\"\r\n", gStringIdForLanguageNames, LanguageData->Name);
        }

        /* Terminate the entry */
        fprintf(FileHandle, "END\r\n\r\n");

        /* Free the allocation */
        free(LanguageData->Name);
        free(LanguageData);

        /* Move to the next entry */
        LanguageData = NextLanguage;
    }

    /* We're done! */
    fclose(FileHandle);
    return TRUE;
}

BOOLEAN
kbd_def(VOID)
{
    CHAR OutputFile[13];
    FILE *FileHandle;

    /* Build the keyboard name and internal name */
    strcpy(OutputFile, gKBDName);
    strcat(OutputFile, ".DEF");

    /* Open it */
    FileHandle = fopen(OutputFile, "wt");
    if (!FileHandle)
    {
        /* Fail */
        printf(" %12s : can't open for write.\n", OutputFile);
        return FALSE;
    }

    /* Write the file exports */
    fprintf(FileHandle,
            "LIBRARY %s\n\n"
            "EXPORTS\n"
            "    KbdLayerDescriptor @1\n",
            gKBDName);

    /* Clean up */
    fclose(FileHandle);
    return TRUE;
}

BOOLEAN
kbd_c(IN ULONG StateCount,
      IN PULONG ShiftStates,
      IN PVOID AttributeData,
      IN PLAYOUT Layout,
      IN PVOID DeadKeyData,
      IN PVOID LigatureData,
      IN PKEYNAME KeyNameData,
      IN PKEYNAME KeyNameExtData,
      IN PKEYNAME KeyNameDeadData)
{
    CHAR OutputFile[13];
    CHAR KeyNameBuffer[50];
    CHAR LineBuffer[256];
    BOOLEAN NeedPlus;
    FILE *FileHandle;
    ULONG States[8];
    ULONG i, j, k;
    ULONG HighestState;
    PVKNAME Entry;
    PCHAR p;

    /* Build the keyboard name and internal name */
    strcpy(OutputFile, gKBDName);
    strcat(OutputFile, ".C");

    /* Open it */
    FileHandle = fopen(OutputFile, "wt");
    if (!FileHandle)
    {
        /* Fail */
        printf(" %12s : can't open for write.\n", OutputFile);
        return FALSE;
    }

    /* Print the header */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "* Module Name: %s\n*\n* keyboard layout\n"
            "*\n"
            "* Copyright (c) 2009, ReactOS Foundation\n"
            "*\n"
            "* History:\n"
            "* KBDTOOL v%d.%02d - Created  %s"
            "\\***************************************************************************/\n\n",
            OutputFile,
            gVersion,
            gSubVersion,
            asctime(Now));

    /* What kind of driver is this? */
    if (FallbackDriver)
    {
        /* Fallback only */
        fprintf(FileHandle, "#include \"precomp.h\"\n");
    }
    else
    {
        /* Add the includes */
        fprintf(FileHandle,
                "#include <windows.h>\n"
                "#include \"kbd.h\"\n"
                "#include \"%s.h\"\n\n",
                gKBDName);
    }

    /* What kind of driver is this? */
    if (FallbackDriver)
    {
        /* Only one section */
        fprintf(FileHandle,
                "#pragma data_seg(\"%s\")\n#define ALLOC_SECTION_LDATA\n\n",
                ".kbdfallback");
    }
    else
    {
        /* Section and attributes depend on architecture */
        fprintf(FileHandle,
                "#if defined(_M_IA64)\n"
                "#pragma section(\"%s\")\n"
                "#define ALLOC_SECTION_LDATA __declspec(allocate(\"%s\"))\n"
                "#else\n"
                "#pragma data_seg(\"%s\")\n"
                "#define ALLOC_SECTION_LDATA\n"
                "#endif\n\n",
                ".data",
                ".data",
                ".data");
    }

    /* Scan code to virtual key conversion table header */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "* ausVK[] - Virtual Scan Code to Virtual Key conversion table\n"
            "\\***************************************************************************/\n\n");

    /* Table begin */
    fprintf(FileHandle,
            "static ALLOC_SECTION_LDATA USHORT ausVK[] = {\n"
            "    T00, T01, T02, T03, T04, T05, T06, T07,\n"
            "    T08, T09, T0A, T0B, T0C, T0D, T0E, T0F,\n"
            "    T10, T11, T12, T13, T14, T15, T16, T17,\n"
            "    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,\n"
            "    T20, T21, T22, T23, T24, T25, T26, T27,\n"
            "    T28, T29, T2A, T2B, T2C, T2D, T2E, T2F,\n"
            "    T30, T31, T32, T33, T34, T35,\n\n");

    /* Table continue */
    fprintf(FileHandle,
            "    /*\n"
            "     * Right-hand Shift key must have KBDEXT bit set.\n"
            "     */\n"
            "    T36 | KBDEXT,\n\n"
            "    T37 | KBDMULTIVK,               // numpad_* + Shift/Alt -> SnapShot\n\n"
            "    T38, T39, T3A, T3B, T3C, T3D, T3E,\n"
            "    T3F, T40, T41, T42, T43, T44,\n\n");

    /* Table continue */
    fprintf(FileHandle,
            "    /*\n"
            "     * NumLock Key:\n"
            "     *     KBDEXT     - VK_NUMLOCK is an Extended key\n"
            "     *     KBDMULTIVK - VK_NUMLOCK or VK_PAUSE (without or with CTRL)\n"
            "     */\n"
            "    T45 | KBDEXT | KBDMULTIVK,\n\n"
            "    T46 | KBDMULTIVK,\n\n");

    /* Numpad table */
    fprintf(FileHandle,
            "    /*\n"
            "     * Number Pad keys:\n"
            "     *     KBDNUMPAD  - digits 0-9 and decimal point.\n"
            "     *     KBDSPECIAL - require special processing by Windows\n"
            "     */\n"
            "    T47 | KBDNUMPAD | KBDSPECIAL,   // Numpad 7 (Home)\n"
            "    T48 | KBDNUMPAD | KBDSPECIAL,   // Numpad 8 (Up),\n"
            "    T49 | KBDNUMPAD | KBDSPECIAL,   // Numpad 9 (PgUp),\n"
            "    T4A,\n"
            "    T4B | KBDNUMPAD | KBDSPECIAL,   // Numpad 4 (Left),\n"
            "    T4C | KBDNUMPAD | KBDSPECIAL,   // Numpad 5 (Clear),\n"
            "    T4D | KBDNUMPAD | KBDSPECIAL,   // Numpad 6 (Right),\n"
            "    T4E,\n"
            "    T4F | KBDNUMPAD | KBDSPECIAL,   // Numpad 1 (End),\n"
            "    T50 | KBDNUMPAD | KBDSPECIAL,   // Numpad 2 (Down),\n"
            "    T51 | KBDNUMPAD | KBDSPECIAL,   // Numpad 3 (PgDn),\n"
            "    T52 | KBDNUMPAD | KBDSPECIAL,   // Numpad 0 (Ins),\n"
            "    T53 | KBDNUMPAD | KBDSPECIAL,   // Numpad . (Del),\n\n");

    /* Table finish */
    fprintf(FileHandle,
            "    T54, T55, T56, T57, T58, T59, T5A, T5B,\n"
            "    T5C, T5D, T5E, T5F, T60, T61, T62, T63,\n"
            "    T64, T65, T66, T67, T68, T69, T6A, T6B,\n"
            "    T6C, T6D, T6E, T6F, T70, T71, T72, T73,\n"
            "    T74, T75, T76, T77, T78, T79, T7A, T7B,\n"
            "    T7C, T7D, T7E\n\n"
            "};\n\n");

    /* Key name table header */
    fprintf(FileHandle, "static ALLOC_SECTION_LDATA VSC_VK aE0VscToVk[] = {\n");

    /* Loop 110-key table */
    for (i = 0; i < 110; i++)
    {
        /* Check for non-extended keys */
        if ((Layout->Entry[i].ScanCode & 0xFF00) == 0xE000)
        {
            /* Which are valid */
            if (Layout->Entry[i].ScanCode != 0xFF)
            {
                /* And mapped */
                if (Layout->Entry[i].VirtualKey != 0xFF)
                {
                    /* Output them */
                    fprintf(FileHandle,
                            "        { 0x%02X, X%02X | KBDEXT              },  // %s\n",
                            Layout->Entry[i].ScanCode,
                            Layout->Entry[i].ScanCode,
                            Layout->Entry[i].Name);
                }
            }
        }
    }

    /* Key name table finish */
    fprintf(FileHandle, "        { 0,      0                       }\n};\n\n");

    /* Extended key name table header */
    fprintf(FileHandle, "static ALLOC_SECTION_LDATA VSC_VK aE1VscToVk[] = {\n");

    /* Loop 110-key table */
    for (i = 0; i < 110; i++)
    {
        /* Check for extended keys */
        if ((Layout->Entry[i].ScanCode & 0xFF00) == 0xE100)
        {
            /* Which are valid */
            if (Layout->Entry[i].ScanCode != 0xFF)
            {
                /* And mapped */
                if (Layout->Entry[i].VirtualKey != 0xFF)
                {
                    /* Output them */
                    fprintf(FileHandle,
                            "        { 0x%02X, Y%02X | KBDEXT            },  // %s\n",
                            Layout->Entry[i].ScanCode,
                            Layout->Entry[i].ScanCode,
                            Layout->Entry[i].Name);
                }
            }
        }
    }

    /* Extended key name table finish */
    fprintf(FileHandle,
            "        { 0x1D, Y1D                       },  // Pause\n"
            "        { 0   ,   0                       }\n};\n\n");

    /* Modifier table description */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "* aVkToBits[]  - map Virtual Keys to Modifier Bits\n"
            "*\n"
            "* See kbd.h for a full description.\n"
            "*\n"
            "* The keyboard has only three shifter keys:\n"
            "*     SHIFT (L & R) affects alphabnumeric keys,\n"
            "*     CTRL  (L & R) is used to generate control characters\n"
            "*     ALT   (L & R) used for generating characters by number with numpad\n"
            "\\***************************************************************************/\n");

    /* Modifier table header */
    fprintf(FileHandle, "static ALLOC_SECTION_LDATA VK_TO_BIT aVkToBits[] = {\n");

    /* Loop modifier table */
    i = 0;
    Entry = &Modifiers[0];
    while (Entry->VirtualKey)
    {
        /* Print out entry */
        fprintf(FileHandle,
                "    { %-12s,   %-12s },\n",
                getVKName(Entry->VirtualKey, 1),
                Entry->Name);

        /* Move to the next one */
        Entry = &Modifiers[++i];
    }

    /* Modifier table finish */
    fprintf(FileHandle, "    { 0,          0        }\n};\n\n");

    /* Modifier conversion table description */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "* aModification[]  - map character modifier bits to modification number\n"
            "*\n"
            "* See kbd.h for a full description.\n"
            "*\n"
            "\\***************************************************************************/\n\n");

    /* Zero out local state data */
    for (i = 0; i < 8; i++) States[i] = -1;

    /* Find the highest set state */
    for (HighestState = 1, i = 0; (i < 8) && (ShiftStates[i] != -1); i++)
    {
        /* Save all state values */
        States[ShiftStates[i]] = i;
        if (ShiftStates[i] > HighestState) HighestState = ShiftStates[i];
    }

    /* Modifier conversion table header */
    fprintf(FileHandle,
            "static ALLOC_SECTION_LDATA MODIFIERS CharModifiers = {\n"
            "    &aVkToBits[0],\n"
            "    %d,\n"
            "    {\n"
            "    //  Modification# //  Keys Pressed\n"
            "    //  ============= // =============\n",
            HighestState);

    /* Loop states */
    for (i = 0; i <= HighestState; i++)
    {
        /* Check for invalid state */
        if (States[i] == -1)
        {
            /* Invalid state header */
            fprintf(FileHandle, "        SHFT_INVALID, // ");
        }
        else
        {
            /* Is this the last one? */
            if (i == HighestState)
            {
                /* Last state header */
                fprintf(FileHandle, "        %d             // ", States[i]);
            }
            else
            {
                /* Normal state header */
                fprintf(FileHandle, "        %d,            // ", States[i]);
            }

            /* State 1 or higher? */
            if (i >= 1)
            {
                /* J is the loop variable, K is the double */
                for (NeedPlus = 0, j = 0, k = 1; (1 << j) <= i; j++, k = (1 << j))
                {
                    /* Do we need to add a plus? */
                    if (NeedPlus)
                    {
                        /* Add it */
                        fprintf(FileHandle, "+");
                        NeedPlus = FALSE;
                    }

                    /* Check if it's time to add a modifier */
                    if (i & k)
                    {
                        /* Get the key state name and copy it into our buffer */
                        strcpy(KeyNameBuffer, getVKName(Modifiers[j].VirtualKey, 1));

                        /* Go go the 4th char (past the "KBD") and lower name */
                        for (p = &KeyNameBuffer[4]; *p; p++) *p = tolower(*p);

                        /* Print it */
                        fprintf(FileHandle, "%s", &KeyNameBuffer[3]);

                        /* We'll need a plus sign next */
                        NeedPlus = TRUE;
                    }
                }
            }

            /* Terminate the entry */
            fprintf(FileHandle, "\n");
        }
    }


    /* Modifier conversion table end */
    fprintf(FileHandle,"     }\n" "};\n\n");

    /* Shift state translation table description */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "*\n"
            "* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states\n"
            "* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states\n"
            "* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states\n");

    /* Check if there's exta shift states */
    for (i = 5; i < HighestState; i++)
    {
        /* Print out extra information */
        fprintf(FileHandle,
                "* aVkToWch%d[]  - Virtual Key to WCHAR translation for %d shift states\n",
                i,
                i);
    }

    /* Shift state translation table description continue */
    fprintf(FileHandle,
            "*\n"
            "* Table attributes: Unordered Scan, null-terminated\n"
            "*\n"
            "* Search this table for an entry with a matching Virtual Key to find the\n"
            "* corresponding unshifted and shifted WCHAR characters.\n"
            "*\n"
            "* Special values for VirtualKey (column 1)\n"
            "*     0xff          - dead chars for the previous entry\n"
            "*     0             - terminate the list\n"
            "*\n"
            "* Special values for Attributes (column 2)\n"
            "*     CAPLOK bit    - CAPS-LOCK affect this key like SHIFT\n"
            "*\n"
            "* Special values for wch[*] (column 3 & 4)\n"
            "*     WCH_NONE      - No character\n"
            "*     WCH_DEAD      - Dead Key (diaresis) or invalid (US keyboard has none)\n"
            "*     WCH_LGTR      - Ligature (generates multiple characters)\n"
            "*\n"
            "\\***************************************************************************/\n\n");

    /* Loop all the states */
    for (i = 2; i <= StateCount; i++)
    {
        /* Check if this something else than state 2 */
        if (i != 2)
        {
            /* Loop all the scan codes */
            for (j = 0; j < 110; j++)
            {
                /* Check if this is the state for the entry */
                if (i == Layout->Entry[j].StateCount) break;
            }
        }

        /* Print the table header */
        fprintf(FileHandle,
                "static ALLOC_SECTION_LDATA VK_TO_WCHARS%d aVkToWch%d[] = {\n"
                "//                      |         |  Shift  |",
                i,
                i);

        /* Print the correct state label */
        for (k = 2; k < i; k++) fprintf(FileHandle, "%-9.9s|",
                                        StateLabel[ShiftStates[k]]);

        /* Print the next separator */
        fprintf(FileHandle, "\n//                      |=========|=========|");

        /* Check for extra states and print their separators too */
        for (k = 2; k < i; k++) fprintf(FileHandle, "=========|");

        /* Finalize the separator header */
        fprintf(FileHandle, "\n");

        /* Loop all the scan codes */
        for (j = 0; j < 110; j++)
        {
            /* Check if this is the state for the entry */
            if (i != Layout->Entry[j].StateCount) continue;

            /* Print out the entry for this key */
            fprintf(FileHandle,
                    "  {%-13s,%-7s",
                    getVKName(Layout->Entry[j].VirtualKey, 1),
                    CapState[Layout->Entry[j].Cap]);

            /* Initialize the buffer for this line */
            *LineBuffer = '\0';

            /* Loop states */
            for (k = 0; k < i; k++)
            {
                /* Check for dead key data */
                if (DeadKeyData)
                {
                    /* Not yet supported */
                    printf("Dead key data not supported!\n");
                    exit(1);
                }

                /* Check if it's a ligature key */
                if (Layout->Entry[j].LigatureCharData[k])
                {
                    /* Not yet supported */
                    printf("Ligature key data not supported!\n");
                    exit(1);
                }

                /* Print out the WCH_ name */
                fprintf(FileHandle,
                        ",%-9s",
                        WChName(Layout->Entry[j].CharData[k], 0));

                /* If we have something on the line buffer by now, add WCH_NONE */
                if (*LineBuffer != '\0') strcpy(LineBuffer, "WCH_NONE ");
            }

            /* Finish the line */
            fprintf(FileHandle, "},\n");

            /* Do we have any data at all? */
            if (*LineBuffer != '\0')
            {
                /* Print it, we're done */
                fprintf(FileHandle, "%s},\n", LineBuffer);
                continue;
            }

            /* Otherwise, we're done, unless this requires SGCAP data */
            if (Layout->Entry[j].Cap != 2) continue;

            /* Not yet supported */
            printf("SGCAP not yet supported!\n");
            exit(1);
        }

        /* Did we only have two states? */
        if (i == 2)
        {
            /* Print out the built-in table */
            fprintf(FileHandle,
                    "  {VK_TAB       ,0      ,'\\t'     ,'\\t'     },\n"
                    "  {VK_ADD       ,0      ,'+'      ,'+'      },\n"
                    "  {VK_DIVIDE    ,0      ,'/'      ,'/'      },\n"
                    "  {VK_MULTIPLY  ,0      ,'*'      ,'*'      },\n"
                    "  {VK_SUBTRACT  ,0      ,'-'      ,'-'      },\n");
        }

        /* Terminate the table */
        fprintf(FileHandle, "  {0            ,0      ");
        for (k = 0; k < i; k++) fprintf(FileHandle, ",0        ");

        /* Terminate the structure */
        fprintf(FileHandle, "}\n" "};\n\n");
    }

    /* Numpad translation table */
    fprintf(FileHandle,
            "// Put this last so that VkKeyScan interprets number characters\n"
            "// as coming from the main section of the kbd (aVkToWch2 and\n"
            "// aVkToWch5) before considering the numpad (aVkToWch1).\n\n"
            "static ALLOC_SECTION_LDATA VK_TO_WCHARS1 aVkToWch1[] = {\n"
            "    { VK_NUMPAD0   , 0      ,  '0'   },\n"
            "    { VK_NUMPAD1   , 0      ,  '1'   },\n"
            "    { VK_NUMPAD2   , 0      ,  '2'   },\n"
            "    { VK_NUMPAD3   , 0      ,  '3'   },\n"
            "    { VK_NUMPAD4   , 0      ,  '4'   },\n"
            "    { VK_NUMPAD5   , 0      ,  '5'   },\n"
            "    { VK_NUMPAD6   , 0      ,  '6'   },\n"
            "    { VK_NUMPAD7   , 0      ,  '7'   },\n"
            "    { VK_NUMPAD8   , 0      ,  '8'   },\n"
            "    { VK_NUMPAD9   , 0      ,  '9'   },\n"
            "    { 0            , 0      ,  '\\0'  }\n"
            "};\n\n");

    /* Translation tables header */
    fprintf(FileHandle,"static ALLOC_SECTION_LDATA VK_TO_WCHAR_TABLE aVkToWcharTable[] = {\n");

    /* Loop states higher than 3 */
    for (i = 3; i <= StateCount; i++)
    {
        /* Print out the extra tables */
        fprintf(FileHandle,
                "    {  (PVK_TO_WCHARS1)aVkToWch%d, %d, sizeof(aVkToWch%d[0]) },\n",
                i,
                i,
                i);
    }

    /* Array of translation tables */
    fprintf(FileHandle,
            "    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },\n"
            "    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },\n"
            "    {                       NULL, 0, 0                    },\n"
            "};\n\n");

    /* Scan code to key name conversion table description */
    fprintf(FileHandle,
            "/***************************************************************************\\\n"
            "* aKeyNames[], aKeyNamesExt[]  - Virtual Scancode to Key Name tables\n"
            "*\n"
            "* Table attributes: Ordered Scan (by scancode), null-terminated\n"
            "*\n"
            "* Only the names of Extended, NumPad, Dead and Non-Printable keys are here.\n"
            "* (Keys producing printable characters are named by that character)\n"
            "\\***************************************************************************/\n\n");

    /* Check for key name data */
    if (KeyNameData)
    {
        /* Table header */
        fprintf(FileHandle, "static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNames[] = {\n");

        /* Print table */
        PrintNameTable(FileHandle, KeyNameData, FALSE);

        /* Table end */
        fprintf(FileHandle, "};\n\n");
    }

    /* Check for extended key name data */
    if (KeyNameExtData)
    {
        /* Table header */
        fprintf(FileHandle, "static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNamesExt[] = {\n");

        /* Print table */
        PrintNameTable(FileHandle, KeyNameExtData, FALSE);

        /* Table end */
        fprintf(FileHandle, "};\n\n");
    }

    /* Check for dead key name data */
    if (KeyNameDeadData)
    {
        /* Not yet supported */
        printf("Dead key name data not supported!\n");
        exit(1);
    }

    /* Check for dead key data */
    if (DeadKeyData)
    {
        /* Not yet supported */
        printf("Dead key data not supported!\n");
        exit(1);
    }

    /* Check for ligature data */
    if (LigatureData)
    {
        /* Not yet supported */
        printf("Ligature key data not supported!\n");
        exit(1);
    }

    /* Main keyboard table descriptor type */
    fprintf(FileHandle, "static ");

    /* FIXME? */

    /* Main keyboard table descriptor header */
    fprintf(FileHandle,
            "ALLOC_SECTION_LDATA KBDTABLES KbdTables%s = {\n"
            "    /*\n"
            "     * Modifier keys\n"
            "     */\n"
            "    &CharModifiers,\n\n"
            "    /*\n"
            "     * Characters tables\n"
            "     */\n"
            "    aVkToWcharTable,\n\n"
            "    /*\n"
            "     * Diacritics\n"
            "     */\n",
            FallbackDriver ? "Fallback" : "" );

    /* Descriptor dead key data section */
    if (DeadKeyData)
    {
        fprintf(FileHandle, "    aDeadKey,\n\n");
    }
    else
    {
        fprintf(FileHandle, "    NULL,\n\n");
    }

    /* Descriptor key name comment */
    fprintf(FileHandle,
            "    /*\n"
            "     * Names of Keys\n"
            "     */\n");

    /* Descriptor key name section */
    if (KeyNameData)
    {
        fprintf(FileHandle, "    aKeyNames,\n");
    }
    else
    {
        fprintf(FileHandle, "    NULL,\n");
    }

    /* Descriptor extended key name section */
    if (KeyNameExtData)
    {
        fprintf(FileHandle, "    aKeyNamesExt,\n");
    }
    else
    {
        fprintf(FileHandle, "    NULL,\n");
    }

    /* Descriptor dead key name section */
    if ((DeadKeyData) && (KeyNameDeadData))
    {
        fprintf(FileHandle, "    aKeyNamesDead,\n\n");
    }
    else
    {
        fprintf(FileHandle, "    NULL,\n\n");
    }

    /* Descriptor conversion table section  */
    fprintf(FileHandle,
            "    /*\n"
            "     * Scan codes to Virtual Keys\n"
            "     */\n"
            "    ausVK,\n"
            "    sizeof(ausVK) / sizeof(ausVK[0]),\n"
            "    aE0VscToVk,\n"
            "    aE1VscToVk,\n\n"
            "    /*\n"
            "     * Locale-specific special processing\n"
            "     */\n");

    /* FIXME: AttributeData and KLLF_ALTGR stuff */

    /* Descriptor locale-specific section */
    fprintf(FileHandle, "    MAKELONG(%s, KBD_VERSION),\n\n", "0");  /* FIXME */

    /* Descriptor ligature data comment */
    fprintf(FileHandle, "    /*\n     * Ligatures\n     */\n    %d,\n", 0); /* FIXME */

    /* Descriptor ligature data section */
    if (!LigatureData)
    {
        fprintf(FileHandle, "    0,\n");
        fprintf(FileHandle, "    NULL\n");
    }
    else
    {
        fprintf(FileHandle, "    sizeof(aLigature[0]),\n");
        fprintf(FileHandle, "    (PLIGATURE1)aLigature\n");
    }

    /* Descriptor finish */
    fprintf(FileHandle, "};\n\n");

    /* Keyboard layout callback function */
    if (!FallbackDriver) fprintf(FileHandle,
                                 "PKBDTABLES KbdLayerDescriptor(VOID)\n"
                                 "{\n"
                                 "    return &KbdTables;\n"
                                 "}\n");

    /* Clean up */
    fclose(FileHandle);
    return TRUE;
}

ULONG
DoOutput(IN ULONG StateCount,
         IN PULONG ShiftStates,
         IN PKEYNAME DescriptionData,
         IN PKEYNAME LanguageData,
         IN PVOID AttributeData,
         IN PVOID DeadKeyData,
         IN PVOID LigatureData,
         IN PKEYNAME KeyNameData,
         IN PKEYNAME KeyNameExtData,
         IN PKEYNAME KeyNameDeadData)
{
    ULONG FailureCode = 0;

    /* Take the time */
    time(&Clock);
    Now = localtime(&Clock);

    /* Check if this just a fallback driver*/
    if (!FallbackDriver)
    {
        /* It's not, create header file */
        if (!kbd_h(&g_Layout)) FailureCode = 1;

        /* Create the resource file */
        if (!kbd_rc(DescriptionData, LanguageData)) FailureCode = 2;
    }

    /* Create the C file */
    if (!kbd_c(StateCount,
               ShiftStates,
               AttributeData,
               &g_Layout,
               DeadKeyData,
               LigatureData,
               KeyNameData,
               KeyNameExtData,
               KeyNameDeadData))
    {
        /* Failed in C file generation */
        FailureCode = 3;
    }

    /* Check if this just a fallback driver*/
    if (!FallbackDriver)
    {
        /* Generate the definition file */
        if (!kbd_def()) FailureCode = 4;
    }

    /* Done */
    return FailureCode;
}


/* EOF */
