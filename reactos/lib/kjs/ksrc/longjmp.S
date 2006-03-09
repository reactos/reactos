        .file   "longjmp.S"
/*
 * Copyright (C) 1998, 1999, Jonathan S. Shapiro.
 *
 * This file is part of the EROS Operating System.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* #include <eros/i486/asm.h> */
        
        /*
         * typedef struct {
         *  unsigned long ebx, esi, edi;
         *  unsigned long ebp;
         *  unsigned long sp;
         *  unsigned long pc;
         * } jmp_buf[1];
         */
        
        /*
         * On entry, the stack to longjmp looks like:
         *
         *     value
         *     ptr to jmp_buf
         *     return PC
         */

.globl _longjmp
_longjmp:
        pushl   %ebp
        movl    %esp,%ebp

        movl    8(%ebp),%ecx            /* address of jmp_buf to ecx */
        movl    12(%ebp),%eax           /* return value to %eax */
        testl   %eax,%eax
        jne     1f
        incl    %eax                    /* return 1 if handed 0 */

1:      
        movl    (%ecx),%ebx             /* restore %ebx */
        movl    4(%ecx),%esi            /* restore %esi */
        movl    8(%ecx),%edi            /* restore %edi */
        
        /*
         * From this instant on we are not running in a valid frame
         */
        
        movl    12(%ecx),%ebp           /* restore %ebp */
        movl    16(%ecx),%esp           /* restore %esp */
        /* movl 20(%ecx),%eax           return PC */

        /*
         * Since we are abandoning the stack in any case, 
         * there isn't much point in doing the usual return 
         * discipline.
         */
        
        jmpl    *20(%ecx)
	
