#pragma once

#define HEX_EDIT_CLASS_NAME     L"HexEdit32"

ATOM WINAPI
RegisterHexEditorClass(HINSTANCE hInstance);
BOOL WINAPI
UnregisterHexEditorClass(HINSTANCE hInstance);

/* styles */
#define HES_READONLY    (0x800)
#define HES_LOWERCASE   (0x10)
#define HES_UPPERCASE   (0x8)
#define HES_AUTOVSCROLL (0x40)
#define HES_HIDEADDRESS (0x4)

/* messages */
#define HEM_BASE                (WM_USER + 50)
#define HEM_LOADBUFFER          (HEM_BASE + 1)
#define HEM_COPYBUFFER          (HEM_BASE + 2)
#define HEM_SETMAXBUFFERSIZE    (HEM_BASE + 3)

/* macros */
#define HexEdit_LoadBuffer(hWnd, Buffer, Size) \
  SendMessageW((hWnd), HEM_LOADBUFFER, (WPARAM)(Buffer), (LPARAM)(Size))

#define HexEdit_ClearBuffer(hWnd) \
  SendMessageW((hWnd), HEM_LOADBUFFER, 0, 0)

#define HexEdit_CopyBuffer(hWnd, Buffer, nMax) \
  SendMessageW((hWnd), HEM_COPYBUFFER, (WPARAM)(Buffer), (LPARAM)(nMax))

#define HexEdit_GetBufferSize(hWnd) \
  SendMessageW((hWnd), HEM_COPYBUFFER, 0, 0)

#define HexEdit_SetMaxBufferSize(hWnd, Size) \
  SendMessageW((hWnd), HEM_SETMAXBUFFERSIZE, 0, (LPARAM)(Size))

/* EOF */
