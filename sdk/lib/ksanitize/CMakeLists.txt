
list(APPEND SOURCE
    ubsan.c)

if (SANITIZE_UB)
    add_library(ksanitize ${SOURCE})
    target_compile_definitions(ksanitize PRIVATE LIBSAN_KMODE)
    target_link_libraries(ksanitize libcntpr)
    add_importlibs(ksanitize ntoskrnl)
    # we have too many alignment warnings at the moment
    target_compile_options(ksanitize INTERFACE "-fsanitize=undefined;-fno-sanitize=alignment")
    target_compile_definitions(ksanitize INTERFACE __SANITIZE_UB__)

    add_library(ksanitize_w32k ${SOURCE})
    target_compile_definitions(ksanitize_w32k PRIVATE LIBSAN_W32K)
    add_importlibs(ksanitize_w32k win32k)
    target_compile_options(ksanitize_w32k INTERFACE "-fsanitize=undefined;-fno-sanitize=alignment")
    target_compile_definitions(ksanitize_w32k INTERFACE __SANITIZE_UB__)
else()
    add_library(ksanitize INTERFACE)
    add_library(ksanitize_w32k INTERFACE)
endif()

target_include_directories(ksanitize INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(ksanitize_w32k INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
