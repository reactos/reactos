/* $Id: pwroff.c,v 1.1 2001/08/21 20:18:27 chorns Exp $
 *
 * FILE       : reactos/hal/x86/apm.c
 * DESCRIPTION: Turn CPU off...
 * PROJECT    : ReactOS Operating System
 * AUTHOR     : D. Lindauer (July 11 1997)
 * NOTE       : This program is public domain
 * REVISIONS  :
 * 	1999-12-26
 */

#define APM_FUNCTION_AVAILABLE	0x5300
#define APM_FUNCTION_CONNREAL	0x5301
#define APM_FUNCTION_POWEROFF	0x5307
#define APM_FUNCTION_ENABLECPU	0x530d
#define APM_FUNCTION_ENABLEAPM	0x530e

#define APM_DEVICE_BIOS 	0
#define APM_DEVICE_ALL		1

#define APM_MODE_DISABLE	0
#define APM_MODE_ENABLE		1



nopm	db	'No power management functionality',10,13,'$'
errmsg	db	'Power management error',10,13,'$'
wrongver db	'Need APM version 1.1 or better',10,13,'$'
;
; Entry point
;
go:
	mov	dx,offset nopm
	jc	error
	cmp	ax,101h			; See if version 1.1 or greater
	mov	dx,offset wrongver
	jc	error
	
	mov	[ver],ax
	mov	ax,5301h		; Do a real mode connection
	mov	bx,0			; device = BIOS
	int	15h
	jnc	noconerr
	
	cmp	ah,2			; Pass if already connected
	mov	dx,offset errmsg	; else error
	jnz	error
noconerr:
	mov	ax,530eh		; Enable latest version of APM
	mov	bx,0			; device = BIOS
	mov	cx,[ver]		; version
	int	15h
	mov	dx,offset errmsg
	jc	error
	
	mov	ax,530dh		; Now engage and enable CPU management
	mov	bx,1			; device = all
	mov	cx,1			; enable
	int	15h
	mov	dx,offset errmsg
	jc	error
	
	mov	ax,530fh
	mov	bx,1			; device = ALL
	mov	cx,1			; enable
	int	15h
	mov	dx,offset errmsg
	jc	error

	mov	dx,offset errmsg
error:
	call	print
	mov	ax,4c01h
	int	21h
	int 3
	end start


BOOLEAN
ApmCall (
	DWORD	Function,
	DWORD	Device,
	DWORD	Mode
	)
{
	/* AX <== Function */
	/* BX <== Device */
	/* CX <== Mode */
	__asm__("int 21\n"); /* 0x15 */
}


BOOLEAN
HalPowerOff (VOID)
{
	ApmCall (
		APM_FUNCTION_AVAILABLE,
		APM_DEVICE_BIOS,
		0
		);
	ApmCall (
		APM_FUNCTION_ENABLEAPM,
		);
	/* Shutdown CPU */
	ApmCall (
		APM_FUNCTION_POWEROFF,
		APM_DEVICE_ALL,
		3
		);
	return TRUE;
}


/* EOF */
