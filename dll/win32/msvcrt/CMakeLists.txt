
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
spec2def(msvcrt.dll msvcrt.spec ADD_IMPORTLIB)

# Let consumers of msvcrt have the right defines
target_compile_definitions(libmsvcrt INTERFACE _DLL __USE_CRTIMP)
# Embed msvcrtex into libmsvcrt
target_sources(libmsvcrt PRIVATE $<TARGET_OBJECTS:msvcrtex>)

# Embed RTC libs
if (STACK_PROTECTOR)
    target_sources(libmsvcrt PRIVATE $<TARGET_OBJECTS:gcc_ssp_msvcrt>)
    target_link_libraries(libmsvcrt INTERFACE libkernel32) # For OutputDebugStringA
endif()

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
    set_property(TARGET libmsvcrt APPEND PROPERTY STATIC_LIBRARY_OPTIONS "/ignore:4102")
    if(ARCH STREQUAL "i386")
        target_sources(libmsvcrt PRIVATE $<TARGET_OBJECTS:ftol2_sse>)
    endif()
endif()

add_importlibs(msvcrt kernel32 ntdll)
add_cd_file(TARGET msvcrt DESTINATION reactos/system32 FOR all)
