/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_STDIO
#define _INC_STDIO

#include <corecrt.h>

#define __need___va_list
#include <stdarg.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#define BUFSIZ 512
#define _NFILE _NSTREAM_
#define _NSTREAM_ 512
#define _IOB_ENTRIES 20
#define EOF (-1)

#ifndef _FILE_DEFINED
  struct _iobuf {
    char *_ptr;
    int _cnt;
    char *_base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char *_tmpfname;
  };
  typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

#ifdef _POSIX_
#define _P_tmpdir "/"
#define _wP_tmpdir L"/"
#else
#define _P_tmpdir "\\"
#define _wP_tmpdir L"\\"
#endif

#define L_tmpnam (sizeof(_P_tmpdir) + 12)

#ifdef _POSIX_
#define L_ctermid 9
#define L_cuserid 32
#endif

#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0

#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2

#define FILENAME_MAX 260
#define FOPEN_MAX 20
#define _SYS_OPEN 20
#define TMP_MAX 32767

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
#ifndef _OFF_T_
#define _OFF_T_
  typedef long _off_t;
#if !defined(NO_OLDNAMES) || defined(_POSIX)
  typedef long off_t;
#endif
#endif
#endif

#ifndef _OFF64_T_DEFINED
#define _OFF64_T_DEFINED
  __MINGW_EXTENSION typedef long long _off64_t;
#if !defined(NO_OLDNAMES) || defined(_POSIX)
  __MINGW_EXTENSION typedef long long off64_t;
#endif
#endif

#ifndef _STDIO_DEFINED
  _CRTIMP FILE *__cdecl __iob_func(void);
  _CRTDATA(extern FILE _iob[];)
#ifdef _M_CEE_PURE
#define _iob __iob_func()
#endif
#endif

#ifndef _STDSTREAM_DEFINED
#define _STDSTREAM_DEFINED
#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])
#endif /* !_STDSTREAM_DEFINED */

#ifndef _FPOS_T_DEFINED
#define _FPOS_T_DEFINED
#undef _FPOSOFF

#if (!defined(NO_OLDNAMES) || defined(__GNUC__)) && _INTEGRAL_MAX_BITS >= 64
  __MINGW_EXTENSION typedef __int64 fpos_t;
#define _FPOSOFF(fp) ((long)(fp))
#else
  __MINGW_EXTENSION typedef long long fpos_t;
#define _FPOSOFF(fp) ((long)(fp))
#endif

#endif

#if defined(_M_IX86) // newer Windows versions always have it
_CRTIMP int* __cdecl __p__commode(void);
#endif

/* On newer Windows windows versions, (*__p__commode()) is used */
extern _CRTIMP int _commode;

#define _IOREAD 0x0001
#define _IOWRT 0x0002

#define _IOFBF 0x0000
#define _IOLBF 0x0040
#define _IONBF 0x0004

#define _IOMYBUF 0x0008
#define _IOEOF 0x0010
#define _IOERR 0x0020
#define _IOSTRG 0x0040
#define _IORW 0x0080
#define _USERBUF 0x0100

#define _TWO_DIGIT_EXPONENT 0x1

#ifndef _STDIO_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _filbuf(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _flsbuf(
    _In_ int _Ch,
    _Inout_ FILE *_File);

#ifdef _POSIX_
  _CRTIMP
  FILE*
  __cdecl
  _fsopen(
    const char *_Filename,
    const char *_Mode);
#else
  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _fsopen(
    _In_z_ const char *_Filename,
    _In_z_ const char *_Mode,
    _In_ int _ShFlag);
#endif

  _CRTIMP
  void
  __cdecl
  clearerr(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fclose(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fcloseall(void);

#ifdef _POSIX_
  FILE*
  __cdecl
  fdopen(
    int _FileHandle,
    const char *_Mode);
#else
  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _fdopen(
    _In_ int _FileHandle,
    _In_z_ const char *_Mode);
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  feof(
    _In_ FILE *_File);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  ferror(
    _In_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fflush(
    _Inout_opt_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fgetc(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fgetchar(void);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fgetpos(
    _Inout_ FILE *_File,
    _Out_ fpos_t *_Pos);

  _Check_return_opt_
  _CRTIMP
  char*
  __cdecl
  fgets(
    _Out_writes_z_(_MaxCount) char *_Buf,
    _In_ int _MaxCount,
    _Inout_ FILE *_File);

#ifdef _POSIX_
  int
  __cdecl
  fileno(
    FILE *_File);
#else
  _Check_return_
  _CRTIMP
  int
  __cdecl
  _fileno(
    _In_ FILE *_File);
#endif

  _Check_return_
  _CRTIMP
  char*
  __cdecl
  _tempnam(
    _In_opt_z_ const char *_DirName,
    _In_opt_z_ const char *_FilePrefix);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _flushall(void);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  fopen(
    _In_z_ const char *_Filename,
    _In_z_ const char *_Mode);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fprintf(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fputc(
    _In_ int _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fputchar(
    _In_ int _Ch);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fputs(
    _In_z_ const char *_Str,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  fread(
    _Out_writes_bytes_(_ElementSize * _Count) void *_DstBuf,
    _In_ size_t _ElementSize,
    _In_ size_t _Count,
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(freopen_s)
  FILE*
  __cdecl
  freopen(
    _In_z_ const char *_Filename,
    _In_z_ const char *_Mode,
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(fscanf_s)
  int
  __cdecl
  fscanf(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fsetpos(
    _Inout_ FILE *_File,
    _In_ const fpos_t *_Pos);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fseek(
    _Inout_ FILE *_File,
    _In_ long _Offset,
    _In_ int _Origin);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  ftell(
    _Inout_ FILE *_File);

  _Check_return_opt_
  __MINGW_EXTENSION
  _CRTIMP
  int
  __cdecl
  _fseeki64(
    _Inout_ FILE *_File,
    _In_ __int64 _Offset,
    _In_ int _Origin);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _ftelli64(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  fwrite(
    _In_reads_bytes_(_Size * _Count) const void *_Str,
    _In_ size_t _Size,
    _In_ size_t _Count,
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  getc(
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  getchar(void);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _getmaxstdio(void);

  _CRTIMP
  char*
  __cdecl
  gets(
    char *_Buffer); // FIXME: non-standard

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _getw(
    _Inout_ FILE *_File);

#ifndef _CRT_PERROR_DEFINED
#define _CRT_PERROR_DEFINED
  _CRTIMP
  void
  __cdecl
  perror(
    _In_opt_z_ const char *_ErrMsg);
#endif

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _pclose(
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _popen(
    _In_z_ const char *_Command,
    _In_z_ const char *_Mode);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  printf(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  putc(
    _In_ int _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  putchar(
    _In_ int _Ch);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  puts(
    _In_z_ const char *_Str);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _putw(
    _In_ int _Word,
    _Inout_ FILE *_File);

#ifndef _CRT_DIRECTORY_DEFINED
#define _CRT_DIRECTORY_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  remove(
    _In_z_ const char *_Filename);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  rename(
    _In_z_ const char *_OldFilename,
    _In_z_ const char *_NewFilename);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _unlink(
    _In_z_ const char *_Filename);

#ifndef NO_OLDNAMES
  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(_unlink)
  int
  __cdecl
  unlink(
    _In_z_ const char *_Filename);
#endif

#endif /* _CRT_DIRECTORY_DEFINED */

  _CRTIMP
  void
  __cdecl
  rewind(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _rmtmp(void);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE_CORE(scanf_s)
  int
  __cdecl
  scanf(
    _In_z_ _Scanf_format_string_ const char *_Format,
    ...);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(setvbuf)
  void
  __cdecl
  setbuf(
    _Inout_ FILE *_File,
    _Inout_updates_opt_(BUFSIZ) _Post_readable_size_(0) char *_Buffer);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _setmaxstdio(
    _In_ int _Max);

  _Check_return_opt_
  _CRTIMP
  unsigned int
  __cdecl
  _set_output_format(
    _In_ unsigned int _Format);

  _Check_return_opt_
  _CRTIMP
  unsigned int
  __cdecl
  _get_output_format(void);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  setvbuf(
    _Inout_ FILE *_File,
    _Inout_updates_opt_z_(_Size) char *_Buf,
    _In_ int _Mode,
    _In_ size_t _Size);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scprintf(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE_CORE(sscanf_s)
  int
  __cdecl
  sscanf(
    _In_z_ const char *_Src,
    _In_z_ _Scanf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE_CORE(_snscanf_s)
  int
  __cdecl
  _snscanf(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_format_string_ const char *_Format,
    ...);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(tmpfile_s)
  FILE*
  __cdecl
  tmpfile(void);

  _CRTIMP
  char*
  __cdecl
  tmpnam(
    _Pre_maybenull_ _Post_z_ char *_Buffer);

  _Check_return_opt_
  _CRTIMP_ALT
  int
  __cdecl
  ungetc(
    _In_ int _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  vfprintf(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  vprintf(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  /* Make sure macros are not defined.  */
#if __MINGW_GNUC_PREREQ(4,4)
#pragma push_macro("vsnprintf")
#pragma push_macro("snprintf")
#endif
  #undef vsnprintf
  #undef snprintf

  _Check_return_opt_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(vsnprintf_s)
  int
  __cdecl
  vsnprintf(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _CRTIMP int __cdecl _snprintf(char *_Dest,size_t _Count,const char *_Format,...);
  _CRTIMP int __cdecl _vsnprintf(char *_Dest,size_t _Count,const char *_Format,va_list _Args);
  int __cdecl sprintf(char *_Dest,const char *_Format,...);
  int __cdecl vsprintf(char *_Dest,const char *_Format,va_list _Args);

#ifndef __NO_ISOCEXT  /* externs in libmingwex.a */
  int __cdecl snprintf(char* s, size_t n, const char*  format, ...);
  int __cdecl vscanf(const char * __restrict__ Format, va_list argp);
  int __cdecl vfscanf (FILE * __restrict__ fp, const char * Format,va_list argp);
  int __cdecl vsscanf (const char * __restrict__ _Str,const char * __restrict__ Format,va_list argp);
#endif

/* Restore may prior defined macros snprintf/vsnprintf.  */
#if __MINGW_GNUC_PREREQ(4,4)
#pragma pop_macro("snprintf")
#pragma pop_macro("vsnprintf")
#endif

#ifndef vsnprintf
  #define vsnprintf _vsnprintf
#endif
#ifndef snprintf
  #define snprintf _snprintf
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscprintf(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

#ifdef _SAFECRT_IMPL
#define _set_printf_count_output(i)
#define _get_printf_count_output() (FALSE)
#else
  _CRTIMP int __cdecl _set_printf_count_output(_In_ int _Value);
  _CRTIMP int __cdecl _get_printf_count_output(void);
#endif

#ifndef _WSTDIO_DEFINED

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

#ifdef _POSIX_
  _CRTIMP
  FILE*
  __cdecl
  _wfsopen(
    const wchar_t *_Filename,
    const wchar_t *_Mode);
#else
  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfsopen(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode,
    _In_ int _ShFlag);
#endif

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  fgetwc(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fgetwchar(void);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  fputwc(
    _In_ wchar_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fputwchar(
    _In_ wchar_t _Ch);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  getwc(
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  getwchar(void);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  putwc(
    _In_ wchar_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  putwchar(
    _In_ wchar_t _Ch);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  ungetwc(
    _In_ wint_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wchar_t*
  __cdecl
  fgetws(
    _Out_writes_z_(_SizeInWords) wchar_t *_Dst,
    _In_ int _SizeInWords,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fputws(
    _In_z_ const wchar_t *_Str,
    _Inout_ FILE *_File);

  _CRTIMP
  wchar_t*
  __cdecl
  _getws(
    wchar_t *_String);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _putws(
    _In_z_ const wchar_t *_Str);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fwprintf(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  wprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  vfwprintf(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  vwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

#if defined __cplusplus || defined _CRT_NON_CONFORMING_SWPRINTFS
  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  swprintf(
    _Out_ wchar_t*,
    _Printf_format_string_ const wchar_t*,
    ...);

  _CRTIMP
  int
  __cdecl
  vswprintf(
    _Out_ wchar_t*,
    const wchar_t*,
    va_list);
#endif

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_c(
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_c(
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _CRTIMP int __cdecl _snwprintf(wchar_t *_Dest,size_t _Count,const wchar_t *_Format,...);
  _CRTIMP int __cdecl _vsnwprintf(wchar_t *_Dest,size_t _Count,const wchar_t *_Format,va_list _Args);
#ifndef __NO_ISOCEXT  /* externs in libmingwex.a */
  _CRTIMP int __cdecl snwprintf (wchar_t* s, size_t n, const wchar_t*  format, ...);
  __CRT_INLINE int __cdecl vsnwprintf (wchar_t* s, size_t n, const wchar_t* format, va_list arg)
  {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:28719) /* disable banned api usage warning */
#endif /* _MSC_VER */
      return _vsnwprintf(s,n,format,arg);
#ifdef _MSC_VER
#pragma warning(pop)
#endif /* _MSC_VER */
  }
  _CRTIMP int __cdecl vwscanf (const wchar_t *, va_list);
  _CRTIMP int __cdecl vfwscanf (FILE *,const wchar_t *,va_list);
  _CRTIMP int __cdecl vswscanf (const wchar_t *,const wchar_t *,va_list);
#endif
  _CRTIMP int __cdecl _swprintf(wchar_t *_Dest,const wchar_t *_Format,...);
  _CRTIMP int __cdecl _vswprintf(wchar_t *_Dest,const wchar_t *_Format,va_list _Args);

#ifndef RC_INVOKED
#include <vadefs.h>
#endif

#ifndef _CRT_NON_CONFORMING_SWPRINTFS
  _Check_return_opt_
  static inline
  int
  __cdecl
  swprintf(
      _Out_writes_z_(_SizeInWords) wchar_t* _DstBuf,
      _In_ size_t _SizeInWords,
      _In_z_ _Printf_format_string_ const wchar_t* _Format,
      ...)
  {
      int ret;
      va_list args;

      va_start(args, _Format);
      ret = _vsnwprintf(_DstBuf, _SizeInWords, _Format, args);
      va_end(args);
      return ret;
  }

  _Check_return_opt_
  static inline
  int
  __cdecl
  vswprintf(
      _Out_writes_z_(_SizeInWords) wchar_t* _DstBuf,
      _In_ size_t _SizeInWords,
      _In_z_ _Printf_format_string_ const wchar_t* _Format,
      va_list _ArgList)
  {
      return _vsnwprintf(_DstBuf, _SizeInWords, _Format, _ArgList);
  }
#endif

  _Check_return_
  _CRTIMP
  wchar_t*
  __cdecl
  _wtempnam(
    _In_opt_z_ const wchar_t *_Directory,
    _In_opt_z_ const wchar_t *_FilePrefix);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  fwscanf(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  swscanf(
    _In_z_ const wchar_t *_Src,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwscanf(
    _In_reads_(_MaxCount) _Pre_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  wscanf(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfdopen(
    _In_ int _FileHandle,
    _In_z_ const wchar_t *_Mode);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfopen(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wfreopen(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode,
    _Inout_ FILE *_OldFile);

#ifndef _CRT_WPERROR_DEFINED
#define _CRT_WPERROR_DEFINED
  _CRTIMP
  void
  __cdecl
  _wperror(
    _In_opt_z_ const wchar_t *_ErrMsg);
#endif

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  _wpopen(
    _In_z_ const wchar_t *_Command,
    _In_z_ const wchar_t *_Mode);

#if !defined(NO_OLDNAMES) && !defined(wpopen)
#define wpopen _wpopen
#endif

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _wremove(
    _In_z_ const wchar_t *_Filename);

  _CRTIMP
  wchar_t*
  __cdecl
  _wtmpnam(
    _Pre_maybenull_ _Post_z_ wchar_t *_Buffer);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fgetwc_nolock(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _fputwc_nolock(
    _In_ wchar_t _Ch,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _ungetwc_nolock(
    _In_ wint_t _Ch,
    _Inout_ FILE *_File);

#undef _CRT_GETPUTWCHAR_NOINLINE

#if !defined(__cplusplus) || defined(_CRT_GETPUTWCHAR_NOINLINE)
#define getwchar() fgetwc(stdin)
#define putwchar(_c) fputwc((_c),stdout)
#else
  __CRT_INLINE wint_t __cdecl getwchar() { return (fgetwc(stdin)); }
  __CRT_INLINE wint_t __cdecl putwchar(wchar_t _C) { return (fputwc(_C,stdout)); }
#endif

#define getwc(_stm) fgetwc(_stm)
#define putwc(_c,_stm) fputwc(_c,_stm)
#define _putwc_nolock(_c,_stm) _fputwc_nolock(_c,_stm)
#define _getwc_nolock(_stm) _fgetwc_nolock(_stm)

#define _WSTDIO_DEFINED
#endif

#define _STDIO_DEFINED
#endif

#define _fgetc_nolock(_stream) (--(_stream)->_cnt >= 0 ? 0xff & *(_stream)->_ptr++ : _filbuf(_stream))
#define _fputc_nolock(_c,_stream) (--(_stream)->_cnt >= 0 ? 0xff & (*(_stream)->_ptr++ = (char)(_c)) : _flsbuf((_c),(_stream)))
#define _getc_nolock(_stream) _fgetc_nolock(_stream)
#define _putc_nolock(_c,_stream) _fputc_nolock(_c,_stream)
#define _getchar_nolock() _getc_nolock(stdin)
#define _putchar_nolock(_c) _putc_nolock((_c),stdout)
#define _getwchar_nolock() _getwc_nolock(stdin)
#define _putwchar_nolock(_c) _putwc_nolock((_c),stdout)

  _CRTIMP
  void
  __cdecl
  _lock_file(
    _Inout_ FILE *_File);

  _CRTIMP
  void
  __cdecl
  _unlock_file(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fclose_nolock(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fflush_nolock(
    _Inout_opt_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  _fread_nolock(
    _Out_writes_bytes_(_ElementSize * _Count) void *_DstBuf,
    _In_ size_t _ElementSize,
    _In_ size_t _Count,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fseek_nolock(
    _Inout_ FILE *_File,
    _In_ long _Offset,
    _In_ int _Origin);

  _Check_return_
  _CRTIMP
  long
  __cdecl
  _ftell_nolock(
    _Inout_ FILE *_File);

  _Check_return_opt_
  __MINGW_EXTENSION
  _CRTIMP
  int
  __cdecl
  _fseeki64_nolock(
    _Inout_ FILE *_File,
    _In_ __int64 _Offset,
    _In_ int _Origin);

  __MINGW_EXTENSION
  _Check_return_
  _CRTIMP
  __int64
  __cdecl
  _ftelli64_nolock(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  _fwrite_nolock(
    _In_reads_bytes_(_Size * _Count) const void *_DstBuf,
    _In_ size_t _Size,
    _In_ size_t _Count,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _ungetc_nolock(
    _In_ int _Ch,
    _Inout_ FILE *_File);

#if !defined(NO_OLDNAMES) || !defined(_POSIX)

#define P_tmpdir _P_tmpdir
#define SYS_OPEN _SYS_OPEN

  _CRTIMP
  char*
  __cdecl
  tempnam(
    _In_opt_z_ const char *_Directory,
    _In_opt_z_ const char *_FilePrefix);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fcloseall(void);

  _Check_return_
  _CRTIMP
  FILE*
  __cdecl
  fdopen(
    _In_ int _FileHandle,
    _In_z_ const char *_Format);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fgetchar(void);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  fileno(
    _In_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  flushall(void);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  fputchar(
    _In_ int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  getw(
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  putw(
    _In_ int _Ch,
    _Inout_ FILE *_File);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  rmtmp(void);

#endif /* !defined(NO_OLDNAMES) || !defined(_POSIX) */

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#include <sec_api/stdio_s.h>

#endif
