/******************************************************************************
 * trace.h
 * 
 * Mark Williamson, (C) 2004 Intel Research Cambridge
 */

#ifndef __XEN_PUBLIC_TRACE_H__
#define __XEN_PUBLIC_TRACE_H__

/* This structure represents a single trace buffer record. */
struct t_rec {
    u64 cycles;               /* 64 bit cycle counter timestamp */
    u32 event;                /* 32 bit event ID                */
    u32 d1, d2, d3, d4, d5;   /* event data items               */
};

/*
 * This structure contains the metadata for a single trace buffer.  The head
 * field, indexes into an array of struct t_rec's.
 */
struct t_buf {
    unsigned long data;      /* pointer to data area.  machine address
                              * for convenience in user space code           */

    unsigned long size;      /* size of the data area, in t_recs             */
    unsigned long head;      /* array index of the most recent record        */

    /* Xen-private elements follow... */
    struct t_rec *head_ptr; /* pointer to the head record                    */
    struct t_rec *vdata;    /* virtual address pointer to data               */
};

#endif /* __XEN_PUBLIC_TRACE_H__ */
