/*
 * Copyright (c) 2001
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifdef WIN32
#include "tme.h"
#include "count_packets.h"
#endif

#ifdef __FreeBSD__

#ifdef _KERNEL
#include <net/tme/tme.h>
#include <net/tme/count_packets.h>
#else
#include <tme/tme.h>
#include <tme/count_packets.h>
#endif

#endif



uint32 count_packets(uint8 *block, uint32 pkt_size, TME_DATA *data, MEM_TYPE *mem_ex, uint8 *mem_data)
{
		
	c_p_data *counters=(c_p_data*)(block+data->key_len*4);

	counters->bytes+=pkt_size;
	counters->packets++;
	
	return TME_SUCCESS;

}
