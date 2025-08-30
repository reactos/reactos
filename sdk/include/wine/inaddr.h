/*
 * Copyright 2010 Detlef Riekenberg
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
 *
 */

#ifndef __INADDR_H__
#define __INADDR_H__

#ifdef USE_WS_PREFIX
#define WS(x)    WS_##x
#else
#define WS(x)    x
#endif

typedef struct WS(in_addr)
{
    union {
        struct {
            UCHAR s_b1,s_b2,s_b3,s_b4;
        } S_un_b;
        struct {
            USHORT s_w1,s_w2;
        } S_un_w;
        ULONG S_addr;
    } S_un;
} IN_ADDR, *PIN_ADDR, *LPIN_ADDR;

#undef WS

#ifndef USE_WS_PREFIX
#define s_addr  S_un.S_addr
#define s_net   S_un.S_un_b.s_b1
#define s_host  S_un.S_un_b.s_b2
#define s_lh    S_un.S_un_b.s_b3
#define s_impno S_un.S_un_b.s_b4
#define s_imp   S_un.S_un_w.s_w2
#else
#define WS_s_addr  S_un.S_addr
#define WS_s_net   S_un.S_un_b.s_b1
#define WS_s_host  S_un.S_un_b.s_b2
#define WS_s_lh    S_un.S_un_b.s_b3
#define WS_s_impno S_un.S_un_b.s_b4
#define WS_s_imp   S_un.S_un_w.s_w2
#endif  /* USE_WS_PREFIX */

#endif /* __INADDR_H__ */
