
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)

list(APPEND SOURCE
    main.c
    chstring.cpp
    provider.cpp)

if(MSVC)
    add_compile_flags("/Zc:wchar_t-")
endif()

add_library(framedyn MODULE ${SOURCE})
spec2def(framedyn framedyn.dll framedyn.spec ADD_IMPORTLIB)

set_module_type(framedyn win32dll UNICODE)
add_importlibs(framedyn oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET framedyn DESTINATION reactos/system32/wbem FOR all)

if(NOT MSVC)
    add_asm_files(framedynex_asm alias_gcc.s)
    add_library(framedynex ${framedynex_asm})
    set_target_properties(framedynex PROPERTIES LINKER_LANGUAGE ASM)
endif()
