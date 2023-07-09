#ifndef _CVWMEMMAP
#define _CVWMEMMAP

#ifdef HOST32
#define _HUGE_
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _fmalloc
#undef _fmalloc
#endif // _fmalloc
#define _fmalloc(x) cvw3_fmalloc(x)
void FAR * cvw3_fmalloc( size_t );

#ifdef halloc
#undef halloc
#endif // halloc
#define halloc(x,y) cvw3_halloc(x,y)
void _HUGE_ * CDECL cvw3_halloc( long, size_t );

#ifdef _fmsize
#undef _fmsize
#endif // _fmsize
#define _fmsize(x) cvw3_fmsize(x)
size_t cvw3_fmsize(void FAR *buffer);

#ifdef _frealloc
#undef _frealloc
#endif // _frealloc
#define _frealloc(x,y) cvw3_frealloc(x,y)
void FAR *cvw3_frealloc(void FAR *buffer, size_t size);

#ifdef _ffree
#undef _ffree
#endif // _ffree
#define _ffree(x) cvw3_ffree(x)
void cvw3_ffree( void FAR * );

#ifdef hfree
#undef hfree
#endif // hfree
#define hfree(x) cvw3_hfree(x)
void cvw3_hfree( void _HUGE_ * );

#ifdef __cplusplus
} //extern "C"
#endif

#endif // !_CVWMEMMAP
