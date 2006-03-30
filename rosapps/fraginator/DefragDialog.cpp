#include "DefragDialog.h"
#include "Defragment.h"
#include "resource.h"


void UpdateDefragInfo (HWND Dlg)
{
    Defragment *Defrag;
    HWND PercentItem;
    char PercentText[100];

    Defrag = (Defragment *) GetWindowLongPtr (Dlg, GWLP_USERDATA);
    
    sprintf (PercentText, "%6.2f%%", Defrag->GetStatusPercent());
    PercentItem = GetDlgItem (Dlg, IDC_PERCENT);
    SendMessage (GetDlgItem (Dlg, IDC_PERCENT), WM_SETTEXT, 0, (LPARAM) PercentText);
    SendMessage (GetDlgItem (Dlg, IDC_STATUS_TEXT), WM_SETTEXT, 0, (LPARAM) Defrag->GetStatusString().c_str());

    return;
}


INT_PTR CALLBACK DefragDialogProc (HWND Dlg, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (Dlg, GWLP_USERDATA, (LONG_PTR)LParam);
            UpdateDefragInfo (Dlg);
            return (1);

        case WM_UPDATEINFO:
            UpdateDefragInfo (Dlg);
            return (1);
    }

    return (0);
}
