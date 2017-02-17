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

struct jpeg_decompress_struct;
struct jpeg_compress_struct;
struct png_struct_def;
struct png_color_struct;
struct png_color_16_struct;

typedef struct jpeg_error_mgr * (__cdecl typeof(jpeg_std_error))(struct jpeg_error_mgr *);
typedef void (__cdecl typeof(jpeg_CreateDecompress))(struct jpeg_decompress_struct *, int, __typeof_size);
typedef int (__cdecl typeof(jpeg_read_header))(struct jpeg_decompress_struct *, unsigned char);
typedef unsigned char (__cdecl typeof(jpeg_start_decompress))(struct jpeg_decompress_struct *);
typedef unsigned int (__cdecl typeof(jpeg_read_scanlines))(struct jpeg_decompress_struct *, unsigned char **, unsigned int);
typedef unsigned char (__cdecl typeof(jpeg_finish_decompress))(struct jpeg_decompress_struct *);
typedef void (__cdecl typeof(jpeg_destroy_decompress))(struct jpeg_decompress_struct *);
typedef unsigned char (__cdecl typeof(jpeg_resync_to_restart))(struct jpeg_decompress_struct *, int);
typedef void (__cdecl typeof(jpeg_CreateCompress))(struct jpeg_compress_struct *, int, __typeof_size);
typedef void (__cdecl typeof(jpeg_start_compress))(struct jpeg_compress_struct *, unsigned char);
typedef void (__cdecl typeof(jpeg_destroy_compress))(struct jpeg_compress_struct *);
typedef void (__cdecl typeof(jpeg_finish_compress))(struct jpeg_compress_struct *);
typedef void (__cdecl typeof(jpeg_set_defaults))(struct jpeg_compress_struct *);
typedef unsigned int (__cdecl typeof(jpeg_write_scanlines))(struct jpeg_compress_struct *, char **, unsigned int);

typedef void (*png_error_ptr_1)(struct png_struct_def *, const char *);
typedef void (*png_rw_ptr_1)(struct png_struct_def *, unsigned char *, unsigned int);
typedef void (*png_flush_ptr_1)(struct png_struct_def *);
typedef struct png_info_def* (__cdecl typeof(png_create_info_struct))(struct png_struct_def *);
typedef struct png_struct_def * (__cdecl typeof(png_create_read_struct))(const char *, void *, png_error_ptr_1, png_error_ptr_1);
typedef struct png_struct_def *(__cdecl typeof(png_create_write_struct))(const char *, void *, png_error_ptr_1, png_error_ptr_1);
typedef void (__cdecl typeof(png_destroy_info_struct))(struct png_struct_def *, struct png_info_def **);
typedef void (__cdecl typeof(png_destroy_read_struct))(struct png_struct_def **, struct png_info_def **, struct png_info_def **);
typedef void (__cdecl typeof(png_destroy_write_struct))(struct png_struct_def **, struct png_info_def **);
typedef void (__cdecl typeof(png_error))(struct png_struct_def *, const char *);
typedef unsigned char (__cdecl typeof(png_get_bit_depth))(const struct png_struct_def *, const struct png_info_def *);
typedef unsigned char (__cdecl typeof(png_get_color_type))(const struct png_struct_def *, const struct png_info_def *);
typedef void *(__cdecl typeof(png_get_error_ptr))(const struct png_struct_def *);
typedef unsigned int (__cdecl typeof(png_get_image_height))(const struct png_struct_def *, const struct png_info_def *);
typedef unsigned int (__cdecl typeof(png_get_image_width))(const struct png_struct_def *, const struct png_info_def *);
typedef void *(__cdecl typeof(png_get_io_ptr))(struct png_struct_def *);
typedef unsigned int (__cdecl typeof(png_get_pHYs))(const struct png_struct_def *, const struct png_info_def *, unsigned int *, unsigned int *, int *);
typedef unsigned int (__cdecl typeof(png_get_PLTE))(const struct png_struct_def *, const struct png_info_def *, struct png_color_struct **, int *);
typedef unsigned int (__cdecl typeof(png_get_tRNS))(const struct png_struct_def *, struct png_info_def *, unsigned char **, int *, struct png_color_16_struct **);
typedef void (__cdecl typeof(png_set_bgr))(struct png_struct_def *);
typedef void (__cdecl typeof(png_set_error_fn))(struct png_struct_def *, void *, png_error_ptr_1, png_error_ptr_1);
typedef void (__cdecl typeof(png_set_expand_gray_1_2_4_to_8))(struct png_struct_def *);
typedef void (__cdecl typeof(png_set_filler))(struct png_struct_def *, unsigned int, int);
typedef int (__cdecl typeof(png_set_interlace_handling))(struct png_struct_def *);
typedef void (__cdecl typeof(png_set_gray_to_rgb))(struct png_struct_def *);
typedef void (__cdecl typeof(png_set_IHDR))(struct png_struct_def *, struct png_info_def *, unsigned int, unsigned int, int, int, int, int, int);
typedef void (__cdecl typeof(png_set_pHYs))(struct png_struct_def *, struct png_info_def *, unsigned int, unsigned int, int);
typedef void (__cdecl typeof(png_set_read_fn))(struct png_struct_def *, void *, png_rw_ptr_1);
typedef void (__cdecl typeof(png_set_strip_16))(struct png_struct_def *);
typedef void (__cdecl typeof(png_set_tRNS_to_alpha))(struct png_struct_def *);
typedef void (__cdecl typeof(png_set_write_fn))(struct png_struct_def *, void *, png_rw_ptr_1, png_flush_ptr_1);
typedef void (__cdecl typeof(png_read_end))(struct png_struct_def *, struct png_info_def *);
typedef void (__cdecl typeof(png_read_image))(struct png_struct_def *, unsigned char **);
typedef void (__cdecl typeof(png_read_info))(struct png_struct_def *, struct png_info_def *);
typedef void (__cdecl typeof(png_write_end))(struct png_struct_def *, struct png_info_def *);
typedef void (__cdecl typeof(png_write_info))(struct png_struct_def *, struct png_info_def *);
typedef void (__cdecl typeof(png_write_rows))(struct png_struct_def *, unsigned char **row, unsigned int);
typedef unsigned int (__cdecl typeof(png_get_iCCP))(struct png_struct_def *, struct png_info_def *, char **, int *, char **, unsigned int *);
typedef void (__cdecl typeof(png_set_crc_action))(struct png_struct_def *, int, int);
typedef void (__stdcall typeof(png_set_PLTE))(struct png_struct_def *, struct png_info_def *, const struct png_color_struct *, int);
typedef void (__stdcall typeof(png_set_tRNS))(struct png_struct_def *, struct png_info_def *, const unsigned char *, int, const struct png_color_16_struct *);
typedef void *thandle_t_1;
typedef int (*TIFFReadWriteProc_1)(thandle_t_1, void *, __typeof_intptr);
typedef unsigned int (*TIFFSeekProc_1)(void *, unsigned int, int);
typedef int (*TIFFCloseProc_1)(thandle_t_1);
typedef unsigned int (*TIFFSizeProc_1)(thandle_t_1);
typedef int (*TIFFMapFileProc_1)(thandle_t_1, void **, unsigned int *);
typedef void (*TIFFUnmapFileProc_1)(thandle_t_1, void *, unsigned int);
typedef struct tiff *(__cdecl typeof(TIFFClientOpen))(const char *, const char *, thandle_t_1, TIFFReadWriteProc_1, TIFFReadWriteProc_1, TIFFSeekProc_1, TIFFCloseProc_1, TIFFSizeProc_1, TIFFMapFileProc_1, TIFFUnmapFileProc_1);
typedef void (__cdecl typeof(TIFFClose))(struct tiff *);
typedef unsigned short (__cdecl typeof(TIFFCurrentDirectory))(struct tiff *);
typedef int (__cdecl typeof(TIFFGetField))(struct tiff *, unsigned int, ...);
typedef int (__cdecl typeof(TIFFReadDirectory))(struct tiff *);
typedef int (__cdecl typeof(TIFFReadEncodedStrip))(struct tiff *, unsigned int, void *, int);
typedef int (__cdecl typeof(TIFFSetDirectory))(struct tiff *, unsigned short);
typedef unsigned int (__cdecl typeof(TIFFCurrentDirOffset))(struct tiff *);
typedef int (__cdecl typeof(TIFFIsByteSwapped))(struct tiff *);
typedef unsigned short (__cdecl typeof(TIFFNumberOfDirectories))(struct tiff *);
typedef long (__cdecl typeof(TIFFReadEncodedTile))(struct tiff *, unsigned long, void *, long);
typedef int (__cdecl typeof(TIFFSetField))(struct tiff *, unsigned long, ...);
typedef int (__cdecl typeof(TIFFWriteDirectory))(struct tiff *);
typedef int (__cdecl typeof(TIFFWriteScanline))(struct tiff *, void *, unsigned long, unsigned short);

#undef __typeof_intptr
#undef __typeof_longptr
#undef __typeof_wchar
#undef __typeof_size

/* EOF */

