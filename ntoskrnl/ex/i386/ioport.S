/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/i386/ioport.S
 * PURPOSE:         FASTCALL Interlocked Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

/* GLOBALS *******************************************************************/

PUBLIC _READ_REGISTER_UCHAR@4
PUBLIC _READ_REGISTER_USHORT@4
PUBLIC _READ_REGISTER_ULONG@4
PUBLIC _READ_REGISTER_BUFFER_UCHAR@12
PUBLIC _READ_REGISTER_BUFFER_USHORT@12
PUBLIC _READ_REGISTER_BUFFER_ULONG@12
PUBLIC _WRITE_REGISTER_UCHAR@8
PUBLIC _WRITE_REGISTER_USHORT@8
PUBLIC _WRITE_REGISTER_ULONG@8
PUBLIC _WRITE_REGISTER_BUFFER_UCHAR@12
PUBLIC _WRITE_REGISTER_BUFFER_USHORT@12
PUBLIC _WRITE_REGISTER_BUFFER_ULONG@12

/* FUNCTIONS *****************************************************************/

.code

_READ_REGISTER_UCHAR@4:

    /* Return the requested memory location */
    mov edx, [esp+4]
    mov al, [edx]
    ret 4


_READ_REGISTER_USHORT@4:

    /* Return the requested memory location */
    mov edx, [esp+4]
    mov ax, [edx]
    ret 4


_READ_REGISTER_ULONG@4:

    /* Return the requested memory location */
    mov edx, [esp+4]
    mov eax, [edx]
    ret 4


_READ_REGISTER_BUFFER_UCHAR@12:

    /* Save volatiles */
    mov eax, esi
    mov edx, edi

    /* Do the transfer */
    mov ecx, [esp+12]
    mov esi, [esp+4]
    mov edi, [esp+8]
    rep movsb

    /* Restore volatiles and return */
    mov edi, edx
    mov esi, eax
    ret 12


_READ_REGISTER_BUFFER_USHORT@12:

    /* Save volatiles */
    mov eax, esi
    mov edx, edi

    /* Do the transfer */
    mov ecx, [esp+12]
    mov esi, [esp+4]
    mov edi, [esp+8]
    rep movsw

    /* Restore volatiles and return */
    mov edi, edx
    mov esi, eax
    ret 12


_READ_REGISTER_BUFFER_ULONG@12:

    /* Save volatiles */
    mov eax, esi
    mov edx, edi

    /* Do the transfer */
    mov ecx, [esp+12]
    mov esi, [esp+4]
    mov edi, [esp+8]
    rep movsd

    /* Restore volatiles and return */
    mov edi, edx
    mov esi, eax
    ret 12


_WRITE_REGISTER_UCHAR@8:

    /* Write to memory */
    mov edx, [esp+4]
    mov al, [esp+8]
    mov [edx], al

    /* Flush posted write buffers and return */
    lock or [esp+4], edx
    ret 8


_WRITE_REGISTER_USHORT@8:

    /* Write to memory */
    mov edx, [esp+4]
    mov eax, [esp+8]
    mov [edx], ax

    /* Flush posted write buffers and return */
    lock or [esp+4], edx
    ret 8


_WRITE_REGISTER_ULONG@8:

    /* Write to memory */
    mov edx, [esp+4]
    mov eax, [esp+8]
    mov [edx], eax

    /* Flush posted write buffers and return */
    lock or [esp+4], edx
    ret 8


_WRITE_REGISTER_BUFFER_UCHAR@12:

    /* Save volatiles */
    mov eax, esi
    mov edx, edi

    /* Do the transfer */
    mov ecx, [esp+12]
    mov esi, [esp+8]
    mov edi, [esp+4]
    rep movsb

    /* Flush posted write buffers */
    lock or [esp+4], ecx

    /* Restore volatiles and return */
    mov edi, edx
    mov esi, eax
    ret 12


_WRITE_REGISTER_BUFFER_USHORT@12:

    /* Save volatiles */
    mov eax, esi
    mov edx, edi

    /* Do the transfer */
    mov ecx, [esp+12]
    mov esi, [esp+8]
    mov edi, [esp+4]
    rep movsw

    /* Flush posted write buffers */
    lock or [esp+4], ecx

    /* Restore volatiles and return */
    mov edi, edx
    mov esi, eax
    ret 12


_WRITE_REGISTER_BUFFER_ULONG@12:

    /* Save volatiles */
    mov eax, esi
    mov edx, edi

    /* Do the transfer */
    mov ecx, [esp+12]
    mov esi, [esp+8]
    mov edi, [esp+4]
    rep movsd

    /* Flush posted write buffers */
    lock or [esp+4], ecx

    /* Restore volatiles and return */
    mov edi, edx
    mov esi, eax
    ret 12

END
/* EOF */
