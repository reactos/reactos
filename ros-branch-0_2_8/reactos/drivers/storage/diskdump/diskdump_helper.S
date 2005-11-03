.globl __DiskDumpScsiPortNotification		
__DiskDumpScsiPortNotification:
	cmp	$0, _IsDumping
	je	.l1
	jmp	_DiskDumpScsiPortNotification
.l1:
	jmp	*_OldScsiPortNotification
.L2:	
	jmp	.L2
