if(DEEPDEBUG)
    add_definitions(-DDEEPDEBUG)
else()
    message(FATAL_ERROR "msvcrtd.dll needs DEEPDEBUG")
endif

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

add_definitions(
    -DUSE_MSVCRT_PREFIX
    -D_MSVCRT_
    -D_MSVCRT_LIB_
    -D_MT
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -DCRTDLL
    -D__MINGW_IMPORT="")

list(APPEND SOURCE
    ../msvcrt/dllmain.c
    ../msvcrt/stubs.c
    msvcrtd.rc
    ${CMAKE_CURRENT_BINARY_DIR}/../msvcrt/msvcrt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/../msvcrt/msvcrt.def)

add_library(msvcrtd MODULE ${SOURCE})
set_module_type(msvcrtd win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrtd psdk msvcrt)
target_link_libraries(msvcrtd crt cpprt wine ${PSEH_LIB})

add_importlibs(msvcrtd kernel32 ntdll)
add_cd_file(TARGET msvcrtd DESTINATION reactos/system32 FOR all)
