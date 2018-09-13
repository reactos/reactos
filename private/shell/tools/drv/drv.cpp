// blesslnk.cpp : Program to bless a LNK file with darwin descriptor
// or logo3 app name/id

#include <tchar.h>
#include <stdio.h>
#include <windows.h>

#define PRINTCASE(a) case (a): printf(TEXT("    %s"), TEXT(#a)); break;

#define FLAG_HELP           0x00000001
#define FLAG_DEBUG          0x00000002
#define FLAG_DRIVE          0x00000004
#define FLAG_FILE           0x00000008
#define FLAG_MOUNTPOINT     0x00000010
#define FLAG_VOLUMEGUID     0x00000020

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

typedef struct _tagFLAGASSOC
{
    TCHAR szName;
    DWORD dwFlag;
} FLAGASSOC;

FLAGASSOC FlagList[] = 
{
    { TEXT('?'), FLAG_HELP       },
    { TEXT('b'), FLAG_DEBUG      },
    { TEXT('d'), FLAG_DRIVE      },
    { TEXT('f'), FLAG_FILE       },
#if 0
    { TEXT('m'), FLAG_MOUNTPOINT },
    { TEXT('g'), FLAG_VOLUMEGUID },
#endif
};

int __cdecl main(int argc, char * argv[])
{
    TCHAR szName[MAX_PATH];
    DWORD dwFlags = 0;
    BOOL fGotFileName = FALSE;

    // process the args
    for (int i = 1; i < argc; ++i)
    {
        if ((TEXT('/') == argv[i][0]) || (TEXT('-') == argv[i][0]))
        {
            // This is a flag
            for (int j = 0; j < ARRAYSIZE(FlagList); ++j)
            {
                if ((FlagList[j].szName == argv[i][1]) || 
                    ((FlagList[j].szName + (TEXT('a') - TEXT('A'))) == argv[i][1]))
                {
                    dwFlags |= FlagList[j].dwFlag;
                    break;
                }
            }
        }
        else
        {
            // This is the filename
            lstrcpyn(szName, argv[i], ARRAYSIZE(szName));
            fGotFileName = TRUE;
        }
    }

    if (!fGotFileName)
    {
        if (!GetCurrentDirectory(ARRAYSIZE(szName), szName))
        {
            dwFlags = FLAG_HELP;
        }
    } 

    if (!dwFlags)
    {
        dwFlags = FLAG_FILE;
    }

    if (dwFlags & FLAG_HELP)
    {
        dwFlags = FLAG_HELP;
    }

    if (dwFlags & FLAG_DEBUG)
    {
        printf("DBG: %d\n", argc);

        for (int i = 0; i < argc; ++i)
            printf("DBG: Arg #%d is \"%s\"\n", i, argv[i]);
    }

    if (dwFlags & FLAG_HELP)
    {
        printf(TEXT("\nDRV [/F] [/D] [/B] [/?] [path]"));
        printf(TEXT("\n\n  [path]"));
        printf(TEXT("\n              Specifies a drive, file, drive mounted on a folder, or Volume GUID."));
        printf(TEXT("\n\n  /F          Retrieves file information (GetFileAttributes)."));
        printf(TEXT("\n  /D          Retrieves drive information (GetDriveType + GetVolumeInformation)."));
        printf(TEXT("\n  /B          Dumps debug info."));
        printf(TEXT("\n  /?          Displays this message."));
        printf(TEXT("\n\nIf no switches are provided, '/F' is assumed.  For mounted volumes on\nfolder problems, try appending a backslash.\n\n(Source code: shell\\tools\\drv)"));
    }
    else
    {
        printf(TEXT("\n---------------------------------------------------\n--- For: \"%s\""), szName);
    }

    if (dwFlags & FLAG_FILE)
    {
        // GetFileAttributes
        {
            printf(TEXT("\n..................................................."));
            printf(TEXT("\n... GetFileAttributes()\n"));

            DWORD dwFA = GetFileAttributes(szName);

            if (0xFFFFFFFF == dwFA)
            {
                printf(TEXT("\n    Return Value:    0x%08X"), dwFA);
            }
            else
            {
                for (DWORD i = 0; i < 32; ++i)
                {
                    DWORD dw = 1 << i;

                    if (dw & dwFA)
                    {
                        printf(TEXT("\n"));

                        switch (dw)
                        {
                            PRINTCASE(FILE_ATTRIBUTE_READONLY           );
                            PRINTCASE(FILE_ATTRIBUTE_HIDDEN             );
                            PRINTCASE(FILE_ATTRIBUTE_SYSTEM             );
                            PRINTCASE(FILE_ATTRIBUTE_DIRECTORY          );
                            PRINTCASE(FILE_ATTRIBUTE_ARCHIVE            );
                            PRINTCASE(FILE_ATTRIBUTE_DEVICE             );
                            PRINTCASE(FILE_ATTRIBUTE_NORMAL             );
                            PRINTCASE(FILE_ATTRIBUTE_TEMPORARY          );
                            PRINTCASE(FILE_ATTRIBUTE_SPARSE_FILE        );
                            PRINTCASE(FILE_ATTRIBUTE_REPARSE_POINT      );
                            PRINTCASE(FILE_ATTRIBUTE_COMPRESSED         );
                            PRINTCASE(FILE_ATTRIBUTE_OFFLINE            );
                            PRINTCASE(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
                            PRINTCASE(FILE_ATTRIBUTE_ENCRYPTED          );

                            default:
                                printf(TEXT("    %08X"), i);
                                break;
                        }
                    }
                }
            }
        }
    }

    if (dwFlags & FLAG_DRIVE)
    {
        // GetDriveType
        {
            printf(TEXT("\n..................................................."));
            printf(TEXT("\n... GetDriveType()\n"));

            UINT u = GetDriveType(szName);

            printf(TEXT("\n"));

            switch (u)
            {
                PRINTCASE(DRIVE_UNKNOWN);
                PRINTCASE(DRIVE_NO_ROOT_DIR);
                PRINTCASE(DRIVE_REMOVABLE);
                PRINTCASE(DRIVE_FIXED);
                PRINTCASE(DRIVE_REMOTE);
                PRINTCASE(DRIVE_CDROM);
                PRINTCASE(DRIVE_RAMDISK);
            }
        }
    }

    if (dwFlags & FLAG_DRIVE)
    {
        // GetVolumeInformation
        {
            printf(TEXT("\n..................................................."));
            printf(TEXT("\n... GetVolumeInformation()\n"));
                           
            {
                TCHAR szVolumeName[MAX_PATH];
                DWORD dwVolSerialNumber;
                DWORD dwMaxCompName;
                DWORD dwFileSysFlags;
                TCHAR szFileSysName[MAX_PATH];

                BOOL b = GetVolumeInformation(
                    szName,
                    szVolumeName,
                    MAX_PATH,
                    &dwVolSerialNumber,
                    &dwMaxCompName,
                    &dwFileSysFlags,
                    szFileSysName,
                    MAX_PATH);

                if (!b)
                {
                    printf(TEXT("\n    Return Value:         %hs"), b ? "TRUE" : "FALSE");
                }
                else
                {
                    printf(TEXT("\n    Volume Name:          \"%s\""), szVolumeName);
                    printf(TEXT("\n    Volume Serial#:       %04X-%04X"), HIWORD(dwVolSerialNumber),
                        LOWORD(dwVolSerialNumber));
                    printf(TEXT("\n    Volume Max Comp Name: %d"), dwMaxCompName);
                    printf(TEXT("\n    File System Name:     \"%s\""), szFileSysName);
                    printf(TEXT("\n    File System Flags:"));

                    for (DWORD i = 0; i < 32; ++i)
                    {
                        DWORD dw = 1 << i;

                        if (dw & dwFileSysFlags)
                        {
                            printf(TEXT("\n    "));

                            switch (dw)
                            {
                                PRINTCASE(FILE_CASE_SENSITIVE_SEARCH  );
                                PRINTCASE(FILE_CASE_PRESERVED_NAMES   );
                                PRINTCASE(FILE_UNICODE_ON_DISK        );
                                PRINTCASE(FILE_PERSISTENT_ACLS        );
                                PRINTCASE(FILE_FILE_COMPRESSION       );
                                PRINTCASE(FILE_VOLUME_QUOTAS          );
                                PRINTCASE(FILE_SUPPORTS_SPARSE_FILES  );
                                PRINTCASE(FILE_SUPPORTS_REPARSE_POINTS);
                                PRINTCASE(FILE_SUPPORTS_REMOTE_STORAGE);
                                PRINTCASE(FILE_VOLUME_IS_COMPRESSED   );
                                PRINTCASE(FILE_SUPPORTS_OBJECT_IDS    );
                                PRINTCASE(FILE_SUPPORTS_ENCRYPTION    );

                                default:
                                    printf(TEXT("    %08X"), i);
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }

    printf(TEXT("\n"));

    return 0;
}                       
