        .file   "setjmp.S"
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
         * On entry, the stack to setjmp looks like:
         *
         *     ptr to jmp_buf
         *     return PC
         */
.globl _setjmp
_setjmp:	
        pushl   %ebp
        movl    %esp,%ebp
        
        movl    0x8(%ebp),%eax          /* address of jmp_buf to eax */
        movl    %ebx,(%eax)             /* save %ebx */
        movl    %esi,4(%eax)            /* save %esi */
        movl    %edi,8(%eax)            /* save %edi */
        leal    8(%ebp),%edx            /* calling proc's esp, not ours! */
        movl    %edx,16(%eax)
        movl    4(%ebp), %edx           /* save return PC */
        movl    %edx,20(%eax)
        movl    0(%ebp),%edx            /* calling proc's ebp, not ours! */
        movl    %edx,12(%eax)

        xorl    %eax,%eax               /* return 0 the first time */
        leave
        ret	$4

