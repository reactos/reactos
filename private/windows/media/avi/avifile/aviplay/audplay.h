
BOOL FAR aviaudioPlay(HWND hwnd, PAVISTREAM pavi, LONG lStart, LONG lEnd, BOOL fWait);
void FAR aviaudioMessage(HWND hwnd, unsigned msg, WPARAM wParam, LONG lParam);
void FAR aviaudioStop(void);
LONG FAR aviaudioTime(void);
