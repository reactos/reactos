
LPDIRECTDRAW7 pDD;
LPDIRECTDRAWSURFACE7 lpddsPrimary; //, lpddsBack;
HRESULT ddrval;
HWND hwnd;

bool running, fullscreen;

void ddRelease ();
char* DDErrorString (HRESULT hr);
LONG WINAPI WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

extern "C" bool HookAPICalls();
