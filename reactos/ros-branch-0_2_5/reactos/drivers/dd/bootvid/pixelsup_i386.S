/* $Id: pixelsup_i386.S,v 1.2 2004/03/04 18:55:09 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/inbv/i386/pixelsup.S
 * PURPOSE:         Boot video support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/*
 * VOID
 * InbvPutPixels(int x, int y, unsigned long c);
 *
 * Will put 4 pixels on the screen at
 * (x+0*8,y), (x+1*8,y), (x+2*8,y), and (x+3*8,y)
 * c will contain:
 * bits  0- 3: Palette index for pixel at (x+0*8,y)
 * bits  8-11: Palette index for pixel at (x+1*8,y)
 * bits 16-19: Palette index for pixel at (x+2*8,y)
 * bits 24-27: Palette index for pixel at (x+3*8,y)
 * 
 * Parameters:
 *   [EBP+08h] - x X-coordinate of first pixel
 *   [ESP+0Ch] - y Y-coordinate of first pixel
 *   [ESP+10h] - c 4*4-bit color indices
 */
.globl _InbvPutPixels
_InbvPutPixels:
	pushl	%ebp
	movl	%esp, %ebp

  /* Compute mask and put it in EBX
     mask = maskbit[x] */
  movl	0x8(%ebp), %esi
  movl	_maskbit(,%esi, 4), %ebx

  /* Don't set bit mask if it is already set */
  cmpl  (inbv_last_mask),%ebx
  je  .nomask

  /* Set Mask Bit Register
     WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
     WRITE_PORT_UCHAR((PUCHAR)0x3cf,mask); */
  movl  %ebx,(inbv_last_mask)
  movw  $0x3ce,%dx
  movb  $0x08,%al
  outb  %al,%dx
  movw  $0x3cf,%dx
  movb  %bl,%al
  outb  %al,%dx

.nomask:

  /* Compute offset in video memory and put it in EBX
     offset = (x >> 3) + (y << 4) + (y << 6); */
  movl	0xC(%ebp), %esi   /* y */
  movl	%esi, %ebx
  shll  $0x6, %ebx
  shll  $0x4, %esi
  addl	%esi, %ebx
  movl	0x8(%ebp), %eax   /* x */
  shrl  $0x3, %eax
  addl	%eax, %ebx

  /* Latch first byte
    (UCHAR) READ_REGISTER_UCHAR(VideoMemory + offset+0); */
  movl	(_VideoMemory), %esi
  addl	%ebx, %esi
  movb  0x0(%esi), %bl
  /* Write color index for first pixel
     *((PUCHAR)(VideoMemory + offset+0)) = (c >> 0*8) & 0xff; */
  movl  0x10(%ebp), %eax
  movb  %al, 0x0(%esi)

  /* Latch second byte
    (UCHAR) READ_REGISTER_UCHAR(VideoMemory + offset+1); */
  movb  0x1(%esi), %bl
  /* Write color index for second pixel
     *((PUCHAR)(VideoMemory + offset+1)) = (c >> 1*8) & 0xff; */
  shrl  $0x8, %eax
  movb  %al, 0x1(%esi)

  /* Latch third byte
    (UCHAR) READ_REGISTER_UCHAR(VideoMemory + offset+2); */
  movb  0x2(%esi), %bl
  /* Write color index for third pixel
     *((PUCHAR)(VideoMemory + offset+2)) = (c >> 2*8) & 0xff; */
  shrl  $0x8, %eax
  movb  %al, 0x2(%esi)

  /* Latch fourth byte
    (UCHAR) READ_REGISTER_UCHAR(VideoMemory + offset+3); */
  movb  0x3(%esi), %bl
  /* Write color index for fourth pixel
     *((PUCHAR)(VideoMemory + offset+3)) = (c >> 3*8) & 0xff; */
  shrl  $0x8, %eax
  movb  %al, 0x3(%esi)

  popl  %ebp
	ret

.bss
inbv_last_mask:
	.short 0
