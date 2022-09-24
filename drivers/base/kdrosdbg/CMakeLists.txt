
spec2def(kdcom.dll kdrosdbg.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    kdrosdbg.c
    kdrosdbg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

add_library(kdcom MODULE ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    if(SANITIZE_UB OR KASAN_ENABLED)
        target_link_libraries(kdcom ksanitize)
    endif()

    if(KASAN_ENABLED)
        target_compile_options(kdcom PRIVATE "-fsanitize=address")
    endif()
endif()

set_module_type(kdcom kerneldll ENTRYPOINT 0)
add_importlibs(kdcom ntoskrnl hal)
target_link_libraries(kdcom cportlib)
add_dependencies(kdcom psdk)
add_cd_file(TARGET kdcom DESTINATION reactos/system32 NO_CAB FOR all)
