/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/powerpc/ioport.s
 * PURPOSE:         FASTCALL Interlocked Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

/* GLOBALS *******************************************************************/

.globl READ_REGISTER_UCHAR
.globl READ_REGISTER_USHORT
.globl READ_REGISTER_ULONG
.globl READ_REGISTER_BUFFER_UCHAR
.globl READ_REGISTER_BUFFER_USHORT
.globl READ_REGISTER_BUFFER_ULONG
.globl WRITE_REGISTER_UCHAR
.globl WRITE_REGISTER_USHORT
.globl WRITE_REGISTER_ULONG
.globl WRITE_REGISTER_BUFFER_UCHAR
.globl WRITE_REGISTER_BUFFER_USHORT
.globl WRITE_REGISTER_BUFFER_ULONG

/* FUNCTIONS *****************************************************************/

READ_REGISTER_UCHAR:
        /* Return the requested memory location */
        sync
        eieio
        lbz 3,0(3)
        blr

READ_REGISTER_USHORT:
        /* Return the requested memory location */
        sync
        eieio
        lhz 3,0(3)
        blr

READ_REGISTER_ULONG:    
        /* Return the requested memory location */
        sync
        eieio
        lwz 3,0(3)
        blr

READ_REGISTER_BUFFER_UCHAR:
1:      
        cmpwi 0,5,0
        beq 2f

        lbz 0,0(4)
        stb 0,0(3)
        
        addi 3,3,1
        addi 4,4,1
        subi 5,5,1
        b 1b
2:
        eieio
        sync
        blr
        
READ_REGISTER_BUFFER_USHORT:    
1:      
        cmpwi 0,5,0
        beq 2f

        lhz 0,0(4)
        sth 0,0(3)
        
        addi 3,3,2
        addi 4,4,2
        subi 5,5,2
        b 1b
2:
        eieio
        sync
        blr

READ_REGISTER_BUFFER_ULONG:
1:      
        cmpwi 0,5,0
        beq 2f

        lwz 0,0(4)
        stw 0,0(3)
        
        addi 3,3,4
        addi 4,4,4
        subi 5,5,4
        b 1b
2:
        eieio
        sync
        blr

WRITE_REGISTER_UCHAR:
        stb 4,0(3)
        eieio
        sync
        blr

WRITE_REGISTER_USHORT:
        sth 4,0(3)
        eieio
        sync
        blr
        
WRITE_REGISTER_ULONG:
        stw 4,0(3)
        eieio
        sync
        blr

WRITE_REGISTER_BUFFER_UCHAR:
        sync
        eieio
1:      
        cmpwi 0,5,0
        beq 2f

        lbz 0,0(4)
        stb 0,0(3)
        
        addi 3,3,1
        addi 4,4,1
        subi 5,5,1
        b 1b
2:
        blr

WRITE_REGISTER_BUFFER_USHORT:
        sync
        eieio
1:      
        cmpwi 0,5,0
        beq 2f

        lhz 0,0(4)
        sth 0,0(3)
        
        addi 3,3,2
        addi 4,4,2
        subi 5,5,2
        b 1b
2:
        blr

WRITE_REGISTER_BUFFER_ULONG:
        sync
        eieio
1:      
        cmpwi 0,5,0
        beq 2f

        lwz 0,0(4)
        stw 0,0(3)
        
        addi 3,3,4
        addi 4,4,4
        subi 5,5,4
        b 1b
2:
        blr

/* EOF */
