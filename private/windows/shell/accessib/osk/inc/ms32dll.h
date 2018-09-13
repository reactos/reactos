/***************************************************************************/
/*     Functions Declaration      */
/***************************************************************************/
BOOL WINAPI InitMouse(void);
LRESULT CALLBACK  MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

BOOL WINAPI KillMouse(void);
BOOL WINAPI CapLetterON(void);

BOOL AltKeyPressed(void);
BOOL ControlKeyPressed(void);


BOOL IsOneOfOurKey(HWND hwnd);
void DoAllUp (HWND hwnd, BOOL sendchr);
void DoButtonDOWN(HWND hwnd);
void SendWord(LPCSTR lpszKeys);
BOOL udfKeyUpProc(HWND khwnd, int keyname);
void MakeClick(int what);
void InvertColors(HWND chwnd);
void ReturnColors(HWND chwnd, BOOL inval);
void CALLBACK YourTimeIsOver(HWND hwnd, UINT uMsg, 
                             UINT_PTR idEvent, DWORD dwTime);
void killtime(void);
void Cursorover(void);
void SetTimeControl(HWND hwnd);
void PaintBucket(HWND hwnd);
void CALLBACK Painttime(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void SendChar(HWND ldhwnd);

int CharTrans(int index, BOOL *SkipSendkey);

void ReDrawModifierKey(void);

void Extra_Key(HWND hwnd, int index);
BOOL IsDeadKey(UINT vk, UINT scancode);

void PaintLine(HWND hwnd, HDC hdc, RECT rect);
void ReleaseAltCtrlKeys(void);

