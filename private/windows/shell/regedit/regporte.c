/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGPORTE.C
*
*  VERSION:     5.00
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        06 Apr 1994
*
*  File import and export engine routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regresid.h"
#include "reg1632.h"
#include "regedit.h"
#include <malloc.h>

//  Association between the ASCII name and the handle of the registry key.
const REGISTRY_ROOT g_RegistryRoots[] = {
    TEXT("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT,
    TEXT("HKEY_CURRENT_USER"), HKEY_CURRENT_USER,
    TEXT("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE,
    TEXT("HKEY_USERS"), HKEY_USERS,
    TEXT("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG,
    TEXT("HKEY_DYN_DATA"), HKEY_DYN_DATA
};

const TCHAR s_RegistryHeader[] = TEXT("REGEDIT");

const TCHAR s_OldWin31RegFileRoot[] = TEXT(".classes");

const TCHAR s_Win40RegFileHeader[] = TEXT("REGEDIT4\n\n");

#ifdef UNICODE
//
// New header is required for version 5.0 because the version detection code
// in Win 4.0 regedit was brain damaged (See comments in ImportRegFile for
// details)
//
const WORD s_UnicodeByteOrderMark = 0xFEFF;
const TCHAR s_WinNT50RegFileHeader[] = TEXT("Windows Registry Editor Version");
const TCHAR s_WinNT50RegFileVersion[] = TEXT("5.00");
#endif

const TCHAR s_HexPrefix[] = TEXT("hex");
const TCHAR s_DwordPrefix[] = TEXT("dword:");
const TCHAR g_HexConversion[16] = {TEXT('0'), TEXT('1'), TEXT('2'), TEXT('3'), TEXT('4'),
                                   TEXT('5'), TEXT('6'), TEXT('7'), TEXT('8'), TEXT('9'),
                                   TEXT('a'), TEXT('b'), TEXT('c'), TEXT('d'), TEXT('e'), TEXT('f')};
const TCHAR s_FileLineBreak[] = TEXT(",\\\n  ");

//BUGBUG - we upped the size of this buffer from 512 to 64K to reduce the chance of hitting the bug
//where a DBCS character is split across two buffers.  The true fix was too risky at the time.
//Changed for NT5 RC2
#define SIZE_FILE_IO_BUFFER             0x10000 //64K

typedef struct _FILE_IO {
#ifdef UNICODE
    //
    // Space for unicode/ansi conversions, assumes worst case
    // where every unicode char is a double-byte char
    //
    CHAR ConversionBuffer[SIZE_FILE_IO_BUFFER*2];
#endif
    TCHAR Buffer[SIZE_FILE_IO_BUFFER];
    FILE_HANDLE hFile;
    int BufferOffset;
    int CurrentColumn;
    int CharsAvailable;
    DWORD FileSizeDiv100;
    DWORD FileOffset;
    UINT LastPercentage;
#ifdef DEBUG
    BOOL fValidateUngetChar;
#endif
}   FILE_IO;

FILE_IO s_FileIo;

UINT g_FileErrorStringID;

UINT g_ImportFileVersion;

BOOL s_fTreatFileAsUnicode = TRUE;

VOID
NEAR PASCAL
ImportWin31RegFile(
    VOID
    );

VOID
NEAR PASCAL
ImportNewerRegFile(
    VOID
    );

VOID
NEAR PASCAL
ParseHeader(
    LPHKEY lphKey
    );

VOID
NEAR PASCAL
ParseValue(
    HKEY hKey,
    LPCTSTR lpszValueName
    );

VOID
NEAR PASCAL
ParseValuename(
    HKEY hKey
    );

BOOL
NEAR PASCAL
ParseString(
    LPTSTR lpString,
    LPDWORD cbStringData
    );

BOOL
NEAR PASCAL
ParseHexSequence(
    LPBYTE lpHexData,
    LPDWORD lpcbHexData
    );

BOOL
NEAR PASCAL
ParseHexDword(
    LPDWORD lpDword
    );

BOOL
NEAR PASCAL
ParseHexByte(
    LPBYTE lpByte
    );

BOOL
NEAR PASCAL
ParseHexDigit(
    LPBYTE lpDigit
    );

BOOL
NEAR PASCAL
ParseEndOfLine(
    VOID
    );

VOID
NEAR PASCAL
SkipWhitespace(
    VOID
    );

VOID
NEAR PASCAL
SkipPastEndOfLine(
    VOID
    );

BOOL
NEAR PASCAL
GetChar(
    PTCHAR lpChar
    );

VOID
NEAR PASCAL
UngetChar(
    VOID
    );

BOOL
NEAR PASCAL
MatchChar(
    TCHAR CharToMatch
    );

BOOL
NEAR PASCAL
IsWhitespace(
    TCHAR Char
    );

BOOL
NEAR PASCAL
IsNewLine(
    TCHAR Char
    );

VOID
NEAR PASCAL
PutBranch(
    HKEY hKey,
    LPTSTR lpKeyName
    );

VOID
NEAR PASCAL
PutLiteral(
    LPCTSTR lpString
    );

VOID
NEAR PASCAL
PutString(
    LPCTSTR lpString
    );

VOID
NEAR PASCAL
PutBinary(
    CONST BYTE FAR* lpBuffer,
    DWORD Type,
    DWORD cbBytes
    );

VOID
NEAR PASCAL
PutDword(
    DWORD Dword,
    BOOL fLeadingZeroes
    );

VOID
NEAR PASCAL
PutChar(
    TCHAR Char
    );

VOID
NEAR PASCAL
FlushIoBuffer(
    VOID
    );

/*******************************************************************************
*
*  EditRegistryKey
*
*  DESCRIPTION:
*     Parses the pFullKeyName string and creates a handle to the registry key.
*
*  PARAMETERS:
*     lphKey, location to store handle to registry key.
*     lpFullKeyName, string of form "HKEY_LOCAL_MACHINE\Subkey1\Subkey2".
*     fCreate, TRUE if key should be created, else FALSE for open only.
*     (returns), ERROR_SUCCESS, no errors occurred, phKey is valid,
*                ERROR_CANTOPEN, registry access error of some form,
*                ERROR_BADKEY, incorrectly formed pFullKeyName.
*
*******************************************************************************/

DWORD
PASCAL
EditRegistryKey(
    LPHKEY lphKey,
    LPTSTR lpFullKeyName,
    UINT uOperation
    )
{

    LPTSTR lpSubKeyName;
    TCHAR PrevChar;
    HKEY hRootKey;
    UINT Counter;
    DWORD Result;

    if ((lpSubKeyName = (LPTSTR) STRCHR(lpFullKeyName, TEXT('\\'))) != NULL) {

        PrevChar = *lpSubKeyName;
        *lpSubKeyName++ = 0;

    }

    CHARUPPERSTRING(lpFullKeyName);

    hRootKey = NULL;

    for (Counter = 0; Counter < NUMBER_REGISTRY_ROOTS; Counter++) {

        if (STRCMP(g_RegistryRoots[Counter].lpKeyName, lpFullKeyName) == 0) {

            hRootKey = g_RegistryRoots[Counter].hKey;
            break;

        }

    }

    if (hRootKey) {

        Result = ERROR_CANTOPEN;

        switch (uOperation) {
        case ERK_CREATE:
            //
            // If trying to open one of these keys just return OK
            // When lpSubKeyName is NULL, you recreate the parent key
            // Since these keys are usually in use this will fail
            // This code path only occurs when restoring a whole root key
            // from a .reg file.
            //
            if (((hRootKey == HKEY_LOCAL_MACHINE) || (hRootKey == HKEY_USERS)) 
                && lpSubKeyName == NULL) {
                Result = ERROR_SUCCESS;                
            }
            else if (RegCreateKey(hRootKey, lpSubKeyName, lphKey) == ERROR_SUCCESS)
                Result = ERROR_SUCCESS;
            break;
        case ERK_OPEN:
            //
            // Used when exporting.
            //
            if(RegOpenKeyEx(hRootKey,lpSubKeyName,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,lphKey) == ERROR_SUCCESS)
                Result = ERROR_SUCCESS;
            break;
        case ERK_DELETE:
            RegDeleteKeyRecursive(hRootKey, lpSubKeyName);
            // asssume success... don't care if this fails
            Result = ERROR_SUCCESS;
            *lphKey = NULL;
            break;
        }

    }

    else
        Result = ERROR_BADKEY;

    if (lpSubKeyName != NULL) {

        lpSubKeyName--;
        *lpSubKeyName = PrevChar;

    }

    return Result;

}

/*******************************************************************************
*
*  ImportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     lpFileName, address of name of file to be imported.
*
*******************************************************************************/

VOID
PASCAL
ImportRegFile(
    LPTSTR lpFileName
    )
{

    TCHAR Char;
    LPCTSTR lpHeader;
    BOOL fNewRegistryFile;
#ifdef UNICODE
    UINT Temp, i;
    TCHAR StrToIntBuf[2];
    LPCTSTR lp50Header;
#endif // UNICODE
    DWORD cch;
    TCHAR tchBuffer[MAX_PATH] = {0};
    LPTSTR lpFilePart;

    g_FileErrorStringID = IDS_IMPFILEERRSUCCESS;

    // OPENREADFILE used to be OpenFile(), but there isn't any Unicode version
    // of that API, so now it's CreateFile().  But OpenFile searched the path
    // automatically, whereas CreateFile does not.  Corel's 'Perfect Office v6'
    // install app depends on the path being searched, so do it manually.

    cch = SearchPath(NULL,                // pointer to search path
                     lpFileName,          // pointer to filename
                     NULL,                // pointer to extension
                     ARRAYSIZE(tchBuffer), // size, in characters, of buffer
                     (TCHAR*)tchBuffer,   // pointer to buffer for found filename
                     &lpFilePart);        // pointer to pointer to file component);

    if ((cch != 0) && (cch <= MAX_PATH) && OPENREADFILE((TCHAR*)tchBuffer, s_FileIo.hFile)) {

        WORD wBOM;
        DWORD NumberOfBytesRead;

        s_FileIo.FileSizeDiv100 = GETFILESIZE(s_FileIo.hFile) / 100;
        s_FileIo.FileOffset = 0;
        s_FileIo.CharsAvailable = 0;
        s_FileIo.LastPercentage = 0;

        // 
        // Read the first two bytes. If it's the Unicode byte order mark,
        // set a flag so all the rest of the file will be interpreted
        // as ANSI or Unicode text properly.
        //
        if (!READFILE(s_FileIo.hFile, &wBOM,
            sizeof(wBOM), &NumberOfBytesRead)) {

            g_FileErrorStringID = IDS_IMPFILEERRFILEREAD;
            goto exit_gracefully;
        }
        
        if (wBOM == s_UnicodeByteOrderMark)
            s_fTreatFileAsUnicode = TRUE;
        else {
            s_fTreatFileAsUnicode = FALSE;
            // We probably just read "RE" from "REGEDIT4".  Back up the file 
            // position so the ANSI import routines get what they expect
            SetFilePointer(s_FileIo.hFile, -2, NULL, FILE_CURRENT);
        }

        //
        //  The following will force GetChar to read in the first block of data.
        //

        s_FileIo.BufferOffset = 0;

        SkipWhitespace();

        lpHeader = s_RegistryHeader;
        g_ImportFileVersion = 0;

# if 0
    Sit back, and I will tell ye a tale of woe.

    Win95 and NT 4 shipped with regedit compiled ANSI.  There are a couple
    of registry types on NT (namely REG_EXPAND_SZ and REG_MULTI_SZ) that
    weren't on Win95, and which regedit doesn't really understand.  regedit
    treats these registry types as hex binary streams.

    You can probably see where this is going.

    If you exported, say your user TEMP environment variable on NT 4
    using regedit, you'd get something that looked like this:

REGEDIT4

[HKEY_CURRENT_USER\Environment]
"TEMP"=hex(2):25,53,59,53,54,45,4d,44,52,49,56,45,25,5c,53,48,54,65,6d,70,00

    ...a nice, null-terminated ANSI string.  Nice, that is, until we decided
    to compile regedit UNICODE for NT 5.  A unicode regedit exports your
    user TEMP variable like this:

REGEDIT4

[HKEY_CURRENT_USER\Environment]
"TEMP"=hex(2):25,00,53,00,59,00,53,00,54,00,45,00,4d,00,44,00,52,00,49,00,56,\
  00,45,00,25,00,5c,00,53,00,48,00,54,00,65,00,6d,00,70,00,00,00

    ...mmmm.  Unicode.  Of course, a unicode regedit also expects anything
    it imports to have all those interspersed zeroes, too.  Otherwise,
    it dumps garbage into your registry.  All it takes is a -DUNICODE, and
    regedit is suddenly incompatible with the thousdands of existing .reg
    files out there.

    So just bump the version in the header to REGEDIT5 and be done with
    it, right?  Wrong.  The regedit on Win95 and NT 4 looks at the first
    character after the string "REGEDIT" and compares it to the digit "4".
    If that character is anything other than the digit "4", the parser
    assumes it is looking at a Windows 3.1 file.  Yep.  There will only
    ever be two formats, right?  Just Win95 and Win3.1.  That's all the
    world needs.

    So a completely new .reg file header had to be invented, so that the
    older, brain damaged regedits of the world would simply regect the new,
    unicodized .reg files outright.  An NT 5 .reg file, exporting your user
    TEMP variable, looks like this:

Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Environment]
"TEMP"=hex(2):25,00,53,00,59,00,53,00,54,00,45,00,4d,00,44,00,52,00,49,00,56,\
  00,45,00,25,00,5c,00,53,00,48,00,54,00,65,00,6d,00,70,00,00,00

    The parser is still brain-dead, but it does bother to convert that 5.00
    into a version number, so that future generations can bump it to 5.50 or
    6.00, and the regedit 5.00 that shipped with NT 5.00 will properly reject
    the files.
#endif // 0

#ifdef UNICODE
        //
        // Compare to the new .reg file header
        //
        lp50Header = s_WinNT50RegFileHeader;
        while (*lp50Header != 0) {

            if (MatchChar(*lp50Header))
                lp50Header = CharNext(lp50Header);

            else
                break;

        }

        //
        // If the above loop pushed lp50Header to its terminating null
        // character, then the header matches.
        //
        if (0 == *lp50Header) {
            
            SkipWhitespace();
            //
            // Now, decode the version number into a hex, _WIN32_WINNT
            // style version number.
            //
            StrToIntBuf[1] = 0;

            //
            // Any number of digits can come before the decimal point
            //
            while (!MatchChar(TEXT('.'))) {
                if (!GetChar(StrToIntBuf) || !IsCharAlphaNumeric(*StrToIntBuf)) {
                    g_FileErrorStringID = IDS_IMPFILEERRFORMATBAD; 
                    goto exit_gracefully;
                }
    
                Temp = StrToInt(StrToIntBuf);
                // Hex version number, so move left four bits.
                g_ImportFileVersion <<= 4;
                g_ImportFileVersion += Temp;
            }

            //
            // Fixed at two digits after the decimal point
            //
            for (i = 0; i < 2; i++) {
                if (!GetChar(StrToIntBuf) || !IsCharAlphaNumeric(*StrToIntBuf)) {
                    g_FileErrorStringID = IDS_IMPFILEERRFORMATBAD;
                    goto exit_gracefully;
                }
    
                Temp = StrToInt(StrToIntBuf);
                // Hex version number, so move left four bits.
                g_ImportFileVersion <<= 4;
                g_ImportFileVersion += Temp;
            }

            //
            // For NT 5, reject any version number that isn't
            // 5.  This can be expanded into a switch statement
            // when the version number is bumped later.
            //
            if (0x0500 != g_ImportFileVersion) {
                g_FileErrorStringID = IDS_IMPFILEERRVERBAD;
                goto exit_gracefully;
            }
            else {
                SkipWhitespace();
                ImportNewerRegFile();
            }

        } // if (0 == *lp50Header)
        //
        // It doesn't use the new .reg file header, so
        // it's not an NT 5.0+ registry file, so use the brain dead
        // older algorithm to see if it's a valid older registry file
        //
        else {
#endif // UNICODE

            while (*lpHeader != 0) {
    
                if (MatchChar(*lpHeader))
                    lpHeader = CharNext(lpHeader);
    
                else
                    break;
    
            }
    
            if (*lpHeader == 0) {
    
                //
                // Win95's and NT 4's regedit shipped with this line
                // of code.  It is the cause of all of the suffering above.
                // Notice the incorrect assumption:  "If the very next
                // character isn't a '4', then we must be reading
                // a Windows 3.1 registry file!"  Of course there won't
                // be a version 5 of regedit.  Version 4 was perfect!
                //
                fNewRegistryFile = MatchChar(TEXT('4'));
    
                SkipWhitespace();
    
                if (GetChar(&Char) && IsNewLine(Char)) {
    
                    if (fNewRegistryFile) {
                        g_ImportFileVersion = 0x0400;
                        ImportNewerRegFile();
                    }
                    else {
                        g_ImportFileVersion = 0x0310;
                        ImportWin31RegFile();
                    }
    
                }
    
            } // if (*lpHeader == 0)
    
            else
                g_FileErrorStringID = IDS_IMPFILEERRFORMATBAD;
#ifdef UNICODE
        }
#endif // UNICODE

    } // if (OPENREADFILE...

    else {
        { TCHAR buff[250];
          wsprintf(buff, L"REGEDIT:  CreateFile failed, GetLastError() = %d\n", GetLastError());
        OutputDebugString(buff);
        }
        s_FileIo.hFile = NULL;
        g_FileErrorStringID = IDS_IMPFILEERRFILEOPEN;
    }

#ifdef UNICODE // Urefd labels generate warnings
exit_gracefully:
#endif // UNICODE
    if (s_FileIo.hFile) {
        CLOSEFILE(s_FileIo.hFile);
    }

}

/*******************************************************************************
*
*  ImportWin31RegFile
*
*  DESCRIPTION:
*     Imports the contents of a Windows 3.1 style registry file into the
*     registry.
*
*     We scan over the file looking for lines of the following type:
*        HKEY_CLASSES_ROOT\keyname = value_data
*        HKEY_CLASSES_ROOT\keyname =value_data
*        HKEY_CLASSES_ROOT\keyname value_data
*        HKEY_CLASSES_ROOT\keyname                          (null value data)
*
*     In all cases, any number of spaces may follow 'keyname'.  Although we
*     only document the first syntax, the Windows 3.1 Regedit handled all of
*     these formats as valid, so this version will as well (fortunately, it
*     doesn't make the parsing any more complex!).
*
*     Note, we also support replacing HKEY_CLASSES_ROOT with \.classes above
*     which must come from some early releases of Windows.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
ImportWin31RegFile(
    VOID
    )
{

    HKEY hKey;
    TCHAR Char;
    BOOL fSuccess;
    LPCTSTR lpClassesRoot;
    TCHAR KeyName[MAXKEYNAME];
    UINT Index;

    //
    //  Keep an open handle to the classes root.  We may prevent some
    //  unneccessary flushing.
    //
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT,NULL,0,KEY_SET_VALUE,&hKey) != ERROR_SUCCESS) {

        g_FileErrorStringID = IDS_IMPFILEERRREGOPEN;
        return;

    }

    while (TRUE) {

        //
        //  Check for the end of file condition.
        //

        if (!GetChar(&Char))
            break;

        UngetChar();                    //  Not efficient, but works for now.

        //
        //  Match the beginning of the line against one of the two aliases for
        //  HKEY_CLASSES_ROOT.
        //

        if (MatchChar(TEXT('\\')))
            lpClassesRoot = s_OldWin31RegFileRoot;

        else
            lpClassesRoot = g_RegistryRoots[INDEX_HKEY_CLASSES_ROOT].lpKeyName;

        fSuccess = TRUE;

        while (*lpClassesRoot != 0) {

            if (!MatchChar(*lpClassesRoot++)) {

                fSuccess = FALSE;
                break;

            }

        }

        //
        //  Make sure that we have a backslash seperating one of the aliases
        //  from the keyname.
        //

        if (fSuccess)
            fSuccess = MatchChar(TEXT('\\'));

        if (fSuccess) {

            //
            //  We've found one of the valid aliases, so read in the keyname.
            //

            //  fSuccess = TRUE;        //  Must be TRUE if we're in this block
            Index = 0;

            while (GetChar(&Char)) {

                if (Char == TEXT(' ') || IsNewLine(Char))
                    break;

                //
                //  Make sure that the keyname buffer doesn't overflow.  We must
                //  leave room for a terminating null.
                //

                if (Index >= (sizeof(KeyName)/sizeof(TCHAR)) - 1) {

                    fSuccess = FALSE;
                    break;

                }

                KeyName[Index++] = Char;

            }

            if (fSuccess) {

                KeyName[Index] = 0;

                //
                //  Now see if we have a value to assign to this keyname.
                //

                SkipWhitespace();

                if (MatchChar(TEXT('=')))
                    MatchChar(TEXT(' '));

                //  fSuccess = TRUE;    //  Must be TRUE if we're in this block
                Index = 0;

                while (GetChar(&Char)) {

                    if (IsNewLine(Char))
                        break;

                    //
                    //  Make sure that the value data buffer doesn't overflow.
                    //  Because this is always string data, we must leave room
                    //  for a terminating null.
                    //

                    if (Index >= MAXDATA_LENGTH - 1) {

                        fSuccess = FALSE;
                        break;

                    }

                    ((PTSTR)g_ValueDataBuffer)[Index++] = Char;

                }

                if (fSuccess) {

                    ((PTSTR)g_ValueDataBuffer)[Index] = 0;

                    if (RegSetValue(hKey, KeyName, REG_SZ, (LPCTSTR)g_ValueDataBuffer,
                        Index*sizeof(TCHAR)) != ERROR_SUCCESS)
                        g_FileErrorStringID = IDS_IMPFILEERRREGSET;

                }

            }

        }

        //
        //  Somewhere along the line, we had a parsing error, so resynchronize
        //  on the next line.
        //

        if (!fSuccess)
            SkipPastEndOfLine();

    }

    RegFlushKey(hKey);
    RegCloseKey(hKey);

}

/*******************************************************************************
*
*  ImportNewerRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
ImportNewerRegFile(
    VOID
    )
{

    HKEY hLocalMachineKey;
    HKEY hUsersKey;
    HKEY hKey;
    TCHAR Char;

#ifdef WINNT
    hLocalMachineKey = NULL;
    hUsersKey = NULL;
#else
    //
    //  Keep open handles for the predefined roots to prevent the registry
    //  library from flushing after every single RegOpenKey/RegCloseKey
    //  operation.
    //

    RegOpenKey(HKEY_LOCAL_MACHINE, NULL, &hLocalMachineKey);
    RegOpenKey(HKEY_USERS, NULL, &hUsersKey);

#ifdef DEBUG
    if (hLocalMachineKey == NULL)
        DbgPrintf(("Unable to open HKEY_LOCAL_MACHINE\n\r"));
    if (hUsersKey == NULL)
        DbgPrintf(("Unable to open HKEY_USERS\n\r"));
#endif
#endif

    hKey = NULL;

    while (TRUE) {

        SkipWhitespace();

        //
        //  Check for the end of file condition.
        //

        if (!GetChar(&Char))
            break;

        switch (Char) {

            case TEXT('['):
                //
                //  If a registry key is currently open, we must close it first.
                //  If ParseHeader happens to fail (for example, no closing
                //  bracket), then hKey will be NULL and any values that we
                //  parse must be ignored.
                //

                if (hKey != NULL) {

                    RegCloseKey(hKey);
                    hKey = NULL;

                }

                ParseHeader(&hKey);

                break;

            case TEXT('"'):
                //
                //  As noted above, if we don't have an open registry key, then
                //  just skip the line.
                //

                if (hKey != NULL)
                    ParseValuename(hKey);

                else
                    SkipPastEndOfLine();

                break;

            case TEXT('@'):
                //
                //
                //

                if (hKey != NULL)
                    ParseValue(hKey, NULL);

                else
                    SkipPastEndOfLine();

                break;

            case TEXT(';'):
                //
                //  This line is a comment so just dump the rest of it.
                //

                SkipPastEndOfLine();

                break;

            default:
                if (IsNewLine(Char))
                    break;

                SkipPastEndOfLine();

                break;

        }

    }

    if (hKey != NULL)
        RegCloseKey(hKey);

    if (hUsersKey != NULL)
        RegCloseKey(hUsersKey);

    if (hLocalMachineKey != NULL)
        RegCloseKey(hLocalMachineKey);

}

/*******************************************************************************
*
*  ParseHeader
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

// bugbug - each subkeyname can be MAXKEYNAME in size
// ideally - we should handle unlimited size names
// let's at least handle bigger names for now
// at least a depth of 10 with maximum length subkey names

#define SIZE_FULL_KEYNAME ((MAXKEYNAME + 40)*10)

VOID
NEAR PASCAL
ParseHeader(
    LPHKEY lphKey
    )
{

    TCHAR FullKeyName[SIZE_FULL_KEYNAME];
    int CurrentIndex;
    int LastRightBracketIndex;
    TCHAR Char;
    UINT uOperation = ERK_CREATE;

    CurrentIndex = 0;
    LastRightBracketIndex = -1;

    if (!GetChar(&Char))
        return;

    if (Char == TEXT('-')) {
        if (!GetChar(&Char))
            return;
        uOperation = ERK_DELETE;
    }

    do {

        if (IsNewLine(Char))
            break;

        if (Char == TEXT(']'))
            LastRightBracketIndex = CurrentIndex;

        FullKeyName[CurrentIndex++] = Char;

        if (CurrentIndex == SIZE_FULL_KEYNAME) {

            do {

                if (Char == TEXT(']'))
                    LastRightBracketIndex = -1;

                if (IsNewLine(Char))
                    break;

            }   while (GetChar(&Char));

            break;

        }

    } while (GetChar(&Char));

    if (LastRightBracketIndex != -1) {

        FullKeyName[LastRightBracketIndex] = 0;

        switch (EditRegistryKey(lphKey, FullKeyName, uOperation)) {

            case ERROR_CANTOPEN:
                g_FileErrorStringID = IDS_IMPFILEERRREGOPEN;
                break;

        }

    }

}

/*******************************************************************************
*
*  ParseValuename
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
ParseValuename(
    HKEY hKey
    )
{

    TCHAR ValueName[MAXVALUENAME_LENGTH];
    DWORD cbData;
    cbData = sizeof(ValueName);

    if (!ParseString(ValueName, &cbData))
        goto ParseError;

    ParseValue(hKey, ValueName);
    return;

ParseError:
    SkipPastEndOfLine();
    return;
}

VOID
NEAR PASCAL
ParseValue(
    HKEY hKey,
    LPCTSTR lpszValueName
    )
{

    DWORD Type;
    DWORD cbData;
    LPCTSTR lpPrefix;

    SkipWhitespace();

    if (!MatchChar(TEXT('=')))
        goto ParseError;

    SkipWhitespace();

    //
    //  REG_SZ.
    //
    //  "ValueName" = "string of text"
    //

    if (MatchChar(TEXT('"'))) {

        //  BUGBUG:  Line continuations for strings?

        cbData = MAXDATA_LENGTH;

        if (!ParseString((PTSTR)g_ValueDataBuffer, &cbData) || !ParseEndOfLine())
            goto ParseError;

        Type = REG_SZ;

    }

    //
    //  REG_DWORD.
    //
    //  "ValueName" = dword: 12345678
    //

    else if (MatchChar(s_DwordPrefix[0])) {

        lpPrefix = &s_DwordPrefix[1];

        while (*lpPrefix != 0)
            if (!MatchChar(*lpPrefix++))
                goto ParseError;

        SkipWhitespace();

        if (!ParseHexDword((LPDWORD) g_ValueDataBuffer) || !ParseEndOfLine())
            goto ParseError;

        Type = REG_DWORD;
        cbData = sizeof(DWORD);

    }
    else if (MatchChar('-')) {

        if (!ParseEndOfLine())
            goto ParseError;
        RegDeleteValue(hKey, lpszValueName);

        return;
    }

    //
    //  REG_BINARY and other.
    //
    //  "ValueName" = hex: 00 , 11 , 22
    //  "ValueName" = hex(12345678): 00, 11, 22
    //

    else {

        lpPrefix = s_HexPrefix;

        while (*lpPrefix != 0)
            if (!MatchChar(*lpPrefix++))
                goto ParseError;

        //
        //  Check if this is a type of registry data that we don't directly
        //  support.  If so, then it's just a dump of hex data of the specified
        //  type.
        //

        if (MatchChar(TEXT('('))) {

            if (!ParseHexDword(&Type) || !MatchChar(TEXT(')')))
                goto ParseError;

        }

        else
            Type = REG_BINARY;

        if (!MatchChar(TEXT(':')) || !ParseHexSequence(g_ValueDataBuffer, &cbData) ||
            !ParseEndOfLine())
            goto ParseError;

    }

#ifdef UNICODE
    //
    // If we're compiled UNICODE and we're reading an older, ANSI .reg
    // file, we have to write all of the data to the registry using
    // RegSetValueExA, because it was read from the registry using
    // RegQueryValueExA.
    //
    if ((g_ImportFileVersion < 0x0500) && ((REG_EXPAND_SZ == Type) || (REG_MULTI_SZ == Type))) {
        CHAR AnsiValueName[MAXVALUENAME_LENGTH];
        AnsiValueName[0] = 0;

        //
        // It's much easier to convert the value name to ANSI
        // and call RegSetValueExA than to try to convert
        // a REG_MULTI_SZ to Unicode before calling RegSetValueExW.
        // We don't lose anything because this is coming from a
        // downlevel .reg file that could only contain ANSI characters
        // to begin with.
        //
        WideCharToMultiByte(
            CP_THREAD_ACP,
            0,
            lpszValueName,
            -1,
            AnsiValueName,
            MAXVALUENAME_LENGTH,
            NULL,
            NULL
        );

        if (RegSetValueExA( 
                hKey, 
                AnsiValueName, 
                0, 
                Type, 
                g_ValueDataBuffer, 
                cbData) 
            != ERROR_SUCCESS)
            g_FileErrorStringID = IDS_IMPFILEERRREGSET;
    }
    else {
#endif // UNICODE
        if (RegSetValueEx(hKey, lpszValueName, 0, Type, g_ValueDataBuffer, cbData) !=
            ERROR_SUCCESS)
            g_FileErrorStringID = IDS_IMPFILEERRREGSET;
#ifdef UNICODE
    }
#endif // UNICODE

    return;

ParseError:
    SkipPastEndOfLine();

}

/*******************************************************************************
*
*  ParseString
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseString(
    LPTSTR lpString,
    LPDWORD lpcbStringData
    )
{

    TCHAR Char;
    DWORD cbMaxStringData;
    DWORD cbStringData;

    cbMaxStringData = *lpcbStringData;
    cbStringData = 1;                   //  Account for the null terminator

    while (GetChar(&Char)) {

        if (cbStringData >= cbMaxStringData)
            return FALSE;

        switch (Char) {

            case TEXT('\\'):
                if (!GetChar(&Char))
                    return FALSE;

                switch (Char) {

                    case TEXT('\\'):
                        *lpString++ = TEXT('\\');
                        break;

                    case TEXT('"'):
                        *lpString++ = TEXT('"');
                        break;

                    default:
                        DbgPrintf(("ParseString:  Invalid escape sequence"));
                        return FALSE;

                }
                break;

            case TEXT('"'):
                *lpString = 0;
                *lpcbStringData = cbStringData * sizeof(TCHAR);
                return TRUE;

            default:
                if (IsNewLine(Char))
                    return FALSE;

                *lpString++ = Char;
                break;

        }

        cbStringData++;

    }

    return FALSE;

}

/*******************************************************************************
*
*  ParseHexSequence
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexSequence(
    LPBYTE lpHexData,
    LPDWORD lpcbHexData
    )
{

    DWORD cbHexData;

    cbHexData = 0;

    do {

        if (cbHexData >= MAXDATA_LENGTH)
            return FALSE;

        SkipWhitespace();

        if (MatchChar(TEXT('\\')) && !ParseEndOfLine())
            return FALSE;

        SkipWhitespace();

        if (!ParseHexByte(lpHexData++))
            break;

        cbHexData++;

        SkipWhitespace();

    }   while (MatchChar(TEXT(',')));

    *lpcbHexData = cbHexData;

    return TRUE;

}

/*******************************************************************************
*
*  ParseHexDword
*
*  DESCRIPTION:
*     Parses a one dword hexadecimal string from the registry file stream and
*     converts it to a binary number.  A maximum of eight hex digits will be
*     parsed from the stream.
*
*  PARAMETERS:
*     lpByte, location to store binary number.
*     (returns), TRUE if a hexadecimal dword was parsed, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexDword(
    LPDWORD lpDword
    )
{

    UINT CountDigits;
    DWORD Dword;
    BYTE Byte;

    Dword = 0;
    CountDigits = 0;

    while (TRUE) {

        if (!ParseHexDigit(&Byte))
            break;

        Dword = (Dword << 4) + (DWORD) Byte;

        if (++CountDigits == 8)
            break;

    }

    *lpDword = Dword;

    return CountDigits != 0;

}

/*******************************************************************************
*
*  ParseHexByte
*
*  DESCRIPTION:
*     Parses a one byte hexadecimal string from the registry file stream and
*     converts it to a binary number.
*
*  PARAMETERS:
*     lpByte, location to store binary number.
*     (returns), TRUE if a hexadecimal byte was parsed, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexByte(
    LPBYTE lpByte
    )
{

    BYTE SecondDigit;

    if (ParseHexDigit(lpByte)) {

        if (ParseHexDigit(&SecondDigit))
            *lpByte = (BYTE) ((*lpByte << 4) | SecondDigit);

        return TRUE;

    }

    else
        return FALSE;

}

/*******************************************************************************
*
*  ParseHexDigit
*
*  DESCRIPTION:
*     Parses a hexadecimal character from the registry file stream and converts
*     it to a binary number.
*
*  PARAMETERS:
*     lpDigit, location to store binary number.
*     (returns), TRUE if a hexadecimal digit was parsed, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseHexDigit(
    LPBYTE lpDigit
    )
{

    TCHAR Char;
    BYTE Digit;

    if (GetChar(&Char)) {

        if (Char >= TEXT('0') && Char <= TEXT('9'))
            Digit = (BYTE) (Char - TEXT('0'));

        else if (Char >= TEXT('a') && Char <= TEXT('f'))
            Digit = (BYTE) (Char - TEXT('a') + 10);

        else if (Char >= TEXT('A') && Char <= TEXT('F'))
            Digit = (BYTE) (Char - TEXT('A') + 10);

        else {

            UngetChar();

            return FALSE;

        }

        *lpDigit = Digit;

        return TRUE;

    }

    return FALSE;

}

/*******************************************************************************
*
*  ParseEndOfLine
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
ParseEndOfLine(
    VOID
    )
{

    TCHAR Char;
    BOOL fComment;
    BOOL fFoundOneEndOfLine;
    BOOL fEOF;

    fComment = FALSE;
    fFoundOneEndOfLine = FALSE;
    fEOF = TRUE;

    while (GetChar(&Char)) {

        if (IsWhitespace(Char))
            continue;

        if (IsNewLine(Char)) {

            fComment = FALSE;
            fFoundOneEndOfLine = TRUE;

        }

        //
        //  Like .INIs and .INFs, comments begin with a semicolon character.
        //

        else if (Char == TEXT(';'))
            fComment = TRUE;

        else if (!fComment) {

            UngetChar();
            fEOF = FALSE;
            break;

        }

    }

    return fFoundOneEndOfLine || fEOF;

}

/*******************************************************************************
*
*  SkipWhitespace
*
*  DESCRIPTION:
*     Advances the registry file pointer to the first character past any
*     detected whitespace.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
SkipWhitespace(
    VOID
    )
{

    TCHAR Char;

    while (GetChar(&Char)) {

        if (!IsWhitespace(Char)) {

            UngetChar();
            break;

        }

    }

}

/*******************************************************************************
*
*  SkipPastEndOfLine
*
*  DESCRIPTION:
*     Advances the registry file pointer to the first character past the first
*     detected new line character.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
SkipPastEndOfLine(
    VOID
    )
{

    TCHAR Char;

    while (GetChar(&Char)) {

        if (IsNewLine(Char))
            break;

    }

    while (GetChar(&Char)) {

        if (!IsNewLine(Char)) {

            UngetChar();
            break;

        }

    }

}

/*******************************************************************************
*
*  GetChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
GetChar(
    PTCHAR lpChar
    )
{

    FILE_NUMBYTES NumberOfBytesRead;
    UINT NewPercentage;

    // If we're at the end of the buffer, read some more.
    // Initially BufferOffset and CharsAvailable will be 0
    if (s_FileIo.BufferOffset == s_FileIo.CharsAvailable) {

        if (TRUE == s_fTreatFileAsUnicode) {
            if (!READFILE(s_FileIo.hFile, s_FileIo.Buffer,
                SIZE_FILE_IO_BUFFER, &NumberOfBytesRead)) {

                g_FileErrorStringID = IDS_IMPFILEERRFILEREAD;
                return FALSE;
            }

            s_FileIo.CharsAvailable = ((int) NumberOfBytesRead / 2);
        }
        else {
            if (!READFILE(s_FileIo.hFile, s_FileIo.ConversionBuffer,
                SIZE_FILE_IO_BUFFER, &NumberOfBytesRead)) {

                g_FileErrorStringID = IDS_IMPFILEERRFILEREAD;
                return FALSE;
            }

            {
                int i;

                i = MultiByteToWideChar(
                        CP_THREAD_ACP,
                        MB_PRECOMPOSED,
                        s_FileIo.ConversionBuffer,
                        NumberOfBytesRead,
                        s_FileIo.Buffer,
                        SIZE_FILE_IO_BUFFER
                        );

                s_FileIo.CharsAvailable = i;
            }
        }

        s_FileIo.BufferOffset = 0;
        s_FileIo.FileOffset += NumberOfBytesRead;

        if (s_FileIo.FileSizeDiv100 != 0) {

            NewPercentage = ((UINT) (s_FileIo.FileOffset /
                s_FileIo.FileSizeDiv100));

            if (NewPercentage > 100)
                NewPercentage = 100;

        }

        else
            NewPercentage = 100;

        if (s_FileIo.LastPercentage != NewPercentage) {

            s_FileIo.LastPercentage = NewPercentage;
            ImportRegFileUICallback(NewPercentage);

        }

    }

    if (s_FileIo.BufferOffset >= s_FileIo.CharsAvailable)
        return FALSE;

    *lpChar = s_FileIo.Buffer[s_FileIo.BufferOffset++];

    return TRUE;

}

/*******************************************************************************
*
*  UngetChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
NEAR PASCAL
UngetChar(
    VOID
    )
{

#ifdef DEBUG
    if (s_FileIo.fValidateUngetChar)
        DbgPrintf(("REGEDIT ERROR: Too many UngetChar's called!\n\r"));
#endif

    s_FileIo.BufferOffset--;

}

/*******************************************************************************
*
*  MatchChar
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
NEAR PASCAL
MatchChar(
    TCHAR CharToMatch
    )
{

    BOOL fMatch;
    TCHAR NextChar;

    fMatch = FALSE;

    if (GetChar(&NextChar)) {

        if (CharToMatch == NextChar)
            fMatch = TRUE;

        else
            UngetChar();

    }

    return fMatch;

}

/*******************************************************************************
*
*  IsWhitespace
*
*  DESCRIPTION:
*     Checks if the given character is whitespace.
*
*  PARAMETERS:
*     Char, character to check.
*     (returns), TRUE if character is whitespace, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
IsWhitespace(
    TCHAR Char
    )
{

    return Char == TEXT(' ') || Char == TEXT('\t');

}

/*******************************************************************************
*
*  IsNewLine
*
*  DESCRIPTION:
*     Checks if the given character is a new line character.
*
*  PARAMETERS:
*     Char, character to check.
*     (returns), TRUE if character is a new line, else FALSE.
*
*******************************************************************************/

BOOL
NEAR PASCAL
IsNewLine(
    TCHAR Char
    )
{

    return Char == TEXT('\n') || Char == TEXT('\r');

}

/*******************************************************************************
*
*  ExportWinNT50RegFile
*
*  DESCRIPTION:
*      Exports an NT 5.0, unicode registry file.  Use this export function
*      for all future .reg file writing.
*
*  PARAMETERS:
*
*******************************************************************************/
VOID
PASCAL
ExportWinNT50RegFile(
    LPTSTR lpFileName,
    LPTSTR lpSelectedPath
    )
{

    HKEY hKey;
    TCHAR SelectedPath[SIZE_SELECTED_PATH];

    g_FileErrorStringID = IDS_EXPFILEERRSUCCESS;

    if (lpSelectedPath != NULL && EditRegistryKey(&hKey, lpSelectedPath,
        ERK_OPEN) != ERROR_SUCCESS) {

        g_FileErrorStringID = IDS_EXPFILEERRBADREGPATH;
        return;

    }

    if (OPENWRITEFILE(lpFileName, s_FileIo.hFile)) {

        DWORD dwNumberOfBytesWritten;

        s_FileIo.BufferOffset = 0;
        s_FileIo.CurrentColumn = 0;

        WRITEFILE(s_FileIo.hFile, &s_UnicodeByteOrderMark, sizeof(s_UnicodeByteOrderMark), &dwNumberOfBytesWritten);

        PutLiteral(s_WinNT50RegFileHeader);
        PutLiteral(TEXT(" "));
        PutLiteral(s_WinNT50RegFileVersion);
        PutLiteral(TEXT("\n\n"));

        if (lpSelectedPath != NULL) {

            STRCPY(SelectedPath, lpSelectedPath);
            PutBranch(hKey, SelectedPath);

        }

        else {

            STRCPY(SelectedPath,
                g_RegistryRoots[INDEX_HKEY_LOCAL_MACHINE].lpKeyName);
            PutBranch(HKEY_LOCAL_MACHINE, SelectedPath);

            STRCPY(SelectedPath,
                g_RegistryRoots[INDEX_HKEY_USERS].lpKeyName);
            PutBranch(HKEY_USERS, SelectedPath);

        }

        FlushIoBuffer();

        CLOSEFILE(s_FileIo.hFile);

    }

    else
        g_FileErrorStringID = IDS_EXPFILEERRFILEOPEN;

    if (lpSelectedPath != NULL)
        RegCloseKey(hKey);

}

/*******************************************************************************
*
*  ExportWin40RegFile
*
*  DESCRIPTION:
*      This function is only kept around to export old, ANSI, regedit 4 .reg
*      files.  Don't touch it except to fix bugs.  Meddling with this code
*      path will result in .reg files that can't be read by older verions
*      of regedit, which is the whole reason this code path is here.  Meddle
*      with ExportWinNT50RegFile if you want to break backwards compatibility.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
ExportWin40RegFile(
    LPTSTR lpFileName,
    LPTSTR lpSelectedPath
    )
{

    HKEY hKey;
    TCHAR SelectedPath[SIZE_SELECTED_PATH];

    g_FileErrorStringID = IDS_EXPFILEERRSUCCESS;

    if (lpSelectedPath != NULL && EditRegistryKey(&hKey, lpSelectedPath,
        ERK_OPEN) != ERROR_SUCCESS) {

        g_FileErrorStringID = IDS_EXPFILEERRBADREGPATH;
        return;

    }

    if (OPENWRITEFILE(lpFileName, s_FileIo.hFile)) {

        s_FileIo.BufferOffset = 0;
        s_FileIo.CurrentColumn = 0;

        PutLiteral(s_Win40RegFileHeader);

        if (lpSelectedPath != NULL) {

            STRCPY(SelectedPath, lpSelectedPath);
            PutBranch(hKey, SelectedPath);

        }

        else {

            STRCPY(SelectedPath,
                g_RegistryRoots[INDEX_HKEY_LOCAL_MACHINE].lpKeyName);
            PutBranch(HKEY_LOCAL_MACHINE, SelectedPath);

            STRCPY(SelectedPath,
                g_RegistryRoots[INDEX_HKEY_USERS].lpKeyName);
            PutBranch(HKEY_USERS, SelectedPath);

        }

        FlushIoBuffer();

        CLOSEFILE(s_FileIo.hFile);

    }

    else
        g_FileErrorStringID = IDS_EXPFILEERRFILEOPEN;

    if (lpSelectedPath != NULL)
        RegCloseKey(hKey);

}

/*******************************************************************************
*
*  PutBranch
*
*  DESCRIPTION:
*     Writes out all of the value names and their data and recursively calls
*     this routine for all of the key's subkeys to the registry file stream.
*
*  PARAMETERS:
*     hKey, registry key to write to file.
*     lpFullKeyName, string that gives the full path, including the root key
*        name, of the hKey.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutBranch(
    HKEY hKey,
    LPTSTR lpFullKeyName
    )
{

    LONG RegError;
    DWORD EnumIndex;
    DWORD cbValueName;
    DWORD cbValueData;
    DWORD Type;
    LPTSTR lpSubKeyName;
    HKEY hSubKey;
    int nLenFullKey;
    LPTSTR lpTempFullKeyName;

    //
    //  Write out the section header.
    //

    PutChar(TEXT('['));
    PutLiteral(lpFullKeyName);
    PutLiteral(TEXT("]\n"));

    //
    //  Write out all of the value names and their data.
    //

    EnumIndex = 0;

    while (TRUE) {

        cbValueName = sizeof(g_ValueNameBuffer);
        cbValueData = MAXDATA_LENGTH;

        if ((RegError = RegEnumValue(hKey, EnumIndex++, g_ValueNameBuffer,
            &cbValueName, NULL, &Type, g_ValueDataBuffer, &cbValueData))
            != ERROR_SUCCESS)
            break;

        //
        //  If cbValueName is zero, then this is the default value of
        //  the key, or the Windows 3.1 compatible key value.
        //

        if (cbValueName)
            PutString(g_ValueNameBuffer);

        else
            PutChar(TEXT('@'));

        PutChar(TEXT('='));

        switch (Type) {

            case REG_SZ:
                PutString((LPTSTR) g_ValueDataBuffer);
                break;

            case REG_DWORD:
                if (cbValueData == sizeof(DWORD)) {

                    PutLiteral(s_DwordPrefix);
                    PutDword(*((LPDWORD) g_ValueDataBuffer), TRUE);
                    break;

                }
                //  FALL THROUGH

            case REG_BINARY:
            default:
                PutBinary((LPBYTE) g_ValueDataBuffer, Type, cbValueData);
                break;

        }

        PutChar(TEXT('\n'));

        if (g_FileErrorStringID == IDS_EXPFILEERRFILEWRITE)
            return;

    }

    PutChar(TEXT('\n'));

    if (RegError != ERROR_NO_MORE_ITEMS)
        g_FileErrorStringID = IDS_EXPFILEERRREGENUM;

    //
    //  Write out all of the subkeys and recurse into them.
    //

    //copy the existing key into a new buffer with enough room for the next key
    nLenFullKey = lstrlen(lpFullKeyName);
    lpTempFullKeyName = (LPTSTR) alloca( (nLenFullKey+MAXKEYNAME)*sizeof(TCHAR));
    lstrcpy(lpTempFullKeyName, lpFullKeyName);
    lpSubKeyName = lpTempFullKeyName + nLenFullKey;
    *lpSubKeyName++ = TEXT('\\');
    *lpSubKeyName = 0;

    EnumIndex = 0;

    while (TRUE) {

        if ((RegError = RegEnumKey(hKey, EnumIndex++, lpSubKeyName,
            MAXKEYNAME)) != ERROR_SUCCESS)
            break;

        if(RegOpenKeyEx(hKey,lpSubKeyName,0,KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE,&hSubKey) == ERROR_SUCCESS) {

            PutBranch(hSubKey, lpTempFullKeyName);

            RegCloseKey(hSubKey);

            if (g_FileErrorStringID == IDS_EXPFILEERRFILEWRITE)
                return;

        }

        else
            g_FileErrorStringID = IDS_EXPFILEERRREGOPEN;

    }

    if (RegError != ERROR_NO_MORE_ITEMS)
        g_FileErrorStringID = IDS_EXPFILEERRREGENUM;

}

/*******************************************************************************
*
*  PutLiteral
*
*  DESCRIPTION:
*     Writes a literal string to the registry file stream.  No special handling
*     is done for the string-- it is written out as is.
*
*  PARAMETERS:
*     lpLiteral, null-terminated literal to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutLiteral(
    LPCTSTR lpLiteral
    )
{

    while (*lpLiteral != 0)
        PutChar(*lpLiteral++);

}

/*******************************************************************************
*
*  PutString
*
*  DESCRIPTION:
*     Writes a string to the registry file stream.  A string is surrounded by
*     double quotes and some characters may be translated to escape sequences
*     to enable a parser to read the string back in.
*
*  PARAMETERS:
*     lpString, null-terminated string to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutString(
    LPCTSTR lpString
    )
{

    TCHAR Char;

    PutChar(TEXT('"'));

    while ((Char = *lpString++) != 0) {

        switch (Char) {

            case TEXT('\\'):
            case TEXT('"'):
                PutChar(TEXT('\\'));
                //  FALL THROUGH

            default:
                PutChar(Char);
                break;

        }

    }

    PutChar(TEXT('"'));

}

/*******************************************************************************
*
*  PutBinary
*
*  DESCRIPTION:
*     Writes a sequence of hexadecimal bytes to the registry file stream.  The
*     output is formatted such that it doesn't exceed a defined line length.
*
*  PARAMETERS:
*     lpBuffer, bytes to write to file.
*     Type, value data type.
*     cbBytes, number of bytes to write.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutBinary(
    CONST BYTE FAR* lpBuffer,
    DWORD Type,
    DWORD cbBytes
    )
{

    BOOL fFirstByteOnLine;
    BYTE Byte;
    
    // If we're writing one of the string formats that regedit doesn't write
    // natively (but rather converts to a string of hex digits for streaming
    // out), AND we're writing in downlevel/ANSI/REGEDIT4 format, we aren't
    // going to write out the high byte of each (internally Unicode) character.
    // So we will be writing half as many characters as the buffer byte size.

    if (g_RegEditData.fSaveInDownlevelFormat &&
        ((Type == REG_EXPAND_SZ) || (Type == REG_MULTI_SZ))) {
        cbBytes = cbBytes / 2;
    }

    PutLiteral(s_HexPrefix);

    if (Type != REG_BINARY) {

        PutChar(TEXT('('));
        PutDword(Type, FALSE);
        PutChar(TEXT(')'));

    }

    PutChar(TEXT(':'));

    fFirstByteOnLine = TRUE;

    while (cbBytes--) {

        if (s_FileIo.CurrentColumn > 75 && !fFirstByteOnLine) {

            PutLiteral(s_FileLineBreak);

            fFirstByteOnLine = TRUE;

        }

        if (!fFirstByteOnLine)
            PutChar(TEXT(','));

        Byte = *lpBuffer++;

        // If we're writing one of the string formats that regedit doesn't 
        // write natively (REG_EXPAND_SZ and REG_MULTI_SZ values get converted 
        // to a string of hex digits for streaming out), AND we're writing in
        // downlevel/ANSI/REGEDIT4 format, we don't want to write out the high
        // byte of each (internally Unicode) character.  So in those cases, we
        // advance another byte to get to the next ANSI character.  Yes, this
        // will lose data on non-SBCS characters, but that's what you get for
        // saving in the downlevel format.

        if (g_RegEditData.fSaveInDownlevelFormat &&
            ((Type == REG_EXPAND_SZ) || (Type == REG_MULTI_SZ))) {
            lpBuffer++;
        }

        PutChar(g_HexConversion[Byte >> 4]);
        PutChar(g_HexConversion[Byte & 0x0F]);

        fFirstByteOnLine = FALSE;

    }

}

/*******************************************************************************
*
*  PutChar
*
*  DESCRIPTION:
*     Writes a 32-bit word to the registry file stream.
*
*  PARAMETERS:
*     Dword, dword to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutDword(
    DWORD Dword,
    BOOL fLeadingZeroes
    )
{

    int CurrentNibble;
    TCHAR Char;
    BOOL fWroteNonleadingChar;

    fWroteNonleadingChar = fLeadingZeroes;

    for (CurrentNibble = 7; CurrentNibble >= 0; CurrentNibble--) {

        Char = g_HexConversion[(Dword >> (CurrentNibble * 4)) & 0x0F];

        if (fWroteNonleadingChar || Char != TEXT('0')) {

            PutChar(Char);
            fWroteNonleadingChar = TRUE;

        }

    }

    //
    //  We need to write at least one character, so if we haven't written
    //  anything yet, just spit out one zero.
    //

    if (!fWroteNonleadingChar)
        PutChar(TEXT('0'));

}

/*******************************************************************************
*
*  PutChar
*
*  DESCRIPTION:
*     Writes one character to the registry file stream using an intermediate
*     buffer.
*
*  PARAMETERS:
*     Char, character to write to file.
*
*******************************************************************************/

VOID
NEAR PASCAL
PutChar(
    TCHAR Char
    )
{

    //
    //  Keep track of what column we're currently at.  This is useful in cases
    //  such as writing a large binary registry record.  Instead of writing one
    //  very long line, the other Put* routines can break up their output.
    //

    if (Char != TEXT('\n'))
        s_FileIo.CurrentColumn++;

    else {

        //
        //  Force a carriage-return, line-feed sequence to keep things like, oh,
        //  Notepad happy.
        //

        PutChar(TEXT('\r'));

        s_FileIo.CurrentColumn = 0;

    }

    s_FileIo.Buffer[s_FileIo.BufferOffset++] = Char;

    if (s_FileIo.BufferOffset == SIZE_FILE_IO_BUFFER)
        FlushIoBuffer();

}

/*******************************************************************************
*
*  FlushIoBuffer
*
*  DESCRIPTION:
*     Flushes the contents of the registry file stream to the disk and resets
*     the buffer pointer.
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
NEAR PASCAL
FlushIoBuffer(
    VOID
    )
{

    FILE_NUMBYTES NumberOfBytesWritten;

    if (s_FileIo.BufferOffset) {

        if (g_RegEditData.fSaveInDownlevelFormat)
        {
            //
            // Convert Unicode to ANSI before writing.
            //

            int i;

            i = WideCharToMultiByte(
                    CP_THREAD_ACP,
                    0,
                    s_FileIo.Buffer,
                    s_FileIo.BufferOffset,
                    s_FileIo.ConversionBuffer,
                    sizeof(s_FileIo.ConversionBuffer),
                    NULL,
                    NULL
                    );

            if (!WRITEFILE(s_FileIo.hFile, s_FileIo.ConversionBuffer, i,
                &NumberOfBytesWritten) || (FILE_NUMBYTES) i !=
                NumberOfBytesWritten)

                g_FileErrorStringID = IDS_EXPFILEERRFILEWRITE;
        }
        else
        {
            //
            // Write Unicode text
            //
            if (!WRITEFILE(s_FileIo.hFile, s_FileIo.Buffer, s_FileIo.BufferOffset * sizeof(WCHAR),
                &NumberOfBytesWritten) || (FILE_NUMBYTES) (s_FileIo.BufferOffset * sizeof(WCHAR)) !=
                NumberOfBytesWritten)
                g_FileErrorStringID = IDS_EXPFILEERRFILEWRITE;
        }
    }

    s_FileIo.BufferOffset = 0;

}
