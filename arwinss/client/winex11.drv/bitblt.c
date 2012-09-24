/*
 * GDI bit-blit operations
 *
 * Copyright 1993, 1994  Alexandre Julliard
 * Copyright 2006 Damjan Jovanovic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitblt);


#define DST 0   /* Destination drawable */
#define SRC 1   /* Source drawable */
#define TMP 2   /* Temporary drawable */
#define PAT 3   /* Pattern (brush) in destination DC */

#define OP(src,dst,rop)   (OP_ARGS(src,dst) << 4 | (rop))
#define OP_ARGS(src,dst)  (((src) << 2) | (dst))

#define OP_SRC(opcode)    ((opcode) >> 6)
#define OP_DST(opcode)    (((opcode) >> 4) & 3)
#define OP_SRCDST(opcode) ((opcode) >> 4)
#define OP_ROP(opcode)    ((opcode) & 0x0f)

#define MAX_OP_LEN  6  /* Longest opcode + 1 for the terminating 0 */

#define SWAP_INT32(i1,i2) \
    do { INT __t = *(i1); *(i1) = *(i2); *(i2) = __t; } while(0)

static const unsigned char BITBLT_Opcodes[256][MAX_OP_LEN] =
{
    { OP(PAT,DST,GXclear) },                         /* 0x00  0              */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXnor) },         /* 0x01  ~(D|(P|S))     */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXand) },        /* 0x02  D&~(P|S)       */
    { OP(PAT,SRC,GXnor) },                           /* 0x03  ~(P|S)         */
    { OP(PAT,DST,GXnor), OP(SRC,DST,GXand) },        /* 0x04  S&~(D|P)       */
    { OP(PAT,DST,GXnor) },                           /* 0x05  ~(D|P)         */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXnor), },     /* 0x06  ~(P|~(D^S))    */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXnor) },        /* 0x07  ~(P|(D&S))     */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXand) },/* 0x08  S&D&~P         */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXnor) },        /* 0x09  ~(P|(D^S))     */
    { OP(PAT,DST,GXandInverted) },                   /* 0x0a  D&~P           */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXnor) }, /* 0x0b  ~(P|(S&~D))    */
    { OP(PAT,SRC,GXandInverted) },                   /* 0x0c  S&~P           */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXnor) },/* 0x0d  ~(P|(D&~S))    */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXnor) },        /* 0x0e  ~(P|~(D|S))    */
    { OP(PAT,DST,GXcopyInverted) },                  /* 0x0f  ~P             */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXand) },        /* 0x10  P&~(S|D)       */
    { OP(SRC,DST,GXnor) },                           /* 0x11  ~(D|S)         */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXnor) },      /* 0x12  ~(S|~(D^P))    */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXnor) },        /* 0x13  ~(S|(D&P))     */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXnor) },      /* 0x14  ~(D|~(P^S))    */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXnor) },        /* 0x15  ~(D|(P&S))     */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXnand),
      OP(TMP,DST,GXand), OP(SRC,DST,GXxor),
      OP(PAT,DST,GXxor) },                           /* 0x16  P^S^(D&~(P&S)  */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXequiv) },                         /* 0x17 ~S^((S^P)&(S^D))*/
    { OP(PAT,SRC,GXxor), OP(PAT,DST,GXxor),
        OP(SRC,DST,GXand) },                         /* 0x18  (S^P)&(D^P)    */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXnand),
      OP(TMP,DST,GXand), OP(SRC,DST,GXequiv) },      /* 0x19  ~S^(D&~(P&S))  */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x1a  P^(D|(S&P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXxor),
      OP(TMP,DST,GXand), OP(SRC,DST,GXequiv) },      /* 0x1b  ~S^(D&(P^S))   */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x1c  P^(S|(D&P))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x1d  ~D^(S&(D^P))   */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXxor) },         /* 0x1e  P^(D|S)        */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXnand) },        /* 0x1f  ~(P&(D|S))     */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXand) }, /* 0x20  D&(P&~S)       */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXnor) },        /* 0x21  ~(S|(D^P))     */
    { OP(SRC,DST,GXandInverted) },                   /* 0x22  ~S&D           */
    { OP(PAT,DST,GXandReverse), OP(SRC,DST,GXnor) }, /* 0x23  ~(S|(P&~D))    */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXand) },                           /* 0x24   (S^P)&(S^D)   */
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0x25  ~P^(D&~(S&P))  */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXand),
      OP(TMP,DST,GXor), OP(SRC,DST,GXxor) },         /* 0x26  S^(D|(S&P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXequiv),
      OP(TMP,DST,GXor), OP(SRC,DST,GXxor) },         /* 0x27  S^(D|~(P^S))   */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXand) },        /* 0x28  D&(P^S)        */
    { OP(SRC,TMP,GXcopy), OP(PAT,TMP,GXand),
      OP(TMP,DST,GXor), OP(SRC,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x29  ~P^S^(D|(P&S)) */
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXand) },       /* 0x2a  D&~(P&S)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXequiv) },                         /* 0x2b ~S^((P^S)&(P^D))*/
    { OP(SRC,DST,GXor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x2c  S^(P&(S|D))    */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXxor) },  /* 0x2d  P^(S|~D)       */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x2e  P^(S|(D^P))    */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXnand) }, /* 0x2f  ~(P&(S|~D))    */
    { OP(PAT,SRC,GXandReverse) },                    /* 0x30  P&~S           */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXnor) },/* 0x31  ~(S|(D&~P))    */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x32  S^(D|P|S)      */
    { OP(SRC,DST,GXcopyInverted) },                  /* 0x33  ~S             */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x34  S^(P|(D&S))    */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x35  S^(P|~(D^S))   */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXxor) },         /* 0x36  S^(D|P)        */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXnand) },        /* 0x37  ~(S&(D|P))     */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0x38  P^(S&(D|P))    */
    { OP(PAT,DST,GXorReverse), OP(SRC,DST,GXxor) },  /* 0x39  S^(P|~D)       */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x3a  S^(P|(D^S))    */
    { OP(PAT,DST,GXorReverse), OP(SRC,DST,GXnand) }, /* 0x3b  ~(S&(P|~D))    */
    { OP(PAT,SRC,GXxor) },                           /* 0x3c  P^S            */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x3d  S^(P|~(D|S))   */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x3e  S^(P|(D&~S))   */
    { OP(PAT,SRC,GXnand) },                          /* 0x3f  ~(P&S)         */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXand) }, /* 0x40  P&S&~D         */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXnor) },        /* 0x41  ~(D|(P^S))     */
    { OP(DST,SRC,GXxor), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXand) },                           /* 0x42  (S^D)&(P^D)    */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0x43  ~S^(P&~(D&S))  */
    { OP(SRC,DST,GXandReverse) },                    /* 0x44  S&~D           */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXnor) }, /* 0x45  ~(D|(P&~S))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x46  D^(S|(P&D))    */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0x47  ~P^(S&(D^P))   */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXand) },        /* 0x48  S&(P^D)        */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x49  ~P^D^(S|(P&D)) */
    { OP(DST,SRC,GXor), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x4a  D^(P&(S|D))    */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXxor) }, /* 0x4b  P^(D|~S)       */
    { OP(PAT,DST,GXnand), OP(SRC,DST,GXand) },       /* 0x4c  S&~(D&P)       */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(TMP,DST,GXequiv) },                         /* 0x4d ~S^((S^P)|(S^D))*/
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXxor) },                           /* 0x4e  P^(D|(S^P))    */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXnand) },/* 0x4f  ~(P&(D|~S))    */
    { OP(PAT,DST,GXandReverse) },                    /* 0x50  P&~D           */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXnor) },/* 0x51  ~(D|(S&~P))    */
    { OP(DST,SRC,GXand), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x52  D^(P|(S&D))    */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0x53  ~S^(P&(D^S))   */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXnor) },        /* 0x54  ~(D|~(P|S))    */
    { OP(PAT,DST,GXinvert) },                        /* 0x55  ~D             */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXxor) },         /* 0x56  D^(P|S)        */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXnand) },        /* 0x57  ~(D&(P|S))     */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0x58  P^(D&(P|S))    */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXxor) },  /* 0x59  D^(P|~S)       */
    { OP(PAT,DST,GXxor) },                           /* 0x5a  D^P            */
    { OP(DST,SRC,GXnor), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x5b  D^(P|~(S|D))   */
    { OP(DST,SRC,GXxor), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x5c  D^(P|(S^D))    */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXnand) }, /* 0x5d  ~(D&(P|~S))    */
    { OP(DST,SRC,GXandInverted), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXxor) },                           /* 0x5e  D^(P|(S&~D))   */
    { OP(PAT,DST,GXnand) },                          /* 0x5f  ~(D&P)         */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXand) },        /* 0x60  P&(D^S)        */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXand),
      OP(PAT,DST,GXor), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0x61  ~D^S^(P|(D&S)) */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0x62  D^(S&(P|D))    */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXxor) }, /* 0x63  S^(D|~P)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0x64  S^(D&(P|S))    */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXxor) }, /* 0x65  D^(S|~P)       */
    { OP(SRC,DST,GXxor) },                           /* 0x66  S^D            */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x67  S^(D|~(S|P)    */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXnor),
      OP(PAT,DST,GXor), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0x68  ~D^S^(P|~(D|S))*/
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXequiv) },      /* 0x69  ~P^(D^S)       */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXxor) },        /* 0x6a  D^(P&S)        */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x6b  ~P^S^(D&(P|S)) */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXxor) },        /* 0x6c  S^(D&P)        */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor),
      OP(PAT,DST,GXequiv) },                         /* 0x6d  ~P^D^(S&(P|D)) */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXorReverse),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0x6e  S^(D&(P|~S))   */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXnand) },     /* 0x6f  ~(P&~(S^D))    */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXand) },       /* 0x70  P&~(D&S)       */
    { OP(SRC,TMP,GXcopy), OP(DST,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXequiv) },                         /* 0x71 ~S^((S^D)&(P^D))*/
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x72  S^(D|(P^S))    */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXnand) },/* 0x73  ~(S&(D|~P))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x74   D^(S|(P^D))   */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXnand) },/* 0x75  ~(D&(S|~P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXandReverse),
      OP(SRC,DST,GXor), OP(TMP,DST,GXxor) },         /* 0x76  S^(D|(P&~S))   */
    { OP(SRC,DST,GXnand) },                          /* 0x77  ~(S&D)         */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXxor) },        /* 0x78  P^(D&S)        */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXor),
      OP(PAT,DST,GXand), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0x79  ~D^S^(P&(D|S)) */
    { OP(DST,SRC,GXorInverted), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x7a  D^(P&(S|~D))   */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXnand) },     /* 0x7b  ~(S&~(D^P))    */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0x7c  S^(P&(D|~S))   */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXnand) },     /* 0x7d  ~(D&~(P^S))    */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXor) },                            /* 0x7e  (S^P)|(S^D)    */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXnand) },       /* 0x7f  ~(D&P&S)       */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXand) },        /* 0x80  D&P&S          */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXnor) },                           /* 0x81  ~((S^P)|(S^D)) */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXand) },      /* 0x82  D&~(P^S)       */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0x83  ~S^(P&(D|~S))  */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXand) },      /* 0x84  S&~(D^P)       */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0x85  ~P^(D&(S|~P))  */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXor),
      OP(PAT,DST,GXand), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x86  D^S^(P&(D|S))  */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXequiv) },      /* 0x87  ~P^(D&S)       */
    { OP(SRC,DST,GXand) },                           /* 0x88  S&D            */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXandReverse),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x89  ~S^(D|(P&~S))  */
    { OP(PAT,SRC,GXorInverted), OP(SRC,DST,GXand) }, /* 0x8a  D&(S|~P)       */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x8b  ~D^(S|(P^D))   */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXand) }, /* 0x8c  S&(D|~P)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x8d  ~S^(D|(P^S))   */
    { OP(SRC,TMP,GXcopy), OP(DST,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXxor) },                           /* 0x8e  S^((S^D)&(P^D))*/
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXnand) },      /* 0x8f  ~(P&~(D&S))    */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXand) },      /* 0x90  P&~(D^S)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXorReverse),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x91  ~S^(D&(P|~S))  */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x92  D^P^(S&(D|P))  */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXequiv) },      /* 0x93  ~S^(P&D)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x94  S^P^(D&(P|S))  */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXequiv) },      /* 0x95  ~D^(P&S)       */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXxor) },        /* 0x96  D^P^S          */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnor),
      OP(SRC,DST,GXor), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x97  S^P^(D|~(P|S)) */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnor),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0x98  ~S^(D|~(P|S))  */
    { OP(SRC,DST,GXequiv) },                         /* 0x99  ~S^D           */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXxor) }, /* 0x9a  D^(P&~S)       */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x9b  ~S^(D&(P|S))   */
    { OP(PAT,DST,GXandReverse), OP(SRC,DST,GXxor) }, /* 0x9c  S^(P&~D)       */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXequiv) },      /* 0x9d  ~D^(S&(P|D))   */
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXand),
      OP(PAT,DST,GXor), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0x9e  D^S^(P|(D&S))  */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXnand) },       /* 0x9f  ~(P&(D^S))     */
    { OP(PAT,DST,GXand) },                           /* 0xa0  D&P            */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xa1  ~P^(D|(S&~P))  */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXand) },  /* 0xa2  D&(P|~S)       */
    { OP(DST,SRC,GXxor), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xa3  ~D^(P|(S^D))   */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xa4  ~P^(D|~(S|P))  */
    { OP(PAT,DST,GXequiv) },                         /* 0xa5  ~P^D           */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXxor) },/* 0xa6  D^(S&~P)       */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0xa7  ~P^(D&(S|P))   */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXand) },         /* 0xa8  D&(P|S)        */
    { OP(PAT,SRC,GXor), OP(SRC,DST,GXequiv) },       /* 0xa9  ~D^(P|S)       */
    { OP(PAT,DST,GXnoop) },                          /* 0xaa  D              */
    { OP(PAT,SRC,GXnor), OP(SRC,DST,GXor) },         /* 0xab  D|~(P|S)       */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xac  S^(P&(D^S))    */
    { OP(DST,SRC,GXand), OP(PAT,SRC,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xad  ~D^(P|(S&D))   */
    { OP(PAT,SRC,GXandInverted), OP(SRC,DST,GXor) }, /* 0xae  D|(S&~P)       */
    { OP(PAT,DST,GXorInverted) },                    /* 0xaf  D|~P           */
    { OP(SRC,DST,GXorInverted), OP(PAT,DST,GXand) }, /* 0xb0  P&(D|~S)       */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xb1  ~P^(D|(S^P))   */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXor),
      OP(TMP,DST,GXxor) },                           /* 0xb2  S^((S^P)|(S^D))*/
    { OP(PAT,DST,GXnand), OP(SRC,DST,GXnand) },      /* 0xb3  ~(S&~(D&P))    */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXxor) }, /* 0xb4  P^(S&~D)       */
    { OP(DST,SRC,GXor), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0xb5  ~D^(P&(S|D))   */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0xb6  D^P^(S|(D&P))  */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXnand) },       /* 0xb7  ~(S&(D^P))     */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0xb8  P^(S&(D^P))    */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0xb9  ~D^(S|(P&D))   */
    { OP(PAT,SRC,GXandReverse), OP(SRC,DST,GXor) },  /* 0xba  D|(P&~S)       */
    { OP(SRC,DST,GXorInverted) },                    /* 0xbb  ~S|D           */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xbc  S^(P&~(D&S))   */
    { OP(DST,SRC,GXxor), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXnand) },                          /* 0xbd  ~((S^D)&(P^D)) */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXor) },         /* 0xbe  D|(P^S)        */
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXor) },        /* 0xbf  D|~(P&S)       */
    { OP(PAT,SRC,GXand) },                           /* 0xc0  P&S            */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xc1  ~S^(P|(D&~S))  */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xc2  ~S^(P|~(D|S))  */
    { OP(PAT,SRC,GXequiv) },                         /* 0xc3  ~P^S           */
    { OP(PAT,DST,GXorReverse), OP(SRC,DST,GXand) },  /* 0xc4  S&(P|~D)       */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xc5  ~S^(P|(D^S))   */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXxor) },/* 0xc6  S^(D&~P)       */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXequiv) },                         /* 0xc7  ~P^(S&(D|P))   */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXand) },         /* 0xc8  S&(D|P)        */
    { OP(PAT,DST,GXor), OP(SRC,DST,GXequiv) },       /* 0xc9  ~S^(P|D)       */
    { OP(DST,SRC,GXxor), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xca  D^(P&(S^D))    */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXor),
      OP(SRC,DST,GXequiv) },                         /* 0xcb  ~S^(P|(D&S))   */
    { OP(SRC,DST,GXcopy) },                          /* 0xcc  S              */
    { OP(PAT,DST,GXnor), OP(SRC,DST,GXor) },         /* 0xcd  S|~(D|P)       */
    { OP(PAT,DST,GXandInverted), OP(SRC,DST,GXor) }, /* 0xce  S|(D&~P)       */
    { OP(PAT,SRC,GXorInverted) },                    /* 0xcf  S|~P           */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXand) },  /* 0xd0  P&(S|~D)       */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xd1  ~P^(S|(D^P))   */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXxor) },/* 0xd2  P^(D&~S)       */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXand),
      OP(SRC,DST,GXequiv) },                         /* 0xd3  ~S^(P&(D|S))   */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(PAT,DST,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXxor) },                           /* 0xd4  S^((S^P)&(D^P))*/
    { OP(PAT,SRC,GXnand), OP(SRC,DST,GXnand) },      /* 0xd5  ~(D&~(P&S))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXor), OP(PAT,DST,GXxor),
      OP(TMP,DST,GXxor) },                           /* 0xd6  S^P^(D|(P&S))  */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXnand) },       /* 0xd7  ~(D&(P^S))     */
    { OP(PAT,SRC,GXxor), OP(SRC,DST,GXand),
      OP(PAT,DST,GXxor) },                           /* 0xd8  P^(D&(S^P))    */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXor), OP(TMP,DST,GXequiv) },       /* 0xd9  ~S^(D|(P&S))   */
    { OP(DST,SRC,GXnand), OP(PAT,SRC,GXand),
      OP(SRC,DST,GXxor) },                           /* 0xda  D^(P&~(S&D))   */
    { OP(SRC,DST,GXxor), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXnand) },                          /* 0xdb  ~((S^P)&(S^D)) */
    { OP(PAT,DST,GXandReverse), OP(SRC,DST,GXor) },  /* 0xdc  S|(P&~D)       */
    { OP(SRC,DST,GXorReverse) },                     /* 0xdd  S|~D           */
    { OP(PAT,DST,GXxor), OP(SRC,DST,GXor) },         /* 0xde  S|(D^P)        */
    { OP(PAT,DST,GXnand), OP(SRC,DST,GXor) },        /* 0xdf  S|~(D&P)       */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXand) },         /* 0xe0  P&(D|S)        */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXequiv) },       /* 0xe1  ~P^(D|S)       */
    { OP(DST,TMP,GXcopy), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0xe2  D^(S&(P^D))    */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xe3  ~P^(S|(D&P))   */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXxor),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0xe4  S^(D&(P^S))    */
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXor),
      OP(PAT,DST,GXequiv) },                         /* 0xe5  ~P^(D|(S&P))   */
    { OP(SRC,TMP,GXcopy), OP(PAT,SRC,GXnand),
      OP(SRC,DST,GXand), OP(TMP,DST,GXxor) },        /* 0xe6  S^(D&~(P&S))   */
    { OP(PAT,SRC,GXxor), OP(PAT,DST,GXxor),
      OP(SRC,DST,GXnand) },                          /* 0xe7  ~((S^P)&(D^P)) */
    { OP(SRC,TMP,GXcopy), OP(SRC,DST,GXxor),
      OP(PAT,SRC,GXxor), OP(SRC,DST,GXand),
      OP(TMP,DST,GXxor) },                           /* 0xe8  S^((S^P)&(S^D))*/
    { OP(DST,TMP,GXcopy), OP(SRC,DST,GXnand),
      OP(PAT,DST,GXand), OP(SRC,DST,GXxor),
      OP(TMP,DST,GXequiv) },                         /* 0xe9  ~D^S^(P&~(S&D))*/
    { OP(PAT,SRC,GXand), OP(SRC,DST,GXor) },         /* 0xea  D|(P&S)        */
    { OP(PAT,SRC,GXequiv), OP(SRC,DST,GXor) },       /* 0xeb  D|~(P^S)       */
    { OP(PAT,DST,GXand), OP(SRC,DST,GXor) },         /* 0xec  S|(D&P)        */
    { OP(PAT,DST,GXequiv), OP(SRC,DST,GXor) },       /* 0xed  S|~(D^P)       */
    { OP(SRC,DST,GXor) },                            /* 0xee  S|D            */
    { OP(PAT,DST,GXorInverted), OP(SRC,DST,GXor) },  /* 0xef  S|D|~P         */
    { OP(PAT,DST,GXcopy) },                          /* 0xf0  P              */
    { OP(SRC,DST,GXnor), OP(PAT,DST,GXor) },         /* 0xf1  P|~(D|S)       */
    { OP(SRC,DST,GXandInverted), OP(PAT,DST,GXor) }, /* 0xf2  P|(D&~S)       */
    { OP(PAT,SRC,GXorReverse) },                     /* 0xf3  P|~S           */
    { OP(SRC,DST,GXandReverse), OP(PAT,DST,GXor) },  /* 0xf4  P|(S&~D)       */
    { OP(PAT,DST,GXorReverse) },                     /* 0xf5  P|~D           */
    { OP(SRC,DST,GXxor), OP(PAT,DST,GXor) },         /* 0xf6  P|(D^S)        */
    { OP(SRC,DST,GXnand), OP(PAT,DST,GXor) },        /* 0xf7  P|~(S&D)       */
    { OP(SRC,DST,GXand), OP(PAT,DST,GXor) },         /* 0xf8  P|(D&S)        */
    { OP(SRC,DST,GXequiv), OP(PAT,DST,GXor) },       /* 0xf9  P|~(D^S)       */
    { OP(PAT,DST,GXor) },                            /* 0xfa  D|P            */
    { OP(PAT,SRC,GXorReverse), OP(SRC,DST,GXor) },   /* 0xfb  D|P|~S         */
    { OP(PAT,SRC,GXor) },                            /* 0xfc  P|S            */
    { OP(SRC,DST,GXorReverse), OP(PAT,DST,GXor) },   /* 0xfd  P|S|~D         */
    { OP(SRC,DST,GXor), OP(PAT,DST,GXor) },          /* 0xfe  P|D|S          */
    { OP(PAT,DST,GXset) }                            /* 0xff  1              */
};


#ifdef BITBLT_TEST  /* Opcodes test */

static int do_bitop( int s, int d, int rop )
{
    int res;
    switch(rop)
    {
    case GXclear:        res = 0; break;
    case GXand:          res = s & d; break;
    case GXandReverse:   res = s & ~d; break;
    case GXcopy:         res = s; break;
    case GXandInverted:  res = ~s & d; break;
    case GXnoop:         res = d; break;
    case GXxor:          res = s ^ d; break;
    case GXor:           res = s | d; break;
    case GXnor:          res = ~(s | d); break;
    case GXequiv:        res = ~s ^ d; break;
    case GXinvert:       res = ~d; break;
    case GXorReverse:    res = s | ~d; break;
    case GXcopyInverted: res = ~s; break;
    case GXorInverted:   res = ~s | d; break;
    case GXnand:         res = ~(s & d); break;
    case GXset:          res = 1; break;
    }
    return res & 1;
}

int main()
{
    int rop, i, res, src, dst, pat, tmp, dstUsed;
    const BYTE *opcode;

    for (rop = 0; rop < 256; rop++)
    {
        res = dstUsed = 0;
        for (i = 0; i < 8; i++)
        {
            pat = (i >> 2) & 1;
            src = (i >> 1) & 1;
            dst = i & 1;
            for (opcode = BITBLT_Opcodes[rop]; *opcode; opcode++)
            {
                switch(*opcode >> 4)
                {
                case OP_ARGS(DST,TMP):
                    tmp = do_bitop( dst, tmp, *opcode & 0xf );
                    break;
                case OP_ARGS(DST,SRC):
                    src = do_bitop( dst, src, *opcode & 0xf );
                    break;
                case OP_ARGS(SRC,TMP):
                    tmp = do_bitop( src, tmp, *opcode & 0xf );
                    break;
                case OP_ARGS(SRC,DST):
                    dst = do_bitop( src, dst, *opcode & 0xf );
                    dstUsed = 1;
                    break;
                case OP_ARGS(PAT,TMP):
                    tmp = do_bitop( pat, tmp, *opcode & 0xf );
                    break;
                case OP_ARGS(PAT,DST):
                    dst = do_bitop( pat, dst, *opcode & 0xf );
                    dstUsed = 1;
                    break;
                case OP_ARGS(PAT,SRC):
                    src = do_bitop( pat, src, *opcode & 0xf );
                    break;
                case OP_ARGS(TMP,DST):
                    dst = do_bitop( tmp, dst, *opcode & 0xf );
                    dstUsed = 1;
                    break;
                case OP_ARGS(TMP,SRC):
                    src = do_bitop( tmp, src, *opcode & 0xf );
                    break;
                default:
                    printf( "Invalid opcode %x\n", *opcode );
                }
            }
            if (!dstUsed) dst = src;
            if (dst) res |= 1 << i;
        }
        if (res != rop) printf( "%02x: ERROR, res=%02x\n", rop, res );
    }

    return 0;
}

#endif  /* BITBLT_TEST */


static void get_colors(X11DRV_PDEVICE *physDevDst, X11DRV_PDEVICE *physDevSrc,
		       int *fg, int *bg)
{
    RGBQUAD rgb[2];

    *fg = physDevDst->textPixel;
    *bg = physDevDst->backgroundPixel;
    if(physDevSrc->depth == 1) {
        if(GetDIBColorTable(physDevSrc->hdc, 0, 2, rgb) == 2) {
            DWORD logcolor;
            logcolor = RGB(rgb[0].rgbRed, rgb[0].rgbGreen, rgb[0].rgbBlue);
            *fg = X11DRV_PALETTE_ToPhysical( physDevDst, logcolor );
            logcolor = RGB(rgb[1].rgbRed, rgb[1].rgbGreen,rgb[1].rgbBlue);
            *bg = X11DRV_PALETTE_ToPhysical( physDevDst, logcolor );
        }
    }
}

/* return a mask for meaningful bits when doing an XGetPixel on an image */
static unsigned long image_pixel_mask( X11DRV_PDEVICE *physDev )
{
    unsigned long ret;
    ColorShifts *shifts = physDev->color_shifts;

    if (!shifts) shifts = &X11DRV_PALETTE_default_shifts;
    ret = (shifts->physicalRed.max << shifts->physicalRed.shift) |
        (shifts->physicalGreen.max << shifts->physicalGreen.shift) |
        (shifts->physicalBlue.max << shifts->physicalBlue.shift);
    if (!ret) ret = (1 << physDev->depth) - 1;
    return ret;
}


/***********************************************************************
 *           BITBLT_StretchRow
 *
 * Stretch a row of pixels. Helper function for BITBLT_StretchImage.
 */
static void BITBLT_StretchRow( int *rowSrc, int *rowDst,
                               INT startDst, INT widthDst,
                               INT xinc, INT xoff, WORD mode )
{
    register INT xsrc = xinc * startDst + xoff;
    rowDst += startDst;
    switch(mode)
    {
    case STRETCH_ANDSCANS:
        for(; widthDst > 0; widthDst--, xsrc += xinc)
            *rowDst++ &= rowSrc[xsrc >> 16];
        break;
    case STRETCH_ORSCANS:
        for(; widthDst > 0; widthDst--, xsrc += xinc)
            *rowDst++ |= rowSrc[xsrc >> 16];
        break;
    case STRETCH_DELETESCANS:
        for(; widthDst > 0; widthDst--, xsrc += xinc)
            *rowDst++ = rowSrc[xsrc >> 16];
        break;
    }
}


/***********************************************************************
 *           BITBLT_ShrinkRow
 *
 * Shrink a row of pixels. Helper function for BITBLT_StretchImage.
 */
static void BITBLT_ShrinkRow( int *rowSrc, int *rowDst,
                              INT startSrc, INT widthSrc,
                              INT xinc, INT xoff, WORD mode )
{
    register INT xdst = xinc * startSrc + xoff;
    rowSrc += startSrc;
    switch(mode)
    {
    case STRETCH_ORSCANS:
        for(; widthSrc > 0; widthSrc--, xdst += xinc)
            rowDst[xdst >> 16] |= *rowSrc++;
        break;
    case STRETCH_ANDSCANS:
        for(; widthSrc > 0; widthSrc--, xdst += xinc)
            rowDst[xdst >> 16] &= *rowSrc++;
        break;
    case STRETCH_DELETESCANS:
        for(; widthSrc > 0; widthSrc--, xdst += xinc)
            rowDst[xdst >> 16] = *rowSrc++;
        break;
    }
}


/***********************************************************************
 *           BITBLT_GetRow
 *
 * Retrieve a row from an image. Helper function for BITBLT_StretchImage.
 */
static void BITBLT_GetRow( XImage *image, int *pdata, INT row,
                           INT start, INT width, INT depthDst,
                           int fg, int bg, unsigned long pixel_mask, BOOL swap)
{
    register INT i;

    assert( (row >= 0) && (row < image->height) );
    assert( (start >= 0) && (width <= image->width) );

    pdata += swap ? start+width-1 : start;
    if (image->depth == depthDst)  /* color -> color */
    {
        if (X11DRV_PALETTE_XPixelToPalette && (depthDst != 1))
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, i, row )];
            else for (i = 0; i < width; i++)
                *pdata++ = X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, i, row )];
        else
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = XGetPixel( image, i, row );
            else for (i = 0; i < width; i++)
                *pdata++ = XGetPixel( image, i, row );
    }
    else
    {
        if (image->depth == 1)  /* monochrome -> color */
        {
            if (X11DRV_PALETTE_XPixelToPalette)
            {
                fg = X11DRV_PALETTE_XPixelToPalette[fg];
                bg = X11DRV_PALETTE_XPixelToPalette[bg];
            }
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = XGetPixel( image, i, row ) ? bg : fg;
            else for (i = 0; i < width; i++)
                *pdata++ = XGetPixel( image, i, row ) ? bg : fg;
        }
        else  /* color -> monochrome */
        {
            if (swap) for (i = 0; i < width; i++)
                *pdata-- = ((XGetPixel( image, i, row ) & pixel_mask) == bg) ? 1 : 0;
            else for (i = 0; i < width; i++)
                *pdata++ = ((XGetPixel( image, i, row ) & pixel_mask) == bg) ? 1 : 0;
        }
    }
}


/***********************************************************************
 *           BITBLT_StretchImage
 *
 * Stretch an X image.
 * FIXME: does not work for full 32-bit coordinates.
 */
static void BITBLT_StretchImage( XImage *srcImage, XImage *dstImage,
                                 INT widthSrc, INT heightSrc,
                                 INT widthDst, INT heightDst,
                                 RECT *visRectSrc, RECT *visRectDst,
                                 int foreground, int background,
                                 unsigned long pixel_mask, WORD mode )
{
    int *rowSrc, *rowDst, *pixel;
    char *pdata;
    INT xinc, xoff, yinc, ysrc, ydst;
    register INT x, y;
    BOOL hstretch, vstretch, hswap, vswap;

    hswap = widthSrc * widthDst < 0;
    vswap = heightSrc * heightDst < 0;
    widthSrc  = abs(widthSrc);
    heightSrc = abs(heightSrc);
    widthDst  = abs(widthDst);
    heightDst = abs(heightDst);

    if (!(rowSrc = HeapAlloc( GetProcessHeap(), 0,
                              (widthSrc+widthDst)*sizeof(int) ))) return;
    rowDst = rowSrc + widthSrc;

      /* When stretching, all modes are the same, and DELETESCANS is faster */
    if ((widthSrc < widthDst) && (heightSrc < heightDst))
        mode = STRETCH_DELETESCANS;

    if (mode == STRETCH_HALFTONE) /* FIXME */
        mode = STRETCH_DELETESCANS;

    if (mode != STRETCH_DELETESCANS)
        memset( rowDst, (mode == STRETCH_ANDSCANS) ? 0xff : 0x00,
                widthDst*sizeof(int) );

    hstretch = (widthSrc < widthDst);
    vstretch = (heightSrc < heightDst);

    if (hstretch)
    {
        xinc = (widthSrc << 16) / widthDst;
        xoff = ((widthSrc << 16) - (xinc * widthDst)) / 2;
    }
    else
    {
        xinc = ((int)widthDst << 16) / widthSrc;
        xoff = ((widthDst << 16) - (xinc * widthSrc)) / 2;
    }

    wine_tsx11_lock();
    if (vstretch)
    {
        yinc = (heightSrc << 16) / heightDst;
        ydst = visRectDst->top;
        if (vswap)
        {
            ysrc = yinc * (heightDst - ydst - 1);
            yinc = -yinc;
        }
        else
            ysrc = yinc * ydst;

        for ( ; (ydst < visRectDst->bottom); ysrc += yinc, ydst++)
        {
            if (((ysrc >> 16) < visRectSrc->top) ||
                ((ysrc >> 16) >= visRectSrc->bottom)) continue;

            /* Retrieve a source row */
            BITBLT_GetRow( srcImage, rowSrc, (ysrc >> 16) - visRectSrc->top,
                           hswap ? widthSrc - visRectSrc->right
                                 : visRectSrc->left,
                           visRectSrc->right - visRectSrc->left,
                           dstImage->depth, foreground, background, pixel_mask, hswap );

            /* Stretch or shrink it */
            if (hstretch)
                BITBLT_StretchRow( rowSrc, rowDst, visRectDst->left,
                                   visRectDst->right - visRectDst->left,
                                   xinc, xoff, mode );
            else BITBLT_ShrinkRow( rowSrc, rowDst,
                                   hswap ? widthSrc - visRectSrc->right
                                         : visRectSrc->left,
                                   visRectSrc->right - visRectSrc->left,
                                   xinc, xoff, mode );

            /* Store the destination row */
            pixel = rowDst + visRectDst->right - 1;
            y = ydst - visRectDst->top;
            for (x = visRectDst->right-visRectDst->left-1; x >= 0; x--)
                XPutPixel( dstImage, x, y, *pixel-- );
            if (mode != STRETCH_DELETESCANS)
                memset( rowDst, (mode == STRETCH_ANDSCANS) ? 0xff : 0x00,
                        widthDst*sizeof(int) );

            /* Make copies of the destination row */

            pdata = dstImage->data + dstImage->bytes_per_line * y;
            while (((ysrc + yinc) >> 16 == ysrc >> 16) &&
                   (ydst < visRectDst->bottom-1))
            {
                memcpy( pdata + dstImage->bytes_per_line, pdata,
                        dstImage->bytes_per_line );
                pdata += dstImage->bytes_per_line;
                ysrc += yinc;
                ydst++;
            }
        }
    }
    else  /* Shrinking */
    {
        yinc = (heightDst << 16) / heightSrc;
        ysrc = visRectSrc->top;
        ydst = ((heightDst << 16) - (yinc * heightSrc)) / 2;
        if (vswap)
        {
            ydst += yinc * (heightSrc - ysrc - 1);
            yinc = -yinc;
        }
        else
            ydst += yinc * ysrc;

        for( ; (ysrc < visRectSrc->bottom); ydst += yinc, ysrc++)
        {
            if (((ydst >> 16) < visRectDst->top) ||
                ((ydst >> 16) >= visRectDst->bottom)) continue;

            /* Retrieve a source row */
            BITBLT_GetRow( srcImage, rowSrc, ysrc - visRectSrc->top,
                           hswap ? widthSrc - visRectSrc->right
                                 : visRectSrc->left,
                           visRectSrc->right - visRectSrc->left,
                           dstImage->depth, foreground, background, pixel_mask, hswap );

            /* Stretch or shrink it */
            if (hstretch)
                BITBLT_StretchRow( rowSrc, rowDst, visRectDst->left,
                                   visRectDst->right - visRectDst->left,
                                   xinc, xoff, mode );
            else BITBLT_ShrinkRow( rowSrc, rowDst,
                                   hswap ? widthSrc - visRectSrc->right
                                         : visRectSrc->left,
                                   visRectSrc->right - visRectSrc->left,
                                   xinc, xoff, mode );

            /* Merge several source rows into the destination */
            if (mode == STRETCH_DELETESCANS)
            {
                /* Simply skip the overlapping rows */
                while (((ydst + yinc) >> 16 == ydst >> 16) &&
                       (ysrc < visRectSrc->bottom-1))
                {
                    ydst += yinc;
                    ysrc++;
                }
            }
            else if (((ydst + yinc) >> 16 == ydst >> 16) &&
                     (ysrc < visRectSrc->bottom-1))
                continue;  /* Restart loop for next overlapping row */

            /* Store the destination row */
            pixel = rowDst + visRectDst->right - 1;
            y = (ydst >> 16) - visRectDst->top;
            for (x = visRectDst->right-visRectDst->left-1; x >= 0; x--)
                XPutPixel( dstImage, x, y, *pixel-- );
            if (mode != STRETCH_DELETESCANS)
                memset( rowDst, (mode == STRETCH_ANDSCANS) ? 0xff : 0x00,
                        widthDst*sizeof(int) );
        }
    }
    wine_tsx11_unlock();
    HeapFree( GetProcessHeap(), 0, rowSrc );
}


/***********************************************************************
 *           BITBLT_GetSrcAreaStretch
 *
 * Retrieve an area from the source DC, stretching and mapping all the
 * pixels to Windows colors.
 */
static int BITBLT_GetSrcAreaStretch( X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                     Pixmap pixmap, GC gc,
                                     const struct bitblt_coords *src, const struct bitblt_coords *dst )
{
    XImage *imageSrc, *imageDst;
    RECT rectSrc = src->visrect;
    RECT rectDst = dst->visrect;
    int fg, bg;

    rectSrc.left   -= src->x;
    rectSrc.right  -= src->x;
    rectSrc.top    -= src->y;
    rectSrc.bottom -= src->y;
    rectDst.left   -= dst->x;
    rectDst.right  -= dst->x;
    rectDst.top    -= dst->y;
    rectDst.bottom -= dst->y;
    if (src->width < 0)
    {
        rectSrc.left  -= src->width;
        rectSrc.right -= src->width;
    }
    if (dst->width < 0)
    {
        rectDst.left  -= dst->width;
        rectDst.right -= dst->width;
    }
    if (src->height < 0)
    {
        rectSrc.top    -= src->height;
        rectSrc.bottom -= src->height;
    }
    if (dst->height < 0)
    {
        rectDst.top    -= dst->height;
        rectDst.bottom -= dst->height;
    }

    get_colors(physDevDst, physDevSrc, &fg, &bg);
    wine_tsx11_lock();
    /* FIXME: avoid BadMatch errors */
    imageSrc = XGetImage( gdi_display, physDevSrc->drawable,
                          physDevSrc->dc_rect.left + src->visrect.left,
                          physDevSrc->dc_rect.top + src->visrect.top,
                          src->visrect.right - src->visrect.left,
                          src->visrect.bottom - src->visrect.top,
                          AllPlanes, ZPixmap );
    wine_tsx11_unlock();

    imageDst = X11DRV_DIB_CreateXImage( rectDst.right - rectDst.left,
                                        rectDst.bottom - rectDst.top, physDevDst->depth );
    BITBLT_StretchImage( imageSrc, imageDst, src->width, src->height,
                         dst->width, dst->height, &rectSrc, &rectDst,
                         fg, physDevDst->depth != 1 ? bg : physDevSrc->backgroundPixel,
                         image_pixel_mask( physDevSrc ), GetStretchBltMode(physDevDst->hdc) );
    wine_tsx11_lock();
    XPutImage( gdi_display, pixmap, gc, imageDst, 0, 0, 0, 0,
               rectDst.right - rectDst.left, rectDst.bottom - rectDst.top );
    XDestroyImage( imageSrc );
    X11DRV_DIB_DestroyXImage( imageDst );
    wine_tsx11_unlock();
    return 0;  /* no exposure events generated */
}


/***********************************************************************
 *           BITBLT_GetSrcArea
 *
 * Retrieve an area from the source DC, mapping all the
 * pixels to Windows colors.
 */
static int BITBLT_GetSrcArea( X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                              Pixmap pixmap, GC gc, RECT *visRectSrc )
{
    XImage *imageSrc, *imageDst;
    register INT x, y;
    int exposures = 0;
    INT width  = visRectSrc->right - visRectSrc->left;
    INT height = visRectSrc->bottom - visRectSrc->top;
    int fg, bg;
    BOOL memdc = (GetObjectType(physDevSrc->hdc) == OBJ_MEMDC);

    if (physDevSrc->depth == physDevDst->depth)
    {
        wine_tsx11_lock();
        if (!X11DRV_PALETTE_XPixelToPalette ||
            (physDevDst->depth == 1))  /* monochrome -> monochrome */
        {
            if (physDevDst->depth == 1)
            {
                /* MSDN says if StretchBlt must convert a bitmap from monochrome
                   to color or vice versa, the foreground and background color of
                   the device context are used.  In fact, it also applies to the
                   case when it is converted from mono to mono. */
                XSetBackground( gdi_display, gc, physDevDst->textPixel );
                XSetForeground( gdi_display, gc, physDevDst->backgroundPixel );
                XCopyPlane( gdi_display, physDevSrc->drawable, pixmap, gc,
                            physDevSrc->dc_rect.left + visRectSrc->left,
                            physDevSrc->dc_rect.top + visRectSrc->top,
                            width, height, 0, 0, 1);
            }
            else
                XCopyArea( gdi_display, physDevSrc->drawable, pixmap, gc,
                           physDevSrc->dc_rect.left + visRectSrc->left,
                           physDevSrc->dc_rect.top + visRectSrc->top,
                           width, height, 0, 0);
            exposures++;
        }
        else  /* color -> color */
        {
            if (memdc)
                imageSrc = XGetImage( gdi_display, physDevSrc->drawable,
                                      physDevSrc->dc_rect.left + visRectSrc->left,
                                      physDevSrc->dc_rect.top + visRectSrc->top,
                                      width, height, AllPlanes, ZPixmap );
            else
            {
                /* Make sure we don't get a BadMatch error */
                XCopyArea( gdi_display, physDevSrc->drawable, pixmap, gc,
                           physDevSrc->dc_rect.left + visRectSrc->left,
                           physDevSrc->dc_rect.top + visRectSrc->top,
                           width, height, 0, 0);
                exposures++;
                imageSrc = XGetImage( gdi_display, pixmap, 0, 0, width, height,
                                      AllPlanes, ZPixmap );
            }
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    XPutPixel(imageSrc, x, y,
                              X11DRV_PALETTE_XPixelToPalette[XGetPixel(imageSrc, x, y)]);
            XPutImage( gdi_display, pixmap, gc, imageSrc,
                       0, 0, 0, 0, width, height );
            XDestroyImage( imageSrc );
        }
        wine_tsx11_unlock();
    }
    else
    {
        if (physDevSrc->depth == 1)  /* monochrome -> color */
        {
	    get_colors(physDevDst, physDevSrc, &fg, &bg);

            wine_tsx11_lock();
            if (X11DRV_PALETTE_XPixelToPalette)
            {
                XSetBackground( gdi_display, gc,
                             X11DRV_PALETTE_XPixelToPalette[fg] );
                XSetForeground( gdi_display, gc,
                             X11DRV_PALETTE_XPixelToPalette[bg]);
            }
            else
            {
                XSetBackground( gdi_display, gc, fg );
                XSetForeground( gdi_display, gc, bg );
            }
            XCopyPlane( gdi_display, physDevSrc->drawable, pixmap, gc,
                        physDevSrc->dc_rect.left + visRectSrc->left,
                        physDevSrc->dc_rect.top + visRectSrc->top,
                        width, height, 0, 0, 1 );
            exposures++;
            wine_tsx11_unlock();
        }
        else  /* color -> monochrome */
        {
            unsigned long pixel_mask;
            wine_tsx11_lock();
            /* FIXME: avoid BadMatch error */
            imageSrc = XGetImage( gdi_display, physDevSrc->drawable,
                                  physDevSrc->dc_rect.left + visRectSrc->left,
                                  physDevSrc->dc_rect.top + visRectSrc->top,
                                  width, height, AllPlanes, ZPixmap );
            if (!imageSrc)
            {
                wine_tsx11_unlock();
                return exposures;
            }
            imageDst = X11DRV_DIB_CreateXImage( width, height, physDevDst->depth );
            if (!imageDst)
            {
                XDestroyImage(imageSrc);
                wine_tsx11_unlock();
                return exposures;
            }
            pixel_mask = image_pixel_mask( physDevSrc );
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    XPutPixel(imageDst, x, y,
                              !((XGetPixel(imageSrc,x,y) ^ physDevSrc->backgroundPixel) & pixel_mask));
            XPutImage( gdi_display, pixmap, gc, imageDst,
                       0, 0, 0, 0, width, height );
            XDestroyImage( imageSrc );
            X11DRV_DIB_DestroyXImage( imageDst );
            wine_tsx11_unlock();
        }
    }
    return exposures;
}


/***********************************************************************
 *           BITBLT_GetDstArea
 *
 * Retrieve an area from the destination DC, mapping all the
 * pixels to Windows colors.
 */
static int BITBLT_GetDstArea(X11DRV_PDEVICE *physDev, Pixmap pixmap, GC gc, RECT *visRectDst)
{
    int exposures = 0;
    INT width  = visRectDst->right - visRectDst->left;
    INT height = visRectDst->bottom - visRectDst->top;
    BOOL memdc = (GetObjectType( physDev->hdc ) == OBJ_MEMDC);

    wine_tsx11_lock();

    if (!X11DRV_PALETTE_XPixelToPalette || (physDev->depth == 1) ||
	(X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL) )
    {
        XCopyArea( gdi_display, physDev->drawable, pixmap, gc,
                   physDev->dc_rect.left + visRectDst->left, physDev->dc_rect.top + visRectDst->top,
                   width, height, 0, 0 );
        exposures++;
    }
    else
    {
        register INT x, y;
        XImage *image;

        if (memdc)
            image = XGetImage( gdi_display, physDev->drawable,
                               physDev->dc_rect.left + visRectDst->left,
                               physDev->dc_rect.top + visRectDst->top,
                               width, height, AllPlanes, ZPixmap );
        else
        {
            /* Make sure we don't get a BadMatch error */
            XCopyArea( gdi_display, physDev->drawable, pixmap, gc,
                       physDev->dc_rect.left + visRectDst->left,
                       physDev->dc_rect.top + visRectDst->top,
                       width, height, 0, 0);
            exposures++;
            image = XGetImage( gdi_display, pixmap, 0, 0, width, height,
                               AllPlanes, ZPixmap );
        }
        if (image)
        {
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++)
                    XPutPixel( image, x, y,
                               X11DRV_PALETTE_XPixelToPalette[XGetPixel( image, x, y )]);
            XPutImage( gdi_display, pixmap, gc, image, 0, 0, 0, 0, width, height );
            XDestroyImage( image );
        }
    }

    wine_tsx11_unlock();
    return exposures;
}


/***********************************************************************
 *           BITBLT_PutDstArea
 *
 * Put an area back into the destination DC, mapping the pixel
 * colors to X pixels.
 */
static int BITBLT_PutDstArea(X11DRV_PDEVICE *physDev, Pixmap pixmap, RECT *visRectDst)
{
    int exposures = 0;
    INT width  = visRectDst->right - visRectDst->left;
    INT height = visRectDst->bottom - visRectDst->top;

    /* !X11DRV_PALETTE_PaletteToXPixel is _NOT_ enough */

    if (!X11DRV_PALETTE_PaletteToXPixel || (physDev->depth == 1) ||
        (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL) )
    {
        XCopyArea( gdi_display, pixmap, physDev->drawable, physDev->gc, 0, 0, width, height,
                   physDev->dc_rect.left + visRectDst->left,
                   physDev->dc_rect.top + visRectDst->top );
        exposures++;
    }
    else
    {
        register INT x, y;
        XImage *image = XGetImage( gdi_display, pixmap, 0, 0, width, height,
                                   AllPlanes, ZPixmap );
        for (y = 0; y < height; y++)
            for (x = 0; x < width; x++)
            {
                XPutPixel( image, x, y,
                           X11DRV_PALETTE_PaletteToXPixel[XGetPixel( image, x, y )]);
            }
        XPutImage( gdi_display, physDev->drawable, physDev->gc, image, 0, 0,
                   physDev->dc_rect.left + visRectDst->left,
                   physDev->dc_rect.top + visRectDst->top, width, height );
        XDestroyImage( image );
    }
    return exposures;
}


/***********************************************************************
 *           BITBLT_GetVisRectangles
 *
 * Get the source and destination visible rectangles for StretchBlt().
 * Return FALSE if one of the rectangles is empty.
 */
static BOOL BITBLT_GetVisRectangles( X11DRV_PDEVICE *physDevDst, X11DRV_PDEVICE *physDevSrc,
                                     struct bitblt_coords *dst, struct bitblt_coords *src )
{
    RECT rect, clipRect;

      /* Get the destination visible rectangle */

    rect.left   = dst->x;
    rect.top    = dst->y;
    rect.right  = dst->x + dst->width;
    rect.bottom = dst->y + dst->height;
    LPtoDP( physDevDst->hdc, (POINT *)&rect, 2 );
    dst->x      = rect.left;
    dst->y      = rect.top;
    dst->width  = rect.right - rect.left;
    dst->height = rect.bottom - rect.top;
    if (dst->layout & LAYOUT_RTL && dst->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        SWAP_INT32( &rect.left, &rect.right );
        dst->x = rect.left;
        dst->width = rect.right - rect.left;
    }
    if (rect.left > rect.right) { SWAP_INT32( &rect.left, &rect.right ); rect.left++; rect.right++; }
    if (rect.top > rect.bottom) { SWAP_INT32( &rect.top, &rect.bottom ); rect.top++; rect.bottom++; }

    GetRgnBox( physDevDst->region, &clipRect );
    if (!IntersectRect( &dst->visrect, &rect, &clipRect )) return FALSE;

      /* Get the source visible rectangle */

    if (!physDevSrc) return TRUE;

    rect.left   = src->x;
    rect.top    = src->y;
    rect.right  = src->x + src->width;
    rect.bottom = src->y + src->height;
    LPtoDP( physDevSrc->hdc, (POINT *)&rect, 2 );
    src->x      = rect.left;
    src->y      = rect.top;
    src->width  = rect.right - rect.left;
    src->height = rect.bottom - rect.top;
    if (src->layout & LAYOUT_RTL && src->layout & LAYOUT_BITMAPORIENTATIONPRESERVED)
    {
        SWAP_INT32( &rect.left, &rect.right );
        src->x = rect.left;
        src->width = rect.right - rect.left;
    }
    if (rect.left > rect.right) { SWAP_INT32( &rect.left, &rect.right ); rect.left++; rect.right++; }
    if (rect.top > rect.bottom) { SWAP_INT32( &rect.top, &rect.bottom ); rect.top++; rect.bottom++; }

    /* Apparently the clipping and visible regions are only for output,
       so just check against dc extent here to avoid BadMatch errors */
    clipRect = physDevSrc->drawable_rect;
    OffsetRect( &clipRect, -(physDevSrc->drawable_rect.left + physDevSrc->dc_rect.left),
                -(physDevSrc->drawable_rect.top + physDevSrc->dc_rect.top) );
    if (!IntersectRect( &src->visrect, &rect, &clipRect ))
        return FALSE;

      /* Intersect the rectangles */

    if ((src->width == dst->width) && (src->height == dst->height)) /* no stretching */
    {
        OffsetRect( &src->visrect, dst->x - src->x, dst->y - src->y );
        if (!IntersectRect( &rect, &src->visrect, &dst->visrect )) return FALSE;
        src->visrect = dst->visrect = rect;
        OffsetRect( &src->visrect, src->x - dst->x, src->y - dst->y );
    }
    else  /* stretching */
    {
        /* Map source rectangle into destination coordinates */
        rect.left   = dst->x + (src->visrect.left - src->x)*dst->width/src->width;
        rect.top    = dst->y + (src->visrect.top - src->y)*dst->height/src->height;
        rect.right  = dst->x + (src->visrect.right - src->x)*dst->width/src->width;
        rect.bottom = dst->y + (src->visrect.bottom - src->y)*dst->height/src->height;
        if (rect.left > rect.right) SWAP_INT32( &rect.left, &rect.right );
        if (rect.top > rect.bottom) SWAP_INT32( &rect.top, &rect.bottom );

        /* Avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!IntersectRect( &dst->visrect, &rect, &dst->visrect )) return FALSE;

        /* Map destination rectangle back to source coordinates */
        rect = dst->visrect;
        rect.left   = src->x + (dst->visrect.left - dst->x)*src->width/dst->width;
        rect.top    = src->y + (dst->visrect.top - dst->y)*src->height/dst->height;
        rect.right  = src->x + (dst->visrect.right - dst->x)*src->width/dst->width;
        rect.bottom = src->y + (dst->visrect.bottom - dst->y)*src->height/dst->height;
        if (rect.left > rect.right) SWAP_INT32( &rect.left, &rect.right );
        if (rect.top > rect.bottom) SWAP_INT32( &rect.top, &rect.bottom );

        /* Avoid rounding errors */
        rect.left--;
        rect.top--;
        rect.right++;
        rect.bottom++;
        if (!IntersectRect( &src->visrect, &rect, &src->visrect )) return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           client_side_dib_copy
 */
static BOOL client_side_dib_copy( X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                                  X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                                  INT width, INT height )
{
    DIBSECTION srcDib, dstDib;
    BYTE *srcPtr, *dstPtr;
    INT srcRowOffset, dstRowOffset;
    INT bytesPerPixel;
    INT bytesToCopy;
    INT y;
    static RECT unusedRect;

    if (GetObjectW(physDevSrc->bitmap->hbitmap, sizeof(srcDib), &srcDib) != sizeof(srcDib))
      return FALSE;
    if (GetObjectW(physDevDst->bitmap->hbitmap, sizeof(dstDib), &dstDib) != sizeof(dstDib))
      return FALSE;

    /* check for oversized values, just like X11DRV_DIB_CopyDIBSection() */
    if (xSrc > srcDib.dsBm.bmWidth || ySrc > srcDib.dsBm.bmHeight)
      return FALSE;
    if (xSrc + width > srcDib.dsBm.bmWidth)
      width = srcDib.dsBm.bmWidth - xSrc;
    if (ySrc + height > srcDib.dsBm.bmHeight)
      height = srcDib.dsBm.bmHeight - ySrc;

    if (GetRgnBox(physDevDst->region, &unusedRect) == COMPLEXREGION)
    {
      /* for simple regions, the clipping was already done by BITBLT_GetVisRectangles */
      FIXME("potential optimization: client-side complex region clipping\n");
      return FALSE;
    }
    if (dstDib.dsBm.bmBitsPixel <= 8)
    {
      FIXME("potential optimization: client-side color-index mode DIB copy\n");
      return FALSE;
    }
    if (!(srcDib.dsBmih.biCompression == BI_BITFIELDS &&
          dstDib.dsBmih.biCompression == BI_BITFIELDS &&
          !memcmp(srcDib.dsBitfields, dstDib.dsBitfields, 3*sizeof(DWORD)))
        && !(srcDib.dsBmih.biCompression == BI_RGB &&
             dstDib.dsBmih.biCompression == BI_RGB))
    {
      FIXME("potential optimization: client-side compressed DIB copy\n");
      return FALSE;
    }
    if (srcDib.dsBm.bmBitsPixel != dstDib.dsBm.bmBitsPixel)
    {
      FIXME("potential optimization: pixel format conversion\n");
      return FALSE;
    }
    if (srcDib.dsBmih.biWidth < 0 || dstDib.dsBmih.biWidth < 0)
    {
      FIXME("negative widths not yet implemented\n");
      return FALSE;
    }

    switch (dstDib.dsBm.bmBitsPixel)
    {
      case 15:
      case 16:
        bytesPerPixel = 2;
        break;
      case 24:
        bytesPerPixel = 3;
        break;
      case 32:
        bytesPerPixel = 4;
        break;
      default:
        FIXME("don't know how to work with a depth of %d\n", physDevSrc->depth);
        return FALSE;
    }

    bytesToCopy = width * bytesPerPixel;

    if (physDevSrc->bitmap->topdown)
    {
      srcPtr = &physDevSrc->bitmap->base[ySrc*srcDib.dsBm.bmWidthBytes + xSrc*bytesPerPixel];
      srcRowOffset = srcDib.dsBm.bmWidthBytes;
    }
    else
    {
      srcPtr = &physDevSrc->bitmap->base[(srcDib.dsBm.bmHeight-ySrc-1)*srcDib.dsBm.bmWidthBytes
        + xSrc*bytesPerPixel];
      srcRowOffset = -srcDib.dsBm.bmWidthBytes;
    }
    if (physDevDst->bitmap->topdown)
    {
      dstPtr = &physDevDst->bitmap->base[yDst*dstDib.dsBm.bmWidthBytes + xDst*bytesPerPixel];
      dstRowOffset = dstDib.dsBm.bmWidthBytes;
    }
    else
    {
      dstPtr = &physDevDst->bitmap->base[(dstDib.dsBm.bmHeight-yDst-1)*dstDib.dsBm.bmWidthBytes
        + xDst*bytesPerPixel];
      dstRowOffset = -dstDib.dsBm.bmWidthBytes;
    }

    /* Handle overlapping regions on the same DIB */
    if (physDevSrc == physDevDst && ySrc < yDst)
    {
      srcPtr += srcRowOffset * (height - 1);
      srcRowOffset = -srcRowOffset;
      dstPtr += dstRowOffset * (height - 1);
      dstRowOffset = -dstRowOffset;
    }

    for (y = yDst; y < yDst + height; ++y)
    {
      memmove(dstPtr, srcPtr, bytesToCopy);
      srcPtr += srcRowOffset;
      dstPtr += dstRowOffset;
    }

    return TRUE;
}

static BOOL same_format(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst)
{
    if (physDevSrc->depth != physDevDst->depth) return FALSE;
    if (!physDevSrc->color_shifts && !physDevDst->color_shifts) return TRUE;
    if (physDevSrc->color_shifts && physDevDst->color_shifts)
        return !memcmp(physDevSrc->color_shifts, physDevDst->color_shifts, sizeof(ColorShifts));
    return FALSE;
}

/***********************************************************************
 *           X11DRV_StretchBlt
 */
BOOL CDECL X11DRV_StretchBlt( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                              X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                              DWORD rop )
{
    BOOL usePat, useSrc, useDst, destUsed, fStretch, fNullBrush;
    struct bitblt_coords src, dst;
    INT width, height;
    INT sDst, sSrc = DIB_Status_None;
    const BYTE *opcode;
    Pixmap pixmaps[3] = { 0, 0, 0 };  /* pixmaps for DST, SRC, TMP */
    GC tmpGC = 0;

    usePat = (((rop >> 4) & 0x0f0000) != (rop & 0x0f0000));
    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    useDst = (((rop >> 1) & 0x550000) != (rop & 0x550000));
    if (!physDevSrc && useSrc) return FALSE;

    src.x      = xSrc;
    src.y      = ySrc;
    src.width  = widthSrc;
    src.height = heightSrc;
    src.layout = physDevSrc ? GetLayout( physDevSrc->hdc ) : 0;
    dst.x      = xDst;
    dst.y      = yDst;
    dst.width  = widthDst;
    dst.height = heightDst;
    dst.layout = GetLayout( physDevDst->hdc );

    if (rop & NOMIRRORBITMAP)
    {
        src.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
        dst.layout |= LAYOUT_BITMAPORIENTATIONPRESERVED;
        rop &= ~NOMIRRORBITMAP;
    }

    if (useSrc)
    {
        if (!BITBLT_GetVisRectangles( physDevDst, physDevSrc, &dst, &src ))
            return TRUE;
        fStretch = (src.width != dst.width) || (src.height != dst.height);

        if (physDevDst != physDevSrc)
            sSrc = X11DRV_LockDIBSection( physDevSrc, DIB_Status_None );
    }
    else
    {
        fStretch = FALSE;
        if (!BITBLT_GetVisRectangles( physDevDst, NULL, &dst, NULL ))
            return TRUE;
    }

    TRACE("    rectdst=%d,%d %dx%d orgdst=%d,%d visdst=%s\n",
          dst.x, dst.y, dst.width, dst.height,
          physDevDst->dc_rect.left, physDevDst->dc_rect.top, wine_dbgstr_rect( &dst.visrect ) );
    if (useSrc)
        TRACE("    rectsrc=%d,%d %dx%d orgsrc=%d,%d vissrc=%s\n",
              src.x, src.y, src.width, src.height,
              physDevSrc->dc_rect.left, physDevSrc->dc_rect.top, wine_dbgstr_rect( &src.visrect ) );

    width  = dst.visrect.right - dst.visrect.left;
    height = dst.visrect.bottom - dst.visrect.top;

    sDst = X11DRV_LockDIBSection( physDevDst, DIB_Status_None );
    if (physDevDst == physDevSrc) sSrc = sDst;

    /* try client-side DIB copy */
    if (!fStretch && rop == SRCCOPY &&
        sSrc == DIB_Status_AppMod && sDst == DIB_Status_AppMod &&
        same_format(physDevSrc, physDevDst))
    {
        if (client_side_dib_copy( physDevSrc, src.visrect.left, src.visrect.top,
                                  physDevDst, dst.visrect.left, dst.visrect.top, width, height ))
            goto done;
    }

    X11DRV_CoerceDIBSection( physDevDst, DIB_Status_GdiMod );

    opcode = BITBLT_Opcodes[(rop >> 16) & 0xff];

    /* a few optimizations for single-op ROPs */
    if (!fStretch && !opcode[1])
    {
        if (OP_SRCDST(*opcode) == OP_ARGS(PAT,DST))
        {
            switch(rop)  /* a few special cases */
            {
            case BLACKNESS:  /* 0x00 */
            case WHITENESS:  /* 0xff */
                if ((physDevDst->depth != 1) && X11DRV_PALETTE_PaletteToXPixel)
                {
                    wine_tsx11_lock();
                    XSetFunction( gdi_display, physDevDst->gc, GXcopy );
                    if (rop == BLACKNESS)
                        XSetForeground( gdi_display, physDevDst->gc, X11DRV_PALETTE_PaletteToXPixel[0] );
                    else
                        XSetForeground( gdi_display, physDevDst->gc,
                                        WhitePixel( gdi_display, DefaultScreen(gdi_display) ));
                    XSetFillStyle( gdi_display, physDevDst->gc, FillSolid );
                    XFillRectangle( gdi_display, physDevDst->drawable, physDevDst->gc,
                                    physDevDst->dc_rect.left + dst.visrect.left,
                                    physDevDst->dc_rect.top + dst.visrect.top,
                                    width, height );
                    wine_tsx11_unlock();
                    goto done;
                }
                break;
            case DSTINVERT:  /* 0x55 */
                if (!(X11DRV_PALETTE_PaletteFlags & (X11DRV_PALETTE_PRIVATE | X11DRV_PALETTE_VIRTUAL)))
                {
                    /* Xor is much better when we do not have full colormap.   */
                    /* Using white^black ensures that we invert at least black */
                    /* and white. */
                    unsigned long xor_pix = (WhitePixel( gdi_display, DefaultScreen(gdi_display) ) ^
                                             BlackPixel( gdi_display, DefaultScreen(gdi_display) ));
                    wine_tsx11_lock();
                    XSetFunction( gdi_display, physDevDst->gc, GXxor );
                    XSetForeground( gdi_display, physDevDst->gc, xor_pix);
                    XSetFillStyle( gdi_display, physDevDst->gc, FillSolid );
                    XFillRectangle( gdi_display, physDevDst->drawable, physDevDst->gc,
                                    physDevDst->dc_rect.left + dst.visrect.left,
                                    physDevDst->dc_rect.top + dst.visrect.top,
                                    width, height );
                    wine_tsx11_unlock();
                    goto done;
                }
                break;
            }
            if (!usePat || X11DRV_SetupGCForBrush( physDevDst ))
            {
                wine_tsx11_lock();
                XSetFunction( gdi_display, physDevDst->gc, OP_ROP(*opcode) );
                XFillRectangle( gdi_display, physDevDst->drawable, physDevDst->gc,
                                physDevDst->dc_rect.left + dst.visrect.left,
                                physDevDst->dc_rect.top + dst.visrect.top,
                                width, height );
                wine_tsx11_unlock();
            }
            goto done;
        }
        else if (OP_SRCDST(*opcode) == OP_ARGS(SRC,DST))
        {
            if (same_format(physDevSrc, physDevDst))
            {
                wine_tsx11_lock();
                XSetFunction( gdi_display, physDevDst->gc, OP_ROP(*opcode) );
                wine_tsx11_unlock();

                if (physDevSrc != physDevDst)
                {
                    if (sSrc == DIB_Status_AppMod)
                    {
                        X11DRV_DIB_CopyDIBSection( physDevSrc, physDevDst, src.visrect.left, src.visrect.top,
                                                   dst.visrect.left, dst.visrect.top, width, height );
                        goto done;
                    }
                    X11DRV_CoerceDIBSection( physDevSrc, DIB_Status_GdiMod );
                }
                wine_tsx11_lock();
                XCopyArea( gdi_display, physDevSrc->drawable,
                           physDevDst->drawable, physDevDst->gc,
                           physDevSrc->dc_rect.left + src.visrect.left,
                           physDevSrc->dc_rect.top + src.visrect.top,
                           width, height,
                           physDevDst->dc_rect.left + dst.visrect.left,
                           physDevDst->dc_rect.top + dst.visrect.top );
                physDevDst->exposures++;
                wine_tsx11_unlock();
                goto done;
            }
            if (physDevSrc->depth == 1)
            {
                int fg, bg;

                X11DRV_CoerceDIBSection( physDevSrc, DIB_Status_GdiMod );
                get_colors(physDevDst, physDevSrc, &fg, &bg);
                wine_tsx11_lock();
                XSetBackground( gdi_display, physDevDst->gc, fg );
                XSetForeground( gdi_display, physDevDst->gc, bg );
                XSetFunction( gdi_display, physDevDst->gc, OP_ROP(*opcode) );
                XCopyPlane( gdi_display, physDevSrc->drawable,
                            physDevDst->drawable, physDevDst->gc,
                            physDevSrc->dc_rect.left + src.visrect.left,
                            physDevSrc->dc_rect.top + src.visrect.top,
                            width, height,
                            physDevDst->dc_rect.left + dst.visrect.left,
                            physDevDst->dc_rect.top + dst.visrect.top, 1 );
                physDevDst->exposures++;
                wine_tsx11_unlock();
                goto done;
            }
        }
    }

    wine_tsx11_lock();
    tmpGC = XCreateGC( gdi_display, physDevDst->drawable, 0, NULL );
    XSetSubwindowMode( gdi_display, tmpGC, IncludeInferiors );
    XSetGraphicsExposures( gdi_display, tmpGC, False );
    pixmaps[DST] = XCreatePixmap( gdi_display, root_window, width, height,
                                  physDevDst->depth );
    wine_tsx11_unlock();

    if (useSrc)
    {
        wine_tsx11_lock();
        pixmaps[SRC] = XCreatePixmap( gdi_display, root_window, width, height,
                                      physDevDst->depth );
        wine_tsx11_unlock();

        if (physDevDst != physDevSrc) X11DRV_CoerceDIBSection( physDevSrc, DIB_Status_GdiMod );

        if(!X11DRV_XRender_GetSrcAreaStretch( physDevSrc, physDevDst, pixmaps[SRC], tmpGC, &src, &dst ))
        {
            if (fStretch)
                BITBLT_GetSrcAreaStretch( physDevSrc, physDevDst, pixmaps[SRC], tmpGC, &src, &dst );
            else
                BITBLT_GetSrcArea( physDevSrc, physDevDst, pixmaps[SRC], tmpGC, &src.visrect );
        }
    }

    if (useDst) BITBLT_GetDstArea( physDevDst, pixmaps[DST], tmpGC, &dst.visrect );
    if (usePat) fNullBrush = !X11DRV_SetupGCForPatBlt( physDevDst, tmpGC, TRUE );
    else fNullBrush = FALSE;
    destUsed = FALSE;

    wine_tsx11_lock();
    for ( ; *opcode; opcode++)
    {
        if (OP_DST(*opcode) == DST) destUsed = TRUE;
        XSetFunction( gdi_display, tmpGC, OP_ROP(*opcode) );
        switch(OP_SRCDST(*opcode))
        {
        case OP_ARGS(DST,TMP):
        case OP_ARGS(SRC,TMP):
            if (!pixmaps[TMP])
                pixmaps[TMP] = XCreatePixmap( gdi_display, root_window,
                                              width, height, physDevDst->depth );
            /* fall through */
        case OP_ARGS(DST,SRC):
        case OP_ARGS(SRC,DST):
        case OP_ARGS(TMP,SRC):
        case OP_ARGS(TMP,DST):
	    if (useSrc)
		XCopyArea( gdi_display, pixmaps[OP_SRC(*opcode)],
			   pixmaps[OP_DST(*opcode)], tmpGC,
			   0, 0, width, height, 0, 0 );
            break;

        case OP_ARGS(PAT,TMP):
            if (!pixmaps[TMP] && !fNullBrush)
                pixmaps[TMP] = XCreatePixmap( gdi_display, root_window,
                                              width, height, physDevDst->depth );
            /* fall through */
        case OP_ARGS(PAT,DST):
        case OP_ARGS(PAT,SRC):
            if (!fNullBrush)
                XFillRectangle( gdi_display, pixmaps[OP_DST(*opcode)],
                                tmpGC, 0, 0, width, height );
            break;
        }
    }
    XSetFunction( gdi_display, physDevDst->gc, GXcopy );
    physDevDst->exposures += BITBLT_PutDstArea( physDevDst, pixmaps[destUsed ? DST : SRC], &dst.visrect );
    XFreePixmap( gdi_display, pixmaps[DST] );
    if (pixmaps[SRC]) XFreePixmap( gdi_display, pixmaps[SRC] );
    if (pixmaps[TMP]) XFreePixmap( gdi_display, pixmaps[TMP] );
    XFreeGC( gdi_display, tmpGC );
    wine_tsx11_unlock();

done:
    if (useSrc && physDevDst != physDevSrc) X11DRV_UnlockDIBSection( physDevSrc, FALSE );
    X11DRV_UnlockDIBSection( physDevDst, TRUE );
    return TRUE;
}


/***********************************************************************
 *           X11DRV_AlphaBlend
 */
BOOL CDECL X11DRV_AlphaBlend( X11DRV_PDEVICE *physDevDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                              X11DRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                              BLENDFUNCTION blendfn )
{
    struct bitblt_coords src, dst;

    src.x      = xSrc;
    src.y      = ySrc;
    src.width  = widthSrc;
    src.height = heightSrc;
    src.layout = GetLayout( physDevSrc->hdc );
    dst.x      = xDst;
    dst.y      = yDst;
    dst.width  = widthDst;
    dst.height = heightDst;
    dst.layout = GetLayout( physDevDst->hdc );

    if (!BITBLT_GetVisRectangles( physDevDst, physDevSrc, &dst, &src )) return TRUE;

    TRACE( "format %x alpha %u rectdst=%d,%d %dx%d orgdst=%d,%d visdst=%s rectsrc=%d,%d %dx%d orgsrc=%d,%d vissrc=%s\n",
           blendfn.AlphaFormat, blendfn.SourceConstantAlpha, dst.x, dst.y, dst.width, dst.height,
           physDevDst->dc_rect.left, physDevDst->dc_rect.top, wine_dbgstr_rect( &dst.visrect ),
           src.x, src.y, src.width, src.height,
           physDevSrc->dc_rect.left, physDevSrc->dc_rect.top, wine_dbgstr_rect( &src.visrect ) );

    if (src.x < 0 || src.y < 0 || src.width < 0 || src.height < 0 ||
        src.width > physDevSrc->drawable_rect.right - physDevSrc->drawable_rect.left - src.x ||
        src.height > physDevSrc->drawable_rect.bottom - physDevSrc->drawable_rect.top - src.y)
    {
        WARN( "Invalid src coords: (%d,%d), size %dx%d\n", src.x, src.y, src.width, src.height );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return XRender_AlphaBlend( physDevDst, physDevSrc, &dst, &src, blendfn );
}
