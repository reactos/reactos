/* WINE ipifcons.h
 * Copyright (C) 2003 Juan Lang
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef WINE_IPIFCONS_H__
#define WINE_IPIFCONS_H__

#define MIB_IF_TYPE_OTHER               1
#define MIB_IF_TYPE_ETHERNET            6
#define MIB_IF_TYPE_TOKENRING           9
#define MIB_IF_TYPE_FDDI                15
#define MIB_IF_TYPE_PPP                 23
#define MIB_IF_TYPE_LOOPBACK            24
#define MIB_IF_TYPE_SLIP                28

#define MIB_IF_ADMIN_STATUS_UP          1
#define MIB_IF_ADMIN_STATUS_DOWN        2
#define MIB_IF_ADMIN_STATUS_TESTING     3

#define MIB_IF_OPER_STATUS_NON_OPERATIONAL      0
#define MIB_IF_OPER_STATUS_UNREACHABLE          1
#define MIB_IF_OPER_STATUS_DISCONNECTED         2
#define MIB_IF_OPER_STATUS_CONNECTING           3
#define MIB_IF_OPER_STATUS_CONNECTED            4
#define MIB_IF_OPER_STATUS_OPERATIONAL          5

#endif /* WINE_ROUTING_IPIFCONS_H__ */
