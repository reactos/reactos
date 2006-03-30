#include "ReportDialog.h"
#include "unfrag.h"
#include "Fraginator.h"
#include "DriveVolume.h"
#include "Defragment.h"
#include "resource.h"


void SetReportInfo (HWND Dlg, DefragReport &Report, uint32 BytesDivisor, char *BytesUnits, bool Fractional)
{
    char Text[1000];
    char Text2[1000];
    char Text3[1000];

    memset (Text, 0, sizeof (Text));

    // Volume name
    SetDlgItemText (Dlg, IDC_DRIVELETTER, Report.RootPath.c_str());

    // Volume label
    SetDlgItemText (Dlg, IDC_VOLUMELABEL, Defrag->GetVolume().GetVolumeInfo().Name.c_str());

    // Volume Serial
    SetDlgItemText (Dlg, IDC_VOLUMESERIAL, Defrag->GetVolume().GetVolumeInfo().Serial.c_str());

    // File System
    SetDlgItemText (Dlg, IDC_FILESYSTEM, Defrag->GetVolume().GetVolumeInfo().FileSystem.c_str());

    // DiskSizeBytes
    if (Fractional)
    {
        sprintf (Text, "%.2f %s", (double)(signed)(Report.DiskSizeBytes / 
            (BytesDivisor / 1024)) / 1024.0, BytesUnits);
    }
    else
    {
        AddCommas (Text, Report.DiskSizeBytes / BytesDivisor);
        strcat (Text, " ");
        strcat (Text, BytesUnits);
    }

    SetDlgItemText (Dlg, IDC_DISKSIZEBYTES, Text);

    // DiskFreeBytes
    if (Fractional)
    {
        sprintf (Text, "%.2f %s", (double)(signed)(Defrag->GetVolume().GetVolumeInfo().FreeBytes / 
            (BytesDivisor / 1024)) / 1024.0, BytesUnits);
    }
    else
    {
        AddCommas (Text, Defrag->GetVolume().GetVolumeInfo().FreeBytes / BytesDivisor);
        strcat (Text, " ");
        strcat (Text, BytesUnits);
    }
    SetDlgItemText (Dlg, IDC_DISKFREEBYTES, Text);

    // DiskSizeClusters
    AddCommas (Text, Defrag->GetVolume().GetVolumeInfo().ClusterCount);
    strcat (Text, " clusters");
    SetDlgItemText (Dlg, IDC_DISKSIZECLUSTERS, Text);

    // DiskClusterSize
    sprintf (Text, "%u bytes", Defrag->GetVolume().GetVolumeInfo().ClusterSize);
    SetDlgItemText (Dlg, IDC_DISKCLUSTERSIZE, Text);

    // DirsCount
    AddCommas (Text, Report.DirsCount);
    SetDlgItemText (Dlg, IDC_DIRSCOUNT, Text);

    // FilesCount
    AddCommas (Text, Report.FilesCount);
    SetDlgItemText (Dlg, IDC_FILESCOUNT, Text);

    // FilesFragged
    sprintf (Text, "(%.2f%%)", Report.PercentFragged);
    AddCommas (Text2, Report.FraggedFiles.size());
    sprintf (Text3, "%s %s", Text, Text2);
    SetDlgItemText (Dlg, IDC_FILESFRAGGED, Text3);

    // Average Frags
    sprintf (Text, "%.2f", Report.AverageFragments);
    SetDlgItemText (Dlg, IDC_AVERAGEFRAGS, Text);

    // FilesSizeBytes
    if (Fractional)
    {
        sprintf (Text, "%.2f %s", (double)(signed)(Report.FilesSizeBytes / 
            (BytesDivisor / 1024)) / 1024.0, BytesUnits);
    }
    else
    {
        AddCommas (Text, Report.FilesSizeBytes / (uint64)BytesDivisor);
        strcat (Text, " ");
        strcat (Text, BytesUnits);
    }
    SetDlgItemText (Dlg, IDC_FILESSIZEBYTES, Text);

    // Files SizeOnDisk
    if (Fractional)
    {
        sprintf (Text, "%.2f %s", (double)(signed)((Report.FilesSizeBytes + Report.FilesSlackBytes) / 
            (BytesDivisor / 1024)) / 1024.0, BytesUnits);
    }
    else
    {
        AddCommas (Text, (Report.FilesSizeBytes + Report.FilesSlackBytes) / (uint64)BytesDivisor);
        strcat (Text, " ");
        strcat (Text, BytesUnits);
       
    }
    SetDlgItemText (Dlg, IDC_FILESSIZEONDISK, Text);

    // FilesSlackBytes
    if (Fractional)
    {
        sprintf (Text, "%.2f %s", (double)(signed)(Report.FilesSlackBytes / 
            (BytesDivisor / 1024)) / 1024.0, BytesUnits);
    }
    else
    {
        AddCommas (Text, Report.FilesSlackBytes / BytesDivisor);
        strcat (Text, " ");
        strcat (Text, BytesUnits);
    }
    sprintf (Text2, "(%.2f%%)", Report.PercentSlack);
    sprintf (Text3, "%s %s", Text2, Text);
    SetDlgItemText (Dlg, IDC_FILESSLACKBYTES, Text3);

    // Recommendation
    bool PFRec = false; // Recommend based off percent fragged files?
    bool AFRec = false; // Recommend based off average fragments per file?

    if (Report.PercentFragged >= 5.0f)
        PFRec = true;

    if (Report.AverageFragments >= 1.1f)
        AFRec = true;

    strcpy (Text, "* ");

    if (PFRec)
    {
        sprintf 
        (
            Text2,
            "%.2f%% of the files on this volume are fragmented. ",
            Report.PercentFragged
        );

        strcat (Text, Text2);
    }

    if (AFRec)
    {
        sprintf
        (
            Text2,
            "The average fragments per file (%.2f) indicates a high degree of fragmentation. ",
            Report.AverageFragments
        );

        strcat (Text, Text2);
    }

    if (Report.PercentFragged <  5.0f  &&  Report.AverageFragments < 1.1f)
        sprintf (Text, "* No defragmentation is necessary at this point.");
    else
    if (Report.PercentFragged < 15.0f  &&  Report.AverageFragments < 1.3f)
        strcat (Text, "It is recommended that you perform a Fast Defrag.");
    else
        strcat (Text, "It is recommended that you perform an Extensive Defrag.");

    // Should we recommend a smaller cluster size?
    if (Report.PercentSlack >= 10.0f)
    {
        sprintf 
        (
            Text2, 
            "\n* A large amount of disk space (%.2f%%) is being lost "
            "due to a large (%u bytes) cluster size. It is recommended "
            "that you use a disk utility such as Partition Magic to "
            "reduce the cluster size of this volume.",
            Report.PercentSlack,
            Defrag->GetVolume().GetVolumeInfo().ClusterSize
        );

        strcat (Text, Text2);
    }

    SetDlgItemText (Dlg, IDC_RECOMMEND, Text);

    return;
}


INT_PTR CALLBACK ReportDialogProc (HWND Dlg, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
            SetReportInfo (Dlg, Defrag->GetDefragReport (), 1, "bytes", false);
            return (1);

        case WM_COMMAND:
            switch (LOWORD(WParam))
            {
                case IDC_REPORTOK:
                    EndDialog (Dlg, 0);
                    return (1);

                case IDC_GIGABYTES:
                    SetReportInfo (Dlg, Defrag->GetDefragReport (), 1024*1024*1024, "GB", true);
                    return (1);

                case IDC_MEGABYTES:
                    SetReportInfo (Dlg, Defrag->GetDefragReport (), 1024*1024, "MB", false);
                    return (1);

                case IDC_KILOBYTES:
                    SetReportInfo (Dlg, Defrag->GetDefragReport (), 1024, "KB", false);
                    return (1);

                case IDC_BYTES:
                    SetReportInfo (Dlg, Defrag->GetDefragReport (), 1, "bytes", false);
                    return (1);
            }
    }

    return (0);
}
