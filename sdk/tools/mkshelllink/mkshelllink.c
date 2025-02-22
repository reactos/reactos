/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shell Link maker
 * FILE:            tools/mkshelllink/mkshelllink.c
 * PURPOSE:         Shell Link maker
 * PROGRAMMER:      Rafal Harabien
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#endif

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#define SW_SHOWNORMAL 1
#define SW_SHOWMINNOACTIVE 7
#define CSIDL_WINDOWS 0x24
#define CSIDL_SYSTEM 0x25

typedef struct _GUID
{
    uint32_t Data1;
    uint16_t  Data2;
    uint16_t  Data3;
    uint8_t  Data4[8];
} GUID;

typedef struct _FILETIME {
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
} FILETIME, *PFILETIME;

#define DEFINE_GUID2(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID name = { l,w1,w2,{ b1,b2,b3,b4,b5,b6,b7,b8 } }
DEFINE_GUID2(CLSID_ShellLink,0x00021401L,0,0,0xC0,0,0,0,0,0,0,0x46);
DEFINE_GUID2(CLSID_MyComputer,0x20D04FE0,0x3AEA,0x1069,0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D);

#define LINK_ID_LIST       0x01
#define LINK_FILE          0x02
#define LINK_DESCRIPTION   0x04
#define LINK_RELATIVE_PATH 0x08
#define LINK_WORKING_DIR   0x10
#define LINK_CMD_LINE_ARGS 0x20
#define LINK_ICON          0x40
#define LINK_UNICODE       0x80

#define LOCATOR_LOCAL   0x1
#define LOCATOR_NETWORK 0x2

#pragma pack(push, 1)

/* Specification: http://ithreats.files.wordpress.com/2009/05/lnk_the_windows_shortcut_file_format.pdf */

typedef struct _LNK_HEADER
{
    uint32_t Signature;
    GUID Guid;
    uint32_t Flags;
    uint32_t Attributes;
    FILETIME CreationTime;
    FILETIME ModificationTime;
    FILETIME LastAccessTime;
    uint32_t FileSize;
    uint32_t IconNr;
    uint32_t Show;
    uint32_t Hotkey;
    uint32_t Unknown;
    uint32_t Unknown2;
} LNK_HEADER;

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

typedef struct _LNK_LOCAL_VOLUME_INFO
{
    uint32_t Size;
    uint32_t VolumeType; /* See GetDriveType */
    uint32_t SerialNumber;
    uint32_t VolumeNameOffset;
    char VolumeLabel[0];
} LNK_LOCAL_VOLUME_INFO;

#define PT_GUID     0x1F
#define PT_DRIVE1   0x2F
#define PT_FOLDER   0x31
#define PT_VALUE    0x32

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

typedef struct _ID_LIST_GUID
{
    uint16_t Size;
    uint8_t Type;
    uint8_t dummy;
    GUID guid;
} ID_LIST_GUID;

typedef struct _ID_LIST_DRIVE
{
    uint16_t Size;
    uint8_t Type;
    char szDriveName[20];
    uint16_t unknown;
} ID_LIST_DRIVE;

#define EXP_SPECIAL_FOLDER_SIG 0xA0000005
typedef struct _EXP_SPECIAL_FOLDER
{
    uint32_t cbSize, dwSignature, idSpecialFolder, cbOffset;
} EXP_SPECIAL_FOLDER;

#pragma pack(pop)

static const struct SPECIALFOLDER {
    unsigned char csidl;
    const char* name;
} g_specialfolders[] = {
    { CSIDL_WINDOWS, "windows" },
    { CSIDL_SYSTEM, "system" },
    { 0, NULL}
};

static unsigned int is_path_separator(unsigned int c)
{
    return c == '\\' || c == '/';
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
    int IconNr = 0;
    GUID Guid = CLSID_MyComputer;
    int bHelp = 0, bMinimized = 0;
    FILE *pFile;
    LNK_HEADER Header;
    uint16_t uhTmp;
    uint32_t dwTmp;
    EXP_SPECIAL_FOLDER CsidlBlock, *pCsidlBlock = NULL;

    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] != '-' && argv[i][0] != '/')
            pszTarget = argv[i];
        else if (!strcmp(argv[i] + 1, "h"))
            bHelp = 1;
        else if (!strcmp(argv[i] + 1, "o") && i + 1 < argc)
            pszOutputPath = argv[++i];
        else if (!strcmp(argv[i] + 1, "d") && i + 1 < argc)
            pszDescription = argv[++i];
        else if (!strcmp(argv[i] + 1, "w") && i + 1 < argc)
            pszWorkingDir = argv[++i];
        else if (!strcmp(argv[i] + 1, "c") && i + 1 < argc)
            pszCmdLineArgs = argv[++i];
        else if (!strcmp(argv[i] + 1, "i") && i + 1 < argc)
        {
            pszIcon = argv[++i];
            if (i + 1 < argc && isdigit(argv[i + 1][0]))
                IconNr = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i] + 1, "m"))
            bMinimized = 1;
        else if (!strcmp(argv[i] + 1, "g") && i + 1 < argc)
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
            printf("Invalid option: %s\n", argv[i]);
    }

    if (!pszTarget || bHelp)
    {
        printf("Usage: %s [-o path][-d descr][-w path][-c cmd_line_args][-i icon_path [nr]][-h][-g guid] target\n"
               "-o path\tSets output path\n"
               "-d descr\tSets shortcut description\n"
               "-w path\tSets working directory for executable\n"
               "-c cmd_line_args\tSets command line arguments passed to program\n"
               "-i icon_path [nr]\tSets icon file and optionally icon index\n"
               "-m\tStart minimized\n"
               "-g guid\tSets GUID to which target path is relative. Default value is MyComputer GUID.\n"
               "target\tAbsolute or relative to guid specified with -g option path\n", argv[0]);
        return 0;
    }

    pFile = fopen(pszOutputPath, "wb");
    if (!pFile)
    {
        printf("Failed to open %s\n", pszOutputPath);
        return -1;
    }

    // Header
    memset(&Header, 0, sizeof(Header));
    Header.Signature = (uint32_t)'L';
    Header.Guid = CLSID_ShellLink;
    Header.Flags = LINK_ID_LIST;
    if (pszDescription)
        Header.Flags |= LINK_DESCRIPTION;
    if (pszWorkingDir)
        Header.Flags |= LINK_WORKING_DIR;
    if (pszCmdLineArgs)
        Header.Flags |= LINK_CMD_LINE_ARGS;
    if (pszIcon)
        Header.Flags |= LINK_ICON;
    Header.IconNr = IconNr;
    Header.Show = bMinimized ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL;
    fwrite(&Header, sizeof(Header), 1, pFile);

    if (Header.Flags & LINK_ID_LIST)
    {
        ID_LIST_FILE IdListFile;
        ID_LIST_GUID IdListGuid;
        ID_LIST_DRIVE IdListDrive;
        unsigned cbListSize = sizeof(IdListGuid) + sizeof(uint16_t), cchName;
        const char *pszName = pszTarget;
        int index = 1, specialindex = -1;
        const struct SPECIALFOLDER *special = get_special_folder(pszTarget);

        // ID list
        // It seems explorer does not accept links without id list. List is relative to desktop.

        if (special)
        {
            Header.Flags &= ~LINK_RELATIVE_PATH;
            CsidlBlock.cbSize = sizeof(CsidlBlock);
            CsidlBlock.dwSignature = EXP_SPECIAL_FOLDER_SIG;
            CsidlBlock.idSpecialFolder = special->csidl;
            specialindex = 3; // Skip GUID, drive and fake windows/reactos folder
            sprintf(targetpath, "x:\\reactos\\%s", pszTarget + sizeof("shell:") + strlen(special->name));
            pszName = pszTarget = targetpath;
        }

        if (pszName[0] && pszName[0] != ':' && pszName[1] == ':')
        {
            ++index;
            cbListSize += sizeof(IdListDrive);
            pszName += 2;
            while (*pszName == '\\' || *pszName == '/')
                ++pszName;
        }

        while (*pszName)
        {
            cchName = 0;
            while (pszName[cchName] && pszName[cchName] != '\\' && pszName[cchName] != '/')
                ++cchName;

            if (cchName != 1 || pszName[0] != '.')
                cbListSize += sizeof(IdListFile) + 2 * (cchName + 1);

            if (++index == specialindex)
            {
                CsidlBlock.cbOffset = cbListSize - sizeof(uint16_t);
                pCsidlBlock = &CsidlBlock;
            }

            pszName += cchName;
            while (*pszName == '\\' || *pszName == '/')
                ++pszName;
        }

        uhTmp = cbListSize;
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile); // size

        IdListGuid.Size = sizeof(IdListGuid);
        IdListGuid.Type = PT_GUID;
        IdListGuid.dummy = 0x50;
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
            while(*pszName == '\\' || *pszName == '/')
                ++pszName;
        }

        while (*pszName)
        {
            cchName = 0;
            while (pszName[cchName] && pszName[cchName] != '\\' && pszName[cchName] != '/')
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
            while (*pszName == '\\' || *pszName == '/')
                ++pszName;
        }

        uhTmp = 0; // list end
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
    }

    if (Header.Flags & LINK_DESCRIPTION)
    {
        // Description
        uhTmp = strlen(pszDescription);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszDescription, pFile);
    }

    if (Header.Flags & LINK_RELATIVE_PATH)
    {
        // Relative Path
        uhTmp = strlen(pszTarget);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszTarget, pFile);
    }

    if (Header.Flags & LINK_WORKING_DIR)
    {
        // Working Dir
        uhTmp = strlen(pszWorkingDir);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszWorkingDir, pFile);
    }

    if (Header.Flags & LINK_CMD_LINE_ARGS)
    {
        // Command line arguments
        uhTmp = strlen(pszCmdLineArgs);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszCmdLineArgs, pFile);
    }

    if (Header.Flags & LINK_ICON)
    {
        // Command line arguments
        uhTmp = strlen(pszIcon);
        fwrite(&uhTmp, sizeof(uhTmp), 1, pFile);
        fputs(pszIcon, pFile);
    }

    // Extra stuff
    if (pCsidlBlock)
        fwrite(pCsidlBlock, sizeof(*pCsidlBlock), 1, pFile);
    dwTmp = 0;
    fwrite(&dwTmp, sizeof(dwTmp), 1, pFile);

    fclose(pFile);

    return 0;
}
