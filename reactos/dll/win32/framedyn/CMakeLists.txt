
set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)
spec2def(framedyn.dll framedyn.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    chstring.cpp
    provider.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/framedyn.def)

if(MSVC)
    add_compile_flags("/Zc:wchar_t-")
endif()

add_library(framedyn SHARED ${SOURCE})
set_module_type(framedyn win32dll UNICODE)
add_importlibs(framedyn oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET framedyn DESTINATION reactos/system32/wbem FOR all)

if(NOT MSVC)
    add_asm_files(framedynex_asm alias_gcc.s)
    add_library(framedynex ${framedynex_asm})
    set_target_properties(framedynex PROPERTIES LINKER_LANGUAGE ASM)
endif()
