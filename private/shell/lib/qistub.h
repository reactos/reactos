#ifndef _QISTUB_H_
#define _QISTUB_H_

#ifdef DEBUG
STDAPI_(BOOL) DBIsQIStub(void *);
STDAPI_(void) DBDumpQIStub(void *);
STDAPI_(BOOL) DBIsQIFunc(int ret, int delta);
#endif

#endif // _QISTUB_H_
