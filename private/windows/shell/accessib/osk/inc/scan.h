
/***********************************************/
//			Functions in this file
/***********************************************/
void Scanning(int from);
void CALLBACK LineScanProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK KeyScanProc_Actual_101(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK KeyScanProc_Actual_106(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK KeyScanProc_Actual_102(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void RestoreRowColor(int Row);
void RestoreKeyColor(int i);
void KillScanTimer(BOOL reset);

void Scanning_Actual(int from);
void Scanning_Block(int from);
void CALLBACK KeyScanProc_Block(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void CALLBACK BlockScanProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
void RestoreBlockColor(int ColStart, int ColEnd);

