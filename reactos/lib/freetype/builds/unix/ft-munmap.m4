## FreeType specific autoconf tests

# serial 1 FT_MUNMAP_DECL

AC_DEFUN(FT_MUNMAP_DECL,
[AC_MSG_CHECKING([whether munmap must be declared])
AC_CACHE_VAL(ft_cv_munmap_decl,
[AC_TRY_COMPILE([
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/mman.h>],
[char *(*pfn) = (char *(*))munmap],
ft_cv_munmap_decl=no,
ft_cv_munmap_decl=yes)])
AC_MSG_RESULT($ft_cv_munmap_decl)
if test $ft_cv_munmap_decl = yes; then
  AC_DEFINE(NEED_MUNMAP_DECL,,
  [Define to 1 if munmap() is not defined in <sys/mman.h>])
fi])

AC_DEFUN(FT_MUNMAP_PARAM,
[AC_MSG_CHECKING([for munmap's first parameter type])
AC_TRY_COMPILE([
#include <unistd.h>
#include <sys/mman.h>
int munmap(void *, size_t);],,
  AC_MSG_RESULT([void *]);AC_DEFINE(MUNMAP_USES_VOIDP,,
    [Define to 1 if the first argument of munmap is of type void *]),
  AC_MSG_RESULT([char *]))
])
