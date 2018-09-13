#ifndef __propshts_h
#define __propshts_h

INT_PTR CALLBACK SummaryPropDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SchedulePropDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DownloadPropDlgProc(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedDownloadDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LoginOptionDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewScheduleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif //__propshts_h
