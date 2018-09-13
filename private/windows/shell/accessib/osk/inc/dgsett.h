//  dgSetting.h


/****************************************************************************/
/*    FUNCTIONS IN THIS FILE			*/
/****************************************************************************/
INT_PTR Type_ModeDlgFunc(HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK Type_ModeDlgProc(HWND hDlg, UINT message, 
                                  WPARAM wParam, LPARAM lParam);

BOOL Type_ModeDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


void ChangeDwellTime(HWND hDlg, UINT message, WPARAM wParam);
void ChangeScanTime(HWND hDlg, UINT message, WPARAM wParam);
void SwitchToBlockKB(void);
void SwitchToActualKB(void);
void SwitchToJapaneseKB(void);
void SwitchToEuropeanKB(void);
void BlockKB(void);
void ActualKB(void);
void JapaneseKB(void);
void EuropeanKB(void);


