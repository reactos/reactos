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

typedef struct tagKEYNAME
{
    ULONG Code;
    PCHAR Name;
    struct tagKEYNAME* Next;
} KEYNAME, *PKEYNAME;

typedef struct tagSCVK
{
    USHORT ScanCode;
    USHORT VirtualKey;
    PCHAR Name;
    PVOID Reserved;
} SCVK, *PSCVK;

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

/* ISO 110-key Keyboard Scancode to Virtual Key Conversion Table */
SCVK ScVk[] =
{
    {0x02, '1', NULL, NULL},
    {0x03, '2', NULL, NULL},
    {0x04, '3', NULL, NULL},
    {0x05, '4', NULL, NULL},
    {0x06, '5', NULL, NULL},
    {0x07, '6', NULL, NULL},
    {0x08, '7', NULL, NULL},
    {0x09, '8', NULL, NULL},
    {0x0a, '9', NULL, NULL},
    {0x0b, '0', NULL, NULL},
    {0x0c, 0xbd, NULL, NULL},
    {0x0d, 0xbb, NULL, NULL},
    {0x10, 'Q', NULL, NULL},
    {0x11, 'W', NULL, NULL},
    {0x12, 'E', NULL, NULL},
    {0x13, 'R', NULL, NULL},
    {0x14, 'T', NULL, NULL},
    {0x15, 'Y', NULL, NULL},
    {0x16, 'U', NULL, NULL},
    {0x17, 'I', NULL, NULL},
    {0x18, 'O', NULL, NULL},
    {0x19, 'P', NULL, NULL},
    {0x1a, 0xdb, NULL, NULL},
    {0x1b, 0xdd, NULL, NULL},
    {0x1e, 'A', NULL, NULL},
    {0x1f, 'S', NULL, NULL},
    {0x20, 'D', NULL, NULL},
    {0x21, 'F', NULL, NULL},
    {0x22, 'G', NULL, NULL},
    {0x23, 'H', NULL, NULL},
    {0x24, 'J', NULL, NULL},
    {0x25, 'K', NULL, NULL},
    {0x26, 'L', NULL, NULL},
    {0x27, 0xba, NULL, NULL},
    {0x28, 0xde, NULL, NULL},
    {0x29, 0xc0, NULL, NULL},
    {0x2b, 0xdc, NULL, NULL},
    {0x2c, 'Z', NULL, NULL},
    {0x2d, 'X', NULL, NULL},
    {0x2e, 'C', NULL, NULL},
    {0x2f, 'V', NULL, NULL},
    {0x30, 'B', NULL, NULL},
    {0x31, 'N', NULL, NULL},
    {0x32, 'M', NULL, NULL},
    {0x33, 0xbc, NULL, NULL},
    {0x34, 0xbe, NULL, NULL},
    {0x35, 0xbf, NULL, NULL},
    {0x53, 0x6e, NULL, NULL},
    {0x56, 0xe2, NULL, NULL},
    {0x73, 0xc1, NULL, NULL},
    {0x7e, 0xc2, NULL, NULL},
    {0xe010, 0xb1, "Speedracer: Previous Track", NULL},
    {0xe019, 0xb0, "Speedracer: Next Track", NULL},
    {0xe01d, 0xa3, "RControl", NULL},
    {0xe020, 0xad, "Speedracer: Volume Mute", NULL},
    {0xe021, 0xb7, "Speedracer: Launch App 2", NULL},
    {0xe022, 0xb3, "Speedracer: Media Play/Pause", NULL},
    {0xe024, 0xb2, "Speedracer: Media Stop", NULL},
    {0xe02e, 0xae, "Speedracer: Volume Up", NULL},
    {0xe030, 0xaf, "Speedracer: Volume Down", NULL},
    {0xe032, 0xac, "Speedracer: Browser Home", NULL},
    {0xe035, 0x6f, "Numpad Divide", NULL},
    {0xe037, 0x2c, "Snapshot", NULL},
    {0xe038, 0xa5, "RMenu", NULL},
    {0xe047, 0x24, "Home", NULL},
    {0xe048, 0x26, "Up", NULL},
    {0xe049, 0x21, "Prior", NULL},
    {0xe04b, 0x25, "Left", NULL},
    {0xe04d, 0x27, "Right", NULL},
    {0xe04f, 0x23, "End", NULL},
    {0xe050, 0x28, "Down", NULL},
    {0xe051, 0x22, "Next", NULL},
    {0xe052, 0x2d, "Insert", NULL},
    {0xe053, 0x2e, "Delete", NULL},
    {0xe05b, 0x5b, "Left Win", NULL},
    {0xe05c, 0x5c, "Right Win", NULL},
    {0xe05d, 0x5d, "Application", NULL},
    {0xe05e, 0xff, "Power", NULL},
    {0xe05f, 0x5f, "Speedracer: Sleep", NULL},
    {0xe060, 0xff, "BAD SCANCODE", NULL},
    {0xe061, 0xff, "BAD SCANCODE", NULL},
    {0xe065, 0xaa, "Speedracer: Browser Search", NULL},
    {0xe066, 0xab, "Speedracer: Browser Favorites", NULL},
    {0xe067, 0xa8, "Speedracer: Browser Refresh", NULL},
    {0xe068, 0xa9, "Speedracer: Browser Stop", NULL},
    {0xe069, 0xa7, "Speedracer: Browser Foward", NULL},
    {0xe06a, 0xa6, "Speedracer: Browser Back", NULL},
    {0xe06b, 0xb6, "Speedracer: Launch App 1", NULL},
    {0xe06c, 0xb4, "Speedracer: Launch Mail", NULL},
    {0xe06d, 0xb5, "Speedracer: Launch Media Selector", NULL},
    {0x53, 0x6e, NULL, NULL},
    {0x0e, 0x08, NULL, NULL},
    {0x01, 0x1b, NULL, NULL},
    {0xe01c, 0x0d, "Numpad Enter", NULL},
    {0x1c, 0x0d, NULL, NULL},
    {0x39, 0x20, NULL, NULL},
    {0xe046, 0x03, "Break (Ctrl + Pause)", NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL},
    {0xFFFF, 0x00, NULL, NULL}
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
DoDESCRIPTIONS(IN PKEYNAME* DescriptionData)
{
    ULONG KeyWord = 0;
    CHAR Token[32];
    ULONG LanguageCode;
    PCHAR p, pp;
    PKEYNAME Description;
    
    /* Assume nothing */
    *DescriptionData = 0;
    
    /* Start scanning */
    while (NextLine(gBuf, 256, gfpInput))
    {        
        /* Search for token */
        if (sscanf(gBuf, "%s", Token) != 1) continue;
        
        /* Make sure it's not just a comment */
        if (*Token == ';') continue;
        
        /* Make sure it's not a keyword */
        KeyWord = isKeyWord(Token);
        if (KeyWord < KEYWORD_COUNT) break;
        
        /* Now scan for the language code */
        if (sscanf(Token, " %4x", &LanguageCode) != 1)
        {
            /* Skip */
            printf("An invalid LANGID was specified.\n");
            continue;
        }
        
        /* Now get the actual description */
        if (sscanf(gBuf, " %*4x %s[^\n]", Token) != 1)
        {
            /* Skip */
            printf("A language description is missing.\n");
            continue;
        }
        
        /* Get the description string and find the ending */
        p = strstr(gBuf, Token);
        pp = strchr(p, '\n');
        if (!pp) pp = strchr(p, '\r');
        
        /* Terminate the description string here */
        if (pp) *pp = 0;
        
        /* Now allocate the description */
        Description = malloc(sizeof(KEYNAME));
        if (!Description)
        {
            /* Fail */
            printf("Unable to allocate the KEYNAME struct (out of memory?).\n");
            exit(1);
        }
        
        /* Fill out the structure */
        Description->Code = LanguageCode;
        Description->Name = strdup(p);
        Description->Next = NULL;
        
        /* Debug only */
        DPRINT1("LANGID: [%4x] Description: [%s]\n", Description->Code, Description->Name);
        
        /* Point to it and advance the pointer */
        *DescriptionData = Description;
        DescriptionData = &Description->Next;
    }

    /* We are done */
    return KeyWord;
}

ULONG
DoLANGUAGENAMES(IN PKEYNAME* LanguageData)
{
    ULONG KeyWord = 0;
    CHAR Token[32];
    ULONG LanguageCode;
    PCHAR p, pp;
    PKEYNAME Language;
    
    /* Assume nothing */
    *LanguageData = 0;
    
    /* Start scanning */
    while (NextLine(gBuf, 256, gfpInput))
    {        
        /* Search for token */
        if (sscanf(gBuf, "%s", Token) != 1) continue;
        
        /* Make sure it's not just a comment */
        if (*Token == ';') continue;
        
        /* Make sure it's not a keyword */
        KeyWord = isKeyWord(Token);
        if (KeyWord < KEYWORD_COUNT) break;
        
        /* Now scan for the language code */
        if (sscanf(Token, " %4x", &LanguageCode) != 1)
        {
            /* Skip */
            printf("An invalid LANGID was specified.\n");
            continue;
        }
        
        /* Now get the actual language */
        if (sscanf(gBuf, " %*4x %s[^\n]", Token) != 1)
        {
            /* Skip */
            printf("A language name is missing\n");
            continue;
        }
        
        /* Get the language string and find the ending */
        p = strstr(gBuf, Token);
        pp = strchr(p, '\n');
        if (!pp) pp = strchr(p, '\r');
        
        /* Terminate the language string here */
        if (pp) *pp = 0;
        
        /* Now allocate the language */
        Language = malloc(sizeof(KEYNAME));
        if (!Language)
        {
            /* Fail */
            printf("Unable to allocate the KEYNAME struct (out of memory?).\n");
            exit(1);
        }
        
        /* Fill out the structure */
        Language->Code = LanguageCode;
        Language->Name = strdup(p);
        Language->Next = NULL;
        
        /* Debug only */
        DPRINT1("LANGID: [%4x] Name: [%s]\n", Language->Code, Language->Name);
        
        /* Point to it and advance the pointer */
        *LanguageData = Language;
        LanguageData = &Language->Next;
    }
    
    /* We are done */
    return KeyWord;
}

ULONG
DoKEYNAME(IN PKEYNAME* KeyNameData)
{
    ULONG KeyWord = 0;
    CHAR Token[32];
    ULONG CharacterCode;
    PCHAR p, pp;
    PKEYNAME KeyName;
    
    /* Assume nothing */
    *KeyNameData = 0;
    
    /* Start scanning */
    while (NextLine(gBuf, 256, gfpInput))
    {        
        /* Search for token */
        if (sscanf(gBuf, "%s", Token) != 1) continue;
        
        /* Make sure it's not just a comment */
        if (*Token == ';') continue;
        
        /* Make sure it's not a keyword */
        KeyWord = isKeyWord(Token);
        if (KeyWord < KEYWORD_COUNT) break;
        
        /* Now scan for the character code */
        if (sscanf(Token, " %4x", &CharacterCode) != 1)
        {
            /* Skip */
            printf("An invalid character code was specified.\n");
            continue;
        }
        
        /* Now get the actual key name */
        if (sscanf(gBuf, " %*4x %s[^\n]", Token) != 1)
        {
            /* Skip */
            printf("A key name is missing\n");
            continue;
        }
        
        /* Get the key name string and find the ending */
        p = strstr(gBuf, Token);
        pp = strchr(p, '\n');
        if (!pp) pp = strchr(p, '\r');
        
        /* Terminate the key name string here */
        if (pp) *pp = 0;
        
        /* Now allocate the language */
        KeyName = malloc(sizeof(KEYNAME));
        if (!KeyName)
        {
            /* Fail */
            printf("Unable to allocate the KEYNAME struct (out of memory?).\n");
            exit(1);
        }
        
        /* Fill out the structure */
        KeyName->Code = CharacterCode;
        KeyName->Name = strdup(p);
        KeyName->Next = NULL;
        
        /* Debug only */
        DPRINT1("CHARCODE: [%4x] Name: [%s]\n", KeyName->Code, KeyName->Name);
        
        /* Point to it and advance the pointer */
        *KeyNameData = KeyName;
        KeyNameData = &KeyName->Next;
    }
    
    /* We are done */
    return KeyWord; 
}

ULONG
DoSHIFTSTATE(IN PULONG StateCount,
             IN OUT PULONG ShiftStates)
{
    ULONG KeyWord;
    ULONG i;
    ULONG ShiftState;
    CHAR Token[32];
    
    /* Reset the shift states */
    for (i = 0; i < 8; i++) ShiftStates[i] = -1;
    
    /* Start with no states */
    *StateCount = 0;
    
    /* Scan for shift states */
    while (NextLine(gBuf, 256, gfpInput))
    {
        /* Search for token */
        if (sscanf(gBuf, "%s", Token) != 1) continue;
        
        /* Make sure it's not a keyword */
        KeyWord = isKeyWord(Token);
        if (KeyWord < KEYWORD_COUNT) break;
        
        /* Now scan for the shift state */
        if (sscanf(gBuf, " %1s[012367]", Token) != 1)
        {
            /* We failed -- should we warn? */
            if (Verbose) printf("An invalid shift state '%s' was found (use 0, 1, 2, 3, 6, or 7.)\n", Token);
            continue;
        }
        
        /* Now read the state */
        ShiftState = atoi(Token);
        
        /* Scan existing states */
        for (i = 0; i < *StateCount; i++)
        {
            /* Check for duplicate */
            if ((ShiftStates[i] == ShiftState) && (Verbose))
            {
                /* Warn user */
                printf("The state '%d' was duplicated for this Virtual Key.\n", ShiftStates[i]);
                break;
            }
        }
        
        /* Make sure we won't overflow */
        if (*StateCount < 8)
        {
            /* Save this state */
            ShiftStates[(*StateCount)++] = ShiftState;
        }
        else
        {
            /* Too many states -- should we warn? */
            if (Verbose) printf("There were too many states (you defined %d).\n", *StateCount);
        }
    }
    
    /* Debug only */
    DPRINT1("Found %d Shift States: [", *StateCount);
    for (i = 0; i < *StateCount; i++) DPRINT1("%d ", ShiftStates[i]);
    DPRINT1("]\n");
    
    /* We are done */
    return KeyWord;
}

ULONG
DoLIGATURE(PVOID LigatureData)
{
    printf("LIGATURE support is not yet implemented. Please bug Arch to fix it\n");
    return SkipLines();
}

ULONG
DoATTRIBUTES(PVOID AttributeData)
{
    printf("ATTRIBUTES support is not yet implemented. Please bug Arch to fix it\n");
    return SkipLines();
}

ULONG
DoMODIFIERS(VOID)
{
    printf("MODIFIERS support is not yet implemented. Please bug Arch to fix it\n");
    return SkipLines();
}

ULONG
DoDEADKEY(PVOID DeadKeyData)
{
    printf("DEADKEY support is not yet implemented. Please bug Arch to fix it\n");
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
    PKEYNAME DescriptionData, LanguageData;
    PKEYNAME KeyNameData, KeyNameExtData, KeyNameDeadData;
    PVOID AttributeData, LigatureData, DeadKeyData;
    
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
                
                DPRINT1("Found KBD section\n");
                KeyWord = DoKBD();
                break;
                
            /* VERSION */
            case 1:
                
                DPRINT1("Found VERSION section\n");
                KeyWord = DoVERSION();
                break;
                
            /* COPYRIGHT */
            case 2:
                
                DPRINT1("Found COPYRIGHT section\n");
                KeyWord = DoCOPYRIGHT();
                break;
                
            /* COMPANY */
            case 3:
                
                DPRINT1("Found COMPANY section\n");
                KeyWord = DoCOMPANY();
                break;
                
            /* LOCALENAME */
            case 4:
                
                DPRINT1("Found LOCALENAME section\n");
                KeyWord = DoLOCALENAME();
                break;
            
            /* MODIFIERS */
            case 5:
                
                DPRINT1("Found MODIFIERS section\n");
                KeyWord = DoMODIFIERS();
                break;
                
            /* SHIFTSTATE */
            case 6:
                
                DPRINT1("Found SHIFTSTATE section\n");
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
                
                DPRINT1("Found ATTRIBUTES section\n");
                KeyWord = DoATTRIBUTES(&AttributeData);
                break;
                
            /* LAYOUT */
            case 8:
                
                DPRINT1("Found LAYOUT section\n");
                KeyWord = DoLAYOUT(&g_Layout,
                                   &LigatureData,
                                   ShiftStates,
                                   StateCount);
                break;
                
            /* DEADKEY */
            case 9:
                
                DPRINT1("Found DEADKEY section\n");
                KeyWord = DoDEADKEY(&DeadKeyData);
                break;
                
            /* LIGATURE */
            case 10:
                
                DPRINT1("Found LIGATURE section\n");
                KeyWord = DoLIGATURE(&LigatureData);
                break;
                
            /* KEYNAME */
            case 11:
                
                DPRINT1("Found KEYNAME section\n");
                KeyWord = DoKEYNAME(&KeyNameData);
                break;
                
            /* KEYNAME_EXT */
            case 12:
                
                DPRINT1("Found KEYNAME_EXT section\n");
                KeyWord = DoKEYNAME(&KeyNameExtData);
                break;
                
            /* KEYNAME_DEAD */
            case 13:
                
                DPRINT1("Found KEYNAME_DEAD section\n");
                KeyWord = DoKEYNAME(&KeyNameDeadData);
                break;
                
            /* DESCRIPTIONS */
            case 14:
                
                DPRINT1("Found DESCRIPTIONS section\n");
                KeyWord = DoDESCRIPTIONS(&DescriptionData);
                break;
                
            /* LANGUAGENAMES */
            case 15:
                
                DPRINT1("Found LANGUAGENAMES section\n");
                KeyWord = DoLANGUAGENAMES(&LanguageData);
                break;
                
            /* ENDKBD */
            case 16:
                
                DPRINT1("Found ENDKBD section\n");
                KeyWord = SkipLines();
                break;
                
                
            default:
                break;
        }
    }   
}
/* EOF */
