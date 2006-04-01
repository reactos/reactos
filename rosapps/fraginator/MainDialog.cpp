#include "unfrag.h"
#include "MainDialog.h"
#include "resource.h"
#include "Fraginator.h"
#include "Defragment.h"
#include "ReportDialog.h"


vector<wstring> DrivesList;
LRESULT AnalyzeID;
LRESULT FastID;
LRESULT ExtensiveID;
bool QuitWhenDone;
bool Stopping;


LRESULT PriHighID;
LRESULT PriAboveNormID;
LRESULT PriNormalID;
LRESULT PriBelowNormID;
LRESULT PriIdleID;


void             InitDialog       (HWND Dlg);
void             UpdateDefragInfo (HWND Dlg);
void             UpdatePriority   (HWND Dlg);
wstring           GetDefaultTitle  (void);
wstring           GetDefragTitle   (void);
void             SetDisables      (HWND Dlg);
INT_PTR CALLBACK MainDialogProc   (HWND Dlg, UINT Msg, WPARAM WParam, LPARAM LParam);


static void InitDialog (HWND Dlg)
{
    // Make internal list
    DWORD DriveMask;
    HWND  DlgItem;
    int d;

    // Clear out wisecracks line for now
    SetDlgItemText (Dlg, IDC_WISECRACKS, L"\"Defrag, baby!\"");

    // Make list of logical drives
    DrivesList.resize (0);
    DriveMask = GetLogicalDrives ();

    for (d = 0; d < 26; d++)
    {
        if (DriveMask & (1 << d))
        {
            wstring Name;

            Name = (wchar_t)(L'A' + d);
            Name += L':';
            DrivesList.push_back (Name);
        }
    }

    // Give list to dropdown list
    DlgItem = GetDlgItem (Dlg, IDC_DRIVES_LIST);
    SendMessage (DlgItem, CB_RESETCONTENT, 0, 0);
    for (d = 0; d < DrivesList.size(); d++)
    {
        SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) DrivesList[d].c_str());
    }

    // Put in defrag methods
    DlgItem = GetDlgItem (Dlg, IDC_METHODS_LIST);
    SendMessage (DlgItem, CB_RESETCONTENT, 0, 0);
    AnalyzeID   = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Analyze Only");
    FastID      = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Fast Defrag");
    ExtensiveID = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Extensive Defrag");

    // Set up process priorities
    DlgItem = GetDlgItem (Dlg, IDC_PRIORITY_LIST);
    SendMessage (Dlg, CB_RESETCONTENT, 0, 0);
    PriHighID      = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"High");
    PriAboveNormID = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Above Normal");
    PriNormalID    = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Normal");
    PriBelowNormID = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Below Normal");
    PriIdleID      = SendMessage (DlgItem, CB_ADDSTRING, 0, (LPARAM) L"Idle");
    UpdatePriority (Dlg);

    // Reset texts and progress meters
    SendDlgItemMessage (Dlg, IDC_STATUS,   WM_SETTEXT,   0, (LPARAM) L"");
    SendDlgItemMessage (Dlg, IDC_PERCENT,  WM_SETTEXT,   0, (LPARAM) L"");
    SendDlgItemMessage (Dlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM (0, 10000));
    SendDlgItemMessage (Dlg, IDC_PROGRESS, PBM_SETPOS,   0, 0);

    return;
}


void UpdateDefragInfo (HWND Dlg)
{
    wchar_t PercentText[100];
    static double OldPercent = 200.0f;
    static wstring OldStatus = L"Non";
    wstring NewStatus;
    double NewPercent;

    if (Defrag == NULL)
        return;
   
    NewPercent = Defrag->GetStatusPercent ();
    if (NewPercent > 100.0f) 
        NewPercent = 100.0f;
    if (NewPercent < 0.0f)
        NewPercent = 0.0f;
    if (NewPercent != OldPercent)
    {
        swprintf (PercentText, L"%6.2f%%", NewPercent);
        SendDlgItemMessage (Dlg, IDC_PERCENT, WM_SETTEXT, 0, (LPARAM) PercentText);
        SendDlgItemMessage (Dlg, IDC_PROGRESS, PBM_SETPOS, 
            (WPARAM) (int)(NewPercent * 100.0f), 0);
        OldPercent = NewPercent;
    }

    NewStatus = Defrag->GetStatusString ();
    if (NewStatus != OldStatus)
    {   // Change & characters to && to avoid underlining
        wstring Status;
        wstring::iterator it;

        Status = NewStatus;
        it = Status.begin ();
        while (it < Status.end())
        {
            if (*it == L'&')
            {
                Status.insert (it, 1, L'&');
                it++;
            }

            it++;
        }

        SendDlgItemMessage (Dlg, IDC_STATUS, WM_SETTEXT, 0, 
            (LPARAM) Status.c_str());

        OldStatus = NewStatus;
    }

    return;
}


wstring GetDefaultTitle (void)
{
    wstring DefaultText;

    DefaultText = wstring(wstring(APPNAME_GUI) + wstring(L" v") + wstring(APPVER_STR) +
                  wstring(L" (C) 2000 by Rick Brewster"));

    return (DefaultText);
}


wstring GetDefragTitle (void)
{
    wstring DefragText;
    wchar_t Percent[10];

    swprintf (Percent, L"%.2f%%", Defrag->GetStatusPercent());

    DefragText = GetDefaultTitle ();
    if (Defrag != NULL)
    {
        DefragText = wstring(Percent) + wstring (L" - ") + Defrag->GetVolume().GetRootPath() + 
            wstring (L" - ") + DefragText;
    }

    return (DefragText);
}


void SetDisables (HWND Dlg)
{
    // If a defrag is in process, set L'Start' button to say L'Stop' and disable
    // the Select Drive and Select Action controls
    if (Defrag != NULL  &&  !Defrag->IsDoneYet()  &&  !Defrag->HasError())
    {
        SendMessage (GetDlgItem (Dlg, IDC_STARTSTOP), WM_SETTEXT, 0, (LPARAM) L"Stop");
        EnableWindow (GetDlgItem (Dlg, IDC_DRIVES_LIST), FALSE);
        EnableWindow (GetDlgItem (Dlg, IDC_METHODS_LIST), FALSE);
    }
    else
    {
        SendMessage (GetDlgItem (Dlg, IDC_STARTSTOP), WM_SETTEXT, 0, (LPARAM) L"Start");
        EnableWindow (GetDlgItem (Dlg, IDC_STARTSTOP), TRUE);
        EnableWindow (GetDlgItem (Dlg, IDC_QUIT), TRUE);
        EnableWindow (GetDlgItem (Dlg, IDC_DRIVES_LIST), TRUE);
        EnableWindow (GetDlgItem (Dlg, IDC_METHODS_LIST), TRUE);
    }

    return;
}


void UpdatePriority (HWND Dlg)
{
    LRESULT Id;
    DWORD Priority;

    Id = SendDlgItemMessage (Dlg, IDC_PRIORITY_LIST, CB_GETCURSEL, 0, 0);

    if (Id == PriHighID)
        Priority = HIGH_PRIORITY_CLASS;
    else
    if (Id == PriAboveNormID)
        Priority = ABOVE_NORMAL_PRIORITY_CLASS;
    else
    if (Id == PriNormalID)
        Priority = NORMAL_PRIORITY_CLASS;
    else
    if (Id == PriBelowNormID)
        Priority = BELOW_NORMAL_PRIORITY_CLASS;
    else
    if (Id == PriIdleID)
        Priority = IDLE_PRIORITY_CLASS;
    else
        return;

    SetPriorityClass (GetCurrentProcess(), Priority);
    return;
}


// Save settings (ie, process priority and defrag type options)
bool GetRegKeys (HKEY *RegKeyResult)
{
    HKEY RegKey;
    LONG Error;

    Error = RegCreateKeyEx
    (
        HKEY_CURRENT_USER,
        L"Software\\Fraginator",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &RegKey,
        NULL
    );

    if (Error != ERROR_SUCCESS)
        return (false);

    *RegKeyResult = RegKey;
    return (true);
}


bool DoneRegKey (HKEY RegKey)
{
    RegCloseKey (RegKey);
    return (true);
}


void SaveSettings (HWND Dlg)
{
    LRESULT DefragID;
    DWORD   DefragVal;
    LRESULT PriID;
    DWORD   PriVal;
    HKEY    RegKey;

    DefragID = SendDlgItemMessage (Dlg, IDC_METHODS_LIST, CB_GETCURSEL, 0, 0);
    PriID    = SendDlgItemMessage (Dlg, IDC_PRIORITY_LIST, CB_GETCURSEL, 0, 0);

    // Action
    if (DefragID == AnalyzeID)
        DefragVal = (DWORD) DefragAnalyze;
    else
    if (DefragID == FastID)
        DefragVal = (DWORD) DefragFast;
    else
    if (DefragID == ExtensiveID)
        DefragVal = (DWORD) DefragExtensive;

    // Process Priority
    if (PriID == PriHighID)
        PriVal = HIGH_PRIORITY_CLASS;
    else
    if (PriID == PriAboveNormID)
        PriVal = ABOVE_NORMAL_PRIORITY_CLASS;
    else
    if (PriID == PriNormalID)
        PriVal = NORMAL_PRIORITY_CLASS;
    else
    if (PriID == PriBelowNormID)
        PriVal = BELOW_NORMAL_PRIORITY_CLASS;
    else
    if (PriID == PriIdleID)
        PriVal = IDLE_PRIORITY_CLASS;

    if (!GetRegKeys (&RegKey))
        return;

    RegSetValueEx
    (
        RegKey,
        L"Default Action",
        0,
        REG_DWORD,
        (CONST BYTE *)&DefragVal,
        sizeof (DefragVal)
    );

    RegSetValueEx
    (
        RegKey,
        L"Process Priority",
        0,
        REG_DWORD,
        (CONST BYTE *)&PriVal,
        sizeof (PriVal)
    );

    DoneRegKey (RegKey);
    return;
}


void LoadSettings (HWND Dlg)
{
    DefragType DType;
    DWORD      DTypeVal;
    LRESULT    DefragID;
    DWORD      PriVal;
    LRESULT    PriID;
    HKEY       RegKey;
    DWORD      RegType;
    DWORD      RegSize;
    LONG       Error;

    if (!GetRegKeys (&RegKey))
        return;

    RegSize = sizeof (DTypeVal);
    RegType = REG_DWORD;

    Error = RegQueryValueEx
    (
        RegKey,
        L"Default Action",
        0,
        &RegType,
        (BYTE *)&DTypeVal,
        &RegSize
    );

    if (Error != ERROR_SUCCESS)
        DTypeVal = DefragAnalyze;

    Error = RegQueryValueEx
    (
        RegKey,
        L"Process Priority",
        0,
        &RegType,
        (BYTE *)&PriVal,
        &RegSize
    );

    DoneRegKey (RegKey);

    if (Error != ERROR_SUCCESS)
        PriVal = NORMAL_PRIORITY_CLASS;

    DType = (DefragType) DTypeVal;
    switch (DType)
    {
        default:
        case DefragAnalyze:
            DefragID = AnalyzeID;
            break;

        case DefragFast:
            DefragID = FastID;
            break;

        case DefragExtensive:
            DefragID = ExtensiveID;
            break;
    }

    switch (PriVal)
    {
        case HIGH_PRIORITY_CLASS:
            PriID = PriHighID;
            break;

        case ABOVE_NORMAL_PRIORITY_CLASS:
            PriID = PriAboveNormID;
            break;

        default:
        case NORMAL_PRIORITY_CLASS:
            PriID = PriNormalID;
            break;

        case BELOW_NORMAL_PRIORITY_CLASS:
            PriID = PriBelowNormID;
            break;

        case IDLE_PRIORITY_CLASS:
            PriID = PriIdleID;
            break;
    }

    SendDlgItemMessage (Dlg, IDC_PRIORITY_LIST, CB_SETCURSEL, PriID,    0);
    SendDlgItemMessage (Dlg, IDC_METHODS_LIST,  CB_SETCURSEL, DefragID, 0);
    return;
}


#define IDLETIME 25
wstring OldWindowText = L"";

INT_PTR CALLBACK MainDialogProc (HWND Dlg, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    static bool ReEntrance = false;

    switch (Msg)
    {
        case WM_INITDIALOG:
            Stopping = false;
            SetWindowText (Dlg, GetDefaultTitle().c_str());
            SetDisables (Dlg);
            InitDialog (Dlg);
            SetTimer (Dlg, 1, IDLETIME, NULL);
            SetClassLong (Dlg, GCL_HICON, (LONG) LoadIcon (GlobalHInstance, MAKEINTRESOURCE(IDI_ICON)));
            QuitWhenDone = false;
            LoadSettings (Dlg);
            UpdatePriority (Dlg);
            return (1);


        case WM_TIMER:
            if (Defrag != NULL  &&  !ReEntrance)
            {
                wstring NewTitle;

                SendMessage (Dlg, WM_UPDATEINFO, 0, 0);

                NewTitle = GetDefragTitle ();
                if (NewTitle != OldWindowText)
                {
                    OldWindowText = NewTitle;
                    SetWindowText (Dlg, NewTitle.c_str());
                }

                if (Defrag->IsDoneYet()  ||  Defrag->HasError())
                {   // This is the code executed when defragging is finished (or stopped :)
                    if (Defrag->GetDefragType() == DefragAnalyze  &&  
                        !Defrag->HasError()  &&
                        !Stopping)
                    {   // Show report
                        ReEntrance = true;

                        DialogBoxParam (GlobalHInstance, MAKEINTRESOURCE (IDD_REPORT),
                            Dlg, ReportDialogProc, (LPARAM) Defrag);

                        ReEntrance = false;
                    }

                    delete Defrag;
                    Defrag = NULL;
                    SetDisables (Dlg);
                    SetWindowText (Dlg, GetDefaultTitle().c_str());

                    Stopping = false;

                    if (QuitWhenDone)
                        SendMessage (GetDlgItem (Dlg, IDC_QUIT), BM_CLICK, 0, 0);
                }
            }

            SetTimer (Dlg, 1, IDLETIME, NULL);
            return (0);


        case WM_UPDATEINFO:
            UpdateDefragInfo (Dlg);
            return (1);


        case WM_CLOSE:
            SendMessage (GetDlgItem (Dlg, IDC_QUIT), BM_CLICK, 0, 0);
            return (1);


        case WM_COMMAND:
            switch (LOWORD(WParam))
            {
                case IDC_PRIORITY_LIST:
                    UpdatePriority (Dlg);
                    return (1);


                case ID_MAIN_HELP:
                    ShellExecute (Dlg, L"open", L"Fraginator.chm", L"", L".", SW_SHOW);
                    return (1);


                case IDC_QUIT:
                    if (Defrag == NULL)
                    {   // This is the code executing when quitting
                        SaveSettings (Dlg);
                        EndDialog (Dlg, 0);
                    }
                    else
                    {   // Tell defragging to finish and disable our button
                        QuitWhenDone = true;
                        SendMessage (GetDlgItem (Dlg, IDC_STARTSTOP), BM_CLICK, 0, 0);
                        EnableWindow (GetDlgItem (Dlg, IDC_QUIT), FALSE);
                    }
                    return (1);


                case IDC_STARTSTOP:
                    if (Defrag == NULL)
                    {   // L"Start"
                        wchar_t Drive[10];
                        LRESULT ID;
                        DefragType Method;
                        HANDLE H;

                        if (Defrag != NULL)
                            return (1);

                        SendMessage (GetDlgItem (Dlg, IDC_DRIVES_LIST), WM_GETTEXT, 
                            sizeof (Drive) - 1, (LPARAM) Drive);

                        if (wcslen(Drive) != 2  ||  Drive[1] != L':')
                            return (1);

                        ID = SendMessage (GetDlgItem (Dlg, IDC_METHODS_LIST), CB_GETCURSEL, 0, 0);
                        Method = DefragInvalid;
                        if (ID == AnalyzeID)
                            Method = DefragAnalyze;
                        else
                        if (ID == FastID)
                            Method = DefragFast;
                        else
                        if (ID == ExtensiveID)
                            Method = DefragExtensive;

                        if (Method != DefragInvalid)
                        {
                            Defrag = StartDefragThread (Drive, Method, H);
                            Defrag->SetDoLimitLength (false);
                            SetWindowText (Dlg, GetDefragTitle().c_str());
                            SetDisables (Dlg);
                        }   
                    }
                    else
                    {   // L"Stop"
                        Stopping = true;
                        Defrag->Stop ();
                        EnableWindow (GetDlgItem (Dlg, IDC_STARTSTOP), FALSE);
                        EnableWindow (GetDlgItem (Dlg, IDC_QUIT), FALSE);
                    }
                    return (1);
            }
    }

    // Otherwise, return 0 to say we did not process the message.
    return (0);
}
