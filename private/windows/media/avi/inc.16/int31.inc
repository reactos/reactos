;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1989-1990
;
;   Title:	INT31.INC - Equates and Structures for Int 31h Interface
;
;   Version:	3.00
;
;   Date:	22-May-1989
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   22-May-1989 RAL Original
;
;==============================================================================



Int31_Sel_Mgt		EQU	00h
    SelMgt_Alloc_Sel	EQU	00h
    SelMgt_Free_Sel	EQU	01h
    SelMgt_Seg_To_Sel	EQU	02h
    SelMgt_Get_LDT_Base EQU	03h
    SelMgt_Lock_Sel	EQU	04h
    SelMgt_Unlock_Sel	EQU	05h
    SelMgt_Get_Base	EQU	06h
    SelMgt_Set_Base	EQU	07h
    SelMgt_Set_Limit	EQU	08h
    SelMgt_Set_Acc_Bits EQU	09h
    SelMgt_Alias_Sel	EQU	0Ah
    SelMgt_Get_Desc	EQU	0Bh
    SelMgt_Set_Desc	EQU	0Ch
    SelMgt_Get_Spec_Sel EQU	0Dh

Int31_DOS_Mem_Mgt	EQU	01h
    DOSMem_Allocate	EQU	00h
    DOSMem_Free 	EQU	01h
    DOSMem_Resize	EQU	02h

Int31_Int_Serv		EQU	02h
    Int_Get_Real_Vec	EQU	00h
    Int_Set_Real_Vec	EQU	01h
    Int_Get_Excep_Vec	EQU	02h
    Int_Set_Excep_Vec	EQU	03h
    Int_Get_PMode_Vec	EQU	04h
    Int_Set_PMode_Vec	EQU	05h

Int31_Trans_Serv	EQU	03h
    Trans_Sim_Int	EQU	00h
    Trans_Far_Call	EQU	01h
    Trans_Call_Int_Proc EQU	02h
    Trans_Call_Back	EQU	03h
    Trans_Free_CB	EQU	04h
    Trans_Get_Save_Addr EQU	05h
    Trans_Get_Sw_Addr	EQU	06h

Int31_Get_Version	EQU	04h

Int31_Mem_Mgt		EQU	05h
    MemMgt_Get_Info	EQU	00h
    MemMgt_Allocate	EQU	01h
    MemMgt_Free 	EQU	02h
    MemMgt_Resize	EQU	03h

Int31_Page_Lock 	EQU	06h
    Lock_Region 	EQU	00h
    Unlock_Region	EQU	01h
    Mark_Pageable	EQU	02h
    Mark_Not_Pageable	EQU	03h
    Get_Page_Size	EQU	04h

Int31_Demand_Page_Tune	EQU	07h
    Page_Candidate	EQU	00h
    Page_Discard	EQU	01h
    DPMI_Candidate	EQU	02h
    DPMI_Discard	EQU	03h

Int31_Map_Phys_Addr	EQU	08h

Int31_Virt_Int_State	EQU	09h
    Get_Clear_Int_State EQU	00h
    Get_Set_Int_State	EQU	01h
    Get_Int_State	EQU	02h

Int31_MCP_Control	EQU	0Eh
    Get_Status		EQU	00h
    Set_Emulation	EQU	01h


Real_Mode_Call_Struc	STRUC
RealMode_EDI	dd	?
RealMode_ESI	dd	?
RealMode_EBP	dd	?
		dd	?
RealMode_EBX	dd	?
RealMode_EDX	dd	?
RealMode_ECX	dd	?
RealMode_EAX	dd	?
RealMode_Flags	dw	?
RealMode_ES	dw	?
RealMode_DS	dw	?
RealMode_FS	dw	?
RealMode_GS	dw	?
RealMode_IP	dw	?
RealMode_CS	dw	?
RealMode_SP	dw	?
RealMode_SS	dw	?
Real_Mode_Call_Struc	ENDS


Real_Mode_Word_Regs	STRUC
RealMode_DI	dw	?
		dw	?
RealMode_SI	dw	?
		dw	?
RealMode_BP	dw	?
		dw	?
		dd	?
RealMode_BX	dw	?
		dw	?
RealMode_DX	dw	?
		dw	?
RealMode_CX	dw	?
		dw	?
RealMode_AX	dw	?
Real_Mode_Word_Regs	ENDS


Real_Mode_Byte_Regs	STRUC
		dd	4 dup (?)
RealMode_BL	db	?
RealMode_BH	db	?
		dw	?
RealMode_DL	db	?
RealMode_DH	db	?
		dw	?
RealMode_CL	db	?
RealMode_CH	db	?
		dw	?
RealMode_AL	db	?
RealMode_AH	db	?
Real_Mode_Byte_Regs	ENDS
