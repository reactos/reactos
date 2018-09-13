/*-----------------------------------------------------------------------
**
** Hotkey.c
**
** Hotkey edit control.
**
**-----------------------------------------------------------------------*/
//
// Win32 REVIEW:
//  See all the Get/SetWindowInt().
//
#include "ctlspriv.h"

#define F_EXT	    0x01000000L

#define GWU_VIRTKEY  0
#define GWU_MODS     1*sizeof(UINT)
#define GWU_INVALID  2*sizeof(UINT)
#define GWU_DEFAULT  3*sizeof(UINT)
#define GWU_HFONT    4*sizeof(UINT)
#define GWU_YFONT    5*sizeof(UINT)
#define NUM_WND_EXTRA (GWU_YFONT+sizeof(UINT))

LRESULT CALLBACK HotKeyWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

#ifndef WIN32
#pragma code_seg(CODESEG_INIT)
#endif

BOOL FAR PASCAL InitHotKeyClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, HOTKEY_CLASS, &wc))
    {
#ifndef WIN32
        extern LRESULT CALLBACK _HotKeyWndProc(HWND, UINT, WPARAM, LPARAM);
	wc.lpfnWndProc	 = _HotKeyWndProc;
#else
	wc.lpfnWndProc	 = HotKeyWndProc;
#endif
	wc.lpszClassName = s_szHOTKEY_CLASS;
	wc.style	 = CS_GLOBALCLASS;
	wc.hInstance	 = hInstance;
	wc.hIcon	 = NULL;
	wc.hCursor	 = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName	 = NULL;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = NUM_WND_EXTRA;

	if (!RegisterClass(&wc))
	    return FALSE;
    }
    return TRUE;
}

#ifndef WIN32
#pragma code_seg()
#endif


#pragma data_seg(DATASEG_READONLY)
const UINT s_Combos[8] = {
                    HKCOMB_NONE,
		    HKCOMB_S,
		    HKCOMB_C,
		    HKCOMB_SC,
		    HKCOMB_A,
		    HKCOMB_SA,
		    HKCOMB_CA,
		    HKCOMB_SCA};
#pragma data_seg()

void NEAR PASCAL SetHotKey(HWND hwnd, WORD wVirtKey, WORD wMods, BOOL fSendNotify)
{
    /* don't invalidate if it's the same
     */
    if (wVirtKey == GetWindowInt(hwnd, GWU_VIRTKEY) &&
        wMods == GetWindowInt(hwnd, GWU_MODS))
	return;

    SetWindowInt(hwnd, GWU_VIRTKEY ,wVirtKey);
    SetWindowInt(hwnd, GWU_MODS ,wMods);
    InvalidateRect(hwnd,NULL,TRUE);
    
    if (fSendNotify) {
        FORWARD_WM_COMMAND(GetParent(hwnd), GetDlgCtrlID(hwnd), hwnd, EN_CHANGE, SendMessage);
    }
}

void NEAR PASCAL GetKeyName(UINT vk, LPSTR lpsz, BOOL fExt)
{
    LONG scan;

    scan = (LONG)MapVirtualKey(vk,0) << 16;
    if (fExt)
	scan |= F_EXT;

    GetKeyNameText(scan,lpsz,50);
}

void NEAR PASCAL PaintHotKey(register HWND hwnd)
{
    char sz[128];
    char szPlus[10];
    WORD cch;
    register HDC hdc;
    UINT wMods;
    UINT wVirtKey;
    PAINTSTRUCT ps;
    int x, y;
    HANDLE hFont;
    // DWORD dwColor;
    // DWORD dwBkColor;

    LoadString(HINST_THISDLL, IDS_PLUS, szPlus, sizeof(szPlus));

    wVirtKey = GetWindowInt(hwnd, GWU_VIRTKEY);
    wMods = GetWindowInt(hwnd, GWU_MODS);
    if (wVirtKey || wMods)
    {
	sz[0] = 0;
	cch = 0;
	if (wMods & HOTKEYF_CONTROL)
	{
	    GetKeyName(VK_CONTROL, sz, FALSE);
	    lstrcat(sz,(LPSTR)szPlus);
	}
	if (wMods & HOTKEYF_SHIFT)
	{
	    GetKeyName(VK_SHIFT, sz+lstrlen(sz), FALSE);
	    lstrcat(sz,szPlus);
	}
	if (wMods & HOTKEYF_ALT)
	{
	    GetKeyName(VK_MENU, sz+lstrlen(sz), FALSE);
	    lstrcat(sz,szPlus);
	}

	GetKeyName(wVirtKey, sz+lstrlen(sz), wMods & HOTKEYF_EXT);
    }
    else
	LoadString(HINST_THISDLL,IDS_NONE,sz,100);

    cch = lstrlen(sz);

    HideCaret(hwnd);

    InvalidateRect(hwnd, NULL, TRUE);
    hdc = BeginPaint(hwnd,&ps);


    hFont = SelectObject(hdc, (HFONT)GetWindowInt(hwnd,GWU_HFONT));

    x = g_cxBorder;
    y = g_cyBorder;

    if (IsWindowEnabled(hwnd))
    {
	SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
	SetTextColor(hdc, g_clrWindowText);
	TextOut(hdc,x,y,sz,cch);
    }
    else 
    { 
	// set the background color to Grayed like edit controls
	SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
    	if (g_clrGrayText)
      	{
	    SetTextColor(hdc,g_clrGrayText);
	    TextOut(hdc,x,y,sz,cch);
      	}
    	else
      	{
	    GrayString(hdc,NULL,NULL,(DWORD)(LPSTR)sz,cch,x,y,0,0);
      	}
    }

    MGetTextExtent(hdc, sz, cch, &x, NULL);
     if (GetFocus() == hwnd)
	 SetCaretPos(x+g_cxBorder,
		    g_cyBorder);
    ShowCaret(hwnd);

#if 0
    if (hFont)
	SelectObject(hdc,hFont);
#endif

    EndPaint(hwnd,&ps);
}

void NEAR PASCAL HKMSetRules(HWND hwnd, int wParam, int lParam)
{
    SetWindowInt(hwnd, GWU_INVALID, wParam);
    SetWindowInt(hwnd, GWU_DEFAULT, lParam);
}

HFONT NEAR PASCAL HKMSetFont(HWND hwnd, HFONT wParam)
{
    HFONT lParam;
    HDC hdc;
    UINT cy;

    lParam = (HFONT)GetWindowInt(hwnd,GWU_HFONT);
    SetWindowInt(hwnd,GWU_HFONT,(int)wParam);
    hdc = GetDC(hwnd);
    if (wParam)
        wParam = SelectObject(hdc, wParam);
    MGetTextExtent(hdc, "C", 1, NULL, &cy);
    SetWindowInt(hwnd,GWU_YFONT,cy);
    if (wParam)
        SelectObject(hdc, wParam);
    ReleaseDC(hwnd,hdc);
    InvalidateRect(hwnd,NULL,TRUE);
    return lParam;
}

LRESULT CALLBACK HotKeyWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    DWORD dw;
    WORD wVirtKey;
    WORD wMods;
    // PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;

    switch (wMsg)
    {
	case WM_NCCREATE:
	    dw = GetWindowLong (hwnd, GWL_EXSTYLE);
	    SetWindowLong (hwnd, GWL_EXSTYLE, dw | WS_EX_CLIENTEDGE);
	    InitGlobalColors();
	    return TRUE;

	case WM_CREATE:
	    SetHotKey(hwnd, 0, 0, FALSE);
            HKMSetRules(hwnd, 0, 0);
            HKMSetFont(hwnd, g_hfontSystem);
	    break;

	case WM_SETFOCUS:
	    InvalidateRect(hwnd,NULL,TRUE);
	    CreateCaret(hwnd,NULL,0,GetWindowInt(hwnd,GWU_YFONT));
	    ShowCaret(hwnd);
	    break;

	case WM_KILLFOCUS:
	    if (!GetWindowInt(hwnd, GWU_VIRTKEY))
		SetHotKey(hwnd, 0, 0, TRUE);
	    DestroyCaret();
	    break;

	case WM_GETDLGCODE:
	    return DLGC_WANTCHARS | DLGC_WANTARROWS; // | DLGC_WANTALLKEYS;

	case HKM_SETHOTKEY:
	    SetHotKey(hwnd, LOBYTE(wParam), HIBYTE(wParam), FALSE);
	    break;

	case HKM_GETHOTKEY:
	    return (256*(BYTE)GetWindowInt(hwnd, GWU_MODS)) +
		((BYTE)GetWindowInt(hwnd, GWU_VIRTKEY));
	    break;

	case HKM_SETRULES:
            HKMSetRules(hwnd, wParam, LOWORD(lParam));
	    break;

	case WM_LBUTTONDOWN:
	    SetFocus(hwnd);
	    break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	    switch (wParam)
	    {
		case VK_RETURN:
		case VK_TAB:
		case VK_SPACE:
		case VK_DELETE:
		case VK_ESCAPE:
		case VK_BACK:
		case VK_LWIN:
		case VK_RWIN:
		case VK_APPS:
		    SetHotKey(hwnd, 0, 0, TRUE);
		    return DefWindowProc(hwnd,wMsg,wParam,lParam);

		case VK_MENU:
		case VK_SHIFT:
		case VK_CONTROL:
		    wVirtKey = 0;
		    goto SetNewHotKey;

		default:
		    wVirtKey = wParam;
SetNewHotKey:
		    wMods = 0;
		    if (GetKeyState(VK_CONTROL) < 0)
			wMods |= HOTKEYF_CONTROL;
		    if (GetKeyState(VK_SHIFT) < 0)
			wMods |= HOTKEYF_SHIFT;
		    if (GetKeyState(VK_MENU) < 0)
                        wMods |= HOTKEYF_ALT;

                    #define IsFUNKEY(vk) ((vk) >= VK_F1 && (vk) <= VK_F24)
                    #define IsNUMKEY(vk) ((vk) >= VK_NUMPAD0 && (vk) <= VK_DIVIDE)

                    //
                    //  dont enforce any rules on the Function keys or
                    //  on the number pad keys.
                    //
		    // if this combination is invalid, use the default
                    if (!IsFUNKEY(wVirtKey) &&
                        !IsNUMKEY(wVirtKey) &&
                        (s_Combos[wMods] & GetWindowInt(hwnd, GWU_INVALID)))
                    {
                        wMods = (WORD)GetWindowInt(hwnd, GWU_DEFAULT);
                    }

		    if (lParam & F_EXT)
			wMods |= HOTKEYF_EXT;

		    SetHotKey(hwnd, wVirtKey, wMods, TRUE);
		    break;
	    }
	    break;

	case WM_SYSKEYUP:
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_KEYUP:
	    if (!GetWindowInt(hwnd, GWU_VIRTKEY))
		SetHotKey(hwnd, 0, 0, TRUE);
	    break;

	case WM_GETFONT:
	    return GetWindowInt(hwnd,GWU_HFONT);

	case WM_SETFONT:
            return (LRESULT)(UINT)HKMSetFont(hwnd, (HFONT)wParam);

	case WM_PAINT:
	    PaintHotKey(hwnd);
	    break;

	case WM_ERASEBKGND:
	    HideCaret(hwnd);
	    hdc = GetDC(hwnd);
            GetClientRect(hwnd, &rc);
	    if (IsWindowEnabled(hwnd)) {
                FillRect(hdc, &rc, GetSysColorBrush(COLOR_WINDOW));
            } else {
		FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));	
            }
	    ReleaseDC(hwnd, hdc);
	    // lParam = DefWindowProc(hwnd,wMsg,wParam,lParam);
	    ShowCaret(hwnd);
            return TRUE;

	case WM_ENABLE:
       	    InvalidateRect(hwnd, NULL, TRUE);
       	    
	default:
	    return DefWindowProc(hwnd,wMsg,wParam,lParam);
    }
    return 0L;
}
