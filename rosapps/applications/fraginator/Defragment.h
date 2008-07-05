/*****************************************************************************

  Defragment

*****************************************************************************/


#ifndef DEFRAGMENT_H
#define DEFRAGMENT_H


#include "Unfrag.h"
#include "DriveVolume.h"
#include "Mutex.h"


extern int FitName (wchar_t *destination, const wchar_t *path, const wchar_t *filename, uint32 totalWidth);


typedef struct DefragReport
{
    wstring    RootPath;
    uint64    DiskSizeBytes;
    uint64    DirsCount;
    uint64    FilesCount;
    uint64    FilesSizeBytes;
    uint64    FilesSizeOnDisk;
    uint64    FilesSizeClusters;
    uint64    FilesSlackBytes;
    uint32    FilesFragments;
    double    AverageFragments;  // = FilesFragments / FilesCount
    double    PercentFragged;
    double    PercentSlack;

    vector<uint32> FraggedFiles;
    vector<uint32> UnfraggedFiles;
    vector<uint32> UnmovableFiles;
} DefragReport;


class Defragment
{
public:
    Defragment (wstring Name, DefragType DefragMethod);
    ~Defragment ();

    // Commands
    void Start       (void);
    void TogglePause (void);
    void Stop        (void);

    // Info
    bool          IsDoneYet (void);
    bool          HasError (void);
    wstring        GetStatusString  (void);
    double         GetStatusPercent (void);
    DefragType    GetDefragType    (void)  { return (Method); }
    DefragReport &GetDefragReport  (void)  { return (Report); }
    DriveVolume  &GetVolume        (void)  { return (Volume); }

    // Mutex
    void Lock (void)   { DefragMutex.Lock ();   }
    void Unlock (void) { DefragMutex.Unlock (); }

    // Limit length of status string to 70 chars?
    bool GetDoLimitLength (void)   { return (DoLimitLength); }
    void SetDoLimitLength (bool L) { DoLimitLength = L; }

private:
    void FastDefrag (void);
    void ExtensiveDefrag (void);
    void SetStatusString (wstring NewStatus);

    DWORD        LastBMPUpdate; // Last time volume bitmap was updated
    DefragReport Report;
    bool         DoLimitLength;
    DefragType   Method;
    wstring       DriveName;
    DriveVolume  Volume;
    wstring       StatusString;
    wstring       ErrorString;
    double        StatusPercent;
    Mutex        DefragMutex;
    bool         Error;
    bool         Done;
    bool         PleaseStop;
    bool         PleasePause;
    DefragType   DefragMethod;
};


#endif // DEFRAGMENT_H
