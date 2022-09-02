
list(APPEND SOURCE
    ubsan.c
    kasan.c)

add_library(ksanitize ${SOURCE})
target_compile_definitions(ksanitize PUBLIC LIBSAN_KMODE)
target_compile_options(ksanitize PUBLIC -fsanitize=address)
add_importlibs(ksanitize ntoskrnl)

add_library(ksanitize_w32k ${SOURCE})
target_compile_definitions(ksanitize_w32k PUBLIC LIBSAN_W32K)
add_importlibs(ksanitize_w32k win32k)
