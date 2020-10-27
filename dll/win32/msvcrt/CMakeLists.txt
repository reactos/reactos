
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
spec2def(msvcrt.dll msvcrt.spec)

# The msvcrt <-> msvcrtex trick
generate_import_lib(libmsvcrt_real msvcrt.dll msvcrt.spec)
add_library(libmsvcrt INTERFACE)
target_compile_definitions(libmsvcrt INTERFACE _DLL __USE_CRTIMP)
# if the linked module is one of win32gui;win32cui;win32dll;win32ocx;cpl link it with msvcrtex, which itself is linked to libmsvcrt_real
# Otherwise, just link to libmsvcrt_real
target_link_libraries(libmsvcrt INTERFACE "$<IF:$<IN_LIST:$<TARGET_PROPERTY:REACTOS_MODULE_TYPE>,win32gui;win32cui;win32dll;win32ocx;cpl>,msvcrtex,libmsvcrt_real>")

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

add_library(msvcrt MODULE ${SOURCE})
set_module_type(msvcrt win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt psdk)
target_link_libraries(msvcrt crt wine ${PSEH_LIB})

if(MSVC)
    # export of deleting destructor "name"
    target_link_options(msvcrt PRIVATE "/ignore:4102")
    set_property(TARGET libmsvcrt_real APPEND PROPERTY STATIC_LIBRARY_OPTIONS "/ignore:4102")
endif()

add_importlibs(msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcrt DESTINATION reactos/system32 FOR all)
