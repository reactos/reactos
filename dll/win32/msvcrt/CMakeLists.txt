
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
spec2def(msvcrt.dll msvcrt.spec ADD_IMPORTLIB)

# Let consumers of msvcrt have the right defines
target_compile_definitions(libmsvcrt INTERFACE _DLL __USE_CRTIMP)
# Embed msvcrtex into libmsvcrt
target_sources(libmsvcrt PRIVATE $<TARGET_OBJECTS:msvcrtex>)

# FIXME: For amd64, combine CRT startup objects with import library
# This ensures CRT startup symbols are available when linking
if(ARCH STREQUAL "amd64")
    add_dependencies(libmsvcrt crt_startup)
    add_custom_command(TARGET libmsvcrt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_tmp
        COMMAND ${CMAKE_AR} x $<TARGET_FILE:crt_startup> --output=${CMAKE_CURRENT_BINARY_DIR}/msvcrt_tmp
        COMMAND ${CMAKE_AR} x $<TARGET_FILE:libmsvcrt> --output=${CMAKE_CURRENT_BINARY_DIR}/msvcrt_tmp 2>/dev/null || true
        COMMAND ${CMAKE_AR} rcs ${CMAKE_CURRENT_BINARY_DIR}/libmsvcrt_combined.a ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_tmp/*.o*
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/libmsvcrt_combined.a $<TARGET_FILE:libmsvcrt>
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_tmp
        COMMENT "FIXME: Combining CRT startup with msvcrt import library (amd64)"
    )
endif()

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
    mbsrtowcs.c
    emutls.c
    strnlen.c
    wctob.c
    wctype.c
    msvcrt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt.def)

add_library(msvcrt MODULE ${SOURCE})
set_module_type(msvcrt win32dll ENTRYPOINT DllMain 12)
add_dependencies(msvcrt psdk)
target_link_libraries(msvcrt crt cpprt wine ${PSEH_LIB})

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
