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
#include "functions.h"
#endif

#ifdef __FreeBSD__

#ifdef _KERNEL
#include <net/tme/tme.h>
#include <net/bpf.h>
#include <net/tme/functions.h>
#else
#include <tme/tme.h>
#include <bpf.h>
#include <tme/functions.h>
#endif

#endif



lut_fcn lut_fcn_mapper(uint32 index)
{

	switch (index)
	{
	case NORMAL_LUT_W_INSERT:
		return (lut_fcn) normal_lut_w_insert;

	case NORMAL_LUT_WO_INSERT:
		return (lut_fcn) normal_lut_wo_insert;

	case BUCKET_LOOKUP:
		return (lut_fcn) bucket_lookup;

	case BUCKET_LOOKUP_INSERT:
		return (lut_fcn) bucket_lookup_insert;
	
	default:
		return NULL;
	}
	
	return NULL;

}

exec_fcn exec_fcn_mapper(uint32 index)
{
	switch (index)
	{
	case COUNT_PACKETS:
		return (exec_fcn) count_packets;
	
	case TCP_SESSION:
		return (exec_fcn) tcp_session;
	default:
		return NULL;
	}
	
	return NULL;
}
