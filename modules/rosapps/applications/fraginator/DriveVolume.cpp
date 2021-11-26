#include "DriveVolume.h"


DriveVolume::DriveVolume ()
{
    Handle = INVALID_HANDLE_VALUE;
    BitmapDetail = NULL;
    return;
}


DriveVolume::~DriveVolume ()
{
    Close ();
    Directories.clear ();
    Files.clear ();
    return;
}


void DriveVolume::Close (void)
{
    if (Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle (Handle);
        Handle = INVALID_HANDLE_VALUE;
    }

    if (BitmapDetail != NULL)
    {
        free (BitmapDetail);
        BitmapDetail = NULL;
    }

    return;
}


// "Name" should be the drive letter followed by a colon. ie, "c:"
// It's a string to allow for further expansion (ie, defragging over the network?)
// or some other baloney reason
bool DriveVolume::Open (wstring Name)
{
    wchar_t FileName[100];
    bool ReturnVal;

    swprintf (FileName, L"\\\\.\\%s", Name.c_str());
    RootPath = Name.c_str();
    RootPath += L"\\";

    Handle = CreateFileW
    (
        FileName,
        MAXIMUM_ALLOWED,                          // access
        FILE_SHARE_READ | FILE_SHARE_WRITE,       // share type
        NULL,                                     // security descriptor
        OPEN_EXISTING,                            // open type
        0,                                     // attributes (none)
        NULL                                      // template
    );

    if (Handle == INVALID_HANDLE_VALUE)
        ReturnVal = false;
    else
    {
        wchar_t  VolName[64];
        DWORD VolSN;
        DWORD VolMaxFileLen;
        DWORD FSFlags;
        wchar_t  FSName[64];
        BOOL  Result;

        ReturnVal = true;
        Result = GetVolumeInformationW
        (
            RootPath.c_str(),
            VolName,
            sizeof (VolName),
            &VolSN,
            &VolMaxFileLen,
            &FSFlags,
            FSName,
            sizeof (FSName)
        );

        if (Result)
        {
            wchar_t SerialText[10];

            VolInfo.FileSystem = FSName;
            VolInfo.MaxNameLen = VolMaxFileLen;
            VolInfo.Name       = VolName;

            swprintf (SerialText, L"%x-%x", (VolSN & 0xffff0000) >> 16,
                VolSN & 0x0000ffff);

            _wcsupr (SerialText);
            VolInfo.Serial     = SerialText;
        }
        else
        {
            VolInfo.FileSystem = L"(Unknown)";
            VolInfo.MaxNameLen = 255;
            VolInfo.Name       = L"(Unknown)";
            VolInfo.Serial     = L"(Unknown)";
        }
    }

    return (ReturnVal);
}


bool DriveVolume::ObtainInfo (void)
{
    BOOL Result;
    DWORD BytesGot;
    uint64 nan;

    BytesGot = 0;
    ZeroMemory (&Geometry, sizeof (Geometry));
    Result = DeviceIoControl
    (
        Handle,
        IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL,
        0,
        &Geometry,
        sizeof (Geometry),
        &BytesGot,
        NULL
    );

    // Call failed? Aww :(
    if (!Result)
        return (false);

    // Get cluster size
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD FreeClusters;
    DWORD TotalClusters;

    Result = GetDiskFreeSpaceW
    (
        RootPath.c_str(),
        &SectorsPerCluster,
        &BytesPerSector,
        &FreeClusters,
        &TotalClusters
    );

    // Failed? Weird.
    if (!Result)
        return (false);

    VolInfo.ClusterSize = SectorsPerCluster * BytesPerSector;

    Result = GetDiskFreeSpaceExW
    (
        RootPath.c_str(),
        (PULARGE_INTEGER)&nan,
        (PULARGE_INTEGER)&VolInfo.TotalBytes,
        (PULARGE_INTEGER)&VolInfo.FreeBytes
    );

    return (true);
}


// Get bitmap, several clusters at a time ...
#define CLUSTERS 4096
bool DriveVolume::GetBitmap (void)
{
    STARTING_LCN_INPUT_BUFFER StartingLCN;
    VOLUME_BITMAP_BUFFER *Bitmap = NULL;
    uint32 BitmapSize;
    DWORD BytesReturned;
    BOOL Result;

    StartingLCN.StartingLcn.QuadPart = 0;

    // Allocate buffer
    // Call FSCTL_GET_VOLUME_BITMAP once with a very small buffer
    // This will leave the total number of clusters in Bitmap->BitmapSize and we can
    // then correctly allocate based off that
    // I suppose this won't work if your drive has only 40 clusters on it or so :)
    BitmapSize = sizeof (VOLUME_BITMAP_BUFFER) + 4;
    Bitmap = (VOLUME_BITMAP_BUFFER *) malloc (BitmapSize);

    Result = DeviceIoControl
    (
        Handle,
        FSCTL_GET_VOLUME_BITMAP,
        &StartingLCN,
        sizeof (StartingLCN),
        Bitmap,
        BitmapSize,
        &BytesReturned,
        NULL
    );

    // Bad result?
    if (Result == FALSE  &&  GetLastError () != ERROR_MORE_DATA)
    {
        //wprintf ("\nDeviceIoControl returned false, GetLastError() was not ERROR_MORE_DATA\n");
        free (Bitmap);
        return (false);
    }

    // Otherwise, we're good
    BitmapSize = sizeof (VOLUME_BITMAP_BUFFER) + (Bitmap->BitmapSize.QuadPart / 8) + 1;
    Bitmap = (VOLUME_BITMAP_BUFFER *) realloc (Bitmap, BitmapSize);
    Result = DeviceIoControl
    (
        Handle,
        FSCTL_GET_VOLUME_BITMAP,
        &StartingLCN,
        sizeof (StartingLCN),
        Bitmap,
        BitmapSize,
        &BytesReturned,
        NULL
    );

    //DWORD LastError = GetLastError ();

    if (Result == FALSE)
    {
        wprintf (L"\nCouldn't properly read volume bitmap\n");
        free (Bitmap);
        return (false);
    }

    // Convert to a L'quick use' bitmap
    //const int BitShift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

    VolInfo.ClusterCount = Bitmap->BitmapSize.QuadPart;

    if (BitmapDetail != NULL)
        free (BitmapDetail);

    BitmapDetail = (uint32 *) malloc (sizeof(uint32) * (1 + (VolInfo.ClusterCount / 32)));
    memcpy (BitmapDetail, Bitmap->Buffer, sizeof(uint32) * (1 + (VolInfo.ClusterCount / 32)));

    /*
    BitmapDetail = (Cluster *) malloc (VolInfo.ClusterCount * sizeof (Cluster));
    for (uint64 i = 0; i < VolInfo.ClusterCount; i++)
    {
        if (Bitmap->Buffer[i / 8] & BitShift[i % 8])
            BitmapDetail[i].Allocated = true;
        else
            BitmapDetail[i].Allocated = false;
    }
    */

    free (Bitmap);
    return (true);
}


bool DriveVolume::IsClusterUsed (uint64 Cluster)
{
    return ((BitmapDetail[Cluster / 32] & (1 << (Cluster % 32))) ? true : false);
    //return (BitmapDetail[Cluster].Allocated);
}


void DriveVolume::SetClusterUsed (uint64 Cluster, bool Used)
{
    if (Used)
        BitmapDetail[Cluster / 32] |= (1 << (Cluster % 32));
    else
        BitmapDetail[Cluster / 32] &= ~(1 << (Cluster % 32));

    return;
}


typedef struct
{
    DriveVolume *Volume;
    double       *Percent;
    bool        *QuitMonitor;
    uint64       ClusterCount;
    uint64       ClusterProgress;
} BuildDBInfo;


bool DriveVolume::BuildFileList (bool &QuitMonitor, double &Percent)
{
    BuildDBInfo Info;

    Files.clear ();
    Directories.clear ();
    Directories.push_back (RootPath);

    Info.Volume = this;
    Info.QuitMonitor = &QuitMonitor;
    Info.ClusterCount = (GetVolumeInfo().TotalBytes - GetVolumeInfo().FreeBytes) / (uint64)GetVolumeInfo().ClusterSize;
    Info.ClusterProgress = 0;
    Info.Percent = &Percent;

    ScanDirectory (RootPath, BuildDBCallback, &Info);

    if (QuitMonitor == true)
    {
        Directories.resize (0);
        Files.resize (0);
    }

    return (true);
}


// UserData = pointer to BuildDBInfo instance
bool BuildDBCallback (FileInfo &Info, HANDLE &FileHandle, void *UserData)
{
    BuildDBInfo *DBInfo = (BuildDBInfo *) UserData;
    DriveVolume *Vol = DBInfo->Volume;

    Vol->Files.push_back (Info);

    if (*(DBInfo->QuitMonitor) == true)
        return (false);

    DBInfo->ClusterProgress += (uint64)Info.Clusters;
    *(DBInfo->Percent) =
        ((double)DBInfo->ClusterProgress / (double)DBInfo->ClusterCount) * 100.0f;

    return (true);
}


wstring &DriveVolume::GetDBDir (uint32 Indice)
{
    return (Directories[Indice]);
}


uint32 DriveVolume::GetDBDirCount (void)
{
    return (Directories.size());
}


FileInfo &DriveVolume::GetDBFile (uint32 Indice)
{
    return (Files[Indice]);
}


uint32 DriveVolume::GetDBFileCount (void)
{
    return (Files.size());
}


uint32 DriveVolume::RemoveDBFile (uint32 Indice)
{
    vector<FileInfo>::iterator it;

    it = Files.begin() + Indice;
    Files.erase (it);
    return (GetDBFileCount());
}


bool DriveVolume::ScanDirectory (wstring DirPrefix, ScanCallback Callback, void *UserData)
{
    WIN32_FIND_DATAW FindData;
    HANDLE          FindHandle;
    wstring          SearchString;
    uint32          DirIndice;

    DirIndice = Directories.size() - 1;

    SearchString = DirPrefix;
    SearchString += L"*.*";
    ZeroMemory (&FindData, sizeof (FindData));
    FindHandle = FindFirstFileW (SearchString.c_str(), &FindData);

    if (FindHandle == INVALID_HANDLE_VALUE)
        return (false);

    do
    {
        FileInfo Info;
        HANDLE   Handle;
        bool     CallbackResult;

        Handle = INVALID_HANDLE_VALUE;

        // First copy over the easy stuff.
        Info.Name = FindData.cFileName;

        // DonLL't ever include '.L' and '..'
        if (Info.Name == L"."  ||  Info.Name == L"..")
            continue;

        //Info.FullName = DirPrefix + Info.Name;
        Info.Size      = (uint64)FindData.nFileSizeLow + ((uint64)FindData.nFileSizeHigh << (uint64)32);
        Info.DirIndice = DirIndice;

        Info.Attributes.Archive    = (FindData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)       ? 1 : 0;
        Info.Attributes.Compressed = (FindData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)    ? 1 : 0;
        Info.Attributes.Directory  = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)     ? 1 : 0;
        Info.Attributes.Encrypted  = (FindData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)     ? 1 : 0;
        Info.Attributes.Hidden     = (FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)        ? 1 : 0;
        Info.Attributes.Normal     = (FindData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)        ? 1 : 0;
        Info.Attributes.Offline    = (FindData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)       ? 1 : 0;
        Info.Attributes.ReadOnly   = (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)      ? 1 : 0;
        Info.Attributes.Reparse    = (FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? 1 : 0;
        Info.Attributes.Sparse     = (FindData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)   ? 1 : 0;
        Info.Attributes.System     = (FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)        ? 1 : 0;
        Info.Attributes.Temporary  = (FindData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)     ? 1 : 0;
        Info.Attributes.AccessDenied = 0;
        Info.Attributes.Unmovable    = 0;
        Info.Attributes.Process      = 1;

        Info.Clusters = 0;
        if (GetClusterInfo (Info, Handle))
        {
            uint64 TotalClusters = 0;

            for (size_t i = 0; i < Info.Fragments.size(); i++)
            {
                TotalClusters += Info.Fragments[i].Length;
            }

            Info.Clusters = TotalClusters;
        }
        else
        {
            Info.Attributes.Unmovable = 1;
            Info.Attributes.Process = 0;
        }

        if (Info.Attributes.Process == 1)
            Info.Attributes.Process = ShouldProcess (Info.Attributes) ? 1 : 0;

        // Run the user-defined callback function
        CallbackResult = Callback (Info, Handle, UserData);

        if (Handle != INVALID_HANDLE_VALUE)
            CloseHandle (Handle);

        if (!CallbackResult)
            break;

        // If directory, perform recursion
        if (Info.Attributes.Directory == 1)
        {
            wstring Dir;

            Dir = GetDBDir (Info.DirIndice);
            Dir += Info.Name;
            Dir += L"\\";

            Directories.push_back (Dir);
            ScanDirectory (Dir, Callback, UserData);
        }

    } while (FindNextFileW (FindHandle, &FindData) == TRUE);

    FindClose (FindHandle);
    return (false);
}


bool DriveVolume::ShouldProcess (FileAttr Attr)
{
    if (Attr.Offline == 1  ||  Attr.Reparse == 1  ||  Attr.Temporary == 1)
    {
        return (false);
    }

    return (true);
}


// Gets info on a file and returns a valid handle for read/write access
// Name, FullName, Clusters, Attributes, and Size should already be filled out.
// This function fills in the Fragments vector
bool DriveVolume::GetClusterInfo (FileInfo &Info, HANDLE &HandleResult)
{
    BOOL     Result;
    HANDLE   Handle;
    wstring   FullName;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    Info.Fragments.resize (0);

    /*
    if (Info.Attributes.Directory == 1)
        return (false);
    */

    FullName = GetDBDir (Info.DirIndice) + Info.Name;

    Handle = CreateFileW
    (
        FullName.c_str(),
        0, //GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        (Info.Attributes.Directory == 1) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        NULL
    );

    if (Handle == INVALID_HANDLE_VALUE)
    {
	    LPVOID lpMsgBuf;

	    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					    NULL, GetLastError(),
					    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					    (LPTSTR) &lpMsgBuf, 0, NULL );


        Info.Attributes.AccessDenied = 1;
	    LocalFree( lpMsgBuf );
        return (false);
    }

    ZeroMemory (&FileInfo, sizeof (FileInfo));
    Result = GetFileInformationByHandle (Handle, &FileInfo);

    if (Result == FALSE)
    {
        Info.Attributes.AccessDenied = 1;
        wprintf (L"GetFileInformationByHandle ('%s%s') failed\n", GetDBDir (Info.DirIndice).c_str(),
            Info.Name.c_str());

        CloseHandle (Handle);
        return (false);
    }

    // Get cluster allocation information
    STARTING_VCN_INPUT_BUFFER  StartingVCN;
    RETRIEVAL_POINTERS_BUFFER *Retrieval;
    uint64                     RetSize;
    uint64                     Extents;
    DWORD                      BytesReturned;

    // Grab info one extent at a time, until it's done grabbing all the extent data
    // Yeah, well it doesn't give us a way to ask L"how many extents?" that I know of ...
    // btw, the Extents variable tends to only reflect memory usage, so when we have
    // all the extents we look at the structure Win32 gives us for the REAL count!
    Extents = 10;
    Retrieval = NULL;
    RetSize = 0;
    StartingVCN.StartingVcn.QuadPart = 0;

    do
    {
        Extents *= 2;
        RetSize = sizeof (RETRIEVAL_POINTERS_BUFFER) + ((Extents - 1) * sizeof (LARGE_INTEGER) * 2);

        if (Retrieval != NULL)
            Retrieval = (RETRIEVAL_POINTERS_BUFFER *) realloc (Retrieval, RetSize);
        else
            Retrieval = (RETRIEVAL_POINTERS_BUFFER *) malloc (RetSize);

        Result = DeviceIoControl
        (
            Handle,
            FSCTL_GET_RETRIEVAL_POINTERS,
            &StartingVCN,
            sizeof (StartingVCN),
            Retrieval,
            RetSize,
            &BytesReturned,
            NULL
        );

        if (Result == FALSE)
        {
            if (GetLastError() != ERROR_MORE_DATA)
            {
                Info.Clusters = 0;
                Info.Attributes.AccessDenied = 1;
                Info.Attributes.Process = 0;
                Info.Fragments.clear ();
                CloseHandle (Handle);
                free (Retrieval);

                return (false);
            }

            Extents++;
        }
    } while (Result == FALSE);

    // Readjust extents, as it only reflects how much memory was allocated and may not
    // be accurate
    Extents = Retrieval->ExtentCount;

    // Ok, we have the info. Now translate it. hrmrmr

    Info.Fragments.clear ();
    for (uint64 i = 0; i < Extents; i++)
    {
        Extent Add;

        Add.StartLCN = Retrieval->Extents[i].Lcn.QuadPart;
        if (i != 0)
            Add.Length = Retrieval->Extents[i].NextVcn.QuadPart - Retrieval->Extents[i - 1].NextVcn.QuadPart;
        else
            Add.Length = Retrieval->Extents[i].NextVcn.QuadPart - Retrieval->StartingVcn.QuadPart;

        Info.Fragments.push_back (Add);
    }

    free (Retrieval);
    HandleResult = Handle;
    return (true);
}


bool DriveVolume::FindFreeRange (uint64 StartLCN, uint64 ReqLength, uint64 &LCNResult)
{
    uint64 Max;
    uint64 i;
    uint64 j;

    // Make sure we don't spill over our array
    Max = VolInfo.ClusterCount - ReqLength;

    for (i = StartLCN; i < Max; i++)
    {
        bool Found = true;

        // First check the first cluster
        if (IsClusterUsed (i))
            Found = false;
        else
        // THen check the last cluster
        if (IsClusterUsed (i + ReqLength - 1))
            Found = false;
        else
        // Check the whole darn range.
        for (j = (i + 1); j < (i + ReqLength - 2); j++)
        {
            if (IsClusterUsed (j) == true)
            {
                Found = false;
                break;
            }
        }

        if (!Found)
            continue;
        else
        {
            LCNResult = i;
            return (true);
        }
    }

    return (false);
}


// btw we have to move each fragment of the file, as per the Win32 API
bool DriveVolume::MoveFileDumb (uint32 FileIndice, uint64 NewLCN)
{
    bool ReturnVal = false;
    FileInfo Info;
    HANDLE FileHandle;
    wstring FullName;
    MOVE_FILE_DATA MoveData;
    uint64 CurrentLCN;
    uint64 CurrentVCN;

    // Set up variables
    Info = GetDBFile (FileIndice);
    FullName = GetDBDir (Info.DirIndice);
    FullName += Info.Name;
    CurrentLCN = NewLCN;
    CurrentVCN = 0;

    /*
    if (Info.Attributes.Directory == 1)
    {
        //
    }
    */

    // Open file
    FileHandle = CreateFileW
    (
        FullName.c_str (),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        (Info.Attributes.Directory == 1) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        NULL
    );

    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        //
	    LPVOID lpMsgBuf;

	    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					    NULL, GetLastError(),
					    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					    (LPTSTR) &lpMsgBuf, 0, NULL );


	    LocalFree (lpMsgBuf);
        //

        ReturnVal = false;
    }
    else
    {
        ReturnVal = true; // innocent until proven guilty ...

        for (uint32 i = 0; i < Info.Fragments.size(); i++)
        {
            BOOL Result;
            DWORD BytesReturned;

            //wprintf (L"%3u", i);

            MoveData.ClusterCount = Info.Fragments[i].Length;
            MoveData.StartingLcn.QuadPart = CurrentLCN;
            MoveData.StartingVcn.QuadPart = CurrentVCN;

            MoveData.FileHandle = FileHandle;

            /*
            wprintf (L"\n");
            wprintf (L"StartLCN: %I64u\n", MoveData.StartingLcn.QuadPart);
            wprintf (L"StartVCN: %I64u\n", MoveData.StartingVcn.QuadPart);
            wprintf (L"Clusters: %u (%I64u-%I64u --> %I64u-%I64u)\n", MoveData.ClusterCount,
                Info.Fragments[i].StartLCN,
                Info.Fragments[i].StartLCN + MoveData.ClusterCount,
                MoveData.StartingLcn.QuadPart,
                MoveData.StartingLcn.QuadPart + MoveData.ClusterCount - 1);
            wprintf (L"\n");
            */

            Result = DeviceIoControl
            (
                Handle,
                FSCTL_MOVE_FILE,
                &MoveData,
                sizeof (MoveData),
                NULL,
                0,
                &BytesReturned,
                NULL
            );

            //wprintf (L"\b\b\b");

            if (Result == FALSE)
            {
                //
	            LPVOID lpMsgBuf;

	            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					            NULL, GetLastError(),
					            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					            (LPTSTR) &lpMsgBuf, 0, NULL );


	            LocalFree( lpMsgBuf );
                //

                ReturnVal = false;
                goto FinishUp;  // yeah, bite me
            }

            // Ok good. Now update our drive bitmap and file infos.
            uint64 j;
            for (j = 0;
                 j < Info.Fragments[i].Length;
                 j++)
            {
                SetClusterUsed (Info.Fragments[i].StartLCN + j, false);
                SetClusterUsed (CurrentLCN + j, true);
                //BitmapDetail[Info.Fragments[i].StartLCN + j].Allocated = false;
                //BitmapDetail[CurrentLCN + j].Allocated = true;
            }

            CurrentLCN += Info.Fragments[i].Length;
            CurrentVCN += Info.Fragments[i].Length;
        }

        // Update file info either way
    FinishUp:
        CloseHandle (FileHandle);
        FileHandle = INVALID_HANDLE_VALUE;
        GetClusterInfo (Files[FileIndice], FileHandle);
        CloseHandle (FileHandle);
    }

    return (ReturnVal);
}


