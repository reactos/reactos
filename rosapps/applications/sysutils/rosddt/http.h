#ifndef _HTTP_H_
#define _HTTP_H_

void *http_get(wchar_t *url, u_long *d_size);
void *http_post(wchar_t *url, void *data, int size, u_long *d_size);

#endif
