/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/wndproc_fixup.S
 * PURPOSE:         Messages
 * PROGRAMMER:      Jérôme Gardou jerome.gardou@reactos.org
 * LICENCE :        LGPL, copyright Alexandre Julliard
 */

#include <asm.inc>
#include <ks386.inc>

/* Some applications provide invalid callbacks which don't follow the stdcall convention */

.code

/* 
 * LRESULT
 * __cdecl 
 * CALL_EXTERN_WNDPROC(
 *     WNDPROC WndProc,
 *     HWND hWnd,
 *     UINT Msg,
 *     WPARAM wParam,
 *     LPARAM lParam);
 */
PUBLIC _CALL_EXTERN_WNDPROC
FUNC _CALL_EXTERN_WNDPROC
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    push ebp
    mov ebp, esp

    push edi
    push esi
    push ebp

    sub esp, 12

    push dword ptr [ebp + 24]
    push dword ptr [ebp + 20]
    push dword ptr [ebp + 16]
    push dword ptr [ebp + 12]
    mov eax, dword ptr [ebp + 8]

    call eax

    lea esp, dword ptr [ebp - 12]
    pop ebx
    pop esi
    pop edi

    leave
    ret

ENDFUNC

END
