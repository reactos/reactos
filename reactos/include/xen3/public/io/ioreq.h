/*
 * ioreq.h: I/O request definitions for device models
 * Copyright (c) 2004, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 *
 */

#ifndef _IOREQ_H_
#define _IOREQ_H_

#define IOREQ_READ      1
#define IOREQ_WRITE     0

#define STATE_INVALID           0
#define STATE_IOREQ_READY       1
#define STATE_IOREQ_INPROCESS   2
#define STATE_IORESP_READY      3
#define STATE_IORESP_HOOK       4

#define IOPACKET_PORT   2

/* VMExit dispatcher should cooperate with instruction decoder to
   prepare this structure and notify service OS and DM by sending
   virq */
typedef struct {
    u64     addr;               /*  physical address            */
    u64     size;               /*  size in bytes               */
    u64     count;		/*  for rep prefixes            */
    union {
        u64     data;           /*  data                        */
        void    *pdata;         /*  pointer to data             */
    } u;
    u8      state:4;
    u8      pdata_valid:1;	/* if 1, use pdata above        */
    u8      dir:1;		/*  1=read, 0=write             */
    u8      port_mm:1;		/*  0=portio, 1=mmio            */
    u8      df:1;
} ioreq_t;

#define MAX_VECTOR    256
#define BITS_PER_BYTE   8
#define INTR_LEN        (MAX_VECTOR/(BITS_PER_BYTE * sizeof(unsigned long)))

typedef struct {
    ioreq_t         vp_ioreq;
    unsigned long   vp_intr[INTR_LEN];
} vcpu_iodata_t;

#endif /* _IOREQ_H_ */
