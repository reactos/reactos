#ifndef OSENV_H
#define OSENV_H

static __inline void osenv_intr_enable(void) {}
static __inline void osenv_intr_disable(void) {}

void oskit_bufio_addref(void *buf);
void oskit_bufio_release(void *buf);
void* oskit_bufio_create(int len);
void oskit_bufio_map(void *srcbuf, void**dstbuf, int off, int len);

#define osenv_sleeprec_t void*

/* We can do this safely because SocketContext will always
 * be the first member in the real CONNECTION_ENDPOINT struct
 */
typedef struct _FAKE_CONNECTION_ENDPOINT
{
    void *SocketContext;
} *PCONNECTION_ENDPOINT;

#endif
