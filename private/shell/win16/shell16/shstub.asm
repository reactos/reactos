        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  SHSTUB.ASM
;-----------------------------------------------------------------------;

?PLM=1      ; PASCAL Calling convention is DEFAULT
?WIN=1      ; Windows calling convention

        .286p
        .xlist
        include cmacros.inc
        .list

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

;-----------------------------------------------------------------------;
;   data seg
;
sBegin  Data
        DB  16 dup (0)          ; reserved param area
sEnd    Data

public  __acrtused
        __acrtused = 1

;-----------------------------------------------------------------------;

sBegin  CodeSeg
        assumes cs,CodeSeg

        externFP    LocalInit           ; in KERNEL
        externFP    FreeLibrary         ; in KERNEL
;;      externFP    LibMain             ; C code to do DLL init

        externNP    LoadShell16         ; in SHINIT.C

;--------------------------Private-Routine-----------------------------;
;
; LibEntry - called when DLL is loaded
;
; Entry:
;       CX    = size of heap
;       DI    = module handle
;       DS    = automatic data segment
;       ES:SI = address of command line (not used)
;
; Returns:
;       AX = TRUE if success
; Error Returns:
;       AX = FALSE if error (ie fail load process)
; Registers Preserved:
;       SI,DI,DS,BP
; Registers Destroyed:
;       AX,BX,CX,DX,ES,FLAGS
; Calls:
;       None
; History:
;
;       06-27-89 -by-  Todd Laney [ToddLa]
;       Created.
;-----------------------------------------------------------------------;

cProc   LibEntry,<FAR,PUBLIC,NODATA>,<>
cBegin
        ;
        ; Push frame for LibMain (hModule,cbHeap,lpszCmdLine)
        ;
;;      push    di
;;      push    cx
;;      push    es
;;      push    si

        ;
        ; Init the local heap (if one is declared in the .def file)
        ;
        jcxz no_heap

        cCall   LocalInit,<0,0,cx>

no_heap:
;;      cCall   LibMain
        mov     ax,1
cEnd

sEnd    CodeSeg

;-----------------------------------------------------------------------;
; RTL STUF
;-----------------------------------------------------------------------;
    include rtl.inc

;;  RTL_PROC REGOPENKEY                     ,SHELL16 ,1       , LoadShell16 ,Y
;;  RTL_PROC REGCREATEKEY                   ,SHELL16 ,2       , LoadShell16 ,Y
;;  RTL_PROC REGCLOSEKEY                    ,SHELL16 ,3       , LoadShell16 ,Y
;;  RTL_PROC REGDELETEKEY                   ,SHELL16 ,4       , LoadShell16 ,Y
;;  RTL_PROC REGSETVALUE                    ,SHELL16 ,5       , LoadShell16 ,Y
;;  RTL_PROC REGQUERYVALUE                  ,SHELL16 ,6       , LoadShell16 ,Y
;;  RTL_PROC REGENUMKEY                     ,SHELL16 ,7       , LoadShell16 ,Y

;;  RTL_PROC DRAGACCEPTFILES                ,SHELL16 ,9       , LoadShell16 ,Y
;;  RTL_PROC DRAGQUERYFILE                  ,SHELL16 ,11      , LoadShell16 ,Y
;;  RTL_PROC DRAGFINISH                     ,SHELL16 ,12      , LoadShell16 ,Y
;;  RTL_PROC DRAGQUERYPOINT                 ,SHELL16 ,13      , LoadShell16 ,Y
;;  RTL_PROC DRAGQUERYINFO                  ,SHELL16 ,11      , LoadShell16 ,Y

    RTL_PROC SHELLEXECUTE                   ,SHELL16 ,20      , LoadShell16 ,Y
    RTL_PROC FINDEXECUTABLE                 ,SHELL16 ,21      , LoadShell16 ,Y
    RTL_PROC SHELLABOUT                     ,SHELL16 ,22      , LoadShell16 ,Y
    RTL_PROC EXTRACTICON                    ,SHELL16 ,34      , LoadShell16 ,Y
    RTL_PROC EXTRACTASSOCIATEDICON          ,SHELL16 ,36      , LoadShell16 ,Y
;;  RTL_PROC DOENVIRONMENTSUBST             ,SHELL16 ,37      , LoadShell16 ,Y
;;  RTL_PROC FINDENVIRONMENTSTRING          ,SHELL16 ,38      , LoadShell16 ,Y
    RTL_PROC INTERNALEXTRACTICON            ,SHELL16 ,39      , LoadShell16 ,Y

    RTL_PROC REGISTERSHELLHOOK              ,SHELL16 ,102     , LoadShell16 ,Y
    RTL_PROC RESTARTDIALOG                  ,SHELL16 ,157     , LoadShell16 ,Y

    RTL_PROC PARSEFIELD                     ,SHELL16 ,163     , LoadShell16 ,Y
    RTL_PROC ADDCOMMAS                      ,SHELL16 ,164     , LoadShell16 ,Y
    RTL_PROC PICKICONDLG                    ,SHELL16 ,166     , LoadShell16 ,Y

    RTL_PROC PATHISROOT                     ,SHELL16 ,170     , LoadShell16 ,Y
    RTL_PROC PATHBUILDROOT                  ,SHELL16 ,171     , LoadShell16 ,Y
    RTL_PROC PATHREMOVEBACKSLASH            ,SHELL16 ,172     , LoadShell16 ,Y
    RTL_PROC PATHADDBACKSLASH               ,SHELL16 ,173     , LoadShell16 ,Y
    RTL_PROC PATHREMOVEBLANKS               ,SHELL16 ,174     , LoadShell16 ,Y
    RTL_PROC PATHFINDFILENAME               ,SHELL16 ,175     , LoadShell16 ,Y
    RTL_PROC PATHREMOVEFILESPEC             ,SHELL16 ,176     , LoadShell16 ,Y
    RTL_PROC PATHAPPEND                     ,SHELL16 ,177     , LoadShell16 ,Y
    RTL_PROC PATHCOMBINE                    ,SHELL16 ,178     , LoadShell16 ,Y
    RTL_PROC PATHCOMMONPREFIX               ,SHELL16 ,179     , LoadShell16 ,Y
    RTL_PROC PATHSETDLGITEMPATH             ,SHELL16 ,189     , LoadShell16 ,Y
    RTL_PROC PATHGETARGS                    ,SHELL16 ,1120    , LoadShell16 ,Y
    RTL_PROC PATHGETSHORTNAME               ,SHELL16 ,1121    , LoadShell16 ,Y
    RTL_PROC PATHGETLONGNAME                ,SHELL16 ,1122    , LoadShell16 ,Y

    RTL_PROC DRIVETYPE                      ,SHELL16 ,262     , LoadShell16 ,Y
    RTL_PROC INVALIDATEDRIVETYPE            ,SHELL16 ,263     , LoadShell16 ,Y
    RTL_PROC ISNETDRIVE                     ,SHELL16 ,264     , LoadShell16 ,Y

    RTL_PROC _SHELLMESSAGEBOX               ,SHELL16 ,350     , LoadShell16 ,Y
    RTL_PROC GETFILENAMEFROMBROWSE          ,SHELL16 ,901     , LoadShell16 ,Y

end LibEntry
