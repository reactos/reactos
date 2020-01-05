#ifndef _FX_MACROS_H_
#define _FX_MACROS_H_

#define FX_TAG 'rDxF'

#define WDFEXPORT(a) imp_ ## a
#define VFWDFEXPORT(a) imp_Vf ## a

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_x) (sizeof((_x))/sizeof((_x)[0]))
#endif

#endif //_FX_MACROS_H_