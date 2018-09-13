; Callback-type id's for COMMDLG callbacks. All callback-type id's are
; 32-bits long and the upper 16-bits must be set to the module's
; Callback Client id.
;
; WARNING -- WARNING -- WARNING
;
;   Win32c.dll uses the lower 16 bits of CBID's to index directly into
;   the tables stored in COMDLG.EXE (dlgcb16.asm) and COMDLG32.DLL 
;   (dlgcb32.asm). Those tables *must* match these id's or callbacks 
;   will fail miserably. Do everyone a big favor and add new id's only 
;   at the end, and never change an existing id.
;   


CBID_OPENFILENAME	equ	(0 + (CBCID_COMMDLG SHL 16))
CBID_CHOOSEFONT		equ	(1 + (CBCID_COMMDLG SHL 16))
CBID_CHOOSECOLOR	equ	(2 + (CBCID_COMMDLG SHL 16))
CBID_PRINTDLG		equ	(3 + (CBCID_COMMDLG SHL 16))



