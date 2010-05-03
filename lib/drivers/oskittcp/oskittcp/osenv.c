#include "oskittcp.h"

unsigned oskit_freebsd_cpl;

/* We have to store a reference count somewhere so we
 * don't free a buffer being referenced in another mbuf.
 * I just decided to add an extra char to the beginning of
 * the buffer and store the reference count there. I doubt the ref count
 * will ever even get close to 0xFF so we should be ok. Remember that
 * only one thread can ever be inside oskit due to OSKLock so this should
 * be safe.
 */

void oskit_bufio_addref(void *buf)
{
   unsigned char* fullbuf = ((unsigned char*)buf) - sizeof(char);

#if DBG
   if (fullbuf[0] == 0xFF)
       panic("oskit_bufio_addref: ref count overflow");
#endif

   fullbuf[0]++;
}
void oskit_bufio_release(void *buf)
{
   unsigned char* fullbuf = ((unsigned char*)buf) - sizeof(char);

   if (--fullbuf[0] == 0)
       free(fullbuf, 0);
}
void* oskit_bufio_create(int len)
{
   unsigned char* fullbuf = malloc(len + sizeof(char), __FILE__, __LINE__);
   if (fullbuf == NULL)
       return NULL;

   fullbuf[0] = 1;

   return (void*)(fullbuf + sizeof(char));
}
void oskit_bufio_map(void *srcbuf, void**dstbuf, int off, int len)
{
   *dstbuf = srcbuf;
}
