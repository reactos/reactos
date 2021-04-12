
spec2def(framedyn.dll framedyn.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    chstring.cpp
    provider.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/framedyn.def)

add_library(framedyn MODULE ${SOURCE})
target_link_libraries(framedyn cpprt)
set_target_cpp_properties(framedyn WITH_EXCEPTIONS)
set_module_type(framedyn win32dll UNICODE)
add_importlibs(framedyn oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET framedyn DESTINATION reactos/system32/wbem FOR all)

if((NOT MSVC) AND (ARCH STREQUAL "i386"))
    add_asm_files(framedynex_asm alias_gcc.s)
    add_library(framedynex ${framedynex_asm})
    set_target_properties(framedynex PROPERTIES LINKER_LANGUAGE ASM)
else()
    target_compile_options(framedyn PRIVATE /Zc:wchar_t-)
endif()
