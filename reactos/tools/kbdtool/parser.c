/*
 * PROJECT:         ReactOS Build Tools [Keyboard Layout Compiler]
 * LICENSE:         BSD - See COPYING.BSD in the top level directory
 * FILE:            tools/kbdtool/parser.c
 * PURPOSE:         Parsing Logic
 * PROGRAMMERS:     ReactOS Foundation
 */

/* INCLUDES *******************************************************************/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <host/typedefs.h>

/* GLOBALS ********************************************************************/

#define KEYWORD_COUNT 17

extern BOOLEAN Verbose, UnicodeFile;
extern PCHAR gpszFileName;
extern FILE* gfpInput;
CHAR gBuf[256];
CHAR gKBDName[10];
CHAR gCopyright[256];
CHAR gDescription[256];
CHAR gCompany[256];
CHAR gLocaleName[256];
ULONG gID = 0;
ULONG gKbdLayoutVersion;
CHAR g_Layout[4096];
ULONG gLineCount;
PCHAR KeyWordList[KEYWORD_COUNT] =
{
    "KBD",
    "VERSION",
    "COPYRIGHT",
    "COMPANY",
    "LOCALENAME",
    "MODIIFERS",
    "SHIFTSTATE",
    "ATTRIBUTES",
    "LAYOUT",
    "DEADKEY",
    "LIGATURE",
    "KEYNAME",
    "KEYNAME_EXT",
    "KEYNAME_DEAD",
    "DESCRIPTIONS",
    "LANGUAGENAMES",
    "ENDKBD",
};

/* FUNCTIONS ******************************************************************/

ULONG
isKeyWord(PCHAR p)
{
    ULONG i;
    
    /* Check if we know this keyword */
    for (i = 0; i < KEYWORD_COUNT; i++) if (strcmp(KeyWordList[i], p) == 0) break;
    
    /* If we didn't find anything, i will be KEYWORD_COUNT, which is invalid */
    return i;
}

BOOLEAN
NextLine(PCHAR LineBuffer,
         ULONG BufferSize,
         FILE *File)
{
    PCHAR p, pp;
    
    /* Scan each line */
    while (fgets(LineBuffer, BufferSize, File))
    {
        /* Remember it */
        gLineCount++;
        
        /* Reset the pointer at the beginning of the line */
        p = LineBuffer; 

        /* Now bypass all whitespace (and tabspace) */
        while ((*p) && ((*p == ' ') || (*p == '\t'))) p++;
        
        /* If this is an old-style comment, skip the line */
        if (*p == ';')  continue;
        
        /* Otherwise, check for new-style comment */
        pp = strstr(p, "//");
        if (pp)
        {
            /* We have a comment, so terminate there (unless the whole line is one) */
            if (pp == p) continue;
            *pp = '\0';
        }
        else
        {
            /* No comment, so find the new line and terminate there */
            p = strchr(p, '\n');
            if (p) *p = '\0';
        }
        
        /* We have a line! */
        return TRUE;
    }
    
    /* No line found */
    return FALSE;
}

ULONG
SkipLines(VOID)
{
    ULONG KeyWord;
    CHAR KeyWordChars[32];
    
    /* Scan each line, skipping it if it's not a keyword */
    while (NextLine(gBuf, sizeof(gBuf), gfpInput))
    {
        /* Read a single word */
        if (sscanf(gBuf, "%s", KeyWordChars) == 1)
        {
            /* If the word is a keyword, stop skipping lines */
            KeyWord = isKeyWord(KeyWordChars);
            if (KeyWord < KEYWORD_COUNT) return KeyWord;
        }
    }
    
    /* We skipped all the possible lines, not finding anything */
    return KEYWORD_COUNT;
}

ULONG
DoKBD(VOID)
{    
    /* On Unicode files, we need to find the Unicode marker (FEEF) */
    ASSERT(UnicodeFile == FALSE);
    
    /* Initial values */
    *gKBDName = '\0';
    *gDescription = '\0';
    
    /* Scan for the values */
    if (sscanf(gBuf, "KBD %8s \"%40[^\"]\" %d", gKBDName, gDescription, &gID) < 2)
    {
        /* Couldn't find them */
        printf("Unable to read keyboard name or description.\n");
        exit(1);
    }
    
    /* Debug only */
    DPRINT1("KBD Name: [%8s] Description: [%40s] ID: [%d]\n", gKBDName, gDescription, gID);
    return SkipLines();
}

ULONG
DoVERSION(VOID)
{           
    /* Scan for the value */
    if (sscanf(gBuf, "VERSION %d", &gKbdLayoutVersion) < 1)
    {
        /* Couldn't find them */
        printf("Unable to read keyboard version information.\n");
    }
    
    /* Debug only */
    DPRINT1("VERSION [%d]\n", gKbdLayoutVersion);
    return SkipLines();
}

ULONG
DoCOPYRIGHT(VOID)
{    
    /* Initial values */
    *gCopyright = '\0';
    
    /* Scan for the value */
    if (sscanf(gBuf, "COPYRIGHT \"%40[^\"]\"", gCopyright) < 1)
    {
        /* Couldn't find them */
        printf("Unable to read the specified COPYRIGHT string.\n");
    }
    
    /* Debug only */
    DPRINT1("COPYRIGHT [%40s]\n", gCopyright);
    return SkipLines();
}

ULONG
DoCOMPANY(VOID)
{    
    /* Initial values */
    *gCompany = '\0';
    
    /* Scan for the value */
    if (sscanf(gBuf, "COMPANY \"%85[^\"]\"", gCompany) < 1)
    {
        /* Couldn't find them */
        printf("Unable to read the specified COMPANY name.\n");
    }
    
    /* Debug only */
    DPRINT1("COMPANY [%85s]\n", gCompany);
    return SkipLines();
}

ULONG
DoLOCALENAME(VOID)
{    
    /* Initial values */
    *gLocaleName = '\0';
    
    /* Scan for the value */
    if (sscanf(gBuf, "LOCALENAME \"%40[^\"]\"", gLocaleName) < 1)
    {
        /* Couldn't find them */
        printf("Unable to read the specified COPYRIGHT string.\n");
    }
    
    /* Debug only */
    DPRINT1("LOCALENAME [%40s]\n", gLocaleName);
    return SkipLines();
}

ULONG
DoMODIFIERS(VOID)
{    
    return SkipLines();
}

ULONG
DoDESCRIPTIONS(PVOID DescriptionData)
{
    return SkipLines();
}

ULONG
DoKEYNAME(PVOID KeyNameData)
{
    return SkipLines();   
}

ULONG
DoLIGATURE(PVOID LigatureData)
{
    return SkipLines();
}

ULONG
DoATTRIBUTES(PVOID AttributeData)
{
    return SkipLines();
}

ULONG
DoLANGUAGENAMES(PVOID LanguageData)
{
    return SkipLines();
}

ULONG
DoDEADKEY(PVOID DeadKeyData)
{
    return SkipLines();
}

ULONG
DoSHIFTSTATE(IN PULONG StateCount,
             IN OUT PULONG ShiftStates)
{
    *StateCount = 10;
    return SkipLines();
}

ULONG
DoLAYOUT(IN PVOID LayoutData,
         IN PVOID LigatureData,
         IN PULONG ShiftStates,
         IN ULONG StateCount)
{
    return SkipLines();
}

VOID
DoParsing(VOID)
{
    ULONG KeyWords[KEYWORD_COUNT];
    ULONG KeyWord;
    ULONG StateCount;
    ULONG ShiftStates[8];
    PVOID AttributeData, LigatureData, DeadKeyData, DescriptionData;
    PVOID KeyNameData, KeyNameExtData, KeyNameDeadData, LanguageData;
    
    /* Parse keywords */
    gLineCount = 0;
    KeyWord = SkipLines();
    if (KeyWord >= KEYWORD_COUNT)
    {
        /* Invalid keyword count, fail */
        fclose(gfpInput);
        printf("No keywords were found in '%s'.\n", gpszFileName);
        exit(1);
    }

    /* Now parse the keywords */
    while (KeyWord < (KEYWORD_COUNT - 1))
    {
        /* Save this keyword */
        KeyWords[KeyWord]++;
        
        /* Check for duplicate entires, other than DEADKEY, which is okay */
        if ((KeyWord != 9) && (KeyWords[KeyWord] > 1) && (Verbose))
        {
            /* On a verbose run, warn the user */
            printf("The '%s' keyword appeared multiple times.\n",
                   KeyWordList[KeyWord]);
        }
               
        
        /* Now parse this keyword */
        switch (KeyWord)
        {
            /* KBD */
            case 0:
                
                printf("Found KBD section\n");
                KeyWord = DoKBD();
                break;
                
            /* VERSION */
            case 1:
                
                printf("Found VERSION section\n");
                KeyWord = DoVERSION();
                break;
                
            /* COPYRIGHT */
            case 2:
                
                printf("Found COPYRIGHT section\n");
                KeyWord = DoCOPYRIGHT();
                break;
                
            /* COMPANY */
            case 3:
                
                printf("Found COMPANY section\n");
                KeyWord = DoCOMPANY();
                break;
                
            /* LOCALENAME */
            case 4:
                
                printf("Found LOCALENAME section\n");
                KeyWord = DoLOCALENAME();
                break;
            
            /* MODIFIERS */
            case 5:
                
                printf("Found MODIFIERS section\n");
                KeyWord = DoMODIFIERS();
                break;
                
            /* SHIFTSTATE */
            case 6:
                
                printf("Found SHIFTSTATE section\n");
                KeyWord = DoSHIFTSTATE(&StateCount, ShiftStates);
                if (StateCount < 2)
                {
                    /* Fail */
                    fclose(gfpInput);
                    printf("ERROR");
                    exit(1);
                }
                break;
                
            /* ATTRIBUTES */
            case 7:
                
                printf("Found ATTRIBUTES section\n");
                KeyWord = DoATTRIBUTES(&AttributeData);
                break;
                
            /* LAYOUT */
            case 8:
                
                printf("Found LAYOUT section\n");
                KeyWord = DoLAYOUT(&g_Layout,
                                   &LigatureData,
                                   ShiftStates,
                                   StateCount);
                break;
                
            /* DEADKEY */
            case 9:
                
                printf("Found DEADKEY section\n");
                KeyWord = DoDEADKEY(&DeadKeyData);
                break;
                
            /* LIGATURE */
            case 10:
                
                printf("Found LIGATURE section\n");
                KeyWord = DoLIGATURE(&LigatureData);
                break;
                
            /* KEYNAME */
            case 11:
                
                printf("Found KEYNAME section\n");
                KeyWord = DoKEYNAME(&KeyNameData);
                break;
                
            /* KEYNAME_EXT */
            case 12:
                
                printf("Found KEYNAME_EXT section\n");
                KeyWord = DoKEYNAME(&KeyNameExtData);
                break;
                
            /* KEYNAME_DEAD */
            case 13:
                
                printf("Found KEYNAME_DEAD section\n");
                KeyWord = DoKEYNAME(&KeyNameDeadData);
                break;
                
            /* DESCRIPTIONS */
            case 14:
                
                printf("Found DESCRIPTIONS section\n");
                KeyWord = DoDESCRIPTIONS(&DescriptionData);
                break;
                
            /* LANGUAGENAMES */
            case 15:
                
                printf("Found LANGUAGENAMES section\n");
                KeyWord = DoLANGUAGENAMES(&LanguageData);
                break;
                
            /* ENDKBD */
            case 16:
                
                printf("Found ENDKBD section\n");
                KeyWord = SkipLines();
                break;
                
                
            default:
                break;
        }
    }   
}
/* EOF */
