;   ovlhdr.inc
;
;   This header file defines the structure at the beginning of the
;   overlay manager stub.  This structure contains information needed
;   by the codeview loader to load and relocate the exe file.

OVLHDR	struc

                db  ?               ;reserve space for the jump instruction
                dw  ?               ; around the structure

chSignature     db  'MSDX'          ;signature bytes used to indicate that

wDXVersion	dw  ?		;version of the dos extender this exe was
				; built to run under
fLoadOptions	dw  ?		;flag bit specifying dos extender options

segDgroup	dw  ?		    ;relative segment DGROUP is exe file image
endDgroup	dw  ?		    ;number of bytes in dgroup
npExeName       dw  ?               ;near pointer to exe file name string
npIntNo         dw  ?               ;near pointer to variable containing
                                    ; interrupt to use
mpGsnBase       dw  ?               ;near pointer to table mapping global
                                    ; segment number to relative segment base
                                    ; address
mpGsnOvl        dw  ?               ;near pointer to table mapping global
                                    ; segment number to overlay number
mpOvlLfa        dw  ?               ;near pointer to table mapping overlay
                                    ; number to file address
lpfnMain        dd  ?               ;far pointer to program entry point
npcGSN          dw  ?               ;near pointer to variable containing
                                    ; count of global segments
npcOVL          dw  ?               ;near pointer to variable containing
                                    ; count of overlays in exe file
segOvlBase      dw  ?               ;relative segment of start of overlay area
segOvlEnd       dw  ?               ;relative segment of end of overlay area

cOvlType	db  ?		    ;[1]overlay type 'D', 'E', or 'X'
szOvlHeap	dw  ?		    ;[1]size in Kbytes of overlay heap for 'D'

offOvlClr	dw  ?		    ;[2]offset of overlay stack clear callback
segOvlClr	dw  ?		    ;[2]segment of callback

offLoadMod	dw  ?		    ;[4]offset of CVLoadModule function
segLoadMod	dw  ?		    ;[4]segment of CVLoadModule function

offCallProc	dw  ?		    ;[5]offset of dll proc call thunk
segCallProc	dw  ?		    ;[5]segment of dll proc call thunk

offEnterExe	dw  ?
segEnterExe	dw  ?

offExitExe	dw  ?
segExitExe	dw  ?

cProcs		dw  ?		    ;number of api procs
mpProcHDR	dd  31 dup (?)	    ;first api proc pointer
		db  31 dup (?)	    ; overhead of call
OVLHDR	ends
