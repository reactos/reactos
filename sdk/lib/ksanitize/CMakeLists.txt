
list(APPEND SOURCE
    ubsan.c)

add_library(ksanitize ${SOURCE})
target_compile_definitions(ksanitize PUBLIC LIBSAN_KMODE)
target_link_libraries(ksanitize libcntpr)
add_importlibs(ksanitize ntoskrnl)

add_library(ksanitize_w32k ${SOURCE})
target_compile_definitions(ksanitize_w32k PUBLIC LIBSAN_W32K)
add_importlibs(ksanitize_w32k win32k ntoskrnl)

if (SANITIZE_UB)
    # we have too many alignment warnings at the moment
    target_compile_options(ksanitize INTERFACE "-fsanitize=undefined;-fno-sanitize=alignment")
    target_compile_definitions(ksanitize INTERFACE __SANITIZE_UB__)

    target_compile_options(ksanitize_w32k INTERFACE "-fsanitize=undefined;-fno-sanitize=alignment")
    target_compile_definitions(ksanitize_w32k INTERFACE __SANITIZE_UB__)
endif()
