/*
    Desktop creation example

    Silver Blade (silverblade_uk@hotmail.com)
    5th July 2003
*/


#include <windows.h>

const char DesktopClassName[] = "DesktopWindow";



LRESULT CALLBACK DeskWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CLOSE :
        {
            // Over-ride close. We need to close desktop some other way.
            return 0;
        }

        case WM_PAINT :
        {
            // We'd want to draw the desktop wallpaper here. Need to
            // maintain a copy of the wallpaper in an off-screen DC and then
            // bitblt (or stretchblt?) it to the screen appropriately. For
            // now, though, we'll just draw some text.

            PAINTSTRUCT ps;
            HDC DesktopDC = BeginPaint(hwnd, &ps);

            char Text [] = "ReactOS 0.1.2 Desktop Example\nby Silver Blade";

            int Width, Height;

            Width = GetSystemMetrics(SM_CXSCREEN);
            Height = GetSystemMetrics(SM_CYSCREEN);

            // This next part could be improved by working out how much
            // space the text actually needs...

            RECT r;
            r.left = Width - 260;
            r.top = Height - 80;
            r.right = r.left + 250;
            r.bottom = r.top + 40;

            SetTextColor(DesktopDC, 0x00ffffff);
            SetBkMode(DesktopDC, TRANSPARENT);
            DrawText(DesktopDC, &Text, -1, &r, DT_RIGHT);

            EndPaint(hwnd, &ps);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



int main()
{
    WNDCLASSEX wc;

    wc.cbSize       = sizeof(WNDCLASSEX);
    wc.style        = 0;
    wc.lpfnWndProc  = &DeskWndProc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = 0;
    wc.hInstance    = GetModuleHandle(NULL);
    wc.hIcon        = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground= (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName= DesktopClassName;
    wc.hIconSm      = NULL;

    if (! RegisterClassEx(&wc))
        return 1; // error


    HWND Desktop;
    int Width, Height;

    Width = GetSystemMetrics(SM_CXSCREEN);
    Height = GetSystemMetrics(SM_CYSCREEN);

    Desktop = CreateWindowEx(0, DesktopClassName, "Desktop",
                            WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN,
                            0, 0, Width, Height,
                            NULL, NULL, GetModuleHandle(NULL), NULL);

    if (! Desktop)
        return 1;   // error

    MSG Msg;

    while (GetMessage(&Msg, 0, 0, 0) != 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return 0;   // get the return code!
}
