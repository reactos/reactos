/*****************************************************************************

  Unfrag

*****************************************************************************/


#include "Unfrag.h"
#include "DriveVolume.h"
#include "Defragment.h"
#include <process.h>


bool QuietMode = false;
bool VerboseMode = false;


// Makes sure we're in Windows 2000
bool CheckWinVer (void)
{
    OSVERSIONINFO OSVersion;

    ZeroMemory (&OSVersion, sizeof (OSVersion));
    OSVersion.dwOSVersionInfoSize = sizeof (OSVersion);
    GetVersionEx (&OSVersion);
    
    // Need Windows 2000!

    // Check for NT first
    // Actually what we do is check that we're not on Win31+Win32s and that we're
    // not in Windows 9x. It's possible that there could be more Windows "platforms"
    // in the future and there's no sense in claiming incompatibility.
    if (OSVersion.dwPlatformId == VER_PLATFORM_WIN32s  ||
        OSVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        return (false);
    }

    // Ok we're in Windows NT, now make sure we're in 2000
    if (OSVersion.dwMajorVersion < 5)
        return (false);

    // Kew, we're in at least Windows 2000 ("NT 5.0")
    return (true);
}


char *AddCommas (char *Result, uint64 Number)
{
	char  Temp[128];
	int   TempLen;
	char *p = NULL;
	int   AddCommas = 0;
	char *StrPosResult = NULL;
	char *StrPosOrig = NULL;

	// we get the string form of the number, then we count down w/ AddCommas
	// while copying the string from Temp1 to Result. when AddCommas % 3  == 1,
	// slap in a commas as well, before the #.
	sprintf (Temp, "%I64u", Number);
	AddCommas = TempLen = strlen (Temp);
	StrPosOrig   = Temp;
	StrPosResult = Result;
	while (AddCommas)
	{
		if ((AddCommas % 3) == 0  &&  AddCommas != TempLen) // avoid stuff like ",345"
		{
			*StrPosResult = ',';
			StrPosResult++;
		}

		*StrPosResult = *StrPosOrig;
		StrPosResult++;
		StrPosOrig++;

		*StrPosResult = 0;

		AddCommas--;
	}

	return (Result);
}


void PrintBanner (void)
{
    printf ("%s v%s\n", APPNAME_CLI, APPVER_STR);
    printf ("%s\n", APPCOPYRIGHT);
    printf ("\n");

    return;
}


void FraggerHelp (void)
{
    printf ("Usage: unfrag drive: [...] <-f | -e>\n");
    printf ("\n");
    printf ("drive:  : The drive to defrag. Should be two characters long, ie 'c:' or 'd:'.\n");
    printf ("          Multiple drives may be given, and all will be simultaneously\n");
    printf ("          defragmented using the same options.\n");
    printf ("-f      : Do a fast defragmentation. Files that are not fragmented will not be\n");
    printf ("          moved. Only one pass is made over the file list. Using this option\n");
    printf ("          may result in not all files being defragmented, depending on\n");
    printf ("          available disk space.\n");
    printf ("-e      : Do an extensive defragmention. Files will be moved in an attempt to\n");
    printf ("          defragment both files and free space.\n");

    if (!CheckWinVer())
    {
        printf ("\n");
        printf ("NOTE: This program requires Windows 2000, which is not presently running on\n");
        printf ("this system.\n");
    }

    return;
}


void __cdecl DefragThread (LPVOID parm)
{
    Defragment *Defrag;

    Defrag = (Defragment *) parm;
    Defrag->Start ();

    _endthread ();
    return;
}


Defragment *StartDefragThread (string Drive, DefragType Method, HANDLE &Handle)
{
    Defragment *Defragger;
    unsigned long Thread;

    Defragger = new Defragment (Drive, Method);
    //Thread = /*CreateThread*/ _beginthreadex (NULL, 0, DefragThread, Defragger, 0, &ThreadID);
    Thread = _beginthread (DefragThread, 0, Defragger);
    Handle = *((HANDLE *)&Thread);
    return (Defragger);
}


// Main Initialization
int __cdecl main (int argc, char **argv)
{
    vector<string>       Drives;
    vector<Defragment *> Defrags;
    DefragType           DefragMode = DefragInvalid;
    int                  d;

    PrintBanner ();

    // Parse command line arguments
    bool ValidCmdLine = false;
    for (int c = 0; c < argc; c++)
    {
        if (strlen(argv[c]) == 2  &&  argv[c][1] == ':')
        {
            Drives.push_back (strupr(argv[c]));
        }
        else
        if (argv[c][0] == '-'  ||  argv[c][0] == '/'  &&  strlen(argv[c]) == 2)
        {
            switch (tolower(argv[c][1]))
            {
                case '?' :
                case 'h' :
                    FraggerHelp ();
                    return (0);

                case 'f' :
                    if (DefragMode != DefragInvalid)
                    {
                        ValidCmdLine = false;
                        break;
                    }
                    DefragMode = DefragFast;
                    ValidCmdLine = true;
                    break;

                case 'e' :
                    if (DefragMode != DefragInvalid)
                    {
                        ValidCmdLine = false;
                        break;
                    }
                    DefragMode = DefragExtensive;
                    ValidCmdLine = true;
                    break;

            }
        }
    }

    if (DefragMode == DefragInvalid)
        ValidCmdLine = false;

    if (!ValidCmdLine)
    {
        printf ("Invalid command-line options. Use '%s -?' for help.\n", argv[0]);
        return (0);
    }

    // Check OS requirements
    if (!CheckWinVer())
    {
        printf ("Fatal Error: This program requires Windows 2000.\n");
        return (0);
    }

    for (d = 0; d < Drives.size (); d++)
    {
        HANDLE TossMe;
        Defrags.push_back (StartDefragThread (Drives[d], DefragMode, TossMe));
    }

    for (d = 0; d < Drives.size () - 1; d++)
        printf ("\n ");

    bool Continue = true;
    HANDLE Screen;

    Screen = GetStdHandle (STD_OUTPUT_HANDLE);
    while (Continue)
    {
        Sleep (25);

        // Get current screen coords
        CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

        GetConsoleScreenBufferInfo (Screen, &ScreenInfo);

        // Now set back to the beginning
        ScreenInfo.dwCursorPosition.X = 0;
        ScreenInfo.dwCursorPosition.Y -= Drives.size();
        SetConsoleCursorPosition (Screen, ScreenInfo.dwCursorPosition);

        for (d = 0; d < Drives.size (); d++)
        {
            printf ("\n%6.2f%% %-70s", Defrags[d]->GetStatusPercent(), Defrags[d]->GetStatusString().c_str());
        }

        // Determine if we should keep going
        Continue = false;
        for (d = 0; d < Drives.size (); d++)
        {
            if (!Defrags[d]->IsDoneYet()  &&  !Defrags[d]->HasError())
                Continue = true;                
        }
    }

#if 0
    // Loop through the drives list
    for (int d = 0; d < Drives.size(); d++)
    {
        DriveVolume *Drive;

        Drive = new DriveVolume;

        // First thing: build a file list.
        printf ("Opening volume %s ...", Drives[d].c_str());
        if (!Drive->Open (Drives[d]))
        {
            printf ("FAILED\n\n");
            delete Drive;
            continue;
        }
        printf ("\n");

        printf ("    Getting drive bitmap ...");
        if (!Drive->GetBitmap ())
        {
            printf ("FAILED\n\n");
            delete Drive;
            continue;
        }
        printf ("\n");

        printf ("    Obtaining drive geometry ...");
        if (!Drive->ObtainInfo ())
        {
            printf ("FAILED\n\n");
            delete Drive;
            continue;
        }
        printf ("\n");

        printf ("    Building file database for drive %s ...", Drives[d].c_str());
        if (!Drive->BuildFileList ())
        {
            printf ("FAILED\n\n");
            delete Drive;
            continue;
        }
        printf ("\n");

        printf ("    %u files\n", Drive->GetDBFileCount ());

        // Analyze only?
        if (DefragMode == DefragAnalyze)
        {
            uint64 UsedBytes  = 0;  // total bytes used, with cluster size considerations
            uint64 TotalBytes = 0;  // total bytes used
            uint64 SlackBytes = 0;  // wasted space due to slack
            uint32 Fragged    = 0;  // fragmented files

            printf ("    Analyzing ...");
            if (VerboseMode)
                printf ("\n");

            for (int i = 0; i < Drive->GetDBFileCount(); i++)
            {
                uint64 Used;
                uint64 Slack;
                FileInfo Info;

                Info = Drive->GetDBFile (i);

                // Compute total used disk space
                Used = ((Info.Size + Drive->GetClusterSize() - 1) / Drive->GetClusterSize()) * Drive->GetClusterSize();
                Slack = Used - Info.Size;

                UsedBytes += Used;
                SlackBytes += Slack;
                TotalBytes += Info.Size;

                if (VerboseMode)
                {
                    printf ("    %s%s, ", Drive->GetDBDir (Info.DirIndice).c_str(), Info.Name.c_str());

                    if (Info.Attributes.AccessDenied)
                        printf ("access was denied\n");
                    else
                    {
                        if (Info.Attributes.Unmovable == 1)
                            printf ("unmovable, ");

                        printf ("%I64u bytes, %I64u bytes on disk, %I64u bytes slack, %u fragments\n", 
                            Info.Size, Used, Slack, Info.Fragments.size());
                    }
                }

                if (Info.Fragments.size() > 1)
                    Fragged++;
            }

            if (!VerboseMode)
                printf ("\n");

            // TODO: Make it not look like ass
            printf ("\n");
            printf ("    Overall Analysis\n");
            printf ("    ----------------\n");
            printf ("    %u clusters\n", Drive->GetClusterCount ());
            printf ("    %u bytes per cluster\n", Drive->GetClusterSize());
            printf ("    %I64u total bytes on drive\n", (uint64)Drive->GetClusterCount() * (uint64)Drive->GetClusterSize());
            printf ("\n");
            printf ("    %u files\n", Drive->GetDBFileCount ());
            printf ("    %u contiguous files\n", Drive->GetDBFileCount () - Fragged);
            printf ("    %u fragmented files\n", Fragged);
            printf ("\n");
            printf ("    %I64u bytes\n", TotalBytes);
            printf ("    %I64u bytes on disk\n", UsedBytes);
            printf ("    %I64u bytes slack\n", SlackBytes);
        }

        // Fast defragment!
        if (DefragMode == DefragFast  ||  DefragMode == DefragExtensive)
        {
            uint32 i;
            uint64 FirstFreeLCN;
            char PrintName[80];
            int Width = 66;

            if (DefragMode == DefragFast)
                printf ("    Performing fast file defragmentation ...\n");
            else
            if (DefragMode == DefragExtensive)
                printf ("    Performing extensive file defragmentation\n");

            // Find first free LCN for speedier searches ...
            Drive->FindFreeRange (0, 1, FirstFreeLCN);

            for (i = 0; i < Drive->GetDBFileCount(); i++)
            {
                FileInfo Info;
                bool Result;
                uint64 TargetLCN;

                printf ("\r");

                Info = Drive->GetDBFile (i);

                FitName (PrintName, Drive->GetDBDir (Info.DirIndice).c_str(), Info.Name.c_str(), Width);
                printf ("    %6.2f%% %-66s", (float)i / (float)Drive->GetDBFileCount() * 100.0f, PrintName);

                // Can't defrag 0 byte files :)
                if (Info.Fragments.size() == 0)
                    continue;

                // If doing fast defrag, skip non-fragmented files
                if (Info.Fragments.size() == 1  &&  DefragMode == DefragFast)
                    continue;

                // Find a place that can fit the file
                Result = Drive->FindFreeRange (FirstFreeLCN, Info.Clusters, TargetLCN);

                // If we're doing an extensive defrag and the file is already defragmented
                // and if its new location would be after its current location, don't
                // move it.
                if (DefragMode == DefragExtensive  &&  Info.Fragments.size() == 1)
                {
                    if (TargetLCN > Info.Fragments[0].StartLCN)
                        continue;
                }

                // Otherwise, defrag0rize it!
                if (Result)
                {
                    bool Success = false;

                    if (Drive->MoveFileDumb (i, TargetLCN))
                        Success = true;
                    else
                    {   // hmm, look for another area to move it to
                        Result = Drive->FindFreeRange (TargetLCN + 1, Info.Clusters, TargetLCN);
                        if (Result)
                        {
                            if (Drive->MoveFileDumb (i, TargetLCN))
                                Success = true;
                            else
                            {   // Try updating the drive bitmap
                                if (Drive->GetBitmap ())
                                {
                                    Result = Drive->FindFreeRange (0, Info.Clusters, TargetLCN);
                                    if (Result)
                                    {
                                        if (Drive->MoveFileDumb (i, TargetLCN))
                                            Success = true;
                                    }
                                }
                            }
                        }
                    }

                    if (!Success)
                        printf ("\n        -> failed\n");

                    Drive->FindFreeRange (0, 1, FirstFreeLCN);
                }
            }

            printf ("\n");
        }
        printf ("Closing volume %s ...", Drives[d].c_str());
        delete Drive;
        printf ("\n");
    }
#endif

    return (0);
}
