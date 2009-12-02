#include "stdafx.h"

#include <zmouse.h>

#include "rdesktop/rdesktop.h"
#include "rdesktop/proto.h"

extern "C"
{
	/* ==== BEGIN POOP ==== */
	// Temporary implementations of stuff we totally positively need to make the Real Thing happy
	/* produce a hex dump */
	void
	hexdump(unsigned char *p, unsigned int len)
	{
		unsigned char *line = p;
		int i, thisline;
		unsigned int offset = 0;

		while (offset < len)
		{
			printf("%04x ", offset);
			thisline = len - offset;
			if (thisline > 16)
				thisline = 16;

			for (i = 0; i < thisline; i++)
				printf("%02x ", line[i]);

			for (; i < 16; i++)
				printf("   ");

			for (i = 0; i < thisline; i++)
				printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

			printf("\n");
			offset += thisline;
			line += thisline;
		}
	}

	void generate_random(uint8 * random)
	{
		memcpy(random, "12345678901234567890123456789012", 32);
	}

	/* report an error */
	void
	error(char *format, ...)
	{
		va_list ap;

		fprintf(stderr, "ERROR: ");

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}

	/* report a warning */
	void
	warning(char *format, ...)
	{
		va_list ap;

		fprintf(stderr, "WARNING: ");

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}

	/* report an unimplemented protocol feature */
	void
	unimpl(char *format, ...)
	{
		va_list ap;

		fprintf(stderr, "NOT IMPLEMENTED: ");

		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}

	/* Create the bitmap cache directory */
	BOOL
	rd_pstcache_mkdir(void)
	{
		char *home;
		char bmpcache_dir[256];

		home = getenv("HOME");

		if (home == NULL)
			return False;

		sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop");

		if ((_mkdir(bmpcache_dir) == -1) && errno != EEXIST)
		{
			perror(bmpcache_dir);
			return False;
		}

		sprintf(bmpcache_dir, "%s/%s", home, ".rdesktop/cache");

		if ((_mkdir(bmpcache_dir) == -1) && errno != EEXIST)
		{
			perror(bmpcache_dir);
			return False;
		}

		return True;
	}

	/* open a file in the .rdesktop directory */
	int
	rd_open_file(char *filename)
	{
		char *home;
		char fn[256];
		int fd;

		home = getenv("HOME");
		if (home == NULL)
			return -1;
		sprintf(fn, "%s/.rdesktop/%s", home, filename);
		fd = _open(fn, _O_RDWR | _O_CREAT, 0);
		if (fd == -1)
			perror(fn);
		return fd;
	}

	/* close file */
	void
	rd_close_file(int fd)
	{
		_close(fd);
	}

	/* read from file*/
	int
	rd_read_file(int fd, void *ptr, int len)
	{
		return _read(fd, ptr, len);
	}

	/* write to file */
	int
	rd_write_file(int fd, void *ptr, int len)
	{
		return _write(fd, ptr, len);
	}

	/* move file pointer */
	int
	rd_lseek_file(int fd, int offset)
	{
		return _lseek(fd, offset, SEEK_SET);
	}

	/* do a write lock on a file */
	BOOL
	rd_lock_file(int fd, int start, int len)
	{
		// TODOOO...
		return False;
	}

	int
	load_licence(RDPCLIENT * This, unsigned char **data)
	{
		return -1;
	}

	void
	save_licence(RDPCLIENT * This, unsigned char *data, int length)
	{
	}

	/* ==== END POOP ==== */

	/* ==== UI ==== */
	// Globals are totally teh evil, but cut me some slack here
	HWND hwnd;
	HBITMAP hbmBuffer;
	PVOID pBuffer;
	HDC hdcBuffer;
	UINT wmZMouseWheel;

};

static
void
mstsc_mousewheel(RDPCLIENT * This, int value, LPARAM lparam)
{
	uint16 button;

	if(value < 0)
		button = MOUSE_FLAG_BUTTON5;
	else
		button = MOUSE_FLAG_BUTTON4;

	if(value < 0)
		value = - value;

	for(int click = 0; click < value; click += WHEEL_DELTA)
		rdp_send_input(This, GetTickCount(), RDP_INPUT_MOUSE, button | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
}

static
LRESULT
CALLBACK
mstsc_WndProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
	// BUGBUG: LongToPtr & PtrToLong will break on Win64

	RDPCLIENT * This = reinterpret_cast<RDPCLIENT *>(LongToPtr(GetWindowLongPtr(hwnd, GWLP_USERDATA)));

	switch(uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

		// FIXME: temporary
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		/* Initialization */
	case WM_CREATE:
		This = static_cast<RDPCLIENT *>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, PtrToLong(This));
		break;

		/* Painting */
	case WM_PRINTCLIENT:
		if(wparam == 0)
			break;

	case WM_PAINT:
		{
			HDC hdc = (HDC)wparam;

			// A DC was provided: print the whole client area into it
			if(hdc)
			{
				RECT rc;
				GetClientRect(hwnd, &rc);
				BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcBuffer, 0, 0, SRCCOPY);
			}
			// Otherwise, we're refreshing to screen
			else
			{
				PAINTSTRUCT ps;
				hdc = BeginPaint(hwnd, &ps);

				BitBlt
				(
					hdc,
					ps.rcPaint.left,
					ps.rcPaint.top,
					ps.rcPaint.right - ps.rcPaint.left,
					ps.rcPaint.bottom - ps.rcPaint.top,
					hdcBuffer,
					ps.rcPaint.left,
					ps.rcPaint.top,
					SRCCOPY
				);

				EndPaint(hwnd, &ps);
			}
		}

		break;

		/* Keyboard stuff */
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_SCANCODE, RDP_KEYPRESS | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0), LOBYTE(HIWORD(lparam)), 0);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_SCANCODE, RDP_KEYRELEASE | (lparam & 0x1000000 ? KBD_FLAG_EXT : 0), LOBYTE(HIWORD(lparam)), 0);
		break;

		/* Mouse stuff */
		// Cursor shape
	case WM_SETCURSOR:
		if(LOWORD(lparam) == HTCLIENT)
		{
			//SetCursor(hcursor);
			return TRUE;
		}

		break;

		// Movement
	case WM_MOUSEMOVE:
		//if(This->sendmotion || wparam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2))
			rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, LOWORD(lparam), HIWORD(lparam));

		break;

		// Buttons
		// TODO: X buttons
	case WM_LBUTTONDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_RBUTTONDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_MBUTTONDOWN:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3 | MOUSE_FLAG_DOWN, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_LBUTTONUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_RBUTTONUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2, LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_MBUTTONUP:
		rdp_send_input(This, GetMessageTime(), RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3, LOWORD(lparam), HIWORD(lparam));
		break;

		// Wheel
	case WM_MOUSEWHEEL:
		mstsc_mousewheel(This, (SHORT)HIWORD(wparam), lparam);
		break;

	default:
		/* Registered messages */
		// Z-Mouse wheel support - you know, just in case
		if(uMsg == wmZMouseWheel)
		{
			mstsc_mousewheel(This, (int)wparam, lparam);
			break;
		}

		/* Unhandled messages */
		return DefWindowProc(hwnd, uMsg, wparam, lparam);
	}

	return 0;
}

static
DWORD
WINAPI
mstsc_ProtocolIOThread
(
	LPVOID lpArgument
)
{
	RDPCLIENT * This = static_cast<RDPCLIENT *>(lpArgument);

	WCHAR hostname[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dw = ARRAYSIZE(hostname);
	GetComputerNameW(hostname, &dw);

	uint32 flags = RDP_LOGON_NORMAL | RDP_LOGON_COMPRESSION | RDP_LOGON_COMPRESSION2;

	rdp_connect(This, "10.0.0.3", flags, L"Administrator", L"", L"", L"", L"", hostname, "");
	//rdp_connect(This, "192.168.7.232", flags, "", "", "", "");

	hdcBuffer = CreateCompatibleDC(NULL);

	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = This->width;
	bmi.bmiHeader.biHeight = This->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = This->server_depth;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0; // TODO! palette displays
	bmi.bmiHeader.biClrImportant = 0; // TODO! palette displays

	hbmBuffer = CreateDIBSection(hdcBuffer, &bmi, DIB_RGB_COLORS, &pBuffer, NULL, 0);

	SelectObject(hdcBuffer, hbmBuffer);

#if 0
	rcClip.left = 0;
	rcClip.top = 0;
	rcClip.right = This->width + 1;
	rcClip.bottom = This->height + 1;
#endif

	BOOL deactivated;
	uint32 ext_disc_reason;

	rdp_main_loop(This, &deactivated, &ext_disc_reason);
	// TODO: handle redirection
	// EVENT: OnDisconnect

	SendMessage(hwnd, WM_CLOSE, 0, 0);

	return 0;
}

/* Virtual channel stuff */
extern "C" void channel_process(RDPCLIENT * This, STREAM s, uint16 mcs_channel)
{
}

DWORD tlsIndex;

typedef struct CHANNEL_HANDLE_
{
	RDPCLIENT * client;
	int channel;
}
CHANNEL_HANDLE;

static
UINT
VCAPITYPE
VirtualChannelInit
(
	LPVOID * ppInitHandle,
	PCHANNEL_DEF pChannel,
	INT channelCount,
	ULONG versionRequested,
	PCHANNEL_INIT_EVENT_FN pChannelInitEventProc
)
{
	if(channelCount <= 0)
		return CHANNEL_RC_BAD_CHANNEL;

	if(ppInitHandle == NULL)
		return CHANNEL_RC_BAD_INIT_HANDLE;

	if(pChannel == NULL)
		return CHANNEL_RC_BAD_CHANNEL;

	if(pChannelInitEventProc == NULL)
		return CHANNEL_RC_BAD_PROC;

	RDPCLIENT * This = (RDPCLIENT *)TlsGetValue(tlsIndex);

	if(This == NULL)
		return CHANNEL_RC_NOT_IN_VIRTUALCHANNELENTRY;

	if(This->num_channels + channelCount > CHANNEL_MAX_COUNT)
		return CHANNEL_RC_TOO_MANY_CHANNELS;

	for(INT i = 0; i < channelCount; ++ i)
	{
		if(strlen(pChannel[i].name) > CHANNEL_NAME_LEN)
			return CHANNEL_RC_BAD_CHANNEL;
	}

	memcpy(This->channel_defs + This->num_channels, pChannel, sizeof(*pChannel) * channelCount);

#if 0 // TODO
	for(INT i = 0; i < channelCount; ++ i)
	{
		pChannel[i].options |= CHANNEL_OPTION_INITIALIZED;

		int j = This->num_channels + i;
		This->channel_data[j].opened = 0;
		This->channel_data[j].pChannelInitEventProc = pChannelInitEventProc;
		This->channel_data[j].pChannelOpenEventProc = NULL;
	}
#endif

	This->num_channels += channelCount;

	*ppInitHandle = This;

	return CHANNEL_RC_OK;
}

UINT
VCAPITYPE
VirtualChannelOpen
(
	LPVOID pInitHandle,
	LPDWORD pOpenHandle,
	PCHAR pChannelName,
	PCHANNEL_OPEN_EVENT_FN pChannelOpenEventProc
)
{
	if(pInitHandle == NULL)
		return CHANNEL_RC_BAD_INIT_HANDLE;

	if(pOpenHandle == NULL)
		return CHANNEL_RC_BAD_CHANNEL_HANDLE;

	if(pChannelName == NULL)
		return CHANNEL_RC_UNKNOWN_CHANNEL_NAME;

	if(pChannelOpenEventProc == NULL)
		return CHANNEL_RC_BAD_PROC;

	RDPCLIENT * This = (RDPCLIENT *)pInitHandle;

#if 0 // TODO
	for(unsigned i = 0; i < This->num_channels; ++ i)
	{
		if(strcmp(pChannelName, This->channel_defs[i].name) == 0)
		{
			if(This->channel_data[i].opened)
				return CHANNEL_RC_ALREADY_OPEN;

			This->channel_data[i].opened = 1;
			This->channel_data[i].pChannelOpenEventProc = pChannelOpenEventProc;

			// TODO: allocate a handle here
			*pOpenHandle = 0;

			break;
		}
	}
#endif

	return CHANNEL_RC_OK;
}

UINT VCAPITYPE VirtualChannelClose
(
	DWORD openHandle
)
{
	// TODO: channel handle management
	return CHANNEL_RC_BAD_CHANNEL_HANDLE;
}

UINT VCAPITYPE VirtualChannelWrite
(
	DWORD openHandle,
	LPVOID pData,
	ULONG dataLength,
	LPVOID pUserData
)
{
	// TODO
	return CHANNEL_RC_BAD_CHANNEL_HANDLE;
}

int wmain()
{
	WSADATA wsd;
	WSAStartup(MAKEWORD(2, 2), &wsd);

	static RDPCLIENT This_; // NOTE: this is HUGE and would overflow the stack!
	ZeroMemory(&This_, sizeof(This_));

	RDPCLIENT * This = &This_;

	/*
		Threading model for MissTosca:
		 - main thread is the GUI thread. Message loop maintained by caller
		 - protocol I/O is handled in an I/O thread (or thread pool)
		 - extra threads maintained by virtual channel handlers. Virtual channel writes are thread-neutral

		How we handle drawing: at the moment just an off-screen buffer we dump on-screen when asked to.
		Still considering how to draw on-screen directly and *then* buffering off-screen (for example,
		when running inside another remote session)
	*/

	// FIXME: keyboard mess
	This->keylayout = 0x409;
	This->keyboard_type = 0x4;
	This->keyboard_subtype = 0x0;
	This->keyboard_functionkeys = 0xc;
	This->width = 800;
	This->height = 600;
	This->server_depth = 24;
	This->bitmap_compression = True;
	//This->sendmotion = True;
	This->bitmap_cache = True;
	This->bitmap_cache_persist_enable = False;
	This->bitmap_cache_precache = True;
	This->encryption = True;
	This->packet_encryption = True;
	This->desktop_save = True;
	This->polygon_ellipse_orders = False; // = True;
	//This->fullscreen = False;
	//This->grab_keyboard = True;
	//This->hide_decorations = False;
	This->use_rdp5 = True;
	//This->rdpclip = True;
	This->console_session = False;
	//This->numlock_sync = False;
	//This->seamless_rdp = False;
	This->rdp5_performanceflags = RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
	This->tcp_port_rdp = TCP_PORT_RDP;

#define NOT_SET -1
	This->cache.bmpcache_lru[0] = NOT_SET;
	This->cache.bmpcache_lru[1] = NOT_SET;
	This->cache.bmpcache_lru[2] = NOT_SET;
	This->cache.bmpcache_mru[0] = NOT_SET;
	This->cache.bmpcache_mru[1] = NOT_SET;
	This->cache.bmpcache_mru[2] = NOT_SET;

	This->rdp.current_status = 1;

	//hcursor = NULL;

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.lpfnWndProc = mstsc_WndProc;
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
	wc.lpszClassName = TEXT("MissTosca_Desktop");

	wmZMouseWheel = RegisterWindowMessage(MSH_MOUSEWHEEL);

	ATOM a = RegisterClass(&wc);

	hwnd = CreateWindow
	(
		MAKEINTATOM(a),
		NULL,
		WS_POPUP | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		This->width,
		This->height,
		NULL,
		NULL,
		NULL,
		This
	);

	// The righ time to start the protocol thread
	DWORD dwThreadId;
	HANDLE hThread = CreateThread(NULL, 0, mstsc_ProtocolIOThread, This, 0, &dwThreadId);

	// Your standard, garden variety message loop
	MSG msg;

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

// EOF
