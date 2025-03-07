
spec2def(ucrtbase.dll ucrtbase.spec ADD_IMPORTLIB)

add_library(ucrtbase SHARED
    stubs.c
    ucrtbase_stubs.c
    ucrtbase.def
)

if(NOT MSVC)
    target_compile_options(ucrtbase PRIVATE -Wno-builtin-declaration-mismatch)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Silence warnings in ucrtbase_stubs.c
    target_compile_options(ucrtbase PRIVATE -Wno-incompatible-library-redeclaration)
endif()

set_entrypoint(ucrtbase __acrt_DllMain 12)

target_link_libraries(ucrtbase
    ucrt
    ucrtsupport
    vcruntime
    wine
)

# Implicitly link to vcstartup
target_link_libraries(libucrtbase vcstartup)

if(MSVC)
    target_link_libraries(ucrtbase runtmchk)
else()
    # For __cxa_guard_acquire / __cxa_guard_release
    target_link_libraries(ucrtbase libsupc++)
endif()

add_importlibs(ucrtbase kernel32 ntdll)

add_cd_file(TARGET ucrtbase DESTINATION reactos/system32 FOR all)
