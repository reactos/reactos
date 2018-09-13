;----------------------------------------------------------------------------
; DVADDDI.INC
;
; Copyright (c) 1993 Microsoft Corporation
;
; DVA 1.0 Interface Definitions
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; E Q U A T E S
;----------------------------------------------------------------------------

DVAGETSURFACE       equ     3074d   ; GDI Escape for DVA
DVA_VERSION         equ     0100h   ; version number of DVA 1.0

;----------------------------------------------------------------------------
; DVASURFACEINFO structure
;----------------------------------------------------------------------------
DVASURFACEINFO  struc
  dvaBitmapInfo     db  40 dup(?)       ; BITMAPINFO of surface
  dvaMasks          dd  3 dup(?)        ; masks for BI_BITFIELDS
  dvaOffSurface     dd  ?               ; surface offset
  dvaSelSurface     dw  ?               ; surface selector
  dvaVersion        dw  ?               ; DVA Version
  dvaFlags          dd  ?               ; Flags
  dvaSurface        dd  ?               ; driver use.
  dvaOpenSurface    dd  ?               ; OpenSurface callback
  dvaCloseSurface   dd  ?               ; CloseSurface callback
  dvaBeginAccess    dd  ?               ; BeginAccess callback
  dvaEndAccess      dd  ?               ; EndAccess callback
  dvaShowSurface    dd  ?               ; ShowSurface callback
DVASURFACEINFO  ends

;----------------------------------------------------------------------------
; Definitions for DVASURFACEINFO.dvaFlags
;----------------------------------------------------------------------------

DVAF_1632_ACCESS    equ     0001h   ; must access using 16:32 pointers
