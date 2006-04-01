/*****************************************************************************

  DriveVolume

  Class for opening a volume and getting information on it and defragging it
  and stuff.

*****************************************************************************/


#ifndef DRIVEVOLUME_H
#define DRIVEVOLUME_H


#include "Unfrag.h"
#include <vector>
#include <string>


using namespace std;

#pragma pack (push, 1)
typedef struct
{
    unsigned int Archive    : 1;
    unsigned int Compressed : 1;
    unsigned int Directory  : 1;
    unsigned int Encrypted  : 1;
    unsigned int Hidden     : 1;
    unsigned int Normal     : 1;
    unsigned int Offline    : 1;
    unsigned int ReadOnly   : 1;
    unsigned int Reparse    : 1;
    unsigned int Sparse     : 1;
    unsigned int System     : 1;
    unsigned int Temporary  : 1;

    // For defragmenting purposes and other information
    unsigned int Unmovable    : 1;  // can we even touch it?
    unsigned int Process      : 1;  // should we process it?
    unsigned int AccessDenied : 1;  // could we not open it?
} FileAttr;


typedef struct
{
    uint64 StartLCN;
    uint64 Length;
} Extent;


typedef struct
{
    wstring   Name;
    uint32   DirIndice;   // indice into directory list
    uint64   Size;
    uint64   Clusters;
    FileAttr Attributes;
    vector<Extent> Fragments;
} FileInfo;


typedef vector<FileInfo> FileList;


typedef struct
{
    wstring Name;
    wstring Serial;
    DWORD  MaxNameLen;
    wstring FileSystem;
    uint64 ClusterCount;
    uint32 ClusterSize;
    uint64 TotalBytes;
    uint64 FreeBytes;
} VolumeInfo;
#pragma pack (pop)


// Callback function for Scan()
// NOTE: Do *NOT* close the HANDLE given to you. It is provided for convenience,
// and is closed automatically by the function that calls you!
typedef bool (*ScanCallback) (FileInfo &Info, HANDLE &FileHandle, void *UserData);


extern bool BuildDBCallback (FileInfo &Info, HANDLE &FileHandle, void *UserData);


class DriveVolume
{
public:
    DriveVolume ();
    ~DriveVolume ();

    bool Open            (wstring Name);  // opens the volume
    void Close           (void);
    bool ObtainInfo      (void);         // retrieves drive geometry
    bool GetBitmap       (void);         // gets drive bitmap

    // builds list of files on drive
    // if QuitMonitor ever becomes true (ie from a separate thread) it will clean up and return
    bool BuildFileList   (bool &QuitMonitor, double &Progress); 
        
    // Functions for accessing the volume bitmap
    bool IsClusterUsed   (uint64 Cluster);
    void SetClusterUsed  (uint64 Cluster, bool Used);

    DISK_GEOMETRY GetGeometry (void) { return (Geometry); }
    VolumeInfo GetVolumeInfo (void) { return (VolInfo); }

    wstring GetRootPath (void) { return (RootPath); }

    // Scans drive starting from the root dir and calls a user defined function
    // for each file/directory encountered. void* UserData is passed to this
    // function so you can give it some good ol' fashioned context.
    bool Scan (ScanCallback Callback, void *UserData);

    // Retrieve a directory string from the file database
    wstring   &GetDBDir       (uint32 Indice);
    uint32    GetDBDirCount  (void);
    // Retrieve file strings/info from the file database
    FileInfo &GetDBFile      (uint32 Indice);
    uint32    GetDBFileCount (void);
    // Kill it!
    uint32    RemoveDBFile   (uint32 Indice);

    // This is for actual defragmenting! It will automatically update the drive bitmap.
    // Will not move any other files out of the way.
    // Failure (return value of false) means that something is in the way.
    bool MoveFileDumb (uint32 FileIndice, uint64 NewLCN);

    // Look for a range of sequential free clusters
    // Returns true if one could be found, false if not
    bool FindFreeRange (uint64 StartLCN, uint64 ReqLength, uint64 &LCNResult);

private:
    friend bool BuildDBCallback (FileInfo &Info, HANDLE &FileHandle, void *UserData);

    // DirPrefix should be in the form "drive:\\path\\" ie, C:\CRAP\     .
    bool ScanDirectory (wstring DirPrefix, ScanCallback Callback, void *UserData);

    // given a file's attributes, should it be processed or not?
    bool ShouldProcess (FileAttr Attr);

    bool GetClusterInfo (FileInfo &Info, HANDLE &HandleResult);

    VolumeInfo               VolInfo;
    FileList                 Files;
    vector<wstring>           Directories; // Directories[Files[x].DirIndice]
    wstring                   RootPath;    // ie, C:\    .
    HANDLE                   Handle;
    DISK_GEOMETRY            Geometry;
    uint32                  *BitmapDetail;
};


#endif // DRIVEVOLUME_H
