#include "Defragment.h"


// Ahh yes I ripped this from my old Findupes project :)
// Fits a path name, composed of a path (i.e. "c:\blah\blah\cha\cha") and a filename ("stuff.txt")
// and fits it to a given length. If it has to truncate it will first truncate from the path,
// substituting in periods. So you might end up with something like:
// C:\Program Files\Micro...\Register.exe
int FitName (wchar_t *destination, const wchar_t *path, const wchar_t *filename, uint32 totalWidth)
{
	uint32 pathLen=0;
	uint32 fnLen=0;
	uint32 halfTotLen=0;
	uint32 len4fn=0;     /* number of chars remaining for filename after path is applied */
	uint32 len4path=0;   /* number of chars for path before filename is applied          */
	wchar_t fmtStrPath[20]=L"";
	wchar_t fmtStrFile[20]=L"";
	wchar_t fmtString[40]=L"";

    /*
	assert (destination != NULL);
	assert (path != NULL);
	assert (filename != NULL);
	assert (totalWidth != 0);
    */

	pathLen = wcslen(path);
	fnLen = wcslen(filename);
	if (!(totalWidth % 2))
		halfTotLen=totalWidth / 2;
	else
		halfTotLen=(totalWidth-1) / 2;  /* -1 because otherwise (halfTotLen*2) ==
(totalWidth+1) which wouldn't be good */

	/* determine how much width the path and filename each get */
	if ( (pathLen >= halfTotLen) && (fnLen < halfTotLen) )
	{
		len4fn = fnLen;
		len4path = (totalWidth - len4fn);
	}

	if ( (pathLen < halfTotLen) && (fnLen < halfTotLen) )
	{
		len4fn = fnLen;
		len4path = pathLen;
	}

	if ( (pathLen >= halfTotLen) && (fnLen >= halfTotLen) )
	{
		len4fn = halfTotLen;
		len4path = halfTotLen;
	}

	if ( (pathLen < halfTotLen) && (fnLen >= halfTotLen) )
	{
		len4path = pathLen;
		len4fn = (totalWidth - len4path);
	}
	/* 
		if halfTotLen was adjusted above to avoid a rounding error, give the 
		extra wchar_t to the filename 
	*/
	if (halfTotLen < (totalWidth/2)) len4path++; 

	if (pathLen > len4path)	swprintf (fmtStrPath, L"%%.%ds...\\", len4path-4);
	else
		swprintf (fmtStrPath, L"%%s");
	
	if (fnLen > len4fn)	swprintf (fmtStrFile, L"%%.%ds...", len4fn-3);
	else
		swprintf (fmtStrFile, L"%%s");

	wcscpy (fmtString, fmtStrPath);
	wcscat (fmtString, fmtStrFile);
	/*swprintf (fmtString, L"%s%s", fmtStrPath, fmtStrFile);*/
	swprintf (destination, fmtString, path,filename);
	
	return (1);	
}


Defragment::Defragment (wstring Name, DefragType DefragMethod)
{
    Method = DefragMethod;
    DoLimitLength = true;
    Error = false;
    Done = false;
    PleaseStop = false;
    PleasePause = false;
    DriveName = Name;
    StatusPercent = 0.0f;
    LastBMPUpdate = GetTickCount ();

    SetStatusString (L"Opening volume " + Name);
    if (!Volume.Open (Name))
    {
        SetStatusString (L"Error opening volume " + Name);
        Error = true;
        Done = true;
        StatusPercent = 100.0f;
    }

    return;
}


Defragment::~Defragment ()
{
    if (!IsDoneYet ())
    {
        Stop ();
        while (!IsDoneYet()  &&  !HasError())
        {
            SetStatusString (L"Waiting for thread to stop ...");
            Sleep (150);
        }
    }

    Volume.Close ();
    return;
}


void Defragment::SetStatusString (wstring NewStatus)
{
    Lock ();
    StatusString = NewStatus;
    Unlock ();

    return;
}


wstring Defragment::GetStatusString (void)
{
    wstring ReturnVal;

    Lock ();
    ReturnVal = StatusString;
    Unlock ();

    return (ReturnVal);
}


double Defragment::GetStatusPercent (void)
{
    return (StatusPercent);
}


bool Defragment::IsDoneYet (void)
{
    return (Done);
}


void Defragment::Start (void)
{
    uint32 i;
    uint64 FirstFreeLCN;
    uint64 TotalClusters;
    uint64 ClustersProgress;
    wchar_t PrintName[80];
    int Width = 70;

    if (Error)
        goto DoneDefrag;

    // First thing: build a file list.
    SetStatusString (L"Getting volume bitmap");
    if (!Volume.GetBitmap())
    {
        SetStatusString (L"Could not get volume " + DriveName + L" bitmap");
        Error = true;
        goto DoneDefrag;
    }

    LastBMPUpdate = GetTickCount ();

    if (PleaseStop)
        goto DoneDefrag;

    SetStatusString (L"Obtaining volume geometry");
    if (!Volume.ObtainInfo ())
    {
        SetStatusString (L"Could not obtain volume " + DriveName + L" geometry");
        Error = true;
        goto DoneDefrag;
    }

    if (PleaseStop)
        goto DoneDefrag;

    SetStatusString (L"Building file database for volume " + DriveName);
    if (!Volume.BuildFileList (PleaseStop, StatusPercent))
    {
        SetStatusString (L"Could not build file database for volume " + DriveName);
        Error = true;
        goto DoneDefrag;
    }

    if (PleaseStop)
        goto DoneDefrag;

    SetStatusString (L"Analyzing database for " + DriveName);
    TotalClusters = 0;
    for (i = 0; i < Volume.GetDBFileCount(); i++)
    {
        TotalClusters += Volume.GetDBFile(i).Clusters;
    }

    // Defragment!
    ClustersProgress = 0;

    // Find first free LCN for speedier searches ...
    Volume.FindFreeRange (0, 1, FirstFreeLCN);

    if (PleaseStop)
        goto DoneDefrag;

    // Analyze?
    if (Method == DefragAnalyze)
    {
        uint32 j;

        Report.RootPath = Volume.GetRootPath ();

        Report.FraggedFiles.clear ();
        Report.UnfraggedFiles.clear ();
        Report.UnmovableFiles.clear ();

        Report.FilesCount = Volume.GetDBFileCount () - Volume.GetDBDirCount ();
        Report.DirsCount = Volume.GetDBDirCount ();
        Report.DiskSizeBytes = Volume.GetVolumeInfo().TotalBytes;

        Report.FilesSizeClusters = 0;
        Report.FilesSlackBytes = 0;
        Report.FilesSizeBytes = 0;
        Report.FilesFragments = 0;

        for (j = 0; j < Volume.GetDBFileCount(); j++)
        {
            FileInfo Info;

            Info = Volume.GetDBFile (j);

            Report.FilesFragments += max (1, Info.Fragments.size()); // add 1 fragment even for 0 bytes/0 cluster files

            if (Info.Attributes.Process == 0)
                continue;

            SetStatusString (Volume.GetDBDir (Info.DirIndice) + Info.Name);

            Report.FilesSizeClusters += Info.Clusters;
            Report.FilesSizeBytes += Info.Size;

            if (Info.Attributes.Unmovable == 1)
                Report.UnmovableFiles.push_back (j);

            if (Info.Fragments.size() > 1)
                Report.FraggedFiles.push_back (j);
            else
                Report.UnfraggedFiles.push_back (j);

            StatusPercent = ((double)j / (double)Report.FilesCount) * 100.0f;
        }

        Report.FilesSizeOnDisk = Report.FilesSizeClusters * (uint64)Volume.GetVolumeInfo().ClusterSize;
        Report.FilesSlackBytes = Report.FilesSizeOnDisk - Report.FilesSizeBytes;
        Report.AverageFragments = (double)Report.FilesFragments / (double)Report.FilesCount;
        Report.PercentFragged = 100.0f * ((double)(signed)Report.FraggedFiles.size() / (double)(signed)Report.FilesCount);

        uint64 Percent;
        Percent = (10000 * Report.FilesSlackBytes) / Report.FilesSizeOnDisk;
        Report.PercentSlack = (double)(signed)Percent / 100.0f;
    }
    else
    // Go through all the files and ... defragment them!
    for (i = 0; i < Volume.GetDBFileCount(); i++)
    {
        FileInfo Info;
        bool Result;
        uint64 TargetLCN;
        uint64 PreviousClusters;

        // What? They want us to pause? Oh ok.
        if (PleasePause)
        {
            SetStatusString (L"Paused");
            PleasePause = false;

            while (PleasePause == false)
            {
                Sleep (50);
            }

            PleasePause = false;
        }

        if (PleaseStop)
        {
            SetStatusString (L"Stopping");
            break;
        }

        // 
        Info = Volume.GetDBFile (i);

        PreviousClusters = ClustersProgress;
        ClustersProgress += Info.Clusters;

        if (Info.Attributes.Process == 0)
            continue;

        if (!DoLimitLength)
            SetStatusString (Volume.GetDBDir (Info.DirIndice) + Info.Name);
        else
        {
            FitName (PrintName, Volume.GetDBDir (Info.DirIndice).c_str(), Info.Name.c_str(), Width);
            SetStatusString (PrintName);
        }

        // Calculate percentage complete
        StatusPercent = 100.0f * double((double)PreviousClusters / (double)TotalClusters);

        // Can't defrag directories yet
        if (Info.Attributes.Directory == 1)
            continue;

        // Can't defrag 0 byte files :)
        if (Info.Fragments.size() == 0)
            continue;

        // If doing fast defrag, skip non-fragmented files
        // Note: This assumes that the extents stored in Info.Fragments
        //       are consolidated. I.e. we assume it is NOT the case that
        //       two extents account for a sequential range of (non-
        //       fragmented) clusters.
        if (Info.Fragments.size() == 1  &&  Method == DefragFast)
            continue;

        // Otherwise, defrag0rize it!
        int Retry = 3;  // retry a few times
        while (Retry > 0)
        {
            // Find a place that can fit the file
            Result = Volume.FindFreeRange (FirstFreeLCN, Info.Clusters, TargetLCN);

            // If yes, try moving it
            if (Result)
            {
                // If we're doing an extensive defrag and the file is already defragmented
                // and if its new location would be after its current location, don't
                // move it.
                if (Method == DefragExtensive  &&  Info.Fragments.size() == 1  &&
                    TargetLCN > Info.Fragments[0].StartLCN)
                {
                    Retry = 1;
                }
                else
                {
                    if (Volume.MoveFileDumb (i, TargetLCN))
                    {
                        Retry = 1; // yay, all done with this file.
                        Volume.FindFreeRange (0, 1, FirstFreeLCN);
                    }
                }
            }
 
            // New: Only update bitmap if it's older than 15 seconds
            if ((GetTickCount() - LastBMPUpdate) < 15000)
                Retry = 1;
            else
            if (!Result  ||  Retry != 1)
            {   // hmm. Wait for a moment, then update the drive bitmap
                //SetStatusString (L"(Reobtaining volume " + DriveName + L" bitmap)");

                if (!DoLimitLength)
                {
                    SetStatusString (GetStatusString() + wstring (L" ."));
                }

                if (Volume.GetBitmap ())
                {
                    LastBMPUpdate = GetTickCount ();

                    if (!DoLimitLength)
                        SetStatusString (Volume.GetDBDir (Info.DirIndice) + Info.Name);
                    else
                        SetStatusString (PrintName);

                    Volume.FindFreeRange (0, 1, FirstFreeLCN);
                }
                else
                {
                    SetStatusString (L"Could not re-obtain volume " + DriveName + L" bitmap");
                    Error = true;
                }
            }

            Retry--;
        }

        if (Error == true)
            break;
    }

DoneDefrag:
    wstring OldStatus;

    OldStatus = GetStatusString ();
    StatusPercent = 99.999999f;
    SetStatusString (L"Closing volume " + DriveName);
    Volume.Close ();
    StatusPercent = 100.0f;

    // If there was an error then the wstring has already been set
    if (Error)
        SetStatusString (OldStatus);
    else
    if (PleaseStop)
        SetStatusString (L"Volume " + DriveName + L" defragmentation was stopped");
    else
        SetStatusString (L"Finished defragmenting " + DriveName);

    Done = true;

    return;
}


void Defragment::TogglePause (void)
{
    Lock ();
    SetStatusString (L"Pausing ...");
    PleasePause = true;
    Unlock ();

    return;
}


void Defragment::Stop (void)
{
    Lock ();
    SetStatusString (L"Stopping ...");
    PleaseStop = true;
    Unlock ();

    return;
}


bool Defragment::HasError (void)
{
    return (Error);
}

