        title  "Debug Support Functions"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    debug.s
;
; Abstract:
;
;    This module implements functions to support debugging NT.
;
; Author:
;
;    Steven R. Wood (stevewo) 3-Aug-1989
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;   11 April 90 (and before)    bryanwi
;       Ported to 386, 386 specific support added.
;
;   2  Aug.  90    (tomp)
;       Added _DbgUnLoadImageSymbols routine.
;
;--
.386p


        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

_TEXT	SEGMENT PUBLIC DWORD 'CODE'
ASSUME  DS:FLAT, ES:FLAT, FS:NOTHING, GS:NOTHING, SS:NOTHING

cPublicProc _DbgBreakPoint        ,0
cPublicFpo 0,0
        int 3
        stdRET    _DbgBreakPoint
stdENDP _DbgBreakPoint

cPublicProc _DbgUserBreakPoint        ,0
cPublicFpo 0,0
        int 3
        stdRET    _DbgUserBreakPoint
stdENDP _DbgUserBreakPoint

cPublicProc _DbgBreakPointWithStatus,1
cPublicFpo 1,0
        mov eax,[esp+4]
        public _RtlpBreakWithStatusInstruction@0
_RtlpBreakWithStatusInstruction@0:
        int 3
        stdRET  _DbgBreakPointWithStatus
stdENDP _DbgBreakPointWithStatus


_TEXT   ends
        end
