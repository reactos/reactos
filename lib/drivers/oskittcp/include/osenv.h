#ifndef OSENV_H
#define OSENV_H

static __inline void osenv_intr_enable(void) {}
static __inline void osenv_intr_disable(void) {}

void oskit_bufio_addref(void *buf);
void oskit_bufio_release(void *buf);
void* oskit_bufio_create(int len);
void oskit_bufio_map(void *srcbuf, void**dstbuf, int off, int len);

#define osenv_sleeprec_t void*

#endif
