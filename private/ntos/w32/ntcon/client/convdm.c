/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    vdm.c

Abstract:

    This module contains the console API for MVDM.

Author:


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

BOOL
APIENTRY
VDMConsoleOperation(
    DWORD  iFunction,
    LPVOID lpData
    )

/*++

Parameters:

    iFunction - Function Index.
    VDM_HIDE_WINDOW

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_VDM_MSG a = &m.u.VDMConsoleOperation;
    LPRECT lpRect;
    LPPOINT lpPoint;
    PBOOL lpBool;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;
#if defined(FE_SB)
#if defined(i386)
    LPVDM_IOCTL_PARAM lpIoctlParam;
#endif
#endif // FE_SB

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->iFunction = iFunction;
    if (iFunction == VDM_CLIENT_TO_SCREEN ||
        iFunction == VDM_SCREEN_TO_CLIENT) {
        lpPoint = (LPPOINT)lpData;
        a->Point = *lpPoint;
    } else if (iFunction == VDM_FULLSCREEN_NOPAINT) {
        a->Bool = (lpData != NULL);
    }
#if defined(FE_SB)
    else if (iFunction == VDM_SET_VIDEO_MODE) {
        a->Bool = (lpData != NULL);
    }
#if defined(i386)
    else if (iFunction == VDM_SAVE_RESTORE_HW_STATE) {
        a->Bool = (lpData != NULL);
    }
    else if (iFunction == VDM_VIDEO_IOCTL) {
        lpIoctlParam = (LPVDM_IOCTL_PARAM)lpData;
        a->VDMIoctlParam = *lpIoctlParam;
        if (lpIoctlParam->lpvInBuffer != NULL) {
            if (lpIoctlParam->lpvOutBuffer != NULL || lpIoctlParam->cbInBuffer == 0) {
                SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                      lpIoctlParam->cbInBuffer
                                                    );
            if (CaptureBuffer == NULL) {
                SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR) lpIoctlParam->lpvInBuffer,
                                     lpIoctlParam->cbInBuffer,
                                     (PVOID *) &a->VDMIoctlParam.lpvInBuffer
                                   );
        }
        if (lpIoctlParam->lpvOutBuffer != NULL) {
            if (lpIoctlParam->cbOutBuffer == 0) {
                SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                      lpIoctlParam->cbOutBuffer
                                                    );
            if (CaptureBuffer == NULL) {
                SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR) lpIoctlParam->lpvOutBuffer,
                                     lpIoctlParam->cbOutBuffer,
                                     (PVOID *) &a->VDMIoctlParam.lpvOutBuffer
                                   );
        }
    }
#endif // i386
#endif // FE_SB
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                         ConsolepVDMOperation
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        switch (iFunction) {
            case VDM_IS_ICONIC:
            case VDM_IS_HIDDEN:
                lpBool = (PBOOL)lpData;
                *lpBool = a->Bool;
                break;
            case VDM_CLIENT_RECT:
                lpRect = (LPRECT)lpData;
                *lpRect = a->Rect;
                break;
            case VDM_CLIENT_TO_SCREEN:
            case VDM_SCREEN_TO_CLIENT:
                *lpPoint = a->Point;
                break;
#if defined(FE_SB)
#if defined(i386)
            case VDM_VIDEO_IOCTL:
                if (lpIoctlParam->lpvOutBuffer != NULL) {
                    try {
                        RtlCopyMemory( lpIoctlParam->lpvOutBuffer,
                                       a->VDMIoctlParam.lpvOutBuffer,
                                       lpIoctlParam->cbOutBuffer );
                    } except( EXCEPTION_EXECUTE_HANDLER ) {
                        CsrFreeCaptureBuffer( CaptureBuffer );
                        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
                        return FALSE;
                    }
                }
                if (CaptureBuffer != NULL) {
                    CsrFreeCaptureBuffer( CaptureBuffer );
                }
                break;
#endif // i386
#endif // FE_SB
            default:
                break;
        }
        return TRUE;
    } else {
#if defined(FE_SB)
#if defined(i386)
        if (CaptureBuffer != NULL) {
            CsrFreeCaptureBuffer( CaptureBuffer );
        }
#endif // i386
#endif // FE_SB
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }
}
