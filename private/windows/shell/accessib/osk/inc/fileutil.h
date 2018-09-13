
/****************************************************************************/
/*    FUNCTIONS IN THIS FILE															    */
/****************************************************************************/
void InitFileUtils(HINSTANCE hInstance);
BOOL OpenNewFile(void);
BOOL SaveAs(void);
void ProcessCDError(DWORD dwErrorCode, HWND hWnd);
void Save_Size_Position_Only(void);

BOOL Read_Map_Setting(LPCSTR filename);
BOOL Read_New_Setting(LPCSTR filename);
BOOL SaveAs_New_Setting(LPCSTR lpstrFile);

BOOL BringUpBowser(void);
PSID GetCurrentUserInfo(void);
BOOL RunningAsAdministrator(void);
BOOL OpenUserSetting(void);
BOOL SaveUserSetting(void);

