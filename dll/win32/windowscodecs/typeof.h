#define typeof(X_) __typeof_ ## X_

#ifdef _WIN64
#define __typeof_intptr long long
#define __typeof_longptr long long
#else
#define __typeof_intptr int
#define __typeof_longptr long
#endif

#ifdef __cplusplus
#define __typeof_size size_t
#define __typeof_wchar wchar_t
#else
#define __typeof_size __typeof_intptr
#define __typeof_wchar unsigned short
#endif

typedef struct jpeg_error_mgr * (__cdecl typeof(jpeg_std_error))(struct jpeg_error_mgr *);
typedef void (__cdecl typeof(jpeg_CreateDecompress))(struct jpeg_decompress_struct *, int, __typeof_size);
typedef int (__cdecl typeof(jpeg_read_header))(struct jpeg_decompress_struct *, int);
typedef int (__cdecl typeof(jpeg_start_decompress))(struct jpeg_decompress_struct *);
typedef unsigned int (__cdecl typeof(jpeg_read_scanlines))(struct jpeg_decompress_struct *, unsigned char **, unsigned int);
typedef int (__cdecl typeof(jpeg_finish_decompress))(struct jpeg_decompress_struct *);
typedef void (__cdecl typeof(jpeg_destroy_decompress))(struct jpeg_decompress_struct *);
typedef unsigned char (__cdecl typeof(jpeg_resync_to_restart))(struct jpeg_decompress_struct *, int);

typedef void (*png_error_ptr_1)(struct png_struct_def *, const char *);
typedef struct png_struct_def * (__cdecl typeof(png_create_read_struct))(const char *, void*, png_error_ptr_1, png_error_ptr_1);
typedef struct png_info_def* (__cdecl typeof(png_create_info_struct))(struct png_struct_def *);

// FIXME the following types are not correct, but they help to compile
typedef void* (__cdecl typeof(png_create_write_struct))(void*);
typedef void* (__cdecl typeof(png_destroy_read_struct))(void*, void*, void*);
typedef void* (__cdecl typeof(png_destroy_write_struct))(void*);
typedef void* (__cdecl typeof(png_destroy_write_struct))(void*);
typedef void* (__cdecl typeof(png_error))(void*);
typedef void* (__cdecl typeof(png_get_bit_depth))(void *);
typedef void* (__cdecl typeof(png_get_color_type))(void *);
typedef void* (__cdecl typeof(png_get_error_ptr))(void *);
typedef void* (__cdecl typeof(png_get_image_height))(void *);
typedef void* (__cdecl typeof(png_get_image_width))(void *);
typedef void* (__cdecl typeof(png_get_io_ptr))(void *);
typedef void* (__cdecl typeof(png_get_pHYs))(void *);
typedef void* (__cdecl typeof(png_get_PLTE))(void *);
typedef void* (__cdecl typeof(png_get_tRNS))(void *);
typedef void* (__cdecl typeof(png_set_bgr))(void *);
typedef void* (__cdecl typeof(png_set_error_fn))(void *);
typedef void* (__cdecl typeof(png_set_expand_gray_1_2_4_to_8))(void *);
typedef void* (__cdecl typeof(png_set_filler))(void *);
typedef void* (__cdecl typeof(png_set_gray_to_rgb))(void *);
typedef void* (__cdecl typeof(png_set_IHDR))(void *);
typedef void* (__cdecl typeof(png_set_pHYs))(void *);
typedef void* (__cdecl typeof(png_set_read_fn))(void *);
typedef void* (__cdecl typeof(png_set_strip_16))(void *);
typedef void* (__cdecl typeof(png_set_tRNS_to_alpha))(void *);
typedef void* (__cdecl typeof(png_set_write_fn))(void *);
typedef void* (__cdecl typeof(png_read_end))(void *);
typedef void* (__cdecl typeof(png_read_image))(void *);
typedef void* (__cdecl typeof(png_read_info))(void *);
typedef void* (__cdecl typeof(png_write_end))(void *);
typedef void* (__cdecl typeof(png_write_info))(void *);
typedef void* (__cdecl typeof(png_write_rows))(void *);

typedef void* (__cdecl typeof(TIFFClientOpen))(void *);
typedef void* (__cdecl typeof(TIFFClose))(void *);
typedef char* (__cdecl typeof(TIFFCurrentDirectory))(void *);
typedef void* (__cdecl typeof(TIFFGetField))(void *);
typedef void* (__cdecl typeof(TIFFReadDirectory))(void *);
typedef void* (__cdecl typeof(TIFFReadEncodedStrip))(void *);
typedef void* (__cdecl typeof(TIFFSetDirectory))(void *);

#undef __typeof_intptr
#undef __typeof_longptr
#undef __typeof_wchar
#undef __typeof_size

/* EOF */

