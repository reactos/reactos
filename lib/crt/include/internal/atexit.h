/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __CRT_INTERNAL_ATEXIT_H
#define __CRT_INTERNAL_ATEXIT_H


struct __atexit {
    struct __atexit* __next;
    void (*__function)(void);
};

extern struct __atexit* __atexit_ptr;


#endif
