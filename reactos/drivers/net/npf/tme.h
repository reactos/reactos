/*
 * Copyright (c) 2001
 *  Politecnico di Torino.  All rights reserved.
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

#ifndef __tme_include_
#define __tme_include_

#ifdef WIN_NT_DRIVER

#include "ntddk.h"
#include "memory_t.h"
#include "time_calls.h"
#endif /*WIN_NT_DRIVER*/

#ifdef WIN32
#include "memory_t.h"
#include "time_calls.h"
#endif /*WIN32*/



#ifdef __FreeBSD__

#ifdef _KERNEL
#include <net/tme/time_calls.h>
#else
#include <tme/time_calls.h>
#endif

#endif

/* error codes */
#define     TME_ERROR           0   
#define     TME_SUCCESS         1
#define     TME_TRUE            2
#define     TME_FALSE           3

/* some constants */
#define     DEFAULT_MEM_EX_SIZE     65536
#define     MAX_TME_DATA_BLOCKS     4
#define     TME_NONE_ACTIVE         0xffffffff
#define     DELTA_READ              2  /* secs */

#define     TME_LUT_ENTRIES                 0x00000000  
#define     TME_MAX_FILL_STATE              0x00000001  /*potrebbe servire per un thread a passive level!?!?! */
#define     TME_REHASHING_VALUE             0x00000002  
#define     TME_KEY_LEN                     0x00000003
#define     TME_SHARED_MEMORY_BLOCKS        0x00000004
#define     TME_FILLED_ENTRIES              0x00000005
#define     TME_BLOCK_SIZE                  0x00000006
#define     TME_EXTRA_SEGMENT_SIZE          0x00000007
#define     TME_LOOKUP_CODE                 0x00000008
#define     TME_OUT_LUT_EXEC                0x00000009
#define     TME_FILLED_BLOCKS               0x0000000a
#define     TME_DEFAULT_EXEC                0x0000000b
#define     TME_LUT_BASE_ADDRESS            0x0000000c
#define     TME_SHARED_MEMORY_BASE_ADDRESS  0x0000000d
#define     TME_EXTRA_SEGMENT_BASE_ADDRESS  0x0000000e
#define     TME_LAST_FOUND                  0x0000000f   /* contains the offset of the last found entry */
#define     TME_LAST_FOUND_BLOCK            0x00000010
/* TME default values */
#define     TME_LUT_ENTRIES_DEFAULT             32007
#define     TME_REHASHING_VALUE_DEFAULT         1
#define     TME_SHARED_MEMORY_BLOCKS_DEFAULT    16000
#define     TME_BLOCK_SIZE_DEFAULT              64
#define     TME_EXTRA_SEGMENT_SIZE_DEFAULT      0
#define     TME_LOOKUP_CODE_DEFAULT             0
#define     TME_OUT_LUT_EXEC_DEFAULT            0
#define     TME_DEFAULT_EXEC_DEFAULT            0
#define     TME_MAX_FILL_STATE_DEFAULT          15000

#define IS_VALIDATED(src,index) (src&(1<<index))

#define VALIDATE(src,index) src|=(1<<index);


#define FORCE_NO_DELETION(timestamp)  (struct timeval*)(timestamp)->tv_sec=0x7fffffff;

/* TME callback prototypes */
#ifndef __GNUC__
typedef uint32 (*lut_fcn)(uint8 *key, struct __TME_DATA *data,MEM_TYPE *mem_ex, struct time_conv *time_ref );
typedef uint32 (*exec_fcn)(uint8 *block, uint32 pkt_size, struct __TME_DATA *data, MEM_TYPE *mem_ex, uint8 *mem_data);
#else
typedef uint32 (*lut_fcn)(uint8 *key, void *data,MEM_TYPE *mem_ex, struct time_conv *time_ref );
typedef uint32 (*exec_fcn)(uint8 *block, uint32 pkt_size, void *data, MEM_TYPE *mem_ex, uint8 *mem_data);
#endif

/* DO NOT MODIFY THIS STRUCTURE!!!! GV */
typedef struct __RECORD

{
    uint32 block;
    uint32 exec_fcn;
}
    RECORD, *PRECORD;

/* TME data registers */
struct __TME_DATA
{
    uint32 lut_entries;
    uint32 max_fill_state;
    uint32 rehashing_value;
    uint32 key_len;
    uint32 shared_memory_blocks;
    uint32 filled_entries;
    uint32 block_size;
    uint32 extra_segment_size;
    uint32 filled_blocks;
    lut_fcn lookup_code;
    uint32 default_exec;
    uint32 out_lut_exec;
    uint8 *lut_base_address;
    uint8 *shared_memory_base_address;
    uint8 *extra_segment_base_address;
    struct timeval last_read;
    uint32  enable_deletion;
    uint8 *last_found;
};

typedef struct __TME_DATA TME_DATA,*PTME_DATA;



/* TME core */
typedef struct __TME_CORE
{
    uint32      working;
    uint32      active;
    uint32      validated_blocks;
    TME_DATA    block_data[MAX_TME_DATA_BLOCKS];
    uint32      active_read;
    
} TME_CORE, *PTME_CORE;

static __inline int32 IS_DELETABLE(void *timestamp, TME_DATA *data)
{
    struct timeval *ts=(struct timeval*)timestamp;

    if (data->enable_deletion==FALSE)
        return FALSE;
    if (data->filled_entries<data->max_fill_state)
        return FALSE;
    if ((ts->tv_sec+DELTA_READ)<data->last_read.tv_sec)
        return TRUE;
    return FALSE;
}

/* functions to manage TME */
uint32 init_tme_block(TME_CORE *tme, uint32 block);
uint32 validate_tme_block(MEM_TYPE *mem_ex, TME_CORE *tme, uint32 block, uint32 mem_ex_offset);
uint32 lookup_frontend(MEM_TYPE *mem_ex, TME_CORE *tme,uint32 mem_ex_offset, struct time_conv *time_ref);
uint32 execute_frontend(MEM_TYPE *mem_ex, TME_CORE *tme, uint32 pkt_size,uint32 offset);
uint32 set_active_tme_block(TME_CORE *tme, uint32 block);
uint32 init_extended_memory(uint32 size, MEM_TYPE *mem_ex);
uint32 reset_tme(TME_CORE *tme);
uint32 get_tme_block_register(TME_DATA *data,MEM_TYPE *mem_ex,uint32 rgstr,uint32 *rval);
uint32 set_tme_block_register(TME_DATA *data,MEM_TYPE *mem_ex,uint32 rgstr,uint32 value, int32 init);
uint32 set_active_read_tme_block(TME_CORE *tme, uint32 block);
uint32 set_autodeletion(TME_DATA *data, uint32 value);

/* function mappers */
lut_fcn lut_fcn_mapper(uint32 index);
exec_fcn exec_fcn_mapper(uint32 index);

#endif