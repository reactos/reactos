#include "editor.h"

#define DEFINE_THISCALL_WRAPPER(func,args) \
    typedef struct {int x[args/4];} _tag_##func; \
    void __stdcall func(_tag_##func p1); \
    __declspec(naked) void __stdcall __thiscall_##func(_tag_##func p1) \
    { \
        __asm pop eax \
        __asm push ecx \
        __asm push eax \
        __asm jmp func \
    }

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetDC,4)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxReleaseDC,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowScrollBar,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxEnableScrollBar,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollRange,20)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollPos,16)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxInvalidateRect,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxViewChange,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxCreateCaret,16)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowCaret,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCaretPos,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetTimer,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxKillTimer,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScrollWindowEx,32)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCapture,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetFocus,4)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCursor,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScreenToClient,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxClientToScreen,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxActivate,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxDeactivate,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetClientRect,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetViewInset,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetCharFormat,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetParaFormat,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSysColor,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetBackStyle,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetMaxLength,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetScrollBars,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPasswordChar,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetAcceleratorPos,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetExtent,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxCharFormatChange,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxParaFormatChange,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPropertyBits,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxNotify,12)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmGetContext,4)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmReleaseContext,8)
DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSelectionBarWidth,8)

#define DEFINE_STDCALL_WRAPPER(num,func,args) \
    __declspec(naked) void __stdcall __stdcall_##func(_tag_##func p1) \
    { \
        __asm pop eax \
        __asm pop ecx \
        __asm push eax \
        __asm mov eax, [ecx] \
        __asm jmp dword ptr [eax + 4*num] \
    }

DEFINE_STDCALL_WRAPPER(3,ITextHostImpl_TxGetDC,4)
DEFINE_STDCALL_WRAPPER(4,ITextHostImpl_TxReleaseDC,8)
DEFINE_STDCALL_WRAPPER(5,ITextHostImpl_TxShowScrollBar,12)
DEFINE_STDCALL_WRAPPER(6,ITextHostImpl_TxEnableScrollBar,12)
DEFINE_STDCALL_WRAPPER(7,ITextHostImpl_TxSetScrollRange,20)
DEFINE_STDCALL_WRAPPER(8,ITextHostImpl_TxSetScrollPos,16)
DEFINE_STDCALL_WRAPPER(9,ITextHostImpl_TxInvalidateRect,12)
DEFINE_STDCALL_WRAPPER(10,ITextHostImpl_TxViewChange,8)
DEFINE_STDCALL_WRAPPER(11,ITextHostImpl_TxCreateCaret,16)
DEFINE_STDCALL_WRAPPER(12,ITextHostImpl_TxShowCaret,8)
DEFINE_STDCALL_WRAPPER(13,ITextHostImpl_TxSetCaretPos,12)
DEFINE_STDCALL_WRAPPER(14,ITextHostImpl_TxSetTimer,12)
DEFINE_STDCALL_WRAPPER(15,ITextHostImpl_TxKillTimer,8)
DEFINE_STDCALL_WRAPPER(16,ITextHostImpl_TxScrollWindowEx,32)
DEFINE_STDCALL_WRAPPER(17,ITextHostImpl_TxSetCapture,8)
DEFINE_STDCALL_WRAPPER(18,ITextHostImpl_TxSetFocus,4)
DEFINE_STDCALL_WRAPPER(19,ITextHostImpl_TxSetCursor,12)
DEFINE_STDCALL_WRAPPER(20,ITextHostImpl_TxScreenToClient,8)
DEFINE_STDCALL_WRAPPER(21,ITextHostImpl_TxClientToScreen,8)
DEFINE_STDCALL_WRAPPER(22,ITextHostImpl_TxActivate,8)
DEFINE_STDCALL_WRAPPER(23,ITextHostImpl_TxDeactivate,8)
DEFINE_STDCALL_WRAPPER(24,ITextHostImpl_TxGetClientRect,8)
DEFINE_STDCALL_WRAPPER(25,ITextHostImpl_TxGetViewInset,8)
DEFINE_STDCALL_WRAPPER(26,ITextHostImpl_TxGetCharFormat,8)
DEFINE_STDCALL_WRAPPER(27,ITextHostImpl_TxGetParaFormat,8)
DEFINE_STDCALL_WRAPPER(28,ITextHostImpl_TxGetSysColor,8)
DEFINE_STDCALL_WRAPPER(29,ITextHostImpl_TxGetBackStyle,8)
DEFINE_STDCALL_WRAPPER(30,ITextHostImpl_TxGetMaxLength,8)
DEFINE_STDCALL_WRAPPER(31,ITextHostImpl_TxGetScrollBars,8)
DEFINE_STDCALL_WRAPPER(32,ITextHostImpl_TxGetPasswordChar,8)
DEFINE_STDCALL_WRAPPER(33,ITextHostImpl_TxGetAcceleratorPos,8)
DEFINE_STDCALL_WRAPPER(34,ITextHostImpl_TxGetExtent,8)
DEFINE_STDCALL_WRAPPER(35,ITextHostImpl_OnTxCharFormatChange,8)
DEFINE_STDCALL_WRAPPER(36,ITextHostImpl_OnTxParaFormatChange,8)
DEFINE_STDCALL_WRAPPER(37,ITextHostImpl_TxGetPropertyBits,12)
DEFINE_STDCALL_WRAPPER(38,ITextHostImpl_TxNotify,12)
DEFINE_STDCALL_WRAPPER(39,ITextHostImpl_TxImmGetContext,4)
DEFINE_STDCALL_WRAPPER(40,ITextHostImpl_TxImmReleaseContext,8)
DEFINE_STDCALL_WRAPPER(41,ITextHostImpl_TxGetSelectionBarWidth,8)

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSendMessage,20)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxDraw,52)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetHScroll,24)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetVScroll,24)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxSetCursor,40)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxQueryHitPoint,44)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceActivate,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInplaceDeactivate,4)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIActivate,4)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIDeactivate,4)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetText,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSetText,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCurTargetX,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetBaseLinePos,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetNaturalSize,36)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetDropTarget,8)
DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxPropertyBitsChange,12)
DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCachedSize,12)
