/* A few helpful macros for implementing COM objects.
 *
 * Copyright 2000 TransGaming Technologies Inc.
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

#ifndef _DDCOMIMPL_H_
#define _DDCOMIMPL_H_

#include <stddef.h>

/* Generates the name for a vtable pointer for a given interface. */
/* The canonical name for a single interface is "lpVtbl". */
#define ICOM_VFIELD_MULTI_NAME2(iface) ITF_##iface
#define ICOM_VFIELD_MULTI_NAME(iface) ICOM_VFIELD_MULTI_NAME2(iface)

/* Declares a vtable pointer field in an implementation. */
#define ICOM_VFIELD_MULTI(iface) \
	iface ICOM_VFIELD_MULTI_NAME(iface)

/* Returns the offset of a vtable pointer within an implementation object. */
#define ICOM_VFIELD_OFFSET(impltype, iface) \
	offsetof(impltype, ICOM_VFIELD_MULTI_NAME(iface))

/* Given an interface pointer, returns the implementation pointer. */
#define ICOM_OBJECT(impltype, ifacename, ifaceptr)		\
	(impltype*)((ifaceptr) == NULL ? NULL			\
		  : (char*)(ifaceptr) - ICOM_VFIELD_OFFSET(impltype,ifacename))

#define ICOM_THIS_FROM(impltype, ifacename, ifaceptr) \
	impltype* This = ICOM_OBJECT(impltype, ifacename, ifaceptr)

/* Given an object and interface name, returns a pointer to that interface. */
#define ICOM_INTERFACE(implobj, iface) \
	(implobj == NULL ? NULL :&((implobj)->ICOM_VFIELD_MULTI_NAME(iface)))

#define ICOM_INIT_INTERFACE(implobj, ifacename, vtblname) \
	do { \
	  (implobj)->ICOM_VFIELD_MULTI_NAME(ifacename).lpVtbl = &(vtblname); \
	} while (0)

#define COM_INTERFACE_CAST(impltype, ifnamefrom, ifnameto, ifaceptr)	\
	ICOM_INTERFACE(ICOM_OBJECT(impltype, ifnamefrom, ifaceptr), ifnameto)

#endif /* _DDCOMIMPL_H_ */
