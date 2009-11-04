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
    BOOLEAN Processed;
} SCVK, *PSCVK;

typedef struct tagVKNAME
{
    ULONG VirtualKey;
    PCHAR Name;
} VKNAME, *PVKNAME;

typedef struct tagLAYOUTENTRY
{
    USHORT ScanCode;
    UCHAR VirtualKey;
    UCHAR OriginalVirtualKey;
    ULONG Cap;
    ULONG StateCount;
    ULONG CharData[8];
    ULONG DeadCharData[8];
    ULONG OtherCharData[8];
    struct LAYOUTENTRY* CapData;
    PCHAR Name;
    ULONG Processed;
    ULONG LineCount;
} LAYOUTENTRY, *PLAYOUTENTRY;

typedef struct tagLAYOUT
{
    LAYOUTENTRY Entry[110];
} LAYOUT, *PLAYOUT;

/* GLOBALS ********************************************************************/

#define KEYWORD_COUNT 17

extern BOOLEAN Verbose, UnicodeFile, SanityCheck;
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
LAYOUT g_Layout;
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
    {0x02, '1', NULL, FALSE},
    {0x03, '2', NULL, FALSE},
    {0x04, '3', NULL, FALSE},
    {0x05, '4', NULL, FALSE},
    {0x06, '5', NULL, FALSE},
    {0x07, '6', NULL, FALSE},
    {0x08, '7', NULL, FALSE},
    {0x09, '8', NULL, FALSE},
    {0x0a, '9', NULL, FALSE},
    {0x0b, '0', NULL, FALSE},
    {0x0c, 0xbd, NULL, FALSE},
    {0x0d, 0xbb, NULL, FALSE},
    {0x10, 'Q', NULL, FALSE},
    {0x11, 'W', NULL, FALSE},
    {0x12, 'E', NULL, FALSE},
    {0x13, 'R', NULL, FALSE},
    {0x14, 'T', NULL, FALSE},
    {0x15, 'Y', NULL, FALSE},
    {0x16, 'U', NULL, FALSE},
    {0x17, 'I', NULL, FALSE},
    {0x18, 'O', NULL, FALSE},
    {0x19, 'P', NULL, FALSE},
    {0x1a, 0xdb, NULL, FALSE},
    {0x1b, 0xdd, NULL, FALSE},
    {0x1e, 'A', NULL, FALSE},
    {0x1f, 'S', NULL, FALSE},
    {0x20, 'D', NULL, FALSE},
    {0x21, 'F', NULL, FALSE},
    {0x22, 'G', NULL, FALSE},
    {0x23, 'H', NULL, FALSE},
    {0x24, 'J', NULL, FALSE},
    {0x25, 'K', NULL, FALSE},
    {0x26, 'L', NULL, FALSE},
    {0x27, 0xba, NULL, FALSE},
    {0x28, 0xde, NULL, FALSE},
    {0x29, 0xc0, NULL, FALSE},
    {0x2b, 0xdc, NULL, FALSE},
    {0x2c, 'Z', NULL, FALSE},
    {0x2d, 'X', NULL, FALSE},
    {0x2e, 'C', NULL, FALSE},
    {0x2f, 'V', NULL, FALSE},
    {0x30, 'B', NULL, FALSE},
    {0x31, 'N', NULL, FALSE},
    {0x32, 'M', NULL, FALSE},
    {0x33, 0xbc, NULL, FALSE},
    {0x34, 0xbe, NULL, FALSE},
    {0x35, 0xbf, NULL, FALSE},
    {0x53, 0x6e, NULL, FALSE},
    {0x56, 0xe2, NULL, FALSE},
    {0x73, 0xc1, NULL, FALSE},
    {0x7e, 0xc2, NULL, FALSE},
    {0xe010, 0xb1, "Speedracer: Previous Track", FALSE},
    {0xe019, 0xb0, "Speedracer: Next Track", FALSE},
    {0xe01d, 0xa3, "RControl", FALSE},
    {0xe020, 0xad, "Speedracer: Volume Mute", FALSE},
    {0xe021, 0xb7, "Speedracer: Launch App 2", FALSE},
    {0xe022, 0xb3, "Speedracer: Media Play/Pause", FALSE},
    {0xe024, 0xb2, "Speedracer: Media Stop", FALSE},
    {0xe02e, 0xae, "Speedracer: Volume Up", FALSE},
    {0xe030, 0xaf, "Speedracer: Volume Down", FALSE},
    {0xe032, 0xac, "Speedracer: Browser Home", FALSE},
    {0xe035, 0x6f, "Numpad Divide", FALSE},
    {0xe037, 0x2c, "Snapshot", FALSE},
    {0xe038, 0xa5, "RMenu", FALSE},
    {0xe047, 0x24, "Home", FALSE},
    {0xe048, 0x26, "Up", FALSE},
    {0xe049, 0x21, "Prior", FALSE},
    {0xe04b, 0x25, "Left", FALSE},
    {0xe04d, 0x27, "Right", FALSE},
    {0xe04f, 0x23, "End", FALSE},
    {0xe050, 0x28, "Down", FALSE},
    {0xe051, 0x22, "Next", FALSE},
    {0xe052, 0x2d, "Insert", FALSE},
    {0xe053, 0x2e, "Delete", FALSE},
    {0xe05b, 0x5b, "Left Win", FALSE},
    {0xe05c, 0x5c, "Right Win", FALSE},
    {0xe05d, 0x5d, "Application", FALSE},
    {0xe05e, 0xff, "Power", FALSE},
    {0xe05f, 0x5f, "Speedracer: Sleep", FALSE},
    {0xe060, 0xff, "BAD SCANCODE", FALSE},
    {0xe061, 0xff, "BAD SCANCODE", FALSE},
    {0xe065, 0xaa, "Speedracer: Browser Search", FALSE},
    {0xe066, 0xab, "Speedracer: Browser Favorites", FALSE},
    {0xe067, 0xa8, "Speedracer: Browser Refresh", FALSE},
    {0xe068, 0xa9, "Speedracer: Browser Stop", FALSE},
    {0xe069, 0xa7, "Speedracer: Browser Foward", FALSE},
    {0xe06a, 0xa6, "Speedracer: Browser Back", FALSE},
    {0xe06b, 0xb6, "Speedracer: Launch App 1", FALSE},
    {0xe06c, 0xb4, "Speedracer: Launch Mail", FALSE},
    {0xe06d, 0xb5, "Speedracer: Launch Media Selector", FALSE},
    {0x53, 0x6e, NULL, FALSE},
    {0x0e, 0x08, NULL, FALSE},
    {0x01, 0x1b, NULL, FALSE},
    {0xe01c, 0x0d, "Numpad Enter", FALSE},
    {0x1c, 0x0d, NULL, FALSE},
    {0x39, 0x20, NULL, FALSE},
    {0xe046, 0x03, "Break (Ctrl + Pause)", FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE},
    {0xFFFF, 0x00, NULL, FALSE}
};

VKNAME VKName[] =
{
    {0x08, "BACK"},
    {0x03, "CANCEL"},
    {0x1b, "ESCAPE"},
    {0x0d, "RETURN"},
    {0x20, "SPACE"},
    {0x6e, "DECIMAL"},
    {0xba, "OEM_1"},
    {0xbb, "OEM_PLUS"},
    {0xbc, "OEM_COMMA"},
    {0xbd, "OEM_MINUS"},
    {0xbe, "OEM_PERIOD"},
    {0xbf, "OEM_2"},
    {0xc0, "OEM_3"},
    {0xdb, "OEM_4"},
    {0xdc, "OEM_5"},
    {0xdd, "OEM_6"},
    {0xde, "OEM_7"},
    {0xdf, "OEM_8"},
    {0xe2, "OEM_102"},
    {0xc1, "ABNT_C1"},
    {0xc2, "ABNT_C2"},
    {0x10, "SHIFT"},
    {0xa0, "LSHIFT"},
    {0xa1, "RSHIFT"},
    {0x12, "MENU"},
    {0xa4, "LMENU"},
    {0xa5, "RMENU"},
    {0x11, "CONTROL"},
    {0xa2, "LCONTROL"},
    {0xa3, "RCONTROL"},
    {0x6c, "SEPARATOR"},
    {0xe4, "ICO_00"},
    {0x2e, "DELETE"},
    {0x2d, "INSERT"},
    {0xe5, "GROUPSHIFT"},
    {0xe6, "RGROUPSHIFT"}
};

#define CHAR_NORMAL_KEY   0
#define CHAR_DEAD_KEY     1
#define CHAR_OTHER_KEY    2
#define CHAR_INVALID_KEY  3
#define CHAR_LIGATURE_KEY 4

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

ULONG
getVKNum(IN PCHAR p)
{
    ULONG Length;
    ULONG i;
    ULONG KeyNumber;
    
    /* Compute the length of the string */
    Length = strlen(p);
    if (!Length) return -1;
    
    /* Check if this is is a simple key */
    if (Length == 1)
    {
        /* If it's a number, return it now */
        if ((*p >= '0') && (*p <= '9')) return *p;

        /* Otherwise, convert the letter to upper case */
        *p = toupper(*p);
        
        /* And make sure it's a valid letter */
        if ((*p >= 'A') && (*p <='Z')) return *p;
        
        /* Otherwise, fail */
        return -1;
    }

    /* Otherwise, scan our virtual key names */
    for (i = 0; i < 36; i++)
    {
        /* Check if we have a match */
        if (!strcmp(VKName[i].Name, p)) return VKName[i].VirtualKey;
    }
    
    /*  Check if this is a hex string */
    if ((*p == '0') && ((*(p + 1) == 'x') || (*(p + 1) == 'X')))
    {
        /* Get the key number from the hex string */
        *(p + 1) = 'x';
        if (sscanf(p, "0x%x", &KeyNumber) == 1) return KeyNumber;
    }

    /* No hope: fail */
    return -1;
}

UCHAR
getCharacterInfo(IN PCHAR State,
                 OUT PULONG EntryChar,
                 OUT PCHAR LigatureChar)
{
    ULONG Length;
    ULONG CharInfo = CHAR_NORMAL_KEY;
    UCHAR StateChar;
    ULONG CharCode;
    
    /* Calculate the length of the state */
    Length = strlen(State);
    
    /* Check if this is at least a simple key state */
    if (Length > 1)
    {
        /* Read the first character and check if it's a dead key */
        StateChar = State[Length - 1];
        if (StateChar == '@')
        {
            /* This is a dead key */
            CharInfo = CHAR_DEAD_KEY;
        }
        else if (StateChar == '%')
        {
            /* This is another key */
            CharInfo = CHAR_OTHER_KEY;
        }
    }
    
    /* Check if this is a numerical key state */
    if ((Length - 1) >= 2)
    {
        /* Scan for extended character code entry */
        if ((sscanf(State, "%6x", &CharCode) == 1) &&
            ((Length == 5) && (State[0] == '0') ||
             (Length == 6) && ((State[0] == '0') && (State[1] == '0'))))
        {
            /* Handle a ligature key */
            CharInfo = CHAR_LIGATURE_KEY;
            
            /* Not yet handled */
            printf("Ligatured character entries not yet supported!\n");
            exit(1);
        }
        else
        {
            /* Get the normal character entry */
            if (sscanf(State, "%4x", &CharCode) == 1)
            {
                /* Does the caller want the key? */
                if (EntryChar) *EntryChar = CharCode;
            }
            else
            {
                /* The entry is totally invalid */
                if (Verbose) printf("An unparseable character entry '%s' was found.\n", State);
                if (EntryChar) *EntryChar = 0;
                CharInfo = CHAR_INVALID_KEY;
            }
        }
    }
    else
    {
        /* Save the key if the caller requested it */
        if (EntryChar) *EntryChar = *State;
    }

    /* Return the type of character this is */
    return CharInfo;
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
DoLAYOUT(IN PLAYOUT LayoutData,
         IN PVOID LigatureData,
         IN PULONG ShiftStates,
         IN ULONG StateCount)
{
    CHAR Token[32];
    CHAR Cap[8];
    ULONG KeyWord;
    ULONG ScanCode, CurrentCode;
    ULONG TokenCount;
    ULONG VirtualKey;
    ULONG i;
    ULONG Count;
    BOOLEAN FullEntry;
    CHAR State[8][8];
    ULONG ScanCodeCount = -1;
    PLAYOUTENTRY Entry;
    UCHAR CharacterType, LigatureChar;
    
    /* Only attempt this is Verbose is enabled (FOR DEBUGGING ONLY) */
    if (!Verbose) return SkipLines();
    
    /* Zero out the layout */
    memset(LayoutData, 0, sizeof(LAYOUT));
    
    /* Read each line */
    Entry = &LayoutData->Entry[0];
    while (NextLine(gBuf, 256, gfpInput))
    {
        /* Search for token */
        if (sscanf(gBuf, "%s", Token) != 1) continue;
        
        /* Make sure it's not just a comment */
        if (*Token == ';') continue;
        
        /* Make sure it's not a keyword */
        KeyWord = isKeyWord(Token);
        if (KeyWord < KEYWORD_COUNT) break;
        
        /* Now read the entry */
        TokenCount = sscanf(gBuf, " %x %s %s", &ScanCode, Token, Cap);
        if (TokenCount == 3)
        {
            /* Full entry with cap */
            FullEntry = TRUE;
        }
        else if (TokenCount != 2)
        {
            /* Fail, invalid LAYOUT entry */
            printf("There are not enough columns in the layout list.\n");
            exit(1);
        }
        else
        {
            /* Simplified layout with no cap */
            FullEntry = FALSE;
        }
        
        /* One more */
        Entry++;
        if (++ScanCodeCount >= 110)
        {
            /* Too many! */
            printf("ScanCode %02x - too many scancodes here to parse.\n", ScanCode);
            exit(1);
        }
        
        /* Fill out this entry */
        Entry->ScanCode = ScanCode;
        Entry->LineCount = gLineCount;
        
        /* Loop scancode table */
        for (i = 0; i < 110; i++)
        {
            /* Get the current code */
            CurrentCode = ScVk[i].ScanCode;
            if (CurrentCode == 0xFFFF)
            {
                /* New code */
                if (Verbose) printf("A new scancode is being defined: 0x%2X, %s\n", Entry->ScanCode, Token);
                
                /* Fill out the entry */
                Entry->VirtualKey = getVKNum(Token);
                break;
            }
            else if (ScanCode == CurrentCode)
            {
                /* Make sure we didn't already process it */
                if (ScVk[i].Processed)
                {
                    /* Fail */
                    printf("Scancode %X was previously defined.\n", ScanCode);
                    exit(1);
                }
                
                /* Check if there is a valid virtual key */
                if (ScVk[i].VirtualKey == 0xFFFF)
                {
                    /* Fail */
                    printf("The Scancode you tried to use (%X) is reserved.\n", ScanCode);
                    exit(1);
                }
                
                /* Fill out the entry */
                Entry->OriginalVirtualKey = ScVk[i].VirtualKey;
                Entry->Name = ScVk[i].Name;
                break;
            }
        }
        
        /* The entry is now processed */
        Entry->Processed = TRUE;
        ScVk[i].Processed = TRUE;
        
        /* Get the virtual key from the entry */
        VirtualKey = getVKNum(Token);
        Entry->VirtualKey = VirtualKey;
        
        /* Make sure it's valid */
        if (VirtualKey == 0xFFFF)
        {
            /* Warn the user */
            if (Verbose) printf("An invalid Virtual Key '%s' was defined.\n", Token);
            continue;
        }
        
        /* Is this a full entry */
        if (FullEntry)
        {
            /* Do we have SGCAP data? Set cap mode to 2 */
            if (!strcmp(Cap, "SGCAP")) *Cap = '2';
            
            /* Read the cap mode */
            if (sscanf(Cap, "%1d[012]", &Entry->Cap) != 1)
            {
                /* Invalid cap mode */
                printf("invalid Cap specified (%s). Must be 0, 1, or 2.\n", Cap);
                exit(1);
            }
        }
        
        /* Read the states */
        Count = sscanf(gBuf,
                       " %*s %*s %*s %s %s %s %s %s %s %s %s",
                       State[0],
                       State[1],
                       State[2],
                       State[3],
                       State[4],
                       State[5],
                       State[6],
                       State[7]);
        Entry->StateCount = Count;
        
        /* Check if there are less than 2 states */
        if ((Count < 2) && (FullEntry))
        {
            /* Fail */
            printf("You must have at least 2 characters.\n");
            exit(1);
        }
        
        /* Loop all states */
        for (i = 0; i < Count; i++)
        {
            /* Check if this is an undefined state */
            if (!strcmp(State[i], "-1"))
            {
                /* No data for this state */
                Entry->CharData[i] = -1;
                continue;
            }
            
            /* Otherwise, check what kind of character this is */
            CharacterType = getCharacterInfo(State[i],
                                             &Entry->CharData[i],
                                             &LigatureChar);
            if (CharacterType == CHAR_DEAD_KEY)
            {
                /* Save it as such */
                Entry->DeadCharData[i] = 1;
            }
            else if (CharacterType == CHAR_OTHER_KEY)
            {
                /* Save it as such */
                Entry->OtherCharData[i] = 1;
            }
        }
        
        /* Check for sanity checks */
        if (SanityCheck)
        {
            /* Not yet handled... */
            printf("Sanity checks not yet handled!\n");
            exit(1);   
        }
        
        /* Check if we had SGCAP data */
        if (Entry->Cap & 2)
        {
            /* Not yet handled... */
            printf("SGCAP state not yet handled!\n");
            exit(1);
        }
    }
    
    /* Skip what's left */
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
