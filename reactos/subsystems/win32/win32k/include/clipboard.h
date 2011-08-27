#pragma once

typedef struct _ClipboardChainElement
{
    PWND                 window;
    struct _ClipboardChainElement *next;
} CLIPBOARDCHAINELEMENT, *PCLIPBOARDCHAINELEMENT;

typedef struct _ClipboardElement
{
    UINT                        format;
    HANDLE                      hData;
    DWORD                       size;   // data may be delayed o synth render
    struct _ClipboardElement   *next;
} CLIPBOARDELEMENT, *PCLIPBOARDELEMENT;

typedef struct _CLIPBOARDSYSTEM
{
    PTHREADINFO     ClipboardThread;
    PTHREADINFO     ClipboardOwnerThread;
    PWND  ClipboardWindow;
    PWND  ClipboardViewerWindow;
    PWND  ClipboardOwnerWindow;
    BOOL            sendDrawClipboardMsg;
    BOOL            recentlySetClipboard;
    BOOL            delayedRender;
    UINT            lastEnumClipboardFormats;
    DWORD           ClipboardSequenceNumber;

    PCLIPBOARDCHAINELEMENT WindowsChain;
    PCLIPBOARDELEMENT      ClipboardData;

    PCHAR synthesizedData;
    DWORD synthesizedDataSize;

} CLIPBOARDSYSTEM, *PCLIPBOARDSYSTEM;

VOID FASTCALL IntClipboardFreeWindow(PWND window);
UINT APIENTRY IntEnumClipboardFormats(UINT format);
VOID FASTCALL IntIncrementSequenceNumber(VOID);
