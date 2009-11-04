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
    /* FIXME: Stub */
    return FALSE;
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
