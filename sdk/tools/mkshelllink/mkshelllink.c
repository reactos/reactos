/*
 * PROJECT:     ReactOS Shell Link maker
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shell Link maker
 * COPYRIGHT:   Copyright 2011 Rafal Harabien <rafalh@reactos.org>
 *              Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

/* INCLUDES ******************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // For isalpha(), isdigit()

#ifndef C_ASSERT
#define C_ASSERT(expr) extern char (*c_assert(void)) [(expr) ? 1 : -1]
#endif

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef __int32 int32_t;
#endif

#ifdef _WIN32
#define strcasecmp _stricmp
#endif


/* SHELL LINK DEFINITIONS ****************************************************/

#define SW_SHOWNORMAL 1
#define SW_SHOWMINNOACTIVE 7

typedef struct _GUID
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID;
C_ASSERT(sizeof(GUID) == 16);

typedef struct _FILETIME
{
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
} FILETIME, *PFILETIME;

#define DEFINE_GUID2(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID name = { l,w1,w2,{ b1,b2,b3,b4,b5,b6,b7,b8 } }
DEFINE_GUID2(CLSID_ShellLink, 0x00021401,0,0,0xC0,0,0,0,0,0,0,0x46);
DEFINE_GUID2(CLSID_MyComputer,0x20D04FE0,0x3AEA,0x1069,0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D);

#define LOCATOR_LOCAL   0x1
#define LOCATOR_NETWORK 0x2

#pragma pack(push, 1)

/*
 * Specification:
 * "[MS-SHLLINK]: Shell Link (.LNK) Binary File Format"
 * https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-shllink/16cb4ca1-9339-4d0c-a68d-bf1d6cc0f943
 * Definitions adapted from undocshell.h
 */

/* SHELL_LINK_HEADER flags. SHELL_LINK_DATA_FLAGS definitions taken from shlobj.h */
#define SLDF_DEFAULT            0x00000000
#define SLDF_HAS_ID_LIST        0x00000001
#define SLDF_HAS_LINK_INFO      0x00000002
#define SLDF_HAS_NAME           0x00000004
#define SLDF_HAS_RELPATH        0x00000008
#define SLDF_HAS_WORKINGDIR     0x00000010
#define SLDF_HAS_ARGS           0x00000020
#define SLDF_HAS_ICONLOCATION   0x00000040
#define SLDF_UNICODE            0x00000080

#define LINK_ID_LIST            SLDF_HAS_ID_LIST
#define LINK_FILE               SLDF_HAS_LINK_INFO
#define LINK_DESCRIPTION        SLDF_HAS_NAME
#define LINK_RELATIVE_PATH      SLDF_HAS_RELPATH
#define LINK_WORKING_DIR        SLDF_HAS_WORKINGDIR
#define LINK_CMDLINE_ARGS       SLDF_HAS_ARGS
#define LINK_ICON               SLDF_HAS_ICONLOCATION
#define LINK_UNICODE            SLDF_UNICODE

// SHELL_LINK_HEADER
typedef struct _LNK_HEADER
{
    uint32_t Size;
    GUID Guid;
    uint32_t Flags;
    uint32_t Attributes;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
    uint32_t FileSizeLow; /* Only the least significant 32 bits */
    int32_t  IconIndex;
    uint32_t ShowCmd;
    uint16_t HotKey;
    uint16_t Reserved1;
    uint32_t Reserved2;
    uint32_t Reserved3;
} LNK_HEADER;

// SHELL_LINK_INFOA
typedef struct _LNK_LOCATOR_INFO
{
    uint32_t Size;
    uint32_t DataOffset;
    uint32_t Flags;
    uint32_t LocalVolumeInfoOffset;
    uint32_t LocalBasePathnameOffset;
    uint32_t NetworkVolumeInfoOffset;
    uint32_t RemainingPathnameOffset;
    char Data[0];
} LNK_LOCATOR_INFO;

// SHELL_LINK_INFO_VOLUME_IDA
typedef struct _LNK_LOCAL_VOLUME_INFO
{
    uint32_t Size;
    uint32_t VolumeType; /* See GetDriveType */
    uint32_t SerialNumber;
    uint32_t VolumeNameOffset;
    char VolumeLabel[0];
} LNK_LOCAL_VOLUME_INFO;

/* For ITEMIDLIST/SHITEMID */
#define PT_GUID     0x1F
#define PT_DRIVE1   0x2F
#define PT_FOLDER   0x31
#define PT_VALUE    0x32

/* uSortOrder values */
#define REGITEMORDER_DEFAULT            0x80
#define REGITEMORDER_LIBRARIES          0x42
#define REGITEMORDER_USERSFILEFOLDER    0x44
#define REGITEMORDER_MYCOMPUTER         0x50

// struct tagGUIDStruct
typedef struct _ID_LIST_GUID
{
    uint16_t Size;
    uint8_t Type;
    uint8_t uSortOrder;
    GUID guid;
} ID_LIST_GUID;

// struct tagDriveStruct
typedef struct _ID_LIST_DRIVE
{
    uint16_t Size;
    uint8_t Type;
    char szDriveName[20];
    uint16_t unknown;
} ID_LIST_DRIVE;

// struct tagFileStruct
typedef struct _ID_LIST_FILE
{
    uint16_t Size;
    uint8_t Type;
    uint8_t dummy;
    uint32_t dwFileSize;
    uint16_t uFileDate;
    uint16_t uFileTime;
    uint16_t uFileAttribs;
    char szName[0];
} ID_LIST_FILE;

#define EXP_SPECIAL_FOLDER_SIG 0xA0000005
typedef struct _EXP_SPECIAL_FOLDER
{
    uint32_t cbSize;
    uint32_t dwSignature;
    uint32_t idSpecialFolder;
    uint32_t cbOffset;
} EXP_SPECIAL_FOLDER;

#pragma pack(pop)


/* GLOBALS *******************************************************************/

/* For a complete list, see: https://smallvoid.com/article/winnt-shell-keyword.html */
#define CSIDL_WINDOWS   0x24
#define CSIDL_SYSTEM    0x25
static const struct SPECIALFOLDER
{
    unsigned char csidl;
    const char* name;
    const char* dummyPath;
} g_specialfolders[] = {
    { CSIDL_WINDOWS, "windows", "X:\\reactos" },
    { CSIDL_SYSTEM,  "system",  "X:\\reactos\\system32" },
    { 0, NULL, NULL }
};


/* FUNCTIONS *****************************************************************/

static bool is_path_separator(char c)
{
    return (c == '\\' || c == '/');
}

static const struct SPECIALFOLDER* get_special_folder(const char *target)
{
    char buf[256];

    strncpy(buf, target, sizeof(buf));
    buf[sizeof("shell:") - 1] = '\0';
    if (strcasecmp("shell:", buf))
        return NULL;

    target += sizeof("shell:") - 1;
    for (unsigned long i = 0;; ++i)
    {
        unsigned long len;
        const struct SPECIALFOLDER *special = &g_specialfolders[i];
        if (!special->name)
            return NULL;
        len = strlen(special->name);
        strncpy(buf, target, sizeof(buf));
        buf[len] = '\0';
        if (!strcasecmp(special->name, buf) && (is_path_separator(target[len]) || !target[len]))
            return &g_specialfolders[i];
    }
}

int main(int argc, const char *argv[])
{
    int i;
    const char *pszOutputPath = "shortcut.lnk";
    const char *pszTarget = NULL;
    const char *pszDescription = NULL;
    const char *pszWorkingDir = NULL;
    const char *pszCmdLineArgs = NULL;
    const char *pszIcon = NULL;
    char targetpath[260];
    int32_t IconIndex = 0;
    GUID Guid = CLSID_MyComputer;
    bool bHelp = false, bMinimized = false;
    FILE *pFile;
    LNK_HEADER Header;
    uint16_t uhTmp;
    uint32_t dwTmp;
    EXP_SPECIAL_FOLDER CsidlBlock, *pCsidlBlock = NULL;

    /* Parse the command-line */
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '-' && argv[i][0] != '/')
            pszTarget = argv[i];
        else if (!strcmp(argv[i] + 1, "h"))
            bHelp = true;
        else if (!strcmp(argv[i] + 1, "o") && (i + 1 < argc))
            pszOutputPath = argv[++i];
        else if (!strcmp(argv[i] + 1, "d") && (i + 1 < argc))
            pszDescription = argv[++i];
        else if (!strcmp(argv[i] + 1, "w") && (i + 1 < argc))
            pszWorkingDir = argv[++i];
        else if (!strcmp(argv[i] + 1, "c") && (i + 1 < argc))
            pszCmdLineArgs = argv[++i];
        else if (!strcmp(argv[i] + 1, "i") && (i + 1 < argc))
        {
            pszIcon = argv[++i];
            if ((i + 1 < argc) && isdigit(argv[i + 1][0]))
                IconIndex = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i] + 1, "m"))
            bMinimized = true;
        else if (!strcmp(argv[i] + 1, "g") && (i + 1 < argc))
        {
            unsigned Data4Tmp[8], j;

            sscanf(argv[++i], "{%8x-%4hx-%4hx-%2x%2x-%2x%2x%2x%2x%2x%2x}",
                   &Guid.Data1, &Guid.Data2, &Guid.Data3,
                   &Data4Tmp[0], &Data4Tmp[1], &Data4Tmp[2], &Data4Tmp[3],
                   &Data4Tmp[4], &Data4Tmp[5], &Data4Tmp[6], &Data4Tmp[7]);
            for (j = 0; j < 8; ++j)
                Guid.Data4[j] = (uint8_t)Data4Tmp[j];
        }
        else
        {
            printf("Invalid option: %s\n", argv[i]);
            bHelp = true;
        }
    }

    if (!pszTarget || bHelp)
    {
        printf("Usage: %s [-h][-o path][-d descr][-w path][-c cmdline_args][-i icon_path [nr]][-g guid] target\n"
               "-h\tShows this help.\n"
               "-o path\tSets the output path.\n"
               "-d descr\tSets the shortcut description.\n"
               "-w path\tSets the working directory for the executable.\n"
               "-c cmdline_args\tSets the command-line arguments passed to the program.\n"
               "-i icon_path [nr]\tSets the icon file and optionally its index.\n"
               "-m\tStart minimized.\n"
               "-g guid\tSets the GUID to which the target path is relative. Default value is MyComputer GUID.\n"
               "target\tAbsolute or relative to GUID specified with the -g option path.\n", argv[0]);
        return 0;
    }

    pFile = fopen(pszOutputPath, "wb");
    if (!pFile)
    {
        printf("Failed to open %s\n", pszOutputPath);
        return -1;
    }

    /* Header */
    memset(&Header, 0, sizeof(Header));
    Header.Size = sizeof(Header);
    Header.Guid = CLSID_ShellLink;
    Header.Flags = LINK_ID_LIST;
    if (pszDescription)
        Header.Flags |= LINK_DESCRIPTION;
    if (pszWorkingDir)
        Header.Flags |= LINK_WORKING_DIR;
    if (pszCmdLineArgs)
        Header.Flags |= LINK_CMDLINE_ARGS;
    if (pszIcon)
        Header.Flags |= LINK_ICON;
    Header.IconIndex = IconIndex;
    Header.ShowCmd = (bMinimized ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL);
    fwrite(&Header, sizeof(Header), 1, pFile);

    if (Header.Flags & LINK_ID_LIST)
    {
        ID_LIST_FILE IdListFile;
        ID_LIST_GUID IdListGuid;
        ID_LIST_DRIVE IdListDrive;
        unsigned cbListSize = sizeof(IdListGuid) + sizeof(uint16_t), cchName;
        const char *pszName = pszTarget;
        size_t specialPathLen = 0;
        const struct SPECIALFOLDER *special = get_special_folder(pszTarget);

        /* ID list. It appears explorer does not accept links
         * without an ID list. It is relative to desktop. */
        if (special)
        {
            Header.Flags &= ~LINK_RELATIVE_PATH;
            CsidlBlock.cbSize = sizeof(CsidlBlock);
            CsidlBlock.dwSignature = EXP_SPECIAL_FOLDER_SIG;
            CsidlBlock.idSpecialFolder = special->csidl;
            specialPathLen = strlen(special->dummyPath);
            sprintf(targetpath, "%s\\%s", special->dummyPath, pszTarget + sizeof("shell:") + strlen(special->name));
            pszName = pszTarget = targetpath;
        }

        if (pszName[0] && pszName[0] != ':' && pszName[1] == ':')
        {
            cbListSize += sizeof(IdListDrive);
            pszName += 2;
            while (is_path_separator(*pszName))
                ++pszName;
        }

        while (*pszName)
        {
            cchName = 0;
            while (pszName[cchName] && !is_path_separator(pszName[cchName]))
                ++cchName;

            if (cchName != 1 || pszName[0] != '.')
                cbListSize += sizeof(IdListFile) + 2 * (cchName + 1);

            if (special && ((pszName+cchName)-pszTarget == specialPathLen))
            {
                /* Point the special folder block to the rest of the path in the ID list */
                CsidlBlock.cbOffset = cbListSize - sizeof(uint16_t);
                pCsidlBlock = &CsidlBlock;
            }

            pszName += cchName;
            while (is_path_separator(*pszName))
                ++pszName;
        }

        /* ID list size */
        uhTmp = cbListSize;
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);

        IdListGuid.Size = sizeof(IdListGuid);
        IdListGuid.Type = PT_GUID;
        IdListGuid.uSortOrder = REGITEMORDER_MYCOMPUTER;
        IdListGuid.guid = Guid;
        fwrite(&IdListGuid, sizeof(IdListGuid), 1, pFile);

        pszName = pszTarget;

        if (isalpha(pszName[0]) && pszName[1] == ':')
        {
            memset(&IdListDrive, 0, sizeof(IdListDrive));
            IdListDrive.Size = sizeof(IdListDrive);
            IdListDrive.Type = PT_DRIVE1;
            sprintf(IdListDrive.szDriveName, "%c:\\", pszName[0]);
            fwrite(&IdListDrive, sizeof(IdListDrive), 1, pFile);
            pszName += 2;
            while (is_path_separator(*pszName))
                ++pszName;
        }

        while (*pszName)
        {
            cchName = 0;
            while (pszName[cchName] && !is_path_separator(pszName[cchName]))
                ++cchName;

            if (cchName != 1 || pszName[0] != '.')
            {
                memset(&IdListFile, 0, sizeof(IdListFile));
                IdListFile.Size = sizeof(IdListFile) + 2 * (cchName + 1);
                if (!pszName[cchName])
                    IdListFile.Type = PT_VALUE; // File
                else
                    IdListFile.Type = PT_FOLDER;
                fwrite(&IdListFile, sizeof(IdListFile), 1, pFile);
                fwrite(pszName, cchName, 1, pFile);
                fputc(0, pFile);
                fwrite(pszName, cchName, 1, pFile);
                fputc(0, pFile);
            }

            pszName += cchName;
            while (is_path_separator(*pszName))
                ++pszName;
        }

        /* End of ID list */
        uhTmp = 0;
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
    }

    if (Header.Flags & LINK_DESCRIPTION)
    {
        /* Description */
        uhTmp = strlen(pszDescription);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszDescription, pFile);
    }

    if (Header.Flags & LINK_RELATIVE_PATH)
    {
        /* Relative path */
        uhTmp = strlen(pszTarget);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszTarget, pFile);
    }

    if (Header.Flags & LINK_WORKING_DIR)
    {
        /* Working directory */
        uhTmp = strlen(pszWorkingDir);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszWorkingDir, pFile);
    }

    if (Header.Flags & LINK_CMDLINE_ARGS)
    {
        /* Command-line arguments */
        uhTmp = strlen(pszCmdLineArgs);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszCmdLineArgs, pFile);
    }

    if (Header.Flags & LINK_ICON)
    {
        /* Icon path */
        uhTmp = strlen(pszIcon);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszIcon, pFile);
    }

    /* Write the data block list */
    if (pCsidlBlock)
        fwrite(pCsidlBlock, sizeof(*pCsidlBlock), 1, pFile);
    /* End of data block list */
    dwTmp = 0;
    fwrite(&dwTmp, sizeof(dwTmp), 1, pFile);

    fclose(pFile);

    return 0;
}
