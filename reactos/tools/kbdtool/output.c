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

/* FUNCTIONS ******************************************************************/

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
    /* FIXME: Stub */
    return FALSE;   
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
    /* FIXME: Stub */
    return FALSE;
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
