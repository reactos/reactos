/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * here's vm.h's primary purpose in life:
 *
 * define `vm_map_t' for the first arg to kmem_alloc()
 */
#ifndef _FAKE_VM_H
#define _FAKE_VM_H

typedef void *vm_map_t; 

#include <vm/vm_param.h>

/*
 * XXX This is used by syscons to compute the address of the video buffer.
 * This definition means the driver will only work when kva == pa.
 * The real solution would be to modify the driver to map the buffer explicitly.
 */
#define vtophys(va) va

/* 
 * ip_icmp.c and possibly other files rely on vm/vm.h to get sys/proc ... 
 */
#include <sys/proc.h>
#include <sys/queue.h>

#endif /* _FAKE_VM_H */
