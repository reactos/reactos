
include_directories(${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)
spec2def(msvcrt.dll msvcrt.spec ADD_IMPORTLIB)

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
    dllmain.c
    stubs.c
    msvcrt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt.def)

add_library(msvcrt SHARED ${SOURCE})
set_module_type(msvcrt win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt psdk)
target_link_libraries(msvcrt crt wine ${PSEH_LIB})

if(MSVC)
    # export of deleting destructor "name"
    add_target_link_flags(msvcrt "/ignore:4102")
    add_target_property(libmsvcrt STATIC_LIBRARY_FLAGS "/ignore:4102")
endif()

add_importlibs(msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcrt DESTINATION reactos/system32 FOR all)
