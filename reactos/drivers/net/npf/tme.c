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
#endif

#ifdef __FreeBSD__
#include <net/tme/tme.h>
#endif

/* resizes extended memory */ 
uint32 init_extended_memory(uint32 size, MEM_TYPE *mem_ex)
{
	uint8 *tmp;
	
	if ((mem_ex==NULL)||(mem_ex->buffer==NULL)||(size==0))
		return TME_ERROR;  /* awfully never reached!!!! */

	tmp=mem_ex->buffer;
	mem_ex->buffer=NULL;
	FREE_MEMORY(tmp);

	ALLOCATE_MEMORY(tmp,uint8,size);
	if (tmp==NULL)
		return TME_ERROR; /* no memory */
			
	mem_ex->size=size;
	mem_ex->buffer=tmp;
	return TME_SUCCESS;

}

/* activates a block of the TME */
uint32 set_active_tme_block(TME_CORE *tme, uint32 block)  
{

	if ((block>=MAX_TME_DATA_BLOCKS)||(!IS_VALIDATED(tme->validated_blocks,block)))
		return TME_ERROR;
	tme->active=block;
	tme->working=block;
	return TME_SUCCESS;

}

/* simply inserts default values in a TME block      */
/* it DOESN'T initialize the block in the core!!     */
/* FIXME default values are defined at compile time, */
/* it will be useful to store them in the registry   */
uint32 init_tme_block(TME_CORE *tme, uint32 block)
{
	
	TME_DATA *data;
	if (block>=MAX_TME_DATA_BLOCKS)
		return TME_ERROR;
	data=&(tme->block_data[block]);
	tme->working=block;

	ZERO_MEMORY(data,sizeof(TME_DATA));

	/* entries in LUT     */
	data->lut_entries=TME_LUT_ENTRIES_DEFAULT;
	/* blocks             */
	data->shared_memory_blocks=TME_SHARED_MEMORY_BLOCKS_DEFAULT;
	/* block size         */
	data->block_size=TME_BLOCK_SIZE_DEFAULT;
	/* lookup function    */
	data->lookup_code=lut_fcn_mapper(TME_LOOKUP_CODE_DEFAULT);
	/* rehashing value    */
	data->rehashing_value=TME_REHASHING_VALUE_DEFAULT;
	/* out lut function   */
	data->out_lut_exec=TME_OUT_LUT_EXEC_DEFAULT;
	/* default function   */
	data->default_exec=TME_DEFAULT_EXEC_DEFAULT;
	/* extra segment size */
	data->extra_segment_size=TME_EXTRA_SEGMENT_SIZE_DEFAULT;
	

	data->enable_deletion=FALSE;
	data->last_read.tv_sec=0;
	data->last_read.tv_usec=0;
	return TME_SUCCESS;

}
/* it validates a TME block and          */
/* (on OK) inserts the block in the core */
uint32 validate_tme_block(MEM_TYPE *mem_ex, TME_CORE *tme, uint32 block, uint32 mem_ex_offset)
{
	uint32 required_memory;
	uint8 *base=mem_ex_offset+mem_ex->buffer;
	TME_DATA *data;
	
	/* FIXME soluzione un po' posticcia... */
	if (mem_ex_offset==0)
		return TME_ERROR;

	if (block>=MAX_TME_DATA_BLOCKS)
		return TME_ERROR;
	data=&tme->block_data[block];
	
	if (data->lut_entries==0)
		return TME_ERROR;

	if (data->key_len==0)
		return TME_ERROR;

	if (data->shared_memory_blocks==0)
		return TME_ERROR;

	if (data->block_size==0)
		return TME_ERROR;

	/* checks if the lookup function is valid       */
	if (data->lookup_code==NULL)
		return TME_ERROR;

	/* checks if the out lut exec function is valid */
	if (exec_fcn_mapper(data->out_lut_exec)==NULL)
		return TME_ERROR;

	/* checks if the default exec function is valid */
	if (exec_fcn_mapper(data->default_exec)==NULL)
		return TME_ERROR;

	/* let's calculate memory needed                */
	required_memory=data->lut_entries*sizeof(RECORD); /*LUT*/
	required_memory+=data->block_size*data->shared_memory_blocks; /*shared segment*/
	required_memory+=data->extra_segment_size; /*extra segment*/

	if (required_memory>(mem_ex->size-mem_ex_offset))
		return TME_ERROR;  /*not enough memory*/

	/* the TME block can be initialized             */
	ZERO_MEMORY(base,required_memory);
	
	data->lut_base_address=base;
	
	data->shared_memory_base_address=
		data->lut_base_address+
		data->lut_entries*sizeof(RECORD);

	data->extra_segment_base_address=
		data->shared_memory_base_address+
		data->block_size*data->shared_memory_blocks;
	data->filled_blocks=1;
	VALIDATE(tme->validated_blocks,block);
	tme->active=block;
	tme->working=block;
	return TME_SUCCESS;
}
														         
/* I/F between the bpf machine and the callbacks, just some checks */
uint32 lookup_frontend(MEM_TYPE *mem_ex, TME_CORE *tme,uint32 mem_ex_offset, struct time_conv *time_ref)
{
	if (tme->active==TME_NONE_ACTIVE)
		return TME_FALSE;
	
	return (tme->block_data[tme->active].lookup_code)(mem_ex_offset+mem_ex->buffer,&tme->block_data[tme->active],mem_ex, time_ref);
}

/* I/F between the bpf machine and the callbacks, just some checks */
uint32 execute_frontend(MEM_TYPE *mem_ex, TME_CORE *tme, uint32 pkt_size, uint32 offset)
{
	
	exec_fcn tmp;
	TME_DATA *data;
	uint8 *block;
	uint8 *mem_data;

	if (tme->active==TME_NONE_ACTIVE)
		return TME_ERROR;

	data=&tme->block_data[tme->active];
	
	if (data->last_found==NULL)
	{	/*out lut exec */
		tmp=exec_fcn_mapper(data->out_lut_exec);
		block=data->shared_memory_base_address;
	}
	else
	{   /*checks if last_found is valid */
		if ((data->last_found<data->lut_base_address)||(data->last_found>=data->shared_memory_base_address))
			return TME_ERROR;
		else
		{
			tmp=exec_fcn_mapper(SW_ULONG_AT(&((RECORD*)data->last_found)->exec_fcn,0));
			if (tmp==NULL)
				return TME_ERROR;
			block=SW_ULONG_AT(&((RECORD*)data->last_found)->block,0)+mem_ex->buffer;
			if ((block<data->shared_memory_base_address)||(block>=data->extra_segment_base_address))
				return TME_ERROR;
		}
	}
	
	if (offset>=mem_ex->size)
		return TME_ERROR;
	
	mem_data=mem_ex->buffer+offset;
	
	return tmp(block,pkt_size,data,mem_ex,mem_data);
}

/*resets all the TME core*/
uint32 reset_tme(TME_CORE *tme)
{
	if (tme==NULL)
		return TME_ERROR;
	ZERO_MEMORY(tme, sizeof(TME_CORE));	
	return TME_SUCCESS;
}
	
/* returns a register value of the active TME block   */
/* FIXME last found in maniera elegante e veloce ?!?! */
uint32 get_tme_block_register(TME_DATA *data,MEM_TYPE *mem_ex,uint32 rgstr,uint32 *rval)
{
	switch(rgstr)
	{
	case TME_LUT_ENTRIES:
		*rval=data->lut_entries;
		return TME_SUCCESS;
	case TME_MAX_FILL_STATE:
		*rval=data->max_fill_state;
		return TME_SUCCESS;
	case TME_REHASHING_VALUE:
		*rval=data->rehashing_value;
		return TME_SUCCESS;
	case TME_KEY_LEN:
		*rval=data->key_len;
		return TME_SUCCESS;
	case TME_SHARED_MEMORY_BLOCKS:
		*rval=data->shared_memory_blocks;
		return TME_SUCCESS;
	case TME_FILLED_ENTRIES:
		*rval=data->filled_entries;
		return TME_SUCCESS;
	case TME_BLOCK_SIZE:
		*rval=data->block_size;
		return TME_SUCCESS;
	case TME_EXTRA_SEGMENT_SIZE:
		*rval=data->extra_segment_size;
		return TME_SUCCESS;
	case TME_FILLED_BLOCKS:
		*rval=data->filled_blocks;
		return TME_SUCCESS;
	case TME_DEFAULT_EXEC:
		*rval=data->default_exec;
		return TME_SUCCESS;
	case TME_OUT_LUT_EXEC:
		*rval=data->out_lut_exec;
		return TME_SUCCESS;
	case TME_SHARED_MEMORY_BASE_ADDRESS:
		*rval=data->shared_memory_base_address-mem_ex->buffer;
		return TME_SUCCESS;
	case TME_LUT_BASE_ADDRESS:
		*rval=data->lut_base_address-mem_ex->buffer;
		return TME_SUCCESS;
	case TME_EXTRA_SEGMENT_BASE_ADDRESS:
		*rval=data->extra_segment_base_address-mem_ex->buffer;
		return TME_SUCCESS;
	case TME_LAST_FOUND_BLOCK:
		if (data->last_found==NULL)
			*rval=0;
		else
			*rval=data->last_found-mem_ex->buffer;
		return TME_SUCCESS;

	default:
		return TME_ERROR;
	}
}

/* sets a register value in the active block          */
/* FIXME last found in maniera elegante e veloce ?!?! */
uint32 set_tme_block_register(TME_DATA *data,MEM_TYPE *mem_ex,uint32 rgstr,uint32 value, int32 init)
{	/* very very very dangerous!!!!!!!!!!! */
	lut_fcn tmp;
	switch(rgstr)
	{
	case TME_MAX_FILL_STATE:
		data->max_fill_state=value;
		return TME_SUCCESS;
	case TME_REHASHING_VALUE:
		data->rehashing_value=value;
		return TME_SUCCESS;
	case TME_FILLED_ENTRIES: 
		data->filled_entries=value;
		return TME_SUCCESS;
	case TME_FILLED_BLOCKS:  
		if (value<=data->shared_memory_blocks)
		{
			data->filled_blocks=value;
			return TME_SUCCESS;
		}
		else
			return TME_ERROR;
	case TME_DEFAULT_EXEC:  
		data->default_exec=value;
		return TME_SUCCESS;
	case TME_OUT_LUT_EXEC:
		data->out_lut_exec=value;
		return TME_SUCCESS;
	case TME_LOOKUP_CODE:
		tmp=lut_fcn_mapper(value);
		if (tmp==NULL)
			return TME_ERROR;
		else
			data->lookup_code=tmp;
		return TME_SUCCESS;
	default:
		break;
	}

	if (init)
		switch (rgstr)
		{

		case TME_LUT_ENTRIES: 
			data->lut_entries=value;
			return TME_SUCCESS;
		case TME_KEY_LEN: 
			data->key_len=value;
			return TME_SUCCESS;
		case TME_SHARED_MEMORY_BLOCKS: 
			data->shared_memory_blocks=value;
			return TME_SUCCESS;
		case TME_BLOCK_SIZE:  
			data->block_size=value;
			return TME_SUCCESS;
		case TME_EXTRA_SEGMENT_SIZE: 
			data->extra_segment_size=value;
			return TME_SUCCESS;
		default:
			return TME_ERROR;
		}
	else
		return TME_ERROR;

}

/* chooses the TME block for read */
uint32 set_active_read_tme_block(TME_CORE *tme, uint32 block)  
{

	if ((block>=MAX_TME_DATA_BLOCKS)||(!IS_VALIDATED(tme->validated_blocks,block)))
		return TME_ERROR;
	tme->active_read=block;
	return TME_SUCCESS;

}

/* chooses if the autodeletion must be used */
uint32 set_autodeletion(TME_DATA *data, uint32 value)
{
	if (value==0)  /* no autodeletion */
		data->enable_deletion=FALSE;
	else
		data->enable_deletion=TRUE;

	return TME_SUCCESS;
}