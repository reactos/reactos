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
#include "tcp_session.h"
#endif

#ifdef __FreeBSD

#ifdef _KERNEL
#include <net/tme/tme.h>
#include <net/tme/tcp_session.h>
#else
#include <tme/tme.h>
#include <tme/tcp_session.h>
#endif

#endif

uint32 tcp_session(uint8 *block, uint32 pkt_size, TME_DATA *data, MEM_TYPE *mem_ex, uint8 *mem_data)

{

	uint32 next_status;  
	uint32 direction=ULONG_AT(mem_data,12);
	uint8 flags=mem_ex->buffer[25];
	tcp_data *session=(tcp_data*)(block+data->key_len*4);
	
	session->last_timestamp=session->timestamp_block;
	session->timestamp_block.tv_sec=0x7fffffff;
	
	if (direction==session->direction)
	{
		session->pkts_cln_to_srv++;
		session->bytes_cln_to_srv+=pkt_size;
	}
	else
	{
		session->pkts_srv_to_cln++;
		session->bytes_srv_to_cln+=pkt_size;
	}
	/* we use only thes four flags, we don't need PSH or URG */
	flags&=(ACK|FIN|SYN|RST);
	
	switch (session->status)
	{
	case ERROR_TCP:
		next_status=ERROR_TCP;
		break;
	
	case UNKNOWN:
		if (flags==SYN)
		{
			if (SW_ULONG_AT(mem_ex->buffer,20)!=0)
			{

				next_status=ERROR_TCP;
				break;
			}
			next_status=SYN_RCV;
			session->syn_timestamp=session->last_timestamp;

			session->direction=direction;
			session->seq_n_0_cln=SW_ULONG_AT(mem_ex->buffer,16);
		}
		else
			next_status=UNKNOWN;
		break;

	case SYN_RCV:
		if ((flags&RST)&&(direction!=session->direction))
		{
			next_status=CLOSED_RST;
			break;
		}
		if ((flags==SYN)&&(direction==session->direction))
		{	/* two syns... */
			next_status=SYN_RCV;
			session->seq_n_0_cln=SW_ULONG_AT(mem_ex->buffer,16);
			break;
		}
						
		if ((flags==(SYN|ACK))&&(direction!=session->direction))
		{		
			if (SW_ULONG_AT(mem_ex->buffer,20)!=session->seq_n_0_cln+1)
			{
				next_status=ERROR_TCP;
				break;
			}
			next_status=SYN_ACK_RCV;
			
			session->syn_ack_timestamp=session->last_timestamp;

			session->seq_n_0_srv=SW_ULONG_AT(mem_ex->buffer,16);
			session->ack_cln=session->seq_n_0_cln+1;
		}
		else
		{
			next_status=ERROR_TCP;
		}
		break;

	case SYN_ACK_RCV:
		if ((flags&ACK)&&(flags&RST)&&(direction==session->direction))
		{
			next_status=CLOSED_RST;
			session->ack_srv=SW_ULONG_AT(mem_ex->buffer,20);
			break;
		}
		
		if ((flags==ACK)&&(!(flags&(SYN|FIN|RST)))&&(direction==session->direction))
		{
			if (SW_ULONG_AT(mem_ex->buffer,20)!=session->seq_n_0_srv+1)
			{
				next_status=ERROR_TCP;
				break;
			}
			next_status=ESTABLISHED;
			session->ack_srv=session->seq_n_0_srv+1;
			break;
		}
		if ((flags&ACK)&&(flags&SYN)&&(direction!=session->direction))
		{
			next_status=SYN_ACK_RCV;
			break;
		}

		next_status=ERROR_TCP;
		break;
	
	case ESTABLISHED:
		if (flags&SYN)
		{
			if ((flags&ACK)&&
				(direction!=session->direction)&&
				((session->ack_cln-SW_ULONG_AT(mem_ex->buffer,20))<MAX_WINDOW)
				)
			{	/* SYN_ACK duplicato */
				next_status=ESTABLISHED;
				break;
			}
			
			if ((!(flags&ACK))&&
				(direction==session->direction)&&
				(SW_ULONG_AT(mem_ex->buffer,16)==session->seq_n_0_cln)&&
				(ULONG_AT(mem_ex->buffer,20)==0)
				)
			{	/* syn duplicato */
				next_status=ESTABLISHED;
				break;
			}
						
			next_status=ERROR_TCP;
			break;
		}
		if (flags&ACK)
			if (direction==session->direction)
			{
				uint32 new_ack=SW_ULONG_AT(mem_ex->buffer,20);
				if (new_ack-session->ack_srv<MAX_WINDOW)
					session->ack_srv=new_ack;
			}
			else
			{
				uint32 new_ack=SW_ULONG_AT(mem_ex->buffer,20);
				if (new_ack-session->ack_cln<MAX_WINDOW)
					session->ack_cln=new_ack;
			}
		if (flags&RST)
		{
			next_status=CLOSED_RST;
			break;
		}
		if (flags&FIN)
			if (direction==session->direction)
			{   /* an hack to make all things work */
				session->ack_cln=SW_ULONG_AT(mem_ex->buffer,16);
				next_status=FIN_CLN_RCV;
				break;
			}
			else
			{
				session->ack_srv=SW_ULONG_AT(mem_ex->buffer,16);
				next_status=FIN_SRV_RCV;
				break;
			}
		next_status=ESTABLISHED;
		break;
	
	case CLOSED_RST:
		next_status=CLOSED_RST;
		break;
	
	case FIN_SRV_RCV:	
		if (flags&SYN)
		{
			next_status=ERROR_TCP;
			break;
		}
			
		next_status=FIN_SRV_RCV;
		
		if (flags&ACK)
		{
			uint32 new_ack=SW_ULONG_AT(mem_ex->buffer,20);
			if (direction!=session->direction)
				if ((new_ack-session->ack_cln)<MAX_WINDOW)
					session->ack_cln=new_ack;
		}
		
		if (flags&RST)
			next_status=CLOSED_RST;
		else
			if ((flags&FIN)&&(direction==session->direction))
			{
				session->ack_cln=SW_ULONG_AT(mem_ex->buffer,16);
				next_status=CLOSED_FIN;
			}

		break;

	case FIN_CLN_RCV:
		if (flags&SYN)
		{
			next_status=ERROR_TCP;
			break;
		}
			
		next_status=FIN_CLN_RCV;
		
		if (flags&ACK)
		{
			uint32 new_ack=SW_ULONG_AT(mem_ex->buffer,20);
			if (direction==session->direction)
				if (new_ack-session->ack_srv<MAX_WINDOW)
					session->ack_srv=new_ack;
		}
		
		if (flags&RST)
			next_status=CLOSED_RST;
		else
			if ((flags&FIN)&&(direction!=session->direction))
			{
				session->ack_srv=SW_ULONG_AT(mem_ex->buffer,16);
				next_status=CLOSED_FIN;
			}

		break;

	case CLOSED_FIN:
			next_status=CLOSED_FIN;
		break;
	default:
		next_status=ERROR_TCP;

	}

	session->status=next_status;
	
	if ((next_status==CLOSED_FIN)||(next_status==UNKNOWN)||(next_status==CLOSED_RST)||(next_status==ERROR_TCP))
		session->timestamp_block=session->last_timestamp;
	
	return TME_SUCCESS;
}