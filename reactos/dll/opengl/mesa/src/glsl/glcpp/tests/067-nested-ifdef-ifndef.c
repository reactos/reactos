#define D1
#define D2

#define result success

#ifdef U1
#ifdef U2
#undef result
#define result failure
#endif
#endif
result

#ifndef D1
#ifndef D2
#undef result
#define result failure
#endif
#endif
result

#undef result
#define result failure
#ifdef D1
#ifdef D2
#undef result
#define result success
#endif
#endif
result

#undef result
#define result failure
#ifndef U1
#ifndef U2
#undef result
#define result success
#endif
#endif
result
